# QtiSAS Graph
# gw=newGraph("name")|newGraph("name",numPanels,rows,cols)|graphWindow("name")|currentGraph()
#    plot(table,("col1","col2"),Graph.CurveType.Line)|plot(matrix,Graph.CurveType.ColorMap)|plot("formula",xMin,xMax)|waterfallPlot(t,cols)
# GraphWindow=multi-layer window(MultiLayer); Graph=one panel. SIP: required params positional-only; named optional params also accept keyword syntax.
# Axis (Graph.Axis): Graph.Axis.Left=0 Graph.Axis.Right=1 Graph.Axis.Bottom=2 Graph.Axis.Top=3
# Scale (Graph.Scale): Graph.Scale.Linear=0 Graph.Scale.Log10=1 Graph.Scale.Ln=2 Graph.Scale.Log2=3
#   Graph.Scale.Reciprocal=4 Graph.Scale.Probability=5 Graph.Scale.Logit=6
# Ticks (Graph.Ticks): Graph.Ticks.NoTicks=0 Graph.Ticks.Out=1 Graph.Ticks.InOut=2 Graph.Ticks.In=3
# CurveType (Graph.CurveType): Graph.CurveType.Line=0 Graph.CurveType.Scatter=1 Graph.CurveType.LineSymbols=2
#   Graph.CurveType.VerticalBars=3 Graph.CurveType.Area=4 Graph.CurveType.Pie=5
#   Graph.CurveType.VerticalDropLines=6 Graph.CurveType.Spline=7 Graph.CurveType.HorizontalSteps=8
#   Graph.CurveType.Histogram=9 Graph.CurveType.HorizontalBars=10
#   Graph.CurveType.VectXYXY=11 Graph.CurveType.ErrorBars=12 Graph.CurveType.Box=13
#   Graph.CurveType.VectXYAM=14 Graph.CurveType.VerticalSteps=15
#   Graph.CurveType.ColorMap=16 Graph.CurveType.GrayScale=17 Graph.CurveType.Contour=18 Graph.CurveType.Function=19

# GraphWindow:
numLayers()->int  activeLayer()->Graph  layer(n)->Graph  layersList()->list
addLayer(x,y,w,h,linked)->Graph  removeLayer(g)->bool  removeActiveLayer()->bool
setNumLayers(n)  setActiveLayer(g)  swapLayers(src,dest)  setEqualSizedLayers()
setCols(n)  setRows(n)  setSpacing(rowGap,colGap)  setMargins(l,r,t,b)
setLayerCanvasSize(w,h)  arrangeLayers(fit,userSize)  setScaleLayersOnResize(on)
setAlignement(hAlign,vAlign)  setAlignPolicy(policy)
setCommonLayerAxes(vertical,horizontal)  linkXLayerAxes(link)  reverseWaterfallOrder()
export("file.png")  exportImage(file,quality,transparent,dpi)
exportVector(file,fontEmbed,dpi,color)  exportTex(file,color,escapeStrings,fontSizes)

# Graph — curves add/remove:
addCurve(t,col,style,lw=1,sSize=5,r0=0,r1=-1)->bool
addCurves(t,cols_tuple,style,lw,sSize,r0,r1)->bool
insertCurve(t,col,style,r0,r1)->DataCurve  insertCurve(t,xcol,ycol,style,r0,r1)->DataCurve
removeCurve(index|title|curve)  deleteFitCurves()
numCurves()->int  curve(index|title)  curveTitle(index)->str
dataCurve(index)->DataCurve  boxCurve(index)->BoxCurve  functionCurve(index)->FunctionCurve
changeCurveIndex(from,to)  reverseCurveOrder()
setCurveLineColor(index,colorIndex|QColor)  setCurveLineStyle(index,Qt.PenStyle)
setCurveLineWidth(index,width)  setCurveAxes(index,xAxis,yAxis)
showMissingDataGap(on,update)  isMissingDataGapEnabled()->bool  setGrayScale()  setIndexedColors()
setWaterfallOffset(x,y,update)  setWaterfallSideLines(on)  setWaterfallFillColor(QColor)

# Graph — error bars:
addErrorBars(curveName|dataCurve,t,col,type=1,w,cap,color,through,minus,plus)  # type:0=X 1=Y

# Graph — special types:
plotBox(t,names_list,r0,r1)  plotPie(t,col,r0,r1)->PieCurve  isPiePlot()->bool  pieLegend()->str
plotSpectrogram(matrix,Graph.CurveType.*|int)->Spectrogram  spectrogram(matrix)->Spectrogram
addHistogram(matrix)->QwtHistogram  plotVectors(t,cols_list,style,r0,r1)->VectorCurve
addFunction(formula,xMin,xMax,points)->FunctionCurve
addParametricFunction(xExpr,yExpr,tMin,tMax,points,var)->FunctionCurve
addPolarFunction(rExpr,thetaExpr,tMin,tMax,points,var)->FunctionCurve

# Graph — scale/axes:
setScale(axis,start,end,step=0,majTicks=5,minTicks=5,type=0,inverted=False)  setAutoScale()  setMargin(px)
setXTitle(text)  setYTitle(text)  setAxisTitle(axis,text)
setAxisTitleFont(axis,QFont)  setAxisTitleColor(axis,QColor)  setAxisTitleAlignment(axis,align)
axisTitleDistance(axis)->int  setAxisTitleDistance(axis,dist)
enableAxis(axis,on)  enableAxisLabels(axis,on)  setAxisColor(axis,QColor)
setAxisFont(axis,QFont)  setAxisLabelsColor(axis,QColor)
setAxisNumericFormat(axis,fmt,prec,suffix)  # fmt:0=auto 1=decimal 2=scientific 3=engineering
setAxisLabelRotation(axis,degrees)
setMajorTicksType(axis,Graph.Ticks.*)  setMinorTicksType(axis,Graph.Ticks.*)
setAxisTicksLength(axis,majType,minType,minLen,majLen)  setTicksLength(min,maj)
setAxesLinewidth(width)  drawAxesBackbones(yes)

# Graph — canvas/frame/grid:
setCanvasColor(QColor)  setCanvasFrame(width,QColor)  setCanvasBackgroundImage(file,autocrop)
canvasBackgroundFileName()->str  backgroundPixmap()->QPixmap
setCanvasGeometry(x,y,w,h)  setCanvasSize(w,h)
setBackgroundColor(QColor)  setFrame(width,QColor)
grid()->Grid  showGrid()  showGrid(axis)  setGridOnTop(on,update)  hasGridOnTop()->bool

# Graph — title/legend/text:
setTitle(text)  setTitleFont(QFont)  setTitleColor(QColor)  setTitleAlignment(align)  removeTitle()
newLegend()->LegendWidget  newLegend(text)->LegendWidget  setLegend(text)  legend()->LegendWidget
removeLegend()  addText(lw)->LegendWidget  addTimeStamp()->LegendWidget  add(fw,copy)->FrameWidget
addArrow(am)->ArrowMarker  remove(marker)  arrowsList()->list  numArrows()->int
addImage(iw|file|QImage)->ImageWidget  remove(iw)

# Graph — render/export:
replot()  setAntialiasing(on,update)  setAutoscaleFonts(on)  enableAutoscaling(on)
setCanvasGeometry(QRect)  setCanvasSize(QSize)
export("file.png")  exportImage(file,quality,transparent,dpi)
exportVector(file,fontEmbed,dpi,color)  exportTex(file,color,escapeStrings,fontSizes)
