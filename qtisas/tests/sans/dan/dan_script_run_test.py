"""Dan's Python calls open nothing modal: what DAN would show in a message box is the ValueError of a call that made
nothing (or a warning in lastMessages()), a missing name or file too. The interface itself asks as before. A mask made
after the configurations is in their lists at once."""

import os
import sys

try:
    from PyQt6.QtCore import QMetaObject
    from PyQt6.QtWidgets import QComboBox, QLineEdit, QSpinBox, QTableWidget, QTabWidget
except ImportError:
    from PyQt5.QtCore import QMetaObject
    from PyQt5.QtWidgets import QComboBox, QLineEdit, QSpinBox, QTableWidget, QTabWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import DIRECT, answering, quiet


def raises(call, text, error=ValueError):
    try:
        call()
    except error as e:
        assert text in str(e), (text, str(e))
        return
    raise AssertionError("no %s: %s" % (error.__name__, text))


with quiet():
    Dan.Options.newSession()
    Dan.Options.selectInstrument("KWS1-2020")

with answering() as boxes:  # would collect (and close) any message box
    raises(lambda: Dan.Options.selectInstrument("no-such-instrument"), "no instrument no-such-instrument")
    raises(lambda: Dan.Processing.newScriptTable("script", ["no-such-run"]), "no run no-such-run")
    raises(lambda: Dan.Sensitivity.newSensitivity("no-such-run"), "no run no-such-run")
    raises(lambda: Dan.Sensitivity.loadSensitivity("/no/such/file.sens"), "no file /no/such/file.sens")
    raises(lambda: Dan.Mask.newMask("m", edge=(1, 2)), "x1, x2, y1, y2", TypeError)
    raises(lambda: Dan.Processing.setConfiguration(9), "configuration 9 of 1..")
assert boxes == [], boxes

with answering() as boxes:  # outside a Python call the interface asks as before
    Dan.findChild(QLineEdit, "lineEditPlexiAnyD").setText("no-such-run")
    QMetaObject.invokeMethod(Dan, "createSens", DIRECT)
assert boxes and "check sensitivity fields" in boxes[0], boxes

# a mask made after the configurations is in their lists at once (before: only after a header press)
Dan.Processing.setConfigurations(2)
assert Dan.Mask.newMask("mask-new", beamstop=(60, 80, 60, 80)) == "mask-new"
assert "mask-new" in Dan.Mask.masks(), Dan.Mask.masks()
t = Dan.findChild(QTableWidget, "tableEC")
for row in (17, 21):
    for col in range(2):
        box = t.cellWidget(row, col)
        names = [box.itemText(i) for i in range(box.count())]
        assert "mask-new" in names, (row, col, names)
Dan.Processing.setConfiguration(2, mask="mask-new")
assert Dan.Processing.configuration(2)["mask"] == "mask-new"

# each part's action shows its tab, as a click would (Options 0, Rawdata 1, Mask 2, Sensitivity 3, Processing 4,
# Merge 5); a query (masks(), runs(), configuration()) changes nothing and leaves the tab as it is
tabs = Dan.findChild(QTabWidget, "sansTab")
Dan.Options.setInputPath(os.path.dirname(os.path.abspath(__file__)))  # any folder: a tab needs an existing one
for call, index in ((lambda: Dan.Mask.newMask("mask-tab"), 2), (lambda: Dan.Processing.setConfigurations(1), 4),
                    (lambda: Dan.Mask.masks(), 4), (lambda: Dan.Options.Config.setCenterMethod("hf"), 0)):
    call()
    assert tabs.currentIndex() == index, (index, tabs.currentIndex())

# leaving the Mask step activates the full mask "mask" (the sensitivity's, its parameters shown again)
Dan.Mask.newMask()
Dan.Mask.newMask("mask-bs-step", beamstop=(61, 79, 62, 78))
assert Dan.findChild(QSpinBox, "spinBoxLTxBS").value() == 61
Dan.Processing.setConfigurations(1)
assert Dan.findChild(QComboBox, "comboBoxMaskFor").currentText() == "mask"
assert Dan.findChild(QSpinBox, "spinBoxLTxBS").value() != 61

# a Table object where a table's name is asked counts as its name
t = newTable("plain", 2, 2)
raises(lambda: Dan.Processing.calculateTransmissions(t), "no script table plain")
raises(lambda: Dan.Merge.run(t), "nothing to merge in plain")
