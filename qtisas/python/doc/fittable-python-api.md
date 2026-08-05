# QtiSAS Fittable Python API

Python scripting interface for the **Fittable** fitting widget.
Access via: `f = Fittable`

> **You cannot define or create fitting functions via Python.**
> `configure()` and `setFunction()` select a function **by name** from the pre-compiled Fittable library.
> Passing a formula string such as `"a*x+b"` will raise `ValueError`.
> Use `f.fitFunctions()` to list all available function names.
> Function authoring via Python will be supported in a future release through the **Compiler** interface.

> **Boolean aliases:** QtiSAS defines `true` and `false` as aliases for `True` and `False`.
> Both forms are accepted in scripts: `f.setInstrumentalFit(true)` and `f.setInstrumentalFit(True)` are equivalent.

---

## Quick Start

### Minimal single-dataset fit

```python
f = Fittable

f.configure("Guinier", ["table_I"])
chi2 = f.fit()

r = f.results()
print(r["Rg"])      # (value, error)
```

---

### Multi-dataset global fit

```python
f = Fittable

f.configure("Guinier", ["table_I1", "table_I2", "table_I3"])

f.setParamShared("Rg", True)
f.setParamLimits("Rg", 1.0, 500.0)
f.setFitMethod("LM")

chi2 = f.fit()
print(f.results())
print(f.fitReport())
```

---

## Ordering Rules

The API has two phases separated by function selection:

1. **Pre-function** — dataset count, fit algorithm options
2. **Function selection** — `configure()` or `setFunction()`
3. **Post-function** — datasets, range, weighting, resolution, polydispersity, parameters

`setInstrumentalFit()` and `setInstrument()` may be called at any stage (before or after function selection). When called at stage `"fit"` or later, the UI widgets and parameter tables are updated immediately.

---

## Stage Navigation

The widget has five stages reflecting the current UI page:

| Stage | Meaning |
|-------|---------|
| `"init"` | no function loaded (function-selection page) |
| `"fit"` | function and datasets configured (fitting page) |
| `"simulate"` | results page, Simulate Curve tab active |
| `"results"` | results page, Generate Tables tab active |
| `"batch-fit"` | results page, Batch-Fit tab active |

---

### stage()

```python
print(f.stage())    # e.g. "fit"
```

---

### selectStage(s)

Navigates to the named stage using the widget's own page-transition logic.

```python
f.selectStage("fit")          # go to fitting page
f.selectStage("simulate")     # go to Simulate Curve tab
f.selectStage("results")      # go to Generate Tables tab
f.selectStage("batch-fit")    # go to Batch-Fit tab
f.selectStage("init")         # go back to function-selection page
```

- Returns `True` on success, `False` if already in `init` and `s == "init"`.
- Raises `RuntimeError` if called from `init` stage with any target other than `"init"`.
- Raises `ValueError` for unknown stage names.

---

## State Introspection

### state()

```python
s = f.state()
```

Returns a dict:

| Key | Type | Description |
|-----|------|-------------|
| `stage` | str | current stage (see Stage Navigation) |
| `function` | str | current function name (empty if not set) |
| `datasets` | int | number of configured datasets |
| `selectedDatasets` | list[str] | names of selected dataset columns |
| `fitMethod` | int | 0=Simplex, 1=LM, 2=GenMin |
| `params` | int | number of parameters |
| `instrumental` | bool | SANS resolution/polydispersity active |
| `chi2` | float or None | last Chi²/DoF, or `None` if not yet fitted |
| `dof` | float or None | last degrees of freedom, or `None` if not yet fitted |

---

## Function Discovery

### fitFunctions(pattern="")

```python
f.fitFunctions()             # all available functions
f.fitFunctions("*sphere*")   # wildcard filter (case-insensitive)
f.fitFunctions("Guinier*")   # prefix match
f.fitFunctions("*SANS*")     # substring match
```

Returns sorted list of function names. Wildcards: `*` matches any substring including `/`.

---

### currentFunction()

```python
name = f.currentFunction()
```

---

## Setup

### configure(function, datasets=[]) — recommended

Resets to `init` stage, then runs the full mandatory sequence
`setDatasetCount → setFunction → selectDataset` in one call.

> `function` must be the **name** of a function already in the Fittable library — **not a formula string**.
> Use `f.fitFunctions()` to see all available names.

```python
f.configure("Guinier")                              # single dataset (select manually after)
f.configure("Guinier", ["table_I"])                 # "table_I" = column "I" in table "table"
f.configure("Guinier", ["table_I1", "table_I2"])    # multi-dataset global fit
# datasets are Y-column names ("tablename_colname"), NOT table names — use f.datasets() to list them
```

- Raises `ValueError` if the function or any dataset name is not found.
- Safe to call from any stage — always starts fresh.

> **IMPORTANT: each item in `datasets` is one COMPLETE `"tablename_colname"` string — a 2-item
> list means a 2-DATASET GLOBAL FIT, not "table name + column name" split apart.** Splitting a
> single table+column pair into two list items asks for two entirely separate datasets, each of
> which must independently exist:
>   - WRONG: `f.configure("Guinier", ["data", "I"])` — this requests dataset `"data"` AND dataset
>     `"I"` as two separate fits (neither of which exists — `"I"` alone is never a valid dataset
>     name) — raises `ValueError` on the first missing one, not a hint that the split was wrong.
>   - RIGHT: `f.configure("Guinier", ["data_I"])` — one combined string, column `I` of table `data`.
>   - RIGHT (genuinely two datasets): `f.configure("Guinier", ["data1_I", "data2_I"])` — a global
>     fit across two different tables' `I` columns, each a complete `tablename_colname` string.

---

### Manual setup (alternative to configure)

```python
f.setDatasetCount(3)            # MUST come before setFunction
f.setFunction("Guinier")        # raises ValueError if not found
f.selectDataset(1, "table_I1")  # 1-indexed; raises ValueError if not found
f.selectDataset(2, "table_I2")
f.selectDataset(3, "table_I3")
```

---

## Datasets

