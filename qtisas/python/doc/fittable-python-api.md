# QtiSAS Fittable Python API

Python scripting interface for the **Fittable** fitting widget.
Access via: `f = Fittable`

> **You cannot define or create fitting functions via Python.**
> `configure()`/`setFunction()` select a function **by name** from the pre-compiled Fittable library.
> Passing a formula string such as `"a*x+b"` will raise `ValueError`.
> Use `f.fitFunctions()` to list all available function names.
> Function authoring via Python will be supported in a future release via the **Compiler** interface.

> **Boolean aliases:** QtiSAS defines `true` and `false` as aliases for `True` and `False`.
> Both forms are accepted: `f.setInstrumentalFit(true)` == `f.setInstrumentalFit(True)`.

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

`setInstrumentalFit()` and `setInstrument()` may be called at any stage; at `"fit"` or later, the UI widgets and parameter tables update immediately.

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
| `stage` | str | `"init"` / `"ready"` / `"fitted"` — **NOT** `stage()`/`selectStage()`'s vocabulary. Tracks only whether a function is loaded and a fit has run, not the active tab: after `f.selectStage("simulate")`, `f.stage()` returns `"simulate"` but `f.state()["stage"]` still shows `"ready"`/`"fitted"`. |
| `function` | str | current function name (empty if not set) |
| `datasets` | int | number of configured datasets |
| `selected` | list[str] | names of selected dataset columns — **NOT** `selectedDatasets`; that key does not exist and raises `KeyError` |
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

> **`Fittable` has `fitFunctions()`; it does NOT have `functionNames()`** — that belongs to
> `Compiler`, a different singleton (see compile-python-api.md). Don't borrow one's method name onto
> the other — `Compiler` needs `c.functionNames("ALL")` to list compiled functions, never
> `c.fitFunctions(...)`.

---

### currentFunction()

```python
name = f.currentFunction()
```

---

### Path — compiled function library directory

