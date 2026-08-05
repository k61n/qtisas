# AI WORKFLOW RULES — read completely before writing any code:
#
# RULE 1 — name resolution:
#   Exact name given → c.open(name) directly; do NOT call functionNames(). After opening named
#   refs, do NOT call functionNames()/c.open() again, do NOT search for "more examples" — proceed
#   IMMEDIATELY to new()/setCode()/compile() in the SAME script. Never stop after printing.
#   Name unknown → filter c.functionNames("ALL") (no wildcards); stop and report if not found.
#   Category known → filter c.categories() first; functionNames() needs the EXACT category name.
#     c.open(refs[0]); ref_code = c.code(); ref_params = c.paramNames()
#
# RULE 2 — save() vs compile(): compile() auto-saves the .fif — NEVER call c.save() before
#   c.compile() (redundant). setParamInitsAll/setParamVariesAll require floats/bools, not strings;
#   setParamLimitsAll is the opposite — strings only ("0..","0..100"), raw numbers → SystemError.
#     WRONG: c.save("Gauss"); c.compile()      RIGHT: c.compile()
#   Use c.save() ALONE only for metadata/param edits that don't need recompiling. If the request
#   says "not save": call setInfo/setCode/etc. but do NOT call save/compile.
#
# RULE 3 — MODIFY-AND-SAVE-AS-NEW: setFunctionName() MUST be called before save()/compile(),
#   even if you also pass a name to save() — save()'s name arg only sets the FILE PATH; the
#   FIF's internal [name] field (and compiled name() symbol) always come from setFunctionName().
#   compile() takes NO name argument at all — it always uses whatever setFunctionName()/open()
#   last set, so skipping this SILENTLY OVERWRITES THE ORIGINAL. This has happened: opening
#   sasviewmodels/ellipsoid, fixing its code, calling c.compile() without setFunctionName()
#   destroyed the original and never created ellipsoid-correct.
#     WRONG: c.open("sasviewmodels/ellipsoid"); c.setCode(fixed); c.compile()
#            ← no name set anywhere; overwrites "ellipsoid" in place
#     WRONG: c.open(...); c.setCode(fixed); c.save("sasviewmodels/ellipsoid-correct")
#            ← file path is right but [name] still says "ellipsoid"
#     RIGHT: c.open("sasviewmodels/ellipsoid"); c.setFunctionName("sasviewmodels/ellipsoid-correct")
#            c.setCode(fixed); c.compile()   # or c.save()
#   Also: the name itself must not contain '(' or ')' — c.save()/c.compile() raise ValueError.
#     WRONG: c.setFunctionName("Cylinder(2)")   RIGHT: c.setFunctionName("Cylinder_v2")
#
# RULE 4 — namespace mixing (combining two sasview models, e.g. P(Q) x S(Q); in [code] use
#   param names directly — never P1/P2/P3; call the reference C functions, do not reimplement):
#   NEVER include two sasview headers bare — kernel_header.c gets included twice → redefinition
#   errors. Wrap each in its own namespace via the single-line syntax in setHHeaders():
#     c.setHHeaders('namespace ff {#include "sasviewmodels/sphere.h"}\n'
#                   'namespace hs {#include "sasviewmodels/hardsphere.h"}')
#   Call with qualified names in [code]; ff::Fq() is void — writes into F1/F2 via pointers:
#     double F1, F2; ff::Fq(q, &F1, &F2, SLD, SLDsolv, R);
#     double PQ = F2 / ff::form_volume(R);  double SQ = hs::Iq(q, R, volfraction);
#     I = scale * PQ * SQ + background;
#   Include ONLY what [code] actually calls — do not copy hHeaders() from a reference wholesale.
#   Inline math (sin, cos, pow, M_PI) needs no extra headers.
#     WRONG: both ff and hs namespaces when [code] only calls hs::Iq() (sphere P(Q) inline)
#     RIGHT: 'namespace hs {#include "sasviewmodels/hardsphere.h"}'   ← only what's called
#   ARG-COUNT (bidirectional):
#     (A) copy the Iq()/Fq() call EXACTLY from the reference code() — rename params, never drop:
#           code() shows Iq(q, radius_effective, volfraction, perturb, stickiness)  ← 5 args
#           WRONG: hs::Iq(q, radius, volfraction, stickiness)          ← missing perturb
#           RIGHT: hs::Iq(q, radius, volfraction, perturb, stickiness)
#     (B) every arg passed must be declared in setParamNamesAll() or as a local variable, or
#         you get "undeclared identifier":
#           WRONG: setParamNamesAll([..., "concentration_salt"]); hm::Iq(q, ..., concentration_salt, dielectconst)
#           RIGHT: setParamNamesAll([..., "concentration_salt", "dielectconst"])
#
# RULE 5 — X-RANGE: x range must cover the function's full dynamic range to constrain all params.
#   Peaked (Gaussian, Lorentzian): x_min < centre-3*width, x_max > centre+3*width
#   Decay (exp(-b*x)+c): x_max >= 3/b (need 3+ decay lengths to separate amp from background)
#   Power-law/plateau: cover the full transition from steep slope to flat region.
#   Feature OUTSIDE the data range → parameters blow up with huge correlated errors.
#
# RULE 6 — Int1/Int2/Int3 + currentInt: cache up to 3 Q-INDEPENDENT quantities ONCE per
#   iteration (via beforeIter), instead of recomputing per Q point / per polydispersity node.
#   currentInt is ONLY EVER 1/2/3 during that one-time setup call — normal evaluation has
#   currentInt==0, so "if currentInt==N: return ..." is skipped there, falling through to the
#   cached IntN. With polydispersity OFF, setup is one direct evaluation. With polydispersity
#   ON, the engine runs the FULL polyIntegral() for that setup call instead — calling [code]
#   once per size-distribution node and weight-averaging the result — so IntN becomes the TRUE
#   distribution average of whatever you return, not a single-point value:
#     WRONG: I = F2; I /= form_volume(radius)   ← recomputed per node → <F^2/V(r)>, not <F^2>/<V>
#     RIGHT: if currentInt==1: return form_volume(radius)   # Int1 = <V>, polydispersity-averaged
#            meanVolume = Int1; I = F2; I /= meanVolume
#   Generalizes to NONLINEAR expressions too (Int2 = <V^2>, not V_mean^2 — same per-node averaging):
#     RIGHT: if currentInt==2: V=form_volume(radius); return V*V
#            meanV2 = Int2
#   GSL is already linked (gsl_sf_bessel_J1/J0/etc.) — never reimplement special functions;
#   copy integration patterns (sin(alpha) weight, trapezoidal rule) from existing code.
#   Place every "if currentInt==N: return ..." check at the very TOP of [code] — these branches
#   return early, so anything computed above them is wasted work on every beforeIter call.
#
# CRITICAL: every action script must start with c = Compiler — variables from an EXPLORE
#   script do NOT carry over; each execution is independent.
#
# RULE 7 — XNAME: setXName("q") → the C++ local IS named "q"; do NOT write "double q = x;".
#   setXName("x") → local IS "x"; write "double q = x;" only to rename it.
#     WRONG: setXName("q") + code "double q = x;"   → error: 'x' undeclared
#
# RULE 8 — PARAMCOUNT: setParamNamesAll(names) requires len(names) == setParamCount(n) EXACTLY —
#   raises ValueError otherwise (used to silently drop/misalign entries; now fails loudly).
#     RIGHT: names=[...]; c.setParamCount(len(names)); c.setParamNamesAll(names)
#
# RULE 9 — NO C++ KEYWORDS as parameter names (switch, class, new, delete, template, if, for,
#   return, int, void, true, const, ...) — setParamName()/setParamNamesAll() raise ValueError.
#   Before this check existed: naming a param "switch" compiled with the WRONG error blamed on a
#   later, unrelated, perfectly valid parameter — the bad declaration corrupts everything after it.
#
# RULE 10 — setCode() is C++, not Python: NO ** operator (exp(-(x-c)**2/...) fails with
#   "indirection requires pointer operand", a cryptic error not mentioning ** at all).
#   Use pow(a,b) or a*a instead.
# RULE 11 — reverse of RULE 10: C++'s "cond ? a : b" ternary leaking into surrounding PYTHON code
#   (happens when a script defines setCode() AND separately duplicates the same formula in Python,
#   e.g. for synthetic test data). Python has no ?: — SyntaxError, confirmed live:
#   WRONG: F = (qR>1e-6) ? 3.0*(sin(qR)-qR*cos(qR))/(qR**3) : 1.0   # SyntaxError: invalid syntax
#   RIGHT: F = 3.0*(sin(qR)-qR*cos(qR))/(qR**3) if qR>1e-6 else 1.0
#   The C++ side inside setCode() is unaffected — ?: is correct there, only wrong around it.
# QtiSAS Compiler — c = Compiler (singleton)
# Manages the fit-function library: .fif source files and compiled shared libraries.
# Default library path (set automatically at startup — setPath() NOT needed in normal use):
#   Linux/macOS: ~/.config/qtisas/FitFunctions/
#   Windows:     %USERPROFILE%\AppData\Local\qtisas\FitFunctions\
# NOTE: true/false (lowercase) are aliases for True/False in QtiSAS Python scripts.
# Quick example:
#   c.scanGroups()
#   c.selectCategory("shape:sphere")
#   c.compileAll()
#   # or for a sub-folder:
#   c.selectCategory("qtiplot/")
#   c.compileAll()