> **Dataset names are Y-column names, not table names.**
> A column name has the form `"tablename_colname"` (e.g. `"data_y"` for column `y` in table `data`).
> Pass a table name such as `"data"` and `configure()` will raise `ValueError`.
> Use `f.datasets()` to list all available Y-column names, or `t.colNames()` on the table.

| Function | Description |
|----------|-------------|
| `datasets()` | list of all Y-column names available in the workspace (form: `"tablename_colname"`) |
| `selectedDatasets()` | list of currently selected dataset names |
| `selectedDataset(m)` | name of dataset m (1-indexed) |
| `datasetCount()` | number of configured datasets |
| `setDatasetCount(n)` | set number (call before `setFunction`) |
| `selectDataset(m, name)` | assign dataset to slot m (call after `setFunction`) |

> **`datasets()` is read-only — it lists what's already configured, it cannot register new ones.**
> Calling it with an argument (`f.datasets(["name"])`, intending to "add" a dataset) raises
> `TypeError: datasets(self): too many arguments`; calling it bare afterward (`f.datasets()`)
> then compiles fine but does nothing useful — the return value is discarded, and the fit still
> has zero datasets configured. To actually set datasets, call `configure(function, [...])` again
> with the full list, or use `setDatasetCount(n)` + `selectDataset(m, name)` above.
> ```python
> # WRONG — configure() called with an empty list "to fill in later", then datasets() misused as a setter
> f.configure("SphereSANS", [])
> t = newTable("SphereSANS", 200, 3)
> ...
> f.datasets(["SphereSANS_I"])   # TypeError: datasets(self): too many arguments
> f.datasets()                   # "fixes" the crash, but configures nothing — still 0 datasets
> f.fit()                        # RuntimeError: failed to read data for the selected dataset(s)
>
> # CORRECT — create the table first, then configure() with the real dataset name
> t = newTable("SphereSANS", 200, 3)
> ...
> f.configure("SphereSANS", ["SphereSANS_I"])
> f.fit()
> ```
> Confirmed live.

---

## Fit Range

| Function | Description |
|----------|-------------|
| `setRange(m, min, max)` | set X range for dataset m (1-indexed) |
| `setRange(m, min, max, byIndex=True)` | range by point index instead of X value |
| `setRangeAll(min, max)` | set same range for all datasets |

---

## Fit Algorithm

> **Use LM for all smooth analytical functions — Simplex is WRONG for them.**
> Levenberg-Marquardt (`"LM"`) is the default and converges in fewer than 20 iterations for
> Gaussian, sphere, Guinier, cylinder, and any continuously differentiable model.
> `setFitMethod("Simplex")` or `setFitMethod(0)` uses derivative-free search; for smooth models
> it runs 500–1000 iterations without convergence — do NOT use it for analytical functions.
> Only use Simplex for tabulated or step-function models where derivatives are unreliable.
>
> ```python
> # RIGHT — LM converges in <20 iterations for smooth models; or omit (LM is the default)
> f.setFitMethod("LM")
> ```

### Method selection

| Function | Description |
|----------|-------------|
| `fitMethod()` | returns current index (0=Simplex, 1=LM, 2=GenMin) |
| `setFitMethod(0)` or `setFitMethod("Simplex")` | Nelder-Mead Simplex — **ONLY for non-differentiable/tabulated models** |
| `setFitMethod(1)` or `setFitMethod("LM")` | Levenberg-Marquardt — **default, use for all smooth analytical functions** |
| `setFitMethod(2)` or `setFitMethod("GenMin")` | Genetic minimization |
| `maxIterations()` / `setMaxIterations(n)` | maximum number of iterations |
| `tolerance()` / `setTolerance(v)` | relative convergence tolerance |
| `toleranceAbs()` / `setToleranceAbs(v)` | absolute convergence tolerance |

---

### Levenberg-Marquardt options

| Function | Values |
|----------|--------|
| `setLmControl(n)` or `setLmControl(name)` | `0`/`gradient-scaled`, `1`/`gradient-unscaled`, `2`/`delta-scaled`, `3`/`delta-unscaled` |
| `setLmAdaptStep(bool)` | adaptive step size (restart on constant Chi²) |
| `setLmDerivStep(v)` | derivative step size |

---

### Simplex options

| Function | Values |
|----------|--------|
| `setSimplexVariant(n)` or `setSimplexVariant(name)` | `0`/`nmsimplex2`, `1`/`nmsimplex2rand`, `2`/`simplex` |
| `setSimplexConvRate(n)` | `0`=fast(×10), `1`=normal, `2`=slow(×0.1) |
| `setSimplexRestart(bool)` | restart on constant Chi² |

---

### GenMin options

| Function | Description |
|----------|-------------|
| `setGenMinPopulation(n)` | population size |
| `setGenMinGenomeSize(n)` | genome size |
| `setGenMinGenerations(n)` | number of generations |
| `setGenMinSelectionRate(v)` | selection rate (0..1) |
| `setGenMinMutationRate(v)` | mutation rate (0..1) |
| `setGenMinRandomSeed(n)` | random seed |
| `setGenMinLocalSearch(n)` or `setGenMinLocalSearch(name)` | `0`/`genmin`, `1`/`levenberg`, `2`/`genmin+levenberg` |

---

## SANS Instrument

| Function | Description |
|----------|-------------|
| `instrumentalFit()` | returns True if SANS mode is active |
| `setInstrumentalFit(bool)` | enable/disable SANS resolution and polydispersity |
| `instrument()` | current instrument name |
| `setInstrument(name)` | `"SANS"` (resolution + polydispersity) or `"Back-Scattering"` (resolution only) |

---

## Resolution (post-function)

### Global

| Function | Values |
|----------|--------|
| `setResoFunction(n)` or `setResoFunction(name)` | `0`/`Gauss-SANS`, `1`/`Triangular`, `2`/`Bessel-SANS`, `3`/`Gauss`, `4`/`InFunction` |
| `setResoSpeedControl(n)` | 0=Fastest … 6=Perfect, 7=Custom |
| `setResoWorkspace(n)` | integration workspace size |
| `setResoSigmaLimit(n)` | sigma limit for integration |

---

### Per-dataset (m is 1-indexed)