```python
lib_dir = f.Path   # e.g. "~/.config/qtisas/FitFunctions/" — readable/writable attribute, not a method
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
> table+column pair into two list items asks for two separate datasets that must independently
> exist:
>   - WRONG: `f.configure("Guinier", ["data", "I"])` — requests datasets `"data"` AND `"I"` as two
>     separate fits (neither exists — `"I"` alone is never valid) — raises `ValueError` on the first
>     missing one, not a hint the split was wrong.
>   - RIGHT: `f.configure("Guinier", ["data_I"])` — one combined string, column `I` of table `data`.
>   - RIGHT (genuinely two datasets): `f.configure("Guinier", ["data1_I", "data2_I"])` — a global
>     fit across two tables' `I` columns, each a complete `tablename_colname` string.

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

> **`datasets()` is read-only — it can't register new datasets, only list configured ones.**
> Calling it with an argument raises `TypeError: datasets(self): too many arguments`; calling it
> bare afterward runs but does nothing — return value discarded, fit still has zero datasets. To set
> datasets, call `configure(function, [...])` again with the full list, or `setDatasetCount(n)` +
> `selectDataset(m, name)`.
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
> Levenberg-Marquardt (`"LM"`, default) converges in under 20 iterations for Gaussian, sphere,
> Guinier, cylinder, any continuously differentiable model. `setFitMethod("Simplex")`/`(0)` is
> derivative-free search; for smooth models it runs 500–1000 iterations without converging — only
> use it for tabulated/step-function models where derivatives are unreliable.
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
| `resoEnabled(m)` / `setResoEnabled(m, bool)` | enable resolution for dataset m — **no bounds check**: an out-of-range `m` (e.g. `0`, beyond `datasetCount()`) silently no-ops / returns `False`, no exception — unlike `setResoDataset` below |
| `resoDataset(m)` / `setResoDataset(m, name)` | resolution σ(Q) source for dataset m — column name or built-in option (see below) |

> **`setResoDataset(m, name)` raises `ValueError` if `name` doesn't match any available resolution
> option for that index** — same silent-no-op bug `setWeightingDataset` had (confirmed live): an
> unmatched name used to silently keep the previous selection, so smearing never applied. The combo
> only populates once `setInstrumentalFit(True)` is set — call that first if a column name isn't
> recognized. `setPolyDataset(m, name)` below has the identical fix, needing `setPolyEnabled(m,
> True)` first instead.

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
| `polyEnabled(m)` / `setPolyEnabled(m, bool)` | enable polydispersity for dataset m — **no bounds check**, same silent no-op as `setResoEnabled` above for an out-of-range `m` |
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
| 6 | `1/(Y^a · c^|Xmax−X|)` |

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

Matching read accessors exist for every setter above (not shown in the code block):
`weightingMethod()`, `weightingA()`/`weightingB()`/`weightingC()`/`weightingXmax()`,
`weightingEnabled(m)`, `weightingDataset(m)`.

> **RULE — Assigning role `Table.PlotDesignation.yErr` does NOT enable weighted fitting; also call
> `setWeightingEnabled(m, True)`.** Without it, weighting silently stays OFF (weight = 1) — no error;
> `chi2`/`f.results()` still return numbers, but chi²/dof reflects raw unnormalized residuals,
> dominated by the data's scale, not fit quality. Confirmed: data ~10⁹ in magnitude reported
> chi²/dof ≈ 1.5×10¹⁴ unweighted vs. ≈ 1.8 weighted, same fit — thirteen orders of magnitude apart.
>
> **`m` here is a 1-based dataset INDEX (int), not a `"tablename_colname"` NAME string like
> elsewhere in this API.** Confirmed live, both wrong forms:
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
> When comparing chi²/dof across models/datasets, confirm weighting is enabled first — otherwise the comparison is meaningless.

> **`setWeightingDataset(m, name)` raises `ValueError` if `name` isn't the exact, fully-qualified
> `"tablename_colname"` name — not a bare column like `"dI"`.** Confirmed live:
> `f.setWeightingDataset(1, "dI")` used to silently no-op instead of failing, same shape as
> forgetting `setWeightingEnabled()` above. Always use the full `"table_dI"` form.
>
> **A correctly-formatted `"table_col"` name still isn't enough — the column must be yErr-role.**
> `setWeightingDataset`'s valid-name pool is a strict subset of `configure()`'s `datasets` argument
> (X/Y too) — see Rule 6 below.

---

## Parameters

> **IMPORTANT — exact parameter names required.**
> `setParamValue`, `setParamVaries`, `paramValue`, etc. need names **exactly** as `paramNames()`
> returns — case-sensitive, no abbreviations. An unknown name raises `ValueError: setParamValue: no
> such parameter '<name>'. Available: <comma-separated real names>.`, so a fix never has to guess.
> `paramValue` (READ) used to silently return `nan` for an unknown name instead of raising; now
> fixed, so e.g. `paramValue("center")` on a model whose real name is `"x0"` is caught immediately.
> Discover names first: `scriptPrint(str(f.paramNames()))`. Example, `sasviewmodels/sphere`:
> `['scale', 'background', 'sld', 'sld_solvent', 'radius']`.

> **IMPORTANT — not all parameters are adjustable (fittable).**
> Each model decides what's fittable. After `configure()`, `paramVaries(name)` reflects that —
> `False` for parameters the model marks fixed (e.g. some SANS-fit functions vary only `scale`,
> fixing `SLD1`/`SLD2`). **Call `paramVaries(name)` after `configure()`** to check;
> `setParamVaries("sld1", True)` on a non-adjustable parameter has no effect.
>
> **For a CUSTOM compiled function, adjustability is just whatever `c.setParamVariesAll()` set at
> compile time** — not a deliberate restriction. `setParamCount(n)` defaults new parameters to
> fixed; skip `setParamVariesAll()` and ALL stay permanently non-adjustable.
>
> **`configure()` re-derives vary/fixed state from that compiled default EVERY call — it drops any
> prior `setParamVaries()` overrides.** A second `configure()` resets to the compiled baseline. If
> nothing ever sets varying params, `fit()` raises `RuntimeError: fit(): no adjustable parameters —
> every parameter is fixed.` Confirmed live.

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
| `fit()` | `"fit"` | float | run fit; returns Chi²/DoF; prints `chi²/dof`, `R²`, `Q`; raises `RuntimeError` if the dataset(s) can't be read (e.g. empty table) |
| `r2()` | `"fit"` | float | R² = 1 − χ²/TSS after last fit or simulate |
| `qFactor()` | `"fit"` | float | goodness-of-fit probability Q(dof/2, χ²/2); meaningful only with instrumental (error-bar) weighting |
| `simulate()` | `"fit"` | float | plot fit curves for all datasets |
| `plotResults(graphName="", graphTemplate="")` | `"fit"` | MultiLayer | plot data + fit curves in a named graph; skips curves already present |
| `simulateCurve()` | `"simulate"` | float or NaN | run single-curve simulation (Simulate Curve tab) |
| `beforeFit()` | `"fit"` | — | called internally by `fit()` — do NOT call manually |
| `afterFit()` | `"fit"` | — | called internally by `fit()` — do NOT call manually |
| `curveColor()` | any | int | current output-curve color index (used by fit, simulate, simulateCurve, plotResults) |
| `setCurveColor(index)` | after `configure()` | bool | set output-curve color by integer index; returns `False` if out of range or called in `"init"` stage |
| `setCurveColor("name")` | after `configure()` | bool | set output-curve color by partial name (case-insensitive); prints available names on failure; call after `configure()`, before `fit()`/`plotResults()` |
| `setScaleErrors(bool)` | — | — | scale parameter errors by √(Chi²/DoF) |
| `setStatisticsNotes(bool)` | — | — | save covariance matrix to a Note after fit |
| `setSaveSession(bool)` | — | — | save session to a Note after fit |

`fit()`, `simulate()`, `plotResults()`, `beforeFit()`, `afterFit()` raise `RuntimeError` if stage isn't `"fit"`; `simulateCurve()` raises it if stage isn't `"simulate"`.

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
| 5 | `residues` | Y | raw, UNNORMALIZED residual `data − fit` — not divided by σ or weight, despite the name |
| 6 | `Characteristics` | None | label column — one text label per row (e.g. `"chi^2/DoF"`, `"R^2=1-chi^2/TSS"`, a parameter name lives in col 8 instead, see below) |
| 7 | `Conditions` | None | value column, same row as its label in col 6 — text, prefixed `"->   "` |
| 8 | `Parameters` | None | one fit parameter NAME per row |
| 9 | `Values` | None | that parameter's fitted value, same row |
| 10 | `Errors` | None | that parameter's fit error, same row (empty for a pure `simulate()`, only populated after a real `fit()`) |

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

**This table is the ONLY reliable source for chi²/dof, R², and every fitted parameter's value+error
in a LATER script/part — never re-derive from memory, printed console text, or a fresh `f.fit()`
call.** The row each label/parameter lands on shifts depending on whether SANS
resolution/polydispersity are enabled (confirmed against source), so **search by label text, never
hardcode a row number**:

```python
def read_characteristic(fc, label):
    for row in range(1, fc.numRows() + 1):
        if fc.text(6, row).strip() == label:
            return fc.text(7, row).replace("->", "").strip()
    return None

