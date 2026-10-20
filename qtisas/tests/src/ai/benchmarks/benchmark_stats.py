#!/usr/bin/env python3
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: statistics of qtisas_benchmark.py runs — score, time, calls and tokens per label and lane
"""Statistics of qtisas_benchmark.py runs, compared label by label.

    benchmark_stats.py LABEL [LABEL ...] [--reports DIR] [--cached-price 0.1] [--items]
    benchmark_stats.py --failures [TEXT] [--item KEY] [--since YYYYMMDD]   # seen before? (failures.jsonl)
    benchmark_stats.py --rebuild-failures                                  # failures.jsonl from every report

A LABEL selects the runs whose --label starts with it (e.g. "BIGMODEL fixes 2026-10-15"). Runs are grouped by
label and lane (the agent preset; the copies --docs/--bigmodel make count as their preset). A round is one exact
label with every list of the group; only complete rounds count. Printed as markdown: score, time and tokens as
separate tables, then the failed items, then warnings (mixed builds, a model change, parallel lanes).

Tokens come from the run logs' "AI cache:" lines (what the provider reported per call): prompt, read from the
cache, not cached, and a billable equivalent (not cached + cached × --cached-price, the provider's discount for
cached input). A provider that reports no cache figures shows its prompt tokens and "not reported".
"""

import argparse
import glob
import json
import os
import re
import statistics
import sys
from collections import defaultdict

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import benchmark_dossier  # noqa: E402

DEFAULT_REPORTS = os.path.join(os.path.expanduser("~"), ".qtisas-test", "reports")
CACHE_RE = re.compile(r"AI cache: (\d+) of (\d+) prompt tokens read from the cache")
NO_CACHE_RE = re.compile(r"AI cache: (\d+) prompt tokens; the provider does not report cache use")


def preset_base(preset):
    """The preset a run's agent was copied from: --docs/--bigmodel add "-docs-…"/"-bigmodel-…" to its name."""
    return re.sub(r"(-docs-\w+|-bigmodel-\w+)+$", "", preset or "")


def lane_of(summary):
    """The preset, then the docs a call got when the run recorded them: "(pyi)"/"(md)" and the doc sets' tiers — runs
    whose docs differ are separate lanes, not rounds of one."""
    docs = {"compact": "pyi", "full": "md"}.get(summary.get("docs_mode") or "", summary.get("docs_mode") or "")
    tiers = summary.get("doc_tiers") or ""
    extra = ", ".join(x for x in (docs, tiers) if x)
    return preset_base(summary.get("preset")) + (" (%s)" % extra if extra else "")


def log_tokens(log_path):
    """(prompt, cached, reported) summed over a run's "AI cache:" lines; reported is False when no line had figures."""
    prompt = cached = 0
    reported = False
    try:
        text = open(log_path, encoding="utf-8", errors="replace").read()
    except OSError:
        return 0, 0, False
    for c, p in CACHE_RE.findall(text):
        prompt += int(p)
        cached += int(c)
        reported = True
    for p in NO_CACHE_RE.findall(text):
        prompt += int(p)
    return prompt, cached, reported


def load_runs(reports, prefixes):
    """{prefix: [run, ...]}: every report whose label starts with one of the prefixes (the longest one wins)."""
    runs = defaultdict(list)
    for path in sorted(glob.glob(os.path.join(reports, "*.json"))):
        try:
            data = json.load(open(path, encoding="utf-8"))
        except (OSError, ValueError):
            continue
        summary = data.get("summary") if isinstance(data, dict) else None
        if not summary:
            continue
        label = summary.get("label") or ""
        matching = [p for p in prefixes if label.startswith(p)]
        if not matching:
            continue
        prompt, cached, reported = log_tokens(path[:-5] + ".log")
        runs[max(matching, key=len)].append({
            "label": label, "lane": lane_of(summary), "list": summary.get("benchmark"),
            "summary": summary, "items": data.get("items", []),
            "prompt": prompt, "cached": cached, "cache_reported": reported})
    return runs


def rounds_of(runs):
    """{lane: [[run, ...] per complete round]}: a round is one exact label holding every list the group ran (a lane
    stopped after LIST 1 has no complete round)."""
    lists = {run["list"] for run in runs}
    by_lane = defaultdict(lambda: defaultdict(list))
    for run in runs:
        by_lane[run["lane"]][run["label"]].append(run)
    return {lane: [rnd for rnd in labels.values() if {run["list"] for run in rnd} == lists]
            for lane, labels in by_lane.items()}


