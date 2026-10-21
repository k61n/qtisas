"""The script server (qtisas/python/server): requests run in this QtiSAS, answered over a local socket.

- python mode: output of print() and scriptPrint() captured, the error with the script's own frames and line, the
  objects created and removed, variables kept only with "keep", modal dialogs closed with "dialogs": "auto";
- status, an unknown mode, and a failure of the server itself are answered too; an AI run's structure (status, items,
  R<n>: lines, scripts, tokens) is read from its printed lines;
- a name in use is refused, a leftover socket file of a QtiSAS that no longer runs is replaced; a socket folder others
  can use is refused by server and client, and a folder the server creates is 0700;
- over the socket, with the real client: qtisas_send.main() and its exit codes 0 (ok), 1 (script failed), 2 (busy).
"""

import os
import socket
import sys
import tempfile
import threading
import time

try:
    from PyQt6.QtCore import QEventLoop, QTimer
except ImportError:
    from PyQt5.QtCore import QEventLoop, QTimer

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "..", "..", "python", "server"))
import qtisas_send
import qtisas_server

folder = tempfile.mkdtemp(prefix="qtisas-server-test-")
path = os.path.join(folder, "t.sock")
server = qtisas_server.ScriptServer("unit", path, "auto")
run = lambda code, **kw: server.run(dict(kw, mode="python", code=code))

# a name in use is never taken over; a leftover socket file of a QtiSAS that no longer runs is
try:
    qtisas_server.ScriptServer("unit", path, "auto")
    refused = ""
except RuntimeError as e:
    refused = str(e)
assert "instance 'unit' is already used" in refused, refused
stale = os.path.join(folder, "stale.sock")
leftover = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
leftover.bind(stale)
leftover.close()  # the file stays, nobody listens
assert os.path.exists(stale)
replacement = qtisas_server.ScriptServer("stale", stale, "auto")
assert replacement.server.isListening()
replacement.server.close()

# a socket folder others can use is refused by the server and by the client (a shared /tmp)
shared = os.path.join(folder, "shared")
os.mkdir(shared)
os.chmod(shared, 0o755)
try:
    qtisas_server.ScriptServer("shared", os.path.join(shared, "s.sock"), "auto")
    refused = ""
except RuntimeError as e:
    refused = str(e)
assert "can be used by other users" in refused, refused
assert qtisas_send.main(["--socket", os.path.join(shared, "s.sock"), "-c", "pass"]) == 2
os.rmdir(shared)
created = os.path.join(folder, "new", "n.sock")  # a folder the server creates: this user's only
fresh = qtisas_server.ScriptServer("new", created, "auto")
assert oct(os.stat(os.path.dirname(created)).st_mode & 0o777) == "0o700"
fresh.server.close()
os.rmdir(os.path.dirname(created))

# output, created / removed objects
r = run('t = newTable("ServerTestT", 3, 2)\nprint("p")\nscriptPrint("s")')
assert r["ok"] and r["output"] == "p\ns\n" and r["created"] == ["table:ServerTestT"], r
r = run('t = table("ServerTestT")\nt.confirmClose(False)\nt.close()')
assert r["ok"] and r["removed"] == ["table:ServerTestT"], r

# the error: the script's frames and line, not the server's
r = run("x = 1\ny = x / 0\n", filename="demo.py")
assert not r["ok"] and r["status"] == "failed", r
assert 'File "demo.py", line 2' in r["error"] and "y = x / 0" in r["error"], r["error"]
assert "qtisas_server" not in r["error"] and r["error"].endswith("ZeroDivisionError: division by zero"), r["error"]
r = run("raise SystemExit(0)")
assert r["ok"], r

# a prompt input (qtisas_send.py's prompt): a bare expression prints its value; error hints as in QtiSAS's console
assert run("1 + 2", interactive=True)["output"] == "3\n"
assert run("1 + 2")["output"] == ""  # a script prints nothing by itself
assert run("a = 1; b = 2", interactive=True)["ok"]
r = run("setDecimalSeparators(2)", interactive=True)
assert "a method of Table" in r["error"] and "t.setDecimalSeparators(...)" in r["error"], r["error"]
r = run('n = newNote("ServerTestN")\nn.tab(1).text()')
assert "toPlainText()" in r["error"] and "contextMenu" not in r["error"], r["error"]
assert "The object is None" in run('note("ServerTestN").tab("nope").setText("x")')["error"]
# a method of one of its parts (Dan.newHeaderTable is Dan.Rawdata.newHeaderTable), two levels down too
assert "Dan.Rawdata.newHeaderTable()" in run('Dan.newHeaderTable("x", ["D"])')["error"]
assert "Dan.Options.Config.setCenterMethod()" in run('Dan.setCenterMethod("hf")')["error"]
assert "Dan.Rawdata.newHeaderTable()" in run('Dan.Processing.newHeaderTable("x", ["D"])')["error"]
closeWindow(note("ServerTestN"))

