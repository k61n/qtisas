s = Svd
s.reset()
s.setSolvent(-5.61e9, 6.335e10)
s.setComponents([("a", 1.48e10, 2.374e10), ("b", 2.11e10, 3.199e10)])
s.setContrasts([("mix0", 0), ("mix46", 0.46), ("mix80", 0.8), ("mix100", 1)])
t = table(s.decompose("svdAB4"))
scriptPrint("R11:", "%.4f" % t.cell(t.colNames().index("S12") + 1, 10), 4)