| Function | Description |
|----------|-------------|
| `resoEnabled(m)` / `setResoEnabled(m, bool)` | enable resolution for dataset m |
| `resoDataset(m)` / `setResoDataset(m, name)` | resolution σ(Q) source for dataset m — column name or built-in option (see below) |

Built-in `name` values (matched by substring, case-sensitive):

| name | σ(Q) |
|------|------|
| `"01%"` | 0.01·Q |
| `"02%"` | 0.02·Q |
| `"05%"` | 0.05·Q |
| `"10%"` | 0.10·Q |
| `"20%"` | 0.20·Q |
| `"ASCII"` | column from the loaded ASCII.1D.SANS file |

Or pass any σ(Q) column name from your data table.

---

## Polydispersity (post-function)

> **Important:** Must be called **after** `configure()` / `setFunction()`. No-op in `init` stage (no function loaded).

### Global

| Function | Values |
|----------|--------|
| `setPolyFunction(n)` or `setPolyFunction(name)` | `0`/`Gauss`, `1`/`Schultz-Zimm`, `2`/`Gamma`, `3`/`Log-Normal`, `4`/`Uniform`, `5`/`Triangular` |
| `setPolySpeedControl(n)` | 0=Fastest … 6=Perfect, 7=Custom |
| `setPolyWorkspace(n)` | integration workspace size |
| `setPolySigmaLimit(n)` | sigma limit |

---

### Per-dataset (m is 1-indexed)

> **Note:** Polydispersity requires instrument `"SANS"`. With `"Back-Scattering"`, these calls are silently ignored.

| Function | Description |
|----------|-------------|
| `polyEnabled(m)` / `setPolyEnabled(m, bool)` | enable polydispersity for dataset m |
| `polyDataset(m)` / `setPolyDataset(m, name)` | polydispersity sigma column for dataset m |

---

## Weighting (post-function)

| Method index | Formula |
|-------------|---------|
| 0 | `1/σ^2` (default, uses error bars) |
| 1 | `1/Y` |
| 2 | `σ` |
| 3 | `1/Y^2` |
| 4 | `1/Y^a` |
| 5 | `1/(c^a + b·Y^a)` |
| 6 | `1/(Y^a · c^(Xmax−X))` |

---

```python
f.setWeightingMethod(0)        # use error bars (default)
f.setWeightingMethod(4)
f.setWeightingA(2.0)           # exponent a for methods 4–6
f.setWeightingB(1.0)           # parameter b for method 5
f.setWeightingC(1.0)           # parameter c for methods 5–6
f.setWeightingXmax(0.3)        # Xmax for method 6

f.setWeightingEnabled(1, True)         # enable weighting for dataset 1
f.setWeightingDataset(1, "table_dI")   # σ column for dataset 1
```

> **RULE — Assigning a column `Table.PlotDesignation.yErr` does NOT enable weighted fitting by itself. You must also call `setWeightingEnabled(m, True)`.**
> Without it, weighting silently defaults to OFF (unweighted, every point weight = 1) — no error, no warning. `chi2`/`f.results()` still return numbers, so nothing *looks* wrong. But chi²/dof then reflects raw, unnormalized residuals rather than residuals-over-σ, so its magnitude is dominated by the absolute scale of the data rather than actual fit quality. Confirmed: a fit with data ~10⁹ in magnitude reported chi²/dof ≈ 1.5×10¹⁴ unweighted vs. ≈ 1.8 with weighting correctly enabled on the exact same fit — thirteen orders of magnitude apart, silently.
>
> **`m` here is a 1-based dataset INDEX (int) — not the `"tablename_colname"` dataset NAME string used almost everywhere else in this API.** That's the one exception, and it's easy to miss given the rest of this API's convention. Confirmed live, both wrong forms, across separate conversations:
> ```python
> f.setWeightingEnabled("Data_I", True)  # WRONG — TypeError: setWeightingEnabled(self, m: int,
>                                         #   enabled: bool): argument 1 has unexpected type 'str'
> f.setWeightingEnabled(0, True)         # WRONG — ValueError: setWeightingEnabled: dataset 0 out
>                                         #   of range (1..N) — valid indices start at 1, not 0
> f.setWeightingEnabled(1, True)         # RIGHT — 1 is the first (or only) configured dataset
> ```
> ```python
> # WRONG — yErr role is set, but weighting is never enabled; chi²/dof will be
> # dominated by absolute data magnitude, not real fit quality — silently.
> t.setColumnRole(3, Table.PlotDesignation.yErr)
> f.configure(model, ["Data_I"])
> chi2 = f.fit()   # chi2 is a real number, but not a meaningful one
>
> # RIGHT — explicitly enable weighting and point it at the error column
> t.setColumnRole(3, Table.PlotDesignation.yErr)
> f.configure(model, ["Data_I"])
> f.setWeightingMethod(0)              # 1/σ² — the standard choice
> f.setWeightingEnabled(1, True)
> f.setWeightingDataset(1, "Data_dI")  # the actual yErr column name
> chi2 = f.fit()
> ```
> If comparing chi²/dof across multiple models or datasets, always confirm weighting is enabled first — otherwise the comparison is meaningless regardless of how good the fits look.

> **IMPORTANT — a good chi²/dof does NOT by itself mean the fit actually finished converging.** `fit()` can stop for several different reasons (converged within tolerance, hit the iteration cap, step size too small, ...) — of these, only "hit the iteration cap" (visible in the console as `iter <N> [<maxIterations>]` with `<N>` equal or very close to `maxIterations()`) genuinely means the optimizer may have been cut off before reaching a true minimum, not that it found one. The reliable, general check: call `fit()` a second time immediately after (same configuration, no changes) — this restarts the optimizer from the just-converged parameters. If chi²/dof barely changes, the fit had already genuinely settled and the result is trustworthy; if the second call's chi²/dof drops meaningfully further, the first run was NOT actually finished. This applies regardless of which specific model or parameter shape is involved — unlike checking a parameter's value against the data's range, which depends on domain judgment and can be flat wrong (a fitted peak center legitimately CAN sit outside the sampled range when the visible tail/wing still constrains it with a tight error bar — that is not a sign of a bad fit).
> ```python
> chi1 = f.fit()
> chi2 = f.fit()  # re-run from the just-converged parameters — cheap, and confirms convergence
> if abs(chi2 - chi1) > 1e-6 * max(abs(chi1), 1e-300):
>     scriptPrint(f"Fit may not have fully converged: chi2/dof went from {chi1} to {chi2} on a "
>                 f"repeat fit — consider raising maxIterations (currently {f.maxIterations()}).")
> else:
>     scriptPrint(f"Fit converged: chi2/dof = {chi2}, R2 = {f.r2()}")
> ```

