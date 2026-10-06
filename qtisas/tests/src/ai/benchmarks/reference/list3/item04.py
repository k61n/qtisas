t = table("Squares")
t.setReadOnlyColumn(2, True)
t.setCommand(2, "10*col(1)")
raised = False
try:
    t.recalculate(2)
except ValueError:
    raised = True
total = sum(t.colData(2))
t.setReadOnlyColumn(2, False)
scriptPrint("R4: %s %g" % (raised, total))
