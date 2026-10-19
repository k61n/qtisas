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
                       [--agent NAME] [--docs compact|full|integrated] [--set KEY=VALUE ...]
                       # an AI agent (preset), the docs it gets (.pyi/.md/none), "Ask AI" page settings
    qtisas_benchmark.py BENCHMARK.json --print [--markers ...]     # the LIST text only, e.g. to paste into QtiSAS
    qtisas_benchmark.py BENCHMARK.json --reference [DIR]           # the reference scripts instead of the AI: 100%
    ... --instance NAME [--home DIR]   # another test instance (default "test"), e.g. one per AI agent in parallel

A benchmark file: {"name", "instruction", "setup" (Python run before), "items": [{"text", "expected", "hint"}]};
an item may add "also": further answers that count as right where the request allows two readings.
Its reference scripts, reference/<file name>/item01.py ..., solve the items with the documented API: run with
--reference (in python mode, in order, in one instance like a LIST), they must score 100% — proof that every item is
solvable and its expected value right.
Each run starts a fresh test instance (a new project, no AI history), runs the setup, sends the LIST in mode ai-run
and compares every item's R<n>: line with its expected value. Verdicts: ok; wrong or missing, each "reported" when the
assistant itself said the item failed, "silent" when it reported success. Written to ~/.qtisas-test/reports: the full
log, a JSON report, and one line per run in history.csv. Exit code: 0 all items ok, 1 not all, 2 no run.
    qtisas_benchmark.py BENCHMARK.json --rescore    # score the saved reports of this benchmark again (after "also")
"""

import argparse
import csv
import datetime
import fcntl
import glob
import json
import os
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
QTISAS = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
sys.dont_write_bytecode = True  # no __pycache__ next to the scripts
sys.path.insert(0, os.path.join(QTISAS, "python", "server"))
import qtisas_send  # noqa: E402

INSTANCE = os.path.join(QTISAS, "tests", "src", "server", "qtisas_test_instance.py")
DEFAULT_REPORTS = os.path.join(os.path.expanduser("~"), ".qtisas-test", "reports")
R_LINE_RE = re.compile(r"^R\d+:.*$", re.MULTILINE)  # as qtisas_server.py finds them


def run_reference(bench, folder, path, on_line):
    """The reference scripts run like a LIST: one item after the other in the same instance, scored the same way."""
    items, all_lines, seconds = [], [], 0.0
    for n in range(1, len(bench["items"]) + 1):
        script = os.path.join(folder, "item%02d.py" % n)
        code = open(script, encoding="utf-8").read() if os.path.isfile(script) else "raise SystemExit('no %s')" % script
        on_line("▸▸  reference item %d/%d: %s" % (n, len(bench["items"]), script))
        answer = qtisas_send.request({"id": 10 + n, "mode": "python", "code": code}, path, timeout=600)
        output = (answer.get("output") or "") + (answer.get("error") or "")
        for line in output.splitlines():
            on_line(line)
        lines = R_LINE_RE.findall(answer.get("output") or "")
        all_lines += lines
        seconds += answer.get("seconds") or 0.0
        items.append({"item": n, "r_lines": lines, "failed": not answer.get("ok"), "scripts": 1,
                      "seconds": answer.get("seconds"), "backend_error": False})
    return {"ok": all(not i["failed"] for i in items), "status": "done", "items": items, "r_lines": all_lines,
            "scripts": len(items), "tokens": 0, "seconds": round(seconds, 2)}


def verify_code(bench):
    """One script that runs every item's "verify" snippet after the list: each reads the real state and reports it
    with check(...), printed as "V<n>: ..."; a snippet that raises reports "V<n>: error ..." instead."""
    code = []
    for n, item in enumerate(bench["items"], 1):
        if not item.get("verify"):
            continue
        body = "\n".join("    " + line for line in item["verify"].splitlines())
        code.append("def _verify%d(check):\n%s\n" % (n, body))
        code.append("try:\n    _verify%d(lambda *v: scriptPrint('V%d:', *v))\n"
                    "except Exception as e:\n    scriptPrint('V%d: error', type(e).__name__, e)\n" % (n, n, n))
    return "\n".join(code)


def run_verify(bench, path, on_line):
    """The items' verify snippets, run after the list in the same instance: {item number: "V<n>: ..." line}."""
    code = verify_code(bench)
    if not code:
        return {}
    on_line("▸▸  verify: the real state after the list")
    answer = qtisas_send.request({"id": 5, "mode": "python", "code": code}, path, timeout=120)
    lines = {}
    for line in ((answer.get("output") or "") + (answer.get("error") or "")).splitlines():
        on_line(line)
        m = re.match(r"^V(\d+):", line)
        if m:
            lines[int(m.group(1))] = line
    return lines


