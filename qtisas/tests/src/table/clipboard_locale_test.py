"""Copy and paste use the clipboard's format (Preferences), whatever the table's or matrix's number format.

The test does not change the user's clipboard setting: it learns the clipboard's decimal point from a copy.
"""

import os
import sys

try:
    from PyQt6.QtCore import QMetaObject
    from PyQt6.QtWidgets import QApplication, QTableView, QTableWidget
except ImportError:
    from PyQt5.QtCore import QMetaObject
    from PyQt5.QtWidgets import QApplication, QTableView, QTableWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import locale_german

german = locale_german()
clipboard = QApplication.clipboard()

# Table: copy a German column; the clipboard gets 1.5 in its own format and never a group separator
t = newTable("ClipboardLocale", 3, 2)
t.setLocale(german)
t.setCell(1, 1, 1.5)
t.setCell(1, 2, 1250)
assert t.text(1, 1) == "1,5", t.text(1, 1)
t.findChild(QTableWidget).selectColumn(0)
QMetaObject.invokeMethod(t, "copySelection")
copied = clipboard.text().split()
assert len(copied) == 2 and copied[1] == "1250", clipboard.text()
point = copied[0][1:-1]  # "1<point>5"
assert copied[0] == "1" + point + "5" and point in (".", ","), clipboard.text()

# ... and pasting clipboard-format numbers gives the table's format
clipboard.setText("0%s125\n2%s75" % (point, point))
t.findChild(QTableWidget).clearSelection()
t.findChild(QTableWidget).setCurrentCell(0, 1)
QMetaObject.invokeMethod(t, "pasteSelection")
assert (t.text(2, 1), t.text(2, 2)) == ("0,125", "2,75"), (t.text(2, 1), t.text(2, 2))

# Matrix: the same both ways
m = newMatrix("ClipboardLocaleMatrix", 2, 2)
m.setLocale(german)
m.setCell(1, 1, 2.5)
view = m.findChild(QTableView)
view.setCurrentIndex(view.model().index(0, 0))
view.clearSelection()
QMetaObject.invokeMethod(m, "copySelection")
assert clipboard.text().startswith("2" + point + "5"), clipboard.text()

clipboard.setText("0%s125" % point)
view.setCurrentIndex(view.model().index(1, 1))
QMetaObject.invokeMethod(m, "pasteSelection")
assert m.cell(2, 2) == 0.125 and m.text(2, 2).startswith("0,125"), (m.cell(2, 2), m.text(2, 2))
