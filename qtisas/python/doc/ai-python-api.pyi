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
#
# NOISE MODEL: sqrt(abs(y)) is a shot-noise (raw photon COUNT) model — do NOT reuse it as a
#   generic "add noise" recipe for a value already in its own calibrated physical units (intensity
#   cm^-1, etc.). For small y (e.g. ~0.001), sqrt(abs(y)) is LARGER than y itself -> noise swamps
#   the signal -> many samples go negative -> clamping to a floor (e.g. 1e-20) with a proportional
#   (now tiny) error bar creates fake near-infinite-weight outliers that corrupt a weighted fit.
#   Confirmed live: I~0.001, noise=sqrt(abs(I))*0.10 (std~3x the signal) -> 31% of 150 points
#   negative/floor-clamped -> meaningless fit (chi2/dof~64, R2=0).
#   WRONG: noise = sqrt(abs(I)) * 0.10; I_obs = I + random.gauss(0, noise)
#   RIGHT: I_obs = I + random.gauss(0, 0.10 * I)   # proportional to I's own magnitude

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
# Everything below (EXPLORE round-trips, the mandatory self-review pass, the automatic error-retry
#   loop) only fires in AUTO-RUN mode (Ctrl+Shift+Enter / Ctrl+Shift+Click) — in plain "generate
#   only" mode (bare Enter/Click) a "# EXPLORE" first line is never intercepted, the code is just
#   shown once as the answer with no execution, no self-review, no error-retry. Don't assume any of
#   this is active if you don't know which mode produced the current request.
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
# QtiSAS ONLY RE-INVOKES YOU WHEN THE SCRIPT'S FIRST LINE IS EXACTLY "# EXPLORE" — ANY OTHER
#   SCRIPT IS TREATED AS FINAL. This applies on EVERY round, not just the first — omitting the
#   marker on round 2, 3, ... silently ends the task the same way omitting it on round 1 would.
#   EXPLORE is not limited to one round: if output from one EXPLORE raises a new question, return
#   ANOTHER # EXPLORE script rather than guessing you must write the action script now (capped at
#   the same configurable limit, default 4, shared with the error-retry mechanism above). The test
#   is simply: does this script's job include "find out X"? If yes it needs the marker, no matter
#   which round — even a purely read-only script is NOT treated as "one more explore step" without
#   it. It runs once, output goes to the console, and the conversation ends there — if the task
#   asked for an action (fit, plot, compile, ...) and this script only gathered info, that action
#   NEVER HAPPENS: no error, no second chance, nothing signals the task is incomplete.
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
# WHEN REPORTING/COMPARING/DECIDING FROM A VALUE AN EARLIER ROUND/PART ALREADY PRINTED, use that
# literal printed value — NEVER invent a plausible placeholder, even one labeled "(example)". The
# real number is sitting right there in this conversation's own visible history; typing a
# different one is fabrication, not a shortcut — worse than not knowing, since it produces a
# validated-looking answer with fabricated evidence. Confirmed live: a part comparing two earlier
# fits' chi2/dof (genuinely printed as 1.0561 and 216.6961) printed brand-new fabricated numbers
# (1.24, 5.79, commented "# example, from prior fit") and decided from those instead.
#   WRONG: scriptPrint("chi2/dof = 1.24 (example, from prior fit)")   ← invented, not the real value
#   RIGHT: scriptPrint("chi2/dof = 1.0561 (from earlier fit)")        ← the value actually printed
# If the real value genuinely isn't visible anymore, say so and re-derive it — never fill the gap
# with an invented number.
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
# ANY task that FITS USING AN EXISTING, ALREADY-COMPILED library function MUST start with
# EXPLORE to discover the exact function name and its real parameters — guessing produces
# ValueError (name doesn't exist) or a fit silently using wrong defaults.
#   WRONG: f.configure("sasviewmodels/gaussian-peak-v2", ["Table1"]); f.fit()  ← guessed
#          name, no EXPLORE step, → ValueError: not found
#   RIGHT: # EXPLORE first: c.open(name); scriptPrint(str(c.paramNames()))
#          # then, using the confirmed real name: f.configure(name, ["Table1"]); f.fit()
#
# THIS IS A DIFFERENT TASK FROM "compile/write/create a CUSTOM fit function" — do NOT
# search fitFunctions()/functionNames() for a coincidentally-matching existing name and
# reuse it when the task's own words ask you to author NEW code. Confirmed live: asked to
# "compile a custom fit function" for a sphere form factor, EXPLORE's search for
# "sphere"+"sans" found a leftover function from an EARLIER session/test run and reused it
# instead of writing fresh code — then, having skipped the authoring step, the script still
# tried to force a "compile" on the found-by-name function via c.compile("FoundName"), which
# raised TypeError (compile() takes only an optional bool, never a name — it always operates
# on whatever's currently open()/new()'d in the Compiler's live state, not a name you pass
# in) and repeated identically on retry. If the task says compile/write/create/author a
# custom function: c.new(); c.setFunctionName("ai/<shortName>"); c.setXName(...);
# c.setYName(...); c.setParamCount(...); ...; c.setCode(...); ok = c.compile()  — no
# fitFunctions() search, no c.open() of someone else's function, and NEVER pass a name to
# compile(). ALWAYS name it "ai/<shortName>" (see compile-python-api RULE 12) — this is what
# lets a LATER "use an existing model" search exclude your own past one-offs instead of
# mistaking them for vetted library models, exactly the confusion that caused this bug.
# If a pre-existing function ALREADY covers what's needed and the task doesn't ask for new
# code, no compile() call is needed at all — a function found via fitFunctions() is already
# compiled; just pass its name straight to f.configure().
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
# table("name")/matrix("name")/graphWindow("name") return None (confirmed) when missing —
# ALWAYS check "if t is None:" before using the result, even for a single-line read —
# BUT ONLY when the task implies the data ALREADY EXISTS (a named table, "the data", "my
# results"). t.cell(...) on None raises AttributeError; passing None to addCurve() raises
# ValueError. One line handles it gracefully: t=table("X"); scriptPrint("not found" if t is
# None else str(t.cell(2,3))) — no separate EXPLORE round-trip needed for a single lookup.
# For tasks needing MORE than one value (structure/columns) before a follow-up action
# script, run EXPLORE first to gather that: existTable("X") for tables; NO existMatrix()/
# existGraph() exist (confirmed live NameError) — use matrix("X") is not None /
# graphWindow("X") is not None instead.
#
# THIS EXISTENCE CHECK DOES NOT APPLY WHEN THE TASK ASKS YOU TO CREATE THE DATA YOURSELF
# (e.g. "create a table with sphere form factor data", "generate synthetic SANS data") —
# that table doesn't exist yet BY DESIGN; gating newTable() behind an existence check on the
# very name you're about to create is backwards, not a safety check. Confirmed live,
# self-review-introduced: a working create+fit+plot script was "corrected" by inserting
# "if not existTable('Sphere'): scriptPrint(...)" in front of the newTable('Sphere', ...)
# call that was about to create it (the elif branch even referenced t.colNames() before t
# was ever assigned) — the script silently printed "does not exist" and never did any of
# the actual work. If the task's own words are create/generate/make me/synthesize a
# dataset, go straight to newTable() — do not check whether it exists first.
#
# THE WHOLE SCRIPT RUNS AT TOP LEVEL, NOT INSIDE A FUNCTION — bare "return" there is a
# SyntaxError (compile-time), NOT a catchable runtime error. The auto-retry only fires on
# exceptions raised DURING execution; a SyntaxError happens before execution starts, so there is
# NO automatic retry at all — the script is shown once as "not runnable" and the exchange just
# stops, unlike a genuine runtime error which DOES get resent. Use if/else nesting for
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
# NAMING: use newPlot3D(name, ...) not plot3D(...)+setWindowName(g,name) — the two-step form
#   raises ValueError: name already exists on any re-run with a taken name, AND plot3D(...)
#   already created ANOTHER orphaned window before that error fired (creation always succeeds,
#   only the rename fails). Confirmed live: this cascaded into graphWindow(name) (2D-only, always
#   None for a Graph3D name -> AttributeError) and left 5 orphaned windows behind.
#     WRONG: g3 = plot3D("sin(x)*cos(y)", -pi,pi,-pi,pi,-1,1,60,60); setWindowName(g3,"Surface3D")
#     RIGHT: g3 = newPlot3D("Surface3D", "sin(x)*cos(y)", -pi,pi,-pi,pi,-1,1,60,60)
#   If the name is already used by a NON-Graph3D window (names are unique across ALL window
#   types), newPlot3D(name,...) raises ValueError: "...already exists but is not a Graph3D
#   window" immediately, before creating anything.
# FORMULA SYNTAX (muParser — NOT Python):
#   Power: x^2  NOT x**2
#   Ternary ?: DOES work (verified in source) — prefer sign(threshold-v) for piecewise switching
#   (+1/-1) anyway for chained/composite shapes, since it stays a smooth multiplier. No if() — it's
#   registered with no actual 3-arg handler (broken, not by design) and silently never becomes callable.
#   Functions (not exhaustive): sin cos tan asin acos atan sinh cosh tanh asinh acosh atanh exp log
#   ln log2 log10 sqrt abs sign rint floor ceil pow min max sum avg mod
#   NOTE: atan2 and round do NOT exist under those names (verified against the registration list) —
#   use rint for round-to-nearest; there is no 2-argument arctangent registered at all.
# SHARED SUB-EXPRESSION: build r(v) as Python string, interpolate into BOTH X and Y
#   WRONG:  plot3D("sqrt(R^2-v^2)*cos(u)", "sqrt(R^2-v^2)*sin(u)", ...)   # duplicated
#   RIGHT:  r = "sqrt(R^2-v^2)"; plot3D(f"({r})*cos(u)", f"({r})*sin(u)", ...)
# setMatrix(matrix): takes Matrix object ONLY — not Graph3D; use matrix("name") or currentMatrix()
# uClosed=True closes azimuthal seam; vClosed=False when surface tapers to r=0 at both v-ends
# setDataColors(c1,c2) vs rescaleColorMap(): mutually exclusive — rescaleColorMap() discards setDataColors()

