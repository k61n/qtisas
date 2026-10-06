m = newMatrix("Grid", 3, 4)
m.setFormula("i+10*j")
m.calculate()
scriptPrint("R3: %g %g" % (m.cell(3, 4), sum(m.cell(3, col) for col in range(1, 5))))  # cell(row, col)
