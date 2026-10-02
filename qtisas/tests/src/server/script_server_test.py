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

server.server.close()
os.remove(path) if os.path.exists(path) else None
os.rmdir(folder)