# Path (override only for non-standard locations):
path  setPath(path="")   # setPath() opens file dialog

# State (read after open()):
functionName()->str  setFunctionName(name)
yName()->str        # [y] dependent variable name (e.g. "I(Q)")
setYName(name)      # set [y] field; updates code header comment
xName()->str        # [x] independent variable name(s) (e.g. "x" or "ix,iy")
setXName(name)      # set [x] field; updates code header comment
info()->str  setInfo(text)  # [description] HTML text (rich-text editor content)
paramCount()->int   # total number of fit parameters; use setParamCount(n) to change
code()->str         setCode(text)              # [code] section: use yName() variable (not always "I"); do NOT end with return {yName}; — boilerplate provides it
# STOP after reading 1-2 references — proceed to new()/setCode()/compile(); never loop c.open() over all functions
hHeaders()->str     setHHeaders(text)          # [h-headers]: extra #include lines; add GSL sub-headers here (e.g. #include <gsl/gsl_sf_bessel.h> for gsl_sf_bessel_J1)
                                               # namespace mixing: 'namespace ff {#include "sasviewmodels/sphere.h"}' — parser expands to proper block
includedFunctions()->str  setIncludedFunctions(text)  # [included functions]: helper C++ above functionSANS
# Data-driven initial guesses: define beforeFitting(void*ParaM){ if(!beforeFit) return; A=...; }
#   here, call it from code(). beforeFit is set by f.beforeFit() / Fittable's "Before Fit" button.
#   XXX/YYY/currentFirstPoint/currentLastPoint index the loaded data. See linear-efit.fif.
# Parameter table (tableParaNames):
paramNames()->list                      # all parameter names
paramName(i)->str     setParamName(i, name)
setParamNamesAll(names)                 # bulk set names
paramInit(i)->float   setParamInit(i, value)
setParamInitsAll(values)                # bulk set initial values
paramLimit(i)->str    setParamLimit(i, limit)   # ".." / "a..b" / "a.." (lower only) / "..b" (upper only) / "mean+-sigma" / "-inf"/"inf" text accepted for either side
setParamLimitsAll(limits)
# setParamLimitsAll wants STRINGS ("0..", "0..100", ".."), opposite of setParamInitsAll/
#   setParamVariesAll (floats/bools) right next to it — raw numbers raise SystemError.
#   WRONG: setParamLimitsAll([0.0, 0.0])   RIGHT: setParamLimitsAll(["0..", "0.."])
paramVaries(i)->bool  setParamVaries(i, varies) # adjustability checkbox
setParamVariesAll(varies)
paramDescr(i)->str    setParamDescr(i, text)
setParamDescrsAll(descrs)
# descriptions join with "," in the FIF with no escaping — a literal "," inside one description
# desyncs the count and pops a blocking "Error: [parameter description]" dialog (or silently
# shifts later descriptions by one). Use ";" instead, e.g. "shape (0=Gauss; 1=Lorentz)".
group()->str        # [group] field of current FIF file (saved in .fif)
setGroup(name)      # set group field before save() — plain name only, NEVER empty, NEVER contains "/"
                    # raises ValueError if empty or contains '/'
                    # WRONG: c.setGroup("")  c.setGroup("qtiplot/")  c.setGroup("custom/sub")
                    # RIGHT: c.setGroup("custom")  c.setGroup("shape:sphere")
