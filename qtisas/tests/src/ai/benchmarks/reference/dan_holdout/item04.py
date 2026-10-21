p = Dan.Processing
s = p.newScriptTable("sans1", ["497839", "497842"])
t = table(s)
cn = list(t.colNames())
for r in range(1, t.numRows() + 1):
    t.setText(cn.index("Thickness") + 1, r, "0.01")
    t.setText(cn.index("Transmission-Sample") + 1, r, "0.9115" if t.text(cn.index("Run-info") + 1, r) == "HC" else "0.9102")
p.run(s)
q = table("QI-SM-497842-PC")
scriptPrint("R4:", q.numRows(), "%.4f" % q.cell(1, 20), "%.4f" % q.cell(2, 20))
