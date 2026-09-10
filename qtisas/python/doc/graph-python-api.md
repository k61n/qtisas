# QtiSAS Graph / GraphWindow Python API

Python scripting interface for 2D plots.

- **`GraphWindow`** — the multi-layer window (SIP name for `MultiLayer`)
- **`Graph`** — one plot panel inside a `GraphWindow`

All SIP-wrapped methods accept positional arguments; named optional parameters also accept keyword syntax.

---

## Quick Start

```python
# plot two columns from a table
t = table("MyTable")
gw = plot(t, ("x", "y"), Graph.CurveType.Line)
g = gw.activeLayer()
g.setXTitle("q, 1/nm")
g.setYTitle("I(q)")
g.setScale(Graph.Axis.Bottom, 0.01, 1.0, 0, 5, 5, Graph.Scale.Log10)
g.setScale(Graph.Axis.Left,   1e-3, 1e3, 0, 5, 5, Graph.Scale.Log10)
g.replot()
```

```python
# create an empty 2-panel window (1 row, 2 cols) and fill each panel
gw = newGraph("MyPlot", 2, 1, 2)      # 2 panels, 1 row, 2 cols
g1 = gw.layer(1)
g2 = gw.layer(2)
t = table("MyTable")
g1.addCurve(t, "I",  Graph.CurveType.Line)
g2.addCurve(t, "dI", Graph.CurveType.Scatter)
```

---

## Getting a GraphWindow

| Call | Returns | Notes |
|------|---------|-------|
| `graphWindow("name")` | `GraphWindow` or `None` | look up existing window by name; returns `None` if no graph with that name exists |
| `currentGraph()` | `GraphWindow` | currently active graph window |
| `newGraph()` | `GraphWindow` | empty 1-panel window named `"Graph1"` |
| `newGraph("name")` | `GraphWindow` | empty 1-panel window |
| `newGraph("name", layers)` | `GraphWindow` | multi-panel window, auto-arranged |
| `newGraph("name", layers, rows, cols)` | `GraphWindow` | multi-panel in explicit `rows × cols` grid |
| `plot(table, cols_tuple, style)` | `GraphWindow` | new window from table columns |
| `plot(table, "col", style)` | `GraphWindow` | new window from single column |
| `plot(matrix)` | `GraphWindow` | colour-map spectrogram from matrix |
| `plot("formula", xMin, xMax)` | `GraphWindow` | function plot — `"formula"` is a MATH EXPRESSION (e.g. `"x^2"`), evaluated over `[xMin, xMax]`; NOT a dataset/table name lookup |
| `waterfallPlot(table, cols_tuple)` | `GraphWindow` | waterfall plot |

> **`newGraph("name", layers, ...)` called again against an existing window of that name now
> clears every layer's curves first**, regardless of whether the row/col grid also needs
> rearranging. `newGraph()` is the build-from-scratch entry point (`graphWindow("name")` is the
> lookup-to-extend one) — re-running a script that calls `newGraph("Fit", 2, 2, 1)` a second time
> used to leave existing curves in place while new `insertCurve()`/`addCurve()` calls piled more on
> top, silently doubling every curve with no error. If you actually want to add to an existing
> graph rather than rebuild it, use `graphWindow("name")` instead.

> **Confirmed live: no `plot()` overload looks up an existing dataset/table by name.**
> `plot("simulatedCurve-ai/PorodPowerLaw", 0.001, 0.5, Graph.CurveType.Line)` matches the
> `plot("formula", xMin, xMax)` overload above and silently adds a new function curve literally
> labeled `F1(x)="simulatedCurve-ai/PorodPowerLaw"` — not the real data, no exception raised.
> To plot an existing table's columns (e.g. a `fitCurve-<function>`/`simulatedCurve-<function>`
> table — see fittable-python-api.md), look it up with `table(name)` and add it with
> `layer.insertCurve(t, "xCol", "yCol", style)` instead:
> ```python
> sc = table("simulatedCurve-ai/PorodPowerLaw")
> gw = newGraph("PorodSim")
> gw.activeLayer().insertCurve(sc, "x", "y", Graph.CurveType.Line)
> ```

> **Warning:** `graphWindow("name")` returns `None` (confirmed) — not an error, not a placeholder — when no graph with that name exists. Always check `if g is None:` before using the result.

> **`graphWindow("name")` is 2D-`GraphWindow`-ONLY — returns `None` for a `Graph3D` window even
> when one exists under that exact name** (confirmed live). To look up an existing 3D surface
> window, use `plot3D("name")` instead (see graph3d-python-api.md) — the single-argument form is a
> LOOKUP, not a creation. Confirmed live: after `setWindowName(g3, "Sincos3D")` created a real
> `Graph3D` window, a later part's `graphWindow("Sincos3D")` returned `None`, misread as "window
> doesn't exist" rather than "wrong lookup function" — burned 8+ retries chasing dead ends
> (`currentGraph3D()`, `graphNames()`, `graphs()`, a from-scratch numpy/matrix rebuild) before
> giving up on reuse entirely.

---

## GraphWindow (MultiLayer)

### Enumerations

```python
GraphWindow.HorAlignement   # HCenter, Left, Right
GraphWindow.VertAlignement  # VCenter, Top, Bottom
GraphWindow.AlignPolicy     # AlignLayers=0, AlignCanvases
```

### Layer access

| Method | Returns | Notes |
|--------|---------|-------|
| `numLayers()` | `int` | total number of layers |
| `activeLayer()` | `Graph` | currently selected panel |
| `setActiveLayer(g)` | — | make `g` the active layer |
| `layer(n)` | `Graph` | **1-based** index — `layer(1)` is the first |
| `layersList()` | `list[Graph]` | all layers |
| `addLayer(x, y, w, h, setPreferences=True)` | `Graph` | add layer at position; all args optional — 5th arg applies the app's default fonts/colors/tick style, NOT axis linking; use `linkXLayerAxes()` for that |
| `removeLayer(g)` | `bool` | remove a specific layer |
| `removeActiveLayer()` | `bool` | remove the currently active layer |
| `setNumLayers(n)` | — | change total number of layers |

### Layout

| Method | Notes |
|--------|-------|
| `setCols(n)` | set number of columns — call `arrangeLayers()` afterward |
| `setRows(n)` | set number of rows — call `arrangeLayers()` afterward |
| `setSpacing(rowGap, colGap)` | gap in pixels between layers or canvases |
| `setMargins(left, right, top, bottom)` | outer margins in pixels |
| `setLayerCanvasSize(w, h)` | canvas size for every layer uniformly |
| `setAlignement(hAlign, vAlign)` | `GraphWindow.HorAlignement.*` / `GraphWindow.VertAlignement.*` |
| `arrangeLayers(fit=True, userSize=False)` | reflow layers |
| `swapLayers(src, dest)` | swap two layers by 1-based index |
| `setEqualSizedLayers()` | make all layers the same size |
| `setScaleLayersOnResize(on=True)` | scale layer geometry on window resize |
| `setAlignPolicy(policy)` | `AlignLayers` (default) or `AlignCanvases` |
| `keepAligned(on=True)` | snapshot canvas rects and re-apply on every update |
| `isKeepAligned()` | `bool` |

### Stacked panels — zero canvas gap

Recipe for data+fit / residuals layouts. **All 7 steps are required, in this exact order — do not skip any, do not add `activateWindow` at the end.**

Adding curves and titles changes each layer's Y-axis tick-label width independently (autoscale/margin recalculation), shifting `l1`/`l2` canvas x/width out of sync — **even after `setCanvasGeometry` already matched them once.** Fix: add curves/titles first, re-match geometry a **second time** afterward, and only lock alignment with `keepAligned(True)` once nothing will disturb it again.

| Step | Code | If wrong |
|------|------|---------|
| 0 | `maximizeWindow(gw)` before `arrangeLayers`/`canvasGeometry` | canvas sizes wrong — geometry computed before resize |
| 1 | `l1.enableAxis(Bottom, False)` + `l2.enableAxis(Top, False)` | seam axis tick labels overlap |
| 2 | `gw.keepAligned(False)` | stale alignment may override step 3 |
| 3 | `setAlignPolicy(AlignCanvases)` + `setSpacing(0,0)` + `arrangeLayers(False,False)` | gap between layers not eliminated |
| 4 | `setCanvasGeometry` on both layers (all args `int`) — first pass | heights are 50/50 instead of asymmetric |
| 5 | `l2.removeTitle()` + `addCurve()`/`insertCurve()` + `setXTitle`/`setYTitle` on both layers | — |
| 6 | **Re-apply `setCanvasGeometry` a second time**, same `canvas_x`/`canvas_w`/heights as step 4, THEN `gw.keepAligned(True)` + `gw.linkXLayerAxes(True)` | skipping the second `setCanvasGeometry` leaves `l1`/`l2` canvas x/width mismatched — curves/titles in step 5 shifted them independently and the first pass no longer holds |
| END | **No `activateWindow(gw)` call** | window reverts from maximized state |