def fmt_m(n):
    return "%.2fM" % (n / 1e6) if n >= 1e5 else "%.0fK" % (n / 1e3)


def main(argv=None):
    p = argparse.ArgumentParser(description="Statistics of qtisas_benchmark.py runs, compared label by label.")
    p.add_argument("labels", nargs="*", help="label prefixes, one group each")
    p.add_argument("--failures", nargs="?", const="", help="failure signatures (containing TEXT), with counts")
    p.add_argument("--item", help='with --failures: one item, e.g. "L2·5" or "ASCII1D·3"')
    p.add_argument("--since", default="", help="with --failures: runs from this date on (YYYYMMDD)")
    p.add_argument("--rebuild-failures", action="store_true", help="rebuild failures.jsonl from every report")
    p.add_argument("--reports", default=DEFAULT_REPORTS)
    p.add_argument("--cached-price", type=float, default=0.1,
                   help="price of a cached prompt token relative to an uncached one (default 0.1)")
    p.add_argument("--items", action="store_true", help="also list every failed item of every run")
    args = p.parse_args(argv)
    if args.rebuild_failures:
        print("failures.jsonl: %d lost items" % benchmark_dossier.rebuild_failures(args.reports))
        return 0
    if args.failures is not None:
        return failures_table(args)
    if not args.labels:
        p.error("give label prefixes, --failures or --rebuild-failures")

    groups = load_runs(args.reports, args.labels)
    rows = []  # (label, lane, rounds)
    for label in args.labels:
        for lane, rounds in sorted(rounds_of(groups.get(label, [])).items()):
            if rounds:
                rows.append((label, lane, rounds))
    if not rows:
        print("no complete round for", ", ".join(repr(l) for l in args.labels))
        return 1

    print("## Score\n")
    print("| Label | Lane | Rounds | Score (mean, min–max) | % | Silent / round | Reported / round "
          "| Backend errors |")
    print("|---|---|---|---|---|---|---|---|")
    for label, lane, rounds in rows:
        scores = [sum(r["summary"]["score"] for r in rnd) for rnd in rounds]
        of = sum(r["summary"]["of"] for r in rounds[0])
        per = lambda key: statistics.mean(sum(r["summary"].get(key) or 0 for r in rnd) for rnd in rounds)
        print("| %s | %s | %d | %.1f/%d (%d–%d) | %.1f%% | %.1f | %.1f | %.1f |"
              % (label, lane, len(rounds), statistics.mean(scores), of, min(scores), max(scores),
                 100 * statistics.mean(scores) / of, per("silent"), per("reported"), per("backend_errors")))

    print("\n## Time and calls (per round)\n")
    print("| Label | Lane | Time | Calls | Call efficiency |")
    print("|---|---|---|---|---|")
    for label, lane, rounds in rows:
        seconds = statistics.mean(sum(r["summary"].get("seconds") or 0 for r in rnd) for rnd in rounds)
        calls = statistics.mean(sum(r["summary"].get("calls") or 0 for r in rnd) for rnd in rounds)
        items = sum(r["summary"]["of"] for r in rounds[0])
        print("| %s | %s | %.1f min | %.0f | %s |" % (label, lane, seconds / 60, calls,
                                                     "%.2f" % (items / calls) if calls else "—"))

    print("\n## Tokens (per round, as the provider reported them)\n")
    print("| Label | Lane | Prompt | Read from cache | Not cached | Cached share | Billable equivalent |")
    print("|---|---|---|---|---|---|---|")
    for label, lane, rounds in rows:
        prompt = statistics.mean(sum(r["prompt"] for r in rnd) for rnd in rounds)
        cached = statistics.mean(sum(r["cached"] for r in rnd) for rnd in rounds)
        if any(r["cache_reported"] for rnd in rounds for r in rnd):
            print("| %s | %s | %s | %s | %s | %.1f%% | %s |"
                  % (label, lane, fmt_m(prompt), fmt_m(cached), fmt_m(prompt - cached),
                     100 * cached / prompt if prompt else 0, fmt_m(prompt - cached + args.cached_price * cached)))
        else:
            print("| %s | %s | %s | not reported | — | — | — |"
                  % (label, lane, fmt_m(prompt) if prompt else "—"))

    print("\n## Failed items\n")
    print("| Label | Lane | Item | Failed | Verdicts |")
    print("|---|---|---|---|---|")
    any_failed = False
    for label, lane, rounds in rows:
        failed = defaultdict(list)
        for rnd in rounds:
            for run in rnd:
                for item in run["items"]:
                    if item.get("verdict") != "ok":
                        failed["%s·%s" % (run["list"].replace("LIST ", "L"), item["item"])].append(item["verdict"])
        for key in sorted(failed, key=lambda k: (-len(failed[k]), k)):
            any_failed = True
            kinds = ", ".join("%s ×%d" % (v, failed[key].count(v)) for v in sorted(set(failed[key])))
            print("| %s | %s | %s | %d/%d | %s |" % (label, lane, key, len(failed[key]), len(rounds), kinds))
    if not any_failed:
        print("| — | — | — | none | — |")

    if args.items:
        print("\n## Every failed item\n")
        for label, lane, rounds in rows:
            for rnd in rounds:
                for run in sorted(rnd, key=lambda r: r["list"]):
                    for item in run["items"]:
                        if item.get("verdict") != "ok":
                            print("- %s · %s · %s item %s: %s — expected %s, got %s"
                                  % (run["summary"]["date"], lane, run["list"], item["item"], item["verdict"],
                                     item.get("expected"), item.get("got") or "nothing"))

    warnings = []
    for label, lane, rounds in rows:
        runs = [r for rnd in rounds for r in rnd]
        # the binary's time is the build that ran; the git part also changes with a test or doc edit
        builds = {r["summary"]["build"].partition(" ")[2] for r in runs if " " in (r["summary"].get("build") or "")}
        if len(builds) > 1:
            warnings.append("%s / %s: %d binaries — %s" % (label, lane, len(builds), ", ".join(sorted(builds))))
        models = {r["summary"].get("probe_model") for r in runs if r["summary"].get("probe_model")}
        if len(models) > 1:
            warnings.append("%s / %s: answered by %d models — %s"
                            % (label, lane, len(models), ", ".join(sorted(models))))
        changed = [r["summary"]["model_changed"] for r in runs if r["summary"].get("model_changed")]
        if changed:
            warnings.append("%s / %s: model changed — %s" % (label, lane, "; ".join(sorted(set(changed)))))
        stale = sum(1 for r in runs if r["summary"].get("docs_stale"))
        if stale:
            warnings.append("%s / %s: %d run(s) with the app's docs differing from the source" % (label, lane, stale))
        parallel = sum(1 for r in runs if r["summary"].get("lanes"))
        if parallel:
            warnings.append("%s / %s: %d of %d runs beside other lanes — times are not comparable"
                            % (label, lane, parallel, len(runs)))
    if warnings:
        print("\n## Warnings\n")
        for w in warnings:
            print("- " + w)
    return 0