# Run in the test instance (its HOME is the test home): a copy of the selected preset with DocsMode set, selected
DOCS_PRESET = r"""
agent = qti.app.aiAgent()
folder = os.path.expanduser("~/.config/qtisas/ai")
text = open(os.path.join(folder, agent + ".ai"), encoding="utf-8").read()
text = re.sub(r"(?m)^DocsMode=.*\n?", "", text).replace("[AI]\n", "[AI]\nDocsMode=%(mode)d\n", 1)
name = agent + "-docs-%(docs)s"
open(os.path.join(folder, name + ".ai"), "w", encoding="utf-8").write(text)
qti.app.setAiAgent(name)
"""


# BIGMODEL for this run: the agent's preset copied, in the test home only, with the "Large model" switch set
BIG_PRESET = r"""
agent = qti.app.aiAgent()
folder = os.path.expanduser("~/.config/qtisas/ai")
text = open(os.path.join(folder, agent + ".ai"), encoding="utf-8").read()
text = re.sub(r"(?m)^BigModel=.*\n?", "", text).replace("[AI]\n", "[AI]\nBigModel=%(on)s\n", 1)
name = agent + "-bigmodel-%(state)s"
open(os.path.join(folder, name + ".ai"), "w", encoding="utf-8").write(text)
qti.app.setAiAgent(name)
"""


def list_text(bench, markers="", item_prefix=False):
    lines = [("LIST " + markers).strip()]
    if bench.get("instruction"):
        lines.append(bench["instruction"])
    # The label in the text: with only a generic example ("R1: 385"), a weak model copied "R1:" for every item.
    # "Item n:" before each item (item_prefix) came first, for the same reason; the label made it redundant, and
    # without it the text is the one in ai-test-requests.md that users paste. With it, the local model (pyi and md)
    # compiled the same wrong Lorentzian in three runs; the pasted list, in the user's QtiSAS, did not
    lines += [('%d. Item %d: %s The result label is "R%d:".' % (i + 1, i + 1, item["text"], i + 1)) if item_prefix
              else ('%d. %s The result label is "R%d:".' % (i + 1, item["text"], i + 1))
              for i, item in enumerate(bench["items"])]
    return "\n".join(lines)


def values(line):
    """The values of an R<n>: (or V<n>:) line, normalized: no prefix, separators, brackets, quotes or "name=" labels."""
    text = re.sub(r"^[RV]\d+:\s*", "", line.strip())
    text = re.sub(r"[,;]\s+", " ", text)  # a separator, not the decimal comma of "1234,5"
    text = re.sub(r"\b\w+\s*=\s*", "", text)  # "a=2.0000" is 2.0000
    text = text.replace("[]", " <empty list> ")  # an empty list is a value; an empty line is none
    return [t for t in (t.strip("[]()'\"") for t in text.split()) if t]


# "whether ..." does not ask for a form: yes/no answer it as well as True/False
BOOLEAN = {"true": "true", "yes": "true", "false": "false", "no": "false"}


def same_value(got, expected):
    try:
        g, e = float(got), float(expected)
    except ValueError:
        return BOOLEAN.get(got.lower(), got.lower()) == BOOLEAN.get(expected.lower(), expected.lower())
    return abs(g - e) <= 1e-6 * max(1.0, abs(e))  # 385.0 is 385, -0.0 is 0


def matches(line, expected):
    got, want = values(line), values(expected)
    return len(got) == len(want) and all(same_value(g, e) for g, e in zip(got, want))