```python
gw = newGraph("Fit", 2, 2, 1)  # 2 layers, 2 rows, 1 col — REQUIRED for vertical stacking
# step 0 — maximize FIRST, before arrangeLayers/canvasGeometry
maximizeWindow(gw)
l1 = gw.layer(1)
l2 = gw.layer(2)
# step 1 — disable seam axes
l1.enableAxis(Graph.Axis.Bottom, False)
l2.enableAxis(Graph.Axis.Top,    False)
# step 2 — unlock alignment (required even on fresh graph)
gw.keepAligned(False)
# step 3 — align canvases with zero gap
gw.setAlignPolicy(GraphWindow.AlignPolicy.AlignCanvases)
gw.setSpacing(0, 0)
gw.arrangeLayers(False, False)
# step 4 — set asymmetric heights; MUST read from BOTH layers and sum
cr1 = l1.canvasGeometry()
cr2 = l2.canvasGeometry()
canvas_x = cr1.x()
canvas_y = min(cr1.y(), cr2.y())
canvas_w = cr1.width()
total_h  = cr1.height() + cr2.height()
l1_h = int(total_h * 0.75)
l2_h = total_h - l1_h
l1.setCanvasGeometry(canvas_x, canvas_y,         canvas_w, l1_h)
l2.setCanvasGeometry(canvas_x, canvas_y + l1_h,  canvas_w, l2_h)
# step 5 — add curves and set titles; this can shift l1/l2 canvas rects independently
l2.removeTitle()
l1.addCurve(t, "I", Graph.CurveType.LineSymbols)
l2.addCurve(t, "residuals", Graph.CurveType.LineSymbols)
# X title ONLY on l2 — l1's bottom axis is disabled, its X title is never rendered
l2.setXTitle("Q (1/nm)")
l1.setYTitle("I(Q)")
l2.setYTitle("Residuals")
# step 6 — re-apply the SAME geometry to correct any drift from step 5, THEN lock
l1.setCanvasGeometry(canvas_x, canvas_y,         canvas_w, l1_h)
l2.setCanvasGeometry(canvas_x, canvas_y + l1_h,  canvas_w, l2_h)
gw.keepAligned(True)
gw.linkXLayerAxes(True)
# DO NOT call activateWindow(gw) — it undoes the maximize
l1.replot()
l2.replot()
```

### Adding a residuals panel to an EXISTING single-layer graph

The recipe above assumes a BRAND-NEW graph via `newGraph("Fit", 2, 2, 1)`, which sets row/column
layout at construction. An equally common case — plotting data+fit first, then adding a residuals
panel in a LATER, separate step — starts from a graph that already exists with exactly 1 layer,
needing one extra call the recipe doesn't show:

**`setNumLayers(n)` does NOT update the window's row/column layout — call `setRows()`/`setCols()`
yourself, before `arrangeLayers()`.** A 1-layer graph has its row/column count fixed at `(1, 1)`;
`setNumLayers(2)` only adds the second `Graph` object, never touching that count.
`arrangeLayers(fit=False, ...)` (step 3 above) then lays out using the OLD `(1, 1)` count — the
second layer's row index clamps onto row 0, landing on top of the first layer, hidden behind it.
**No error, nothing printed wrong — the script "succeeds," the residuals layer is just invisible.**
`arrangeLayers(fit=True, ...)` doesn't fix it either: for 2 layers its auto-layout picks 1×2
(side-by-side), not a vertical stack — `setRows`/`setCols` must be called explicitly regardless of
`fit`.

```python
# WRONG — silently invisible second layer, no error
gw = graphWindow("FitResults")   # already exists with 1 layer from an earlier step
gw.setNumLayers(2)
gw.arrangeLayers(False, False)   # still uses the OLD (rows=1, cols=1) — layer 2 collapses onto layer 1

# RIGHT — explicitly set the row/column layout BEFORE arrangeLayers
gw = graphWindow("FitResults")
gw.setNumLayers(2)
gw.setRows(2)
gw.setCols(1)
l1 = gw.layer(1)
l2 = gw.layer(2)
l1.enableAxis(Graph.Axis.Bottom, False)
l2.enableAxis(Graph.Axis.Top, False)
gw.keepAligned(False)
gw.setAlignPolicy(GraphWindow.AlignPolicy.AlignCanvases)
gw.setSpacing(0, 0)
gw.arrangeLayers(False, False)   # now correctly splits into 2 rows
# ... continue with steps 4-6 of the recipe above (asymmetric heights, curves/titles, re-apply, lock) ...
```

### Axis linking

| Method | Notes |
|--------|-------|
| `setCommonLayerAxes(vertical=True, horizontal=True)` | enforce identical scale range across all layers |
| `linkXLayerAxes(on=True)` | link X-axes for zoom/pan sync; `False` to unlink |

### Waterfall

| Method | Notes |
|--------|-------|
| `reverseWaterfallOrder()` | reverse the stacking order |

### Export

| Method | Notes |
|--------|-------|
| `export("file.png")` | export in format deduced from extension |
| `exportImage(file, quality=100, transparent=False, dpi=72, customSize=QSizeF(), unit=FrameWidget.Unit.Pixel, fontsFactor=1.0, compression=0)` | raster export |
| `exportVector(file, fontEmbedding, dpi=72, color=True, customSize=QSizeF(), unit=FrameWidget.Unit.Pixel, fontsFactor=1.0)` | vector (PDF/SVG/EPS) export — `fontEmbedding` is required (no default) |
| `exportTex(file, color=True, escapeStrings=True, fontSizes=True, customSize=QSizeF(), unit=FrameWidget.Unit.Pixel, fontsFactor=1.0)` | LaTeX/TikZ export |

---

## Graph (single panel)

### Enumerations

**Axis** (`Graph.Axis`)

```python
Graph.Axis.Left=0  Graph.Axis.Right=1  Graph.Axis.Bottom=2  Graph.Axis.Top=3
Graph.Axis.X=Bottom  Graph.Axis.Y=Left   # convenience aliases for the common case
```

