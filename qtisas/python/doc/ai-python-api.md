# QtiSAS AI Assistant

The AI assistant is built into QtiSAS and can generate Python scripts, explain the API,
and help automate data analysis workflows.

---

## For Users: How to Use the AI Agent

---

### Entry points

The AI can be invoked from three places:

---

**1. Status bar input line** — bottom of the main window

| Key | Action |
|-----|--------|
| `Enter` | Run as a direct command / short script |
| `Ctrl+Enter` (`Cmd+Enter` on macOS) | Send to AI — code shown in Scripting Console, **not** run |
| `Ctrl+Shift+Enter` | Send to AI — code **auto-executed** |
| `?` | Show help summary in the console |

---

**2. Script Window** — **Ask AI** button (top-right)

- Sends the **selected text** to the AI; if nothing is selected, sends the entire script.
- Click → generate only; `Ctrl+Shift`+Click → generate and run.

---

**3. Note window** — **Ask AI** button (tab corner)

- Sends the **full content of the current tab** to the AI.
- Click → generate only; `Ctrl+Shift`+Click → generate and run.

---

### Setup

Before using the AI for the first time, configure the provider in **Settings → AI**:
set the API key, model, and base URL.

---

### Modes

**Generate code only** — use when you want to review the script before running it.
The code appears in the Scripting Console; copy it to the Script Window to edit and run.

**Generate and run** — generated code is executed immediately after it arrives.
Use for well-defined, trusted requests.

---

### What the AI can do

- Generate Python scripts using the QtiSAS API (tables, matrices, graphs, fitting).
- Explain any QtiSAS Python function or workflow.
- Create datasets, run fits, and plot results in a single script.
- Batch-process multiple datasets.
- Author, edit, compile, and save custom fitting functions — new or existing — via the `Compiler`
  class (`c = Compiler`) — see `compile-python-api.md`.

### What the AI cannot do

- Access files outside the QtiSAS workspace or make network requests.

---

### Tips for better results

- Name your tables and columns clearly — the AI uses them to build column references.
- Mention the fitting function name explicitly (e.g. `"Guinier"`, `"sphere_SANS"`).
- Use `f.fitFunctions()` to list available function names, `f.datasets()` to list Y-columns.
- When a script's only job is printing several lines of static, non-computed text (a summary,
  a status report), build ONE multi-line string and call `scriptPrint()` once — not one call
  per line. This doesn't apply when lines are printed one at a time as they're computed (e.g.
  progress inside a loop, or values only known after a call earlier in the same line).

---

### User Rules

A small, manually-curated list of your own rules, sent to the AI on every request (unlike the
built-in docs, which are omitted on automatic error-retry follow-ups to save context). Use it for
corrections the built-in docs don't cover — e.g. a lab convention, a preferred default, or a
recurring mistake.

| Call | Notes |
|------|-------|
| `addRule(text)` | add a rule; raises `ValueError` at the rule-count limit (default 20, configurable in **Preferences → AI**) |
| `removeRule(index)` | remove a rule by its 1-based position (see `showRules()`); raises `ValueError` if out of range |
| `removeAllRules()` | remove every rule at once |
| `showRules()` | print the current numbered rules to the console and return them as a list |

The exact current count/limit are shown live: the "User Rules" section below (if any rules are
set) is headed `## User Rules (N/limit)`. Trust that heading over the "default 20" figure above
if they differ.

```python
addRule("Our SANS beamline reports Q in 1/nm, not 1/Å — never convert.")
showRules()          # 1. Our SANS beamline reports Q in 1/nm, not 1/Å — never convert.
removeRule(1)
removeAllRules()     # start over
```

**This is manual only** — the AI never calls `addRule()` on its own initiative, even after being
corrected; add a rule yourself to persist it across sessions.

If `addRule()` raises the rule-limit error, **report the message and its three options — do not
autonomously call `removeRule()` / `removeAllRules()` / change the Preferences limit.** Which
rule to remove, or whether to raise the limit, is the user's call — even after they say to
proceed. A vague "try again"/"go ahead" authorizes making room but doesn't say *which* rule to
sacrifice — ask, unless the instruction already specifies a target (by index, content, or
"remove the oldest"). Picking one yourself (e.g. by index order) can discard a valuable rule
while leaving near-duplicates intact.

Rules are stored in `~/.config/qtisas/ai/user-rules.md` (plain text, editable directly), and the
currently active list is also shown in its own **Help → Python API → User Rules** window.

> **RULE — Before every action script runs, you will automatically be asked to review it once against a checklist (User Rules + a few known silent-failure gotchas). Take that pass seriously — rules like these get missed under task pressure even when known.**
> The message: `Before this runs, check it against the checklist below. If it violates anything, return ONLY the corrected script. If not, return the SAME script unchanged.` A real check — return the corrected script if anything applies, else unchanged; no explanation, code only. Happens exactly once per candidate script, regardless of source (fresh request, EXPLORE follow-up, or error-retry correction).
>
> **Checklist items — including User Rules — are DEFAULTS, not overrides.** If the request explicitly specifies something contradicting a checklist item for that aspect (e.g. names a color while a rule sets a different default), the explicit request wins — apply a checklist item only where the request left that aspect unspecified. Confirmed live: a request asking for a blue line got silently forced back to green by a standing "use green lines" rule during self-review.
>
> **This review is a compliance check, not a chance to improve the script.** Only fix what the checklist requires — never add functionality, styling, or embellishments beyond it. Confirmed live: reviewing a "plot it nicely for a presentation" request, the review pass added unrequested tick-length styling with a wrong argument count, causing a `TypeError` absent from the original draft — anything added during review is itself unreviewed, equally risky code.

---

## For AI: Agentic Explore Mode

### Scope — EXPLORE, self-review and error-retry are AUTO-RUN only

> **EXPLORE round-trips, the mandatory self-review pass and the automatic error-retry loop all fire
> only in AUTO-RUN mode** — `Ctrl+Shift+Enter` at the status bar, or `Ctrl+Shift`+Click in the
> Script Window / Note "Ask AI" button.
>
> In plain "generate only" mode (bare `Enter`/Click) a probe round is **refused**: the request will
> say so outright, and a `# EXPLORE` script returned anyway is never executed, so no output comes
> back and the round-trip is wasted. A probe is model-authored code, and the read-only guarantee is
> a blocklist of recognized write calls rather than a sandbox — it has leaked twice, once running an
> entire `Compiler` authoring sequence for real. Executing that when the user declined execution is
> not a risk worth taking, so the probe is refused instead. When told probing is unavailable, read
> the live value AT RUN TIME inside the action script and branch on what you find.
>
> What generate-only mode does NOT do is run the ACTION script: it is shown once, as the answer,
> with no execution, no output capture, no self-review pass and no error-retry. So a mistake in the
> action script is not corrected for you there — it is simply displayed. Write it as if it were
> going to run unchecked, because it will not get a second round.

### PRE-FLIGHT checklist

