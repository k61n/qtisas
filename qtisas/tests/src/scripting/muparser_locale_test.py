"""Preferences "muParser uses C locale settings" with the 1.000,0 format, thousands separators omitted.

Checked: muParser reads text typed into a table like Python, "0.5*2" and "max(1,2)". Unchecked: in the numerical
format, "0,5*2" and "max(1;2)", and a "." is no separator at all. Several comma- (or semicolon-) separated
expressions are no value, in a cell or a column formula: "1,1+2,2" in C would otherwise give the last one, 2.
Preferences' Apply saves the settings: the user's qtisas.ini is restored at the end.
"""

import os
import sys

try:
    from PyQt6.QtCore import Q_ARG, Q_RETURN_ARG, QCoreApplication, QEvent, QMetaObject
    from PyQt6.QtWidgets import QTableWidget
except ImportError:
    from PyQt5.QtCore import Q_ARG, Q_RETURN_ARG, QCoreApplication, QEvent, QMetaObject
    from PyQt5.QtWidgets import QTableWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import DIRECT, apply_preferences, user_settings_kept

SCRIPTING_CHANGE_EVENT = getattr(QEvent, "Type", QEvent).User
CASES = {
    True: {"0.5*2": 1, "max(1,2)": 2, "1.500": 1.5, "2^3": 8, "1,1+2,2": None},
    False: {"0,5*2": 1, "max(1;2)": 2, "0.5*2": None, "1.500": None, "2^3": 8, "1,1+2,2": 3.3, "1;2": None},
}


with user_settings_kept():
    # New tables take muParser. The switch's ScriptingChangeEvents are dropped: delivered in the dialog's event
    # loop they would replace the engine of this running Python script.
    assert QMetaObject.invokeMethod(qti.app, "setScriptingLanguage", DIRECT, Q_RETURN_ARG(bool),
                                    Q_ARG(str, "muParser"), Q_ARG(bool, False)), "cannot switch to muParser"
    QCoreApplication.removePostedEvents(None, SCRIPTING_CHANGE_EVENT)

    for c_locale, cases in CASES.items():
        apply_preferences(number_format="1.000,0", omit_thousands=True, muparser_c=c_locale)
        t = newTable("MuParserCLocale%d" % c_locale, len(cases), 1)
        widget = t.findChild(QTableWidget)
        for row, (typed, expected) in enumerate(cases.items()):
            widget.item(row, 0).setText(typed)  # itemChanged -> Table::cellEdited -> muParser
            got = t.cellOrNone(1, row + 1)
            assert got == expected, (c_locale, typed, t.text(1, row + 1), expected)

        # Column formulas too: one value per row, not the last of several
        separator = "," if c_locale else ";"
        for formula, expected in (("max(1%s 2)" % separator, 2), ("1%s 2" % separator, None)):
            t.setCommand(1, formula)
            try:
                t.recalculate(1, 1, -1, True)
                got = t.cellOrNone(1, 1)
            except ValueError:  # the formula failed to evaluate
                got = None
            assert got == expected, (c_locale, formula, t.text(1, 1), expected)
