# QtiSAS Application Python API

Global functions available in every QtiSAS Python script, bound to the running `ApplicationWindow` instance and imported automatically.

> **Note:** SIP-wrapped methods accept positional arguments for all parameters. Optional (defaulted) parameters also accept keyword syntax if their names appear in the binding; required (non-defaulted) parameters are positional-only.

---

## Quick Start

```python
# create a table, fill it, and plot
t = newTable("MyData", 10, 3)
t.setColName(1, "x")
t.setColName(2, "y")
for i in range(1, 11):
    t.setCell(1, i, i * 0.1)
    t.setCell(2, i, i * i * 0.01)
g = plot(t, ("x", "y"), 1)
```

```python
# create a matrix and display it as a colour map
m = newMatrix("Z", 64, 64)
m.setFormula("sin(x) * cos(y)")
m.calculate()
plot(m)
```

```python
# navigate folders
f = addFolder("Results")
changeFolder(f)
t = newTable("output")
```

```python
# iterate over all tables and close empty ones
for t in tables():
    if t.numRows() == 0:
        closeWindow(t)
```

```python
# keyword arguments — optional named parameters can be passed by name
g = graphWindow("MyPlot")
saveGraphAsProject(g, "/data/results", "figure1", format="PDF")
saveGraphAsProject(g, "/data/results", "figure1", compress=True, format="PNG", resolution=300)

t = table("MyData")
n = newNote("StemPlot")
n.setText(stemPlot(t, "y", power=2, startRow=5, endRow=50))   # power=n: stem unit = 10^n; omit to get dialog

changeFolder(rootFolder(), force=True)

plot3D(matrix("Z"), style=4)   # 1=wireframe, 2=hidden-line, 3=filled, 4=filled+mesh, 5=dots, 6=bars
```

---

## Python Operator Pitfall: `^` vs `**`

> **`^` in Python is bitwise XOR — not power. Use `**` for exponentiation.**

| Goal | Wrong ✗ | Correct ✓ |
|------|---------|-----------|
| Raise `a` to the power `b` | `a ^ b` | `a ** b` |
| Raise 2 to the 10th | `2 ^ 10` → `8` (XOR!) | `2 ** 10` → `1024` |

`^` performs a **bitwise XOR** on integers; a standalone `a ^ b` expression also **discards the result** — `a` is left unchanged.

```python
a = 3
b = 4

f = a ^ b      # f = 7   (bitwise XOR: 011 ^ 100 = 111)
a ^ b          # result is 7 but immediately discarded — a is still 3

p = a ** b     # p = 81  (3 to the power 4)
```

> **muParser note:** in muParser (formula bar, matrices, and table formulas run with `forceMuParser=True` or "Apply muParser"), `^` is the power operator. Table formulas otherwise run in the global language — with Python, use `**` or `pow()`.

---

## Tables

| Call | Returns | Notes |
|------|---------|-------|
| `table("name")` | `Table` | look up existing table by object name (current folder first, then whole project) |
| `newTable()` | `Table` | blank table (30 rows, 2 cols) |
| `newTable("name", rows, cols)` | `Table` | if a table named `"name"` already exists, resize and clear it; otherwise create new |
| `currentTable()` | `Table` | currently active table window |
| `tables()` | `list[Table]` | all tables in the project |
| `tableNames()` | `list[str]` | object names of all tables (cheaper than `tables()`) |

> **Cross-type name collisions rename silently.** The "reuse if it exists" logic in
> `newTable`/`newMatrix`/`newGraph`/`newNote` only checks for a window of the SAME type. If `name`
> belongs to a DIFFERENT-type window (e.g. a `Note` already named "X" when you call `newTable("X")`),
> that logic never triggers — but the constructor still enforces a project-wide unique-name rule, so
> it silently renames the new window (e.g. to "X2"): no exception, no dialog, no return-value
> signal. Check `t.objectName()` afterward if the exact name matters.

---

## Matrices

| Call | Returns | Notes |
|------|---------|-------|
| `matrix("name")` | `Matrix` | look up existing matrix by object name |
| `newMatrix()` | `Matrix` | blank matrix (32×32) |
| `newMatrix("name", rows, cols)` | `Matrix` | if a matrix named `"name"` already exists, resize it in place; otherwise create new — **shrinking truncates**: existing rows/cols beyond the new size are permanently lost, no warning |
| `currentMatrix()` | `Matrix` | currently active matrix window |
| `matrices()` | `list[Matrix]` | all matrices in the project |
| `matrixNames()` | `list[str]` | object names of all matrices |

---

## 2D Graphs

### Getting a graph window

