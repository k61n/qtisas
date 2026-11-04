"""The Q resolution (KWS-3 example runs, a focusing instrument; skipped without the qtisas.com example files):

- "Resolution:: Options": the focusing optic, "lenses" or "mirror", a combo box beside "Focusing Geometry" shown only
  when it is checked; the instrument's `[Resolusion-Focusing-Type]` (KWS3-2020: mirror, KWS1-2020: lenses), set by
  Config.setParameters(), kept by a session.
- The chromatic term and gravity of the focusing sigma only for lenses (a mirror is achromatic and compensates
  gravity): sigma^2 of lenses minus that of a mirror is the same at every Q. 2f = C (the collimation), the sample at
  D: a mirror's sigma does not depend on C, the lenses' chromatic term grows with C^2, their gravity with C^4.
- The focusing terms as variances: the source image r1^2 / 4 (it was 0.4247^2 r1^2), the wavelength Q^2 dl^2 / 6
  (it was 0.4247^2 Q^2 dl^2 / 3), the lenses' blur 16 (dl^2 / 6) (r2 C / D)^2 / 4 and gravity (the fall
  A lambda^2 z^2 spread by 4 A lambda^2 C^2 e, vertically) (4 A lambda^2 C^2)^2 (dl^2 / 6) / 2; rectangular
  apertures with r^2 = (x^2 + y^2) / 6 (they were circles of the same area).
- The slices' sigma is the resolution at the slice's own Q (it was the mean over the pixels' full Q): I-Qx sigma at
  Qx = the I[Q] sigma at Q = |Qx|.
- Pinhole sigma (focusing off): the detector term "Sigma-D1" from "Detector Space Resolusion" (0: one pixel), the
  averaging term "Sigma-AV" from the ring's pixels (both took the larger of the two): a resolution of one pixel gives
  the sigma of none; the resolution is the detector's real pixel, a box (it was a FWHM divided by sqrt(pi)): twice
  it adds (4 - 1) w^2 / 12 of detector width.
- dQ: one estimate for I[Q] and the matrix dQ-x-y (they had two, neither from the header's wavelength spread; the
  matrix's was infinite at Q = 0): the matrix's dQ at a pixel = the I[Q] dQ at its Q and ring width, no NaN.
"""

import math
import os
import sys

try:
    from PyQt6.QtWidgets import QCheckBox, QComboBox, QLineEdit
except ImportError:
    from PyQt5.QtWidgets import QCheckBox, QComboBox, QLineEdit

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
o = p.Options
cfg = Dan.Options.Config
RUN = "00082561"  # H-L at 9.2 m
focus = Dan.findChild(QCheckBox, "checkBoxResoFocus")
optic = Dan.findChild(QComboBox, "comboBoxResoFocusType")

# --- the option
Dan.Options.selectInstrument("KWS1-2020")
assert not focus.isChecked() and optic.isHidden() and optic.currentText() == "lenses"
assert cfg.parameters()["Resolusion-Focusing-Type"] == "lenses"
dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
assert focus.isChecked() and not optic.isHidden() and optic.currentText() == "mirror", optic.currentText()
focus.setChecked(False)
assert optic.isHidden()
focus.setChecked(True)
assert not optic.isHidden()
p.autoConfig("resolution-base")
o.setRadialAveraging(method="hf", linear=1, progressive=1.0)
o.setCurveFormat("dq-sigma")

count = [0]


def made(kind, runs=(RUN,)):
    count[0] += 1
    s = p.newScriptTable("resolution-%d" % count[0], list(runs))
    names = [n for n in p.process(s, kind, "project") if RUN in n]
    assert names, (kind, p.lastMessages())
    return s, names[0]


def curve(kind="I-Q"):
    _, name = made(kind)
    t = table(name)
    rows = [[float(t.text(c, r)) for c in range(1, t.numCols() + 1)] for r in range(1, t.numRows() + 1)]
    closeWindow(t)
    return rows


