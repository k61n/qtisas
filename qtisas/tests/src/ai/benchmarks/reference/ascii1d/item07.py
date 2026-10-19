a = Ascii1d
a.reset()
a.setOutputFolder("~/benchdata/out")
a.setColumns(3)
f = a.exportTables("QI-alpha", "alpha3.dat")[0]
scriptPrint("R7:", len(open(f).readline().split()))
