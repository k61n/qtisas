# QtiSAS Graph3D
# Factory (all return Graph3D):
#   plot3D("name")                                           — look up existing window
#   newPlot3D() / newPlot3D("name")                          — new empty window
#   plot3D(table, "zCol", type=0)                            — XYZ from table column
#   plot3D(matrix, style=5)                                  — surface from Matrix (default FILLEDMESH)
#   plot3D("f(x,y)", xl,xr, yl,yr, zl,zr, nx=40, ny=40)     — function surface
#   plot3D("X","Y","Z", ul,ur, vl,vr, nu=40, nv=40, uC=True, vC=True) — parametric surface
#
# ONE plot3D() call = ONE surface = ONE window — no multi-surface overlay.
# A second plot3D() call opens a new window.
#
# FORMULA STRING SYNTAX (muParser — NOT Python):
#   Power: x^2  NOT x**2
#   No ternary ?:  No if() — use sign(threshold-v) for piecewise switching (+1/-1)
#   Functions: sin cos tan asin acos atan atan2 exp log log2 log10 sqrt abs sign floor ceil round
#   Embed numeric constants via f-strings; build shared sub-expressions as Python strings:
#     r = "sqrt(4.0 - v^2)"
#     g3 = plot3D(f"({r})*cos(u)", f"({r})*sin(u)", "v", 0, 2*pi, -2, 2, 60, 60, False, True)
#
# uClosed=True closes the azimuthal seam; vClosed=False when surface tapers to r=0 at both ends.
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
# setData(table,"zCol",type=0)  setMatrix(matrix)  ← Matrix object ONLY, not Graph3D
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
# setDataColors(minColor,maxColor)   — set two-color gradient AS the active map
# setDataColorMap(fileName)  setDataColorMap(LinearColorMap)
# colorMap()->LinearColorMap  colorMapFile()->str
# rescaleColorMap()   — reapply SYSTEM color map to current data range
# NOTE: setDataColors and rescaleColorMap() are mutually exclusive —
#       rescaleColorMap() discards any setDataColors() gradient
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