def model_name(record):
    """A short name for who answered: "01 - MiniMax-M2.7 - our best ..." -> "MiniMax-M2.7"; else the preset."""
    parts = [p.strip() for p in (record.get("model") or "").split(" - ") if p.strip()]
    if len(parts) > 1:
        return parts[1][:28]
    return (parts[0] if parts else preset_base(record.get("preset")))[:28]


def failures_table(args):
    """Each failure signature with how often, where and with which models it occurred."""
    records = [r for r in benchmark_dossier.load_failures(args.reports)
               if args.failures.lower() in (r["signature"] + " " + r["key"]).lower()
               and (not args.item or r["key"] == args.item) and (r.get("date") or "") >= args.since]
    if not records:
        print("no failure matches")
        return 1
    groups = defaultdict(list)
    for r in records:
        groups[r["signature"]].append(r)
    print("| Signature | Times | Items | Models | First | Last |")
    print("|---|---|---|---|---|---|")
    for sig, rs in sorted(groups.items(), key=lambda kv: -len(kv[1])):
        items = sorted({r["key"] for r in rs})
        models = sorted({model_name(r) for r in rs})
        dates = sorted(r.get("date") or "" for r in rs)
        print("| %s | %d | %s | %s | %s | %s |" % (sig.replace("|", "/"), len(rs), ", ".join(items[:6]) +
                                                 (" …" if len(items) > 6 else ""), ", ".join(models),
                                                 dates[0][:8], dates[-1][:8]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
