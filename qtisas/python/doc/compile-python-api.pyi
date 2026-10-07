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
#   setParamLimitsAll is the opposite — strings only ("0..","0..100"), raw numbers → TypeError.
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
#            c.setCode(fixed); c.compile()
#   USE compile(), NOT save(), WHEN THE NEW NAME MUST BE FITTABLE — save() (SIP: saveFIF()) only
#   writes the .fif SOURCE TEXT; it never builds a shared library. compile() (SIP:
#   buildSharedLibrary()) is the ONLY call that produces the compiled .dylib Fittable.configure()
#   actually loads. Confirmed live: c.open("SphereSANS_Custom"); c.setGroup("custom");
#   c.save("custom_sphere_fit") (no setFunctionName(), and no compile() at all) was then passed to
#   f.configure("custom_sphere_fit", ...) → ValueError: function not found — "custom_sphere_fit"
#   never became a real, loadable function under any name. save() alone is ONLY for persisting
#   metadata/param edits on the function ALREADY loaded under its EXISTING compiled name (see
#   RULE 2) — it can never make a NEW name fittable, with or without setFunctionName() first.
#   Also: the name itself must not contain '(' or ')' — c.save()/c.compile() raise ValueError.
#     WRONG: c.setFunctionName("Cylinder(2)")   RIGHT: c.setFunctionName("Cylinder_v2")
#   Also: a name differing from an EXISTING function only in letter case ALSO raises ValueError
#   (case-insensitive filesystem: mismatched .fif/.dylib pair, vanishes from fitFunctions() under
#   either name — confirmed live). Pick a name not already in functionNames("ALL") in any case.
#   EXCEPTION — inside "ai/" specifically, for a COMPILE-A-NEW-FUNCTION task (not "use/modify
#   existing X"): the collision is a disposable scratch slot, not a function to fall back to.
#   c.delete(collidingName) it, then compile fresh under the ORIGINALLY-intended name — do NOT
#   silently adopt the colliding function's old code/params as if it satisfied the current task.
#   Confirmed live: collision "ai/SphereSANS" vs existing "ai/sphereSANS" was "resolved" by
#   opening the old one and using its old 4-param set instead of the intended new 5-param model —
#   every later part then silently built on unrelated, unverified old code.
#     WRONG: hits=[...matching collision...]; c.open(hits[0])   # uses OLD code, never the new one
#     RIGHT: hits=[...matching collision...]; c.delete(hits[0]); c.new(); c.setFunctionName(...)
#   Does NOT apply outside "ai/", nor when the request names a SPECIFIC EXISTING function to use
#   or modify — that's RULE 1/RULE 3's open()-and-modify pattern; the collision there IS the target.
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
#   Oscillatory/periodic (SANS sphere/cylinder form factors, sinc-like sin(qR)/qR terms): TOO WIDE
#   a Q-range is its own separate failure — unlike the "not wide enough" cases above, a range with
#   MANY oscillation periods (qR_max / (2*pi) more than ~5) makes a gradient-based fit (LM) fail via
#   "cycle-skipping": if the initial R guess is off by even ~10-20%, the fitted curve's oscillations
#   drift out of phase with the data after a few cycles, and the optimizer has no way to tell which
#   cycle it should be matching — it converges to a nearby WRONG local minimum instead of the true
#   one, with badly elevated chi2/dof and NO error raised (the fit "succeeds", just at a bad point).
#   Confirmed live: R=50 (Angstroms) sphere, Q from 0.01 to 4.48 (same units) -> qR up to 224, ~35
#   oscillation periods; starting the fit from R=40 (a modest, plausible-looking 20% guess) gave
#   chi2/dof ~ 8.5e5 and R^2 ~ 0.06 in 18 iterations — a converged-looking but badly wrong result.
#   Keep qR_max roughly under 20-30 (a handful of periods) unless the task explicitly asks for a
#   wider range, and prefer a closer initial guess for R specifically when the range is wide.
#
# RULE 6 — Int1/Int2/Int3 + currentInt: cache up to 3 Q-INDEPENDENT quantities ONCE per
#   iteration (via beforeIter — NOT the same hook as beforeFitting() below, a different mechanism
#   for initial-guess computation), instead of recomputing per Q point / per polydispersity node.
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
# RULE 7 — X/YNAME: whatever setXName()/setYName() are set to are ALREADY-DECLARED C++ locals
#   supplied by the boilerplate BEFORE [code] runs — [code] must use those EXACT names directly,
#   never a generic default ("x"/"q"/"y"/"I") and never re-declare them with "double <name> = ...".
#   After c.new() they are x = "x" and y = "f" (NOT "y"): without setYName(), assign "f = ...;".
#   setXName("q") → the C++ local IS named "q"; do NOT write "double q = x;".
#     WRONG: setXName("q") + code "double q = x;"   → error: 'x' undeclared
#   SAME for setYName: the dependent-variable name in [code] is whatever setYName()/yName() says,
#   NOT always "I" — call c.yName() to check if unsure. Confirmed live 3+ times: setYName("I")
#   was called, but [code] still assigned to "y" (a plausible-looking but wrong default), each time
#   raising "clang: error: use of undeclared identifier 'y'" only after the failed compile, not before.
#     WRONG: setYName("I") + code "y = scale * pq + background;"   → error: 'y' undeclared
#     RIGHT: setYName("I") + code "I = scale * pq + background;"
#   ALSO confirmed live: setYName("I") + code that starts with "double I = 0.0; ... I = ...;" —
#   redeclaring the already-in-scope "I" as a NEW local → "error: redefinition of 'I'" (yName is a
#   same-scope local declared inside functionSANS, right where [code] is spliced in). Assign to it,
#   never declare it: "I = 0.0; ... I = ...;" (no "double" in front).
#   xName/PARAMETER NAMES ARE MORE DANGEROUS TO REDECLARE THAN yName: they are file-scope
#   "thread_local static" globals only ASSIGNED (not declared) inside functionSANS — so "double q =
#   5.0;" inside [code] does NOT redefine anything, it's legal C++ shadowing. This COMPILES SILENTLY
#   and discards the real value passed in via "q=key;", producing a wrong-but-successful fit with
#   NO compiler error at all — worse than yName's redefinition error, precisely because nothing
#   catches it. Never write "double <xName-or-param-name> = ...;" inside [code], for either reason.
#   MINIMAL CORRECT TEMPLATE for setXName("q")/setYName("I"):
#     double R = radius;                 // OK: params/new helper names get "double"
#     I = (q < 1e-6) ? 1.0 : pow(3.0*(sin(q*R)-q*R*cos(q*R))/pow(q*R,3), 2);  // "q" used directly,
#                                         // "I" assigned directly — neither is declared with "double"
#   IF setXName()/setYName() ARE NEVER CALLED AT ALL, the unset defaults are "x" and "f" — NOT
#   "I", despite "I" being the typical convention. A THIRD failure mode, worse than both above:
#   guessing "I" when setYName() was never called (real default "f") does NOT collide with
#   anything, so "double I = scale*pq+background;" compiles clean as a new unrelated local — the
#   boilerplate's "return f;" then returns f, never assigned, uninitialized, on every evaluation.
#   No error anywhere. Always call setXName()/setYName() explicitly for a new function; never rely
#   on either default.
#
# RULE 8 — PARAMCOUNT: setParamNamesAll(names) requires len(names) == setParamCount(n) EXACTLY —
#   raises ValueError otherwise (used to silently drop/misalign entries; now fails loudly).
#     RIGHT: names=[...]; c.setParamCount(len(names)); c.setParamNamesAll(names)
#   Also: save()/compile() raise ValueError if setParamCount() was never called (still 0) — the
#   interactive path shows a blocking "Check function!" dialog instead, which would hang a script.
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
# RULE 12 — EVERY function authored via c.new() goes under the "ai/" folder — always
#   setFunctionName("ai/<shortName>"), never a bare name and never an existing domain folder
#   (sasviewmodels/, custom/, qtiplot/, ...) that a real library function might already use or
#   later collide with. This is the SAME "/" folder mechanism the library already uses for
#   sasviewmodels/sphere, custom/sphere, etc. — "ai/" is just a reserved namespace for it.
#   WRONG: c.new(); c.setFunctionName("sansTest")
#   RIGHT: c.new(); c.setFunctionName("ai/sansTest")
#   PAYOFF: when a task says "use an EXISTING library model" (the discovery pattern above, or
#   fittable-python-api's), EXCLUDE names starting with "ai/" from the candidates — anything
#   there is a prior AI-authored one-off, never a vetted library model. Confirmed live (twice):
#   a leftover custom function from an earlier session/turn was found by fitFunctions()/
#   functionNames() search and mistaken for "the existing model to reuse" instead of the AI
#   authoring fresh code as the task actually asked — reserving "ai/" and filtering it out of
#   "existing model" searches removes this confusion at the source.
#     WRONG: hits = [h for h in c.functionNames("ALL") if "sphere" in h.lower()]  ← can match
#            an old "ai/sphere-test" from a previous turn, not a real library model
#     RIGHT: hits = [h for h in c.functionNames("ALL")
#                     if "sphere" in h.lower() and not h.startswith("ai/")]
# RULE 13 — GROUP: before setGroup(), check for an existing group that already fits the
#   function's domain and reuse it EXACTLY (byte-for-byte) — only invent a new one if nothing
#   matches. Reusing near-variants inconsistently (e.g. "SANS" vs "sans" vs "Sans") fragments
#   the Explorer category tree into duplicates instead of one real group.
#   scanGroups() CLEARS the editor (group and function name) while it refreshes the explorer, so
#   call it BEFORE new() — never between setFunctionName() and compile(), or compile() fails with
#   "function name is empty" despite the setFunctionName() call sitting right above it.
#   c.scanGroups(); existing = c.groups()
#   match = next((g for g in existing if "sans" in g.lower()), None)
#   c.setGroup(match if match else "custom:sans")   # reuse if found, else propose one
#   New group names follow the library's existing ":"-hierarchy convention (e.g. "shape:sphere"
#   in the Quick Example below) — setGroup() still rejects "/" (ValueError, see RULE 3's group
#   note); ":" for group hierarchy and "/" for the function-name folder are separate mechanisms,
#   don't mix them.
# RULE 14 — a SAS/SANS/SAXS-style model always needs a free "scale" AND "background" param, both
#   with broad "0.." limits — a multiplicative scale on the WHOLE model, independent of the
#   physically-meaningful shape params (radius, sld, ...). Without a free scale, the model's
#   absolute normalization is fixed entirely by the shape-parameter formula — nothing in the fit
#   can correct a mismatch against the data's actual absolute scale, however correct R/sld are.
#   Confirmed live: a compiled function's I = (sld-1e-6)^2 * F*F + background (sld~6e-6, no free
#   scale) predicted intensities ~1e-11 while the actual data was O(1) — a ten-order-of-magnitude
#   gap no amount of R/sld adjustment within physical limits can bridge. Fit ran with NO error but
#   converged to a meaningless result: chi2/dof ~3217, R2 ~3.5e-10.
#   WRONG: setParamNamesAll(["R","sld","background"]); I = (sld-1e-6)**2 * F*F + background;
#   RIGHT: setParamNamesAll(["scale","R","sld","background"]); setParamLimitsAll(["0..",...,"0.."])
#          I = scale * (sld-1e-6)**2 * F*F + background;
#
# RULE 15 — before compile(), self-check every declared param name is actually used in [code] — a
#   plain string check, no new API: a declared-but-unused param is a dead free direction (zero
#   effect on output, unconstrained "fitted" value) that can measurably destabilize the fit, no
#   error raised anywhere. Confirmed live: a 4-param sphere model never referenced "SLD" in [code];
#   the fit hit the iteration cap without converging (gradient ~9 orders above tolerance).
#     import re
#     unused = [p for p in param_names if not re.search(r"\b"+re.escape(p)+r"\b", code_body)]
#     if unused: scriptPrint("WARNING: unused param(s): " + str(unused))
#   Advisory, not a hard gate — a param used only via a beforeIter/Int1-3 caching hook (RULE 6),
#   not directly in [code], will false-positive here; check the hook before assuming it's dead.
#   Standard SAS/SAXS/SANS practice, not a workaround — real instruments' absolute calibration,
#   concentration, and path length all introduce an unknown multiplicative factor scale absorbs.
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
# IMPORTANT: c.open(existing) + setFunctionName(newFolder/newName) does NOT rewrite the inherited
#   hHeaders — copied verbatim, never corrected for the new location. A quoted #include resolves
#   relative to the .cpp's OWN folder, which changes when setFunctionName() moves it to a
#   DIFFERENT folder than the one it was open()-ed from. Confirmed live: open("sasviewmodels/sphere")
#   inherits hHeaders='#include "sphere.h"' (valid there, colocated) -> setFunctionName("ai/X") ->
#   compile() -> fatal error: 'sphere.h' file not found (moved folders, header no longer colocated).
#   WRONG: c.open("sasviewmodels/sphere"); c.setFunctionName("ai/X"); c.compile()  # stale header path
#   RIGHT: c.open("sasviewmodels/sphere"); c.setFunctionName("ai/X")
#          c.setHHeaders('#include "sasviewmodels/sphere.h"')  # re-prefix with the ORIGINAL folder
#          c.compile()
#   Only matters if the original hHeaders references its own subfolder — no-op otherwise.
includedFunctions()->str  setIncludedFunctions(text)  # [included functions]: helper C++ above functionSANS
# Data-driven initial guesses: define beforeFitting(void*ParaM){ if(!beforeFit) return; A=...; }
#   here, call it from code() — NOT the same hook as RULE 6's beforeIter above (that one caches
#   Q-independent quantities per iteration; this one sets initial parameter guesses, once).
#   beforeFit is set by f.beforeFit() / Fittable's "Before Fit" button.
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
#   setParamVariesAll (floats/bools) right next to it — raw numbers raise TypeError.
#   WRONG: setParamLimitsAll([0.0, 0.0])   RIGHT: setParamLimitsAll(["0..", "0.."])
paramVaries(i)->bool  setParamVaries(i, varies) # adjustability checkbox
setParamVariesAll(varies)
# Every param defaults to FIXED (not varying) when declared via setParamCount(n) — skipping this
# call bakes "all fixed" into the compiled function. Fittable.configure() re-derives its vary
# checkboxes from exactly this baked-in default EVERY call (never inherits a prior fit session's
# f.setParamVaries() override) — a function compiled with no vary flags ever set has zero
# adjustable parameters after configure() too, and f.fit() raises RuntimeError: "no adjustable
# parameters". Set what should vary here, at compile time — don't rely on setting it only via
# f.setParamVaries() after configure(), which must be repeated after every single configure() call.
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
                    # MANDATORY for a new function: c.new() leaves group empty, and save()/compile() now
                    # raise ValueError if group is still empty when called — call setGroup() before them,
                    # not just when passing "" explicitly (confirmed live: omitting it entirely used to
                    # silently corrupt internal state and fail later with a confusing clang .cpp-not-found
                    # error instead of a clear message).
