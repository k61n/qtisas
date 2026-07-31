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

`^` performs a **bitwise XOR** on integers. A standalone `a ^ b` expression also **discards the result** — the variable `a` is left unchanged.

```python
a = 3
b = 4

f = a ^ b      # f = 7   (bitwise XOR: 011 ^ 100 = 111)
a ^ b          # result is 7 but immediately discarded — a is still 3

p = a ** b     # p = 81  (3 to the power 4)
```

> **muParser note:** in the muParser scripting environment (used in table formulas and the formula bar), `^` is the power operator as usual. The Python-vs-muParser difference only matters inside Python scripts and Notes running in Python mode.

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

---

## Matrices

| Call | Returns | Notes |
|------|---------|-------|
| `matrix("name")` | `Matrix` | look up existing matrix by object name |
| `newMatrix()` | `Matrix` | blank matrix (32×32) |
| `newMatrix("name", rows, cols)` | `Matrix` | if a matrix named `"name"` already exists, resize it; otherwise create new |
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
| `newGraph(name="Graph1", layers=1, rows=1, cols=1)` | `MultiLayer` | if a graph named `"name"` already exists, return it; otherwise create blank graph — **do not use with `plot()`**; `plot()` always creates its own window |
| `graphs()` | `list[MultiLayer]` | all graph windows in the project |

---

### Plotting from a Table

| Call | Returns | Notes |
|------|---------|-------|
| `plot(table, (col, …), style=1)` | `MultiLayer` | creates a new graph window and plots columns; **do not call `newGraph()` before this** |
| `plot(table, "colName", style=1)` | `MultiLayer` | creates a new graph window and plots a single column; **do not call `newGraph()` before this** |

`style` is a `Graph.CurveType` integer value: `0` = Line, `1` = Scatter, `2` = LineSymbols, `3` = VerticalBars, `10` = HorizontalBars.

> **`plot()` always creates a new graph window** with an auto-generated name (`"Graph1"`, `"Graph2"`, …) and returns it.
> Do **not** call `newGraph()` first — it is redundant and the window it creates will be left empty.
> `g` is a `GraphWindow` (`MultiLayer`) — it has **no** `replot()` method.
> Use `g.activeLayer().replot()` to force a redraw, or `g.activeLayer().addCurve(...)` to add more curves.

> **When you need a specific graph name** — use `newGraph("name")` + `addCurve()` instead of `plot()`.
> `plot()` cannot assign a custom name; rename after the fact with `setWindowName(g, "name")` or use `newGraph` from the start.

```python
# quick plot — auto-named "Graph1", "Graph2", ...
g = plot(t, ("Q", "I"), style=Graph.CurveType.LineSymbols)

# named graph — use newGraph + addCurve (preferred for templates and scripts)
g = newGraph("SANS_Template")
g.activeLayer().addCurve(t, "I", Graph.CurveType.LineSymbols)
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

### Getting a 3D window

| Call | Returns | Notes |
|------|---------|-------|
| `plot3D("name")` | `Graph3D` | look up existing 3D graph by object name |
| `newPlot3D(name="")` | `Graph3D` | blank 3D window (empty surface) |

---

### Plotting from a Table column

| Call | Returns | Notes |
|------|---------|-------|
| `plot3D(table, "colName", style=0)` | `Graph3D` | XYZ scatter/ribbon from a Z column with X/Y from preceding columns; `style`: `0`=ribbons, `1`=bars, `2`=cones |

---

### Plotting from a Matrix

| Call | Returns | Notes |
|------|---------|-------|
| `plot3D(matrix, style=4)` | `Graph3D` | surface from a matrix; `style`: `1`=wireframe, `2`=hidden-line, `3`=filled (no edges), `4`=filled+mesh, `5`=dots, `6`=bars |

---

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
| `setWindowName(window, "name")` | `bool` | rename a window — raises `ValueError` if the name is empty, has invalid characters, or is already used by another window (see gotcha below) |
| `autoArrangeLayers()` | — | tile all visible windows |
| `setPreferences(graph)` | — | apply current global preferences to a `Graph` layer — takes a `Graph`, **not** a `GraphWindow`; preferences are auto-applied at layer creation so this is only needed to re-apply after manual overrides |
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

> All `qti.app` methods are available as **global functions** — call without `qti.app.` prefix.

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

## Colour Palette

The indexed colour list is used by `setCurveLineColor(curveIndex, colorIndex)` and `setCurveSymbolColor(curveIndex, colorIndex|QColor)`. The list is user-configurable in preferences.

| Call | Returns | Notes |
|------|---------|-------|
| `colorNames()` | `list[str]` | ordered list of colour names (e.g. `["black", "red", ...]`) |
| `colorIndex("name")` | `int` | index of that name; returns `-1` if not found |

> `colorIndex()` returns a palette **index** (`int`). It is **not** a `QColor` and cannot be passed to `QPen`, `QBrush`, or any Qt class expecting a colour object. Use `QColor("red")` or `QColor(r, g, b)` when a `QColor` is required.

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

The indexed symbol list is used by `setCurveSymbolStyle(curveIndex, style)`. User-configurable in preferences.

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

Custom actions are named Python scripts that appear in a QtiSAS menu and can be triggered from the GUI or a script.  
Scripts are stored as `.py` files and registered with a `.qca` descriptor in `customActionsPath()`.

### Workflow

To create a custom action, always follow these three steps in order:

1. **Write the Python script** — save it into `customActionsPath()/scripts/`:
   ```python
   path = customActionsPath()
   with open(path + "/scripts/my_action.py", "w") as f:
       f.write("# your QtiSAS Python code here\n")
   ```

2. **Provide an icon** *(optional)* — copy or create an image file into `customActionsPath()/icons/`.  
   Supported formats: PNG, SVG, etc.

3. **Register the action** — call `addCustomAction` with the bare filenames (not full paths):
   ```python
   addCustomAction("My Action", "my_action.py",
                   menuName="scriptingMenu",
                   iconName="my_action.png",   # omit if no icon
                   shortcut="Ctrl+Shift+M",    # omit if no shortcut
                   tooltip="What this action does")
   ```

> **Important:** `scriptName` and `iconName` are bare filenames only, not full paths.  
> The function automatically prepends `customActionsPath()/scripts/` and `customActionsPath()/icons/`.

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

`sin`, `cos`, `exp`, `log`, `sqrt`, `pi`, `e`, `floor`, `ceil`, `abs` are all global.
Never prefix with `math.` and never write `import math`.

```python
# WRONG
import math
y = math.exp(-x * x)

# RIGHT — exp() is global; no import needed
y = exp(-x * x)
```

### Graph preference getters — always use `qti.app.` prefix

Graph preference getters require the `qti.app.` prefix. Bare names raise `NameError`.

```python
# WRONG — NameError: name 'graphTitleOn' is not defined
titleOn = graphTitleOn()

# RIGHT
titleOn = qti.app.graphTitleOn()
```

### EXPLORE graph defaults before creating a template

Read current defaults first, then only override what differs:

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
