#!/usr/bin/env python3
# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: MCP server: QtiSAS as a tool of an external AI chat
# **************************************************************************** #
"""MCP server (Model Context Protocol, stdio) that gives an external AI chat - Claude Code, Claude Desktop, an IDE -
tools for a running QtiSAS: run Python in it, ask its own AI, list its objects, see a graph as an image. It talks to
the QtiSAS script server (qtisas_server.py) on this machine; the API docs are offered as resources.

    qtisas_mcp.py [--instance NAME] [--start-offscreen] [--qtisas PATH]

Claude Code:  claude mcp add qtisas -- python3 /path/to/qtisas_mcp.py --instance main
Codex CLI:    codex mcp add qtisas -- python3 /path/to/qtisas_mcp.py --instance main
              (or in ~/.codex/config.toml: [mcp_servers.qtisas] command = "python3"
                                           args = ["/path/to/qtisas_mcp.py", "--instance", "main"])
from another computer over SSH (QtiSAS and this server run on computer1):
              claude mcp add qtisas -- ssh computer1 python3 /path/to/qtisas_mcp.py --instance main
--start-offscreen starts QtiSAS without a window (-S=NAME) when no instance answers yet.
Plain Python 3, no packages needed. stdout carries the protocol only; messages go to stderr.
"""

import argparse
import base64
import json
import mimetypes
import os
import subprocess
import sys
import tempfile
import threading
import time

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import qtisas_send  # noqa: E402  the same folder: socket path, its privacy check, the request itself
import qtisas_coach  # noqa: E402  the coach tools (benchmark runs), offered where the source tree's tests are

PROTOCOL = "2025-06-18"
HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.normpath(os.path.join(HERE, "..", "doc"))  # source tree and app bundle alike: python/doc

INSTRUCTIONS = """Tools for a running QtiSAS (scientific data analysis: tables, matrices, 2D/3D graphs, fits).
Read the API docs first (resources qtisas-doc://...; the .pyi files are the compact form). run_python runs Python
inside QtiSAS, with its variables kept between calls; print() and scriptPrint() output comes back. Names you create
(tables, graphs, fit functions "ai/...") persist in QtiSAS. Use show_graph to see a graph as an image. Nothing is
saved by itself: save with saveProjectAs("/path/file.qti") when asked. ask_qtisas_ai hands a request (or a LIST of
numbered tasks) to QtiSAS's own built-in AI instead of writing the code yourself; while it runs, correct_qtisas_ai
steers it and stop cancels it. ask_qtisas_chat talks with QtiSAS's chat instead: it answers in words, reading the
project itself, and makes a change only after a yes to its proposal; the conversation continues across calls. To
supervise a long request, start it with wait=false and watch it with qtisas_ai_progress (wait_seconds avoids tight
polling); correct it when it goes wrong."""

COACH_INSTRUCTIONS = """
Coach tools (source tree only): qtisas_run starts a benchmark list in a test instance and returns at once;
qtisas_wait returns when it is done (call again on timeout); qtisas_summary gives the score and every lost item's
dossier; qtisas_failures says whether a failure was seen before; qtisas_stats compares runs by label; qtisas_probe
checks an API call on a side instance; qtisas_environment shows the build, the docs and the local backends."""

def chat_work(lines, limit=20000):
    """What the chat did for a message: its probes with what they printed and a change it handed over, without the
    echo of the message, the "question ..." and "loaded API docs" lines and the reply itself (returned on its own)."""
    work = [line for line in lines
            if not line.startswith(("▸▸▸ AI chat request:", "▸▸▸ AI chat:\n\n", "▸▸▸ AI chat: question ",
                                    "▸▸▸ AI loaded API docs"))]
    text = "\n".join(work).strip()
    if len(text) > limit:  # a long one: keep the end, where the last probe and the change are
        text = "...(earlier lines left out)\n" + text[-limit:]
    return ("what the chat did:\n" + text) if text else ""


