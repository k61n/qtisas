"""The KWS-1 and KWS-3 reductions of qtisas.com/dan-sans through DAN's Python interface (Dan.Options, Dan.Mask, ...).

The run files are not in the repository: data-kws1.zip and data-kws3.zip of qtisas.com/dan-sans, unpacked into the
folders kws1/ and kws3/ of $QTISAS_DAN_DATA (default ~/.qtisas-test/home/benchdata/dan); without them a test is
skipped. The reference values are those of the projects there (dan-example-kws1.qti, dan-example-kws3.qti).
"""

import glob
import os

try:
    from PyQt6.QtWidgets import QTableWidget
except ImportError:
    from PyQt5.QtWidgets import QTableWidget

from qtisas_testing import quiet, skip


def data_folder(name, pattern):
    """The folder of one example's run files, or the test is skipped."""
    root = os.environ.get("QTISAS_DAN_DATA", os.path.expanduser("~/.qtisas-test/home/benchdata/dan"))
    folder = os.path.join(root, name)
    if not glob.glob(os.path.join(folder, pattern)):
        skip("no %s run files in %s (data-%s.zip of qtisas.com/dan-sans; QTISAS_DAN_DATA)" % (name, folder, name))
    return folder


def column(script, name):
    return list(script.colNames()).index(name) + 1


def rows(name, *indices):
    """(Q, I) of the given rows (1-based, -1 the last) of a table."""
    t = table(name)
    assert t is not None, name
    return [(t.cell(1, r if r > 0 else t.numRows() + 1 + r), t.cell(2, r if r > 0 else t.numRows() + 1 + r))
            for r in indices]


def close(a, b, rel=1e-6):
    return abs(a - b) <= rel * max(abs(a), abs(b))


def check_rows(name, count, expected, rel=1e-6):
    """A table's number of rows and (Q, I) of its first, 20th and last row."""
    t = table(name)
    assert t is not None, name
    assert t.numRows() == count, (name, t.numRows(), count)
    for (q, i), (eq, ei) in zip(rows(name, 1, 20, -1), expected):
        assert close(q, eq, rel) and close(i, ei, rel), (name, q, i, eq, ei)


def new_session(instrument, folder):
    with quiet():
        Dan.Options.newSession()
        Dan.Options.selectInstrument(instrument)
    Dan.Options.setInputPath(folder)


def set_centers(centers):
    """Centers typed into the configuration table, over the calculated ones (as a user does)."""
    t = Dan.findChild(QTableWidget, "tableEC")
    for col, (x, y) in enumerate(centers):
        t.item(15, col).setText(x)
        t.item(16, col).setText(y)


def kws1(folder, centers=None, samples=True):
    """The KWS-1 reduction up to the transmissions: 3 configurations (C20D20, C8D8, C8D2), the instrument's mask, the
    provided sensitivity file, absolute calibration with Plexiglas, centers from H-L, transmissions measured in C8D8.
    centers: (X, Y) of the configurations instead of the calculated ones; samples=False: up to the centers.
    Returns the script table's name."""
    new_session("KWS1-2020", folder)
    Dan.Mask.newMask()
    Dan.Sensitivity.loadSensitivity(os.path.join(folder, "sens-20200117.sens"))
    Dan.Processing.setConfigurations(3)
    runs = (("53216", "53229", "53230", "53218"), ("53220", "53224", "53225", "53222"),
            ("53225", "53224", "53225", "53227"))  # EB, Plexiglas, its EB, center
    for i, (eb, plexi, plexi_eb, center) in enumerate(runs, 1):
        Dan.Processing.setConfiguration(i, emptyBeam=eb, darkCurrent="48462", calibrant=plexi,
                                        calibrantEmptyBeam=plexi_eb, calibrantDarkCurrent="48462", center=center,
                                        cellEmptyBeam=eb, transmission=(i == 2))
    Dan.Processing.calculate()
    if centers:
        set_centers(centers)
    if not samples:
        return None
    script = Dan.Processing.newScriptTable("script", [53217, 53218, 53219, 53221, 53222, 53223, 53226, 53227, 53228])
    Dan.Processing.calculateTransmissions(script)
    return script


def kws3(folder):
    """The KWS-3 reduction up to the transmissions: 2 configurations (D9.2m, D1.2m), sensitivity from run 00000001
    (a pure sensitivity: no dark current subtracted), absolute calibration with the direct beam (EB and B4C only),
    centers from the EB runs, beam-stop and transmission masks of the example, the 1.2 m data scaled to the 9.2 m
    transmission (Scale = Tr(1.2 m) / Tr(9.2 m)), the last 5 I[Q] points removed. Returns the script table's name."""
    new_session("KWS3-2020", folder)
    # in the order of the tabs: every mask, the sensitivity (through "mask", active again), the configurations
    Dan.Mask.newMask()
    boxes = (("9m", (114, 126, 134, 147), (117, 123, 137, 144)), ("1m", (110, 131, 119, 142), (117, 123, 127, 135)))
    for tag, bs, tr in boxes:
        Dan.Mask.newMask("mask-bs-" + tag, beamstop=bs)
        Dan.Mask.newMask("mask-tr-" + tag, beamstop=tr, transmission=True)
    Dan.Sensitivity.newSensitivity("00000001")
    Dan.Processing.setConfigurations(2)
    for i, (eb, (tag, _, _)) in enumerate(zip(("00082558", "00082645"), boxes), 1):
        Dan.Processing.setConfiguration(i, emptyBeam=eb, darkCurrent="00000002", calibrantEmptyBeam=eb,
                                        calibrantDarkCurrent="00000002", center=eb, cellEmptyBeam=eb,
                                        mask="mask-bs-" + tag, transmissionMask="mask-tr-" + tag, transmission=True)
    Dan.Processing.calculate()  # direct beam: the factors and centers through "mask", I[Q] through mask-bs-*
    name = Dan.Processing.newScriptTable("script", ["00082557", "00082559", "00082561", "00082646", "00082647",
                                                    "00082648"])
    Dan.Processing.calculateTransmissions(name)
    script = table(name)
    tr = lambda r: float(script.text(column(script, "Transmission-Sample"), r).split("[")[0])
    sample = lambda r: script.text(column(script, "Run-info"), r).strip()
    condition = lambda r: script.text(column(script, "#-Condition"), r)
    tr9 = {sample(r): tr(r) for r in range(1, script.numRows() + 1) if condition(r) == "1"}
    script.addColumns(1)
    script.setColName(script.numCols(), "Scale")
    for r in range(1, script.numRows() + 1):
        script.setText(script.numCols(), r, "1" if condition(r) == "1" else repr(tr(r) / tr9[sample(r)]))
    Dan.Processing.Options.setRemovePoints(0, 5)
    return name
