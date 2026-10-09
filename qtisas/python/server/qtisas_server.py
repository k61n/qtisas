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
("interactive": true runs the code as one prompt input: a bare expression prints its value, as in Python's prompt)
is answered by zero or more events {"id": 1, "event": "line", "text": "..."} and one final
    {"id": 1, "event": "result", "ok": true, "output": "...", "error": "", "seconds": 0.12,
     "created": ["table:T"], "removed": [], "dialogs": [], "status": "done"}
or {"id": 1, "event": "busy"} while another request runs. While an AI request runs, "python" and "status" requests
still run (between the AI's steps), "stop" cancels it (its result then has status "stopped"; a running Python script
cannot be interrupted), "note" ({"mode": "note", "text": "..."}) adds a correction to it, from its next model
call on or at once with a leading "!", and "follow" ({"mode": "follow", "from": N}) streams its lines from line N on
and then its result, e.g. to a client that stopped waiting. Modes: "python", "status", "quit", "stop", "note",
"follow", and the AI: "ai" (generates only, nothing runs) and "ai-run" (the full pipeline with execution), with
"text" instead of "code"; their lines are streamed as "line" events, and the result adds "transcript", "items" (one
per LIST item), "r_lines", "scripts", "tokens".
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
        self.busy = None  # the mode of the request being handled
        self.inner_busy = False  # a "python"/"status" request handled while an AI request runs
        self.ai_run = None  # the running AI request: its lines so far and the clients following it
        self.stop_requested = False  # a "stop" request arrived while an AI request was running
        self.session = None  # the namespace kept between requests with "keep"
        self._buffers = {}
        self._reading = set()  # sockets whose readyRead is being handled right now (a request may run inside it)
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
        # Not while its own readyRead is still being handled: an AI request waits in a nested event loop inside it, and
        # a client that left meanwhile (Ctrl+C, then "stop") had its socket deleted in there — confirmed live, a
        # SIGSEGV in doActivate() once that emission went on. _read() deletes it when it is done.
        if socket not in self._reading:
            socket.deleteLater()

    def _read(self, socket):
        if socket not in self._buffers:  # already disconnected
            return
        self._reading.add(socket)
        try:
            self._buffers[socket] += bytes(socket.readAll())
            # A request can run a nested event loop (a dialog, the AI) in which this socket disconnects or sends more:
            # the buffer is stored before each request and read again after it.
            while socket in self._buffers and b"\n" in self._buffers[socket]:
                line, self._buffers[socket] = self._buffers[socket].split(b"\n", 1)
                if line.strip():
                    self._handle(socket, line)
        finally:
            self._reading.discard(socket)
            if socket not in self._buffers:  # it left while being handled: deleted now that nothing uses it
                socket.deleteLater()

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
        mode = request.get("mode", "python")
        if mode in ("stop", "note"):  # get through while another request runs
            result = self.stop_ai() if mode == "stop" else self.note_ai(request.get("text") or "")
            result.update({"id": rid, "event": "result"})
            self._send(socket, result)
            return
        if mode == "follow":
            self._follow(socket, rid, int(request.get("from") or 0))
            return
        # While the AI works, the event loop is mostly free (it waits for the model): Python and status get through.
        # A script of the AI's own that is running at that moment simply finishes first (one thread).
        inside_ai = self.busy in ("ai", "ai-run") and mode in ("python", "status") and not self.inner_busy
        if self.busy and not inside_ai:  # e.g. a second request while a dialog of the first one runs its own loop
            self._send(socket, {"id": rid, "event": "busy"})
            return
        if inside_ai:
            self.inner_busy = True
        else:
            self.busy = mode
        try:
            result = self.run(request, lambda text: self._send(socket, {"id": rid, "event": "line", "text": text}))
        except Exception:  # a failure of the server itself: still answered, never a client left waiting
            result = {"ok": False, "status": "failed", "error": "qtisas_server: " + traceback.format_exc().rstrip()}
        finally:
            if inside_ai:
                self.inner_busy = False
            else:
                self.busy = None
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
                    "pid": os.getpid(), "home": os.path.expanduser("~"),
                    "running": None if self.busy == "status" else self.busy,  # not this status request itself
                    "ai_lines": len(self.ai_run["lines"]) if self.ai_run else 0}
        if mode == "quit":
            return {"ok": True, "status": "done"}
        if mode == "python":
            return self.run_python(request)
        if mode in ("ai", "ai-run"):
            return self.run_ai(request, emit)
        return {"ok": False, "status": "failed", "error": "unknown mode %r" % mode}

    def stop_ai(self):
        """Cancels the running AI request the way QtiSAS's own "stop" command does: the model call and every pending
        round trip, a LIST's remaining items and a split plan's remaining parts. A Python script that is running
        cannot be interrupted; an AI script ends at its own time limit."""
        app = qti.app
        if app._aiIdle():
            return {"ok": True, "status": "done", "stopped": False,
                    "output": "nothing to stop: the AI is idle" + (" (a Python script is running)" if
                                                                      self.busy == "python" else "") + "\n"}
        self.stop_requested = True
        app._askAi("stop", False)  # handled before the AI's own busy check, see handleAiTextCommand()
        return {"ok": True, "status": "done", "stopped": True, "output": "stop sent: the AI request is cancelled\n"}

    def note_ai(self, text):
        """A correction to the running AI request, as typed into QtiSAS's input line while it runs: from its next
        model call on, or at once with a leading "!" (the call in flight starts again with it)."""
        app = qti.app
        if not text.strip():
            return {"ok": False, "status": "failed", "error": "no correction text"}
        if app._aiIdle():
            return {"ok": True, "status": "done", "noted": False,
                    "output": "nothing to correct: the AI is idle — send it as a new request\n"}
        app._askAi(text, False)  # while the AI works, askAi() takes the text as a correction
        return {"ok": True, "status": "done", "noted": True,
                "output": "correction sent" + (": the current model call starts again with it\n"
                                               if text.strip().startswith("!") else
                                               ", applied from the next step of the request\n")}

    def _follow(self, socket, rid, start):
        """Streams the running AI request's lines from `start` on, then its result (sent by run_ai())."""
        run = self.ai_run
        if run is None:
            self._send(socket, {"id": rid, "event": "result", "ok": True, "status": "done",
                                "output": "nothing to follow: no AI request is running\n"})
            return
        for text in run["lines"][start:]:
            self._send(socket, {"id": rid, "event": "line", "text": text})
        run["followers"].append((socket, rid))

    def run_ai(self, request, emit=None):
        """Asks the internal AI (generate only, or with execution) and waits until it is idle; see summarize_ai()."""
        app = qti.app
        text = request.get("text") or request.get("code") or ""
        if not text.strip():
            return {"ok": False, "status": "failed", "error": "no request text"}
        if not app._aiIdle():
            return {"ok": False, "status": "failed", "error": "the AI is busy with another request"}
        timeout = float(request.get("timeout") or 3600)
        self.stop_requested = False
        first = app._aiTranscriptSize()
        lines = []
        self.ai_run = run = {"lines": lines, "followers": []}
        state = {"idle_since": None, "timeout": False}
        start = time.monotonic()
        loop = QEventLoop()

        def collect():
            for line in app._aiTranscript(first + len(lines)):
                lines.append(line)
                if emit:
                    emit(line)
                for socket, frid in run["followers"]:
                    self._send(socket, {"id": frid, "event": "line", "text": line})

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
        elif self.stop_requested:
            result.update({"ok": False, "status": "stopped"})
            self.stop_requested = False
        after = snapshot()
        result.update({"seconds": round(time.monotonic() - start, 3), "created": sorted(after - before),
                       "removed": sorted(before - after), "dialogs": answerer.dialogs})
        self.ai_run = None
        for socket, frid in run["followers"]:
            self._send(socket, dict(result, id=frid, event="result"))
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
        def script_print(*args, sep=" "):  # captured, not sent to the console; like qtisasrc.py's scriptPrint()
            output.write(sep.join(str(a) for a in args if not (len(args) == 1 and a is None)) + "\n")

        ns["scriptPrint"] = script_print
        before = snapshot()
        error = ""
        start = time.monotonic()
        answer = request.get("dialogs", self.dialogs) == "auto"
        answerer = _DialogAnswerer()
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            with answerer if answer else contextlib.nullcontext():
                try:
                    exec(self._compile(code, filename, bool(request.get("interactive"))), ns)
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
    def _compile(code, filename, interactive):
        """Python's "single" mode for a prompt input (a bare expression's value is printed), else a whole script."""
        if interactive:
            try:
                return compile(code + "\n", filename, "single")
            except SyntaxError:
                pass  # several statements: run them as a script, the real error (if any) comes from there
        return compile(code, filename, "exec")

    @staticmethod
    def _format_error(e, filename):
        tb = e.__traceback__
        while tb is not None and tb.tb_frame.f_code.co_filename == __file__:
            tb = tb.tb_next  # drop this server's own frames
        hint = ScriptServer._error_hint(e)
        return "".join(traceback.format_exception(type(e), e, tb)).rstrip() + ("\n" + hint if hint else "")

    @staticmethod
    def _error_hint(e):
        """The hints QtiSAS's own error messages carry (PythonScripting.cpp: methodOwnerHint()/attributeHint())."""
        name = getattr(e, "name", None)
        if isinstance(e, NameError) and name and not name.startswith("_"):
            owners = [(kind, call) for kind, call, obj in (
                ("Table", "t", getattr(qti, "Table", None)), ("Matrix", "m", getattr(qti, "Matrix", None)),
                ("Note", "n", getattr(qti, "Note", None)), ("GraphWindow", "g", getattr(qti, "GraphWindow", None)),
                ("Graph", "layer", getattr(qti, "Graph", None)), ("Graph3D", "g3", getattr(qti, "Graph3D", None)),
                ("Compiler", "Compiler", getattr(builtins, "Compiler", None)),
                ("Fittable", "Fittable", getattr(builtins, "Fittable", None)),
                ("Dan", "Dan", getattr(builtins, "Dan", None))) if obj is not None and hasattr(obj, name)]
            if owners:
                return ("'%s' is not a global function but a method of %s — call it on that object, e.g. %s.%s(...)"
                        % (name, ", ".join(kind for kind, _ in owners), owners[0][1], name))
        if isinstance(e, AttributeError) and name and hasattr(e, "obj"):
            if e.obj is None:
                return ("The object is None: the call that should have returned it found nothing — e.g. "
                        "table(\"…\"), matrix(\"…\") or n.tab(\"…\") for a name that does not exist.")
            if len(name) >= 3:
                similar = sorted((c for c in dir(e.obj) if not c.startswith("_") and c != name and
                                  ScriptServer._has_word(c, name)), key=len)[:6]
                if similar:
                    return "'%s' has no '%s'; its methods with a similar name: %s()" % (
                        type(e.obj).__name__, name, "(), ".join(similar))
        return ""

    @staticmethod
    def _has_word(candidate, name):
        """`name` as a word of the camelCase `candidate`: "text" is in setText and toPlainText, not in contextMenu."""
        at = candidate.lower().find(name.lower())
        while at >= 0:
            if at == 0 or candidate[at].isupper():
                return True
            at = candidate.lower().find(name.lower(), at + 1)
        return False
