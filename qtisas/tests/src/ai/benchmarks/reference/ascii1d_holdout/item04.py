a = Ascii1d
a.reset()
a.setBinning(ratio=1.2)
t = table(a.convertTables("QI-alpha", "QI-alpha-log")[0])
scriptPrint("R4:", t.numRows())
