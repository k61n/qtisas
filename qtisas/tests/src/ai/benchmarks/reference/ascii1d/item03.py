a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/in")
a.setHeader("***** Data *****")
t = table(a.importFiles("grasp.dat")[0])
scriptPrint("R3:", t.numRows(), t.cell(1, 1), t.cell(1, t.numRows()))
