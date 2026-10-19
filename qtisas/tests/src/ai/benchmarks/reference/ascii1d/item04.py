a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/in")
a.setFormat("cansas")
t = table(a.importFiles("cansas.xml")[0])
scriptPrint("R4:", t.numRows(), sum(t.colData(2)), t.cell(3, 1))
