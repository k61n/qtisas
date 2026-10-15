# AI Assistant — Real Test Requests

Realistic requests for testing the AI assistant, from single requests to whole conversations.

| Part | What | How it runs |
|------|------|-------------|
| [1. Topic detection](#part-1--topic-detection) | one request per entry, with the doc topics it must select | automatically, by `ai_content_tests` |
| [2. LIST requests](#part-2--list-requests) | whole lists of tasks; LIST 3, LIST 4 and LIST 5 are also the [benchmarks](benchmarks/README.md) | by hand, pasted into the app |
| [3. Conversations](#part-3--conversations) | several turns, typed one at a time | by hand |
| [4. Commands, markers, settings, rules](#part-4--commands-markers-settings-rules) | what QtiSAS handles itself, and what switches an AI stage on or off | by hand |

## Before a manual test

- **Sending:** type into the status bar input line (bottom of the main window), then
  - **Ctrl+Shift+Enter** — ask **and run** (every test below, unless it says otherwise);
  - **Ctrl+Enter** (Cmd+Enter on macOS) — ask only: the script is shown, nothing runs ("generate-only").

  The Ask AI button of a Script or Note window does the same: click = ask only, Ctrl+Shift+click = ask and run.
- **Clean start:** a new project, and `clear history` in the input line.
- **Console lines:** the AI's own messages start with `▸▸▸ AI`. The expected messages below are quoted without
  that prefix.

This file is not sent to the AI and not shipped: the AI only reads `qtisas/python/doc/*-python-api.{md,pyi}`.

# Part 1 — Topic detection

Before writing a script, the AI picks which API docs to send (table, matrix, graph, graph3d, note, compile,
fittable). Each entry below is one request and the topics that must be picked from its text alone. The C++ test
`realRequests` reads them from this file.

**Format — the test parses it, keep it exact:**
- a `##` heading (the test row's name);
- `**Request:**`, then the request on ONE line starting with `>`;
- `**Expected topics:**` with a comma-separated list;
- a `**Why:**` note. No other part of the file may have a line starting with `**Expected topics:**`.

## Compound multi-topic fit-and-plot request

**Request:**
> Create a table "GaussPeak" with 200 rows of synthetic Gaussian-peak data (x, y, dy columns) with 5% noise. Compile a custom Gaussian peak fit function and a Lorentzian fit function. Fit both models with weighted errors. Compare chi2/dof and report which model fits better. Plot the winning model's data, fit curve, and residuals into graph "GaussPeakFit" with two stacked panels, a styled legend, and axis titles.

**Expected topics:** table, compile, fittable, graph

**Why:** the standard regression request — table, compile, fit and graph in one. It has found most of the
backstops in `QtiSasAiContent.cpp`.

## Plain table creation

**Request:**
> Create a table "SAXS_data" with 3 columns Q, I, dI and fill it with synthetic small-angle-scattering intensities for a sphere form factor plus 3% Gaussian noise.

**Expected topics:** table

**Why:** a single topic with no fit or plot — no unrelated topic may be added.

## Matrix formula fill

**Request:**
> Create a 50x50 matrix "SphereMap" and fill every cell using the formula sin(x)*cos(y) via setFormula() and calculate().

**Expected topics:** matrix

**Why:** matrices have no checklist of their own, so this keyword match is their only mechanical safety net.

## 3D surface plot

**Request:**
> Plot a 3D surface of sin(x)*cos(y) over x and y in the range -pi to pi, using a filled mesh style, then add a legend and axis labels.

**Expected topics:** graph3d

**Why:** natural wording without "graph3d", "matrix" or "graph". It once found no topic at all; "3D",
"surface plot" and "plot3d" now select graph3d.

## Auto-executing note

**Request:**
> Create a note "Setup" with an auto-executing script tab that configures analysis defaults when the project file is opened; keep it as the active tab so it actually runs.

**Expected topics:** note

**Why:** autoexec runs only the active tab — "keep it as the active tab" is that real constraint.

## Custom function compile + simulate (no real data)

**Request:**
> Compile a custom Porod power-law fit function with an adjustable exponent parameter, then simulate a curve from it without fitting any real data.

**Expected topics:** compile, fittable

**Why:** compile and fittable are separate topics; both are needed even though nothing is fitted.

## Fit an existing library model against real data

**Request:**
> Fit the existing "Sphere" form-factor model from the Fittable library against table "SAXS_data" columns I and dI, using weighted Levenberg-Marquardt, and report chi2/dof.

**Expected topics:** fittable, table

**Why:** no compile at all, and the word "fittable" never appears — it must come from the synonyms.

## Restyle an existing graph

**Request:**
> Restyle the existing graph "FitResults" — set the legend font to bold 12pt, change the first curve's line color to red, and add minor tick marks on both axes.

**Expected topics:** graph

**Why:** styling only; the name "FitResults" must not pull in the fit topics.

## Delete fit functions from the library

**Request:**
> Delete the fit functions "ai/RefLine" and "ai/RefDecay" from the library, then print which of the two still exist.

**Expected topics:** compile

**Why:** LIST 3 item 12. It matched no topic until "fit function(s)" became a compile synonym — deleting,
opening and listing library functions is the Compiler's job.

## Note script reading a table on autoexec

**Request:**
> Add a script tab to note "Batch" that reads table "RawScan" and prints its row count each time the project loads via autoexec.

**Expected topics:** note, table

**Why:** two topics in one request — both must be found, not just the first one checked.

# Part 2 — LIST requests

A LIST runs its numbered items one after the other, each as its own request. Paste a whole block into the app.

- Markers go on the `LIST` line, e.g. `LIST NOSPLIT NOREVIEW NOEXPLORE` (see Part 4).
- A line between `LIST` and item 1 is an instruction: it is appended, as written, to every item.
- The blocks are fenced code, not `>` quotes, so the Part 1 test skips them.

## LIST 1 — full AI integration test

**Tests:** the whole LIST path — parsing, doc topics per item, the existence check, and both single-script and
split generation.

**Content:** every Part 1 request, ordered from simplest to most complex, so an early failure points at the
simplest cause. Items 1, 2 and 10 are extra: they create "Batch", "RawScan" and "FitResults", which later items
use.

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
9. Fit the existing "sasviewmodels/sphere" form-factor model (scale, background and radius adjustable) from the Fittable library against table "SAXS_data" columns I and dI, using weighted Levenberg-Marquardt, and report chi2/dof.
10. Plot the fit results from the previous step into graph "FitResults", including the data, fit curve, and residuals.
11. Restyle the existing graph "FitResults" — set the legend font to bold 12pt, change the first curve's line color to red, and add minor tick marks on both axes.
12. Create a table "GaussPeak" with 200 rows of synthetic Gaussian-peak data (x, y, dy columns) with 5% noise. Compile a custom Gaussian peak fit function and a Lorentzian fit function. Fit both models with weighted errors. Compare chi2/dof and report which model fits better. Plot the winning model's data, fit curve, and residuals into graph "GaussPeakFit" with two stacked panels, a styled legend, and axis titles.
```

**Pass when:**

| Item | Pass when |
|------|-----------|
| all | `AI LIST: item n/12` for every item, then `all 12 item(s) completed`; each item's doc topics include its Part 1 entry's expected topics; no item reported as done without its object |
| 1–2 | note "Batch" (empty), table "RawScan" with 50 rows and columns x, y |
| 3 | a new tab in "Batch", set to autoexec, whose script prints `table("RawScan")`'s row count; "Batch" and "RawScan" are not created again |
| 4 | note "Setup": its active tab is autoexec and switches automatic recalculation off |
| 5 | matrix "SphereMap", 50×50, filled from `sin(x)*cos(y)` (not all zeros) |
| 6 | a 3D surface plot, filled mesh, with legend and axis labels |
| 7 | table "SAXS_data": Q, I, dI (roles X, Y, yErr), sphere-like intensities with noise |
| 8 | a new `ai/` power-law function with an adjustable exponent, compiled; a simulated curve; no fit run |
| 9 | `sasviewmodels/sphere` fitted to "SAXS_data" with weights dI, scale/background/radius varied, chi²/dof printed from the fit result |
| 10 | graph "FitResults" with the data, the fit curve and the residuals |
| 11 | the same "FitResults" (no new graph): legend bold 12 pt, first curve red, minor ticks on both axes |
| 12 | runs as split parts (`AI split-mode: part i/N`); "GaussPeak" with 200 rows, two compiled functions, two fits, the better chi²/dof named, graph "GaussPeakFit" with two stacked layers, legend and axis titles |

**Checkable version:** [benchmarks/list1.json](benchmarks/list1.json) — the same twelve tasks with fixed data and
names and one `R<n>:` line each, for `qtisas_benchmark.py` (see "LIST 1 and LIST 2 as benchmarks" in
[benchmarks/README.md](benchmarks/README.md)). The list above stays the manual test: it also checks what a result
line cannot, such as the doc topics per item and the split parts of item 12.

## LIST 2 — extended AI mechanics test

**Tests:** what LIST 1 cannot reach, because each of its items creates something new:

| Item | Tests |
|------|-------|
| line 1 | the instruction line reaches every item, not only the first |
| 2 | the existence check finding an object that exists; Z role (not Y), so "Baseline" keeps one Y column |
| 3 | EXPLORE first (list the compiled functions), then act |
| 4 | compiling the same `ai/` name twice on purpose: the second overwrites the first |
| 5 | an underspecified request ("the model", "the data"): no invented model name |
| 6–7 | "it" referring to what the previous item created; item 6 gives "Metrics" a real z with role Z |

**Run it twice:** as written (stages as set in Preferences — item 3 needs EXPLORE), then with
`LIST NOSPLIT NOREVIEW NOEXPLORE`.

```
LIST
For every item, use the exact object names given and verify the underlying logic (formulas, fit setup, column roles) before considering it done.
1. Create a table "Baseline" with 30 rows of x, y sample data.
2. Add a column "z" to the existing table "Baseline" computed as x*y, using the Z column role — reuse the table you just created, don't make a new one.
3. Check which sasviewmodels fit functions are already compiled, then simulate one of them without fitting any real data.
4. Compile a custom exponential-decay fit function named "ai/ExpDecay" with parameters scale and rate. Then recompile "ai/ExpDecay" again, this time adding an extra "offset" parameter — the second call should intentionally overwrite the first, not fail or create a duplicate.
5. Fit the model against the data.
6. Create a note "Log", a table "Metrics" with 20 rows of x, y, and z sample data (z computed as a function of x and y, using the Z column role), and a styled graph of it with a legend — all in one request.
7. Plot it as a 3D surface instead, using a filled mesh style.
```

**Pass when:**

| Item | Pass when |
|------|-----------|
| all | every item follows the instruction line (exact names, logic checked) |
| 1 | table "Baseline", 30 rows, x and y |
| 2 | the existence check finds "Baseline" (no "stopping — … refers to"); column "z" = x*y with role Z in the SAME table, no new table |
| 3 | an `AI code (exploring)` round lists the compiled `sasviewmodels` functions before the action; one of them simulated, no fit (with `NOEXPLORE`: one script does both, or the list is printed and the item says what it chose) |
| 4 | one function "ai/ExpDecay" with scale, rate and offset; the second compile overwrites the first without an error |
| 5 | "ai/ExpDecay" fitted to "Baseline" (or the item says which data it chose); no invented model name |
| 6 | note "Log"; table "Metrics", 20 rows, z from x and y with role Z; a graph of it with a legend |
| 7 | a 3D surface of "Metrics" with a filled mesh — not of another table, and not skipped |

**Seen in earlier runs:** with Y instead of Z, item 5 failed on "expected exactly one Y column; found ['y',
'z']". Without the role in item 6, "z" got role Y, and item 7 noticed it but gave up silently, without a 3D
plot.

**Checkable version:** [benchmarks/list2.json](benchmarks/list2.json) — the same seven tasks with fixed data and
names and one `R<n>:` line each (see "LIST 1 and LIST 2 as benchmarks" in [benchmarks/README.md](benchmarks/README.md)).

## LIST 3, LIST 4 and LIST 5 — the benchmark lists

**Tests:** every item prints one checkable `R<n>: …` line. The expected values are in
[benchmarks/list3.json](benchmarks/list3.json), [benchmarks/list4.json](benchmarks/list4.json) and
[benchmarks/list5.json](benchmarks/list5.json);
`benchmarks/qtisas_benchmark.py` runs and scores the same lists automatically.

**Before pasting**, delete the functions the list creates (Python console):

```python
Compiler.delete("ai/RefLine"); Compiler.delete("ai/RefDecay"); Compiler.scanGroups()   # LIST 3
Compiler.delete("ai/RefGauss"); Compiler.scanGroups()                                   # LIST 4
Compiler.delete("ai/Quad"); Compiler.delete("ai/Gauss5"); Compiler.delete("ai/Lor5"); Compiler.scanGroups()  # LIST 5
```

**After editing a JSON file**, regenerate its text here: `qtisas_benchmark.py <list>.json --print`, without
the "Item n:" prefixes.

**LIST 3:**

```
LIST
Use exactly the object names given. End the item by printing exactly ONE line: the result label the item gives, then only the values asked for, in the order asked, separated by spaces.
1. Create a table "Squares" with 10 rows and 4 columns; fill column 1 with 1, 2, ..., 10. Give column 2 a column formula that squares column 1 and recalculate it in the default scripting language. Print the sum of column 2. The result label is "R1:".
2. In "Squares", give column 3 a formula for the cube of column 1 written in muParser syntax with ^, and recalculate it with muParser explicitly. Print the sum of column 3. The result label is "R2:".
3. In "Squares", give column 4 the formula 1/(column 1 - 5) and recalculate it. Print the number of empty cells in column 4 and the name of the Python error the formula caused. The result label is "R3:".
4. Make column 2 of "Squares" read-only, set its formula to 10 times column 1 and try to recalculate it. Print whether the recalculation raised an error and the sum of column 2 afterwards. Make the column writable again. The result label is "R4:".
5. Create a table "Format" with 3 rows and 1 column holding 1234.5, 0.25 and -3. Switch its decimal separators to German (comma), without group separators, and read the three cells as displayed text. Then write the text "2,5" into row 2 and read the number that cell now holds. Print the three texts read first, then that number. The result label is "R5:".
6. Create a 4x4 matrix "Prod" and fill it with the formula i*j. Print the sum of all cells, the value in row 2, column 3, and the determinant rounded to 6 decimals. The result label is "R6:".
7. Compile a new fit function "ai/RefLine" for y = a*x + b with both parameters adjustable. Create a table "LineData" with 20 rows: x = 1..20, y = 2x + 1, dy = 0.1 (roles X, Y, yErr). Fit "ai/RefLine" to it with Levenberg-Marquardt, weighted by dy. Print a and b with 4 decimals. The result label is "R7:".
8. Compile a new fit function "ai/RefDecay" for y = A*exp(-k*x) + c with all three parameters adjustable. Create a table "DecayData" with 101 rows: x = 0, 0.1, ..., 10, y = 3*exp(-0.5*x) + 0.2, dy = 0.01 (roles X, Y, yErr). Fit it starting from A = 1, k = 1, c = 0. Print A, k and c with 4 decimals. The result label is "R8:".
9. Create a graph "DecayPlot" with two stacked layers: the top one with the "DecayData" points and the fitted curve from table "fitCurve-ai/RefDecay", the bottom one with that table's residuals. Print the number of layers, the number of curves in the top and in the bottom layer, and whether the largest absolute residual is below 1e-4. The result label is "R9:".
10. Create a note "Report" whose first tab contains print('A') and a second tab named "Calc" containing print(6*7). Do not run them. Print the number of tabs and the text of the "Calc" tab. The result label is "R10:".
11. In "Squares", change the formula of column 2 to 2 to the power of column 1 and recalculate it in the default scripting language. Print the sum of column 2. The result label is "R11:".
12. Delete the fit functions "ai/RefLine" and "ai/RefDecay" from the library. Then print which of these two names still exist (a list, [] if none). The result label is "R12:".
```

**LIST 4:**

```
LIST
Use exactly the object names given. End the item by printing exactly ONE line: the result label the item gives, then only the values asked for, in the order asked, separated by spaces.
1. Create a note "Steps" with three tabs named "A", "B" and "C" (the first tab renamed too); each tab contains print() of its own name, e.g. print('A'). Do not run them. Print the number of tabs, the text of tab "C" and the text of tab "A". The result label is "R1:".
2. Compile a new fit function "ai/RefGauss" for y = A*exp(-(x-x0)^2/(2*s^2)) + c with all four parameters adjustable. Create a table "Peak" with 41 rows: x = -2, -1.9, ..., 2 and y = 5*exp(-x^2/0.18) + 1 (roles X, Y). Fit it starting from A = 1, x0 = 0.5, s = 1, c = 0. Print A, x0, s as a positive number, and c, each with 4 decimals. The result label is "R2:".
3. Create a matrix "Grid" with 3 rows and 4 columns and fill it with the formula i+10*j. Print the value in row 3, column 4, and the sum of row 3. The result label is "R3:".
4. In table "Peak", add a column named "y2" with a column formula that is 2 times the y column, recalculate it, and print the sum of "y2" with 4 decimals. The result label is "R4:".
5. Create a table "Names" with 3 rows and 2 columns: column 1 holds 1, 2, 3 and column 2, in text format, holds "alpha", "beta", "gamma". Print the text in row 2 of column 2 and the number in row 3 of column 1. The result label is "R5:".
6. Close and delete the table "Names" and the matrix "Grid". Print whether each still exists afterwards, table first. The result label is "R6:".
```

**LIST 5** (combined tasks: each item chains several steps and reuses what earlier items created):

```
LIST
Use exactly the object names given. End the item by printing exactly ONE line: the result label the item gives, then only the values asked for, in the order asked, separated by spaces.
1. Create a table "Calib" with 21 rows and three columns: "x" (role X) = 0, 0.1, ..., 2; "y" (role Y) given by a column formula 3*x^2 - 2*x + 1 recalculated in the default scripting language; "dy" (role yErr) = 0.05 everywhere. Compile a new fit function "ai/Quad" for y = a*x^2 + b*x + c with all three parameters adjustable, and fit it to "Calib" weighted by dy, starting from a = b = c = 1. Print a, b and c with 4 decimals, and the number of rows where y is greater than 5. The result label is "R1:".
2. Create a table "Peak5" with 81 rows: x = -4, -3.9, ..., 4 and y = 2/(1 + (x/0.5)^2) + 0.1 (roles X, Y). Compile two new fit functions, "ai/Gauss5" for y = A*exp(-x^2/(2*w^2)) + c and "ai/Lor5" for y = A/(1 + (x/w)^2) + c, each with A, w and c adjustable. Fit both to "Peak5", each starting from A = 1, w = 1, c = 0. Print the name of the function with the smaller chi2/dof, then its fitted A and w (as a positive number), each with 4 decimals. The result label is "R2:".
3. Create a graph "Report5" with two stacked layers: the top one with the "Calib" y data as points and the fitted curve from table "fitCurve-ai/Quad" as a line, the bottom one with that table's residuals. Set the Y axis title of the top layer to "y" and the X axis title of the bottom layer to "x". Print the number of layers, the number of curves in the top and in the bottom layer, and the largest absolute residual with 4 decimals. The result label is "R3:".
4. Create a 21x21 matrix "Saddle" spanning x and y from -1 to 1 and fill it with x*x - y*y. Plot it as a 3D surface in a window named "Saddle3D". Print the value at the centre of the matrix (row 11, column 11), the smallest and the largest value of all cells, and whether a 3D window "Saddle3D" exists. The result label is "R4:".
5. Create a note "Pipeline" with two tabs named "make" and "check" (the first tab renamed too). The "make" tab holds a script that creates a table "Auto" with 5 rows whose column 1 is 1, 2, ..., 5; the "check" tab holds a script that prints the sum of column 1 of "Auto". Run only the "make" tab. Print the number of tabs, whether table "Auto" exists, and the sum of its column 1. The result label is "R5:".
6. Delete the fit functions "ai/Quad", "ai/Gauss5" and "ai/Lor5" from the library, and close and delete the table "Peak5" and the note "Pipeline". Print which of the three function names still exist (a list, [] if none), then whether "Peak5" and "Pipeline" still exist. The result label is "R6:".
```

# Part 3 — Conversations

Each test is a sequence of turns. Type one, wait until it has finished, then type the next.

## Short confirmation continues the task

| # | Type | Expected |
|---|------|----------|
| 1 | `make a 3D plot with 3 spheres connected along one line: radius ratio 5:1:3` | a 3D plot |
| 2 | `it looks like pressed along the aligned line. Could be scaling of all 3 axes the same?` | the AI proposes a fix |
| 3 | `go` | the proposed fix is applied to the same plot |

**Never:** turn 3 ending as `scriptPrint("Ready.")` or another stub reported as success.

**Why:** "go" contains no topic word. It once lost the 3D docs, self-review rejected the correct fix as out of
scope, and the run ended as a stub with no sign that nothing was fixed. A short reply must keep the topics of
the task it continues.

## Questions are answered directly

**Setup:** "Split large requests into parts by default" ON (Preferences → Ask AI); table "Squares" with 10 rows
(LIST 3 item 1 creates it).

| # | Type | Expected |
|---|------|----------|
| 1 | `How many rows does table "Squares" have?` | `a question — answering it directly, without a split plan.`; a script reads the table and prints 10 |
| 2 | `What is the difference between setFormula() and setCommand()?` | the same line; a text-only answer, with no `AI code (self-review pending)` round |
| 3 | `Can you plot table "Squares" as a scatter graph?` | NOT treated as a question ("plot" is an action): a split plan, then the graph |

**Rule:** a question ends in "?", is at most 300 characters and has no action verb (create, plot, fit, …).

## New task or continuation

| # | Type | Expected |
|---|------|----------|
| 1 | `Create a table "T1" with 5 rows of x, y sample data.` | table "T1" |
| 2 | `Create a 10x10 matrix "M1" filled with i*j.` | `this looks like a new, unrelated task — earlier conversation won't be resent as context`; matrix "M1" |
| 3 | `and the same for the other one` | either `not sure whether this continues the current task or starts a new one — please clarify`, or a script that clearly continues "M1" |
| 4 | (only after the clarification) `new task` or `continue` | handled as answered |

**Never:** table "T1" created again. `history` still lists every turn.

**Rule:** at the clarification, a reply containing new, different, unrelated or separate means a new task;
anything else means continue.

## Generate-only, then run it

| # | Type | Expected |
|---|------|----------|
| 1 | `Create a table "Gen" with 3 rows of x, y sample data.` with **Ctrl+Enter** | the script is shown, nothing runs: no "Gen"; an EXPLORE script is shown as `EXPLORE not run — generate-only mode` |
| 2 | `run it` | `running the previously shown script directly (no re-generation needed)`; "Gen" created, no call to the model |
| 3 | `run it again` | `running the previous script again directly`, no call to the model |
| 4 | `history` | the numbered turns |
| 5 | `run Q1` | `running turn #1's script directly` |
| 6 | `run Q99` | `no turn #99 in history (N turn(s) recorded).` |
| 7 | `run it and then plot column y of table "Gen"` | a normal request (only a bare "run it" is the shortcut): the model is asked, the graph is made |

## Giving up honestly

| # | Type | Expected |
|---|------|----------|
| 1 | `Plot the existing table "NoSuchTable" as a scatter graph.` | `stopping — this request refers to "NoSuchTable" as already existing, but no match was found for it. Nothing was created or run.` — no window |
| 2 | `create a new one` | goes ahead and creates it |
| 3 | `Fit the existing "nosuchgroup/NoSuchModel" model to table "Squares" and print chi2/dof.` | stops and names the missing function, or lists the available ones and stops (`the same list of available alternatives was offered …` or `stopped after N automatic EXPLORE/retry rounds`) |

**Never:** another model fitted in its place, or a chi²/dof printed in turn 3.

## Busy, then stop

**Setup:** paste LIST 4 (Part 2); type the turns while its first item is still running.

| # | Type | Expected |
|---|------|----------|
| 1 | `Create a table "Late" with 2 rows.` | `previous request still in progress.`; nothing changes |
| 2 | `run it again` | the same |
| 3 | `stop` | `request cancelled.`; no further LIST item starts; a late reply of the stopped call is ignored (no crash) |
| 4 | `Create a table "After" with 2 rows.` | a normal request — not taken for the next LIST item; "After" created |

# Part 4 — Commands, markers, settings, rules

## Commands answered by QtiSAS itself

None of these may call the model (no "sending" line, no tokens).

| Type | Expected |
|------|----------|
| `status` (or `ai status`) | Model, URL, Docs (compact/full/integrated), History (turns), Max resp |
| `model`, `url` | the preset's model and base URL, `(not set)` if empty |
| `history` (also `show history`, `request history`) | the numbered turns, including those before a "new task" boundary |
| `clear history` (also `reset history`) | `conversation history cleared.`; `history` is then empty |
| `stop` / `cancel` while idle | `request cancelled.`, nothing else changes |
| `STOP` | the same as `stop` (commands ignore case) |
| `history please`, `stop the fit` | not a command: sent to the model as a request |

## Markers in a single request

A marker is a CAPITAL word anywhere in the request: `SPLIT`, `REVIEW`, `EXPLORE`, or the same with `NO`
(`NOSPLIT`, also `NO SPLIT`). The `NO` form wins. Request: the Part 1 "Compound multi-topic" one.

| Setting | Request | Expected |
|---------|---------|----------|
| split by default OFF | request + `SPLIT` | `AI split-plan (…)` with parts, then `part i/N done` |
| split by default ON | request + `NOSPLIT` | no split plan, one script |
| any | request + `split` (lower case) | no effect: the word stays in the request |
| self-review OFF | request + `REVIEW` | `AI code (self-review pending)` before the run |
| self-review ON | request + `NOREVIEW` | no self-review round |
| EXPLORE OFF | `Fit table "Squares" with one of the compiled sasviewmodels functions EXPLORE` | an `AI code (exploring)` round first |
| EXPLORE ON | the same with `NOEXPLORE` | no exploring round; an EXPLORE script sent anyway is shown as `EXPLORE disabled — not run` |

Settings (Preferences → Ask AI): "Split large requests into parts by default", "Mandatory pre-execution
self-review", "Allow EXPLORE probe rounds".

## Settings (Preferences → Ask AI)

| Setting | Test | Expected |
|---------|------|----------|
| Max split parts = 2 | the compound request + `SPLIT` | `AI split-plan: N parts requested — capping at 2`, or 2 parts or fewer |
| "Request a merged reference script after split parts complete" ON | the same | `requesting merged script`, then `AI split-mode merged script (… NOT executed, verify before reuse)`; nothing runs again |
| the same OFF | the same | `merge step skipped (disabled)`; inside a LIST always `(LIST mode)` |
| Auto-continue limit = 1 | Part 3 "Giving up honestly", turn 3 | stops after 1 round, saying why |
| API docs sent: Compact / Full / Integrated | `status`, then any request | `Docs:` follows the setting; the loaded-docs line names `.pyi` files, `.md` files, or none |

## User rules (Python console)

| # | Do | Expected |
|---|----|----------|
| 1 | `removeAllRules()`, then `addRule('Name every new table you create with the prefix "T_" unless the request gives a name.')` | — |
| 2 | `showRules()` | `1. Name every new table …` |
| 3 | ask `Create a table with 3 rows of sample data.` | the new table's name starts with "T_" |
| 4 | `removeRule(1)`, then the same request | no prefix |
| 5 | `removeRule(99)` | `ValueError: removeRule: no rule at index 99 (have 0).` |
| 6 | "User Rules limit" = 1, then `addRule("a")`, `addRule("b")` | the second raises `ValueError: addRule: rule limit reached (1/1)…` |
| 7 | clean up: `removeAllRules()`, limit back to 20 | — |

The AI itself never calls `addRule()`.
