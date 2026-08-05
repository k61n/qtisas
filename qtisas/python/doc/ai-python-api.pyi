# QtiSAS AI Assistant
# Entry: Status bar (Ctrl+Enter=show code, Ctrl+Shift+Enter=auto-run) | Script Window Ask AI | Note Ask AI
# Setup: Settings → AI  (API key, model, base URL, unlimited docs toggle)

# MANDATORY IMPORT RULE — EVERY script that uses QColor, QFont, QPen, Qt, QFrame, QBrush MUST begin with:
#   from PyQt6.QtGui import QColor, QFont, QPen, QBrush
#   from PyQt6.QtCore import Qt
#   from PyQt6.QtWidgets import QFrame
# These symbols are NEVER in the default namespace. Omitting the import → NameError at runtime.
# NEVER import QApplication — crashes the app.

# scriptPrint() REQUIRES a str (or None) argument — NOT float/int/list/tuple directly.
#   cell(), paramNames(), fitFunctions() and most read methods return non-string types.
#   WRONG: scriptPrint(t.cell(2, 3))        → TypeError: argument 1 has unexpected type 'float'
#   RIGHT: scriptPrint(str(t.cell(2, 3)))

# MATH FUNCTIONS ARE GLOBAL — sin cos exp log sqrt pi e floor ceil abs are in the global namespace.
#   Do NOT write "import math" — use bare names directly. No import needed.
#   WRONG: import math; math.exp(-x)   ← leads to math.random() mistakes; "import math" not needed
#   RIGHT: exp(-x)   sin(q)   sqrt(v)   pi   e             ← no import, works everywhere
#   APPLIES DURING SELF-REVIEW TOO: don't "fix" compliant bare sin/cos/pi/sqrt usage by adding
#   "from math import ...", even though that looks like safer standard Python — the bare globals
#   ARE correct here. Confirmed live: self-review silently inserted that import into an already-
#   compliant script — same unrequested-edit pattern as dropping a needed import, just adding a
#   forbidden one instead.
#
# RANDOM NUMBERS — "import random" is required; random is NOT global:
#   WRONG: math.random()  math.gauss()  math.uniform()  → AttributeError: math has NO random functions
#   RIGHT:
#     import random
#     noise = random.gauss(0, sigma)   # Gaussian noise
#     v     = random.random()          # uniform [0, 1)
#     v     = random.uniform(a, b)     # uniform [a, b)
#
# KEYWORD ARGS — only params WITH a default can be passed by name (SIP keyword_arguments="Optional");
#   required params must be positional even though a TypeError's own printed signature shows their
#   name/type as if keyword-capable:
#   WRONG: g.activeLayer().setLinOrLogAxis(Graph.Axis.Bottom, logYN=True)  → TypeError: 'logYN' is not a valid keyword argument
#   RIGHT: g.activeLayer().setLinOrLogAxis(Graph.Axis.Bottom, True)        # positional; confirmed live
#
# UNSELECTED-TOPIC API USE IS A WARNING SIGN — if your script calls into Fittable/Compiler/Graph3D/
#   etc. whose topic docs weren't loaded this request, the task likely drifted, not just the API
#   choice — you're now guessing that API from memory instead of its authoritative reference.
#   Confirmed live: "create a table with sphere form factor data" (docs: table only) drifted into
#   repeatedly exploring/configuring an unrelated Fittable model and never created any table.
#   NOT A REASON TO REFUSE THE TASK: if it's achievable with no API call at all (e.g. a known
#   formula computed directly in a plain loop), just do that — that's not "drifting", it's correct.
#   Confirmed live (regression from the rule above): asked for the same sphere-form-factor table,
#   a response refused outright — "requires a fitting function, not just a table" — false; the
#   formula was already known (correctly recited when asked) and needed zero Fittable involvement.
#
# RETRIES RE-RUN THE WHOLE SCRIPT, INCLUDING calls that already succeeded before the failure point
#   — newTable()/newGraph() at least reuse by name (see Lookup vs create), but plot() CANNOT: it
#   unconditionally makes a new window every call, no name-based reuse at all. A plot() call before
#   a later failure creates one new orphaned window per retry round. Confirmed live: several
#   retries to fix an unrelated addErrorBars() call left six graphs from one request. If a script
#   has plot() and might need retries, check graphWindow(name) first and skip re-plotting if it
#   already exists, or put plot() only after the rest of the script is verified to work.
#   NOT JUST plot() — Compiler.compile() is a second confirmed case, worse: a real compile+link,
#   not a cheap window. Confirmed live: 4 full recompiles of the same custom function across
#   retries fixing an unrelated later call. No name-based reuse exists for compile() — mitigate by
#   ordering the risky/uncertain calls BEFORE expensive ones so a failure happens before the cost.
#
# Example — synthetic data with noise (no "import math" needed):
#   import random
#   t = newTable("Data", 100, 2)
#   for i in range(1, 101):
#       x = 0.01 * i
#       y = exp(-x) + random.gauss(0, 0.02)   # exp() is global; random needs import
#       t.setCell(1, i, x)
#       t.setCell(2, i, y)

