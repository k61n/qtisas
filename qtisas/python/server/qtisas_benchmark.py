#!/usr/bin/env python3
# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: runs an AI benchmark list in the test instance and scores it
# **************************************************************************** #
"""Runs an AI benchmark (a LIST whose items each print one known "R<n>: ..." line) in the test instance and scores it.

    qtisas_benchmark.py BENCHMARK.json [--markers "NOSPLIT NOREVIEW NOEXPLORE"] [--label TEXT] [--keep-instance]
                       [--agent NAME] [--set KEY=VALUE ...]   # an AI agent (preset), "Ask AI" page settings
    qtisas_benchmark.py BENCHMARK.json --print [--markers ...]     # the LIST text only, e.g. to paste into QtiSAS

A benchmark file: {"name", "instruction", "setup" (Python run before), "items": [{"text", "expected", "hint"}]}.
Each run starts a fresh test instance (a new project, no AI history), runs the setup, sends the LIST in mode ai-run
and compares every item's R<n>: line with its expected value. Verdicts: ok; wrong or missing, each "reported" when the
assistant itself said the item failed, "silent" when it reported success. Written to ~/.qtisas-test/reports: the full
log, a JSON report, and one line per run in history.csv. Exit code: 0 all items ok, 1 not all, 2 no run.
"""

import argparse
import csv
import datetime
import json
import os
import re
import subprocess
import sys

sys.dont_write_bytecode = True  # no __pycache__ next to the scripts
import qtisas_send  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
INSTANCE = os.path.join(HERE, "qtisas_test_instance.py")
DEFAULT_REPORTS = os.path.join(os.path.expanduser("~"), ".qtisas-test", "reports")


def list_text(bench, markers=""):
    lines = [("LIST " + markers).strip()]
    if bench.get("instruction"):
        lines.append(bench["instruction"])
    # "Item n:" in the text: a LIST item is sent on its own, without its number, which the R<n>: line needs
    lines += ["%d. Item %d: %s" % (i + 1, i + 1, item["text"]) for i, item in enumerate(bench["items"])]
    return "\n".join(lines)


def values(line):
    """The values of an R<n>: line, normalized: no prefix, separators, brackets, quotes or "name=" labels."""
    text = re.sub(r"^R\d+:\s*", "", line.strip())
    text = re.sub(r"[,;]\s+", " ", text)  # a separator, not the decimal comma of "1234,5"
    text = re.sub(r"\b\w+\s*=\s*", "", text)  # "a=2.0000" is 2.0000
    return [t for t in (t.strip("[]()'\"") for t in text.split()) if t]


def same_value(got, expected):
    try:
        g, e = float(got), float(expected)
    except ValueError:
        return got.lower() == expected.lower()
    return abs(g - e) <= 1e-6 * max(1.0, abs(e))  # 385.0 is 385, -0.0 is 0


def matches(line, expected):
    got, want = values(line), values(expected)
    return len(got) == len(want) and all(same_value(g, e) for g, e in zip(got, want))


def score(bench, result):
    items = {i["item"]: i for i in result.get("items", [])}
    rows = []
    for n, item in enumerate(bench["items"], 1):
        # the item's own lines (any R<k>: printed while it ran), else an R<n>: line anywhere
        lines = items.get(n, {}).get("r_lines") or [r for r in result.get("r_lines", []) if r.startswith("R%d:" % n)]
        got = lines[-1] if lines else ""
        run = items.get(n, {})
        reported = run.get("failed", False) or result.get("status") in ("stopped", "timeout")
        if got and matches(got, item["expected"]):
            verdict = "ok"
        else:
            verdict = ("wrong" if got else "missing") + (" (reported)" if reported else " (silent)")
        rows.append({"item": n, "expected": item["expected"], "got": got, "verdict": verdict,
                     "scripts": run.get("scripts"), "seconds": run.get("seconds"), "hint": item.get("hint", "")})
    return rows


def instance(*args):
    return subprocess.run([sys.executable, INSTANCE] + list(args), capture_output=True, text=True)


