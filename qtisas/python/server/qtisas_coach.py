# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2026 Vitaliy Pipich <v.pipich@gmail.com>
# Description: coach tools of the MCP server: benchmark runs, their summaries, failure history, probes, environment
# **************************************************************************** #
"""Coach tools for qtisas_mcp.py: an IDE AI supervising QtiSAS's built-in AI runs a benchmark list in a test
instance (qtisas_run, returns at once), waits for it (qtisas_wait: one event, below the client's time limit), reads its
summary with the lost-item dossiers (qtisas_summary), asks whether a failure was seen before (qtisas_failures),
compares runs (qtisas_stats), checks an API claim on a side instance (qtisas_probe, qtisas_instance) and sees what it
runs with (qtisas_environment). Everything goes through the benchmark harness of the source tree
(tests/src/ai/benchmarks), so a coach run and a harness run are the same run; in an app bundle without the tests the
tools are not offered.
"""

import contextlib
import glob
import io
import json
import os
import re
import subprocess
import sys
import tempfile
import threading
import time
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
QTISAS = os.path.normpath(os.path.join(HERE, "..", ".."))
BENCH = os.path.join(QTISAS, "tests", "src", "ai", "benchmarks")
INSTANCE_TOOL = os.path.join(QTISAS, "tests", "src", "server", "qtisas_test_instance.py")
REPORTS = os.path.join(os.path.expanduser("~"), ".qtisas-test", "reports")
WAIT_LIMIT = 1500  # below the MCP clients' own 1800 s
LOCAL_PORTS = {8000: "md", 8002: "pyi", 8004: "md-pyi", 8006: "all-pyi", 8008: "all-md", 8010: "no-doc"}


def available():
    return os.path.isfile(os.path.join(BENCH, "qtisas_benchmark.py"))


def _schema(props, required=()):
    return {"type": "object", "properties": props, "required": list(required)}


TOOLS = [
    {"name": "qtisas_run",
     "description": "Start a benchmark list (tests/src/ai/benchmarks/<benchmark>.json) in a fresh test instance, as "
                    "qtisas_benchmark.py does; returns a run id at once — wait for it with qtisas_wait. One run per "
                    "instance at a time; lanes in parallel use different instances.",
     "inputSchema": _schema({
         "benchmark": {"type": "string", "description": 'the list, e.g. "list2" or "ascii1d"'},
         "agent": {"type": "string", "description": "the AI agent preset, e.g. QtiSAS-Codex-Bridge-BIGMODEL"},
         "instance": {"type": "string", "description": 'test instance, default "coach"'},
         "docs": {"type": "string", "description": '"compact" (.pyi), "full" (.md) or "integrated"'},
         "bigmodel": {"type": "string", "description": '"on" or "off"'},
         "markers": {"type": "string", "description": 'LIST markers, e.g. "SPLIT" or "NOEXPLORE"'},
         "label": {"type": "string", "description": "a note for history.csv"},
         "reference": {"type": "boolean", "description": "run the reference scripts instead of the AI"}},
         ["benchmark"])},
    {"name": "qtisas_wait",
     "description": "Wait for a run started with qtisas_run: returns done (with the summary) or timeout (still "
                    "running — call again). timeout in seconds, at most 1500.",
     "inputSchema": _schema({"run_id": {"type": "string"}, "timeout": {"type": "number"}}, ["run_id"])},
    {"name": "qtisas_summary",
     "description": "The summary of a run (by run id, or a report file name): score, calls, tokens, run identity, "
                    "every lost item with its dossier (last script, errors, blocks, doc topics, why it stopped) and "
                    "how often its failure signature was seen before.",
     "inputSchema": _schema({"run_id": {"type": "string"}, "report": {"type": "string"}})},
    {"name": "qtisas_failures",
     "description": "Was this failure seen before? Failure signatures of all benchmark runs (failures.jsonl) "
                    "containing text, with counts, items, models and dates.",
     "inputSchema": _schema({"text": {"type": "string"}, "item": {"type": "string", "description": 'e.g. "L2·5"'},
                             "since": {"type": "string", "description": "YYYYMMDD"}})},
    {"name": "qtisas_stats",
     "description": "Compare runs by label prefix: score, time and calls, tokens (prompt, cached, billable), failed "
                    "items and warnings (mixed builds, model changes, parallel lanes).",
     "inputSchema": _schema({"labels": {"type": "array", "items": {"type": "string"}}}, ["labels"])},
    {"name": "qtisas_probe",
     "description": "Run Python on a side test instance (started if needed, never a benchmark's instance) — to check "
                    "what an API call really does before writing it into a doc.",
     "inputSchema": _schema({"code": {"type": "string"}, "instance": {"type": "string",
                                                                    "description": 'default "probe"'}}, ["code"])},
    {"name": "qtisas_instance",
     "description": "Start, stop or show a test instance.",
     "inputSchema": _schema({"action": {"type": "string", "description": '"start", "stop" or "status"'},
                             "name": {"type": "string"}}, ["action", "name"])},
    {"name": "qtisas_environment",
     "description": "What runs would run with: the build (git hash, uncommitted changes, binary time), the app's doc "
                    "copy vs the source docs, the local AI backends and llama-server, and the benchmark runs going "
                    "on.",
     "inputSchema": _schema({})},
]


