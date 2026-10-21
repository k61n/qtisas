p = Dan.Processing
s = p.newScriptTable("script", [53217, 53218, 53219, 53221, 53222, 53223, 53226, 53227, 53228])
p.calculateTransmissions(s)
t = table(s)
cn = list(t.colNames())
row = [t.text(cn.index("#-Run") + 1, r) for r in range(1, t.numRows() + 1)].index("53222") + 1
scriptPrint("R5:", t.numRows(), t.text(cn.index("Transmission-Sample") + 1, row).split("[")[0].strip())
