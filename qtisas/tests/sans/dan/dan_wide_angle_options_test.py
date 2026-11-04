"""Wide-angle corrections (KWS3-2020 at 1.2 m; skipped without the qtisas.com example files):

- Dan.Processing.Options.setWideAngle(parallax, transmission, window) / wideAngle(): the three options of
  "I[x,y] :: Wide Angle Corrections"; another parallax word is a ValueError.
- The parallax correction is centered on the row's beam center, not on its mask's beam-stop box: a mask with the
  beam stop in a corner gives the same I[Q] as one with it on the beam, where neither box reaches.
- The detector window's factor multiplies the data after the dark current is subtracted (the dark current does not
  cross the window), relative to the window factor in the sensitivity's own measurement: the 2D image with the window
  is the one without, times T^(1 - 1/cos) of the row over T^(1 - 1/cos) of the sensitivity, pixel by pixel.
"""

import math
import os
import sys

try:
    from PyQt6.QtWidgets import QComboBox, QLineEdit
except ImportError:
    from PyQt5.QtWidgets import QComboBox, QLineEdit

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
o = p.Options
RUN = "00082646"  # H-J at 1.2 m

dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
p.autoConfig("wide-base")

# --- the options
o.setWideAngle(parallax="tubes", transmission=True, window=False)
assert o.wideAngle() == {"parallax": "tubes", "transmission": True, "window": False}, o.wideAngle()
o.setWideAngle(parallax="flat")  # the others as they are
assert o.wideAngle() == {"parallax": "flat", "transmission": True, "window": False}, o.wideAngle()
o.setWideAngle(parallax="off", transmission=False)
assert o.wideAngle() == {"parallax": "off", "transmission": False, "window": False}, o.wideAngle()
try:
    o.setWideAngle(parallax="cos3")
    raise AssertionError("no ValueError")
except ValueError as e:
    assert "one of off, flat" in str(e), str(e)


def row_table(name, mask=None, trdet=None):
    """A script table of the run alone (its row as auto mode made it), optionally another mask and a TrDet column."""
    s = p.newScriptTable(name, [RUN])
    t = table(s)
    cn = list(t.colNames())
    if mask:
        t.setText(cn.index("Mask") + 1, 1, mask)
    if trdet is not None:
        t.addColumns(1)
        t.setColName(t.numCols(), "TrDet")
        t.setText(t.numCols(), 1, str(trdet))
    return s


def made(s, kind):
    names = [n for n in p.process(s, kind) if "-%s-" % RUN in n]
    assert names, (s, kind, p.lastMessages())
    return names[0]


def i_q(s):
    t = table(made(s, "I-Q"))
    values = [(float(t.text(1, r)), float(t.text(2, r))) for r in range(1, t.numRows() + 1)]
    closeWindow(t)
    return values


# --- the parallax correction around the row's beam center: the beam stop of the mask in a corner changes nothing
# where neither box reaches (rings of 10..100 pixels; the corner box is 140 pixels away)
o.setWideAngle(parallax="flat", transmission=True)
Dan.Mask.newMask("mask-corner", beamstop=[20, 30, 20, 30])
on_beam = i_q(row_table("wide-on-beam"))
in_corner = i_q(row_table("wide-in-corner", mask="mask-corner"))
pixel = float(Dan.findChild(QLineEdit, "lineEditResoPixelSize").text())
binning = float(Dan.findChild(QComboBox, "comboBoxBinning").currentText())
D = 120.0  # cm


def q_at(r):
    return 4 * math.pi / 12.787 * math.sin(0.5 * math.atan(r * pixel * binning / D))


corner = {round(q, 9): i for q, i in in_corner}  # the curves start at other Q: compared at the same Q
compared = [(i, corner[round(q, 9)]) for q, i in on_beam if q_at(10) < q < q_at(100) and round(q, 9) in corner]
assert len(compared) > 20, len(compared)
worst = max(abs(b / a - 1) for a, b in compared)
assert worst < 2e-8, worst  # 9 digits in the tables

# --- the detector window: after the dark current, relative to the sensitivity's window factor
o.setWideAngle(parallax="off", transmission=False, window=False)
without = matrix(made(row_table("window-off"), "I-x-y"))
values_off = [[without.cell(i, j) for j in range(1, without.numCols() + 1)] for i in range(1, without.numRows() + 1)]
closeWindow(without)  # process() returns what it makes new
o.setWideAngle(window=True)
TRDET = 0.5
s = row_table("window-on", trdet=TRDET)
row = table(s)
cn = list(row.colNames())
xc = float(row.text(cn.index("X-center") + 1, 1)) - 1
yc = float(row.text(cn.index("Y-center") + 1, 1)) - 1
d_row = float(row.text(cn.index("D") + 1, 1)) * 100
with_window = matrix(made(s, "I-x-y"))
xs, ys, _ = Dan.Sensitivity.center("sens")
d_sens = Dan.Rawdata.info(["00000001"])[0]["D"] * 100  # the sensitivity run
asym = float(Dan.findChild(QLineEdit, "lineEditAsymetry").text())


def window(x, y, x0, y0, d):
    r = pixel * binning * math.hypot(x - x0, asym * (y - y0))
    return TRDET ** (1.0 - 1.0 / math.cos(math.atan(r / d)))


checked = 0
worst = 0.0
for i in range(0, with_window.numRows(), 7):
    for j in range(0, with_window.numCols(), 7):
        a = values_off[i][j]
        if a == 0:
            continue
        expected = a * window(j, i, xc, yc, d_row) / window(j, i, xs - 1, ys - 1, d_sens)
        worst = max(worst, abs(with_window.cell(i + 1, j + 1) / expected - 1))
        checked += 1
assert checked > 500, checked
assert worst < 1e-6, worst
o.setWideAngle(window=False)
