# QtiSAS Compiler Python API

Python scripting interface for the **Compiler** fit-function authoring widget.
Access via: `c = Compiler`

> **The Compiler manages the fit-function library** — `.fif` source files and the compiled shared
> libraries that the Fittable widget loads. Use it to query available functions, build libraries,
> import SasView models, or automate batch compilation.

> **Boolean aliases:** QtiSAS defines `true` and `false` as aliases for `True` and `False`.

---

## Quick Reference

The minimal create-and-compile flow:

```python
c = Compiler
c.new()
c.setFunctionName("MyFunc")
c.setGroup("custom")
c.setCode("...")
c.compile()   # auto-saves the .fif; no c.save() before this
```

---

## Default library path

The fit-function library path is set automatically at startup:

| Platform | Default path |
|----------|-------------|
| Linux / macOS | `~/.config/qtisas/FitFunctions/` |
| Windows | `%USERPROFILE%\AppData\Local\qtisas\FitFunctions\` |

`setPath()` is **not needed** in normal use — the path is already configured.
Call it only when the library lives in a non-standard location (e.g. a shared network drive or a test checkout).

---

## Quick Start

### Compile all functions of a category

```python
c = Compiler
c.scanGroups()
c.selectCategory("shape:sphere")
c.compileAll()
```

---

### Compile all functions of a sub-folder

```python
c = Compiler
c.scanGroups()
c.selectCategory("qtiplot/")
c.compileAll()
```

---

### Compile the entire library

```python
c = Compiler
c.selectCategory("ALL")
c.compileAll()
```

---

### Download and compile the official fit-function library

Clones the official QtiSAS fit-function repository into a temporary folder, then compiles every
function into a shared library. Each step:

- `setPath(testPath)` — switches the library path to the temporary folder
- `download()` — clones the repository (new files only, no dialog)
- `compileAll()` — compiles every `.fif` file into a shared library

```python
import os, tempfile
c = Compiler
testPath = os.path.join(tempfile.gettempdir(), "Functions-Test")
os.makedirs(testPath, exist_ok=True)
c.setPath(testPath)
c.download()
c.compileAll()
```

---

## Path

### Path

```python
path = c.path        # current fit-function library path
```

---

### setPath(path="")

```python
c.setPath("/home/user/fitfunctions")
c.setPath()     # opens file dialog
```

Overrides the library path and refreshes the explorer.
Only needed when the library is in a non-standard location — the default path is set automatically at startup (see above).
Raises `ValueError` if the path is non-empty and does not exist.

---

## State Getters

### functionName() / setFunctionName(name)

```python
name = c.functionName()          # name of the currently open FIF file (without extension)
c.setFunctionName("MySphere")    # set the function name field
```

Returns an empty string if no function is loaded.
`setFunctionName()` sets the **Function name** editor field; the value is saved to the FIF file on the next `save()` or `save()` call.

---

### yName() / setYName(name)

```python
y = c.yName()             # dependent variable name, e.g. "I"
c.setYName("I")        # set the [y] field; also updates the code header comment
```

The `[y]` field names the dependent variable of the fit function and is saved in the FIF file.
Must be a valid C++ identifier — letters, digits, underscores only (e.g. `"I"`, `"IofQ"`, `"f"`).
`setYName()` updates the first-line comment in the code editor (`// ---> y = f(x, {P1, ...})`).

---

### xName() / setXName(name)

```python
x = c.xName()             # independent variable name(s), e.g. "x" or "ix,iy"
c.setXName("x")           # set the [x] field; also updates the code header comment
```

The `[x]` field names the independent variable(s) of the fit function.
For 2D functions, multiple names are comma-separated (e.g. `"ix,iy"`).
`setXName()` updates the first-line comment in the code editor.

> **xName consistency:** whatever name you pass to `setXName()` is a file-scope `thread_local static`
> variable that the generated code only ASSIGNS inside `functionSANS` (not declares there — it's
> declared at file scope, outside the function). If you set `"q"`, the variable in `[code]` is `q` —
> do **not** write `double q = x;` (`x` is not declared and gives `undeclared identifier 'x'`).
>
> | `setXName` | What to write in `[code]` |
> |---|---|
> | `"x"` | `double q = x;`  (rename if you prefer `q`) |
> | `"q"` | use `q` directly — no reassignment line needed |
>
> **Do not write `double q = <anything>;` (re-declaring `q` itself) either.** Because `xName`/
> parameter names are file-scope globals only *assigned* inside `functionSANS`, redeclaring one
> with `double` in front is legal C++ shadowing — it compiles with **no error at all** and silently
> discards the real value passed in via `q = x;` (or the real parameter value), producing a
> wrong-but-successful fit. This is more dangerous than the equivalent mistake with `yName` (see
> below), which at least fails loudly with a `redefinition` compile error.

---

### info() / setInfo(text)

```python
html = c.info()                          # [description] HTML text
c.setInfo("<p>Sphere form factor.</p>")  # replace description
```

Gets or sets the `[description]` field — the HTML description shown in the UI and stored in the FIF file.
Content is HTML (as produced by the rich-text editor); plain text is also accepted.

---

### paramCount()

```python
n = c.paramCount()    # number of fit parameters of the currently open function
```

Returns the total parameter count from the parameter-count spinboxes.
Use `setParamCount(n)` to change it.

---

### code() / setCode(text)

```python
src = c.code()              # body of the fit function (the [code] section)
c.setCode("I = R*R*x;")    # replace the fit function body
```

Gets or sets the C++ snippet that forms the body of `functionSANS` — the `[code]` section of the FIF file.
This is the only part written by hand; everything else in the generated `.cpp` is boilerplate.
Inside this snippet the independent variable and all parameter names are available as plain C++ locals.
The dependent variable **must be assigned** before the snippet ends.

**This snippet is C++, not Python — there is no `**` exponentiation operator.** Confirmed live:
`exp(-(x - center)**2 / (2 * sigma**2))` fails with a cryptic `indirection requires pointer operand`
error (`**2` parses as double-dereference, not "squared"). Use `pow(a, b)` or repeated multiplication:
```cpp
// WRONG — Python syntax; C++ has no ** operator
y = scale * exp(-(x - center)**2 / (2 * sigma**2)) + background;

// correct
y = scale * exp(-pow((x - center) / sigma, 2.0) / 2.0) + background;
// or, equivalently:
y = scale * exp(-(x - center) * (x - center) / (2.0 * sigma * sigma)) + background;
```

**Standard models — a request that names one of these uses exactly this formula** (no extra factors, no
invented normalisation, no squaring a power law, parameters as named, background added). None needs a header:
`exp`/`pow`/`sin`/`cos` are always available.

| Model | `[code]` (C++) |
|---|---|
| Porod / power law | `I = scale*pow(q,-n) + background;` |
| Guinier | `I = I0*exp(-q*q*Rg*Rg/3.0) + background;` |
| Sphere form factor | `I = scale*pow(3.0*(sin(q*R)-q*R*cos(q*R))/pow(q*R,3), 2) + background;` |
| Gaussian peak | `y = A*exp(-(x-xc)*(x-xc)/(2.0*w*w)) + c;` |
| Lorentzian peak | `y = A/(1.0+((x-xc)/w)*((x-xc)/w)) + c;` |

The sphere form factor is one model among these, not a template for others: a power law is not squared, and
no other model is multiplied by it.

### setCode() — C++ ternary vs Python ternary

**The reverse mistake also happens: C++'s `cond ? a : b` ternary leaking into surrounding PYTHON
code.** Happens when a script's `setCode()` C++ body and its Python code (e.g. synthetic-data
generation) share the same formula. Python has no `?:` operator — writing it is a plain
`SyntaxError`, caught before the script even runs. Confirmed live:
```python
# WRONG — valid inside c.setCode("""...""") as C++, but this line is bare Python
F = (qR > 1e-6) ? 3.0 * (sin(qR) - qR * cos(qR)) / (qR**3) : 1.0   # SyntaxError: invalid syntax

# CORRECT — Python's ternary is "a if cond else b", not "cond ? a : b"
F = 3.0 * (sin(qR) - qR * cos(qR)) / (qR**3) if qR > 1e-6 else 1.0
```
The C++ version inside `c.setCode("""...""")` is unaffected by this — `cond ? a : b` is correct
there; it's only wrong in the Python code around it.

### setCode() — dependent variable (yName) naming and redeclaration rules

**The dependent variable name is whatever `yName()` returns** — it is NOT always `I`. After `c.new()` the
variables are `x` and **`f`** (not `y`): without `setYName()`, the code assigns `f = ...;`. Always call
`c.yName()` (or check `setYName()`) to find the correct name. If `setYName("y")` was called, write
`y = ...;`, not `I = ...;`. Confirmed live 3+ times: `setYName("I")` was called, but `[code]`
assigned to `y` instead (a plausible-looking but wrong default) — only caught after a failed
compile (`clang: error: use of undeclared identifier 'y'`), not before.

**Both `xName`/`yName` locals are ALREADY DECLARED by the boilerplate before `[code]` runs — never
re-declare them with `double <name> = ...;`.** Confirmed live: `setYName("I")` + code starting
`double I = 0.0; ... I = ...;` compiles with `error: redefinition of 'I'`, because `I` already
exists as a local. Assign to it, never declare it:

```cpp
// WRONG — "I" is already declared; this re-declares it
double I = 0.0;
I = scale * pq + background;

// RIGHT — assign directly, no "double" in front
I = scale * pq + background;
```

### setCode() — unset xName/yName defaults and silent failure

**A request written as `y = a*x^2 + b*x + c` uses its own names:** `c.setXName("x")`, `c.setYName("y")`, code
`"y = a*x*x + b*x + c;"` — `y` assigned directly, no `double`. The `q`/`I` in the other examples are the SAS
convention, not a default. A formula written with `exp`/`sin`/`pow`/`sqrt` needs **no** `setHHeaders()` — include a
header only when the code calls a library function whose header exists; never a guessed name
(`"sasviewmodels/gaussian.h"`).

**If `setXName()`/`setYName()` are never called, the unset defaults are `"x"` and `"f"`** — NOT
`"I"`, despite `"I"` being the typical SANS/SAXS convention and the most common value used in
practice. Never assume either default; call `c.xName()`/`c.yName()` (or set them explicitly)
before writing `[code]`.

**A wrong assumed name is only guaranteed to be caught by a compile error if it collides with
something already declared — an uncalled `setYName()` leaves nothing to collide with, so the
mistake compiles clean and fails silently instead.** Confirmed live: with `setYName()` never
called (leaving the real unset default `"f"`), `[code]` wrote `double I = scale * pq + background;`
— this compiles without error, because `I` is simply a new, unrelated local, not a redeclaration
of anything. But the boilerplate's `return f;` at the end returns `f`, which was never assigned —
an uninitialized value returned on every evaluation, with no error anywhere to catch it. The fix
is not a syntax rule but a required step: always call `setXName("...")`/`setYName("...")`
explicitly for a new function, never rely on either default being correct.

```cpp
// WRONG — setYName() never called; "f" (the real unset default) is never assigned;
// "I" is a plausible-looking guess that compiles fine but is a separate, unrelated local
double I = scale * pq + background;

// RIGHT — call c.setYName("I") in Python first, matching the C++ variable used here
I = scale * pq + background;
```

### setCode() — parameter names are C++ globals, never redeclare

**Parameter names are plain C++ globals available by name — never use `P1`, `P2`, `P3`…**
Every name from `paramNames()` is declared as a file-scope `thread_local static` and only
*assigned* inside `functionSANS`, not declared there (same mechanism as `xName`, above).
Use the names directly — and, same warning as `xName`: never write `double <paramName> = ...;`
inside `[code]`, since that silently shadows the real value with no compile error at all.

**Never use a C++ reserved keyword as a parameter name** (`switch`, `class`, `new`, `delete`,
`template`, `if`, `for`, `return`, `int`, `void`, `true`, `const`, …) — `setParamName()`/
`setParamNamesAll()` reject these with a `ValueError` naming the bad word. Confirmed live before
that check existed: naming a parameter `"switch"` blamed the *wrong* parameter for the failure
(`undeclared identifier 'bg'`, a different, valid parameter declared later) — a bad declaration
corrupts everything after it, so the error shown is not the error you have. `setParamNamesAll()`
also requires the list length to exactly match `setParamCount()`.
```cpp
// correct — parameters declared by name
double V = M_PI * radius * radius * length;
I = scale * contrast * contrast * V * V * PQ + background;

// WRONG — P1/P2/... are not declared; compile error
I = P1 * P2 * P3;
```

### setCode() — using hHeaders functions directly

**Functions from `hHeaders` are available directly in `[code]` and `[included functions]`.**
When combining two reference functions, use their library functions — do not reimplement:
```cpp
// correct — uses Fq() from barbell.h and Iq() from hardsphere.h
double F1, F2;
Fq(x, &F1, &F2, sld, sld_solvent, radius_bell, radius, length);
double PQ = F2 / form_volume(radius_bell, radius, length);
double SQ = Iq(x, radius_effective, volfraction);
I = PQ * SQ * scale + background;

// WRONG — reimplements physics already in the header files
double alpha = (1.0 + 2.0*eta) / pow(1.0-eta, 2.0);
...
```

