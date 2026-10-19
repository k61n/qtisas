"""SVD of a contrast series: the partial structure factors, the reconstructed intensities, the input checks, the
read-back.

The contrast series of qtisas.com/svd (2 components, 5 H/D mixtures) with known S11, S12, S22: the SVD table holds
them, the solvent terms S13, S23, S33 and the intensities Icon = A S of every mixture. Input that cannot be
decomposed makes no table and says why; Q points that differ between the data tables give a warning. Reading the
table back restores the numbers of components, mixtures and Q points, also with unnamed components.
"""

import math
import os
import sys

try:
    from PyQt6.QtCore import Q_ARG, QMetaObject
    from PyQt6.QtWidgets import QApplication, QComboBox, QLineEdit, QSpinBox, QTableWidget, QTableWidgetItem
except ImportError:
    from PyQt5.QtCore import Q_ARG, QMetaObject
    from PyQt5.QtWidgets import QApplication, QComboBox, QLineEdit, QSpinBox, QTableWidget, QTableWidgetItem

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import DIRECT, accept_text, answering, locale_c, set_app_locale

w = [w for w in QApplication.allWidgets() if w.metaObject().className() == "svd"][0]
child = lambda cls, name: w.findChild(cls, name)
tableM, tableN = child(QTableWidget, "tableMsvd"), child(QTableWidget, "tableNsvd")
spinN, spinM, spinK = (child(QSpinBox, name) for name in ("spinBoxNsvd", "spinBoxMsvd", "spinBoxKsvd"))

H, D = -5.61e9, 6.335e10
fractions = [0.0, 0.34, 0.46, 0.8, 1.0]
components = [("a", 1.48e10, 2.374e10), ("b", 2.11e10, 3.199e10)]
q = [0.005 * (k + 1) for k in range(20)]
S11 = [1 / (1 + (50 * v) ** 2) for v in q]
S12 = [0.3 * math.exp(-((20 * v) ** 2)) for v in q]
S22 = [math.exp(-((30 * v) ** 2)) for v in q]


def contrast(name, x, shift=1.0):
    """The data table of an H/D mixture: Q (scaled by shift), I, dI."""
    solvent = H + x * (D - H)
    a, b = (h + x * (d - h) - solvent for _, h, d in components)
    intensity = [a * a * s11 + 2 * a * b * s12 + b * b * s22 for s11, s12, s22 in zip(S11, S12, S22)]
    t = newTable(name, len(q), 3)
    t.setColData(1, [shift * v for v in q])
    t.setColData(2, intensity)
    t.setColData(3, [0.01 * i for i in intensity])
    return intensity


set_app_locale(locale_c())
intensities = [contrast("SVDweb%d" % m, x) for m, x in enumerate(fractions)]


def fill(names=("SVDweb0", "SVDweb1", "SVDweb2", "SVDweb3", "SVDweb4"), labels=("a", "b"), k=len(q)):
    child(QLineEdit, "lineEditRhoH").setText("%.5e" % H)
    child(QLineEdit, "lineEditRhoD").setText("%.5e" % D)
    spinN.setValue(2)
    spinM.setValue(len(names))
    spinK.setValue(k)
    for row, (label, (_, h, d)) in enumerate(zip(labels, components)):
        for col, text in enumerate((label, "%.5e" % h, "%.5e" % d)):
            tableN.setItem(row, col, QTableWidgetItem(text))
    for row, (name, x) in enumerate(zip(names, fractions)):
        tableM.setItem(row, 2, QTableWidgetItem(name))
        tableM.setItem(row, 0, QTableWidgetItem("%.5e" % x))  # cellChanged fills the solvent SLD


def transfer(name):
    """Run the SVD, answering its table-name dialog with name: the texts of the message boxes shown."""
    with answering(accept_text(name)) as messages:
        QMetaObject.invokeMethod(w, "slotSVD1dataTransfer")
    return messages


def column(t, name):
    return [t.cellOrNone(t.colNames().index(name) + 1, row + 1) for row in range(len(q))]


def close(got, expected, what):
    for row, (g, e) in enumerate(zip(got, expected)):
        assert g is not None and abs(g - e) <= 1e-5 * abs(e) + 1e-12 * max(map(abs, expected)), (what, row, g, e)


# the decomposition: S_ij, the solvent's terms (Babinet), and the intensities reconstructed from them
fill()
assert transfer("SVDweb") == []
t = table("SVDweb")
close(column(t, "S11"), S11, "S11")
close(column(t, "S12"), S12, "S12")
close(column(t, "S22"), S22, "S22")
close(column(t, "S13"), [-a - b for a, b in zip(S11, S12)], "S13")
close(column(t, "S23"), [-a - b for a, b in zip(S22, S12)], "S23")
close(column(t, "S33"), [a + b + 2 * c for a, b, c in zip(S11, S22, S12)], "S33")
for m, intensity in enumerate(intensities):
    close(column(t, "Icon%d" % (m + 1)), intensity, "Icon%d" % (m + 1))
assert child(QComboBox, "comboBoxSVDtables").currentText() == "SVDweb"

# input that cannot be decomposed: no table, the reason shown
fill(names=("SVDweb0", "SVDweb1"))  # 2 mixtures for 3 partial structure factors
messages = transfer("SVDtooFew")
assert len(messages) == 1 and "at least 3 H/D contrasts" in messages[0], messages
assert not existTable("SVDtooFew")

fill(names=("SVDweb0", "SVDweb1", "SVDmissing", "SVDweb3", "SVDweb4"))
messages = transfer("SVDmissing1")
assert len(messages) == 1 and '"SVDmissing" of contrast 3 does not exist' in messages[0], messages
assert not existTable("SVDmissing1")

fill(k=len(q) + 5)
messages = transfer("SVDtooLong")
assert len(messages) == 1 and "has 20 Q points, fewer than the 25 used" in messages[0], messages
assert not existTable("SVDtooLong")

# Q points that differ: the table is made, with a warning
contrast("SVDshifted", fractions[3], shift=1.05)
fill(names=("SVDweb0", "SVDweb1", "SVDweb2", "SVDshifted", "SVDweb4"))
messages = transfer("SVDshiftedQ")
assert len(messages) == 1 and 'the Q points of "SVDshifted" differ from "SVDweb0"' in messages[0], messages
assert existTable("SVDshiftedQ")

# read back: unnamed components counted, the mixtures and Q points restored
fill(labels=("", ""))
messages = transfer("SVDunnamed")
assert messages == [], messages
spinN.setValue(3)
spinM.setValue(2)
spinK.setValue(7)
QMetaObject.invokeMethod(w, "slotReadSvdData", DIRECT, Q_ARG(str, "SVDunnamed"))
restored = (spinN.value(), spinM.value(), spinK.value(), tableN.item(1, 2).text(), tableM.item(4, 2).text())
assert restored == (2, 5, 20, "3.19900e+10", "SVDweb4"), restored
