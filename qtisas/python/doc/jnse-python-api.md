# QtiSAS JNSE Python API

Python interface of the **JNSE** tool (the "J-NSE" dock): neutron spin echo data of the J-NSE spectrometer. A
`.dtr` file holds one dataset per q — S(q,t)/S(q) against the Fourier time τ — and its header values. `Jnse` reads
the files into a **header table** (one row per q) and one **dataset table** per q, and fits several q together in
Fittable.
The global object is `Jnse` — already defined, like `Fittable` and `Ascii1d`: use it without any import
(`from Jnse import Jnse` raises ModuleNotFoundError).

> **Use `Jnse` for every neutron-spin-echo task** — whenever a request names NSE, neutron spin echo, J-NSE, a
> `.dtr` file, S(q,t)/S(q), the intermediate scattering function or Fourier times, or fits relaxation or diffusion
> over several q. Do not read a `.dtr` file with your own Python code (`open()`, `split()`): its datasets, header
> lines and line ends (CR) are read only here.

**Units of the files:** as the header's `q-unit` and `t-unit` say (1e10: q in 1/Å, 1e-09: τ in ns) — then the
diffusion coefficient D of `simpleDiffusion` is in Å²/ns and a rate D q² in 1/ns.

> **Note:** All SIP-wrapped methods accept positional arguments. Named optional parameters also accept keyword syntax;
> required parameters are always positional.

---

## Quick Start — the recipe

First choose the call:

| The request asks for … | call |
|---|---|
| read `.dtr` files | `importFiles(files, headerTable="")` — returns the header table's name |
| a value of every q (q, temperature, …) | `header(headerTable, column)` |
| the data of one q | `table(datasets(headerTable)[row - 1])` — its columns by label |
| a fit of some or all q together, in one call | `fit(...)` — its results also go to the results log |
| set up the fit, then tune it in Fittable | `initFit(...)`, then `Fittable.setParam…()` and `Fittable.fit()` — never `Fittable.configure()` after `initFit()`: it sets Fittable up anew and resets each row's fixed q |
| the fit function is missing | compile it with `Compiler` (e.g. `simpleDiffusion` of qtisas.com/jnse) |

Then every task, in this order:

```python
j = Jnse
j.reset()                                                  # 1. always first: the input folder stays set
j.setInputFolder("~/<folder>")                             # 2. "~/..." or absolute
info = j.importFiles("<file>.dtr")                       # 3. or a list, or "*.dtr"; a name only when the request
                                                           #    names the header table: importFiles(files, "<name>")
q = j.header(info, "q")                                  # 4. a header column, row by row
sets = j.datasets(info)                                  #    the dataset table of each row, same order
chi2 = j.fit(info, "<function>", <first row>, <last row>,
             ["<shared parameter>", ...], {"<parameter>": <start value>}, {"<parameter>": <fixed value>})
D = Fittable.paramValue("D")                               # 5. results from Fittable
```

The values in `< >` are the request's.

---

## Reading

| Method | Returns | Description |
|---|---|---|
| `reset()` | | the input folder back to the home folder |
| `setInputFolder(folder)` / `inputFolder()` | `str` | where relative file names start |
| `importFiles(files, headerTable="")` | `str` | the files — a name, a list, or wildcards (`"~/d/*.dtr"`) — into a header table; returns its name. Default `"<first file read>-info"` (`report19971_1.2.dtr` → `report19971_1-2-info`); a header table of that name is **appended** to, any other table of that name is refused. Nothing read: `ValueError` with the reason; a missing file is printed and skipped, a file that does not fit the format is read up to that line (printed) |
| `header(headerTable, column)` | `list` | a column of the header table, row by row: numbers for a number column, texts otherwise |
| `datasets(headerTable)` | `list` | the dataset tables, row by row: row k → `datasets(...)[k - 1]` |
| `lastMessages()` | `list` | the warnings of the last call |

**The header table**, one row per dataset — per q, in file order; a second file appends its rows:

| Columns | Content |
|---|---|
| `sample-code`, `sample-info`, `mode`, `run-number` | texts of the dataset's first lines |
| `q` | q in 1/Å |
| `dq`, `q-unit`, `t-unit`, `temp`, `numor`, `ref`, `bgr`, `with-bgr`, `tfac-bgr`, `volfrac`, `qbins`, `tbins`, `oor-cnt`, `upd-cnt` | the dataset's header values |
| `table-name` | its dataset table |

**A dataset table** — hidden, one per row, named `"<file>-<run>-v-<n>"` — has one row per Fourier time. Every
import makes new dataset tables (n counts up), so always take their names from `datasets(headerTable)`, never build
or reuse them. Its columns:
`tau-ns` [X], `Sqt-To-Sq` [Y] = S(q,t)/S(q), `d-Sqt-To-Sq` [yErr], `var-Sqt-To-Sq`, `var-tau-ns`, `nupdates`,
`Qeff`. Its Fittable dataset is `"<dataset table>_Sqt-To-Sq"`.

To plot S(q,t)/S(q) against τ with error bars (a curve is named `<table>_<column>`; `QColor` comes from
`PyQt6.QtGui`):

```python
t = table(name)
g = newGraph("<graph>").activeLayer()
g.insertCurve(t, "tau-ns", "Sqt-To-Sq", Graph.CurveType.LineSymbols)
g.addErrorBars(name + "_Sqt-To-Sq", t, "d-Sqt-To-Sq", 1, 1, 8, QColor("gray"), False, True, True)
```

## Fitting

| Method | Returns | Description |
|---|---|---|
| `initFit(headerTable, function, fromRow=1, toRow=0)` | `list` | Fittable set up for a multi-q fit of the rows fromRow..toRow (1-based; `toRow` 0 = the last): one dataset per row, the function's parameter q **fixed** at each row's q; returns the Fittable datasets. `ValueError` for a function that is not in the library (compile it first), one without a parameter q, or rows outside the table |
| `fit(headerTable, function, fromRow=1, toRow=0, shared=[], start={}, fixed={})` | `float` | `initFit()` and the fit: `shared` parameters fitted in common for all rows (e.g. `["A", "D"]`), `start` values `{name: value}`, `fixed` parameters held at a value `{name: value}`; returns chi²/dof (already reduced) and writes the fitted parameters to the results log |

The rows of one series — a temperature, a file, a sample — come from a header column; the rows of one import are
consecutive:

```python
rows = [k + 1 for k, T in enumerate(j.header(info, "temp")) if T == <temperature>]
j.fit(info, "<function>", rows[0], rows[-1], ["<shared parameter>"], {}, {"<parameter>": <fixed value>})
```

A q given by the request is rounded ("q = 0.0705"): take the nearest row, never test equality —
`q = j.header(info, "q"); row = 1 + min(range(len(q)), key=lambda k: abs(q[k] - <q>))`.

Fittable holds only the last fit: read each fit's parameters right after it. To compare two series, fit one, read
its parameters, then fit the other — never parse the fitCurve tables for that.

After a fit, read the parameters from Fittable: `Fittable.paramValue(name)` for a shared one,
`Fittable.paramValue(name, k)` for row k of the fit (1-based within fromRow..toRow).

The fit functions are those of the Fittable library: an NSE function is a function of t with the parameter q, e.g.
`simpleDiffusion` (S = A exp(−q²(D t + ½ |w| w t²)), parameters q, A, D, w) of qtisas.com/jnse. A missing one is
compiled with `Compiler`.
