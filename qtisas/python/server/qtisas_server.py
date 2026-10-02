# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: script server — runs requests sent by qtisas_send.py in this QtiSAS
# **************************************************************************** #
"""Script server: runs Python sent over a local socket in the running QtiSAS.

Started by QtiSAS's -s[=NAME] / -S[=NAME] options (--server, --server-offscreen), or by start() when the
environment variable QTISAS_SERVER names the instance (e.g. "test"); without either nothing listens. The socket is a Unix socket only the user can open (QLocalServer.UserAccessOption), by default
/tmp/qtisas-<uid>/<instance>.sock, or QTISAS_SERVER_SOCKET. A name another QtiSAS still answers on is refused
(RuntimeError), so a request always reaches the QtiSAS that started it; only a leftover socket file is replaced.

Protocol: one JSON object per line, both ways. A request
    {"id": 1, "mode": "python", "code": "...", "keep": false, "dialogs": "auto", "filename": "script.py"}
is answered by zero or more events {"id": 1, "event": "line", "text": "..."} and one final
    {"id": 1, "event": "result", "ok": true, "output": "...", "error": "", "seconds": 0.12,
     "created": ["table:T"], "removed": [], "dialogs": [], "status": "done"}
or {"id": 1, "event": "busy"} while another request runs. Modes: "python", "status", "quit".
"""

import builtins
import contextlib
import io
import json
import linecache
import os
import socket
import time
import traceback

try:
    from PyQt6.QtCore import QMetaObject, Qt, QTimer
    from PyQt6.QtNetwork import QLocalServer
    from PyQt6.QtWidgets import QApplication, QMessageBox
except ImportError:
    from PyQt5.QtCore import QMetaObject, Qt, QTimer
    from PyQt5.QtNetwork import QLocalServer
    from PyQt5.QtWidgets import QApplication, QMessageBox

import qti

PROTOCOL = 1
DIRECT = getattr(Qt, "ConnectionType", Qt).DirectConnection
USER_ACCESS = getattr(QLocalServer, "SocketOption", QLocalServer).UserAccessOption

_server = None  # the running server, kept alive here


def default_socket_path(instance):
    return os.path.join("/tmp", "qtisas-%d" % os.getuid(), instance + ".sock")


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
        os.makedirs(os.path.dirname(path), mode=0o700, exist_ok=True)
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
            result = self.run(request)
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

    def run(self, request):
        mode = request.get("mode", "python")
        if mode == "status":
            return {"ok": True, "status": "done", "protocol": PROTOCOL, "instance": self.instance,
                    "pid": os.getpid(), "home": os.path.expanduser("~")}
        if mode == "quit":
            return {"ok": True, "status": "done"}
        if mode == "python":
            return self.run_python(request)
        return {"ok": False, "status": "failed", "error": "unknown mode %r" % mode}

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