def read_param(fc, name):
    for row in range(1, fc.numRows() + 1):
        if fc.text(8, row).strip() == name:
            value = float(fc.text(9, row).strip())
            err_str = fc.text(10, row).strip()
            error = None if err_str == "---" else float(err_str.replace("±", "").strip())
            return value, error
    return None, None

fc = table("fitCurve-ai/GaussPeakModel")
chi2_dof = float(read_characteristic(fc, "chi^2/DoF"))
r2       = float(read_characteristic(fc, "R^2=1-chi^2/TSS"))
scale_val, scale_err = read_param(fc, "scale")
```
**Col 7 (`Conditions`) is `"->"` + variable spacing + the value** (e.g. `"->   7.88510912046E+03"`)
— `read_characteristic()` above handles this via `.replace("->", "").strip()`. Don't slice a
fixed-width prefix — spacing isn't constant across rows.

**Col 10 (`Errors`) is `"±<number>"` for a varied parameter, or `"---"` for a fixed one** — check
for `"---"` first and strip the `"±"` before `float()` otherwise. Confirmed live: `float()` on
`"±0.00974278485"` directly raises `ValueError: could not convert string to float`.

Run an `# EXPLORE` first to confirm exact label/parameter text for the model in question
(`fc.colNames()`, print a few rows of cols 6-9) before hardcoding a label string.

> **`t.text(col, row)` used to raise `TypeError: decoding str is not supported` for EVERY
> non-empty cell, on any table** — a Python-3-porting bug (an already-decoded `str` passed through a
> second, redundant decode step, which CPython rejects). Fixed at the source (`qtimod.sip`) as of
> this build. On a build predating the fix, use `cellData(col, row)` instead — same text, unaffected
> code path.

> **`t.cell(col, row)` silently returns `0.0` when the cell's text can't be parsed — it never
> raises, and can't distinguish "value really is 0.0" from "parsing failed."** Confirmed-live-
> dangerous on these label/value columns: reading `cell(7, row)` at the WRONG row (e.g. one holding
> `"Number of Parameters"` text like `"->   4"` instead of `chi^2/DoF`) silently returns `0.0`,
> indistinguishable from a perfect fit. Confirmed live: exactly this produced `chi2/dof = 0.0000` for
> BOTH of two different models compared — an implausibly identical "perfect" value that flagged
> nothing. Never use `cell()` on columns 6-10 of a `fitCurve` table — they're text columns; use
> `text()`/`cellData()` and parse the result yourself.

**Comparing two already-fit models (e.g. "fit both Gauss and Lorentzian, pick the better one") is
solved once BOTH models have been fit at least once — no re-fit, guess, or recall needed.** Both
`fitCurve-<model>` tables persist as real, lookupable objects — read both directly. Worked example:
compare, pick the winner, read a parameter, then plot data + winner's ACTUAL fit curve + residuals
in two stacked panels (`canvasGeometry()`/`setCanvasGeometry()` below is the confirmed-working
pattern — prefer it over guessing `setLayerGeometry()`/`setLayerRect()`/`setLayout()`, none of which
exist):