TOOLS = [
    {"name": "run_python",
     "description": "Run Python code inside QtiSAS (its API is global: newTable(), table(), plot(), Fittable, ...). "
                    "Variables are kept between calls. Returns the printed output, the error with hints, and the "
                    "windows/fit functions created or removed.",
     "inputSchema": {"type": "object", "properties": {
         "code": {"type": "string", "description": "Python code"},
         "timeout": {"type": "number", "description": "seconds to wait, default 600"}},
         "required": ["code"]}},
    {"name": "ask_qtisas_ai",
     "description": "Hand a request to QtiSAS's built-in AI: it writes the script (run=false) or writes and runs it "
                    "(run=true). A text starting with LIST and numbered lines is a task list, item by item. Returns "
                    "the AI's transcript and a summary.",
     "inputSchema": {"type": "object", "properties": {
         "text": {"type": "string"},
         "run": {"type": "boolean", "description": "also run the scripts (default true)"},
         "timeout": {"type": "number", "description": "seconds, default 1800"},
         "wait": {"type": "boolean", "description": "wait for the result (default true); false starts it and "
                                                    "returns at once, to watch it with qtisas_ai_progress and steer "
                                                    "it with correct_qtisas_ai"}},
         "required": ["text"]}},
    {"name": "ask_qtisas_chat",
     "description": "Talk with QtiSAS's chat (/ai-chat), the AI selected in that QtiSAS: it answers in words, reads "
                    "the project itself with read-only probes, and changes it only after a yes to a change it "
                    "proposed (its script AI then makes the change). The conversation continues from call to call "
                    "(its history lives in that QtiSAS; the text \"clear history\" starts over). Returns the reply, "
                    "the probes it ran with what they printed, and the windows created or removed.",
     "inputSchema": {"type": "object", "properties": {
         "text": {"type": "string", "description": "the message"},
         "timeout": {"type": "number", "description": "seconds, default 900"},
         "wait": {"type": "boolean", "description": "wait for the reply (default true); false starts it and returns "
                                                    "at once, to watch it with qtisas_ai_progress"}},
         "required": ["text"]}},
    {"name": "qtisas_ai_progress",
     "description": "The progress of an ask_qtisas_ai or ask_qtisas_chat request started with wait=false: its new "
                    "lines from line from_line on, and its result once it has finished. wait_seconds (up to 120) "
                    "waits for new lines first.",
     "inputSchema": {"type": "object", "properties": {
         "from_line": {"type": "integer", "description": "the first line to return, default 0"},
         "wait_seconds": {"type": "number", "description": "wait up to this long for something new, default 0"}}}},
    {"name": "list_objects",
     "description": "The current project's tables, matrices, 2D graphs, 3D plots, notes and compiled ai/ fit "
                    "functions.",
     "inputSchema": {"type": "object", "properties": {}}},
    {"name": "show_graph",
     "description": "A 2D graph or 3D plot of QtiSAS, by window name, as a PNG image.",
     "inputSchema": {"type": "object", "properties": {"name": {"type": "string"}}, "required": ["name"]}},
    {"name": "get_file",
     "description": "A file from the computer QtiSAS runs on (e.g. one it exported or saved): an image comes back as "
                    "an image, a text file (.dat, .txt, .csv, .py, ...) as text, another small file as an embedded "
                    "blob. For large files (a project of several MB) copy them instead, e.g. with scp.",
     "inputSchema": {"type": "object", "properties": {"path": {"type": "string"}}, "required": ["path"]}},
    {"name": "status",
     "description": "The script server's state: instance, pid, whether a request (e.g. an AI run) is running.",
     "inputSchema": {"type": "object", "properties": {}}},
    {"name": "stop",
     "description": "Cancel QtiSAS's running AI request (a running Python script cannot be interrupted).",
     "inputSchema": {"type": "object", "properties": {}}},
    {"name": "correct_qtisas_ai",
     "description": "While ask_qtisas_ai runs: a correction for QtiSAS's AI (e.g. 'use table B, not A'), followed "
                    "from its next step on; a text starting with '!' restarts its current model call with it.",
     "inputSchema": {"type": "object", "properties": {"text": {"type": "string"}}, "required": ["text"]}},
]