# PLOTTING
# When the data's physical quantity/unit is known (SANS Q in 1/A, I(Q) in 1/cm, radius in nm, ...),
#   set g.setXTitle()/setYTitle() with that unit — default axis labels just echo the bare column name
#   ("Q","I") with no unit. Never errors, easy to forget every time (confirmed live, repeatedly).
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

# FITTING — X-RANGE
# When generating synthetic data or setting a fit range, the data MUST cover the function's full
#   dynamic range — a feature (peak/knee/plateau) outside the data range makes all parameters
#   highly correlated and errors blow up:
#   Peaked (Gaussian/Lorentzian): x_min < x0-3*sigma AND x_max > x0+3*sigma
#   Decay A*exp(-b*x)+c: x_max >= 3/b (need >=3 decay lengths to separate amp from background)
#   Guinier I0*exp(-Rg^2*q^2/3)+bgd: q_min < 1.73/Rg — else signal ~= 0 everywhere, only bgd visible
#   Power-law/plateau: cover the full transition from steep slope to flat region

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
#   (e.g. an error-bar column) — breaking any graph plotting it. newGraph() is NOT the same risk:
#   with its default layers=1, calling it on an existing name is a complete no-op — the graph comes
#   back completely untouched, no clearing, no resize. Only a DIFFERENT row/col layer grid than the
#   graph already has causes any change, and even then it's just the layer arrangement, never curves.
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
# Fittable datasets: "tablename_colname" (table "MyData", col "y" → "MyData_y")
# IMPORTANT: colNames() returns BARE labels, same as plot()/addCurve() above — NEVER the
#   table-prefixed form: t.colNames() → ("x","y"), NOT ("MyData_x","MyData_y"). Confirmed against
#   source (SIP calls colLabel(), not the differently-named, table-prefixing C++ colName(int),
#   never invoked from Python). Build "tablename_colname" yourself for Fittable — colNames() is
#   never where it comes from. Confirmed live: "MyData_y" not in t.colNames() is ALWAYS true even
#   when col "y" exists — silently blocked an entire comparison+plot stage, no error, "done" anyway.
# NEVER put "_" inside the column label itself — split is on the LAST "_", so col "y_noisy" in
#   table "data" builds "data_y_noisy" -> misparsed as table "data_y" (doesn't exist) + "noisy".
#   Fit then silently fails with "A problem with Reading Data". Use "y"/"ynoisy", not "y_noisy".

