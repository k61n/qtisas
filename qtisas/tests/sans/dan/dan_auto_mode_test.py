"""Auto mode (from qtikws's DAN-BlackBox): the options of Options > Auto-Mode, saved with the instrument, find the
runs of a reduction (skipped without the qtisas.com example files).

- KWS1-2020: four conditions of empty beams, the Plexiglas-only one (C20 D1.98) dropped for want of a center run; the
  three configurations get the runs of dan_reduction.kws1() (B4C, Plexiglas at the nearest distance, its empty beam,
  the H-L centers, transmissions in C8D7.68), and the factors and centers of the reference.
- KWS3-2020: direct-beam calibration, centers from the empty beams, beam-stop and transmission masks per
  configuration, the pure sensitivity run, transmissions in both: a "Scale" column normalizes the 1.2 m rows to the
  9.2 m transmission of the same sample.
- A session (its script table) brings auto mode's rules back.
- After auto mode the interface's buttons (factor, center, Calculate Tr, I[Q], Merge) give the same results.
- Centers from the empty beams + beam-stop masks per configuration + flat sensitivity: "mask" has no beam stop (the
  flat "sens" = "mask"), the direct beam's factor counts the beam; the auto option scale off: no Scale column; the
  auto option openMask (auto: that rule; yes, no).
- flexibleFrom, flexibleMin: from which distance and above which one "other D" takes the calibrant; a calibrant
  chosen before does not stay in the configuration table.
- centerOffsets: HShift, VShift of each sample onto its empty beam's center (runCenter(), centerOffset(),
  addCenterOffsets()).
- Center "beamstop", transmissions measured at 9.2 m only: 1.2 m has no direct beam, so no center run (the center of
  its beam-stop mask, the instrument's box) and no transmission mask; 9.2 m the max-flux center.
- Separate transmission configurations (Skip Transmission Configurations, Transmission Method with the direct beam):
  of two with the same beam-stop mask only the one whose empty beam is the direct beam measures the transmissions;
  with transmissions from the header or without the option both.
- SANS-1 (Ebeam, Ecell, B4C, H2O): its own preset finds the holdout's 4 m configuration; every built-in family has
  its preset, an instrument without "[AutoMode-...]" lines the defaults; a wrong option is a ValueError.
- infoTable: an info table of the runs the run filters select; autoInfoTable(): that table alone.
- processIQ, merge: autoConfig() ends with the I[Q] and the merged curves in the project; Merge shown with I[Q] only.
- No run left by the filters: the error names them, not the empty beam; minCounts 0 is no count filter.
"""

import os
import shutil
import sys
import tempfile

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

try:
    from PyQt6.QtWidgets import QCheckBox
except ImportError:
    from PyQt5.QtWidgets import QCheckBox

ROLES = ("emptyBeam", "darkCurrent", "calibrant", "calibrantEmptyBeam", "calibrantDarkCurrent", "center",
         "cellEmptyBeam", "transmission")


def raises(call, text):
    try:
        call()
    except ValueError as e:
        assert text in str(e), (text, str(e))
        return
    raise AssertionError("no ValueError: " + text)


p = Dan.Processing

# --- KWS-1
dr.new_session("KWS1-2020", dr.data_folder("kws1", "*.DAT"))
assert Dan.Options.Config.autoOptions()["center"] == "maxflux"
assert [c["runs"] for c in Dan.Rawdata.conditions()][1:] == [
    ["53216", "53217", "53218", "53219"], ["53220", "53221", "53222", "53223"],
    ["53224", "53225", "53226", "53227", "53228"], ["53229", "53230"]]
assert Dan.Rawdata.sampleRuns() == ["53217", "53218", "53219", "53221", "53222", "53223", "53226", "53227", "53228"]
assert Dan.Rawdata.runs(minDuration=500) == ["48462"] + [str(n) for n in range(53216, 53229)]  # not the 300 s runs
assert Dan.Mask.autoMasks() == ["mask"]
assert Dan.Sensitivity.autoSensitivity() == "sens"
chosen = [tuple(c[k] for k in ROLES) for c in p.autoConfigurations()]
assert chosen == [("53216", "48462", "53229", "53230", "48462", "53218", "53216", False),
                  ("53220", "48462", "53224", "53225", "48462", "53222", "53220", True),
                  ("53225", "48462", "53224", "53225", "48462", "53227", "53225", False)], chosen
