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

Clones the official QtiSAS fit-function repository into a temporary folder, then compiles every function into a shared library ready for use in the Fittable widget.
Each step:

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

> **xName consistency:** whatever name you pass to `setXName()` is the C++ local declared inside
> `functionSANS`. If you set `"q"`, the variable in `[code]` is `q` — do **not** write `double q = x;`
> (`x` is not declared and gives `undeclared identifier 'x'`).
>
> | `setXName` | What to write in `[code]` |
> |---|---|
> | `"x"` | `double q = x;`  (rename if you prefer `q`) |
> | `"q"` | use `q` directly — no reassignment line needed |

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

**The dependent variable name is whatever `yName()` returns** — it is NOT always `I`.
Always call `c.yName()` (or check `setYName()`) to find the correct name.
If `setYName("y")` was called, write `y = ...;` in the code, not `I = ...;`.

**Parameter names are plain C++ locals — never use `P1`, `P2`, `P3`…**
Every name from `paramNames()` is declared as a local variable automatically.
Use the names directly:

**Never use a C++ reserved keyword as a parameter name** (`switch`, `class`, `new`, `delete`,
`template`, `if`, `for`, `return`, `int`, `void`, `true`, `const`, …) — `setParamName()`/
`setParamNamesAll()` reject these with a `ValueError` naming the bad word. Confirmed live before
that check existed: naming a parameter `"switch"` failed to compile with the *wrong* parameter
blamed (`undeclared identifier 'bg'`, a different, perfectly valid parameter declared later) —
the bad declaration corrupts everything after it, so the error you see is not the error you have.
`setParamNamesAll()` also requires the list length to exactly match `setParamCount()`.
```cpp
// correct — parameters declared by name
double V = M_PI * radius * radius * length;
I = scale * contrast * contrast * V * V * PQ + background;

// WRONG — P1/P2/... are not declared; compile error
I = P1 * P2 * P3;
```

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

**Any other header must be added explicitly.** In particular, every GSL sub-library requires its own header:

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

---

### group()

```python
grp = c.group()    # [group] field of the currently open FIF file
```

Returns the group name stored in the `[group]` tag of the currently open FIF file (shown in the **Group name** editor field).
This is the value that gets saved into the FIF file and determines where the function appears in the explorer.
Returns an empty string if no function is loaded.

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

Selects a category in the explorer list and populates `listViewFunctions`.
Called automatically by `functionNames()`.
Raises `ValueError` if the category name is not found.

---

### functionNames(groupName)

```python
functions = c.functionNames("Spheres")    # select category and return its function names
functions = c.functionNames("ALL")        # select ALL and return every function name
```

Calls `selectCategory(categoryName)` first, then returns the function list from the explorer.
After this call the category is selected in the UI and `open()` + `compile()` work correctly.

Requires an **exact** category name as returned by `categories()` — wildcards and substrings are not accepted.

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

---

### delete(name="", path="")

```python
c.delete()                              # delete currently open function
c.delete("cylinder")                    # delete by name
c.delete("sasviewmodels/cylinder")      # delete from sub-folder
c.delete("cylinder", path="/custom/p")  # delete from custom path
```

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
When `silent=True` (the default) only new files are added automatically, without showing a dialog.
Pass `silent=False` to open the interactive dialog that lets you review and select which files to apply.

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
Returns `True` on success, `False` on failure. Use `qti.app.resultsLog().toPlainText()` to read the full compiler output — **`resultsLog()` is an `ApplicationWindow` method, NOT a Compiler method**.
`compile()` saves the FIF internally first, so it raises the same `ValueError` as `save()` (see
above) if the function name contains `(` or `)`.

> **CRITICAL — `compile()` returns a bool; always check it.**
> Printing a success message unconditionally is wrong — it runs even when compilation failed:
> ```python
> # WRONG — prints "compiled!" even if compile() returned False
> c.compile()
> print("compiled!")
>
> # RIGHT — check the return value
> ok = c.compile()
> if not ok:
>     scriptPrint(qti.app.resultsLog().toPlainText()[-500:])
>     raise RuntimeError("Compilation failed")
> scriptPrint("Compiled OK")
> ```

Self-debugging loop:

