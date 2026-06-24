# QtiSAS Graph3D Python API

Python scripting interface for 3D surface/scatter plots.

- **`Graph3D`** — a single 3D plot window wrapping Qwt3D
- **`Qti3D`** — module-level namespace with `PLOTSTYLE` constants

> **`QColor`, `QFont`** are available in the default QtiSAS script namespace (imported by `qtisasrc.py`). No extra import is needed unless you are running outside QtiSAS.

> **Axis indices:** X = 0, Y = 1, Z = 2. Used by `setAxisNumericFormat`, `axisNumericFormat`, `axisNumericPrecision`, `setScales(…, axis)`.

---

## Quick Start

```python
g3 = plot3D("sin(x)*cos(y)", -pi, pi, -pi, pi, -1.5, 1.5)

g3.setXAxisLabel("x")
g3.setYAxisLabel("y")
g3.setZAxisLabel("f(x,y)")
g3.setPlotStyle(Qti3D.FILLEDMESH)
g3.setMeshColor(QColor("black"))
g3.setDataColors(QColor("navy"), QColor("red"))
g3.rescaleColorMap()
g3.setRotation(30, 0, 315)
g3.setScale(1.0, 1.0, 0.7)
g3.setTitle("sin(x)·cos(y)")
g3.showLegend(True)
g3.findBestLayout()
g3.update()
```

---

## Getting a Graph3D

| Call | Returns | Notes |
|------|---------|-------|
| `plot3D("name")` | `Graph3D` | look up existing window by name |
| `newPlot3D()` | `Graph3D` | new empty 3D window |
| `newPlot3D("name")` | `Graph3D` | new empty 3D window with name |
| `plot3D(table, "zCol", type=0)` | `Graph3D` | XYZ surface from table column |
| `plot3D(matrix, style=5)` | `Graph3D` | surface from Matrix (default: FILLEDMESH) |
| `plot3D("f(x,y)", xl, xr, yl, yr, zl, zr)` | `Graph3D` | function surface |
| `plot3D("f(x,y)", xl, xr, yl, yr, zl, zr, nx, ny)` | `Graph3D` | function surface with grid density |
| `plot3D("X(u,v)", "Y(u,v)", "Z(u,v)", ul, ur, vl, vr)` | `Graph3D` | parametric surface |
| `plot3D("X(u,v)", "Y(u,v)", "Z(u,v)", ul, ur, vl, vr, nu, nv, uClosed, vClosed)` | `Graph3D` | parametric surface, full options |

> All `plot3D(…)` overloads are factory functions on `app` (the `ApplicationWindow`).

---

## Enumerations

### `Qti3D` — plot style constants

| Constant | Value | Style |
|----------|-------|-------|
| `Qti3D.NOPLOT` | 0 | no plot |
| `Qti3D.WIREFRAME` | 1 | wireframe mesh |
| `Qti3D.HIDDENLINE` | 2 | hidden-line wireframe |
| `Qti3D.FILLED` | 3 | filled polygons |
| `Qti3D.FILLEDMESH` | 4 | filled polygons + mesh |
| `Qti3D.POINTS` | 5 | scatter dots |
| `Qti3D.USER` | 6 | bar plot |

### `Graph3D.PlotType`

| Constant | Value |
|----------|-------|
| `Graph3D.PlotType.Scatter` | 0 |
| `Graph3D.PlotType.Trajectory` | 1 |
| `Graph3D.PlotType.Bars` | 2 |
| `Graph3D.PlotType.Ribbon` | 3 |

### `Graph3D.AxisNumericFormat`

| Constant | Value |
|----------|-------|
| `Graph3D.AxisNumericFormat.Default` | 0 |
| `Graph3D.AxisNumericFormat.Decimal` | 1 |
| `Graph3D.AxisNumericFormat.Scientific` | 2 |
| `Graph3D.AxisNumericFormat.Engineering` | 3 |

---

## View Control

| Method | Notes |
|--------|-------|
| `setRotation(xDeg, yDeg, zDeg)` | rotate the 3D view |
| `setScale(xFactor, yFactor, zFactor)` | scale each axis independently |
| `setShift(xVal, yVal, zVal)` | shift the view centre |
| `setZoom(factor)` | zoom level |
| `setOrthogonal(on=True)` | orthographic projection (False = perspective) |

---

## Data Source

| Method | Notes |
|--------|-------|
| `setData(table, "zCol", type=0)` | update from table column |
| `setMatrix(matrix)` | update from Matrix object |
| `setFunction("f(x,y)", xl, xr, yl, yr, zl, zr, nx=40, ny=40)` | set/replace function surface |
| `setParametricSurface("X","Y","Z", ul, ur, vl, vr, nu=40, nv=40, uClosed=True, vClosed=True)` | set/replace parametric surface |
| `update()` | force redraw |

