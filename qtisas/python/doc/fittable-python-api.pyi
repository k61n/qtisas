# QtiSAS Fittable — f = Fittable (singleton)
# Call order: pre-function → configure → post-function settings → fit/simulate/plotResults
# IMPORTANT: configure() selects a function BY NAME — do NOT pass a formula string.
#   WRONG: f.configure("a*x+b", ...)   RIGHT: f.configure("Guinier", ["data_I"])
# IMPORTANT: NEVER hardcode a function name when using an EXISTING library function.
#   Function names depend on the compiled library and may differ between installations.
#   Exception: if the script itself compiled the function (via Compiler), the name is already known — no discovery needed.
#   Discovery pattern (REQUIRED when using a pre-existing model):
#     hits = f.fitFunctions("*sphere*SANS*")  # most specific first
#     if not hits: hits = f.fitFunctions("*sphere*")
#     if not hits:
#         print("No sphere model found. SANS models:"); print(f.fitFunctions("*SANS*"))
#         raise RuntimeError("No suitable model found in Fittable library")
#     model = hits[0]
#     print(f"Model: {model}")
#     if len(hits) > 1: print(f"Also available: {hits[1:]}")
#     f.configure(model, [...])  # use variable, not a hardcoded string
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

configure(fn,datasets=[])  fit()->chi2  simulate()->chi2  simulateCurve()->chi2
# fit() raises RuntimeError if the selected dataset(s) can't be read (e.g. an empty table) —
#   previously returned a stale chi2 from a prior fit with no sign anything failed.
beforeFit()  afterFit()  plotResults(graphName="",graphTemplate="")->MultiLayer
# plotResults returns the MultiLayer it plotted into — use it to style the graph:
#   gw = f.plotResults("FitResults"); g = gw.activeLayer(); g.setXTitle("q, 1/nm")
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
r2()->float  qFactor()->float  lastChi2  lastDoF  stage()->str  selectStage(s)->bool  state()->dict
fitFunctions(pattern="")->list  currentFunction()->str
datasets()->list  selectedDatasets()->list  selectedDataset(m)->str  datasetCount()->int
# datasets() is READ-ONLY (lists what's configured) — NOT a setter. datasets(["x"]) raises
#   TypeError: too many arguments; a bare datasets() afterward compiles but sets nothing, still 0
#   datasets configured -> fit() raises "failed to read data for the selected dataset(s)".
#   To set datasets: configure(fn, [...]) again with the full list, or setDatasetCount+selectDataset.
paramNames()->list  paramCount()->int  Fittable.lastFitReport
setScaleErrors(bool)  setStatisticsNotes(bool)  setSaveSession(bool)

# FIT CURVE TABLE — created by fit() and simulate()
# Single dataset: table name = "fitCurve-<model>"   Global fit N: "fitCurve-<model>-global-N"
# Columns (1-based index for Python cell/addCurve access):
#   col 1  "x"              role X     — Q values (same grid as input data)
#   col 2  "y"              role Y     — fitted I(Q)
#   col 3  "weight"         role yErr  — fit weight per point
#   col 4  "sigma"          role xErr  — resolution σ(Q)
#   col 5  "residues"       role Y     — normalised residuals (data−fit)/σ  ← USE THIS for residuals panel
#   col 6+ metadata columns (Characteristics, Conditions, Parameters, Values, Errors) role None
# Access from Python — ALWAYS guard: table returns None if fit failed or was not yet run:
#   fc = table("fitCurve-" + model)
#   if fc is None: raise RuntimeError("fitCurve table missing — did the fit succeed?")
#   g.insertCurve(fc, "x", "y", Graph.CurveType.Line)           # fit line
#   gr.insertCurve(fc, "x", "residues", Graph.CurveType.Scatter) # residuals panel
#   i_fit = fc.cell(2, idx)   # fit value at row idx
#   resid = fc.cell(5, idx)   # residual at row idx

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
setWeightingMethod(0=1/σ²|1=1/Y|2=σ|3=1/Y²|4=1/Y^a|5=1/(c^a+b·Y^a)|6=1/(Y^a·c^(Xmax-X)))
setWeightingEnabled(m,bool)  setWeightingDataset(m,name)
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

# Resolution (post-function, m 1-indexed):
setResoFunction(0/Gauss-SANS|1/Triangular|2/Bessel-SANS|3/Gauss|4/InFunction)
setResoSpeedControl(0..7)  setResoWorkspace(n)  setResoSigmaLimit(n)
setResoEnabled(m,bool)  setResoDataset(m,name)  resoEnabled(m)->bool  resoDataset(m)->str

# Polydispersity (post-function, m 1-indexed):
setPolyFunction(0/Gauss|1/Schultz-Zimm|2/Gamma|3/Log-Normal|4/Uniform|5/Triangular)
setPolySpeedControl(0..7)  setPolyWorkspace(n)  setPolySigmaLimit(n)
setPolyEnabled(m,bool)  setPolyDataset(m,name)  polyEnabled(m)->bool  polyDataset(m)->str

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

# Simulate Curve tab (selectStage("simulate") first):
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