> **IMPORTANT — `f.lastChi2` and `f.results()` reflect the CURRENT fit state, not a history — reading them again after a second `fit()` call does NOT give you the "before" values.** There is only ever one live result; a second `fit()` overwrites it in place. To compare before-vs-after (e.g. "raise maxIterations and compare with the last result"), you must capture the "before" snapshot into local variables **before** calling `fit()` again — reading `f.lastChi2`/`f.results()` afterward, expecting them to still hold the old values, silently gives you the SAME (new) values twice under different labels, with no error and no warning that the comparison never actually happened.
> ```python
> # WRONG — both "old" reads happen AFTER fit() already overwrote the state, so
> # chi2_old ends up identical to chi2_new — this is not a real before/after comparison
> f.setMaxIterations(1000)
> chi2_new = f.fit()
> r_new = f.results()
> chi2_old = f.lastChi2   # already overwritten — this is the NEW value, not the old one
> r_old = f.results()      # same mistake
>
> # RIGHT — capture "before" state first, using the values already in hand
> chi2_before = f.lastChi2   # or the chi2 returned by the PREVIOUS fit() call, if still in scope
> r_before = f.results()
> f.setMaxIterations(1000)
> chi2_after = f.fit()        # fit()'s own return value is always the freshest "after" reading
> r_after = f.results()
> ```
> This is the same underlying gotcha as the convergence check above — `fit()`'s own return value is always safe to use as a distinct snapshot (it's a fresh value each call), while `lastChi2`/`results()` are only safe to read once, at the moment you actually need "the current state," never as a stand-in for "the state from before the call I just made."

---

## Parameters

> **IMPORTANT — exact parameter names required.**
> `setParamValue`, `setParamVaries`, `paramValue`, and related methods require names **exactly** as returned by `paramNames()` — case-sensitive, no abbreviations.
> An unknown name raises `ValueError: setParamValue: no such parameter '<name>'. Available: <comma-separated real names>.` (or the equivalent for `setParamVaries`/`paramValue`) — it does not silently fail, and the error itself already names every valid parameter, so a correction never has to guess.
> `paramValue` — the READ accessor — used to silently return `nan` for an unknown name instead of raising, unlike the setters; this was fixed so a wrong guess (e.g. `paramValue("center")` for a function whose real parameter is `"x0"`) is caught immediately instead of reporting a misleading `nan` as if it were a real result.
> Always discover names in an EXPLORE step: `scriptPrint(str(f.paramNames()))` — then use those exact strings.
> Example: for `sasviewmodels/sphere` the names are `['scale', 'background', 'sld', 'sld_solvent', 'radius']` —
> `setParamValue("R", ...)` or `setParamValue("SLD", ...)` raise `ValueError`, they don't silently do nothing.

> **IMPORTANT — not all parameters are adjustable (fittable).**
> Each model defines which parameters can be fitted. After `configure()`, `paramVaries(name)` reflects
> the model's built-in adjustability — it returns `False` for parameters the model marks as non-adjustable
> (e.g. in some SANS-fit functions only `scale` is adjustable while `SLD1`/`SLD2` are fixed by the model).
> **Always call `paramVaries(name)` after `configure()` to discover which parameters are actually fittable.**
> Calling `setParamVaries("sld1", True)` on a non-adjustable parameter has no effect.

All `dataset` arguments are **1-indexed** (1 = first dataset).
For `setParamValue` and `setParamVaries`, `dataset=-1` (default) applies to **all datasets**.
For limits, `dataset=-1` means the global (shared) limit.

---

### Reading parameters

```python
names  = f.paramNames()          # list of parameter names
n      = f.paramCount()          # number of parameters

val    = f.paramValue("Rg")          # value, dataset 1
val2   = f.paramValue("Rg", 2)       # value, dataset 2
err    = f.paramError("Rg")          # fit error (NaN if fixed or not yet fitted)
varies = f.paramVaries("Rg")         # True if free parameter
shared = f.paramShared("Rg")         # True if shared across datasets

lL  = f.paramLimitLeft("Rg")         # global left limit
lR  = f.paramLimitRight("Rg")        # global right limit
lL2 = f.paramLimitLeft("Rg", 1)      # local left limit for dataset 1
bay = f.paramBayesian("Rg")          # True if Bayesian prior active
```

---

### Setting parameters

```python
f.setParamValue("Rg", 50.0)           # set value, all datasets (default)
f.setParamValue("Rg", 50.0, 2)        # set value, dataset 2 only
f.setParamVaries("bgd", False)         # fix parameter, all datasets (default)
f.setParamVaries("bgd", False, 1)      # fix parameter, dataset 1 only
f.setParamShared("Rg", True)           # share across datasets (global fit)

# bulk setters — values list ordered as paramNames()
f.setParamValueAll([50.0, 0.001, 1.0])        # set all param values, all datasets
f.setParamValueAll([50.0, 0.001, 1.0], 2)     # set all param values, dataset 2 only
f.setParamVariesAll([True, False, True])       # set varies for all params, all datasets
f.setParamVariesAll([True, False, True], 1)   # set varies for all params, dataset 1 only

# global limits
f.setParamLimits("Rg", 1.0, 500.0)

# global Bayesian prior
f.setParamLimits("Rg", 45.0, 55.0, bayesianYN=True)

# local limits for dataset 1 only
f.setParamLimits("Rg", 1.0, 500.0, dataset=1)

# local Bayesian for dataset 2
f.setParamLimits("bgd", 0.0, 0.01, bayesianYN=True, dataset=2)

# unbounded / one-sided — use float('-inf')/float('inf'), NOT a bracket-notation string like ".."
# (that syntax belongs to Compiler.setParamLimit(), not Fittable.setParamLimits() — the two are
# unrelated APIs with different signatures, even though both are called "parameter limits")
f.setParamLimits("Rg", float('-inf'), float('inf'))   # fully free, no constraint
f.setParamLimits("A1", 0.0, float('inf'))              # lower-bounded only (e.g. amplitude >= 0)
```

