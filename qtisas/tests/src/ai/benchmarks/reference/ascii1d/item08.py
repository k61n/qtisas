a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/in")
a.setRemovePoints(first=2, last=5)
t = table(a.importFiles("opt.dat", "optRemoved")[0])
scriptPrint("R8:", t.numRows(), t.cell(1, 1), t.cell(1, t.numRows()))
