"""The error bars of I[Q] with DAN's matrix smoothing ("Matrix convolusion": 5 neighbours, 3 x 3, 5 x 5) are those of
the data, not sqrt(n) too small: a smoothed pixel holds the counts of its neighbours too, and the radial average
takes the variances from the pixels before smoothing. Checked against the data themselves: 16 copies of one KWS-3
run with fresh Poisson counts, the spread of their I[Q] against the error bars, the same with and without smoothing
(skipped without the qtisas.com example files).

"Mask negative points" masks a row's negative pixels for that row only: they stayed masked in every following row of
the same mask.
"""

import math
import os
import shutil
import statistics
import sys
import tempfile

import numpy as np

try:
    from PyQt6.QtWidgets import QCheckBox
except ImportError:
    from PyQt5.QtWidgets import QCheckBox

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
kws3 = dr.data_folder("kws3", "*.yaml")
SOURCE = "00082561_0000_H-L_HRD_standard"  # H-L at 9.2 m
COPIES = 16


def write_copy(work, run, counts):
    shutil.copy(os.path.join(kws3, SOURCE + ".yaml"), os.path.join(work, "%s_0000_H-L_HRD_standard.yaml" % run))
    with open(os.path.join(work, "%s_0000_H-L_HRD_standard.det" % run), "w") as f:
        f.write("\n".join(" ".join(str(int(v)) for v in row) for row in counts) + "\n")


def curve(name):
    t = table(name)
    return [(float(t.text(1, r)), float(t.text(2, r)), float(t.text(3, r))) for r in range(1, t.numRows() + 1)]


with open(os.path.join(kws3, SOURCE + ".det")) as f:
    mean = np.array([[float(v) for v in line.split()] for line in f])
rng = np.random.default_rng(12345)
runs = ["%08d" % (90000 + k) for k in range(COPIES)]

work = tempfile.mkdtemp()
try:
    for f in os.listdir(kws3):
        shutil.copy(os.path.join(kws3, f), work)
    for run in runs:
        write_copy(work, run, rng.poisson(mean))

    # error bars / spread of the copies' I[Q], per smoothing (none, 5, 9, 25 pixels)
    ratios = []
    for smoothing in range(4):
        dr.new_session("KWS3-2020", work)
        p.Options.setSmoothing((1, 5, 9, 25)[smoothing])
        s = p.autoConfig("smooth-%d" % smoothing)
        p.process(s, "I-Q")
        t = table(s)
        cn = list(t.colNames())
        names = ["QI-SM-%s-%s" % (t.text(cn.index("#-Run") + 1, r).strip(), t.text(cn.index("Run-info") + 1, r).strip())
                 for r in range(1, t.numRows() + 1) if t.text(cn.index("#-Run") + 1, r).strip() in runs]
        assert len(names) == COPIES, names
        curves = [curve(n) for n in names]
        points = min(len(c) for c in curves)
        per_q = []
        for i in range(20, points - 5):  # beyond the beam stop's edge
            values = [c[i][1] for c in curves]
            spread = statistics.stdev(values)
            if spread > 0:
                per_q.append(statistics.mean(c[i][2] for c in curves) / spread)
        ratios.append(statistics.median(per_q))
        for n in names:
            closeWindow(table(n))
    # without smoothing the error bars match the spread; with it the same, not 1/sqrt(n) of it (0.45, 0.33, 0.2)
    assert 0.8 < ratios[0] < 1.25, ratios
    for smoothing in (1, 2, 3):
        assert abs(ratios[smoothing] / ratios[0] - 1) < 0.15, ratios

    # "Mask negative points": a row's negative pixels masked for it alone. Two rows of one mask: a copy with no
    # counts in a band (negative after the empty beam's subtraction there) first, then a normal copy: the normal
    # copy's I[Q] is the one it has alone
    holes = rng.poisson(mean)
    holes[100:160, :] = 0
    write_copy(work, "00099999", holes)
    dr.new_session("KWS3-2020", work)
    p.Options.setSmoothing(1)
    Dan.findChild(QCheckBox, "checkBoxMaskNegative").setChecked(True)
    p.autoConfig("negative")
    name = "QI-SM-%s-H-L" % runs[0]
    p.process(p.newScriptTable("negative-alone", [runs[0]]), "I-Q")
    alone = curve(name)
    closeWindow(table(name))
    p.process(p.newScriptTable("negative-pair", ["00099999", runs[0]]), "I-Q")
    after = curve(name)
    assert len(after) == len(alone)
    differ = [(a, b) for a, b in zip(alone, after) if not math.isclose(a[1], b[1], rel_tol=1e-9, abs_tol=1e-12)]
    assert not differ, (len(differ), differ[:3])
finally:
    Dan.findChild(QCheckBox, "checkBoxMaskNegative").setChecked(False)
    shutil.rmtree(work)
