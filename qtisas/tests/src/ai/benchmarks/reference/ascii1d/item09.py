a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/in")
a.setAutoPrefix(True)
a.setPresentation("Kratky")
t = table(a.importFiles("opt.dat")[0])
scriptPrint("R9:", t.objectName(), round(t.cell(2, 3), 6))
