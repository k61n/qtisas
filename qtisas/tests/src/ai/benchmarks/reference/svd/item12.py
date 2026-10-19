from PyQt6.QtGui import QColor

t = table("svdAB")
g = newGraph("gSVD").activeLayer()
for name in ("S11", "S12", "S22"):
    g.insertCurve(t, "Q1", name, Graph.CurveType.LineSymbols)
    g.addErrorBars("svdAB_" + name, t, "d" + name, 1, 1, 8, QColor("gray"), False, True, True)
scriptPrint("R12:", 3)