| Call | Returns | Notes |
|------|---------|-------|
| `graphWindow("name")` | `MultiLayer` | look up existing graph by object name |
| `currentGraph()` | `MultiLayer` | currently active graph window |
| `newGraph(name="Graph1", layers=1, rows=1, cols=1, applyPreferences=True)` | `MultiLayer` | if a graph named `"name"` exists AND `layers<=1`, returns it untouched; if it exists AND `layers>1` with a differing row/col grid, it DOES rearrange the existing layer grid in place (never touching curve/plot data) — otherwise creates a blank graph. **Do not use with `plot()`** — `plot()` always creates its own window |
| `graphs()` | `list[MultiLayer]` | all graph windows in the project |

---

### Plotting from a Table

| Call | Returns | Notes |
|------|---------|-------|
| `plot(table, (col, …), style=1)` | `MultiLayer` | creates a new graph window and plots columns; **do not call `newGraph()` before this** |
| `plot(table, "colName", style=1)` | `MultiLayer` | creates a new graph window and plots a single column; **do not call `newGraph()` before this** |

`style` is a `Graph.CurveType` integer value: `0` = Line, `1` = Scatter, `2` = LineSymbols, `3` = VerticalBars, `10` = HorizontalBars.

> **`plot()` always creates a new graph window** with an auto-generated name (`"Graph1"`, `"Graph2"`, …) and returns it. Do **not** call `newGraph()` first — it's redundant and left empty.
> `g` is a `GraphWindow` (`MultiLayer`) with **no** `replot()` method — use `g.activeLayer().replot()` to force a redraw, or `g.activeLayer().addCurve(...)` to add curves.

> **For a specific graph name** — use `newGraph("name")` + `addCurve()` instead; `plot()` cannot assign a custom name (rename after with `setWindowName(g, "name")`).

```python
# quick plot — auto-named "Graph1", "Graph2", ...
g = plot(t, ("x", "y"), style=Graph.CurveType.LineSymbols)

# named graph — use newGraph + addCurve (preferred for templates and scripts)
g = newGraph("Template")
g.activeLayer().addCurve(t, "y", Graph.CurveType.LineSymbols)
```

---

### Plotting from a Matrix

| Call | Returns | Notes |
|------|---------|-------|
| `plot(matrix, style)` | `MultiLayer` | plot a matrix as a spectrogram; `style` is a `Graph.CurveType` integer value |

`Graph.CurveType.ColorMap` (16), `Graph.CurveType.GrayScale` (17), `Graph.CurveType.Contour` (18).

```python
m = matrix("Z")
g = plot(m, Graph.CurveType.ColorMap)
```

---

### Plotting a function

| Call | Returns | Notes |
|------|---------|-------|
| `plot("f(x)", xmin, xmax, points=100)` | `MultiLayer` | plot an analytic function string over [xmin, xmax] |

```python
g = plot("sin(x)*exp(-x/10)", 0, 30, 500)
```

---

### Waterfall and profile plots

| Call | Returns | Notes |
|------|---------|-------|
| `waterfallPlot(table, (col, …))` | `MultiLayer` or `None` | offset stack of curves from multiple columns — returns `None` (no dialog) if the column tuple is empty |
| `plotImageProfiles(matrix)` | `MultiLayer` | horizontal and vertical line profiles through the matrix |
| `stemPlot(table, "colName", power=1001, startRow=0, endRow=-1)` | `str` | stem-and-leaf plot; `power=1001` (default) auto-estimates the stem unit and **opens a dialog** asking for `n`; pass an explicit integer to skip the dialog |

```python
t = table("MyData")
g = waterfallPlot(t, ("I1", "I2", "I3"))
```

---

### Saving a graph as project + image

| Call | Returns | Notes |
|------|---------|-------|
| `saveGraphAsProject(graph, path, name, compress=False, format="", resolution=-1)` | — | save graph as a `.qti` project and export an image side-by-side |

- `path` — directory (e.g. `"/data/results"`)
- `name` — base filename without extension (e.g. `"figure1"`)
- `compress` — `True` writes a compressed `.qti.gz`; default `False` → `.qti`
- `format` — image format string; `""` uses the global setting. Accepted values: `"PNG"`, `"PDF"`, `"SVG"`, `"TEX"`, `"PS"`, `"EPS"`, `"Archive Graph"` (exports PDF + SVG + PNG + TIFF together)
- `resolution` — image DPI; `-1` uses the global setting

```python
g = graphWindow("MyPlot")
saveGraphAsProject(g, "/data/results", "figure1")               # .qti + image (global format/DPI)
saveGraphAsProject(g, "/data/results", "figure1", True)         # .qti.gz + image
saveGraphAsProject(g, "/data/results", "figure1", False, "PDF") # .qti + .pdf
saveGraphAsProject(g, "/data/results", "figure1", False, "PNG", 300)  # .qti + .png at 300 DPI
```

---

