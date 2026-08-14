# QtiSAS Graph
# gw=newGraph("name")|newGraph("name",layers,rows,cols)|graphWindow("name")|currentGraph()
# graphWindow("name") returns None (confirmed) if no graph with that name exists — ALWAYS
# check "if gw is None:" before use.
#    plot(table,("x","y"),Graph.CurveType.Line)|plot(matrix,Graph.CurveType.ColorMap)|plot("f(x)",xMin,xMax)
# GraphWindow=multi-layer window(MultiLayer); Graph=one panel.
# plot("f(x)",xMin,xMax) — the string is a MATH FORMULA (e.g. "x^2"), NOT a dataset/table name
#   lookup — there is no such overload. Confirmed live: plot("simulatedCurve-<model>", xMin, xMax,
#   style) silently added a function curve literally labeled with that string, no exception raised.
#   To plot an existing table (e.g. fitCurve-/simulatedCurve-<model>): t=table(name);
#   gw.activeLayer().insertCurve(t,"x","y",style) instead.
# graphWindow("name") is 2D-GraphWindow-ONLY — returns None for a Graph3D window even when one
#   genuinely exists under that name (confirmed live). Use plot3D("name") to look up an existing
#   3D surface window instead (see graph3d-python-api.pyi) — the single-arg form is a LOOKUP, not
#   a creation. Confirmed live: this cost 8+ retries chasing dead ends after setWindowName()
#   already created the real Graph3D window.
#
# DEFAULTS: every new layer has user Preferences (fonts, tick style, backbone, canvas frame, grid,
#   antialiasing, axis visibility) auto-applied. DO NOT re-set these unless explicitly required.
#   Only call setAxisFont/setTitleFont/setAxesLinewidth/drawAxesBackbones/showGrid/... when overriding
#   default — showGrid() unconditionally is the single most common violation of this rule.
#
# Qt CLASSES (QColor, QFont, QPen): use ONLY when no native QtiSAS method exists.
#   Must import explicitly: from PyQt6.QtGui import QColor, QFont, QPen, QBrush
#                           from PyQt6.QtCore import Qt
#   Do NOT import PyQt6's QFrame for setFrameStyle() — see FrameWidget note below; it's a different,
#   unrelated enum and silently produces no visible frame.
#   colorIndex("name")->int   use for curve colours  (NOT QColor)
#   WARNING: colorIndex("name") and QColor("name") for the SAME name are NOT the same colour.
#   colorIndex() uses QtiSAS's own palette ("green"=Qt::green, bright RGB(0,255,0)). QColor()
#   uses Qt's SVG/X11 named-colour table, where "green" is RGB(0,128,0) — dark, looks like the
#   palette's own "olive", not its "green". Confirmed: setCurveLineColor(0, QColor("green"))
#   renders as olive-looking, not bright green. Prefer colorIndex() for standard colour names on
#   setCurveLineColor()/setCurveSymbolColor()/setCurveSymbolBorderColor() — the only three methods
#   that accept it — for two reasons: no palette-mismatch surprises, AND no import needed at all
#   (unlike QColor, which needs "from PyQt6.QtGui import QColor" first). Every other colour param in
#   this API (addErrorBars, setAxisColor, setCanvasColor, setBackgroundColor, ...) takes QColor ONLY.
#   symbolIndex("name")->int  use for curve symbols  (NOT QwtSymbol)
#   Use QColor only when a method explicitly requires it (e.g. addErrorBars color arg, setCanvasColor)
#   For one specific RGB value with NO import at all: a bare 0xRRGGBB int works anywhere a QColor
#   param is expected (confirmed live): leg.setBackgroundColor(0xFFFFFF) == QColor(255,255,255).
#   Prefer this over falling back to Qt.white (also unimported) after a QColor NameError.
#   EXCEPTION: does NOT work for setCurveLineColor()/setCurveSymbolColor()/setCurveSymbolBorderColor()
#   — each has BOTH an (index,int) colorIndex overload AND an (index,QColor) overload; a bare int
#   binds to the (index,int) one, not QColor, so setCurveLineColor(0, 0x336699) means colorIndex
#   3375257 (wildly out of range) — confirmed in source: silently no-ops, no error, curve keeps its
#   old colour (a SMALL hex value is worse — silently picks whatever unrelated colour is at that
#   small index instead). For these 3 methods only: use colorIndex("name") or QColor(r,g,b), never
#   a bare int, however much it looks like a valid RGB hex literal.
# Axis:      Graph.Axis.Left=0 Right=1 Bottom=2 Top=3 X=Bottom Y=Left (X/Y are convenience aliases)
# Scale:     Graph.Scale.Linear=0 Log10=1 Ln=2 Log2=3 Reciprocal=4 Probability=5 Logit=6
# Ticks:     Graph.Ticks.NoTicks=0 Out=1 InOut=2 In=3
# CurveType: Graph.CurveType.Line=0 Scatter=1 LineSymbols=2 VerticalBars=3 Area=4 Pie=5
#            VerticalDropLines=6 Spline=7 HorizontalSteps=8 Histogram=9 HorizontalBars=10
#            VectXYXY=11 ErrorBars=12 Box=13 VectXYAM=14 VerticalSteps=15
#            ColorMap=16 GrayScale=17 Contour=18 Function=19

