merged = Dan.Merge.run("script-mergingTemplate")
t = table("H-L")
scriptPrint("R7:", len(merged), t.numRows(), "%.4f" % t.cell(1, 1), "%.1f" % t.cell(2, 1))
