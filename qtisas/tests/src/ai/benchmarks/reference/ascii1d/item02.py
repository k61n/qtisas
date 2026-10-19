a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/in")
a.setHeader("5")
t = table(a.importFiles("header.dat", "HeaderData")[0])
scriptPrint("R2:", t.numRows(), t.cell(1, 1))