groups()->list      # all group names from .fif files (no "ALL", no sub-folders)

# Explorer (call scanGroups() first to refresh):
scanGroups()
category()->str                       # currently selected row in category list
categories()->list                    # includes "ALL" and sub-folders
selectCategory(categoryName)->bool    # selects category, populates function list
functions(groupName)->list            # selectCategory(categoryName) + return function list

# FIF management:
open(fifName)  # fifName may include subfolder: "sasviewmodels/barbell" opens sasviewmodels/barbell.fif
new()
save(name="", path="", askYN=False)->str  # returns saved FIF text; save(); save("cylinder"); save("subfolder/cylinder"); c.path is a string attribute NOT a method
# compile() auto-saves — c.save() before c.compile() is NOT needed.
# Use c.save() only when persisting changes WITHOUT recompiling (metadata/param edits).
#   WRONG: c.setGroup("")           # raises ValueError — group must not be empty
#   WRONG: c.setGroup("qtiplot/")   # raises ValueError — group must not contain '/'
# Subfolder storage goes in save(), not setGroup():
#   c.save("qtiplot/MyGauss")  → saved to pathFIF/qtiplot/MyGauss.fif; f.configure("qtiplot/MyGauss")
delete(name="", path="")  # delete(); delete("cylinder"); delete("sasviewmodels/cylinder"); delete("cylinder",path="/p")
download(repoUrl="https://iffgit.fz-juelich.de/qtisas/fitfunctions.git", silent=True)

