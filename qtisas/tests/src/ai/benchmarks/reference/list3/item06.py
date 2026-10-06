m = newMatrix("Prod", 4, 4)
m.setFormula("i*j")
m.calculate()
total = sum(m.cell(row, col) for row in range(1, 5) for col in range(1, 5))  # not integrate(): that is the integral
scriptPrint("R6: %g %g %.6f" % (total, m.cell(2, 3), abs(m.determinant())))
