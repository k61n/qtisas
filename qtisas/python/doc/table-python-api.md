# QtiSAS Table Python API

Python scripting interface for `Table`. Obtain one via `table("name")`, `newTable()`, or `currentTable()`.

---

## Quick Start

```python
# create a 5-row, 3-column table
t = newTable("MyData", 5, 3)
t.setColNames(["x", "y", "dy"])
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(2, Table.PlotDesignation.Y)
t.setColumnRole(3, Table.PlotDesignation.yErr)

for i in range(1, 6):
    t.setCell(1, i, i * 0.1)
    t.setCell(2, i, i * i * 0.01)
    t.setCell(3, i, 0.001)

# read back
print(t.cell(2, 3))    # numeric value of column 2, row 3
print(t.colData(2))    # full column as list
```

```python
# access an existing table and import ASCII data
t = table("RawData")
t.importASCII("/path/to/data.dat", " ", 2, True, True, True)
```

---

## Column Addressing

> **Warning:** Column index comes **before** row in all cell access methods — `t.cell(col, row)`, `t.setCell(col, row, value)` — the **opposite** of Matrix's `(row, col)`.

A column argument can be:

- **`int`** — 1-based column index (column 1 is the first column)
- **`str`** — short label (e.g. `"y"`) or full name (e.g. `"MyData_y"`)

Row arguments are always 1-based.

> **Exception: `colName(col)` only accepts `int`** — not the string-accepting pattern other
> column-arg methods use. `t.colName("y")` raises `TypeError` (int required).

---

## Getting a Table

| Call | Returns | Notes |
|------|---------|-------|
| `table("name")` | `Table` or `None` | look up table by object name; `None` if not found — see warning below |
| `newTable()` | `Table` | blank table (30 rows, 2 cols) |
| `newTable("name", rows, cols)` | `Table` | blank table with given size — if `"name"` already exists, resizes AND clears it in place instead of creating a second table, silently destroying its data (no exception, no dialog) |
| `currentTable()` | `Table` | currently active table |

> **Warning:** `table("name")` returns `None` (not an error/placeholder) when not found — check `if t is None:` before use; `t.cell(...)` on `None` raises `AttributeError`.

---

## Size

| Method | Returns | Notes |
|--------|---------|-------|
| `numRows()` | `int` | number of rows |
| `numCols()` | `int` | number of columns |
| `len(t)` | `int` | same as `numRows()` |
| `setNumRows(n)` | — | resize rows |
| `setNumCols(n)` | — | resize columns |
| `addRow()` | `int` | append one row; returns new row index |
| `addColumn(pd=Table.PlotDesignation.Y)` | — | append column with given plot designation |

---

## Column Names

> **Warning:** avoid `_` in column labels — QtiSAS uses `_` as the table-name/column-name separator (e.g. `"MyTable_I"`), so `"I_true"` gets misidentified when looked up by name.

| Method | Returns | Notes |
|--------|---------|-------|
| `colName(col)` | `str` | short label only, e.g. `"y"` — `col` must be `int` here (see exception above) |
| `colNames()` | `tuple` | short labels only, e.g. `("x", "y", "dy")` — **NEVER table-name-prefixed** |
| `setColName(col, label, updateTableDesignations=False)` | `bool` | rename one column — raises `ValueError` if `label` is empty or already taken in this table |
| `setColNames(["x","y",...])` | — | set all column labels at once |

> **`colNames()`/`colName(col)` return the BARE label, never `"tablename_colname"`** (Fittable's
> dataset-naming convention: `configure()`, `f.datasets()`, `setWeightingDataset()`, ...) — both
> call `Table::colLabel()`, not the prefixing `Table::colName(int)` C++ method Python never invokes.
> So a fully-qualified name check inside `colNames()` never matches, even if the column exists.
> ```python
> # WRONG — colNames() never contains the "table_col" form, this check ALWAYS reports "missing"
> if "GaussPeak_y" not in t.colNames():
>     scriptPrint("missing")   # prints even though column "y" genuinely exists
>
> # RIGHT — check the bare label; only build the "table_col" string when calling Fittable
> if "y" not in t.colNames():
>     scriptPrint("missing")
> f.configure(model, [t.objectName() + "_y"])   # the "table_col" form belongs here, not in colNames()
> ```
> Confirmed live: this check (`"GaussPeak_y" not in t.colNames()`) blocked a model-comparison step
> and the final plotting step — both printed "missing required y or dy columns" and skipped real
> work, while split-mode reported both parts "done."

