from PyQt6.QtGui import QColor

g = newGraph("gNSE").activeLayer()
sets = Jnse.datasets("report90001_1-2-info")[:3]  # rows sorted by q, smallest first
for name in sets:
    g.insertCurve(table(name), "tau-ns", "Sqt-To-Sq", Graph.CurveType.LineSymbols)
    g.addErrorBars(name + "_Sqt-To-Sq", table(name), "d-Sqt-To-Sq", 1, 1, 8, QColor("gray"), False, True, True)
scriptPrint("R9:", len(sets))
