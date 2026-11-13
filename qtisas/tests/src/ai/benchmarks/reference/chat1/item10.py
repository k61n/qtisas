info = table("Info")
role = {info.text(2, r): info.text(1, r) for r in range(1, info.numRows() + 1)}  # sample -> table name
s = table(role["lysozyme 5 mg/ml"])
b = table(role["buffer"])
q = s.colData(1)
i_sub = [a - c for a, c in zip(s.colData(2), b.colData(2))]
d_sub = [sqrt(a * a + c * c) for a, c in zip(s.colData(3), b.colData(3))]
t = newTable("Lyso-sub", len(q), 3)
t.setColNames(["q", "I", "dI"])
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(2, Table.PlotDesignation.Y)
t.setColumnRole(3, Table.PlotDesignation.yErr)
t.setColData(1, q)
t.setColData(2, i_sub)
t.setColData(3, d_sub)
print(t.numRows(), "%.4f" % t.cell(2, 1))
