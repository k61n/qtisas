"""Sensitivity tab, its Options page and the Transmission and Save buttons, from Python as from the interface (the
KWS-1 example's Plexiglas 53224 with its empty beam 53225 and B4C 48462; skipped without the qtisas.com files):
- transmission="measured" is the Transmission button (= Dan.Sensitivity.transmission()), "formula" the button with
  "Calculate Transmission by Formula T(lambda)": the instrument calibrant's To - TA exp(-lambdaT / lambda);
- errorRange drops the pixels of a larger relative error from the sensitivity AND masks them in the mask (as the
  interface does, with a message), the mask keeping its own label; errorMatrix makes "<sens>Error"; maskedValue is
  the sensitivity of the masked pixels;
- saveSensitivity() writes the file loadSensitivity() reads back;
- the mask asked for is the one used, also when the call comes from the Mask tab (before: "mask").
"""

import math
import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtWidgets import QLineEdit
except ImportError:
    from PyQt5.QtWidgets import QLineEdit

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

sens = Dan.Sensitivity
RUN, EB, DC = "53224", "53225", "48462"


def raises(call, text, error=ValueError):
    try:
        call()
    except error as e:
        assert text in str(e), (text, str(e))
        return
    raise AssertionError("no %s: %s" % (error.__name__, text))


def field(name):
    return Dan.findChild(QLineEdit, name).text()


def cells(name):
    m = matrix(name)
    return [m.cell(r, c) for r in range(1, m.numRows() + 1) for c in range(1, m.numCols() + 1)]


dr.new_session("KWS1-2020", dr.data_folder("kws1", "*.DAT"))
Dan.Mask.newMask()
labelBefore = matrix("mask").windowLabel()

# transmission: measured (the button) and by the calibrant's formula
sens.newSensitivity(RUN, "s-measured", EB, DC, transmission="measured", errorRange=(0, 100), maskedValue=0)
tr, dTr = sens.transmission(RUN, EB, "mask")
assert field("lineEditTransAnyD") == "%.4f" % tr, (field("lineEditTransAnyD"), tr)
sens.newSensitivity(RUN, "s-formula", EB, DC, transmission="formula")
lam = Dan.Rawdata.info([RUN])[0]["lambda"]
to, ta, late = (float(field(n)) for n in ("lineEditTo", "lineEditTA", "lineEditLate"))
assert field("lineEditTransAnyD") == "%.4f" % (to - ta * math.exp(-late / lam)), (field("lineEditTransAnyD"), lam)
sens.newSensitivity(RUN, "s-number", EB, DC, transmission=0.5)
assert field("lineEditTransAnyD") == "0.5000"

# errorRange: noisy pixels out of the sensitivity and into the mask; the mask keeps its label
maskedBefore = cells("mask").count(0)
sens.newSensitivity(RUN, "s-range", EB, DC, transmission="measured", errorRange=(0, 3))
added = cells("mask").count(0) - maskedBefore
assert added > 0, added
assert any("mask mask: %d pixels with errors outside 0..3 %% masked" % added in m for m in sens.lastMessages()), \
    sens.lastMessages()
assert matrix("mask").windowLabel() == labelBefore, (matrix("mask").windowLabel(), labelBefore)
assert cells("s-range").count(0) == cells("mask").count(0)  # masked pixels: sensitivity 0
Dan.Mask.newMask()  # the mask anew, without them

# errorMatrix, maskedValue
sens.newSensitivity(RUN, "s-err", EB, DC, errorRange=(0, 100), errorMatrix=True, maskedValue=0.5)
assert "s-errError" in matrixNames()
values = cells("s-err")
assert values.count(0.5) == cells("mask").count(0) and values.count(0) == 0, (values.count(0.5), values.count(0))
sens.newSensitivity(RUN, "s-err", EB, DC, errorMatrix=False, maskedValue=0)

# the mask asked for, also from the Mask tab
Dan.Mask.newMask("mask-big", beamstop=(40, 100, 40, 100))
Dan.Mask.newMask("mask-other", beamstop=(60, 80, 60, 80))  # the Mask tab is open now
sens.newSensitivity(RUN, "s-big", EB, DC, mask="mask-big")
counts = [cells(n).count(0) for n in ("s-big", "mask-big", "mask")]
assert counts[0] == counts[1] > counts[2], counts

# saveSensitivity
folder = tempfile.mkdtemp()
try:
    path = sens.saveSensitivity("s-number", os.path.join(folder, "s.sens"))
    assert path == os.path.join(folder, "s.sens") and os.path.isfile(path), path
    sens.loadSensitivity(path, "s-back")
    assert all(abs(a - b) <= 1e-6 * max(abs(a), 1) for a, b in zip(cells("s-back"), cells("s-number")))
    raises(lambda: sens.saveSensitivity("mask", path), "no sensitivity mask")
    raises(lambda: sens.saveSensitivity("s-number", os.path.join(folder, "no", "s.sens")), "no folder")
finally:
    shutil.rmtree(folder)

# wrong arguments
raises(lambda: sens.newSensitivity(RUN, "s-x", EB, transmission="guessed"), "a number, \"measured\" or \"formula\"")
raises(lambda: sens.newSensitivity(RUN, "s-x", transmission="measured"), "the empty beam is needed")
raises(lambda: sens.newSensitivity(RUN, "s-x", EB, transmission=1.5), "0 < Tr <= 1")
raises(lambda: sens.newSensitivity(RUN, "s-x", EB, errorRange=(5, 2)), "0 <= min < max <= 100")
raises(lambda: sens.newSensitivity(RUN, "s-x", EB, errorRange=5), "errorRange: (a, b)", TypeError)
raises(lambda: sens.newSensitivity(RUN, "s-x", EB, maskedValue=-1), "maskedValue: >= 0")
raises(lambda: sens.newSensitivity(RUN, "s-x", EB, transmission=True), "transmission: a number", TypeError)
