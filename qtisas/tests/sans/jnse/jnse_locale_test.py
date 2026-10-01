"""JNSE: numbers read from a .dtr file (C) go into German tables in the table's format.

A one-dataset .dtr file is written to a temporary folder and read with "Read NSE data", answering its table-name
and file dialogs. The header table's Numeric columns and the dataset table must hold the file's numbers, and the
dataset table is hidden in the project (also after a folder switch and in the saved project), the header table
maximized as the active window, so that a new graph opens maximized.
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtCore import QMetaObject
    from PyQt6.QtWidgets import QApplication
except ImportError:
    from PyQt5.QtCore import QMetaObject
    from PyQt5.QtWidgets import QApplication

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import (accept_file, accept_text, answering, locale_german, set_app_locale,
                            show_windows_of_active_folder, use_qt_dialogs)

use_qt_dialogs()
show_windows_of_active_folder()
set_app_locale(locale_german())

HEADER = [("q", 0.05), ("q_var", 0.001), ("q_unit_SI", 1e10), ("t_unit_SI", 1e-9), ("temp", 300.5), ("numor1", 123),
          ("ref1", 124), ("bgr1", 125), ("withbgr", 1), ("tfac_bgr", 0.95), ("volfrac", 0.1), ("qbins", 1),
          ("tbins", 2), ("oor_cnt", 0), ("upd_cnt", 5)]
ROWS = [[0.1, 0.95, 0.01, 0.0001, 0.001, 5, 0.05], [1.5, 0.75, 0.02, 0.0004, 0.002, 5, 0.05]]
lines = ["sample1 2026-10-01 12:00", "Sqt/Sq vs t/ns 42", "parameter"]
lines += ["%s %r" % (key, value) for key, value in HEADER]
lines += ["values", "tau Sqt dSqt varSqt vartau nupdates Qeff"]
lines += [" ".join(repr(v) for v in row) for row in ROWS]
lines += ["#eod", "", "", ""]

folder = tempfile.mkdtemp()
path = os.path.join(folder, "jnselocale.dtr")
with open(path, "w") as f:
    f.write("\n".join(lines) + "\n")

jnse = [w for w in QApplication.allWidgets() if w.metaObject().className() == "jnse18"][0]
with answering(accept_text("JNSElocale"), accept_file(folder, os.path.basename(path))) as messages:
    QMetaObject.invokeMethod(jnse, "readNseData")
assert not messages, messages

header = table("JNSElocale")
assert header is not None and header.numRows() == 1, header
for col, (key, value) in enumerate(HEADER, 5):  # columns q .. upd-cnt
    assert header.cellOrNone(col, 1) == value, (key, header.text(col, 1), value)
assert header.text(5, 1) == "0,05", header.text(5, 1)

dataset = table(header.text(20, 1))  # table-name column
for row, values in enumerate(ROWS, 1):
    for col, value in enumerate(values, 1):
        assert dataset.cellOrNone(col, row) == value, (col, row, dataset.text(col, row), value)
assert dataset.text(1, 2).startswith("1,5"), dataset.text(1, 2)

# the dataset tables are hidden in the project: they stay hidden after a folder switch and are saved as hidden
folderJnse = activeFolder()
changeFolder(addFolder("JNSEother"))
changeFolder(folderJnse)
assert not dataset.isVisible(), "a JNSE dataset table reappeared after a folder switch"
project = os.path.join(folder, "jnse.qti")
saveProjectAs(project)
lines = open(project, encoding="utf-8", errors="replace").read().split("\n")
geometry = next(lines[i + 1] for i, line in enumerate(lines) if line.startswith(dataset.objectName() + "\t"))
assert geometry.endswith("hidden"), geometry
# the header table is the folder's maximized active window
geometry = next(lines[i + 1] for i, line in enumerate(lines) if line.startswith(header.objectName() + "\t"))
assert geometry == "geometry\tmaximized\tactive", geometry

# a new graph takes over the maximized state from the header table, as after any maximized active window
graph = newGraph("JNSEgraph")
area = workspace()
assert area.activeSubWindow() is graph and graph.isMaximized(), (area.activeSubWindow(), graph.isMaximized())
states = (header.isVisible(), header.isMaximized(), dataset.isVisible())
assert states == (True, False, False), states  # header shown and restored, dataset hidden

shutil.rmtree(folder)