```python
c.setYName("y")
c.setCode("y = a * cos(b * x) + c * sin(f / x);")   # correct — matches yName()
c.setCode("I = a * cos(b * x) + c * sin(f / x);")   # WRONG — I is undeclared; compile error
```

### setCode() — return statement rules

The snippet is inserted inside `functionSANS`, which has return type `double`.

**Do not end the snippet with `return {yName};`** — the boilerplate appends `saveParameters(ParaM); return {yName};`
automatically after the user code. A trailing return makes `saveParameters` unreachable dead code.

Use `return {yName};` only as an **early exit** mid-body:

```cpp
// correct early exit (assuming yName = "I")
if (x < 0) { I = 0.0; return I; }
I = phi * contrast * contrast * V * V * result;
// no return I; here — the boilerplate provides it

// wrong — makes saveParameters unreachable
I = phi * contrast * contrast * V * V * result;
return I;   // do NOT put this at the end

// wrong — compile error (return; in a double function)
return;
```

---

### hHeaders() / setHHeaders(text)

```python
hdrs = c.hHeaders()                    # extra #include lines ([h-headers] section)
c.setHHeaders('#include "mylib.h"')    # replace the extra headers block
```

Gets or sets the `[h-headers]` section — additional `#include` lines appended to the standard headers
in the generated `.cpp`. Only `#include` lines are copied into the output; other content is ignored by the generator.

The following headers are **always included automatically** — do not add them:
`<math.h>`, `<gsl/gsl_math.h>`, `<gsl/gsl_vector.h>`, `<gsl/gsl_matrix.h>`.

**Any other header must be added explicitly** — but only for a function whose `[code]` really calls a library
function. A formula of `exp`/`pow`/`sin`/`sqrt` (power law, Guinier, Gaussian, Lorentzian) needs **no** header;
`sasviewmodels/sphere.h` in the examples is the case of *reusing* a library model, not a default. Every GSL
sub-library requires its own header:

| If you use | Add to hHeaders |
|---|---|
| `gsl_sf_bessel_J0/J1/Jn/…` | `#include <gsl/gsl_sf_bessel.h>` |
| `gsl_sf_gamma/lngamma/…` | `#include <gsl/gsl_sf_gamma.h>` |
| `gsl_sf_erf/erfc/…` | `#include <gsl/gsl_sf_erf.h>` |
| `gsl_integration_qags/…` | `#include <gsl/gsl_integration.h>` |
| `gsl_sf_expint_E1/…` | `#include <gsl/gsl_sf_expint.h>` |

---

### includedFunctions() / setIncludedFunctions(text)

```python
helpers = c.includedFunctions()          # helper C++ code ([included functions] section)
c.setIncludedFunctions("double sq(double x){ return x*x; }")
```

Gets or sets the `[included functions]` section — verbatim C++ code inserted above `functionSANS`
in the generated `.cpp`. Use it for helper functions, constants, or lookup tables shared across
multiple calls to the fit function.

### setIncludedFunctions() — beforeFitting pattern (initial values from loaded data)

**Initial parameter values computed from the loaded data.** Fittable's "Before Fit" button (and
`f.beforeFit()` — see fittable-python-api.md) calls the compiled function once with `beforeFit` set,
before the fit starts. Define a `beforeFitting(void *ParaM)` helper that checks the flag and, when
set, computes values from the loaded data via the `XXX`/`YYY` macros and assigns them into the
parameter names directly — plain thread-local C++ variables, not read-only (see the `functionT`
struct/accessor macros below). Call it from `code()`. Confirmed working, adapted from the shipped
`linear-efit.fif` — a straight line through the fit range's first and last points:
```python
c.setIncludedFunctions("""
void beforeFitting(void *ParaM)
{
    if (!beforeFit) return;   // only run on the Before Fit trigger, not on every evaluation
    A = (YYY[currentFirstPoint] - YYY[currentLastPoint])
      / (XXX[currentFirstPoint] - XXX[currentLastPoint]);
    B = YYY[currentFirstPoint] - A * XXX[currentFirstPoint];
}
""")
c.setCode("""
y = A*x + B;
beforeFitting(ParaM);
""")
```
`currentFirstPoint`/`currentLastPoint` index into `XXX`/`YYY` for the data range currently selected
for the fit, so this recomputes from the *actual* loaded data — not from `setParamInitsAll()`'s
fixed numbers — and adapts automatically if the range or dataset changes. Same pattern for
nonlinear guesses: `Allometric1a.fif`'s power law `y = a*x^b` derives `a`/`b` log-log from the same
two endpoints inside the same `if (!beforeFit) return;` guard.

### setIncludedFunctions() — dynamic parameter freezing/activation at runtime