## 3D Plots

> **Prefer `newPlot3D(name, ...)` over `plot3D(...)` + `setWindowName(g, name)`** when the window
> needs a specific name: `plot3D(...)` always makes a brand-new, nameless window, so a following
> `setWindowName()` raises `ValueError: name already exists` if that name is taken — and by then
> `plot3D` has already created ANOTHER orphaned window, since creation always succeeds and only the
> rename fails. `newPlot3D(name, ...)` checks the existing window FIRST — updates it in place if
> found, else creates it fresh (already named, atomically) — so it never leaks an orphan. If `name`
> belongs to a NON-`Graph3D` window (names are unique across ALL window types), it raises
> `ValueError: newPlot3D: name '<name>' already exists but is not a Graph3D window` immediately,
> before creating anything. Full form: see graph3d-python-api.md's "Getting a Graph3D" section.

### Getting a 3D window

| Call | Returns | Notes |
|------|---------|-------|
| `newPlot3D("name")` | `Graph3D` | create-or-update by name — see the recommendation above |
| `plot3D("name")` | `Graph3D` | look up existing 3D graph by object name |
| `newPlot3D(name="")` | `Graph3D` | blank 3D window (empty surface) |

---

### Plotting from a Table column

| Call | Returns | Notes |
|------|---------|-------|
| `newPlot3D(name, table, "colName", style=0)` | `Graph3D` | recommended — see above |
| `plot3D(table, "colName", style=0)` | `Graph3D` | XYZ scatter/ribbon from a Z column with X/Y from preceding columns; `style`: `0`=ribbons, `1`=bars, `2`=cones |

---

### Plotting from a Matrix

| Call | Returns | Notes |
|------|---------|-------|
| `newPlot3D(name, matrix, style=5)` | `Graph3D` | recommended — see above |
| `plot3D(matrix, style=5)` | `Graph3D` | surface from a matrix; `style`: `1`=wireframe, `2`=hidden-line, `3`=filled (no edges), `4`=filled+mesh, `5`=dots, `6`=bars |

---

### Plotting a surface formula

| Call | Returns | Notes |
|------|---------|-------|
| `newPlot3D(name, "z(x,y)", xl, xr, yl, yr, zl, zr, cols=40, rows=40)` | `Graph3D` | recommended — see above |
| `plot3D("z(x,y)", xl, xr, yl, yr, zl, zr, cols=40, rows=40)` | `Graph3D` | analytic surface Z = f(x, y) |
| `plot3D("fx", "fy", "fz", umin, umax, vmin, vmax, cols=40, rows=40, openU=True, openV=True)` | `Graph3D` | parametric surface (fx, fy, fz as functions of u, v) |

```python
g = newPlot3D("SinCos3D", "sin(sqrt(x*x+y*y))", -6, 6, -6, 6, -1, 1)
```

---

## Notes

| Call | Returns | Notes |
|------|---------|-------|
| `note("name")` | `Note` | look up existing note by object name |
| `newNote(name="")` | `Note` | if a note named `"name"` already exists, return it; otherwise create new |
| `currentNote()` | `Note` | currently active note window |
| `notes()` | `list[Note]` | all note windows in the project |

```python
n = newNote("analysis")
n.setWindowLabel("My analysis")
n.setText("print('hello from note')")
n.currentEditor().execute()
```

---

## Folder Management

Every QtiSAS project has a tree of folders; windows live inside folders.

| Call | Returns | Notes |
|------|---------|-------|
| `activeFolder()` | `Folder` | the currently selected folder |
| `rootFolder()` | `Folder` | the top-level project folder |
| `addFolder("name", parent=None)` | `Folder` | create a new sub-folder; `parent=None` adds under `activeFolder()` |
| `deleteFolder(folder)` | `bool` | remove folder and all its contents — **shows a blocking confirmation dialog by default** (the "confirm close folder" preference defaults to on); can hang an unattended script with no way to dismiss it |
| `changeFolder(folder, force=False)` | `bool` | switch active folder; there is no confirmation dialog involved — `force=True` re-runs the folder switch/refresh even if `folder` is already the active one (otherwise a no-op) |
| `copyFolder(src, dest)` | `bool` | copy all windows from `src` into `dest` |
| `saveFolder(folder, "path", compress=False)` | — | save one folder as a project file |
| `appendProject("path", parent=None)` | `Folder` | load an external project file and attach it as a sub-folder |

```python
results = addFolder("Results")
changeFolder(results)
t = newTable("summary", 5, 2)
saveFolder(results, "/data/results.qti")
```

---

## Project and Windows

### Project lifecycle

