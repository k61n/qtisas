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
Its settings start as defaults except the AI's (presets and options copied); dialogs are closed automatically.
Every start copies the user's AI options ([AI] group) again, so the instance runs with the user's current ones.
Packages installed with "qtisas pip ..." are read from the user's site-packages (or copied: --copy-site-packages).
"""

import argparse
import os
import shutil
import signal
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
sys.dont_write_bytecode = True  # no __pycache__ next to the scripts
sys.path.insert(0, os.path.join(REPO, "qtisas", "python", "server"))
import qtisas_send  # noqa: E402

DEFAULT_HOME = os.path.join(os.path.expanduser("~"), ".qtisas-test", "home")
# the build in the source tree; --app for another one, e.g. a release's Contents/MacOS/launcher (its own Python)
DEFAULT_APP = os.path.join(REPO, "bin", "qtisas.app", "Contents", "MacOS", "qtisas")
# QtiSAS runs every *.py of python-scripts at startup. Appended last: the test home's own packages win
SITE_PACKAGES_STARTER = """\
# written by qtisas_test_instance.py reset: the user's packages, readable here, never written to
import os
import sys

_user_packages = %r
if os.path.isdir(_user_packages) and _user_packages not in sys.path:
    sys.path.append(_user_packages)
"""
COPIED = ["FitFunctions", "SANSinstruments", "ascii1dFormats", "templates", "calibrationStandards", "ai"]


def config(home):
    return os.path.join(home, ".config", "qtisas")


def copy_tree(source, target):
    if sys.platform == "darwin":  # APFS clone: instant, no extra space until files change
        subprocess.run(["cp", "-cR", source, target], check=True)
    else:
        shutil.copytree(source, target)


def ai_settings(ini):
    """The [AI] group of a QtiSAS settings file, as its lines (empty if there is none)."""
    if not os.path.isfile(ini):
        return []
    lines, inside = [], False
    for line in open(ini, encoding="utf-8"):
        if line.startswith("["):
            inside = line.strip() == "[AI]"
        if inside:
            lines.append(line)
    return lines


def with_ai_settings(ini_lines, ai_lines):
    """The settings file's lines with its [AI] group replaced by ai_lines (appended when it has none)."""
    result, inside, replaced = [], False, False
    for line in ini_lines:
        if line.startswith("["):
            inside = line.strip() == "[AI]"
            if inside and not replaced:
                result.extend(ai_lines)
                replaced = True
        if not inside:
            result.append(line)
    if not replaced and ai_lines:
        if result and not result[-1].endswith("\n"):
            result[-1] += "\n"
        if result and result[-1].strip():
            result.append("\n")
        result.extend(ai_lines)
    return result


def sync_ai_settings(args):
    """Copies the user's [AI] group into the test home again; prints what changed (API keys only by name)."""
    user_ai = ai_settings(os.path.join(config(os.path.expanduser("~")), "qtisas.ini"))
    test_ini = os.path.join(config(args.home), "qtisas.ini")
    if not user_ai:
        return
    old_ai = ai_settings(test_ini)
    if old_ai == user_ai:
        return
    lines = open(test_ini, encoding="utf-8").readlines() if os.path.isfile(test_ini) else []
    with open(test_ini, "w", encoding="utf-8") as f:
        f.write("".join(with_ai_settings(lines, user_ai)))
    old = dict(line.rstrip("\n").split("=", 1) for line in old_ai if "=" in line)
    new = dict(line.rstrip("\n").split("=", 1) for line in user_ai if "=" in line)
    changed = sorted(k for k in set(old) | set(new) if old.get(k) != new.get(k) and k != "Info")
    shown = [k if "key" in k.lower() else "%s=%s" % (k, new.get(k, "(removed)")) for k in changed]
    print("AI settings updated from the user's: " + (", ".join(shown) if shown else "(Info only)"))


