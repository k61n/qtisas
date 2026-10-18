#!/usr/bin/env python3
# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: client of the script server (qtisas_server.py)
# **************************************************************************** #
"""Sends a request to a running QtiSAS script server and prints its result.

    qtisas_send.py [--instance NAME | --socket PATH] [--mode python|ai|ai-run|ai-chat|status|stop|note|quit]
                   [-c CODE | -f FILE | -] [--json | --stream-json] [--keep] [--timeout S] [--dialogs auto|leave]
    qtisas_send.py [--instance NAME] [--mode python|ai|ai-run|ai-chat]     # no code in a terminal: the interactive prompt

python: the script's output goes to stdout, its error to stderr. ai / ai-run: the text is a request to the internal AI
(ai generates only, ai-run also runs its scripts; ai-chat is conversation without scripts); its lines are printed as they come. A one-line summary goes to
stderr; --json prints the whole result as JSON instead, --stream-json every event as one JSON line.
Exit code: 0 ok, 1 the script or the AI run failed, 2 no server, busy or no reply in time.
Without code in a terminal it opens a prompt: Python (variables kept between inputs) or the AI, see :help.
Plain Python 3, no PyQt needed.
"""

import argparse
import codeop
import json
import os
import socket
import stat
import sys


def default_socket_path(instance):
    return os.path.join("/tmp", "qtisas-%d" % os.getuid(), instance + ".sock")


def private_folder_problem(folder):
    """None if `folder` is missing or a folder only this user can use; otherwise why it is not.

    /tmp is shared: a folder another user created, or one others can write to, could hold their socket, which would
    receive this user's code and answer with anything.
    """
    if not hasattr(os, "getuid"):
        return None
    try:
        st = os.lstat(folder)
    except FileNotFoundError:
        return None
    if not stat.S_ISDIR(st.st_mode):
        return "%s is not a folder" % folder
    if st.st_uid != os.getuid():
        return "%s belongs to another user (uid %d)" % (folder, st.st_uid)
    if st.st_mode & 0o077:
        return "%s can be used by other users (mode %03o)" % (folder, stat.S_IMODE(st.st_mode))
    return None


def private_socket_problem(path):
    """None if the socket at `path` (and its folder) belong to this user only; otherwise why not."""
    problem = private_folder_problem(os.path.dirname(path) or ".")
    if problem is None and hasattr(os, "getuid"):
        try:
            if os.lstat(path).st_uid != os.getuid():
                problem = "%s belongs to another user" % path
        except FileNotFoundError:
            pass
    return problem


def request(message, path, timeout=None, on_line=None):
    """Sends `message` (a dict), returns the final event; `on_line(text)` gets the streamed "line" events.

    PermissionError if the socket or its folder is not this user's own (see private_socket_problem()).
    """
    problem = private_socket_problem(path)
    if problem:
        raise PermissionError("refusing to connect: " + problem)
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.settimeout(timeout)
        s.connect(path)
        s.sendall((json.dumps(message) + "\n").encode("utf-8"))
        buffer = b""
        while True:
            chunk = s.recv(65536)
            if not chunk:
                raise ConnectionError("the server closed the connection without a result")
            buffer += chunk
            while b"\n" in buffer:
                line, buffer = buffer.split(b"\n", 1)
                event = json.loads(line.decode("utf-8"))
                if event.get("event") == "line":
                    if on_line:
                        on_line(event.get("text", ""))
                    continue
                return event


