info = Dan.Rawdata.newHeaderTable("kws1info", ["Sample", "D", "lambda"])
t = table(info)
names = list(t.colNames())
runs = Dan.Rawdata.runs()
r = runs.index("53226") + 1
scriptPrint("R2:", t.numRows(), "%.2f" % t.cell(names.index("D") + 1, r))