```python
for attempt in range(3):
    if c.compile():
        break
    log = qti.app.resultsLog().toPlainText()
    if "gsl_sf_bessel" in log:
        c.setHHeaders(c.hHeaders() + "\n#include <gsl/gsl_sf_bessel.h>")
    elif "gsl_sf_gamma" in log:
        c.setHHeaders(c.hHeaders() + "\n#include <gsl/gsl_sf_gamma.h>")
    else:
        raise RuntimeError(log)
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
`PySAS.h` wraps the Python C API (`PyObject*`, `PyArg_ParseTuple`, …) which is defined in `Python.h`.

> **`c.compile()` will fail with `fatal error: 'Python.h' file not found`** if the Python
> development headers are not on the include path.

The include path is set from the **Python path** field in the Compiler UI
(set once and baked into every compile script). On macOS the correct path is the system Python
framework, e.g. `/opt/homebrew/opt/python@3.x/Frameworks/Python.framework/Versions/3.x/Headers`.

**Do NOT use `subprocess.run([sys.executable, ...sysconfig...])` to find the path** — QtiSAS's
embedded Python lacks dev headers; `sysconfig.get_path('include')` will return empty.

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

**Values must be Python floats or ints — strings cause a `SystemError`:**
```python
c.setParamInitsAll([50.0, 6e-6, 0.0])    # correct
c.setParamInitsAll(["50.0", "6e-6"])      # WRONG — SystemError
```

---

### paramLimit(i) / setParamLimit(i, limit)

```python
lim = c.paramLimit(0)               # e.g. "..", "0..100", "5.2+-2.1"
c.setParamLimit(0, "0..100")        # absolute range [0, 100]
c.setParamLimit(1, "5.2+-2.1")      # Bayesian: mean = 5.2, sigma = 2.1
```

Limit string stored in bracket notation in `[initial values]` (e.g. `10[0..100]`).

| Format | Meaning |
|---|---|
| `".."` | free, no constraint |
| `"a..b"` | absolute range [a, b] |
| `"mean+-sigma"` | Bayesian prior: mean and sigma (e.g. `"5.2+-2.1"`) |

---

### setParamLimitsAll(limits)

```python
c.setParamLimitsAll(["..", "0..1e-4", ".."])
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
| 6 | Variance = Yᵃ∘c^\|Xmax−X\|: w = \|Y\|ᵃ/c^\|Xmax−X\| |

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

Clones the official QtiSAS fit-function repository into a temporary folder, then compiles every function into a shared library ready for use in the Fittable widget.
Each step:

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

When two sasview model headers both define `Fq()` or `form_volume()`, including both bare would
cause redefinition errors. Wrap each header in its own namespace using the single-line syntax
`namespace X {#include "..."}` in `[h-headers]`. The compiler expands each such line into a proper
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

If Guinier P(Q) is written inline and only S(Q) comes from a sasview model, include only that one
model's header. Including a second unused header wastes compile time and can cause symbol conflicts.

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

After reading a reference with `c.open("sasviewmodels/stickyhardsphere")` and `c.code()`, the snippet
shows: `Iq(q, radius_effective, volfraction, perturb, stickiness)` — 5 arguments. Your call must
also pass all 5 in the same order; only rename, never drop:

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

The `[h-headers]` writer in QtiSAS tests every line for `.contains("#include")`. Lines that also
`.startsWith("namespace ")` have `{` and `}` expanded to newlines before being written into the
`.cpp`. Lines without `#include` are silently dropped. This single-line syntax is therefore the
exact intended way to put namespace-wrapped model headers in `[h-headers]`.

**Include-guard note:** Both `sphere.h` and `hardsphere.h` include `models/kernel_header.c`, which
has an `#ifndef KERNEL_HEADER_H` guard. The guard fires on the second include, so
`kernel_header.c` content lands in `namespace ff` only. `hardsphere.c` (inside `namespace hs`)
uses only standard C math — it compiles correctly without the kernel header infrastructure.

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
| `[parameter description]` | Comma-separated short descriptions (used in `listComments()`). |
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

---

## For AI: Workflow Rules

**Read this section completely before writing any Compiler code.**

> **NEVER write `c.save()` before `c.compile()` — `compile()` auto-saves the `.fif`.**

### Rule 1 — Read references first, then create immediately