---

## Reading Cells

> **Note:** Argument order is **(col, row)**, both 1-based — e.g. `t.cell(2, 3)` reads column 2, row 3 (unusual vs. most table APIs).

| Method | Returns | Notes |
|--------|---------|-------|
| `text(col, row)` | `str` | raw cell text |
| `cell(col, row)` | `float` | numeric value; raises `ValueError` if empty |
| `cellData(col, row)` | `str` or `float` | type-aware: float for numeric cols, str for text |
| `rowData(row)` | `tuple` | all cells of a row — an empty numeric cell becomes `None`, NOT `0` |
| `colData(col)` | `list` | all cells of a column — an empty numeric cell becomes `None`, NOT `0` |
| `t[n]` | `tuple` | row at **0-based** index `n`; equivalent to `rowData(n+1)`; supports negative indices (`t[-1]` = last row) |
| `t[start:stop]` | `list` | standard Python slice (0-based) as list of tuples |

> **Warning:** Negative indices work for `t[n]`/`t[start:stop]` but **NOT** for
> `cell()`/`text()`/`cellData()`/`setCell()` — those are 1-based only and raise `ValueError: There's
> no row -1 in table <name>!` on negative row/col (see example below). Confirmed live.

> **`t.text(col, row)` used to raise `TypeError: decoding str is not supported` for every
> non-empty cell, on any table** (Python-3-porting bug: an already-decoded `str` hit a redundant
> second decode step, which CPython rejects). Fixed at the source (`qtimod.sip`) as of this build.
> On older builds, use `cellData(col, row)` instead (unaffected, same text).

> **`t.cell(col, row)` silently returns `0.0` when the text can't be parsed as a number** — it only
> raises on a genuinely EMPTY cell; no way to tell "really 0.0" from "parsing failed." Confirmed
> live: reading a text-label cell by mistake gave `0.0` with no error. Never use `cell()` on an
> unconfirmed-numeric column — use `text()`/`cellData()` and parse it yourself so failures stay
> visible.

```python
# WRONG — cell() doesn't support Python-style negative indexing, unlike t[n]
last_q = t.cell(1, -1)          # ValueError: There's no row -1 in table SANS_Data!

# RIGHT — use numRows() for the last row, or t[-1] for bracket access to the whole row
last_q = t.cell(1, t.numRows())
last_row_tuple = t[-1]          # bracket access DOES support -1
```

---

## Writing Cells

> **Note:** Argument order is **(col, row)**, both 1-based — e.g. `t.setCell(2, 3, 1.0)` writes column 2, row 3.

| Method | Notes |
|--------|-------|
| `setText(col, row, value)` | write string to cell |
| `setCell(col, row, value)` | write float to cell |
| `setCellData(col, row, value)` | type-aware write (str or float) |
| `setRowData(row, tuple)` | overwrite entire row from a tuple — `row` is **1-based** |
| `t[row] = tuple` | **0-based, negative-index-capable** (`t[0]` writes row 1, `t[-1]` writes the last row) — converts to `setRowData(row+1, tuple)`; unlike `setRowData`'s own 1-based `row` above |
| `appendRowData(tuple)` | add a new row at the end; returns new row index |
| `setColData(col, data, offset=0)` | fill column from any iterable; keyword is `offset` (not `startRow`), 0-based |

---

## Deleting Rows and Columns

| Call | Notes |
|------|-------|
| `deleteRows(start, end)` | delete rows `start`…`end` (1-based, inclusive) — raises `ValueError` if any column is read-only (whole op aborted) |
| `del t[row]` | delete single row (0-based Python index) — same read-only restriction, raises `ValueError` |
| `del t[start:stop]` | delete row slice (0-based Python indices) — same read-only restriction, raises `ValueError` |
| `removeCol(col)` | delete column — silently skips read-only columns (no dialog, no exception) |

---

## Inserting / Swapping Columns

| Method | Notes |
|--------|-------|
| `insertColumns(col, n)` | insert `n` empty columns before `col` |
| `swapColumns(col1, col2)` | swap two columns |

---

## Sorting

