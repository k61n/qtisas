"""Data normalization ("I[x,y] :: Data Normalization") per monitor on a copy of the KWS-3 runs (skipped without the
qtisas.com example files):

- Dan.Processing.Options.setNormalization(by, factor, darkCurrentByTime) / normalization(): the three options; another
  "by" or a factor outside 1..10^6 is a ValueError.

- "Subtract #BC with TIME normalization": the dark current of the buffer is rescaled to the buffer's own duration
  and monitor also in a row without an empty cell (it kept the sample's: buffer - BC subtracted the wrong dark
  current). A buffer with the sample's counts and monitor, measured twice as long, leaves the sample's dark
  current: I[sample - buffer] = I[sample without BC] - I[sample with BC]. Its error bars weigh the BC as subtracted:
  with half the buffer the BC cancels (-BC + BC/2 * 2), I and dI are those without any BC (the error weighed the
  buffer's BC as the sample's: -BC + BC/2, a quarter of the BC's variance and a signal short of BC/2).
- The transmission's error adds the monitors' relative variance 1/M = f/N, not the normalization factor f = N/M:
  it does not depend on the constant N.
- A run whose monitor is 0 cannot be normalized (its image is zero): said in the messages, not only on the console.
"""

import math
import os
import re
import shutil
import sys
import tempfile

try:
    from PyQt6.QtWidgets import QLineEdit
except ImportError:
    from PyQt5.QtWidgets import QLineEdit

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
kws3 = dr.data_folder("kws3", "*.yaml")
SAMPLE, EB = "00082561", "00082558"  # H-L and its empty beam at 9.2 m
SOURCE = SAMPLE + "_0000_H-L_HRD_standard"
LONGER, NO_MONITOR = "00090001", "00090002"


def device_value(text, device, value):
    """The yaml with the first value of a device replaced."""
    pattern = r"(-\s+name: %s\n\s+unit: [^\n]*\n\s+value:\s*\n\s+-\s+)[^\n]+" % re.escape(device)
    changed, n = re.subn(pattern, lambda m: m.group(1) + value, text, count=1)
    assert n == 1, device
    return changed


def write_copy(work, run, **devices):
    with open(os.path.join(kws3, SOURCE + ".yaml")) as f:
        text = f.read()
    for device, value in devices.items():
        text = device_value(text, device, value)
    name = "%s_0000_H-L_HRD_standard" % run
    with open(os.path.join(work, name + ".yaml"), "w") as f:
        f.write(text)
    shutil.copy(os.path.join(kws3, SOURCE + ".det"), os.path.join(work, name + ".det"))


def timer(text):
    return float(re.search(r"-\s+name: timer\n\s+unit: [^\n]*\n\s+value:\s*\n\s+-\s+([^\n]+)", text).group(1))


with open(os.path.join(kws3, SOURCE + ".yaml")) as f:
    duration = timer(f.read())

