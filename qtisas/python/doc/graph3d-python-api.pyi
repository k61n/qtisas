# QtiSAS Graph3D
# Qti3D (PLOTSTYLE constants) is already GLOBAL — no import needed, use Qti3D.<CONSTANT> directly.
#   PyQt6/PyQt5 don't provide it (confirmed live: `from PyQt6.Qt3DExtras import Qti3D` raises
#   ImportError), nor is it a "qti.Qti3D" submodule. `from qti import Qti3D` happens to work
#   (qti is QtiSAS's real extension module) but is unnecessary — just use it bare.
# graphWindow("name") is 2D-GraphWindow-ONLY — returns None for a Graph3D window even when one
#   genuinely exists under that name. Use plot3D("name") below to look up an existing Graph3D
#   window instead — confirmed live, this mixup cost 8+ retries chasing dead ends after
#   setWindowName() already created the real window.
# Factory (all return Graph3D):
#   plot3D("name")                                           — look up existing window
#   newPlot3D() / newPlot3D("name")                          — new empty window
#   plot3D(table, "zCol", type=0)                            — XYZ from table column
#   plot3D(matrix, style=5)                                  — surface from Matrix (default FILLEDMESH)
#   plot3D("matrixName", style)                              — surface from Matrix, looked up by NAME
#   plot3D("f(x,y)", xl,xr, yl,yr, zl,zr, nx=40, ny=40)     — function surface
#   plot3D("X","Y","Z", ul,ur, vl,vr, nu=40, nv=40, uC=True, vC=True) — parametric surface
#
# plot3D(matrix, style=5) above: "matrix" is an actual Matrix OBJECT — fetch it FIRST via
#   matrix("name")/currentMatrix()/newMatrix(...). Passing the matrix's NAME string directly also
#   works — plot3D("matrixName", style) — but style has NO default there, so it always needs 2 args
#   (a bare 1-string-arg call always means the plot3D("name") window-lookup form instead):
#     g3 = plot3D("SphereMap", 5)      # matrix looked up by name — style required
#     m = matrix("SphereMap"); g3 = plot3D(m, 5)   # equivalent, object form
#
# UPSERT (create-or-update by NAME) — name is the FIRST argument, same trailing args as above:
#   newPlot3D(name, matrix, style=5)
#   newPlot3D(name, "matrixName", style)          — style required, same reason as plot3D above
#   newPlot3D(name, table, "zCol", type=0)
#   newPlot3D(name, "f(x,y)", xl,xr, yl,yr, zl,zr, nx=40, ny=40)
#   newPlot3D(name, "X","Y","Z", ul,ur, vl,vr, nu=40, nv=40, uC=True, vC=True)
# If a Graph3D window named `name` exists, its data is updated in place; otherwise a new window
# is created and named atomically. PREFER THIS over plot3D(...)+setWindowName(g3,name) — that
# two-step CAN raise ValueError: name already exists on a re-run, AFTER already creating an
# orphaned window; newPlot3D(name, ...) never creates anything before failing. Confirmed live:
# exactly this two-step pattern raised that ValueError when an earlier turn had already used the
# same window name.
#   g3 = newPlot3D("SinCos3D", "sin(x)*cos(y)", -pi, pi, -pi, pi, -1, 1, 60, 60)
#
# If `name` is already used by a NON-Graph3D window (Table/Matrix/Note/MultiLayer — names are
# unique across ALL window types, not just within one), newPlot3D(name, ...) raises
# ValueError: "newPlot3D: name '<name>' already exists but is not a Graph3D window" IMMEDIATELY,
# before creating anything — it does not create an orphan under an auto-generated name instead.
#
# ONE plot3D() call = ONE surface = ONE window — no multi-surface overlay.
# A second plot3D() call opens a new window.
# WHICH TO USE IS NOT FREE CHOICE: "create A 3d graph/plot with N shapes" (singular) means ONE
# shared view -> attempt the connected single-surface technique FIRST. Only use N separate
# windows if the request implies separate views, or the shapes don't share one revolution axis
# (e.g. arranged side by side in a plane). Confirmed failure: asked for "a 3d graph with 3
# spheres", produced 3 separate windows instead — valid Python, wrong deliverable, because the
# singular phrasing was never weighed against the easier separate-windows default.
# Separate/non-touching shapes -> one plot3D() call per shape (N shapes = N windows); do NOT
# call setParametricSurface()/setFunction() again on the same object expecting it to add a
# shape — it silently REPLACES, leaving only the last one.
# Shapes meant to connect into ONE surface -> single radius formula switching via sign() (see
# example below); for 3+ shapes use a 0/1 indicator weight per shape (sign() vs each neighbor,
# weights sum to 1) instead of the 2-term Ra+Rh*sign() form, which only has 2 slots.
# Spacing between chained shapes changes their RENDERED SIZE, silently, no error: tangent
# spacing (center distance = sum of radii) preserves true size; overlapping spacing shrinks
# both neighbors (mild if similar size, can shrink a much-smaller neighbor to invisible — seen
# at radius ratio 5:1:2). Default to tangent (sep = R1+R2) unless merging/blending was asked for.
# CRITICAL — the sign() switch THRESHOLD between neighbor i and i+1 is the tangent coordinate
# c[i]+R[i] (== c[i+1]-R[i+1]) — NOT the separation R[i]+R[i+1], and NOT c[i+1] itself. sep is
# correct for PLACING c[i+1] (c[i+1]=c[i]+sep) but WRONG if reused as the switch point, because
# c[i]+sep IS c[i+1] (the next shape's own CENTER, not its edge). Confirmed failure: threshold
# set to c[1]=3.0 instead of c[0]+R[0]=2.5 -> every shape past the first rendered as only its
# upper half (formula stays "valid" past the true edge up to the wrong threshold = NaN gap
# eating the lower half). Invisible with only 2 shapes; appears from the 3rd shape onward.
#
# FORMULA STRING SYNTAX (muParser — NOT Python):
#   Power: x^2  NOT x**2
#   Ternary ?: DOES work (verified in source — a core muParser feature) — prefer sign(threshold-v)
#   for piecewise switching (+1/-1) anyway for chained/composite shapes (see above), since it stays
#   a smooth multiplier rather than a hard branch, not because ternary is unavailable.
#   No if() — it's registered with no actual 3-arg handler (broken, not by design) and silently
#   never becomes callable. Use ternary or sign() instead.
#   Functions (verified against the actual registration, not exhaustive — stats/special functions
#   also exist): sin cos tan asin acos atan sinh cosh tanh asinh acosh atanh exp log ln log2 log10
#   sqrt abs sign rint floor ceil pow min max sum avg mod
#   NOTE: atan2 and round do NOT exist under those names (verified against the registration list) —
#   use rint for round-to-nearest; there is no 2-argument arctangent registered at all.
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
# QColor/QFont must be imported explicitly: from PyQt6.QtGui import QColor, QFont  (macOS/Windows)
# Use QColor("name") string form for Qt5/Qt6 compat (e.g. QColor("darkGreen")).
#
# View:
# setRotation(xDeg,yDeg,zDeg)  setScale(xF,yF,zF)  setShift(x,y,z)
# setZoom(factor)  setOrthogonal(on=True)
#
# Data:
# setData(table,"zCol",type=0)  setMatrix(matrix)  ← Matrix object ONLY, not Graph3D
# setFunction("f(x,y)",xl,xr,yl,yr,zl,zr,nx=40,ny=40)
# setParametricSurface("X","Y","Z",ul,ur,vl,vr,nu=40,nv=40,uC=True,vC=True)
# dataSource()->str        # formula/"X,Y,Z" parametric/"matrix<Name>"/table-col-assoc/"" if empty
# sourceMatrix()->Matrix|None
# update()
#
# setMatrix()/setFunction()/setParametricSurface() correctly switch an existing window between
# source types — dataSource()/sourceMatrix() always reflect the CURRENT source, never stale.
#
# Get-or-create/update BY NAME: prefer newPlot3D(name, ...) (see Factory section above) — it
# creates the first time and updates in place on every later call, no setWindowName() collision
# possible:
#   g3 = newPlot3D("SinCos3D", m, Qti3D.FILLEDMESH)
#
# To SKIP the update entirely when nothing changed (newPlot3D always re-applies the data), check
# dataSource()/sourceMatrix() yourself instead:
#   g3 = plot3D("SinCos3D")                # lookup; None if it doesn't exist yet
#   if g3 is None:
#       g3 = newPlot3D("SinCos3D", m, Qti3D.FILLEDMESH)
#   elif g3.sourceMatrix() is not m:       # different matrix, or a formula/table currently
#       g3.setMatrix(m)                    # update EXISTING window's data in place
#
# Axes:
# setXAxisLabel(s)  setYAxisLabel(s)  setZAxisLabel(s)
# setXAxisTickLength(maj,min)  setYAxisTickLength(maj,min)  setZAxisTickLength(maj,min)
# setScales(xl,xr,yl,yr,zl,zr,axis=-1)   # axis -1=all 0=X 1=Y 2=Z
# setScales() ONLY changes the displayed axis range — it does NOT rescale the surface. A size
# variable must be baked into the X/Y/Z formula strings themselves (f"{r}*cos(u)*cos(v)", not
# "cos(u)*cos(v)" with r only fed to setScales()/title) or the shape stays unscaled. Confirmed
# failure: 3 "different-radius" spheres all left as hardcoded unit spheres, only setScales()
# varied — bigger axis range around the same unit sphere makes it look SMALLER, so the visual
# size order came out inverted from what was intended.
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
