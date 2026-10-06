t = table("Squares")
t.setCommand(2, "2**col(1)")
t.recalculate(2)
scriptPrint("R11: %g" % sum(t.colData(2)))
