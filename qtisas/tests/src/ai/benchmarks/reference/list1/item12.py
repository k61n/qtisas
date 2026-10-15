import random
from PyQt6.QtGui import QFont
xs = [0.025 * i for i in range(200)]
clean = [exp(-(x - 2.5)**2 / (2 * 0.5**2)) + 0.1 for x in xs]
t = newTable("GaussPeak", 200, 3)
t.setColNames(["x", "y", "dy"])
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(2, Table.PlotDesignation.Y)
t.setColumnRole(3, Table.PlotDesignation.yErr)
t.setColData(1, xs)
t.setColData(2, [v + random.gauss(0, 0.05 * v) for v in clean])
t.setColData(3, [0.05 * v for v in clean])
c = Compiler
for name, code in (("ai/GPeak", "y = A*exp(-(x-xc)*(x-xc)/(2.0*w*w)) + c;"), ("ai/LPeak", "y = A/(1.0+((x-xc)/w)*((x-xc)/w)) + c;")):
    c.new()
    c.setFunctionName(name)
    c.setGroup("reference")
    c.setXName("x")
    c.setYName("y")
    c.setParamCount(4)
    c.setParamNamesAll(["A", "xc", "w", "c"])
    c.setParamInitsAll([1.0, 2.5, 0.5, 0.0])
    c.setParamVariesAll([True, True, True, True])
    c.setCode(code)
    c.compile()
f = Fittable
chi2 = {}
for name in ("ai/GPeak", "ai/LPeak"):
    f.configure(name, ["GaussPeak_y"])
    f.setWeightingEnabled(1, True)
    f.setWeightingDataset(1, "GaussPeak_dy")
    chi2[name] = f.fit()
best = min(chi2, key=chi2.get)
scriptPrint("chi2/dof: %s, better: %s" % (chi2, best))
f.configure(best, ["GaussPeak_y"])
f.setWeightingEnabled(1, True)
f.setWeightingDataset(1, "GaussPeak_dy")
f.fit()
gw = newGraph("GaussPeakFit", 2, 2, 1)
f.plotResults("GaussPeakFit")
top, bottom = gw.layer(1), gw.layer(2)
leg = top.legend() or top.newLegend()
leg.setFont(QFont("Arial", 10))
top.setYTitle("y")
bottom.setXTitle("x")
bottom.setYTitle("residuals")
scriptPrint("R12: %s %d %d" % (best, top.numCurves(), bottom.numCurves()))
