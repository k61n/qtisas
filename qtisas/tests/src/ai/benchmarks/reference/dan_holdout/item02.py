Dan.Sensitivity.newSensitivity("497846", emptyBeam="497844", darkCurrent="497847", transmission=0.5191, mask="mask-4m")
m, s = matrix("mask-4m"), matrix("sens")
values = [s.cell(r, c) for r in range(1, s.numRows() + 1) for c in range(1, s.numCols() + 1) if m.cell(r, c) > 0]
scriptPrint("R2:", "%.4f" % (sum(values) / len(values)), "%.4f" % min(values), "%.4f" % max(values))