---

## Axes

| Method | Notes |
|--------|-------|
| `setXAxisLabel(label)` | X axis title |
| `setYAxisLabel(label)` | Y axis title |
| `setZAxisLabel(label)` | Z axis title |
| `setXAxisTickLength(major, minor)` | tick length on X |
| `setYAxisTickLength(major, minor)` | tick length on Y |
| `setZAxisTickLength(major, minor)` | tick length on Z |
| `setScales(xl, xr, yl, yr, zl, zr, axis=-1)` | set axis ranges; -1 = all three |
| `axisNumericFormat(axis)` → `int` | format code for axis (0=X, 1=Y, 2=Z) |
| `axisNumericPrecision(axis)` → `int` | decimal places for axis |
| `setAxisNumericFormat(axis, format, precision)` | format + precision for one axis; `format` accepts `int` or `AxisNumericFormat` |
| `setXAxisNumericFormat(format, precision)` | shorthand for X axis; `format` accepts `int` or `AxisNumericFormat` |
| `setYAxisNumericFormat(format, precision)` | shorthand for Y axis; `format` accepts `int` or `AxisNumericFormat` |
| `setZAxisNumericFormat(format, precision)` | shorthand for Z axis; `format` accepts `int` or `AxisNumericFormat` |

---

## Frame Style

| Method | Notes |
|--------|-------|
| `setFramed()` | framed edges only |
| `setBoxed()` | full bounding box |
| `setNoAxes()` | no axes or box |

---

## Grid Faces

| Method | Notes |
|--------|-------|
| `setLeftGrid(on=True)` | left face grid |
| `setRightGrid(on=True)` | right face grid |
| `setCeilGrid(on=True)` | ceiling face grid |
| `setFloorGrid(on=True)` | floor face grid |
| `setFrontGrid(on=True)` | front face grid |
| `setBackGrid(on=True)` | back face grid |

---

## Floor Projection

| Method | Notes |
|--------|-------|
| `showFloorProjection()` | project data colours onto floor |
| `showFloorIsolines()` | project isolines onto floor |
| `setEmptyFloor()` | clear floor projection |

---

## Plot Style

| Method | Notes |
|--------|-------|
| `setPlotStyle(style)` | set style from `Qti3D` constant |
| `setWireframeStyle()` | `Qti3D.WIREFRAME` |
| `setHiddenLineStyle()` | `Qti3D.HIDDENLINE` |
| `setPolygonStyle()` | `Qti3D.FILLED` |
| `setFilledMeshStyle()` | `Qti3D.FILLEDMESH` |
| `setDotStyle()` | `Qti3D.POINTS` |
| `setBarStyle()` | `Qti3D.USER` |
| `setConeStyle()` | cone glyphs |
| `setCrossStyle()` | cross/star glyphs |

### Style Options

| Method | Notes |
|--------|-------|
| `setConeOptions(rad, quality)` | cone radius and tessellation quality |
| `setCrossOptions(rad, linewidth, smooth, boxed)` | cross glyph parameters |
| `setDotOptions(size, smooth)` | dot size and anti-aliased smoothing |
| `setBarRadius(rad)` | bar thickness |
| `setBarLines(lines=True)` | draw lines on bar faces |
| `setFilledBars(filled=True)` | fill bar faces |
| `setMeshLineWidth(width)` | mesh line width (integer) |

---

## Colors

All color arguments accept `QColor`. Use `QColor("name")` string form for Qt5/Qt6 compatibility (e.g. `QColor("darkGreen")`).

| Method | Notes |
|--------|-------|
| `setMeshColor(QColor)` | mesh line color |
| `setAxesColor(QColor)` | axis line color |
| `setNumbersColor(QColor)` | tick number color |
| `setLabelsColor(QColor)` | axis label color |
| `setBackgroundColor(QColor)` | background color |
| `setGridColor(QColor)` | grid line color |
| `setDataColors(minColor, maxColor)` | two-color gradient for data range |
| `setDataColorMap(fileName)` | load color map from `.map` file |
| `setDataColorMap(colorMap)` | apply a `LinearColorMap` object |
| `colorMap()` → `LinearColorMap` | get the current color map |
| `colorMapFile()` → `str` | path to the current color map file |
| `rescaleColorMap()` | reapply color map to the current data range |

---

## Display

