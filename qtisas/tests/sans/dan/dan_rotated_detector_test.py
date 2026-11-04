"""Radial averaging on a rotated detector (KWS-3 example runs at 1.2 m; skipped without the qtisas.com example files):
the 4-Pixel Interpolation (H.F.) samples each ring along the cone of its scattering angle around the beam, met by the
tilted detector, as Standard (Shells) puts each pixel into the ring of its angle; it took circles on the detector
(the rotation ignored). With a constant rotation the two methods give the same I[Q]; without one H.F. is unchanged.
"""

import os
import statistics
import sys

try:
    from PyQt6.QtWidgets import QDoubleSpinBox, QRadioButton
except ImportError:
    from PyQt5.QtWidgets import QDoubleSpinBox, QRadioButton

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
o = p.Options
RUN = "00082646"  # H-J at 1.2 m: the largest angles

dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
p.autoConfig("rotated-base")
count = [0]


def curve(method, rotation):
    Dan.findChild(QRadioButton, "radioButtonDetRotConstX").setChecked(True)
    Dan.findChild(QDoubleSpinBox, "doubleSpinBoxDetRotX").setValue(rotation)
    o.setRadialAveraging(method=method, linear=1, progressive=1.0)
    count[0] += 1
    s = p.newScriptTable("rotated-%d" % count[0], [RUN])
    names = [n for n in p.process(s, "I-Q", "project") if n.startswith("QI")]
    assert names, p.lastMessages()
    t = table(names[0])
    rows = [(float(t.text(1, r)), float(t.text(2, r))) for r in range(1, t.numRows() + 1)]
    closeWindow(t)
    return rows


def at(rows, q):
    for (q1, i1), (q2, i2) in zip(rows, rows[1:]):
        if q1 <= q <= q2:
            return i1 + (q - q1) / (q2 - q1) * (i2 - i1)
    return None


def ratios(a, b):
    """I of a over I of b at a's Q, in the middle of the curves (beyond the beam stop, before the edge)."""
    out = []
    for q, i in a[len(a) // 5: 4 * len(a) // 5]:
        j = at(b, q)
        if j and i > 0 and j > 0:
            out.append(i / j)
    return out


try:
    hf0 = curve("hf", 0.0)
    shells0 = curve("shells", 0.0)
    hf = curve("hf", 1.0)
    shells = curve("shells", 1.0)
    agree0 = statistics.median(ratios(hf0, shells0))
    agree = statistics.median(ratios(hf, shells))
    moved = statistics.median(abs(r - 1) for r in ratios(hf, hf0))
    # the methods agree with the rotation as without it; the rotation itself changes the curve
    assert abs(agree0 - 1) < 0.02, agree0
    assert abs(agree - 1) < 0.02, (agree, agree0)
    assert moved > 0.02, moved
finally:
    Dan.findChild(QDoubleSpinBox, "doubleSpinBoxDetRotX").setValue(0.0)
    o.setRadialAveraging(method="hf")
