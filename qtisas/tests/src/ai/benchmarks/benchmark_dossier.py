# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: the story of every lost benchmark item (dossier) and a store of failure signatures across runs
"""Lost-item dossiers and failure signatures for qtisas_benchmark.py runs.

A dossier is what a run's log says about one item: its last script, its errors, the checks that blocked it, the
repairs, the doc topics it was given, its model calls and plan, and why it stopped. A signature is a short, normalised
key of a failure (names and numbers replaced), so the same failure in other runs, items, lists or models counts
together: failures.jsonl next to history.csv holds one line per lost item of every run.
"""

import glob
import json
import os
import re

ITEM_RE = re.compile(r"^▸▸  AI LIST: item (\d+)/\d+")
END_RE = re.compile(r"^▸▸  (?:verify:|AI LIST: all)")
R_LINE_RE = re.compile(r"^R\d+:")


def item_sections(log_text):
    """{item number: [its log lines]} — from its "AI LIST: item n/N" line up to the next item or the verify."""
    sections, current = {}, None
    for line in log_text.splitlines():
        m = ITEM_RE.match(line)
        if m:
            current = int(m.group(1))
            sections[current] = [line]
        elif END_RE.match(line):
            current = None
        elif current is not None:
            sections[current].append(line)
    return sections


def _block_after(lines, start):
    """The lines after lines[start] up to the next status line ("▸▸"), result line or blank run of two."""
    out, blanks = [], 0
    for line in lines[start + 1:]:
        if line.startswith("▸▸") or R_LINE_RE.match(line):
            break
        blanks = blanks + 1 if not line.strip() else 0
        if blanks >= 2 and out:
            break
        out.append(line)
    while out and not out[-1].strip():
        out.pop()
    while out and not out[0].strip():
        out.pop(0)
    return out


def dossier(lines):
    """What the log says about one item."""
    d = {"scripts": 0, "calls": 0, "last_script": "", "errors": [], "blocks": [], "repairs": [], "stopped": "",
         "doc_topics": "", "plan_parts": None, "replans": 0, "connection_errors": 0, "r_lines": []}
    for i, line in enumerate(lines):
        if line.startswith("▸▸▸ AI code ("):
            d["scripts"] += 1
            d["last_script"] = "\n".join(_block_after(lines, i))
        elif line.startswith("▸▸▸ Python script Error"):
            body = [l for l in _block_after(lines, i) if l.strip()]
            if body and (not d["errors"] or d["errors"][-1] != body[0]):
                d["errors"].append(body[0])
        elif line.startswith("▸▸▸ AI: why it was blocked"):
            bullets = [l.strip()[1:].strip() for l in _block_after(lines, i) if l.strip().startswith("•")]
            d["blocks"] += [b for b in bullets if b not in d["blocks"]]
        elif line.startswith("▸▸▸ AI: auto-corrected") or line.startswith("▸▸▸ AI: removed") or \
                line.startswith("▸▸▸ AI: closed what the failed attempt created"):
            d["repairs"].append(line[len("▸▸▸ AI: "):].strip())
        elif line.startswith("▸▸▸ AI: stopping") or line.startswith("▸▸▸ AI: script failed again"):
            d["stopped"] = line[len("▸▸▸ AI: "):].strip()
        elif line.startswith("▸▸▸ AI: sending"):
            d["calls"] += 1
            m = re.search(r"docs: (.+)$", line)
            if m:
                d["doc_topics"] = m.group(1).strip()
        elif line.startswith("▸▸▸ AI: connection error"):
            d["connection_errors"] += 1
        elif "issue(s) detected" in line:
            d["replans"] += 1
        elif R_LINE_RE.match(line):
            d["r_lines"].append(line)
        m = re.match(r"^▸▸▸ AI split-plan \((\d+) part", line)
        if m:
            d["plan_parts"] = int(m.group(1))
    d["last_script"] = d["last_script"][:4000]
    return d