LIST_OBJECTS = r"""
import json
def _names(get):
    try:
        return sorted(get())
    except Exception as e:
        return ["(%s)" % e]
print(json.dumps({
    "tables": _names(tableNames),
    "matrices": _names(matrixNames),
    "graphs": _names(lambda: [w.objectName() for w in graphs()]),
    "plots3d": _names(lambda: [w.objectName() for w in windows() if type(w).__name__ == "Graph3D"]),
    "notes": _names(lambda: [w.objectName() for w in notes()]),
    "fit_functions_ai": _names(lambda: [f for f in Compiler.functionNames("ALL") if f.startswith("ai/")]),
}, indent=1))
"""

SHOW_GRAPH = r"""
_g = graphWindow(%(name)r) or plot3D(%(name)r)
if _g is None:
    raise ValueError("no 2D graph or 3D plot named %%r" %% %(name)r)
_g.export(%(path)r)
try:
    from PyQt6.QtCore import Qt
    from PyQt6.QtGui import QImage
except ImportError:
    from PyQt5.QtCore import Qt
    from PyQt5.QtGui import QImage
_img = QImage(%(path)r)  # a chat takes images of a few hundred KB, not a full-resolution export
if max(_img.width(), _img.height()) > 1200:
    _img = _img.scaled(1200, 1200, getattr(Qt, "AspectRatioMode", Qt).KeepAspectRatio,
                       getattr(Qt, "TransformationMode", Qt).SmoothTransformation)
_img.save(%(path)r, "PNG")
print("%%d x %%d px" %% (_img.width(), _img.height()))
"""


def log(*args):
    print("qtisas_mcp:", *args, file=sys.stderr, flush=True)


