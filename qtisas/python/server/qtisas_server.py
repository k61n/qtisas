# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: script server — runs requests sent by qtisas_send.py in this QtiSAS
# **************************************************************************** #
"""Script server: runs Python sent over a local socket in the running QtiSAS.

Started by QtiSAS's -s[=NAME] / -S[=NAME] options (--server, --server-offscreen), or by start() when the
environment variable QTISAS_SERVER names the instance (e.g. "test"); without either nothing listens. The socket is
a Unix socket only the user can open (QLocalServer.UserAccessOption), by default /tmp/qtisas-<uid>/<instance>.sock,
or QTISAS_SERVER_SOCKET; a socket folder other users own or can use is refused. A name another QtiSAS still answers
on is refused (RuntimeError), so a request always reaches the QtiSAS that started it; only a leftover socket file is
replaced.

Protocol: one JSON object per line, both ways. A request
    {"id": 1, "mode": "python", "code": "...", "keep": false, "dialogs": "auto", "filename": "script.py"}
is answered by zero or more events {"id": 1, "event": "line", "text": "..."} and one final
    {"id": 1, "event": "result", "ok": true, "output": "...", "error": "", "seconds": 0.12,
     "created": ["table:T"], "removed": [], "dialogs": [], "status": "done"}
or {"id": 1, "event": "busy"} while another request runs. Modes: "python", "status", "quit", and the internal AI:
"ai" (generates only, nothing runs) and "ai-run" (the full pipeline with execution), with "text" instead of "code";
their lines are streamed as "line" events, and the result adds "transcript", "items" (one per LIST item), "r_lines",
"scripts", "tokens".
"""

import builtins
import contextlib
import io
import json
import linecache
import os
import re
import socket
import time
import traceback

try:
    from PyQt6.QtCore import QEventLoop, QMetaObject, Qt, QTimer
    from PyQt6.QtNetwork import QLocalServer
    from PyQt6.QtWidgets import QApplication, QMessageBox
except ImportError:
    from PyQt5.QtCore import QEventLoop, QMetaObject, Qt, QTimer
    from PyQt5.QtNetwork import QLocalServer
    from PyQt5.QtWidgets import QApplication, QMessageBox

import qti
import qtisas_send  # the same folder: the socket path and its privacy check, shared with the client

PROTOCOL = 1
AI_IDLE_SECONDS = 1.5  # see AiAssistant::isIdle(): between two steps of one request it is idle for an instant
DIRECT = getattr(Qt, "ConnectionType", Qt).DirectConnection
USER_ACCESS = getattr(QLocalServer, "SocketOption", QLocalServer).UserAccessOption

_server = None  # the running server, kept alive here


def default_socket_path(instance):
    return qtisas_send.default_socket_path(instance)


def start(instance=None, path=None, dialogs=None):
    """Starts the server for `instance` (default: $QTISAS_SERVER); returns it, or None when no instance is named.

    dialogs: "auto" closes the dialogs a request opens, "leave" leaves them to the user (default; -S passes "auto");
    $QTISAS_SERVER_DIALOGS, when set, wins. The socket: `path`, else $QTISAS_SERVER_SOCKET, else the default.
    """
    global _server
    instance = instance or os.environ.get("QTISAS_SERVER", "")
    if not instance:
        return None
    if _server is None:
        path = path or os.environ.get("QTISAS_SERVER_SOCKET") or default_socket_path(instance)
        _server = ScriptServer(instance, path, os.environ.get("QTISAS_SERVER_DIALOGS") or dialogs or "leave")
    return _server


def snapshot():
    """The objects a request can create or remove: windows by kind, and the fit functions of the "ai/" folder."""
    objects = set()
    for w in qti.app.windows():
        objects.add("%s:%s" % (_window_kind(w), w.objectName()))
    folder = os.path.join(str(qti.app.Compiler.path), "ai")
    if os.path.isdir(folder):
        for f in os.listdir(folder):
            if f.endswith(".fif"):
                objects.add("function:ai/" + f[:-4])
    return objects