# USER RULES — a small, manually-curated list sent to the AI on every request, for corrections
# specific to the user's setup that the built-in docs don't cover.
#   addRule(text)->bool       add a rule; raises ValueError at the rule-count limit (default 20,
#                             configurable in Preferences > AI)
#   removeRule(index)->bool   remove by 1-based position (see showRules()); ValueError if out of range
#   removeAllRules()->bool    remove every rule at once
#   showRules()->list[str]    print numbered rules to console, return them as a list
# The exact current count/limit are always shown to you directly as "## User Rules (N/limit)"
# in your own prompt (live values) — trust that over the "default 20" figure above if they differ.
# MANUAL ONLY — never call addRule() on your own initiative, even right after being corrected.
# Only add a rule when the user explicitly asks you to remember/persist something.
# If addRule() raises the rule-limit error, REPORT its 3 options to the user — do NOT
# autonomously call removeRule()/removeAllRules() or change the Preferences limit yourself.
# This still applies after the user says to proceed — a vague "try again"/"go ahead" authorizes
# making room, NOT which rule to sacrifice. Ask which one, unless the user already specified a
# target (index, content, or "remove the oldest"). Picking one yourself (e.g. by index order)
# can discard a distinct rule while leaving near-duplicates intact.
# Stored in ~/.config/qtisas/ai/user-rules.md; also shown in Help > Python API > User Rules.
#
# BEFORE EVERY ACTION SCRIPT RUNS, you are automatically asked once to review it against a
# checklist (User Rules + a few known silent-failure gotchas) — take it seriously, it exists
# because these get missed under task pressure even when you already know them. The message:
# "Before this runs, check it against the checklist below. If it violates anything, return ONLY
# the corrected script. If not, return the SAME script unchanged." Respond with real corrected
# code if something applies, or the unchanged script if not — no explanation either way. Runs
# exactly once per candidate script, automatically, regardless of what produced it.
# CHECKLIST ITEMS (including User Rules) ARE DEFAULTS, NOT ABSOLUTE OVERRIDES. If the original
# request explicitly specified something that contradicts a checklist item for that exact aspect
# (e.g. named a specific color while a rule sets a different default color), the explicit request
# wins — only apply a checklist item where the request left that aspect unspecified. This already
# went wrong once: an explicit "blue line" request got silently forced back to green by a
# standing "use green lines" rule during self-review.
# THIS REVIEW IS A COMPLIANCE CHECK, NOT A CHANCE TO IMPROVE THE SCRIPT — only remove/correct
# what's needed to comply with the checklist, never add functionality/styling/embellishments
# beyond what's actually asked for, even if it seems like a nice addition. Already went wrong
# once: reviewing a "plot it nicely for a presentation" request added unrequested tick-length
# styling with a wrong argument count, causing a TypeError absent from the original draft.
# Anything you add during review is itself unreviewed code — same risk of being wrong as anything
# else you generate.

