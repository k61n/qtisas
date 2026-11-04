"""The detector's dead-time correction c = n/m (true over measured count rate) of a whole image, both models (KWS-3
example runs; skipped without the qtisas.com example files): a row without dark current and empty cell (and a fixed
transmission), its image with the dead time tau over the one without.

- "Paralysable:" N = n exp(-n tau): c solves c exp(-c m tau) = 1, so m = ln(c) / (c tau) is the same for every tau.
  The root took one Newton step only (the loop waited for GSL_CONTINUE, which gsl_root_fdfsolver_iterate() does not
  return; the fdf function returned the derivative): c too small at high rates, 12 % at m tau = 0.33.
- "Non-Paralysable:" N = n / (1 + n tau): c = 1 / (1 - m tau), m = (1 - 1/c) / tau.
- A session keeps the model by its formula; an older session's "Non-Paralysable" is the exp model.
"""

import math
import os
import sys

try:
    from PyQt6.QtWidgets import QLineEdit, QRadioButton
except ImportError:
    from PyQt5.QtWidgets import QLineEdit, QRadioButton

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
RUN = "00082561"  # H-L at 9.2 m

dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
p.autoConfig("dead-time-base")
deadTime = Dan.findChild(QLineEdit, "lineEditDeadTime")
exponential = Dan.findChild(QRadioButton, "radioButtonDeadTimeCh")
linear = Dan.findChild(QRadioButton, "radioButtonDeadTimeDet")
assert exponential.text() == "Paralysable:" and linear.text() == "Non-Paralysable:"


tables = []


def image_sum(tau):
    """The sum of the row's 2D image with the detector dead time tau."""
    deadTime.setText(repr(tau))
    tables.append("dead-time-%d" % len(tables))
    s = p.newScriptTable(tables[-1], [RUN])
    t = table(s)
    cn = list(t.colNames())
    for column in ("#-BC", "#-EC [EB]"):
        t.setText(cn.index(column) + 1, 1, "")
    # a new table measures the transmission (direct beam, its own dead time with the model): fixed here
    t.setText(cn.index("Transmission-Sample") + 1, 1, "0.5")
    made = [n for n in p.process(s, "I-x-y") if "-%s-" % RUN in n]
    assert made, p.lastMessages()
    m = matrix(made[0])
    total = sum(m.cell(i, j) for i in range(1, m.numRows() + 1) for j in range(1, m.numCols() + 1))
    closeWindow(m)
    return total


try:
    exponential.setChecked(True)
    plain = image_sum(0.0)
    rate = (image_sum(1e-7) / plain - 1) / 1e-7  # m, to first order
    taus = [x / rate for x in (0.05, 0.2, 0.33)]
    rates = []
    for tau in taus:
        c = image_sum(tau) / plain
        rates.append(math.log(c) / (c * tau))
    assert all(math.isclose(r, rates[0], rel_tol=1e-4) for r in rates), (rate, rates)

    linear.setChecked(True)
    rates = []
    for tau in taus:
        c = image_sum(tau) / plain
        rates.append((1 - 1 / c) / tau)
    assert all(math.isclose(r, rates[0], rel_tol=1e-4) for r in rates), (rate, rates)
finally:
    exponential.setChecked(True)
    deadTime.setText("1.0e-10")

# the session keeps the model, the other one too (a session's "Paralysable" left the button that was checked)
linear.setChecked(True)
p.newScriptTable("dead-time-session", [RUN])
exponential.setChecked(True)
Dan.Options.openSession("dead-time-session")
assert linear.isChecked()
settings = table("dead-time-session-Settings")
models = [settings.text(2, r) for r in range(1, settings.numRows() + 1)
          if settings.text(1, r) == "Options::2D::DeadTimeModel"]
assert models == ["Non-Paralysable: N = n/[1+n tau] <"], models
# an older session: "Non-Paralysable" was the exp model
row = [settings.text(1, r) for r in range(1, settings.numRows() + 1)].index("Options::2D::DeadTimeModel") + 1
settings.setText(2, row, "Non-Paralysable <")
linear.setChecked(True)
Dan.Options.openSession("dead-time-session")
assert exponential.isChecked()
