a = Ascii1d
a.reset()
a.setResolution(True)
a.setResolutionGeometry(800, 400, 0.525, 10, 10, 30, 30, 6, 0.1)  # cm, cm, cm, mm, mm, mm, mm, Å, Δλ/λ
t = table(a.convertTables("QI-alpha", "QI-alpha-sigma")[0])
scriptPrint("R16:", t.numCols(), "%.6g" % t.cell(4, 1))
