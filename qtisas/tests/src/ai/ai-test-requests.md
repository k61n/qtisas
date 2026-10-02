# AI Assistant — Real Test Requests

This file is a fixture for `ai_content_tests` (see the `realRequests`/`realRequests_data` test) and
a human-readable reference of realistic requests used to exercise the AI assistant's behavior
throughout this project's development.

**Not loaded by the AI at runtime.** `AiContentProvider::loadDocs()` (`QtiSasAiContent.cpp`) only
ever globs `*-python-api.{md,pyi}` inside `qtisas/python/doc/` — this file lives in
`qtisas/tests/src/ai/` and matches neither that location nor that naming pattern, so it is invisible
to the model by construction, regardless of its content. It is also never touched by the
`file(COPY "python/doc" ...)` resource-copy step, so it is never shipped in the built app either.

Each entry states the request as a blockquote (so it's actually readable in a rendered preview, not
hidden), followed by **Expected topics:** — the minimum set of doc topics
`splitPartTopicsFromAnswer()`'s keyword/synonym backstop must detect from that text alone (the same
mechanism that rescues a split-mode part whose model-provided `DOCS:` field came back incomplete or
`"none"`). Both lines are parsed by the test via a plain regex; a prose note under each entry
explains why it's here.

## Compound multi-topic fit-and-plot request

**Request:**
> Create a table "GaussPeak" with 200 rows of synthetic Gaussian-peak data (x, y, dy columns) with 5% noise. Compile a custom Gaussian peak fit function and a Lorentzian fit function. Fit both models with weighted errors. Compare chi2/dof and report which model fits better. Plot the winning model's data, fit curve, and residuals into graph "GaussPeakFit" with two stacked panels, a styled legend, and axis titles.

**Expected topics:** table, compile, fittable, graph

The standing regression request used throughout this project's live-transcript testing — it touches
table/compile/fittable/graph creation in one request and has surfaced the majority of the
Directions and self-review backstops documented in `QtiSasAiContent.cpp`.

## Plain table creation

**Request:**
> Create a table "SAXS_data" with 3 columns Q, I, dI and fill it with synthetic small-angle-scattering intensities for a sphere form factor plus 3% Gaussian noise.

**Expected topics:** table

A single-topic request with no fitting or plotting — checks the backstop doesn't over-fire on
unrelated topics for a plain data-generation request.

## Matrix formula fill

**Request:**
> Create a 50x50 matrix "SphereMap" and fill every cell using the formula sin(x)*cos(y) via setFormula() and calculate().

**Expected topics:** matrix

Matrix has no dedicated checklist function (confirmed during the docs/checklist correlation audit)
— this keyword backstop is effectively its only mechanical safety net today.

## 3D surface plot

**Request:**
> Plot a 3D surface of sin(x)*cos(y) over x and y in the range -pi to pi, using a filled mesh style, then add a legend and axis labels.

**Expected topics:** graph3d

Deliberately natural phrasing — no literal "graph3d"/"matrix"/"graph" substring anywhere. This
entry originally detected ZERO topics: `topicSynonymRe` (`AiAssistant.cpp`) had a dedicated entry
for "fittable" but none for "graph3d". Fixed by adding a `graph3d` synonym regex matching "3D",
"surface plot", "plot3d" — this entry is now the regression test for that fix.

## Auto-executing note

**Request:**
> Create a note "Setup" with an auto-executing script tab that configures analysis defaults when the project file is opened; keep it as the active tab so it actually runs.

**Expected topics:** note

Ties to the note autoexec multi-tab documentation fixed during the docs/checklist correlation pass
— "keep it as the active tab" reflects that exact real constraint.

## Custom function compile + simulate (no real data)

**Request:**
> Compile a custom Porod power-law fit function with an adjustable exponent parameter, then simulate a curve from it without fitting any real data.

**Expected topics:** compile, fittable

Compile and Fittable are two separate topics/checklist functions even though compiling almost
always precedes fitting — checks both are detected from one request that only compiles+simulates,
never calls fit().

## Fit an existing library model against real data

**Request:**
> Fit the existing "Sphere" form-factor model from the Fittable library against table "SAXS_data" columns I and dI, using weighted Levenberg-Marquardt, and report chi2/dof.

**Expected topics:** fittable, table