# "stop" with nothing running: answered, nothing cancelled (while an AI request runs it cancels it, even when busy)
r = server.stop_ai()
assert r["ok"] and r["stopped"] is False and "nothing to stop" in r["output"], r
r = server.run({"mode": "status"})
assert r["running"] is None and r["ai_lines"] == 0, r  # while an AI request runs: "ai-run" and its lines so far
r = server.note_ai("use table B")  # a correction needs a running AI request; idle, nothing is started
assert r["ok"] and r["noted"] is False and "nothing to correct" in r["output"], r
assert not server.note_ai(" ")["ok"]

# variables: fresh by default, kept with "keep"
assert run("a = 41", keep=True)["ok"]
r = run("print(a + 1)", keep=True)
assert r["output"] == "42\n", r
r = run("print(a)")
assert not r["ok"] and "NameError" in r["error"], r

# a modal dialog is closed (Cancel) and reported
r = run("try:\n    from PyQt6.QtWidgets import QMessageBox\nexcept ImportError:\n"
        "    from PyQt5.QtWidgets import QMessageBox\n"
        "B = getattr(QMessageBox, 'StandardButton', QMessageBox)\n"
        "print(QMessageBox.question(None, 'Q', 'Continue?') == B.Yes)")
assert r["ok"] and r["output"] == "False\n" and [d["text"] for d in r["dialogs"]] == ["Continue?"], r

# other modes and the server's own failures
r = server.run({"mode": "status"})
assert r["ok"] and r["instance"] == "unit" and r["protocol"] == qtisas_server.PROTOCOL, r
r = server.run({"mode": "nonsense"})
assert not r["ok"] and "unknown mode" in r["error"], r
r = server.run({"mode": "ai", "text": " "})
assert not r["ok"] and "no request text" in r["error"], r

# an AI run's structure, read from what it printed (lines as in a real LIST run)
# Chat replies are opaque, even when discussing script/LIST output and errors.
reply = "AI LIST: item 1/2 failed\nAI code (running)\nR1: 42\n▸▸▸ AI: connection error"
r = qtisas_server.summarize_chat(["▸▸▸ AI chat:\n\n" + reply])
assert r["ok"] and r["reply"] == reply and r["scripts"] == 0 and r["items"] == [], r
assert not qtisas_server.summarize_chat(["▸▸▸ AI: connection error: offline"])["ok"]
assert not qtisas_server.summarize_chat([])["ok"]
assert qtisas_server.summarize_chat([], allow_empty=True)["ok"]

lines = ["▸▸▸ AI: sending ~9KB (~2K tokens estimated), history not sent for this call",
         "▸▸  AI LIST: item 1/3 — Create a table \"Squares\" ...",
         "▸▸▸ AI: sending ~214KB (~55K tokens estimated), 0 history turn(s)",
         "▸▸▸ AI code (running):\n\nscriptPrint(\"R1: \" + str(s))", "Sum: 385.0\nR1: 385.0",
         "▸▸  AI LIST: item 2/3 (took 27.5sec, 27.5sec total) — Compile ...",
         "▸▸▸ AI code (running):\n\nc = Compiler", "▸▸▸ Python script Error\n\nAttributeError: x",
         "▸▸▸ AI code (running):\n\nc = Compiler", "▸▸▸ AI: stopping — the same corrective explanation ...",
         "(LIST item 2/3 failed — skipping it and continuing with the next item.)",
         "▸▸  AI LIST: item 3/3 (took 1min 4.5sec, 1min 32sec total) — Delete ...",
         "R3: []", "▸▸  AI LIST: all 3 item(s) completed (took 8.9sec, 1min 41sec total)."]
r = qtisas_server.summarize_ai(lines)
assert (r["status"], r["failed_items"], r["scripts"], r["tokens"]) == ("items-failed", [2], 3, 57000), r
assert r["r_lines"] == ["R1: 385.0", "R3: []"], r["r_lines"]
got = [(i["item"], i["scripts"], i["r_lines"], i["seconds"], i["failed"]) for i in r["items"]]
assert got == [(1, 1, ["R1: 385.0"], 27.5, False), (2, 2, [], 64.5, True), (3, 0, ["R3: []"], 8.9, False)], got
r = qtisas_server.summarize_ai(["▸▸  AI LIST: item 1/2 — a", "R1: 1",
                                "▸▸  AI LIST: item 2/2 (took 1.0sec, 1.0sec total) — b",
                                "▸▸▸ AI: connection error (ConnectionRefusedError): Connection refused"])