`paramLimitLeft()`/`paramLimitRight()` return `float('-inf')`/`float('inf')` for a side that's
unbounded — compare with `== float('-inf')` / `== float('inf')`, not by testing for `None` or an
empty string.

---

## Execute

| Function | Stage required | Returns | Description |
|----------|---------------|---------|-------------|
| `fit()` | `"fit"` | float | run fit; returns Chi²/DoF; prints `chi²/dof`, `R²`, `Q` to console; raises `RuntimeError` if the selected dataset(s) can't be read (e.g. an empty table) |
| `r2()` | `"fit"` | float | R² = 1 − χ²/TSS after last fit or simulate |
| `qFactor()` | `"fit"` | float | goodness-of-fit probability Q(dof/2, χ²/2); meaningful only with instrumental (error-bar) weighting |
| `simulate()` | `"fit"` | float | plot fit curves for all datasets |
| `plotResults(graphName="", graphTemplate="")` | `"fit"` | MultiLayer | plot data + fit curves in a named graph; skips curves already present |
| `simulateCurve()` | `"simulate"` | float or NaN | run single-curve simulation (Simulate Curve tab) |
| `beforeFit()` | `"fit"` | — | called internally by `fit()` — do NOT call manually |
| `afterFit()` | `"fit"` | — | called internally by `fit()` — do NOT call manually |
| `curveColor()` | any | int | current output-curve color index (used by fit, simulate, simulateCurve, plotResults) |
| `setCurveColor(index)` | after `configure()` | bool | set output-curve color by integer index; returns `False` if out of range or called in `"init"` stage |
| `setCurveColor("name")` | after `configure()` | bool | set output-curve color by partial name (case-insensitive); prints available names on failure; call after `configure()` and before `fit()` / `plotResults()` |
| `setScaleErrors(bool)` | — | — | scale parameter errors by √(Chi²/DoF) |
| `setStatisticsNotes(bool)` | — | — | save covariance matrix to a Note after fit |
| `setSaveSession(bool)` | — | — | save session to a Note after fit |

`fit()`, `simulate()`, `plotResults()`, `beforeFit()` and `afterFit()` raise `RuntimeError` if stage is not `"fit"`.
`simulateCurve()` raises `RuntimeError` if stage is not `"simulate"`.

After `fit()` and after `simulate()`, fit curve tables are created in the workspace:
- Single dataset: `fitCurve-<function>`
- Global fit dataset N: `fitCurve-<function>-global-N`

**Fit curve table columns** (column index is 1-based for Python cell access):

| Col | Label | Role | Contents |
|-----|-------|------|----------|
| 1 | `x` | X | Q values (same as input data) |
| 2 | `y` | Y | fitted curve I(Q) |
| 3 | `weight` | yErr | fit weight per point |
| 4 | `sigma` | xErr | resolution σ(Q) |
| 5 | `residues` | Y | normalised residuals (data − fit) / σ |
| 6+ | `Characteristics`, `Conditions`, `Parameters`, `Values`, `Errors` | None | fit metadata |

To access the fit curve from Python after `fit()`:

```python
fc = table("fitCurve-" + model)     # model is the string passed to configure()

# add fit line to an existing layer g:
g.insertCurve(fc, "x", "y", Graph.CurveType.Line)

# add residuals to a second layer gr:
gr.insertCurve(fc, "x", "residues", Graph.CurveType.Scatter)

# read fit value at row idx (1-based):
i_fit = fc.cell(2, idx)    # col 2 = "y"
resid = fc.cell(5, idx)    # col 5 = "residues"
```

---

### plotResults(graphName="", graphTemplate="")

Always plots into **layer 1** (data scatter + fit line).  
If the MultiLayer has a second layer and `radioButtonSameQrange` is checked, residuals (scatter) are added to **layer 2** and the X axes are linked.  
Existing curves are never duplicated — repeated calls are safe.

**Curve order/index in layer 1, per dataset: the DATA scatter is inserted first, the FIT line second.**
For a single dataset that's curve index **0 = data scatter**, index **1 = fit line** — NOT the other way
round, and NOT "the only curve = the line". Styling curve `0` via `Graph.setCurveLineColor()` after
`plotResults()` recolors the raw data points, not the fit. To style the fit line's color, prefer
`f.setCurveColor(...)` (see above) called *before* `fit()`/`plotResults()` — it's the purpose-built API
and already applies correctly when the curve is created. Only reach for `g.setCurveLineColor(1, ...)`
(index **1**, not `0`) if you need something `setCurveColor()` can't express, e.g. an exact `QColor`
outside its named palette.

```python
# WRONG — index 0 is the DATA scatter; this recolors the raw points, not "the line"
f.fit()
f.plotResults("FitResult")
g = graphWindow("FitResult").activeLayer()
g.setCurveLineColor(0, colorIndex("orange"))   # recolors data points, fit line untouched by this call

# RIGHT — set the fit curve's color via Fittable's own API, before fit()/plotResults()
f.setCurveColor("orange")
f.fit()
f.plotResults("FitResult")
```

Confirmed live: a request to "set line color to orange" was fulfilled correctly via `f.setCurveColor
("orange")`, called in the right place — but the script ALSO added a redundant `g.setCurveLineColor
(0, colorIndex("orange"))` afterward, which doesn't touch the fit line at all and instead recolors the
data scatter to match it, unintentionally making data and fit visually blend together.

| Parameter | Description |
|-----------|-------------|
| `graphName` | window name; reuses existing graph or creates new one if not found / empty |
| `graphTemplate` | template file stem (without `.qpt`); applied when creating a new graph |