> **Dynamically freezing/activating parameters at runtime (e.g. "only fit as many peaks as the
> data actually supports, fix the rest") — adapted from the shipped `manyPeaksEfit`/
> `gaussian-x10-qtisas` functions.** No Python-side call exists for this: `setParamVaries()` is a
> `Fittable` method (see fittable-python-api.md), not present in compiled code — calling it from
> `code()`/`setIncludedFunctions()` fails with `use of undeclared identifier`. The real mechanism
> has two independent parts:
> - **Parameter VALUES** are plain named globals, like `A`/`B` above — `readParameters()`
>   (auto-generated) populates them from `Para` BEFORE `code()` runs, and `saveParameters()`
>   (also auto-generated) writes them back AFTER — do not poke the raw `Para` GSL vector directly
>   from `code()`; it gets silently overwritten by `saveParameters()` using stale pre-assignment
>   values.
> - **Which parameters VARY** has no named-variable equivalent — it goes through the raw
>   `para_fit_yn` field on the `functionT` struct (`gsl_vector_int`, one entry per parameter in
>   `setParamNamesAll()` order; `1`=free, `0`=fixed), via `gsl_vector_int_set()`.
>
### setIncludedFunctions() — full worked example: LorentzianX5 (multi-peak)

> **Don't hand-write this per peak — it doesn't scale past a handful.** Generate the repetitive
> parts (parameter names, pointer arrays) from a single Python `N`, so the C++ loop logic is
> identical whether `N` is 5 or 50. Full worked example — a 5-Lorentzian-peak function,
> `LorentzianX5`, from `setFunctionName()` through `compile()`:
> ```python
> N = 5   # number of peak slots — change only this to scale up/down
>
> names = ["nreal"]
> for i in range(1, N + 1):
>     names += [f"scale{i}", f"center{i}", f"gamma{i}"]
> names += ["background"]
>
> c = Compiler
> c.new()
> c.setFunctionName("LorentzianX5")
> c.setGroup("custom:peaks")
> c.setYName("I")
> c.setXName("x")
> c.setInfo("<p>Sum of up to " + str(N) + " Lorentzian peaks plus a constant background. "
>           "Peaks are ordered by position; only as many as have real amplitude are left "
>           "free to vary, the rest are fixed with amplitude=0.</p>")
>
> c.setParamCount(len(names))
> c.setParamNamesAll(names)
>
> inits = [float(N)]
> for i in range(N):
>     inits += [1.0, float(i) - N / 2.0, 0.1]
> inits += [0.0]
> c.setParamInitsAll(inits)
>
> limits = ["1..{}".format(N)]
> for i in range(N):
>     limits += ["0..", "..", "0.."]
> limits += [".."]
> c.setParamLimitsAll(limits)
>
> varies = [False] + [True] * (3 * N) + [True]   # "nreal" itself is never varied by the optimizer
> c.setParamVariesAll(varies)
>
> descrs = ["number of real peaks"]
> for i in range(1, N + 1):
>     descrs += [f"amplitude {i}", f"position {i}", f"width {i}"]
> descrs += ["background"]
> c.setParamDescrsAll(descrs)
>
> scale_ptrs  = ", ".join(f"&scale{i}"  for i in range(1, N + 1))
> center_ptrs = ", ".join(f"&center{i}" for i in range(1, N + 1))
> gamma_ptrs  = ", ".join(f"&gamma{i}"  for i in range(1, N + 1))
>
> c.setIncludedFunctions(f"""
> const int NPEAKS = {N};
> double *scaleP[{N}]  = {{{scale_ptrs}}};    // pointers into the plain named globals above —
> double *centerP[{N}] = {{{center_ptrs}}};   // this is the ONLY per-N-specific text, and it's
> double *gammaP[{N}]  = {{{gamma_ptrs}}};    // generated by Python, never hand-written
>
> void orderAndSuppressPeaks(void *ParaM)
> {{
>     if (!beforeFit && !beforeIter) return;   // run at fit start and before each iteration only
>
>     double amp[{N}], center[{N}], gamma[{N}];
>     for (int i = 0; i < NPEAKS; i++)
>     {{
>         amp[i] = *scaleP[i]; center[i] = *centerP[i]; gamma[i] = *gammaP[i];
>     }}
>
>     for (int i = 1; i < NPEAKS; i++)          // order by position — insertion sort, N is small
>         for (int j = i; j > 0 && center[j] < center[j - 1]; j--)
>         {{
>             std::swap(amp[j], amp[j - 1]);
>             std::swap(center[j], center[j - 1]);
>             std::swap(gamma[j], gamma[j - 1]);
>         }}
>
>     int nActive = 0;
>     for (int i = 0; i < NPEAKS; i++)
>         if (fabs(amp[i]) > 1e-6) nActive++;   // however many peaks currently have real amplitude
>     nreal = (double) nActive;                  // report back through the "nreal" parameter
>
>     gsl_vector_int *fitYN = ((struct functionT *) ParaM)->para_fit_yn;
>     for (int i = 0; i < NPEAKS; i++)
>     {{
>         bool active = i < nActive;
>         *scaleP[i] = active ? amp[i] : 0.0;   // excess peaks: amplitude=0 ...
>         *centerP[i] = center[i];
>         *gammaP[i] = gamma[i];
>         int v = active ? 1 : 0;
>         gsl_vector_int_set(fitYN, 1 + 3 * i, v);   // ... AND fixed (not just zeroed)
>         gsl_vector_int_set(fitYN, 2 + 3 * i, v);   // index order matches setParamNamesAll():
>         gsl_vector_int_set(fitYN, 3 + 3 * i, v);   // 0=nreal, 1=scale1, 2=center1, 3=gamma1, ...
>     }}
> }}
>
> double sumPeaks(double x)
> {{
>     double s = 0.0;
>     for (int i = 0; i < NPEAKS; i++)
>         s += *scaleP[i] * pow(*gammaP[i], 2) / (pow(x - *centerP[i], 2) + pow(*gammaP[i], 2));
>     return s;
> }}
> """)
>
> c.setCode("""
> orderAndSuppressPeaks(ParaM);
> I = background + sumPeaks(x);
> """)
>
> ok = c.compile()
> if not ok:
>     scriptPrint(qti.app.resultsLog().toPlainText()[-1000:])
>     raise RuntimeError("Compilation failed")
> scriptPrint("LorentzianX5 compiled OK")
> ```
> This has NOT been compiled/tested — it's built directly from the shipped reference functions'
> confirmed-working mechanism (raw `functionT`/`para_fit_yn` access, `readParameters()`/
> `saveParameters()` ordering), adapted to (a) use plain named globals per the modern `setCode()`
> convention instead of the older files' fully hand-written `readParameters()`, and (b) generate the
> per-peak boilerplate from Python so it scales to any `N` without manual unrolling. Verify it
> compiles before relying on it.

---

### group()

```python
grp = c.group()    # [group] field of the currently open FIF file
```

Returns the group name stored in the `[group]` tag of the currently open FIF file (shown in the
**Group name** editor field) — the value saved into the FIF file, determining where the function
appears in the explorer. Empty string if no function is loaded.

---

### setGroup(name)

```python
c.setGroup("custom")          # plain group name — correct
c.setGroup("shape:sphere")    # another valid plain name
```

Sets the **Group name** editor field. The new value is written to the FIF file on the next `save()` call.

> **Rules — `setGroup()` raises `ValueError` if violated:**
> - Name must not be empty: `c.setGroup("")` → `ValueError`
> - Name must not contain `'/'`: `c.setGroup("qtiplot/")` → `ValueError`
>
> The group is a plain logical label, not a file-system path.
> To save a function into a sub-folder, pass the path to `save()` instead:
> ```python
> c.setGroup("custom")              # group label
> c.save("qtiplot/MyGauss")         # saves to pathFIF/qtiplot/MyGauss.fif
> ```

> **`save()`/`compile()` also raise `ValueError` if `setGroup()` was NEVER called at all for a
> brand-new function** (`c.new()` leaves the group field empty) — not just when `setGroup("")` is
> called explicitly. `c.new()` → `setFunctionName(...)` → `setCode(...)` → `compile()`, skipping
> `setGroup(...)`, now raises `ValueError: compile: group is empty — call setGroup("name") before
> compile()...` immediately. Before this guard existed, an empty group made the explorer's internal
> state-restore step (runs after every save of a brand-new function, matching against the group
> name to relocate it) fail silently, clearing the function name/code — `compile()` then failed deep
> inside its shelled-out clang invocation with `clang: error: no such file or directory:
> '<name>.cpp'`, with no hint that a missing `setGroup()` call was the actual cause. Confirmed live.
> **The minimal create-and-compile flow is always: `c.new()` → `setFunctionName(...)` →
> `setGroup(...)` → `setCode(...)` → `compile()`** — `setGroup()` is not optional for a new function.

---

### groups()

```python
grps = c.groups()    # all group names found in the library (excluding "ALL" and sub-folders)
```

Returns the list of unique group names read from the `[group]` tag of every `.fif` file in the library.
Does not include the `ALL` meta-entry or sub-folder names.
Call `scanGroups()` first to refresh.

---

### category()

```python
cat = c.category()    # currently selected row in the category explorer list
```

Returns the category currently selected in the explorer list (i.e. the active filter in `listViewGroup`).
Returns an empty string if nothing is selected.

---

### categories()

```python
grps = c.categories()    # list of category names from the last scanGroups() call
```

Returns the categories currently shown in the explorer list (including `"ALL"`).
Call `scanGroups()` first to refresh.

```python
c.scanGroups()
print(c.categories())    # e.g. ['ALL', 'Cylinders', 'Polymers', 'Spheres']
```

---

### selectCategory(categoryName)

```python
c.selectCategory("Spheres")    # select category in explorer, populate function list
c.selectCategory("ALL")        # select the "ALL" meta-group
```

Selects a category in the explorer list and populates `listViewFunctions`; this clears the function that was open.
Raises `ValueError` if the category name is not found.

> `categoryChanged(category="")` is the lower-level primitive `selectCategory()` calls internally —
> unlike `selectCategory()`, it does NOT validate the name and silently does nothing for an unknown
> category. Prefer `selectCategory()`; there's no reason to call `categoryChanged()` directly.

---

### Compiler.functionExists(name)

A method of `Compiler` (also `Fittable.functionExists`) — a plain `functionExists(...)` call is a `NameError`.

```python
c.functionExists("ai/MyPeak")    # True when a compiled function of that full name exists
```

No category is selected; the same as `"ai/MyPeak" in Fittable.fitFunctions()`.

### functionNames(categoryName="ALL")

```python
functions = c.functionNames("Spheres")         # the function names of a category (a group)
functions = c.functionNames("sasviewmodels/")  # a folder category: its files, "sasviewmodels/sphere", ...
functions = c.functionNames()                  # "ALL" by default: every function name
```

Returns the names as the explorer lists them (the top level first, `(...)` copies left out) without selecting the
category: the explorer and the function that is open stay as they are. `scanGroups()` and `new()` clear the open
function (`functionName()` is `""` afterwards).

Requires an **exact** category name as returned by `categories()` — wildcards and substrings are not accepted.

> **`Compiler` has `functionNames()`; it does NOT have `fitFunctions()`** — that name belongs to
> `Fittable`, a different singleton. Confirmed live, repeated even after the first AttributeError:
> `c = Compiler` then `c.fitFunctions(...)` — never call a `Fittable`-only method (`fitFunctions()`,
> `configure()`, `fit()`, `setWeightingMethod()`, ...) on `c`. To check whether a name already
> exists among the definitions, filter `c.functionNames("ALL")` yourself, e.g.
> `[h for h in c.functionNames("ALL") if h.lower() == name.lower()]` — never `c.fitFunctions(...)`.

> **`functionNames()` lists DEFINITIONS (the `.fif` files) — also one whose last `compile()` failed.**
> Whether a function is compiled and can be fitted: `name in Fittable.fitFunctions()` (on `Fittable`, not on
> `c`). A function the request asks to compile is defined and compiled again every time — never put
> `c.new()` … `c.compile()` behind a check whether it exists: after a failed compile the definition is still
> listed, the check skips the compile, and `configure()` then reports "not found".

To search by **category keyword**, filter `categories()` first:
```python
cats = [cat for cat in c.categories() if "cylinder" in cat.lower()]
refs = c.functionNames(cats[0]) if cats else []
```

To search by **function name keyword** (when you know the name but not the category), use `"ALL"`:
```python
all_funcs = c.functionNames("ALL")
refs = [f for f in all_funcs if "barbell" in f.lower()]
```

**Function names can include a subfolder prefix** — `"sasviewmodels/barbell"` means the file is
`{libraryPath}/sasviewmodels/barbell.fif`. Pass the full name (including subfolder) to `open()`:
```python
c.open("sasviewmodels/barbell")    # correct — opens sasviewmodels/barbell.fif
c.open("barbell")                  # wrong if the file is in a subfolder
```
A substring filter on the full name still works: `"barbell" in "sasviewmodels/barbell"` is `True`.

---

## FIF Management

### scanGroups()

```python
c.scanGroups()
```

Rescans the library path and refreshes the category and function explorer lists.
Always call this after `setPath()` or after adding new FIF files externally.

> **It CLEARS the editor — call it BEFORE `new()`, never between `setFunctionName()` and
> `compile()`.** Refreshing the explorer resets the editor fields as it goes, including the group
> and function name. Confirmed live: `new()` → `setFunctionName("ai/PorodPowerLaw")` →
> `scanGroups()` → ... → `compile()` failed with `compile: function name is empty` while the
> `setFunctionName()` call sat three lines above it. Looking up a group name mid-sequence is the
> natural reason to call it, so get the groups first:
>
> ```python
> c.scanGroups()                      # first — it clears the editor
> group = next((g for g in c.groups() if "porod" in g.lower()), "custom:porod")
> c.new()
> c.setFunctionName("ai/PorodPowerLaw")
> c.setGroup(group)
> ```

---

### open(fifName)

```python
c.open("SpherePY")                 # function in the library root
c.open("sasviewmodels/barbell")    # function in a subfolder
```

Loads the FIF file into the editor and populates all UI fields.
After `open()`, `functionName()` and `category()` return the loaded function's name and category.
Raises `ValueError` if the function file is not found.

Function names returned by `functionNames()` already include the subfolder prefix when applicable
(e.g. `"sasviewmodels/barbell"`). Pass the name directly to `open()` without stripping the prefix.

---

### newFIF()

```python
c.new()
```

Clears the editor — resets group name, function name, code, parameters, and all options.
Use before creating a new function from scratch.

---

### save(name="", path="", askYN=False)

```python
c.save()                                  # save current function to current library path
c.save("cylinder")                        # save as cylinder.fif in current library path
c.save("sasviewmodels/cylinder")          # save into sasviewmodels/ subfolder
c.save("sasviewmodels/cylinder", askYN=True)  # with overwrite confirmation
c.save("cylinder", path="/custom/path")   # save to custom base path
fif = c.save()                            # returns the saved FIF text as a string
```

Saves the current editor content as a `.fif` file.
- `name=""` — use the function name currently shown in the editor
- `path=""` — use the current library path (`c.path`)
- Subfolder prefix in `name` (e.g. `"sasviewmodels/cylinder"`) is created automatically
- Returns the saved FIF text as a string; empty string on other (non-name) failures
- Refreshes the explorer after saving

> **`c.path` is a string attribute — read it as `c.path`, not `c.path()`.**

> **IMPORTANT: the function name must not contain `(` or `)`** — raises `ValueError` (checked
> before anything is written). A name like `"Cylinder(2)"` breaks the function list's keyboard
> navigation (Down/Up jumps back to a different, wrongly-matched row) and can silently overwrite
> a *different* function's `.fif`/library on a later save/compile, since the saved-to filename is
> derived from this same name.
>   - WRONG: `c.setFunctionName("Cylinder(2)")` then `c.save()`
>   - RIGHT: `c.setFunctionName("Cylinder_v2")` — use `_`/`-` instead of parentheses

> **IMPORTANT: a name differing from an EXISTING function only in letter case also raises
> `ValueError`** (e.g. saving `"SphereFit"` when `"spherefit"` already exists) — checked upfront. On
> a case-insensitive filesystem (macOS, Windows), a different-case variant resolves the `.fif` write
> to the SAME file while a later `.dylib` build creates a separately-cased file — `save()`/`compile()`
> both report success, but the mismatched pair then fails Fittable's case-sensitive matching and
> disappears from `fitFunctions()`/`configure()` under **either** casing. Confirmed live: compiling
> `"SphereSANS_Fit"` over an existing `"SphereSANS_fit"` made both names simultaneously unusable — now
> caught upfront instead. Pick a name that isn't a case-variant of one already in
> `functionNames("ALL")` — don't retry the same colliding name expecting a different result.
>
> **Exception — inside `"ai/"` specifically, when compiling a NEW/custom function (not modifying a
> specific EXISTING one by name): treat the collision as disposable scratch, not a fallback.**
> `"ai/"` holds AI-authored one-offs with no guaranteed relationship to the current task.
> `c.delete(collidingName)` the old entry, then compile fresh under the ORIGINALLY-intended name —
> never silently adopt the colliding function's code/params as satisfying the current task. Confirmed
> live: hitting collision `"ai/SphereSANS"` vs. existing `"ai/sphereSANS"`, the recovery opened the
> pre-existing function and used its old parameter set (`scale, R, sld, background`) instead of the
> intended 5-parameter model (`scale, R, SLD, SLDsolv, background`) — every later part (fit, plot,
> legend) then silently built on an unrelated, unverified old function.
> ```python
> # WRONG — collision recovery falls back to reusing whatever was already there
> c.new(); c.setFunctionName("ai/SphereSANS"); c.setCode(new_code); ok = c.compile()
> # ValueError: 'ai/sphereSANS' already exists, differing only in case
> hits = [f for f in c.functionNames("ALL") if f.lower() == "ai/spheresans"]
> c.open(hits[0])              # uses whatever OLD code was left there — never runs new_code at all
>
> # RIGHT — clear the disposable old slot, then compile the intended NEW function
> c.new(); c.setFunctionName("ai/SphereSANS"); c.setCode(new_code); ok = c.compile()
> # ValueError: 'ai/sphereSANS' already exists, differing only in case
> hits = [f for f in c.functionNames("ALL") if f.lower() == "ai/spheresans"]
> c.delete(hits[0])             # old content is disposable — clear the collision
> c.new(); c.setFunctionName("ai/SphereSANS"); c.setCode(new_code)   # now compiles the NEW function
> ok = c.compile()
> ```
> This exception does NOT apply outside `"ai/"`, nor when the request names a SPECIFIC EXISTING
> function to use or modify — that case is Rule 1/Rule 3's `open()`-and-modify pattern instead, and
> a collision there is the actual target, not disposable scratch content.

---

### delete(name="", path="")

```python
c.delete()                              # delete currently open function
c.delete("cylinder")                    # delete by name
c.delete("sasviewmodels/cylinder")      # delete from sub-folder
c.delete("cylinder", path="/custom/p")  # delete from custom path
```

`delete()` is a `Compiler` method like every one here — a bare `delete("...")` is a `NameError`.

Deletes the FIF file (and its `.cpp`/`.so`/`.dylib`/`.dll`) from disk and refreshes the explorer.
If `name` is empty, deletes the currently open function. Empty sub-folders are removed automatically.

---

### download(repoUrl="…", silent=True)

```python
c.download()                                      # clone from default repository (silent)
c.download("https://example.com/myfunctions.git") # clone from custom URL (silent)
c.download(silent=False)                          # show interactive selection dialog
```

Downloads the fit-function library from a Git repository into the current path.
`silent=True` (default) adds only new files automatically, no dialog. `silent=False` opens an
interactive dialog to review and select which files to apply.

---

## Code Generation

### makeCPP()

```python
c.open("MySphere")
c.makeCPP()             # save .cpp to disk
cpp = c.makeCPP()       # save and return the generated C++ source as a string
```

Generates a C++ `.cpp` source file from the currently open FIF and saves it to disk.
Returns the full generated C++ source as a string (empty string on error).

---

### makeScript()

```python
c.makeScript()               # save compile script to disk
script = c.makeScript()      # save and return the script text as a string
```

Generates the compile script (`.sh` / `.ps1`) for the current function and saves it to disk.
Returns the full script text as a string (empty string on error).

---

## Build

### compile(compileAll=False)

```python
c.open("MySphere")
c.compile()           # compile currently open function
c.compile(True)       # compile all functions (same as compileAll())
```

Builds the shared library (`.so` / `.dylib` / `.dll`) for the current function.
Returns `True` on success. **On failure it raises `RuntimeError`, with the compiler's own
`error:` output included directly in the exception message** — no need to separately read
`qti.app.resultsLog()` to find out what went wrong (though the full log — including non-error
output — is still there if needed; `resultsLog()` is an `ApplicationWindow` method, NOT a
Compiler method). `compile()` saves the FIF internally first, so it raises the same `ValueError`
as `save()` (see above) if the function name contains `(` or `)`.

```python
c.compile()
scriptPrint("Compiled OK")   # only reached if compile() didn't raise
```

Self-debugging loop, reading the exception text instead of scraping the results log:

```python
for attempt in range(3):
    try:
        c.compile()
        break
    except RuntimeError as e:
        msg = str(e)
        if "gsl_sf_bessel" in msg:
            c.setHHeaders(c.hHeaders() + "\n#include <gsl/gsl_sf_bessel.h>")
        elif "gsl_sf_gamma" in msg:
            c.setHHeaders(c.hHeaders() + "\n#include <gsl/gsl_sf_gamma.h>")
        else:
            raise
else:
    raise RuntimeError("compilation failed after 3 attempts")
```

---

### compileAll()

```python
c.compileAll()
```

Compiles all functions in the library. Equivalent to `compile(True)`.

---

## SasView

### openSasView()

```python
c.openSasView()
```

Opens a SasView `.py` model file and converts it into a QtiSAS fit-function header.
Requires Python 3 with `sasmodels` installed (`pip install sasmodels`).

---

## PySAS models

Some sasviewmodels functions call `PySAS::Iq()` instead of a native C `Iq()`. These are
**PySAS models** — they route evaluation through the sasmodels Python package at runtime.

Recognizing a PySAS model:
- `hHeaders` contains `#include "IncludedFunctions/PySAS.h"`
- `code` contains `PySAS::Iq("model_name", q, ...)`
- The FIF has `[python] 1` set internally

**Compiling a PySAS model requires `Python.h`** — the Python development header — at compile time.
`PySAS.h` wraps the Python C API (`PyObject*`, `PyArg_ParseTuple`, …), defined in `Python.h`.

> **`c.compile()` will fail with `fatal error: 'Python.h' file not found`** if the Python
> development headers are not on the include path.

The include path is set from the **Python path** field in the Compiler UI (set once, baked into
every compile script). On macOS the correct path is the system Python framework, e.g.
`/opt/homebrew/opt/python@3.x/Frameworks/Python.framework/Versions/3.x/Headers`.

**Do NOT use `subprocess.run([sys.executable, ...sysconfig...])` to find the path** — QtiSAS's
embedded Python lacks dev headers; `sysconfig.get_path('include')` returns empty.

Instead, find the system Python headers manually:

```python
import os
# common macOS locations (Homebrew / system):
candidates = [
    "/opt/homebrew/include/python3.12",
    "/opt/homebrew/include/python3.11",
    "/usr/local/include/python3.12",
    "/usr/local/include/python3.11",
]
python_h = next((p for p in candidates if os.path.isfile(p + "/Python.h")), None)
scriptPrint("Python.h found at: " + str(python_h))
```

Once the correct path is known, set it in the Compiler UI's Python path field
(or prepend it to `CPLUS_INCLUDE_PATH` in the compile script before calling `c.compile()`).

**Alternative — rewrite as a native C model:** if `sasmodels` is unavailable, implement the
physics directly in `[code]` or `[included functions]` using the references in the FIF `[description]`.
This removes the Python dependency entirely and compiles like any other function.

---

## Parameters

### setParamCount(n)

```python
c.open("MySphere")
c.setParamCount(4)
```

Sets the number of fit parameters for the currently open function.
Always call this before filling the parameter table.

> **paramCount must equal the list length.** Entries in `setParamNamesAll()` beyond `n` are
> silently dropped and will not be declared as C++ locals. Using them in `[code]` gives
> `undeclared identifier` at compile time. The safest pattern:
> ```python
> names = ["scale", "R", "SLD", "SLDsolv", "volfraction", "dielectconst"]
> c.setParamCount(len(names))   # derived from the list — never a hard-coded number
> c.setParamNamesAll(names)
> ```

> **`save()`/`compile()` raise `ValueError` if the parameter count is still 0** — i.e. `setParamCount(n)`
> was never called (or called with `0`) for the current function. The interactive C++ path instead shows
> a blocking **"Check function!"** dialog, which has no one to click it in a script and would hang.
> Verified directly: calling `setFunctionName()`/`setGroup()`/`setCode()` and going straight to
> `compile()`, skipping `setParamCount()`/`setParamNamesAll()` entirely, now raises this `ValueError`
> immediately instead of risking a hang on the blocking dialog.

---

### paramNames()

```python
names = c.paramNames()    # ['R', 'SLD', 'background']
```

---

### paramName(i) / setParamName(i, name)

```python
name = c.paramName(0)
c.setParamName(0, "Radius")
```

---

### setParamNamesAll(names)

```python
c.setParamNamesAll(["Radius", "SLD", "background"])
```

Sets all parameter names at once. Extra list entries beyond `paramCount()` are ignored.

---

### paramInit(i) / setParamInit(i, value)

```python
v = c.paramInit(0)
c.setParamInit(0, 10.0)
```

Initial value stored in the FIF `[initial values]` block.

---

### setParamInitsAll(values)

```python
c.setParamInitsAll([10.0, 1e-6, 0.0])
```

**Values should be Python floats or ints.** Unlike `setParamNamesAll`/`setParamLimitsAll`/
`setParamDescrsAll` (which explicitly reject non-string input), this setter has no type check at
all — it calls `PyFloat_AsDouble()` on each item, which silently coerces a NUMERIC string via
Python's own `float()` parsing:
```python
c.setParamInitsAll([50.0, 6e-6, 0.0])    # correct
c.setParamInitsAll(["50.0", "6e-6"])      # ALSO WORKS — silently coerced to 50.0 / 6e-06, no error
c.setParamInitsAll(["fifty", "6e-6"])     # WRONG — SystemError (only a genuinely non-numeric
                                           #   string triggers the "exception set but function
                                           #   returns normally" failure)
```

---

### paramLimit(i) / setParamLimit(i, limit)

```python
lim = c.paramLimit(0)               # e.g. "..", "0..100", "5.2+-2.1"
c.setParamLimit(0, "0..100")        # absolute range [0, 100]
c.setParamLimit(1, "5.2+-2.1")      # Bayesian: mean = 5.2, sigma = 2.1
c.setParamLimit(2, "0..")           # lower bound only — amplitude/width ≥ 0, no upper bound
c.setParamLimit(3, "..10")          # upper bound only, no lower bound
c.setParamLimit(4, "..")            # fully free, no constraint on either side
```

Limit string stored in bracket notation in `[initial values]` (e.g. `10[0..100]`).

| Format | Meaning |
|---|---|
| `".."` | free, no constraint |
| `"a..b"` | absolute range [a, b] |
| `"a.."` | lower-bounded only — no upper limit |
| `"..b"` | upper-bounded only — no lower limit |
| `"mean+-sigma"` | Bayesian prior: mean and sigma (e.g. `"5.2+-2.1"`) |

Either side of `..` accepts the literal text `-inf`/`inf` instead of being left empty — `"-inf..10"` is
equivalent to `"..10"`. `paramLimit()` returns this same `-inf`/`inf` text form when reading back a
limit that's genuinely unbounded on one side, so round-tripping a limit string through
`setParamLimit`/`paramLimit` is safe.

Prefer `"0.."` over inventing an arbitrary large upper bound like `"0..1e4"` for a parameter that's
only physically constrained on one side (e.g. an amplitude or width that must be non-negative but has
no natural ceiling) — an arbitrary ceiling can bias the fit if the true value happens to be near it.

---

### setParamLimitsAll(limits)

```python
c.setParamLimitsAll(["..", "0..1e-4", ".."])
```

**Values must be Python strings in the `setParamLimit` format above — raw numbers cause a
`TypeError`.** This is the OPPOSITE requirement from its neighbors `setParamInitsAll`/
`setParamVariesAll` (floats/bools, not strings) — easy to mix up since all three are called the
same way, right next to each other:
```python
c.setParamLimitsAll(["0..", "0.."])       # correct — range strings, even for "no upper bound"
c.setParamLimitsAll([0.0, 0.0])           # WRONG — TypeError; these are not limit strings
```

---

### paramVaries(i) / setParamVaries(i, varies)

```python
adj = c.paramVaries(0)
c.setParamVaries(0, False)    # fix parameter 0
```

Corresponds to the adjustability checkbox (`[adjustibility]` in FIF).

---

### setParamVariesAll(varies)

```python
c.setParamVariesAll([True, True, False])
```

> **Values are converted with Python truthiness (`PyObject_IsTrue`), not a bool/string check —
> a non-empty STRING is truthy regardless of its text.** `c.setParamVariesAll(["True", "False"])`
> silently sets **both** parameters to varying=`True` — `"False"` is a non-empty string and is
> therefore truthy. No exception, no warning, just the opposite of the obvious intent for the
> second entry. Always pass actual Python `bool` values (`True`/`False`), never strings.

> **Every parameter defaults to fixed (not varying) when first declared via `setParamCount(n)`** —
> skipping `setParamVariesAll()`/`setParamVaries()` leaves ALL parameters fixed, baked into the compiled
> function as its default state. `Fittable.configure()` re-derives its own vary/fixed checkboxes from
> exactly this baked-in default every time it's called — so a function compiled with no vary flags ever
> set has NO adjustable parameters after `configure()` either, and `f.fit()` raises `RuntimeError: fit():
> no adjustable parameters`. Set the parameters you intend to fit here, at compile time, rather than
> assuming you can enable them later purely via `Fittable.setParamVaries()` — that works too, but only as
> an override you must repeat after every `configure()` call (see fittable-python-api.md), not as a
> substitute for a sensible compiled default.

---

### paramDescr(i) / setParamDescr(i, text)

```python
d = c.paramDescr(0)
c.setParamDescr(0, "radius nm")
```

---

### setParamDescrsAll(descrs)

```python
c.setParamDescrsAll(["radius nm", "scattering length density", "flat background"])
```

> **IMPORTANT: never put a literal comma inside a description string.** The `[parameter
> description]` FIF field is comma-joined with no escaping — a comma inside one description splits
> into an extra field, desyncing the count from the parameter count, so re-reading the FIF (during
> `c.compile()`'s save→reload, or `c.open()`) pops a blocking `"Error: [parameter description]"`
> dialog. Even when the count still happens to match, every description after the stray comma
> silently shifts by one and ends up attached to the wrong parameter.
>   - WRONG: `c.setParamDescr(3, "shape (0=Gauss,1=Lorentz)")`
>   - RIGHT: `c.setParamDescr(3, "shape (0=Gauss; 1=Lorentz)")` — use `;` or drop punctuation instead

---

## Fit.Control

Controls in the **Fit.Control** tab: custom simulation x-range and fit algorithm options.
These are saved in the `.fif` file and restored when the function is re-opened.

### X-range

| Method | Description |
|---|---|
| `setXRange(uniform, xMin, xMax, nPoints, logStep, yMin=0.0)` | Enable custom x-range and set all values at once |
| `customXrange()` → `bool` | Whether custom x-range is active |
| `setCustomXrange(bool)` | Enable/disable custom x-range |
| `xMin()` → `float` | Simulation x minimum |
| `setXMin(float)` | Set simulation x minimum |
| `xMax()` → `float` | Simulation x maximum |
| `setXMax(float)` | Set simulation x maximum |
| `xPoints()` → `int` | Number of simulation points |
| `setXPoints(int)` | Set number of simulation points |
| `yMin()` → `float` | Display y minimum |
| `setYMin(float)` | Set display y minimum |
| `logStep()` → `bool` | Whether x-axis is log-spaced |
| `setLogStep(bool)` | Enable/disable log-spaced x |

```python
c.setXRange(False, 0.001, 1.0, 200, True)          # uniform=False, logStep=True
c.setXRange(False, 0.001, 1.0, 200, True, 1e-5)    # with yMin
```

### Fit options

| Method | Description |
|---|---|
| `eFitEnabled()` → `bool` | Whether eFit mode is active |
| `setEFitEnabled(bool)` | Enable/disable eFit mode |
| `eFit()` → `str` | eFit options string (space-separated keywords) |
| `setEFit(str)` | Set eFit options string (see keywords below) |
| `weight()` → `bool` | Whether weighting is on |
| `setWeight(bool)` | Enable/disable weighting |
| `weightMethod()` → `int` | Weighting method index (0–6, see below) |
| `setWeightMethod(int)` | Set weighting method (0–6) |
| `superpositional()` → `bool` | Whether function is superpositional |
| `setSuperpositional(bool)` | Set superpositional flag |
| `subFitCount()` → `int` | Number of sub-fits (superpositional) |
| `setSubFitCount(int)` | Set number of sub-fits |
| `useAlgorithm()` → `bool` | Whether custom fit algorithm is active |
| `setUseAlgorithm(bool)` | Enable/disable custom fit algorithm |
| `fitMethod()` → `int` | Fit algorithm index (0–2, see below) |
| `setFitMethod(int)` | Set fit algorithm |
| `fitMethodPara()` → `str` | Fit algorithm parameter string |
| `setFitMethodPara(str)` | Set fit algorithm parameters (see below) |

Weighting method values:

| Index | Formula |
|---|---|
| 0 | Instrumental: w = 1/σ² |
| 1 | Statistical: w = 1/\|Y\| |
| 2 | Direct: w = σ |
| 3 | Variance = Y²: w = 1/Y² |
| 4 | Variance = Yᵃ: w = 1/\|Y\|ᵃ |
| 5 | Variance = cᵃ+b·Yᵃ: w = 1/(cᵃ+b\|Y\|ᵃ) |
| 6 | Variance = Yᵃ∘c^\|Xmax−X\|: w = 1/(\|Y\|ᵃ·c^\|Xmax−X\|) |

Verified directly against the weight computation in `fittable-data.cpp` — this is the same underlying
enum as `Fittable.setWeightingMethod()` (see fittable-python-api.md), just exposed through the Compiler's
own Fit.Control tab. All of methods 3-6 are reciprocals (`1/...`) — larger `|Y|` means *less* weight.

eFit string keywords (space-separated, all optional):

| Keyword | Effect |
|---|---|
| `no-before` | Skip "Before Fit" step |
| `no-fit` | Skip "Fit" step |
| `no-after` | Skip "After Fit" step |
| `yes-simulate` | Simulate data when `no-fit` is set |
| `color=NAME` | Color of the fitting curve |
| `name=TABLE` | Name of the fit curve table (default: `fitCurve-FUNCTION-NAME`) |
| `no-reslog` | Skip showing results in Res-Log |
| `yes-res-in-plot` | Show fit results in the graph |

```python
c.setEFitEnabled(True)
c.setEFit("no-before color=red yes-res-in-plot")
```

Algorithm index values for `setFitMethod`:

| Index | Algorithm |
|---|---|
| 0 | Nelder-Mead Simplex |
| 1 | Levenberg-Marquardt (default) |
| 2 | GenMin (genetic) |

`setFitMethodPara` parameter strings:

| Algorithm | Parameters |
|---|---|
| Simplex (0) | `SD=... MODE=... ITERS=... REL=... CONVRATE=... STAGNATION=...` |
| Levenberg (1) | `SD=... MODE=... ITERS=... REL=... ABS=... DER=... DSSV=...` |
| Genetic (2) | `SD=... GENCOUNT=... GENSIZE=... ITERS=... SELRATE=... MUTRATE=... SEED=... MODE=...` |

`SD` = Significant Digits

```python
c.setUseAlgorithm(True)
c.setFitMethod(0)                              # Nelder-Mead Simplex
c.setFitMethodPara("ITERS=500 CONVRATE=1e-6")
```

```python
c.setWeight(True)
c.setWeightMethod(1)   # statistical weighting
```

---

## Logging

### log(text)

```python
c.log("Compilation started")
```

Appends a message to the result log panel.

---

## Complete Examples

### Create a new fit function from scratch

Full pipeline: define metadata, fill the parameter table, write the C++ body, then compile (compile auto-saves).

```python
c = Compiler

# --- identity ---
c.new()
c.setFunctionName("MySphere")
c.setGroup("shape:sphere")
c.setYName("I")
c.setXName("x")
c.setInfo("<p>Sphere form factor. I(Q) = phi * V * (SLD-SLDsolv)^2 * F(Q)^2 + background</p>")

# --- parameters ---
c.setParamCount(5)
c.setParamNamesAll(["R", "SLD", "SLDsolv", "phi", "background"])
c.setParamInitsAll([50.0, 6e-6, 0.0, 0.1, 0.0])
c.setParamLimitsAll(["0.1..1000", "0..1e-4", "0..1e-4", "0..1", ".."])
c.setParamVariesAll([True, True, False, True, True])
c.setParamDescrsAll(["radius nm", "SLD nm^-2", "solvent SLD nm^-2", "volume fraction", "flat background"])

# --- fit function body ---
c.setCode("""
  double qR = x * R;
  double F = (qR > 1e-6) ? 3.0*(sin(qR) - qR*cos(qR)) / (qR*qR*qR) : 1.0;
  double V = 4.0/3.0 * M_PI * R*R*R;
  I = phi * V * (SLD - SLDsolv)*(SLD - SLDsolv) * F*F + background;
""")

# --- compile (auto-saves the .fif, then generates MySphere.cpp and links MySphere.dylib/.so) ---
ok = c.compile()
if not ok:
    scriptPrint(qti.app.resultsLog().toPlainText()[-500:])
    raise RuntimeError("Compilation failed")
```

To also inspect the generated files:

```python
cpp    = c.makeCPP()     # save MySphere.cpp and return C++ source
script = c.makeScript()  # save compile script and return script text
ok = c.compile()         # auto-saves the .fif, then compiles
if not ok: raise RuntimeError(qti.app.resultsLog().toPlainText()[-500:])
```

---

### Extend an existing fit function

Load a function, add a parameter, patch the code, save and recompile.

```python
c = Compiler
c.scanGroups()

# --- load ---
c.open("MySphere")

# --- add 'scale' as a new last parameter ---
n = c.paramCount()       # current count, e.g. 4
c.setParamCount(n + 1)   # extend to 5
c.setParamName(n, "scale")
c.setParamInit(n, 1.0)
c.setParamLimit(n, "0..1e10")
c.setParamVaries(n, True)
c.setParamDescr(n, "absolute scale factor")

# --- patch the [code] body to use the new parameter ---
old = c.code()
new = old.replace(
    "I = phi * V * (SLD - SLDsolv)*(SLD - SLDsolv) * F*F + background;",
    "I = scale * phi * V * (SLD - SLDsolv)*(SLD - SLDsolv) * F*F + background;"
)
c.setCode(new)

# --- save and recompile ---
c.compile()
```

---

### Fill the parameter table

```python
c = Compiler
c.new()
c.setFunctionName("MySphere")
c.setGroup("shape:sphere")
c.setYName("I")
c.setXName("x")
c.setParamCount(3)
c.setParamNamesAll(["R", "SLD", "background"])
c.setParamInitsAll([10.0, 1e-6, 0.0])
c.setParamLimitsAll(["0.1..1000", "0..1e-4", ".."])
c.setParamVariesAll([True, True, False])
c.setParamDescrsAll(["radius nm", "scattering length density", "flat background"])
```

---

### Compile all functions of a category

```python
c = Compiler
c.scanGroups()
c.selectCategory("shape:sphere")
c.compileAll()
```

---

### Compile all functions of a sub-folder

```python
c = Compiler
c.scanGroups()
c.selectCategory("qtiplot/")
c.compileAll()
```

---

### Compile all functions, group by group, with logging

```python
c = Compiler
c.scanGroups()

for grp in c.categories():
    if grp == "ALL":
        continue
    c.log(f"--- {grp} ---")
    functions = c.functionNames(grp)
    for fn in functions:
        c.open(fn)
        c.compile()
        c.log(f"  {fn}")
```

---

### List all available functions

```python
c = Compiler
c.scanGroups()
functions = c.functionNames("ALL")

for fn in functions:
    print(fn)
```

---

### Download and compile the official fit-function library

Clones the official QtiSAS fit-function repository into a temporary folder, then compiles every
function into a shared library. Each step:

- `setPath(testPath)` — switches the library path to the temporary folder
- `download()` — clones the repository (new files only, no dialog)
- `compileAll()` — compiles every `.fif` file into a shared library

```python
import os, tempfile
c = Compiler
testPath = os.path.join(tempfile.gettempdir(), "Functions-Test")
os.makedirs(testPath, exist_ok=True)
c.setPath(testPath)
c.download()
c.compileAll()
```

---

### Import a SasView model and compile

```python
c = Compiler
c.setPath("/data/fitfunctions")
c.openSasView()    # opens file dialog to select .py model
c.save()
c.compile()
```

---

### Combine two sasview models: P(Q) × S(Q) using namespaces

When two sasview model headers both define `Fq()` or `form_volume()`, including both bare causes
redefinition errors. Wrap each header in its own namespace using the single-line syntax
`namespace X {#include "..."}` in `[h-headers]`. The compiler expands each line into a proper
namespace block, so the generated `.cpp` contains:

```cpp
namespace ff {
#include "sasviewmodels/sphere.h"    // exports: static void Fq(...), static double form_volume(...)
}
namespace hs {
#include "sasviewmodels/hardsphere.h"  // exports: double Iq(...)
}
```

Inside `[code]` call the functions with their namespace prefix. Note that `ff::Fq()` is `void` —
it writes the form factor into `F1` and `F2` via pointers; `F2 / ff::form_volume(R)` gives
normalized P(Q).

```python
c = Compiler
c.scanGroups()

# --- read references ---
c.open("sasviewmodels/sphere")
sphere_params = c.paramNames()   # ['scale', 'background', 'sld', 'sld_solvent', 'radius']

c.open("sasviewmodels/hardsphere")
hs_params = c.paramNames()       # ['scale', 'background', 'radius_effective', 'volfraction']

# --- create new function ---
c.new()
c.setFunctionName("SphereHS")
c.setGroup("custom:mixed")
c.setYName("I")
c.setXName("x")
c.setInfo("<p>Sphere P(Q) × hard-sphere S(Q). "
          "P(Q) from sasviewmodels/sphere, S(Q) from sasviewmodels/hardsphere.</p>")

c.setParamCount(6)
c.setParamNamesAll(["scale", "R", "SLD", "SLDsolv", "volfraction", "background"])
c.setParamInitsAll([1.0,  50.0,  6e-6,  0.0,   0.2,   0.0])
c.setParamLimitsAll(["0.001..1e6", "1..10000", "..", "..", "0.001..0.74", ".."])
c.setParamVariesAll([True, True, False, False, True, True])
c.setParamDescrsAll(["scale", "radius (A)", "particle SLD", "solvent SLD",
                     "volume fraction", "background"])

# --- namespace-wrapped headers in h-headers ---
# each line contains "#include" so it passes the filter;
# "namespace X {" prefix is expanded to a proper block by the compiler
c.setHHeaders(
    'namespace ff {#include "sasviewmodels/sphere.h"}\n'
    'namespace hs {#include "sasviewmodels/hardsphere.h"}'
)
c.setIncludedFunctions("")

# --- code body: ff::Fq() is void — it writes into F1, F2 via pointers ---
c.setCode("""
  double q = x;
  double F1, F2;
  ff::Fq(q, &F1, &F2, SLD, SLDsolv, R);
  double PQ = F2 / ff::form_volume(R);
  double SQ = hs::Iq(q, R, volfraction);
  I = scale * PQ * SQ + background;
""")

ok = c.compile()
if not ok:
    scriptPrint(qti.app.resultsLog().toPlainText()[-1000:])
    raise RuntimeError("Compilation failed")
scriptPrint("SphereHS compiled OK")
```

**INCLUDE ONLY WHAT YOU CALL — do not add a namespace header for every reference you opened:**

If Guinier P(Q) is written inline and only S(Q) comes from a sasview model, include only that
model's header — a second unused header wastes compile time and can cause symbol conflicts.

```python
# WRONG — hardsphere.h included but never called; hs:: namespace is dead weight
c.setHHeaders(
    'namespace gp {#include "sasviewmodels/stickyhardsphere.h"}\n'
    'namespace hs {#include "sasviewmodels/hardsphere.h"}'   # unused!
)

# RIGHT — only include what the code body actually calls
c.setHHeaders('namespace hs {#include "sasviewmodels/stickyhardsphere.h"}')
```

**ARG-COUNT RULE — copy the Iq()/Fq() call exactly from the reference `code()` snippet:**

After reading a reference with `c.open("sasviewmodels/stickyhardsphere")` and `c.code()`, the
snippet shows: `Iq(q, radius_effective, volfraction, perturb, stickiness)` — 5 arguments. Your call
must pass all 5 in the same order; only rename, never drop:

```python
# WRONG — drops perturb; clang: too few arguments, expected 5, have 4
double SQ = hs::Iq(q, radius, volfraction, stickiness);

# RIGHT — all 5 args, same order as the reference code() snippet
double SQ = hs::Iq(q, radius, volfraction, perturb, stickiness);
```

This rule is **bidirectional**: every arg in the `Iq()`/`Fq()` call must also be declared — either
in `setParamNamesAll()` or as a local C++ variable. Passing an identifier that is not in the
parameter list gives `undeclared identifier` at compile time.

```python
# reference code() shows: Iq(q, ..., concentration_salt, dielectconst)

# WRONG — dielectconst passed in call but never added to param list
c.setParamNamesAll([..., "concentration_salt"])          # dielectconst missing
hm::Iq(q, ..., concentration_salt, dielectconst)        # error: undeclared identifier 'dielectconst'

# RIGHT — both args declared and passed
c.setParamNamesAll([..., "concentration_salt", "dielectconst"])
hm::Iq(q, ..., concentration_salt, dielectconst)
```

**How the parser handles `namespace X {#include "..."}`:**

The `[h-headers]` writer tests every line for `.contains("#include")`. Lines that also
`.startsWith("namespace ")` have `{`/`}` expanded to newlines before being written into the `.cpp`.
Lines without `#include` are silently dropped. This single-line syntax is the exact intended way to
put namespace-wrapped model headers in `[h-headers]`.

**Include-guard note:** Both `sphere.h` and `hardsphere.h` include `models/kernel_header.c`, guarded
by `#ifndef KERNEL_HEADER_H`. The guard fires on the second include, so `kernel_header.c` content
lands in `namespace ff` only. `hardsphere.c` (inside `namespace hs`) uses only standard C math — it
compiles correctly without the kernel header infrastructure.

---

## Compilation Pipeline

A fit function goes through three stages before the Fittable widget can use it:

```
FIF file  →  C++ source file  →  shared library (.so / .dylib / .dll)
 (text)        (auto-generated)     (loaded at fit time via dlopen)
```

---

### Stage 1 — FIF file (`.fif` / `.2dfif`)

A FIF file is a plain-text source file with tagged sections separated by blank lines.
It is the **only file you author** — everything else is generated from it.

| Tag | Content |
|-----|---------|
| `[group]` | Group name that determines where the function appears in the explorer. Optional inline flags on the same line: `[eFit]`, `[Weight] N`, `[Superpositional] N`, `[Algorithm] N flags`. |
| `[name]` | Function name (becomes the shared-library filename and the `name()` export). |
| `[number parameters]` | Integer — total number of fit parameters. |
| `[description]` | HTML description text (rendered in the UI and stored in `listComments()`). |
| `[x]` | Independent variable name(s). Single name for 1D (e.g. `x`); comma-separated for 2D (e.g. `ix,iy`). Must be a valid C++ identifier. |
| `[y]` | Dependent variable name (e.g. `I`). Must be a valid C++ identifier — it becomes a local `double` declaration inside `functionSANS`. |
| `[parameter names]` | Comma-separated parameter names (e.g. `R,SLD,background`). Each becomes a `thread_local static double` in the generated C++. |
| `[initial values]` | Comma-separated `value[limit]` pairs. Limit formats: `[a..b]` = hard range, `[mean+-sigma]` = Bayesian prior (e.g. `10[0..100]`, `5.2[5.2+-2.1]`). Omit brackets for unconstrained. |
| `[adjustibility]` | Comma-separated `1`/`0` per parameter (`1` = free to vary, `0` = fixed). |
| `[parameter description]` | Comma-separated short descriptions (used in `listComments()`). No escaping — a literal `,` inside one description desyncs the field count and either pops an `"Error: [parameter description]"` dialog or silently shifts later descriptions by one. Use `;` instead of `,` inside description text. |
| `[h-headers]` | Additional `#include` lines added to the generated C++ (e.g. `#include "myhelper.h"`). |
| `[included functions]` | Helper C++ code inserted verbatim above `functionSANS` (utility functions, constants). |
| `[code]` | **The user's fit function body** — C++ snippet inserted inside `functionSANS`. Has access to the x variable and all parameter names as plain locals. The dependent variable name matches `[y]` (e.g. if `[y]` is `y`, write `y = ...;` not `I = ...;`). Do NOT end with `return {yName};` — the boilerplate provides it after `saveParameters`. Use `return {yName};` only as an early exit mid-body; a bare `return;` is a compile error. |
| `[fortran]` | `0` or `1` enable flag, Fortran source filename, forward declarations for `extern "C"` block. |
| `[python]` | `0` or `1` — link Python at compile time. |
| `[after.fit: python]` | `0` or `1` — run a Python hook after each fit. |
| `[after.fit: python code]` | Python snippet executed after fit convergence. |
| `[x.range]` | Custom simulation X range: enable flag, uniform flag, min, max, points, ymin, log-step flag. |
| `[end]` | End-of-file marker. |

---

### Stage 2 — Generated C++ file (`.cpp`)

`makeCPP()` reads the open FIF and writes a self-contained `.cpp` file.
The generated file has the following structure (top to bottom):

**1. Platform export macro**
```cpp
#if defined(_WIN64) || defined(_WIN32)
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT
#endif
```

**2. Standard headers**
Always included: `<math.h>`, `<iostream>`, `<gsl/gsl_vector.h>`, `<gsl/gsl_matrix.h>`, `<gsl/gsl_math.h>`.
Optionally `<Python.h>` when `[python]` is `1`.
Then the user's `[h-headers]` `#include` lines.

**3. Runtime path strings (baked in at generation time)**
```cpp
std::string fitFunctionPath = "/home/user/.config/qtisas/FitFunctions/";
std::string OS = "LINUX";   // "MAC" or "WINDOWS"
```

**4. `functionT` struct**
The parameter-passing contract between the Fittable engine and the function:
```cpp
struct functionT {
    gsl_vector *para;              // current parameter values
    gsl_vector *para_limit_left;   // left bounds
    gsl_vector *para_limit_right;  // right bounds
    gsl_vector_int *para_fit_yn;   // which parameters are free
    double *Q;                     // x-data array pointer (current dataset)
    double *I;                     // y-data array pointer
    double *dI;                    // y-error array pointer
    double *SIGMA;                 // resolution array pointer
    int *listLastPoints;           // per-dataset last-point indices (multi-curve fit)
    int currentM;                  // current dataset index
    int currentFirstPoint;         // first point in current slice
    int currentLastPoint;          // last point in current slice
    int currentPoint;              // current evaluation point
    bool polyYN;                   // polydispersity active
    int polyFunction;              // polydispersity function index
    bool beforeFit;                // lifecycle flag: called before fit starts
    bool afterFit;                 // lifecycle flag: called after fit ends
    bool beforeIter;               // lifecycle flag: called before each iteration
    bool afterIter;                // lifecycle flag: called after each iteration
    double Int1, Int2, Int3;       // scratch integrals (polydispersity)
    int currentInt;                // current integral index
    int prec;                      // display precision
    char *tableName;               // aux table name
    char **tableColNames;          // aux table column names
    int *tableColDestinations;     // aux table column destinations
    gsl_matrix *mTable;            // aux table data
    int currentFunction;           // function index in multi-function fit
};
```

**5. Thread-local globals**
```cpp
thread_local static double x;              // the [x] variable
thread_local static double R, SLD, background;  // the [parameter names]
thread_local static bool initNow;
```

**6. Accessor macros** (map struct fields to short names inside function body)
```cpp
#define Para    ((struct functionT *) ParaM)->para
#define XXX     ((struct functionT *) ParaM)->Q
#define YYY     ((struct functionT *) ParaM)->I
#define beforeFit  ((struct functionT *) ParaM)->beforeFit
// ... and so on for all functionT fields
```

**7. `readParameters(void *ParaM)`**
Copies `gsl_vector_get(Para, i)` into each named thread-local.
Sets `initNow = true` when `beforeFit || afterFit || beforeIter`.

**8. `saveParameters(void *ParaM)`**
Writes thread-locals back via `gsl_vector_set`.
Only runs when a lifecycle flag is active (`beforeFit || afterFit || beforeIter || afterIter`).

**9. Exported metadata functions** (resolved by Fittable at load time)
```cpp
extern "C" MY_EXPORT bool  isThreadSafe()     // always true
extern "C" MY_EXPORT char *name()             // function name string
extern "C" MY_EXPORT char *parameters()       // "P1,P2,...,x"
extern "C" MY_EXPORT char *init_parameters()  // "10[0..100],1e-6,0"
extern "C" MY_EXPORT char *adjust_parameters()// "1,1,0"
extern "C" MY_EXPORT char *paraNumber()       // "3"
extern "C" MY_EXPORT char *listComments()     // HTML + ",,descr1,,descr2"
```

**10. Included functions** (verbatim from `[included functions]`)

**11. Optional Fortran forward declarations**
```cpp
extern "C" { /* [forward declarations from FIF] */ }
```

**12. `functionSANS` — the main entry point**
```cpp
extern "C" MY_EXPORT double functionSANS(double key, void *ParaM)
{
    x = key;               // assign x variable (from [x] field)
    double I;              // declare y variable (from [y] field)
    readParameters(ParaM); // unpack parameters into thread-locals
    gsl_set_error_handler_off();

    /* ---- USER CODE from [code] ---- */
    I = /* ... user formula ... */;
    /* -------------------------------- */

    saveParameters(ParaM); // write back (only during lifecycle hooks)
    return I;              // return y value
}
```

The x variable and all parameter names are usable directly in `[code]` as plain C++ locals.
`initNow` is `true` on the first call of a fit (useful for allocating scratch memory).
The lifecycle flags (`beforeFit`, `afterFit`, `beforeIter`, `afterIter`) let the code run
initialization or teardown logic inside the same `functionSANS` body.

---

### Stage 3 — Compile script (`.sh` / `.ps1`)

`makeScript()` generates a platform-specific script that compiles the `.cpp` into a shared library.

**macOS / Linux** (`.sh`):
```sh
cd "/path/to/FitFunctions/"
GSL="/path/to/qtisas.app/Contents"
export LIBRARY_PATH=$GSL/Frameworks/:$LIBRARY_PATH
export CPLUS_INCLUDE_PATH=$GSL/Resources/:$CPLUS_INCLUDE_PATH

# compile
g++ -fPIC -shared ... MySphere.cpp -o MySphere.o

# optional: compile Fortran helper
gfortran -c helper.f90

# link
g++ -shared MySphere.o -o MySphere.so -lgsl -lgslcblas

# clean up
rm MySphere.o
```

**Windows** (`.ps1`):
```powershell
$PATHFIF = "C:\Users\user\FitFunctions\"
Set-Location $PATHFIF
$vcvars = "C:\...\vcvarsall.bat"

cmd /c "$vcvars && cl /c ... MySphere.cpp /FoMySphere.obj"
cmd /c "$vcvars && link ... MySphere.obj /OUT:MySphere.dll"
Remove-Item "MySphere.obj", "MySphere.lib", "MySphere.exp" -ErrorAction SilentlyContinue
```

The GSL path, Python paths (if `[python]` is enabled), compile flags, and link flags
are all read from the Compiler UI fields at script-generation time and baked into the script.

---

### Runtime loading by Fittable

When a fit starts, the Fittable widget:

1. Resolves the shared library filename: `{functionName}.so` / `.dylib` / `.dll`
2. Loads it with `QLibrary::load()` (wraps `dlopen` / `LoadLibrary`)
3. Resolves these exported symbols: `isThreadSafe`, `name`, `parameters`,
   `init_parameters`, `adjust_parameters`, `paraNumber`, `listComments`, `functionSANS`
4. Allocates a `functionT` struct, fills `para` from the parameter table
5. For each data point calls `functionSANS(x_i, &ft)` and collects the return value
6. During fitting calls `functionSANS` once with `beforeFit=true` (init hook),
   once per iteration with `beforeIter=true`, and once with `afterFit=true` (cleanup hook)

---

### After Fit — three ways to export results

The C++ `afterFit` flag and the FIF `[after.fit: python]` hook are independent mechanisms
that both fire once, when a fit finishes — they can be used separately or together.

**1. C++ → QtiSAS table**, via `tableName` / `tableColNames` / `tableColDestinations` / `mTable`

Guard extra computation with `if (afterFit) { ... }` inside `functionSANS` and fill the aux-table
struct fields — this creates/overwrites a table in the workspace with no Python involved. Unlike
`beforeFit`/`afterFit`/`XXX`/`YYY`, these four fields get **no auto-generated macro** — the
generated boilerplate's accessor-macro list stops right after `Int1`/`Int2`/`Int3` (the struct
always has the 4 fields; only the macro aliases are missing). **They must be defined manually** in
`[included functions]` (inserted above `functionSANS`) before use in `[code]`:

```cpp
// --- [included functions] ---
#define tablename       ((struct functionT *)ParaM)->tableName
#define colNames        ((struct functionT *)ParaM)->tableColNames
#define colDestinations ((struct functionT *)ParaM)->tableColDestinations
#define mTable          ((struct functionT *)ParaM)->mTable

void afterFitTable(void *&ParaM, int length)
{
    double *x_values = new double[length];
    double *profile   = new double[length];
    // ... compute x_values[i], profile[i] for i in [0, length) ...

    tablename = "Density-Profile";
    colNames = (char **)malloc(2 * sizeof(char *));
    colNames[0] = "x-values";
    colNames[1] = "profile";
    colDestinations = new int[2];
    colDestinations[0] = 1;   // 1 = X
    colDestinations[1] = 2;   // 2 = Y
    mTable = gsl_matrix_alloc(length, 2);
    for (int i = 0; i < length; i++)
    {
        gsl_matrix_set(mTable, i, 0, x_values[i]);
        gsl_matrix_set(mTable, i, 1, profile[i]);
    }
    delete[] x_values;
    delete[] profile;
}

// --- [code] ---
extern "C" MY_EXPORT double functionSANS(double key, void *ParaM)
{
    // ... I = ...;
    if (afterFit) afterFitTable(ParaM, 1000);
    saveParameters(ParaM);
    return I;
}
```
`colDestinations` values use the same X/Y role convention as fit curve columns (1 = X, 2 = Y).
The table is then readable as `table("Density-Profile")` once the fit ends.

**2. Python → export to a file**, via `Table.exportASCII` (see `table-python-api`)

Inside `[after.fit: python code]` (option 3 below), read the fit's result table and write it
out — same method as any other table:

```python
t = table("fitCurve-" + model)
t.exportASCII("/path/to/output.dat", "\t", False, False, False)
```

**3. `[after.fit: python]` — auto-executed Python script**

Set `[after.fit: python]` to `1` in the FIF; the `[after.fit: python code]` block then runs
automatically every time a fit converges — no manual `f.plotResults()` call or button click needed.
Typical use: refresh a persistent graph from a table the C++ side already wrote (option 1), or hand
off to a separate Python process for plotting/analysis needing a library not embedded in QtiSAS's
own Python:

```python
# read a table written by the compiled function's afterFitTable() (option 1) and
# refresh a persistent graph in place, instead of creating a new one every fit
t = table("Density-Profile")

win_name = "Graph-Density-Profiles"
existing = [w.objectName() for w in qti.app.windows()]
g = graphWindow(win_name) if win_name in existing else newGraph(win_name)

l = g.activeLayer()
for i in range(l.numCurves()):
    l.removeCurve(0)          # clear previous run's curves first
l.addCurves(t, ("profile",))
l.setAxisTitle(0, "Volume fraction")
l.setAxisTitle(2, "Distance from bilayer center [Å]")
```

```python
# export (option 2) then hand off to an external python3 process
t = table("fitCurve-" + model)
t.exportASCII("/path/to/output.dat", "\t", False, False, False)

import subprocess
subprocess.call("python3 /path/to/external_plot_script.py", shell=True)
```
`subprocess.call` runs a fully separate Python interpreter — useful when the external script
needs packages not available inside QtiSAS's embedded interpreter. It blocks until that
external script exits, so the fit UI stays unresponsive for its duration.

---

---

## For AI: Workflow Rules

**Read this section completely before writing any Compiler code.**

> **NEVER write `c.save()` before `c.compile()` — `compile()` auto-saves the `.fif`.**

### Rule 1 — Read references first, then create immediately

**Before generating a new fit function, read 1-2 existing functions as references,** then proceed
**immediately** to `c.new()` / `c.setCode()` / `c.compile()` in the **same script**.

**Hard constraints — violations cause the task to fail:**
- If exact function names are given in the request: call `c.open(name)` directly. Do NOT call `functionNames()` at all.
- After opening the named references: do NOT call `functionNames()` again, do NOT call `c.open()` again.
- Do NOT search for "example combined functions" or "additional patterns" after reading the references.
- Do NOT stop after printing reference data and wait — the creation code must follow in the same script.
- Do NOT print "ready to create" or any similar checkpoint and stop — that is the same as stopping.
- One script = read references + create function + compile. No exceptions.
- `setParamInitsAll` / `setParamVariesAll` require Python floats/bools — **not strings**.
  `setParamLimitsAll` is the opposite — it requires range **strings** like `"0.."`/`"0..100"`, **not
  raw numbers** — confirmed live: `setParamLimitsAll([0.0, 0.0])` raises `TypeError`.
- If the request says "not save" / "do not save": call `setInfo()` / `setCode()` / etc. but do **NOT** call `save()` or `compile()` afterwards.
- `compile()` **auto-saves** the `.fif` — an explicit `c.save()` before `c.compile()` is **not needed**. The minimal create-and-compile flow is `c.new()` → `c.setFunctionName(…)` → `c.setGroup(…)` → `c.setCode(…)` → `c.compile()`.
- Call `c.save()` **only** when you want to persist changes (metadata, parameter edits) **without recompiling**.
- In `[code]`, use parameter names directly (`radius`, `scale`, …) — **never `P1`, `P2`, `P3`…**.
- When combining two functions, call their exported C functions (`Fq()`, `Iq()`, …) — do not reimplement.

```python
c = Compiler
c.scanGroups()

# === CASE 1: exact names are given in the request ===
# If the user says "use sasviewmodels/barbell and sasviewmodels/hardsphere",
# open them directly — NO search needed.
# If open() returns False the function does not exist: stop and report, do not search for alternatives.
if not c.open("sasviewmodels/barbell"):
    raise RuntimeError("sasviewmodels/barbell not found")
barbell_code     = c.code()
barbell_included = c.includedFunctions()
barbell_headers  = c.hHeaders()
barbell_params   = c.paramNames()

if not c.open("sasviewmodels/hardsphere"):
    raise RuntimeError("sasviewmodels/hardsphere not found")
hs_code    = c.code()
hs_params  = c.paramNames()

# proceed immediately to c.new() / c.setCode() / c.compile()

# === CASE 2: only a keyword is given (name unknown) ===
# Search functionNames("ALL") and filter — do NOT pass wildcards to functionNames().
all_funcs    = c.functionNames("ALL")
barbell_refs = [f for f in all_funcs if "barbell"    in f.lower()]
hs_refs      = [f for f in all_funcs if "hardsphere" in f.lower()]
# If no match: stop and report — do not loop through all functions looking for alternatives.
if not barbell_refs:
    raise RuntimeError("no barbell function found in library")
c.open(barbell_refs[0])   # name may be "sasviewmodels/barbell" — pass as-is
ref_code     = c.code()
ref_included = c.includedFunctions()
ref_headers  = c.hHeaders()
ref_params   = c.paramNames()

# === CASE 3: category is known ===
# functionNames() requires an EXACT category name — filter categories() first.
cats          = [cat for cat in c.categories() if "cylinder" in cat.lower()]
cylinder_refs = c.functionNames(cats[0]) if cats else []

# --- STOP after reading 1-2 references and create the new function ---
# Do NOT loop c.open() over all functions searching for more patterns.
# Iterating over the entire library is forbidden: slow, noisy, and the new function never gets created.
#
# If the request asks to CREATE a new function: the full pipeline (newFIF → setCode → compile)
# MUST be in the same script as the reference reads — do not stop after printing and wait.
# One script: read references + create function + compile.
```

### Why this matters

- GSL functions (`gsl_sf_bessel_J1`, `gsl_sf_bessel_J0`, …) are already linked — never reimplement them.
- **Every GSL function used in `code()` or `includedFunctions()` requires its header in `hHeaders()`.**
  The standard headers (`<math.h>`, `<gsl/gsl_math.h>`, `<gsl/gsl_vector.h>`, `<gsl/gsl_matrix.h>`)
  are always included automatically. Everything else must be added explicitly:

  | GSL functions used | Header to add to hHeaders |
  |---|---|
  | `gsl_sf_bessel_J0`, `gsl_sf_bessel_J1`, `gsl_sf_bessel_Jn`, … | `#include <gsl/gsl_sf_bessel.h>` |
  | `gsl_sf_gamma`, `gsl_sf_lngamma`, … | `#include <gsl/gsl_sf_gamma.h>` |
  | `gsl_sf_erf`, `gsl_sf_erfc`, … | `#include <gsl/gsl_sf_erf.h>` |
  | `gsl_integration_qags`, … | `#include <gsl/gsl_integration.h>` |
  | `gsl_sf_expint_E1`, … | `#include <gsl/gsl_sf_expint.h>` |

- Orientational averages, polydispersity integrals, and resolution convolutions follow established
  patterns in the existing library — copy them, do not reinvent.
- Unit conventions (Å vs nm, SLD in nm⁻², …) are consistent across a category — read them from neighbors.

### Rule 2 — Do NOT copy hHeaders blindly from reference functions

When writing inline code (not calling library header functions), **do not copy `hHeaders()` from reference
functions**. Only add headers for functions your code body actually calls.

**WRONG — copies hardsphere.h even though the code never calls it:**
```python
c.open("sasviewmodels/hardsphere")
hs_headers = c.hHeaders()     # e.g. '#include "hardsphere.h"'

c.new()
c.setHHeaders(hs_headers)     # ERROR: hardsphere.h not found at compile root
c.setCode("... inline SQ formula ...")
c.compile()
```

**RIGHT — only set headers for what the code actually uses:**
```python
c.new()
c.setHHeaders("")              # inline math needs no extra headers — math.h is automatic
c.setCode("... inline SQ formula using sin/cos/pow/M_PI ...")
c.compile()
```

**To call a function from a subfolder header** (e.g. `Iq()` from `sasviewmodels/hardsphere.h`), prefix
the header path with the subfolder — the new function compiles from the library root:

```python
# WRONG — bare path is relative to the subfolder, not the compile root
c.setHHeaders('#include "hardsphere.h"')          # fatal error: hardsphere.h not found

# RIGHT — prefix with the subfolder
c.setHHeaders('#include "sasviewmodels/hardsphere.h"')
```

**Cloning an existing function via `open()` + `setFunctionName()` into a DIFFERENT folder does NOT
rewrite its inherited `hHeaders` — this silently breaks a same-folder-relative include.** `open()`
copies the `hHeaders` text verbatim; `compile()` never corrects it for the new location. A quoted
`#include "foo.h"` resolves relative to the `.cpp` file's OWN folder, which changes the moment
`setFunctionName()` places the function under a different folder than it was `open()`-ed from.
Confirmed live: `open("sasviewmodels/sphere")` inherits `hHeaders` = `'#include "sphere.h"'` — valid
there since that function compiles from `sasviewmodels/sphere.cpp`, colocated with its header — but
renaming to `"ai/SphereSANS_Fit"` moves compilation to a different folder where the same bare
include no longer resolves: `fatal error: 'sphere.h' file not found`.

**WRONG — rename into a new folder, inherited header path goes stale:**
```python
c.open("sasviewmodels/sphere")            # hHeaders inherited verbatim: '#include "sphere.h"'
c.setFunctionName("ai/SphereSANS_Fit")    # different folder — the inherited header path is now wrong
ok = c.compile()                          # fatal error: 'sphere.h' file not found
```

**RIGHT — re-prefix the inherited header with its ORIGINAL folder before compiling:**
```python
c.open("sasviewmodels/sphere")
c.setFunctionName("ai/SphereSANS_Fit")
c.setHHeaders('#include "sasviewmodels/sphere.h"')   # prefix with the folder it came FROM
ok = c.compile()
```

**Or, safer when adapting the logic anyway — pull the code out and rebuild fresh with `c.new()`:**
```python
c.open("sasviewmodels/sphere")            # read-only inspection — EXPLORE-safe
code = c.code()
c.new()
c.setFunctionName("ai/SphereSANS_Fit")
c.setCode(code)
c.setHHeaders('#include "sasviewmodels/sphere.h"')
ok = c.compile()
```

Only matters when the ORIGINAL function's headers are folder-relative includes of its own subfolder
— a function with no `hHeaders` (or only root-level/system includes) is unaffected.

### Rule 3 — `setFunctionName()` before `save()`/`compile()` when saving a modified function under a new name

**`setFunctionName()` MUST be called before `save()` OR `compile()`.** `save()`'s `name` argument only
controls the **file path/stem** — the FIF's internal `[name]` field always comes from whatever
`setFunctionName()` last set (or `open()` loaded), never from `save()`'s argument. `compile()` is
even easier to get wrong: it takes **no name argument at all** (only parameter is `compileAll`) — it
unconditionally uses whatever `setFunctionName()`/`open()` last set.

Skipping this produces a file at the right path with the wrong internal identity (`save()` case), or
**silently overwrites the original function in place** (`compile()` case) — confirmed live: opening
`sasviewmodels/ellipsoid`, fixing its polydispersity normalization, and calling `c.compile()` without
`setFunctionName()` destroyed the original and never created the requested `ellipsoid-correct`.

**WRONG (`compile()`) — no name set anywhere; `compile()` overwrites `ellipsoid` in place:**
```python
c = Compiler
c.open("sasviewmodels/ellipsoid")
c.setCode(fixed_code)
c.compile()
```

**WRONG (`save()`) — file path says `ellipsoid-correct` but `[name]` (and the compiled `name()` symbol) still
say `ellipsoid`:**
```python
c = Compiler
c.open("sasviewmodels/ellipsoid")
c.setCode(fixed_code)
c.save("sasviewmodels/ellipsoid-correct")
```

**RIGHT — `setFunctionName()` FIRST, then `compile()`:**
```python
c = Compiler
c.open("sasviewmodels/ellipsoid")
c.setFunctionName("sasviewmodels/ellipsoid-correct")
c.setCode(fixed_code)
c.compile()
```

**Use `compile()`, NOT `save()`, whenever the new name needs to be fittable.** `save()` (SIP:
`saveFIF()`) only writes the `.fif` **source text** — it never builds a shared library. `compile()`
(SIP: `buildSharedLibrary()`) is the ONLY call that produces the compiled `.dylib`/`.so`/`.dll` that
`Fittable.configure()` actually loads. Confirmed live: `c.open("SphereSANS_Custom")`;
`c.setGroup("custom")`; `c.save("custom_sphere_fit")` — no `setFunctionName()`, no `compile()` — was
then passed to `f.configure("custom_sphere_fit", ...)` → `ValueError: function not found`.
`"custom_sphere_fit"` never became loadable under any name: `[name]` was still `"SphereSANS_Custom"`
(the first mistake above), and even a correctly-renamed `save()`-only copy would have no compiled
library backing it. `save()` alone is only for persisting metadata/parameter edits on a function
**already loaded under its existing compiled name** (see Rule 2 below) — it can never make a NEW
name fittable, with or without `setFunctionName()` first.

The read step (`c.open()`, `c.code()`, `c.paramNames()`, `c.hHeaders()`, `c.includedFunctions()` on
the *original* function) is read-only and belongs in `# EXPLORE`. `setFunctionName()`, `setCode()`,
`save()`, and `compile()` are writes — per the FORBIDDEN-in-EXPLORE table (see `ai-python-api.md`),
they belong only in the action script that follows.

### Rule 4 — `Int1`/`Int2`/`Int3` + `currentInt`: cache Q-independent quantities, correctly under polydispersity

**`Int1`/`Int2`/`Int3` + `currentInt` cache up to 3 Q-INDEPENDENT quantities ONCE per iteration**,
instead of recomputing them for every Q point (and every polydispersity node within it). `currentInt`
is only ever `1`/`2`/`3` during the dedicated `beforeIter` setup call(s) that run once BEFORE the
main per-point loop starts — during normal per-point/per-node evaluation, `currentInt == 0`, so an
`if (currentInt == N) return ...;` branch is skipped and execution falls through to the normal
formula using the cached `IntN`. With polydispersity OFF, that setup call is one direct evaluation of
`[code]`. With polydispersity ON, the engine instead runs the full `polyIntegral()` quadrature for
that setup call (still with `currentInt` held fixed), calling `[code]` once per size-distribution
node and weight-averaging whatever it returns — so `IntN` becomes the TRUE distribution average of
whatever expression you return, not just a value at the central parameter.

```cpp
// WRONG — form_volume(radius) recomputed inside the main per-node call. Averaging
// these per-node ratios gives <F(q,r)^2 / V(r)>, not the correct polydisperse average.
I = F2;
I /= form_volume(radius);

// RIGHT — only true during the beforeIter setup call; skipped during normal
// per-point evaluation (currentInt == 0 there). Int1 caches <V>.
if (currentInt == 1)
    return form_volume(radius);
double meanVolume = Int1;
I = F2;
I /= meanVolume;
```

This generalizes beyond `<V>` — a compound nonlinear expression works too, since what matters is the
engine's per-node weighted average, not whether the expression is linear:

```cpp
// Int2 = <V^2>, correctly polydispersity-averaged — NOT V_mean^2. Only true during
// the beforeIter setup call; the engine calls this once per node (each with that
// node's own radius) during that call and averages the results.
if (currentInt == 2)
{
    double V = form_volume(radius);
    return V * V;
}
double meanV2 = Int2;
```

With polydispersity off there's only one node, so all forms agree — any Q-independent sub-calculation is a
candidate for this caching, purely for performance, even without polydispersity.

**Place every `if (currentInt == N) return ...;` check at the very TOP of `[code]`, before any other
computation.** Since these branches return early, anything computed above them (a form-factor call,
an integral, any real work) runs and is thrown away on every `beforeIter` call — once per
polydispersity node, if polydispersity is on — for nothing.

```cpp
// WRONG — Fq() computed first, then discarded every time currentInt == 1 returns early
double F1, F2;
Fq(q, &F1, &F2, sld, sld_solvent, radius_polar, radius_equatorial);
if (currentInt == 1)
    return form_volume(radius_polar, radius_equatorial);
double meanVolume = Int1;
I = F2;
I /= meanVolume;

// RIGHT — check first; Fq() only runs on the path that actually needs its result
if (currentInt == 1)
    return form_volume(radius_polar, radius_equatorial);
double meanVolume = Int1;

double F1, F2;
Fq(q, &F1, &F2, sld, sld_solvent, radius_polar, radius_equatorial);
I = F2;
I /= meanVolume;
```

### Rule 5 — X-range must cover the function's full dynamic range, but not so wide that an oscillatory function becomes unfittable

The x-range of the DATA used for fitting must be wide enough to constrain every parameter — a
feature that falls entirely outside the sampled range makes its parameters blow up with huge,
correlated errors:
- **Peaked** (Gaussian, Lorentzian): `x_min < centre - 3*width`, `x_max > centre + 3*width`.
- **Decay** (`exp(-b*x) + c`): `x_max >= 3/b` (need 3+ decay lengths to separate amplitude from background).
- **Power-law/plateau**: cover the full transition from steep slope to flat region.

(This is a different concept from `Fit.Control`'s own x-range setting for simulation/plotting —
see the Fit.Control section below.)

**Oscillatory/periodic functions (SANS sphere/cylinder form factors, any `sin(qR)/qR`-style term)
have the OPPOSITE failure mode: too WIDE a range breaks the fit, not too narrow.** A range spanning
many oscillation periods (`qR_max / (2π)` more than roughly 5) makes a gradient-based fit
(Levenberg-Marquardt) fail via "cycle-skipping": if the initial radius guess is off by even
~10-20%, the fitted curve's oscillations drift out of phase with the data after a few cycles, and
the optimizer can't tell which cycle it should be matching — it converges to a nearby WRONG local
minimum instead of the true one, with badly elevated chi²/dof and **no error raised** (the fit
"succeeds," just at a bad point).

Confirmed live: a synthetic sphere dataset with `R = 50` (Å) and `Q` from `0.01` to `4.48` (same
units) gives `qR` up to `224` — about 35 oscillation periods. Fitting from a plausible starting
guess of `R = 40` (only 20% off) converged in 18 iterations to `chi²/dof ≈ 8.5e5` and `R² ≈ 0.06` —
converged-looking but badly wrong, with nothing in the fit's own output flagging it (see the general
"good chi²/dof ≠ converged" caveat in `fittable-python-api.md`, about a different symptom — hitting
the iteration cap — not this cycle-skipping failure).