groups()->list      # all group names from .fif files (no "ALL", no sub-folders)

# Explorer (call scanGroups() first to refresh):
scanGroups()
category()->str                       # currently selected row in category list
categories()->list                    # includes "ALL" and sub-folders
selectCategory(categoryName)->bool    # selects category, populates function list
functionNames(categoryName)->list     # selectCategory(categoryName) + return its function names
#   ("ALL" returns every function). NOTE: there is no functions() method — confirmed live
#   AttributeError, repeatedly, across separate conversations; the real name is functionNames().
# Compiler has functionNames(); it does NOT have fitFunctions() — that belongs to Fittable, a
#   DIFFERENT singleton. Confirmed live, repeated even after the first AttributeError: c=Compiler
#   then c.fitFunctions(...). To check if a name already exists: filter functionNames("ALL")
#   yourself (e.g. [h for h in c.functionNames("ALL") if h.lower()==name.lower()]), never fitFunctions().

# FIF management:
open(fifName)  # fifName may include subfolder: "sasviewmodels/barbell" opens sasviewmodels/barbell.fif
new()
save(name="", path="", askYN=False)->str  # returns saved FIF text; save(); save("cylinder"); save("subfolder/cylinder"); c.path is a string attribute NOT a method
# compile() auto-saves — c.save() before c.compile() is NOT needed.
# Use c.save() only when persisting changes WITHOUT recompiling (metadata/param edits).
# setGroup() rules (empty/'/' both raise ValueError) are above — save()/compile() need it set too.
# Subfolder storage goes in save(), not setGroup():
#   c.save("qtiplot/MyGauss")  → saved to pathFIF/qtiplot/MyGauss.fif; f.configure("qtiplot/MyGauss")
delete(name="", path="")  # delete(); delete("cylinder"); delete("sasviewmodels/cylinder"); delete("cylinder",path="/p")
# NOTE: delete() silently no-ops if `name` doesn't match any existing function — no error, no
#   return value, nothing (verified in source: plain QFile::remove(), no existence check first).
download(repoUrl="https://iffgit.fz-juelich.de/qtisas/fitfunctions.git", silent=True)