def normalise(text):
    """Names and numbers replaced, so the same failure on other data reads the same."""
    text = re.sub(r"'[^']*'|\"[^\"]*\"", "<s>", text)
    text = re.sub(r"\b\d+(?:\.\d+)?(?:[eE][-+]?\d+)?\b", "<n>", text)
    text = re.sub(r"\bat <ai>:<n>.*$", "", text)
    return re.sub(r"\s+", " ", text).strip()[:160]


def signature(verdict, d, item_key):
    """A failure's key: the give-up, the last error or the first block when the log has one; otherwise the verdict
    and the item (a silent wrong value has nothing else to compare by)."""
    if verdict in ("backend error", "not run"):
        return verdict
    if d["stopped"]:
        return "gave up: " + normalise(d["stopped"])
    if d["errors"]:
        return "error: " + normalise(d["errors"][-1])
    if d["blocks"]:
        return "blocked: " + normalise(d["blocks"][0])
    kind = verdict.split(" (")[0]
    return "%s: %s" % (kind, item_key)


def item_key(benchmark, item):
    return "%s·%s" % (benchmark.replace("LIST ", "L"), item)


def dossiers(log_path, rows):
    """{item number: dossier} for the rows that are not ok."""
    try:
        text = open(log_path, encoding="utf-8", errors="replace").read()
    except OSError:
        return {}
    sections = item_sections(text)
    return {r["item"]: dossier(sections.get(r["item"], [])) for r in rows if r["verdict"] != "ok"}


def failure_records(report_path):
    """One record per lost item of a saved report (its log next to it), for failures.jsonl."""
    data = json.load(open(report_path, encoding="utf-8"))
    s = data.get("summary") or {}
    rows = data.get("items", [])
    found = {int(k): v for k, v in (data.get("dossiers") or {}).items()} or dossiers(report_path[:-5] + ".log", rows)
    out = []
    for r in rows:
        if r["verdict"] == "ok":
            continue
        d = found.get(r["item"]) or dossier([])
        key = item_key(s.get("benchmark", ""), r["item"])
        out.append({"date": s.get("date"), "benchmark": s.get("benchmark"), "item": r["item"], "key": key,
                    "label": s.get("label", ""), "preset": s.get("preset", ""), "model": s.get("probe_model") or
                    s.get("model", ""), "build": s.get("build", ""), "verdict": r["verdict"],
                    "signature": signature(r["verdict"], d, key), "expected": r.get("expected"),
                    "got": r.get("got"), "error": (d["errors"] or [""])[-1], "block": (d["blocks"] or [""])[0],
                    "stopped": d["stopped"], "report": os.path.basename(report_path)})
    return out


def append_failures(reports, report_path):
    """Adds a run's lost items to failures.jsonl (a run already there is replaced)."""
    store = os.path.join(reports, "failures.jsonl")
    name = os.path.basename(report_path)
    old = []
    if os.path.exists(store):
        old = [l for l in open(store, encoding="utf-8") if l.strip() and json.loads(l).get("report") != name]
    with open(store, "w", encoding="utf-8") as f:
        f.writelines(old)
        for rec in failure_records(report_path):
            f.write(json.dumps(rec, ensure_ascii=False) + "\n")


def rebuild_failures(reports):
    """failures.jsonl from every saved report; returns the number of lost items."""
    store = os.path.join(reports, "failures.jsonl")
    n = 0
    with open(store, "w", encoding="utf-8") as f:
        for path in sorted(glob.glob(os.path.join(reports, "*.json"))):
            try:
                records = failure_records(path)
            except (OSError, ValueError, KeyError, TypeError):
                continue
            for rec in records:
                f.write(json.dumps(rec, ensure_ascii=False) + "\n")
                n += 1
    return n


def load_failures(reports):
    store = os.path.join(reports, "failures.jsonl")
    if not os.path.exists(store):
        return []
    return [json.loads(l) for l in open(store, encoding="utf-8") if l.strip()]
