"""The KWS-1 example of qtisas.com/dan-sans reduced through Dan.Options, Dan.Processing, ... (dan_reduction.py;
skipped without its files).

- "Polarization as parameter" (on for KWS1-2020): each run gets its configuration in the script table — the
  comparison read the header of a run named like the polarization value, so every condition came out 0 (174d3540).
- Beam centers (H.F. moments): the same from any start value, and stable when pressed again — the iteration ended
  after 50 steps while still jumping between two centers; it now starts at the mask's beam-stop center.
- The calibration runs' dark current defaults to the configuration's.
- "Standard (Shells)" radial averaging: one ring per pixel — with the centers of dan-example-kws1.qti every I[Q]
  table and the merged ones are that project's, row by row (rings of equal angle gave 72 instead of 74 rows at 2 m).
- Dan.Merge.join() merges as the interface does (the template as the project table, the merge button): all three
  samples — from a project table the button left out the last row and read a column past the table.
- Dan.Merge.join() scales the curves only when asked (smart=True): "Smart" left on in the Merge tab rescaled them.
- Each Dan part shows its own tab: Processing.process makes "QI-SM-..." even when the Rawdata tab was open (radial
  averaging named its tables "raw-..." there).
"""

import os
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    from PyQt6.QtWidgets import QCheckBox, QGroupBox, QTabWidget
except ImportError:
    from PyQt5.QtWidgets import QCheckBox, QGroupBox, QTabWidget

import dan_reduction as dr

folder = dr.data_folder("kws1", "*.DAT")

# --- centers of the project on qtisas.com: I[Q] and merged tables as there
script = dr.kws1(folder, centers=(("73.514", "84.793"), ("73.138", "85.597"), ("72.691", "83.711")))
assert Dan.findChild(QCheckBox, "checkBoxPolarizationAsPara").isChecked()
s = table(script)
conditions = [s.text(dr.column(s, "#-Condition"), r) for r in range(1, s.numRows() + 1)]
assert conditions == ["1"] * 3 + ["2"] * 3 + ["3"] * 3, conditions
tabs = Dan.findChild(QTabWidget, "sansTab")
tabs.setCurrentIndex(1)  # the Rawdata tab, as a user may have left it
made = Dan.Processing.process(script)
assert tabs.currentIndex() == 4, tabs.currentIndex()
assert "QI-SM-53217-H-J" in made and script + "-mergingTemplate" in made, made
Dan.findChild(QGroupBox, "checkBoxSmart").setChecked(True)  # "Smart" left on in the interface: not used
assert Dan.Merge.join(script + "-mergingTemplate") == ["H-J", "H-L", "H-M"]
assert tabs.currentIndex() == 5, tabs.currentIndex()
# dan-example-kws1.qti: rows, (Q, I) of rows 1, 20 and the last. Its intensities predate two fixes of the wide-angle
# corrections (KWS1-2020: He-tubes parallax, on the data and on the Plexiglas calibrant): sin(alpha) from the offset in
# pixels over the distance in cm; their center the mask's beam-stop box, not the row's (the configuration's) beam
# center. Together 0.02..0.4 % (row 1 of 53217: 595.856827 -> 598.234004)
reference = {
    "QI-SM-53217-H-J": (76, ((0.00270696975, 598.234004), (0.0129929703, 1.84017106), (0.0432928456, 0.0623767956))),
    "QI-SM-53221-H-J": (76, ((0.00693654466, 11.3164326), (0.0332872661, 0.192647419), (0.110671861, 0.105676476))),
    "QI-SM-53228-H-M": (74, ((0.0269011888, 0.181545889), (0.128653254, 0.104871763), (0.403763094, 0.0919599957))),
    "H-L": (164, ((0.00270696975, 1745.03386), (0.0129929703, 3.83316711), (0.403763094, 0.100784219))),
}
for name, (count, values) in reference.items():
    dr.check_rows(name, count, values, rel=1e-6)

# --- centers calculated: the same from other start values, and when calculated again
dr.kws1(folder, samples=False)


def centers():
    c = [Dan.Processing.configuration(i) for i in (1, 2, 3)]
    return [("%.3f" % x["xCenter"], "%.3f" % x["yCenter"]) for x in c]


expected = [("73.250", "84.801"), ("73.253", "85.592"), ("73.134", "85.057")]
assert centers() == expected, centers()
for start in (("64.0", "64.0"), ("80.0", "90.0")):
    dr.set_centers([start] * 3)
    Dan.Processing.calculate()
    Dan.Processing.calculate()
    assert centers() == expected, (start, centers())

# --- the calibration runs' dark current is the configuration's unless given
dr.new_session("KWS1-2020", folder)
Dan.Processing.setConfigurations(1)
Dan.Processing.setConfiguration(1, emptyBeam="53216", darkCurrent="48462", calibrant="53229")
assert Dan.Processing.configuration(1)["calibrantDarkCurrent"] == "48462", Dan.Processing.configuration(1)

# --- transmissions without a configuration that measures them: a ValueError that says so, not "check transmission"
Dan.Processing.setConfiguration(1, center="53218", cellEmptyBeam="53216")
Dan.Processing.calculate()
name = Dan.Processing.newScriptTable("noTr", [53217])
try:
    Dan.Processing.calculateTransmissions(name)
    raise AssertionError("no ValueError")
except ValueError as e:
    assert "transmission=True" in str(e), str(e)

# the Rebin page: one Q grid of the points asked (the merged tables are made anew)
assert Dan.Merge.rebin(script + "-mergingTemplate", points=50) == ["H-J", "H-L", "H-M"]
assert table("H-L").numRows() <= 50, table("H-L").numRows()

# the session keeps whether the edge and the beam stop are on (before: only their boxes and shapes)
bs = Dan.findChild(QGroupBox, "groupBoxMaskBS")
settings = table(script + "-Settings")
keys = [settings.text(1, r) for r in range(1, settings.numRows() + 1)]
assert settings.text(2, keys.index("Mask::BeamStop::Status") + 1).startswith("Yes"), keys
assert "Mask::Edges::Status" in keys
bs.setChecked(False)
Dan.Options.openSession(script)
assert bs.isChecked()

