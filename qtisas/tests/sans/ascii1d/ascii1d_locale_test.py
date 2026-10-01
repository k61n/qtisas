"""ASCII.1D follows the numerical format; its format files stay C, tables are read and written in their format.

In German: the resolution fields and both factors of Modification / Mathematic are converted when the format
changes, typing is checked, and resolution values keep to their range; a German Q-I-dI table is read with "Load"
(source: tables), multiplied by a factor typed as "2,5", and written to a new German table with the right numbers.
A format saved with "New" is written in C and read back in German; the format file is removed again.
"""

import os
import sys

try:
    from PyQt6.QtCore import Qt
    from PyQt6.QtTest import QTest
    from PyQt6.QtWidgets import QApplication, QCheckBox, QComboBox, QLineEdit, QPushButton, QToolButton
except ImportError:
    from PyQt5.QtCore import Qt
    from PyQt5.QtTest import QTest
    from PyQt5.QtWidgets import QApplication, QCheckBox, QComboBox, QLineEdit, QPushButton, QToolButton

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import accept_text, answering, locale_c, locale_german, set_app_locale

RETURN = getattr(Qt, "Key", Qt).Key_Return
ascii1d = [w for w in QApplication.allWidgets() if w.metaObject().className() == "ascii1d18"][0]
child = lambda cls, name: ascii1d.findChild(cls, name)
pixel = child(QLineEdit, "lineEditResoPixelSize")
factor = child(QLineEdit, "lineEditMath")

# Modification / Mathematic: both factors, typed in C, are converted on the format change (they are enabled only
# with "Actions" and "Math" on)
# after the start: lenses off, so their focus field is hidden
lenses, focus = child(QCheckBox, "checkBoxLenses"), child(QLineEdit, "lineEditResoFocus")
assert (lenses.isChecked(), focus.isHidden()) == (False, True), (lenses.isChecked(), focus.isHidden())

set_app_locale(locale_c())
for name in ("checkBoxActions", "checkBoxMath", "checkBoxMath2"):
    if not child(QCheckBox, name).isChecked():
        child(QCheckBox, name).click()
factor2 = child(QLineEdit, "lineEditMath2")
pixel.setText("0.525")
factor.setText("2.5")
factor2.setText("0.125")
set_app_locale(locale_german())
shown = (pixel.text(), factor.text(), factor2.text())
assert shown == ("0,525", "2,5", "0,125"), shown

# typing into a factor: numbers only, in the current format; the other format's decimal point is corrected
def typed(edit, keys):
    edit.clear()
    QTest.keyClicks(edit, keys)
    return edit.text()


for locale, cases in ((locale_german(), {"2.5": "2,5", "2,5": "2,5", "abc": "", "1.5e-3": "1,5e-3"}),
                      (locale_c(), {"2,5": "2.5", "2.5": "2.5", "-3": "-3"})):  # the validator follows the format
    set_app_locale(locale)
    for keys, expected in cases.items():
        assert typed(factor2, keys) == expected, (locale.name(), keys, factor2.text(), expected)
# when editing ends: an empty factor becomes 1; with German group separators shown, a C number is rewritten
set_app_locale(locale_german())
factor2.clear()
QTest.keyClick(factor2, RETURN)
assert factor2.text() == "1", factor2.text()
for keys, expected in (("E4", "1E4"), ("-e-3", "-1e-3"), ("1e", "1")):  # an exponent alone means 1
    typed(factor2, keys)
    QTest.keyClick(factor2, RETURN)
    assert factor2.text() == expected, (keys, factor2.text(), expected)
set_app_locale(locale_german(omit=False))
typed(factor2, "2.5")
QTest.keyClick(factor2, RETURN)
assert factor2.text() == "2,5", factor2.text()
set_app_locale(locale_german())
child(QCheckBox, "checkBoxMath2").click()  # off again: the table below is multiplied by factor 1 only

# resolution fields keep to their range when editing ends; empty: the default
for name, keys, expected in (("lineEditResoDet", "20000", "10000"), ("lineEditResoDet", "0.5", "1"),
                             ("lineEditResoCol", "1e5", "10000"), ("lineEditResoPixelSize", "0.525", "0,525"),
                             ("lineEditResoDeltaLambda", "", "0,1"), ("lineEditResoLambda", "6,5", "6,5")):
    edit = child(QLineEdit, name)
    typed(edit, keys)
    QTest.keyClick(edit, RETURN)
    assert edit.text() == expected, (name, keys, edit.text(), expected)

# a German table, read, multiplied by 2,5 and written to a new table
data = newTable("Ascii1dLocaleData", 3, 3)
data.setLocale(locale_german())
q, i = [0.01, 0.02, 0.03], [1.5, 2.25, 3.125]
data.setColData(1, q)
data.setColData(2, i)
data.setColData(3, [0.1] * 3)
assert data.text(2, 1) == "1,5", data.text(2, 1)

source = child(QComboBox, "comboBoxSource")
source.setCurrentIndex(1)  # tables
child(QComboBox, "comboBoxDestination").setCurrentIndex(0)  # table
child(QCheckBox, "checkBoxIndexing").setChecked(True)  # a new table, not the source overwritten
child(QCheckBox, "checkBoxActions").setChecked(True)
child(QCheckBox, "checkBoxMath").setChecked(True)
math = child(QComboBox, "comboBoxMath")
math.setCurrentIndex(math.findText("*"))
factor.setText("2,5")

before = set(tableNames())
with answering(accept_text("Ascii1dLocaleData")) as messages:
    child(QPushButton, "ASCIIpushButton").click()
assert not messages, messages
created = [table(n) for n in tableNames() if n not in before]
assert len(created) == 1, sorted(set(tableNames()) - before)
result = created[0]
got = [(result.cellOrNone(1, r), result.cellOrNone(2, r)) for r in range(1, 4)]
expected = [(x, 2.5 * y) for x, y in zip(q, i)]
assert all(abs(a - c) < 1e-9 and abs(b - d) < 1e-9 for (a, b), (c, d) in zip(got, expected)), (got, expected)
assert "," in result.text(2, 1), result.text(2, 1)  # written in the table's format

# a format file: written in C, read back in German
name = "QtisasTestAscii1dLocale"
path = os.path.join(os.path.expanduser("~"), ".config", "qtisas", "ascii1dFormats", name + ".ASCII1D")
try:
    with answering(accept_text(name)):
        child(QToolButton, "pushButtonNewASCII1D").click()
    text = open(path, encoding="utf-8").read()
    for line in ("{Reso-PixelSize} 0.525", "{Reso-Lambda} 6.5", "{Math1-Factor} 2.5"):
        assert line in text, (line, text)

    lambda_ = child(QLineEdit, "lineEditResoLambda")
    pixel.setText("1")
    lambda_.setText("1")
    factor.setText("1")
    formats = child(QComboBox, "comboBoxStructureASCII1D")
    formats.setCurrentIndex(formats.findText(name))
    formats.activated.emit(formats.currentIndex())  # as when chosen: reads the file
    read = (pixel.text(), lambda_.text(), factor.text())
    assert read == ("0,525", "6,5", "2,5"), read
finally:
    if os.path.exists(path):
        os.remove(path)
