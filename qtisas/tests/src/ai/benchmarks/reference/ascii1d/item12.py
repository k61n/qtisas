a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/in")
a.setBinning(3)
t = table(a.importFiles("opt.dat", "optBinned")[0])
scriptPrint("R12:", t.numRows(), t.cell(1, 1), t.cell(2, 1))