| Method | Notes |
|--------|-------|
| `sort(type=0, order=0, leadCol="")` | sort whole table; `type` 0=single, 1=grouped; `order` 0=asc, 1=desc — raises `ValueError` if `type=1` and `leadCol` is missing/empty/blank |
| `sortColumn(col, order=0)` | sort one column independently |
| `sortColumns(cols_tuple, type=0, order=0, leadCol="")` | sort a subset of columns — same `leadCol` restriction as `sort()`, raises `ValueError` |

---

## Normalization

| Method | Notes |
|--------|-------|
| `normalize(col)` | divide all values in column `col` by its maximum value |
| `normalize()` | normalize all numeric columns at once — silently skips read-only columns (no dialog, no exception) |

---

## Plot Designation and Format

### Designation constants

Access via `Table.PlotDesignation.<name>` — plain integers are **not** accepted by `setColumnRole`.

| Constant | Value | Meaning |
|----------|-------|---------|
| `Table.PlotDesignation.None_` | 0 | no role |
| `Table.PlotDesignation.X` | 1 | X axis |
| `Table.PlotDesignation.Y` | 2 | Y axis |
| `Table.PlotDesignation.Z` | 3 | Z axis |
| `Table.PlotDesignation.xErr` | 4 | x error bar |
| `Table.PlotDesignation.yErr` | 5 | y error bar |
| `Table.PlotDesignation.Label` | 6 | label column |

```python
t.setColumnRole("y",  Table.PlotDesignation.Y)
t.setColumnRole("dy", Table.PlotDesignation.yErr)
```

---

### NumericFormat constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `Table.NumericFormat.Default` | 0 | automatic (application default) |
| `Table.NumericFormat.Decimal` | 1 | fixed decimal notation |
| `Table.NumericFormat.Scientific` | 2 | scientific notation (e.g. `1.23e-4`) |

---

### Format methods

| Method | Notes |
|--------|-------|
| `setColTextFormat(col)` | mark column as text |
| `setColNumericFormat(col, format, prec, updateCells=True)` | `format` (not `fmt`): `Table.NumericFormat.*`; `prec` = significant digits |
| `setColDateFormat(col, fmtStr, updateCells=True)` | e.g. `"yyyy-MM-dd"` |
| `setColTimeFormat(col, fmtStr, updateCells=True)` | e.g. `"hh:mm:ss"` |
| `setColMonthFormat(col, fmtStr, updateCells=True)` | e.g. `"MMM"` (Jan), `"MMMM"` (January), `"MM"` (01); column must contain integers 1–12 |
| `setColDayFormat(col, fmtStr, updateCells=True)` | e.g. `"ddd"` (Mon), `"dddd"` (Monday); column must contain integers 1–7 (1=Mon) |
| `setColumnWidth(col, px)` | set column width in pixels |
| `hideColumn(col, on=True)` | hide or show column — keyword is `on`, not `hide`; `hideColumn(1, hide=True)` raises `TypeError` |
| `showAllColumns()` | un-hide all columns |
| `setDecimalSeparators(locale)` | 0=system, 1=English (dot), 2=German (comma) |

---

## Comments

| Method | Notes |
|--------|-------|
| `comment(col)` → `str` | get column comment |
| `setComment(col, text)` | set column comment |
| `showComments(on=True)` | show or hide comment row |

---

## Column Formula

| Method | Notes |
|--------|-------|
| `setCommand(col, expr)` | assign a formula expression to a column |
| `recalculate(col, startRow=1, endRow=-1, forceMuParser=False, notifyChanges=True)` | evaluate formula; returns `True` on success — raises `ValueError` if the column is read-only or the formula fails to evaluate |

---

## Filling with Random Values

| Method | Notes |
|--------|-------|
| `setRandomValues(col, startRow=1, endRow=-1)` | uniform random values in \[0, 1\] |
| `setNormalRandomValues(col, startRow=1, endRow=-1, sd=1.0)` | Gaussian random values (mean 0); `startRow`, `endRow`, `sd` accept keyword syntax |

---

## Read-Only / Selection