```python
# Both models are assumed already compiled and fit earlier — each fit() call already created its
# own persistent fitCurve-<model> table (see the table structure above).

def read_characteristic(fc, label):
    for row in range(1, fc.numRows() + 1):
        if fc.text(6, row).strip() == label:
            return fc.text(7, row).replace("->", "").strip()
    return None

def read_param(fc, name):
    for row in range(1, fc.numRows() + 1):
        if fc.text(8, row).strip() == name:
            value = float(fc.text(9, row).strip())
            err_str = fc.text(10, row).strip()
            error = None if err_str == "---" else float(err_str.replace("±", "").strip())
            return value, error
    return None, None

# --- Step 1: look up both fit-result tables by name — no re-fit needed ---
fc1 = table("fitCurve-ai/fittable-model1")
fc2 = table("fitCurve-ai/fittable-model2")
if fc1 is None or fc2 is None:
    scriptPrint("One of the fit-curve tables is missing — did both fits actually run?")
else:
    # --- Step 2: read the REAL chi2/dof from each — never recall printed text or fabricate ---
    chi2_1 = float(read_characteristic(fc1, "chi^2/DoF"))
    chi2_2 = float(read_characteristic(fc2, "chi^2/DoF"))
    scriptPrint(f"Model 1 chi2/dof = {chi2_1:.4f}")
    scriptPrint(f"Model 2 chi2/dof = {chi2_2:.4f}")

    # --- Step 3: select the winner ---
    best_fc, best_name = (fc1, "ai/fittable-model1") if chi2_1 <= chi2_2 else (fc2, "ai/fittable-model2")
    scriptPrint(f"Best model: {best_name}")

    # --- Step 4: pull the winner's parameters, e.g. "scale" ---
    scale_val, scale_err = read_param(best_fc, "scale")
    scriptPrint(f"Best-fit scale = {scale_val}" + (f" ± {scale_err}" if scale_err is not None else " (fixed)"))

    # --- Step 5: plot ORIGINAL data + winner's ACTUAL fit curve + ACTUAL residuals, 2 stacked panels ---
    # "y" on best_fc is the MODEL's calculated curve, not the original data (see fitCurve table
    # structure above) — the original data must be added separately, from its own table, or the
    # plot silently shows only the fit line with no data points to compare it against.
    t = table("YourDataTable")   # the table you originally called f.configure(model, ["YourDataTable_y"]) with
    gw = newGraph("FitResults", 2, 2, 1)
    maximizeWindow(gw)
    l1, l2 = gw.layer(1), gw.layer(2)
    l1.enableAxis(Graph.Axis.Bottom, False)
    l2.enableAxis(Graph.Axis.Top, False)
    gw.keepAligned(False)
    gw.setAlignPolicy(GraphWindow.AlignPolicy.AlignCanvases)
    gw.setSpacing(0, 0)
    gw.arrangeLayers(False, False)

    cr1, cr2 = l1.canvasGeometry(), l2.canvasGeometry()
    x, y, w = cr1.x(), min(cr1.y(), cr2.y()), cr1.width()
    total_h = cr1.height() + cr2.height()
    h1 = int(total_h * 0.75)
    l1.setCanvasGeometry(x, y, w, h1)
    l2.setCanvasGeometry(x, y + h1, w, total_h - h1)
    l2.removeTitle()

    l1.addCurve(t, "y", Graph.CurveType.Scatter)                       # ORIGINAL measured data — curve index 0
    l1.insertCurve(best_fc, "x", "y", Graph.CurveType.Line)            # winner's ACTUAL fit curve — index 1
    l2.insertCurve(best_fc, "x", "residues", Graph.CurveType.Scatter)  # winner's ACTUAL residuals

    l1.setCanvasGeometry(x, y, w, h1)          # re-apply — inserting curves can drift the geometry
    l2.setCanvasGeometry(x, y + h1, w, total_h - h1)
    gw.keepAligned(True)
    gw.linkXLayerAxes(True)
    l1.replot()
    l2.replot()
```
This avoids four confirmed-live failures at once: inventing placeholder comparison numbers instead
of real ones; plotting a raw data column (e.g. `dI`) mislabeled "residuals" instead of the model's
actual `residues`; skipping the best-fit overlay because "which model won" seemed to need a re-fit;
and — most likely to go unnoticed, since nothing errors — plotting `best_fc`'s "y" alone, mistaken
for the original data instead of adding the real data table's own curve.

---

### plotResults(graphName="", graphTemplate="")

Always plots into **layer 1** (data scatter + fit line).  
If the MultiLayer has a second layer and `radioButtonSameQrange` is checked, residuals (scatter) are added to **layer 2** and the X axes are linked.  
Existing curves are never duplicated — repeated calls are safe.

**Curve order/index in layer 1, per dataset: DATA scatter is inserted first, FIT line second.** For
a single dataset that's index **0 = data**, index **1 = fit** — NOT reversed, NOT "the only curve =
the line." Styling curve `0` recolors data points, not the fit. Prefer `f.setCurveColor(...)`,
called *before* `fit()`/`plotResults()`. Only reach for `g.setCurveLineColor(1, ...)` for something
`setCurveColor()` can't express, e.g. an exact `QColor` outside its named palette.

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

> **The fit curve's `Graph.curveTitle(i)` is NOT the bare model/function name — it's
> `"fitCurve-" + function + "_y"`.** Checking `g.curveTitle(i) == model` to verify the fit curve
> exists will NEVER match — a false "not found" with no error. Confirmed live: exactly this check
> (`g.curveTitle(i) == "ai/SphereFormFactor"`) silently skipped a legend/styling part — just printed
> "Fit curve missing from graph," even though `plotResults()` had succeeded. **Prefer the INDEX
> convention above (0 = data, 1 = fit) or `g.numCurves() >= 2`** instead of guessing a title format.
> ```python
> # WRONG — invented title guess, never matches the real "fitCurve-<function>_y" format
> if any(g.curveTitle(i) == model for i in range(g.numCurves())):
>     ...  # never true — silently skips everything inside, no error
>
> # RIGHT — trust the already-documented index convention instead
> if g.numCurves() >= 2:
>     ...
> ```