```python
# plain new graph
f.plotResults("FitResults")

# graph styled from templates/graph.qpt
f.plotResults("FitResults", "graph")

# re-running is safe: existing curves are kept, missing ones are added
f.fit()
f.plotResults("FitResults", "graph")

# plotResults returns the MultiLayer — use it to style the plot
gw = f.plotResults("FitResults")
g = gw.activeLayer()
g.setXTitle("q, 1/nm")
g.setYTitle("I(q), 1/cm")
g.setLinOrLogAxis(Graph.Axis.Left, True)   # log Y
```

---

## Simulate Curve tab

`simulateCurve()` corresponds to the **Simulate Curve** tab; call `selectStage("simulate")` first.

Two modes: **uniform X** (generates a new X grid) and **dataset** (uses X values of an existing dataset and returns Chi²/DoF).

---

### Mode selection

| Function | Description |
|----------|-------------|
| `simUniformX()` | returns True if uniform X mode is active |
| `setSimUniformX(True)` | uniform X mode: range defined by From / To / N points |
| `setSimUniformX(False)` | dataset mode: use the X values of the selected dataset |

---

### Uniform Q mode

| Function | Description |
|----------|-------------|
| `simFromX()` / `setSimFromX(v)` | X start |
| `simToX()` / `setSimToX(v)` | X end |
| `simNumPoints()` / `setSimNumPoints(n)` | number of points |
| `simLogStep()` / `setSimLogStep(bool)` | logarithmic spacing |

---

### Dataset mode

| Function | Description |
|----------|-------------|
| `simDataset()` | name of the currently selected dataset |
| `setSimDataset(name)` | select dataset by name (partial match); prints available names on failure |

`simulateCurve()` returns Chi²/DoF in dataset mode; returns `NaN` in uniform X mode.

---

### Example

```python
f.selectStage("simulate")

# uniform X: generate model curve over a custom range
f.setSimUniformX(True)
f.setSimFromX(0.005)
f.setSimToX(0.5)
f.setSimNumPoints(200)
f.setSimLogStep(True)
f.simulateCurve()

# dataset mode: simulate and compare to existing data
f.setSimUniformX(False)
f.setSimDataset("table_I")
chi2 = f.simulateCurve()
print(f"simulation Chi²/DoF = {chi2:.4f}")
```

---

## Results

### Scalar results

```python
chi2 = f.lastChi2    # Chi²/DoF from last fit
dof  = f.lastDoF     # degrees of freedom
```

---

### results() — structured dict

```python
r = f.results()
```

- **Single dataset:** `{ name: (value, error), ... }` — `error` is `None` for fixed parameters.
- **Multi-dataset:** `{ name: [(v1, e1), (v2, e2), ...], ... }`

```python
# single dataset
rg_val, rg_err = r["Rg"]
if rg_err is None:
    print("Rg is fixed")

# multi-dataset
for m, (v, e) in enumerate(r["Rg"], start=1):
    print(f"DS{m}: Rg = {v:.3f} ± {e:.3f}")
```

---

### fitReport() — formatted text

```python
print(f.fitReport())
```

Returns the full fit statistics and covariance matrix as a formatted string.
`Fittable.lastFitReport` holds the same string as a plain attribute.

---

## Error Handling

| Exception | Raised by | Condition |
|-----------|-----------|-----------|
| `RuntimeError` | `fit()`, `simulate()`, `plotResults()`, `beforeFit()`, `afterFit()` | stage is not `"fit"` |
| `RuntimeError` | `fit()` | selected dataset(s) could not be read (e.g. an empty table) — previously returned a stale chi²/dof from a prior fit with no indication anything was wrong |
| `RuntimeError` | `simulateCurve()` | stage is not `"simulate"` |
| `RuntimeError` | `selectStage(s)` | in `init` stage and `s` ≠ `"init"` |
| `ValueError` | `setFunction`, `selectDataset`, `configure` | name not found |
| `ValueError` | `selectStage(s)` | unknown stage name |

When a name is not found, available names are printed to the script console before raising.

---

## Complete Examples

### SANS fit with resolution

```python
f = Fittable

# instrument — may be called before or after configure()
f.setInstrumentalFit(True)
f.setInstrument("SANS")    # "SANS" or "Back-Scattering"

# configure: resets to init, loads function, selects dataset
f.configure("sphere_SANS", ["table_I"])

# resolution for dataset 1
f.setResoFunction("Gauss-SANS")
f.setResoSpeedControl(3)
f.setResoEnabled(1, True)
f.setResoDataset(1, "table_sigma")   # or "10%" for 10% dQ/Q

# parameters
f.setParamValue("R", 50.0)
f.setParamLimits("R", 1.0, 1000.0)
f.setParamVaries("bgd", False)

# fit
chi2 = f.fit()
print(f"Chi²/DoF = {chi2:.4f}")
print(f.results())
```

---

### Iterative multi-method fit

```python
f = Fittable
f.configure("Guinier", ["table_I"])

f.setFitMethod("Simplex")
f.setMaxIterations(500)
chi2 = f.fit()

if chi2 > 10.0:
    f.setFitMethod("LM")
    chi2 = f.fit()

if chi2 > 10.0:
    f.setFitMethod("GenMin")
    f.setGenMinGenerations(200)
    chi2 = f.fit()

print(f"Final Chi²/DoF = {chi2:.4f}")
r = f.results()
for name in f.paramNames():
    v, e = r[name]
    if e is not None:
        print(f"  {name:12s} = {v:.5E} ± {e:.2E}")
    else:
        print(f"  {name:12s} = {v:.5E}  (fixed)")
```

---

### Global fit with shared and local parameters

```python
f = Fittable
f.configure("Guinier", ["table_I1", "table_I2", "table_I3"])

# Rg shared across all datasets, I0 fitted independently
f.setParamShared("Rg", True)
f.setParamLimits("Rg", 1.0, 500.0)

chi2 = f.fit()
r = f.results()

rg_val, rg_err = r["Rg"][0]
print(f"Rg = {rg_val:.3f} ± {rg_err:.3f}")

for m, (v, e) in enumerate(r["I0"], start=1):
    print(f"I0[DS{m}] = {v:.4E} ± {e:.2E}")
```

---

### Fit, plot results, then simulate on custom X grid

