o, p = Dan.Options, Dan.Processing
o.newSession()
o.selectInstrument("KWS3-2020")
o.setInputPath("~/benchdata/dan/kws3")
s = p.autoConfig("auto3")
p.process(s)
merged = Dan.Merge.join(s + "-mergingTemplate")
t = table(s)
cn = list(t.colNames())
scale = [t.text(cn.index("Scale") + 1, r) for r in range(1, t.numRows() + 1)
         if t.text(cn.index("Run-info") + 1, r).strip() == "H-J" and t.text(cn.index("#-Condition") + 1, r) == "2"]
scriptPrint("R19:", len(merged), table(merged[0]).numRows(), "%.4f" % float(scale[0]))
