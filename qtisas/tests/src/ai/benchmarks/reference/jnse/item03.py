j = Jnse
header = "report90001_1-2-info"
row = min(range(6), key=lambda k: abs(j.header(header, "q")[k] - 0.0705))
t = table(j.datasets(header)[row])
scriptPrint("R3:", "%.4f" % t.cell(t.colNames().index("Sqt-To-Sq") + 1, 1))