# GraphWindow:
# numLayers()->int  activeLayer()->Graph  layer(n)->Graph  layersList()->list
# addLayer(x,y,w,h,linked)->Graph  removeLayer(g)->bool  removeActiveLayer()->bool
# setNumLayers(n)  setActiveLayer(g)  swapLayers(src,dest)  setEqualSizedLayers()
# setCols(n)  setRows(n)  setMargins(l,r,t,b)
# setLayerCanvasSize(w,h)  # ALL layers uniformly — GraphWindow method
# arrangeLayers(fit,userSize)  setScaleLayersOnResize(on)
# setAlignement(hAlign,vAlign)
# setAlignPolicy(policy)  — GraphWindow.AlignPolicy.AlignLayers(default) | GraphWindow.AlignPolicy.AlignCanvases
#   AlignCanvases: arrangeLayers pins canvases directly → same X, same width for all layers in a column
#                  setSpacing(rowGap,colGap) becomes canvas-to-canvas gap (NOT layer-widget gap)
#   g.setAlignPolicy(GraphWindow.AlignPolicy.AlignCanvases)  ← correct; int fails; g.AlignCanvases fails
# setSpacing(rowGap,colGap)  — layer-widget gap (AlignLayers) OR canvas-to-canvas gap (AlignCanvases)
#
# STACKED PANELS (e.g. data+fit / residuals) — ZERO CANVAS GAP — ALWAYS use this pattern:
#   g = newGraph("name", 2, 2, 1)
#   l1 = g.layer(1); l2 = g.layer(2)
#   l1.enableAxis(Graph.Axis.Bottom, False)   # seam axes disabled BEFORE arrangeLayers
#   l2.enableAxis(Graph.Axis.Top,    False)
#   g.keepAligned(False)                                     # release before ANY layout change (safe if script re-runs)
#   g.setAlignPolicy(GraphWindow.AlignPolicy.AlignCanvases)  # REQUIRED for zero canvas gap
#   g.setSpacing(0, 0)                                       # canvas-to-canvas gap = 0
#   g.arrangeLayers(False, False)
#   # ... setCanvasGeometry for asymmetric heights here (both layers, int args) — 1st pass ...
#   # add curves and set titles HERE — this shifts l1/l2 canvas rects INDEPENDENTLY via
#   # per-layer Y-axis tick-label autoscale/margin changes, even after the 1st setCanvasGeometry
#   # X title: only on l2 — titles have no effect on a disabled axis, see setXTitle note below
#   # ... setCanvasGeometry AGAIN with the SAME values — 2nd pass, corrects drift from curves/titles ...
#   g.keepAligned(True)          # lock LAST, only once nothing will disturb geometry again
#   g.linkXLayerAxes(True)
#
# ADDING A PANEL TO AN EXISTING single-layer graph (e.g. residuals added in a LATER step, not at
# creation time): setNumLayers(n) does NOT update the window's row/col layout — it only adds the
# Graph object. arrangeLayers(fit=False,...) then uses the OLD (rows=1,cols=1), clamping the new
# layer's row back onto row 0 — SAME position as layer 1, hidden behind it. No error, no wrong
# output — the script "succeeds" and the panel is just invisible. fit=True doesn't fix it either:
# for exactly 2 layers it picks 1 row x 2 cols (side-by-side), not a vertical stack. Fix: call
# setRows()/setCols() explicitly, BEFORE arrangeLayers(), every time you grow layer count on an
# already-existing graph:
#   gw = graphWindow("FitResults")   # already exists with 1 layer from an earlier step
#   gw.setNumLayers(2)
#   gw.setRows(2); gw.setCols(1)     # REQUIRED — setNumLayers() alone leaves rows/cols unchanged
#   l1 = gw.layer(1); l2 = gw.layer(2)
#   # ... continue with the STACKED PANELS pattern above from enableAxis() onward ...
#
# keepAligned(on=True)  isKeepAligned()->bool  # pin canvas rect (pos+size); re-applied on every layer update
#   keepAligned(False) before re-arrangement, keepAligned(True) after the 2nd setCanvasGeometry pass
# setCommonLayerAxes(vertical,horizontal)  linkXLayerAxes(link)  reverseWaterfallOrder()
# export("file.png")  exportImage(file,quality,transparent,dpi)
# exportVector(file,fontEmbed,dpi,color)  exportTex(file,color,escapeStrings,fontSizes)