| Call | Returns | Notes |
|------|---------|-------|
| `newProject(save=None)` | `bool` | empty project in this window; `save` as for `closeProject()` |
| `closeProject(save=None)` | `bool` | close the project: `None` asks if unsaved (Cancel → `False`), `False` discards, `True` saves first (`ValueError` if it has no file yet) |
| `openProject("path", save=None)` | `bool` | close the current project (`save` as above), then open a `.qti`/`.qti.gz`/`.qti.svg` file in this window; `ValueError` if missing or not a project |
| `saveProject(compress=False)` | `bool` | save to the current file path; returns `True` on success |
| `saveProjectAs("path", compress=False)` | — | save to an explicit path (forced to end in `.qti`); `compress=True` additionally gzips it to `<path>.qti.gz` — NOT `.qtz` |

> All three keep the current main window, so a running script (and the script server) keeps working. In a
> script, prefer `save=False` or `save=True`: `save=None` opens a dialog, which blocks an unattended run.

---

### Window lifecycle

| Call | Returns | Notes |
|------|---------|-------|
| `existWindow("name")` | `bool` | `True` if any window with that object name exists anywhere in the project |
| `existTable("name")` | `bool` | `True` only if a `Table` with that name exists |
| `existNote("name")` | `bool` | `True` only if a `Note` with that name exists |
| `closeWindow(window)` | — | close and delete a window outright — no "hide or delete?" prompt |
| `activateWindow(window)` | — | bring a window to the front (accepts `MdiSubWindow` or name string) |
| `activateWindow("name")` | `bool` | activate by name; returns `True` if found |
| `hideWindow(window)` | — | hide a window without closing it |
| `maximizeWindow(window)` | — | maximize a window (accepts `MdiSubWindow` or name string); call `activateWindow` first |

---

### Window arrangement and templates

| Call | Returns | Notes |
|------|---------|-------|
| `openTemplate("path")` | `MdiSubWindow` | open a saved window template file |
| `saveAsTemplate(window, "path")` | — | save a single window as a reusable template |
| `clone(window)` | `MdiSubWindow` | duplicate a window into the active folder |
| `setWindowName(window, "name")` | `bool` | rename a window — raises `ValueError` if the name is empty, invalid, or already used by another window (see gotcha below). **Allowed: letters, digits, underscore, hyphen** — confirmed in source (`ApplicationWindow::setWindowName()` strips `-` before checking; the check itself is `\W`, which excludes `_`). The runtime error text ("only letters and digits are allowed") is misleading: `_`/`-` are fine, e.g. `"SphereMap_3D"` is valid, not just `"SphereMap3D"`. Only spaces/punctuation beyond `_`/`-` are rejected. |
| `autoArrangeLayers()` | — | tile all visible windows |
| `setPreferences(graph)` | — | apply current global preferences to a `Graph` layer — takes a `Graph`, **not** a `GraphWindow`; preferences are auto-applied at layer creation so this is only needed to re-apply after manual overrides |
| `windows()` | `list[MdiSubWindow]` | all open windows regardless of type |
| `workspace()` | `QMdiArea` | the central MDI area widget |

### Graph preference getters (qti.app.graph*)

| Call | Returns | Notes |
|------|---------|-------|
| `graphTitleOn()` | `bool` | whether layer titles are shown by default (QtiSAS Preferences → 2D Plots) |
| `graphDrawBackbones()` | `bool` | whether axis backbones are drawn by default |
| `graphAntialiasing()` | `bool` | whether antialiasing is on by default |
| `graphAutoscale()` | `bool` | whether autoscale is on by default |
| `graphAxesLineWidth()` | `int` | default axes line width |
| `graphMajTicksStyle()` | `int` | default major tick style (0=none, 1=out, 2=in, 3=both) |
| `graphMinTicksStyle()` | `int` | default minor tick style |
| `graphMajTicksLength()` | `int` | default major tick length (pixels) |
| `graphMinTicksLength()` | `int` | default minor tick length (pixels) |
| `graphCanvasFrameWidth()` | `int` | default canvas frame width |
| `graphDefaultMargin()` | `int` | default plot margin (pixels) |
| `graphTickLabelsDist()` | `int` | default distance between ticks and tick labels |
| `graphAxesLabelsDist()` | `int` | default distance between axes title and backbone |
| `graphDefaultCurveStyle()` | `int` | default curve style index |
| `graphDefaultSymbolSize()` | `int` | default symbol size |
| `graphDefaultCurveLineWidth()` | `float` | default curve line width |
| `graphCanvasColor()` | `QColor` | default canvas background colour |
| `graphBackgroundColor()` | `QColor` | default outer background colour |
| `graphBorderColor()` | `QColor` | default border colour |
| `graphAxesFont()` | `QFont` | default axes tick-label font |
| `graphTitleFont()` | `QFont` | default plot title font |
| `graphLegendFont()` | `QFont` | default legend font |
| `graphNumbersFont()` | `QFont` | default axis numbers font |