No compile() call at all — an existing library model fit against a named, pre-existing table.
Checks "fittable" is detected via the synonym regex (the word "fittable" itself never has to
appear verbatim) alongside a plain "table" keyword hit for the input data reference.

## Restyle an existing graph

**Request:**
> Restyle the existing graph "FitResults" — set the legend font to bold 12pt, change the first curve's line color to red, and add minor tick marks on both axes.

**Expected topics:** graph

No table/compile/fit anywhere — pure post-hoc styling of an already-built graph, checking the
backstop doesn't force in unrelated topics just because "FitResults" sounds fit-related.

## Note script reading a table on autoexec

**Request:**
> Add a script tab to note "Batch" that reads table "RawScan" and prints its row count each time the project loads via autoexec.

**Expected topics:** note, table

A composite note+table request — checks both topics are detected together, not just whichever one
happens to be checked first in `knownDocTopics()`'s iteration order.

## LIST request — full AI integration test

Not a `realRequests_data()` fixture row like the entries above — deliberately a fenced code block,
not a `>` blockquote, so `parseAiTestRequestsFixture()`'s regex parser (`ai_content_tests.cpp`,
which overwrites its `request` capture on every `>` line and would otherwise silently collapse a
multi-line blockquote down to just its last line) skips it entirely. This is a manual, paste-into-
the-live-app scenario exercising the whole `AiAssistant::parseListRequest()`/`beginListRequest()`
pipeline end to end — LIST parsing, per-item mechanical doc-topic matching, existence-checks, and
both non-split and split-mode generation — not a per-request topic-detection unit test.

Every entry above, combined into one `LIST`, ordered simplest to most complex rather than in the
file's own order — each item builds on more moving parts than the last, so an early failure points
at the simplest thing that could be wrong instead of surfacing deep inside a compound request.
Dependency order is kept regardless: an object referenced as already existing is always created by
an earlier item first. Three items (1, 2, 10) don't correspond to any entry above — "Restyle...
`FitResults`" and "...note `Batch`...table `RawScan`..." reference names no other entry ever
creates, so these three add the missing prerequisites rather than leaving those two entries
unrunnable as part of a real end-to-end LIST.

```
LIST
1. Create a note "Batch" with no content yet.
2. Create a table "RawScan" with 50 rows of x, y sample data.
3. Add a script tab to note "Batch" that reads table "RawScan" and prints its row count each time the project loads via autoexec.
4. Create a note "Setup" with an auto-executing script tab that disables automatic table-formula recalculation project-wide when the project file is opened; keep it as the active tab so it actually runs.
5. Create a 50x50 matrix "SphereMap" and fill every cell using the formula sin(x)*cos(y) via setFormula() and calculate().
6. Plot a 3D surface of sin(x)*cos(y) over x and y in the range -pi to pi, using a filled mesh style, then add a legend and axis labels.
7. Create a table "SAXS_data" with 3 columns Q, I, dI and fill it with synthetic small-angle-scattering intensities for a sphere form factor plus 3% Gaussian noise.
8. Compile a custom Porod power-law fit function with an adjustable exponent parameter, then simulate a curve from it without fitting any real data.
9. Fit the existing "sasviewmodels/sphere" form-factor model (scale, backgroubd, Rg are fitable) from the Fittable library against table "SAXS_data" columns I and dI, using weighted Levenberg-Marquardt, and report chi2/dof.
10. Plot the fit results from the previous step into graph "FitResults", including the data, fit curve, and residuals.
11. Restyle the existing graph "FitResults" — set the legend font to bold 12pt, change the first curve's line color to red, and add minor tick marks on both axes.
12. Create a table "GaussPeak" with 200 rows of synthetic Gaussian-peak data (x, y, dy columns) with 5% noise. Compile a custom Gaussian peak fit function and a Lorentzian fit function. Fit both models with weighted errors. Compare chi2/dof and report which model fits better. Plot the winning model's data, fit curve, and residuals into graph "GaussPeakFit" with two stacked panels, a styled legend, and axis titles.
```

## LIST 2 request — extended AI mechanics test

