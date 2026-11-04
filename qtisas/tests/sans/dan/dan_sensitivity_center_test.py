"""The sensitivity against the rows reduced with it (the KWS-1 example's Plexiglas 53224 as sensitivity AND as a sample,
parallax correction on; skipped without the qtisas.com files):
- the sensitivity's label keeps the center of its wide-angle corrections, "::Center::x|y|configuration" (the beam
  center of the run's configuration in the configuration table, as its rows use it), "...|mask" (the mask's beam stop,
  when no configuration has the run's) or "...|manual" (the fields' values, center=(x, y) from Python), shown in the
  center fields;
  Dan.Sensitivity.center() reads it back, None for a label without one (made before 2026-10-29);
- the same run with the same transmission and center is flat: I(Q) constant;
- another transmission (the sensitivity tab's Tr against the row's) or another center makes it Q-dependent, and the
  reduction says so (a warning per row, the reduction goes on).
"""

import os
import sys

try:
    from PyQt6.QtWidgets import QCheckBox, QComboBox, QDoubleSpinBox, QGroupBox, QLineEdit
except ImportError:
    from PyQt5.QtWidgets import QCheckBox, QComboBox, QDoubleSpinBox, QGroupBox, QLineEdit

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

RUN, DC = "53224", "48462"

script = dr.kws1(dr.data_folder("kws1", "*.DAT"), samples=False)
script = Dan.Processing.newScriptTable("script", [53224])
t = table(script)
ec = t.text(dr.column(t, "#-EC [EB]"), 1)
tr = 0.9  # any: the sensitivity's and the row's the same
t.setText(dr.column(t, "Transmission-Sample"), 1, "%.4f" % tr)
t.setText(dr.column(t, "Sens"), 1, "sens")
Dan.findChild(QCheckBox, "checkBoxParallax").setChecked(True)
made = []


def curve():
    """I of the row, the warnings."""
    for name in [n for n in made if table(n)]:
        closeWindow(table(name))
    made[:] = Dan.Processing.process(script, "I-Q")
    q_i = table(made[0])
    return [float(q_i.text(2, r)) for r in range(1, q_i.numRows() + 1)], Dan.Processing.lastMessages()


def spread(values):
    return max(values) / min(values) - 1


# the beam center of 53224's configuration (1.98 m): label, fields, center()
Dan.Sensitivity.newSensitivity(RUN, emptyBeam=ec, darkCurrent=DC, transmission=tr)
x, y, source = Dan.Sensitivity.center("sens")
own = [c for c in (Dan.Processing.configuration(i) for i in range(1, Dan.Processing.configurations() + 1))
       if abs(c["D"] - 1.98) < 0.01][0]
assert source == "configuration" and (x, y) == (round(own["xCenter"], 3), round(own["yCenter"], 3)), (source, x, y)
label = matrix("sens").windowLabel()
assert label.endswith("::Tr::%.4f::Center::%.3f|%.3f|configuration" % (tr, x, y)), label
assert not Dan.findChild(QGroupBox, "buttonGroupSensCenter").isChecked()
assert abs(Dan.findChild(QDoubleSpinBox, "doubleSpinBoxXcenterSens").value() - x) < 1e-3
assert abs(Dan.findChild(QDoubleSpinBox, "doubleSpinBoxYcenterSens").value() - y) < 1e-3

intensity, messages = curve()
assert spread(intensity) < 1e-6, spread(intensity)
assert not [m for m in messages if "sensitivity" in m], messages

# another transmission: Q-dependent, a warning
Dan.Sensitivity.newSensitivity(RUN, emptyBeam=ec, darkCurrent=DC, transmission=tr + 0.01)
intensity, messages = curve()
assert spread(intensity) > 1e-4, spread(intensity)
assert [m for m in messages if "same run %s with Tr = %.4f" % (RUN, tr + 0.01) in m], messages

# another center, given: label "manual", Q-dependent, a warning
Dan.Sensitivity.newSensitivity(RUN, emptyBeam=ec, darkCurrent=DC, transmission=tr, center=(x + 3, y))
assert Dan.Sensitivity.center("sens") == (round(x + 3, 3), round(y, 3), "manual"), Dan.Sensitivity.center("sens")
assert Dan.findChild(QGroupBox, "buttonGroupSensCenter").isChecked()
intensity, messages = curve()
assert spread(intensity) > 1e-4, spread(intensity)
assert [m for m in messages if "is made with the center %.2f|%.2f (manual)" % (x + 3, y) in m], messages

# the same center given by hand: flat again, no warning
Dan.Sensitivity.newSensitivity(RUN, emptyBeam=ec, darkCurrent=DC, transmission=tr, center=(x, y))
intensity, messages = curve()
assert spread(intensity) < 1e-6, spread(intensity)
assert not [m for m in messages if "sensitivity" in m], messages

# selecting a sensitivity shows its parameters, the center too; the group unchecked for "configuration"
Dan.Sensitivity.newSensitivity(RUN, emptyBeam=ec, darkCurrent=DC, transmission=tr, center=(x + 3, y), name="sens2")
Dan.Sensitivity.newSensitivity(RUN, emptyBeam=ec, darkCurrent=DC, transmission=tr)
Dan.findChild(QGroupBox, "buttonGroupSensCenter").setChecked(False)
combo = Dan.findChild(QComboBox, "comboBoxSensFor")
combo.setCurrentIndex(combo.findText("sens2"))
combo.activated.emit(combo.currentIndex())
assert Dan.findChild(QGroupBox, "buttonGroupSensCenter").isChecked()
assert abs(Dan.findChild(QDoubleSpinBox, "doubleSpinBoxXcenterSens").value() - (x + 3)) < 1e-3
assert Dan.findChild(QLineEdit, "lineEditTransAnyD").text() == "%.4f" % tr

# a label without the center (an older sensitivity): None, no center warning
m = matrix("sens")
m.setWindowLabel(m.windowLabel().split("::Center::")[0])
assert Dan.Sensitivity.center("sens") is None
intensity, messages = curve()
assert not [m for m in messages if "center" in m], messages