| Method | Notes |
|--------|-------|
| `setReadOnlyColumn(col, readOnly=True)` | prevent editing of column |
| `isRowSelected(row, full=False)` → `bool` | — |
| `isColSelected(col, full=False)` → `bool` | — |
| `firstSelectedColumn()` → `int` | — |
| `numSelectedRows()` → `int` | — |
| `setSelectedCol(col)` | move selection to column |
| `selectedColumn()` → `int` | currently selected column |

---

## Import / Export

### importASCII

All arguments are **positional only** (unnamed in the SIP binding):

```python
t.importASCII(
    "/path/to/data.dat",  # 1  file
    "\t",                 # 2  sep
    0,                    # 3  ignoredLines: header lines to skip
    False,                # 4  renameCols: use first data line as col headers
    True,                 # 5  stripSpaces
    False,                # 6  simplifySpaces: collapse multiple spaces
    False,                # 7  importComments
    "#",                  # 8  commentString
    False,                # 9  readOnly
    Table.ImportMode.Overwrite,  # 10 mode — see enum below
    # args 11-15 (locale, startRow, endRow, colFilter, colDestinations) use defaults
)
```

> **`mode` is an actual enum (`Table.ImportMode`), not a plain int** — same strict SIP enum parsing
> as `Table.PlotDesignation` above. A raw literal (`t.importASCII(..., 2)`) raises `TypeError`; use
> `Table.ImportMode.Overwrite` etc. as shown.

| Constant | Value | Meaning |
|----------|-------|---------|
| `Table.ImportMode.NewColumns` | 0 | append imported data as new columns |
| `Table.ImportMode.NewRows` | 1 | append imported data as new rows |
| `Table.ImportMode.Overwrite` | 2 | replace existing contents |

---

### exportASCII

All arguments are **positional only**:

```python
ok = t.exportASCII(
    "/path/to/output.dat",  # 1 file
    "\t",                   # 2 sep
    False,                  # 3 withLabels: include column header row
    False,                  # 4 exportSelection: export only selected cells
    False,                  # 5 exportComments: include comment row
)
```

Raises `ValueError` if the file can't be opened for writing (bad path/permissions) — no dialog, script stops.

---

## Miscellaneous

| Method | Notes |
|--------|-------|
| `notifyChanges()` | trigger a repaint/recalculation of dependent windows |
| `scrollToCell(col, row)` | scroll the table view to make the cell visible |

---

## Examples

### Build a table from Python lists

```python
q  = [0.01 * i for i in range(1, 51)]
I  = [exp(-q_**2 * 1000) for q_ in q]   # exp() is global — no import needed
dI = [0.01 * v for v in I]

t = newTable("Guinier", len(q), 3)
t.setColNames(["Q", "I", "dI"])
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(2, Table.PlotDesignation.Y)
t.setColumnRole(3, Table.PlotDesignation.yErr)
t.setColNumericFormat(1, Table.NumericFormat.Default,    6)
t.setColNumericFormat(2, Table.NumericFormat.Scientific, 4)
t.setColNumericFormat(3, Table.NumericFormat.Scientific, 4)

t.setColData(1, q)
t.setColData(2, I)
t.setColData(3, dI)
```

---

### Read a column into a Python list

```python
t  = table("MyData")
x  = t.colData("x")     # list of floats (empty cells become None, NOT 0 — differs from cell(),
                         # which raises ValueError on an empty numeric cell instead)
y  = t.colData("y")
n  = t.numRows()

# iterate row by row
for row in range(1, n + 1):
    xv = t.cell("x", row)
    yv = t.cell("y", row)
    print(f"  row {row:3d}: x={xv:.4f}  y={yv:.4E}")
```

---

### Read rows as tuples (slice syntax)

```python
t = table("MyData")

# single row — t[n] uses 0-based indexing
first = t[0]    # first row  (= rowData(1))
fourth = t[3]   # fourth row (= rowData(4))
last  = t[-1]   # last row   (negative index supported)

# first 10 rows (standard Python slice, 0-based)
block = t[0:10]
for vals in block:
    print(vals)
```

---

### Append rows one at a time

```python
t = newTable("Log", 0, 3)
t.setColNames(["step", "chi2", "Rg"])
t.setColumnRole(1, Table.PlotDesignation.X)

for step in range(1, 6):
    t.appendRowData((step, step * 0.5, 10.0 + step))
```

---

### Column formula and recalculate

