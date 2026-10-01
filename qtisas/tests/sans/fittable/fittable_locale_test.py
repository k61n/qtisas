"""FitTable follows the numerical format: widgets, result tables and notes are localized, Python stays C.

Needs the compiled "Guinier" function of the FitTable library; without it the test is reported as skipped.
"""

import math
import os
import sys

try:
    from PyQt6.QtCore import QMetaObject
    from PyQt6.QtGui import QColor
    from PyQt6.QtTest import QTest
    from PyQt6.QtWidgets import QApplication, QCheckBox, QLineEdit, QTableWidget
except ImportError:
    from PyQt5.QtCore import QMetaObject
    from PyQt5.QtGui import QColor
    from PyQt5.QtTest import QTest
    from PyQt5.QtWidgets import QApplication, QCheckBox, QLineEdit, QTableWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import locale_c, locale_german, quiet, report, set_app_locale, skip

f = Fittable
if "Guinier" not in f.fitFunctions("Guinier"):
    skip("FitTable function 'Guinier' is not available")

set_app_locale(locale_c())  # start in C, whatever numerical format the user's Preferences saved

# Guinier data: I0 = 2.5, Rg = 40, bgd = 0.01, with a small ripple so the errors are not zero
data = newTable("FitLocaleData", 40, 3)
q = [0.002 + i * 0.001 for i in range(40)]
data.setColData(1, q)
data.setColData(2, [2.5 * math.exp(-(x * 40.0) ** 2 / 3.0) + 0.01 + 0.001 * math.sin(i) for i, x in enumerate(q)])
data.setColData(3, [0.01] * 40)

fw = [w for w in QApplication.allWidgets() if w.metaObject().className() == "fittable18"][0]
tablePara = fw.findChild(QTableWidget, "tablePara")
tableControl = fw.findChild(QTableWidget, "tableControl")
tableCurves = fw.findChild(QTableWidget, "tableCurves")
edit = lambda name: fw.findChild(QLineEdit, name).text()


def points(text):
    return [line for line in text.splitlines() if "Number of Points" in line][0]


f.configure("Guinier", ["FitLocaleData_2"])
f.setParamValue("Rg", 30.5)
f.setParamLimits("Rg", 1.5, 500.25)
f.setParamLimits("I0", 0.5, 7.5, False, 1)
f.setRange(1, 0.0055, 0.0305)
f.setTolerance(0.00025)
f.setLmDerivStep(0.0125)
statistics = fw.findChild(QCheckBox, "checkBoxCovar")
statistics.blockSignals(True)  # checking it by hand would also open the note
statistics.setChecked(True)
statistics.blockSignals(False)

with quiet():
    f.fit()
c_results = f.results()
c_points = points(f.fitReport())
assert tableControl.item(1, 0).text() == "1.5", tableControl.item(1, 0).text()
assert tableCurves.item(2, 1).text() == "0.0055", tableCurves.item(2, 1).text()

# --- switch the application to German: FitTable converts what it shows
set_app_locale(locale_german())

assert tablePara.item(0, 1).text() == "0,5..7,5", tablePara.item(0, 1).text()
assert tableControl.item(1, 0).text() == "1,5", tableControl.item(1, 0).text()
assert tableControl.item(1, 4).text() == "500,25", tableControl.item(1, 4).text()
assert tableCurves.item(2, 1).text() == "0,0055", tableCurves.item(2, 1).text()
assert edit("lineEditTolerance") == "0,00025", edit("lineEditTolerance")
assert edit("lineEditSTEP") == "0,0125", edit("lineEditSTEP")
assert f.tolerance() == 0.00025 and f.lmDerivStep() == 0.0125, (f.tolerance(), f.lmDerivStep())


# typing (NumberValidator): the decimal point corrected; no "-" where the value is not negative
def typed(name, keys):
    field = fw.findChild(QLineEdit, name)
    old = field.text()
    field.clear()
    QTest.keyClicks(field, keys)
    text = field.text()
    field.setText(old)
    return text


