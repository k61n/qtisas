a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/in")
a.setFormat("nist")
t = table(a.importFiles("nist.txt")[0])
scriptPrint("R1:", t.numRows(), max(t.colData(2)))
