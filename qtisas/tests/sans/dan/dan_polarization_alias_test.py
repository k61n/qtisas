"""PN mode: the polarization states of the header values (instrument parameters POL-ALIAS-*), on copies of the
synthetic HDF5 run (data/100001.nxs, ILL-SAM-init's header map) with their Polarization values rewritten:
- out-up and out-down (polarizer out, analyzer up/down; POL-ALIAS-OUT-UP, POL-ALIAS-OUT-DOWN) are states of their own,
  matched before "down" and "up": a broad "down" alias (*down*) does not take an out-down run;
- the configuration table's polarization combo box shows them (positions 7, 8 after the seven states), also after
  Dan.Processing.calculate() (before, it left the combo boxes at "out");
- the Pol-script table gets their columns after the buffer columns ("#-out-up" 11, "#-out-down" 12): an out-down run
  goes there, the sample name and condition (columns 0, 1) stay; a table made before them gets the columns on update;
- outside the PN mode every run is "out".
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtCore import Q_ARG, QMetaObject, Qt
    from PyQt6.QtWidgets import QCheckBox, QTableWidget
except ImportError:
    from PyQt5.QtCore import Q_ARG, QMetaObject, Qt
    from PyQt5.QtWidgets import QCheckBox, QTableWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

r = Dan.Rawdata
values = {"100001": "pol_out_ana_down", "100002": "flipper_down", "100003": "pol_out_ana_up", "100004": "Out",
          "100005": "down-down"}

work = tempfile.mkdtemp()
try:
    source = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "100001.nxs")
    for run in values:
        shutil.copy(source, os.path.join(work, run + ".nxs"))
    dr.new_session("ILL-SAM-init", work)
    for run, value in values.items():
        r.setHeaderValue(run, "Polarization", value)

    Dan.Options.selectMode("(PN) Polarized Neutrons")
    Dan.Options.Config.setParameters({"POL-ALIAS-DOWN": "*down*", "POL-ALIAS-OUT-DOWN": "*out_ana_down*",
                                      "POL-ALIAS-OUT-UP": "*out_ana_up*"})
    t = table(r.newHeaderTable("aliases", ["Polarization-Aliased"], "100001..100005"))
    aliased = {t.text(1, row): t.text(2, row) for row in range(1, t.numRows() + 1)}
    assert aliased == {"100001": "out-down", "100002": "down", "100003": "out-up", "100004": "out",
                       "100005": "down-down"}, aliased

    # the configuration table: its polarization combo box at the state of the configuration's empty cell
    Dan.Processing.setConfigurations(2)
    Dan.Processing.setConfiguration(1, emptyBeam="100001")
    Dan.Processing.setConfiguration(2, emptyBeam="100003")
    Dan.Processing.calculate()  # the polarization row too, as the interface's corner button
    combos = Dan.findChild(QTableWidget, "tableEC")
    items = [combos.cellWidget(6, 0).itemText(i) for i in range(combos.cellWidget(6, 0).count())]
    assert items[7:] == ["out-up", "out-down"], items
    assert combos.cellWidget(6, 0).currentText() == "out-down", combos.cellWidget(6, 0).currentText()
    assert combos.cellWidget(6, 1).currentText() == "out-up", combos.cellWidget(6, 1).currentText()

    # the Pol-script table (no Python call yet: its slots), polarization as a condition: one configuration per state
    Dan.findChild(QCheckBox, "checkBoxPolarizationAsPara").setChecked(True)
    Dan.Processing.setConfigurations(3)
    for i, run in enumerate(["100004", "100002", "100001"], 1):
        Dan.Processing.setConfiguration(i, emptyBeam=run)
    Dan.Processing.calculate()
    script = Dan.Processing.newScriptTable("script", ["100004", "100002", "100001"])
    for slot in ("newPolarizedScriptTable", "updatePolarizedScriptTable"):
        QMetaObject.invokeMethod(Dan, slot, Qt.ConnectionType.DirectConnection, Q_ARG(str, "scriptPN"))
    pol = table("scriptPN")
    names = list(pol.colNames())
    assert names[11:] == ["#-out-up", "#-out-down"], names
    sample = r.info(["100001"])[0]["sample"]
    row = [pol.text(c, 1) for c in range(1, pol.numCols() + 1)]
    assert pol.numRows() == 1 and row[:5] == [sample, "1", "100004", "", "100002"] and row[11:] == ["", "100001"], row
    pol.setNumCols(11)  # as made before out-up, out-down
    QMetaObject.invokeMethod(Dan, "updatePolarizedScriptTable", Qt.ConnectionType.DirectConnection,
                             Q_ARG(str, "scriptPN"))
    assert list(pol.colNames())[11:] == ["#-out-up", "#-out-down"] and pol.text(13, 1) == "100001", pol.colNames()

    # outside the PN mode: no states
    Dan.Options.selectMode(Dan.Options.modes()[0])
    closeWindow(t)
    t = table(r.newHeaderTable("aliases", ["Polarization-Aliased"], "100001..100005"))
    assert {t.text(2, row) for row in range(1, t.numRows() + 1)} == {"out"}
finally:
    shutil.rmtree(work, ignore_errors=True)
