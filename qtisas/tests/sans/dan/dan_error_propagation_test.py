"""Error bars of DAN's I(Q) (the KWS-1 example; skipped without the qtisas.com files):
- every presentation (Guinier, Zimm, Porod, ..., GuinierRod, GuinierPlate) is the QI table transformed, its dI that
  of the transformed quantity (before: Debye had dI twice too large, GuinierRod and GuinierPlate dI multiplied by Q
  and Q^2, logQ wrote undefined I and dI);
- a constant background (the script's "Background" column) shifts I and keeps dI (before: the relative error was
  kept, so dI shrank with I - c);
- HF averaging: a pixel sampled several times on a ring counts with its total weight (dI as the standard averaging's,
  before about 1.75x smaller);
- the transmission error of 1D outputs once per point, dI^2 = dI_stat^2 + (eps I)^2, not in the pixel variance;
- the absolute factor with its statistical error, "F±s" / "F [ ±s ]" (an old plain "F" read the same), its relative
  error added to eps.
"""

import math
import os
import sys

try:
    from PyQt6.QtWidgets import QComboBox
except ImportError:
    from PyQt5.QtWidgets import QComboBox

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

LN10 = math.log(10.0)
# presentation: (x, y, dy) from (Q, I, dI)
EXPECTED = {
    "Guinier": lambda q, i, d: (q * q, math.log(abs(i)), d / abs(i)),
    "Zimm": lambda q, i, d: (q * q, 1 / i, d / (i * i)),
    "Porod": lambda q, i, d: (math.log10(q), math.log10(abs(i)), d / abs(i) / LN10),
    "Porod2": lambda q, i, d: (q * q, i * q ** 4, d * q ** 4),
    "logQ": lambda q, i, d: (math.log10(q), i, d),
    "logI": lambda q, i, d: (q, math.log10(abs(i)), d / abs(i) / LN10),
    "Debye": lambda q, i, d: (q * q, abs(i) ** -0.5, 0.5 * d * abs(i) ** -1.5),
    "1Moment": lambda q, i, d: (q, q * i, q * d),
    "2Moment": lambda q, i, d: (q, q * q * i, q * q * d),
    "Kratky": lambda q, i, d: (q, q * q * i, q * q * d),
    "GuinierRod": lambda q, i, d: (q * q, math.log(abs(i * q)), d / abs(i)),
    "GuinierPlate": lambda q, i, d: (q * q, math.log(abs(i * q * q)), d / abs(i)),
}


def rows(name):
    t = table(name)
    return [tuple(float(t.text(c, r)) for c in (1, 2, 3)) for r in range(1, t.numRows() + 1)]


def close(a, b, rel=2e-6):
    return abs(a - b) <= rel * max(abs(a), abs(b), 1e-12)


script = dr.kws1(dr.data_folder("kws1", "*.DAT"))
presentation = Dan.findChild(QComboBox, "comboBoxSelectPresentation")
presentation.setCurrentText("QI")
made = Dan.Processing.process(script, "I-Q")
reference = made[0]  # QI-SM-53217-H-J
qi = rows(reference)

for label, expected in EXPECTED.items():
    presentation.setCurrentText(label)
    name = Dan.Processing.process(script, "I-Q")[0]
    assert name == reference.replace("QI-", "QI-%s-" % label, 1), name
    got = rows(name)
    assert len(got) == len(qi), (label, len(got), len(qi))
    for (q, i, d), point in zip(qi, got):
        want = expected(q, i, d)
        assert all(close(a, b) for a, b in zip(point, want)), (label, (q, i, d), point, want)
presentation.setCurrentText("QI")

# constant background: I - c, dI kept
t = table(script)
t.addColumns(1)
t.setColName(t.numCols(), "Background")
c = 0.001
for r in range(1, t.numRows() + 1):
    t.setText(t.numCols(), r, str(c))
for name in made:  # process() returns the tables it made: the old ones closed first
    closeWindow(table(name))
shifted = rows(Dan.Processing.process(script, "I-Q")[0])
assert len(shifted) == len(qi), (len(shifted), len(qi))
for (q0, i0, d0), (q, i, d) in zip(qi, shifted):
    # dI: its statistical part is kept; the transmission part (once per point) follows I, here by c / I < 1e-4
    assert close(q, q0) and abs(i - (i0 - c)) <= 2e-6 * max(abs(i0), c) and close(d, d0, 1e-4), \
        ((q0, i0, d0), (q, i, d))