def summary(result):
    parts = ["%s %.2f s" % ("✓" if result.get("ok") else "✗", result.get("seconds", 0.0))]
    if result.get("created"):
        parts.append("created: " + ", ".join(result["created"]))
    if result.get("removed"):
        parts.append("removed: " + ", ".join(result["removed"]))
    if result.get("items"):
        parts.append("%d item(s), failed: %s" % (len(result["items"]), result.get("failed_items") or "none"))
    if "scripts" in result:
        parts.append("%d script(s) run, ~%dK tokens" % (result["scripts"], result.get("tokens", 0) // 1000))
    if result.get("status") not in (None, "done", "failed"):
        parts.append("status: " + result["status"])
    for d in result.get("dialogs", []):
        parts.append("dialog closed: %s%s" % (d.get("title", ""), (" — " + d["text"]) if d.get("text") else ""))
    return " · ".join(parts)


PROMPTS = {"python": "py> ", "ai": "ai> ", "ai-run": "ai-run> ", "ai-chat": "ai-chat> "}

PROMPT_HELP = """Python runs in the QtiSAS script server, with its variables kept from one input to the next; a line
that opens a block (if/for/def ...) continues until an empty line. ai: the text is a request to QtiSAS's AI, which only
writes the script; ai-run: the AI also runs it; ai-chat: conversation without scripts. The AI's lines are printed as they come; a request starting
with LIST (a numbered task list) continues until an empty line, so a whole list can be pasted.
  :py  :ai  :ai-run  :ai-chat  switch the mode (the prompt shows it)
  :file PATH            send a file in the current mode: a Python script, or an AI request such as a LIST
  :status               the server's state          :instance NAME   talk to another QtiSAS
  :help                 this text                   :quit or Ctrl+D  leave the prompt (QtiSAS keeps running)
  :stop                 cancel QtiSAS's running AI request (also from another terminal)
  :follow               show the running AI request's lines again, from where you left it, and its result
  :note TEXT            correct the running AI request: from its next step on, or at once with "!TEXT"
Ctrl+C while the AI works gives the prompt back and the AI goes on: py> inputs run meanwhile (between its steps),
:follow returns to it, :note corrects it, :stop cancels it. While Python runs, Ctrl+C only stops waiting: a running
script cannot be interrupted from outside."""


def needs_more(source):
    """True while `source` is an incomplete Python block, as Python's own prompt decides it."""
    try:
        return codeop.compile_command(source, "<qtisas-prompt>", "exec") is None
    except (SyntaxError, OverflowError, ValueError):
        return False  # a real error: send it, the server reports it


def read_entry(mode):
    """One input: a line; in python mode a block until it is complete; for the AI a LIST until an empty line."""
    lines = [input(PROMPTS[mode])]
    if mode == "python":
        while needs_more("\n".join(lines)):
            lines.append(input("... "))
    elif mode != "ai-chat" and lines[0].split()[:1] == ["LIST"]:  # "LIST", "LIST NOSPLIT ...": the items follow, one per line
        while lines[-1].strip():
            lines.append(input("... "))
    return "\n".join(lines).rstrip()


def print_result(result, mode):
    if result.get("output"):
        sys.stdout.write(result["output"] if result["output"].endswith("\n") else result["output"] + "\n")
    if result.get("error"):
        print(result["error"], file=sys.stderr)
    if mode != "python" or not result.get("ok") or result.get("created") or result.get("removed"):
        print(summary(result), file=sys.stderr)


def history_path():
    """The prompt's history, in QtiSAS's own settings folder: ~/.config/qtisas (AppData/Local/qtisas on Windows)."""
    folder = os.path.join(os.path.expanduser("~"), "AppData", "Local", "qtisas") if os.name == "nt" else \
        os.path.join(os.path.expanduser("~"), ".config", "qtisas")
    os.makedirs(folder, exist_ok=True)
    history = os.path.join(folder, "qtisas_send_history")
    old = os.path.expanduser("~/.qtisas_send_history")  # where earlier versions kept it
    if os.path.exists(old) and not os.path.exists(history):
        os.replace(old, history)
    return history


def interactive(instance, path, mode="python", timeout=None):
    """The prompt for a person at a terminal: Python or the AI of the QtiSAS behind `path`, one input at a time."""
    try:
        import readline  # arrow keys and history; not on every platform
        history = history_path()
        try:
            readline.read_history_file(history)
        except OSError:
            pass
        import atexit
        atexit.register(lambda: readline.write_history_file(history))
    except ImportError:
        pass

    def stop():
        try:
            result = request({"id": 0, "mode": "stop"}, path, 10)
            print((result.get("output") or "").rstrip(), file=sys.stderr)
        except (OSError, ValueError) as e:
            print("qtisas_send: stop not sent: %s" % e, file=sys.stderr)

    seen = {"lines": 0}  # lines of the current AI request already shown, where :follow goes on from

    def show(text):
        seen["lines"] += 1
        print(text, flush=True)

    def send(message):
        if message.get("mode") in ("ai", "ai-run", "ai-chat"):
            seen["lines"] = 0
        try:
            return request(message, path, timeout, show if message.get("mode") in ("ai", "ai-run", "ai-chat", "follow") else
                           lambda text: print(text, flush=True))
        except KeyboardInterrupt:
            if message.get("mode") in ("ai", "ai-run", "ai-chat", "follow"):
                print("\n(the AI request goes on in QtiSAS — :follow shows it again, :stop cancels it)",
                      file=sys.stderr)
            else:
                print("\n(stopped waiting — the script may still be running in QtiSAS)", file=sys.stderr)
        except PermissionError as e:
            print("qtisas_send: %s" % e, file=sys.stderr)
        except (FileNotFoundError, ConnectionRefusedError):
            print("qtisas_send: no server at %s (start QtiSAS with -s=%s)" % (path, instance), file=sys.stderr)
        except (ConnectionError, socket.timeout) as e:
            print("qtisas_send: %s" % (e or "no result in time"), file=sys.stderr)
        return None

    status = send({"id": 0, "mode": "status"})
    print("QtiSAS %s%s — :help for commands" % (instance, "" if status else " (not reachable yet)"))
    rid = 0
    while True:
        try:
            entry = read_entry(mode)
        except EOFError:
            print()
            return 0
        except KeyboardInterrupt:
            print()
            continue
        text = entry.strip()
        if not text:
            continue
        source_file = None
        if text.startswith(":file"):
            source_file = os.path.expanduser(text[len(":file"):].strip())
            try:
                with open(source_file, encoding="utf-8") as f:
                    entry = f.read()
            except OSError as e:
                print("qtisas_send: %s" % e, file=sys.stderr)
                continue
        elif text.startswith(":"):
            command, _, argument = text[1:].partition(" ")
            if command in ("py", "python", "ai", "ai-run", "ai-chat"):
                mode = "python" if command == "py" else command
            elif command == "stop":
                stop()
            elif command == "note":
                if argument.strip():
                    result = send({"id": 0, "mode": "note", "text": argument.strip()})
                    if result:
                        print((result.get("output") or result.get("error") or "").rstrip(), file=sys.stderr)
                else:
                    print("qtisas_send: :note TEXT — the correction for the running AI request", file=sys.stderr)
            elif command == "follow":
                result = send({"id": 0, "mode": "follow", "from": seen["lines"]})
                if result and "transcript" in result:  # the followed request's own result
                    print_result(result, "ai")
                elif result:  # nothing running
                    print((result.get("output") or "").rstrip(), file=sys.stderr)
            elif command == "status":
                result = send({"id": 0, "mode": "status"})
                if result:
                    print(json.dumps({k: v for k, v in result.items() if k not in ("id", "event")}))
            elif command == "instance" and argument.strip():
                instance = argument.strip()
                path = default_socket_path(instance)
                print("now talking to QtiSAS %s" % instance)
            elif command in ("quit", "exit", "q"):
                return 0
            elif command == "help":
                print(PROMPT_HELP)
            else:
                print("unknown command %s — :help lists them" % text.split()[0], file=sys.stderr)
            continue
        rid += 1
        message = {"id": rid, "mode": mode}
        if mode == "python":
            message.update(code=entry, keep=True, interactive=source_file is None,
                           filename=os.path.abspath(source_file) if source_file else "<qtisas-prompt>")
        else:
            message["text"] = entry
        result = send(message)
        if result is None:
            continue
        if result.get("event") == "busy":
            print("qtisas_send: QtiSAS is busy with another request", file=sys.stderr)
            continue
        print_result(result, mode)


def main(argv=None):
    p = argparse.ArgumentParser(description="Run Python in a running QtiSAS (script server).")
    p.add_argument("--instance", default=os.environ.get("QTISAS_SERVER", "test"))
    p.add_argument("--socket", help="socket path (default /tmp/qtisas-<uid>/<instance>.sock)")
    p.add_argument("--mode", default="python", choices=["python", "ai", "ai-run", "ai-chat", "status", "stop", "note", "quit"])
    source = p.add_mutually_exclusive_group()
    source.add_argument("-c", dest="code", help="code to run")
    source.add_argument("-f", dest="file", help="script file to run, '-' for stdin")
    p.add_argument("--json", action="store_true", help="print the whole result as JSON")
    p.add_argument("--stream-json", action="store_true", help="print every event (lines, result) as one JSON line")
    p.add_argument("--keep", action="store_true", help="use (and keep) the session's variables")
    p.add_argument("--timeout", type=float, default=None, help="seconds to wait for the result")
    p.add_argument("--dialogs", choices=["auto", "leave"], help="close dialogs automatically, or leave them")
    args = p.parse_args(argv)

    message = {"id": 1, "mode": args.mode}
    if args.mode == "note":  # qtisas_send.py --mode note -c "use table B instead"
        message["text"] = args.code if args.code is not None else (sys.stdin.read() if not sys.stdin.isatty() else "")
    elif args.mode in ("python", "ai", "ai-run", "ai-chat"):
        if args.code is not None:
            message["code"] = args.code
        elif args.file == "-" or (args.file is None and not sys.stdin.isatty()):
            message["code"] = sys.stdin.read()
        elif args.file:
            with open(args.file, encoding="utf-8") as f:
                message["code"] = f.read()
            message["filename"] = os.path.abspath(args.file)
        else:  # a person at a terminal
            return interactive(args.instance, args.socket or default_socket_path(args.instance), args.mode,
                               args.timeout)
        message["keep"] = args.keep
        if args.dialogs:
            message["dialogs"] = args.dialogs
        if args.mode != "python":  # a request to the AI, not code
            message["text"] = message.pop("code")
            if args.timeout:
                message["timeout"] = args.timeout

    def on_line(text):
        if args.stream_json:
            print(json.dumps({"event": "line", "text": text}, ensure_ascii=False))
        elif not args.json:
            print(text)
        getattr(sys.stdout, "flush", lambda: None)()

    path = args.socket or default_socket_path(args.instance)
    # the client waits a little longer than an AI run's own timeout, which the server then reports itself
    wait = args.timeout + 30 if args.timeout and args.mode in ("ai", "ai-run", "ai-chat") else args.timeout
    try:
        result = request(message, path, wait, on_line)
    except PermissionError as e:
        print("qtisas_send: %s" % e, file=sys.stderr)
        return 2
    except (FileNotFoundError, ConnectionRefusedError):
        print("qtisas_send: no server at %s (is QtiSAS running with QTISAS_SERVER=%s?)" % (path, args.instance),
              file=sys.stderr)
        return 2
    except socket.timeout:
        print("qtisas_send: no result within %g s — the script may still be running in QtiSAS" % args.timeout,
              file=sys.stderr)
        return 2
    if result.get("event") == "busy":
        print("qtisas_send: the server is busy with another request", file=sys.stderr)
        return 2

    if args.stream_json:
        print(json.dumps(result, ensure_ascii=False))
    elif args.json:
        print(json.dumps(result, indent=1, ensure_ascii=False))
    else:
        if result.get("output"):
            sys.stdout.write(result["output"])
            getattr(sys.stdout, "flush", lambda: None)()  # before the summary on stderr
        if result.get("error"):
            print(result["error"], file=sys.stderr)
        if args.mode == "status":
            print(json.dumps({k: v for k, v in result.items() if k not in ("id", "event")}))
        elif args.mode in ("python", "ai", "ai-run", "ai-chat"):
            print(summary(result), file=sys.stderr)
    return 0 if result.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
