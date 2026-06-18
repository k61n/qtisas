# QtiSAS Graph
# gw=newGraph("name")|newGraph("name",layers,rows,cols)|graphWindow("name")|currentGraph()
#    plot(table,("x","y"),Graph.CurveType.Line)|plot(matrix,Graph.CurveType.ColorMap)|plot("f(x)",xMin,xMax)
# GraphWindow=multi-layer window(MultiLayer); Graph=one panel.
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
# setCols(n)  setRows(n)  setSpacing(rowGap,colGap)  setMargins(l,r,t,b)
# setLayerCanvasSize(w,h)  arrangeLayers(fit,userSize)  setScaleLayersOnResize(on)
# setAlignement(hAlign,vAlign)  setAlignPolicy(policy)
# keepAligned(on=True)  isKeepAligned()->bool  # pin canvas rect (pos+size); re-applied on every layer update
# setCommonLayerAxes(vertical,horizontal)  linkXLayerAxes(link)  reverseWaterfallOrder()
# export("file.png")  exportImage(file,quality,transparent,dpi)
# exportVector(file,fontEmbed,dpi,color)  exportTex(file,color,escapeStrings,fontSizes)

# Graph — add curves:
# ONE Y column (str), X auto-detected: g.addCurve(t, "y", Graph.CurveType.Line)
# TUPLE of Y columns:                  g.addCurves(t, ("y1","y2"), Graph.CurveType.Scatter)
# addCurve(t,col,style=0,lineWidth=1,pointSize=3,startRow=0,endRow=-1)->bool
# addCurves(t,cols_tuple,style=0,lineWidth=1,pointSize=3,startRow=0,endRow=-1)->bool
# insertCurve(t,col,style,startRow=0,endRow=-1)->DataCurve
# insertCurve(t,xcol,ycol,style,startRow=0,endRow=-1)->DataCurve
# removeCurve(index|title|curve)  deleteFitCurves()
# numCurves()->int  curve(index|title)  curveTitle(index)->str
# dataCurve(index)->DataCurve  boxCurve(index)->BoxCurve  functionCurve(index)->FunctionCurve
# changeCurveIndex(from,to)  reverseCurveOrder()
# setCurveStyle(index, Graph.CurveType)  — change curve type (Line/Spline/Scatter/…); call on layer, NOT on curve object
# setCurveLineColor: index 0-based; colorIndex int (0=black 1=red 2=green 3=blue 4=cyan ...)
# setCurveLineColor(index,colorIndex|QColor)  setCurveLineStyle(index,Qt.PenStyle)
# setCurveLineWidth(index,width)  setCurveAxes(index,xAxis,yAxis)
# showMissingDataGap(on,update)  isMissingDataGapEnabled()->bool  setGrayScale()  setIndexedColors()
# setWaterfallOffset(x,y,update)  setWaterfallSideLines(on)  setWaterfallFillColor(QColor)

# Graph — curve symbols  (NEVER import from PyQt6.Qwt — use Graph methods below):
# setCurveSymbolColor(index, colorIndex|QColor)       # fill / brush colour
# setCurveSymbolBorderColor(index, colorIndex|QColor) # border / pen colour
# setCurveSymbolBorderWidth(index, float)             # border / pen width in points
# setCurveSymbolOpacity(index, int)                   # fill opacity 0=transparent 255=opaque
# setCurveSymbolStyle(index, int)                     # shape; use symbolIndex("name") for lookup
# setCurveSymbolSize(index, int)                      # size = PlotDialog spinner value; pixel width = 2*size+1
# Usage: layer.setCurveSymbolColor(0, colorIndex("red"))
#        layer.setCurveSymbolStyle(0, symbolIndex("diamond"))
# pointSize at curve creation: addCurve(t, col, Graph.CurveType.LineSymbols, lineWidth, pointSize)
# Note: addCurve pointSize is raw pixel width; setCurveSymbolSize uses UI convention (2*n+1 pixels)

