"""Typing a number into a Numeric cell, German (1.000,0) and French (1 000,0), group separators omitted and shown.

Without group separators "." cannot group digits: "0.002" and "1.500" are read by the scripting engine as C
numbers. With them German "1.500" is 1500. Spaces, also French no-break ones, are dropped before reading.
Text that is no number in the table's format is evaluated by the scripting engine: run once with Python, then
with muParser ("2^3" is 1 in Python, 8 in muParser, so the engine that evaluated it is checked too), set to use C
like Python in Preferences. Preferences' Apply saves the settings: the user's qtisas.ini is restored at the end.
"""

import os
import sys

try:
    from PyQt6.QtCore import Q_ARG, Q_RETURN_ARG, QMetaObject
    from PyQt6.QtWidgets import QTableWidget
except ImportError:
    from PyQt5.QtCore import Q_ARG, Q_RETURN_ARG, QMetaObject
    from PyQt5.QtWidgets import QTableWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import DIRECT, apply_preferences, locale_french, locale_german, user_settings_kept

CASES = {
    # locale, omitted: {typed: value}
    ("German", True): {"0,002": 0.002, "0.002": 0.002, "0.02": 0.02, "1.500": 1.5, "1500": 1500, "0.5*2": 1},
    ("German", False): {"0,002": 0.002, "0.002": 2, "0.02": 0.02, "1.500": 1500, "1.500,5": 1500.5},
    ("French", True): {"0,002": 0.002, "0.002": 0.002, "1 500,5": 1500.5, "1 500": 1500, "1 500": 1500},
    ("French", False): {"0.002": 0.002, "1 500,5": 1500.5, "1 500": 1500, "1 500": 1500},
}
POWER = {"Python": 1, "muParser": 8}  # "2^3"


def run(engine):
    for (language, omitted), cases in CASES.items():
        locale = (locale_german if language == "German" else locale_french)(omitted)
        cases = dict(cases, **{"2^3": POWER[engine]})
        t = newTable("CellEdit%s%s%d" % (engine, language, omitted), len(cases), 1)
        t.setLocale(locale)
        widget = t.findChild(QTableWidget)
        for row, (typed, expected) in enumerate(cases.items()):
            widget.item(row, 0).setText(typed)  # as the cell editor does: itemChanged -> Table::cellEdited
            got = t.cell(1, row + 1)
            assert got == expected, (engine, language, omitted, typed, t.text(1, row + 1), expected)


run("Python")

with user_settings_kept():
    apply_preferences(muparser_c=True)  # "muParser uses C locale settings", whatever the user chose

    # New tables take the application's scripting engine. No processEvents() after the switch: the posted
    # ScriptingChangeEvent would replace the engine of this running Python script.
    switched = QMetaObject.invokeMethod(qti.app, "setScriptingLanguage", DIRECT, Q_RETURN_ARG(bool),
                                        Q_ARG(str, "muParser"), Q_ARG(bool, False))
    assert switched, "cannot switch to muParser"
    run("muParser")
