"""DAN's GUI stays C (like its instrument files) when the application switches to another number format."""

import os
import sys

try:
    from PyQt6.QtWidgets import QDoubleSpinBox
except ImportError:
    from PyQt5.QtWidgets import QDoubleSpinBox

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import locale_german, set_app_locale

spin = Dan.findChild(QDoubleSpinBox, "spinBoxRebinQmin")  # lost its fixed English locale in c6e86bca
spin.setValue(0.0125)
set_app_locale(locale_german())

assert qti.app.locale().name() == "de_DE", qti.app.locale().name()
assert Dan.locale().name() == "C", Dan.locale().name()
assert spin.text().endswith("0.0125"), spin.text()