# Graph — error bars:
# addErrorBars(curveName|dataCurve,t,col,type=1,w=1,cap=8,color=Qt.black,through=True,minus=True,plus=True)
# type: 0=X 1=Y

# Graph — special types:
# plotBox(t,names_list,startRow=0,endRow=-1)
# plotPie(t,col,startRow=0,endRow=-1)->PieCurve  isPiePlot()->bool  pieLegend()->str
# plotSpectrogram(matrix,Graph.CurveType.ColorMap)->Spectrogram  spectrogram(matrix)->Spectrogram
# addHistogram(matrix)->QwtHistogram  plotVectors(t,cols_list,style,startRow=0,endRow=-1)->VectorCurve
# addFunction(formula,xMin,xMax,points)->FunctionCurve
# addParametricFunction(xExpr,yExpr,tMin,tMax,points,var)->FunctionCurve
# addPolarFunction(rExpr,thetaExpr,tMin,tMax,points,var)->FunctionCurve

# Graph — scale/axes:
# e.g.: g.setScale(Graph.Axis.Bottom, 0.01, 1.0, 0, 5, 5, Graph.Scale.Log10)
# setScale(axis,start,end,step=0,majorTicks=5,minorTicks=5,type=0,inverted=False)
# setLinOrLogAxis(axis,logYN,changeAxisFormat=True)   # single-axis log/lin toggle
# setLogLog()   # all 4 axes → log scale
# setLinLin()   # all 4 axes → linear scale
# setAutoScale()  setMargin(px)
# setXTitle(text)  setYTitle(text)  setAxisTitle(axis,text)
# setAxisTitleFont(axis,QFont)  setAxisTitleColor(axis,QColor)  setAxisTitleAlignment(axis,align)
# axisTitleDistance(axis)->int  setAxisTitleDistance(axis,dist)
# enableAxis(axis,on)  enableAxisLabels(axis,on)  setAxisColor(axis,QColor)
# setAxisFont(axis,QFont)  setAxisLabelsColor(axis,QColor)
# setAxisNumericFormat(axis,fmt,prec,suffix)   # fmt:0=auto 1=decimal 2=scientific 3=engineering
# setAxisLabelRotation(axis,degrees)
# setMajorTicksType(axis,Graph.Ticks.Out)  setMinorTicksType(axis,Graph.Ticks.Out)
# setAxisTicksLength(axis,majType,minType,minLength,majLength)  setTicksLength(minLength,majLength)
# setAxesLinewidth(width)  drawAxesBackbones(yes)

# Graph — canvas/frame/grid:
# setCanvasColor(QColor)  setCanvasFrame(width,QColor)  setCanvasBackgroundImage(file,autocrop)
# setCanvasGeometry(x,y,w,h)  setCanvasSize(w,h)
# setBackgroundColor(QColor)  setFrame(width,QColor)
# grid()->Grid  showGrid()  showGrid(axis)  setGridOnTop(on,update)  hasGridOnTop()->bool

# Graph — title/legend/text:
# setTitle(text)  setTitleFont(QFont)  setTitleColor(QColor)  setTitleAlignment(align)  removeTitle()
# newLegend()->LegendWidget  newLegend(text)->LegendWidget  setLegend(text)  legend()->LegendWidget
# removeLegend()  addText(lw)->LegendWidget  addTimeStamp()->LegendWidget  add(fw,copy)->FrameWidget
# addArrow(am)->ArrowMarker  remove(marker)  arrowsList()->list  numArrows()->int
# addImage(iw|file|QImage)->ImageWidget  remove(iw)

# Graph — render/export:
# replot()  setAntialiasing(on,update)  setAutoscaleFonts(on)  enableAutoscaling(on)
# export("file.png")  exportImage(file,quality,transparent,dpi)
# exportVector(file,fontEmbed,dpi,color)  exportTex(file,color,escapeStrings,fontSizes)