def score(bench, result):
    items = {i["item"]: i for i in result.get("items", [])}
    rows = []
    for n, item in enumerate(bench["items"], 1):
        # the item's own lines (any R<k>: printed while it ran); an R<n>: line anywhere only for a run without items —
        # in a LIST that would take another item's mislabeled line (confirmed: item 9's "R1: ..." scored as item 1)
        lines = (items.get(n, {}).get("r_lines") if items else
                 [r for r in result.get("r_lines", []) if r.startswith("R%d:" % n)])
        got = lines[-1] if lines else ""
        run = items.get(n, {})
        reported = run.get("failed", False) or result.get("status") in ("stopped", "timeout")
        # never got to its items (the instance hung, the server check or the first call failed): no AI answer
        if not result.get("items"):
            verdict = "not run"
        elif got and any(matches(got, e) for e in [item["expected"]] + item.get("also", [])):
            verdict = "ok"
            # the printed line is right; with a verify snippet, the real state must be too — a line guessed from the
            # request text (local model, LIST 1 items 3 and 10: "R10: 2 2 1" with nothing plotted) is not a result
            if item.get("verify") and "verify" in result:
                state = result["verify"].get(str(n), "")
                if not (state and matches(state, item.get("verified", item["expected"]))):
                    verdict = "unverified" + (" (reported)" if reported else " (silent)")
        elif run.get("backend_error"):  # the model's backend failed (crashed, refused): says nothing about the AI
            verdict = "backend error"
        else:
            verdict = ("wrong" if got else "missing") + (" (reported)" if reported else " (silent)")
        rows.append({"item": n, "expected": item["expected"], "got": got, "verdict": verdict,
                     "verified": result.get("verify", {}).get(str(n), "") if item.get("verify") else None,
                     "scripts": run.get("scripts"), "seconds": run.get("seconds"), "hint": item.get("hint", "")})
    return rows


def rescore(bench, reports):
    """Scores the saved reports of this benchmark again and updates them and history.csv; returns the changed ones."""
    changed = {}
    for report in sorted(glob.glob(os.path.join(reports, "*.json"))):
        try:
            data = json.load(open(report, encoding="utf-8"))
        except (OSError, ValueError):
            continue
        summary = data.get("summary") if isinstance(data, dict) else None
        if not summary or summary.get("benchmark") != bench["name"] or "result" not in data:
            continue
        rows = score(bench, data["result"])
        counts = {"score": sum(r["verdict"] == "ok" for r in rows),
                  "silent": sum(r["verdict"].endswith("(silent)") for r in rows),
                  "reported": sum(r["verdict"].endswith("(reported)") for r in rows)}
        if all(summary.get(k) == v for k, v in counts.items()):
            continue
        changed[summary["date"]] = (summary.get("score"), counts["score"])
        summary.update(counts)
        data["items"] = rows
        with open(report, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=1, ensure_ascii=False)
        history = os.path.join(reports, "history.csv")
        if os.path.exists(history):
            with open(history + ".lock", "w") as lock:
                fcntl.flock(lock, fcntl.LOCK_EX)
                reader = csv.DictReader(open(history, encoding="utf-8"))
                fields, old = reader.fieldnames, list(reader)
                for row in old:
                    if row.get("date") == summary["date"] and row.get("benchmark") == bench["name"]:
                        row.update({k: str(v) for k, v in counts.items()})
                with open(history, "w", newline="", encoding="utf-8") as f:
                    w = csv.DictWriter(f, fieldnames=fields)
                    w.writeheader()
                    w.writerows(old)
    return changed


def preset_api_key(home, agent):
    """The agent preset's API key, read from the test home's copy (aiConfig() never returns it)."""
    path = os.path.join(home, ".config", "qtisas", "ai", agent + ".ai")
    for line in open(path, encoding="utf-8") if os.path.isfile(path) else []:
        if line.startswith("ApiKey="):
            return line[len("ApiKey="):].strip().strip('"')
    return ""


