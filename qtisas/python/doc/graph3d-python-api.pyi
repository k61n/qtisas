# QtiSAS Graph3D
# Factory (all return Graph3D):
#   plot3D("name")                                           — look up existing window
#   newPlot3D() / newPlot3D("name")                          — new empty window
#   plot3D(table, "zCol", type=0)                            — XYZ from table column
#   plot3D(matrix, style=5)                                  — surface from Matrix (default FILLEDMESH)
#   plot3D("f(x,y)", xl,xr, yl,yr, zl,zr, nx=40, ny=40)     — function surface
#   plot3D("X","Y","Z", ul,ur, vl,vr, nu=40, nv=40, uC=True, vC=True) — parametric surface
#
# Qti3D constants (PLOTSTYLE):
#   Qti3D.NOPLOT=0  WIREFRAME=1  HIDDENLINE=2  FILLED=3  FILLEDMESH=4  POINTS=5  USER=6(bars)
#
# Graph3D.PlotType:    Scatter=0  Trajectory=1  Bars=2  Ribbon=3
# Graph3D.AxisNumericFormat:  Default=0  Decimal=1  Scientific=2  Engineering=3
# Axis index: X=0  Y=1  Z=2
#
# QColor available in default namespace (qtisasrc.py). Use QColor("name") for Qt5/Qt6 compat.
#
# View:
# setRotation(xDeg,yDeg,zDeg)  setScale(xF,yF,zF)  setShift(x,y,z)
# setZoom(factor)  setOrthogonal(on=True)
#
# Data:
# setData(table,"zCol",type=0)  setMatrix(matrix)
# setFunction("f(x,y)",xl,xr,yl,yr,zl,zr,nx=40,ny=40)
# setParametricSurface("X","Y","Z",ul,ur,vl,vr,nu=40,nv=40,uC=True,vC=True)
# update()
#
# Axes:
# setXAxisLabel(s)  setYAxisLabel(s)  setZAxisLabel(s)
# setXAxisTickLength(maj,min)  setYAxisTickLength(maj,min)  setZAxisTickLength(maj,min)
# setScales(xl,xr,yl,yr,zl,zr,axis=-1)   # axis -1=all 0=X 1=Y 2=Z
# axisNumericFormat(axis)->int  axisNumericPrecision(axis)->int
# setAxisNumericFormat(axis,format,prec)
# setXAxisNumericFormat(format,prec)  setYAxisNumericFormat(format,prec)  setZAxisNumericFormat(format,prec)
#
# Frame: setFramed()  setBoxed()  setNoAxes()
#
# Grid faces (each on=True):
# setLeftGrid  setRightGrid  setCeilGrid  setFloorGrid  setFrontGrid  setBackGrid
#
# Floor:
# showFloorProjection()  showFloorIsolines()  setEmptyFloor()
#
# Plot style:
# setPlotStyle(Qti3D.*)  setWireframeStyle()  setHiddenLineStyle()  setPolygonStyle()
# setFilledMeshStyle()  setDotStyle()  setBarStyle()  setConeStyle()  setCrossStyle()
# setConeOptions(rad,quality)  setCrossOptions(rad,lw,smooth,boxed)
# setDotOptions(size,smooth)  setBarRadius(rad)  setBarLines(on=True)  setFilledBars(on=True)
# setMeshLineWidth(w)
#
# Colors (QColor):
# setMeshColor  setAxesColor  setNumbersColor  setLabelsColor
# setBackgroundColor  setGridColor
# setDataColors(minColor,maxColor)
# setDataColorMap(fileName)  setDataColorMap(LinearColorMap)
# colorMap()->LinearColorMap  colorMapFile()->str
# rescaleColorMap()   — reapply color map to current data range
#
# Display:
# setOpacity(0.0-1.0)  setResolution(res)  showLegend(on=True)
# setTitle(text)  setTitle(text,QColor,QFont)
# setAntialiasing(on=True)  setLabelsDistance(d)
# animate(on=True)  findBestLayout()
# setWindowTitle(title)   — title bar only (inherited from QMdiSubWindow)
# resize(w, h)            — window size in pixels (inherited from QMdiSubWindow)
# setWindowName(g3, name) — full rename: project tree + plot3D("name") lookup (app-level call)
#
# Curve access (single surface):
# curve(index)->Graph3D   — returns self; index ignored
#
# Export:
# export(fileName)  exportVector(fileName,...)  exportImage(fileName,quality=100,transparent=False,dpi=72,...)