work = tempfile.mkdtemp()
o = p.Options
try:
    for f in os.listdir(kws3):
        shutil.copy(os.path.join(kws3, f), work)
    write_copy(work, LONGER, timer=repr(2 * duration))  # the same counts and monitors, twice as long
    write_copy(work, NO_MONITOR, mon2="0")

    dr.new_session("KWS3-2020", work)
    p.autoConfig("normalization-base")
    assert o.normalization() == {"by": "time", "factor": 1, "darkCurrentByTime": True}, o.normalization()
    o.setNormalization(by="monitor2")  # the others as they are
    assert o.normalization() == {"by": "monitor2", "factor": 1, "darkCurrentByTime": True}, o.normalization()
    for wrong in ({"by": "monitor3"}, {"factor": 0}, {"factor": 10 ** 7}):
        try:
            o.setNormalization(**wrong)
            raise AssertionError("no ValueError: %s" % wrong)
        except ValueError:
            pass
    assert o.normalization()["by"] == "monitor2"
    for field in ("lineEditDeadTime", "lineEditDeadTimeM2"):  # the dead times would differ with the duration
        Dan.findChild(QLineEdit, field).setText("0")
    p.Options.setWideAngle(parallax="off", transmission=False, window=False)
    Dan.Options.selectMode("(BS) Buffer Subtraction [2D]")

    def reduce(name, bc, buffer="", fraction="1"):
        """I[Q] of the sample alone in a script table without an empty cell."""
        s = p.newScriptTable(name, [SAMPLE])
        t = table(s)
        cn = list(t.colNames())
        t.setText(cn.index("#-EC [EB]") + 1, 1, "")
        t.setText(cn.index("#-BC") + 1, 1, bc)
        t.setText(cn.index("#-Buffer") + 1, 1, buffer)
        t.setText(cn.index("Buffer-Fraction") + 1, 1, fraction if buffer else "0")
        # without the errors common to the row (transmission, factor): the pixels' counts only
        for column in ("Transmission-Sample", "Factor"):
            t.setText(cn.index(column) + 1, 1, t.text(cn.index(column) + 1, 1).split("[")[0].strip())
        # the buffer's absolute scale the sample's
        t.setText(cn.index("Transmission-Buffer") + 1, 1, t.text(cn.index("Transmission-Sample") + 1, 1))
        made = [n for n in p.process(s, "I-Q") if "-%s-" % SAMPLE in n]
        assert made, p.lastMessages()
        c = table(made[0])
        values = [(float(c.text(1, r)), float(c.text(2, r)), float(c.text(3, r))) for r in range(1, c.numRows() + 1)]
        closeWindow(c)
        return values

    bc = table("normalization-base").text(list(table("normalization-base").colNames()).index("#-BC") + 1, 1).strip()
    assert bc, "no dark current"
    with_bc = reduce("norm-with-bc", bc)
    without_bc = reduce("norm-without-bc", "")
    buffered = reduce("norm-buffer", bc, LONGER)
    assert len(buffered) == len(with_bc) == len(without_bc)
    checked = 0
    for (q, b, _), (_, w, _), (_, wo, _) in zip(buffered, with_bc, without_bc):
        dark = wo - w
        if abs(dark) > 1e-5 * abs(wo):  # the tables have 9 digits: the difference is known to 1e-9 of I
            assert math.isclose(b, dark, rel_tol=1e-6, abs_tol=1e-8 * abs(wo)), (q, b, dark, w, wo)
            checked += 1
    assert checked > 20, checked
    # half the buffer: the BC cancels (-BC + BC/2 * 2), the row is the same as without any BC, error bars too
    half = reduce("norm-half-buffer", bc, LONGER, "0.5")
    half_without_bc = reduce("norm-half-buffer-no-bc", "", LONGER, "0.5")
    assert len(half) == len(half_without_bc) > 20
    for (q, h, dh), (_, h0, dh0) in zip(half, half_without_bc):
        assert math.isclose(h, h0, rel_tol=1e-7, abs_tol=1e-9 * abs(h0)), (q, h, h0)
        assert math.isclose(dh, dh0, rel_tol=1e-6), (q, dh, dh0)

    # the transmission's error: the same for any constant N
    Dan.Options.Config.setTransmissionMethod("Direct Beam [dead-time +]")
    Dan.Mask.newMask("mask-norm-tr", beamstop=(110, 130, 130, 150), transmission=True)
    errors = []
    for n in (1, 1000):
        o.setNormalization(factor=n)
        tr, dtr = Dan.Sensitivity.transmission(SAMPLE, EB, "mask-norm-tr")
        errors.append(dtr / tr)
    o.setNormalization(factor=1)
    assert math.isclose(errors[0], errors[1], rel_tol=1e-9), errors

    # a run without monitor counts
    s = p.newScriptTable("norm-no-monitor", [NO_MONITOR])
    p.process(s, "I-Q")
    assert any("run %s: no Monitor2" % NO_MONITOR in m and "cannot be normalized" in m for m in p.lastMessages()), \
        p.lastMessages()
finally:
    o.setNormalization(by="time", factor=1)
    shutil.rmtree(work)
