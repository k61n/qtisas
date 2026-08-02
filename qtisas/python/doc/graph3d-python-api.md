# QtiSAS Graph3D Python API

Python scripting interface for 3D surface/scatter plots.

- **`Graph3D`** — a single 3D plot window wrapping Qwt3D
- **`Qti3D`** — module-level namespace with `PLOTSTYLE` constants

> **`QColor`, `QFont`** must be imported explicitly: `from PyQt6.QtGui import QColor, QFont` (macOS/Windows; use `PyQt5` on Linux if Qt5 is installed). Use `QColor("name")` string form for Qt5/Qt6 compatibility.

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
| `setScales(xl, xr, yl, yr, zl, zr, axis=-1)` | set axis ranges; -1 = all three — **display only, does NOT rescale the surface** (see Rule 6) |
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
# sep MUST equal R1+R2 (tangent spacing) — see Rule 4 below for why a smaller value distorts
# both spheres' sizes, sometimes severely.
R1, R2   = 2.0, 1.5
sep      = R1 + R2
cz1, cz2 = -sep / 2, sep / 2
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

---

---

## For AI: Common Mistakes

### Rule 1 — Never `import math`

Math functions (`sin`, `cos`, `exp`, `sqrt`, `pi`, …) are global in QtiSAS — `import math` raises
`ImportError`. Use the bare names directly.

**WRONG:**
```python
import math
pi = math.pi
g3 = plot3D("sin(x)*cos(y)", -pi, pi, -pi, pi, -1.0, 1.0)
```

**RIGHT:**
```python
g3 = plot3D("sin(x)*cos(y)", -pi, pi, -pi, pi, -1.0, 1.0)
```

### Rule 2 — PyQt6 imports must be at the top of the script

`QColor` and `QFont` are not in the global namespace — import before any usage.

**WRONG:**
```python
g3 = newPlot3D()
g3.setMeshColor(QColor("black"))   # NameError: QColor not defined
from PyQt6.QtGui import QColor
```

**RIGHT:**
```python
from PyQt6.QtGui import QColor, QFont
g3 = newPlot3D()
g3.setMeshColor(QColor("black"))
```

Use `QColor("name")` string form for Qt5/Qt6 compatibility.

### Rule 3 — `findBestLayout()` + `update()` required after changes

Appearance changes (axis labels, scale, rotation) are not reflected until `update()` is called.
Call `findBestLayout()` first to recalculate tick spacing, then `update()`.

**WRONG:**
```python
g3.setXAxisLabel("Q (1/nm)")
# missing findBestLayout() + update() — changes may not appear
```

**RIGHT:**
```python
g3.setXAxisLabel("Q (1/nm)")
g3.findBestLayout()
g3.update()
```

### Rule 4 — Graph3D holds exactly ONE surface

Each `plot3D()` call creates a new window or replaces the single surface in an existing one.
You cannot add a second surface to the same `Graph3D`.

**Which approach to use is not free choice — read what the request actually asked for.** "Create
**a** 3d graph/plot with N shapes" (singular container) means the user wants ONE shared view —
attempt the single-surface connected-chain technique below FIRST. Only fall back to separate
windows when the request itself implies separate views (e.g. "create 3 plots", "show each
sphere separately"), or when the connected technique genuinely cannot represent what was asked
(it only works for shapes stacked sequentially along ONE shared axis of revolution — it does
not apply to shapes arranged side by side in a plane, which have no common axis to switch
along). Confirmed failure mode:
asked for "**a** 3d graph with 3 spheres", the AI produced 3 separate `plot3D()` windows —
technically valid Python, but not what was asked, because it defaulted to the easier
separate-windows path without weighing the singular phrasing of the request.

**For genuinely separate, non-touching shapes** (e.g. 3 independent spheres you want visible
at once): use one `plot3D()` call per shape — 3 shapes means 3 windows. There is no overlay
mode; do not call `setParametricSurface()`/`setFunction()` more than once on the same object
expecting it to add a shape — it silently replaces, and you end up with only the last shape.

**For shapes meant to connect/merge into one solid** (e.g. the two-sphere example above): the
only way to combine multiple shapes into ONE surface is a single radius formula that switches
between each shape's own formula via `sign()`, as shown above. That 2-term algebraic form
(`Ra+Rh*sign(...)`) doesn't extend past 2 shapes by adding more terms — for 3 or more, give
each shape a 0/1 indicator weight from `sign()` comparisons against its neighbors so exactly
one shape's formula is active at each point (weights summing to 1), rather than trying to
chain the 2-term form.

**RULE — how much neighboring centers overlap changes each shape's rendered size, silently,
with no error — know which one was actually asked for.** The crossing point between two
neighbors is defined as where their sizes happen to be equal. With **tangent** spacing (center
distance = sum of the two radii/half-sizes) that point is exactly where they touch, so each
shape keeps its true, full requested size — use this whenever the request implies the shapes
should touch but keep their own size (e.g. "connect 3 spheres of these sizes"). With
**overlapping** spacing, the crossing point shifts into the overlap and both neighbors render
*smaller* than requested — appropriate when the request actually wants a fused/blended
transition between shapes, but if that wasn't asked for, treat it as an unintended distortion.
The effect is mild when two neighbors are close in size, but a shape can shrink to a
barely-visible sliver or vanish entirely when it's much smaller than its neighbor(s) (confirmed
with radius ratios 5:1:2: the middle shape's visible zone collapsed to under 2% of the total
range from an arbitrarily-chosen overlap amount). Default to tangent spacing
(`sep = R1 + R2`, derived from the radii themselves) unless the request specifically calls for
shapes to overlap/merge.