```python
# WRONG — Q-range implies ~35 oscillation periods (qR_max = 4.48 * 50 = 224); a 20%-off
# starting radius guess all but guarantees the fit lands in the wrong oscillation cycle
for i in range(1, 151):
    q = 0.01 + 0.03 * (i - 1)   # q up to 4.48
...
f.setParamValue("R", 40.0)     # true R is 50 — a plausible guess, but this range punishes it

# RIGHT — keep qR_max to a handful of periods (~20-30) unless the task explicitly asks for
# a wider range, and/or start closer to the expected radius when the range must be wide
for i in range(1, 151):
    q = 0.001 + 0.003 * (i - 1)   # q up to ~0.45 -> qR_max ~= 22, a handful of periods
```

### Rule 6 — Every function authored via `c.new()` goes under the `"ai/"` folder

**Always `setFunctionName("ai/<shortName>")`, never a bare name and never an existing domain
folder** (`sasviewmodels/`, `custom/`, `qtiplot/`, ...) that a real library function might already
use or later collide with. This is the SAME `/`-folder mechanism the library already uses for
`sasviewmodels/sphere`, `custom/sphere`, etc. — `"ai/"` is simply a reserved namespace for it.

**`"ai/"` is both the folder the function is stored in (`pathFIF/ai/<shortName>.fif`) and part of its
name:** the function *is* `"ai/<shortName>"` in every call — `f.configure("ai/MyPeak", ...)`,
`Compiler.delete("ai/MyPeak")`, `f.fitFunctions()`, `table("fitCurve-ai/MyPeak")`. A bare `"MyPeak"` is
another, missing function. Print and compare it with the folder too.

