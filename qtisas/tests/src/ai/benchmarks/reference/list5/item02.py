t = newTable("Peak5", 81, 2)
t.setColNames(["x", "y"])
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(2, Table.PlotDesignation.Y)
xs = [-4 + 0.1*i for i in range(81)]
t.setColData(1, xs)
t.setColData(2, [2/(1 + (x/0.5)**2) + 0.1 for x in xs])
c = Compiler
for name, code in (("ai/Gauss5", "y = A*exp(-x*x/(2*w*w)) + c;"), ("ai/Lor5", "y = A/(1 + (x/w)*(x/w)) + c;")):
    c.new()
    c.setFunctionName(name)
    c.setGroup("reference")
    c.setXName("x")
    c.setYName("y")
    c.setParamCount(3)
    c.setParamNamesAll(["A", "w", "c"])
    c.setParamInitsAll([1.0, 1.0, 0.0])
    c.setParamVariesAll([True, True, True])
    c.setCode(code)
    c.compile()
f = Fittable
fits = {}
for name in ("ai/Gauss5", "ai/Lor5"):
    f.configure(name, ["Peak5_y"])
    for p, v in (("A", 1.0), ("w", 1.0), ("c", 0.0)):
        f.setParamValue(p, v)
    chi2_dof = f.fit()
    r = f.results()
    fits[name] = (chi2_dof, r["A"][0], abs(r["w"][0]))
best = min(fits, key=lambda n: fits[n][0])
scriptPrint("R2: %s %.4f %.4f" % (best, fits[best][1], fits[best][2]))