# AGENTIC EXPLORE MODE
# If you need to read current state before acting (e.g. read existing code/info/params),
# return ONLY a read-only script with "# EXPLORE" as the VERY FIRST LINE.
# QtiSAS will run it, capture all scriptPrint() / print() output, and send it back as context.
# Once you have everything you need, return the action script (without # EXPLORE).
# Use scriptPrint() to output data — it is captured and returned to you.
# Example:
#   # EXPLORE
#   c = Compiler
#   c.open("sasviewmodels/mass-fractal")
#   scriptPrint(c.info())
#   scriptPrint(str(c.paramNames()))
# NEVER mix EXPLORE and action code in one script — EXPLORE scripts must be read-only.
#   No newTable()/newGraph()/addCurve()/maximizeWindow()/plot()/etc. — plot() is the easy one to
#   miss, it doesn't read like a "write" call but unconditionally creates a new graph window every
#   single call. Anything write/create belongs in the action script only, or it runs twice.
#
# IF YOUR ACTION SCRIPT RAISES A RUNTIME ERROR, QtiSAS automatically sends it back to you
# ("The action script raised an error:" + traceback) and asks for a corrected script — respond
# with ONLY the corrected script, no explanation. Same automatic round-trip as EXPLORE, no need
# to ask the user to paste the error. Capped at a configurable number of total automatic
# continuations per user request (default 4, Preferences > AI; shared with EXPLORE rounds below)
# — if still failing after several attempts, reconsider whether the task's premise is wrong
# instead of retrying the same mistake.
#
# DO NOT WRAP setParamValue()/setParamVaries()/fit() IN A BROAD try/except THAT JUST PRINTS AND
# CONTINUES — this defeats the automatic retry above. QtiSAS only captures errors that reach the
# script's top level (uncaught); swallow it locally and execution continues past the failed
# call (e.g. plotResults() runs even though fit() was never reached), reporting success off
# whatever the graph already contained. A misleading "done" message is worse than a visible
# failure. Only catch exceptions you can actually recover from within the same script.
#   WRONG: try: f.setParamValue("radius", 50.0); f.fit()
#          except Exception as e: scriptPrint("Fit error: " + str(e))
#          f.plotResults("FitResult", "")   ← runs anyway, reports stale/no-op success
#   RIGHT: f.setParamValue("radius", 50.0); f.fit(); f.plotResults("FitResult", "")
#          # let it raise — QtiSAS captures it and asks you for a corrected script
#
# EXPLORE IS NOT LIMITED TO ONE ROUND — each round's output is added to the conversation. If
# output from one EXPLORE raises a new question, return ANOTHER # EXPLORE script rather than
# guessing you must write the action script now (capped at the same configurable limit, default 4,
# shared with the error-retry mechanism above). Any script whose job is still "find out X" needs
# the marker, no matter which round — omitting it (even on a read-only script) makes it the
# FINAL script and the task ends right there.
#   WRONG (round 2): c=Compiler; c.open("Gauss"); scriptPrint(str(c.paramNames()))  ← no marker,
#          becomes final, no fit ever attempted
#   RIGHT (round 2): # EXPLORE
#                    c=Compiler; c.open("Gauss"); scriptPrint(str(c.paramNames()))
#
# EACH SCRIPT RUN GETS A FRESH NAMESPACE — variables from an earlier round (f=Fittable,
# c=Compiler, t=table(...)) are NOT still bound in a later round, even the very next one.
# Only printed output carries forward, not the Python objects. Redefine setup lines every time.
#   WRONG (round 2): scriptPrint(str(f.datasets()))  ← NameError, f was only set in round 1
#   RIGHT (round 2): f = Fittable; scriptPrint(str(f.datasets()))
#
# ONCE EXPLORE CONFIRMS A SPECIFIC NAME (function/table/column matched by a search), use that
# literal name in the action script — do NOT re-run the same search again and index into it.
# Unsafe, not just wasteful: the search can resolve to a DIFFERENT match if the library changed
# between rounds, silently using unverified parameters believed to be the confirmed ones.
#   WRONG: hits = f.fitFunctions("*lorentzian*"); f.configure(hits[0], [...])  ← re-searches
#   RIGHT: f.configure("DoubleLorentzian", [...])  ← the literal name EXPLORE already confirmed
#
# OMITTING "# EXPLORE" ON A DISCOVERY-ONLY SCRIPT SILENTLY ENDS THE TASK — no follow-up round
# happens. QtiSAS only re-invokes you when the script's FIRST LINE is exactly "# EXPLORE". A
# script without that marker — even a purely read-only one — is treated as FINAL: it runs once,
# output goes to the console, and the conversation ends there. If the task asked for an action
# (fit, plot, compile, ...) and the script only gathered info, that action NEVER HAPPENS.
#   WRONG: t = table("Table1"); scriptPrint(str(t.colNames()))   ← no marker, becomes final;
#          task was "fit table Table1" but no fit is ever attempted, no error either
#   RIGHT: # EXPLORE
#          t = table("Table1"); scriptPrint(str(t.colNames()))   ← triggers follow-up round
#
# "STOP AND REPORT" MEANS CALL scriptPrint(...) WITH THE MESSAGE — never return plain English
# text as the response. Every response must be valid, executable Python (the app already
# requires this: "Return ONLY executable Python code — no explanations... no text"). That still
# applies when the right answer is to give up (e.g. EXPLORE found zero tables in the project).
# Plain text fails to compile and is shown as "AI code (not runnable)" — your message never
# reaches the user as an actual result.
#   WRONG: No data tables are present in the workspace. Please import data first.  ← not
#          Python, fails to compile, shown as "not runnable"
#   RIGHT: scriptPrint("No data tables are present in the workspace. Please import data first.")
#
# VERIFY BEFORE YOU STATE A FACT — if you haven't run code to check it THIS conversation,
# you don't know it. Partial hedging is still an unverified answer for the WHOLE claim —
# stating some values as fact while hedging only on the rest is the same mistake as
# guessing all of it. If already retrieved earlier in this conversation, cite that prior
# output — do not re-guess.
#   WRONG: "The function has 7 parameters: A, xc, sigma, background, ... (5-7 need
#          checking)" — for a function that doesn't even exist; EVERY part is fabricated.
#   RIGHT: # EXPLORE
#          c = Compiler
#          c.open("sasviewmodels/gaussian-peak-v2")
#          scriptPrint(str(c.paramNames()))
#          # only answer after seeing real output (or a real error)
#
# For read-only parameter/info lookups use Compiler.open(name) + c.paramNames(), NOT
# Fittable.configure(). f.configure(name, []) reconfigures the LIVE Fittable widget,
# discarding any fit already set up; c.open(name) only reads the source.
#   WRONG: for name in f.fitFunctions("*gauss*"): f.configure(name, []); ...  ← reconfigures
#          the live widget once per candidate, discarding whatever fit was set up
#   RIGHT: for name in f.fitFunctions("*gauss*"): c.open(name); scriptPrint(c.paramNames())
# Reserve f.configure() for when the task actually means to set that function up to fit.
#
# ANY compile-and-fit task MUST start with EXPLORE to discover the exact function name
# and its real parameters — guessing produces ValueError (name doesn't exist) or a fit
# silently using wrong defaults.
#   WRONG: f.configure("sasviewmodels/gaussian-peak-v2", ["Table1"]); f.fit()  ← guessed
#          name, no EXPLORE step, → ValueError: not found
#   RIGHT: # EXPLORE first: c.open(name); scriptPrint(str(c.paramNames()))
#          # then, using the confirmed real name: f.configure(name, ["Table1"]); f.fit()
#
# f.setParamValue(name, value) does an EXACT-STRING match against the configured function's
# real parameter names and RAISES ValueError on a mismatch (same for setParamVaries) — the
# script aborts right there, f.fit() never runs. Generic guessed names (xc, A, sigma, y0) are
# not facts; real names vary per function (e.g. GaussianPeak uses scale/center/width/background).
# Confirm via c.paramNames() BEFORE writing these calls — don't rely on catching the error.
#   WRONG: f.configure("GaussianPeak", ["Table1_2"]); f.setParamValue("xc", 0.5)  ← ValueError:
#          setParamValue: no such parameter 'xc'. — script aborts, fit never runs
#   RIGHT: # EXPLORE: c.open("GaussianPeak"); scriptPrint(str(c.paramNames()))
#          # -> ['scale', 'center', 'width', 'background']
#          f.configure("GaussianPeak", ["Table1_2"]); f.setParamValue("center", 0.5)
#
# MODIFYING AN EXISTING FUNCTION TO SAVE UNDER A NEW NAME (e.g. "fix X, save as X-correct"):
# call c.setFunctionName("new-name") BEFORE c.compile()/c.save() — they use whatever name
# is currently loaded. Opening the original, editing [code], and compiling WITHOUT renaming
# first SILENTLY OVERWRITES THE ORIGINAL instead of creating a new function. This already
# happened: opening sasviewmodels/ellipsoid, fixing its normalization, and calling
# c.compile() without setFunctionName() destroyed the original and never created the
# requested ellipsoid-correct.
#   WRONG: c.open("sasviewmodels/ellipsoid"); c.setCode(fixed); c.compile()
#          ← no setFunctionName() → overwrites "sasviewmodels/ellipsoid" in place
#   RIGHT: c.open("sasviewmodels/ellipsoid")
#          c.setFunctionName("sasviewmodels/ellipsoid-correct")
#          c.setCode(fixed); c.compile()
#
# table("name")/matrix("name")/graph("name") return None (confirmed) when missing —
# ALWAYS check "if t is None:" before using the result, even for a single-line read.
# t.cell(...) on None raises AttributeError; passing None to addCurve() raises ValueError.
# One line handles it gracefully: t=table("X"); scriptPrint("not found" if t is None
# else str(t.cell(2,3))) — no separate EXPLORE round-trip needed for a single lookup.
# For tasks needing MORE than one value (structure/columns) before a follow-up action
# script, run EXPLORE first (existTable()/existMatrix()/existGraph()) to gather that.
#
# THE WHOLE SCRIPT RUNS AT TOP LEVEL, NOT INSIDE A FUNCTION — bare "return" there is a
# SyntaxError (compile-time), NOT a catchable runtime error, so the auto-retry can't fix
# it; the same broken script gets resent and fails identically. Use if/else nesting for
# multi-line early exits, not "if cond: scriptPrint(...); return".
#   WRONG: if not existTable("X"): scriptPrint("missing"); return
#          t = table("X"); scriptPrint(str(t.colNames()))
#   RIGHT: if not existTable("X"): scriptPrint("missing")
#          else: t = table("X"); scriptPrint(str(t.colNames()))
# "raise" IS valid at top level — use it only when you want auto-retry to trigger; for a
# graceful stop-and-report, just print and let the script end normally.
# exit()/quit()/sys.exit() have the SAME problem as raise: they throw SystemExit, which the
# script runner treats as an uncaught error like any crash, triggering an unwanted auto-retry
# instead of a clean stop. Never use them for early exit — use if/else nesting instead.

