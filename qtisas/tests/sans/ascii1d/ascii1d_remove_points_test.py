"""ASCII.1D Modification / Remove "Bad" Points: each option works on its own, with "Actions" on.

A table of 10 points (Q = 1..10, I = Q, the 3rd point negative) is loaded with "Load" (source: tables) into a new
table for each case; the Q values left are compared. A From or To of 0 removes no range; removing all points gives
no table and a message.
"""

import os
import sys

try:
    from PyQt6.QtWidgets import QApplication, QCheckBox, QComboBox, QPushButton, QSpinBox
except ImportError:
    from PyQt5.QtWidgets import QApplication, QCheckBox, QComboBox, QPushButton, QSpinBox

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import accept_text, answering, locale_c, set_app_locale

set_app_locale(locale_c())
ascii1d = [w for w in QApplication.allWidgets() if w.metaObject().className() == "ascii1d18"][0]
child = lambda cls, name: ascii1d.findChild(cls, name)

data = newTable("Ascii1dRemoveData", 10, 3)
q = [float(k) for k in range(1, 11)]
data.setColData(1, q)
data.setColData(2, [-x if x == 3 else x for x in q])
data.setColData(3, [0.1] * 10)

child(QComboBox, "comboBoxSource").setCurrentIndex(1)  # tables
child(QComboBox, "comboBoxDestination").setCurrentIndex(0)  # table
for name, on in (("checkBoxIndexing", True), ("checkBoxActions", True), ("checkBoxConvert", False),
                 ("checkBoxMath", False), ("checkBoxBinningLinear", False), ("checkBoxBinningProgressive", False),
                 ("checkBox1DCalculator", False), ("checkBoxReso", False)):
    child(QCheckBox, name).setChecked(on)

OPTIONS = ("checkBoxRemoveRange", "checkBoxRemoveFirst", "checkBoxRemoveLast", "checkBoxNoNegative")


def load(checked, **values):
    """Q values of the table that "Load" makes with only the checked options; None if it makes none."""
    for name in OPTIONS:
        child(QCheckBox, name).setChecked(name in checked)
    for name in ("lineEditRemoveFrom", "lineEditRemoveTo", "lineEditRemoveFirst", "lineEditRemoveLast"):
        child(QSpinBox, name).setValue(values.get(name, 0))
    before = set(tableNames())
    with answering(accept_text("Ascii1dRemoveData")) as messages:
        child(QPushButton, "ASCIIpushButton").click()
    created = [table(n) for n in tableNames() if n not in before]
    if not created:
        return None, messages
    t = created[0]
    return [t.cellOrNone(1, r) for r in range(1, t.numRows() + 1)], messages


CASES = [
    # checked options, spin box values, Q left
    ((), {}, q),
    (("checkBoxRemoveFirst",), {"lineEditRemoveFirst": 2}, q[2:]),
    (("checkBoxRemoveLast",), {"lineEditRemoveLast": 3}, q[:7]),
    (("checkBoxRemoveRange",), {"lineEditRemoveFrom": 4, "lineEditRemoveTo": 5}, q[:3] + q[5:]),
    (("checkBoxRemoveRange",), {"lineEditRemoveFrom": 5, "lineEditRemoveTo": 4}, q[:3] + q[5:]),  # either order
    (("checkBoxRemoveRange",), {"lineEditRemoveFrom": 0, "lineEditRemoveTo": 5}, q),  # 0: no range
    (("checkBoxNoNegative",), {}, q[:2] + q[3:]),
    # values of unchecked options do nothing
    (("checkBoxRemoveLast",), {"lineEditRemoveFirst": 2, "lineEditRemoveFrom": 4, "lineEditRemoveTo": 5,
                               "lineEditRemoveLast": 1}, q[:9]),
    (("checkBoxRemoveFirst", "checkBoxRemoveLast"), {"lineEditRemoveFirst": 1, "lineEditRemoveLast": 1}, q[1:9]),
]
for checked, values, expected in CASES:
    left, messages = load(checked, **values)
    assert left == expected and not messages, (checked, values, left, expected, messages)

# all points removed: no table, a message
left, messages = load(("checkBoxRemoveFirst",), lineEditRemoveFirst=10)
assert left is None and len(messages) == 1 and "no data" in messages[0], (left, messages)
