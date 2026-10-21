p = Dan.Processing
s = p.newScriptTable("script3", ["00082557", "00082559", "00082561", "00082646", "00082647", "00082648"])
p.calculateTransmissions(s)
t = table(s)
cn = list(t.colNames())
tr = {(t.text(cn.index("Run-info") + 1, r), t.text(cn.index("#-Condition") + 1, r)):
      t.text(cn.index("Transmission-Sample") + 1, r).split("[")[0].strip() for r in range(1, t.numRows() + 1)}
scriptPrint("R11:", tr[("H-L", "1")], tr[("H-L", "2")])
