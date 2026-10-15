# QtiSAS Fittable — f = Fittable (singleton)
# Call order: pre-function → configure → post-function settings → fit/simulate/plotResults
# IMPORTANT: configure() selects a function BY NAME — do NOT pass a formula string.
#   WRONG: f.configure("a*x+b", ...)   RIGHT: f.configure("Guinier", ["data_I"])
# IMPORTANT: NEVER hardcode a function name when using an EXISTING library function.
#   Function names depend on the compiled library and may differ between installations.
#   Exception: if the script itself compiled the function (via Compiler), the name is already known — no discovery needed.
#   Discovery pattern (REQUIRED when using a pre-existing model) — exclude "ai/*" from the
#   candidates: that namespace holds only prior AI-authored one-offs (see compile-python-api
#   RULE 12), never a vetted library model, and matching one there means the task actually
#   wanted a NEW custom function authored, not an existing model reused. If the request quoted a
#   SPECIFIC name, check it EXACTLY first — a quoted name asserts it's the real one, same as a
#   quoted table/column name; a miss is a reportable mismatch, never silent license to broaden:
#     requested = "Sphere"  # exact name the request quoted, if any — else ""
#     if requested and requested in f.fitFunctions():
#         model = requested
#     else:
#         hits = [h for h in f.fitFunctions("*sphere*SANS*") if not h.startswith("ai/")]  # most specific first
#         if not hits: hits = [h for h in f.fitFunctions("*sphere*") if not h.startswith("ai/")]
#         if not hits:
#             print(f"Model '{requested}' not found, none similar either. SANS models:"); print(f.fitFunctions("*SANS*"))
#             raise RuntimeError(f"No suitable model found in Fittable library for '{requested}'")
#         model = hits[0]
#         # MUST announce the substitution — confirmed live: skipping this silently landed on an
#         # unrelated multi-param model for a quoted "Sphere" that doesn't exist, R² = -190, no hint printed.
#         print(f"Model '{requested}' not found exactly — using closest match: {model}" if requested else f"Model: {model}")
#         if len(hits) > 1: print(f"Also available: {hits[1:]}")
#     f.configure(model, [...])  # use variable, not a hardcoded string
#     # STILL NOT DONE: configure() alone does NOT give the model sensible starting values — set
#     #   physically reasonable initial guesses (see Quick example above) before fit(), unless the
#     #   function has its own beforeFitting() hook that derives them from the loaded data (see
#     #   beforeFit() below). Skipping this does not raise an error — it silently fits from
#     #   whatever arbitrary defaults the library ships with, which can converge to garbage
#     #   (huge chi2/dof, R²~0) with nothing to flag it as wrong.
# IMPORTANT: datasets are "tablename_colname" strings — NOT table names.
#   WRONG: datasets=["data"]   RIGHT: datasets=["data_I"]
# IMPORTANT: each list item is one COMPLETE "tablename_colname" string. A 2-item list means a
#   2-DATASET GLOBAL FIT, not "table + column" split apart — splitting requests two separate
#   datasets that must each independently exist.
#   WRONG: datasets=["data", "I"]   (requests dataset "data" AND dataset "I" — neither exists)
#   RIGHT: datasets=["data_I"]      (one dataset: column I of table data)
# IMPORTANT: an explicitly-named table is a GUESS until checked — verify with app-level
#   existTable("name") BEFORE configure(), don't react to configure()'s failure after the fact.
#   f.datasets() is the wrong check for "does the table exist" — it only lists Y-columns already
#   fit-ready, so it can't tell "table missing" apart from "table exists, column not Y-tagged".
#   Table names can themselves contain "_" — "SAXS_data_I" splits at the LAST "_" (table=
#   "SAXS_data", col="I"), never the first; column names never contain "_".
# NOTE: true/false (lowercase) are aliases for True/False in QtiSAS Python scripts.
# Quick example:
#   f.configure("Guinier", datasets=["data_I"])
#   f.setParamValue("Rg", 50.0)
#   f.setParamLimits("Rg", 10, 200)
#   chi2 = f.fit()
#   f.plotResults()
#   print(f.results())   # {"Rg": (52.3, 0.4), "I0": (0.01, 0.001)}