```python
# WRONG — no dedicated folder; risks colliding with a real library name later, and looks
# indistinguishable from a vetted model in future EXPLORE searches
c.new()
c.setFunctionName("sansTest")

# RIGHT
c.new()
c.setFunctionName("ai/sansTest")
```

**Payoff: when a task says "use an EXISTING library model" (the discovery pattern in
`fittable-python-api.md`/`ai-python-api.md`), EXCLUDE names starting with `"ai/"` from the
candidates** — anything there is a prior AI-authored one-off, never a vetted library model.
Confirmed live (twice): a leftover custom function from an earlier session/turn was found by
`fitFunctions()`/`functionNames()` search and mistaken for "the existing model to reuse," instead of
the AI authoring fresh code as the task actually asked. Reserving `"ai/"` and filtering it out of
"existing model" searches removes this confusion at the source.

```python
# WRONG — can match an old "ai/sphere-test" left over from a previous turn, not a real library model
hits = [h for h in c.functionNames("ALL") if "sphere" in h.lower()]

# RIGHT — exclude the AI-authored namespace when looking for a vetted, pre-existing model
hits = [h for h in c.functionNames("ALL") if "sphere" in h.lower() and not h.startswith("ai/")]
```

### Rule 7 — `setGroup()`: `"ai"` for an `ai/` function, an existing group reused exactly