```python
f = Fittable
f.configure("Guinier", ["table_I"])
chi2 = f.fit()
print(f"fit Chi²/DoF = {chi2:.4f}")

# plot with template; layer 1 = data+fit, layer 2 = residuals (if present)
f.plotResults("FitResults", "graph")

# navigate to Simulate Curve tab and run simulation on custom X grid
f.selectStage("simulate")
f.setSimUniformX(True)
f.setSimFromX(0.001)
f.setSimToX(1.0)
f.setSimNumPoints(500)
f.setSimLogStep(True)
f.simulateCurve()
```

---

### Collect fit results into a Table

```python
datasets = ["sample1_I", "sample2_I", "sample3_I"]
results  = []

f = Fittable
for ds in datasets:
    f.configure("Guinier", [ds])
    f.setParamLimits("Rg", 1.0, 500.0)
    chi2 = f.fit()
    r    = f.results()
    rg_v, rg_e = r["Rg"]
    i0_v, i0_e = r["I0"]
    results.append((ds, chi2, rg_v, rg_e, i0_v, i0_e))

t = newTable("FitSummary", len(results), 6)
t.setColNames(["dataset", "chi2", "Rg", "dRg", "I0", "dI0"])
t.setColumnRole(1, Table.Label)
for row, (ds, chi2, rg_v, rg_e, i0_v, i0_e) in enumerate(results, start=1):
    t.setText(1, row, ds)
    t.setCell(2, row, chi2)
    t.setCell(3, row, rg_v)
    t.setCell(4, row, rg_e)
    t.setCell(5, row, i0_v)
    t.setCell(6, row, i0_e)
```

---

### Sequential refinement (Simplex seed → LM polish)

```python
f = Fittable
f.configure("sphere_SANS", ["table_I"])

f.setParamValue("R", 40.0)
f.setParamValue("bgd", 0.001)
f.setParamVaries("bgd", True)

# broad exploration with Simplex
f.setFitMethod("Simplex")
f.setMaxIterations(2000)
f.fit()

# precise refinement with LM
f.setFitMethod("LM")
chi2 = f.fit()
print(f"R = {f.paramValue('R'):.2f} ± {f.paramError('R'):.2f}")
print(f"Chi²/DoF = {chi2:.4f}")
```

---

### Weighting and range example

```python
f = Fittable
f.configure("PowerLaw", ["table_I"])

# fit only the Guinier–Porod crossover range
f.setRange(1, 0.02, 0.15)

# weight by error bars from table_dI
f.setWeightingMethod(0)
f.setWeightingEnabled(1, True)
f.setWeightingDataset(1, "table_dI")

chi2 = f.fit()
f.plotResults("PowerLaw_fit", "graph")
print(f.results())
```

---

### Global fit: inspect per-dataset results

```python
f = Fittable
f.configure("Guinier", ["ds1_I", "ds2_I", "ds3_I"])

f.setParamShared("Rg", True)               # Rg common to all datasets
f.setParamLimits("Rg", 1.0, 300.0)

chi2 = f.fit()
r    = f.results()

print(f"Shared Rg = {r['Rg'][0][0]:.3f} ± {r['Rg'][0][1]:.3f}")
print()
for m in range(1, f.datasetCount() + 1):
    v, e = r["I0"][m - 1]
    print(f"  DS{m}  I0 = {v:.4E} ± {e:.2E}")

f.plotResults("GlobalFit", "graph")
```

---

### beforeFit / afterFit workflow

```python
f = Fittable
f.configure("Guinier", ["table_I"])

# load saved parameter values from the UI table into the solver
f.beforeFit()

chi2 = f.fit()

# write fitted values back into the UI parameter table
f.afterFit()

print(f.fitReport())
```