# Code generation:
makeCPP()->str      # saves .cpp and returns generated source
makeScript()->str   # saves compile script and returns script text

# Build:
compile(compileAll=False)->bool  compileAll()  # True=ok, False=error; errors in resultsLog() — that is qti.app.resultsLog(), NOT c.resultsLog()
# CRITICAL — compile() returns bool; ALWAYS check it. Print does NOT mean success.
#   WRONG: c.compile(); print("compiled!")         # prints even if compile failed
#   RIGHT: ok = c.compile()
#          if not ok:
#              scriptPrint(qti.app.resultsLog().toPlainText()[-500:])
#              raise RuntimeError("Compilation failed")
#          scriptPrint("Compiled OK")

# SasView import:
openSasView()   # requires: pip install sasmodels
# PySAS models: hHeaders has '#include "IncludedFunctions/PySAS.h"'; code calls PySAS::Iq("name", q, ...)
#   These require Python.h at compile time ([python] 1 in FIF).
#   compile() fails with "Python.h not found" if dev headers are missing.
#   Do NOT use subprocess.run([sys.executable, sysconfig...]) — embedded Python has no dev headers.
#   Find system Python.h instead: /opt/homebrew/include/python3.x/Python.h (macOS/Homebrew)
#   Alternative: rewrite the function inline in C++ without the PySAS call.

# Parameters:
setParamCount(n)

# Fit.Control — x-range (Fit.Control tab, simulation range):
setXRange(uniform, xMin, xMax, nPoints, logStep, yMin=0.0)  # single call to enable custom range + set all values
customXrange()->bool   setCustomXrange(bool)
xMin()->float          setXMin(float)
xMax()->float          setXMax(float)
xPoints()->int         setXPoints(int)
yMin()->float          setYMin(float)
logStep()->bool        setLogStep(bool)

# Fit.Control — fit options:
eFitEnabled()->bool    setEFitEnabled(bool)
eFit()->str            setEFit(str)            # keywords: no-before no-fit no-after yes-simulate color=NAME name=TABLE no-reslog yes-res-in-plot
weight()->bool         setWeight(bool)
weightMethod()->int    setWeightMethod(int)    # 0=Instrumental(1/σ²) 1=Statistical(1/|Y|) 2=Direct(σ) 3=Y² 4=Yᵃ 5=cᵃ+bYᵃ 6=Yᵃ∘c^|Xmax-X|
superpositional()->bool  setSuperpositional(bool)
subFitCount()->int     setSubFitCount(int)
useAlgorithm()->bool   setUseAlgorithm(bool)
fitMethod()->int       setFitMethod(int)       # 0=Nelder-Mead-Simplex 1=Levenberg-Marquardt(default) 2=GenMin
fitMethodPara()->str   setFitMethodPara(str)   # SD=Significant Digits; Simplex: SD=.. MODE=.. ITERS=.. REL=.. CONVRATE=.. STAGNATION=..
                                               # Levenberg: SD=.. MODE=.. ITERS=.. REL=.. ABS=.. DER=.. DSSV=..
                                               # Genetic: SD=.. GENCOUNT=.. GENSIZE=.. ITERS=.. SELRATE=.. MUTRATE=.. SEED=.. MODE=..

# Logging:
log(text)