class Server:
    def __init__(self, instance, start_offscreen=False, qtisas=None):
        self.background = None  # an ask_qtisas_ai or ask_qtisas_chat request started with wait=false, see start_ai()
        self.instance = instance
        self.path = qtisas_send.default_socket_path(instance)
        self.start_offscreen = start_offscreen
        self.qtisas = qtisas
        self.rid = 0
        self.coach = qtisas_coach.Coach(self.text) if qtisas_coach.available() else None

    # ---- QtiSAS side ----

    def reachable(self):
        try:
            return qtisas_send.request({"id": 0, "mode": "status"}, self.path, 5).get("ok", False)
        except (OSError, ValueError):
            return False

    def default_qtisas(self):
        """The QtiSAS this file belongs to: an app bundle's launcher, or the source tree's build."""
        bundle = os.path.normpath(os.path.join(HERE, "..", "..", "..", "MacOS", "launcher"))  # Resources/python/server
        build = os.path.normpath(os.path.join(HERE, "..", "..", "..", "bin", "qtisas.app", "Contents", "MacOS",
                                              "qtisas"))  # qtisas/python/server in the source tree
        return next((p for p in (bundle, build) if os.access(p, os.X_OK)), None)

    def ensure_running(self):
        if self.reachable() or not self.start_offscreen:
            return
        exe = self.qtisas or self.default_qtisas()
        if not exe:
            raise RuntimeError("no QtiSAS to start: give --qtisas PATH")
        log("starting %s -S=%s" % (exe, self.instance))
        logfile = open(os.path.join(tempfile.gettempdir(), "qtisas-mcp-%s.log" % self.instance), "ab")
        subprocess.Popen([exe, "-S=%s" % self.instance], stdout=logfile, stderr=logfile, stdin=subprocess.DEVNULL,
                         start_new_session=True)
        for _ in range(120):
            if self.reachable():
                return
            time.sleep(0.5)
        raise RuntimeError("QtiSAS started but its script server did not answer within 60 s")

    def send(self, message, timeout, on_line=None):
        self.ensure_running()
        self.rid += 1
        message["id"] = self.rid
        try:
            result = qtisas_send.request(message, self.path, timeout, on_line)
        except (FileNotFoundError, ConnectionRefusedError):
            raise RuntimeError("no QtiSAS script server for instance %r: start QtiSAS with -s=%s (window) or -S=%s "
                               "(offscreen), or this MCP server with --start-offscreen"
                               % (self.instance, self.instance, self.instance))
        if result.get("event") == "busy":
            raise RuntimeError("QtiSAS is busy with another request; try again, or use stop for a running AI request")
        return result

    # ---- tools ----

    @staticmethod
    def text(*parts, error=False, image=None):
        content = [{"type": "text", "text": "\n".join(p for p in parts if p)}]
        if image:
            content.append({"type": "image", "data": image, "mimeType": "image/png"})
        return {"content": content, "isError": error}

    def run_python(self, code, timeout=600):
        r = self.send({"mode": "python", "code": code, "keep": True, "filename": "<mcp>"},
                      timeout)
        changes = "; ".join(["%s: %s" % (key, ", ".join(r[key])) for key in ("created", "removed") if r.get(key)])
        return self.text(r.get("output", "").rstrip(), r.get("error", ""), changes, error=not r.get("ok"))

    def ask_qtisas_ai(self, text, run=True, timeout=1800, wait=True):
        if not wait:
            return self.start_ai(text, "ai-run" if run else "ai", timeout)
        lines = []
        r = self.send({"mode": "ai-run" if run else "ai", "text": text, "timeout": timeout},
                      timeout + 30, lines.append)
        transcript = "\n".join(lines)
        if len(transcript) > 30000:  # a long LIST: keep the end, where the results are
            transcript = "...(earlier lines left out)\n" + transcript[-30000:]
        return self.text(transcript, qtisas_send.summary(r), error=not r.get("ok"))

    def ask_qtisas_chat(self, text, timeout=900, wait=True):
        if not wait:
            return self.start_ai(text, "ai-chat", timeout)
        lines = []
        r = self.send({"mode": "ai-chat", "text": text, "timeout": timeout}, timeout + 30, lines.append)
        return self.text(r.get("reply") or "(no reply)", chat_work(lines), qtisas_send.summary(r),
                         r.get("error", ""), error=not r.get("ok"))

    def start_ai(self, text, mode, timeout):
        """Starts the request in a thread of its own and returns at once: the caller watches it with
        qtisas_ai_progress() and can correct or stop it meanwhile."""
        if self.background and not self.background["done"]:
            return self.text("an AI request started earlier is still running: watch it with qtisas_ai_progress, or "
                             "stop it", error=True)
        job = self.background = {"lines": [], "result": None, "done": False}

        def work():
            try:
                job["result"] = self.send({"mode": mode, "text": text, "timeout": timeout}, timeout + 30,
                                          job["lines"].append)
            except Exception as e:  # reported by qtisas_ai_progress
                job["result"] = {"ok": False, "status": "failed", "error": "%s: %s" % (type(e).__name__, e)}
            job["done"] = True

        threading.Thread(target=work, daemon=True).start()
        return self.text("started: watch it with qtisas_ai_progress(from_line=0), steer it with correct_qtisas_ai, "
                         "cancel it with stop")

    def ai_progress(self, from_line=0, wait_seconds=0):
        job = self.background
        if job is None:
            return self.text("no AI request was started with ask_qtisas_ai(wait=false)", error=True)
        deadline = time.monotonic() + min(max(wait_seconds, 0), 120)
        while not job["done"] and len(job["lines"]) <= from_line and time.monotonic() < deadline:
            time.sleep(0.5)
        lines = job["lines"][from_line:]
        done = job["done"]
        head = "lines %d to %d, %s" % (from_line, from_line + len(lines),
                                       "finished" if done else "still running")
        result = job["result"] if done else None
        return self.text(head, "\n".join(lines),
                         ("chat reply:\n" + result["reply"]) if result and result.get("reply") else "",
                         (qtisas_send.summary(result) + ("\n" + result["error"] if result.get("error") else ""))
                         if result else "",
                         error=bool(result) and not result.get("ok"))

    def list_objects(self):
        r = self.send({"mode": "python", "code": LIST_OBJECTS, "filename": "<mcp>"}, 60)
        return self.text(r.get("output", "").rstrip(), r.get("error", ""), error=not r.get("ok"))

    def show_graph(self, name):
        path = os.path.join(tempfile.gettempdir(), "qtisas-mcp-%d.png" % os.getpid())
        r = self.send({"mode": "python", "code": SHOW_GRAPH % {"name": name, "path": path}, "filename": "<mcp>"}, 120)
        if not r.get("ok") or not os.path.exists(path):
            return self.text(r.get("error", "") or "the graph was not exported", error=True)
        with open(path, "rb") as f:
            image = base64.b64encode(f.read()).decode("ascii")
        os.remove(path)
        return self.text("graph %r, %s" % (name, r.get("output", "").strip()), image=image)

    IMAGE_LIMIT = 5 * 1024 * 1024  # what chat apps take as an image
    TEXT_LIMIT = 200 * 1024  # more is cut, with a note: the model reads it, a copy is what scp is for
    BLOB_LIMIT = 1024 * 1024

    def get_file(self, path):
        """Read here, not through QtiSAS: this server runs on QtiSAS's computer, and a file needs no window."""
        path = os.path.abspath(os.path.expanduser(path))
        if not os.path.isfile(path):
            return self.text("no file %s" % path, error=True)
        size = os.path.getsize(path)
        mime = mimetypes.guess_type(path)[0] or ""
        with open(path, "rb") as f:
            data = f.read(max(self.IMAGE_LIMIT, self.TEXT_LIMIT, self.BLOB_LIMIT) + 1)
        if mime in ("image/png", "image/jpeg", "image/gif", "image/webp"):
            if size > self.IMAGE_LIMIT:
                return self.text("%s is %.1f MB, too large to show: export it smaller, or copy it (scp)"
                                 % (path, size / 1e6), error=True)
            return {"content": [{"type": "text", "text": "%s (%d bytes)" % (path, size)},
                                {"type": "image", "data": base64.b64encode(data).decode("ascii"), "mimeType": mime}],
                    "isError": False}
        if b"\0" not in data[:8192]:  # text: a NUL byte is the plain sign of a binary file
            text = data[:self.TEXT_LIMIT].decode("utf-8", errors="replace")
            note = "" if size <= self.TEXT_LIMIT else "\n...(cut: the file has %d bytes; copy it for all of it)" % size
            return self.text("%s (%d bytes)\n%s%s" % (path, size, text, note))
        if size > self.BLOB_LIMIT:
            return self.text("%s is a %.1f MB binary file: copy it instead (scp)" % (path, size / 1e6), error=True)
        return {"content": [{"type": "resource", "resource": {
                    "uri": "file://" + path, "mimeType": mime or "application/octet-stream",
                    "blob": base64.b64encode(data).decode("ascii")}}], "isError": False}

    def status(self):
        r = self.send({"mode": "status"}, 10)
        return self.text(json.dumps({k: v for k, v in r.items() if k not in ("id", "event")}))

    def stop(self):
        r = self.send({"mode": "stop"}, 10)
        return self.text(r.get("output", "").rstrip())

    def correct_qtisas_ai(self, text):
        r = self.send({"mode": "note", "text": text}, 10)
        return self.text((r.get("output") or r.get("error") or "").rstrip(), error=not r.get("ok"))

    def call(self, name, args):
        if self.coach and name in {t["name"] for t in qtisas_coach.TOOLS}:
            return self.coach.call(name, args)
        if name == "run_python":
            return self.run_python(args["code"], args.get("timeout") or 600)
        if name == "ask_qtisas_ai":
            return self.ask_qtisas_ai(args["text"], args.get("run", True), args.get("timeout") or 1800,
                                      args.get("wait", True))
        if name == "ask_qtisas_chat":
            return self.ask_qtisas_chat(args["text"], args.get("timeout") or 900, args.get("wait", True))
        if name == "qtisas_ai_progress":
            return self.ai_progress(int(args.get("from_line") or 0), float(args.get("wait_seconds") or 0))
        if name == "list_objects":
            return self.list_objects()
        if name == "show_graph":
            return self.show_graph(args["name"])
        if name == "get_file":
            return self.get_file(args["path"])
        if name == "status":
            return self.status()
        if name == "stop":
            return self.stop()
        if name == "correct_qtisas_ai":
            return self.correct_qtisas_ai(args["text"])
        raise KeyError(name)

    # ---- resources: the API docs ----

    @staticmethod
    def docs():
        if not os.path.isdir(DOCS):
            return []
        return sorted(f for f in os.listdir(DOCS) if f.endswith(("-python-api.md", "-python-api.pyi")))

    def resources(self):
        return [{"uri": "qtisas-doc://" + f, "name": f, "mimeType": "text/markdown" if f.endswith(".md")
                 else "text/x-python", "description": ("QtiSAS Python API: " + f.split("-python-api")[0] +
                                                       (" (compact stub)" if f.endswith(".pyi") else ""))}
                for f in self.docs()]

    def read_resource(self, uri):
        name = uri[len("qtisas-doc://"):] if uri.startswith("qtisas-doc://") else ""
        if name not in self.docs():
            raise KeyError(uri)
        with open(os.path.join(DOCS, name), encoding="utf-8") as f:
            return {"contents": [{"uri": uri, "mimeType": "text/markdown" if name.endswith(".md") else
                                  "text/x-python", "text": f.read()}]}

    # ---- JSON-RPC over stdio ----

    def handle(self, msg):
        method, params = msg.get("method"), msg.get("params") or {}
        if method == "initialize":
            return {"protocolVersion": params.get("protocolVersion") or PROTOCOL,
                    "capabilities": {"tools": {}, "resources": {}},
                    "serverInfo": {"name": "qtisas", "version": "1"},
                    "instructions": INSTRUCTIONS + (COACH_INSTRUCTIONS if self.coach else "")}
        if method == "ping":
            return {}
        if method == "tools/list":
            return {"tools": TOOLS + (qtisas_coach.TOOLS if self.coach else [])}
        if method == "tools/call":
            try:
                return self.call(params.get("name"), params.get("arguments") or {})
            except KeyError as e:
                return self.text("unknown tool or missing argument: %s" % e, error=True)
            except Exception as e:  # QtiSAS not reachable, timed out, busy: the model should see it, not a crash
                return self.text("%s: %s" % (type(e).__name__, e), error=True)
        if method == "resources/list":
            return {"resources": self.resources()}
        if method == "resources/read":
            return self.read_resource(params.get("uri", ""))
        raise LookupError(method)

    def answer(self, msg, stdout, lock):
        try:
            reply = {"jsonrpc": "2.0", "id": msg["id"], "result": self.handle(msg)}
        except LookupError as e:
            reply = {"jsonrpc": "2.0", "id": msg["id"], "error": {"code": -32601, "message": "unknown: %s" % e}}
        except Exception as e:
            reply = {"jsonrpc": "2.0", "id": msg["id"], "error": {"code": -32603, "message": str(e)}}
        with lock:
            stdout.write(json.dumps(reply) + "\n")
            stdout.flush()

    def serve(self, stdin=sys.stdin, stdout=sys.stdout):
        # A tool call runs in a thread of its own: an ask_qtisas_ai call can take minutes, and stop,
        # correct_qtisas_ai and status are meant to be called meanwhile.
        lock = threading.Lock()
        calls = []
        for line in stdin:
            if not line.strip():
                continue
            try:
                msg = json.loads(line)
            except ValueError:
                continue
            if "id" not in msg:  # a notification (initialized, cancelled): nothing to answer
                continue
            if msg.get("method") == "tools/call":
                calls.append(threading.Thread(target=self.answer, args=(msg, stdout, lock), daemon=True))
                calls[-1].start()
            else:
                self.answer(msg, stdout, lock)
        for call in calls:
            call.join()


def main(argv=None):
    p = argparse.ArgumentParser(description="MCP server: QtiSAS as a tool of an external AI chat.")
    p.add_argument("--instance", default=os.environ.get("QTISAS_SERVER", "main"))
    p.add_argument("--start-offscreen", action="store_true", help="start QtiSAS with -S=NAME if none answers")
    p.add_argument("--qtisas", help="the QtiSAS to start (default: the one this file belongs to)")
    args = p.parse_args(argv)
    Server(args.instance, args.start_offscreen, args.qtisas).serve()
    return 0


if __name__ == "__main__":
    sys.exit(main())