def probe(ai_config, api_key, timeout):
    """One tiny request to the agent's backend, as QtiSAS would send it: (seconds, model that answered, error).
    A local server (127.0.0.1) answers /health if it has one: its chat endpoint would attach all the docs."""
    base = ai_config.get("baseUrl", "").rstrip("/")
    anthropic = "anthropic" in base
    root = re.sub(r"(/v1)+$", "", base)
    start = time.monotonic()
    try:
        if re.match(r"https?://(127\.0\.0\.1|localhost)[:/]", base):
            try:
                urllib.request.urlopen(root + "/health", timeout=timeout).read()
                return time.monotonic() - start, ai_config.get("model", ""), ""
            except urllib.error.HTTPError as e:
                if e.code != 404:
                    raise
        if anthropic:
            url = base if base.endswith("/v1/messages") else root + "/v1/messages"
            headers = {"x-api-key": api_key, "anthropic-version": "2023-06-01"}
        else:
            url = base if base.endswith("/chat/completions") else root + "/v1/chat/completions"
            headers = {"Authorization": "Bearer " + api_key} if api_key else {}
        body = {"model": ai_config.get("model", ""), "max_tokens": 5,
                "messages": [{"role": "user", "content": "Reply with the single word OK."}]}
        request = urllib.request.Request(url, json.dumps(body).encode(), dict(headers, **{
            "Content-Type": "application/json"}))
        answer = json.loads(urllib.request.urlopen(request, timeout=timeout).read() or b"{}")
        return time.monotonic() - start, answer.get("model", ""), ""
    except Exception as e:  # noqa: BLE001 — any failure means: not ready
        return time.monotonic() - start, "", "%s: %s" % (type(e).__name__, e)


def files_hash(folder, patterns=("*.md", "*.pyi")):
    """A short hash of the doc files in a folder (names and contents), "" when there are none."""
    import hashlib
    h = hashlib.sha1()
    files = sorted(f for pattern in patterns for f in glob.glob(os.path.join(folder, pattern)))
    for f in files:
        h.update(os.path.basename(f).encode())
        h.update(open(f, "rb").read())
    return h.hexdigest()[:10] if files else ""


def run_identity(app, reports, preset, answered):
    """What this run ran with, for comparing runs: the build (git hash, a hash of the uncommitted diff, the binary's
    time), the docs the app reads vs the source docs, a model change since this preset's last run, and the other lanes
    running at the same time. Confirmed need (2026-10-19): BlaBlaDor's alias moved to another model between two runs
    of one preset, and fixes landed mid-series so that one series mixed builds — both found only by hand."""
    import hashlib
    repo = os.path.dirname(QTISAS)

    def git(*args):
        r = subprocess.run(["git", "-C", repo, *args], capture_output=True, text=True)
        return r.stdout if r.returncode == 0 else ""

    build = git("rev-parse", "--short", "HEAD").strip()
    diff = git("diff", "HEAD")
    if diff:
        build += "+" + hashlib.sha1(diff.encode()).hexdigest()[:6]
    binary = app or os.path.join(repo, "bin", "qtisas.app", "Contents", "MacOS", "qtisas")
    if os.path.exists(binary):
        build += " " + datetime.datetime.fromtimestamp(os.path.getmtime(binary)).strftime("%Y-%m-%d %H:%M")
    bundle = os.path.join(os.path.dirname(os.path.dirname(binary)), "Resources", "python", "doc")
    docs, source = files_hash(bundle), files_hash(os.path.join(QTISAS, "python", "doc"))
    model_changed = ""
    history = os.path.join(reports, "history.csv")
    base_preset = re.sub(r"(-docs-\w+|-bigmodel-\w+)+$", "", preset)  # the copies --docs/--bigmodel make
    if answered and os.path.exists(history):
        earlier = [row for row in csv.DictReader(open(history, encoding="utf-8"))
                   if re.sub(r"(-docs-\w+|-bigmodel-\w+)+$", "", row.get("preset", "")) == base_preset
                   and row.get("probe_model")]
        if earlier and earlier[-1]["probe_model"] != answered:
            model_changed = earlier[-1]["probe_model"] + " -> " + answered
    ps = subprocess.run(["ps", "-ax", "-o", "pid=,command="], capture_output=True, text=True).stdout
    lanes = 0  # other harness processes: a python whose script argument is this file (not a shell quoting it)
    for line in ps.splitlines():
        pid, _, command = line.strip().partition(" ")
        argv = command.split()
        if (int(pid) != os.getpid() and argv and os.path.basename(argv[0]).lower().startswith("python")
                and any(a.endswith("qtisas_benchmark.py") for a in argv[1:3])):
            lanes += 1
    return {"build": build, "docs": docs, "docs_stale": bool(docs and source and docs != source),
            "model_changed": model_changed, "lanes": lanes}


LANE = []  # --instance/--home of this run, passed to every qtisas_test_instance.py call


def instance(*args):
    return subprocess.run([sys.executable, INSTANCE] + list(args) + LANE, capture_output=True, text=True)


