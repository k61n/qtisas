# Coach — design (updated 2026-10-19)

Coach = an IDE AI (e.g. Claude Code through the MCP server) that supervises QtiSAS's built-in AI: it runs
benchmark lists, reads the results, turns failures into doc changes, API fixes or rules, verifies API claims on a
side instance, and integrates the modules (ascii1d, dan, jnse) into the AI. This document covers the tools, the
workflow, the module pipeline and the phases.

Grounded in the benchmark work of 2026-10-11..19: ~40 hand-driven MCP runs (2026-10-11), series 4–6 (~90 runs,
2026-10-12), the BIGMODEL lanes and statistics series (2026-10-14/15: Codex 5 rounds, BlaBlaDor 3, local 1), LIST
REVIEW and LIST SPLIT series (2026-10-15) and the all-md lanes (2026-10-19).

## 0. Strategy (changed since the first draft)

- **Two stages per module or list** (2026-10-19; replaces "tune only where Codex fails" of 2026-10-15):
  1. **The strongest, fastest model first** (Codex all-pyi BIGMODEL) **to its maximum** — it shows whether the API
     and docs make the task solvable; its failures are API, doc or QtiSAS bugs.
  2. **Then the weakest** (local qwen3-coder all-pyi), **iterating docs and code from its score to its own
     maximum** — not to a fixed score. Its failures show where the docs are easy to misread.
  Only general fixes (docs first, then an API that is harder to misuse; never a guard shaped to one item); every
  change re-run on the strongest model (score and η must not drop); the holdout at the end on both. A QtiSAS bug is
  fixed whichever model reveals it. ascii1d (2026-10-19): Codex 10 → 16/16 (η 0.8 → 1.0), local 0 → 4 → 7 → 9 → 11
  of 16 over five doc/API iterations.
- **When stage 2 stops:** each remaining failure is sorted — a general doc/API lever exists (a misread choice
  between similar calls, a copied example value, a unit trap, an error that names no fix) → another iteration; the
  model's own limit (its coding errors, the "same explanation twice" loop, made-up result lines) → no change. Stop
  when only the second kind is left and two iterations bring no gain beyond run-to-run noise; a score called the
  maximum is confirmed by a second round on the same docs.
- **BIGMODEL is the baseline configuration:** all docs first and cacheable, split plan, self-review and EXPLORE off.
  Codex BIGMODEL all-pyi: 99.5% over 5 rounds (lists 1–5, 43 items), 48 calls, η 0.89, 5.8 min per round. SPLIT
  costs Codex 2.4× the calls and twice the time for no gain (41.7/43); REVIEW doubled the calls for no gain.
- **Docs: md vs pyi** (2026-10-19): Codex all-md 43/43, 45 calls, 5.4 min; prompt tokens ×2.07 but 99.4% cached,
  billable ×1.8 (0.62M vs 0.35M per round). Local all-md LIST 1: 11/12 in 16 calls vs all-pyi 9/12 in 20.
- **Between runs, not during runs:** most losses of the reference model are silent wrong values or misreadings that
  no escalation would catch; the coach's work is measuring, explaining and fixing between series.

## 1. What exists

| Layer | Pieces |
|---|---|
| Script server (`python/server/qtisas_server.py`) | modes `python`, `ai`, `ai-run`, `ai-chat`, `follow`, `note`, `stop`, `status`, `quit` |
| MCP server (`python/server/qtisas_mcp.py`) | `run_python`, `ask_qtisas_ai` (wait or not), `qtisas_ai_progress` (`wait_seconds` ≤ 120), `correct_qtisas_ai`, `stop`, `status`, `list_objects`, `show_graph`, `get_file`; in the source tree also the coach tools (`qtisas_coach.py`, section 3) |
| Benchmark harness (`tests/src/ai/benchmarks/qtisas_benchmark.py`) | fresh test instance per run (`--instance`), setup, markers, `--agent`, `--docs compact\|full\|integrated`, `--bigmodel`, `--set`, `--timeout` (1800 s local, 900 s otherwise), scoring with `verify` scripts (verdicts ok / wrong / missing / unverified, silent or reported), calls and call efficiency η, server probe with the model that answered, `--reference`, `--rescore`, `history.csv` |
| Benchmark lists | `list1..5.json` (43 items) and `ascii1d.json` + holdout (20); `verify` scripts for 40 of 43 LIST items and the ASCII1D ones; reference scripts for every item (`benchmarks/reference/`) |
| Run records | run identity per report and `history.csv` (build, docs, model change, lanes); lost-item dossiers per report; `failures.jsonl` with signatures; `benchmark_stats.py` (labels, `--failures`) |
| Test instances (`tests/src/server/qtisas_test_instance.py`) | `start/stop/status/reset --instance NAME` |
| Local backend (`ai-models/qtisas-ai-backend`) | hybrid-rag-server modes md / pyi / md-pyi / all-pyi / all-md / no-doc, one llama-server slot shared, slot snapshots keyed by a docs fingerprint, `CONTEXT_TOKENS` checked against llama-server at startup |
| Give-up points (`AiAssistant.cpp`) | all through `recordGiveUpInHistory()`: blocked-attempts cap, same diagnosis twice, same alternatives twice, retry budget, EXPLORE refusal |
| Corrections log | `ai/corrections.log`, notes given during a run |