```python
t = table("MyData")

# add x^2 column
t.addColumn(Table.PlotDesignation.X)
t.setColName(t.numCols(), "x2")
t.setCommand("x2", "pow(col(\"x\"), 2)")  # pow() works in both MuParser and Python
t.recalculate("x2")

# add ln(y) column
t.addColumn(Table.PlotDesignation.Y)
t.setColName(t.numCols(), "lny")
t.setCommand("lny", "ln(col(\"y\"))")
t.recalculate("lny")
```

---

### Sort by a column

```python
t = table("Results")

# sort all columns together, descending by "chi2" (grouped sort)
t.sort(type=1, order=1, leadCol="chi2")

# sort a single column independently
t.sortColumn("chi2", order=0)   # ascending
```

---

### Import ASCII and assign column roles

```python
t = newTable("Raw")
t.importASCII(
    "/data/sample.dat",
    " ",    # sep
    3,      # ignoredLines: skip 3 header lines
    True,   # renameCols: use line 4 as column headers
    True,   # stripSpaces
    True,   # simplifySpaces
    False,  # importComments
    "#",    # commentString
)

# set column names, then assign roles
t.setColNames(["x", "y", "dy", "dx"])
t.setColumnRole("x",  Table.PlotDesignation.X)
t.setColumnRole("y",  Table.PlotDesignation.Y)
t.setColumnRole("dy", Table.PlotDesignation.yErr)
t.setColumnRole("dx", Table.PlotDesignation.xErr)
t.notifyChanges()
```

---

### Export to a space-separated file with headers

```python
t = table("FitSummary")
t.exportASCII("/output/summary.txt", " ", True)
```

---

### Add comments to columns

```python
t = table("MyData")
t.setComment("x",  "independent variable")
t.setComment("y",  "measured value")
t.setComment("dy", "statistical error")
t.showComments(True)
```

---

### Insert a derived column between existing ones

```python
t = table("MyData")          # has columns x, y, dy  (indices 1, 2, 3)

# insert 1 empty column before column 2 (between x and y)
t.insertColumns(2, 1)
t.setColName(2, "x3")
t.setColumnRole(2, Table.PlotDesignation.X)
t.setCommand("x3", "pow(col(\"x\"), 3)")
t.recalculate("x3")

# move dy (now at index 4) next to y (index 3) — they are already adjacent, just an example
t.swapColumns(3, 4)
```

---

### Generate test data with noise

```python
t = newTable("Noise", 100, 4)
t.setColNames(["Q", "Itrue", "noise", "Iobs"])
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(4, Table.PlotDesignation.Y)

# Q grid
t.setColData(1, [0.001 + 0.001 * i for i in range(100)])

# true model: Guinier Rg=50
t.setCommand("Itrue", "exp(-pow(col(1), 2) * 2500 / 3)")
t.recalculate("Itrue")

# Gaussian noise column (sd = 0.01)
t.setNormalRandomValues("noise", 1, -1, 0.01)

# observed = true + noise
t.setCommand("Iobs", "col(\"Itrue\") + col(\"noise\")")
t.recalculate("Iobs")
```

---

### Copy column data between tables

```python
src = table("RawData")
dst = table("ProcessedData")

n = min(src.numRows(), dst.numRows())
for row in range(1, n + 1):
    dst.setCell("y",  row, src.cell("y",  row))
    dst.setCell("dy", row, src.cell("dy", row))

dst.notifyChanges()
```

---

### Read-only result table

```python
t = newTable("FitResults", 3, 2)
t.setColNames(["parameter", "value"])
t.setColTextFormat(1)
t.setColumnRole(1, Table.PlotDesignation.Label)

params = [("Rg", 52.3), ("I0", 0.0041), ("bgd", 1.2e-4)]
for row, (name, val) in enumerate(params, start=1):
    t.setText(1, row, name)
    t.setCell(2, row, val)

# lock both columns against accidental edits
t.setReadOnlyColumn(1, True)
t.setReadOnlyColumn(2, True)
```

---

### Delete outlier rows above a threshold

```python
t = table("MyData")

# scan in reverse to keep row indices valid after deletion
for row in range(t.numRows(), 0, -1):
    try:
        val = t.cell("y", row)
    except ValueError:
        continue
    if val < 0 or val > 1e6:
        t.deleteRows(row, row)

t.notifyChanges()
```

