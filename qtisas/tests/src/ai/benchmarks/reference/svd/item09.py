s = Svd
s.reset()
s.setSolvent(-5.61e9, 6.335e10)
s.setComponents([("a", 1.48e10, 2.374e10), ("b", 2.11e10, 3.199e10)])
t = table("svdAB")
at = lambda label, row: t.cell(t.colNames().index(label) + 1, row)  # row 10: Q = 0.05, row 20: Q = 0.1
partials = [at("S11", 10), at("S12", 10), at("S22", 10)]
scriptPrint("R9:", "%.3f" % s.plan([0, 0.5, 1], partials, 0.01, 0.01)[1])