def reset(args):
    user = config(os.path.expanduser("~"))
    test = config(args.home)
    if os.path.isdir(args.home):
        shutil.rmtree(args.home)
    os.makedirs(test)
    for name in COPIED:
        if os.path.isdir(os.path.join(user, name)):
            copy_tree(os.path.join(user, name), os.path.join(test, name))
    # packages installed with "qtisas pip ...": the launcher's PYTHONPATH is ~/.config/qtisas/python/site-packages, i.e.
    # the test home's own (where a pip install in the test instance goes); the user's are read from, never written to
    user_packages = os.path.join(user, "python", "site-packages")
    if args.copy_site_packages:
        if os.path.isdir(user_packages):
            os.makedirs(os.path.join(test, "python"), exist_ok=True)
            copy_tree(user_packages, os.path.join(test, "python", "site-packages"))
    else:
        os.makedirs(os.path.join(test, "python-scripts"))
        with open(os.path.join(test, "python-scripts", "user_site_packages.py"), "w", encoding="utf-8") as f:
            f.write(SITE_PACKAGES_STARTER % user_packages)
    # settings: the defaults, except the AI's (selected preset, options), so the internal AI runs as for the user
    ai = ai_settings(os.path.join(user, "qtisas.ini"))
    if ai:
        with open(os.path.join(test, "qtisas.ini"), "w", encoding="utf-8") as f:
            f.write("".join(ai))
    print("test home ready: %s" % args.home)
    return 0


def pid_file(args):
    return os.path.join(args.home, "qtisas.pid")


def status(args, quiet=False):
    try:
        result = qtisas_send.request({"id": 1, "mode": "status"}, args.socket, timeout=5)
    except (OSError, ValueError):
        if not quiet:
            print("not running")
        return 1
    if not quiet:
        if result.get("event") == "busy":
            print("running, busy with a request")
        else:
            print("running: instance %s, pid %s, home %s"
                  % (result.get("instance"), result.get("pid"), result.get("home")))
    return 0


def start(args):
    if status(args, quiet=True) == 0:
        print("already running")
        return 0
    if not os.path.isdir(config(args.home)):
        reset(args)
    # confirmed: a test home made days earlier kept MaxAutoContinue=4 and MaxSplitParts=8 while the user's QtiSAS ran
    # with 15 and 3 — the benchmark scored 0-2 of 6 where the same list in the user's QtiSAS scored 6
    sync_ai_settings(args)
    # dialogs closed automatically also with a visible window: nobody is expected to click them
    env = dict(os.environ, HOME=args.home, QTISAS_SERVER_SOCKET=args.socket, QTISAS_SERVER_DIALOGS="auto")
    option = ("-s=" if args.visible else "-S=") + args.instance
    log = open(os.path.join(args.home, "qtisas.log"), "w")
    process = subprocess.Popen([args.app, option], env=env, stdout=log, stderr=subprocess.STDOUT,
                               stdin=subprocess.DEVNULL, start_new_session=True)
    with open(pid_file(args), "w") as f:  # for stop, when the server cannot answer (busy with a long AI run)
        f.write(str(process.pid))
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
        answer = qtisas_send.request({"id": 1, "mode": "quit"}, args.socket, timeout=10)
    except (OSError, ValueError):
        answer = None
    if answer is None or answer.get("event") == "busy":  # not answering, or busy: end the process itself
        try:
            os.kill(int(open(pid_file(args)).read()), signal.SIGTERM)
        except (OSError, ValueError):
            if answer is None:
                print("not running")
                return 1
    deadline = time.monotonic() + 10  # it exits shortly after answering
    while status(args, quiet=True) == 0 and time.monotonic() < deadline:
        time.sleep(0.2)
    if status(args, quiet=True) == 0:
        print("still running")
        return 2
    if os.path.exists(pid_file(args)):
        os.remove(pid_file(args))
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
    p.add_argument("--copy-site-packages", action="store_true",
                   help="reset: copy the user's site-packages (a fixed state) instead of reading them where they are")
    args = p.parse_args(argv)
    args.socket = args.socket or qtisas_send.default_socket_path(args.instance)
    return {"reset": reset, "start": start, "status": status, "stop": stop}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