## 2. What the coach lacked (measured)

From 2026-10-11 (hand-driven MCP runs):
1. Four calls per run, three routine (new project, clear history, the 3 KB list text) plus an outside follower.
2. No cheap wait: `wait=true` hits the MCP client's 1800 s limit; progress returns thousands of lines.
3. No view of the prompt that was sent.
4. No side instance for a probe during a running series.

From series 4–6 (2026-10-12):
1. Finding a lost item's story: 3–5 grep/sed calls per item (last script, error, blocks, repairs).
2. What the model was given: doc topics per call, "self-review changed the draft".
3. Is it new? Every "docs first / guard on a 2nd occurrence / remove unproven fixes" decision needed a count of
   earlier occurrences by grep.
4. Which build ran: fixes landed mid-series four times; one series mixed builds.
5. Which docs and which model.
6. Is the API what the doc says: each doc claim verified on a hand-started probe instance.
7. Time and tokens per run and configuration; time distorted by parallel lanes.
8. Silent wrong values cannot escalate — only scoring against expected values catches them.

Confirmed again 2026-10-15..19:
- **The model behind a preset changed silently:** BlaBlaDor `alias-huge` went from MiniMax-M2.7 to
  MiMo-V2.6-Pro-RL (reasoning, 1M context); all-md calls took 50–120 s and hit ~120 s gateway timeouts. Found by
  hand from the provider's model list; the old baseline (92.2%) belongs to the old model.
