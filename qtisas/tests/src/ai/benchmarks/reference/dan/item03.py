Dan.Mask.newMask()
Dan.Sensitivity.loadSensitivity("~/benchdata/dan/kws1/sens-20200117.sens")
m = matrix("mask")
s = matrix("sens")
ones = sum(m.cell(r, c) for r in range(1, m.numRows() + 1) for c in range(1, m.numCols() + 1))
values = [s.cell(r, c) for r in range(1, s.numRows() + 1) for c in range(1, s.numCols() + 1) if m.cell(r, c) > 0]
scriptPrint("R3:", int(ones), "%.3f" % (sum(values) / len(values)))