# Code generation:
makeCPP()->str      # saves .cpp and returns generated source
makeScript()->str   # saves compile script and returns script text

# Build:
compile(compileAll=False)->bool  compileAll()
# Raises RuntimeError on failure, with the compiler's error: output included in the message —
# no separate resultsLog() read needed. Returns True on success (never False).

# SasView import:
openSasView()   # requires: pip install sasmodels
# PySAS models: hHeaders has '#include "IncludedFunctions/PySAS.h"'; code calls PySAS::Iq("name", q, ...)
#   These require Python.h at compile time ([python] 1 in FIF).
#   compile() fails with "Python.h not found" if dev headers are missing.
#   Do NOT use subprocess.run([sys.executable, sysconfig...]) — embedded Python has no dev headers.
#   Find system Python.h instead: /opt/homebrew/include/python3.x/Python.h (macOS/Homebrew)
#   Alternative: rewrite the function inline in C++ without the PySAS call.

# Fit.Control — x-range (Fit.Control tab's OWN simulation-range setting — a UI-level x-axis range for
#   plotting/simulating, NOT the same as RULE 5's x-range above, which is about the DATA table's range
#   needing to constrain fit parameters; unrelated concepts that happen to share the term):
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
weightMethod()->int    setWeightMethod(int)    # 0=Instrumental(1/σ²) 1=Statistical(1/Y) 2=Direct(σ) 3=1/Y² 4=1/Y^a 5=1/(c^a+b*Y^a) 6=1/(Y^a*c^|Xmax-X|)
# Verified against the actual weight computation (fittable-data.cpp) — same enum as Fittable's
# setWeightingMethod(). NOTE: 3/4/5/6 are all RECIPROCALS (1/...) — larger |Y| means LESS weight,
# the opposite of what "3=Y²" (dropping the 1/) would suggest.
superpositional()->bool  setSuperpositional(bool)
subFitCount()->int     setSubFitCount(int)
useAlgorithm()->bool   setUseAlgorithm(bool)
fitMethod()->int       setFitMethod(int)       # 0=Nelder-Mead-Simplex 1=Levenberg-Marquardt(default) 2=GenMin
fitMethodPara()->str   setFitMethodPara(str)   # SD=Significant Digits; Simplex: SD=.. MODE=.. ITERS=.. REL=.. CONVRATE=.. STAGNATION=..
                                               # Levenberg: SD=.. MODE=.. ITERS=.. REL=.. ABS=.. DER=.. DSSV=..
                                               # Genetic: SD=.. GENCOUNT=.. GENSIZE=.. ITERS=.. SELRATE=.. MUTRATE=.. SEED=.. MODE=..

# Logging:
log(text)