> **The `graph*` preference getters above are NOT global** — real `ApplicationWindow` methods,
> deliberately excluded from the auto-imported global namespace. Bare `graphTitleOn()` raises
> `NameError`; use `qti.app.graphTitleOn()` (see Common Mistakes below). Most OTHER `qti.app`
> methods — including `windows()`/`workspace()` in the Window arrangement table above — ARE
> available as global functions without the prefix.

```python
# save, close, reopen
saveProjectAs("/data/results/run1.qti")
closeProject(save=False)              # discard anything changed since
openProject("/data/results/run1.qti")
newProject(save=False)

# existTable/existNote are type-specific — safe to call table()/note() after
if existTable("RawData"):
    closeWindow(table("RawData"))
```

---

## Conversions

| Call | Returns | Notes |
|------|---------|-------|
| `tableToMatrix(table)` | `Matrix` | copy table data into a new matrix (direct, column-by-column) |
| `tableToMatrixRegularXYZ(table, "zColName")` | `Matrix` | interpolate irregular (x, y, z) triplets onto a regular grid; the named column is Z |
| `matrixToTable(matrix, mode=Direct)` | `Table` | convert matrix data to a table |

`mode` is one of:

| Constant | Meaning |
|----------|---------|
| `ApplicationWindow.MatrixToTableConversion.Direct` | cell (i,j) → row, Z column |
| `ApplicationWindow.MatrixToTableConversion.XYZ` | three columns: X, Y, Z (row-major scan) |
| `ApplicationWindow.MatrixToTableConversion.YXZ` | three columns: Y, X, Z (column-major scan) |

```python
m = matrix("RawData")
t = matrixToTable(m, ApplicationWindow.MatrixToTableConversion.XYZ)
```

---

## Output and Display

| Call | Returns | Notes |
|------|---------|-------|
| `scriptPrint(*values, sep=" ")` | — | append a line to the scripting console (use inside scripts and Notes); like `print()`: values converted with `str()`, joined by spaces |
| `scriptPrintReplace("text")` | — | replace the last console line (useful for progress updates in loops) |
| `displayInfo("text")` | — | append a line to the status bar info log |
| `resultsLog()` | `QTextEdit` | the Results Log text widget (append text with `.append()`) — an `ApplicationWindow` method; call as `resultsLog()` (global) or `qti.app.resultsLog()`. Not `c.resultsLog()` on a `Compiler` object — `Compiler` has no `resultsLog()`. |
| `infoLineEdit()` | `QLineEdit` | the status bar line edit widget |

```python
for i, t in enumerate(tables()):
    scriptPrintReplace(f"Processing {i+1}/{len(tables())}: {t.objectName()}")

resultsLog().append("Fit converged: chi² = 1.23")
displayInfo("Processing complete.")
```

---

## Running Scripts from the Command Line

| Command | Effect |
|---------|--------|
| `qtisas -x script.py` | start QtiSAS and run the script; QtiSAS stays open (on error the script window shows it) |
| `qtisas -x --quit script.py` | run, then quit: exit code 0 on success, 1 on error (error text on stderr) |
| `qtisas -X script.py` | run without GUI, then quit: exit code 0/1 as above; 2D plots are not handled correctly without GUI |
| `qtisas -x "print(1)"` | the argument is run as code when it is not a readable file (a missing `*.py` is reported instead) |

In a script run this way:
- `__file__` is the script's full path; the working directory is where QtiSAS was started, not the script's folder.
- `sys.argv` is `['']`: arguments cannot be passed to the script.
- `print()` goes to the QtiSAS console, not the terminal.
- `globals` is QtiSAS's shared-variables module (`globals.x = 1`), not Python's `globals()`; use `builtins.globals()`.
- `sys.executable` is the QtiSAS application, not a Python interpreter: to run a script in plain Python use
  `os.path.join(sys.base_prefix, "python.exe" if os.name == "nt" else "bin/python3")`.

## Colour Palette

The indexed colour list, user-configurable in preferences, is used by `setCurveLineColor(curveIndex, colorIndex)` and `setCurveSymbolColor(curveIndex, colorIndex|QColor)`.

| Call | Returns | Notes |
|------|---------|-------|
| `colorNames()` | `list[str]` | ordered list of colour names (e.g. `["black", "red", ...]`) |
| `colorIndex("name")` | `int` | index of that name; returns `-1` if not found |

> `colorIndex()` returns a palette **index** (`int`), **not** a `QColor` — it cannot be passed to `QPen`, `QBrush`, or any Qt class expecting a colour object. Use `QColor("red")` or `QColor(r, g, b)` when a `QColor` is required.

