"""Python Table.setText() stores C numbers in the table's locale; cellOrNone() reports unparsable cells."""

import os
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import locale_german

german = locale_german()

t = newTable("PythonSetText", 4, 2)
t.setLocale(german)
t.setColTextFormat(2)

t.setText(1, 1, "1.5")      # C number -> localized value
assert t.text(1, 1) == "1,5", t.text(1, 1)
assert t.cell(1, 1) == 1.5, t.cell(1, 1)

t.setText(1, 2, "1.500")    # C 1.5, not German 1500
assert t.text(1, 2) == "1,5", t.text(1, 2)
assert t.cell(1, 2) == 1.5, t.cell(1, 2)

t.setText(1, 3, "1,234")    # not a C number: left to the cell editor, which reads German 1.234
assert t.cell(1, 3) == 1.234, t.cell(1, 3)

t.setText(2, 1, "1.5")      # Text column -> unchanged
assert t.text(2, 1) == "1.5", t.text(2, 1)

# cellOrNone(): a number, or None for an empty/unparsable cell (cell() would give 0.0 or raise)
assert t.cellOrNone(1, 1) == 1.5, t.cellOrNone(1, 1)
assert t.cellOrNone(1, 4) is None, t.cellOrNone(1, 4)          # empty
t.setText(1, 4, "---")
assert t.text(1, 4) == "", t.text(1, 4)                          # the cell editor clears non-numbers
t.setColTextFormat(1)
t.setText(1, 4, "label")
assert t.cellOrNone(1, 4) is None, t.cellOrNone(1, 4)          # text that is not a number
assert t.cellOrNone(2, 1) == 1.5, t.cellOrNone(2, 1)            # C number in a Text column
