a = Ascii1d
a.reset()
a.setOutputFolder("~/benchdata/out")
a.setExtension("dat")
f = a.exportTables("QI-beta")[0]
scriptPrint("R2:", len([l for l in open(f) if l.strip()]))
