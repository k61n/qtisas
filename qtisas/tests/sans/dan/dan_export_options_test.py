"""The output options of DAN (KWS-3 example runs; skipped without the qtisas.com example files):

- Dan.Processing.Options.setMatrixFormat(format, header, sasview, ignoreMask) / matrixFormat(): "I[x,y] :: Output
  Format of ASCII-files"; the five formats into files: their header line and columns; SasView's resolution columns:
  sigma(perp) is sigma(para) without the wavelength term (with focusing it was that term); Qx, Qy the components of
  the pixel's Q (Qx^2 + Qy^2 = Q^2 of the polar file); a 9th column, SasView's mask (1: masked; it read none: with
  "ignore mask" the masked pixels counted). "ignore mask" off in every instrument and kept by a session.
- The "###" header of the I[Q] files names the columns of the presentation (Guinier: Q^2, ln(I), ...); it was written
  for QI only.
- setCurveFormat(columns, header, anisotropy, anisotropyOffset) / curveFormat(): "I[Q] :: Output Format"; the
  columns of the I[Q] table.
- setPresentation(name) / presentation(): "I[Q] :: SAS Presentation", the name's case ignored; the label of the page
  shows its axes also when set by a script (it followed the mouse only). The dQ column follows the Q column's
  presentation, sigma stays the resolution in Q — in MS mode too (it wrote dQ as in QI).
- Wrong names and values are ValueErrors.
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtWidgets import QLabel
except ImportError:
    from PyQt5.QtWidgets import QLabel

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
o = p.Options
RUN = "00082646"  # H-J at 1.2 m

dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
p.autoConfig("export-base")


tables_made = []


def wrong(call, *args, **kwargs):
    try:
        call(*args, **kwargs)
    except ValueError:
        return
    raise AssertionError("no ValueError: %s %s" % (args, kwargs))


# --- the options
assert o.matrixFormat() == {"format": "matrix", "header": False, "sasview": False, "ignoreMask": False}
assert o.curveFormat() == {"columns": "sigma", "header": True, "anisotropy": False, "anisotropyOffset": 0}
assert o.presentation() == "QI"
o.setMatrixFormat("qxqy", header=True)
assert o.matrixFormat() == {"format": "qxqy", "header": True, "sasview": False, "ignoreMask": False}
o.setCurveFormat(anisotropy=True, anisotropyOffset=45)  # the others as they are
assert o.curveFormat() == {"columns": "sigma", "header": True, "anisotropy": True, "anisotropyOffset": 45}
wrong(o.setMatrixFormat, format="table")
wrong(o.setCurveFormat, columns="sigma-dq")
wrong(o.setCurveFormat, anisotropyOffset=400)
wrong(o.setPresentation, "Holtzer")
label = Dan.findChild(QLabel, "textLabelPres")
o.setPresentation("guinier")
assert o.presentation() == "Guinier" and label.text() == "ln[I] vs. Q<sup>2</sup>", label.text()
o.setPresentation("QI")
assert label.text() == "I vs. Q", label.text()

# --- I[Q] columns
expected = {"sigma": ["Q", "I", "dI", "Sigma"], "dq": ["Q", "I", "dI", "dQ"], "none": ["Q", "I", "dI"],
            "dq-sigma": ["Q", "I", "dI", "dQ", "Sigma"]}
for k, (columns, names) in enumerate(expected.items()):
    o.setCurveFormat(columns, anisotropy=(k == 0))
    s = p.newScriptTable("export-curve-%d" % k, [RUN])
    made = [n for n in p.process(s, "I-Q", "project") if n.startswith("QI")]
    assert made, p.lastMessages()
    t = table(made[0])
    assert list(t.colNames()) == names + (["Anisotropy"] if k == 0 else []), (columns, t.colNames())
    closeWindow(t)
o.setCurveFormat("sigma", anisotropy=False, anisotropyOffset=0)

# --- a presentation: dQ follows the Q column (Guinier: x = Q^2, dx = 2 Q dQ), sigma stays the resolution in Q; in MS
# mode too (it wrote dQ as in QI)
o.setCurveFormat("dq-sigma")
for mode in ("(SM) Standard Mode", "(MS) Magnetic Samples"):
    Dan.Options.selectMode(mode)
    curves = {}
    for presentation in ("QI", "Guinier"):
        o.setPresentation(presentation)
        s = p.newScriptTable("export-presentation-%d" % len(tables_made), [RUN])
        tables_made.append(s)
        for name in [n for n in p.process(s, "I-Q", "project") if n.startswith("QI")]:
            t = table(name)
            curves[(presentation, "MC" in name)] = [[float(t.text(c, r)) for c in range(1, 6)]
                                                     for r in range(1, t.numRows() + 1)]
            closeWindow(t)
    for kind in {k[1] for k in curves}:
        qi, guinier = curves[("QI", kind)], curves[("Guinier", kind)]
        assert len(qi) == len(guinier) > 10, (mode, len(qi), len(guinier))
        for (q, _, _, dq, sigma), (x, _, _, dx, sigma_g) in zip(qi, guinier):
            assert abs(x / (q * q) - 1) < 1e-6 and abs(dx / (2 * q * dq) - 1) < 1e-5, (mode, q, dq, x, dx)
            assert sigma_g == sigma, (mode, q, sigma, sigma_g)
Dan.Options.selectMode("(SM) Standard Mode")
o.setPresentation("QI")
o.setCurveFormat("sigma")

# --- "ignore mask": off in every instrument, kept by a session (it was in neither)
assert Dan.Options.Config.parameters()["Options-2D-Output-IgnoreMask"] == "No"
o.setMatrixFormat(ignoreMask=True)
s = p.newScriptTable("export-ignore-mask", [RUN])
o.setMatrixFormat(ignoreMask=False)
Dan.Options.openSession(s)
assert o.matrixFormat()["ignoreMask"] is True, o.matrixFormat()
o.setMatrixFormat(ignoreMask=False)

# --- I[x,y] into files
work = tempfile.mkdtemp()
try:
    Dan.Options.setOutputPath(work)
    headers = {"xy": "x\t\ty\t\tI[x,y]\t\tdI[x,y]", "qxqy": "Qx\t\tQy\t\tI[x,y]\t\tdI[x,y]",
               "qxqy-resolution": "Qx\t\tQy\t\tI[x,y]\t\tdI[x,y]\t\tQz\tσQ(para)\tσQ(perp)\tshadow\tmasked",
               "polar": "Q\t\tphi\t\tI[x,y]\t\tdI[x,y]"}
    columns = {"xy": 4, "qxqy": 4, "qxqy-resolution": 9, "polar": 4}
    written = {}
    for k, fmt in enumerate(["matrix", "xy", "qxqy", "qxqy-resolution", "polar"]):
        o.setMatrixFormat(fmt, header=True, sasview=(fmt == "qxqy-resolution"))
        s = p.newScriptTable("export-matrix-%d" % k, [RUN])
        made = p.process(s, "I-x-y", "files")
        assert len(made) == 1 and os.path.isfile(made[0]), made
        with open(made[0]) as f:
            lines = f.read().splitlines()
        os.remove(made[0])
        if fmt == "matrix":  # the image, no header
            assert len(lines) == len(lines[0].split()) == 256, (len(lines), len(lines[0].split()))
            continue
        assert lines[0] == headers[fmt], (fmt, lines[0])
        data = lines[3:] if fmt == "qxqy-resolution" else lines[1:]
        written[fmt] = [[float(v) for v in line.split()] for line in data]
        if fmt == "qxqy-resolution":
            # SasView's reader takes the 4th column as err(I) only after a "Data columns ... err(I)" line (else as Qz)
            assert lines[1] == ("Data columns are Qx - Qy - I(Qx,Qy) - err(I) - Qz - SigmaQ_parall - SigmaQ_perp - "
                                "fSubS(beam stop shadow) - mask"), lines[1]
            assert lines[2] == "ASCII data", lines[2]
        assert all(len(line.split()) == columns[fmt] for line in data[:100]), (fmt, data[0])
        if fmt == "qxqy-resolution":
            # sigma(perp): sigma(para) without the wavelength term, which grows with Q along the radius only (KWS-3
            # focusing: sigma(perp) was that term itself, 1.1e-3 > sigma(para) 7.1e-4): para^2 - perp^2 ~ Q^2
            ratios = []
            for line in data:
                qx, qy, _, _, _, para, perp, shadow, masked = (float(v) for v in line.split())
                assert masked == 1 - shadow, line  # SasView's mask: 1 = masked (it read none)
                if shadow > 0:
                    assert perp <= para, (line, para, perp)
                    ratios.append((para * para - perp * perp) / (qx * qx + qy * qy))
            assert max(ratios) / min(ratios) < 1.01, (min(ratios), max(ratios))
        if fmt == "qxqy-resolution":  # with "ignore mask" the masked pixels too, SasView's mask 1 (left out)
            o.setMatrixFormat(fmt, header=True, sasview=True, ignoreMask=True)
            s = p.newScriptTable("export-matrix-ignore", [RUN])
            made = p.process(s, "I-x-y", "files")
            o.setMatrixFormat(ignoreMask=False)
            with open(made[0]) as f:
                every = [line.split() for line in f.read().splitlines()[3:]]
            os.remove(made[0])
            masked = [line for line in every if line[8] == "1"]
            assert len(every) == 256 * 256 and len(masked) == len(every) - len(data), (len(every), len(masked))
            assert all(line[7] == "0" for line in masked), masked[0]
        if fmt == "xy":  # 1-based pixels
            assert all(float(v).is_integer() and float(v) >= 1 for line in data[:100] for v in line.split()[:2])
    # Qx, Qy: the components of the pixel's Q (that of the polar file, the same pixels in the same order), Qx growing
    # with the column, Qy with the row (they were 4 pi / lambda sin(asin(x / D) / 2) per axis: 0.07 % off here)
    xy, qxqy, polar = written["xy"], written["qxqy"], written["polar"]
    assert len(xy) == len(qxqy) == len(polar) > 1000
    for (qx, qy, *_), (q, *_) in zip(qxqy, polar):
        assert abs((qx * qx + qy * qy) ** 0.5 - q) <= 1e-7 * q, (qx, qy, q)
    for a, b, pa, pb in zip(qxqy, qxqy[1:], xy, xy[1:]):
        if pa[1] == pb[1] and pb[0] > pa[0]:
            assert b[0] > a[0], (pa, pb, a, b)
    # I[Q] files: the header of every presentation (it was written for QI only), the QI one as before
    headers = {"QI": "###     Q[1/A]          I[1/cm]         dI[1/cm]          dQ[1/A]       Sigma[1/A]",
               "Guinier": "### Q^2[1/A^2]            ln(I)          d(ln I)    d(Q^2)[1/A^2]       Sigma[1/A]",
               "Kratky": "###     Q[1/A]             IQ^2          d(IQ^2)          dQ[1/A]       Sigma[1/A]"}
    o.setCurveFormat("dq-sigma", header=True)
    for k, (presentation, header) in enumerate(headers.items()):
        o.setPresentation(presentation)
        s = p.newScriptTable("export-header-%d" % k, [RUN])
        made = [f for f in p.process(s, "I-Q", "files") if os.path.basename(f).startswith("QI")]
        assert made, p.lastMessages()
        with open(made[0]) as f:
            first = f.readline().rstrip("\n")
        os.remove(made[0])
        assert first == header, (presentation, first)
    o.setPresentation("QI")
    o.setCurveFormat("sigma")
finally:
    o.setMatrixFormat("matrix", header=False, sasview=False)
    shutil.rmtree(work)