# Rules for AI code generation — violations produce runtime errors:

# GRAPH3D
# One plot3D() = one surface = one window — NO multi-surface overlay
# A second plot3D() call opens a new window; setMatrix(g2) fails if g2 is Graph3D, not Matrix
# FORMULA SYNTAX (muParser — NOT Python):
#   Power: x^2  NOT x**2
#   No ternary ?:  No if() — use sign(threshold-v) for piecewise switching (+1/-1)
#   Functions: sin cos tan asin acos atan atan2 exp log log2 log10 sqrt abs sign floor ceil round
# SHARED SUB-EXPRESSION: build r(v) as Python string, interpolate into BOTH X and Y
#   WRONG:  plot3D("sqrt(R^2-v^2)*cos(u)", "sqrt(R^2-v^2)*sin(u)", ...)   # duplicated
#   RIGHT:  r = "sqrt(R^2-v^2)"; plot3D(f"({r})*cos(u)", f"({r})*sin(u)", ...)
# setMatrix(matrix): takes Matrix object ONLY — not Graph3D; use matrix("name") or currentMatrix()
# uClosed=True closes azimuthal seam; vClosed=False when surface tapers to r=0 at both v-ends
# setDataColors(c1,c2) vs rescaleColorMap(): mutually exclusive — rescaleColorMap() discards setDataColors()

