c = Compiler
names = [n for n in c.functionNames("ALL") if n.startswith("sasviewmodels/")]
scriptPrint("compiled sasviewmodels functions: %d" % len(names))
name = "sasviewmodels/sphere" if "sasviewmodels/sphere" in names else names[0]
f = Fittable
f.configure(name, [])
f.selectStage("simulate")
f.setSimUniformX(True)
f.setSimFromX(0.001)
f.setSimToX(0.5)
f.setSimNumPoints(100)
f.simulateCurve()
scriptPrint("R3: %s %s" % (name in names, existTable("simulatedCurve-" + name)))
