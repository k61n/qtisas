"""The script table's transmissions with empty cells and a row selection:
- an emptied Polarization (PN mode) or Transmission-Sample cell: Force copy/paste took the first part of the split cell
  with [0] of an empty list and crashed QtiSAS (2026-11-12, "Make script table" in the PN mode); now the row is
  handled as one without a transmission;
- only the selected rows are calculated, also a row past the number of tables in the project: the selection check
  looped over the tables, not the rows, so a selected row 5 with two tables in the project went unseen and every row
  was calculated.
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtWidgets import QCheckBox, QTableWidget
except ImportError:
    from PyQt5.QtWidgets import QCheckBox, QTableWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

r = Dan.Rawdata
p = Dan.Processing
runs = ["1000%02d" % n for n in range(1, 7)]
work = tempfile.mkdtemp()
try:
    source = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "100001.nxs")
    for run in runs:
        shutil.copy(source, os.path.join(work, run + ".nxs"))
    dr.new_session("ILL-SAM-init", work)
    for run in runs:
        r.setHeaderValue(run, "Polarization", "up")
    Dan.Options.selectMode("(PN) Polarized Neutrons")
    p.setConfigurations(1)
    p.setConfiguration(1, emptyBeam=runs[0], transmission=True)
    p.calculate()
    t = table(p.newScriptTable("script", runs[1:]))
    names = list(t.colNames())
    pol, tr = names.index("Polarization") + 1, names.index("Transmission-Sample") + 1
    assert t.numRows() == 5 and len(tableNames()) < 5, (t.numRows(), tableNames())

    # only the selected row 5 is calculated
    for row in range(1, 6):
        t.setText(tr, row, "")
    view = t.findChild(QTableWidget)
    view.clearSelection()
    view.selectRow(4)
    p.calculateTransmissions("script")
    values = [t.text(tr, row) for row in range(1, 6)]
    assert values[:4] == ["", "", "", ""] and values[4].strip().startswith("1.0000"), values

    # an emptied Polarization and Transmission-Sample cell with Force copy/paste: no crash, the row gets a value
    view.clearSelection()
    Dan.findChild(QCheckBox, "checkBoxForceCopyPaste").setChecked(True)
    t.setText(pol, 2, "")
    t.setText(tr, 2, "")
    p.calculateTransmissions("script")
    assert t.text(pol, 2) == "" and t.text(tr, 2) != "", (t.text(pol, 2), t.text(tr, 2))
finally:
    shutil.rmtree(work, ignore_errors=True)
