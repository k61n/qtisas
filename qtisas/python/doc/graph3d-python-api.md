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

> **One `plot3D(…)` call = one surface = one window.** A second `plot3D(…)` call opens a new window — Graph3D has no multi-surface overlay mode.

---

## Formula String Syntax

Formula strings (used in `plot3D("f(x,y)", …)` and `plot3D("X","Y","Z", …)`) are parsed by **muParser**, not Python.

| Rule | Correct | Wrong |
|------|---------|-------|
| Power | `x^2` | `x**2` |
| Conditional | `sign(threshold - v)` | `a ? b : c` — not supported |
| Function call | — | `if(cond, a, b)` — not available |

**Available functions:** `sin` `cos` `tan` `asin` `acos` `atan` `atan2` `exp` `log` `log2` `log10` `sqrt` `abs` `sign` `floor` `ceil` `round`

**Piecewise switching** — `sign(threshold − v)` returns −1, 0, or +1:

```python
z_int  = 0.35                         # computed in Python
r_expr = f"({Ra:.4f}+{Rh:.4f}*sign({z_int:.6f}-v))"
```

**Shared sub-expression pattern** — when X and Y depend on the same term (e.g. a radius `r(v)`), define it as a Python string and interpolate it into both formulas. Do **not** inline the same expression twice with different trig factors — the expressions diverge when you edit one:

```python
r  = "sqrt(4.0 - v^2)"               # sub-expression as Python string
g3 = plot3D(f"({r})*cos(u)", f"({r})*sin(u)", "v",
            0, 2*pi, -2, 2, 60, 60, False, True)
```

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
| `setMatrix(matrix)` | update from `Matrix` object |
| `setFunction("f(x,y)", xl, xr, yl, yr, zl, zr, nx=40, ny=40)` | set/replace function surface |
| `setParametricSurface("X","Y","Z", ul, ur, vl, vr, nu=40, nv=40, uClosed=True, vClosed=True)` | set/replace parametric surface |
| `update()` | force redraw |

> `setMatrix(matrix)` takes a `Matrix` object only — **not** another `Graph3D`. Use `matrix("name")` or `currentMatrix()` to obtain one.

> **`uClosed=True`** closes the seam along the u-axis (azimuthal). **`vClosed=False`** when the surface naturally tapers to zero radius at both ends of v (e.g. poles of a sphere or ends of a surface of revolution) — using `True` there creates degenerate faces.

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
| `setDataColors(minColor, maxColor)` | set a two-color gradient as the active color map |
| `setDataColorMap(fileName)` | load color map from `.map` file |
| `setDataColorMap(colorMap)` | apply a `LinearColorMap` object |
| `colorMap()` → `LinearColorMap` | get the current color map |
| `colorMapFile()` → `str` | path to the current color map file |
| `rescaleColorMap()` | reapply the **system** color map to current data range |

> **`setDataColors` and `rescaleColorMap()` are mutually exclusive.** `rescaleColorMap()` reloads the system color map and discards any `setDataColors()` gradient. Use one or the other, never both.

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
# Elevation parametrisation: u = azimuth [-pi, pi], v = elevation [-pi/2, pi/2].
# vClosed=False: surface tapers to r=0 at poles — no degenerate seam needed.
# curve(0) returns self — useful as a style handle.

g = plot3D("cos(u)*cos(v)", "sin(u)*cos(v)", "sin(v)",
           -pi, pi, -pi/2, pi/2, 40, 40, False, False)
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
```

```python
# --- parametric surface: two connected spheres (surface of revolution) ---
# Graph3D holds ONE surface — one plot3D() call covers the whole shape.
# sign(z_int - v) switches between the two sphere radii as v crosses z_int.
# r is built as a Python string so X and Y share exactly the same sub-expression.
import math
R1, R2   = 2.0, 1.5
sep      = 2.5
cz1, cz2 = -sep / 2, sep / 2
pi       = math.pi
z_int    = (R1**2 - R2**2 - cz1**2 + cz2**2) / (2 * (cz2 - cz1))
zl, zr   = cz1 - R1, cz2 + R2

Ra = (R1 + R2) / 2;  Rh = (R1 - R2) / 2
ca = (cz1 + cz2) / 2;  ch = (cz1 - cz2) / 2
s  = f"sign({z_int:.6f}-v)"
Re = f"({Ra:.4f}+{Rh:.4f}*{s})"
ce = f"({ca:.4f}+{ch:.4f}*{s})"
r  = f"sqrt(({Re})^2-(v-({ce}))^2)"

g3 = plot3D(f"({r})*cos(u)", f"({r})*sin(u)", "v",
            0, 2*pi, zl, zr, 60, 80, False, True)
setWindowName(g3, "ConnectedSpheres")
g3.setScales(-(R1 + 0.2), R1 + 0.2, -(R1 + 0.2), R1 + 0.2, zl - 0.1, zr + 0.1)
g3.setPlotStyle(Qti3D.FILLEDMESH)
g3.findBestLayout()
g3.update()
```
