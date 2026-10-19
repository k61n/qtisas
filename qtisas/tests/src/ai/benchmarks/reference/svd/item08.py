s = Svd
s.reset()
s.setSolvent(-5.61e9, 6.335e10)
s.setComponents([("a", 1.48e10, 2.374e10), ("b", 2.11e10, 3.199e10)])
t = table("svdAB")
at = lambda label, row: t.cell(t.colNames().index(label) + 1, row)  # row 10: Q = 0.05, row 20: Q = 0.1
partials = [at("S11", 10), at("S12", 10), at("S22", 10)]
plans = {"A": [0, 0.25, 0.5, 0.75, 1], "B": [0, 0.34, 0.46, 0.8, 1], "C": [0.2, 0.3, 0.4, 0.5, 0.6]}
best = lambda background: min(plans, key=lambda p: s.plan(plans[p], partials, 0.01, background)[1])  # [1]: S12
scriptPrint("R8:", best(0.01), best(0))