**An existing group is reused EXACTLY (byte-for-byte)** — near-variants (`"SANS"` vs `"sans"` vs `"Sans"`)
fragment the Explorer category tree into duplicates instead of one real group.

An `ai/` function takes `c.setGroup("ai")` — unless the request names a group: then that group, exactly. The group
only sorts the Explorer tree; a guessed domain group is worse than `"ai"`.

**The group is not the folder:** `setGroup("ai")` only files the function under `"ai"` in the Explorer tree and adds
nothing to its name. The name is what `setFunctionName()` gave — `"ai/MyPeak"` — and is used whole in every call;
`"MyPeak"` stays a missing function whatever the group is.

Call `setGroup()` **after** `c.new()` and `setFunctionName()`: `new()` clears the group again, as `scanGroups()`
clears the editor — a group set before `new()` is lost, and `compile()` then fails with "group is empty".

New group names follow the library's existing `":"`-hierarchy convention (e.g. `"shape:sphere"` in
the Quick Start above) — `setGroup()` still rejects `"/"` (`ValueError`, see Rule 3's group note).
`":"` is for group hierarchy and `"/"` is for the function-name folder — two separate mechanisms,
don't mix them.

### Rule 8 — A SAS/SANS/SAXS-style model always needs a free `scale` AND `background` parameter

Any form-factor/structure-factor custom function should always get TWO free "nuisance" parameters
acting on the WHOLE model, independent of the physically-meaningful shape parameters (radius, SLD,
...): a multiplicative `scale` and an additive `background`, both with broad, effectively-unbounded
limits (`"0.."`).

Without a free `scale`, the model's absolute normalization is entirely fixed by whatever the shape
parameters happen to produce — nothing in the fit can correct a mismatch with the data's actual
absolute scale, no matter how correct the shape parameters are. Confirmed live: a compiled function
used a `(sld - 1.0e-6)²` contrast-squared term with `sld` initialized near `6.0e-6`, making the
model's predicted intensity ~10⁻¹¹ in magnitude — but the actual data (generated by a slightly
different formula with no equivalent contrast term) was of order 1. No adjustment to `R`/`sld`
within reasonable limits can bridge a ten-order-of-magnitude scale mismatch: the fit ran without
error but converged to a meaningless result (chi²/dof ≈ 3217, R² ≈ 3.5×10⁻¹⁰) — a free `scale`
parameter would have absorbed the mismatch and let the fit converge on the correct shape parameters
regardless.