# FITTABLE
# configure()/setFunction() select BY NAME — do NOT pass formula string
#   WRONG: f.configure("a*x+b")   RIGHT: f.configure("Guinier",["data_I"])
# EVERY configure()/setFunction() call RESETS all parameter adjustability to the COMPILED
#   function's baked-in defaults — it never inherits vary flags from a prior fit session on the
#   same function. Re-apply setParamVaries()/setParamVariesAll() after EVERY configure() call, not
#   just the first. Confirmed live: switching functions then switching back produced a bad fit
#   because the varies flags from before the switch were silently gone.
# datasets= takes Y-column names NOT table names: ["data_I"] not ["data"]
# setCurveColor(name_or_index): call AFTER configure(), BEFORE fit()/plotResults()
# do NOT import PyQt5/PyQt6 — use f.setCurveColor("blue")
# fit()/simulate()/plotResults()/beforeFit()/afterFit() require stage=="fit"
# simulateCurve() requires stage=="simulate"
# plotResults() returns a GraphWindow (MultiLayer), NOT a Graph (layer) — legend()/newLegend()/
#   setLegend()/setXTitle()/setYTitle()/addCurve()/setCurveLineColor()/etc. all raise
#   AttributeError if called directly on it; always: g = f.plotResults(...).activeLayer(); then
#   call these on g. Confirmed live, FIVE AttributeErrors in one conversation from this exact
#   mistake (gw.setXTitle, gw.legend, gw.setLegend x2, gw.newLegend) — see fittable-python-api.
#   plotResults(graphName,...) DOES reuse a same-named existing window (no pileup from this
#   alone) — a SEPARATE, redundant bare plot(t,...) call before fitting (which has no reuse)
#   is what actually orphans one window per retry; skip it if plotResults() already shows the data.

# META COMMANDS (type in AI input — answered instantly, no API call)
# ai status     — model, URL, docs mode, history turns, max tokens
# ai model      — active model name
# ai url        — base URL
# history       — list Q&A turns from this session
# clear history — reset conversation context (same as Settings → AI → Clear)
# stop / ai stop / cancel / ai cancel — abort a request currently in flight

# QT BINDINGS
# macOS/Windows: PyQt6 only. Linux: PyQt5 or PyQt6 depending on Qt version.
# (import rule for QColor/QFont/QPen/Qt/QFrame/QBrush and the QApplication ban: see top of file)
# Prefer QtiSAS API over direct Qt calls where one exists.