# HF averaging (bilinear samples on each ring): a pixel sampled several times counts with its total weight, so its
# dI is of the size of the standard averaging's (before: about 1.75x smaller, each sample taken as independent)
try:
    from PyQt6.QtWidgets import QRadioButton
except ImportError:
    from PyQt5.QtWidgets import QRadioButton
t.setNumCols(t.numCols() - 1)  # no background
errors = {}
for mode in ("radioButtonRadStd", "radioButtonRadHF"):
    Dan.findChild(QRadioButton, mode).setChecked(True)
    for name in [n for n in made if table(n)]:
        closeWindow(table(name))
    curve = rows(Dan.Processing.process(script, "I-Q")[0])
    errors[mode] = {round(q, 6): d for q, i, d in curve}
ratios = sorted(errors["radioButtonRadHF"][q] / errors["radioButtonRadStd"][q]
                for q in set(errors["radioButtonRadHF"]) & set(errors["radioButtonRadStd"])
                if errors["radioButtonRadStd"][q] > 0)
assert len(ratios) > 50 and 0.65 < ratios[len(ratios) // 2] < 1.2, (len(ratios), ratios[len(ratios) // 2])

# transmission error, 1D: not in the pixel variance (it would shrink with the number of pixels) but once per point,
# dI^2 = dI_stat^2 + (eps I)^2, eps = kappa sigmaT/T >= sigmaT/T (kappa: the sample's share before the EC subtraction)
Dan.findChild(QRadioButton, "radioButtonRadStd").setChecked(True)
column = list(t.colNames()).index("Transmission-Sample") + 1
original = t.text(column, 1)
tr = float(original.replace("±", "").replace("[", " ").replace("]", " ").split()[0])
curves = {}
for relative in (0.0, 0.01):
    t.setText(column, 1, "%.6f [ ±%.6f ]" % (tr, relative * tr))
    for name in [n for n in made if table(n)]:
        closeWindow(table(name))
    curves[relative] = rows(Dan.Processing.process(script, "I-Q")[0])
t.setText(column, 1, original)
eps = [math.sqrt(max(d1 * d1 - d0 * d0, 0.0)) / abs(i1)
       for (q0, i0, d0), (q1, i1, d1) in zip(curves[0.0], curves[0.01]) if i1 != 0]
assert all(close(i0, i1) for (q0, i0, d0), (q1, i1, d1) in zip(curves[0.0], curves[0.01]))  # I unchanged
assert min(eps) >= 0.01 * (1 - 1e-4) and max(eps) - min(eps) <= 1e-6, (min(eps), max(eps))  # one eps per row

# absolute factor with its statistical error (the calibrant's counts, not the sensitivity's): "F±s" in the
# configuration table, "F [ ±s ]" in the script's Factor column (Text); an old plain "F" is read the same way
configuration = Dan.Processing.configuration(1)
assert configuration["factorError"] > 0 and configuration["factorError"] < 1e-2 * configuration["factor"], configuration
factorColumn = list(t.colNames()).index("Factor") + 1
text = t.text(factorColumn, 1)
assert text == "%.4E [ ±%.2E ]" % (configuration["factor"], configuration["factorError"]), text
for name in [n for n in made if table(n)]:
    closeWindow(table(name))
withError = rows(Dan.Processing.process(script, "I-Q")[0])
for r in range(1, t.numRows() + 1):  # the old format: the number only
    t.setText(factorColumn, r, t.text(factorColumn, r).split("[")[0].strip())
for name in [n for n in made if table(n)]:
    closeWindow(table(name))
plain = rows(Dan.Processing.process(script, "I-Q")[0])
assert all(close(a[1], b[1]) for a, b in zip(withError, plain)), "the factor read differently"

# the factor's error is common too: with "F [ ±1 % ]" in a row (plain F before), dI^2 grows by (0.01 I)^2
value = float(t.text(factorColumn, 1).split("[")[0])
t.setText(factorColumn, 1, "%.4E [ ±%.2E ]" % (value, 0.01 * value))
for name in [n for n in made if table(n)]:
    closeWindow(table(name))
withFactorError = rows(Dan.Processing.process(script, "I-Q")[0])
eps = [math.sqrt(max(d1 * d1 - d0 * d0, 0.0)) / abs(i1) for (q0, i0, d0), (q1, i1, d1) in zip(plain, withFactorError)
       if i1 != 0]
assert all(abs(e - 0.01) <= 1e-4 for e in eps), (min(eps), max(eps))
