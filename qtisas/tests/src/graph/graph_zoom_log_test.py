"""Rectangle zoom on log axes: a box near the low end of a log axis spanning many decades zooms into that box. Qwt
widened a box smaller than 1/10^5 of the full range around its center in data units, which on y = 1e-4..1e5 put every
box below y = 0.5 at a negative lower bound: the axis then ran from 1e-150. The minimum is now taken on the axis' own
scale (decades on a log axis), for log10, ln, log2, mirrored (negative) log axes, inverted or with a break across
zero (also zoomed into one half of it, drawn with that half's scale); on a linear axis it is Qwt's one. Zoom-out goes
back through the zoom stack to the exact bounds, also to a bound near zero of a negative log axis. The axis (ticks,
break decoration) is drawn with the zoomed range as the canvas is, and the matplotlib script exports that range.
"""

import math
import os
import tempfile

import qti

try:
    from PyQt6.QtCore import QEvent, QMetaObject, QPoint, QPointF, Qt
    from PyQt6.QtGui import QMouseEvent
    from PyQt6.QtWidgets import QApplication, QFrame
except ImportError:
    from PyQt5.QtCore import QEvent, QMetaObject, QPoint, QPointF, Qt
    from PyQt5.QtGui import QMouseEvent
    from PyQt5.QtWidgets import QApplication, QFrame

DIRECT = getattr(Qt, "ConnectionType", Qt).DirectConnection
EVENT = getattr(QEvent, "Type", QEvent)
LEFT = getattr(Qt, "MouseButton", Qt).LeftButton
NO_BUTTON = getattr(Qt, "MouseButton", Qt).NoButton
NO_MODIFIER = getattr(Qt, "KeyboardModifier", Qt).NoModifier
LINEAR, LOG10, LN, LOG2 = 0, 1, 2, 3
Y_LEFT, X_BOTTOM = 0, 2

t = newTable("ZoomLogData", 200, 2)
xs = [10 ** (-3 + 4 * i / 199) for i in range(200)]
t.setColData(1, xs)
t.setColData(2, [1e-4 * x ** -2.25 for x in xs])
g = newGraph("ZoomLogGraph").activeLayer()
g.addCurve(t, "2", Graph.CurveType.Scatter)
canvas = [w for w in g.findChildren(QFrame) if w.metaObject().className() == "QwtPlotCanvas"][0]


def mouse(kind, point, buttons):
    p = QPointF(point)
    QApplication.sendEvent(canvas, QMouseEvent(kind, p, QPointF(canvas.mapToGlobal(p)), LEFT, buttons, NO_MODIFIER))