# --- the chromatic term: lenses only
mirror = {round(r[0], 12): r[4] for r in curve()}
cfg.setParameters({"Resolusion-Focusing-Type": "lenses"})
assert optic.currentText() == "lenses"
lenses = {round(r[0], 12): r[4] for r in curve()}
differences = [lenses[q] ** 2 - mirror[q] ** 2 for q in mirror if q in lenses]
assert len(differences) > 50 and min(differences) > 0, differences[:3]
assert max(differences) / min(differences) < 1 + 1e-4, (min(differences), max(differences))

# --- the focusing terms as variances (a uniformly lit source image r1^2 / 4, the wavelength's triangle
# Q^2 dl^2 / 6, the lenses' blur 16 (dl^2 / 6) (r2 C / D)^2 / 4; the apertures' r^2 = (x^2 + y^2) / 6): a mirror's
# sigma^2 is linear in Q^2, its slope dl^2 / 6 sets the lenses' chromatic term; the intercept is the source's and
# the detector's
script = table(p.newScriptTable("resolution-terms", [RUN]))
names = list(script.colNames())
C = float(script.text(names.index("C") + 1, 1)) * 100
D = float(script.text(names.index("D") + 1, 1)) * 100
lam = float(script.text(names.index("Lambda") + 1, 1))
beam = script.text(names.index("Beam Size") + 1, 1)  # "CA-X x CA-Y | SA-X x SA-Y" [mm]
(cx, cy), (sx, sy) = [[float(v) / 10 for v in a.split("x")] for a in beam.split("|")]
r1sq, r2sq = (cx * cx + cy * cy) / 6, (sx * sx + sy * sy) / 6
k0 = 2 * math.pi / lam
qs = sorted(mirror)
slope = (mirror[qs[-1]] ** 2 - mirror[qs[len(qs) // 2]] ** 2) / (qs[-1] ** 2 - qs[len(qs) // 2] ** 2)
for q in qs[::10]:
    line = mirror[qs[-1]] ** 2 + slope * (q * q - qs[-1] ** 2)
    assert abs(mirror[q] ** 2 / line - 1) < 1e-6, (q, mirror[q] ** 2, line)
pixel = float(Dan.findChild(QLineEdit, "lineEditResoPixelSize").text()) * float(
    Dan.findChild(QComboBox, "comboBoxBinning").currentText())
fwhm = float(Dan.findChild(QLineEdit, "lineEditDetReso").text())
intercept = k0 * k0 * (r1sq / 4 + (fwhm * fwhm + pixel * pixel) / 12) / D / D
measured = mirror[qs[-1]] ** 2 - slope * qs[-1] ** 2
assert abs(measured / intercept - 1) < 1e-4, (measured, intercept)
fall = 3.13311e-9 * lam * lam  # A lambda^2 [1/cm]: g m^2 / (2 h^2)
chromatic = k0 * k0 * 16 * slope * r2sq * C * C / 4 / D ** 4
gravity = k0 * k0 * (4 * fall * C * C) ** 2 * slope / 2 / D ** 2
assert abs(differences[0] / (chromatic + gravity) - 1) < 1e-4, (differences[0], chromatic, gravity)

# --- 2f = C (the collimation), the sample at D: a mirror's sigma does not depend on C; the lenses' chromatic term
# grows with C^2 (the beam at the optic r2 C / D, the image plane moved by 4 C dlambda), their gravity with C^4
def sigmas_with_c(c):
    count[0] += 1
    s = p.newScriptTable("resolution-c-%d" % count[0], [RUN])
    t = table(s)
    t.setText(list(t.colNames()).index("C") + 1, 1, str(c))
    names = [n for n in p.process(s, "I-Q", "project") if RUN in n]
    assert names, p.lastMessages()
    c_table = table(names[0])
    values = {round(float(c_table.text(1, r)), 12): float(c_table.text(5, r)) for r in range(1, c_table.numRows() + 1)}
    closeWindow(c_table)
    return values


c0 = float(table(p.newScriptTable("resolution-c-0", [RUN])).text(5, 1))  # the row's C [m]
cfg.setParameters({"Resolusion-Focusing-Type": "mirror"})
assert sigmas_with_c(c0) == sigmas_with_c(2 * c0)
cfg.setParameters({"Resolusion-Focusing-Type": "lenses"})
extra = []
for c in (c0, 2 * c0):
    lens = sigmas_with_c(c)
    extra.append(sum(lens[q] ** 2 - mirror[q] ** 2 for q in mirror if q in lens) / len(mirror))
growth = (4 * chromatic + 16 * gravity) / (chromatic + gravity)
assert abs(extra[1] / extra[0] / growth - 1) < 1e-3, (extra, growth)

# --- a session keeps the optic
s, name = made("I-Q")
closeWindow(table(name))  # process() returns what it makes new
cfg.setParameters({"Resolusion-Focusing-Type": "mirror"})
Dan.Options.openSession(s)
assert optic.currentText() == "lenses", optic.currentText()
cfg.setParameters({"Resolusion-Focusing-Type": "mirror"})

# --- the slices: sigma at the slice's own Q
iq = curve()
qs, sigmas = [r[0] for r in iq], [r[4] for r in iq]


def sigma_at(q):
    for k in range(1, len(qs)):
        if qs[k - 1] <= q <= qs[k]:
            t = (q - qs[k - 1]) / (qs[k] - qs[k - 1])
            return sigmas[k - 1] + t * (sigmas[k] - sigmas[k - 1])
    return None


checked = 0
o.setCurveFormat("sigma")  # the slices write sigma only with "Q-I-dI-sigma"
for q, _, _, sigma in [r[:4] for r in curve("I-Qx")]:
    expected = sigma_at(abs(q))
    if expected:
        assert abs(sigma / expected - 1) < 1e-3, (q, sigma, expected)
        checked += 1
assert checked > 50, checked
o.setCurveFormat("dq-sigma")

# --- dQ: the same in the matrix and in I[Q]; nothing infinite
_, qname = made("Q-x-y")
_, dqname = made("dQ-x-y")
qm, dqm = matrix(qname), matrix(dqname)
dq_of_iq = {round(r[0], 12): r[3] for r in iq}
pairs = []
for i in range(1, qm.numRows() + 1, 3):
    for j in range(1, qm.numCols() + 1, 3):
        q, dq = qm.cell(i, j), dqm.cell(i, j)
        assert not math.isnan(dq) and not math.isinf(dq), (i, j, dq)
        if q > 0 and dq > 0:
            pairs.append((q, dq))
assert len(pairs) > 1000, len(pairs)
# at large Q the radius term fades: dQ / Q -> the same constant in both
far_matrix = sorted(pairs)[-50:]
far_iq = sorted(dq_of_iq.items())[-20:]
ratio_matrix = sum(dq / q for q, dq in far_matrix) / len(far_matrix)
ratio_iq = sum(dq / q for q, dq in far_iq) / len(far_iq)
assert abs(ratio_matrix / ratio_iq - 1) < 0.01, (ratio_matrix, ratio_iq)
o.setCurveFormat("sigma")

# --- pinhole: Sigma-D1 from the detector resolution, Sigma-AV from the pixels of the ring
focus.setChecked(False)
o.setCurveFormat("sigma")
resolution = Dan.findChild(QLineEdit, "lineEditDetReso")
pixel = float(Dan.findChild(QLineEdit, "lineEditResoPixelSize").text())
binning = float(Dan.findChild(QComboBox, "comboBoxBinning").currentText())
width = pixel * binning
info = Dan.Rawdata.info([RUN])[0]
D, wavelength = info["D"] * 100, info["lambda"]
sigmas = {}
for value in ("0", repr(width), repr(2 * width)):
    resolution.setText(value)
    sigmas[value] = {round(r[0], 12): r[3] for r in curve()}
assert sigmas["0"] == sigmas[repr(width)]
k0 = 2 * math.pi / wavelength
checked = 0
for q, s1 in sigmas["0"].items():
    s2 = sigmas[repr(2 * width)][q]
    theta0 = math.asin(q / 2 / k0)
    c = math.cos(2 * theta0) ** 2
    added = (k0 * math.cos(theta0) * c) ** 2 * (4 - 1) * width ** 2 / 12 / D ** 2
    assert abs((s2 * s2 - s1 * s1) / added - 1) < 1e-3, (q, s1, s2, added)
    checked += 1
assert checked > 50, checked
resolution.setText("0.08")
focus.setChecked(True)
