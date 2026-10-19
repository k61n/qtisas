import os
a = Ascii1d
a.reset()
a.setOutputFolder("~/benchdata/out")
a.setExtension("RAD")
f = a.exportTables("plain")[0]
scriptPrint("R6:", os.path.basename(f), len([l for l in open(f) if l.strip()]))
