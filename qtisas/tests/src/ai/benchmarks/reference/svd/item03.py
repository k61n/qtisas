t = table("svdAB")
at = lambda label, row: t.cell(t.colNames().index(label) + 1, row)  # row 10: Q = 0.05, row 20: Q = 0.1
scriptPrint("R3:", "%.4f" % (at("Icon3", 10) / table("mix46").cell(2, 10)))
