"""A run without a thickness in its header (empty or 0) gets 1 mm (0.1 cm) in a new script table, and the results log
says so; a thickness in the header stays. The run: data/synthetic_0_00024.edf (KWSX-init, "Thickness = 0").
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtWidgets import QApplication, QDockWidget, QTextEdit
except ImportError:
    from PyQt5.QtWidgets import QApplication, QDockWidget, QTextEdit

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr


def thickness(script):
    t = table(script)
    return float(t.text(list(t.colNames()).index("Thickness") + 1, 1))


def results_log():
    dock = [w for w in QApplication.allWidgets() if isinstance(w, QDockWidget) and w.objectName() == "logWindow"][0]
    return dock.findChild(QTextEdit).toPlainText()


work = tempfile.mkdtemp()
try:
    shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "synthetic_0_00024.edf"), work)
    dr.new_session("KWSX-init", work)
    header = Dan.Rawdata.headerValue("00024", "Sample-Thickness").strip()
    assert float(header) == 0, header
    assert Dan.Processing.newScriptTable("thickness-0", ["00024"]) == "thickness-0"
    assert thickness("thickness-0") == 0.1
    assert "no thickness in the header (empty or 0) of 1 run(s), 0.1 cm (1 mm) set: 00024" in results_log()
    # a thickness in the header stays (KWSX-init's header in mm: 2 mm = 0.2 cm)
    Dan.Rawdata.setHeaderValue("00024", "Sample-Thickness", 2)
    assert Dan.Processing.newScriptTable("thickness-2", ["00024"]) == "thickness-2"
    assert thickness("thickness-2") == 0.2, thickness("thickness-2")
finally:
    shutil.rmtree(work)
