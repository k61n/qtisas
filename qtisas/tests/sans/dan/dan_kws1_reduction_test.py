"""The KWS-1 example of qtisas.com/dan-sans reduced through Dan.Options, Dan.Processing, ... (dan_reduction.py;
skipped without its files).

- "Polarization as parameter" (on for KWS1-2020): each run gets its configuration in the script table — the
  comparison read the header of a run named like the polarization value, so every condition came out 0 (174d3540).
- Beam centers (H.F. moments): the same from any start value, and stable when pressed again — the iteration ended
  after 50 steps while still jumping between two centers; it now starts at the mask's beam-stop center.
- The calibration runs' dark current defaults to the configuration's.
- "Standard (Shells)" radial averaging: one ring per pixel — with the centers of dan-example-kws1.qti every I[Q]
  table and the merged ones are that project's, row by row (rings of equal angle gave 72 instead of 74 rows at 2 m).
- Each Dan part shows its own tab: Processing.run makes "QI-SM-..." even when the Rawdata tab was open (radial
  averaging named its tables "raw-..." there).
"""

import os
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    from PyQt6.QtWidgets import QCheckBox, QTabWidget
except ImportError:
    from PyQt5.QtWidgets import QCheckBox, QTabWidget

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
made = Dan.Processing.run(script)
assert tabs.currentIndex() == 4, tabs.currentIndex()
assert "QI-SM-53217-H-J" in made and script + "-mergingTemplate" in made, made
assert Dan.Merge.run(script + "-mergingTemplate") == ["H-J", "H-L", "H-M"]
assert tabs.currentIndex() == 5, tabs.currentIndex()
reference = {  # dan-example-kws1.qti: rows, (Q, I) of rows 1, 20 and the last (merged tables saved to 7 digits)
    "QI-SM-53217-H-J": (76, ((0.00270696975, 595.856827), (0.0129929703, 1.83282018), (0.0432928456, 0.0621309327))),
    "QI-SM-53221-H-J": (76, ((0.00693654466, 11.2781022), (0.0332872661, 0.192008152), (0.110671861, 0.105337522))),
    "QI-SM-53228-H-M": (74, ((0.0269011888, 0.180945959), (0.128653254, 0.104600095), (0.403763094, 0.0921452575))),
    "H-L": (164, ((0.00270697, 1738.1), (0.01299297, 3.817852), (0.4037631, 0.1009857))),
}
for name, (count, values) in reference.items():
    dr.check_rows(name, count, values, rel=1e-6 if name.startswith("QI") else 5e-6)

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