The LIST above deliberately covers all seven `knownDocTopics()` entries at least once, but every one of
its items collapses to "single part — running directly" except item 12 — so mechanisms that only show
up when a request is NOT a fresh, single-topic creation are still untested: the EXISTS branch of the
existence-check preflight (every LIST-1 item only ever hits MISSING, since each creates its own
brand-new object), explicit read-before-write EXPLORE sequencing, intentional same-name recompilation,
and anaphora ("it", "the model", "the data") resolved against what an earlier item in the SAME list just
created. Self-contained like LIST 1 — creates its own prerequisites rather than depending on LIST 1
having already run.

Also worth running this same block with `LIST NOSPLIT NOREVIEW NOEXPLORE` appended, to confirm each
item still behaves reasonably with the split-plan/self-review/EXPLORE stages forced off — none of the
existing fixtures exercise that combination even though it is real, user-facing syntax (see
`AiAssistant::parseListRequest()`).

```
LIST NOSPLIT NOREVIEW NOEXPLORE
For every item, use the exact object names given and verify the underlying logic (formulas, fit setup, column roles) before considering it done.
1. Create a table "Baseline" with 30 rows of x, y sample data.
2. Add a column "z" to the existing table "Baseline" computed as x*y, using the Z column role — reuse the table you just created, don't make a new one.
3. Check which sasviewmodels fit functions are already compiled, then simulate one of them without fitting any real data.
4. Compile a custom exponential-decay fit function named "ai/ExpDecay" with parameters scale and rate. Then recompile "ai/ExpDecay" again, this time adding an extra "offset" parameter — the second call should intentionally overwrite the first, not fail or create a duplicate.
5. Fit the model against the data.
6. Create a note "Log", a table "Metrics" with 20 rows of x, y, and z sample data (z computed as a function of x and y, using the Z column role), and a styled graph of it with a legend — all in one request.
7. Plot it as a 3D surface instead, using a filled mesh style.
```

