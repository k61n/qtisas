# QtiSAS Fittable — f = Fittable (singleton)
# Call order: pre-function → configure → post-function settings → fit/simulate/plotResults
# IMPORTANT: configure() selects a function BY NAME — do NOT pass a formula string.
#   WRONG: f.configure("a*x+b", ...)   RIGHT: f.configure("Guinier", ["data_I"])
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
results()->{n:(v,e)} or {n:[(v,e),...]}  fitReport()->str
r2()->float  qFactor()->float  lastChi2  lastDoF  stage()->str  selectStage(s)->bool  state()->dict
fitFunctions(pattern="")->list  currentFunction()->str
datasets()->list  selectedDatasets()->list  selectedDataset(m)->str  datasetCount()->int
paramNames()->list  paramCount()->int  Fittable.lastFitReport
setScaleErrors(bool)  setStatisticsNotes(bool)  setSaveSession(bool)

# Manual setup (configure preferred):
setDatasetCount(n)  setFunction(name)  selectDataset(m,name)  # m 1-indexed, call in order

# SANS instrument (callable at any stage):
setInstrumentalFit(bool)  instrumentalFit()->bool
setInstrument("SANS"|"Back-Scattering")  instrument()->str

# Fit range (m 1-indexed):
setRange(m,min,max,byIndex=False)  setRangeAll(min,max)

# Fit algorithm:
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
# setCurveColor: call AFTER configure(), BEFORE fit()/plotResults()
curveColor()->int  setCurveColor(index_or_name)->bool  # f.setCurveColor("blue") or f.setCurveColor(3)