# Graph — add curves:
# ALWAYS pass a Table object — addCurve("tableName_col", type) raises TypeError at runtime (no string overload)
# col is the BARE column label ("I"), NOT the full "tableName_I" form
# WRONG: g.addCurve("SANS_Data_I", Graph.CurveType.Scatter)
# RIGHT: t = table("SANS_Data"); g.addCurve(t, "I", Graph.CurveType.Scatter)
# ONE Y column (str), X auto-detected: g.addCurve(t, "y", Graph.CurveType.Line)
# TUPLE of Y columns:                  g.addCurves(t, ("y1","y2"), Graph.CurveType.Scatter)
# addCurve(t,col,style=0,lineWidth=1,pointSize=3,startRow=0,endRow=-1)->bool
# addCurves(t,cols_tuple,style=0,lineWidth=1,pointSize=3,startRow=0,endRow=-1)->bool
# insertCurve(t,col,style,startRow=0,endRow=-1)->DataCurve
# insertCurve(t,xcol,ycol,style,startRow=0,endRow=-1)->DataCurve
#   validates t like addCurve does -> ValueError: Invalid table in insertCurve() if invalid/None
# removeCurve(index|title|curve)  deleteFitCurves()
# numCurves()->int  curve(index|title)  curveTitle(index)->str
# dataCurve(index)->DataCurve  boxCurve(index)->BoxCurve  functionCurve(index)->FunctionCurve
# RULE: curve(index) returns a low-level PlotCurve/Qwt object. ALL styling below (curve type, line
#   color/style/width, symbol color/style/size) is a method on the LAYER, taking an index — never a
#   method on this object. EXCEPTION: curve(index).setPen(QPen(...)) genuinely works (inherited
#   straight from Qwt) as a lower-level escape hatch — but prefer setCurveLineColor()/etc. below, the
#   idiomatic QtiSAS calls. Confirmed live AttributeError, repeatedly: curve(0).setSymbolColor(...),
#   setCurveColor(), setCurvePenColor(), setCurvePen() — none of these exist under those names.
# changeCurveIndex(from,to)  reverseCurveOrder()
# setCurveStyle(index, Graph.CurveType)  — change curve type (Line/Spline/Scatter/…)
# setCurveLineColor: index 0-based; colorIndex int (0=black 1=red 2=green 3=blue 4=cyan ...)
# NOTE: Fittable has a DIFFERENT, same-named setCurveColor() (styles the fit curve before
#   fit()/plotResults() even creates it) — don't confuse the two classes' methods.
# setCurveLineColor(index,colorIndex|QColor)  setCurveLineStyle(index,Qt.PenStyle.DashLine)  # PyQt6: Qt.PenStyle.X
# setCurveLineWidth(index,width)  setCurveAxes(index,xAxis,yAxis)
# showMissingDataGap(on,update)  isMissingDataGapEnabled()->bool  setGrayScale()  setIndexedColors()
# setWaterfallOffset(x,y,update)  setWaterfallSideLines(on)  setWaterfallFillColor(QColor)