- **Prompt comparison by inference:** whether L3·11 failed because of split mode was decided from a 4-token
  difference in the reported prompt size (it was the project state's column names, not split).
- **Statistics by scratchpad scripts:** score, silent/reported, time, calls/η, prompt / cached / billable tokens and
  per-item failures were computed by throw-away scripts for every comparison.
- **A false check found only by reading logs:** the split-plan checks read the list instruction and the run-mode
  suffix as part of the request (35 of 39 re-plans false) — visible only after counting "issue(s) detected" lines.
- **Stale cache found by hand:** 27 slot snapshots (65 GB) whose docs fingerprint no longer matched.
- **Stage 2 waits for a person:** the local backend builds its doc block once at startup, so every doc iteration of
  the weak model needs a restart of the all-pyi server by hand (six times for ascii1d). Open: let the coach restart
  it, or have the server reload when the doc files change.

## 3. Tools

New tools; the existing ones stay unchanged. Everything runs in the script server and the harness (Python); the
MCP tools only wrap script-server modes, so a human in a terminal (`qtisas_send.py`) gets the same.

| Tool | Arguments | Returns |
|---|---|---|
| `qtisas_run` | `text` **or** `benchmark` ("list5"), `markers`, `agent`, `docs`, `bigmodel`, `config` ({key: value}), `instance`, `save_transcript` | `run_id` at once |
| `qtisas_wait` | `run_id`, `timeout` (s, below the client limit, default 1500) | one event: `done` (with the summary) or `timeout` (call again) |
| `qtisas_summary` | `run_id` | the summary (section 5) |
| `qtisas_last_prompt` | `part` ("user", "system", "docs", "all"), `n=1` | the last n requests as sent; the whole prompt saved to a file, sizes per section returned |
| `qtisas_instance` | `action` ("start", "stop", "status"), `name` | wraps `qtisas_test_instance.py`; `run_python`/`status`/`list_objects` gain `instance=` |
| `qtisas_probe` | `code`, `instance` ("probe") | runs the code on a side instance (started if needed) — verifies an API claim before it goes into a doc |
| `qtisas_failures` | `signature` or `item`/`error` filter, `since` | occurrences in the failure-signature store (section 6a) |
| `qtisas_environment` | — | build id, docs hash (app copy vs source vs backend), agent preset → model that answers, model change since the preset's last run, running lanes |

`benchmark=` reads `benchmarks/<name>.json` exactly like the harness and appends to the same `history.csv`, so an
MCP run and a harness run are comparable. `qtisas_last_prompt` needs one C++ addition: `AiAssistant` keeps the last
n request payloads in a ring buffer, read through a server mode `last_prompt`.

## 4. Run identity and statistics

Every report and `history.csv` row records:
- **build:** git hash, hash of the uncommitted diff, binary modification time — a series whose runs differ is
  flagged;
- **docs:** hash of the app bundle's doc copy and of the source docs (differ → `cmake .` missing);
- **model:** the model that answered (server probe and every call's reply), and a flag when it differs from the
  preset's previous run;
- **lanes:** other lanes running at the same time (time comparisons with parallel lanes are marked).

The statistics tool (in the repo, replacing scratchpad scripts) compares labels or label prefixes: per lane and
list score (mean, min–max over complete rounds), silent / reported per round, time, calls and η, prompt / cached /
uncached / billable-equivalent tokens (cached at the provider's discount; "not reported" where the provider gives no
cache figures), and per-item failures with verdict kinds.

## 5. Run summary and lost-item dossier

Returned by `qtisas_wait` (`done`) and `qtisas_summary`, also written into the report JSON:

```json
{"run_id": "…", "status": "done", "took_s": 734, "calls": 45, "prompt_tokens": 5870000, "cached_tokens": 5840000,
 "build": "c43740c8+3f2a", "model": "gpt-5.6-luna", "score": {"ok": 42, "of": 43},
 "items": [{"n": 11, "verdict": "wrong (silent)", "expected": "2046", "got": "R11: 385.0",
            "last_script": "…", "last_error": null, "blocks": [], "repairs": [], "doc_topics": ["ai", "app", "table"],
            "plan_parts": 1, "replans": 0, "signature": "formula: pow(col(1),2) for 2^col(1)", "seen_before": 2}]}
```

Fields are parsed from the lines a run already prints (`AI LIST: item`, `R<n>:`, `blocked —`, `auto-corrected`,
`stopping —`, `split-plan (k part(s))`, `issue(s) detected`, `AI cache:`); the full transcript stays on disk.

## 6. Coach workflow (between runs)

1. `qtisas_run` per configuration (lanes in parallel: one instance per lane); `qtisas_wait` until `done`.
2. Sort every failure: **QtiSAS bug** (any model: fix it) / **strongest model fails** (stage 1) / **weak model
   misreads where a general doc or API lever exists** (stage 2) / **the model's own limit** (no change).
3. Docs first: a missing or unclear doc → doc proposal (md + pyi), applied after the series, `cmake .`, re-index.
4. Doc correct and demonstrably ignored → mechanical check or repair, general (never shaped to one item).
5. A check that never fires, or fires without helping, over a full series is removed with its test.
6. **One build per series**; a run whose build differs is flagged.
7. **Verify before documenting:** an API claim is checked with `qtisas_probe` first.
8. **Compare with separate tables:** score, time, tokens (prompt / cached / billable).
9. **Judge by recurrence:** a signature seen ≥ 2 times (or once per run of a configuration) is a pattern; a single
   miss on an otherwise passing item is noise.

## 6a. Failure signatures

Every lost item (and every block or repair that fired) is stored with a signature: item id, error class + first
line normalised (names → placeholders), the check header, the call it points at. The store (JSONL next to
`history.csv`) answers "seen before?" across items, lists, models and configurations: signature seen twice →
propose a doc fix, then a rule.

## 7. Module integration pipeline (ascii1d → dan → jnse)

Tasks and their benchmark items come first and act as acceptance tests; the strongest model validates the API before
anything is tuned.

| # | Step | Output | Done when |
|---|---|---|---|
| 0 | **Task inventory** with the user: GUI actions, files, typical analyses, existing user scripts | task list with inputs/outputs | the user agrees on the list |
| 1 | **Benchmark items** from the tasks (before code), computable expected values, `verify` scripts; a **holdout list** kept aside | `benchmarks/<module>.json` + holdout | every task has ≥ 1 item |
| 2 | **C++ preparation**: the API surface the items need, checked against the AI-ready rules below | C++ methods | items are expressible |
| 3 | **SIP bindings + contract tests** (normal path, error path, empty/None, repeated calls) | `qtimod.sip`, `<module>_contract.py` | contract tests pass on a probe instance |
| 4 | **Reference scripts** by the strongest model, no QtiSAS checks | `benchmarks/reference/<module>/` | 100% — otherwise back to 2–3 |
| 5 | **Docs**: skeleton from the SIP, curated notes in marked sections; md + pyi | `<module>-python-api.md/.pyi` | regeneration keeps the curated parts |
| 6 | **AI structure**: doc topic + keywords, host facts for PROJECT STATE (`entityKinds`, shapes), benchmark cleanup; a large doc gets a doc-set tier (on demand) | C++ host + topic table | a request naming a module object gets its doc |
| 7 | **Benchmark iterations** with the reference model (BIGMODEL): docs first, signatures, one build per series | doc fixes, API fixes, rules | reference model 100% on the module list; holdout within ~5 points |
| 8 | **Regression**: LISTs 1–5 after a module series; C++ changes re-run the contract tests and regenerate the doc skeleton | | no drop on LISTs 1–5 |

**AI-ready API rules (step 2):**
- **No silent failure.** Every failure raises with a reason (`calculate()` zeroing a matrix, a tab's `execute()`
  failing invisibly, `configure()` succeeding for a function that does not load, `simulateCurve()` making no table —
  all cost items).
- **The error names the fix** ("Did you mean 'ai/Lor5'?", "its parameters are 'scale', 'rate', 'offset'").
- **One indexing convention**, or say it in the name.
- **Name methods the way models guess** when cheap (`functionExists()`), or list guessed-but-missing names as absent.
- **Methods written with their receiver in docs** (`c.functionExists(...)`), never readable as globals.
- **Plain Python return values**; a yes/no is a `bool`.
- **Reuse vs create explicit** (`newNote()` reuses, `newTable()` wipes).
- **Units in the parameter names** (`collimationCm`, `sampleApertureWmm`, `lambdaA`), and an error for a value in the
  wrong unit (a distance below 50 "cm" was given in metres) — ascii1d: the local model passed 8 for 8 m.
- **Accept the types models pass:** a number as int or string (`setHeader(5)`), one source or a list, one name or a
  list, in any combination.
- **A script never meets a modal dialog:** warnings of a scripted run are collected and printed, not shown.
- **A run returns what it made** (names, absolute paths), so later steps use them instead of guessing names.
- **Say which call fits when a similar one was used:** "QI-alpha is a table of the project, not a file: … with
  convertTables(\"QI-alpha\")" — a bare "no such file" was retried with .dat and .RAD until the item was lost.
- **Refuse what silently destroys:** a result whose default name is its own source is refused with the ways out.

**AI-ready doc rules (step 5, from the ascii1d iterations: local model 0 → 4 → 7 → 9 → 11 of 16, Codex 16/16 at
η 1.0):**
- **The routing rule first:** which requests this module serves — by the words users use (SANS curve, ascii file,
  radially averaged dataset …) and by the operations only it has, not only by its own name.
- **"Global object, no import"** when the module is a global (`from Ascii1d import Ascii1d` cost Codex a call in
  most runs until said).
- **A recipe with placeholders, never example values.** `a.setHeader(5)` in a recipe was copied into every script by
  the local model (plain files lost their first 5 points); `setHeader(<number of header lines>)` with "ONLY what the
  request says" was not.
- **A choice table when calls look alike** (source × destination → the call), with the one wrong use spelled out ("a
  table is never a file").
- **One example per variant of the main task**, each one call with its options — the local model chained import and
  export, so the options ran twice.
- **When to give a name and when to take the returned one**, with the request's own wording as the example; never
  invented names.
- **Facts of the language around the API:** Python's `open()` does not expand `~`; use the returned absolute paths.
- **Every iteration re-run on the strongest model:** a doc change that helps the weak one must not cost the strong
  one calls (η is the measure, not only the score).

**Order:** ascii1d first (smallest, ~2.9K lines, no Python API — the cleanest test of the pipeline), then dan
(~27.6K lines; needs doc sets first, since BIGMODEL sends every doc on every call), then jnse (~0.5K lines).

## 8. Deferred until evidence asks for it

- **Escalation to the coach** (pause at a give-up, `help` event, `qtisas_hint`, "assisted" scoring; design kept:
  one hook in `recordGiveUpInHistory()`, ≤ 2 escalations per item, hints are facts not scripts, enforced in the
  script server, `COACH`/`NOCOACH`, `assisted` column). Its main beneficiary was a stalling weak model, which the
  reference rule treats as model-dependent. Build only if module work shows stalls of the reference model that a
  fact would fix.
- **Escalation ladder** (temperature step, help-stage step): the help-stage step was built and removed 2026-10-15
  (fired 5 times, rescued nothing, about a third more tokens); the temperature step was never measured.
- **Split mode**: done as far as needed — one-part plans send the request itself, the part-output check, doc topics
  from live object kinds, the earlier-parts note with table shapes, plan checks reading only the item (2026-10-19).
  No further split work: SPLIT does not help the reference model.
- **Rule format and check merge** (`rule(ctx) -> Block | Repair | None`, Python rules loaded at runtime, C++ checks
  migrated family by family with a parity corpus): when module work produces checks worth sharing.
- **Formula fidelity** (evaluate the request's formula and the compiled function at sample points): one occurrence
  so far.

## 9. Phases

| Phase | Content | Touches | Done when |
|---|---|---|---|
| C0 | this document — **done 2026-10-19** | docs | approved |
| C1 | **done 2026-10-19:** run identity (build, docs hash, model + change flag, lanes) in reports and `history.csv`; `verify` scripts for LIST 3–5 from the reference scripts; statistics tool in the repo | harness (Python) | a series comparison is one command; model/build changes are flagged |
| C2 | lost-item dossier in the report JSON; failure-signature store + query — **done 2026-10-19** (`benchmark_dossier.py`, `failures.jsonl`, `benchmark_stats.py --failures`) | harness (Python) | "seen before?" is a lookup |
| C3 | MCP tools `qtisas_run`, `qtisas_wait`, `qtisas_summary`, `qtisas_failures`, `qtisas_stats`, `qtisas_probe`, `qtisas_instance`, `qtisas_environment` — **done 2026-10-19** (`python/server/qtisas_coach.py`; runs go through the harness as a subprocess instead of new script-server modes: one run engine for harness, terminal and MCP; offered only in the source tree) | MCP (Python) | a series runs from the IDE in ~2 calls per run |
| C4 | `qtisas_last_prompt` (ring buffer, server mode) | small C++ + server | two runs' prompts compared in one call |
| M1 | ascii1d, pipeline steps 0–8 — **stage 1 done** (Codex 16/16 at η 1.0, holdout 4/4, LIST 1–5 unchanged), **stage 2 running** (local 11/16 at docs v5) | C++, SIP, docs, benchmarks | strongest model 100% on module list and holdout; weakest at its own maximum |
| M2, M3 | dan (after doc sets), jnse | | |
| later | section 8, on evidence | | |

## 10. Decisions

1. Runs go through the benchmark harness (`qtisas_benchmark.py`); the MCP coach tools start it as a subprocess, a
   terminal coach runs it directly — one run engine, the same reports, dossiers and failure store. (First planned
   as script-server modes; changed 2026-10-19.)
2. One build per series; build, docs and model recorded per run.
3. Escalation, if ever built: ≤ 2 per item (loop guard), a per-run cap only for a paid cloud coach; hints checked
   mechanically (> ~3 lines of code rejected); escalated items scored `assisted`, outside `score`.
4. `qtisas_last_prompt`: `part="user"` by default; system/docs/all on demand; the whole prompt always saved to a
   file.