**Before generating a new fit function, read 1-2 existing functions as references.**
After reading them, proceed **immediately** to `c.new()` / `c.setCode()` / `c.compile()` in the **same script**.

**Hard constraints — violations cause the task to fail:**
- If exact function names are given in the request: call `c.open(name)` directly. Do NOT call `functionNames()` at all.
- After opening the named references: do NOT call `functionNames()` again, do NOT call `c.open()` again.
- Do NOT search for "example combined functions" or "additional patterns" after reading the references.
- Do NOT stop after printing reference data and wait — the creation code must follow in the same script.
- Do NOT print "ready to create" or any similar checkpoint and stop — that is the same as stopping.
- One script = read references + create function + compile. No exceptions.
- `setParamInitsAll` / `setParamVariesAll` require Python floats/bools — **not strings**.
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

**If you do need to call a function from a subfolder header** (e.g. `Iq()` from `sasviewmodels/hardsphere.h`),
the header path must be prefixed with the subfolder, because the new function compiles from the library root:

```python
# WRONG — bare path is relative to the subfolder, not the compile root
c.setHHeaders('#include "hardsphere.h"')          # fatal error: hardsphere.h not found

# RIGHT — prefix with the subfolder
c.setHHeaders('#include "sasviewmodels/hardsphere.h"')
```

### Rule 3 — `setFunctionName()` before `save()`/`compile()` when saving a modified function under a new name

**`setFunctionName()` MUST be called before `save()` OR `compile()`.** `save()`'s `name` argument only controls
the **file path/stem** — the FIF's internal `[name]` field always comes from whatever `setFunctionName()` last
set (or whatever `open()` loaded), never from `save()`'s argument. `compile()` is even easier to get wrong: it
takes **no name argument at all** (its only parameter is `compileAll`) — it unconditionally uses whatever
`setFunctionName()`/`open()` last set, so there's no name-parameter illusion of safety to even get half-right.

Skipping this produces a file at the right path with the wrong internal identity (`save()` case), or **silently
overwrites the original function in place** (`compile()` case) — this has actually happened: opening
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

**RIGHT — `setFunctionName()` FIRST; works correctly for either `compile()` or `save()`:**
```python
c = Compiler
c.open("sasviewmodels/ellipsoid")
c.setFunctionName("sasviewmodels/ellipsoid-correct")
c.setCode(fixed_code)
c.compile()   # or c.save()
```

The read step (`c.open()`, `c.code()`, `c.paramNames()`, `c.hHeaders()`, `c.includedFunctions()` on the
*original* function) is read-only and belongs in `# EXPLORE`. `setFunctionName()`, `setCode()`, `save()`, and
`compile()` are writes — per the FORBIDDEN-in-EXPLORE table (see `ai-python-api.md`), they belong only in the
action script that follows.

### Rule 4 — `Int1`/`Int2`/`Int3` + `currentInt`: cache Q-independent quantities, correctly under polydispersity

**`Int1`/`Int2`/`Int3` + `currentInt` cache up to 3 Q-INDEPENDENT quantities ONCE per iteration**, instead of
recomputing them for every Q point (and every polydispersity node within each Q point). `currentInt` is only
ever `1`/`2`/`3` during the dedicated `beforeIter` setup call(s) that run once, BEFORE the main per-point loop
for that iteration starts — during every normal per-point/per-node evaluation, `currentInt == 0`, so an
`if (currentInt == N) return ...;` branch is simply skipped and execution falls through to the normal formula
using the cached `IntN`. With polydispersity OFF, that setup call is one direct evaluation of `[code]`. With
polydispersity ON, the engine instead runs the full `polyIntegral()` quadrature for that setup call (still with
`currentInt` held fixed), calling `[code]` once per size-distribution node and weight-averaging whatever it
returns — so `IntN` becomes the TRUE distribution average of whatever expression you return for that branch,
not just a value at the central parameter.

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

This generalizes beyond `<V>` — a compound nonlinear expression works too, since what matters is the engine's
per-node weighted average, not whether the expression is linear:

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
computation.** Since these branches return early, anything computed above them (a form-factor call, an
integral, any real work) runs and is thrown away on every `beforeIter` call — once per polydispersity node, if
polydispersity is on — for nothing.

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