**RULE — the switch threshold is the crossing/tangent z-coordinate itself: `c[i] + R[i]`
(equivalently `c[i+1] - R[i+1]` when tangent) — NOT the center-to-center separation
(`R[i]+R[i+1]`) and NOT the next shape's own center `c[i+1]`.** These are three different
numbers that are easy to conflate because `sep = R[i]+R[i+1]` is *also* the correct quantity
for placing the next center (`c[i+1] = c[i] + sep`) — that reuse is fine for computing centers,
but plugging the same `sep` value in again as the switch threshold is wrong, because
`c[i]+sep == c[i+1]`, the CENTER of the next shape, not its bottom edge.

Confirmed failure: for radii 2.5/0.5/1.0 tangent-centered at 0/3.0/4.5, the switch was set to
`3.0` (`sep12`, which equals `c[1]`) instead of the correct `2.5` (`c[0]+R[0]`, sphere 0's true
edge). Consequence: for `v` between the true edge (2.5) and the wrong threshold (3.0), the
*previous* shape's formula stays active past its own valid range (negative under `sqrt`, i.e.
NaN) — a dead gap sits exactly where the next shape's lower half should be, so every shape
after the first renders as only its upper half. (Verified: 75/400 sampled `v` gave NaN with the
wrong threshold, 0/400 with the correct one.) The first shape in the chain looks fine only
because it has no earlier neighbor to be confused with — this bug is invisible on a 2-shape
chain and only shows up from the 3rd shape onward.

```python
# WRONG — reuses sep as the switch point; sep is a distance, not a coordinate
sep12 = R1 + R2
c2 = c1 + sep12
threshold_12 = sep12               # == c2 (sphere 2's CENTER) when c1 == 0 — wrong

# RIGHT — the switch point is where shape 1 actually ends
sep12 = R1 + R2
c2 = c1 + sep12                    # sep reused correctly HERE, to place the center
threshold_12 = c1 + R1              # the true tangent point (== c2 - R2), NOT sep12
```

### Rule 5 — Axis index for numeric format methods

`setAxisNumericFormat(fmt, prec, axis)` and related calls use axis index: X=0, Y=1, Z=2.

### Rule 6 — `setScales()` changes the displayed axis range, NOT the surface's actual size

Computing a size/radius variable and only passing it to `setScales()` (and/or a title string)
does not scale the shape itself — the surface formula still uses whatever numbers are literally
written in it. A hardcoded unit-sphere formula (`"cos(u)*cos(v)"`, ...) stays radius 1 no matter
what `setScales()` is told; only the axis box gets bigger or smaller around that same size-1
shape. The effect is not just "the size is wrong" — it can be the **opposite** of what was
intended: a bigger `setScales()` range around unscaled unit geometry makes the shape look
*smaller* (more empty space around it), so the sphere meant to be the biggest can end up looking
the smallest, and vice versa.

**WRONG** — `r` is computed and used for the axis range and the title, but never reaches the surface:
```python
r = 5.0
g = plot3D("cos(u)*cos(v)", "sin(u)*cos(v)", "sin(v)", -pi, pi, -pi/2, pi/2, 40, 40, False, False)
g.setScales(-r - 0.2, r + 0.2, -r - 0.2, r + 0.2, -r - 0.2, r + 0.2)
g.setTitle(f"Sphere R={r}")
```

**RIGHT** — `r` is baked into the formula itself; `setScales()` still needed to fit the view to it:
```python
r = 5.0
g = plot3D(f"{r}*cos(u)*cos(v)", f"{r}*sin(u)*cos(v)", f"{r}*sin(v)", -pi, pi, -pi/2, pi/2, 40, 40, False, False)
g.setScales(-r - 0.2, r + 0.2, -r - 0.2, r + 0.2, -r - 0.2, r + 0.2)
g.setTitle(f"Sphere R={r}")
```

This applies to any parametric/function surface, not just spheres: whatever varies the shape's
actual size must be a factor inside the `X`/`Y`/`Z` (or `f(x,y)`) formula strings themselves —
`setScales()` only ever follows along to fit the view, it never drives the geometry.