| Method | Notes |
|--------|-------|
| `setOpacity(val)` | transparency, 0.0 (transparent) – 1.0 (opaque) |
| `setResolution(res)` | render resolution (lower = faster) |
| `showLegend(on=True)` | show/hide colour scale legend |
| `setTitle(text)` | plot title |
| `setTitle(text, color, font)` | title with `QColor` and `QFont` |
| `setAntialiasing(on=True)` | smooth edges |
| `setLabelsDistance(d)` | distance between axis ticks and labels |
| `animate(on=True)` | enable auto-rotate animation |
| `findBestLayout()` | auto-fit the viewport to the data |
| `setWindowTitle(title)` | title bar only — does not update project tree (inherited from `QMdiSubWindow`) |
| `resize(w, h)` | resize the window in pixels (inherited from `QMdiSubWindow`) |
| `setWindowName(g3, name)` | full rename: project tree + `plot3D("name")` lookup — call as `setWindowName(g3, "name")` |

---

## Curve Access

| Method | Returns | Notes |
|--------|---------|-------|
| `curve(index)` | `Graph3D` | returns `self` — Graph3D has a single surface; `index` is ignored |

> This method exists for script compatibility. `g3.curve(0)` is equivalent to `g3`.

---

## Export

| Method | Notes |
|--------|-------|
| `export(fileName)` | export to file; format from extension (`.png`, `.pdf`, `.svg`, …) |
| `exportVector(fileName, ...)` | vector export (PDF, SVG, EPS) |
| `exportImage(fileName, quality=100, transparent=False, dpi=72, ...)` | raster export |

---

## Full Example

```python
# --- 3D surface from a function ---
g3 = plot3D("sin(x)*cos(y)", -pi, pi, -pi, pi, -1.5, 1.5, 60, 60)
setWindowName(g3, "SinCos")

# view
g3.setRotation(30, 0, 315)
g3.setScale(1.0, 1.0, 0.7)
g3.setZoom(0.9)
g3.setOrthogonal(False)

# style
g3.setPlotStyle(Qti3D.FILLEDMESH)
g3.setMeshLineWidth(1)

# colors
g3.setMeshColor(QColor("black"))
g3.setDataColors(QColor("navy"), QColor("firebrick"))
g3.rescaleColorMap()
g3.setBackgroundColor(QColor("white"))
g3.setAxesColor(QColor("black"))
g3.setGridColor(QColor("darkGray"))

# axes
g3.setXAxisLabel("x")
g3.setYAxisLabel("y")
g3.setZAxisLabel("f(x,y)")
g3.setXAxisNumericFormat(Graph3D.AxisNumericFormat.Decimal, 1)
g3.setYAxisNumericFormat(Graph3D.AxisNumericFormat.Decimal, 1)
g3.setZAxisNumericFormat(Graph3D.AxisNumericFormat.Decimal, 2)

# grids
g3.setFloorGrid(True)
g3.showFloorProjection()

# title and legend
g3.setTitle("sin(x) · cos(y)")
g3.showLegend(True)

# finalize
g3.findBestLayout()
g3.update()
```

```python
# --- parametric surface: torus ---
R, r = 3.0, 1.0
g3 = plot3D(
    f"({R} + {r}*cos(v))*cos(u)",
    f"({R} + {r}*cos(v))*sin(u)",
    f"{r}*sin(v)",
    0, 2*pi, 0, 2*pi, 60, 60, True, True
)
setWindowName(g3, "Torus")
g3.setPlotStyle(Qti3D.FILLEDMESH)
g3.setDataColors(QColor("steelblue"), QColor("orange"))
g3.rescaleColorMap()
g3.findBestLayout()
g3.update()
```

```python
# --- surface from a Matrix ---
m = currentMatrix()          # active matrix — or: matrix("MyMatrixName")
if m is None:
    raise RuntimeError("No active matrix")
g3 = plot3D(m, Qti3D.FILLED)
setWindowName(g3, "MatrixPlot")
g3.setXAxisLabel("x")
g3.setYAxisLabel("y")
g3.setZAxisLabel("z")
g3.rescaleColorMap()
g3.showLegend(True)
g3.findBestLayout()
g3.update()
```

```python
# --- parametric surface: sphere ---
# pi is in scope (math is globally imported by qtisasrc.py)
# curve(0) returns self — useful as a style handle

g = plot3D("cos(u)*cos(v)", "sin(u)*cos(v)", "sin(v)",
           -pi, pi, -2, 2, 40, 40, False, False)
setWindowName(g, "Sphere")
g.setScales(-1, 1, -1, 1, -1, 1)
g.showFloorProjection()
g.setTitle("Sphere", QColor("blue"), QFont("Arial", 26))
g.setBackGrid(True)
g.setRightGrid(True)
g.setFloorGrid(True)
g.setFramed()
g.resize(550, 500)

c = g.curve(0)                               # c is g — single-surface alias
c.setPlotStyle(Qti3D.FILLEDMESH)
c.setMeshColor(QColor("darkGreen"))
c.setDataColors(QColor("red"), QColor("yellow"))
c.rescaleColorMap()
```
