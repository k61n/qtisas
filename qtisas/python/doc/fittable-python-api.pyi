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
# NOTE: true/false (lowercase) are aliases for True/False in QtiSAS Python scripts.
# Quick example:
#   f.configure("Guinier", datasets=["data_I"])
#   f.setParamValue("Rg", 50.0)
#   f.setParamLimits("Rg", 10, 200)
#   chi2 = f.fit()
#   f.plotResults()
#   print(f.results())   # {"Rg": (52.3, 0.4), "I0": (0.01, 0.001)}

configure(fn,datasets=[])  fit()->chi2  simulate()->chi2  simulateCurve()->chi2
beforeFit()  afterFit()  plotResults(graphName="",graphTemplate="")
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

# Weighting (m 1-indexed):
setWeightingMethod(0=1/σ²|1=1/Y|2=σ|3=1/Y²|4=1/Y^a|5=1/(c^a+b·Y^a)|6=1/(Y^a·c^(Xmax-X)))
setWeightingEnabled(m,bool)  setWeightingDataset(m,name)
setWeightingA(v)  setWeightingB(v)  setWeightingC(v)  setWeightingXmax(v)

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
setParamValueAll(values,dataset=-1)  setParamVariesAll(varies,dataset=-1)

# Simulate Curve tab (selectStage("simulate") first):
setSimUniformX(bool)  setSimFromX(v)  setSimToX(v)  setSimNumPoints(n)  setSimLogStep(bool)
setSimDataset(name)  simDataset()->str
simUniformX()->bool  simFromX()->float  simToX()->float  simNumPoints()->int  simLogStep()->bool
# setCurveColor: MUST be called AFTER configure() and BEFORE fit()/plotResults() — NOT after fit()
#   WRONG: chi2 = f.fit(); f.setCurveColor("red")   ← has no effect on the fit curve
#   RIGHT: f.configure(...); f.setCurveColor("red"); chi2 = f.fit()
curveColor()->int  setCurveColor(index_or_name)->bool  # f.setCurveColor("blue") or f.setCurveColor(3)