Same pattern removes **empty rows**: an empty/non-numeric cell raises `ValueError` from `cell()`,
so treat that as "empty" instead of skipping it:
```python
t = table("MyData")
for row in range(t.numRows(), 0, -1):
    try:
        t.cell("y", row)  # any value at all -> row has data, keep it
    except ValueError:
        t.deleteRows(row, row)  # unreadable/empty -> remove
t.notifyChanges()
```
There is no `removeEmptyRows()`/`setColumn()`-style one-call helper. `Table` also cannot be
constructed directly (`Table("name")` raises `TypeError`; use the `table("name")` function).

---

### Normalize columns

```python
t = table("MyData")

# divide column "y" by its maximum value
t.normalize("y")

# normalize all numeric columns at once
t.normalize()
```

---

---

## For AI: Common Mistakes

### Rule 0 — There is no column role getter

`setColumnRole(col, role)` sets a column's plot designation, but there is no getter.
`columnDesignation()`, `columnRole()`, `plotDesignation()` — none of these exist.

**WRONG:**
```python
role = t.columnDesignation(1)   # AttributeError: 'Table' object has no attribute 'columnDesignation'
```

**RIGHT — infer roles from column names or EXPLORE output:**
```python
# EXPLORE — use colNames() to discover columns; infer X/Y from naming conventions
scriptPrint("cols: " + str(t.colNames()))
```

### Rule 1 — Cell order is (col, row) — opposite of Matrix

Table uses **(col, row)**; Matrix uses **(row, col)**. Mixing them silently reads/writes the wrong
cell. Both indices are **1-based**.

**WRONG (Matrix habit applied to Table):**
```python
t.setCell(row, col, value)   # wrong order for Table
```

**RIGHT:**
```python
t.setCell(col, row, value)   # column first
v = t.cell(col, row)
t.text(col, row)
```

### Rule 2 — Math functions are global; all other standard library modules must be imported

Math functions (`exp`, `sin`, `cos`, `sqrt`, `pi`, `log`, …) are global — `import math` raises
`ImportError`; use bare names.

All other stdlib modules (`random`, `os`, `sys`, `re`, `json`, `numpy`, …) are **not** global —
import explicitly before use.

**WRONG:**
```python
import math
t.setCell(1, i, math.exp(-x))          # import math — fails
t.setCell(2, i, random.random() * 10)  # random not imported — NameError
```

**RIGHT:**
```python
import random
t.setCell(1, i, exp(-x))              # math function — global, no import
t.setCell(2, i, random.random() * 10) # random — must import
```

### Rule 3 — `plot()` creates its own graph window; don't call `newGraph()` first

`t.plot(...)` creates and returns a `GraphWindow`; calling `newGraph()` first just adds an extra,
empty, unused window.

**WRONG:**
```python
gw = newGraph()
gw = t.plot("x", "y", Graph.CurveType.Line)   # newGraph() window is now orphaned
```

**RIGHT:**
```python
gw = t.plot("x", "y", Graph.CurveType.Line)
```

### Rule 4 — `notifyChanges()` required after bulk cell writes

After filling cells in a loop, call `t.notifyChanges()` to refresh the view and downstream plots —
skipping it leaves the display stale.

**RIGHT:**
```python
for i in range(1, n + 1):
    t.setCell(1, i, x[i-1])
    t.setCell(2, i, y[i-1])
t.notifyChanges()
```

### Rule 5 — `cell()` raises on empty cells; EXPLORE before reading

`cell(col, row)` raises `ValueError` (surfaced as `SystemError`) on an empty cell — a freshly
created table has **all cells empty**.

**Before reading any table with uncertain content, use EXPLORE** to confirm it exists and has
populated cells — only then call `cell()`.

**WRONG — create or look up a table then immediately read without knowing cell state:**
```python
t = table("RawData") if existTable("RawData") else newTable("RawData", 10, 2)
vals = [t.cell(2, r) for r in range(1, t.numRows() + 1)]   # SystemError if cells are empty
```

**RIGHT — EXPLORE first, then read:**
```python
# After EXPLORE confirms "RawData" exists with populated numeric column 2:
t = table("RawData")
n = t.numRows()
vals = [t.cell(2, r) for r in range(1, n + 1)]
mean_val = sum(vals) / len(vals)
print(mean_val)
```