Item 2 is the deliberate EXISTS counterpart to every LIST-1 existence-check, which only ever sees
MISSING; its Z role (not Y — matches the earlier real run's own "expected exactly one Y column;
found ['y', 'z']" failure) keeps "Baseline" down to one unambiguous Y column, so item 5's own
ambiguity stays confined to WHICH model rather than also being about which column. Item 3 forces
an EXPLORE-then-act two-step (mirrors the real transcript where a mixed explore+action script got
truncated to its read-only prefix — see `AiAssistant.cpp`'s EXPLORE write-detection). Item 4 is the
"ai/" namespace intentional-overwrite path (compile-python-api.md Rule 6) made explicit rather than
incidental. Item 5 is deliberately underspecified — no model name — checking the assistant falls
back to whatever `Fittable.currentFunction()` already implies, per its own "never stop merely
because a name is missing" rule, rather than inventing an unrelated name. Items 6-7 test anaphora
("it") resolved against the compound object item 6 just created, one LIST item later; item 6 now
gives "Metrics" a real z dimension WITH the Z role pinned explicitly, so item 7's 3D surface has
actual, correctly-roled data to plot — confirmed live: leaving the role unspecified repeatedly got
"z" assigned the Y role instead, and one real run's item 7 correctly detected the mismatch via
qualified names but then gave up instead of fixing it, silently completing with no 3D plot created.

The line between `LIST` and item 1 is `parseListRequest()`'s `instructionBlock` — free text there gets
sent, together with the whole numbered list, to a one-shot LLM call that rewrites EVERY item to
incorporate it before any item runs (see `beginListRequest()`'s doc comment). Untested by LIST 1, whose
`LIST` line goes straight into item 1. Exercises whether that rewrite reaches all 7 items, not just the
first, and leaves each item's own quoted object names intact.

## Interactive follow-up sequence — short-confirmation continuation

Not a `LIST` — LIST items are independent, non-conversational requests run in sequence, so this
scenario can only be reproduced as real, separate chat turns typed one at a time (paste each line
below as its own message, waiting for the previous one to finish):

> make a 3D plot with 3 spheres connected along one line: radius ratio 5:1:3

> it looks like pressed along the aligned line. Could be scaling of all 3 axes the same?

> go

Confirmed live: the third turn — a bare one-word confirmation of a fix the assistant itself had just
proposed — resolved to doc topics `[]` (no keyword in "go" matches anything), which self-review's own
scope check then read as "this part's docs are baseline-only, so any graph3d-specific code is out of
scope." The actually-correct fix script was rejected, degraded through a second rejected stub, and
finally landed on `scriptPrint("Ready.")` — reported as a normal successful run, with no visible sign
that the requested fix was never applied. This sequence is the regression test for that failure mode:
a short confirmatory reply must inherit the ongoing task's topic scope from conversation history
instead of being reclassified from scratch with no context.

## LIST 3 request — mode comparison benchmark with checkable results

Not a `realRequests_data()` fixture row — a fenced block like LIST 1/2, and no line starts with `>`,
so `parseAiTestRequestsFixture()` skips it. Purpose: ONE fixed task list to run unchanged under every
AI mode and compare the modes by their results, not by impressions. Every item asks for exactly one
printed line `R<n>: …` whose correct content is known in advance (table below), so a run is scored by
reading those lines from the console — no judgement needed. Data is noise-free and every name is
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

Run it in a NEW project, once per mode, pasting the block unchanged except for the first line:

| Mode | First line |
|------|-----------|
| default | `LIST` |
| no split plan | `LIST NOSPLIT` |
| no self-review | `LIST NOREVIEW` |
| no EXPLORE | `LIST NOEXPLORE` |
| all stages off | `LIST NOSPLIT NOREVIEW NOEXPLORE` |

```
LIST
Use exactly the object names given. End every item by printing one line that starts with "R" and the item number and a colon (e.g. "R1:"), followed only by the values the item asks for, in the order asked. Also write that line into table "Results": create it if it does not exist (12 rows, columns "item" and "answer", "answer" in text format), then put the item number into column "item" and the printed line into column "answer" of the row with that number; never clear the other rows.
1. Create a table "Squares" with 10 rows and 4 columns; fill column 1 with 1, 2, ..., 10. Give column 2 a column formula that squares column 1 and recalculate it in the default scripting language. Print the sum of column 2.
2. In "Squares", give column 3 a formula for the cube of column 1 written in muParser syntax with ^, and recalculate it with muParser explicitly. Print the sum of column 3.
3. In "Squares", give column 4 the formula 1/(column 1 - 5) and recalculate it. Print the number of empty cells in column 4 and the name of the Python error the failure reported.
4. Make column 2 of "Squares" read-only, set its formula to 10 times column 1 and try to recalculate it. Print whether the recalculation raised an error and the sum of column 2 afterwards. Make the column writable again.
5. Create a table "Format" with 3 rows and 1 column holding 1234.5, 0.25 and -3. Switch its decimal separators to German (comma), without group separators. Print the three cells as displayed text. Then write the text "2,5" into row 2 and print the number that cell now holds.
6. Create a 4x4 matrix "Prod" and fill it with the formula i*j. Print the sum of all cells, the value in row 2, column 3, and the determinant rounded to 6 decimals.
7. Compile a new fit function "ai/RefLine" for y = a*x + b with both parameters adjustable. Create a table "LineData" with 20 rows: x = 1..20, y = 2x + 1, dy = 0.1 (roles X, Y, yErr). Fit "ai/RefLine" to it with Levenberg-Marquardt, weighted by dy. Print a and b with 4 decimals.
8. Compile a new fit function "ai/RefDecay" for y = A*exp(-k*x) + c with all three parameters adjustable. Create a table "DecayData" with 101 rows: x = 0, 0.1, ..., 10, y = 3*exp(-0.5*x) + 0.2, dy = 0.01 (roles X, Y, yErr). Fit it starting from A = 1, k = 1, c = 0. Print A, k and c with 4 decimals.
9. Create a graph "DecayPlot" with two stacked layers: the top one with the "DecayData" points and the fitted curve from table "fitCurve-ai/RefDecay", the bottom one with that table's residuals. Print the number of layers, the number of curves in the top and in the bottom layer, and whether the largest absolute residual is below 1e-4.
10. Create a note "Report" whose first tab contains print('A') and a second tab named "Calc" containing print(6*7). Do not run them. Print the number of tabs and the text of the "Calc" tab.
11. In "Squares", change the formula of column 2 to 2 to the power of column 1 and recalculate it in the default scripting language. Print the sum of column 2.
12. Delete the fit functions "ai/RefLine" and "ai/RefDecay" from the library. Print the list of the two names that still exist afterwards.
```

**Expected results** (Python as the global scripting language):

| Item | Expected line | What a wrong answer usually means |
|------|---------------|-----------------------------------|
| 1 | `R1: 385` | `^` used in Python (XOR → error, empty column) |
| 2 | `R2: 3025` | muParser not forced, so `^` ran in Python |
| 3 | `R3: 1 ZeroDivisionError` | error not caught, or the reason read from somewhere else than the `ValueError` |
| 4 | `R4: True 385` | read-only ignored, or the formula's result printed instead of the unchanged sum |
| 5 | `R5: 1234,5 0,25 -3` then `2.5` | number format not applied, or `text()` mistaken for the number |
| 6 | `R6: 100 6 0` (`-0.0` counts as 0) | `integrate()` taken for the sum of the cells (gives 56.25), (col, row) order, or 0-based i/j |
| 7 | `R7: 2.0000 1.0000` | not compiled (only saved), no group, or parameters left fixed |
| 8 | `R8: 3.0000 0.5000 0.2000` | initial guesses printed instead of `results()`, or parameters fixed |
| 9 | `R9: 2 2 1 True` | `GraphWindow` used where a layer is needed, residuals not from `fitCurve-ai/RefDecay` col 5, or a different model fitted again |
| 10 | `R10: 2 print(6*7)` | `renameTab()` given a 1-based index, or the tabs executed |
| 11 | `R11: 2046` | `^` in Python again (the item depends on remembering item 1's rule) |
| 12 | `R12: []` | `delete()` silently no-ops on a misspelled name — it never raises |

Reference run (2026-10-02): every value above. It also found a bug, fixed since: `setDecimalSeparators()`
passed the new locale where the old one was expected, so the table neither changed its format nor
converted its cells (item 5 printed `1234.5 0.25 -3` and an empty cell).

First run (2026-10-02, `LIST NOSPLIT NOREVIEW NOEXPLORE`): 9 of 12. Item 3 stopped and said so: the
formula error the script had caught still counted as the script's failure, so the retries went on
(fixed since). Item 6 printed `integrate()` as the sum; item 9 read "item 8" as the 8th fit function
(the wording, fixed since). No `R<n>:` lines: the instruction line was dropped when the list rewrite
could not be parsed (fixed since: it is then appended to every item).

**After the LIST, scoring request** — a separate message, sent once the LIST has finished. Keep the
expected values out of the LIST itself: the LIST rewrite call sees every item at once and could
carry them into the earlier ones.

```
Compare table "Results" with these expected answers, ignoring number formatting (385.0 equals 385, -0.0 equals 0) and the "Rn:" prefix: 1: 385; 2: 3025; 3: 1 ZeroDivisionError; 4: True 385; 5: 1234,5 0,25 -3 then 2.5; 6: 100 6 0; 7: 2.0000 1.0000; 8: 3.0000 0.5000 0.2000; 9: 2 2 1 True; 10: 2 print(6*7); 11: 2046; 12: []. Add a column "ok" to "Results" with True or False per row (False for an empty answer), and print "Score: <number of True> of 12".
```

What the assistant can and cannot judge: every LIST item starts a new task, so no item sees the
answers or the console of the earlier ones. A table written by the items is the only way for a later
request to read their results; hence "Results". Efficiency is NOT visible to the assistant: the time
per item, the number of scripts run and the retries are printed by the LIST runner, never passed to
the model. Read them from the console instead:

| Measure | Where in the console |
|---------|---------------------|
| total time | `AI LIST: all 12 item(s) completed (took …, … total)` |
| time per item | `AI LIST: item n/12 (took …)` — the time of the PREVIOUS item |
| scripts run per item | the `AI code (running)` lines between two `AI LIST: item` lines |
| skipped items | `LIST item n/12 failed — skipping it` |
| tokens sent | `AI: sending ~…K tokens` per call |

Save the project after each mode ("Results" with its "ok" column stays in it), then compare:

| Mode | Score | Honest failures | Silent failures | Total time | Scripts run |
|------|-------|-----------------|-----------------|------------|-------------|
| `LIST` | | | | | |
| `LIST NOSPLIT` | | | | | |
| `LIST NOREVIEW` | | | | | |
| `LIST NOEXPLORE` | | | | | |
| `LIST NOSPLIT NOREVIEW NOEXPLORE` | | | | | |

Score per mode: the number of `R<n>` lines equal to the table, plus, for every wrong or missing
line, whether the assistant itself reported a failure (honest) or reported success (silent). The
same item failing in all modes points at the docs; failing in only one mode points at that stage.
