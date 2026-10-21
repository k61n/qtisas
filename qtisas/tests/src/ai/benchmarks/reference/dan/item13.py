q1, q2, n, ratio = Dan.Merge.overlap("H-J", "KWS1-H-J")
joined = Dan.Merge.join("H-J-both", ["H-J", "KWS1-H-J"])
scriptPrint("R13:", n, "%.2f" % ratio, table(joined).numRows())
