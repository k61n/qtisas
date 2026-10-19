a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/in")
a.setMath("*", 2, "-", 0.01)
t = table(a.importFiles("opt.dat", "optMath")[0])
scriptPrint("R11:", "%.2f" % sum(t.colData(2)))
