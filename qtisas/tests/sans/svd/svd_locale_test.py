"""SVD interface follows the numerical format; its table and settings keep the numbers.

The same contrast-variation input (2 components, 3 H/D mixtures) is run once in C and once in German: the SVD
tables must hold the same numbers. Reading an SVD table back fills the interface in German, and the settings are
written in C. writeSettings() changes the user's qtisas.ini, which is restored at the end.
"""

import math
import os
import sys

try:
    from PyQt6.QtCore import Q_ARG, QMetaObject
    from PyQt6.QtWidgets import QApplication, QLineEdit, QSpinBox, QTableWidget, QTableWidgetItem
except ImportError:
    from PyQt5.QtCore import Q_ARG, QMetaObject
    from PyQt5.QtWidgets import QApplication, QLineEdit, QSpinBox, QTableWidget, QTableWidgetItem

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import DIRECT, accept_text, answering, locale_c, locale_german, set_app_locale, user_settings_kept

w = [w for w in QApplication.allWidgets() if w.metaObject().className() == "svd"][0]
child = lambda cls, name: w.findChild(cls, name)
rhoH, rhoD = child(QLineEdit, "lineEditRhoH"), child(QLineEdit, "lineEditRhoD")
tableM, tableN = child(QTableWidget, "tableMsvd"), child(QTableWidget, "tableNsvd")

# three H/D mixtures of two components: I = sum of (rho_i - rho_solvent)(rho_j - rho_solvent) S_ij, K = 20 points
H, D = -0.56e10, 6.34e10
fractions = [0.0, 0.5, 1.0]
components = [("A", 1.0e10, 3.0e10), ("B", 4.0e10, 5.0e10)]
q = [0.01 * (k + 1) for k in range(20)]
for m, x in enumerate(fractions):
    solvent = H + x * (D - H)
    rho = [h + x * (d - h) - solvent for _, h, d in components]
    t = newTable("SVDdata%d" % (m + 1), len(q), 3)
    intensity = [rho[0] ** 2 * math.exp(-v) + 2 * rho[0] * rho[1] * math.exp(-2 * v) + rho[1] ** 2 for v in q]
    t.setColData(1, q)
    t.setColData(2, intensity)
    t.setColData(3, [0.01 * i for i in intensity])


def fill():
    """The input typed in the current format."""
    locale = qti.app.locale()
    number = lambda v: locale.toString(v, "e", 5)
    rhoH.setText(number(H))
    rhoD.setText(number(D))
    child(QSpinBox, "spinBoxNsvd").setValue(2)
    child(QSpinBox, "spinBoxMsvd").setValue(3)
    child(QSpinBox, "spinBoxKsvd").setValue(len(q))
    for row, (name, h, d) in enumerate(components):
        for col, text in enumerate((name, number(h), number(d))):
            tableN.setItem(row, col, QTableWidgetItem(text))
    for row, x in enumerate(fractions):
        tableM.setItem(row, 2, QTableWidgetItem("SVDdata%d" % (row + 1)))
        tableM.setItem(row, 0, QTableWidgetItem(number(x)))  # cellChanged fills the solvent SLD


def transfer(name):
    """Run the SVD, answering its table-name dialog with name."""
    with answering(accept_text(name)) as messages:
        QMetaObject.invokeMethod(w, "slotSVD1dataTransfer")
    assert not messages, messages
    return table(name)


set_app_locale(locale_c())
fill()
shown = lambda: (rhoH.text(), tableM.item(1, 1).text())  # solvent SLD of the 50% mixture, filled by the interface
assert shown() == ("-5.60000e+09", "2.89000e+10"), shown()
svd_c = transfer("SVDlocaleC")

set_app_locale(locale_german())
assert shown() == ("-5,60000e+09", "2,89000e+10"), shown()  # converted on the format change
fill()
svd_de = transfer("SVDlocaleDE")

assert svd_c.numCols() == svd_de.numCols(), (svd_c.numCols(), svd_de.numCols())
for col in range(1, svd_c.numCols() + 1):
    if col in (1, 4):  # Comment, TableName
        continue
    for row in range(1, svd_c.numRows() + 1):
        c_value, de_value = svd_c.cellOrNone(col, row), svd_de.cellOrNone(col, row)
        assert c_value == de_value, (svd_c.colName(col), row, c_value, de_value)
known = (svd_de.cellOrNone(2, 1), svd_de.cellOrNone(5, 2))  # solvent SLD in H, H/D fraction of the 2nd mixture
assert known == (H, 0.5), known
# and the SVD recovers the partial structure factors of the data: S11 = exp(-q), S12 = exp(-2q), S22 = 1
names = svd_de.colNames()
for name, expected in (("S11", math.exp(-q[4])), ("S12", math.exp(-2 * q[4])), ("S22", 1.0)):
    got = svd_de.cellOrNone(names.index(name) + 1, 5)
    assert got is not None and abs(got - expected) < 1e-6 * abs(expected), (name, got, expected)

# the SVD table back into the German interface
rhoH.setText("")
QMetaObject.invokeMethod(w, "slotReadSvdData", DIRECT, Q_ARG(str, "SVDlocaleC"))
read = (rhoH.text(), tableN.item(1, 2).text())
assert read == ("-5,60000e+09", "5,00000e+10"), read

# settings in C
with user_settings_kept():
    QMetaObject.invokeMethod(w, "writeSettings", DIRECT)
    ini = os.path.join(os.path.expanduser("~"), ".config", "qtisas", "qtisas.ini")
    group = open(ini, encoding="utf-8").read().split("[SVD]")[1].split("\n[")[0]
    assert "RhoH=-5.60000e+09" in group and "svdSLDinD=3.00000e+10, 5.00000e+10" in group, group