assert any("no run for the center" in m for m in p.lastMessages()), p.lastMessages()
p.calculate()
assert [round(p.configuration(i)["factor"]) for i in (1, 2, 3)] == [53390, 1765, 117]
# no run left by the filters: the error names the filter (a header map without Sum gave every run 0 counts, and the
# error named the empty beam); minCounts 0: no count filter
Dan.Options.Config.setAutoOptions({"minCounts": 2000000000})
raises(p.autoConfigurations, "no runs pass the auto filters:")
raises(p.autoConfigurations, "with counts > 2000000000 (minCounts)")
Dan.Options.Config.setAutoOptions({"minCounts": 0})
assert len(Dan.Rawdata.runs(minDuration=30, minCounts=0)) == len(Dan.Rawdata.runs(minDuration=30, minCounts=-1))
Dan.Options.Config.setAutoOptions({"minCounts": 100})
s = p.autoConfig("auto-kws1")
assert s == "auto-kws1" and table(s).numRows() == 9
assert "QI-SM-53217-H-J" in p.process(s)
assert Dan.Merge.join(s + "-mergingTemplate") == ["H-J", "H-L", "H-M"]
assert "Scale" not in table("auto-kws1").colNames()  # one configuration measures the transmissions
# the auto option infoTable (off by default): an info table "<script table>-info" of all runs the run filters select
assert Dan.Options.Config.autoOptions()["infoTable"] is False and not table("auto-kws1-info")
Dan.Options.Config.setAutoOptions({"infoTable": True})
assert p.autoConfig("auto-info") == "auto-info", p.lastError()
options = Dan.Options.Config.autoOptions()
selected = Dan.Rawdata.runs(options["minDuration"], options["minCounts"])
assert table("auto-info-info").numRows() == len(selected) > 9, (table("auto-info-info").numRows(), len(selected))
Dan.Options.Config.setAutoOptions({"infoTable": False})
# the auto options processIQ and merge (off by default; Merge shown only with radial averaging): autoConfig() ends with
# the I[Q] of the samples and their merged curves in the project
merge_box = Dan.findChild(QCheckBox, "checkBoxAutoMerge")
assert not Dan.Options.Config.autoOptions()["processIQ"] and merge_box.isHidden()
Dan.Options.Config.setAutoOptions({"processIQ": True, "merge": True})
assert not merge_box.isHidden()
dr.new_session("KWS1-2020", dr.data_folder("kws1", "*.DAT"))
Dan.Options.Config.setAutoOptions({"processIQ": True, "merge": True})
for name in ("QI-SM-53217-H-J", "H-J", "H-L", "H-M"):
    if table(name):
        closeWindow(table(name))
assert p.autoConfig("auto-iq") == "auto-iq", p.lastError()
assert table("QI-SM-53217-H-J") and all(table(n) for n in ("H-J", "H-L", "H-M"))
Dan.Options.Config.setAutoOptions({"processIQ": False})
assert merge_box.isHidden()
Dan.Options.Config.setAutoOptions({"merge": False})
# the info table alone: Dan.Processing.autoInfoTable(), the same runs; a table of that name is an error
assert p.autoInfoTable() == "info" and table("info").numRows() == len(selected)
raises(lambda: p.autoInfoTable("info"), "a table info exists already")
Dan.Options.Config.setAutoOptions({"skipSamples": "EB||Plexy||b4c"})
assert p.autoInfoTable("info-samples") == "info-samples" and table("info-samples").numRows() == 9
Dan.Options.Config.setAutoOptions({"skipSamples": ""})
# "other D" from flexibleFrom (3.5 m) on, above flexibleMin (1.5 m): the 19.68 m and 7.68 m configurations take the
# Plexiglas at 1.98 m; from 10 m on only 19.68 m, above 2 m none (no calibrant: the direct beam; the cell of the
# calibrant chosen before emptied, it stayed)
assert (Dan.Options.Config.autoOptions()["flexibleFrom"], Dan.Options.Config.autoOptions()["flexibleMin"]) == (3.5, 1.5)
for values, expected in (({"flexibleFrom": 10}, ["53229", "", "53224"]),
                         ({"flexibleFrom": 3.5, "flexibleMin": 2}, ["", "", "53224"])):
    Dan.Options.Config.setAutoOptions(values)
    chosen = [c["calibrant"] for c in p.autoConfigurations()]
    assert chosen == expected, (values, chosen)
Dan.Options.Config.setAutoOptions({"flexibleMin": 1.5})
raises(lambda: Dan.Options.Config.setAutoOptions({"flexibleFrom": "far"}), "flexibleFrom: a number")