**WRONG — no free `scale`; the contrast/SLD term alone fixes the absolute normalization:**
```python
c.setParamCount(3)
c.setParamNamesAll(["R", "sld", "background"])
c.setParamInitsAll([50.0, 6.0e-6, 0.001])
c.setParamLimitsAll(["0.1..500", "0..1e-4", "0..0.1"])
c.setCode("""
  double qR = q * R;
  double F = (qR > 1e-6) ? 3.0 * (sin(qR) - qR * cos(qR)) / (qR * qR * qR) : 1.0;
  double V = 4.0/3.0 * M_PI * R * R * R;
  I = (3.0 * V / (4.0 * M_PI * R * R * R)) * (sld - 1.0e-6) * (sld - 1.0e-6) * F * F + background;
""")
```

**RIGHT — add a free `scale` multiplying the whole model, broad limits on both `scale` and `background`:**
```python
c.setParamCount(4)
c.setParamNamesAll(["scale", "R", "sld", "background"])
c.setParamInitsAll([1.0, 50.0, 6.0e-6, 0.001])
c.setParamLimitsAll(["0..", "0.1..500", "0..1e-4", "0.."])
c.setCode("""
  double qR = q * R;
  double F = (qR > 1e-6) ? 3.0 * (sin(qR) - qR * cos(qR)) / (qR * qR * qR) : 1.0;
  double V = 4.0/3.0 * M_PI * R * R * R;
  I = scale * (3.0 * V / (4.0 * M_PI * R * R * R)) * (sld - 1.0e-6) * (sld - 1.0e-6) * F * F + background;
""")
```