# Graph — curve symbols (same RULE as above applies — layer methods, not curve(index) methods;
#   also NEVER construct a raw QwtSymbol and call dataCurve(...).setSymbol() with it — same rule):
# setCurveSymbolColor(index, colorIndex|QColor)       # fill / brush colour
# setCurveSymbolBorderColor(index, colorIndex|QColor) # border / pen colour
# setCurveSymbolBorderWidth(index, float)             # border / pen width in points
# setCurveSymbolOpacity(index, int)                   # fill opacity 0=transparent 255=opaque
# setCurveSymbolStyle(index, int)                     # shape; use symbolIndex("name") for lookup
# setCurveSymbolSize(index, int)                      # size = PlotDialog spinner value; pixel width = 2*size+1
# WRONG: layer.setMarkerSize(3)  — method does not exist; use setCurveSymbolSize(curveIndex, size)
# Prefer colorIndex("name") over QColor("name") for both colour params above — no import needed,
# no palette mismatch (see WARNING near setCurveLineColor above); QColor only for a specific RGB
# value or a colour outside the indexed palette.
# Usage: layer.setCurveSymbolColor(0, colorIndex("red"))
#        layer.setCurveSymbolStyle(0, symbolIndex("diamond"))
#        layer.setCurveSymbolSize(0, 3)
# pointSize at curve creation: addCurve(t, col, Graph.CurveType.LineSymbols, lineWidth, pointSize)
# Note: addCurve pointSize is raw pixel width; setCurveSymbolSize uses UI convention (2*n+1 pixels)

# Graph — error bars:
# SIGNATURE: addErrorBars(curveName, table, col, ...)  — 2nd arg is Table, 3rd is bare col label
# WRONG: g.addErrorBars("SANS_Data_I", "SANS_Data_dI")       ← TypeError (no string/string overload)
# RIGHT: g.addErrorBars("SANS_Data_I", t, "dI", 1, 1, 8, QColor("gray"), False, True, True)
# addErrorBars(curveName|dataCurve, t, col, type=1, w=1, cap=8, color=QColor(...), through=True, minus=True, plus=True)
# type:0=X 1=Y  color:QColor NOT colorIndex()
# through=line through point  minus=lower bar  plus=upper bar
# (False,False,False) = INVISIBLE — use (False,True,True) for classic ±dI SANS bars
#
# TO RESTYLE existing error bars (color/width/cap/through/minus/plus), use updateErrorBars() —
#   NEVER call addErrorBars() again, it has no "already exists" check and just adds a SECOND,
#   overlapping ErrorBarsCurve on top of the first. Exact same args as addErrorBars() (type included):
#   updateErrorBars(curveName, t, col, type=1, w=1, cap=8, color=QColor(...), through=True, minus=True, plus=True)
# WRONG: g.addErrorBars("SANS_Data_I", t, "dI", 1,1,8, QColor("red"), False,True,True)  # duplicate!
# RIGHT: g.updateErrorBars("SANS_Data_I", t, "dI", 1,1,8, QColor("red"), False,True,True)  # confirmed live