# --- KWS-3
dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
s = p.autoConfig("auto-kws3")
p.process(s)
assert Dan.Merge.join(s + "-mergingTemplate") == ["H-M", "H-J", "H-L"]
chosen = [tuple(p.configuration(i)[k] for k in ROLES) for i in (1, 2)]
assert chosen == [("00082558", "00000002", "", "00082558", "00000002", "00082558", "00082558", True),
                  ("00082645", "00000002", "", "00082645", "00000002", "00082645", "00082645", True)], chosen
assert [p.configuration(i)["mask"] for i in (1, 2)] == ["mask-bs-9.2m", "mask-bs-1.2m"]
assert [p.configuration(i)["transmissionMask"] for i in (1, 2)] == ["mask-tr-9.2m", "mask-tr-1.2m"]
assert [round(p.configuration(i)["factor"]) for i in (1, 2)] == [280100, 2816]
s = table("auto-kws3")
cn = list(s.colNames())
tr = {(s.text(cn.index("Run-info") + 1, r).strip(), s.text(cn.index("#-Condition") + 1, r)):
      float(s.text(cn.index("Transmission-Sample") + 1, r).split("[")[0]) for r in range(1, s.numRows() + 1)}
for r in range(1, s.numRows() + 1):
    sample, condition = s.text(cn.index("Run-info") + 1, r).strip(), s.text(cn.index("#-Condition") + 1, r)
    expected = 1.0 if condition == "1" else tr[(sample, "2")] / tr[(sample, "1")]
    assert abs(float(s.text(cn.index("Scale") + 1, r)) - expected) < 1e-9, (r, expected)

# --- the buttons pressed again after auto mode change nothing: factor and center headers, Calculate Tr, I[Q], Merge
try:
    from PyQt6.QtCore import QMetaObject, Q_ARG
    from PyQt6.QtWidgets import QCheckBox, QToolButton
except ImportError:
    from PyQt5.QtCore import QMetaObject, Q_ARG
    from PyQt5.QtWidgets import QCheckBox, QToolButton
from qtisas_testing import DIRECT


def snapshot():
    st = table("auto-kws3")
    col = list(st.colNames())
    script = [[st.text(col.index(k) + 1, r) for k in ("Transmission-Sample", "Scale")] for r in range(1, 7)]
    config = [(round(p.configuration(i)["factor"], 3), p.configuration(i)["xCenter"], p.configuration(i)["yCenter"])
              for i in (1, 2)]
    curves = {n: [(table(n).cell(1, r), table(n).cell(2, r)) for r in (1, 20, table(n).numRows())]
              for n in ("QI-SM-00082646-H-J", "H-L")}
    return script, config, curves, sorted(tableNames())


before = snapshot()
for row in (13, 15):  # dptACFAC, dptCENTERX: the configuration table's headers
    QMetaObject.invokeMethod(Dan, "vertHeaderTableECPressed", DIRECT, Q_ARG(int, row), Q_ARG(bool, True))
for button in ("pushButtonCalcTr", "pushButtonIQ", "pushButtonMerge"):
    Dan.findChild(QToolButton, button).click()
after = snapshot()
for part, (x, y) in zip(("script table", "configurations", "curves", "tables"), zip(before, after)):
    assert x == y, (part, x, y)

# --- the session keeps auto mode's rules: they are in the script's Settings table and come back with it
settings = table("auto-kws3-Settings")
keys = [settings.text(1, r) for r in range(1, settings.numRows() + 1)]
assert "Options::AutoMode::Center-Mode" in keys
settings.setText(2, keys.index("Options::AutoMode::Min-Duration") + 1, "7 <")  # as if the session had used 7 s
Dan.Options.Config.setAutoOptions({"center": "wildcard"})
Dan.Options.openSession("auto-kws3")
options = Dan.Options.Config.autoOptions()
assert (options["center"], options["minDuration"]) == ("emptybeam", 7), options

# --- centers from the empty beams, a beam-stop mask per configuration, a flat sensitivity: "mask" without the beam
# stop, "sens" = "mask", so the direct beam's center and factor count the beam (with the beam stop: 432840, 4238)
dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
assert Dan.Options.Config.autoOptions()["scale"] is True  # the KWS-3 presets: a Scale column (default: none)
Dan.Options.Config.setAutoOptions({"center": "emptybeam", "masks": "configuration", "sensitivity": "flat",
                                   "scale": False})
p.autoConfig("auto-flat")
assert "Scale" not in table("auto-flat").colNames()  # the option off: no Scale column (KWS-3 has one otherwise)