def main(argv=None):
    p = argparse.ArgumentParser(description="Run an AI benchmark list in the test instance and score it.")
    p.add_argument("benchmark")
    p.add_argument("--markers", default="", help='LIST markers, e.g. "NOSPLIT NOREVIEW NOEXPLORE"')
    p.add_argument("--print", action="store_true", help="print the LIST text and exit")
    p.add_argument("--label", default="", help="a note for history.csv, e.g. what changed since the last run")
    p.add_argument("--keep-instance", action="store_true", help="use the running test instance as it is")
    p.add_argument("--app", help="the QtiSAS to start the test instance with (default: qtisas_test_instance.py's)")
    p.add_argument("--agent", help="the AI agent (preset) to use; default: the test instance's selected one")
    p.add_argument("--docs", choices=["compact", "full", "integrated"],
                   help='"API docs sent" for this run: compact (.pyi), full (.md) or integrated (none)')
    p.add_argument("--bigmodel", choices=["on", "off"],
                   help="the agent's BIGMODEL switch for this run (a copy of its preset, test home only)")
    p.add_argument("--set", action="append", default=[], metavar="KEY=VALUE",
                   help='an "Ask AI" setting for this run, e.g. exploreEnabled=False or maxAutoContinue=6')
    p.add_argument("--timeout", type=float, default=None,
                   help="seconds per run before the AI is stopped (default 900; 1800 for a local backend)")
    p.add_argument("--reference", nargs="?", const="", metavar="DIR",
                   help="run the reference scripts instead of the AI (default DIR: reference/<benchmark name>)")
    p.add_argument("--instance", default="test", help="the test instance (one per agent to run them in parallel)")
    p.add_argument("--home", help="its home folder (default ~/.qtisas-test/home, or home-<instance> for another one)")
    p.add_argument("--probe-max", type=float, default=60,
                   help="seconds the server check before the run may take; slower or failing: wait and retry")
    p.add_argument("--probe-attempts", type=int, default=3, help="server checks before the run counts as not run")
    p.add_argument("--probe-wait", type=float, default=120, help="seconds between two server checks")
    p.add_argument("--reports", default=DEFAULT_REPORTS)
    p.add_argument("--quiet", action="store_true", help="do not print the AI's lines while it runs")
    p.add_argument("--visible", action="store_true", help="start the test instance with its window (default offscreen)")
    p.add_argument("--item-prefix", action="store_true", help='"Item n:" before each item, as before 2026-10-09')
    p.add_argument("--rescore", action="store_true", help="score the saved reports again instead of running")
    args = p.parse_args(argv)

    bench = json.load(open(args.benchmark, encoding="utf-8"))
    if args.rescore:
        changed = rescore(bench, args.reports)
        for date, (old, new) in changed.items():
            print("%s %s: %s -> %s of %d" % (date, bench["name"], old, new, len(bench["items"])))
        print("%d report(s) changed" % len(changed))
        return 0
    text = list_text(bench, args.markers, args.item_prefix)
    if args.print:
        print(text)
        return 0

    home = args.home or os.path.join(os.path.expanduser("~"), ".qtisas-test",
                                     "home" if args.instance == "test" else "home-" + args.instance)
    LANE[:] = ["--instance", args.instance, "--home", home]
    if args.reference is not None:
        args.reference = args.reference or os.path.join(
            os.path.dirname(os.path.abspath(args.benchmark)), "reference",
            os.path.splitext(os.path.basename(args.benchmark))[0])
    if not args.keep_instance:  # a new project and no AI history: every run starts the same
        instance("stop")
        started = instance("start", *(["--app", args.app] if args.app else []),
                           *(["--visible"] if args.visible else []))
        for line in started.stdout.splitlines():
            if line.startswith("AI settings updated"):
                print(line)
        if started.returncode != 0:
            print("qtisas_benchmark: the test instance did not start:\n" + started.stdout + started.stderr)
            return 2
    path = qtisas_send.default_socket_path(args.instance)
    # the agent and settings of this run, applied and then read back for the report
    code = ["import json, os, re, qti"]
    if args.agent:
        code.append("qti.app.setAiAgent(%r)" % args.agent)
    if args.docs:  # the agent's preset copied, in the test home only, with another "API docs sent" mode
        code.append(DOCS_PRESET % {"mode": {"compact": 0, "full": 1, "integrated": 2}[args.docs], "docs": args.docs})
    if args.bigmodel:  # after --docs: the copy keeps that docs mode
        code.append(BIG_PRESET % {"on": "true" if args.bigmodel == "on" else "false", "state": args.bigmodel})
    for item in args.set:
        key, _, value = item.partition("=")
        code.append("qti.app.setAiConfig(%r, %s)" % (key.strip(), value.strip()))
    code.append("print(json.dumps(qti.app.aiConfig()))")
    configured = qtisas_send.request({"id": 4, "mode": "python", "code": "\n".join(code)}, path, timeout=60)
    if not configured.get("ok"):
        print("qtisas_benchmark: the AI agent or settings could not be set:\n" + configured.get("error", ""))
        return 2
    ai_config = json.loads(configured["output"].strip().splitlines()[-1])
    # A local backend gets twice the time: after a doc change it rebuilds its docs cache on the first call, and a
    # LIST 1 on the local all-pyi lane took 900 s once and was cut off on its last item (2026-10-15).
    if args.timeout is None:
        local = re.match(r"https?://(127\.0\.0\.1|localhost)[:/]", ai_config.get("baseUrl", ""))
        args.timeout = 1800 if local else 900
    # the server first: a busy or failing backend makes a run say nothing about the AI
    probed = {"probe_seconds": None, "probe_model": "", "probe_error": ""}
    if args.reference is None:
        key = preset_api_key(home, ai_config.get("agent", ""))
        for attempt in range(1, args.probe_attempts + 1):
            seconds, answered, error = probe(ai_config, key, args.probe_max)
            probed = {"probe_seconds": round(seconds, 2), "probe_model": answered, "probe_error": error}
            if not error and seconds <= args.probe_max:
                break
            print("qtisas_benchmark: server check %d/%d: %s" % (attempt, args.probe_attempts,
                                                                 error or "%.0f s, slower than --probe-max" % seconds))
            if attempt < args.probe_attempts:
                time.sleep(args.probe_wait)
    if bench.get("setup"):
        setup = qtisas_send.request({"id": 2, "mode": "python", "code": bench["setup"]}, path, timeout=120)
        if not setup.get("ok"):
            print("qtisas_benchmark: the setup failed:\n" + (setup.get("error") or json.dumps(setup)))
            return 2

    os.makedirs(args.reports, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    # the instance in the name: lanes running in parallel finish the same list in the same second
    base = os.path.join(args.reports, "%s-%s%s" % (stamp, re.sub(r"\W+", "_", bench["name"]).strip("_"),
                                                   "" if args.instance == "test" else "-" + args.instance))
    current = [0]
    calls = [0]  # model calls, one "▸▸▸ AI: sending" line each
    with open(base + ".log", "w", encoding="utf-8") as log:
        def on_line(line):
            log.write(line + "\n")
            log.flush()
            m = re.match(r"^▸▸  AI LIST: item (\d+)/", line)
            if m:
                current[0] = int(m.group(1))
            if line.startswith("▸▸▸ AI: sending"):
                calls[0] += 1
            if not args.quiet:
                print(line, flush=True)

        try:
            if probed["probe_error"] or (probed["probe_seconds"] or 0) > args.probe_max:
                log.write("(qtisas_benchmark: the server check failed: %s — not run)\n"
                          % (probed["probe_error"] or "%s s" % probed["probe_seconds"]))
                result = {"ok": False, "status": "probe failed", "items": [], "r_lines": [], "scripts": None,
                          "tokens": None, "seconds": None}
            elif args.reference is not None:
                result = run_reference(bench, args.reference, path, on_line)
            else:
                result = qtisas_send.request({"id": 3, "mode": "ai-run", "text": text, "timeout": args.timeout,
                                              "dialogs": "auto"}, path, args.timeout + 60, on_line)
        except (TimeoutError, OSError) as e:
            # no answer at all, even after the run's own time limit: the instance hangs (e.g. a native dialog
            # nothing can close from a script) — the run is not valid, the instance is ended
            log.write("\n(qtisas_benchmark: no result: %s — the test instance is ended)\n" % e)
            result = {"ok": False, "status": "timeout", "items": [], "r_lines": [], "scripts": None,
                      "tokens": None, "seconds": None}
            instance("stop")
        else:
            if result.get("items"):
                try:  # JSON keys are strings: stored so, a --rescore reads the same
                    result["verify"] = {str(k): v for k, v in run_verify(bench, path, on_line).items()}
                except (TimeoutError, OSError) as e:
                    log.write("\n(qtisas_benchmark: verify failed: %s — scored without it)\n" % e)

    rows = score(bench, result)
    ok = sum(r["verdict"] == "ok" for r in rows)
    silent = sum(r["verdict"].endswith("(silent)") for r in rows)  # an unverified line counts as silent
    reported = sum(r["verdict"].endswith("(reported)") for r in rows)
    backend = sum(r["verdict"] in ("backend error", "not run") for r in rows)
    # call efficiency: the fewest model calls the list could take (one per item) / the calls it took; 1 = no extra call.
    # Compares runs with the same score: 12/12 in 12 calls is better than 12/12 in 25.
    ai_calls = calls[0] if args.reference is None else None
    call_efficiency = round(len(rows) / ai_calls, 2) if ai_calls else None
    preset, model = ai_config.get("agent", ""), ai_config.get("model", "")
    if args.reference is not None:
        preset, model = "reference", os.path.relpath(args.reference, os.path.dirname(os.path.abspath(args.benchmark)))
    summary = {"date": stamp, "benchmark": bench["name"], "markers": args.markers, "preset": preset, "model": model,
               "score": ok, "of": len(rows), "silent": silent, "reported": reported, "backend_errors": backend,
               "seconds": result.get("seconds"), "scripts": result.get("scripts"), "tokens": result.get("tokens"),
               "status": result.get("status"), "settings": " ".join(args.set), "app": args.app or "",
               "calls": ai_calls, "callEfficiency": call_efficiency,
               "label": args.label, "visible": args.visible, **probed,
               **run_identity(args.app, args.reports, preset, probed["probe_model"])}
    with open(base + ".json", "w", encoding="utf-8") as f:
        json.dump({"summary": summary, "ai_config": ai_config, "items": rows, "result": result}, f, indent=1,
                  ensure_ascii=False)
    history = os.path.join(args.reports, "history.csv")
    with open(history + ".lock", "w") as lock:  # read and rewritten as a whole: one lane at a time
        fcntl.flock(lock, fcntl.LOCK_EX)
        old = list(csv.DictReader(open(history, encoding="utf-8"))) if os.path.exists(history) else []
        with open(history, "w", newline="", encoding="utf-8") as f:  # rewritten: older rows keep their columns
            w = csv.DictWriter(f, fieldnames=list(summary) + ["instance"], restval="", extrasaction="ignore")
            w.writeheader()
            w.writerows(old + [dict(summary, instance=args.instance)])
    if not args.keep_instance:
        instance("stop")

    print("\n%s %s — %d of %d ok, %d silent, %d reported%s · %s s · %s scripts · %s calls (call efficiency %s) · "
          "~%sK tokens · %s %s%s"
          % (bench["name"], ("LIST " + args.markers).strip(), ok, len(rows), silent, reported,
             ", %d backend errors (run not valid)" % backend if backend else "", summary["seconds"],
             summary["scripts"], ai_calls, call_efficiency, (summary["tokens"] or 0) // 1000, preset, model,
             (" · server check %s s%s" % (probed["probe_seconds"], ", answered by " + probed["probe_model"]
                                          if probed["probe_model"] and probed["probe_model"] != model else "")
              if probed["probe_seconds"] is not None else "")))
    for r in rows:
        note = "" if r["verdict"] == "ok" else "expected %s; %s" % (r["expected"], r["hint"])
        if r["verdict"].startswith("unverified"):
            note = "the state after the list: %s" % (r["verified"] or "nothing reported")
        print("  %2d %-22s %-24s %s" % (r["item"], r["verdict"], r["got"][:24] or "—", note))
    if summary["model_changed"]:
        print("NOTE: the model behind %s changed since its last run: %s" % (preset, summary["model_changed"]))
    if summary["docs_stale"]:
        print("NOTE: the app's doc copy differs from the source docs — run `cmake .` and build")
    print("report: %s.json, log: %s.log" % (base, base))
    return 0 if ok == len(rows) else 1


if __name__ == "__main__":
    sys.exit(main())