def main(argv=None):
    p = argparse.ArgumentParser(description="Run an AI benchmark list in the test instance and score it.")
    p.add_argument("benchmark")
    p.add_argument("--markers", default="", help='LIST markers, e.g. "NOSPLIT NOREVIEW NOEXPLORE"')
    p.add_argument("--print", action="store_true", help="print the LIST text and exit")
    p.add_argument("--label", default="", help="a note for history.csv, e.g. what changed since the last run")
    p.add_argument("--keep-instance", action="store_true", help="use the running test instance as it is")
    p.add_argument("--agent", help="the AI agent (preset) to use; default: the test instance's selected one")
    p.add_argument("--set", action="append", default=[], metavar="KEY=VALUE",
                   help='an "Ask AI" setting for this run, e.g. exploreEnabled=False or maxAutoContinue=6')
    p.add_argument("--timeout", type=float, default=7200)
    p.add_argument("--reports", default=DEFAULT_REPORTS)
    p.add_argument("--quiet", action="store_true", help="do not print the AI's lines while it runs")
    args = p.parse_args(argv)

    bench = json.load(open(args.benchmark, encoding="utf-8"))
    text = list_text(bench, args.markers)
    if args.print:
        print(text)
        return 0

    if not args.keep_instance:  # a new project and no AI history: every run starts the same
        instance("stop")
        started = instance("start")
        if started.returncode != 0:
            print("qtisas_benchmark: the test instance did not start:\n" + started.stdout + started.stderr)
            return 2
    path = qtisas_send.default_socket_path("test")
    # the agent and settings of this run, applied and then read back for the report
    code = ["import json, qti"]
    if args.agent:
        code.append("qti.app.setAiAgent(%r)" % args.agent)
    for item in args.set:
        key, _, value = item.partition("=")
        code.append("qti.app.setAiConfig(%r, %s)" % (key.strip(), value.strip()))
    code.append("print(json.dumps(qti.app.aiConfig()))")
    configured = qtisas_send.request({"id": 4, "mode": "python", "code": "\n".join(code)}, path, timeout=60)
    if not configured.get("ok"):
        print("qtisas_benchmark: the AI agent or settings could not be set:\n" + configured.get("error", ""))
        return 2
    ai_config = json.loads(configured["output"].strip().splitlines()[-1])
    if bench.get("setup"):
        setup = qtisas_send.request({"id": 2, "mode": "python", "code": bench["setup"]}, path, timeout=120)
        if not setup.get("ok"):
            print("qtisas_benchmark: the setup failed:\n" + (setup.get("error") or json.dumps(setup)))
            return 2

    os.makedirs(args.reports, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    base = os.path.join(args.reports, "%s-%s" % (stamp, re.sub(r"\W+", "_", bench["name"]).strip("_")))
    with open(base + ".log", "w", encoding="utf-8") as log:
        def on_line(line):
            log.write(line + "\n")
            log.flush()
            if not args.quiet:
                print(line, flush=True)

        result = qtisas_send.request({"id": 3, "mode": "ai-run", "text": text, "timeout": args.timeout,
                                      "dialogs": "auto"}, path, args.timeout + 60, on_line)

    rows = score(bench, result)
    ok = sum(r["verdict"] == "ok" for r in rows)
    silent = sum(r["verdict"].endswith("(silent)") for r in rows)
    reported = sum(r["verdict"].endswith("(reported)") for r in rows)
    preset, model = ai_config.get("agent", ""), ai_config.get("model", "")
    summary = {"date": stamp, "benchmark": bench["name"], "markers": args.markers, "preset": preset, "model": model,
               "score": ok, "of": len(rows), "silent": silent, "reported": reported,
               "seconds": result.get("seconds"), "scripts": result.get("scripts"), "tokens": result.get("tokens"),
               "status": result.get("status"), "settings": " ".join(args.set), "label": args.label}
    with open(base + ".json", "w", encoding="utf-8") as f:
        json.dump({"summary": summary, "ai_config": ai_config, "items": rows, "result": result}, f, indent=1,
                  ensure_ascii=False)
    history = os.path.join(args.reports, "history.csv")
    old = list(csv.DictReader(open(history, encoding="utf-8"))) if os.path.exists(history) else []
    with open(history, "w", newline="", encoding="utf-8") as f:  # rewritten: older rows keep their columns
        w = csv.DictWriter(f, fieldnames=list(summary), restval="", extrasaction="ignore")
        w.writeheader()
        w.writerows(old + [summary])
    if not args.keep_instance:
        instance("stop")

    print("\n%s %s — %d of %d ok, %d silent, %d reported · %s s · %s scripts · ~%sK tokens · %s %s"
          % (bench["name"], ("LIST " + args.markers).strip(), ok, len(rows), silent, reported, summary["seconds"],
             summary["scripts"], (summary["tokens"] or 0) // 1000, preset, model))
    for r in rows:
        print("  %2d %-20s %-24s %s" % (r["item"], r["verdict"], r["got"][:24] or "—",
                                        "" if r["verdict"] == "ok" else "expected %s; %s" % (r["expected"], r["hint"])))
    print("report: %s.json, log: %s.log" % (base, base))
    return 0 if ok == len(rows) else 1


if __name__ == "__main__":
    sys.exit(main())
