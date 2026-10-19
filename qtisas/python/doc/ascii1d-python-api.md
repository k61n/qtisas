# QtiSAS ASCII.SANS.1D Python API

Python interface of the **ASCII.SANS.1D** tool (the "ASCII 1D SANS" dock): import and export of radially averaged
1D small-angle scattering data — columns **Q, I, dI, sigma** — with optional processing on the way.
The global object is `Ascii1d` — already defined, like `Fittable` and `Compiler`: use it without any import
(`from Ascii1d import Ascii1d` raises ModuleNotFoundError).

> **Use `Ascii1d` for every import or export of 1D SAS data** — whenever a request reads or writes a SANS, SAXS,
> SALS or SAS curve, a radially averaged dataset, a 1D / ascii / `.dat` / `.RAD` / canSAS / NIST / GRASP data file,
> or names ASCII.SANS.1D, SANS.1D or ascii1d — and also for a file whose columns are Q, I (dI, sigma), and for any
> task with an option only this tool has: removing points, a SAS presentation (Kratky, Guinier, ...), the resolution
> sigma, binning. Do not read such a file with your own Python code (`open()`,
> `split()`) or with `Table.importASCII()`: their headers hold lines with numbers that are not data, and the
> options below (removed points, presentations, resolution) exist only here.

> **Note:** All SIP-wrapped methods accept positional arguments. Named optional parameters also accept keyword syntax;
> required parameters are always positional.

---

## Quick Start — the recipe

First choose the call by where the data are and where they go:

| The source is … | to a table | to a file |
|---|---|---|
| a **table** in the project (`QI-alpha`, `QI-*`) | `convertTables(...)` | `exportTables(...)` |
| a **file** on disk (`sample.dat`, `*.dat`) | `importFiles(...)` | `convertFiles(...)` |

A table is never a file: `importFiles("QI-alpha")` is wrong when `QI-alpha` is a table of the project, and
`exportTables()` always writes files — a new *table* from a table is `convertTables()`.

**One call does the whole move, with its options.** Do not import and then export: the options stay set, so a
second run applies them again (removed points are removed twice). The four moves, `< >` = the request's values:

```python
a = Ascii1d
a.reset(); a.setInputFolder("~/<in>")                    # file on disk -> table
made = a.importFiles("<file>.dat")                       # ["<file>"]

a.reset(); a.setOutputFolder("~/<out>"); a.setExtension("<ext>")   # table -> file
written = a.exportTables("<table>")                     # ["/abs/path/<out>/<table>.<ext>"]

a.reset(); a.setOutputFolder("~/<out>"); a.setExtension("<ext>")   # files -> files, nothing in the project
written = a.convertFiles("~/<in>/*.dat")                # one file per source file

a.reset(); a.setPresentation("Kratky"); a.setAutoPrefix(True)      # table -> new table
made = a.convertTables(["<table1>", "<table2>"])         # ["Kratky-<table1>", "Kratky-<table2>"]
```

Options go between `reset()` and the run, for any move: `a.setRemovePoints(first=3)` before `convertFiles(...)`
cuts 3 points of every file it writes; `a.setResolution(True)` and `a.setResolutionGeometry(...)` before
`convertTables("<table>", "<new name>")` add the sigma column to the new table.

Then every task, in this order:

```python
a = Ascii1d
a.reset()                                  # 1. always first: options stay set between calls
a.setInputFolder("~/<folder>")             # 2. the request's folder: "~/..." or absolute, never relative
# 3. ONLY what the request says about the file — nothing for a plain file of numbers:
#    a.setFormat("cansas") / a.setFormat("nist")              a canSAS XML (.xml) or a NIST file
#    a.setHeader(<number of header lines>)                     "starts with a 5-line header" -> a.setHeader(5)
#    a.setHeader("<text of the last header line>")             "data after the line X" -> a.setHeader("X")
# 4. ONLY the options the request asks for: a.setRemovePoints(...), a.setPresentation(...), a.setResolution(True)...
made = a.importFiles("<file>")             # 5. run: no name unless the request names the result
t = table(made[0])                         #    use the name the run returned ("sample" for sample.dat)
```

The lines in `< >` are the request's values; an option the request does not ask for is not set — `setHeader()` on
a file without a header drops its first data points.

> **Names:** give `names` only when the request names the result — "import header.dat as the table
> \"HeaderData\"" is `importFiles("header.dat", "HeaderData")`. Otherwise use the default name: take it from what
> the run returned (`t = table(made[0])`), never from the file name (`table("opt.dat")` does not exist), and never
> invent one such as "Data".
>
> **Paths:** a run returns absolute file paths — use them. Python's own `open()` does not expand `~`: write
> `open(os.path.expanduser("~/x/a.RAD"))` or use the returned path.