`f.beforeFit()` (same as pressing **Before Fit** in the Fittable interface) does more than sync the
UI table — it also calls the compiled fit function once with its `beforeFit` lifecycle flag set,
*before* the fit itself runs. If the function defines a `beforeFitting(void *ParaM)` helper in its
`[included functions]` section (see compile-python-api.md's `includedFunctions()`), that's the hook
where it can compute its own initial parameter values from the loaded data instead of relying on
fixed numbers from `setParamInitsAll()`.

---

### Parameter constraints: checkbox-level vs in-code

Two independent mechanisms can make a parameter "fixed" or "linked" — don't confuse them:

| | Decided | Mechanism | Needs code? |
|---|---|---|---|
| **Not varying** | above the function, by Fittable | `setParamVaries(name, False)` — optimizer never touches it | No |
| **Shared** | above the function, by Fittable | `setParamShared(name, True)` — forced equal to dataset 1's value, for every other dataset | No |
| **In-code constrained** | inside the compiled function | any transform written in `[code]` (e.g. `Rg = fabs(Rg);`) | Yes |

The first two are checkbox flags Fittable resolves itself, before the compiled function ever runs
— there is nothing to write in `[code]` for either. The third is arbitrary — the compiled function
can do anything to its own parameters. It runs on **every** call (every point, every iteration) so
the returned value is always physically consistent, but the *persisted* result (what ends up back
in the optimizer's vector and in the UI table) only updates during the two `beforeFit()`/`afterFit()`
calls above — not on the many ordinary per-point calls in between.

**Ordering constraint on two peaks (`x1 < x2`) — swap the whole parameter group, not just the compared variable.**
If an iteration proposes `x1 > x2`, swapping only the positions leaves `amplitude1`/`width1`
attached to the wrong peak:
```cpp
// WRONG — only the positions swap; amplitude1/width1 now describe the wrong peak
if (x2 < x1) { double t = x1; x1 = x2; x2 = t; }
```
```cpp
// RIGHT — swap every parameter that belongs to each peak, together
if (x2 < x1)
{
    double t;
    t = x1;         x1 = x2;         x2 = t;
    t = amplitude1;  amplitude1 = amplitude2;  amplitude2 = t;
    t = width1;      width1 = width2;          width2 = t;
}
```
Place this as the first lines of `[code]`, before the formula uses any of these names.

---

### Parameter sensitivity scan

```python
import numpy as np

f = Fittable
f.configure("Guinier", ["table_I"])
f.setParamVaries("Rg", False)   # hold Rg fixed at each probe point

chi2_list = []
rg_values = np.linspace(10.0, 200.0, 20)

for rg in rg_values:
    f.setParamValue("Rg", float(rg))
    chi2 = f.fit()
    chi2_list.append(chi2)

# store in a table for plotting
t = newTable("RgScan", len(rg_values), 2)
t.setColNames(["Rg", "chi2"])
t.setColumnRole(1, Table.X)
t.setColumnRole(2, Table.Y)
for i, (rg, chi2) in enumerate(zip(rg_values, chi2_list), start=1):
    t.setCell(1, i, float(rg))
    t.setCell(2, i, chi2)
```

---

---

## For AI: Common Mistakes

### Rule 1 — Never hardcode a function name from the library

Function names depend on what is compiled into the running Fittable library and may differ between
installations. **Exception:** if the script itself compiled the function via `Compiler`, the name is
already known — no discovery needed. For all other cases, discover first.

**WRONG:**
```python
f.configure("sasviewmodels/sphere_SANS", ["table_I"])   # hardcoded — may not exist
```

**RIGHT — discovery pattern (required when using a pre-existing model):**
```python
f = Fittable
hits = f.fitFunctions("*sphere*SANS*")   # most specific pattern first
if not hits:
    hits = f.fitFunctions("*sphere*")    # broaden on miss
if not hits:
    print("No sphere model found. SANS-related models:")
    print(f.fitFunctions("*SANS*"))
    raise RuntimeError("No suitable model found in Fittable library")
model = hits[0]
print(f"Model: {model}")
if len(hits) > 1:
    print(f"Also available: {hits[1:]}")
# use `model` as the function name — never a hardcoded string
f.configure(model, ["table_I"])
```

If the expected model is not found: print the search results and raise an error — do not silently
fall back to a wrong model.

### Rule 2 — Never guess whether an explicitly named table/dataset exists

When a request names a specific table (e.g. "table SAXS_data column I"), the derived
`"tablename_colname"` string is a *guess*, not a guarantee the table exists at all — the naming
convention only tells you the *shape*, not whether the underlying data is actually there. Verify
before calling `configure()`, not after it fails:

**WRONG — go straight to `configure()` and react to the failure:**
```python
f.configure("Guinier", ["SAXS_data_I"])   # if SAXS_data doesn't exist, this wastes a full round-trip
```

**RIGHT — check BOTH levels: the table itself, then the specific named column inside it:**
```python
if not existTable("SAXS_data"):
    scriptPrint("Table 'SAXS_data' does not exist.")
else:
    t = table("SAXS_data")
    if "I" not in t.colNames():
        scriptPrint("Table 'SAXS_data' exists but has no column named 'I'. Columns: " + str(t.colNames()))
    else:
        f.configure("Guinier", ["SAXS_data_I"])
        f.setParamValue("Rg", 50.0)
        f.fit()
```
**Checking `existTable()` alone is NOT enough** — it only rules out "the table doesn't exist at
all." A table can exist while the specific named column doesn't, and `existTable()` says nothing
about that: `configure()` still fails with the exact same runtime `ValueError` it would have
raised with no check at all, just one level down (confirmed live: an `existTable()`-only guard
passed straight through into `configure()`, which then failed on the missing `"...I"` column).
The second check (`"I" not in t.colNames()`) is what actually prevents that.

Use `existTable("name")` (an `app`-level function — see `app-python-api.md`), not `f.datasets()`,
for the table-level check: `f.datasets()` only lists Y-columns *already recognized as fit-ready*
(with a Y role assigned), so it can't distinguish "table doesn't exist" from "table exists but
that column isn't set up for fitting" — a real difference the report should state precisely.

> **The table name itself can contain underscores — the column name never does.** `"SAXS_data_I"`
> means table `"SAXS_data"`, column `"I"` — split at the LAST underscore, not the first, since
> `"SAXS"` + `"data_I"` would be the wrong split. When going the other direction (constructing the
> dataset string from a table+column you already know), this is unambiguous either way — it only
> matters when parsing an EXISTING dataset name string back into its table/column parts.
>
> **This includes passing a dataset name straight to `table()` without splitting it first.**
> `table()` takes a TABLE name, not a `"table_col"` dataset identifier — passing the whole string
> through looks like it should work (it's a valid-looking name) but silently returns `None`
> because no table is actually named that.
> ```python
> # WRONG — "SAXS_data_2" is a dataset identifier (table "SAXS_data", column "2"), not a table
> # name; table() returns None, and the next line crashes with AttributeError on NoneType
> t = table("SAXS_data_2")
> scriptPrint("rows=" + str(t.nRows()))
>
> # RIGHT — split at the LAST underscore first, then look up the real table name
> t = table("SAXS_data")
> scriptPrint("rows=" + str(t.nRows()))
> ```

### Rule 3 — `setParamInitsAll` / `setParamVariesAll` require Python floats/bools, not strings

**WRONG:**
```python
f.setParamInitsAll(["1.0", "100.0", "0.001"])   # strings — raises TypeError
```

**RIGHT:**
```python
f.setParamInitsAll([1.0, 100.0, 0.001])
f.setParamVariesAll([True, True, False])
```

### Rule 4 — `configure()` does not accept formula strings

**WRONG:**
```python
f.configure("a*x + b", ["table_I"])   # raises ValueError
```

`configure()` only accepts names from `f.fitFunctions()`. To define a custom function, use
the `Compiler` API first, then discover it with `f.fitFunctions()`.

### Rule 5 — Call ordering: function selection separates two phases

Pre-function calls (`setFitMethod`, dataset count) must come before `configure()`/`setFunction()`.
Post-function calls (datasets, ranges, parameters) must come after. Mixing the order silently
resets parameter settings.

**WRONG:**
```python
f.setParamValue("Rg", 50.0)   # too early — no function selected yet → ValueError: no such parameter 'Rg'
f.configure("Guinier", ["table_I"])
```

**RIGHT:**
```python
f.configure("Guinier", ["table_I"])
f.setParamValue("Rg", 50.0)   # after function selection
```
