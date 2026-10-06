t = table("Squares")
t.setCommand(3, "col(1)^3")
t.recalculate(3, forceMuParser=True)
scriptPrint("R2: %g" % sum(t.colData(3)))