def zeros(name):
    m = matrix(name)
    return {(i, j) for i in range(1, m.numRows() + 1) for j in range(1, m.numCols() + 1) if m.cell(i, j) == 0}


Dan.Mask.newMask("edge-only", beamstop=0)
assert zeros("mask") == zeros("edge-only") == zeros("sens"), (len(zeros("mask")), len(zeros("edge-only")))
assert [p.configuration(i)["mask"] for i in (1, 2)] == ["mask-bs-9.2m", "mask-bs-1.2m"]
assert [round(p.configuration(i)["factor"]) for i in (1, 2)] == [253070, 2654], \
    [p.configuration(i)["factor"] for i in (1, 2)]
# the auto option openMask: auto (above: these three options); yes: "mask" without the beam stop whatever the other
# options (KWS1-2020: masks none, its instrument's beam stop in "mask" otherwise)
assert Dan.Options.Config.autoOptions()["openMask"] == "auto"
raises(lambda: Dan.Options.Config.setAutoOptions({"openMask": "maybe"}), "openMask: one of auto, yes, no")
kws1 = dr.data_folder("kws1", "*.DAT")
dr.new_session("KWS1-2020", kws1)
assert Dan.Mask.autoMasks() == ["mask"]
with_beamstop = zeros("mask")
Dan.Mask.newMask("edge-only", beamstop=0)  # (it leaves the beam stop off: a new session below)
edge = zeros("edge-only")
assert with_beamstop > edge
dr.new_session("KWS1-2020", kws1)
Dan.Options.Config.setAutoOptions({"openMask": "yes"})
assert Dan.Mask.autoMasks() == ["mask"] and zeros("mask") == edge

# --- center offsets (auto option centerOffsets, off by default): each sample's center onto its empty beam's, in the
# script table's HShift, VShift; the empty beam itself has none
dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
assert Dan.Options.Config.autoOptions()["centerOffsets"] is False
Dan.Options.Config.setAutoOptions({"centerOffsets": True})
st = table(p.autoConfig("auto-offsets"))
cn = list(st.colNames())
shifts = [(float(st.text(cn.index("HShift") + 1, r)), float(st.text(cn.index("VShift") + 1, r)))
          for r in range(1, st.numRows() + 1)]
assert len(shifts) == 6 and all(abs(h) < 1 and abs(v) < 1 for h, v in shifts) and any(h != 0 for h, _ in shifts), shifts
eb = p.configuration(1)
h, v = p.centerOffset(eb["emptyBeam"], eb["xCenter"], eb["yCenter"], "mask", "sens")  # as its center was found
assert abs(h) < 0.01 and abs(v) < 0.01, (h, v)
x, y = p.runCenter(eb["emptyBeam"], "mask", "sens")
assert abs(x - eb["xCenter"]) < 0.01 and abs(y - eb["yCenter"]) < 0.01, (x, y, eb["xCenter"], eb["yCenter"])
assert p.addCenterOffsets("auto-offsets") == 6  # written again, the same
assert [(float(st.text(cn.index("HShift") + 1, r)), float(st.text(cn.index("VShift") + 1, r)))
        for r in range(1, st.numRows() + 1)] == shifts
raises(lambda: p.runCenter("99999"), "no run 99999")
raises(lambda: p.addCenterOffsets("auto-offsets", mask="no-such-mask"), "no mask no-such-mask")
raises(lambda: p.addCenterOffsets("no-such-table"), "no script table no-such-table")

# --- center "beamstop", transmissions measured at 9.2 m only: the 1.2 m configuration has no direct beam, so no
# center run (its center: that of its beam-stop mask, the instrument's box 123..133) and no transmission mask (its
# beam-stop mask instead); 9.2 m: the max-flux center, its masks around the direct beam
dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
Dan.Options.Config.setAutoOptions({"center": "beamstop", "masks": "configuration", "transmission": "*D9.2*"})
assert Dan.Options.Config.autoOptions()["center"] == "beamstop"
assert Dan.Mask.autoMasks() == ["mask", "mask-bs-9.2m", "mask-tr-9.2m", "mask-bs-1.2m"]
assert "|123|133|123|133|" in matrix("mask-bs-1.2m").windowLabel()
assert Dan.Sensitivity.autoSensitivity() == "sens"
chosen = [(c["center"], c["mask"], c["transmissionMask"], c["transmission"]) for c in p.autoConfigurations()]
assert chosen == [("00082561", "mask-bs-9.2m", "mask-tr-9.2m", True),
                  ("", "mask-bs-1.2m", "mask-bs-1.2m", False)], chosen