`X`/`Y` are aliases for the typical primary axes (`X`→`Bottom`, `Y`→`Left`) — added because scripts
kept guessing `Graph.Axis.X`/`.Y` by analogy with `Table.PlotDesignation.X`/`.Y` (a *different* enum,
on `Table`, for a column's data role). If a layer's primary axes are swapped (e.g. horizontal
error-bar plots using `Left`/`Right` as data axes), use the real `Bottom`/`Left`/`Top`/`Right` names
explicitly instead of `X`/`Y`.

**Scale** (`Graph.Scale`) — 7th arg of `setScale`

```python
Graph.Scale.Linear=0  Log10=1  Ln=2  Log2=3  Reciprocal=4  Probability=5  Logit=6
```

**Tick direction** (`Graph.Ticks`)

```python
Graph.Ticks.NoTicks=0  Out=1  InOut=2  In=3
```

**Curve types** (`Graph.CurveType`)

```python
Graph.CurveType.Line=0  Scatter=1  LineSymbols=2  Spline=7
VerticalBars=3  HorizontalBars=10  Area=4  VerticalDropLines=6
HorizontalSteps=8  VerticalSteps=15  StackBar=21  StackColumn=22
Histogram=9  Box=13  Pie=5
VectXYXY=11  VectXYAM=14  ErrorBars=12
ColorMap=16  GrayScale=17  Contour=18  ImagePlot=20  Function=19
```

### Curves — add / remove

| Method | Returns | Notes |
|--------|---------|-------|
| `addCurve(t, col, style=0, lineWidth=1, pointSize=3, startRow=0, endRow=-1)` | `bool` | add one Y column; `col` is bare Y column name — **no xcol arg**; X is the column with `PlotDesignation.X` role |
| `addCurves(t, cols_tuple, style=0, lineWidth=1, pointSize=3, startRow=0, endRow=-1)` | `bool` | add multiple Y columns |
| `insertCurve(t, col, style=1, startRow=0, endRow=-1)` | `DataCurve` | note the default `style` differs from `addCurve`'s (`0`) |
| `insertCurve(t, xcol, ycol, style=1, startRow=0, endRow=-1)` | `DataCurve` | explicit X and Y columns by name |
| `removeCurve(index)` | — | remove by 0-based index |
| `removeCurve("title")` | — | remove by curve title |
| `removeCurve(curve)` | — | remove a `QwtPlotCurve` object |
| `deleteFitCurves()` | — | remove all fit result curves |

### Curves — access

| Method | Returns | Notes |
|--------|---------|-------|
| `numCurves()` | `int` | total curve count |
| `curve(index)` | `PlotCurve` | 0-based index |
| `curve("title")` | `PlotCurve` | by title string |
| `curveTitle(index)` | `str` | title at index |
| `dataCurve(index)` | `DataCurve` | `None` if not a data curve |
| `boxCurve(index)` | `BoxCurve` | |
| `functionCurve(index)` | `FunctionCurve` | |

### Curves — reorder / style

**There is no `Graph.setCurveColor()` / `setCurvePenColor()` / `setCurvePen()`.** The real method is
`setCurveLineColor(index, color)` below. `setCurveColor()` sounds plausible because `Fittable` DOES
have a same-named method — a different class, different purpose (styles the fit curve *before*
`fit()`/`plotResults()` creates it; see fittable-python-api.md). On a `Graph` layer, after curves
already exist, use `setCurveLineColor(index, ...)`. Confirmed live, repeatedly, across separate
conversations: `setCurveColor`, `setCurvePenColor`, `setCurvePen` all raised `AttributeError` before
landing on the real name. (`g.curve(index).setPen(QPen(...))` also works — `PlotCurve` inherits
Qwt's own `setPen()` — but prefer `setCurveLineColor()` over reaching into the raw Qwt object.)

| Method | Notes |
|--------|-------|
| `changeCurveIndex(fromIdx, toIdx)` | change draw order |
| `reverseCurveOrder()` | reverse all curves |
| `setCurveStyle(index, Graph.CurveType)` | change curve type |
| `setCurveLineColor(index, colorIndex\|QColor)` | line colour |
| `setCurveLineStyle(index, Qt.PenStyle)` | e.g. `Qt.PenStyle.DashLine` |
| `setCurveLineWidth(index, width)` | line width in points |
| `setCurveAxes(index, xAxis, yAxis)` | attach to axis pair; 0=Bottom/Left, 1=Top/Right |
| `showMissingDataGap(on=True, update=True)` | show gaps for `NaN` rows |
| `isMissingDataGapEnabled()` | `bool` |
| `setGrayScale()` | apply greyscale palette to all curves |
| `setIndexedColors()` | apply indexed colour palette |

### colorIndex() vs QColor() — same name, different colour

> **Warning:** `colorIndex("name")` and `QColor("name")` for the **same name are NOT the same colour.** `colorIndex()` looks up QtiSAS's own indexed palette (`"green"` = `Qt::green`, bright RGB(0,255,0)). `QColor("name")` uses Qt's SVG/X11 named-colour table, where `"green"` is RGB(0,128,0) — a dark, muted shade matching the palette's own `"olive"` entry, not its `"green"`. Confirmed: `setCurveLineColor(0, QColor("green"))` renders olive-looking, not the bright green most expect.
> ```python
> # WRONG — expecting bright green; QColor("green") is actually dark/olive-looking
> l.setCurveLineColor(0, QColor("green"))
>
> # RIGHT — colorIndex uses QtiSAS's palette, matches the expected bright "green"
> l.setCurveLineColor(0, colorIndex("green"))
> ```
> Prefer `colorIndex()` for standard colour names on `setCurveLineColor()`/`setCurveSymbolColor()`/
> `setCurveSymbolBorderColor()` — the only three methods that accept it: no palette-mismatch
> surprises like `"green"` above, and **no import needed**, unlike `QColor` (`from PyQt6.QtGui
> import QColor`). Reach for `QColor(...)` only for a specific RGB value, a colour outside the
> indexed palette, or anything OTHER than these three methods — every other colour parameter in
> this API (`addErrorBars`, `setAxisColor`, `setBackgroundColor`, `setFrameColor`, Graph3D's colour
> setters, ...) takes `QColor` only, with no `colorIndex()` alternative.

### Bare hex int as a QColor substitute

> **For a specific RGB value, a bare `0xRRGGBB` int works anywhere a `QColor` parameter is expected — no `PyQt6` import needed.** Confirmed live: `leg.setBackgroundColor(0xFFFFFF)` sets a real white background, same as `QColor(255, 255, 255)`. Simplest fix for a `NameError: name 'QColor' is not defined` — add the int literal, don't fall back to `Qt.white` (also unimported) or abandon the color entirely.
> ```python
> leg.setBackgroundColor(0xFFFFFF)      # white — no import needed
> g.setAxisColor(Graph.Axis.Left, 0x336699)   # a specific blue — no import needed
> ```
>
### Exception — bare int breaks setCurveLineColor/SymbolColor/SymbolBorderColor

> **EXCEPTION — does NOT work for `setCurveLineColor()`/`setCurveSymbolColor()`/
> `setCurveSymbolBorderColor()`.** Each has BOTH an `(index, int)` overload, where the int is a
> small PALETTE INDEX (0=black, 1=red, ...), and an `(index, QColor)` overload. A bare Python int
> binds to `(index, int)`, not `QColor` — so `g.setCurveLineColor(0, 0x336699)` is interpreted as
> colour**index** 3,375,257, wildly out of range. Confirmed in source
> (`Graph::setCurveLineColor(int,int)`): an out-of-range index silently returns without changing
> anything — no error, no warning. A small hex value (e.g. `0x000003`) is worse — it silently
> selects whatever unrelated colour sits at that small palette index instead of failing at all.
> ```python
> # WRONG for these 3 methods — silently no-ops (or picks an unrelated colour), never errors
> g.setCurveLineColor(0, 0x336699)
>
> # RIGHT — colorIndex() for a palette name, or QColor for a specific RGB value
> g.setCurveLineColor(0, colorIndex("blue"))
> g.setCurveLineColor(0, QColor(0x33, 0x66, 0x99))
> ```

### Waterfall

| Method | Notes |
|--------|-------|
| `setWaterfallOffset(x, y, update=True)` | offset between successive curves |
| `setWaterfallSideLines(on=True)` | draw closing side lines |
| `setWaterfallFillColor(QColor)` | fill colour below curves |

### Special plot types

| Method | Returns | Notes |
|--------|---------|-------|
| `plotBox(t, names_list, startRow=0, endRow=-1)` | — | box-and-whisker |
| `plotPie(t, col, startRow=0, endRow=-1)` | `PieCurve` | pie chart |
| `isPiePlot()` | `bool` | |
| `plotSpectrogram(matrix, style)` | `Spectrogram` | colour map / contour |
| `spectrogram(matrix)` | `Spectrogram` | look up existing spectrogram |
| `addHistogram(matrix)` | `Histogram` | histogram from matrix |
| `plotVectors(t, cols_list, style, startRow=0, endRow=-1)` | `VectorCurve` | |

#### `Histogram` object

| Method | Returns | Notes |
|--------|---------|-------|
| `setAutoBinning(autoBin=True)` | — | |
| `autoBinning()` | `bool` | |
| `setBinning(size, begin, end)` | — | disables auto |
| `begin()` / `end()` | `float` | |
| `binSize()` / `ratio()` | `float` | |
| `setRatio(r)` | — | `1.0`=linear, `>1.0`=geometric |
| `setSkipEmptyBins(on)` | — | manual mode only |
| `loadData()` | — | recompute after parameter change |
| `dataSize()` | `int` | number of bins |
| `sample(i)` | `QPointF` | per-bin access — **`x(i)`/`y(i)` do NOT exist**; use `hist.sample(i).x()` / `.y()` for bin centre / count |
| `mean()` / `standardDeviation()` / `minimum()` / `maximum()` | `float` | |

### Function curves

| Method | Returns | Notes |
|--------|---------|-------|
| `addFunction(formula, xMin, xMax, points)` | `FunctionCurve` | Cartesian |
| `addParametricFunction(xExpr, yExpr, tMin, tMax, points, var)` | `FunctionCurve` | |
| `addPolarFunction(rExpr, thetaExpr, tMin, tMax, points, var)` | `FunctionCurve` | |

### Curve symbols

| Method | Notes |
|--------|-------|
| `setCurveSymbolColor(index, colorIndex\|QColor)` | fill colour |
| `setCurveSymbolBorderColor(index, colorIndex\|QColor)` | border colour |
| `setCurveSymbolBorderWidth(index, float)` | border width |
| `setCurveSymbolOpacity(index, int)` | 0=transparent, 255=opaque |
| `setCurveSymbolStyle(index, int)` | use `symbolIndex("name")` for lookup |
| `setCurveSymbolSize(index, int)` | pixel width = 2×size+1 |

**Prefer `colorIndex("name")` over `QColor("name")` for both colour methods above** — no import
needed (unlike `QColor`, which needs `from PyQt6.QtGui import QColor`) and avoids the palette
mismatch some names have between QtiSAS's colour list and Qt's SVG names (see the `"green"` warning
under Curve — reorder/style above). Reach for `QColor(...)` only for a specific RGB value or a
colour outside the indexed palette.

**All of these are called on the LAYER (with a curve `index`), never on a curve object itself** —
there is no `.setSymbolColor()`/`.setSymbolStyle()` on the `PlotCurve` returned by `g.curve(index)`.
Confirmed live: `g.curve(0).setSymbolColor(QColor(0, 0, 0))` raised `AttributeError: 'PlotCurve'
object has no attribute 'setSymbolColor'` — the call belongs on `g` itself:
`g.setCurveSymbolColor(0, ...)`.

```python
g = currentGraph().activeLayer()
g.setCurveSymbolColor(0, colorIndex("red"))
g.setCurveSymbolBorderColor(0, colorIndex("black"))
g.setCurveSymbolStyle(0, symbolIndex("diamond"))
g.setCurveSymbolSize(0, 5)
```

### Error bars

| Method | Returns | Notes |
|--------|---------|-------|
| `addErrorBars("curveName", t, col, type=1, w=1, cap=8, color=QColor(black), through=True, minus=True, plus=True)` | `ErrorBarsCurve` | |
| `addErrorBars(dataCurve, t, col, ...)` | `ErrorBarsCurve` | attach to `DataCurve` object |
| `updateErrorBars("curveName", t, col, type=1, w=1, cap=8, color=QColor(black), through=True, minus=True, plus=True)` | `bool` | restyle EXISTING error bars |

- `type`: `0`=X bars, `1`=Y bars; `color`: **`QColor`** not `colorIndex()`
- `through=False` for classic style; `minus=True, plus=True` to show bars

```python
t = table("SANS_Data")
layer.addErrorBars("SANS_Data_I", t, "dI", 1, 1, 8, QColor("gray"), False, True, True)
```

**To restyle an EXISTING error bar (color, width, cap, through/minus/plus), use `updateErrorBars()`
— never `addErrorBars()` again.** `addErrorBars()` always creates a brand new `ErrorBarsCurve` with
no "already exists" check — a second call for the same curve/column adds a second, overlapping set
rather than restyling the first. `updateErrorBars()` takes the same argument shape — including
`type` — and finds the existing error bars instead of creating a new curve; returns `False` if none
match, instead of creating anything.

```python
# WRONG — adds a SECOND, overlapping ErrorBarsCurve; the original gray one is still there too
layer.addErrorBars("SANS_Data_I", t, "dI", 1, 1, 8, QColor("red"), False, True, True)

# CORRECT — restyles the existing error bars in place, nothing duplicated
layer.updateErrorBars("SANS_Data_I", t, "dI", 1, 1, 8, QColor("red"), False, True, True)
```

Confirmed live.

### Markers — arrows and images

| Method | Returns | Notes |
|--------|---------|-------|
| `addArrow(arrowMarker)` | `ArrowMarker` | |
| `remove(arrowMarker)` | — | |
| `arrowsList()` | `list[ArrowMarker]` | |
| `numArrows()` | `int` | |
| `addImage("filename")` | `ImageWidget` | load from file |
| `addImage(QImage)` | `ImageWidget` | |
| `addImage(existingImageWidget)` | `ImageWidget` | **CLONES** the passed-in widget into a brand-new `ImageWidget` (copies its coordinates) and returns the clone — same trap as `addText()` below: the original is left untouched, so restyle the RETURN VALUE, not the argument |
| `remove(imageWidget)` | — | |

### Text and legend

| Method | Returns | Notes |
|--------|---------|-------|
| `newLegend()` | `LegendWidget` | add default legend |
| `newLegend("text")` | `LegendWidget` | add legend with custom text |
| `setLegend("text")` | — | replace legend text |
| `legend()` | `LegendWidget` or `None` | get the EXISTING legend — `None` if none was ever added |
| `removeLegend()` | — | |
| `addText(legendWidget)` | `LegendWidget` | add text box |
| `addTimeStamp()` | `LegendWidget` | add date/time stamp |

**To add a legend to a fresh graph, call `newLegend()` — never `legend()`.** `legend()` is a pure
getter: on a layer with no legend yet it returns `None`, and `.setText(...)`/`.setFrameStyle(...)`
on that `None` raises `AttributeError: 'NoneType' object has no attribute 'setText'`. There is no
`addLegend()` method (a plausible but nonexistent guess). Confirmed live: a script called
`g.legend()`, hit the `None` `AttributeError`, guessed the nonexistent `g.addLegend()` next, then
tried `g.legend()` again — three wrong guesses in a row, missing the real answer, `g.newLegend()`.
```python
# WRONG — legend() is a getter; returns None if no legend exists yet
leg = g.legend()
leg.setText("R = 12.3 nm")   # AttributeError: 'NoneType' object has no attribute 'setText'

# RIGHT — newLegend() creates one
leg = g.newLegend()
leg.setText("R = 12.3 nm")
```

### newLegend() has no duplicate check — always reuse an existing legend

**`newLegend()` has no duplicate check — it unconditionally creates and adds another `LegendWidget`
every call**, even if the layer already has one. Calling it more than once on the same graph (e.g. a
retried script, or a re-run on an existing plot) silently stacks up overlapping legends with no
error. Always check `legend()` first and reuse what's there:
```python
leg = g.legend()
if leg is None:
    leg = g.newLegend()
leg.setText("R = 12.3 nm")   # updates the existing legend if there was one, creates it otherwise
```

### addText() clones instead of modifying

**`addText(existingLegendWidget)` does NOT modify or style the widget you pass in — it CLONES it
into a brand-new `LegendWidget`, leaves the original untouched, and returns the clone.**
`.setFrameStyle(...)`/`.setText(...)`/etc. on the return value styles only the clone, overlapping
the original, unstyled widget — two legend-like widgets end up where one was intended, silently. To
style an *existing* legend, call the setter directly on the widget — never route through `addText()`:
```python
# WRONG — leg keeps no frame style at all; fw is a SEPARATE clone that overlaps it
leg = g.newLegend("R = 12.3 nm")
fw = g.addText(leg)
fw.setFrameStyle(Frame.FrameStyle.Shadow.value)   # styles the clone, not leg

# RIGHT — style the legend itself directly
leg = g.newLegend("R = 12.3 nm")
leg.setFrameStyle(Frame.FrameStyle.Shadow.value)
```
`addText()` adds an independent, additional text box (optionally seeded from an existing widget's
appearance) — it is not a way to reach back into an already-added legend.

| Method | Notes |
|--------|-------|
| `setText("text")` | supports `\n` |
| `setTextColor(QColor)` | use `QColor`, not `colorIndex()` |
| `setFont(QFont)` | |
| `setAngle(degrees)` | rotation |
| `setAutoUpdate(bool)` | auto-refresh from curve titles |

### FrameWidget base methods (Legend, Image, Rectangle, Ellipse)

**`FrameWidget` base methods** (available on `LegendWidget`, `ImageWidget`, `RectangleWidget`,
`EllipseWidget`):

| Method | Notes |
|--------|-------|
| `setFrameStyle(int)` | `Frame.FrameStyle.Line.value` (1) or `Frame.FrameStyle.Shadow.value` (2) — **NOT** PyQt6's `QFrame` |
| `setFrameLineStyle(Qt.PenStyle)` | line PATTERN only (dashed/dotted/…) — separate from `setFramePen`, which also sets colour/width |
| `setFramePen(QPen)` | |
| `setFrameColor(QColor)` | |
| `setFrameWidth(float)` | |
| `setBackgroundColor(QColor)` | fill colour inside the frame |
| `setBrush(QBrush)` | |
| `setOrigin(x, y)` | position in pixels (int, int) |
| `setOriginCoord(x, y)` | position in axis data coordinates |
| `setSize(w, h)` | size in pixels (int, int) |

`FrameWidget.Unit` enum (`Inch`/`Millimeter`/`Centimeter`/`Point`/`Pixel`/`Scale`) is used as the
default `unit` argument to `exportImage`/`exportVector`/`exportTex` below.

### Rectangle and Ellipse annotations

`Rectangle`/`Ellipse` (`RectangleWidget`/`EllipseWidget` in C++) are real, constructible
`FrameWidget` subclasses — plain shape annotations, not text/legend/image boxes. Constructing one
**already attaches it to the layer's canvas** — no separate "add" call needed to draw it:

```python
r = Rectangle(g)                          # already attached and visible on layer g
r.setFrameStyle(Frame.FrameStyle.Shadow.value)
r.setSize(200, 60)
r.setOriginCoord(0.8, 0.8)

e = Ellipse(g)                            # same pattern for an ellipse
```

**`Graph.add(frameWidget, copy=True) -> FrameWidget` — the default CLONES, same trap as
`addText()` above.** With `copy=True` (the default), `add()` builds a brand-new widget, copies the
passed-in widget's appearance into it, and returns that — the original stays untouched, so styling
the RETURN VALUE (not the original) is what takes effect. Pass `copy=False` to attach/move the SAME
object you already have (e.g. one just constructed above) without cloning — the one case where you
want it:
```python
r = Rectangle(g)          # already attached — do NOT call g.add(r) again for this
r.setFrameStyle(...)      # style the object directly

# add(copy=False) is for re-attaching/moving an existing, already-built widget object elsewhere —
# NOT needed for a freshly-constructed Rectangle/Ellipse, which is already attached on construction
g.add(r, False)
```
**Don't build a separate `Rectangle` just to frame an existing `LegendWidget`/`ImageWidget`** — both
already inherit every `FrameWidget` method (see above); call `setFrameStyle(...)` on the legend/image
object itself instead of layering a second shape on top.

### Positioning annotations relative to the data range (axisScale())

**`g.axisScale(axis) -> (start, end)` reads an axis's current visible range back** — use it to
compute a `setOriginCoord(x, y)` position relative to the live data range (e.g. "95% up the visible
y-axis": `ymin, ymax = l.axisScale(Graph.Axis.Left); y = ymin + 0.95 * (ymax - ymin)`), instead of
guessing a nonexistent conversion method or hardcoding a bare fraction as if it were already a data
coordinate. `setOriginCoord` does no range validation — an out-of-range value places the widget
off-plot with no error, so always derive it from `axisScale()` or known data bounds, never a bare
0..1 fraction. (`g.scale(axis, pos)` never existed — use `axisScale(axis)`.)

### QtiSAS classes need no import, ever

**`Frame`/`Image`/`Legend`/`Rectangle`/`Ellipse` (and every other QtiSAS-native class — `Table`,
`Matrix`, `Graph`, `Compiler`, `Fittable`, `Qti3D`, ...) need NO import at all, ever, from anywhere.**
They're globals in every script's namespace already — no `import qti`, no `from qti import Frame`,
no `from qti.api import Frame`, no `import qti.Frame`. `qti` is genuinely QtiSAS's own extension
module and `from qti import X` happens to work for names that live there, but it's unnecessary; any
other guessed form — a `.api`/`.core`/similar submodule, or `import qti.X` treating X as a
submodule — raises `ImportError`/`ModuleNotFoundError`, since the module has no internal package
structure to guess a path into. Confirmed live, twice, same guessed submodule both times for two
different classes: `from qti.api import Qti3D` and, separately, `from qti.api import Frame` —
`ModuleNotFoundError: No module named 'qti.api'; 'qti' is not a package` — before falling back to
using the class bare with no import, which is all that was ever needed.

### setFrameStyle() takes an int, not a bare enum member

**`setFrameStyle(int)` takes QtiSAS's own `Frame.FrameStyle` enum value, not Qt's `QFrame`.** The two
are unrelated types: `FrameWidget::setFrameStyle()` does an *exact* match against
`FrameStyle{None_=0, Line=1, Shadow=2}`, without decoding bit-flags like real `QFrame`. Passing
`QFrame.Shape.Box | QFrame.Shadow.Sunken` (a PyQt6 enum combo, e.g. `4 | 0x30`) silently sets a value
matching neither `Line` nor `Shadow`, so **no frame is drawn at all** — no error, just an invisible
border. Confirmed live.

**Pass `Frame.FrameStyle.Shadow.value` (or a bare int), never the bare enum member
`Frame.FrameStyle.Shadow`.** `FrameStyle` has a member named `None_` (`None` is a reserved Python
keyword), so it's bound as a real `enum.Enum` — NOT an int subclass — unlike enums with no `None`
member (`Graph.Axis`, `Graph.Ticks`, `Graph.Scale`, `Graph.CurveType`, ...), which are plain int
constants usable directly as a bare `int`. `setFrameStyle(int)` takes a genuine C++ `int`, and sip's
parser rejects the enum object outright — `.value` (or a literal int) is required. Same applies to
any other enum with a `.None_` member, e.g. `Table.PlotDesignation`, passed to a plain-`int`
parameter.

```python
# WRONG — real Qt enum on a QtiSAS FrameWidget; frame silently never renders
from PyQt6.QtWidgets import QFrame
leg.setFrameStyle(QFrame.Shape.Box | QFrame.Shadow.Sunken)

# WRONG — QtiSAS's own enum, but the bare member isn't an int: raises TypeError
leg.setFrameStyle(Frame.FrameStyle.Shadow)
# TypeError: setFrameStyle(self, a0: int): argument 1 has unexpected type 'FrameStyle'

# RIGHT — .value extracts the underlying int; no PyQt6 import needed
from PyQt6.QtGui import QColor, QFont
leg = layer.newLegend()
leg.setTextColor(QColor(50, 50, 50))
leg.setFont(QFont("DejaVu Sans", 10))
leg.setFrameStyle(Frame.FrameStyle.Shadow.value)
leg.setBackgroundColor(QColor(255, 255, 255))
leg.setOrigin(10, 10)
```

### Title

| Method | Notes |
|--------|-------|
| `setTitle("text")` | set plot title |
| `setTitleFont(QFont)` | |
| `setTitleColor(QColor)` | |
| `setTitleAlignment(align)` | Qt alignment flags |
| `removeTitle()` | remove the plot title |

### Axes — titles

| Method | Notes |
|--------|-------|
| `setXTitle("text")` | bottom-axis title shortcut |
| `setYTitle("text")` | left-axis title shortcut |
| `setAxisTitle(axis, "text")` | any axis |
| `setAxisTitleFont(axis, QFont)` | bold: `QFont("name", 12, QFont.Weight.Bold)` |
| `setAxisTitleColor(axis, QColor)` | requires explicit `axis` argument |
| `setAxisTitleAlignment(axis, align)` | |
| `axisTitleDistance(axis)` | `int` — gap in pixels |
| `setAxisTitleDistance(axis, dist)` | |

**Subscript and superscript** — use `<sub>...</sub>` / `<sup>...</sup>` directly in the title string. Rich text is auto-detected (Qwt renders any string with markup as rich text, plain strings stay plain) — no `setTitleFormat()` call needed.

> **RULE — Use `<sup>`/`<sub>` HTML tags, never LaTeX-style `^{...}`/`_{...}` notation, in titles.**
> `^{-1}` is not markup Qwt recognizes — it renders literally as `^{-1}`, not a superscript. Only `<sup>`/`<sub>` trigger rich-text rendering.
> ```python
> # WRONG — renders literally as "I(Q) (cm^{-1})", not "I(Q) (cm⁻¹)"
> g.setYTitle("I(Q) (cm^{-1})")
>
> # RIGHT
> g.setYTitle("I(Q) (cm<sup>-1</sup>)")
> g.setXTitle("Q (&Aring;<sup>-1</sup>)")   # &Aring; = Å ; HTML entities also work
> g.setYTitle("R<sub>g</sub> (nm)")
> ```

### Axes — labels and ticks

| Method | Notes |
|--------|-------|
| `enableAxis(axis, on)` | show / hide an axis — `on` is `True` or `False` |
| `enableAxisLabels(axis, on)` | show / hide tick labels |
| `setAxisColor(axis, QColor)` | axis line colour |
| `setAxisFont(axis, QFont)` | tick label font |
| `setAxisLabelsColor(axis, QColor)` | tick-label colour — requires explicit `axis` argument |
| `setAxisNumericFormat(axis, format, precision=6, formula="")` | `format`: 0=Automatic, 1=Decimal, 2=Scientific, 3=Superscripts, 4=Engineering, 5=SuperscriptsGER — **3 is Superscripts, not engineering** (4 is); 4th arg is a label-transform FORMULA string, not a text suffix appended to labels |
| `setAxisLabelRotation(axis, degrees)` | |
| `setMajorTicksType(axis, Graph.Ticks.*)` | |
| `setMinorTicksType(axis, Graph.Ticks.*)` | |
| `setAxisTicksLength(axis, majType, minType, minLength, majLength)` | |
| `setTicksLength(minLength, majLength)` | set for all axes at once |
| `axesLinewidth()` | current width of ticks and backbone, all axes |
| `setAxesLinewidth(width)` | set width of ticks and backbone, all axes |
| `drawAxesBackbones(yes)` | show / hide axis backbone lines |

**Tick THICKNESS is not a separate property — ticks share the axis line's own width.** Qwt's scale
draw uses one pen for both backbone and tick marks, so there is no "tick width" call distinct from
`setAxesLinewidth()`/`axesLinewidth()`. Only tick *length* (`setAxisTicksLength`/`setTicksLength`)
is independently controllable.

```python
# Read the CURRENT axes linewidth of THIS graph, then reuse it (e.g. to size ticks to match)
current_width = g.axesLinewidth()
ticklen = int(g.canvasGeometry().width() * 0.02)
g.setTicksLength(ticklen, ticklen)
g.setAxesLinewidth(current_width)   # thickness for both backbone AND ticks — same call, no separate one
```

**Don't confuse `g.axesLinewidth()` (this graph's actual current value) with
`qti.app.graphAxesLineWidth()` (the global Preferences default for NEW graphs).** The latter is the
app-wide default, not this specific graph — if this graph's width changed after creation, the two
differ. Confirmed live: before `axesLinewidth()` was exposed, a script needing "thickness as canvas
line width" with no getter available guessed the nonexistent `axesLineWidth()`, then gave up and
hardcoded `1` — silently substituting an arbitrary literal for the graph's real current width.

### Scale

| Method | Notes |
|--------|-------|
| `setScale(axis, start, end)` | set axis range (all trailing params default) |
| `setScale(axis, start, end, step=0.0, majTicks=5, minTicks=5, type=Graph.Scale.Linear, inverted=False, left_break=-DBL_MAX, right_break=DBL_MAX, pos=50, stepBeforeBreak=0.0, stepAfterBreak=0.0, minTicksBeforeBreak=4, minTicksAfterBreak=4, typeAfterBreak=Graph.Scale.Linear, breakWidth=4, breakDecoration=True)` | full signature — `left_break`/`right_break` define an axis-break gap (see below); `type`/`typeAfterBreak` are `Graph.Scale.{Linear,Log10,Ln,Log2,Reciprocal,Probability,Logit}` |
| `axisScale(axis) -> (start, end)` | read the axis's CURRENT visible range back |
| `setAutoScale()` | auto-fit all axes to data |
| `setLinOrLogAxis(axis, logYN, changeAxisFormat=True, forceRescale=False)` | switch one axis |
| `setLogLog()` | all 4 axes → log scale |
| `setLinLin()` | all 4 axes → linear scale |

**There is no standalone `setMajTicks(axis, n)` / `setMinTicks(axis, n)` / `setMajorTicks(axis, n)` /
`setMinorTicks(axis, n)` method on `Graph`.** Major/minor tick COUNT is only settable through the
`majTicks`/`minTicks` parameters of the full `setScale(...)` signature above — there's no way to
change tick count alone without also restating `start`/`end`/`step`, but you don't need to know
those in advance: read them first with `axisScale(axis)` and pass them straight back in.

```python
start, end = l.axisScale(Graph.Axis.Bottom)
l.setScale(Graph.Axis.Bottom, start, end, 0, 5, 8)   # same range, new tick counts (5 major, 8 minor)
```

Don't confuse this with the real `setMajorTicksType(axis, Graph.Ticks.*)` / `setMinorTicksType(axis,
Graph.Ticks.*)` (tick DIRECTION — In/Out/InOut/NoTicks, see the axes table above) — the name is
real, but dropping "Type" from it is not. Confirmed live TWICE with different guesses before
`axisScale()` existed: `g.setMajTicks(Graph.Axis.Bottom, 5)`, then `g.setMajorTicks(Graph.Axis.Bottom,
5)` (mirroring the real `setMajorTicksType` name) — both raised `AttributeError: 'Graph' object has
no attribute '...'` (same pattern for Min/minor variants).

### Scale — axis breaks

**Axis breaks** — `setScale(...)`'s trailing params (past `inverted`) split an axis into two
segments with a visual gap between them, useful when a middle range carries no interesting data
(e.g. `0..10` and `90..100` on the same axis, with the boring `10..90` region compressed away):

| Param | Default | Meaning |
|-------|---------|---------|
| `left_break`, `right_break` | `-DBL_MAX`, `DBL_MAX` (no break) | the data-value interval to compress into the gap |
| `pos` | `50` | break's position along the axis, as a percentage (0-100) of the available length |
| `stepBeforeBreak`, `stepAfterBreak` | `0.0`, `0.0` | tick step for each segment (`0.0` = auto) |
| `minTicksBeforeBreak`, `minTicksAfterBreak` | `4`, `4` | minimum tick count for each segment |
| `typeAfterBreak` | `Graph.Scale.Linear` | scale type for the segment AFTER the break — independent of `type` (the segment before), so e.g. linear-before/log-after is valid |
| `breakWidth` | `4` | visual width of the gap, in pixels |
| `breakDecoration` | `True` | draw the small diagonal "break" marks at the gap |

```python
# axis breaks at 10..90 — compresses that range into a narrow visual gap
l.setScale(Graph.Axis.Left, 0, 100, 0, 5, 5, Graph.Scale.Linear, False,
           10, 90, 50, 0, 0, 4, 4, Graph.Scale.Linear, 4, True)
```

> `typeAfterBreak` was WRONGLY exposed as a `bool` in older bindings (only `Linear`/`Log10` were
> reachable from Python) — fixed to accept the full `Graph.Scale.*` enum, matching `type` above.

### Canvas and frame

| Method | Notes |
|--------|-------|
| `setCanvasColor(QColor)` | canvas background fill |
| `setCanvasFrame(width=1, QColor=black)` | border around canvas |
| `setCanvasBackgroundImage(filename="", autocrop=True)` | image behind data |
| `canvasBackgroundFileName()` | `str` |
| `setCanvasGeometry(x, y, w, h)` | position and size of canvas — all arguments must be `int` |
| `setCanvasGeometry(QRect)` | |
| `canvasGeometry()` | `QRect` — canvas position and size in MultiLayer coordinates |
| `setCanvasSize(w, h)` | per-layer canvas size |
| `setBackgroundColor(QColor)` | outer background (outside canvas) |
| `setFrame(width=1, QColor=black)` | outer frame of the graph widget |

### Grid

| Method | Notes |
|--------|-------|
| `grid()` | `Grid` object for fine-grained control |
| `showGrid()` | show grid on all axes |
| `showGrid(axis)` | show grid for one axis only |
| `setGridOnTop(on=True, update=True)` | draw grid above curves |
| `hasGridOnTop()` | `bool` |

**`Grid` pen methods:**

| Method | Notes |
|--------|-------|
| `setMajPenX(QPen)` / `majPenX()` | major grid lines on X axis |
| `setMajPenY(QPen)` / `majPenY()` | major grid lines on Y axis |
| `setMinPenX(QPen)` / `minPenX()` | minor grid lines on X axis |
| `setMinPenY(QPen)` / `minPenY()` | minor grid lines on Y axis |
| `enableXMin(on)` / `xMinEnabled()` | show/hide X minor grid |
| `enableYMin(on)` / `yMinEnabled()` | show/hide Y minor grid |
| `enableXMax(on)` / `xMaxEnabled()` | show/hide X **major** grid (the major-line counterpart to `enableXMin` above — easy to miss since only the minor pair is otherwise obvious) |
| `enableYMax(on)` / `yMaxEnabled()` | show/hide Y **major** grid |
| `enableZeroLineX()` / `xZeroLineEnabled()` | show/hide a distinct X=0 reference line |
| `enableZeroLineY()` / `yZeroLineEnabled()` | show/hide a distinct Y=0 reference line |
| `setXZeroLinePen(QPen)` / `xZeroLinePen()` | style of the X=0 line |
| `setYZeroLinePen(QPen)` / `yZeroLinePen()` | style of the Y=0 line |

```python
from PyQt6.QtGui import QColor, QPen
from PyQt6.QtCore import Qt
layer.showGrid()
layer.grid().setMajPenX(QPen(QColor(200, 200, 200), 1, Qt.PenStyle.SolidLine))
layer.grid().setMajPenY(QPen(QColor(200, 200, 200), 1, Qt.PenStyle.SolidLine))
```

### Rendering and export

| Method | Notes |
|--------|-------|
| `replot()` | redraw canvas |
| `setAntialiasing(on=True, update=True)` | enable antialiasing |
| `setAutoscaleFonts(on=True)` | scale fonts when canvas is resized |
| `enableAutoscaling(on=True)` | auto-scale axes when data changes |
| `export("file.png")` | export by file extension |
| `exportImage(file, quality=100, transparent=False, dpi=72, customSize=QSizeF(), unit=FrameWidget.Unit.Pixel, fontsFactor=1.0, compression=0)` | raster |
| `exportVector(file, fontEmbedding, dpi=72, color=True, customSize=QSizeF(), unit=FrameWidget.Unit.Pixel, fontsFactor=1.0)` | vector (PDF/SVG/EPS) — `fontEmbedding` is required (no default), confirmed against the C++ implementation (unnamed in the SIP header itself) |
| `exportTex(file, color=True, escapeStrings=True, fontSizes=True, customSize=QSizeF(), unit=FrameWidget.Unit.Pixel, fontsFactor=1.0)` | LaTeX/TikZ |

---

## Constants quick reference

```python
# Graph.Axis      Left=0  Right=1  Bottom=2  Top=3
# Graph.Scale     Linear=0  Log10=1  Ln=2  Log2=3  Reciprocal=4  Probability=5  Logit=6
# Graph.Ticks     NoTicks=0  Out=1  InOut=2  In=3
# Graph.CurveType Line=0  Scatter=1  LineSymbols=2  VerticalBars=3  Area=4  Pie=5  ColorMap=16
```

---

## Full example

```python
from PyQt6.QtGui import QColor

t = newTable("SANS", 50, 3)
t.setColNames(["q", "I", "dI"])

gw = newGraph("SANS_plot")
g = gw.activeLayer()
g.addCurve(t, "I", Graph.CurveType.LineSymbols)
g.addErrorBars("SANS_I", t, "dI", 1, 1, 8, QColor("gray"), False, True, True)
g.setXTitle("q, 1/nm")
g.setYTitle("I(q), 1/cm")
g.setScale(Graph.Axis.Bottom, 0.01, 1.0, 0, 5, 5, Graph.Scale.Log10)
g.setScale(Graph.Axis.Left,   1e-3, 1e3, 0, 5, 5, Graph.Scale.Log10)
g.showGrid()
g.setAntialiasing(True)
g.replot()
```

Restyling those same error bars later (e.g. changing their color) — look the graph up, don't recreate it, and use `updateErrorBars()`, not another `addErrorBars()`:

```python
from PyQt6.QtGui import QColor

t = table("SANS")
gw = graphWindow("SANS_plot")
g = gw.activeLayer()
g.updateErrorBars("SANS_I", t, "dI", 1, 1, 8, QColor("red"), False, True, True)
g.replot()
```

---

---

## For AI: Common Mistakes and Mandatory Patterns

### GraphWindow vs Graph (layer) — the most common error source

Many methods belong to `Graph` (a single layer) but are accidentally called on `GraphWindow`. This raises `AttributeError`.

| Method | Correct object | Wrong object |
|--------|---------------|-------------|
| `grid()` | `layer.grid()` | ~~`gw.grid()`~~ |
| `showGrid()` | `layer.showGrid()` | ~~`gw.showGrid()`~~ |
| `replot()` | `layer.replot()` | ~~`gw.replot()`~~ |
| `canvasGeometry()` | `layer.canvasGeometry()` | ~~`gw.canvasGeometry()`~~ |
| `showAxis()` | does not exist — use `layer.enableAxis(ax, True)` | — |
| `setXTitle()` / `setYTitle()` / `setAxisTitle()` | `layer.setXTitle(...)` etc. | ~~`gw.setXTitle(...)`~~ |
| `setScale()` | `layer.setScale(...)` | ~~`gw.setScale(...)`~~ |

```python
# WRONG — all raise AttributeError
gw.grid().setMajPenX(...)
gw.showGrid()
gw.replot()
gw.canvasGeometry()
gw.setXTitle("Q")

# RIGHT
layer = gw.activeLayer()   # or gw.layer(n)
layer.showGrid()
layer.grid().setMajPenX(...)
layer.replot()
layer.canvasGeometry()
layer.setXTitle("Q")
```

### `showAxis()` does not exist

```python
# WRONG — TypeError: no such method
layer.showAxis(Graph.Axis.Right)
# RIGHT
layer.enableAxis(Graph.Axis.Right, True)   # show
layer.enableAxis(Graph.Axis.Right, False)  # hide
```

### `activateWindow` after `maximizeWindow` — undoes the maximize

```python
# WRONG — activateWindow at the end cancels the maximize; window will NOT be maximized
gw = newGraph("Fit", 2, 2, 1)
maximizeWindow(gw)
# ... setup ...
activateWindow(gw)   # ← BREAKS IT

# RIGHT — for a freshly created graph, maximizeWindow is sufficient; never follow it with activateWindow
gw = newGraph("Fit", 2, 2, 1)
maximizeWindow(gw)
# ... setup ...
# (no activateWindow at the end)
```

Only use `activateWindow(gw)` **before** `maximizeWindow(gw)` when other windows were opened between `newGraph` and the maximize call — never after.

### `maximizeWindow` must come before `arrangeLayers` and `canvasGeometry`

Getting layer references with `gw.layer(n)` is a pure getter, safe before or after `maximizeWindow`. What matters: `maximizeWindow` must be called **before `arrangeLayers` and `canvasGeometry()`**.

```python
# WRONG — maximizeWindow after arrangeLayers/canvasGeometry; geometry is measured before resize
gw = newGraph("Fit", 2, 2, 1)
gw.arrangeLayers(False, False)   # ← before maximize
cr1 = l1.canvasGeometry()        # ← sizes are pre-maximize
maximizeWindow(gw)

# RIGHT — maximizeWindow before arrangeLayers (layer() getters may come before or after)
gw = newGraph("Fit", 2, 2, 1)
maximizeWindow(gw)
l1 = gw.layer(1)
l2 = gw.layer(2)
# ... enableAxis, keepAligned ...
gw.arrangeLayers(False, False)   # ← after maximize: correct
cr1 = l1.canvasGeometry()        # ← correct post-maximize size
```

### PyQt6 imports must be at the top

```python
# WRONG — imports buried mid-script; easy to miss and breaks if any PyQt6 symbol is used before the import line
gw = newGraph(...)
# ... many lines ...
l1.showGrid()
from PyQt6.QtGui import QColor, QPen   # ← too late / too buried
from PyQt6.QtCore import Qt

# RIGHT — all PyQt6 imports first, unconditionally
from PyQt6.QtGui import QColor, QFont, QPen, QBrush
from PyQt6.QtCore import Qt
# from PyQt6.QtWidgets import QFrame   # only if QFrame is used
gw = newGraph(...)
```

### Stacked panels — additional rules not covered by the recipe or call-outs above

The 7-step recipe above plus the two `maximizeWindow`/`activateWindow` call-outs already cover
ordering, `int`-vs-`float` for `setCanvasGeometry`, `removeTitle()`, `linkXLayerAxes(True)`, and why
`l1.setXTitle(...)` never renders. Two things they don't cover:

**Do not call `setTitle()` on a layer after `removeTitle()`** — `removeTitle()` removes the title
widget, `setTitle("text")` re-adds it; they cancel each other. For the residuals layer (l2): call
**only** `removeTitle()`, never `setTitle()` afterward.

**`newGraph` rows/cols for vertical stacking must be `(2, 2, 1)`** — `(2, 1, 1)` (1 row × 1 col)
cannot accommodate 2 layers vertically.

**Call `replot()` on every layer, not just `l1`** — `l2`'s display is not otherwise updated.

### `setAxisLabelsColor` and `setAxisTitleColor` require an explicit axis

```python
# WRONG — raises TypeError
layer.setAxisLabelsColor(QColor(40, 40, 40))
# RIGHT
for ax in [Graph.Axis.Bottom, Graph.Axis.Left, Graph.Axis.Top, Graph.Axis.Right]:
    layer.setAxisLabelsColor(ax, QColor(40, 40, 40))
    layer.setAxisTitleColor(ax, QColor(30, 30, 30))
```

### `addCurve` requires a Table object, not a string

```python
# WRONG — TypeError
g.addCurve("SANS_Data_I", Graph.CurveType.Scatter)
# RIGHT
t = table("SANS_Data")
g.addCurve(t, "I", Graph.CurveType.Scatter)
```

### addCurve/insertCurve need the fitCurve TABLE, not the Fittable object itself

Confirmed live: after `f.fit()`, a script called `l1.addCurve(f, "y", Graph.CurveType.Line)` —
passing the `Fittable` object itself — raising `TypeError: argument 1 has unexpected type
'fittable18'`. `f`/`Fittable` is never a valid curve source; `fit()` creates a real, lookupable
`fitCurve-<model>` TABLE (see fittable-python-api.md's "Fit curve table" section) — curve from
that instead.

```python
# WRONG — Fittable object is not a Table
f.fit()
l1.addCurve(f, "y", Graph.CurveType.Line)

# RIGHT — look up the table fit() actually created
f.fit()
fc = table("fitCurve-" + model_name)
l1.addCurve(fc, "y", Graph.CurveType.Line)
```

### EXPLORE the data table before adding curves

When a task says "plot data from table X", **EXPLORE the table first**.

- `table("name")` returns an **invalid object** (not `None`) when the table does not exist.
  Passing it to `addCurve()` raises `ValueError: Invalid table in addCurve()`.
- Always call `existTable("name")` before `table("name")`.
- Never invent column names — use EXPLORE to find them.
- Never assume Fittable is running or that fit/residuals columns exist unless the task says so.

```python
# EXPLORE — confirm table exists and check column names before plotting
scriptPrint("exists: " + str(existTable("Fit")))
if existTable("Fit"):
    t = table("Fit")
    scriptPrint("cols: " + str(t.colNames()))
    scriptPrint("rows: " + str(t.numRows()))
else:
    scriptPrint("table Fit not found — cannot plot")
```

After EXPLORE reveals the actual column names, use them in `addCurve`.

**WRONG — `table()` without existence check:**
```python
t = table("Fit")                              # returns invalid object if table missing
l1.addCurve(t, "y", Graph.CurveType.Line)    # ValueError: Invalid table in addCurve()
```

**RIGHT:**
```python
if not existTable("Fit"):
    raise RuntimeError("table 'Fit' not found")
t = table("Fit")
l1.addCurve(t, "I", Graph.CurveType.Line)    # use column name confirmed by EXPLORE
```

### `addCurve` takes one column name (Y only) — no xcol argument

`addCurve(t, col, style)` takes **only the Y column name**. The X column is the one with
`PlotDesignation.X` role in the table. There is no `(t, xcol, ycol, style)` form.
Use `insertCurve(t, xcol, ycol, style)` when you need to specify both X and Y by name.

**WRONG — inventing a 4-argument xcol+ycol form:**
```python
l1.addCurve(fc, "x", "y", Graph.CurveType.Line)   # TypeError: argument 3 has unexpected type 'str'
```

**RIGHT — Y column only; X from PlotDesignation.X:**
```python
fc.setColumnRole(1, Table.PlotDesignation.X)   # mark column 1 as X
l1.addCurve(fc, "y", Graph.CurveType.Line)     # column "y" is the Y axis
```

**RIGHT — explicit xcol and ycol via insertCurve:**
```python
l1.insertCurve(fc, "x", "y", Graph.CurveType.Line)
```

`insertCurve()` validates its table argument the same way `addCurve()` does — passing an
invalid/missing table raises `ValueError: Invalid table in insertCurve()` rather than plotting
nothing, so apply the same EXPLORE-before-plotting check as above.

**`insertCurve()` also validates that the named column(s) actually exist in the table** — passing
a column name the table doesn't have raises `ValueError: insertCurve(): column '<name>' not found
in table '<table>'` (or `X column`/`Y column` for the two-name form) instead of silently plotting
nothing. This matters most when mixing tables with different naming: a `fitCurve-<name>` analysis
table's own columns are ALWAYS the fixed `x`/`y`/`weight`/`sigma`/`residues`/... set (see
fittable-python-api.md), never the SOURCE data table's own column names — passing the source
table's custom names (e.g. `"xx"`/`"yy"`) to `insertCurve()` on a `fitCurve-<name>` table now
raises immediately instead of silently producing an empty curve. Confirmed live: a script mixed
the source table's own custom column names into an `insertCurve()` call against the `fitCurve-`
table, which returned `None` with no visible error — the winning model's fit curve and residuals
were silently never plotted, and the run reported success anyway.

```python
# WRONG — "xx"/"yy" are the SOURCE table's columns, not fitCurve-<name>'s (always x/y/residues)
fc = table("fitCurve-ai/LorentzianPeak")
l1.insertCurve(fc, "xx", "yy", Graph.CurveType.Line)   # ValueError: X column 'xx' not found

# RIGHT — fitCurve-<name> tables always use the fixed x/y/residues names, regardless of what the
# source data table's own columns were called
l1.insertCurve(fc, "x", "y", Graph.CurveType.Line)
```

### `addErrorBars` — second argument is a Table object

The first argument is the **curve title** — `"TableName_ColumnName"` when added via `addCurve` or
`plot()` (e.g. column `"I"` of table `"Exp"` → title `"Exp_I"`).

`addErrorBars` requires `QColor` — import it **at the top of the script**, not at point of use.

```python
# WRONG — import mid-script
l.addErrorBars("Exp_I", t, "dI", 1, 1, 8, QColor("gray"), False, True, True)
from PyQt6.QtGui import QColor   # ← too late; also NameError on the line above

# RIGHT — import at top, before any other code
from PyQt6.QtGui import QColor
...
l.addErrorBars("Exp_I", t, "dI", 1, 1, 8, QColor("gray"), False, True, True)
#              ^^^^^^^ curve title = TableName_ColumnName; second arg is Table object
```

### Do not override appearance defaults — let Preferences apply

Every new layer auto-applies user Preferences at creation (fonts, colors, tick style, backbone, grid). The calls below **override those defaults** and must be omitted from a neutral template:

```python
# ALL WRONG — each overrides a user preference
l1.setAxisTitleFont(Graph.Axis.Left, QFont("DejaVu Sans", 11, QFont.Weight.Bold))
l1.setAxisFont(Graph.Axis.Bottom,    QFont("DejaVu Sans", 10))
l1.setAxisLabelsColor(Graph.Axis.Left,   QColor(40, 40, 40))
l1.setAxisLabelsColor(Graph.Axis.Bottom, QColor(40, 40, 40))
l1.setAxisColor(Graph.Axis.Left, QColor("gray"))
l1.drawAxesBackbones(False)
l1.showGrid()

# RIGHT — omit all of the above; the layer already has correct defaults from Preferences
```

Complete list of banned hardcodes (omit unless the template deliberately differs from user prefs;
audited against every control in Preferences → 2D Plots → Options/Axes/Grid):
`setAxisTitleFont`, `setAxisFont`, `setAxisLabelsColor`, `setAxisTitleColor`, `setAxisColor`,
`setAxesLinewidth`, `drawAxesBackbones`, `showGrid`, `setCanvasColor`, `setBackgroundColor`,
`enableAxis` (per-axis visibility), `enableAxisLabels` (per-axis label visibility),
`setAntialiasing`, `enableAutoscaling`, `setAutoscaleFonts`, `setFrame` (outer border),
`setCanvasFrame`, and the `Grid` pen methods (`setMajPenX`/`setMajPenY`/`setMinPenX`/`setMinPenY`/
`enableXMin`/`enableYMin`/`enableXMax`/`enableYMax`) — Preferences → 2D Plots → Grid controls
color/style/width/antialiasing for major AND minor grid lines per axis, not just whether the grid
shows at all.

```python
# WRONG — showGrid() overrides the user's grid preference
l1.showGrid()
l2.showGrid()

# RIGHT — omit showGrid(); grid state is inherited from Preferences
# Only call showGrid() if the template explicitly requires a grid regardless of preferences
```

### Do not override curve/symbol appearance defaults either

A newly-added curve (`addCurve()`/`plot()`) already inherits its line width/style, symbol style/
size/border, and colours from Preferences → 2D Plots → **Curves** — the same "only touch it if the
request asks for specific styling" rule as the layer-level list above applies to curve methods too:

`setCurveLineWidth`, `setCurveLineStyle`, `setCurveLineColor` (unless a specific colour was
requested), `setCurveSymbolStyle`, `setCurveSymbolSize`, `setCurveSymbolBorderWidth`,
`setCurveSymbolColor`/`setCurveSymbolBorderColor` (unless a specific colour was requested).

```python
# WRONG — no styling was requested; this overrides the user's Preferences → Curves defaults
l.addCurve(t, "y", Graph.CurveType.Line)
l.setCurveLineWidth(0, 2)
l.setCurveSymbolStyle(0, symbolIndex("Circle"))
l.setCurveSymbolSize(0, 6)

# RIGHT — omit all of the above; the new curve already has correct defaults from Preferences
l.addCurve(t, "y", Graph.CurveType.Line)
```

**Do** call these when the request explicitly names a colour, line width, or symbol — that's real,
requested content, not an unprompted override of the user's own defaults.

### Graph defaults — read before overriding
To read the current defaults before creating a template:

```python
# EXPLORE — check defaults before creating a graph template
scriptPrint("titleOn=%s" % qti.app.graphTitleOn())
scriptPrint("majTicksStyle=%d  minTicksStyle=%d" % (qti.app.graphMajTicksStyle(), qti.app.graphMinTicksStyle()))
scriptPrint("canvasColor=%s" % qti.app.graphCanvasColor().name())
scriptPrint("axesLineWidth=%d  drawBackbones=%s" % (qti.app.graphAxesLineWidth(), qti.app.graphDrawBackbones()))
```

Full list of default getters in `app-python-api` (`graphAxesFont()`, `graphTitleFont()`, `graphBackgroundColor()`, etc.).
