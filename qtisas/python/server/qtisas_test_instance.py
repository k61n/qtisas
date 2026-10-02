#!/usr/bin/env python3
# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: a separate QtiSAS for scripted tests, with the script server running
# **************************************************************************** #
"""A QtiSAS instance of its own for scripted tests: it never touches the user's settings or fit-function library.

    qtisas_test_instance.py reset    # (re)create the test home from the user's fit library and data folders
    qtisas_test_instance.py start    # start QtiSAS there with the script server; returns once it answers
                                     # (offscreen; --visible to see its window)
    qtisas_test_instance.py status
    qtisas_test_instance.py stop

QtiSAS builds every path from the home folder (~/.config/qtisas/...: settings, FitFunctions, python, ai), so the
instance runs with HOME set to the test home, started with -S=<instance> (offscreen script server; -s with --visible).
Its settings start as defaults; dialogs are closed automatically.
"""

import argparse
import os
import shutil
import subprocess
import sys
import time

import qtisas_send

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
DEFAULT_HOME = os.path.join(os.path.expanduser("~"), ".qtisas-test", "home")
# next to a build in the source tree (bin/qtisas.app), or inside an app bundle (Contents/Resources/python/server)
DEFAULT_APP = os.path.join(REPO, "bin", "qtisas.app", "Contents", "MacOS", "qtisas")
if os.path.basename(REPO) == "Contents":
    DEFAULT_APP = os.path.join(REPO, "MacOS", "qtisas")
COPIED = ["FitFunctions", "SANSinstruments", "ascii1dFormats", "templates", "calibrationStandards"]


def config(home):
    return os.path.join(home, ".config", "qtisas")


def copy_tree(source, target):
    if sys.platform == "darwin":  # APFS clone: instant, no extra space until files change
        subprocess.run(["cp", "-cR", source, target], check=True)
    else:
        shutil.copytree(source, target)


def reset(args):
    user = config(os.path.expanduser("~"))
    test = config(args.home)
    if os.path.isdir(args.home):
        shutil.rmtree(args.home)
    os.makedirs(test)
    for name in COPIED:
        if os.path.isdir(os.path.join(user, name)):
            copy_tree(os.path.join(user, name), os.path.join(test, name))
    print("test home ready: %s" % args.home)
    return 0


def status(args, quiet=False):
    try:
        result = qtisas_send.request({"id": 1, "mode": "status"}, args.socket, timeout=5)
    except (OSError, ValueError):
        if not quiet:
            print("not running")
        return 1
    if not quiet:
        print("running: instance %s, pid %s, home %s" % (result.get("instance"), result.get("pid"), result.get("home")))
    return 0


def start(args):
    if status(args, quiet=True) == 0:
        print("already running")
        return 0
    if not os.path.isdir(config(args.home)):
        reset(args)
    # dialogs closed automatically also with a visible window: nobody is expected to click them
    env = dict(os.environ, HOME=args.home, QTISAS_SERVER_SOCKET=args.socket, QTISAS_SERVER_DIALOGS="auto")
    option = ("-s=" if args.visible else "-S=") + args.instance
    log = open(os.path.join(args.home, "qtisas.log"), "w")
    process = subprocess.Popen([args.app, option], env=env, stdout=log, stderr=subprocess.STDOUT,
                               stdin=subprocess.DEVNULL, start_new_session=True)
    deadline = time.monotonic() + args.wait
    while time.monotonic() < deadline:
        if status(args, quiet=True) == 0:
            return status(args)
        if process.poll() is not None:  # e.g. the instance name is in use: QtiSAS -S exits with its error
            log.close()
            print("QtiSAS exited (%s): %s" % (process.returncode, open(log.name).read().strip()))
            return 2
        time.sleep(0.5)
    print("QtiSAS did not answer within %g s; see %s" % (args.wait, log.name))
    return 2


def stop(args):
    try:
        qtisas_send.request({"id": 1, "mode": "quit"}, args.socket, timeout=10)
    except (OSError, ValueError):
        print("not running")
        return 1
    print("stopped")
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(description="A separate QtiSAS for scripted tests, with the script server.")
    p.add_argument("command", choices=["reset", "start", "status", "stop"])
    p.add_argument("--home", default=DEFAULT_HOME)
    p.add_argument("--app", default=DEFAULT_APP)
    p.add_argument("--instance", default="test")
    p.add_argument("--socket", help="socket path (default /tmp/qtisas-<uid>/<instance>.sock)")
    p.add_argument("--wait", type=float, default=60, help="seconds start waits for the server")
    p.add_argument("--visible", action="store_true", help="start with its window shown (default: offscreen)")
    args = p.parse_args(argv)
    args.socket = args.socket or qtisas_send.default_socket_path(args.instance)
    return {"reset": reset, "start": start, "status": status, "stop": stop}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
