"""Graph.setScale() on a log axis keeps an all-negative range (drawn as a mirrored log axis): the guess of a break
from the data replaced it by the data range on every call without a break (Python, the axis dialog, a restored
project). Switching an axis to log still guesses from the data: all-negative data get their data range, mixed signs
a break across zero; so does a range across zero set without a break.
"""

LINEAR, LOG10, LN, LOG2 = 0, 1, 2, 3
Y_LEFT = 0

t = newTable("LogNegativeData", 50, 2)
xs = [1.0 + i for i in range(50)]
t.setColData(1, xs)
t.setColData(2, [-1e-3 * x ** 2 for x in xs])  # -1e-3 .. -2.5
g = newGraph("LogNegativeGraph").activeLayer()
g.addCurve(t, "2", Graph.CurveType.Scatter)

# an explicit negative range, wider and narrower than the data, on every log type and inverted
for y_type in (LOG10, LN, LOG2):
    for y1, y2 in ((-1e5, -1e-4), (-1.0, -0.01)):
        g.setScale(Y_LEFT, y1, y2, 0.0, 5, 5, y_type)
        assert g.axisScale(Y_LEFT) == (y1, y2), (y_type, g.axisScale(Y_LEFT))
        g.setScale(Y_LEFT, y1, y2, 0.0, 5, 5, y_type, True)
        assert g.axisScale(Y_LEFT) == (y2, y1), (y_type, g.axisScale(Y_LEFT))

# switching to log: the data range of all-negative data
g.setScale(Y_LEFT, -3.0, 0.0, 0.0, 5, 5, LINEAR)
g.setLinOrLogAxis(Y_LEFT, True, True, True)
low, high = g.axisScale(Y_LEFT)
assert low <= -2.5 and -1e-3 <= high < 0, (low, high)

# mixed signs: switching to log breaks the axis across zero; an explicit negative part of it is kept
t.setColData(2, [(-1 if i % 2 else 1) * 1e-3 * x ** 2 for i, x in enumerate(xs)])
g.setScale(Y_LEFT, -3.0, 3.0, 0.0, 5, 5, LINEAR)
g.setLinOrLogAxis(Y_LEFT, True, True, True)
low, high = g.axisScale(Y_LEFT)
assert low < 0 < high, (low, high)
g.setScale(Y_LEFT, -1.0, -0.01, 0.0, 5, 5, LOG10)
assert g.axisScale(Y_LEFT) == (-1.0, -0.01), g.axisScale(Y_LEFT)

# a range across zero without a break: the break is guessed from the data, the range becomes the data's
g.setScale(Y_LEFT, -3.0, 3.0, 0.0, 5, 5, LOG10)
low, high = g.axisScale(Y_LEFT)
assert -3.0 < low < 0 < high < 3.0, (low, high)
