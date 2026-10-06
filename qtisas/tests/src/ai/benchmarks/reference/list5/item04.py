m = newMatrix("Saddle", 21, 21)
m.setCoordinates(-1, 1, -1, 1)
m.setFormula("x*x - y*y")
m.calculate()
g3 = plot3D(m, 5)
setWindowName(g3, "Saddle3D")
values = [m.cell(row, col) for row in range(1, 22) for col in range(1, 22)]
scriptPrint("R4: %g %g %g %s" % (m.cell(11, 11), min(values), max(values), plot3D("Saddle3D") is not None))
