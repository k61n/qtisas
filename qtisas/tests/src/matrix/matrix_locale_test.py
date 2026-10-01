"""Matrix numbers as text: display in the matrix's locale, strict parsing of edits, C numbers from Python."""

import math
import os
import sys

try:
    from PyQt6.QtCore import Qt
    from PyQt6.QtWidgets import QTableView
except ImportError:
    from PyQt5.QtCore import Qt
    from PyQt5.QtWidgets import QTableView

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import locale_german

german = locale_german()

m = newMatrix("MatrixLocale", 3, 3)
m.setLocale(german)

# Python: a C number is stored as that number, whatever the matrix's format
m.setText(1, 1, "1.5")
assert m.cell(1, 1) == 1.5 and m.text(1, 1).startswith("1,5"), (m.cell(1, 1), m.text(1, 1))
m.setText(1, 2, "1.500")            # C 1.5, not German 1500
assert m.cell(1, 2) == 1.5, m.cell(1, 2)
m.setText(1, 3, "0,25")             # the matrix's own format is accepted too
assert m.cell(1, 3) == 0.25, m.cell(1, 3)
m.setText(2, 1, "abc")              # not a number: empty, not 0
assert m.text(2, 1) == "", m.text(2, 1)

# Editing a cell: the matrix's format, grouped input rejected while grouping is omitted
model = m.findChild(QTableView).model()
edit = lambda row, col, text: model.setData(model.index(row, col), text, getattr(Qt, "ItemDataRole", Qt).EditRole)
edit(2, 0, "2,5")
assert m.cell(3, 1) == 2.5, m.cell(3, 1)
edit(2, 1, "1.500")                 # grouped German 1500: rejected
assert m.text(3, 2) == "", m.text(3, 2)
edit(2, 2, "xyz")                   # was 0 before: now empty
assert m.text(3, 3) == "", m.text(3, 3)