assert [i["backend_error"] for i in r["items"]] == [False, True] and r["backend_errors"] == 1, r["items"]
r = qtisas_server.summarize_ai(["▸▸▸ AI code (not run):\n\nx = 1", "▸▸▸ AI: stopping — this request refers to ..."])
assert (r["status"], r["ok"], r["items"]) == ("stopped", False, []), r

# over the socket, with the real client: exit codes 0, 1 and 2 (busy)
codes = {}


def client():
    try:
        requests()
    except BaseException as e:  # reported by the assertion below, not lost in this thread
        codes["exception"] = repr(e)


def requests():
    codes["ok"] = qtisas_send.main(["--socket", path, "--timeout", "20", "-c", "print('over the socket')"])
    codes["failed"] = qtisas_send.main(["--socket", path, "--timeout", "20", "-c", "1/0"])
    server.busy = True  # as while another request runs
    codes["busy"] = qtisas_send.main(["--socket", path, "--timeout", "20", "-c", "pass"])
    server.busy = False


loop = QEventLoop()
thread = threading.Thread(target=client)
thread.start()
timer = QTimer()
timer.timeout.connect(lambda: loop.quit() if not thread.is_alive() else None)
timer.start(50)
QTimer.singleShot(30000, loop.quit)  # never hang the test
loop.exec()
timer.stop()
thread.join(1)
assert codes == {"ok": 0, "failed": 1, "busy": 2}, codes

# the MCP server (qtisas_mcp.py), its protocol side only: tools, the API docs as resources, errors as JSON-RPC errors
import io  # noqa: E402
import json  # noqa: E402
import qtisas_mcp  # noqa: E402
mcp_in = io.StringIO("\n".join(json.dumps(m) for m in (
    {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"protocolVersion": "2025-06-18"}},
    {"jsonrpc": "2.0", "method": "notifications/initialized"},
    {"jsonrpc": "2.0", "id": 2, "method": "tools/list"},
    {"jsonrpc": "2.0", "id": 3, "method": "resources/list"},
    {"jsonrpc": "2.0", "id": 4, "method": "resources/read", "params": {"uri": "qtisas-doc://table-python-api.pyi"}},
    {"jsonrpc": "2.0", "id": 5, "method": "tools/call", "params": {"name": "nope", "arguments": {}}},
    {"jsonrpc": "2.0", "id": 6, "method": "no/such"},
    {"jsonrpc": "2.0", "id": 7, "method": "tools/call",
     "params": {"name": "get_file",
                "arguments": {"path": os.path.join(qtisas_mcp.DOCS, "table-python-api.pyi")}}})) + "\n")
mcp_out = io.StringIO()
qtisas_mcp.Server("mcp-test").serve(mcp_in, mcp_out)
replies = {r["id"]: r for r in map(json.loads, mcp_out.getvalue().splitlines())}
assert sorted(replies) == [1, 2, 3, 4, 5, 6, 7], replies.keys()  # the notification is not answered
assert replies[1]["result"]["serverInfo"]["name"] == "qtisas", replies[1]
assert {t["name"] for t in replies[2]["result"]["tools"]} >= {"run_python", "ask_qtisas_ai", "show_graph", "stop",
                                                               "correct_qtisas_ai"}
assert any(r["uri"] == "qtisas-doc://table-python-api.md" for r in replies[3]["result"]["resources"]), replies[3]
assert "newTable" in replies[4]["result"]["contents"][0]["text"], replies[4]
assert replies[5]["result"]["isError"], replies[5]  # an unknown tool: reported to the model, not a protocol error
assert replies[6]["error"]["code"] == -32601, replies[6]
assert not replies[7]["result"]["isError"] and "newTable" in replies[7]["result"]["content"][0]["text"], replies[7]

# an AI request started without waiting (wait=false): watched with qtisas_ai_progress (the script server faked here)
mcp = qtisas_mcp.Server("mcp-test")
assert mcp.ai_progress()["isError"]  # nothing started
mcp.send = lambda message, timeout, on_line=None: [on_line(t) for t in ("first", "second")] and {
    "ok": True, "status": "done", "seconds": 1.0, "created": [], "removed": [], "scripts": 1, "tokens": 100}
assert "started" in mcp.ask_qtisas_ai("make a table", wait=False)["content"][0]["text"]
r = mcp.ai_progress(0, 10)["content"][0]["text"]
for _ in range(20):  # the thread has finished within moments
    if "finished" in r:
        break
    time.sleep(0.1)
    r = mcp.ai_progress(0)["content"][0]["text"]
assert "finished" in r and "first\nsecond" in r, r
assert "lines 1 to 2" in mcp.ai_progress(1)["content"][0]["text"]

server.server.close()
os.remove(path) if os.path.exists(path) else None
os.rmdir(folder)