configure(fn,datasets=[])  fit()->chi2_dof  simulate()->chi2_dof  simulateCurve()->chi2_dof
# fn is the FULL name, folder included: f.configure("ai/MyPeak", ...), never "MyPeak"
# configure(fn) with datasets OMITTED/EMPTY still sets datasetCount=1 but selects NO actual
#   table/column for it — always pass the real dataset list: configure(fn, ["table_col"]).
# fit() raises RuntimeError if the selected dataset(s) can't be read (e.g. an empty table) —
#   previously returned a stale chi2 from a prior fit with no sign anything failed.
# IMPORTANT: fit()'s return value is ALREADY chi2/dof (REDUCED chi-squared) — despite examples
#   elsewhere naming the variable "chi2" (short physics convention), it is NOT raw chi-squared.
#   Dividing it by dof AGAIN silently produces a value too small by a factor of dof, no error.
#   WRONG: chi2 = f.fit(); scriptPrint(str(chi2 / f.lastDoF))   # already chi2/dof — don't divide again
#   RIGHT: chi2_dof = f.fit(); scriptPrint(str(chi2_dof))
#   Confirmed live: a real fit printed "chi²/dof: 1.14126291068E+00" but the script's own
#   chi2/dof arithmetic reported "0.0078" (= 1.1413/146, its own dof) — self-consistent-looking,
#   no error, silently wrong by a factor of 146.
beforeFit()  afterFit()  plotResults(graphName="",graphTemplate="")->MultiLayer
# plotResults("Name") into an EXISTING 2-layer graph fills it: data + fit curve -> layer 1, residuals -> layer 2.
#   Either let plotResults() build the panels, or build them yourself with insertCurve() — not both (every curve twice).
# plotResults() returns a MultiLayer (GraphWindow) — NOT a Graph (layer). EVERY layer-level
#   method (legend(), newLegend(), setLegend(), setXTitle(), setYTitle(), addCurve(),
#   setCurveLineColor(), setCurveSymbolColor(), etc.) raises AttributeError if called directly
#   on this return value — always go through .activeLayer() first:
#     gw = f.plotResults("FitResults"); g = gw.activeLayer()
#     g.setXTitle("q, 1/nm")           # NOT gw.setXTitle(...)
#     leg = g.legend()                 # NOT gw.legend()
#     if leg is None: leg = g.newLegend()   # NOT gw.newLegend()
#   Confirmed live, FIVE separate AttributeErrors in one conversation, each burning a retry
#   round: gw.setXTitle(...), gw.legend(), gw.setLegend(...) (twice), gw.newLegend() — all
#   raised "AttributeError: 'GraphWindow' object has no attribute '<name>'". NOTE:
#   plotResults(graphName, ...) itself DOES reuse an existing window by that exact name (it
#   searches app windows for a name match before creating one) — it is NOT the source of
#   window pileup here. The actual orphaned-window driver in that same conversation was an
#   earlier, redundant bare plot(t, (...)) call made BEFORE fitting just to preview the raw
#   data — plot() has no name-based reuse at all (see ai-python-api's RETRIES RE-RUN THE WHOLE
#   SCRIPT rule) and created one new window per retry. If plotResults() already shows the data
#   (it does — see below), that earlier preview plot() call is usually unnecessary. This is the single most
#   common mistake with plotResults() specifically — see graph-python-api's own "GraphWindow vs
#   Graph" note for the general rule this is one instance of.
# beforeFit() also fires the compiled function's own beforeFit lifecycle flag (not just UI-table
#   sync) — a beforeFitting(void*ParaM) helper in the function's includedFunctions() can use this
#   to compute initial parameter values from the loaded data. See compile-python-api.
# setParamVaries(name,False)/setParamShared(name,True) are checkbox-level constraints Fittable
#   resolves itself, above the compiled function — no [code] needed. In-code constraints (e.g.
#   `Rg=fabs(Rg);` written directly in [code]) are a different mechanism: they run on every call
#   but only get persisted into the optimizer's vector / UI table during beforeFit()/afterFit().
#   For a two-peak x1<x2 ordering constraint, swap the WHOLE parameter group per peak, not just
#   the compared variable, or amplitude1/width1 end up attached to the wrong peak after a swap.
# plotResults — graphTemplate must be a name from qti.app.graphTemplates() (without .qpt), or "".
#   WRONG: f.plotResults("MyFit", "graph")   ← "graph" may not exist → silent fallback to newGraph()
#   RIGHT: templates = qti.app.graphTemplates()
#          tpl = "fit-plus-residues" if "fit-plus-residues" in templates else ""
#          f.plotResults("MyFit", tpl)
# qti.app.graphTemplates() → list of available template names (without .qpt extension)
# plotResults(graphName="", graphTemplate="")
#   graphName: name for the created graph window (optional)
#   graphTemplate: path to a .qpt template file WITHOUT extension — e.g. "/path/to/myTemplate"
#                  leave empty ("") in most cases; passing a word like "graph" silently fails (no .qpt found)
results()->{n:(v,e)} or {n:[(v,e),...]}  fitReport()->str
# results()["name"] IS the (value,error) tuple — never index with [0], never format it directly:
#   WRONG: f.results()["R"][0][0]              # TypeError: 'float' object is not subscriptable
#   WRONG: f"{f.results()['R']:.2f}"           # TypeError: unsupported format string for tuple
#   RIGHT: r_val, r_err = f.results()["R"]     # then use r_val
# IMPORTANT: a legend/print claiming to show "the fitted X" MUST read it from results() AFTER
#   fit() — never hardcode/reuse an initial guess or a synthetic-data-generation value. Confirmed
#   live: leg.setText("R = 50.00 Å") hardcoded from the parameter's initial guess, never touched
#   again — silently wrong the moment the fit converges anywhere else, no error, no warning.
#   WRONG: f.setParamValue("R",50.0); f.fit(); leg.setText("R = 50.00 Å")
#   RIGHT: f.fit(); r_val,_ = f.results()["R"]; leg.setText(f"R = {r_val:.2f} Å")
r2()->float  qFactor()->float  lastChi2 (already chi2/dof — don't divide by lastDoF)  lastDoF  stage()->str  selectStage(s)->bool  state()->dict
# stage()/selectStage(s) use a DIFFERENT 5-value vocabulary than state()["stage"] (see below):
#   "init"(no function loaded) "fit"(function+datasets configured) "simulate"(Simulate Curve tab)
#   "results"(Generate Tables tab) "batch-fit"(Batch-Fit tab). selectStage(s) raises RuntimeError if
#   called from "init" with s!="init", ValueError for an unknown name. fit()/simulate()/
#   plotResults()/beforeFit()/afterFit() all raise RuntimeError if stage is not "fit";
#   simulateCurve() raises RuntimeError if stage is not "simulate".
# state() keys: stage("init"|"ready"|"fitted")  function(str)  datasets(int count)
#   selected(list of "table_col" names)  fitMethod(int)  params(int count)  instrumental(bool)
#   chi2(float or None)  dof(float or None) — TWO SEPARATE keys (not one "chi2/dof" key), both None
#   until fitted — a one-call diagnostic snapshot, e.g. in EXPLORE.
fitFunctions(pattern="")->list  currentFunction()->str
f.functionExists(name)->bool  # Fittable.functionExists("ai/Quad") = "ai/Quad" in fitFunctions(); a method, never bare
# Fittable has fitFunctions(); it does NOT have functionNames() — that belongs to Compiler, a
#   DIFFERENT singleton (see compile-python-api.pyi). Don't borrow either singleton's method name
#   onto the other — Compiler needs c.functionNames("ALL"), never c.fitFunctions(...).
datasets()->list  selectedDatasets()->list  selectedDataset(m)->str  datasetCount()->int
# datasets() is READ-ONLY (lists what's configured) — NOT a setter. datasets(["x"]) raises
#   TypeError: too many arguments; a bare datasets() afterward compiles but sets nothing, still 0
#   datasets configured -> fit() raises "failed to read data for the selected dataset(s)".
#   To set datasets: configure(fn, [...]) again with the full list, or setDatasetCount+selectDataset.
#   Also NOT the right check for "does this table exist" (see dataset-naming rules above) — it only
#   lists Y-columns already fit-ready, so it can't distinguish "table missing" from "table exists,
#   column not Y-tagged".
paramNames()->list  paramCount()->int  f.lastFitReport
setScaleErrors(bool)  setStatisticsNotes(bool)  setSaveSession(bool)

# FIT CURVE TABLE — created by fit() and simulate()
# Single dataset: table name = "fitCurve-<model>"   Global fit N: "fitCurve-<model>-global-N"
#   <model> = the FULL name given to configure(), folder included: "fitCurve-ai/GaussPeak", never "fitCurve-GaussPeak"
# Columns (1-based index for Python cell/addCurve access):
#   col 1  "x"              role X     — Q values (same grid as input data)
#   col 2  "y"              role Y     — fitted I(Q)
#   col 3  "weight"         role yErr  — fit weight per point
#   col 4  "sigma"          role xErr  — resolution σ(Q)
#   col 5  "residues"       role Y     — RAW, UNNORMALIZED residual data-fit, NOT divided by σ/weight
#                                         despite the name suggesting a normalized residual ← still
#                                         USE THIS for a residuals panel (real residuals, not a proxy)
#   col 6  "Characteristics" role None — label per row, e.g. "chi^2/DoF", "R^2=1-chi^2/TSS"
#   col 7  "Conditions"      role None — value for that SAME row, text, prefixed "->   "
#   col 8  "Parameters"      role None — one fit param NAME per row
#   col 9  "Values"          role None — that param's fitted value, same row
#   col 10 "Errors"          role None — that param's fit error, same row (empty for simulate())
# Access from Python — ALWAYS guard: table returns None if fit failed or was not yet run:
#   fc = table("fitCurve-" + model)
#   if fc is None: raise RuntimeError("fitCurve table missing — did the fit succeed?")
#   g.insertCurve(fc, "x", "y", Graph.CurveType.Line)           # fit line
#   gr.insertCurve(fc, "x", "residues", Graph.CurveType.Scatter) # residuals panel
#   i_fit = fc.cell(2, idx)   # fit value at row idx
#   resid = fc.cell(5, idx)   # residual at row idx
# IMPORTANT: row position of each Characteristics/Parameters entry SHIFTS depending on whether SANS
#   resolution/polydispersity are enabled (confirmed against source) — search by label text, NEVER
#   hardcode a row number. Confirm exact label/param strings via EXPLORE (colNames(), print a few
#   rows) before hardcoding them.
#     def read_characteristic(fc, label):
#         for row in range(1, fc.numRows()+1):
#             if fc.text(6, row).strip() == label: return fc.text(7, row).replace("->", "").strip()
#     def read_param(fc, name):
#         for row in range(1, fc.numRows()+1):
#             if fc.text(8, row).strip() == name:
#                 value = float(fc.text(9, row).strip())
#                 err_str = fc.text(10, row).strip()
#                 return value, (None if err_str == "---" else float(err_str.replace("±", "").strip()))
# IMPORTANT: col 7 ("Conditions") is always "->" + variable spacing + the value (e.g.
#   "->   7.88510912046E+03") — .replace("->","").strip() above handles this regardless of exact
#   spacing; don't assume a fixed-width prefix (slicing off N chars) — spacing isn't constant.
# IMPORTANT: col 10 ("Errors") is "±<number>" (e.g. "±0.00974278485") for a VARYING param, or the
#   literal "---" for a FIXED one — check for "---" first, strip "±" before float() otherwise.
#   Confirmed live: float("±0.00974278485") raises ValueError: could not convert string to float.
# THIS TABLE IS THE ONLY RELIABLE SOURCE for chi2/dof, R2, and every fitted param+error once you're
#   in a LATER script/part — never re-derive from memory, printed console text, or a fresh fit().
# COMPARING TWO ALREADY-FIT MODELS needs no re-fit, no guessing, no recalled numbers — both
#   fitCurve-<model> tables persist as real by-name-lookupable objects. Full worked example:
#     fc1 = table("fitCurve-ai/fittable-model1"); fc2 = table("fitCurve-ai/fittable-model2")
#     chi2_1 = float(read_characteristic(fc1, "chi^2/DoF")); chi2_2 = float(read_characteristic(fc2, "chi^2/DoF"))
#     best_fc, best_name = (fc1,"ai/fittable-model1") if chi2_1<=chi2_2 else (fc2,"ai/fittable-model2")
#     scale_val, scale_err = read_param(best_fc, "scale")   # winner's own fitted parameters
#     # plot ORIGINAL data + winner's ACTUAL fit curve + ACTUAL residuals, 2 stacked panels:
#     # "y" on best_fc is the MODEL's calculated curve, NOT the original data — add the real data
#     # table's own curve separately or the plot silently shows only the fit line, no data points.
#     t = table("YourDataTable")   # the table originally passed to f.configure(model, ["YourDataTable_y"])
#     gw = newGraph("FitResults", 2, 2, 1); maximizeWindow(gw)
#     l1, l2 = gw.layer(1), gw.layer(2)
#     l1.enableAxis(Graph.Axis.Bottom, False); l2.enableAxis(Graph.Axis.Top, False)
#     gw.keepAligned(False); gw.setAlignPolicy(GraphWindow.AlignPolicy.AlignCanvases)
#     gw.setSpacing(0, 0); gw.arrangeLayers(False, False)
#     cr1, cr2 = l1.canvasGeometry(), l2.canvasGeometry()
#     x, y, w = cr1.x(), min(cr1.y(), cr2.y()), cr1.width(); total_h = cr1.height()+cr2.height()
#     h1 = int(total_h*0.75)
#     l1.setCanvasGeometry(x, y, w, h1); l2.setCanvasGeometry(x, y+h1, w, total_h-h1)
#     l2.removeTitle()
#     l1.addCurve(t, "y", Graph.CurveType.Scatter)                        # ORIGINAL data — index 0
#     l1.insertCurve(best_fc, "x", "y", Graph.CurveType.Line)             # winner's fit curve — index 1
#     l2.insertCurve(best_fc, "x", "residues", Graph.CurveType.Scatter)   # winner's residuals
#     l1.setCanvasGeometry(x, y, w, h1); l2.setCanvasGeometry(x, y+h1, w, total_h-h1)  # re-apply
#     gw.keepAligned(True); gw.linkXLayerAxes(True); l1.replot(); l2.replot()
#   This canvasGeometry()/setCanvasGeometry() layout is the CONFIRMED-WORKING pattern for stacked
#   panels — prefer it over guessing setLayerGeometry()/setLayerRect()/setLayout(), none of which
#   exist (confirmed live: all three raised AttributeError across repeated retries).
#   Confirmed live: skipping this and instead fabricating placeholder chi2 values, or plotting a
#   raw data column (e.g. "dI") mislabeled as "residuals", or omitting the best-fit curve overlay
#   entirely, or plotting best_fc's "y" alone and mistaking it for the original data — all four
#   happened separately when this table wasn't used for the comparison.
# IMPORTANT: t.text(col,row) used to raise "TypeError: decoding str is not supported" for EVERY
#   non-empty cell (Python-3-porting bug, an already-decoded str re-decoded a second time — fixed
#   at the source, qtimod.sip, as of this build). If you hit this exact TypeError on an older
#   build, use cellData(col,row) instead — a different, unaffected code path, same text result.
# IMPORTANT: t.cell(col,row) SILENTLY returns 0.0 when the cell's text can't be parsed as a number
#   — never raises, indistinguishable from a genuine 0.0. Confirmed live: reading cell(7, row) at
#   the WRONG row (e.g. landing on "Number of Parameters" -> "4" instead of chi^2/DoF's actual row)
#   silently produced chi2/dof=0.0 for TWO DIFFERENT models being compared — an identical,
#   implausibly perfect value that should have been an immediate red flag but wasn't. NEVER use
#   cell() on cols 6-10 of a fitCurve table (they're text, not numeric) — use text()/cellData()
#   and parse the string yourself so a parse failure is visible in what you tried to convert.

# SIMULATE CURVE TABLE — created by simulateCurve(); table name = "simulatedCurve-<model>"
#   SAME 10-column layout as the FIT CURVE TABLE above (x/y/weight/sigma/residues/Characteristics/
#   Conditions/Parameters/Values/Errors) — confirmed against source (fittable-simulate-simulate.cpp).
#     sc = table("simulatedCurve-" + model)
#     g.insertCurve(sc, "x", "y", Graph.CurveType.Line)
# Confirmed live, twice: (1) datasets() lists this as "simulatedCurve-<model>_y" — the "_y" is the
#   COLUMN-qualified dataset suffix (same tablename_colname convention as everywhere else in this
#   doc), NOT part of the table name — table("simulatedCurve-<model>_y") returns None. Strip the
#   "_y" before calling table(). (2) plot("simulatedCurve-<model>", xMin, xMax, style) is ALSO
#   wrong — graph-python-api.md's plot("formula", xMin, xMax) overload evaluates its string as a
#   MATH FORMULA, not a dataset lookup; this silently plots a nonsense function literally labeled
#   with that string, no exception raised. Use table()+insertCurve() above instead.

# Manual setup (configure preferred):
setDatasetCount(n)  setFunction(name)  selectDataset(m,name)  # m 1-indexed, call in order

# SANS instrument (callable at any stage):
setInstrumentalFit(bool)  instrumentalFit()->bool
setInstrument("SANS"|"Back-Scattering")  instrument()->str

# Fit range (m 1-indexed):
setRange(m,min,max,byIndex=False)  setRangeAll(min,max)

# Fit algorithm:
# DEFAULT: LM (Levenberg-Marquardt) — use for all smooth analytical functions (sphere, Guinier, cylinder, …).
#   LM converges in <20 iterations with reasonable starting values; do NOT use Simplex for these.
#   Simplex: only when derivatives are unreliable (tabulated data, step functions, heavy external calls).
#   GenMin: global search when starting values are completely unknown; slow, use as last resort.
# WRONG: f.setFitMethod("Simplex")  ← fails to converge for smooth SANS/optics models; chi² stays huge
# RIGHT: f.setFitMethod("LM")       ← or omit entirely (LM is the default)
setFitMethod(0/"Simplex"|1/"LM"|2/"GenMin")  fitMethod()->int
setMaxIterations(n)  maxIterations()->int  setTolerance(v)  setToleranceAbs(v)
setLmControl(0..3)  setLmAdaptStep(bool)  setLmDerivStep(v)
setSimplexVariant(0..2)  setSimplexConvRate(n)  setSimplexRestart(bool)
setGenMinPopulation(n)  setGenMinGenomeSize(n)  setGenMinGenerations(n)
setGenMinSelectionRate(v)  setGenMinMutationRate(v)  setGenMinRandomSeed(n)
setGenMinLocalSearch(0..2)

# Weighting (m is a 1-indexed dataset INDEX, int — NOT the "tablename_colname" dataset NAME
#   string used almost everywhere else in this API, easy to confuse given that convention:
#   setWeightingEnabled("Data_I",True) -> TypeError: argument 1 has unexpected type 'str'.
#   Passing 0 or any out-of-range m raises ValueError; used to silently no-op instead, e.g.
#   setWeightingEnabled(0,...) looked fine but never enabled weighting):
setWeightingMethod(0=1/σ²|1=1/Y|2=σ|3=1/Y²|4=1/Y^a|5=1/(c^a+b·Y^a)|6=1/(Y^a·c^|Xmax-X|))
setWeightingEnabled(m,bool)  setWeightingDataset(m,name)  # name MUST be "table_col", not a bare
#   column name ("dI") — raises ValueError now; used to silently no-op (weighting stayed unapplied).
# IMPORTANT: "table_col" alone is NOT enough — col MUST be the yErr-role column specifically, a
#   stricter subset than configure()'s dataset pool (which accepts X/Y columns too). A Y-column
#   name like "Sphere_I" is validly-formatted and STILL raises — same generic ValueError as a
#   nonexistent name, no hint it's a role mismatch. Confirmed live (Q/I/dI table, I=Y, dI=yErr):
#   WRONG: f.setWeightingDataset(1, "Sphere_I")    # I is Y-role — wrong pool for this call
#   RIGHT: f.setWeightingDataset(1, "Sphere_dI")   # dI is yErr-role — the only valid pool here
# IMPORTANT: newTable()'s columns default to Y for EVERY column except col 1 (defaults to X) —
#   there is NO default yErr. A column never given an explicit setColumnRole(col, yErr) call stays
#   Y-role silently — passes configure() (accepts X/Y) but ALWAYS fails setWeightingDataset(), same
#   generic "not available" error, no hint it's a role problem, not a name-format problem. Confirmed
#   live: newTable(...); setColNames([...]) with NO setColumnRole() calls at all left "dI" as
#   Y-role; 2 retries only re-checked the name string, never the column's actual role, before the
#   identical-error guard gave up. Fixable on the EXISTING table, no recreation needed, even from a
#   later part: t=table(name); t.setColumnRole(3, Table.PlotDesignation.yErr)
setWeightingA(v)  setWeightingB(v)  setWeightingC(v)  setWeightingXmax(v)
# WARNING: assigning Table.PlotDesignation.yErr to a column does NOT enable weighting by itself
# — you must ALSO call setWeightingEnabled(m, True) + setWeightingDataset(m, name). Without it,
# weighting silently defaults OFF (weight=1 for every point), no error, no warning. chi2/dof
# then reflects raw unnormalized residuals dominated by absolute data magnitude, not fit quality.
# Confirmed: data ~1e9 in magnitude gave chi2/dof ~1.5e14 unweighted vs ~1.8 weighted correctly
# — same fit, 13 orders of magnitude apart, silently. Always confirm weighting is enabled before
# comparing chi2/dof across models/datasets.

# GOOD chi2/dof DOES NOT MEAN fit() ACTUALLY CONVERGED — it may have just hit maxIterations().
# General check (works regardless of model): call fit() again right after (same config) — if
# chi2/dof barely changes, it had genuinely settled; if it drops further, the first run was cut
# off, not finished. Do NOT use "is the fitted center within the data's x-range" as a sanity
# check instead — a real peak center legitimately CAN sit outside the sampled range when the
# visible tail still constrains it with a tight error bar; that's not a sign of a bad fit.
#   chi1 = f.fit(); chi2 = f.fit()
#   if abs(chi2 - chi1) > 1e-6 * max(abs(chi1), 1e-300): scriptPrint("may not have converged")

# f.lastChi2 / f.results() are the CURRENT fit state, not a history — a second fit() overwrites
# them in place. For before/after comparisons, capture "before" into local vars BEFORE calling
# fit() again; reading lastChi2/results() again afterward silently returns the SAME new values
# under an "old" label, no error, no warning that the comparison never happened.
#   WRONG: f.fit(); new=f.results(); old=f.lastChi2  # "old" is already the new value
#   RIGHT: old=f.lastChi2; f.fit(); new=f.lastChi2   # capture old BEFORE the second fit()
# configure() resets every parameter to the function's initial values — results() after a new
#   configure() (without fit()) returns the START values, not the last fit's. Comparing two models
#   in one script: read each model's results() right after its own fit():
#   chi_g = f.fit(); A_g, _ = f.results()["A"]   # then configure the 2nd model, fit, read, compare

# IMPORTANT: resolution smearing (below) and polydispersity (further below) are OPTIONAL
#   refinements — add them ONLY when the request explicitly asks for resolution/instrumental
#   smearing or polydispersity, never as a default "more thorough SANS fit" embellishment. Same
#   category as unrequested axis/tick styling. Confirmed live: a plain "fit with weighted errors"
#   request grew a full setInstrumentalFit/setResoFunction/setResoDataset setup PLUS
#   setPolyFunction/setPolyEnabled/setPolyDataset(1,"") (an empty, meaningless sigma column) on
#   top of the actual fit — none of it requested, all of it unreviewed extra failure surface.
# Resolution (post-function, m 1-indexed):
setResoFunction(0/Gauss-SANS|1/Triangular|2/Bessel-SANS|3/Gauss|4/InFunction)
setResoSpeedControl(0..7)  setResoWorkspace(n)  setResoSigmaLimit(n)
setResoEnabled(m,bool)  setResoDataset(m,name)  resoEnabled(m)->bool  resoDataset(m)->str
# setResoDataset/setPolyDataset raise ValueError for the SAME reasons as setWeightingDataset above
# (dataset index out of range; name not a fully-qualified "table_col" match) — used to silently
# no-op instead. setResoDataset's combo only populates once setInstrumentalFit(True) is set.

# Polydispersity (post-function, m 1-indexed):
setPolyFunction(0/Gauss|1/Schultz-Zimm|2/Gamma|3/Log-Normal|4/Uniform|5/Triangular)
setPolySpeedControl(0..7)  setPolyWorkspace(n)  setPolySigmaLimit(n)
setPolyEnabled(m,bool)  setPolyDataset(m,name)  polyEnabled(m)->bool  polyDataset(m)->str
# setPolyDataset's combo only populates once setPolyEnabled(m, True) is set for that dataset first.

# Parameters (dataset=-1→all datasets; m 1-indexed):
# IMPORTANT: parameter names must be EXACT matches to what paramNames() returns — case-sensitive, no aliases.
#   Wrong: setParamValue("R", ...) / setParamValue("SLD", ...) / setParamValue("SLDsolv", ...)
#   Right: use names from paramNames() — e.g. "radius", "sld", "sld_solvent"
#   Unknown names raise ValueError: no such parameter — always verify names with paramNames() in EXPLORE first.
# IMPORTANT: not all parameters are adjustable — the model defines which ones can be fitted.
#   After configure(), paramVaries(name) reflects the model's built-in adjustability:
#   paramVaries returns False for parameters the model marks as non-adjustable (e.g. SLD1/SLD2
#   in some SANS-fit functions where only scale is adjustable).
#   ALWAYS call paramVaries(name) after configure() to discover which params are actually fittable.
#   Calling setParamVaries("sld1", True) on a non-adjustable parameter has no effect.
#   For a CUSTOM function: "adjustability" is just whatever c.setParamVariesAll() set at compile time
#   (setParamCount(n) defaults every param to fixed) — NOT a modeling restriction unless you made it one.
#   configure() re-derives vary/fixed from that compiled default on EVERY call, never inheriting a
#   previous session's setParamVaries() override. Skip setParamVariesAll() at compile time AND after
#   configure() -> fit() raises RuntimeError: "no adjustable parameters" (confirmed live).
paramValue(name,dataset=1)->float  paramError(name)->float
paramVaries(name)->bool  paramShared(name)->bool
paramLimitLeft(name,dataset=-1)->float  paramLimitRight(name,dataset=-1)->float
paramBayesian(name)->bool
setParamValue(name,value,dataset=-1)     # f.setParamValue("Rg", 50.0)
setParamVaries(name,varies,dataset=-1)   setParamShared(name,shared)
setParamLimits(name,lo,hi,bayesianYN=False,dataset=-1)
# lo/hi are floats — for unbounded/one-sided use float('-inf')/float('inf'), NOT a bracket string
# like ".." (that's Compiler.setParamLimit()'s syntax, a different, unrelated API).
#   RIGHT: f.setParamLimits("Rg", float('-inf'), float('inf'))
# paramLimitLeft/Right return float('-inf')/float('inf') for an unbounded side — compare with
# == float('-inf'), not None or "".
setParamValueAll(values,dataset=-1)  setParamVariesAll(varies,dataset=-1)

# Simulate Curve tab. ORDER: configure()/setFunction() FIRST, then selectStage("simulate"), then
#   these setters, then simulateCurve(). selectStage() before a function is selected raises.
#   RIGHT: f=Fittable; f.configure("Guinier",[]); f.selectStage("simulate"); f.setSimUniformX(True)
#   WRONG: f.selectStage("simulate"); f.setFunction("Guinier")   <- raises, blocked before running
# In uniform X mode simulateCurve() returns NaN BY DEFINITION (no data to compare) — do NOT report
#   it as a result; report the output table "simulatedCurve-<model>" instead.
setSimUniformX(bool)  setSimFromX(v)  setSimToX(v)  setSimNumPoints(n)  setSimLogStep(bool)
setSimDataset(name)  simDataset()->str
simUniformX()->bool  simFromX()->float  simToX()->float  simNumPoints()->int  simLogStep()->bool
# setCurveColor: MUST be called AFTER configure() and BEFORE fit()/plotResults() — NOT after fit()
#   WRONG: chi2 = f.fit(); f.setCurveColor("red")   ← has no effect on the fit curve
#   RIGHT: f.configure(...); f.setCurveColor("red"); chi2 = f.fit()
curveColor()->int  setCurveColor(index_or_name)->bool  # f.setCurveColor("blue") or f.setCurveColor(3)
# plotResults() layer-1 curve order per dataset: index 0 = DATA scatter, index 1 = FIT line.
#   WRONG: g.setCurveLineColor(0, colorIndex("orange"))  ← recolors DATA points, not the fit line
#   RIGHT: f.setCurveColor("orange") BEFORE fit()/plotResults() (see above)
# setCurveColor()/curveColor() are the ONLY curve-styling methods on Fittable — no setCurveLineWidth(),
# no symbol setters. For those, use g = f.plotResults(...).activeLayer() and call Graph methods on
# curve index 1: g.setCurveLineWidth(1, 2). f.setCurveLineWidth(2) raises AttributeError (confirmed live).
# IMPORTANT: the fit curve's g.curveTitle(i) is NOT the bare model name — it's
#   "fitCurve-"+function+"_y" (the fit-curve table's name + its "_y" column, same tablename_colname
#   convention as every curve title). Checking g.curveTitle(i)==model to verify the fit curve exists
#   NEVER matches — false "not found", no error, even though the curve is genuinely there. Confirmed
#   live: exactly this check silently skipped an entire legend/styling part ("Fit curve missing from
#   graph" printed, nothing else, right after plotResults() had just succeeded).
#   WRONG: if any(g.curveTitle(i)==model for i in range(g.numCurves())): ...   # never true
#   RIGHT: if g.numCurves() >= 2: ...   # trust the already-documented index convention instead