for name, keys, expected in (("lineEditSTEP", "1.5e-3", "1,5e-3"), ("lineEditSTEP", "-2", "2"),
                             ("lineEditFromQ", "-0.5", "-0,5"), ("lineEditAbsErr", "abc", "")):
    assert typed(name, keys) == expected, (name, keys, typed(name, keys), expected)
limits = (f.paramLimitLeft("Rg"), f.paramLimitRight("Rg"))
assert limits == (1.5, 500.25), limits
limits = (f.paramLimitLeft("I0", 1), f.paramLimitRight("I0", 1))
assert limits == (0.5, 7.5), limits

f.setParamValue("Rg", 30.5)
with quiet():
    f.fit()
fit_report = f.fitReport()
assert points(fit_report) == c_points, (points(fit_report), c_points)
for name in ("I0", "Rg", "bgd"):  # the second fit starts from the first one's result: same minimum, within its error
    value, error = f.results()[name]
    assert abs(value - c_results[name][0]) < 0.01 * error, (name, value, c_results[name])

# fitReport() is for scripts and stays C; the statistics note follows the numerical format
rg_report = [line for line in fit_report.splitlines() if line.endswith("| Rg")][-1]
assert rg_report.startswith("4") and "." in rg_report.split()[0], rg_report
note_text = note("fitCurve-Guinier-statistics").text()
rg_note = [line for line in note_text.splitlines() if line.endswith("| Rg") and "±" in line][-1]
assert "," in rg_note.split()[0], rg_note

# fitCurve table (reused from the first fit, so it keeps its own locale): values and errors in that locale
fit_table = table("fitCurve-Guinier")
point = fit_table.locale().decimalPoint()
assert fit_table.cell(9, 2) == f.results()["Rg"][0] and point in fit_table.text(9, 2), fit_table.text(9, 2)
assert fit_table.text(10, 2).startswith("±") and point in fit_table.text(10, 2), fit_table.text(10, 2)

# Results -> New Table (col): Numeric values, localized Text errors usable as error bars
QMetaObject.invokeMethod(fw, "newTabResCol")
result = table([n for n in tableNames() if n.startswith("newFitTableColGuinier")][-1])
rg_col = result.colIndex("Rg") + 1
types = (result.columnType(rg_col - 1), result.columnType(rg_col))
assert types == (0, 1), types  # Numeric, Text (0-based)
assert "," in result.text(rg_col, 1) and "," in result.text(rg_col + 1, 1), result.text(rg_col + 1, 1)
layer = newGraph("FitLocaleGraph").activeLayer()
bars = layer.addErrorBars(layer.insertCurve(result, "Rg"), result, "RgError", 1, 1, 8, QColor("black"), True, True,
                          True)
assert abs(bars.errorValue(0) - f.results()["Rg"][1]) < 1e-9 * f.results()["Rg"][1], bars.errorValue(0)

# Uniform simulation: localized table, created hidden and still hidden after a folder switch
f.selectStage("simulate")
f.setSimUniformX(True)
f.setSimFromX(0.0025)
f.setSimToX(0.0525)
f.setSimNumPoints(11)
with quiet():
    f.simulateCurve()
simulated = table("simulatedCurve-Guinier")
assert simulated.text(1, 1) == "2,500000000e-03", simulated.text(1, 1)
x = (f.simFromX(), simulated.cell(1, 11))
assert x == (0.0025, 0.0525), x
root = activeFolder()
changeFolder(addFolder("FitLocaleOther"))
changeFolder(root)
assert not simulated.isVisible(), "simulatedCurve table reappeared after a folder switch"

report("FitTable locale (Guinier, C -> German): widgets converted, same fit, fitReport() C / note localized, "
       "result tables Numeric values + localized errors (error bars OK), simulation table localized and hidden",
       first=True)
