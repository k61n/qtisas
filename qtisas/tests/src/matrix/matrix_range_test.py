"""Matrix.min() and Matrix.max(): the smallest and the largest cell value; existPlot3D() for a 3D plot."""

m = newMatrix("MatrixRange", 21, 21)
m.setCoordinates(-1.0, 1.0, -1.0, 1.0)
m.setFormula("x*x - y*y")
m.calculate()
assert abs(m.cell(11, 11)) < 1e-12, m.cell(11, 11)
assert abs(m.min() + 1.0) < 1e-12, m.min()
assert abs(m.max() - 1.0) < 1e-12, m.max()

m.setCell(5, 5, -7.5)
m.setCell(6, 6, 12.25)
assert m.min() == -7.5 and m.max() == 12.25, (m.min(), m.max())

# existPlot3D(): the 3D counterpart of existGraph(), which knows 2D graph windows only
g3 = newPlot3D("MatrixRange3D", m)
assert existPlot3D("MatrixRange3D") and not existGraph("MatrixRange3D")
assert not existPlot3D("NoSuchPlot3D")
closeWindow(g3)
