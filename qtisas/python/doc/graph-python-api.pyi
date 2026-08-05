# QtiSAS Graph
# gw=newGraph("name")|newGraph("name",layers,rows,cols)|graphWindow("name")|currentGraph()
# graphWindow("name") returns None (confirmed) if no graph with that name exists — ALWAYS
# check "if gw is None:" before use.
#    plot(table,("x","y"),Graph.CurveType.Line)|plot(matrix,Graph.CurveType.ColorMap)|plot("f(x)",xMin,xMax)
# GraphWindow=multi-layer window(MultiLayer); Graph=one panel.
#
# DEFAULTS: every new layer has user Preferences (fonts, tick style, backbone, canvas frame,
#   antialiasing, axis visibility) auto-applied. DO NOT re-set these unless explicitly required.
#   Only call setAxisFont/setTitleFont/setAxesLinewidth/drawAxesBackbones/... when overriding default.
#
# Qt CLASSES (QColor, QFont, QPen): use ONLY when no native QtiSAS method exists.
#   Must import explicitly: from PyQt6.QtGui import QColor, QFont, QPen, QBrush
#                           from PyQt6.QtCore import Qt
#                           from PyQt6.QtWidgets import QFrame
#   colorIndex("name")->int   use for curve colours  (NOT QColor)
#   WARNING: colorIndex("name") and QColor("name") for the SAME name are NOT the same colour.
#   colorIndex() uses QtiSAS's own palette ("green"=Qt::green, bright RGB(0,255,0)). QColor()
#   uses Qt's SVG/X11 named-colour table, where "green" is RGB(0,128,0) — dark, looks like the
#   palette's own "olive", not its "green". Confirmed: setCurveLineColor(0, QColor("green"))
#   renders as olive-looking, not bright green. Prefer colorIndex() for standard colour names.
#   symbolIndex("name")->int  use for curve symbols  (NOT QwtSymbol)
#   Use QColor only when a method explicitly requires it (e.g. addErrorBars color arg, setCanvasColor)
# Axis:      Graph.Axis.Left=0 Right=1 Bottom=2 Top=3
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
#   # X title: only on l2 (l1 bottom axis is disabled → setXTitle on l1 has no effect)
#   # ... setCanvasGeometry AGAIN with the SAME values — 2nd pass, corrects drift from curves/titles ...
#   g.keepAligned(True)          # lock LAST, only once nothing will disturb geometry again
#   g.linkXLayerAxes(True)
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
# changeCurveIndex(from,to)  reverseCurveOrder()
# setCurveStyle(index, Graph.CurveType)  — change curve type (Line/Spline/Scatter/…); call on layer, NOT on curve object
# setCurveLineColor: index 0-based; colorIndex int (0=black 1=red 2=green 3=blue 4=cyan ...)
# setCurveLineColor(index,colorIndex|QColor)  setCurveLineStyle(index,Qt.PenStyle.DashLine)  # PyQt6: Qt.PenStyle.X
# setCurveLineWidth(index,width)  setCurveAxes(index,xAxis,yAxis)
# showMissingDataGap(on,update)  isMissingDataGapEnabled()->bool  setGrayScale()  setIndexedColors()
# setWaterfallOffset(x,y,update)  setWaterfallSideLines(on)  setWaterfallFillColor(QColor)

# Graph — curve symbols  (NEVER use dataCurve(curve(...))+setSymbol or QwtSymbol — use Graph methods below):
# setCurveSymbolColor(index, colorIndex|QColor)       # fill / brush colour
# setCurveSymbolBorderColor(index, colorIndex|QColor) # border / pen colour
# setCurveSymbolBorderWidth(index, float)             # border / pen width in points
# setCurveSymbolOpacity(index, int)                   # fill opacity 0=transparent 255=opaque
# setCurveSymbolStyle(index, int)                     # shape; use symbolIndex("name") for lookup
# setCurveSymbolSize(index, int)                      # size = PlotDialog spinner value; pixel width = 2*size+1
# WRONG: layer.setMarkerSize(3)  — method does not exist; use setCurveSymbolSize(curveIndex, size)
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
# setLinOrLogAxis(axis,logYN,changeAxisFormat=True,forceRescale=False)  # single-axis log/lin toggle; forceRescale=True snaps to data extent even when current scale already contains data
# setLogLog()   # all 4 axes → log scale
# setLinLin()   # all 4 axes → linear scale
# setAutoScale()
# NOTE: setMargin() does NOT exist on Graph — method was removed in Qwt 6.x. No replacement.
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
# setAxisFont(axis,QFont)  setAxisLabelsColor(axis,QColor)  # tick label number colour; axis required
# to apply to all axes: for ax in [Graph.Axis.Bottom,Graph.Axis.Left,Graph.Axis.Top,Graph.Axis.Right]: layer.setAxisLabelsColor(ax,c); layer.setAxisTitleColor(ax,c)
# setAxisNumericFormat(axis,fmt,prec,suffix)   # fmt:0=auto 1=decimal 2=scientific 3=engineering
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
# newLegend()->LegendWidget  newLegend(text)->LegendWidget  setLegend(text)  legend()->LegendWidget
# removeLegend()  addText(lw)->LegendWidget  addTimeStamp()->LegendWidget  add(fw,copy)->FrameWidget
# LegendWidget: setText(text)  setTextColor(QColor)  setFont(QFont)  setAngle(deg)  setAutoUpdate(bool)
#   (no setTextPen — use setTextColor(QColor))
# FrameWidget base: setFrameStyle(int)  setFramePen(QPen)  setFrameColor(QColor)  setFrameWidth(float)
#   setBackgroundColor(QColor)  setBrush(QBrush)
#   setOrigin(x:int,y:int)       # PIXELS — setOrigin(0.02,0.02) truncates to (0,0); use e.g. setOrigin(10,10)
#   setOriginCoord(x:float,y:float)  # axis data coordinates
#   setSize(w:int,h:int)
#   PyQt6 frame style: QFrame.Shape.Box|QFrame.Shadow.Sunken  (NOT QFrame.Box|QFrame.Sunken)
# addArrow(am)->ArrowMarker  remove(marker)  arrowsList()->list  numArrows()->int
# addImage(iw|file|QImage)->ImageWidget  remove(iw)

# Graph — render/export:
# replot() is a GRAPH (layer) method — GraphWindow has NO replot()
# WRONG: gw.replot()  ← AttributeError
# RIGHT: gw.layer(1).replot(); gw.layer(2).replot()   — or — gw.activeLayer().replot()
# replot()  setAntialiasing(on,update)  setAutoscaleFonts(on)  enableAutoscaling(on)
# export("file.png")  exportImage(file,quality,transparent,dpi)
# exportVector(file,fontEmbed,dpi,color)  exportTex(file,color,escapeStrings,fontSizes)