def _live_server(path):
    """None if nothing listens at `path`; else the pid of the server there, or "" if it does not tell in time."""
    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
            s.settimeout(1)
            s.connect(path)
            s.sendall(b'{"id": 0, "mode": "status"}\n')
            reply = b""
            while b"\n" not in reply:
                chunk = s.recv(4096)
                if not chunk:
                    break
                reply += chunk
    except (FileNotFoundError, ConnectionRefusedError):
        return None
    except OSError:  # listening, but busy or not answering in time: still in use
        return ""
    try:
        return str(json.loads(reply.split(b"\n")[0].decode("utf-8")).get("pid", ""))
    except ValueError:
        return ""


_LIST_ITEM_RE = re.compile(r"AI LIST: item (\d+)/(\d+)")
_TOOK_RE = re.compile(r"\(took (?:(\d+)min )?([\d.]+)sec")
_ALL_DONE_RE = re.compile(r"AI LIST: all (\d+) item\(s\) completed \(took (?:(\d+)min )?([\d.]+)sec")
_TOKENS_RE = re.compile(r"~(\d+)K tokens estimated")
_R_LINE_RE = re.compile(r"^R\d+:.*$", re.MULTILINE)


def _seconds(match, first_group):
    minutes = match.group(first_group)
    return (int(minutes) * 60 if minutes else 0) + float(match.group(first_group + 1))


def summarize_ai(lines):
    """The structured part of an AI run, read from what it printed: status, R<n>: lines, scripts run, tokens sent,
    and one entry per LIST item (its text, scripts, R lines, seconds, failed)."""
    transcript = "\n".join(lines)
    # R<n>: lines come from the scripts' output, never from the AI's own messages ("▸▸▸ AI code ..." shows the code)
    output = "\n".join(line for line in lines if not line.startswith("▸"))
    items = []
    for line in lines:
        m = _LIST_ITEM_RE.search(line)
        if m and "failed" not in line:  # "LIST item n/N failed" belongs to the item before
            took = _TOOK_RE.search(line)
            if took and items:  # "item n/N (took ...)" is the time of the PREVIOUS item
                items[-1]["seconds"] = _seconds(took, 1)
            items.append({"item": int(m.group(1)), "of": int(m.group(2)), "text": line.split(" — ", 1)[-1].strip(),
                          "scripts": 0, "r_lines": [], "seconds": None, "failed": False, "backend_error": False})
            continue
        done = _ALL_DONE_RE.search(line)
        if done and items:
            items[-1]["seconds"] = _seconds(_TOOK_RE.search(line), 1)
        if items:
            if "AI code (running)" in line:
                items[-1]["scripts"] += 1
            if line.startswith("▸▸▸ AI: connection error"):  # the model's backend, not the model's answer
                items[-1]["backend_error"] = True
            if re.search(r"LIST item %d/\d+ failed" % items[-1]["item"], line):
                items[-1]["failed"] = True
            if not line.startswith("▸"):
                items[-1]["r_lines"] += _R_LINE_RE.findall(line)
    stopped = any(line.startswith("▸▸▸ AI: stopping") for line in lines) and not items
    backend_errors = sum(line.startswith("▸▸▸ AI: connection error") for line in lines)
    failed_items = [i["item"] for i in items if i["failed"]]
    status = "stopped" if stopped else ("items-failed" if failed_items else "done")
    return {"ok": status == "done", "status": status, "transcript": transcript,
            "r_lines": _R_LINE_RE.findall(output), "items": items, "failed_items": failed_items,
            "scripts": transcript.count("AI code (running)"), "backend_errors": backend_errors,
            "tokens": 1000 * sum(int(t) for t in _TOKENS_RE.findall(transcript))}


def _window_kind(w):
    for kind in ("Table", "Matrix", "Note", "MultiLayer", "Graph3D"):
        if w.inherits(kind):
            return {"MultiLayer": "graph", "Graph3D": "graph3d"}.get(kind, kind.lower())
    return "window"


class _DialogAnswerer:
    """While a request runs: closes each modal dialog that opens (reject, i.e. Cancel) and records its title/text."""

    def __init__(self):
        self.dialogs = []
        self._seen = []
        self._timer = QTimer()
        self._timer.timeout.connect(self._check)

    def __enter__(self):
        self._timer.start(100)
        return self

    def __exit__(self, *exc):
        self._timer.stop()
        return False

    def _check(self):
        dialog = QApplication.activeModalWidget()
        if dialog is None or any(dialog is d for d in self._seen):
            return
        self._seen.append(dialog)
        text = dialog.text() if isinstance(dialog, QMessageBox) else ""
        self.dialogs.append({"title": dialog.windowTitle(), "text": text})
        QMetaObject.invokeMethod(dialog, "reject", DIRECT)


