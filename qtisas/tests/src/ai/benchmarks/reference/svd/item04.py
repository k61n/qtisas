t = table("svdAB")
at = lambda label, row: t.cell(t.colNames().index(label) + 1, row)  # row 10: Q = 0.05, row 20: Q = 0.1
scriptPrint("R4:", "%.4f" % at("dS11", 10))
