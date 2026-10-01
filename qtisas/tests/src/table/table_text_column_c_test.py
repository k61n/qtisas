"""Numbers in non-Numeric columns stay C; only Numeric columns follow the table's locale."""

import os
import sys

try:
    from PyQt6.QtGui import QColor
except ImportError:
    from PyQt5.QtGui import QColor

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import locale_german

german = locale_german()

t = newTable("TextColumnC", 3, 3)
t.setLocale(german)
t.setColTextFormat(3)

# Numeric column: localized text; Text column: C text, read back as a number
t.setCell(1, 1, 1.5)
t.setCell(3, 1, 0.25)
assert t.text(1, 1) == "1,5", t.text(1, 1)
assert t.text(3, 1) == "0.25", t.text(3, 1)
assert t.cell(3, 1) == 0.25, t.cell(3, 1)

# Error bars read a Text error column as C
t.setCell(1, 2, 2.0)
t.setCell(2, 1, 10.0)
t.setCell(2, 2, 20.0)
t.setCell(3, 2, 0.5)
g = newGraph("TextColumnCGraph").activeLayer()
curve = g.insertCurve(t, "1", "2")
bars = g.addErrorBars(curve, t, "3", 1, 1, 8, QColor("black"), True, True, True)
assert bars is not None, "error bars not created"
assert abs(bars.errorValue(0) - 0.25) < 1e-12, bars.errorValue(0)
assert abs(bars.errorValue(1) - 0.5) < 1e-12, bars.errorValue(1)

# Text -> Numeric parses the C text and shows it in the table's locale
t.setText(3, 3, "1.75")
t.setColNumericFormat(3, Table.NumericFormat.Default, 6, True)
assert t.text(3, 3) == "1,75", t.text(3, 3)
assert t.cell(3, 3) == 1.75, t.cell(3, 3)
