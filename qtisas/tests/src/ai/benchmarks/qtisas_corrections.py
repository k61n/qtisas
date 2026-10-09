#!/usr/bin/env python3
# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: summary of the corrections given to QtiSAS's AI during supervised runs
# **************************************************************************** #
"""Reads QtiSAS's corrections.log (one JSON line per correction typed while an AI request ran, and one per outcome of
the next script) and groups the corrections by the error that preceded them, as material for new mechanical checks,
retry hints and doc wording: the correction that fixed a step is a tested hint text.

    qtisas_corrections.py                       # ~/.config/qtisas/ai/corrections.log
    qtisas_corrections.py --home ~/.qtisas-test/home --since 2026-10-09
"""

import argparse
import json
import os
import re
import sys
from collections import OrderedDict

sys.dont_write_bytecode = True


def error_kind(error):
    """A short key for the error before a correction: the exception and its message with names and numbers blurred."""
    lines = [line.strip() for line in (error or "").splitlines() if line.strip()]
    if not lines:
        return "(no error: a correction without a failure before it)"
    last = next((line for line in reversed(lines) if re.match(r"^[A-Za-z_.]*(Error|Exception)\b", line)), lines[-1])
    last = re.sub(r"'[^']*'|\"[^\"]*\"", "'…'", last)
    return re.sub(r"\b\d+(\.\d+)?\b", "N", last)[:160]


def read(path, since=None):
    """(correction, outcome or None) pairs, in order."""
    pairs = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            try:
                entry = json.loads(line)
            except ValueError:
                continue
            if since and entry.get("time", "") < since:
                continue
            if entry.get("event") == "correction":
                pairs.append([entry, None])
            elif entry.get("event") == "outcome" and pairs and pairs[-1][1] is None:
                pairs[-1][1] = entry
    return pairs


def report(pairs):
    groups = OrderedDict()
    for correction, outcome in pairs:
        groups.setdefault(error_kind(correction.get("last_error")), []).append((correction, outcome))
    out = ["# Corrections: %d, grouped by the error before them\n" % len(pairs)]
    for kind, members in sorted(groups.items(), key=lambda kv: -len(kv[1])):
        fixed = sum(1 for _, o in members if o and o.get("result") == "ran")
        out.append("## %s\n\n%d correction(s), %d followed by a script that ran\n" % (kind, len(members), fixed))
        out.append("| model | item/part | step | correction | next script |")
        out.append("|---|---|---|---|---|")
        for c, o in members:
            where = "/".join(str(c[k]) for k in ("list_item", "split_part") if k in c) or "-"
            result = (o.get("result") + (": " + o["detail"].splitlines()[-1][:80] if o.get("detail") else "")) \
                if o else "(none)"
            out.append("| %s | %s | %s | %s%s | %s |" % (c.get("model", ""), where, c.get("step", ""),
                                                           "! " if c.get("now") else "",
                                                           c.get("text", "").replace("|", "\\|"),
                                                           result.replace("|", "\\|")))
        out.append("")
    return "\n".join(out)


def main(argv=None):
    p = argparse.ArgumentParser(description="Group the corrections of supervised AI runs by the error before them.")
    p.add_argument("--home", default=os.path.expanduser("~"), help="the HOME QtiSAS ran with (a test home)")
    p.add_argument("--log", help="the log file itself, instead of HOME/.config/qtisas/ai/corrections.log")
    p.add_argument("--since", help="only entries from this ISO time on, e.g. 2026-10-09")
    args = p.parse_args(argv)
    path = args.log or os.path.join(args.home, ".config", "qtisas", "ai", "corrections.log")
    if not os.path.exists(path):
        print("no corrections log at %s" % path, file=sys.stderr)
        return 1
    print(report(read(path, args.since)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
