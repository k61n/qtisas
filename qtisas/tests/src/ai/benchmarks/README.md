# AI benchmarks

Fixed LISTs of tasks whose every item prints one checkable `R<n>: …` line, run against the internal AI and
scored without judgement. Nothing here is loaded by the AI or shipped with QtiSAS.

| File | What it is |
|------|------------|
| `list1.json`, `list2.json` | LIST 1 and LIST 2 of ai-test-requests.md made checkable (see below) |
| `list3.json`, `list4.json` | the benchmarks: instruction, setup, items with expected values and hints |
| `list5.json` | combined tasks: each item chains several steps (data, formula, compile, fit, graph, 3D, note) and reuses what earlier items created |
| `reference/<list>/itemNN.py` | reference scripts: every item solved with the documented API, must score 100% |
| `qtisas_benchmark.py` | runs a benchmark list in the test instance and scores it |
| `../../server/qtisas_test_instance.py` | a separate QtiSAS for scripted tests (own settings and fit library) |

Both scripts use the script server's client, `qtisas/python/server/qtisas_send.py` (macOS and Linux only).

## The test instance

```bash
qtisas_test_instance.py reset     # (re)create ~/.qtisas-test/home: your fit library, AI presets and AI options
qtisas_test_instance.py start     # QtiSAS -S=test with HOME set there (add --visible to see its window)
qtisas_test_instance.py status
qtisas_test_instance.py stop      # also ends it while it's busy with a long request
```

QtiSAS builds every path from the home folder, so the instance never touches your own settings or fit
library. Its settings start as defaults, except the AI's. Packages installed with `qtisas pip …` are read from
your `~/.config/qtisas/python/site-packages` (added last to Python's search path; a `pip install` in the test
instance goes to its own folder), or copied with `reset --copy-site-packages` for a fixed state. It starts the
build in the source tree (`bin/qtisas.app`); `--app` starts another QtiSAS, e.g. a release through its launcher.

## Running a benchmark

```bash
qtisas_benchmark.py list3.json --markers "NOSPLIT NOREVIEW NOEXPLORE" \
    --agent BlaBlaDor-Huge --set maxAutoContinue=6 --label "what changed"
qtisas_benchmark.py list3.json --print     # the LIST text, to paste by hand
qtisas_benchmark.py list3.json --reference # the reference scripts: must be 100%
qtisas_benchmark.py ... --instance codex --agent QtiSAS-Codex-Bridge     # a test instance of its own, to run
                                                                          # several agents at the same time
```

Each run starts a fresh test instance, runs the list in mode `ai-run` and compares every item's
`R<n>:` line with the expected value. Results go to `~/.qtisas-test/reports`: the log, a JSON report and
one line per run in `history.csv` (the instance in the file names and in a column of its own). A single
run varies by one or two items, so compare several runs before drawing conclusions.

## LIST 1 and LIST 2 as benchmarks

`list1.json` and `list2.json` are the manual integration lists of
[ai-test-requests.md](../ai-test-requests.md) (LIST 1: every Part 1 request, simplest first; LIST 2: the
instruction line, the existence check, EXPLORE, recompiling one name on purpose, an underspecified request and
"it"), each item with the same task but scorable:

- **Fixed data and names.** "Sample data" is given as a formula (LIST 1 item 2: y = x squared), a window the
  request left unnamed gets a name ("SinCos3D", "MetricsPlot", "Metrics3D"), and so do the compiled functions
  ("ai/Porod", "ai/GPeak", "ai/LPeak").
- **Noise only where the request asks for it, and never printed.** Items 7 and 12 of LIST 1 still add 3% and 5%
  Gaussian noise; their result lines hold values the noise cannot change: row counts, the noise-free model
  value, the fitted radius rounded to an integer, chi²/dof below a bound, the better model's name.
- **Formula fidelity is scored.** LIST 1 item 7 prints the noise-free sphere intensity at Q = 0.1 and item 8 the
  first simulated Porod value (q = 0.01, n = 4: 1e8) — a wrong formula gives a wrong number, where the manual
  list could only look at the code.
- **State, not claims.** Where the API can read it back, the line reports the result's state: tab counts and
  `autoexec()`, `autoUpdateTableValues()` after running the "Setup" tab, layers and curves per layer,
  `legend() is not None`, `existPlot3D()`, the project's Z columns (`columnsList(Table.PlotDesignation.Z)`).
