p = Dan.Processing
p.process("auto1")
merged = Dan.Merge.join("auto1-mergingTemplate")
scriptPrint("R18:", len(merged), *[table(m).numRows() for m in merged])
