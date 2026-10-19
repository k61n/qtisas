t = table("svdAB")
at = lambda label, row: t.cell(t.colNames().index(label) + 1, row)  # row 10: Q = 0.05, row 20: Q = 0.1
scriptPrint("R2:", "%.4f" % at("S13", 20), "%.4f" % at("S33", 20))