Confirmed live: "set line color to orange" was fulfilled correctly via `f.setCurveColor("orange")`
— but the script ALSO added a redundant `g.setCurveLineColor(0, colorIndex("orange"))` afterward,
which recolors the data scatter instead, unintentionally blending data and fit visually.

**`setCurveColor()`/`curveColor()` are the ONLY curve-styling methods `Fittable` has** — no
`setCurveLineWidth()` or any symbol-styling setter exists on it. For line width, symbol
shape/size/color, get the layer via `f.plotResults(...).activeLayer()` and call the `Graph` method on
curve index **1** (the fit line), e.g. `g.setCurveLineWidth(1, 2)`. Confirmed live:
`f.setCurveLineWidth(2)` raised `AttributeError: 'fittable18' object has no attribute
'setCurveLineWidth'` — the method doesn't exist on `Fittable`.

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

**`plotResults()` returns a `MultiLayer` (`GraphWindow`) — NOT a `Graph` (layer). EVERY layer-level
method (`legend()`, `newLegend()`, `setLegend()`, `setXTitle()`, `setYTitle()`, `addCurve()`,
`setCurveLineColor()`, `setCurveSymbolColor()`, etc.) raises `AttributeError` if called directly on
it — always go through `.activeLayer()` first.**

```python
gw = f.plotResults("FitResults")
g = gw.activeLayer()
g.setXTitle("q, 1/nm")               # NOT gw.setXTitle(...)
leg = g.legend()                     # NOT gw.legend()
if leg is None:
    leg = g.newLegend()              # NOT gw.newLegend()
```

Confirmed live, FIVE separate `AttributeError`s in one conversation, each burning a retry round:
`gw.setXTitle(...)`, `gw.legend()`, `gw.setLegend(...)` (twice), `gw.newLegend()` — all
`AttributeError: 'GraphWindow' object has no attribute '<name>'`.

**`plotResults(graphName, ...)` itself DOES reuse an existing window by that exact name** — it was
NOT the source of window pileup here. The actual driver was an earlier, redundant bare `plot(t,
(...))` call made BEFORE fitting just to preview raw data — `plot()` has no name-based reuse (see
`ai-python-api.md`'s "retries re-run the whole script" rule) and created a new window per retry.
Since `plotResults()` already plots one scatter curve per dataset before the fit line, that preview
call is usually unnecessary — the single most common `plotResults()` mistake, an instance of
`graph-python-api.md`'s "GraphWindow vs Graph" note.

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

### `simulateCurve()`'s output table

`simulateCurve()` creates a workspace table named **`"simulatedCurve-" + function`** (`function` is
the string passed to `configure()`) — same naming scheme as `fitCurve-<function>`, different prefix.
**Same exact 10-column layout as `fitCurve-<function>`** (see "Fit curve table columns" above): `x`,
`y`, `weight`, `sigma`, `residues`, `Characteristics`, `Conditions`, `Parameters`, `Values`, `Errors`.
Read/plot it the same way:

```python
sc = table("simulatedCurve-" + model)          # model is the string passed to configure()
g.insertCurve(sc, "x", "y", Graph.CurveType.Line)
```

> **Confirmed live: do NOT pass `"simulatedCurve-<function>_y"` to `table()`** — the `_y` suffix is
> the COLUMN-qualified dataset name (same `"tablename_colname"` convention as `f.datasets()`), not
> the table name. Confirmed live, separately: do NOT reach for `plot("simulatedCurve-<function>",
> xMin, xMax, style)` either — `graph-python-api.md`'s `plot("formula", xMin, xMax)` overload
> evaluates its string as a MATH FORMULA (e.g. `"x^2"`), not a dataset lookup; a table name there
> silently plots a nonsense function labeled with that string, no exception. Use `table()` +
> `insertCurve()` instead.

---

## Results

### Scalar results

```python
chi2 = f.lastChi2    # Chi²/DoF from last fit
dof  = f.lastDoF     # degrees of freedom
```

> **IMPORTANT — a good chi²/dof does NOT by itself mean the fit finished converging.** `fit()` can
> stop for several reasons — only "hit the iteration cap" (console `iter <N> [<maxIterations>]`,
> `<N>` at/near `maxIterations()`) means it may have been cut off early. Reliable check: call
> `fit()` again immediately (same config); barely-changed chi²/dof means it had settled, a further
> drop means the first run wasn't finished. Works regardless of model — unlike checking a parameter
> against the data's range, which is domain judgment (a peak center CAN legitimately sit outside the
> sampled range with a tight error bar).
> ```python
> chi1 = f.fit()
> chi2 = f.fit()  # re-run from the just-converged parameters — cheap, and confirms convergence
> if abs(chi2 - chi1) > 1e-6 * max(abs(chi1), 1e-300):
>     scriptPrint(f"Fit may not have fully converged: chi2/dof went from {chi1} to {chi2} on a "
>                 f"repeat fit — consider raising maxIterations (currently {f.maxIterations()}).")
> else:
>     scriptPrint(f"Fit converged: chi2/dof = {chi2}, R2 = {f.r2()}")
> ```

