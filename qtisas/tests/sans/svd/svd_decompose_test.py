"""SVD of a contrast series: the partial structure factors, the reconstructed intensities, the input checks, the
read-back.

The contrast series of qtisas.com/svd (2 components, 5 H/D mixtures) with known S11, S12, S22: the SVD table holds
them, the solvent terms S13, S23, S33, the intensities Icon = A S of every mixture, and the errors propagated from
dI through the pseudo-inverse of A - large for mixtures near the match points. Input that cannot be decomposed makes
no table and says why; Q points that differ between the data tables give a warning. Reading the
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


def fill(names=("SVDweb0", "SVDweb1", "SVDweb2", "SVDweb3", "SVDweb4"), labels=("a", "b"), k=len(q), xs=fractions):
    child(QLineEdit, "lineEditRhoH").setText("%.5e" % H)
    child(QLineEdit, "lineEditRhoD").setText("%.5e" % D)
    spinN.setValue(2)
    spinM.setValue(len(names))
    spinK.setValue(k)
    for row, (label, (_, h, d)) in enumerate(zip(labels, components)):
        for col, text in enumerate((label, "%.5e" % h, "%.5e" % d)):
            tableN.setItem(row, col, QTableWidgetItem(text))
    for row, (name, x) in enumerate(zip(names, xs)):
        tableM.setItem(row, 2, QTableWidgetItem(name))
        tableM.setItem(row, 0, QTableWidgetItem("%.5e" % x))  # cellChanged fills the solvent SLD


def transfer(name):
    """Run the SVD, answering its table-name dialog with name: the texts of the message boxes shown."""
    with answering(accept_text(name)) as messages:
        QMetaObject.invokeMethod(w, "slotSVD1dataTransfer")
    return messages


def column(t, name):
    return [t.cellOrNone(t.colNames().index(name) + 1, row + 1) for row in range(len(q))]


def expected_errors(xs, intensities):
    """dS11, dS12, dS22, dS13, dS23, dS33 of every Q: Cov = P diag(dI^2) P^T with P = (A^T A)^-1 A^T, dI = 0.01 I."""
    rows = []
    for x in xs:
        solvent = H + x * (D - H)
        a, b = (h + x * (d - h) - solvent for _, h, d in components)
        rows.append([a * a, 2 * a * b, b * b])
    n = [[sum(r[i] * r[j] for r in rows) for j in range(3)] for i in range(3)]  # A^T A
    det = (n[0][0] * (n[1][1] * n[2][2] - n[1][2] * n[2][1]) - n[0][1] * (n[1][0] * n[2][2] - n[1][2] * n[2][0])
           + n[0][2] * (n[1][0] * n[2][1] - n[1][1] * n[2][0]))
    inv = [[(n[(j + 1) % 3][(i + 1) % 3] * n[(j + 2) % 3][(i + 2) % 3]
             - n[(j + 1) % 3][(i + 2) % 3] * n[(j + 2) % 3][(i + 1) % 3]) / det for j in range(3)] for i in range(3)]
    pinv = [[sum(inv[i][j] * r[j] for j in range(3)) for r in rows] for i in range(3)]
    terms = [(1, 0, 0), (0, 1, 0), (0, 0, 1), (-1, -1, 0), (0, -1, -1), (1, 2, 1)]  # S11..S22, S13, S23, S33
    errors = []
    for k in range(len(q)):
        dI2 = [(0.01 * intensity[k]) ** 2 for intensity in intensities]
        cov = [[sum(pinv[i][m] * pinv[j][m] * dI2[m] for m in range(len(xs))) for j in range(3)] for i in range(3)]
        errors.append([math.sqrt(sum(g[i] * g[j] * cov[i][j] for i in range(3) for j in range(3))) for g in terms])
    return errors


def check_errors(t, xs, intensities):
    expected = expected_errors(xs, intensities)
    for nn, name in enumerate(("S11", "S12", "S22", "S13", "S23", "S33")):
        got = column(t, "d" + name)
        for row in range(len(q)):
            e = expected[row][nn]
            assert got[row] is not None and abs(got[row] - e) <= 1e-4 * e, ("d" + name, row, got[row], e)
    return expected


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
check_errors(t, fractions, intensities)

# mixtures near the match points of both components: the same S_ij, but errors larger than the values
near = [0.6, 0.62, 0.64, 0.66, 0.68]
near_intensities = [contrast("SVDnear%d" % m, x) for m, x in enumerate(near)]
fill(names=["SVDnear%d" % m for m in range(5)], xs=near)
assert transfer("SVDnear") == []
t = table("SVDnear")
close(column(t, "S11"), S11, "S11 near")
errors = check_errors(t, near, near_intensities)
assert errors[0][0] > S11[0] and errors[0][2] > S22[0], errors[0]

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
