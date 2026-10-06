c = Compiler
c.new()
c.setFunctionName("ai/RefGauss")
c.setGroup("reference")
c.setXName("x")
c.setYName("y")
c.setParamCount(4)
c.setParamNamesAll(["A", "x0", "s", "c"])
c.setParamInitsAll([1.0, 0.5, 1.0, 0.0])
c.setParamVariesAll([True, True, True, True])
c.setCode("y = A*exp(-(x - x0)*(x - x0)/(2*s*s)) + c;")  # C++: no ^ for a power
c.compile()
t = newTable("Peak", 41, 2)
t.setColNames(["x", "y"])
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(2, Table.PlotDesignation.Y)
xs = [-2 + 0.1*i for i in range(41)]
t.setColData(1, xs)
t.setColData(2, [5*exp(-x*x/0.18) + 1 for x in xs])
f = Fittable
f.configure("ai/RefGauss", ["Peak_y"])
for name, value in (("A", 1.0), ("x0", 0.5), ("s", 1.0), ("c", 0.0)):
    f.setParamValue(name, value)
f.fit()
r = f.results()
scriptPrint("R2: %.4f %.4f %.4f %.4f" % (r["A"][0], r["x0"][0], abs(r["s"][0]), r["c"][0]))
