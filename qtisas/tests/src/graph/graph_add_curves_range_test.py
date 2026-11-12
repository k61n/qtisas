"""Add/Remove Curves on a log-log graph keeps the graph's own axis limits.

Graph::updatePlot() autoscaled every axis with Qwt's autoscale, which rounds a log axis out to other decades than the
graph's own limits: adding a second copy of the same curve moved x 0.00729..1 to 0.001..1 and y 1..111 to 1..1000,
with no new data. Now a linear or log axis takes the limits of setLinOrLogAxis(), as setAutoScale() does.
"""

try:
    from PyQt6.QtCore import QMetaObject
    from PyQt6.QtGui import QAction
    from PyQt6.QtWidgets import QApplication, QDialog, QTreeWidget
except ImportError:
    from PyQt5.QtCore import QMetaObject
    from PyQt5.QtWidgets import QAction, QApplication, QDialog, QTreeWidget

app = [w for w in QApplication.topLevelWidgets() if w.metaObject().className() == "ApplicationWindow"][0]

t = newTable("AddRangeT", 20, 4)
t.setColNames(["x", "a", "x2", "b"])
t.setColumnRole(3, Table.PlotDesignation.X)
t.setColData(1, [0.01 * (i + 1) for i in range(20)])
t.setColData(2, [100.0 / (i + 1) for i in range(20)])
t.setColData(3, [0.5 * (i + 1) for i in range(20)])
t.setColData(4, [5.0 + i for i in range(20)])


def add_with_dialog(window, column):
    """Adds `column` to the window's active layer through the Add/Remove Curves dialog."""
    app.activateWindow(window)
    window.show()
    QApplication.processEvents()
    [a for a in app.findChildren(QAction) if a.text() == "Add/Remove &Curve..."][0].trigger()
    QApplication.processEvents()
    dialog = [d for d in app.findChildren(QDialog) if d.metaObject().className() == "CurvesDialog" and d.isVisible()]
    assert dialog, "no Add/Remove Curves dialog"
    tree = dialog[-1].findChildren(QTreeWidget)[0]

    def select(item):
        for i in range(item.childCount()):
            if item.child(i).text(0) == column:
                item.child(i).setSelected(True)
            select(item.child(i))

    for i in range(tree.topLevelItemCount()):
        select(tree.topLevelItem(i))
    assert tree.selectedItems(), column
    QMetaObject.invokeMethod(dialog[-1], "addCurves")
    QApplication.processEvents()
    dialog[-1].close()
    QApplication.processEvents()


def scales(g):
    return [tuple(round(v, 6) for v in g.axisScale(axis)) for axis in (2, 0)]


w = newGraph("AddRangeG", 1, 1, 1)
g = w.activeLayer()
g.insertCurve(t, "a")
g.setLinOrLogAxis(2, True)
g.setLinOrLogAxis(0, True)
g.replot()
before = scales(g)

add_with_dialog(w, "AddRangeT_a")  # the same data again: the limits stay
assert g.numCurves() == 2, g.numCurves()
assert scales(g) == before, (before, scales(g))

add_with_dialog(w, "AddRangeT_b")  # x data up to 10: x grows to show it; y data inside: y stays
assert g.numCurves() == 3, g.numCurves()
x, y = scales(g)
assert x[0] <= 0.01 and x[1] >= 10.0, (before, x)
assert y == before[1], (before, y)

closeWindow(w)
closeWindow(t)