```python
a = Ascii1d
a.reset()
a.setOutputFolder("~/data/export")
a.setExtension("RAD")
files = a.exportTables("QI-sample")         # table -> file; returns the file paths written
```

---

## The four directions

Every run is one call; it returns what it made — **table names** for a table destination, **absolute file paths**
for a file destination — in source order. A run that makes nothing raises `ValueError` with the reason; a dataset it
skipped (e.g. left empty by `setRemovePoints()`) is printed as a line `importFiles: File ... contains no data, and
is skipped` and listed by `lastMessages()`, while the others are made.

| Call | Direction | Sources |
|------|-----------|---------|
| `importFiles(files, names=[])` | file → table | file names: absolute, `~/...`, or relative to `inputFolder()`; wildcards in the file part (`"~/data/run_*.dat"`, `"*.dat"`) |
| `convertFiles(files, names=[])` | file → file | as above; files go to `outputFolder()` |
| `exportTables(tables, names=[])` | table → file | table names or wildcard patterns (`"QI-*"`); files go to `outputFolder()` |
| `convertTables(tables, names=[])` | table → table | table names or wildcard patterns |

`files`/`tables` and `names` are each one string or a list of strings, in any combination
(`importFiles("a.dat", "A")`, `importFiles("a.dat", ["A"])`, `importFiles(["a.dat", "b.dat"], ["A", "B"])`).

> **Warning:** `convertTables()` refuses a result whose default name is its own source (`convertTables("QI-a")`
> with no prefix would refill `QI-a` with its processed data): give `names`, use `setAutoPrefix(True)` with a
> presentation (`Kratky-QI-a`), or `setIndexing(True)`.

### Names

* **Given names win.** `names[i]` names the output of source *i* exactly: a table name
  (`importFiles("a.dat", "Sample1")`), or a file name in `outputFolder()` (`exportTables("QI-a", "a3.dat")`), or a
  path. A file name without an extension gets `extension()`: `exportTables("QI-a", "a3")` writes `a3.RAD`. An existing table with that name is filled again (replaced). At most one name per source; `""` keeps the
  default for that source.
* **Default names** (no name given) follow the tool's name structure:
  * the source's base name with `_` and other non-alphanumerics replaced by `-`: `run_102.dat` → table
    `run-102`, file `run-102.RAD`;
  * `setAutoPrefix(True)`: the presentation as prefix — `QI-run-102`, `Kratky-run-102` (default: no prefix);
  * `setNameNumber(k)`: the k-th number in the source name instead of the name, always with the presentation
    prefix (`run_102_d01.dat`, k=1 → `QI-102`);
  * `setIndexing(True)`: a new unique name instead of replacing an existing table or file — tables `...-v-1`,
    `...-v-2`, …; files `...-v-0.RAD`, `...-v-1.RAD`, …;
  * files: the extension `extension()` (default `RAD`).

> **Warning:** Without a given name and without `setIndexing(True)`, importing the same file again **replaces**
> the table made before (same default name). Give each run its own name when both results are needed.

---

## Options

Options stay set until changed or `reset()` — call `reset()` at the start of every task, then set only what the
task asks. Each setter raises `ValueError` for an invalid value, naming the valid ones.

### Reading files

| Call | Notes |
|------|-------|
| `setFormat(format)` / `format()` | `"ascii"` (numbers on every line), `"ascii+header"` (the default after `reset()`, with 0 header lines), `"cansas"` (canSAS 1D XML), `"nist"` (6-column NIST, header up to "The 6 columns are"). Changing the format resets the header: call `setHeader()` after `setFormat()` |
| `setHeader(linesOrMarker)` / `header()` | a number (`setHeader(5)` or `"5"`): skip that many header lines — a file that "starts with a 5-line text header"; any other text: skip up to and including the line containing it — `setHeader("***** Data *****")` for a GRASP file. Switches the format to `"ascii+header"` |
| `setColumns(n)` / `columns()` | 2–4: the columns written (Q, I, dI, sigma); default 4 |
| `setInputFolder(folder)` / `inputFolder()` | where relative file names are read |

> **Warning:** Every line with two or more numbers is read as a data point (Q, I, dI, sigma = its first numbers).
> A header line such as `Beam Centre1 : Cx = 64.4493 Cy = 64.1428` or `# date: 2026-10-19` becomes a wrong point
> unless the header is skipped. **Whenever a file has a header — a request says "header", "starts with N text
> lines", or names the line the data follow — call `setHeader(N)` or `setHeader("<that line>")` before the import.**

