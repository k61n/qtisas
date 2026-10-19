"""ASCII.1D: an empty dataset skips only itself; a saved format restores the destination and the number of columns.

Files and tables: with "Remove first 3 points", a dataset of 2 points is left empty — it is skipped with a message,
and the datasets after it are still processed (it used to end the whole batch). A format saved with destination
"File" and 3 columns restores both (the destination used to be applied to the source, the columns read as 0).
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtWidgets import QApplication, QCheckBox, QComboBox, QPushButton, QSpinBox, QToolButton
except ImportError:
    from PyQt5.QtWidgets import QApplication, QCheckBox, QComboBox, QPushButton, QSpinBox, QToolButton

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import accept_file, accept_text, answering, locale_c, set_app_locale, use_qt_dialogs

set_app_locale(locale_c())
use_qt_dialogs()
ascii1d = [w for w in QApplication.allWidgets() if w.metaObject().className() == "ascii1d18"][0]
child = lambda cls, name: ascii1d.findChild(cls, name)

source, destination = child(QComboBox, "comboBoxSource"), child(QComboBox, "comboBoxDestination")
columns = child(QSpinBox, "spinBoxForceNumCols")
destination.setCurrentIndex(0)  # table
for name, on in (("checkBoxIndexing", True), ("checkBoxActions", True), ("checkBoxConvert", False),
                 ("checkBoxMath", False), ("checkBoxMath2", False), ("checkBoxBinningLinear", False),
                 ("checkBoxBinningProgressive", False), ("checkBox1DCalculator", False), ("checkBoxReso", False),
                 ("checkBoxRemoveRange", False), ("checkBoxRemoveLast", False), ("checkBoxNoNegative", False),
                 ("checkBoxRemoveFirst", True)):
    child(QCheckBox, name).setChecked(on)
child(QSpinBox, "lineEditRemoveFirst").setValue(3)


def created_since(before):
    return sorted(n for n in tableNames() if n not in before)


# --- tables: the first one is left empty, the second one is still processed
short, long_ = newTable("Ascii1dSkipA", 2, 3), newTable("Ascii1dSkipB", 10, 3)
for t in (short, long_):
    n = t.numRows()
    t.setColData(1, [float(k) for k in range(1, n + 1)])
    t.setColData(2, [float(k) for k in range(1, n + 1)])
    t.setColData(3, [0.1] * n)
source.setCurrentIndex(1)  # tables
before = set(tableNames())
with answering(accept_text("Ascii1dSkip*")) as messages:
    child(QPushButton, "ASCIIpushButton").click()
made = created_since(before)
assert len(messages) == 1 and "Ascii1dSkipA" in messages[0] and "no data" in messages[0], messages
assert len(made) == 1 and table(made[0]).numRows() == 7, [(n, table(n).numRows()) for n in made]

# --- files: the same with two files in one batch
folder = tempfile.mkdtemp(prefix="qtisas-ascii1d-")
try:
    for name, n in (("a_short.dat", 2), ("b_long.dat", 10)):
        with open(os.path.join(folder, name), "w", encoding="utf-8") as f:
            f.writelines("%d %d 0.1\n" % (k, k) for k in range(1, n + 1))
    source.setCurrentIndex(0)  # files
    child(QComboBox, "comboBoxFormat").setCurrentIndex(0)  # ASCII
    before = set(tableNames())
    with answering(accept_file(folder, '"a_short.dat" "b_long.dat"')) as messages:
        child(QPushButton, "ASCIIpushButton").click()
    made = created_since(before)
    assert len(messages) == 1 and "a_short.dat" in messages[0] and "no data" in messages[0], messages
    assert len(made) == 1 and table(made[0]).numRows() == 7, [(n, table(n).numRows()) for n in made]
finally:
    shutil.rmtree(folder, ignore_errors=True)

# --- a format restores the destination (not the source) and the number of columns
name = "QtisasTestAscii1dFormat"
path = os.path.join(os.path.expanduser("~"), ".config", "qtisas", "ascii1dFormats", name + ".ASCII1D")
try:
    source.setCurrentIndex(0)  # files
    destination.setCurrentIndex(1)  # file
    columns.setValue(3)
    with answering(accept_text(name)):
        child(QToolButton, "pushButtonNewASCII1D").click()
    text = open(path, encoding="utf-8").read()
    assert "{Data-Destination-To} File" in text and "{Data-Number-Columns} 3" in text, text

    destination.setCurrentIndex(0)
    columns.setValue(4)
    formats = child(QComboBox, "comboBoxStructureASCII1D")
    formats.setCurrentIndex(formats.findText(name))
    formats.activated.emit(formats.currentIndex())  # as when chosen: reads the file
    read = (source.currentIndex(), destination.currentIndex(), columns.value())
    assert read == (0, 1, 3), read
finally:
    destination.setCurrentIndex(0)
    if os.path.exists(path):
        os.remove(path)
