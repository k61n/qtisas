#!/usr/bin/env python3
# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: client of the script server (qtisas_server.py)
# **************************************************************************** #
"""Sends a request to a running QtiSAS script server and prints its result.

    qtisas_send.py [--instance NAME | --socket PATH] [--mode python|status|quit]
                   [-c CODE | -f FILE | -] [--json] [--keep] [--timeout S] [--dialogs auto|leave]

The script's output goes to stdout, its error to stderr; a one-line summary to stderr (unless --json, which prints the
whole result as JSON instead). Exit code: 0 ok, 1 the script failed, 2 no server, busy or no reply in time.
Plain Python 3, no PyQt needed.
"""

import argparse
import json
import os
import socket
import sys


def default_socket_path(instance):
    return os.path.join("/tmp", "qtisas-%d" % os.getuid(), instance + ".sock")


def request(message, path, timeout=None, on_line=None):
    """Sends `message` (a dict), returns the final event; `on_line(text)` gets the streamed "line" events."""
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
    for d in result.get("dialogs", []):
        parts.append("dialog closed: %s%s" % (d.get("title", ""), (" — " + d["text"]) if d.get("text") else ""))
    return " · ".join(parts)


def main(argv=None):
    p = argparse.ArgumentParser(description="Run Python in a running QtiSAS (script server).")
    p.add_argument("--instance", default=os.environ.get("QTISAS_SERVER", "test"))
    p.add_argument("--socket", help="socket path (default /tmp/qtisas-<uid>/<instance>.sock)")
    p.add_argument("--mode", default="python", choices=["python", "status", "quit"])
    source = p.add_mutually_exclusive_group()
    source.add_argument("-c", dest="code", help="code to run")
    source.add_argument("-f", dest="file", help="script file to run, '-' for stdin")
    p.add_argument("--json", action="store_true", help="print the whole result as JSON")
    p.add_argument("--keep", action="store_true", help="use (and keep) the session's variables")
    p.add_argument("--timeout", type=float, default=None, help="seconds to wait for the result")
    p.add_argument("--dialogs", choices=["auto", "leave"], help="close dialogs automatically, or leave them")
    args = p.parse_args(argv)

    message = {"id": 1, "mode": args.mode}
    if args.mode == "python":
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

    path = args.socket or default_socket_path(args.instance)
    try:
        result = request(message, path, args.timeout)
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

    if args.json:
        print(json.dumps(result, indent=1, ensure_ascii=False))
    else:
        if result.get("output"):
            sys.stdout.write(result["output"])
            getattr(sys.stdout, "flush", lambda: None)()  # before the summary on stderr
        if result.get("error"):
            print(result["error"], file=sys.stderr)
        if args.mode == "status":
            print(json.dumps({k: v for k, v in result.items() if k not in ("id", "event")}))
        elif args.mode == "python":
            print(summary(result), file=sys.stderr)
    return 0 if result.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
