s = Svd
s.reset()
s.setSolvent(-5.61e9, 6.335e10)
s.setComponents([("p", 2.0e10, 2.9e10), ("n", 3.6e10, 4.2e10)])
tables = ["pn0", "pn20", "pn60", "pn80", "pn90", "pn100"]
s.setContrasts(list(zip(tables, [0, 0.2, 0.6, 0.8, 0.9, 1])))
t = table(s.decompose("svdPN"))
scriptPrint("R1:", len(tables), "%.4f" % t.cell(t.colNames().index("S12") + 1, 10))  # row 10: Q = 0.06
