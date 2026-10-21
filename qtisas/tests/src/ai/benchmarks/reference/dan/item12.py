p = Dan.Processing
t = table("script3")
cn = list(t.colNames())
tr = lambda r: float(t.text(cn.index("Transmission-Sample") + 1, r).split("[")[0])
sample = lambda r: t.text(cn.index("Run-info") + 1, r).strip()
cond = lambda r: t.text(cn.index("#-Condition") + 1, r)
tr9 = {sample(r): tr(r) for r in range(1, t.numRows() + 1) if cond(r) == "1"}
t.addColumns(1)
t.setColName(t.numCols(), "Scale")
for r in range(1, t.numRows() + 1):
    t.setText(t.numCols(), r, "1" if cond(r) == "1" else repr(tr(r) / tr9[sample(r)]))
p.Options.setRemovePoints(0, 5)
p.run("script3")
Dan.Merge.run("script3-mergingTemplate")
m = table("H-M")
scriptPrint("R12:", m.numRows(), "%.0f" % m.cell(2, 1), "%.3f" % m.cell(2, m.numRows()))
