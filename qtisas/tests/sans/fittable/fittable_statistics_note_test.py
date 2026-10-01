"""FitTable's hidden statistics note does not stop a new graph from opening maximized.

The data table is the maximized active window. A fit with "Statistics" on creates the hidden notes
fitCurve-<function>-statistics and -session; a new graph must then open maximized, as after any maximized active
window.
Needs the compiled "qtiplot/Linear" function of the FitTable library; without it the test is reported as skipped.
"""

import os
import sys

try:
    from PyQt6.QtWidgets import QApplication, QCheckBox
except ImportError:
    from PyQt5.QtWidgets import QApplication, QCheckBox

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import quiet, skip

f = Fittable
if "qtiplot/Linear" not in f.fitFunctions("qtiplot/Linear"):
    skip("FitTable function 'qtiplot/Linear' is not available")

area = workspace()
active = lambda: area.activeSubWindow().objectName()  # the MDI area's active window
fittable = [w for w in QApplication.allWidgets() if w.metaObject().className() == "fittable18"][0]
statistics = fittable.findChild(QCheckBox, "checkBoxCovar")

data = newTable("StatisticsNoteData", 20, 2)
x = [0.1 * (i + 1) for i in range(20)]
data.setColData(1, x)
data.setColData(2, [2.0 + 3.0 * v for v in x])
maximizeWindow(data)
f.configure("qtiplot/Linear", [data.objectName() + "_2"])

# a fit creates the statistics note hidden
statistics.blockSignals(True)  # checking it would also fit and show the note
statistics.setChecked(True)
statistics.blockSignals(False)
with quiet():
    f.fit()
note = [w for w in QApplication.allWidgets() if w.objectName() == "fitCurve-qtiplot/Linear-statistics"][0]
states = (note.isVisible(), active(), data.isMaximized())
assert states == (False, data.objectName(), True), states

graph = newGraph("StatisticsNoteGraph")
states = (active(), graph.isMaximized(), data.isMaximized())
assert states == (graph.objectName(), True, False), states
