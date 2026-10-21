"""A script-table row whose condition is beyond the current configurations is skipped, not read (it crashed)."""

import os
import sys

try:
    from PyQt6.QtCore import QMetaObject, Q_ARG
    from PyQt6.QtWidgets import QSlider, QTableWidget
except ImportError:
    from PyQt5.QtCore import QMetaObject, Q_ARG
    from PyQt5.QtWidgets import QSlider, QTableWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import DIRECT, answering, quiet

with quiet():
    Dan.Options.newSession()
    Dan.Options.selectInstrument("KWS1-2020")
slider = Dan.findChild(QSlider, "sliderConfigurations")
configurations = Dan.findChild(QTableWidget, "tableEC")
slider.setValue(3)

with answering():
    QMetaObject.invokeMethod(Dan, "newScriptTable", DIRECT, Q_ARG(str, "script"))
script = table("script")
names = list(script.colNames())
condition, ec, xCenter = names.index("#-Condition") + 1, names.index("#-EC [EB]") + 1, names.index("X-center") + 1
script.setNumRows(2)
for row, value in ((1, "3"), (2, "1")):  # row 1: the third configuration, made before there were only two
    script.setText(condition, row, value)
    script.setText(ec, row, "old")
    script.setText(xCenter, row, "1.5")

slider.setValue(2)
assert configurations.columnCount() == 2, configurations.columnCount()
configurations.item(0, 0).setText("12345")  # EC of the first configuration

with answering():
    QMetaObject.invokeMethod(Dan, "vertHeaderTableECPressed", DIRECT, Q_ARG(int, 0), Q_ARG(bool, True))
    QMetaObject.invokeMethod(Dan, "vertHeaderTableECPressed", DIRECT, Q_ARG(int, 15), Q_ARG(bool, True))  # centers

assert script.text(ec, 1) == "old", script.text(ec, 1)
assert script.text(ec, 2) == "12345", script.text(ec, 2)
assert float(script.text(xCenter, 1)) == 1.5, script.text(xCenter, 1)
center = configurations.item(15, 0).text()[:6]
assert float(script.text(xCenter, 2)) == float(center) != 1.5, (script.text(xCenter, 2), center)