This is standard SAS/SAXS/SANS practice, not a special-case workaround — real instruments' absolute
intensity calibration, sample concentration, and path length all introduce an unknown multiplicative
factor that `scale` exists to absorb. `background` deserves the same broad `"0.."` treatment: an
unnecessarily tight background limit can just as easily block convergence if the true background
sits outside a too-narrow guessed range.

### Rule 9 — Before `compile()`, verify every declared parameter is actually used in `[code]`

A parameter listed in `setParamNamesAll()` but never referenced in the `setCode()` body is a dead
free direction in parameter space: it has zero effect on the model output, so nothing constrains its
"fitted" value or error, and its presence can measurably destabilize the fit (a flatter,
harder-to-navigate parameter space) — with no error raised anywhere, since declaring an unused
parameter is perfectly valid C++/API usage. Confirmed live: a 4-parameter sphere model (`scale`,
`R`, `SLD`, `background`) never referenced `SLD` in its `[code]`; the resulting fit hit the iteration
cap without converging (gradient still ~9 orders of magnitude above tolerance) — `SLD`'s flat,
unconstrained direction is a plausible contributor.

**Before calling `compile()`, self-check every declared name appears as a whole identifier in the
code body** — this is a plain string check the AI can run itself, no new API needed:

```python
import re
code_body = """
  double qR = q * R;
  double F = (qR > 1e-6) ? 3.0 * (sin(qR) - qR * cos(qR)) / (qR * qR * qR) : 1.0;
  I = scale * F * F + background;
"""
param_names = ["scale", "R", "SLD", "background"]

unused = [p for p in param_names if not re.search(r"\b" + re.escape(p) + r"\b", code_body)]
if unused:
    scriptPrint("WARNING: parameter(s) declared but never used in [code]: " + str(unused) +
                " — remove them or add the missing physics before compiling.")
```

If a parameter is genuinely only used indirectly — e.g. cached into `Int1`/`Int2`/`Int3` inside a
`beforeIter`-style hook rather than referenced directly in `[code]` (see Rule 4) — this check will
still flag it as a false positive; that's fine, it's advisory, not a hard gate. Confirm by checking
the hook code too before deciding a flagged name is truly dead. The common case, though, is simply a
parameter declared and then forgotten in the formula — remove it or wire it in before compiling, not
after a puzzling non-convergent fit.