> **PRE-FLIGHT — answer these before writing ANY script:**
> 1. **Does the task ask you to READ/analyze/fit/plot data the user implies already exists** (a named table, "the data", "my results", a column referenced as if already there)? → `table("name")` / `matrix("name")` / `graphWindow("name")` / `note("name")` return **`None`** (confirmed) when missing — **always check `if t is None:` before using the result**, even for a single-line read: `t = table("X"); scriptPrint("not found" if t is None else str(t.cell(2, 3)))`. Beyond a single read — needing column names/structure first — **run EXPLORE** (`existTable("X")`, or `matrix("X") is not None` / `graphWindow("X") is not None` / `existNote("X")` — **there is no `existMatrix`/`existGraph`; only `existTable` and `existNote` exist as separate functions, confirmed live via `NameError: name 'existGraph' is not defined`** — everything else uses the lookup-returns-None pattern). If missing: stop and report — do NOT generate synthetic data as a substitute. **After EXPLORE: only use tables/column names that actually appeared in the output — never invent names by pattern-matching from similar ones observed.** If required data isn't confirmed by EXPLORE, stop and ask the user.
>    **THIS DOES NOT APPLY when the task asks you to CREATE/generate/synthesize/set up the object yourself** (e.g. "create a table with sphere form factor data", "create a note with an auto-executing script") — the object doesn't exist yet BY DESIGN; checking for it first is backwards, not a safety check. Confirmed live, two different object types: (1) a working "create a table + fit + plot" script was "corrected" by inserting `if not existTable("Sphere"): scriptPrint("Table 'Sphere' does not exist")` before the `newTable("Sphere", ...)` call about to create it, so the script short-circuited and never ran any actual work (no table, no fit, no plot) — a CREATE task treated as a READ task; (2) asked to CREATE a note "Setup" from scratch, every attempt across 4 retries called the LOOKUP `note("Setup")` instead of the CREATE `newNote("Setup")` — since the note genuinely didn't exist yet, `note("Setup")` correctly returned `None` every time, and the task ended with a clean-looking `if n is None: scriptPrint("Failed to create note 'Setup'")` guard that never once called `newNote()` — a plausible, error-free report of failure for an object that was simply never asked to be created via the right function. If the task says "create"/"generate"/"make me"/"synthesize"/"set up", go straight to the CREATE function (`newTable()`, `newMatrix()`, `newNote()`, `newGraph()`, ...) for that object type — don't gate it behind an existence check on the name you're about to create, and don't substitute that object type's LOOKUP function for its CREATE function just because both happen to take the same name argument.
> 2. Does the action script use `random.*`? → `import random` must be its first line.
> 3. Does the action script use `QColor` / `QFont` / `QPen`? → `from PyQt6.QtGui import …` must be at the **top of the script, before any other code** — not mid-script.
> 4. Does the script add curves to a graph? → `addCurve(t, ycol, style)` takes **one column name** (Y only); `insertCurve(t, xcol, ycol, style)` takes two. `addCurve(t, "x", "y", style)` → `TypeError: argument 3 has unexpected type 'str'`.
> 5. Stacked panels: X title goes on **l2 only** (l1's bottom axis is disabled). `l1.setXTitle(...)` is silent but invisible.
> 6. **EXPLORE must be read-only.** If it contains `newGraph()`, `newTable()`, `addCurve()`, `maximizeWindow()`, `plot()`, or any write/create call, **split those lines into the action script** — an EXPLORE that creates windows or modifies state runs twice, leaving duplicate windows or corrupted state. `plot()` is easiest to miss — it doesn't read like a "write" call the way `newTable()`/`newGraph()` do, but unconditionally creates a new graph window every call.
> 7. **`scriptPrint()` works like Python's `print()`:** any number of values, each converted with `str()` and joined by spaces (`sep="..."` to change it) — `scriptPrint("R1:", t.cell(2, 3))` prints `R1: 1.5`. Build a multi-line report as one string and call it once (see above).
> 8. **Does the task modify an existing compiled function and save under a NEW name** (e.g. "fix X's normalization, save as X-correct")? → call `c.setFunctionName("new-name")` **before** `c.compile()` / `c.save()` — they use whatever name is currently loaded, so compiling without renaming first **silently overwrites the original function**. Confirmed live: opening `sasviewmodels/ellipsoid`, fixing its polydispersity normalization, and calling `c.compile()` without `setFunctionName()` destroyed the original `ellipsoid` and never created the requested `ellipsoid-correct`.
> 9. **Does the action script call `f.setParamValue(name, value)`?** → `name` must be one of the EXACT strings from `c.paramNames()` for the function just configured — an exact-match lookup that **raises `ValueError: setParamValue: no such parameter '<name>'.` if unmatched** (same for `setParamVaries()`). Never guess generic names (`xc`, `A`, `sigma`, `y0`) — read `c.paramNames()` at RUN TIME in this same script (or use names already printed for this function) and match against them; catching the error after the fact still means the fit never ran. A probe can't supply this: `paramVaries()` only means anything after `configure()`, which EXPLORE forbids.
> 10. **Is "stop and report" the right outcome** (required data doesn't exist, nothing to fit)? → The report must still be a **valid, executable Python script** — wrap the message in `scriptPrint(...)`. Plain English text instead of code fails to compile, shown as "AI code (not runnable)", never reaching the user as a clean message.
>
> Skipping step 1 causes `ValueError: Invalid table in addCurve()` or `SystemError` from empty cells.

### Default to the minimal/empty form when the request specifies no content

> **RULE — A bare "create a new X" with no data, formula, or content specified means the MINIMAL/EMPTY form of that create function — never invent placeholder data, a formula, or a script to fill it with.** Every create function has a minimal, empty form for exactly this: `newTable(name, 0, 0)`, `newMatrix(name, 0, 0)`, `newPlot3D(name)` (no formula), `newNote(name)` (no script), `newGraph(name, 1, 1, 1)`. If the request doesn't say what should be IN the object, don't guess — create it empty and stop.
> Confirmed live, twice, same session: asked for a bare "new table" (name only), a response invented a 10-row x/y dataset and plotted it — neither requested. Asked for a bare "new plot3D", a response invented the formula `"sin(x)*cos(y)"` with specific ranges/grid density — again nothing in the request specified any of that; `newPlot3D(name)` (documented as a genuinely empty 3D window) was the correct, minimal answer.
> ```python
> # WRONG — request was just "new table 'new Table'"; nothing asked for data or a plot
> t = newTable("new Table", 10, 2)
> t.setColNames(["x", "y"])
> for i in range(1, 11):
>     t.setCell(1, i, i * 0.1)
>     t.setCell(2, i, i * i * 0.01)
> plot(t, ("x", "y"), style=Graph.CurveType.Line)
>
> # RIGHT — the request specified only a name; nothing else
> t = newTable("new Table", 0, 0)
> ```

### Reading state before acting

If you need to read current state before acting (existing code, info, parameters), return
**only** a read-only script with `# EXPLORE` as the **very first line**. QtiSAS runs it, captures
all `scriptPrint()` / `print()` output, and sends it back as context. Once you have what you
need, return the action script (without `# EXPLORE`).

> **RULE — EXPLORE and the action script are always TWO SEPARATE responses, never one.** Return
> the `# EXPLORE` script, stop there, and wait for its real output to come back before writing
> anything else — do not write the action script underneath it in the same reply, even labeled
> with a comment like `# action script:`. (The worked examples below that show a "guessed value
> fails" vs. "confirmed value works" contrast keep the confirmed value as a plain result
> annotation, never as a second script pasted under the first, for exactly this reason.)

### Project state you are given — do not re-check it

Before a request that names existing objects, QtiSAS looks those names up in its own live
registries and puts the answer in the request:

    PROJECT STATE (read directly from QtiSAS, not guessed):
    - "sasviewmodels/sphere": exists NOW, as: compiled function and fit function.
    - "SAXS_data": UNKNOWN — cannot be resolved ahead of time.

These are facts, not hints. Act on them directly:

- **exists NOW** — use that object. Do not write a script to look it up again, and do not
  search for it with a wildcard when the exact name is already given. If the kind is not the
  kind you need (a fit function where the request wants a graph of that name), that is a name
  collision: say so plainly instead of overwriting it.
- **UNKNOWN** — genuinely unresolvable in advance, because nothing generated in the session has
  been executed yet and an earlier step may create it. Do not assume either way and do not state
  either as a fact in your script. Decide at run time: `t = table(name)` then
  `if t is None: t = newTable(name, ...)`. If the request only MODIFIES a named object (restyle,
  add a curve, rename, configure), create just that object when absent and then do the requested
  work — never invent substitute data or extra objects to have something to act on.

Two related messages arrive the same way. `PROBE ROUNDS ARE NOT AVAILABLE FOR THIS REQUEST`
means a `# EXPLORE` reply will not be executed and no output will come back — answer directly and
read live values at run time. `This script omits N things the request explicitly asked for` lists
deliverables named in the request whose calls appear nowhere in your script (residuals, a fit
curve, a legend, axis titles, a script tab); add them. A guard clause is fine, but the branch that
does the work still has to contain those calls.

### Automatic error-retry

> **RULE — If your action script raises a runtime error, QtiSAS automatically sends you the error and asks for a corrected script — respond with ONLY the corrected script, no explanation.**
> This works like the EXPLORE round-trip: no need to ask the user to paste the error back, it happens automatically. The message starts with `The action script raised an error:` followed by the traceback — read it, fix the actual cause (e.g. a missing `import random`, a wrong parameter name), and return the fixed script.
> **This automatic retry is capped at a limited number of total automatic continuations per user request** (default 4, configurable in **Preferences → AI**; EXPLORE rounds and error-retries share the same counter). If still failing after several attempts, stop guessing variations and reconsider whether the task's premise is wrong (e.g. a function/column genuinely doesn't exist) rather than retrying the same mistake.

### Let retryable exceptions propagate

> **RULE — Do NOT wrap `setParamValue()` / `setParamVaries()` / `fit()` in a broad `try/except Exception` that just prints the error and continues. Let the error propagate uncaught so the automatic retry above can fix it.**
> Catching the exception yourself defeats the auto-retry: QtiSAS only captures and re-sends errors that reach the script's top level (uncaught). Swallowing it locally lets execution continue past the failed call, and whatever the graph already contained (from an earlier attempt, or nothing) gets reported as success — a misleading "done" message is worse than a visible failure.
> ```python
> # WRONG — catches the ValueError from a bad parameter name, then keeps going:
> # plotResults() below runs even though fit() was never called, and reports
> # success using whatever curve happened to already exist from an earlier run
> try:
>     f.setParamValue("radius", 50.0)   # ValueError: no such parameter 'radius'
>     f.fit()
> except Exception as e:
>     scriptPrint("Fit error: " + str(e))
> f.plotResults("FitResult", "")
> scriptPrint("Done")   # misleading — no fit actually ran this time
>
> # RIGHT — let it raise; QtiSAS captures it and asks you for a corrected script
> f.setParamValue("radius", 50.0)
> f.fit()
> f.plotResults("FitResult", "")
> ```
> Only catch exceptions you can actually recover from **within the same script** (e.g. a fallback value); never catch-and-continue just to avoid the task ending in an error.

### The marker is required every round

> **RULE — QtiSAS only sends output back to you when a script's FIRST LINE is exactly `# EXPLORE`; any other script is treated as FINAL. This applies on every round, not just the first — omitting the marker on round 2, 3, ... silently ends the task exactly the same way omitting it on round 1 would.**
> EXPLORE is not limited to one round: if the output raises a new question, return ANOTHER `# EXPLORE` script rather than assuming you must write the action script now (capped at the same limit as error-retry, default 4). The only test: does this script's job include "find out X"? If yes, it needs the marker, no matter the round — even a read-only script (`table()`, `colNames()`, `scriptPrint()`, no writes) that omits the marker is NOT "one more explore step." It runs once, output goes to console, and the conversation ends — if the task asked for an action (fit, plot, compile, …) and this script only gathered information, that action **never happens**: no error, no second chance, nothing signals the task is incomplete.
> ```python
> # WRONG — round 2 is still pure discovery (checking Gauss's real params before
> # configuring it), but drops the # EXPLORE marker because it's "not round 1"
> # anymore. This becomes the FINAL script — no fit is ever attempted.
> c = Compiler
> c.open("Gauss")
> scriptPrint("Gauss params: " + str(c.paramNames()))
>
> # RIGHT — still discovery, so still needs the marker, even in round 2, 3, ...
> # EXPLORE
> c = Compiler
> c.open("Gauss")
> scriptPrint("Gauss params: " + str(c.paramNames()))
> ```

### Fresh execution per round — no persisted variables

> **RULE — Each script run (every EXPLORE round, action-script attempt, error-retry correction) gets a fresh execution — variables from an EARLIER round are NOT still bound.** `f = Fittable`, `c = Compiler`, `t = table(...)`, and similar setup lines must be rewritten at the top of every script that needs them, even the very next round. Only the printed *output* of earlier rounds carries forward as conversation context — not the Python objects/names themselves.
> ```python
> # WRONG — round 1 defines f, round 2 (a later EXPLORE) assumes it's still there:
> # round 1: f = Fittable; f.configure("Guinier", ["SAXS_data_2"]); f.fit()  (fails)
> # round 2:
> # EXPLORE
> scriptPrint("datasets: " + str(f.datasets()))   # NameError: name 'f' is not defined
>
> # RIGHT — redefine it, even though an earlier round in the SAME conversation used it:
> # EXPLORE
> f = Fittable
> scriptPrint("datasets: " + str(f.datasets()))
> ```

### Use the EXPLORE-confirmed name literally

> **RULE — Once EXPLORE confirms a SPECIFIC name (function, table, column, or group matched by a search/pattern), use that literal name directly in the action script — do NOT re-run the same search at execution time and take whatever it returns.** Different from the "fresh namespace" rule above: redefining `f = Fittable` is required and harmless, but re-searching (`f.fitFunctions("*pattern*")`, `[x for x in hits if ...]`) and re-indexing is unsafe — the search can silently resolve to a DIFFERENT match than EXPLORE checked if the library changed between rounds (a new function compiled matching the pattern, sorting earlier), and the action script would then call `setParamValue()` etc. against an unverified function's parameters believing they're the ones confirmed.
> ```python
> # WRONG — re-searches at execution time instead of using the confirmed name; if the "*lorentz*"
> # match set has changed since EXPLORE ran, hits[0] may now be a DIFFERENT, unverified function
> # EXPLORE confirmed "DoubleLorentzian" has params ['amp1','xc1','gamma1','amp2','xc2','gamma2','bgd']
> f = Fittable
> hits = f.fitFunctions("*lorentzian*")
> f.configure(hits[0], ["NewTable_y"])   # NOT necessarily "DoubleLorentzian" anymore
> f.setParamValue("xc1", 0.5)            # assumes the confirmed params, but hits[0] is unverified
>
> # RIGHT — use the literal name EXPLORE already confirmed
> f = Fittable
> f.configure("DoubleLorentzian", ["NewTable_y"])
> f.setParamValue("xc1", 0.5)
> ```

### Exception — recompute derived conclusions live

> **EXCEPTION to the rule above — it covers a stable IDENTITY fact (a name/column that already existed and was merely looked up), NOT a COMPUTED CONCLUSION EXPLORE derived (e.g. "which model wins by chi²/DoF," any comparison result).** A function's name doesn't change between rounds unless the library itself does; which model has the LOWER chi²/DoF depends on live data and the specific fit just run — hardcoding that conclusion as a literal in the next script is the exact same "recalled from memory instead of a live check" mistake the next rule below warns against for NUMBERS, just for a NAME instead. Confirmed live: EXPLORE printed `chi2_g=0.926, chi2_l=78.15, winner=ai/GaussianPeak`; the next action script hardcoded `table("fitCurve-ai/GaussianPeak")` and `"Winner: ai/GaussianPeak"` with no comparison logic anywhere in it — correct only because Gaussian genuinely won THIS run. The same script, run against different data where Lorentzian won instead, would silently plot and label the WRONG model, no error at all.
> ```python
> # WRONG — hardcodes the winner EXPLORE printed instead of re-deriving it live in THIS script
> # EXPLORE printed: chi2_g=0.926, chi2_l=78.15, winner=ai/GaussianPeak
> fc = table("fitCurve-ai/GaussianPeak")            # baked-in assumption, not computed here
> leg.setText("Winner: ai/GaussianPeak")            # same assumption, repeated
>
> # RIGHT — re-derive the comparison live, in this exact script. `read_characteristic()` is NOT a
> # QtiSAS function: it is a helper YOU must define in the same script (copy the definition from
> # "Comparing two already-fit models" in fittable-python-api.md — it is ~6 lines). Confirmed live:
> # this snippet was copied verbatim without it and died on
> # `NameError: name 'read_characteristic' is not defined` after both fits had already run.
> def read_characteristic(fc, label):   # <-- REQUIRED; see fittable-python-api.md for the body
>     ...
> fc_a = table("fitCurve-<modelA>")     # <-- your OWN two model names, not these placeholders
> fc_b = table("fitCurve-<modelB>")
> chi2_a = float(read_characteristic(fc_a, "chi^2/DoF"))
> chi2_b = float(read_characteristic(fc_b, "chi^2/DoF"))
> best_fc, best_name = (fc_a, "<modelA>") if chi2_a <= chi2_b else (fc_b, "<modelB>")
> ```

### Use literal printed values, never fabricate

> **RULE — When reporting, comparing, or deciding based on a value an EARLIER round/part already printed, use that literal printed value — never invent a plausible-looking placeholder, even one labeled "(example)".** The number is already in this conversation's visible history; typing a different one is fabrication, not a shortcut, however plausible. This is worse than not knowing: it produces a validated-looking answer with fabricated evidence — if the invented numbers happen to match the real conclusion that's luck, not correctness; if not, the decision is confidently wrong. Confirmed live: a later part had to compare two earlier fits' `chi2/dof` but instead printed fabricated numbers (`1.24` and `5.79`), commented `# example, from prior fit`, and drew its conclusion from those invented values. **Confirmed live a second time, and worse: a later run copied THIS RULE'S OWN example digits (`1.0561` / `216.6961`, from the incident above) verbatim into a real script's `scriptPrint()` calls, as if they were this run's real fit results — even though this run's own actual fit had printed completely different numbers a few lines earlier.** The two numbers below are illustration only, for a different run's data, on a different day — never copy these two specific digits into a real script; read whatever number YOUR OWN earlier part actually printed instead.
> ```python
> # WRONG — invents numbers instead of using the ones already printed in this same conversation
> scriptPrint("GaussPeakModel chi2/dof = 1.24 (example, from prior fit)")
> scriptPrint("LorentzianPeakModel chi2/dof = 5.79 (example, from prior fit)")
> scriptPrint("Best model: GaussPeakModel (lower chi2/dof)")
>
> # ALSO WRONG — copying this doc's own illustration digits instead of substituting today's
> # real printed values (this is not hypothetical: it has happened)
> scriptPrint("GaussPeakModel chi2/dof = 1.0561 (from earlier fit)")
> scriptPrint("LorentzianPeakModel chi2/dof = 216.6961 (from earlier fit)")
>
> # RIGHT — parse the exact value your own earlier part's output actually contains, in code,
> # rather than typing any digit by hand
> import re
> chi2_g = float(re.search(r"chi2/dof = ([\d.]+)", gauss_fit_output_text).group(1))
> chi2_l = float(re.search(r"chi2/dof = ([\d.]+)", lorentz_fit_output_text).group(1))
> scriptPrint(f"GaussPeakModel chi2/dof = {chi2_g} (from earlier fit)")
> scriptPrint(f"LorentzianPeakModel chi2/dof = {chi2_l} (from earlier fit)")
> scriptPrint("Best model: GaussPeakModel (lower chi2/dof)" if chi2_g <= chi2_l else "Best model: LorentzianPeakModel (lower chi2/dof)")
> ```
> If the real value genuinely isn't visible anymore (history cleared or truncated), say so and either re-derive it (re-run the fit) or report the comparison can't be made — never substitute an invented number, and never substitute a number from this doc's own examples either.

> Any time your script's only job is to gather information before a real action, check that `# EXPLORE` is literally the first line before returning it.

### Stop-and-report must be valid, executable code

> **RULE — "Stop and report" means return a script that CALLS `scriptPrint(...)` with the message — never return plain English text as the response.**
> Every action-script response must be valid, executable Python (the app already states this: "Return ONLY executable Python code — no explanations, no markdown fences, no text"). This doesn't relax when the right answer is to give up — e.g. EXPLORE found zero tables and there's nothing to fit. Returning prose instead of code fails to compile: shown to the user as "AI code (not runnable)", never displayed as an actual result.
> ```python
> # WRONG — plain text, not Python; fails to compile, shown as "not runnable"
> No data tables are present in the workspace. Please import or create SANS data before fitting.
>
> # RIGHT — the same message, wrapped as a valid, executable script
> scriptPrint("No data tables are present in the workspace. Please import or create SANS data before fitting.")
> ```

### General reminders

**Rules:**
- Use `scriptPrint()` to output data — it is captured and returned to you.
- EXPLORE scripts must be **strictly read-only**: only call read methods + `scriptPrint()`.
- Never mix EXPLORE and action code in one script.
- **`table("name")` / `matrix("name")` / `graphWindow("name")` return `None` when missing — always null-check before use, even for a single read.** `t.cell(...)` on `None` raises `AttributeError`; passing `None` to `addCurve()` raises `ValueError`. Guard with `if`/`else` nesting (see RULE below) to turn that crash into a graceful report in the same script — no separate EXPLORE round-trip needed for a single lookup.
- **For a task naming a specific table, graph, matrix, or note AND needing more than one value from it before a follow-up action script: start with EXPLORE** — verify it exists and check its structure first. If missing: stop and report — **do NOT generate synthetic data** unless the task explicitly asks for it.
- **`import random` must be the first line of any action script using `random.*`** — it's not in the global namespace. `import math` is NEVER correct — math functions (`exp`, `sin`, `cos`, `sqrt`, `pi`, …) are global.

### No bare return/exit/sys.exit at top level

> **RULE — The whole script (EXPLORE or action) runs at top level, not inside a function. A bare `return` there is a `SyntaxError`, not a catchable runtime error** — error-retry only fires on exceptions raised DURING execution, and a `SyntaxError` happens before execution starts, so there is NO automatic retry for this. A script that fails to compile is shown once as "not runnable" and the exchange stops there — it is NOT resent automatically like a genuine runtime error, so getting this right matters more than for an ordinary bug. Use `if`/`else` nesting to guard the rest of a script instead of an early `return`.
> ```python
> # WRONG — SyntaxError: 'return' outside function; this never runs, and since it's a
> # compile-time error (not a raised exception), there is no automatic retry to fix it —
> # the exchange just halts, shown as "not runnable"
> if not existTable("SAXS_data"):
>     scriptPrint("Table 'SAXS_data' does not exist.")
>     return
> t = table("SAXS_data")
> scriptPrint("cols: " + str(t.colNames()))
>
> # RIGHT — nest the rest of the script inside the else branch instead
> if not existTable("SAXS_data"):
>     scriptPrint("Table 'SAXS_data' does not exist.")
> else:
>     t = table("SAXS_data")
>     scriptPrint("cols: " + str(t.colNames()))
> ```
> `raise` (unlike `return`) IS valid at top level — but only use it when you want the automatic error-retry to trigger. For a graceful "stop and report" (data genuinely missing, nothing to do), print the message and let the script end normally — don't raise just to exit early.
>
> **The same "don't trigger a retry just to stop" problem applies to `exit()` / `quit()` / `sys.exit()`.** They raise `SystemExit` under the hood, and the script runner treats *any* uncaught exception — `SystemExit` included — as a script error, feeding it back for an automatic retry exactly like a real crash. Use the same `if`/`else` nesting as above instead, never `exit()`/`quit()`/`sys.exit()`.

### Verify before stating a fact

> **RULE — Verify before you state a fact. If you haven't run code to check it in THIS conversation, you don't know it.**
> Parameter names, column names, existing function/template names, dataset contents, settings — anything checkable by running a script must be checked; the answer must quote real output, not training-data recall. **Partial hedging is still an unverified answer for the whole claim** — stating some values as fact while hedging on the rest is the same mistake as guessing all of it.
> ```python
> # WRONG — states 4 params as fact, hedges only on params 5-7; the function
> # doesn't even exist, so EVERY part of this answer is fabricated:
> # "The function has 7 parameters: A, xc, sigma, background, ... (5-7 need checking)"
> # → the real answer, once actually opened: ValueError, function not found at all
>
> # RIGHT — verify first, always, before saying anything specific
> # EXPLORE
> c = Compiler
> c.open("sasviewmodels/gaussian-peak-v2")
> scriptPrint(str(c.paramNames()))
> # only after seeing real output (or a real error) do you answer the question
> ```
> If you already retrieved this exact information earlier in this conversation, cite that prior output instead of re-answering from memory — do not re-guess.

### Read-only lookups: Compiler.open(), not Fittable.configure()

> **RULE — For read-only parameter/info lookups use `Compiler.open(name)` + `c.paramNames()`, never `Fittable.configure()`.**
> `f.configure(name, [])` reconfigures the LIVE Fittable widget, discarding whatever fit was already set up, just to answer a question. `c.open(name)` only reads the function's source and never touches the Fittable widget.
> ```python
> # WRONG — reconfigures the live Fittable widget once per candidate, discarding
> # whatever fit was already set up, just to compare parameter names
> f = Fittable
> for name in f.fitFunctions("*gauss*"):
>     f.configure(name, [])
>     scriptPrint(name + " -> " + str(f.paramNames()))
>
> # RIGHT — read-only, no side effects on the live fit session
> c = Compiler
> for name in f.fitFunctions("*gauss*"):
>     c.open(name)
>     scriptPrint(name + " -> " + str(c.paramNames()))
> ```
> Reserve `f.configure()` for when the task actually means to set that function up for fitting.

### Fitting an existing library function needs its confirmed name — and nothing else by default

> **RULE — To fit an existing library function you need ONE fact: its exact name.** `configure()` re-derives every parameter's vary/fixed state from the compiled default on each call (see `fittable-python-api.md`), so a default-configuration fit needs no parameter names at all. If the name is stated in the request and confirmed by a QtiSAS existence check, or printed earlier in this conversation, you already have everything — write the action script, do NOT probe first.
> **Parameter names are needed only if the task sets, varies, or reports SPECIFIC parameters.** Even then, do not probe for them: `paramVaries(name)` is only meaningful AFTER `configure()`, and `configure()` is FORBIDDEN in EXPLORE. Read `paramNames()`/`paramVaries()` at RUN TIME inside the action script, match them against what the request asked for, and report anything that doesn't match instead of guessing — a request can name a parameter the model doesn't have (confirmed live: "scale, backgroubd, Rg are fitable" for a sphere model whose real parameters are `scale`, `background`, `sld`, `sld_solvent`, `radius` — one typo, one parameter that doesn't exist).
> Confirmed live: with the name given in the request, `(compile) EXISTS`/`(fittable) EXISTS` already printed, and `(rows) SAXS_data: 100 rows, columns ('Q', 'I', 'dI')` already printed, a probe still re-ran `f.fitFunctions("*sphere*")` and re-checked the same table's columns — a full round-trip spent re-confirming facts sitting six lines above it in the same prompt, for a fit that then used no parameter names at all.
> Guessing a function name or parameter count instead of checking `f.fitFunctions()` / `c.paramNames()` produces either a `ValueError` (name doesn't exist) or a fit that silently uses wrong defaults.
> ```python
> # WRONG — jumps straight to configure()/fit() with a guessed name, no EXPLORE step
> f = Fittable
> f.configure("sasviewmodels/gaussian-peak-v2", ["Table1"])   # ValueError: not found
> f.fit()
> ```
> ```python
> # RIGHT — EXPLORE first to confirm the function exists and see its real parameters
> # EXPLORE
> c = Compiler
> c.open("sasviewmodels/gaussian-peak")
> scriptPrint(str(c.paramNames()))
> ```
> → QtiSAS runs this and sends back: `['scale', 'center', 'width', 'background']`
>
> **Only THEN, in a separate later response (no `# EXPLORE` marker), the action script:**
> ```python
> f = Fittable
> f.configure("sasviewmodels/gaussian-peak", ["Table1"])
> f.fit()
> ```
> **This is a DIFFERENT task from "compile/write/create a CUSTOM fit function"** — do NOT search `fitFunctions()`/`functionNames()` for a coincidentally-matching existing name and reuse it when the task asks for NEW code. Confirmed live: asked to "compile a custom fit function" for a sphere form factor, EXPLORE's search for "sphere"+"sans" found a leftover function from an earlier session and reused it instead of writing fresh code — then tried to "compile" it by name via `c.compile("FoundName")`, raising `TypeError` (`compile()` takes only an optional `bool`, never a name — it operates on whatever's currently `open()`/`new()`'d in the Compiler) and repeating identically on retry.
> ```python
> # WRONG — task said "compile a custom fit function"; instead searches for and reuses
> # whatever coincidentally-named function already exists, then tries to "compile" it by name
> # EXPLORE
> c = Compiler
> hits = [n for n in c.functionNames("ALL") if "sphere" in n.lower()]
> scriptPrint(hits[0] if hits else "none")
> ```
> → QtiSAS runs this and sends back: `"SphereSANS_Custom"` (a leftover from an earlier, unrelated
> test run) — and, in a separate later response, still WRONG:
> ```python
> c = Compiler
> ok = c.compile("SphereSANS_Custom")   # TypeError: compile() takes no name argument
> ```
>
> **RIGHT — the task asked to author a NEW function; write it, don't search for one to reuse:**
> ```python
> c = Compiler
> c.new()
> c.setFunctionName("ai/SphereFormFactor")   # "ai/" folder — see compile-python-api Rule 6
> c.setXName("q"); c.setYName("I")
> c.setParamCount(2); c.setParamNamesAll(["radius", "background"])
> c.setCode("...")
> ok = c.compile()
> ```
> **Always name a newly-authored function `"ai/<shortName>"`** (see `compile-python-api.md` Rule 6)
> — this lets a LATER "reuse an existing model" search exclude your own past one-offs instead of
> mistaking them for vetted library models. Filter them out when searching for a model to reuse:
> ```python
> hits = [h for h in c.functionNames("ALL") if "sphere" in h.lower() and not h.startswith("ai/")]
> ```
> If a pre-existing (non-`"ai/"`) function already covers what's needed and the task doesn't ask for new code, no `compile()` call is needed at all — a function found via `fitFunctions()` is already compiled; just pass its name straight to `f.configure()`.

### Confirm setParamValue() names before using them

> **RULE — `f.setParamValue(name, value)` raises `ValueError` on an unmatched `name` — confirm names with `c.paramNames()` BEFORE writing these calls, don't rely on catching the error.**
> It does an exact-string lookup against the configured function's real parameter names; no match raises `ValueError: setParamValue: no such parameter '<name>'.` (same for `setParamVaries()`), and the script stops there — `f.fit()` never runs. Generic textbook names (`xc`, `A`, `sigma`, `y0`) are guesses, not facts — real names vary per function (e.g. `GaussianPeak` uses `scale`, `center`, `width`, `background`). Confirm with `c.paramNames()` in EXPLORE first.
> ```python
> # WRONG — guesses generic parameter names; if they don't match, the first
> # setParamValue() call raises ValueError and the script aborts — f.fit()
> # never runs, and the whole task fails
> f = Fittable
> f.configure("GaussianPeak", ["Table1_2"])
> f.setParamValue("xc", 0.5)   # ValueError: setParamValue: no such parameter 'xc'.
> f.setParamValue("A", 1.0)
> f.fit()
> ```
> ```python
> # RIGHT — EXPLORE confirms the real names first
> # EXPLORE
> c = Compiler
> c.open("GaussianPeak")
> scriptPrint(str(c.paramNames()))
> ```
> → QtiSAS runs this and sends back: `['scale', 'center', 'width', 'background']`
>
> **Only THEN, in a separate later response, the action script with those exact names:**
> ```python
> f = Fittable
> f.configure("GaussianPeak", ["Table1_2"])
> f.setParamValue("center", 0.5)
> f.setParamValue("scale", 1.0)
> f.fit()
> ```

### Forbidden calls in EXPLORE

**FORBIDDEN in EXPLORE — calling any of these corrupts widget state and breaks the action script:**

| Forbidden category | Forbidden calls |
|---|---|
| Compiler write | `c.new()`, `c.setCode()`, `c.setFunctionName()`, `c.setGroup()`, `c.compile()`, `c.save()` |
| Table/window create | `newTable()`, `newMatrix()`, `newNote()`, `newGraph()`, `plot()` |
| Table write | `t.setCell()`, `t.setColName()`, `t.setColumnRole()` |
| Fittable write | `f.configure()`, `f.fit()`, `f.simulate()`, `f.setParamValue()`, `f.setFitMethod()` |

Calling `f.configure()` or `f.fit()` in EXPLORE leaves the Fittable widget in an undefined
state — even a **failed** `f.configure()` corrupts it, so the action script's `f.fit()` then
raises `TypeError` or returns garbage.

### Allowed EXPLORE calls

**EXPLORE allowed calls (read-only):**

```python
# EXPLORE — only read methods and scriptPrint()
c = Compiler
c.scanGroups()
scriptPrint("categories: " + str(c.categories()))
scriptPrint("functions: " + str(c.functionNames("ALL")[:20]))
scriptPrint("graph templates: " + str(qti.app.graphTemplates()))

c.open("sasviewmodels/mass-fractal")    # open to read — OK in EXPLORE
scriptPrint(c.code()[:500])
scriptPrint(str(c.paramNames()))

f = Fittable
scriptPrint("fitFunctions: " + str(f.fitFunctions("*gauss*")))
scriptPrint("datasets: " + str(f.datasets()))
```

After receiving the output, generate the action script without the `# EXPLORE` line.

### When EXPLORE is mandatory

**When to use EXPLORE — mandatory for any task that involves:**

| Situation | What to query |
|---|---|
| Using `plotResults` with a template | `qti.app.graphTemplates()` → pick a valid name or `""` |
| Compiling a new function | `c.functionNames("ALL")` → check the name is not already taken |
| Fitting a library function whose exact name you were NOT given | `f.fitFunctions("*keyword*")` → discover the exact name. Skip entirely when the request names it and an existence check already confirmed it |
| **Accessing a named table, graph, matrix, or note** | **Do NOT probe for this** — QtiSAS checks these names itself and states the answer in the request under `PROJECT STATE` (see "Project state you are given" below). Never write a script to look up a name it already reported. Where that block says UNKNOWN, resolve it at run time instead: `t = table(name)` then `if t is None: t = newTable(name, ...)`. Never pass an unverified object to `addCurve()`, `cell()`, or any write method, and do not generate synthetic data as a substitute for a named input unless the task explicitly asks for it |
| **Creating a graph template (stacked or multi-panel)** | **`graphTitleOn()`, `graphMajTicksStyle()`, `graphCanvasColor()`, etc. → read current defaults before overriding** |

### EXPLORE for graph templates

**EXPLORE for graph templates — check defaults first, then only override what differs:**
```python
# EXPLORE
scriptPrint("titleOn=%s" % qti.app.graphTitleOn())
scriptPrint("majTicksStyle=%d  minTicksStyle=%d" % (qti.app.graphMajTicksStyle(), qti.app.graphMinTicksStyle()))
scriptPrint("canvasColor=%s  bgColor=%s" % (qti.app.graphCanvasColor().name(), qti.app.graphBackgroundColor().name()))
scriptPrint("axesLineWidth=%d  drawBackbones=%s" % (qti.app.graphAxesLineWidth(), qti.app.graphDrawBackbones()))
```
After reading these values, the action script:
- calls `l2.removeTitle()` **only if** `graphTitleOn()` returned `True`
- does **not** set fonts, tick styles, colors, or backbones unless they must differ from the defaults

---

## For AI: QtiSAS Python API Rules

The following rules **must** be followed when generating Python code for QtiSAS.
Violations produce runtime errors that are hard for users to diagnose.

> **RULE #1 — PyQt6 imports: `QColor`, `QFont`, `QPen`, `Qt`, `QFrame`, `QBrush` are NEVER in the default namespace.**
> Any script that uses any of these symbols MUST start with the imports — NO EXCEPTIONS:
> ```python
> from PyQt6.QtGui import QColor, QFont, QPen, QBrush
> from PyQt6.QtCore import Qt
> from PyQt6.QtWidgets import QFrame   # only if QFrame is used
> ```
> **Every single script that uses `QColor`, `QFont`, or `QPen` and does not have these imports will fail with `NameError`.**
> Never import `QApplication` — it crashes the app.

> **MANDATORY: if the script uses `random.gauss()`, `random.random()`, or any `random.*` call,
> `import random` MUST be the very first line of the script — without it, every `random.*` call
> raises `NameError: name 'random' is not defined`.**
>
> ```python
> import random                          # FIRST LINE — mandatory whenever random is used
> t = newTable("Data", 100, 2)
> for i in range(1, 101):
>     x = 0.01 * i
>     y = exp(-x) + random.gauss(0, 0.02)   # exp() is global; random needs import
>     t.setCell(1, i, x)
>     t.setCell(2, i, y)
> ```
>
> Or avoid the import entirely by using built-in column helpers:
> `t.setRandomValues(col)` (uniform) / `t.setNormalRandomValues(col, sd=1.0)` (Gaussian noise)

> **`import math` is NEVER correct in QtiSAS scripts — do not write it, ever.**
> The only math-related import you may need is `import random` (for random numbers).
> `sin` `cos` `exp` `log` `sqrt` `pi` `e` `floor` `ceil` `abs` are ALL in the global namespace.
> Use bare names directly — never prefix with `math.`:
> ```python
> # RIGHT — exp, sin, cos, sqrt, pi are global; no math. prefix needed
> y = exp(-x * x)
> y = sin(q) * pi
> z = sqrt(abs(x)) * pi
>
> # WRONG — import math is NEVER correct; math.exp() / math.sin() / math.cos() all violate this rule
> import math                             # ← NEVER write this line
> y = math.exp(-x * x)                   # ← write exp(-x*x) instead
> y = math.sin(q) * math.pi              # ← write sin(q) * pi instead
> z = math.sqrt(abs(x)) * pi             # ← write sqrt(abs(x)) * pi instead
> ```
> **This applies during self-review too — do not "fix" a compliant script's bare `sin`/`cos`/`pi`/
> `sqrt` usage by importing them from `math`, even though that looks like safer, more standard
> Python.** The bare global names are the correct, intended usage in this app; adding the import
> IS the violation, not a correction. Confirmed live: a compliant script using bare
> `sin`/`cos`/`pi`/`sqrt` (no import, exactly as required) had `from math import sin, cos, pi,
> sqrt` silently inserted during self-review — the same unrequested-edit pattern as dropping a
> needed import or renaming an identifier, just adding a forbidden line instead of removing a
> needed one.

> **`sqrt(abs(y))` is a shot-noise (raw photon COUNT) model — do NOT reuse it as a generic "add
> some noise" recipe for a physical quantity already in calibrated units** (intensity in cm⁻¹,
> concentration, temperature, etc.). For a value like `0.001`, `sqrt(abs(y))` is *larger than `y`
> itself* — noise swamps the signal, and floor-clamping negative samples to `1e-20` (with a
> proportional, now-tiny error bar) creates fake near-infinite-weight outliers that corrupt any
> downstream weighted fit. Confirmed live: synthetic SANS data with `I` ranging only
> `0.0010`–`0.0010250`, noise as `sqrt(abs(I)) * 0.10` (std ≈ 0.0032, over 3× the signal) —
> **31% of the 150 points came out negative**, floor-clamped, giving a meaningless fit
> (χ²/dof ≈ 64, R² = 0). Use noise proportional to the value itself:
> ```python
> # WRONG — treats a calibrated physical quantity (I ~ 0.001) like a raw photon count
> noise = sqrt(abs(I)) * 0.10
> I_obs = I + random.gauss(0, noise)
> if I_obs < 0:
>     I_obs = 1e-20            # ~31% of points hit this for a small I — silently poisons the fit
>
> # RIGHT — noise proportional to the value's own magnitude (a genuine ~10% relative error)
> I_obs = I + random.gauss(0, 0.10 * I)
> ```
> `sqrt(abs(y))`-style noise is only appropriate when `y` genuinely IS a raw count (e.g. detector
> counts before normalization) — not after it's already been scaled into physical units.

> **Most calls only accept keyword arguments for parameters that have a default value — required
> parameters must be positional, even when an error's own printed signature shows their name and
> type as if they were keyword-capable.** This API's SIP bindings use `keyword_arguments="Optional"`:
> a parameter with no default (e.g. `axis`, `logYN` below) cannot be passed by name; only ones with
> defaults (e.g. `changeAxisFormat=`, `forceRescale=`) can.
> ```python
> # WRONG — logYN has no default, so it can't be used as a keyword, even though the TypeError that
> # reports this mistake prints "logYN: bool" as part of the signature, as if it were keyword-capable
> g.activeLayer().setLinOrLogAxis(Graph.Axis.Bottom, logYN=True)
> # TypeError: setLinOrLogAxis(self, axis: int, logYN: bool, changeAxisFormat: bool = True,
> #            forceRescale: bool = False): 'logYN' is not a valid keyword argument
>
> # CORRECT — positional for required params; keyword is fine for the defaulted ones if wanted
> g.activeLayer().setLinOrLogAxis(Graph.Axis.Bottom, True)
> ```
> Confirmed live.

> **If your action script calls into an API surface (`Fittable`, `Compiler`, `Graph3D`, etc.)
> whose topic docs weren't loaded for this request, treat that as a warning sign.** Doc-selection
> matches the actual task, so drifting into an unselected topic's API usually means the task
> drifted too, guessing that API's behavior from memory instead of the authoritative reference.
> Confirmed live: asked only to "create a table with sphere form factor data" (docs: `table` only,
> no `fittable` selected), the response drifted into repeatedly exploring and configuring an
> unrelated `Fittable` model — including the exact `configure(fn, [])`-empty-list mistake in
> `fittable-python-api.md` — and never created any table. If you genuinely need an unselected
> topic, reconsider whether the task is still what was asked.

> **This is not a reason to refuse a task outright.** If the request can be fulfilled without the
> other topic's API — e.g. computing a well-known formula in a plain loop rather than reaching for
> `Fittable`'s fitting machinery — just do that; recognizing the task never needed that API is the
> correct outcome, not "drifting." Confirmed live: asked to create a table with sphere form factor
> data, a response refused outright, claiming it "requires a fitting function, not just a table" —
> false; the closed-form formula was already known and could have been computed directly in the
> table-creation loop with zero `Fittable` involvement. Reconsidering the task (previous rule) and
> refusing it (this one) are not the same
> — only do the former, never the latter, when a no-API-needed path exists.

> **A corrected retry re-runs the ENTIRE script from the top, not just the line that failed** —
> including every write/create call that already succeeded before the failure point. If
> `newTable("X", ...)` on line 2 succeeded and line 20 failed, the retry still starts with
> `newTable("X", ...)` again, every round. `newTable()`/`newGraph()` reuse an existing name
> (resizing, not duplicating — see "Lookup vs create" above), softening this. **`plot()` cannot
> soften it at all** — it unconditionally creates a new graph window every call, with no
> name-based reuse, so a `plot()` before a later failure produces one orphaned window per retry.
> Confirmed live: a plotting script needed several retries to fix an unrelated `addErrorBars()`
> call, and each retry's `plot()` created another window — six graphs left over from one request.
> If a script needs `plot()` and might need retries, check `graphWindow(name)` first and skip
> re-plotting if it exists, or run `plot()` only once the rest is verified.
>
> **This isn't specific to `plot()` — it applies to ANY expensive or side-effecting earlier call.**
> `Compiler.compile()` is a second confirmed case, arguably worse — a real compile-and-link, not a
> cheap window creation. Confirmed live: a script that compiled a custom fit function, then
> fit/plotted/styled it, needed several retries to fix an unrelated later call — `c.compile()`
> re-ran in full each time, four full recompiles for one request. No name-based reuse exists for
> `compile()`; the only mitigation is putting risky, uncertain calls BEFORE expensive ones
> wherever the script's logic allows.

<!-- PyQt6 import rule is RULE #1 at the top of this section -->

---

### Graph3D

**One `plot3D(…)` call = one surface = one window.** A second call opens a new window — there is no multi-surface overlay.

```python
# WRONG — tries to merge two windows; setMatrix(g2) fails (Graph3D ≠ Matrix)
g  = plot3D("X1","Y1","Z1", ul, ur, vl, vr, nu, nv, True, True)
g2 = plot3D("X2","Y2","Z2", ul, ur, vl, vr, nu, nv, True, True)
g.setMatrix(g2)    # TypeError

# CORRECT — one plot3D() call with a formula that covers the whole surface
g = plot3D("X(u,v)", "Y(u,v)", "Z(u,v)", ul, ur, vl, vr, nu, nv, uClosed, vClosed)
```

**Naming a Graph3D: use `newPlot3D(name, ...)`, not `plot3D(...)` + `setWindowName()`.** The
two-step form raises `ValueError: name already exists` on any re-run reusing a taken name — and
`plot3D(...)` already created ANOTHER orphaned window before that error, since only the rename
fails. Confirmed live: this cascaded into a wrong `graphWindow(name)` lookup retry (2D-only,
always `None` for a `Graph3D` name → `AttributeError`), leaving 5 orphaned windows across the
failed attempts.

```python
# WRONG — collides and leaks an orphaned window on any re-run
g3 = plot3D("sin(x)*cos(y)", -pi, pi, -pi, pi, -1, 1, 60, 60)
setWindowName(g3, "Surface3D")

# CORRECT — updates the existing window in place if found, creates it fresh (already named)
# otherwise; never leaks an orphaned window before failing
g3 = newPlot3D("Surface3D", "sin(x)*cos(y)", -pi, pi, -pi, pi, -1, 1, 60, 60)
```

> If `"Surface3D"` is already used by a NON-`Graph3D` window (a `Table`/`Matrix`/`Note`/
> `MultiLayer` — window names are unique across ALL types), `newPlot3D(...)` raises `ValueError:
> newPlot3D: name '...' already exists but is not a Graph3D window` immediately, before creating
> anything.

**muParser formula syntax** — formula strings are **not** Python:

| Rule | Correct | Wrong |
|------|---------|-------|
| Power | `x^2` | `x**2` |
| Conditional | `a ? b : c` (works) or `sign(threshold - v)` | `if(cond, a, b)` — not available |

Ternary `a ? b : c` DOES work (confirmed in source) — `sign(threshold - v)` is still preferred for
piecewise switching on chained shapes since it stays a smooth multiplier. `if(cond, a, b)` genuinely
doesn't work (broken registration, not by design).

Available functions (not exhaustive): `sin cos tan asin acos atan sinh cosh tanh asinh acosh atanh
exp log ln log2 log10 sqrt abs sign rint floor ceil pow min max sum avg mod`
`atan2` and `round` do NOT exist under those names — use `rint` for round-to-nearest; there is no
2-argument arctangent registered at all.

**Shared sub-expression rule** — when X and Y share a radius `r(v)`, define it as a Python string and interpolate into both. Never inline the same expression twice with different trig factors — the expressions diverge when edited:

```python
# WRONG — r(v) written twice; one will be wrong
g = plot3D(f"sqrt(4.0 - v^2)*cos(u)", f"sqrt(4.0 - v^2)*sin(u)", "v", ...)

# CORRECT — r defined once, reused
r  = "sqrt(4.0 - v^2)"
g = plot3D(f"({r})*cos(u)", f"({r})*sin(u)", "v",
           0, 2*pi, -2, 2, 60, 60, False, True)
```

**`setMatrix(matrix)`** takes a `Matrix` object only — **not** another `Graph3D`. Use `matrix("name")` or `currentMatrix()`.

**`setDataColors` and `rescaleColorMap()` are mutually exclusive.** `rescaleColorMap()` reloads the system color map and discards any `setDataColors()` gradient. Call one or the other, never both:

```python
g3.setDataColors(QColor("navy"), QColor("firebrick"))  # custom gradient — no rescaleColorMap()
# OR
g3.rescaleColorMap()                                   # system map — no setDataColors()
```

---

### Plotting

**When the request's data has a known physical quantity and unit (e.g. SANS `Q` in Å⁻¹, `I(Q)` in
cm⁻¹, a fitted radius in nm), set real axis titles with those units via `setXTitle()`/`setYTitle()`
(see graph-python-api.md) — don't leave a new graph's axes at default labels, which just echo the
raw column name (`"Q"`, `"I"`) with no unit or physical meaning.** This never raises an error, so
it's easy to skip — confirmed live across many SANS-fit requests in a row, none of which called
`setXTitle()`/`setYTitle()` despite an explicitly known `Q`/`I(Q)` physical meaning. Use HTML tags
for units/exponents, not LaTeX (see graph-python-api.md's superscript rule):
```python
g.setXTitle("Q (&Aring;<sup>-1</sup>)")
g.setYTitle("I(Q) (cm<sup>-1</sup>)")
```

**`plot()` always creates its own new graph window.**
Never call `newGraph()` before `plot()`.

```python
# t = newTable("data", 100, 2); t.setColName(1, "x"); t.setColName(2, "y")
# plot() takes bare column names when the table object is passed

# CORRECT
t = table("data")
g = plot(t, ("x", "y"), style=Graph.CurveType.Line)
setWindowName(g, "MyGraph")
activateWindow(g)
maximizeWindow(g)

# WRONG — newGraph() creates an empty orphan window; plot() ignores it
g = newGraph("MyGraph")
plot(t, ("x", "y"), style=Graph.CurveType.Line)
```

**After `plot()` returns a window, use that object directly — don't re-look it up afterward.**
`setWindowName()` renames the window in place; the variable you already have (`g`) remains valid
and already IS the same object `graphWindow("name")` would return.

```python
# WRONG — gw is a redundant re-fetch of the exact same object already held in g
g = plot(t, ("x", "y"), style=Graph.CurveType.Line)
setWindowName(g, "MyGraph")
gw = graphWindow("MyGraph")
gw.activeLayer().replot()

# CORRECT — g is already the renamed window; use it directly
g = plot(t, ("x", "y"), style=Graph.CurveType.Line)
setWindowName(g, "MyGraph")
g.activeLayer().replot()
```

Confirmed live.

**`GraphWindow` (`MultiLayer`) has no `replot()` method.**
Use `g.activeLayer().replot()`.

```python
g.activeLayer().replot()
g.activeLayer().addCurve(t, "y", Graph.CurveType.Line)  # single Y col str — NOT a tuple; X is auto-detected
```

---

### Widget names

**Widget names are case-sensitive.** `table("Sample1")` and `table("sample1")` are different
objects. Always use the exact name shown in the window title bar. Don't default to a capitalized
generic placeholder like `table("Data")` when a request uses a plain lowercase word (e.g. "table
data") — that's the common English word, not a real table name; use the exact name the user gave,
case included, or discover it via `windows()`/an EXPLORE step if genuinely unnamed.

**Lookup vs create — naming pattern:**

| Look up existing | Create new |
|------------------|------------|
| `table("name")` | `newTable("name", rows, cols)` |
| `note("name")` | `newNote("name")` |
| `matrix("name")` | `newMatrix("name", rows, cols)` |
| `graphWindow("name")` | `newGraph("name", layers, rows, cols)` |

**If the intent is to modify/extend/add to something that might already exist, use the LOOKUP
function (`table()`/`graphWindow()`/etc.), not the CREATE function** — even though the create
function also works on an existing object by name, that's not what it's for, and for tables it's
actively destructive. `newTable(name, rows, cols)` on an existing table resizes AND CLEARS it —
fewer columns than the table currently has silently discards the extra columns and ALL data, even
columns added in an earlier, separate turn. Confirmed live: a table created with 2 columns, later
extended to 3 (an error-bar column added separately) and plotted with error bars, was "corrected"
via `newTable(name, n_points, 2)` again — silently wiping the error-bar column and all data,
breaking the graph's error-bar curve.
`newGraph(name, ...)` is a DIFFERENT risk with `layers=1`: calling it on an existing name is a
complete no-op — untouched, no clearing, no resize — so it silently does nothing useful if the
intent was actually to add fresh content. With `layers>1`, `newGraph(name, layers, rows, cols)`
DOES clear every existing layer's curves (and rearranges the grid if it differs) before handing
the window back — it's the "build from scratch" entry point, not an incremental-add one. Still,
use `graphWindow(name)` to work with an existing window incrementally (adding more curves without
clearing what's there), and reserve `newGraph()` for genuinely starting over.

```python
# WRONG — "correcting" an existing table via newTable(): cols=2 matches the ORIGINAL creation
# call, not the table's CURRENT structure (it has grown a 3rd "dI" column since) — this silently
# drops that column and clears every cell, breaking any graph already plotting it
t = newTable("SphereSANS", 200, 2)
for i in range(1, 201):
    q = 0.001 + 0.0025 * (i - 1)
    I = corrected_formula(q)
    t.setCell(1, i, q)
    t.setCell(2, i, I)

# CORRECT — look up the existing table and fix only what actually needs fixing; the "dI" column,
# its data, and any graph/curve depending on this table are all left untouched
t = table("SphereSANS")
for i in range(1, t.numRows() + 1):
    q = t.cell(1, i)
    t.setCell(2, i, corrected_formula(q))
t.notifyChanges()
```

Adding a new column to an existing table is the same principle, one level further: `newTable(name, rows, cols+1)` would still hit the same resize+clear path and wipe every existing column's data just to add one more. Use `table()` + `addColumn()` instead — it appends in place, leaving every existing column untouched:

```python
# WRONG — bumping cols to "add" a column still resizes+clears the WHOLE table via newTable()
t = newTable("SphereSANS", 200, 3)   # existing "q"/"I" data is wiped even though the intent was
                                      # only to ADD a 3rd column, not touch the first two
t.setColNames(["q", "I", "dI"])
# ... q and I now need to be recomputed/recopied from scratch just to undo the wipe

# CORRECT — look up the existing table, append a column with addColumn(); q and I are untouched
t = table("SphereSANS")
t.addColumn(Table.PlotDesignation.yErr)          # appends one new column, existing ones unchanged
t.setColName(t.numCols(), "dI")                  # numCols() now reflects the column just added
for row in range(1, t.numRows() + 1):
    I = t.cell(2, row)
    t.setCell(3, row, 0.01 * I)                  # 1% relative error, purely additive
t.notifyChanges()
```

---

### Column names

**When a table object is passed, use bare column names.**
When only a string is used (fittable `datasets=`, `addCurve` without a table object), use `"tablename_colname"`.
For a table named `Table` with a column `y`: bare = `"y"`, string-only = `"Table_y"`.

```python
t = newTable("Table", 100, 2)
t.setColName(1, "x")
t.setColName(2, "y")
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(2, Table.PlotDesignation.Y)
# Y-column name for fitting: "Table_y"   (tablename_colname)
```

Use `f.datasets()` to list all Y-column names available in the workspace.

> **Never put `_` inside the column label itself.** `"tablename_colname"` is split on the *last*
> underscore internally — a column named `"y_noisy"` in table `"data"` builds the identifier
> `"data_y_noisy"`, which gets misparsed as table `"data_y"` (doesn't exist) + column `"noisy"`.
> The fit then silently fails to find the table and aborts with `"A problem with Reading Data"`.
> Confirmed live. Use `"y"` or `"ynoisy"`, never `"y_noisy"`.

---

### Column Addressing

Different objects use **different argument order** for cell access:

| Context | Argument order | Example |
|---------|----------------|---------|
| **Table** cell access | col **first**, row second — both 1-based | `t.setCell(col, row, value)` · `t.cell(col, row)` |
| **Matrix** cell access | row **first**, col second — both 1-based | `m.setCell(row, col, value)` · `m.cell(row, col)` |
| `plot()` / `addCurve()` with table object | bare column names | `"x"`, `"y"` — **not** `"MyData_x"` |
| Fittable `datasets=` (string-only context) | `"tablename_colname"` | table `"MyData"`, col `"y"` → `"MyData_y"` |

**`colNames()` returns BARE labels, same as the `plot()`/`addCurve()` row above — NEVER the
table-prefixed form:** `t.colNames()` → `("x", "y")`, not `("MyData_x", "MyData_y")`. Confirmed
against source (SIP bindings call `colLabel()`, not the table-prefixing C++ `Table::colName(int)`,
never invoked from Python). The `"tablename_colname"` form in the Fittable row above must be built
manually (`t.objectName() + "_" + label`) — `colNames()` never produces it. Confirmed live:
checking `"MyData_y" not in t.colNames()` as an existence guard is ALWAYS true even when column
`"y"` exists, since that string never appears in the result — this silently blocked an entire
comparison-and-plot stage with no error, while split-mode reported the affected parts "done."

---

### Compiler / Fittable

Compiler and Fittable rules (the `"ai/"` namespace requirement, vary-flag reset on `configure()`,
dataset naming, plotResults()/curve-color/stage rules, X-range selection, LM vs Simplex, ...) live
in `compile-python-api.md`'s own "For AI: Workflow Rules" section and `fittable-python-api.md`'s
own "For AI: Common Mistakes" section, not duplicated here — those files are the ones actually
scoped in whenever `compile`/`fittable` topics are requested, so a copy here would only ever be
redundant or, worse, drift out of sync with the real one.

---

### Meta commands

Type these directly into the AI input — they are answered instantly without an API call:

| Command | Result |
|---|---|
| `ai status` | model, URL, docs mode, history turns, max tokens |
| `ai model` | active model name |
| `ai url` | base URL |
| `history` | list Q&A turns from this session |
| `clear history` | reset conversation context |
| `stop` / `ai stop` / `cancel` / `ai cancel` | abort a request currently in flight |

---

### Qt bindings

QtiSAS embeds **PyQt6**. Always import from `PyQt6`, never `PyQt5`.

Qt symbols (`QColor`, `QFont`, `QPen`, `Qt`, `QFrame`) are **not** in the default script namespace — import them explicitly when needed:

```python
from PyQt6.QtGui import QColor, QFont, QPen, QBrush
from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import QFrame
```

> **Never switch an already-correct `PyQt6` import to `PyQt5` (or vice versa) — confirmed live:** a
> script correctly started with `from PyQt6.QtGui import QColor, QFont`. A LATER self-review pass,
> while fixing an unrelated issue elsewhere in the same script, silently rewrote that line to
> `from PyQt5.QtGui import QColor, QFont` for no reason connected to the actual fix — the script
> crashed with `ModuleNotFoundError: No module named 'PyQt5'` a full round-trip later. If a script
> already imports from `PyQt6`, never change that to `PyQt5` while fixing something unrelated.
>
> (A small number of older Linux builds may still embed PyQt5 instead — if `ModuleNotFoundError: No
> module named 'PyQt6'` is the actual error seen at runtime, that specific build is the exception;
> switch that script's imports to `PyQt5` and prefer PyQt6 for everything else.)

Never import `QApplication` — it transfers ownership to Python GC and crashes the app.