### Writing

| Call | Notes |
|------|-------|
| `setOutputFolder(folder)` / `outputFolder()` | where files are written (created if missing) |
| `setExtension(ext)` / `extension()` | file extension without the dot, default `RAD` |
| `setAutoPrefix(on)`, `setNameNumber(k)`, `setIndexing(on)` | default names, see above |

### Processing (applied in this order, in every direction)

| Call | Notes |
|------|-------|
| `setRemovePoints(first=0, last=0, from=0, to=0, negative=False)` | drop `first` points at the start, `last` at the end, points `from`..`to` (1-based, both > 0), and with `negative` every point with I ≤ 0 |
| `setPresentation(to, from="QI")` / `presentations()` | write another SAS presentation: `"Guinier"` (ln I vs Q²), `"Zimm"`, `"Porod"`, `"Porod2"`, `"logQ"`, `"logI"`, `"Debye"`, `"1Moment"`, `"2Moment"`, `"GuinierRod"`, `"GuinierPlate"`, `"Kratky"` (I·Q² vs Q); `to == from` switches it off |
| `setResolution(on)` | add the instrument resolution sigma(Q) as column 4 (Sigma, role xErr) |
| `setResolutionGeometry(collimationCm, detectorCm, pixelCm=0.525, sampleApertureWmm=10, sampleApertureHmm=10, collimationApertureWmm=30, collimationApertureHmm=30, lambdaA=6, deltaLambda=0.1)` | SANS geometry in the tool's units: **centimetres and millimetres, not metres** — collimation and detector distance and pixel size in cm, apertures in mm, the wavelength in Å, deltaLambda as Δλ/λ. 8 m collimation and 4 m detector distance are `setResolutionGeometry(800, 400, ...)`; a distance below 50 raises ValueError (it was given in metres) |
| `setResolutionLenses(on, focus=0)` | lenses, focus in cm |
| `resolutionSigma(q)` | sigma at one Q with the current geometry |
| `setMath(op, factor=1, op2="", factor2=1)` | I op factor, then I op2 factor2 (`"+"`, `"-"`, `"*"`, `"/"`; `""` = off): "scale by 2, then subtract 0.01" is `setMath("*", 2, "-", 0.01)` |
| `setCalculator(op, column)` | I op another curve, point by point — `column` is a Y column (`"buffer_I"`) or a table (its 2nd column); both must have the same number of points (otherwise the dataset is kept unchanged and the run prints why); `op=""` = off |
| `setBinning(every=1, ratio=1)` | average every `every` points (a last incomplete group is dropped: 20 points, every 3 → 6); `ratio` > 1: progressive binning with that common ratio (1.0–1.995) |

### Plotting and saved settings

| Call | Notes |
|------|-------|
| `setPlot(on, xError=False, yError=True)` | add every table made to the **active graph** (only for a table destination; with no active graph nothing is plotted and the run prints so) |
| `saveFormat(name)` / `loadFormat(name)` / `formats()` | the tool's saved settings ("formats"), shared with the GUI's Session Tools |
| `lastMessages()` | the skipped datasets of the last run |

---

## Examples

```python
# GRASP output: data after the "***** Data *****" line, the header holds numeric lines
a = Ascii1d
a.reset()
a.setHeader("***** Data *****")
t = table(a.importFiles("~/data/sample_grasp.dat")[0])    # table "sample-grasp"
```

```python
# every file of a folder as a table, with the QI- prefix
a.reset()
a.setAutoPrefix(True)
names = a.importFiles("~/data/run_*.dat")        # ["QI-run-101", "QI-run-102", ...]
```

```python
# file -> file: cut 2 points at each end, add the SANS resolution, write .RAD files
a.reset()
a.setRemovePoints(first=2, last=2)
a.setResolution(True)
a.setResolutionGeometry(800, 400, 0.525, 10, 10, 30, 30, 6, 0.1)   # 8 m, 4 m, 6 Å, 10 %
a.setOutputFolder("~/data/converted")
files = a.convertFiles("~/data/*.dat")
```

```python
# table -> table: Kratky presentation of every QI- table
a.reset()
a.setAutoPrefix(True)
a.setPresentation("Kratky")
made = a.convertTables("QI-*")                   # ["Kratky-QI-a", ...]
```

```python
# table -> file with 3 columns (no sigma) under a given name
a.reset()
a.setOutputFolder("~/export")
a.setColumns(3)
a.exportTables("QI-alpha", "alpha3.dat")
```