# PLOTTING
# plot() ALWAYS creates its own new graph window — do NOT call newGraph() before plot()
# WRONG: newGraph() then plot() — plot() creates its own window, newGraph() is never needed before it
# plot() takes bare column names: t=table("data"); g=plot(t,("x","y"),style=0)
# importASCII is a Table METHOD not a global: t=newTable("D",100,3); t.importASCII("f.txt"); g=plot(t,("x","y"),style=0)
# After plot() returns g, use g directly — setWindowName(g,"name") renames in place, g stays valid;
#   don't re-fetch with graphWindow("name") afterward, that's a redundant lookup of the same object
#   WRONG: g=plot(t,("x","y"),style=0); setWindowName(g,"MyGraph"); gw=graphWindow("MyGraph"); gw.activeLayer().replot()
#   RIGHT: g=plot(t,("x","y"),style=0); setWindowName(g,"MyGraph"); g.activeLayer().replot()   # confirmed live
# GraphWindow (MultiLayer) has NO replot() — use g.activeLayer().replot()
# GraphWindow layers are 1-based: g.layer(1) is first, g.layer(0) is invalid
# addCurve(t,col,style): col is SINGLE Y column str — g.activeLayer().addCurve(t,"y",0)
# addCurves(t,cols_tuple,style): tuple — g.activeLayer().addCurves(t,("y1","y2"),0)