```python
print(colorNames())          # ['black', 'red', 'green', 'blue', ...]
i = colorIndex("red")        # 1 — palette index (int)
layer.setCurveLineColor(0, i)             # ok: setCurveLineColor accepts int index
layer.setCurveLineColor(0, QColor("red")) # ok: also accepts QColor

# WRONG — colorIndex() is not a QColor:
QPen(colorIndex("black"), 1)             # TypeError
QBrush(colorIndex("red"))               # TypeError
addErrorBars(..., colorIndex("gray"), ...)  # TypeError — color= expects QColor
```

## Symbol Palette

The indexed symbol list, user-configurable in preferences, is used by `setCurveSymbolStyle(curveIndex, style)`.

| Call | Returns | Notes |
|------|---------|-------|
| `symbolNames()` | `list[str]` | ordered list of symbol names |
| `symbolIndex("name")` | `int` | `QwtSymbol::Style` value; `-1` = no symbol / not found |

Available names: `"no symbol"` `"ellipse"` `"circle"` `"rectangle"` `"diamond"` `"triangle"` `"down triangle"` `"up triangle"` `"left triangle"` `"right triangle"` `"cross"` `"diagonal cross"` `"horizontal line"` `"vertical line"` `"star 1"` `"star 2"` `"hexagon"`

(`"circle"` is an alias for `"ellipse"`)

```python
print(symbolNames())                              # ['ellipse', 'rectangle', ...]
layer.setCurveSymbolStyle(0, symbolIndex("diamond"))
```

---

## Data Import

| Call | Returns | Notes |
|------|---------|-------|
| `importWaveFile()` | `Table` | open an ILL/HZB `.wav` wave file via a dialog and import it into a new table |

---

## Data and Table State

| Call | Returns | Notes |
|------|---------|-------|
| `autoUpdateTableValues()` | `bool` | `True` if formula columns recalculate automatically on edit |
| `setAutoUpdateTableValues(on)` | — | enable or disable automatic formula recalculation project-wide |
| `columnsList(role)` | `list[str]` | all column full-names (e.g. `"MyTable_I"`) matching the given plot role |

`role` values: `Table.PlotDesignation.X`, `.Y`, `.Z`, `.xErr`, `.yErr`, `.Label`, `.None_`.  
Call `columnsList()` with no argument to get all columns (default `All = -1`).

```python
# pause recalc during bulk edits, then resume
setAutoUpdateTableValues(False)
for t in tables():
    for row in range(1, t.numRows() + 1):
        t.setCell(2, row, t.cell(2, row) * 1e-3)
setAutoUpdateTableValues(True)

# list all Y columns in the project
y_cols = columnsList(Table.PlotDesignation.Y)
print(y_cols)   # ['Sample_I', 'Background_I', ...]

# list all columns
all_cols = columnsList()
```

---

## Custom Actions

Custom actions are named Python scripts that appear in a QtiSAS menu and can be triggered from the GUI or a script, stored as `.py` files and registered with a `.qca` descriptor in `customActionsPath()`.

### Workflow

Create a custom action in three steps, in order:

1. **Write the Python script** — save it into `customActionsPath()/scripts/`:
   ```python
   path = customActionsPath()
   with open(path + "/scripts/my_action.py", "w") as f:
       f.write("# your QtiSAS Python code here\n")
   ```

2. **Provide an icon** *(optional)* — copy or create an image file (PNG, SVG, etc.) into `customActionsPath()/icons/`.

3. **Register the action** — call `addCustomAction` with the bare filenames (not full paths):
   ```python
   addCustomAction("My Action", "my_action.py",
                   menuName="scriptingMenu",
                   iconName="my_action.png",   # omit if no icon
                   shortcut="Ctrl+Shift+M",    # omit if no shortcut
                   tooltip="What this action does")
   ```

> **Important:** `scriptName`/`iconName` are bare filenames only — the function prepends
> `customActionsPath()/scripts/` and `customActionsPath()/icons/` automatically.

---

### API

| Call | Returns | Notes |
|------|---------|-------|
| `addCustomAction(name, scriptName, menuName="", iconName="", shortcut="", tooltip="")` | `bool` | create a new custom action and add it to the menu; returns `True` on success |
| `updateCustomActionName(name, newName)` | `bool` | rename an existing action (renames the `.qca` file too); returns `True` if found |
| `updateCustomActionScript(name, newScriptName)` | `bool` | replace the script file for an existing action; returns `True` if found |
| `updateCustomActionIcon(name, newIconName)` | `bool` | replace (or clear with `""`) the icon for an existing action; returns `True` if found |
| `updateCustomActionMenu(name, newMenuName)` | `bool` | move an existing action to a different menu; returns `True` if found |
| `removeCustomAction(name)` | `bool` | remove action by name and delete its `.qca`; returns `True` if found |
| `runCustomAction(name)` | `bool` | execute the named action's script; returns `True` if found |
| `customActionNames()` | `list[str]` | names of all currently registered custom actions |
| `customActionsPath()` | `str` | directory where `.qca` descriptors and scripts are stored |