class ScriptServer:
    def __init__(self, instance, path, dialogs):
        self.instance = instance
        self.path = path
        self.dialogs = dialogs  # default for requests without their own "dialogs"
        self.busy = False
        self.session = None  # the namespace kept between requests with "keep"
        self._buffers = {}
        folder = os.path.dirname(path) or "."
        if not os.path.exists(folder):
            os.makedirs(folder)
            os.chmod(folder, 0o700)  # makedirs' mode is reduced by the umask, never widened: set it explicitly
        problem = qtisas_send.private_folder_problem(folder)
        if problem:  # in shared /tmp: another user's folder could hold their socket instead of ours
            raise RuntimeError("qtisas_server: refusing %s: %s" % (path, problem))
        owner = _live_server(path)
        if owner is not None:  # never take the name over from a QtiSAS that still answers on it
            raise RuntimeError("qtisas_server: instance %r is already used by %s (%s)"
                               % (instance, "QtiSAS pid %s" % owner if owner else "another QtiSAS", path))
        QLocalServer.removeServer(path)  # a leftover socket file of a QtiSAS that no longer runs
        self.server = QLocalServer()
        self.server.setSocketOptions(USER_ACCESS)
        self.server.newConnection.connect(self._connected)
        if not self.server.listen(path):
            raise RuntimeError("qtisas_server: cannot listen on %s: %s" % (path, self.server.errorString()))

    def _connected(self):
        while self.server.hasPendingConnections():
            socket = self.server.nextPendingConnection()
            self._buffers[socket] = b""
            socket.readyRead.connect(lambda s=socket: self._read(s))
            socket.disconnected.connect(lambda s=socket: self._forget(s))

    def _forget(self, socket):
        self._buffers.pop(socket, None)
        socket.deleteLater()

    def _read(self, socket):
        if socket not in self._buffers:  # already disconnected
            return
        self._buffers[socket] += bytes(socket.readAll())
        # A request can run a nested event loop (a dialog) in which this socket disconnects or sends more: the
        # buffer is stored before each request and read again after it.
        while socket in self._buffers and b"\n" in self._buffers[socket]:
            line, self._buffers[socket] = self._buffers[socket].split(b"\n", 1)
            if line.strip():
                self._handle(socket, line)

    def _send(self, socket, message):
        try:
            socket.write((json.dumps(message) + "\n").encode("utf-8"))
            socket.flush()
        except RuntimeError:  # the client left and its socket is deleted: nobody to answer
            pass

    def _handle(self, socket, line):
        try:
            request = json.loads(line.decode("utf-8"))
        except ValueError as e:
            self._send(socket, {"id": None, "event": "result", "ok": False, "error": "bad request: %s" % e})
            return
        rid = request.get("id")
        if self.busy:  # e.g. a second request while a dialog of the first one runs its own event loop
            self._send(socket, {"id": rid, "event": "busy"})
            return
        self.busy = True
        try:
            result = self.run(request, lambda text: self._send(socket, {"id": rid, "event": "line", "text": text}))
        except Exception:  # a failure of the server itself: still answered, never a client left waiting
            result = {"ok": False, "status": "failed", "error": "qtisas_server: " + traceback.format_exc().rstrip()}
        finally:
            self.busy = False
        result.update({"id": rid, "event": "result"})
        self._send(socket, result)
        if request.get("mode") == "quit":
            QTimer.singleShot(200, self._quit)

    def _quit(self):
        self.server.close()  # removes the socket file
        os._exit(0)  # no "save changes?" question, no settings written

    def run(self, request, emit=None):
        mode = request.get("mode", "python")
        if mode == "status":
            return {"ok": True, "status": "done", "protocol": PROTOCOL, "instance": self.instance,
                    "pid": os.getpid(), "home": os.path.expanduser("~")}
        if mode == "quit":
            return {"ok": True, "status": "done"}
        if mode == "python":
            return self.run_python(request)
        if mode in ("ai", "ai-run"):
            return self.run_ai(request, emit)
        return {"ok": False, "status": "failed", "error": "unknown mode %r" % mode}

    def run_ai(self, request, emit=None):
        """Asks the internal AI (generate only, or with execution) and waits until it is idle; see summarize_ai()."""
        app = qti.app
        text = request.get("text") or request.get("code") or ""
        if not text.strip():
            return {"ok": False, "status": "failed", "error": "no request text"}
        if not app._aiIdle():
            return {"ok": False, "status": "failed", "error": "the AI is busy with another request"}
        timeout = float(request.get("timeout") or 3600)
        first = app._aiTranscriptSize()
        lines = []
        state = {"idle_since": None, "timeout": False}
        start = time.monotonic()
        loop = QEventLoop()

        def collect():
            for line in app._aiTranscript(first + len(lines)):
                lines.append(line)
                if emit:
                    emit(line)

        def poll():
            collect()
            now = time.monotonic()
            if not app._aiIdle():
                state["idle_since"] = None
            elif state["idle_since"] is None:
                state["idle_since"] = now
            elif now - state["idle_since"] >= AI_IDLE_SECONDS:
                loop.quit()
            if now - start > timeout:
                state["timeout"] = True
                app._aiStop()
                loop.quit()

        before = snapshot()
        answerer = _DialogAnswerer()
        timer = QTimer()
        timer.timeout.connect(poll)
        with answerer if request.get("dialogs", self.dialogs) == "auto" else contextlib.nullcontext():
            app._askAi(text, request.get("mode") == "ai-run")
            timer.start(100)
            loop.exec()
            timer.stop()
        timer.timeout.disconnect(poll)
        collect()
        result = summarize_ai(lines)
        if state["timeout"]:
            result.update({"ok": False, "status": "timeout"})
        after = snapshot()
        result.update({"seconds": round(time.monotonic() - start, 3), "created": sorted(after - before),
                       "removed": sorted(before - after), "dialogs": answerer.dialogs})
        return result

    def namespace(self, keep):
        if keep:
            if self.session is None:
                self.session = self._fresh_namespace()
            return self.session
        return self._fresh_namespace()

    @staticmethod
    def _fresh_namespace():
        import __main__  # where qtisasrc.py ran: qti, Table, Graph, ... as in the console
        ns = {k: v for k, v in __main__.__dict__.items() if not k.startswith("__")}
        ns["__name__"] = "__main__"
        ns["__builtins__"] = builtins
        return ns

    def run_python(self, request):
        code = request.get("code", "")
        filename = request.get("filename") or "<qtisas-send>"
        ns = self.namespace(bool(request.get("keep")))
        ns["__file__"] = filename
        lines = code.splitlines(True)
        linecache.cache[filename] = (len(code), None, lines, filename)  # tracebacks show the sent code's lines
        output = io.StringIO()
        ns["scriptPrint"] = lambda text="": output.write(str(text) + "\n")  # captured, not sent to the console
        before = snapshot()
        error = ""
        start = time.monotonic()
        answer = request.get("dialogs", self.dialogs) == "auto"
        answerer = _DialogAnswerer()
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            with answerer if answer else contextlib.nullcontext():
                try:
                    exec(compile(code, filename, "exec"), ns)
                except SystemExit as e:
                    if e.code not in (None, 0):
                        error = "SystemExit: %s" % e.code
                except BaseException as e:  # the script's error, with its own frames only
                    error = self._format_error(e, filename)
        seconds = time.monotonic() - start
        after = snapshot()
        return {"ok": not error, "status": "done" if not error else "failed", "output": output.getvalue(),
                "error": error, "seconds": round(seconds, 3), "created": sorted(after - before),
                "removed": sorted(before - after), "dialogs": answerer.dialogs}

    @staticmethod
    def _format_error(e, filename):
        tb = e.__traceback__
        while tb is not None and tb.tb_frame.f_code.co_filename == __file__:
            tb = tb.tb_next  # drop this server's own frames
        return "".join(traceback.format_exception(type(e), e, tb)).rstrip()