def _home(instance):
    return os.path.join(os.path.expanduser("~"), ".qtisas-test", "home" if instance == "test" else "home-" + instance)


def _bench_module(name):
    if BENCH not in sys.path:
        sys.path.insert(0, BENCH)
    sys.dont_write_bytecode = True
    return __import__(name)


class Coach:
    def __init__(self, text):
        self.text = text  # the MCP server's result builder
        self.runs = {}
        self.lock = threading.Lock()

    # ---- runs ----

    def run(self, benchmark, agent=None, instance="coach", docs=None, bigmodel=None, markers=None, label=None,
            reference=False):
        name = benchmark[:-5] if benchmark.endswith(".json") else benchmark
        path = os.path.join(BENCH, name + ".json")
        if not os.path.isfile(path):
            lists = sorted(os.path.basename(p)[:-5] for p in glob.glob(os.path.join(BENCH, "*.json")))
            return self.text("no benchmark %r — the lists: %s" % (name, ", ".join(lists)), error=True)
        with self.lock:
            busy = [r for r in self.runs.values() if r["instance"] == instance and r["proc"].poll() is None]
            if busy:
                return self.text("instance %r is running %s — wait for it or use another instance"
                                 % (instance, busy[0]["id"]), error=True)
            cmd = [sys.executable, os.path.join(BENCH, "qtisas_benchmark.py"), path, "--instance", instance]
            for flag, value in (("--agent", agent), ("--docs", docs), ("--bigmodel", bigmodel),
                                ("--markers", markers), ("--label", label)):
                if value:
                    cmd += [flag, value]
            if reference:
                cmd.append("--reference")
            run_id = "%s-%s-%s" % (time.strftime("%H%M%S"), name, instance)
            out = os.path.join(tempfile.gettempdir(), "qtisas-coach-%s.out" % run_id)
            proc = subprocess.Popen(cmd + ["--quiet"], stdout=open(out, "w"), stderr=subprocess.STDOUT,
                                    stdin=subprocess.DEVNULL, cwd=BENCH, start_new_session=True)
            self.runs[run_id] = {"id": run_id, "proc": proc, "out": out, "instance": instance,
                                 "started": time.time(), "benchmark": name}
        return self.text("started %s: %s in instance %r — wait with qtisas_wait(run_id=%r)"
                         % (run_id, name, instance, run_id))

    def wait(self, run_id, timeout=600):
        r = self.runs.get(run_id)
        if not r:
            return self.text("no run %r in this session (runs: %s)" % (run_id, ", ".join(self.runs) or "none"),
                             error=True)
        deadline = time.time() + min(max(float(timeout or 0), 0), WAIT_LIMIT)
        while r["proc"].poll() is None and time.time() < deadline:
            time.sleep(2)
        if r["proc"].poll() is None:
            return self.text("timeout: %s still running after %.0f s — call qtisas_wait again"
                             % (run_id, time.time() - r["started"]))
        return self.summary(run_id=run_id)

    def _report_of(self, run_id):
        r = self.runs[run_id]
        text = open(r["out"], encoding="utf-8", errors="replace").read()
        m = re.search(r"^report: (\S+\.json)", text, re.MULTILINE)
        return (m.group(1) if m else None), text

    def summary(self, run_id=None, report=None):
        output = ""
        if run_id:
            if run_id not in self.runs:
                return self.text("no run %r in this session" % run_id, error=True)
            report, output = self._report_of(run_id)
            if not report:
                return self.text("run %s ended without a report:\n%s" % (run_id, output[-3000:]), error=True)
        elif report:
            report = report if os.path.isabs(report) else os.path.join(REPORTS, report)
        else:
            return self.text("give run_id or report", error=True)
        data = json.load(open(report, encoding="utf-8"))
        s = data["summary"]
        identity = {k: s.get(k) for k in ("build", "docs", "docs_stale", "model_changed", "lanes") if k in s}
        head = ["%s %s — %d of %d ok, %d silent, %d reported, %d backend errors · %s s · %s calls (efficiency %s) · "
                "%s %s" % (s.get("benchmark"), ("LIST " + (s.get("markers") or "")).strip(), s["score"], s["of"],
                           s.get("silent") or 0, s.get("reported") or 0, s.get("backend_errors") or 0,
                           s.get("seconds"), s.get("calls"), s.get("callEfficiency"), s.get("preset"),
                           s.get("probe_model") or s.get("model")),
                "run identity: " + json.dumps(identity, ensure_ascii=False), "report: " + report]
        seen = {}
        dossier = _bench_module("benchmark_dossier")
        for rec in dossier.load_failures(REPORTS):
            if rec.get("report") != os.path.basename(report):
                seen[rec["signature"]] = seen.get(rec["signature"], 0) + 1
        found = data.get("dossiers")
        if found is None:  # a report from before the dossiers: read them from its log
            found = {str(k): v for k, v in dossier.dossiers(report[:-5] + ".log", data.get("items", [])).items()}
        lost = []
        for row in data.get("items", []):
            if row["verdict"] == "ok":
                continue
            d = found.get(str(row["item"])) or {}
            sig = dossier.signature(row["verdict"], d or dossier.dossier([]),
                                    dossier.item_key(s.get("benchmark", ""), row["item"]))
            lost.append("\n".join(filter(None, [
                "item %d: %s — expected %s, got %s" % (row["item"], row["verdict"], row["expected"],
                                                      row.get("got") or "nothing"),
                "  signature: %s (seen %d× before)" % (sig, seen.get(sig, 0)),
                "  doc topics: %s · calls %s · scripts %s" % (d.get("doc_topics"), d.get("calls"), d.get("scripts"))
                if d else "",
                "  errors: " + " | ".join(d["errors"][-2:]) if d.get("errors") else "",
                "  blocked: " + " | ".join(b[:200] for b in d["blocks"][:2]) if d.get("blocks") else "",
                "  stopped: " + d["stopped"] if d.get("stopped") else "",
                "  last script:\n" + "\n".join("    " + l for l in d.get("last_script", "").splitlines()[:40])
                if d.get("last_script") else ""])))
        return self.text("\n".join(head), "\n\n".join(lost) if lost else "no item lost")

    # ---- history and comparison ----

    def _stats(self, argv):
        stats = _bench_module("benchmark_stats")
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = stats.main(argv)
        return self.text(out.getvalue().rstrip(), error=bool(code) and "no " in out.getvalue()[:40])

    def failures(self, text="", item=None, since=None):
        argv = ["--failures", text or ""]
        if item:
            argv += ["--item", item]
        if since:
            argv += ["--since", since]
        return self._stats(argv)

    def stats(self, labels):
        return self._stats(list(labels))

    # ---- side instances ----

    def instance(self, action, name):
        if action not in ("start", "stop", "status"):
            return self.text('action is "start", "stop" or "status"', error=True)
        r = subprocess.run([sys.executable, INSTANCE_TOOL, action, "--instance", name, "--home", _home(name)],
                           capture_output=True, text=True, timeout=180)
        return self.text((r.stdout + r.stderr).strip(), error=r.returncode != 0)

    def probe(self, code, instance="probe"):
        if any(r["instance"] == instance and r["proc"].poll() is None for r in self.runs.values()):
            return self.text("instance %r runs a benchmark — probe on another one" % instance, error=True)
        send = _bench_module("qtisas_benchmark").qtisas_send
        path = send.default_socket_path(instance)
        if not os.path.exists(path):
            started = self.instance("start", instance)
            if started["isError"]:
                return started
        r = send.request({"id": 1, "mode": "python", "code": code, "keep": True}, path, 300)
        return self.text(r.get("output", "").rstrip(), r.get("error", ""), error=not r.get("ok"))

    # ---- environment ----

    def environment(self):
        bench = _bench_module("qtisas_benchmark")
        identity = bench.run_identity(None, REPORTS, "", "")
        lines = ["build: %s" % identity["build"],
                 "docs: app copy %s, %s" % (identity["docs"], "STALE — differs from the source docs (cmake .)"
                                            if identity["docs_stale"] else "same as the source docs"),
                 "benchmark runs going on: %d" % identity["lanes"]]
        for port, mode in sorted(LOCAL_PORTS.items()):
            try:
                with urllib.request.urlopen("http://127.0.0.1:%d/health" % port, timeout=3) as h:
                    lines.append("local backend %s (port %d): %s" % (mode, port, h.read().decode()[:160]))
            except OSError:
                pass
        try:
            with urllib.request.urlopen("http://127.0.0.1:8090/slots", timeout=30) as h:
                slot = json.loads(h.read().decode())[0]
            lines.append("llama-server: %s, prompt of the current/last task %s tokens"
                         % ("busy" if slot.get("is_processing") else "idle", slot.get("n_prompt_tokens")))
        except (OSError, ValueError, IndexError):
            lines.append("llama-server: not reachable")
        for r in self.runs.values():
            lines.append("coach run %s: %s" % (r["id"], "running" if r["proc"].poll() is None else "done"))
        return self.text("\n".join(lines))

    def call(self, name, args):
        if name == "qtisas_run":
            return self.run(args["benchmark"], args.get("agent"), args.get("instance") or "coach", args.get("docs"),
                            args.get("bigmodel"), args.get("markers"), args.get("label"), bool(args.get("reference")))
        if name == "qtisas_wait":
            return self.wait(args["run_id"], args.get("timeout") or 600)
        if name == "qtisas_summary":
            return self.summary(args.get("run_id"), args.get("report"))
        if name == "qtisas_failures":
            return self.failures(args.get("text") or "", args.get("item"), args.get("since"))
        if name == "qtisas_stats":
            return self.stats(args["labels"])
        if name == "qtisas_probe":
            return self.probe(args["code"], args.get("instance") or "probe")
        if name == "qtisas_instance":
            return self.instance(args["action"], args["name"])
        if name == "qtisas_environment":
            return self.environment()
        raise KeyError(name)
