"""The configuration table's run cells show the run's header as a tooltip when the mouse rests on them (the synthetic
HDF5 run data/100001.nxs, ILL-SAM-init's header map): sample name, C, D, lambda, time, counts and the file; a cell of
another row, an empty run cell or a run that is not in the folder shows none.
"""

import os
import shutil
import sys
import tempfile
import time

try:
    from PyQt6.QtCore import QEvent
    from PyQt6.QtGui import QHelpEvent
    from PyQt6.QtWidgets import QApplication, QTableWidget, QTableWidgetItem, QToolTip
except ImportError:
    from PyQt5.QtCore import QEvent
    from PyQt5.QtGui import QHelpEvent
    from PyQt5.QtWidgets import QApplication, QTableWidget, QTableWidgetItem, QToolTip

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

EC, C, CENTER = 0, 2, 14  # rows of the configuration table (dan18.h: dptEC, dptC, dptCENTER)


def tooltip(table, row, text):
    """The tooltip shown when the mouse rests on cell (row, 0) holding text."""
    item = table.item(row, 0)
    if item is None:
        item = QTableWidgetItem()
        table.setItem(row, 0, item)
    item.setText(text)
    settle()  # a tooltip shown before hides with a delay
    pos = table.visualItemRect(item).center()
    viewport = table.viewport()
    QApplication.sendEvent(viewport, QHelpEvent(QEvent.Type.ToolTip, pos, viewport.mapToGlobal(pos)))
    shown = QToolTip.text() if QToolTip.isVisible() else ""
    QToolTip.hideText()
    settle()
    return shown


def settle(seconds=0.5):
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        QApplication.processEvents()


work = tempfile.mkdtemp()
try:
    shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "100001.nxs"), work)
    dr.new_session("ILL-SAM-init", work)
    table = Dan.findChild(QTableWidget, "tableEC")
    info = Dan.Rawdata.info(["100001"])[0]

    text = tooltip(table, EC, "100001")
    lines = text.split("\n")
    assert len(lines) == 4, text
    assert lines[0] == "100001 · " + info["sample"], text
    assert lines[1].startswith("C %g m · D %g m · λ " % (round(info["C"], 3), round(info["D"], 3))), text
    assert " counts · attenuator " in lines[2], text
    assert lines[3] == "100001.nxs", text
    assert tooltip(table, CENTER, "100001") == text  # every run row

    assert tooltip(table, C, "100001") == ""  # not a run row
    assert tooltip(table, EC, "") == ""
    assert tooltip(table, EC, "999999") == ""  # no such run in the folder
finally:
    QToolTip.hideText()
    shutil.rmtree(work)
