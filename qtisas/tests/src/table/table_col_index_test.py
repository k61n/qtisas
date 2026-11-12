"""A column is found by its label also when the label has "_" in it ("q_nm"), and by its full name "<table>_<label>".

Confirmed (benchmark LIST 6): Table::colIndex() compared only the part after the last "_", so insertCurve(t, "q_nm",
"I") failed with "X column not found" and LIST 6 had to name the column "Qnm".
"""

t = newTable("ColIndexT6", 3, 3)
t.setColNames(["q_nm", "I", "dI"])
t.setColData(1, [0.1, 0.2, 0.3])
t.setColData(2, [10.0, 5.0, 2.0])
t.setColData(3, [1.0, 0.5, 0.2])

g = newGraph("ColIndexG", 1, 1, 1).activeLayer()
for x in ("q_nm", "ColIndexT6_q_nm"):
    g.insertCurve(t, x, "I")  # raised "X column not found" for both
assert g.numCurves() == 2, g.numCurves()
g.addErrorBars(g.curveTitle(0), t, "dI")  # a label without "_" as before

# the full name of a label without "_" still works
curves = g.numCurves()
g.insertCurve(t, "ColIndexT6_I", "dI")
assert g.numCurves() == curves + 1, g.numCurves()

# colIndex() is 1-based like every column argument (it gave the C++ 0-based index: setColumnRole(t.colIndex("q_nm"), X)
# set the role of the column before it — LIST 6, Luna); the full name too; -1 for no such column
assert t.colIndex("q_nm") == 1, t.colIndex("q_nm")
assert t.colIndex("ColIndexT6_dI") == 3, t.colIndex("ColIndexT6_dI")
assert t.colIndex("nope") == -1, t.colIndex("nope")

closeWindow(graphWindow("ColIndexG"))
closeWindow(t)
