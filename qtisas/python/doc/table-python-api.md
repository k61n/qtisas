# QtiSAS Table Python API

Python scripting interface for the `Table` class.  
Obtain a table via `table("name")`, `newTable()`, or `currentTable()`.

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

> **Warning:** Column index comes **before** row index in all cell access methods — `t.cell(col, row)`, `t.setCell(col, row, value)`. This is the **opposite** of Matrix which uses `(row, col)`.

Wherever a column argument is accepted it can be:

- **`int`** — 1-based column index (column 1 is the first column)
- **`str`** — short label (e.g. `"y"`) or full name (e.g. `"MyData_y"`)

Row arguments are always 1-based integers.

---

## Getting a Table

| Call | Returns | Notes |
|------|---------|-------|
| `table("name")` | `Table` or `None` | look up existing table by object name; returns `None` if no table with that name exists |
| `newTable()` | `Table` | blank table (30 rows, 2 cols) |
| `newTable("name", rows, cols)` | `Table` | blank table with given size |
| `currentTable()` | `Table` | currently active table |

> **Warning:** `table("name")` returns `None` (confirmed), not an error and not a placeholder object, when no table with that name exists. Always check `if t is None:` before using the result — `t.cell(...)` on `None` raises `AttributeError`.

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

> **Warning:** avoid `_` in column labels. QtiSAS uses `_` as the separator between table name and column name (e.g. `"MyTable_I"`), so a label like `"I_true"` will be misidentified when looked up by name.

| Method | Returns | Notes |
|--------|---------|-------|
| `colName(col)` | `str` | short label only, e.g. `"I"` |
| `colNames()` | `tuple` | full names, e.g. `("MyTable_x", "MyTable_I")` |
| `setColName(col, label)` | — | rename one column |
| `setColNames(["x","y",...])` | — | set all column labels at once |

---

## Reading Cells

> **Note:** Argument order is **(col, row)** — column first, row second, both 1-based. For example, `t.cell(2, 3)` reads column 2, row 3. This is unusual compared to most table APIs.

| Method | Returns | Notes |
|--------|---------|-------|
| `text(col, row)` | `str` | raw cell text |
| `cell(col, row)` | `float` | numeric value; raises `ValueError` if empty |
| `cellData(col, row)` | `str` or `float` | type-aware: float for numeric cols, str for text |
| `rowData(row)` | `tuple` | all cells of a row |
| `colData(col)` | `list` | all cells of a column |
| `t[n]` | `tuple` | row at **0-based** index `n`; equivalent to `rowData(n+1)`; supports negative indices (`t[-1]` = last row) |
| `t[start:stop]` | `list` | standard Python slice (0-based) as list of tuples |

---

## Writing Cells

> **Note:** Argument order is **(col, row)** — column first, row second, both 1-based. For example, `t.setCell(2, 3, 1.0)` writes to column 2, row 3.

| Method | Notes |
|--------|-------|
| `setText(col, row, value)` | write string to cell |
| `setCell(col, row, value)` | write float to cell |
| `setCellData(col, row, value)` | type-aware write (str or float) |
| `setRowData(row, tuple)` | overwrite entire row from a tuple |
| `t[row] = tuple` | same as `setRowData` |
| `appendRowData(tuple)` | add a new row at the end; returns new row index |
| `setColData(col, iterable, startRow=0)` | fill column from any iterable; `startRow` is 0-based offset |

---

## Deleting Rows and Columns

| Call | Notes |
|------|-------|
| `deleteRows(start, end)` | delete rows `start`…`end` (1-based, inclusive) |
| `del t[row]` | delete single row (0-based Python index) |
| `del t[start:stop]` | delete row slice (0-based Python indices) |
| `removeCol(col)` | delete column |

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
| `sort(type=0, order=0, leadCol="")` | sort whole table; `type` 0=single, 1=grouped; `order` 0=asc, 1=desc |
| `sortColumn(col, order=0)` | sort one column independently |
| `sortColumns(cols_tuple, type=0, order=0, leadCol="")` | sort a subset of columns |

---

## Normalization

| Method | Notes |
|--------|-------|
| `normalize(col)` | divide all values in column `col` by its maximum value |
| `normalize()` | normalize all numeric columns at once |

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
t.setColumnRole("I",  Table.PlotDesignation.Y)
t.setColumnRole("dI", Table.PlotDesignation.yErr)
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
| `setColNumericFormat(col, fmt, prec)` | `fmt`: `Table.NumericFormat.*`; `prec` = significant digits |
| `setColDateFormat(col, fmtStr)` | e.g. `"yyyy-MM-dd"` |
| `setColTimeFormat(col, fmtStr)` | e.g. `"hh:mm:ss"` |
| `setColMonthFormat(col, fmtStr)` | e.g. `"MMM"` (Jan), `"MMMM"` (January), `"MM"` (01); column must contain integers 1–12 |
| `setColDayFormat(col, fmtStr)` | e.g. `"ddd"` (Mon), `"dddd"` (Monday); column must contain integers 1–7 (1=Mon) |
| `setColumnWidth(col, px)` | set column width in pixels |
| `hideColumn(col, hide=True)` | hide or show column |
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
| `recalculate(col, startRow=1, endRow=-1)` | evaluate formula; returns `True` on success |

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
| `isRowSelected(row)` → `bool` | — |
| `isColSelected(col)` → `bool` | — |
| `firstSelectedColumn()` → `int` | — |
| `numSelectedRows()` → `int` | — |
| `setSelectedCol(col)` | move selection to column |
| `selectedColumn()` → `int` | currently selected column |

---

## Import / Export

### importASCII

All arguments are **positional only** (parameters are unnamed in the SIP binding):

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
    2,                    # 10 mode: 0=NewColumns, 1=NewRows, 2=Overwrite
    # args 11-15 (locale, startRow, endRow, colFilter, colDestinations) use defaults
)
```

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
x  = t.colData("x")     # list of floats (empty cells become 0)
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
t.setColNames(["Q", "I", "dI", "sigma"])
t.setColumnRole("Q",     Table.PlotDesignation.X)
t.setColumnRole("I",     Table.PlotDesignation.Y)
t.setColumnRole("dI",    Table.PlotDesignation.yErr)
t.setColumnRole("sigma", Table.PlotDesignation.xErr)
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

Table uses **(col, row)** order. Matrix uses **(row, col)**. Mixing them silently reads/writes the
wrong cell. Both indices are **1-based**.

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

Math functions (`exp`, `sin`, `cos`, `sqrt`, `pi`, `log`, …) are global in QtiSAS — `import math`
raises `ImportError`. Use the bare names directly.

All other standard library modules (`random`, `os`, `sys`, `re`, `json`, `numpy`, …) are **not**
global and must be imported explicitly before use.

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

`t.plot("x", "y", Graph.CurveType.Line)` creates and returns a `GraphWindow`. Calling `newGraph()`
before it creates an extra, empty, unused window.

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

After filling cells in a loop, call `t.notifyChanges()` to refresh the view and downstream plots.
Skipping it leaves the table display stale.

**RIGHT:**
```python
for i in range(1, n + 1):
    t.setCell(1, i, x[i-1])
    t.setCell(2, i, y[i-1])
t.notifyChanges()
```

### Rule 5 — `cell()` raises on empty cells; EXPLORE before reading

`cell(col, row)` raises `ValueError` (surfaced as `SystemError`) if the cell is empty.
A freshly created table has **all cells empty**.

**Before reading from any table whose content is uncertain, use EXPLORE** to check whether
the table exists and its cells are populated. Only after EXPLORE confirms data should you
call `cell()`.

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
