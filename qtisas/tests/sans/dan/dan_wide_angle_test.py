"""Wide-angle corrections, parallax "cos(α)⋅cos²(Θ) [He-tubes]": sin(α) is the vertical offset in cm over the
distance (the pixel offset was divided by the distance in cm: asin of more than 1, NaN, for the outer rows), so
I[Q] is finite, and close to the cos³(Θ) [Li] one at a small angle (KWS3-2020 at 1.2 m: Θ up to 2°; skipped without
the qtisas.com example files).

"Transmission :: Tr = f(cos Θ)" in buffer subtraction: the buffer's empty-cell term gets the buffer's own angular
part f(T_b) (it kept the sample's f(T)): sample minus buffer is exactly the sample reduced alone minus the buffer
reduced alone with its transmission.
"""

import math
import os
import sys

try:
    from PyQt6.QtWidgets import QCheckBox, QComboBox
except ImportError:
    from PyQt5.QtWidgets import QCheckBox, QComboBox

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
NAME = "QI-SM-00082646-H-J"  # 1.2 m

dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
s = p.autoConfig("wide-angle")
Dan.findChild(QCheckBox, "checkBoxParallax").setChecked(True)
curves = []
for kind in (0, 1):  # cos³(Θ) [Li], cos(α)⋅cos²(Θ) [He-tubes]
    Dan.findChild(QComboBox, "comboBoxParallax").setCurrentIndex(kind)
    if table(NAME):
        closeWindow(table(NAME))
    p.process(s, "I-Q")
    t = table(NAME)
    curves.append([float(t.text(2, r)) for r in range(1, t.numRows() + 1)])
li, he = curves
assert all(math.isfinite(v) for v in he), sum(1 for v in he if not math.isfinite(v))
assert len(li) == len(he)
deviation = max(abs(h / l - 1) for l, h in zip(li, he) if l > 0)
assert deviation < 0.005, deviation

# --- buffer subtraction with Tr(Θ): (BS) sample - buffer = (SM) sample alone - (SM) buffer alone with T_b
SAMPLE, BUFFER, TB = "00082646", "00082647", 0.9  # H-J, H-L at 1.2 m; T_b set apart from the samples' (about 0.5)
Dan.findChild(QCheckBox, "checkBoxParallaxTr").setChecked(True)
t = table(s)  # process() returns the tables it makes new: those of the script table above closed
cn = list(t.colNames())
for r in range(1, t.numRows() + 1):
    name = "QI-SM-%s-%s" % (t.text(cn.index("#-Run") + 1, r).strip(), t.text(cn.index("Run-info") + 1, r).strip())
    if table(name):
        closeWindow(table(name))


def reduce(name, run, transmission, buffer=None):
    s = p.newScriptTable(name, [run])
    t = table(s)
    cn = list(t.colNames())
    if transmission is not None:
        t.setText(cn.index("Transmission-Sample") + 1, 1, str(transmission))
    if buffer:
        t.setText(cn.index("#-Buffer") + 1, 1, buffer)
        t.setText(cn.index("Buffer-Fraction") + 1, 1, "1")
        t.setText(cn.index("Transmission-Buffer") + 1, 1, str(TB))
    made = [n for n in p.process(s, "I-Q") if "-%s-" % run in n]
    assert made, (name, p.lastMessages())
    c = table(made[0])
    values = [float(c.text(2, r)) for r in range(1, c.numRows() + 1)]
    closeWindow(c)
    return values


alone_sample = reduce("tr-sample", SAMPLE, None)
alone_buffer = reduce("tr-buffer", BUFFER, TB)
Dan.Options.selectMode([m for m in Dan.Options.modes() if m.startswith("(BS)")][0])
subtracted = reduce("tr-subtracted", SAMPLE, None, BUFFER)
assert len(subtracted) == len(alone_sample) == len(alone_buffer)
worst = max(abs(c - (a - b)) / (abs(a) + abs(b)) for a, b, c in zip(alone_sample, alone_buffer, subtracted))
assert worst < 2e-8, worst  # the tables hold 9 digits