def drag(fx1, fy1, fx2, fy2):
    """A zoom box from (fx1, fy1) to (fx2, fy2), in fractions of the canvas, or in pixels when > 1."""
    def at(f, size):
        return int(f) if f > 1 else int(f * size)
    p1 = QPoint(at(fx1, canvas.width()), at(fy1, canvas.height()))
    p2 = QPoint(at(fx2, canvas.width()), at(fy2, canvas.height()))
    mouse(EVENT.MouseButtonPress, p1, LEFT)
    for k in range(1, 5):
        mouse(EVENT.MouseMove, QPoint(p1.x() + (p2.x() - p1.x()) * k // 4, p1.y() + (p2.y() - p1.y()) * k // 4), LEFT)
    mouse(EVENT.MouseButtonRelease, p2, NO_BUTTON)
    QApplication.processEvents()
    return g.axisScale(X_BOTTOM), g.axisScale(Y_LEFT)


def start(y_type, y1, y2, x_type=LOG10, x1=1e-3, x2=10.0):
    QMetaObject.invokeMethod(qti.app, "zoomIn", DIRECT)
    g.setScale(X_BOTTOM, x1, x2, 0.0, 5, 5, x_type)
    g.setScale(Y_LEFT, y1, y2, 0.0, 5, 5, y_type)
    QApplication.processEvents()


# a box near the low end of y = 1e-4..1e5: positive, inside the box (~1.4e-4 .. 7e-4), on every log type
for y_type in (LOG10, LN, LOG2):
    start(y_type, 1e-4, 1e5)
    (x1, x2), (y1, y2) = drag(0.4, 0.9, 0.6, 0.98)
    assert 1e-4 <= y1 < y2 < 1e-3, (y_type, y1, y2)
    assert 0.03 < x1 < x2 < 0.4, (y_type, x1, x2)
    QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
    assert g.axisScale(Y_LEFT) == (1e-4, 1e5), (y_type, g.axisScale(Y_LEFT))
    # the high end was right before and stays so
    (x1, x2), (y1, y2) = drag(0.4, 0.02, 0.6, 0.1)
    assert 1e4 < y1 < y2 <= 1e5, (y_type, y1, y2)
    QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)

# mirrored log axis (all values negative, y = -1e5 .. -1e-4): near zero (top), at the far end (bottom), the minimum
# zoom size in decades, on every log type and inverted
t.setColData(2, [-1e-4 * x ** -2.25 for x in xs])
for y_type in (LOG10, LN, LOG2):
    start(y_type, -1e5, -1e-4)
    base = g.axisScale(Y_LEFT)
    assert base == (-1e5, -1e-4), base
    _, (y1, y2) = drag(0.4, 0.02, 0.6, 0.1)
    assert -1e-3 < y1 < y2 <= -1e-4, (y_type, y1, y2)
    QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
    _, (y1, y2) = drag(0.4, 0.9, 0.6, 0.98)
    assert -1e5 <= y1 < y2 < -1e4, (y_type, y1, y2)
    QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
    for _ in range(5):
        _, (y1, y2) = drag(0.5, 0.5, canvas.width() // 2 + 2, canvas.height() // 2 + 2)
    assert y2 < 0 and math.isclose(math.log10(y1 / y2), math.log10(base[0] / base[1]) / 1e5, rel_tol=1e-3), (y1, y2)
    for _ in range(5):
        QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
    assert g.axisScale(Y_LEFT) == base, (y_type, g.axisScale(Y_LEFT), base)
start(LOG10, -1e5, -1e-4)
g.setScale(Y_LEFT, -1e5, -1e-4, 0.0, 5, 5, LOG10, True)  # inverted: zero at the bottom
_, (y2, y1) = drag(0.4, 0.9, 0.6, 0.98)  # high to low
assert -1e-3 < y1 < y2 <= -1e-4, (y1, y2)
QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
assert g.axisScale(Y_LEFT) == (base[1], base[0]), g.axisScale(Y_LEFT)

# zoom-out back to a negative range of 17 decades: the bound near zero is exact (the zoom stack's QRectF holds it as
# -1e5 + width, which gave 0.0)
t.setColData(2, [-1e-12 * 10 ** (17 * i / 199) for i in range(200)])
start(LOG10, -1e5, -1e-12)
base = g.axisScale(Y_LEFT)
assert base == (-1e5, -1e-12), base
drag(0.4, 0.3, 0.6, 0.7)
QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
assert g.axisScale(Y_LEFT) == base, g.axisScale(Y_LEFT)

# log axis across zero with a break at +-1e-4 (negative half mirrored): boxes next to the break stay on their half
t.setColData(2, [(-1 if i % 2 else 1) * 1e-4 * x ** -2.25 for i, x in enumerate(xs)])
start(LOG10, -1e5, 1e5)
g.setScale(Y_LEFT, -1e5, 1e5, 0.0, 5, 5, LOG10, False, -1e-4, 1e-4, 50, 0.0, 0.0, 4, 4, LOG10)
_, (y1, y2) = drag(0.4, 0.40, 0.6, 0.47)
assert 1e-4 <= y1 < y2 < 1e-2, (y1, y2)
QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
_, (y1, y2) = drag(0.4, 0.53, 0.6, 0.60)
assert -1e-2 < y1 < y2 <= -1e-4, (y1, y2)
QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
assert g.axisScale(Y_LEFT) == (-1e5, 1e5), g.axisScale(Y_LEFT)


def log_between(top, bottom, f):
    """The value at the fraction f from the top of a log (or mirrored log) range."""
    return math.copysign(abs(top) ** (1 - f) * abs(bottom) ** f, top)


# the same with the break switching to log guesses: a box inside one half zooms to that half alone, which is drawn
# with that half's scale, a mirrored log one for the negative half: a second box in its middle is log in |y| (it
# was drawn as a positive log axis, so the curve was gone and a second box gave nonsense)
g.setScale(Y_LEFT, -1.0, 1.0, 0.0, 5, 5, LINEAR)
g.setLinOrLogAxis(Y_LEFT, True, True, True)
QApplication.processEvents()
base = g.axisScale(Y_LEFT)
assert base[0] < 0 < base[1], base
QMetaObject.invokeMethod(qti.app, "zoomIn", DIRECT)
for box, sign in (((0.75, 0.9), -1), ((0.1, 0.25), 1)):
    _, (y1, y2) = drag(0.3, box[0], 0.7, box[1])
    assert y1 * sign > 0 and y2 * sign > 0, (sign, y1, y2)
    _, (z1, z2) = drag(0.3, 0.25, 0.7, 0.75)
    assert math.isclose(z2, log_between(y2, y1, 0.25), rel_tol=0.05), (z2, log_between(y2, y1, 0.25))
    assert math.isclose(z1, log_between(y2, y1, 0.75), rel_tol=0.05), (z1, log_between(y2, y1, 0.75))
    QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
    QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
    assert g.axisScale(Y_LEFT) == base, g.axisScale(Y_LEFT)


def rebuilt():
    """The graph as drawn after a resize there and back, which rebuilds every break transform from the scale."""
    g.resize(g.width() + 1, g.height() + 1)
    QApplication.processEvents()
    g.resize(g.width() - 1, g.height() - 1)
    QApplication.processEvents()
    return g.grab().toImage()


# a box across the break, off its center: the axis (ticks, labels, break decoration on it) is drawn with the zoomed
# range's break transform as the canvas is (curves, grid, break decoration on the canvas frame): the axis kept the
# transform of the range before, so its decoration and labels sat apart from the canvas. Also zoomed into one half
# (no break decoration then: the break is off the axis) and back to the full range.
def matplotlib_y():
    """The y axis lines (scale, break decoration, ticks) of the graph's matplotlib script."""
    path = os.path.join(tempfile.mkdtemp(), "zoom.py")
    assert g.exportMatplotlibScript(path)
    with open(path) as f:
        return [line for line in f if line.startswith(("ax.set_y", "ax.yaxis", "ax.add_line(mlines.Line2D(["))]


# The matplotlib script draws the zoomed range as QtiSAS does: across the break the break scale and its decoration,
# inside one half that half's scale (mirrored log for the negative one) and no decoration (it was drawn outside
# the axes, at the break's place off the range).
g.showGrid()
for box, scale, marks in (((0.2, 0.62), "qtisas_break_scale(", 2), ((0.52, 0.95), 'qtisas_scale("neglog")', 0),
                          ((0.1, 0.25), '"log", base=10', 0)):
    drag(0.3, box[0], 0.7, box[1])
    zoomed = g.grab().toImage()
    assert zoomed == rebuilt(), box
    lines = matplotlib_y()
    assert any(line.startswith("ax.set_yscale(") and scale in line for line in lines), (box, lines)
    assert sum(line.startswith("ax.add_line(") for line in lines) == marks, (box, lines)
    QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
    assert g.grab().toImage() == rebuilt(), box
g.setScale(Y_LEFT, 1e-4, 1e5, 0.0, 5, 5, LOG10)
t.setColData(2, [1e-4 * x ** -2.25 for x in xs])

# a log range under one decade: the script sets the minor ticks (none here) itself, else matplotlib's own log minor
# ticks came with labels (3x10^-4) QtiSAS does not draw
g.setScale(Y_LEFT, 1.2e-4, 6.6e-4, 0.0, 5, 5, LOG10)
lines = matplotlib_y()
assert any(line.startswith("ax.set_yticks([") and "minor=True" in line for line in lines), lines
assert "ax.yaxis.set_minor_formatter(mticker.NullFormatter())\n" in lines, lines

# the minimum zoom size: 1/10^5 of the full range, in decades on a log axis (9 decades -> 9e-5 decades), in data
# units on a linear one (Qwt's); boxes of 2 x 2 pixels, zoomed into until it applies
start(LOG10, 1e-4, 1e5, LINEAR, 0.0, 10.0)
for _ in range(5):
    (x1, x2), (y1, y2) = drag(0.5, 0.5, canvas.width() // 2 + 2, canvas.height() // 2 + 2)
assert math.isclose(math.log10(y2 / y1), 9e-5, rel_tol=1e-3), math.log10(y2 / y1)
assert math.isclose(x2 - x1, 1e-4, rel_tol=1e-3), x2 - x1
assert 0 < y1, y1
for _ in range(5):  # back through the stack to the full range
    QMetaObject.invokeMethod(qti.app, "zoomOut", DIRECT)
assert g.axisScale(Y_LEFT) == (1e-4, 1e5) and g.axisScale(X_BOTTOM) == (0.0, 10.0), g.axisScale(Y_LEFT)
