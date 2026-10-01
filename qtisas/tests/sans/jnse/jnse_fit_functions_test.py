"""JNSE lists fit functions when Multi-Fitting is opened with an empty list or its filter is used, even if no group
is selected in FitTable (ALL is selected), sorted as in FitTable.

Uses the NIST/Misra1* functions of the FitTable library; without them the test is reported as skipped.
"""

import os
import sys

try:
    from PyQt6.QtWidgets import QApplication, QComboBox, QLineEdit, QListView, QTabWidget
except ImportError:
    from PyQt5.QtWidgets import QApplication, QComboBox, QLineEdit, QListView, QTabWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import skip

widget = lambda name: [w for w in QApplication.allWidgets() if w.metaObject().className() == name][0]
jnse, fittable = widget("jnse18"), widget("fittable18")
functions = jnse.findChild(QComboBox, "comboBoxFitFunctions")
search = jnse.findChild(QLineEdit, "lineEditFunctionsFilter")
groups = fittable.findChild(QListView, "listViewGroup")

if "NIST/Misra1a" not in Fittable.fitFunctions("NIST/Misra1*"):
    skip("NIST/Misra1* functions are not in the library")

assert groups.currentIndex().data() is None, groups.currentIndex().data()  # nothing selected after the start
assert functions.count() == 0, functions.count()

# Data Import -> Multi-Fitting: the empty list is filled with the filter's functions ("*": all)
tabs = jnse.findChild(QTabWidget, "jnseTab")
tabs.setCurrentIndex(0)
tabs.setCurrentWidget(tabs.findChild(type(tabs.widget(0)), "TabPageFitting"))
assert groups.currentIndex().data() == "ALL", groups.currentIndex().data()
assert functions.count() > 0 and functions.findText("NIST/Misra1a") >= 0, functions.count()

search.setText("NIST/Misra1*")
search.returnPressed.emit()
assert groups.currentIndex().data() == "ALL", groups.currentIndex().data()
listed = [functions.itemText(i) for i in range(functions.count())]
assert listed[:2] == ["NIST/Misra1a", "NIST/Misra1b"] and all(f.startswith("NIST/Misra1") for f in listed), listed
assert listed == sorted(listed), listed  # sorted, not in the order of the files on disk