- **What a line cannot show** (doc topics per item, split parts, colors, fonts, plot style) stays with the manual
  lists.

Reference scripts: `reference/list1`, `reference/list2` — 12 of 12 and 7 of 7 (2026-10-14), with verification.

**Verification.** Several of these result lines can be guessed from the request text alone: in the first local
run (all-pyi BIGMODEL) the model answered "plot into two layers… print the layers and curves" with
`scriptPrint("R10:", 2, 2, 1)` and nothing plotted. An item may therefore carry `"verify"`: a snippet the harness
runs in the instance **after the whole list**, which reads the real state and reports it with `check(...)`
(printed as `V<n>: …`), compared with `"verified"` (or, without it, `"expected"`). A right `R<n>:` line whose
state does not match scores `unverified (silent)` — it counts as a silent wrong answer, not as ok. Verified after the
whole list, so a snippet checks what later items leave unchanged (LIST 1 item 1: the note exists; its tab count is
item 3's). A negative test (item 10 and 11 replaced by the bare lines) scored both `unverified`. Reports from before
verification have no state and keep their scores on `--rescore`.

LIST 3–5 carry verify snippets too (2026-10-19, 21 of 24 items; reference scripts 12/12, 6/6, 6/6 with them). Three
items have none: LIST 3 item 4 (the column made writable again — there is no read-only getter) and LIST 4 items 3
and 5 check nothing of their own after item 6 deletes "Grid" and "Names" (item 6's snippet checks that).

## LIST 3 request — mode comparison benchmark with checkable results

Purpose: ONE fixed task list to run unchanged under every
AI mode and compare the modes by their results, not by impressions. Every item asks for exactly one
printed line `R<n>: …` whose correct content is known in advance (`list3.json`), so a run is
scored by reading those lines — no judgement needed. Data is noise-free and every name is
given, so the correct answer does not depend on the model, the run or the machine.

The expected values were produced by a hand-written reference script run with `qtisas -x --quit`
(same mechanism as the CTest Python tests). The items deliberately hit traps found while writing
those tests:
- column formulas run in the GLOBAL scripting language (Python by default): `^` is XOR there, powers
  need `**` or `pow()`; muParser only on request (`recalculate(col, 1, -1, True)`);
- rows without a finite result are left empty and `recalculate()` raises `ValueError`, with the
  formula's first error on its second line;
- a read-only column raises instead of changing;
- `text()` shows the table's number format, `setText()` accepts it, `cell()` is the number;
- Matrix indices are (row, col), Table's are (col, row); matrix formula variables i, j are 1-based;
- a new fit function needs a group, explicit x/y names, varying parameters, and `compile()`, not `save()`;
- `fit()` already returns chi²/dof; fitted values come from `results()`, never from the initial guesses.

**Source:** [list3.json](list3.json): the instruction, the 12 items, their expected
`R<n>:` values and a hint per item on what a wrong answer usually means. Edit the list there, not here.

**Run and score it** (the test instance, `qtisas/tests/src/server`, and the script server, `qtisas/python/server`):

```
python3 qtisas/tests/src/ai/benchmarks/qtisas_benchmark.py qtisas/tests/src/ai/benchmarks/list3.json --markers "NOSPLIT NOREVIEW NOEXPLORE"
```

Each run starts a fresh test instance (a new project, no AI history, the user's AI preset), runs the list
in mode `ai-run` and compares every item's `R<n>:` line with the expected value — number formatting
ignored (385.0 is 385, -0.0 is 0). Where a request allows two readings, the item lists the other answer
under `"also"` (LIST 5 item 2: the function's name with or without its `ai/` folder); after adding one,
`--rescore` scores the saved reports again and updates `history.csv`. Verdicts: `ok`; `wrong` or `missing`,
each `(reported)` when the assistant itself said the item failed, `(silent)` when it reported success. Per run, in
`~/.qtisas-test/reports`: the full log, a JSON report (per item: expected, got, verdict, scripts,
seconds) and one line in `history.csv` (markers, preset, model, score, silent, reported, seconds,
scripts, tokens, calls, callEfficiency, `--label`). `calls` counts the model calls (one "AI: sending" line each);
`callEfficiency` is the fewest calls the list could take (one per item) divided by `calls` — 1 means no extra call,
and it tells two runs with the same score apart. What the run ran with: `build` (git hash, `+` a hash of the
uncommitted diff, the binary's time), `docs` (a hash of the app's doc copy; `docs_stale` when it differs from the
source docs — `cmake .` missing), `model_changed` (the model that answered differs from the preset's last run, e.g.
a provider alias moved to another model) and `lanes` (other harness runs at the same time — times are then not
comparable); a model change or stale docs are also printed after the run. The harness reads the `R<n>:` lines itself, so the items do not write a
results table (an earlier "Results" table instruction added an error-prone task to every item and was
the main source of retries in one run).

**By hand:** `qtisas_benchmark.py … --print --markers "…"` prints the LIST text to paste into QtiSAS.

| Mode | `--markers` |
|------|-------------|
| default | `""` |
| no split plan | `"NOSPLIT"` |
| no self-review | `"NOREVIEW"` |
| no EXPLORE | `"NOEXPLORE"` |
| all stages off | `"NOSPLIT NOREVIEW NOEXPLORE"` |

The same item failing in all modes points at the docs; failing in only one mode points at that stage.

**Runs before the harness** (pasted by hand, `LIST NOSPLIT NOREVIEW NOEXPLORE`, 2026-10-02):
- Reference script (`-x`): every value. It found a bug, fixed since: `setDecimalSeparators()` passed the
  new locale where the old one was expected (item 5 printed `1234.5 0.25 -3` and an empty cell).
- First run: 9 of 12 values. Item 3 stopped: the formula error the script had caught still counted as the
  script's failure (fixed since). Item 6 printed `integrate()` as the sum; item 9 read "item 8" as the
  8th fit function (wording fixed since). No `R<n>:` lines: the instruction was dropped when the list
  rewrite could not be parsed (fixed since: then appended to every item).
- Second run: about 6 of 12. The list rewrite turned the instruction into invented code
  (`table("Results").setItem(...)`), which every item copied; retries re-ran whole scripts and repeated
  their side effects (item 10: 4 tabs); items 7 and 8 were skipped although their fits were right, because
  the retries contained only the failing tail. Item 12: the two functions were not deleted, and all `ai/`
  functions were listed (wording fixed since).

**Harness runs** (2026-10-02, `NOSPLIT NOREVIEW NOEXPLORE`, preset BlaBlaDor-Huge; one run each, so differences
of one or two items are within the run-to-run variation):

| Code | Score | Silent | Reported | Seconds | Scripts | Tokens |
|------|-------|--------|----------|---------|---------|--------|
| before X1–X4 | 8/12 | 3 | 1 | 165 | 14 | ~1200K |
| X1 (instruction appended, no rewrite call) | 10/12 | 2 | 0 | 115 | 12 | ~1080K |
| X1–X4 | 8/12 | 4 | 0 | 149 | 12 | ~1030K |

The wrong answers left are the model's own (item 3 reads the exception's type instead of its second line,
item 6 takes `integrate()` for the sum, item 12 lists names without deleting); the last two runs needed no
retry at all, so X2/X3 (what a failed attempt left behind) did not come into play.

After item 5 was reworded (one line), "fit function(s)" was added to the compile topic and a deletion that never
happens is blocked — preset QtiSAS-Codex-Bridge (gpt-5.6-luna), so not comparable with the runs above:

| Run | Score | Silent | Reported | Seconds | Scripts | Tokens |
|-----|-------|--------|----------|---------|---------|--------|
| 1 | 12/12 | 0 | 0 | 190 | 12 | ~707K |
| 2 | 12/12 (11 by today's scoring: an empty `R12:` line no longer counts as `[]`) | 0 | 0 | 166 | 12 | ~707K |
| 3 | not valid: the local backend closed the connection from item 8 on | — | — | 182 | 7 | ~707K |


## Multi-AI baseline — the status of the AI integration

The reference value for the AI integration as a whole: LIST 3 and LIST 4 (`list4.json`, six tasks whose
first attempt tends to fail late, after side effects), three runs each, in six configurations, measured against
**reference scripts** that score 100%.

**Reference scripts** (`reference/<list>/itemNN.py`): one per item, written with the API docs and the
source, run by `qtisas_benchmark.py <list>.json --reference` in python mode, item after item in one instance like a
LIST. They prove that every item can be solved with the documented API and that its expected value is right.

**Lanes:** `--instance NAME` runs the harness in a test instance of its own (home `~/.qtisas-test/home-NAME`), so
three agents run at once; within a lane the configurations run one after the other (the two local configurations
share one llama-server). `.md`/`.pyi` is `--docs full`/`--docs compact` for BlaBlaDor and Codex, and the backend's
own `DOCS_SOURCE_MODE` (`md`, port 8000; `pyi`, port 8002) for the local QtiSAS-ai-backend, which QtiSAS sends no
docs (Integrated).

**Baseline** (2026-10-05, `NOSPLIT NOREVIEW NOEXPLORE`, 3 runs per list and configuration; score = items ok of all
items run; integral = mean of the six configurations):

| Configuration | Score | LIST 4 (of 6) | LIST 3 (of 12) | Silent / reported | s per LIST 3 | Tokens per LIST 3 |
|---------------|-------|---------------|----------------|-------------------|--------------|-------------------|
| reference scripts | 100% | 6 | 12 | — | 2 | 0 |
| QtiSAS-Codex-Bridge (gpt-5.6-luna), `.md` | 100% | 6, 6, 6 | 12, 12, 12 | 0 / 0 | 166–199 | ~712K |
| QtiSAS-Codex-Bridge, `.pyi` | 98.1% | 5, 6, 6 | 12, 12, 12 | 1 / 0 | 169–192 | ~312K |
| QtiSAS-ai-backend (qwen3-coder:30b, local), `.md` | 92.6% | 6, 6, 6 | 10, 11, 11 | 4 / 0 | 437–619 | ~22–29K |
| BlaBlaDor-Huge (alias-huge), `.md` | 81.5% | 6, 6, 6 | 8, 9, 9 | 8 / 0 (2 backend errors) | 117–576 | ~0.9–1.0M |
| BlaBlaDor-Huge, `.pyi` | 79.6% | 6, 6, 6 | 9, 9, 7 | 11 / 0 | 78–128 | ~312–546K |
| QtiSAS-ai-backend, `.pyi` | 42.6% | 1, 2, 2 | 6, 6, 6 | 25 / 6 | 205–276 | ~28–35K |

**Integral: 82.4%.**

Per item, passes of 3:

| LIST 3 item | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
|-------------|---|---|---|---|---|---|---|---|---|----|----|----|
| Codex `.md` / `.pyi` | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 |
| local `.md` | 3 | 3 | 0 | 3 | 3 | 3 | 3 | 3 | 2 | 3 | 3 | 3 |
| BlaBlaDor `.md` | 2 | 3 | 0 | 2 | 3 | 3 | 2 | 2 | 2 | 3 | 3 | 1 |
| BlaBlaDor `.pyi` | 3 | 2 | 0 | 3 | 3 | 3 | 3 | 2 | 2 | 1 | 2 | 1 |
| local `.pyi` | 2 | 3 | 0 | 3 | 0 | 3 | 2 | 0 | 0 | 2 | 3 | 0 |

LIST 4: all 3 of 3 everywhere, except Codex `.pyi` item 3 (2 of 3) and local `.pyi` (items 1, 2: 1 of 3; 3: 3 of 3;
4–6: none).

- LIST 3 item 3 fails everywhere but Codex: the reason is on the `ValueError`'s second line, the reference reads it
  there; the others print the exception's type, and miss the empty cell.
- The local backend, the weakest model, is the one the AI's mechanical checks were tuned on: 92.6% at a few percent
  of the others' tokens. Its `.pyi` mode is not usable.
- Silent failures (a wrong value printed as if right) count more than reported ones; BlaBlaDor has the most.

To run it again: one shell per lane, started at the same time, each running its configurations one after the
other, e.g. the Codex lane:

```bash
for docs in full compact; do for list in list4 list3; do for i in 1 2 3; do
    qtisas_benchmark.py $list.json --instance codex --agent QtiSAS-Codex-Bridge --docs $docs \
        --markers "NOSPLIT NOREVIEW NOEXPLORE" --label "baseline, Codex $docs, run $i/3" --quiet
done; done; done
```

The lists as text, to paste into QtiSAS by hand: [ai-test-requests.md, "LIST 3, LIST 4 and LIST 5"](../ai-test-requests.md#list-3-list-4-and-list-5--the-benchmark-lists).
