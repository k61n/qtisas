# QtiSAS Application Python API

Global functions available in every QtiSAS Python script.  
They are bound to the running `ApplicationWindow` instance and imported automatically.

> **Note:** SIP-wrapped methods accept positional arguments for all parameters. Optional (defaulted) parameters whose names appear in the binding also accept keyword syntax. Required (non-defaulted) parameters are positional-only.

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
g = plot(t, ("MyData_x", "MyData_y"), 1)
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

stemPlot(t, "MyData_y", startRow=5, endRow=50)

changeFolder(rootFolder(), force=True)

plot3D(matrix("Z"), style=5)
```

---

## Tables

| Call | Returns | Notes |
|------|---------|-------|
| `table("name")` | `Table` | look up existing table by object name (current folder first, then whole project) |
| `newTable()` | `Table` | blank table (30 rows, 2 cols) |
| `newTable("name", rows, cols)` | `Table` | blank table with given size |
| `currentTable()` | `Table` | currently active table window |
| `tables()` | `list[Table]` | all tables in the project |
| `tableNames()` | `list[str]` | object names of all tables (cheaper than `tables()`) |

---

## Matrices

| Call | Returns | Notes |
|------|---------|-------|
| `matrix("name")` | `Matrix` | look up existing matrix by object name |
| `newMatrix()` | `Matrix` | blank matrix (32×32) |
| `newMatrix("name", rows, cols)` | `Matrix` | blank matrix with given size |
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
| `newGraph(name="Graph1", layers=1, rows=1, cols=1)` | `MultiLayer` | blank graph; `layers>1` creates a multi-panel layout with `rows×cols` panels |
| `graphs()` | `list[MultiLayer]` | all graph windows in the project |

### Plotting from a Table

| Call | Returns | Notes |
|------|---------|-------|
| `plot(table, (col, …), style=1)` | `MultiLayer` | plot one or more columns (full names, e.g. `"MyData_y"`); tuple of strings |
| `plot(table, "colName", style=1)` | `MultiLayer` | plot a single column given as a string |

`style` is a `Graph.CurveType` integer value: `0` = Line, `1` = Scatter, `2` = LineSymbols, `3` = VerticalBars, `10` = HorizontalBars.

```python
t = table("MyData")
g = plot(t, ("MyData_x", "MyData_y"), Graph.CurveType.Line)
```

### Plotting from a Matrix

| Call | Returns | Notes |
|------|---------|-------|
| `plot(matrix, style)` | `MultiLayer` | plot a matrix as a spectrogram; `style` is a `Graph.CurveType` integer value |

`Graph.CurveType.ColorMap` (16), `Graph.CurveType.GrayScale` (17), `Graph.CurveType.Contour` (18).

```python
m = matrix("Z")
g = plot(m, Graph.CurveType.ColorMap)
```

### Plotting a function

| Call | Returns | Notes |
|------|---------|-------|
| `plot("f(x)", xmin, xmax, points=100)` | `MultiLayer` | plot an analytic function string over [xmin, xmax] |

```python
g = plot("sin(x)*exp(-x/10)", 0, 30, 500)
```

### Waterfall and profile plots

| Call | Returns | Notes |
|------|---------|-------|
| `waterfallPlot(table, (col, …))` | `MultiLayer` | offset stack of curves from multiple columns |
| `plotImageProfiles(matrix)` | `MultiLayer` | horizontal and vertical line profiles through the matrix |
| `stemPlot(table, "colName", power=1001, startRow=0, endRow=-1)` | `str` | stem-and-leaf plot; result is returned as text |

```python
g = waterfallPlot(t, ("MyData_I1", "MyData_I2", "MyData_I3"))
```

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

### Getting a 3D window

| Call | Returns | Notes |
|------|---------|-------|
| `plot3D("name")` | `Graph3D` | look up existing 3D graph by object name |
| `newPlot3D(name="")` | `Graph3D` | blank 3D window (empty surface) |

### Plotting from a Table column

| Call | Returns | Notes |
|------|---------|-------|
| `plot3D(table, "colName", style=0)` | `Graph3D` | XYZ scatter/ribbon from a Z column with X/Y from preceding columns; `style`: `0`=ribbons, `1`=bars, `2`=cones |

### Plotting from a Matrix

| Call | Returns | Notes |
|------|---------|-------|
| `plot3D(matrix, style=5)` | `Graph3D` | surface from a matrix; `style`: `5`=no-mesh surface |

### Plotting a surface formula

| Call | Returns | Notes |
|------|---------|-------|
| `plot3D("z(x,y)", xl, xr, yl, yr, zl, zr, cols=40, rows=40)` | `Graph3D` | analytic surface Z = f(x, y) |
| `plot3D("fx", "fy", "fz", umin, umax, vmin, vmax, cols=40, rows=40, openU=True, openV=True)` | `Graph3D` | parametric surface (fx, fy, fz as functions of u, v) |

```python
g = plot3D("sin(sqrt(x*x+y*y))", -6, 6, -6, 6, -1, 1)
```

---

## Notes

| Call | Returns | Notes |
|------|---------|-------|
| `note("name")` | `Note` | look up existing note by object name |
| `newNote(name="")` | `Note` | create a new note window |
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

Every QtiSAS project has a tree of folders. Windows live inside folders.

| Call | Returns | Notes |
|------|---------|-------|
| `activeFolder()` | `Folder` | the currently selected folder |
| `rootFolder()` | `Folder` | the top-level project folder |
| `addFolder("name", parent=None)` | `Folder` | create a new sub-folder; `parent=None` adds under `activeFolder()` |
| `deleteFolder(folder)` | `bool` | remove folder and all its contents |
| `changeFolder(folder, force=False)` | `bool` | switch active folder; `force=True` skips confirmation |
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
| `newProject()` | — | close current project and start a blank one |
| `closeProject()` | — | close the current project (prompts to save if unsaved) |
| `saveProject(compress=False)` | `bool` | save to the current file path; returns `True` on success |
| `saveProjectAs("path", compress=False)` | — | save to an explicit path; `compress=True` writes a `.qtz` archive |

### Window lifecycle

| Call | Returns | Notes |
|------|---------|-------|
| `existWindow("name")` | `bool` | `True` if any window with that object name exists anywhere in the project |
| `existTable("name")` | `bool` | `True` only if a `Table` with that name exists |
| `existNote("name")` | `bool` | `True` only if a `Note` with that name exists |
| `closeWindow(window)` | — | close and destroy a window |
| `activateWindow(window)` | — | bring a window to the front (accepts `MdiSubWindow` or name string) |
| `activateWindow("name")` | `bool` | activate by name; returns `True` if found |
| `hideWindow(window)` | — | hide a window without closing it |
| `maximizeWindow(window)` | — | maximize a window (accepts `MdiSubWindow` or name string) |

### Window arrangement and templates

| Call | Returns | Notes |
|------|---------|-------|
| `openTemplate("path")` | `MdiSubWindow` | open a saved window template file |
| `saveAsTemplate(window, "path")` | — | save a single window as a reusable template |
| `clone(window)` | `MdiSubWindow` | duplicate a window into the active folder |
| `setWindowName(window, "name")` | — | rename a window |
| `autoArrangeLayers()` | — | tile all visible windows |
| `setPreferences(graph)` | — | apply current global preferences to `graph` |
| `windows()` | `list[MdiSubWindow]` | all open windows regardless of type |
| `workspace()` | `QMdiArea` | the central MDI area widget |

```python
# save, close, reopen
saveProject()
closeProject()
newProject()

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
| `scriptPrint("text")` | — | append a line to the scripting console (use inside scripts and Notes) |
| `scriptPrintReplace("text")` | — | replace the last console line (useful for progress updates in loops) |
| `displayInfo("text")` | — | append a line to the status bar info log |
| `resultsLog()` | `QTextEdit` | the Results Log text widget (append text with `.append()`) |
| `infoLineEdit()` | `QLineEdit` | the status bar line edit widget |

```python
for i, t in enumerate(tables()):
    scriptPrintReplace(f"Processing {i+1}/{len(tables())}: {t.objectName()}")

resultsLog().append("Fit converged: chi² = 1.23")
displayInfo("Processing complete.")
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
g = plot(t_sub, ("Subtracted_Q", "Subtracted_I"), 1)

# save
saveProjectAs("/data/result.qti")
```
