a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/svd")
names = a.importFiles(["run0.dat", "run34.dat", "run46.dat", "run80.dat", "run100.dat"])
s = Svd
s.reset()
s.setSolvent(-5.61e9, 6.335e10)
s.setComponents([("a", 1.48e10, 2.374e10), ("b", 2.11e10, 3.199e10)])
s.setContrasts(list(zip(names, [0, 0.34, 0.46, 0.8, 1])))
t = table(s.decompose("svdFiles"))
scriptPrint("R13:", "%.4f" % t.cell(t.colNames().index("S22") + 1, 10))