p.calculate()
c = p.configuration(2)
assert (c["center"], c["xCenter"], c["yCenter"]) == ("", 128.0, 128.0), c

# --- separate transmission configurations (Skip Transmission Configurations, on in KWS3-2020, with the direct beam's
# Transmission Method): of the configurations with the same beam-stop mask (here another beam center) the one whose
# empty beam is the direct beam measures the transmissions, not the one whose "EB" is a sample's scattering; without
# the option every one the wildcard matches
work = tempfile.mkdtemp()
try:
    src = dr.data_folder("kws3", "*.yaml")
    for f in os.listdir(src):
        shutil.copy(os.path.join(src, f), work)
    for ext in ("det", "yaml"):
        shutil.copy(os.path.join(src, "00082561_0000_H-L_HRD_standard." + ext),
                    os.path.join(work, "00082599_0000_EB_HRD_standard." + ext))
    dr.new_session("KWS3-2020", work)
    Dan.Rawdata.setHeaderValue("00082599", "Sample-Title", "EB")
    Dan.Rawdata.setHeaderValue("00082599", "Detector-X || Beamcenter-X", 130)
    skip_tr = Dan.findChild(QCheckBox, "checkBoxSkiptransmisionConfigurations")
    assert skip_tr.isChecked() and Dan.Options.Config.autoOptions()["transmission"] == "*"
    chosen = [(c["emptyBeam"], c["transmission"]) for c in p.autoConfigurations()]
    assert chosen == [("00082558", True), ("00082599", False), ("00082645", True)], chosen
    # transmissions from the header (not the direct beam): measured in every configuration, none separate
    method = Dan.Options.Config.transmissionMethod()
    Dan.Options.Config.setTransmissionMethod("ROI in Header  [dead-time +]")
    chosen = [(c["emptyBeam"], c["transmission"]) for c in p.autoConfigurations()]
    assert chosen == [("00082558", True), ("00082599", True), ("00082645", True)], chosen
    Dan.Options.Config.setTransmissionMethod(method)
    skip_tr.setChecked(False)
    chosen = [(c["emptyBeam"], c["transmission"]) for c in p.autoConfigurations()]
    assert chosen == [("00082558", True), ("00082599", True), ("00082645", True)], chosen
finally:
    shutil.rmtree(work)

# --- SANS-1 names its runs Ebeam, Ecell, B4C, H2O: its own preset; the 4 m configuration picks the holdout's runs
dr.new_session("SANS1", dr.data_folder("sans1", "D*.001"))
assert Dan.Options.Config.autoOptions()["emptyBeam"] == "*cell*"
chosen = {c["emptyBeam"]: tuple(c[k] for k in ROLES) for c in p.autoConfigurations()}
assert chosen["497844"] == ("497844", "497847", "497846", "497844", "497847", "497842", "497840", True), chosen

# --- every built-in family has its preset; an instrument without "[AutoMode-...]" lines gets the defaults
for instrument, values in (("KWS1", {"center": "maxflux", "sensitivity": "files", "masks": "none"}),
                           ("KWS1-He3", {"calibrant": "*plex[i,y]*", "flexibleDistance": True}),
                           ("KWS2", {"masks": "wavelength", "detectorY": 520, "skip": "*C20D17*"}),
                           ("KWS2-HRD", {"masks": "wavelength", "sensitivity": "flat"}),
                           ("KWS3-2018", {"center": "emptybeam", "calibrant": "", "beamSize": 2}),
                           ("KWS3-VHRD", {"masks": "configuration", "beamSize": 1, "scale": True}),
                           ("KWS1-2020", {"scale": False}), ("SANS1", {"scale": False}),
                           ("MARIA", {"emptyBeam": "*eb*||*ec*||*empty*cell*||*empty*beam*", "masks": "none"})):
    with dr.quiet():
        Dan.Options.selectInstrument(instrument)
    options = Dan.Options.Config.autoOptions()
    assert all(options[k] == v for k, v in values.items()), (instrument, options)

# --- options checked when set
Dan.Options.Config.setAutoOptions({"center": "emptybeam", "minDuration": 10, "singleDarkCurrent": True})
options = Dan.Options.Config.autoOptions()
assert (options["center"], options["minDuration"], options["singleDarkCurrent"]) == ("emptybeam", 10, True), options
raises(lambda: Dan.Options.Config.setAutoOptions({"centre": "hf"}), "no auto option centre")
raises(lambda: Dan.Options.Config.setAutoOptions({"masks": "all"}), "masks: one of none, configuration")