# WIDGET NAMES
# Names are case-sensitive: table("Sample1") != table("sample1")
# Use exact name from window title bar. Don't default to a capitalized placeholder like
# table("Data") when the request uses a plain lowercase word (e.g. "table data") — that's the
# common English word, not a real table name; use the exact name given, case included.
# Lookup vs create: table("n")/newTable("n",rows,cols)  note("n")/newNote("n")
#   matrix("n")/newMatrix("n",rows,cols)  graphWindow("n")/newGraph("n",layers,rows,cols)
# To MODIFY/EXTEND an existing table/graph, use the LOOKUP function, never the CREATE one — even
#   though newTable()/newGraph() also accept an existing name, that's not what they're for.
#   newTable(name,rows,cols) on an existing table RESIZES+CLEARS it: fewer cols than it currently
#   has silently drops the extra ones (and ALL data), even cols added in an earlier, separate turn
#   (e.g. an error-bar column) — breaking any graph plotting it. newGraph() on an existing graph
#   is gentler (no curve-clearing, just layer-grid resize if dims differ) but same principle.
#   WRONG: t = newTable("Sphere", 200, 2)  # table already has 3 cols (q,I,dI) -> dI silently lost
#   RIGHT: t = table("Sphere"); t.setCell(2, i, newVal)  # modify in place, nothing else disturbed
# ADDING a column is the same principle one level further — bumping cols in newTable() still
#   resizes+clears the WHOLE table just to add one column:
#   WRONG: t = newTable("Sphere", 200, 3)   # wipes existing "q"/"I" data just to add col 3
#   RIGHT: t = table("Sphere"); t.addColumn(Table.PlotDesignation.yErr)  # appends in place
#          t.setColName(t.numCols(), "dI")  # numCols() reflects the column just added

# COLUMN ADDRESSING
# Table: col FIRST, row second — both 1-based: t.setCell(col,row,value)  t.cell(col,row)
# Matrix: row FIRST, col second — both 1-based: m.setCell(row,col,value)  m.cell(row,col)
# plot/addCurve: bare column names ("x","y") — NOT full tableName_col names
# Fittable datasets: "tablename_colname" (table "data", col "I" → "data_I")
# colNames() returns full names: t.colNames() → ("MyData_x","MyData_y")
# NEVER put "_" inside the column label itself — split is on the LAST "_", so col "y_noisy" in
#   table "data" builds "data_y_noisy" -> misparsed as table "data_y" (doesn't exist) + "noisy".
#   Fit then silently fails with "A problem with Reading Data". Use "y"/"ynoisy", not "y_noisy".

# FITTABLE
# configure()/setFunction() select BY NAME — do NOT pass formula string
#   WRONG: f.configure("a*x+b")   RIGHT: f.configure("Guinier",["data_I"])
# datasets= takes Y-column names NOT table names: ["data_I"] not ["data"]
# setCurveColor(name_or_index): call AFTER configure(), BEFORE fit()/plotResults()
# do NOT import PyQt5/PyQt6 — use f.setCurveColor("blue")
# fit()/simulate()/plotResults()/beforeFit()/afterFit() require stage=="fit"
# simulateCurve() requires stage=="simulate"

# META COMMANDS (type in AI input — answered instantly, no API call)
# ai status     — model, URL, docs mode, history turns, max tokens
# ai model      — active model name
# ai url        — base URL
# history       — list Q&A turns from this session
# clear history — reset conversation context (same as Settings → AI → Clear)

# QT BINDINGS
# macOS/Windows: PyQt6 only. Linux: PyQt5 or PyQt6 depending on Qt version.
# (import rule for QColor/QFont/QPen/Qt/QFrame/QBrush and the QApplication ban: see top of file)
# Prefer QtiSAS API over direct Qt calls where one exists.
