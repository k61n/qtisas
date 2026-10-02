#!/usr/bin/env python3
# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: client of the script server (qtisas_server.py)
# **************************************************************************** #
"""Sends a request to a running QtiSAS script server and prints its result.

    qtisas_send.py [--instance NAME | --socket PATH] [--mode python|ai|ai-run|status|quit]
                   [-c CODE | -f FILE | -] [--json | --stream-json] [--keep] [--timeout S] [--dialogs auto|leave]

python: the script's output goes to stdout, its error to stderr. ai / ai-run: the text is a request to the internal AI
(ai generates only, ai-run also runs its scripts); its lines are printed as they come. A one-line summary goes to
stderr; --json prints the whole result as JSON instead, --stream-json every event as one JSON line.
Exit code: 0 ok, 1 the script or the AI run failed, 2 no server, busy or no reply in time.
Plain Python 3, no PyQt needed.
"""

import argparse
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


def main(argv=None):
    p = argparse.ArgumentParser(description="Run Python in a running QtiSAS (script server).")
    p.add_argument("--instance", default=os.environ.get("QTISAS_SERVER", "test"))
    p.add_argument("--socket", help="socket path (default /tmp/qtisas-<uid>/<instance>.sock)")
    p.add_argument("--mode", default="python", choices=["python", "ai", "ai-run", "status", "quit"])
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
    if args.mode in ("python", "ai", "ai-run"):
        if args.code is not None:
            message["code"] = args.code
        elif args.file == "-" or (args.file is None and not sys.stdin.isatty()):
            message["code"] = sys.stdin.read()
        elif args.file:
            with open(args.file, encoding="utf-8") as f:
                message["code"] = f.read()
            message["filename"] = os.path.abspath(args.file)
        else:
            print("qtisas_send: give -c CODE, -f FILE or code on stdin (the interactive prompt comes later)",
                  file=sys.stderr)
            return 2
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
    wait = args.timeout + 30 if args.timeout and args.mode in ("ai", "ai-run") else args.timeout
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
        elif args.mode in ("python", "ai", "ai-run"):
            print(summary(result), file=sys.stderr)
    return 0 if result.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