# Graph — special types:
# plotBox(t,names_list,startRow=0,endRow=-1)
# plotPie(t,col,startRow=0,endRow=-1)->PieCurve  isPiePlot()->bool  pieLegend()->str
# plotSpectrogram(matrix,Graph.CurveType.ColorMap)->Spectrogram  spectrogram(matrix)->Spectrogram
# addHistogram(matrix)->Histogram  plotVectors(t,cols_list,style,startRow=0,endRow=-1)->VectorCurve
#
# Histogram (returned by addHistogram):
#   h = layer.addHistogram(matrix("Det")); h.loadData()           # auto
#   h.setAutoBinning(False); h.setBinning(1.0,h.minimum(),h.maximum())
#   h.setRatio(1.003); h.setSkipEmptyBins(True); h.loadData()     # manual geometric
#   setAutoBinning(autoBin=True)  autoBinning()->bool
#   setBinning(size,begin,end)    — first-bin width + range; disables auto
#   begin()->float  end()->float  binSize()->float
#   ratio()->float  setRatio(r)   — 1.0=linear  >1.0=geometric progression
#   skipEmptyBins()->bool  setSkipEmptyBins(on)   — manual mode only
#   loadData()                    — recompute after parameter change
#   dataSize()->int  x(i)->float  y(i)->float  — bin centre / count (inherited from QwtPlotCurve)
#   mean()->float  standardDeviation()->float  minimum()->float  maximum()->float
#   matrix()->Matrix
# addFunction(formula,xMin,xMax,points)->FunctionCurve
# addParametricFunction(xExpr,yExpr,tMin,tMax,points,var)->FunctionCurve
# addPolarFunction(rExpr,thetaExpr,tMin,tMax,points,var)->FunctionCurve

# Graph — scale/axes:
# e.g.: g.setScale(Graph.Axis.Bottom, 0.01, 1.0, 0, 5, 5, Graph.Scale.Log10)
# setScale(axis,start,end,step=0,majorTicks=5,minorTicks=5,type=0,inverted=False)
# axisScale(axis)->(start,end)  # read current visible range back; feed into setScale(...) to change
#   tick count while keeping the same range: start,end = l.axisScale(ax); l.setScale(ax,start,end,0,5,8)
# setLinOrLogAxis(axis,logYN,changeAxisFormat=True,forceRescale=False)  # single-axis log/lin toggle; forceRescale=True snaps to data extent even when current scale already contains data
# setLogLog()   # all 4 axes → log scale
# setLinLin()   # all 4 axes → linear scale
# setAutoScale()
# NOTE: setMargin() does NOT exist on Graph — method was removed in Qwt 6.x. No replacement.
# NOTE: setMajTicks/setMinTicks/setMajorTicks/setMinorTicks(axis,n) do NOT exist on Graph — use
#   axisScale(axis)+setScale(...) above instead. Don't confuse with the REAL
#   setMajorTicksType(axis,Graph.Ticks.*)/setMinorTicksType(...) (tick DIRECTION, not count) below.
# setXTitle(text)  setYTitle(text)  setAxisTitle(axis,text)
# setXTitle/setAxisTitle on a DISABLED axis has no visible effect — set titles only on enabled axes
# e.g. in stacked panels: set X title on BOTTOM layer only (Bottom axis enabled), not on top layer
# setAxisTitleFont(axis,QFont)  setAxisTitleColor(axis,QColor)  setAxisTitleAlignment(axis,align)
# QFont bold — PyQt6: QFont("name",12,QFont.Weight.Bold)   PyQt5: QFont("name",12,QFont.Bold)
# QFont.Bold raises AttributeError in PyQt6 — always use QFont.Weight.Bold
# setAxisTitleColor/setAxisLabelsColor — axis arg REQUIRED; setAxisTitleColor(QColor) fails
# axisTitleDistance(axis)->int  setAxisTitleDistance(axis,dist)
# SUBSCRIPT/SUPERSCRIPT: use <sub>...</sub> / <sup>...</sup> directly in the title string — rich
# text is auto-detected, no setTitleFormat() call needed.
#   WRONG: g.setYTitle("I(Q) (cm^{-1})")  ← LaTeX-style ^{} is NOT recognized, renders literally
#   RIGHT: g.setYTitle("I(Q) (cm<sup>-1</sup>)")   g.setXTitle("Q (&Aring;<sup>-1</sup>)")
#          g.setYTitle("R<sub>g</sub> (nm)")                                      # &Aring;=Å
# enableAxis(axis,on)  enableAxisLabels(axis,on)  setAxisColor(axis,QColor)
# showAxis() does NOT exist (TypeError: no such method) — use enableAxis(axis,True)/False to
#   show/hide instead: layer.enableAxis(Graph.Axis.Right, True)
# setAxisFont(axis,QFont)  setAxisLabelsColor(axis,QColor)  # tick label number colour; axis required
# to apply to all axes: for ax in [Graph.Axis.Bottom,Graph.Axis.Left,Graph.Axis.Top,Graph.Axis.Right]: layer.setAxisLabelsColor(ax,c); layer.setAxisTitleColor(ax,c)
# setAxisNumericFormat(axis,fmt,prec,formula)   # fmt:0=auto 1=decimal 2=scientific 3=superscripts
#   4=engineering 5=superscriptsGER — 3 is superscripts, NOT engineering (4 is); 4th arg is a
#   label-transform FORMULA string, not a text suffix appended to labels
# setAxisLabelRotation(axis,degrees)
# setMajorTicksType(axis,Graph.Ticks.Out)  setMinorTicksType(axis,Graph.Ticks.Out)
# setAxisTicksLength(axis,majType,minType,minLength,majLength)  setTicksLength(minLength,majLength)
# axesLinewidth()->int  setAxesLinewidth(width)  # width of ticks AND backbone, all axes — same call
# drawAxesBackbones(yes)
#   NO separate tick-thickness call — ticks share the axis line's own pen/width (Qwt scale draw).
#   axesLinewidth() is THIS graph's current value; qti.app.graphAxesLineWidth() is only the global
#   Preferences default for NEW graphs — they diverge once a graph's width is changed post-creation.