---

**Storage directory by OS:**

| OS | Path |
|----|------|
| Windows | `%USERPROFILE%\AppData\Local\qtisas\python-actions` |
| Linux / macOS | `~/.config/qtisas/python-actions` |

Directory layout:
```
python-actions/
├── MyAction.qca          ← action descriptors (XML)
├── AnotherAction.qca
├── scripts/
│   ├── MyAction.py       ← Python scripts
│   └── AnotherAction.py
└── icons/
    ├── MyAction.png      ← optional action icons
    └── AnotherAction.svg
```

---

**Parameters for `addCustomAction`:**

- `name` — display name shown in the menu; also used as the `.qca` filename
- `scriptName` — filename of the Python script inside `customActionsPath()/scripts/` (e.g. `"export.py"`)
- `menuName` — objectName (e.g. `"scriptingMenu"`) or visible title (e.g. `"Scripting"`) of the target menu; defaults to `scriptingMenu` when empty
- `iconName` — filename of the icon inside `customActionsPath()/icons/` (e.g. `"export.png"`); `""` shows a generic placeholder
- `shortcut` — keyboard shortcut string, e.g. `"Ctrl+Shift+R"`; `""` sets no shortcut
- `tooltip` — tooltip text; defaults to `name` when empty

---

### Custom Actions — worked examples

```python
import os

# write the script into the scripts/ subfolder
scripts_dir = os.path.join(customActionsPath(), "scripts")
with open(os.path.join(scripts_dir, "export_tables.py"), "w") as f:
    f.write("""
for t in tables():
    t.exportASCII(customActionsPath() + "/" + t.objectName() + ".csv", ",", False, False)
scriptPrint("Exported " + str(len(tables())) + " tables.")
""")

# register — pass the bare filename, not the full path
addCustomAction("Export all tables", "export_tables.py",
                menuName="scriptingMenu",
                shortcut="Ctrl+Shift+E", tooltip="Export every table to CSV")

# list all registered actions
print(customActionNames())   # ['Export all tables', ...]

# run it programmatically
runCustomAction("Export all tables")

# remove it
removeCustomAction("Export all tables")
```

---

```python
# with icon — file must exist in customActionsPath()/icons/
addCustomAction("Export all tables", "export_tables.py",
                menuName="scriptingMenu", iconName="export.png")
```

```python
# minimal — no menu, no icon, no shortcut
addCustomAction("My Script", "my_script.py")
```

```python
# create a family of actions in a loop
actions = [("Normalize", "norm.py"), ("Smooth", "smooth.py"), ("Fit Peaks", "fitpeak.py")]
for name, fname in actions:
    addCustomAction(name, fname, menuName="scriptingMenu")
print(customActionNames())
```

---

## Special Module Widgets

These globals expose the main UI widgets of optional QtiSAS modules.

| Name | Type | Module |
|------|------|--------|
| `Dan` | `dan18` | SANS data reduction (DAN) |
| `Fittable` | `fittable18` | curve fitting module |
| `Compiler` | `compile18` | function compiler |

```python
# access the DAN module and trigger a recalculation
Dan.recalculate()

# access the Fittable module
Fittable.fit()
```

---

## Complete Example

```python
# load data, reduce background, fit, and save
t_data = table("Sample")
t_bg   = table("Background")

# subtract background into a new table
n = t_data.numRows()
t_sub = newTable("Subtracted", n, 3)
t_sub.setColName(1, "Q")
t_sub.setColName(2, "I")
t_sub.setColName(3, "dI")
for row in range(1, n + 1):
    t_sub.setCell(1, row, t_data.cell(1, row))
    t_sub.setCell(2, row, t_data.cell(2, row) - t_bg.cell(2, row))
    t_sub.setCell(3, row, t_data.cell(3, row))

# plot
g = plot(t_sub, ("Q", "I"), 1)

# save
saveProjectAs("/data/result.qti")
```

---

---

## For AI: Common Mistakes

### `import math` is NEVER correct in QtiSAS scripts

`sin`, `cos`, `exp`, `log`, `sqrt`, `pi`, `e`, `floor`, `ceil`, `abs` are all global — never prefix with `math.` or write `import math`.

```python
# WRONG
import math
y = math.exp(-x * x)

# RIGHT — exp() is global; no import needed
y = exp(-x * x)
```

### Graph preference getters — always use `qti.app.` prefix

Bare names raise `NameError` — always use the `qti.app.` prefix.

