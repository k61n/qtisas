"""Auto mode's options are saved with an instrument and come back with it: "Save instrument" writes their
"[AutoMode-...]" lines (and the script-table options, which were written without line breaks and so lost), selecting
the instrument again reads them. The test instrument is removed afterwards.
"""

import glob
import os
import sys

try:
    from PyQt6.QtCore import QMetaObject, QTimer
    from PyQt6.QtWidgets import QApplication, QInputDialog
except ImportError:
    from PyQt5.QtCore import QMetaObject, QTimer
    from PyQt5.QtWidgets import QApplication, QInputDialog

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import DIRECT, quiet

NAME = "AutoModeOptionsTest"
folder = os.path.join(os.path.expanduser("~"), ".config", "qtisas", "SANSinstruments")
if sys.platform.startswith("win"):
    folder = os.path.join(os.path.expanduser("~"), "AppData", "Local", "qtisas", "SANSinstruments")


def answer():
    for w in QApplication.topLevelWidgets():
        if isinstance(w, QInputDialog) and w.isVisible():
            w.setTextValue(NAME)
            w.accept()


try:
    with quiet():
        Dan.Options.newSession()
        Dan.Options.selectInstrument("KWS3-2020")
    Dan.Options.Config.setAutoOptions({"center": "wildcard", "centerRuns": "*H-L*", "minDuration": 77,
                                       "singleDarkCurrent": False, "masks": "files"})
    timer = QTimer()
    timer.timeout.connect(answer)
    timer.start(100)
    QMetaObject.invokeMethod(Dan, "saveInstrumentAs", DIRECT)
    timer.stop()
    text = open(os.path.join(folder, NAME + ".SANS")).read()
    assert "[AutoMode-Center] *H-L*\n" in text and "[Polarization-as-Condition]" in text, text[-2000:]
    assert "[Sample-Position-As-Condition] No\n" in text or "[Sample-Position-As-Condition] Yes\n" in text

    with quiet():
        Dan.Options.selectInstrument("SANS1")
    assert Dan.Options.Config.autoOptions()["center"] == "maxflux"
    with quiet():
        Dan.Options.selectInstrument(NAME)
    options = Dan.Options.Config.autoOptions()
    expected = {"center": "wildcard", "centerRuns": "*H-L*", "minDuration": 77, "singleDarkCurrent": False,
                "masks": "files", "beamSize": 2, "calibrant": ""}
    assert all(options[k] == v for k, v in expected.items()), options
finally:
    for f in glob.glob(os.path.join(folder, NAME + ".SANS")) + glob.glob(os.path.join(folder, "cpp", NAME + ".cpp")):
        os.remove(f)
    with quiet():
        Dan.Options.selectInstrument("KWS3-2020")