> **IMPORTANT — `f.lastChi2` and `f.results()` reflect the CURRENT fit state, not a history —
> reading them again after a second `fit()` does NOT give the "before" values.** Only one live
> result exists; a second `fit()` overwrites it. To compare before-vs-after, capture the "before"
> snapshot **before** calling `fit()` again — reading them afterward instead silently gives the SAME
> (new) values twice under different labels, no error or warning.
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
> Same gotcha as the convergence check above — `fit()`'s return value is always a fresh snapshot
> each call; `lastChi2`/`results()` are safe only for "the current state," never as a stand-in for
> "before the call I just made."

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

**`r["name"]` is the `(value, error)` tuple itself — never index it with `[0]`, and never pass it
directly into a format spec expecting a number.** Confirmed live, twice in the same conversation:
```python
# WRONG — one index too many; r["R"][0] is already the float, not another indexable tuple
r_val = f.results()["R"][0][0]           # TypeError: 'float' object is not subscriptable

# WRONG — the tuple itself, unindexed, passed straight into a numeric format spec
leg.setText(f"R = {f.results()['R']:.2f} nm")   # TypeError: unsupported format string passed to tuple.__format__

# RIGHT — unpack once
r_val, r_err = f.results()["R"]
leg.setText(f"R = {r_val:.2f} nm")
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
| `RuntimeError` | `fit()` | selected dataset(s) could not be read (e.g. empty table) — previously silently returned a stale chi²/dof with no indication of failure |
| `RuntimeError` | `simulateCurve()` | stage is not `"simulate"` |
| `RuntimeError` | `selectStage(s)` | in `init` stage and `s` ≠ `"init"` |
| `RuntimeError` | `fit()` | no adjustable parameters — every parameter is fixed |
| `RuntimeError` | `setInstrument()`, `setInstrumentalFit()` | invalid usage (see SANS Instrument section) |
| `ValueError` | `setFunction`, `selectDataset`, `configure` | name not found |
| `ValueError` | `selectStage(s)` | unknown stage name |
| `ValueError` | `setParamValue`, `setParamVaries`, `paramValue` and related | unknown parameter name |
| `ValueError` | `setWeightingDataset`, `setResoDataset`, `setPolyDataset` | bad/unqualified dataset name, or index out of range |
| `ValueError` | `setWeightingEnabled`, `weightingDataset` | dataset index out of range |

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

`f.beforeFit()` (same as pressing **Before Fit** in the UI) does more than sync the UI table — it
also calls the compiled fit function once with its `beforeFit` lifecycle flag set, *before* fitting.
If the function defines a `beforeFitting(void *ParaM)` helper in `[included functions]` (see
compile-python-api.md's `includedFunctions()`), that hook can compute initial parameter values from
loaded data instead of fixed numbers from `setParamInitsAll()`.

---

### Parameter constraints: checkbox-level vs in-code

Two independent mechanisms can make a parameter "fixed" or "linked" — don't confuse them:

| | Decided | Mechanism | Needs code? |
|---|---|---|---|
| **Not varying** | above the function, by Fittable | `setParamVaries(name, False)` — optimizer never touches it | No |
| **Shared** | above the function, by Fittable | `setParamShared(name, True)` — forced equal to dataset 1's value, for every other dataset | No |
| **In-code constrained** | inside the compiled function | any transform written in `[code]` (e.g. `Rg = fabs(Rg);`) | Yes |

The first two are checkbox flags Fittable resolves itself before the compiled function runs —
nothing to write in `[code]` for either. The third is arbitrary: the compiled function can do
anything to its own parameters, running on **every** call (every point/iteration) so the returned
value stays physically consistent, but the *persisted* result (optimizer vector + UI table) only
updates during the `beforeFit()`/`afterFit()` calls above — not the many per-point calls in between.

**Ordering constraint on two peaks (`x1 < x2`) — swap the whole parameter group, not just the
compared variable.** If an iteration proposes `x1 > x2`, swapping only positions leaves
`amplitude1`/`width1` attached to the wrong peak:
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

Function names depend on what's compiled into the running library and may differ between
installations. **Exception:** if the script itself compiled the function via `Compiler`, the name is
already known. Otherwise, discover first.

**WRONG:**
```python
f.configure("sasviewmodels/sphere_SANS", ["table_I"])   # hardcoded — may not exist
```

**RIGHT — discovery pattern (required for a pre-existing model). Exclude `"ai/*"` from candidates —
that namespace holds only prior AI-authored one-offs (see `compile-python-api.md` Rule 6), never a
vetted library model; a match there means a NEW custom function was wanted, not reuse:**
```python
f = Fittable
requested = "Sphere"   # the exact name the request itself quoted, if any — else ""

# If the request quoted a SPECIFIC name, check it exactly FIRST — a quoted name is the user
# asserting it's the real one, the same as a quoted table/column name; treat a miss as a
# reportable mismatch, not silent license to broaden the search.
if requested and requested in f.fitFunctions():
    model = requested
