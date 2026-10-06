t = table("Squares")
t.setCommand(4, "1/(col(1)-5)")
error = ""
try:
    t.recalculate(4)
except ValueError as e:
    error = str(e).splitlines()[1].split(":")[0]  # the second line is the formula's first error
empty = sum(t.cellOrNone(4, row) is None for row in range(1, t.numRows() + 1))
scriptPrint("R3: %d %s" % (empty, error))
