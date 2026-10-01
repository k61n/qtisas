"""Compile interface follows the numerical format; the Python API stays C. Uses NIST/Misra1b, else skipped.

Saving is not tested here (it would write into the user's function library); .fif files are written in C.
"""

import os
import sys

try:
    from PyQt6.QtWidgets import QLineEdit, QTableWidget
except ImportError:
    from PyQt5.QtWidgets import QLineEdit, QTableWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import locale_c, locale_german, report, set_app_locale, skip

set_app_locale(locale_c())  # start in C, whatever numerical format the user's Preferences saved

c = Compiler
if "NIST/Misra1b" not in c.functionNames("ALL"):
    skip("function NIST/Misra1b is not in the library")
c.open("NIST/Misra1b")
table = c.findChild(QTableWidget, "tableParaNames")
edit = lambda name: c.findChild(QLineEdit, name).text()
assert table.item(1, 1).text() == "0.0001", table.item(1, 1).text()
algorithm = edit("lineEditFitMethodPara")
assert "DER=0.001" in algorithm, algorithm

set_app_locale(locale_german())
assert table.item(1, 1).text() == "0,0001", table.item(1, 1).text()  # converted on the format change
assert c.paramInit(1) == 0.0001, c.paramInit(1)
assert edit("lineEditFitMethodPara") == algorithm.replace(".", ","), edit("lineEditFitMethodPara")
assert c.fitMethodPara() == algorithm, c.fitMethodPara()

c.setParamInit(1, 0.00025)
c.setParamLimit(0, "1.5..700.25")
c.setXRange(True, 0.0025, 0.525, 11, False, 0.0)
shown = (table.item(1, 1).text(), table.item(0, 2).text())
assert shown == ("0,00025", "1,5..700,25"), shown
assert (edit("lineEditMin"), edit("lineEditMax")) == ("0,0025", "0,525"), (edit("lineEditMin"), edit("lineEditMax"))
assert c.paramInit(1) == 0.00025 and c.paramLimit(0) == "1.5..700.25", (c.paramInit(1), c.paramLimit(0))
assert (c.xMin(), c.xMax()) == (0.0025, 0.525), (c.xMin(), c.xMax())
report("Compile locale: widgets converted and localized, Python API in C", first=True)
