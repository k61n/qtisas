a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/in")
t = table(a.importFiles("plain.dat")[0])
scriptPrint("R1:", t.objectName(), t.numRows(), t.cell(1, 1), sum(t.colData(2)))
