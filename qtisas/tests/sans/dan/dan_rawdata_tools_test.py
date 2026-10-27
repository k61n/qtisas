"""Rawdata tab, "Conventional" page, from Python as from the interface (the qtisas.com KWS-1 and KWS-3 examples;
skipped without them), the files written into a temporary output folder:
- addRuns(): one new run from several, its matrix the sum and its header's duration, counts and monitors the sums
  (before: 6 digits, 4887564 + 6979012 counts written as 11866600); KWS-3's YAML header named as the detector file
  (before: "..._HRD_standard_HRD_standard.yaml");
- sampleGroups(): the same sample and configuration anywhere in the folder; newAddingTemplate() of them (no table of
  configurations needed) + addRunsFromTable(by="columns"), a table by rows (before: by rows a pattern with "*" had no
  file name to write);
- newInfoTable() (added to), newInfoMatrix() (added to; other columns refused), extractMatrices();
- info() of a run that is not there is an error (before: left out); runs() lists only the runs DAN opens (before:
  KWSX-init's "*_0_#.edf" listed the frames db_..._0_00007_0.edf of a sub-folder as runs no call found);
- files(runs), info()["files"]: a run's files (detector, and its own header file), e.g. to move the runs that were
  added;
- neighbours(): the runs to add, found as DAN groups them, whole series or groups of n; info()["added"] marks a run
  made by adding; newAddingTemplate(consecutive=True) of the series; a sample in two configurations, and two names
  that become one column name, get columns of their own; addRunsFromTable(skipSingle=) skips or copies a single run;
- the "Fast Info Extractor" list keeps each action once when "Ascii" is toggled (before: it renamed the first one).
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtWidgets import QCheckBox, QComboBox
except ImportError:
    from PyQt5.QtWidgets import QCheckBox, QComboBox

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

r = Dan.Rawdata


def raises(call, text, error=ValueError):
    try:
        call()
    except error as e:
        assert text in str(e), (text, str(e))
        return
    raise AssertionError("no %s: %s" % (error.__name__, text))


def cells(name):
    m = matrix(name)
    return [m.cell(i, j) for i in range(1, m.numRows() + 1) for j in range(1, m.numCols() + 1)]


def fields(info):
    return info["duration"], info["counts"], info["lambda"], info["sample"]


kws1 = dr.data_folder("kws1", "*.DAT")
kws3 = dr.data_folder("kws3", "*.yaml")
out = tempfile.mkdtemp()
try:
    # --- KWS-1: two runs added; the new run read back from the output folder
    dr.kws1(kws1, samples=False)  # its configurations: the adding template groups by them
    Dan.Options.setOutputPath(out)
    a, b = r.info(["53217", "53218"])
    assert a["files"] == [os.path.join(kws1, "example-kws1_53217_Stan_C20_S3_D0.DAT")], a["files"]  # to move/copy
    assert r.files("53217") == a["files"] and r.files(["53218", "53217"]) == b["files"] + a["files"]
    raises(lambda: r.files(["53217", "99999"]), "no run 99999")
    files = r.addRuns(["53217", "53218"])
    assert [os.path.basename(f) for f in files] == ["example-kws1_5321700002_Stan_C20_S3_D0.DAT"], files
    r.newInfoTable("info-raw", ["53217", "53218"], matrices=True)  # their raw matrices: Matrix-53217, ...
    raw = [x + y for x, y in zip(cells("Matrix-53217"), cells("Matrix-53218"))]
    Dan.Options.setInputPath(out)
    (added,) = r.info(["5321700002"])
    assert fields(added) == (a["duration"] + b["duration"], a["counts"] + b["counts"], a["lambda"], a["sample"]), \
        (added, a, b)
    r.newInfoTable("info-added", ["5321700002"], matrices=True)
    assert cells("Matrix-5321700002") == raw
    Dan.Options.setInputPath(kws1)
    Dan.Options.setOutputPath(out)  # setInputPath() sets the output folder to the input's parent

    # the two Add buttons: by columns (first row the new run, below its runs), by rows
    raises(lambda: r.newAddingTemplate(), "no sample measured twice in one configuration")  # the KWS-1 example
    cols = newTable("add-cols", 3, 2)
    for row, run in enumerate(("5321700001", "53217", "53221"), 1):
        cols.setText(1, row, run)
    for row, run in enumerate(("5322200001", "53222"), 1):
        cols.setText(2, row, run)
    files = r.addRunsFromTable("add-cols")  # "Skip single runs" (checked by default): 53222 alone is skipped
    assert [os.path.basename(f).split("_")[1] for f in files] == ["5321700001"], files
    files = r.addRunsFromTable("add-cols", skipSingle=False)  # 53222 copied under its new number too
    assert sorted(os.path.basename(f).split("_")[1] for f in files) == ["5321700001", "5322200001"], files
    single = newTable("add-single", 2, 1)
    single.setText(1, 1, "5321800001")
    single.setText(1, 2, "53218")
    raises(lambda: r.addRunsFromTable("add-single", skipSingle=True), "or single runs only (skipSingle)")
    rows = newTable("add-rows", 2, 3)
    for col, run in enumerate(("99001", "53217", "53218"), 1):
        rows.setText(col, 1, run)
    for col, run in enumerate(("99002", "53221", "53222"), 1):
        rows.setText(col, 2, run)
    files = r.addRunsFromTable("add-rows", by="rows")
    assert sorted(os.path.basename(f).split("_")[1] for f in files) == ["99001", "99002"], files

    # info table (added to), info matrix (added to, the same columns only), extracted matrices
    r.newInfoTable("info-1", ["53217", "53218"], compact=True)
    r.newInfoTable("info-1", ["53221"])
    assert table("info-1").numRows() == 3 and table("info-1").numCols() == 56
    assert r.newInfoMatrix("info_m", ["53217", "53218", "53221"], columns=2) == "info-m"  # "_" as "-", as DAN
    r.newInfoMatrix("info-m", ["53222"])
    m = matrix("info-m")
    assert (m.numRows(), m.numCols()) == (288, 288) and "[2,2] 53222; [1,3]" in m.windowLabel(), m.windowLabel()
    raises(lambda: r.newInfoMatrix("info-m", ["53222"], columns=3), "info-m: 2 columns of images 144 pixels wide")
    files = r.extractMatrices(["53217"], asDouble=False)
    assert files == [os.path.join(out, "raw-matrix", "only_53217_rawdata.matrix")] and os.path.isfile(files[0]), files

    raises(lambda: r.addRuns(["53217"]), "at least two runs")
    raises(lambda: r.addRuns(["53217", "nope"]), "no run nope")
    raises(lambda: r.addRunsFromTable("add-rows", by="cols"), "by: \"columns\" or \"rows\"")
    raises(lambda: r.newInfoTable("add-rows", ["53217"]), "add-rows is no info table")
    raises(lambda: r.newAddingTemplate(["nope"]), "no run nope")
    raises(lambda: r.info(["53217", "99999"]), "no run 99999")  # before: left out without a word
    assert all(x["sample"] for x in r.info(r.runs()))  # runs() lists only runs DAN opens

    # the Fast Info Extractor's actions: each once, the one chosen kept, after "Ascii" on and off
    actions = Dan.findChild(QComboBox, "comboBoxCheck")
    ascii_ = Dan.findChild(QCheckBox, "checkBoxBigMatrixASCII")
    actions.setCurrentIndex(actions.findText("View Header"))
    for state in (True, False):
        ascii_.setChecked(state)
        items = [actions.itemText(i) for i in range(actions.count())]
        assert len(items) == len(set(items)), items
    assert actions.currentText() == "Plot Matrix [Plot-Active]" and "View I(Q)" in items, (actions.currentText(), items)
    actions.setCurrentIndex(actions.findText("View Header"))
    ascii_.setChecked(True)
    ascii_.setChecked(False)
    assert actions.currentText() == "Plot Matrix [Plot-Active]"  # "View Header" is no ASCII action: the default

    # --- neighbours(): runs next to each other, the same sample and configuration, in groups of n; added runs left
    # out (KWS-1 writes "added files: ..." into the header's comment; KWSX-init has no comment: the name rule)
    near = os.path.join(out, "near")
    os.makedirs(near)
    copies = {"70001": "53217", "70002": "53217", "70003": "53217", "70004": "53218", "70005": "53221",
              "70006": "53217", "70007": "53218"}  # H-J C20 three times, H-L, H-J C8, H-J C20, H-L again
    for new, old_ in copies.items():
        src = [f for f in os.listdir(kws1) if "_%s_" % old_ in f][0]
        shutil.copy(os.path.join(kws1, src), os.path.join(near, src.replace("_%s_" % old_, "_%s_" % new)))
    dr.new_session("KWS1-2020", near)
    Dan.Options.setOutputPath(near)
    groups = r.neighbours(3)
    assert [(g["sample"], g["runs"]) for g in groups] == [("H-J", ["70001", "70002", "70003"])], groups
    assert r.neighbours() == groups  # the whole series: these three
    assert [g["runs"] for g in r.neighbours(2)] == [["70001", "70002"]]  # 70003: no second one next to it
    assert r.neighbours(2, ["70003", "70006"]) == []  # in the folder 70004, 70005 are between them
    assert [g["runs"] for g in r.neighbours(2, ["70002", "70003", "70006"])] == [["70002", "70003"]]
    r.addRuns(groups[0]["runs"])
    (added,) = r.info(["7000100003"])
    assert added["added"] == ["70001", "70002", "70003"] and r.info(["70001"])[0]["added"] == [], added
    assert [g["runs"] for g in r.neighbours()] == [["70001", "70002", "70003"]]  # the added run is no neighbour
    # sampleGroups(): the same sample and configuration anywhere in the folder; the adding template of them
    groups = r.sampleGroups()
    assert [(g["condition"], g["sample"], g["runs"]) for g in groups] == [
        (1, "H-J", ["70001", "70002", "70003", "70006"]), (1, "H-L", ["70004", "70007"])], groups  # 70005: alone
    assert [g["runs"] for g in r.sampleGroups(["70002", "70006", "70005"])] == [["70002", "70006"]]
    t = r.newAddingTemplate()
    tt = table(t)
    assert list(tt.colNames()) == ["C1-H-J", "C1-H-L"], tt.colNames()
    assert [tt.text(1, i) for i in range(1, 6)] == ["7000100004", "70001", "70002", "70003", "70006"]
    t2 = table(r.newAddingTemplate(consecutive=True))  # "Only consecutive runs": 70006, 70004 + 70007 are apart
    assert list(t2.colNames()) == ["C1-H-J"] and [t2.text(1, i) for i in range(1, 5)] == [
        "7000100003", "70001", "70002", "70003"], (t2.colNames(), [t2.text(1, i) for i in range(1, 5)])
    r.newAddingTemplate(consecutive=False)  # the checkbox back
    files = r.addRunsFromTable(t)
    assert sorted(os.path.basename(f).split("_")[1] for f in files) == ["7000100004", "7000400002"], files
    assert [g["runs"] for g in r.sampleGroups()] == [g["runs"] for g in groups]  # the added ones left out
    raises(lambda: r.sampleGroups(["99999"]), "no run 99999")

    # one sample name in two configurations: a group (a column) each; "H-J" and "H_J" in one: two, named apart
    names = os.path.join(out, "names")
    os.makedirs(names)
    for new, old_, sample in (("90001", "53217", None), ("90002", "53217", None), ("90003", "53217", "H_J"),
                              ("90004", "53217", "H_J"), ("90005", "53221", None), ("90006", "53221", None)):
        src = [f for f in os.listdir(kws1) if "_%s_" % old_ in f][0]
        text = open(os.path.join(kws1, src), encoding="latin-1").read()
        if sample:
            text = text.replace("H-J | H-J", "%s | %s" % (sample, sample), 1)
        open(os.path.join(names, src.replace("_%s_" % old_, "_%s_" % new)), "w", encoding="latin-1").write(text)
    dr.new_session("KWS1-2020", names)
    groups = {(g["label"], g["sample"]): g["runs"] for g in r.sampleGroups()}
    assert sorted(groups.values()) == [["90001", "90002"], ["90003", "90004"], ["90005", "90006"]], groups
    tt = table(r.newAddingTemplate())
    assert sorted(tt.colNames()) == sorted(["C%d-H-J" % c for c in (1, 2)] + ["C%d-H-J-2" % [
        g["condition"] for g in r.sampleGroups() if g["sample"] == "H_J"][0]]), tt.colNames()

    # a series of 5: whole (default), or cut into the groups the request gives
    series = os.path.join(out, "series")
    os.makedirs(series)
    src = [f for f in os.listdir(kws1) if "_53217_" in f][0]
    for k in range(1, 6):
        shutil.copy(os.path.join(kws1, src), os.path.join(series, src.replace("_53217_", "_8000%d_" % k)))
    dr.new_session("KWS1-2020", series)
    five = ["80001", "80002", "80003", "80004", "80005"]
    assert [g["runs"] for g in r.neighbours()] == [five]
    assert [g["runs"] for g in r.neighbours(2)] == [five[0:2], five[2:4]]
    assert [g["runs"] for g in r.neighbours(3)] == [five[0:3]]
    raises(lambda: r.neighbours(1), "n: None (the whole series) or 2 or more")
    raises(lambda: r.neighbours("3"), "n: None", TypeError)
    raises(lambda: r.neighbours(3, ["99999"]), "no run 99999")

    # --- KWS-3: YAML header + detector file, added
    dr.new_session("KWS3-2020", kws3)
    Dan.Options.setOutputPath(out)
    a, b = r.info(["00082557", "00082558"])
    assert [os.path.basename(f) for f in a["files"]] == ["00082557_0000_H-M_HRD_standard.det",
                                                         "00082557_0000_H-M_HRD_standard.yaml"], a["files"]
    assert r.files(["00082557"]) == a["files"]
    files = r.addRuns(["00082557", "00082558"])
    assert sorted(os.path.basename(f) for f in files) == ["0008255700002_0000_H-M_HRD_standard.det",
                                                           "0008255700002_0000_H-M_HRD_standard.yaml"], files
    Dan.Options.setInputPath(out)
    (added,) = r.info(["0008255700002"])
    assert fields(added) == (a["duration"] + b["duration"], a["counts"] + b["counts"], a["lambda"], a["sample"])
finally:
    shutil.rmtree(out)