# Graph — canvas/frame/grid:
# setCanvasColor(QColor)  setCanvasFrame(width,QColor)  setCanvasBackgroundImage(file,autocrop)
# setCanvasGeometry(x,y,w,h)
# canvasGeometry()->QRect  # canvas position+size in MultiLayer coords; use for zero-gap asymmetric sizing
# setCanvasSize(w,h)  # per-layer — Graph method; for all layers use graphWindow.setLayerCanvasSize(w,h)
# setBackgroundColor(QColor)  setFrame(width,QColor)
# grid()->Grid  showGrid()  showGrid(axis)  setGridOnTop(on,update)  hasGridOnTop()->bool
# Grid pen methods — use QPen (no setMajorGridColor/setMinorGridColor methods):
#   grid().setMajPenX(QPen)  grid().majPenX()->QPen   # major X grid
#   grid().setMajPenY(QPen)  grid().majPenY()->QPen   # major Y grid
#   grid().setMinPenX(QPen)  grid().minPenX()->QPen   # minor X grid
#   grid().setMinPenY(QPen)  grid().minPenY()->QPen   # minor Y grid
#   grid().enableXMin(on)  grid().xMinEnabled()->bool
#   grid().enableYMin(on)  grid().yMinEnabled()->bool

# Graph — title/legend/text:
# setTitle(text)  setTitleFont(QFont)  setTitleColor(QColor)  setTitleAlignment(align)  removeTitle()
# newLegend()->LegendWidget  newLegend(text)->LegendWidget  setLegend(text)  legend()->LegendWidget|None
# NOTE: legend() is a GETTER — returns None if no legend exists yet; use newLegend() to CREATE one.
#   No addLegend() method exists. Confirmed live: legend() -> None -> AttributeError on .setText().
# NOTE: setLegend(text) is ALSO silent if no legend exists yet — it calls legend() internally and
#   just no-ops if that's None (verified in source: Graph::setLegend). No error, text goes nowhere.
#   Same "check/create first" pattern as above applies before calling setLegend() too.
# NOTE: newLegend() has NO dedup check — calling it again stacks a 2nd overlapping legend, silently.
#   Always: leg = g.legend(); leg = g.newLegend() if leg is None else leg  (reuse, don't blindly create)
# removeLegend()  addText(lw)->LegendWidget  addTimeStamp()->LegendWidget  add(fw,copy)->FrameWidget
# NOTE: addText(lw) CLONES lw into a new, separate LegendWidget and returns the CLONE — it does NOT
#   modify/frame lw itself. leg=g.newLegend(); g.addText(leg).setFrameStyle(...) styles the clone, not
#   leg (silently leaves 2 overlapping widgets). To style leg, call leg.setFrameStyle(...) directly.
# add(fw,copy): copy=True clones fw (same behavior as addText, for Legend/Image/Rectangle/Tex widgets)
#   and returns the clone; copy=False attaches fw ITSELF (no clone) and returns the same object —
#   the one way to move/reuse an existing, unattached widget without duplicating it.
# LegendWidget: setText(text)  setTextColor(QColor)  setFont(QFont)  setAngle(deg)  setAutoUpdate(bool)
#   (no setTextPen — use setTextColor(QColor))
# Rectangle(graphLayer) is a real, constructible FrameWidget subclass — a rectangle annotation, not
#   just Legend/Image. r = Rectangle(g); g.add(r, False)   # or add(r, True) to clone an existing one
# FrameWidget base (LegendWidget/ImageWidget/RectangleWidget ALL inherit these DIRECTLY — call them
#   on the legend/image object you already have, e.g. leg.setFrameStyle(...); do NOT build a
#   separate Rectangle just to frame an existing legend/image, that leaves two overlapping widgets
#   instead of one framed one. Confirmed live: asked for "a legend with a shadowed box frame",
#   produced a leg=newLegend()+text with NO frame styling, plus a separate empty Rectangle stacked
#   at the same coordinates for the shadow — leg.setFrameStyle(Frame.FrameStyle.Shadow.value) on
#   the legend itself was all that was needed):
#   setFrameStyle(int)  setFramePen(QPen)  setFrameColor(QColor)  setFrameWidth(float)
#   setBackgroundColor(QColor)  setBrush(QBrush)
#   setOrigin(x:int,y:int)       # PIXELS — setOrigin(0.02,0.02) truncates to (0,0); use e.g. setOrigin(10,10)
#   setOriginCoord(x:float,y:float)  # axis data coordinates — use axisScale(axis) (see above) to
#   compute e.g. "95% up the y-axis"; no range validation, so a wrong value places the widget off-plot
#   with no error.
#   setSize(w:int,h:int)
#   frame style: Frame.FrameStyle.Line(1)/.Shadow(2) — NOT PyQt6 QFrame; exact-match int, no bit-flags,
#   so QFrame.Shape.Box|QFrame.Shadow.Sunken silently draws NO frame at all (confirmed live)
#   MUST use .value: setFrameStyle(Frame.FrameStyle.Shadow.value) — the bare enum member raises
#   TypeError (FrameStyle has a None_ member -> real enum.Enum, not an int, unlike Axis/Ticks/Scale/
#   CurveType which have no None member and ARE plain ints, usable directly). Confirmed live.
# addArrow(am)->ArrowMarker  remove(marker)  arrowsList()->list  numArrows()->int
# addImage(iw|file|QImage)->ImageWidget  remove(iw)

# Graph — render/export:
# replot() is a GRAPH (layer) method — GraphWindow has NO replot()
# WRONG: gw.replot()  ← AttributeError
# RIGHT: gw.layer(1).replot(); gw.layer(2).replot()   — or — gw.activeLayer().replot()
# Same GraphWindow-vs-Graph trap applies to setXTitle()/setYTitle()/setAxisTitle()/setScale() —
#   all are Graph (layer) methods, all raise AttributeError on GraphWindow. Confirmed live.
# replot()  setAntialiasing(on,update)  setAutoscaleFonts(on)  enableAutoscaling(on)
# export("file.png")  exportImage(file,quality,transparent,dpi)
# exportVector(file,fontEmbed,dpi,color)  exportTex(file,color,escapeStrings,fontSizes)
