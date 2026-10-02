"""Graph.addCurve() / addCurves(): a column the table does not have raises ValueError (naming the table's columns),
instead of adding nothing and reporting nothing — as insertCurve() already did.
"""

t = newTable("AddCurveData", 3, 2)
t.setColData(1, [1.0, 2.0, 3.0])
t.setColData(2, [4.0, 5.0, 6.0])
g = newGraph("AddCurveGraph").activeLayer()

assert g.addCurve(t, "2", Graph.CurveType.Scatter) and g.numCurves() == 1, g.numCurves()
for call in (lambda: g.addCurve(t, "I", Graph.CurveType.Scatter),
             lambda: g.addCurves(t, ("2", "nope"), Graph.CurveType.Scatter)):
    try:
        call()
        message = ""
    except ValueError as e:
        message = str(e)
    assert "no column" in message and "columns: 1, 2" in message, message
assert g.numCurves() == 1, g.numCurves()  # nothing was added by the failing calls