```python
# WRONG — NameError: name 'graphTitleOn' is not defined
titleOn = graphTitleOn()

# RIGHT
titleOn = qti.app.graphTitleOn()
```

### EXPLORE graph defaults before creating a template

Read defaults first; override only what differs:

```python
# EXPLORE — check graph defaults before creating a template
scriptPrint("titleOn=%s  majTicks=%d  canvasColor=%s" % (
    qti.app.graphTitleOn(),
    qti.app.graphMajTicksStyle(),
    qti.app.graphCanvasColor().name()
))
```

### `plot()` creates its own window — do not call `newGraph()` before it

```python
# WRONG — newGraph() creates an empty orphan window; plot() ignores it
g = newGraph("MyGraph")
plot(t, ("x", "y"), Graph.CurveType.Line)

# RIGHT
g = plot(t, ("x", "y"), Graph.CurveType.Line)
setWindowName(g, "MyGraph")
```

### `setWindowName()` raises `ValueError` on a naming conflict — no dialog, script stops

```python
# WRONG — assumes silent success; if "MyGraph" is already taken, the OLD dialog behaviour
# is gone: there is no popup to dismiss, the call raises immediately and the script stops
setWindowName(g, "MyGraph")
plot2 = plot(t2, ("x", "y"))
setWindowName(plot2, "MyGraph")  # ValueError: setWindowName: name 'MyGraph' already exists.

# RIGHT — check first, or catch the exception, if the name might collide
existing = [w.objectName() for w in windows()]
name = "MyGraph"
if name in existing:
    name = f"{name}_2"
setWindowName(g, name)
```

> The `_2` suffix is genuinely valid — underscores and hyphens are allowed, despite the runtime
> error's "only letters and digits" wording (see the character-rule note above). Confirmed live: a
> script avoided an underscore-suffixed name (`"SphereMap_3D"`), wrongly believing it too would be
> rejected, and settled for a disconnected generic name (`"surface"`) — losing the traceable link
> to its source object for no reason.

### Graph3D: use `newPlot3D(name, ...)`, not `plot3D(...)` + `setWindowName()`

```python
# WRONG — re-running this (e.g. a later turn asking to redo/tweak the same plot) raises
# ValueError: setWindowName: name 'Surface3D' already exists — AND plot3D(...) already created
# a second, orphaned, auto-named window before that error was even raised
g3 = plot3D("sin(x)*cos(y)", -pi, pi, -pi, pi, -1, 1, 60, 60)
setWindowName(g3, "Surface3D")

# RIGHT — updates the existing "Surface3D" window in place if one exists, creates it fresh
# (already named) otherwise; never raises, never leaks an orphaned window
g3 = newPlot3D("Surface3D", "sin(x)*cos(y)", -pi, pi, -pi, pi, -1, 1, 60, 60)
```

> Confirmed live: after this collision, a retry tried `graphWindow("Surface3D")` — but
> `graphWindow()` is 2D-`GraphWindow`-ONLY, always returning `None` for a `Graph3D` name and raising
> `AttributeError` on the next line. The correct lookup is `plot3D("name")`, never
> `graphWindow("name")` — though `newPlot3D("name", ...)` above makes that lookup unnecessary.

### `colorIndex()` returns an `int`, not a `QColor`

```python
# WRONG — TypeError: QPen/QBrush/addErrorBars expect QColor, not int
QPen(colorIndex("black"), 1)
addErrorBars(..., colorIndex("gray"), ...)

# RIGHT — use QColor when a colour object is required
from PyQt6.QtGui import QColor, QPen
QPen(QColor("black"), 1)
addErrorBars(..., QColor("gray"), ...)
```

### `changeFolder(activeFolder())` is a guaranteed no-op — never a way to "activate" anything

`changeFolder(folder, force=False)` bails out immediately when `folder` is already the active
one (see the Folder Management table above); `activeFolder()` always returns exactly that folder,
so `changeFolder(activeFolder())` does nothing at all, by construction, every time. Folders and a
window's own state (e.g. a Note's active tab) are unrelated — this call cannot "activate",
refresh, or confirm anything about a table/graph/note just created.

Confirmed live: generated as an attempt to make a just-created Note's script tab "active" before
relying on autoexec, paired with a `scriptPrint()` falsely reporting success — the note only had
one tab (no `addTab()` call was ever made), so there was nothing to activate in the first place,
and this line was invented busywork rather than a real action.

```python
# WRONG — does nothing; folders have nothing to do with a Note's tabs
n = newNote("Setup")
n.setAutoexec(True)
changeFolder(activeFolder())
scriptPrint("Note is now active and will auto-execute on project open.")  # false — nothing happened

# RIGHT — nothing needed: a note with only one tab has no other tab to "activate"
n = newNote("Setup")
n.setAutoexec(True)
```