else:
    hits = [h for h in f.fitFunctions("*sphere*SANS*") if not h.startswith("ai/")]   # most specific first
    if not hits:
        hits = [h for h in f.fitFunctions("*sphere*") if not h.startswith("ai/")]    # broaden on miss
    if not hits:
        print(f"Model '{requested}' not found, and no similar model was found either. "
              f"SANS-related models: {f.fitFunctions('*SANS*')}")
        raise RuntimeError(f"No suitable model found in Fittable library for '{requested}'")
    model = hits[0]
    if requested:
        print(f"Model '{requested}' does not exist exactly — using closest match: {model}")
    else:
        print(f"Model: {model}")
    if len(hits) > 1:
        print(f"Also available: {hits[1:]}")
# use `model` as the function name — never a hardcoded string
f.configure(model, ["table_I"])
```

> **`configure()` alone does not give the model sensible starting values.** Set physically
> reasonable initial guesses via `setParamValue()`/`setParamLimits()` before calling `fit()` —
> unless the function has its own `beforeFitting()` hook deriving them from loaded data (see
> `beforeFit()`). Skipping this raises no error: the fit silently runs from arbitrary library
> defaults, which can converge to garbage (huge `chi²/dof`, `R²` near 0) with nothing to flag it.

If no model is found: print the search results and raise an error — don't fall back silently. If a
quoted SPECIFIC name doesn't exist but a broader search finds something plausible, use it but print
that the exact name wasn't found and name the substitute. Confirmed live: a request quoting
`"Sphere"` (no exact match) had its search broadened with no announcement, landed on an unrelated
model matching `hits[0]` alphabetically, and produced a catastrophically bad fit (`R² = -190`) with
nothing printed to flag the mismatch.

### Rule 2 — Never guess whether an explicitly named table/dataset exists

When a request names a specific table (e.g. "table SAXS_data column I"), the derived
`"tablename_colname"` string is a *guess*, not a guarantee it exists — the naming convention only
gives the *shape*, not whether the data is there. Verify before calling `configure()`, not after it
fails:

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
**Checking `existTable()` alone is NOT enough** — it only rules out "table doesn't exist at all." A
table can exist while the named column doesn't; `configure()` still fails with the same runtime
`ValueError` one level down (confirmed live: an `existTable()`-only guard passed straight through
into `configure()`, which failed on the missing `"...I"` column). The second check
(`"I" not in t.colNames()`) is what actually prevents that.

Use `existTable("name")` (an `app`-level function), not `f.datasets()`, for the table-level check —
`f.datasets()` only lists Y-columns already recognized as fit-ready, so it can't distinguish "table
doesn't exist" from "table exists but isn't set up for fitting."

> **The table name can contain underscores — the column name never does.** `"SAXS_data_I"` means
> table `"SAXS_data"`, column `"I"` — split at the LAST underscore, not the first (`"SAXS"` +
> `"data_I"` would be wrong). Only matters when parsing an EXISTING dataset name string back apart.
>
> **This includes passing a dataset name straight to `table()` without splitting it.** `table()`
> takes a TABLE name, not a `"table_col"` identifier — the whole string looks valid but silently
> returns `None` since no table is actually named that.
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

### Rule 6 — `setWeightingDataset()`'s valid names are a stricter subset than `configure()`'s — yErr-role columns only

`configure(fn, datasets)` accepts a dataset string from any plotted-role column (X or Y) — why
`"table_I"` is valid there. `setWeightingDataset(m, name)` draws from a smaller pool: only **yErr**
columns qualify. A correctly-formatted `"table_col"` string for a Y column raises the same generic
`ValueError` as a nonexistent name — nothing distinguishes "wrong role" from "doesn't exist."

**WRONG — `"Sphere_I"` looks valid (`I` is really a column of table `Sphere`), but it's the
Y-role column, not yErr — confirmed live:**
```python
t.setColNames(["Q", "I", "dI"])
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(2, Table.PlotDesignation.Y)
t.setColumnRole(3, Table.PlotDesignation.yErr)
f.configure("SphereSANS", ["Sphere_I"])   # OK — Y column is the right pool for configure()
f.setWeightingDataset(1, "Sphere_I")      # ValueError — wrong pool: only yErr columns qualify here
```

**RIGHT — point at the yErr-role column instead:**
```python
f.setWeightingDataset(1, "Sphere_dI")     # dI is the yErr-role column
```

If unsure which column is yErr-role, check which had `Table.PlotDesignation.yErr` assigned — don't
assume it's the same column named in the main `configure()` string.

**A `newTable()`'d table's columns default to `Y` except column 1 (`X`) — there is NO default
`yErr`.** Without an explicit `t.setColumnRole(col, Table.PlotDesignation.yErr)` call, the error
column silently stays `Y`-role: a properly-formatted `"table_dI"` string can still be rejected even
though the column exists and survives `configure()` — it's absent from the yErr pool, and the error
only says "not one of the available weighting datasets," never "wrong role." Confirmed live:
`newTable("GaussPeak", 150, 3); t.setColNames(["Q", "I", "dI"])` with no `setColumnRole()` left `dI`
as `Y` — `f.setWeightingDataset(1, "GaussPeak_dI")` failed twice, retries only re-checking the name
string, never the column's role, until the identical-error-repeat guard gave up.

**This is fixable on the spot, even from a LATER part/script — the table doesn't need to be
recreated:**
```python
t = table("GaussPeak")
t.setColumnRole(3, Table.PlotDesignation.yErr)   # fix the role on the EXISTING table
f.setWeightingDataset(1, "GaussPeak_dI")          # now succeeds
```
If `setWeightingDataset()` rejects an otherwise-correct-looking name, check the column's actual
role before assuming the name is wrong — especially if the table was created in an earlier, separate
part/script that might have skipped `setColumnRole()`.

### Rule 7 — A "fitted"/"resulting" value shown in a legend or print must come from `f.results()`, never a hardcoded literal

When a request asks to display "the fitted X," the value must be read back from the fit's own
output — not reused from an initial parameter guess, a synthetic-data comment, or anything else
already visible in context. A literal that happens to match the true/seed value in a test case will
keep showing that stale number if the fit ever converges elsewhere — nothing raises or warns.

**WRONG — `"R = 50.00"` is copied from the parameter's initial guess and never touched again;
confirmed live, the fit's actual result was never queried:**
```python
f.setParamValue("R", 50.0)
chi2 = f.fit()
leg.setText("R = 50.00 Å")   # unconditionally the initial guess, regardless of what fit() found
```

**RIGHT — read the fitted value back from `results()` after `fit()` returns:**
```python
f.setParamValue("R", 50.0)
chi2 = f.fit()
r_val, r_err = f.results()["R"]
leg.setText(f"R = {r_val:.2f} Å")
```

This applies to every "show the fitted/resulting/computed ___" phrasing — the number must trace
back to a `fit()` call's actual output (`f.results()`, `f.lastChi2`, `f.fitReport()`), never to
something decided before `fit()` ran.

### Rule 8 — Don't add resolution/polydispersity/instrumental config the request never asked for

Instrumental resolution smearing (`setInstrumentalFit()`, `setResoFunction()`, `setResoDataset()`,
...) and polydispersity (`setPolyFunction()`, `setPolyEnabled()`, `setPolyDataset()`, ...) are real,
optional refinements — not a default "more thorough" mode for any SANS/SAXS request. Add them only
when explicitly asked for — never as an unprompted embellishment, same as unrequested axis/tick
styling.

**WRONG — request only asked for "fit with weighted errors":**
```python
f.configure(model, ["data_I"])
f.setInstrumentalFit(True)
f.setResoFunction("Gauss-SANS")
f.setResoEnabled(1, True)
f.setResoDataset(1, "10%")
f.setPolyFunction("Gauss")
f.setPolyEnabled(1, True)
f.setPolyDataset(1, "")          # empty/meaningless — no real sigma column was ever created
f.setWeightingMethod(0)
f.setWeightingEnabled(1, True)
f.setWeightingDataset(1, "data_dI")
chi2 = f.fit()
```

**RIGHT — only what was actually asked for:**
```python
f.configure(model, ["data_I"])
f.setWeightingMethod(0)
f.setWeightingEnabled(1, True)
f.setWeightingDataset(1, "data_dI")
chi2 = f.fit()
```

Confirmed live: a plain "fit with weighted errors" request grew an unrequested resolution-smearing
setup plus `setPolyDataset(1, "")` — an empty, meaningless argument — on top of the actual fit. Every
added call is unreviewed surface area with failure modes unrelated to what was asked.

### Rule 9 — `f.fit()`'s return value is ALREADY chi²/dof — never divide it by `dof` again

**`chi2 = f.fit()` is the REDUCED chi-squared (chi²/dof), not the raw sum-of-squared-residuals**
(the variable name `chi2` here is just short physics shorthand). Computing `chi2 / dof` again
silently produces a value too small by a factor of `dof`, with no error — it still looks plausible.

**WRONG — divides an already-reduced chi² by `dof` a second time:**
```python
chi2 = f.fit()
dof = f.lastDoF
scriptPrint(f"chi2/dof = {chi2/dof:.4f}")   # WRONG — chi2 IS ALREADY chi2/dof
```

**RIGHT — `f.fit()`'s return value is the final chi²/dof, use it directly:**
```python
chi2_dof = f.fit()
scriptPrint(f"chi2/dof = {chi2_dof:.4f}")
```

Confirmed live, both fits in one conversation: console printed `chi²/dof: 1.14126291068E+00`, but
`scriptPrint(f"... chi2/dof = {chi2/dof:.4f}")` reported `0.0078` — exactly `1.1413 / 146` (its own
`dof`). A second fit showed the same pattern: `chi²/dof: 1.43413853647E+08` printed as
`975604.4466` — divided by its own `dof` (147) again. Both looked plausible enough that neither
raised suspicion — model-selection logic later read the correct value from the `fitCurve` table
instead, so the final "best model" choice was unaffected, but any report relying on these printed
numbers would be wrong by a large, silent factor.
