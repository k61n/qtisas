"""Mask tab, "Adv Masking Options" and the rest of the mask fields, from Python as from the interface:
- the tools on any matrix (setValue, rectangle, sector, ellipse, ellipseShell), each pressing its button with the page's
  fields set: a sector from a negative start goes through 0 (-30..30 is 330..360 and 0..30; before: only 330..360), a
  shell as wide as the ellipse is the whole outer ellipse (before: divided by zero), the matrix list selects the named
  matrix (before: it renamed the first entry, the tools then worked on another matrix);
- newMask(deadRows=, deadCols=), checked as triangles= is;
- newMask(edgeShape=): an ellipse edge masks the box's corners;
- addBeamstop() masks the box in the mask named (before: in the matrix of the tools' list);
- saveMask() writes the file loadMask() reads back;
- a mask's label keeps the edge and beam-stop switches and shapes, selecting the mask restores them; an older label
  leaves them as they are.
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtCore import QMetaObject, Q_ARG
    from PyQt6.QtWidgets import QComboBox, QGroupBox, QLineEdit, QSpinBox, QTabWidget
except ImportError:
    from PyQt5.QtCore import QMetaObject, Q_ARG
    from PyQt5.QtWidgets import QComboBox, QGroupBox, QLineEdit, QSpinBox, QTabWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import DIRECT, quiet

N = 61
C = N // 2 + 1
mask = Dan.Mask


def raises(call, text, error=ValueError):
    try:
        call()
    except error as e:
        assert text in str(e), (text, str(e))
        return
    raise AssertionError("no %s: %s" % (error.__name__, text))


def ones(name, n=N):
    m = matrix(name) if name in matrixNames() else newMatrix(name, n, n)
    for r in range(1, n + 1):
        for c in range(1, n + 1):
            m.setCell(r, c, 1)
    return m


def zeros(name):
    m = matrix(name)
    return {(r, c) for r in range(1, m.numRows() + 1) for c in range(1, m.numCols() + 1) if m.cell(r, c) == 0}


def masked(tool, *args, **kwargs):
    """The pixels a tool sets to 0 in a matrix of ones, centered."""
    ones("tool-test")
    assert getattr(mask, tool)("tool-test", (C, C), *args, **kwargs) == "tool-test"
    return zeros("tool-test")


with quiet():
    Dan.Options.newSession()
    Dan.Options.selectInstrument("KWS1-2020")

# the tools' list selects the named matrix, each name once
matrices = Dan.findChild(QComboBox, "comboBoxMlistMask")
ones("tool-a")
ones("tool-b")
Dan.findChild(QTabWidget, "toolBoxMask").setCurrentIndex(1)
for name in ("tool-b", "tool-a"):
    QMetaObject.invokeMethod(Dan, "matrixList", DIRECT, Q_ARG(str, name))
    items = [matrices.itemText(i) for i in range(matrices.count())]
    assert matrices.currentText() == name and items.count("tool-a") == items.count("tool-b") == 1, (name, items)

# the tools
ones("tool-test")
mask.setValue("tool-test", 0)
assert len(zeros("tool-test")) == N * N
assert len(masked("rectangle", 10)) == 11 * 11  # |x - center| <= 5
assert len(masked("rectangle", (10, 4), inside=False)) == N * N - 11 * 5
assert masked("rectangle", 10, value=2) == set()  # set to 2, not 0

across, before, after = masked("sector", (-30, 30)), masked("sector", (330, 360)), masked("sector", (0, 30))
assert across == before | after, (len(across), len(before), len(after))
assert len(before) > 100 and len(after) > 100, (len(before), len(after))
assert masked("sector", (0, 360)) == masked("sector", (-180, 180)), "a full turn is all of it from any start"
east, north = masked("sector", (80, 100)), masked("sector", (-10, 10))  # 0 toward larger y (rows), 90 larger x
assert (C, C + 10) in east and (C + 10, C) not in east and (C + 10, C) in north and (C - 10, C) not in north

ring = masked("ellipseShell", 20, 4)
assert ring and (C, C) not in ring and ring < masked("ellipse", 24), len(ring)
assert masked("ellipseShell", 10, 10) == masked("ellipse", 20), "a shell as wide as the ellipse: all of it"
assert masked("ellipseShell", (10, 6), 14) == masked("ellipse", (24, 20))

raises(lambda: mask.rectangle("no-such-matrix", (1, 1), 3), "no matrix no-such-matrix")
raises(lambda: mask.sector("tool-test", (C, C), (30, -30)), "phi from <= phi to")
raises(lambda: mask.sector("tool-test", (C, C), (0, 400)), "phi to: 0..360")
raises(lambda: mask.ellipse("tool-test", (C, C), 0), "sizes > 0")
raises(lambda: mask.rectangle("tool-test", C, 3), "center: (a, b)", TypeError)
raises(lambda: mask.sector("tool-test", (C, C), 30), "phi: (a, b)", TypeError)

# dead rows and columns: "a-b" all of them, "a-b-c-d" rows a..b in columns c..d (columns: the other way round)
md = 144
plain = zeros(mask.newMask("mask-dead", edge=0, beamstop=0, triangles="", deadRows="", deadCols=""))
assert plain == set(), len(plain)
dead = zeros(mask.newMask("mask-dead", deadRows="1-3;10-11-1-5", deadCols="20-20"))
assert len(dead) == 3 * md + 2 * 5 + md - 3 - 0, len(dead)  # column 20 crosses rows 1..3, counted once
for rows in ("3-1", "1-2-3", "0-3", "1-145", "a-3"):
    raises(lambda: mask.newMask("mask-dead", deadRows=rows), "dead rows \"" + rows + "\"")
raises(lambda: mask.newMask("mask-dead", deadCols="5"), "dead columns \"5\"")
raises(lambda: mask.newMask("mask-dead", deadRows=3), "deadRows: a string", TypeError)
mask.newMask("mask-dead", deadRows="", deadCols="")

# the edge's shape: an ellipse inside the edge box masks its corners too
box = zeros(mask.newMask("mask-edge", edge=(11, 134, 11, 134), beamstop=0, edgeShape="rectangle"))
round_ = zeros(mask.newMask("mask-edge", edgeShape="ellipse"))
assert (12, 12) in round_ and (12, 12) not in box and (72, 72) not in round_ and len(round_) > len(box)
raises(lambda: mask.newMask("mask-edge", edgeShape="circle"), "edge shape: rectangle or ellipse")
mask.newMask("mask-edge", edge=0, edgeShape="rectangle")

# addBeamstop: the mask named, not the matrix the tools' list shows
mask.newMask("mask-bs-add", edge=0, beamstop=0)
assert zeros("mask-bs-add") == set()
ones("tool-test")
mask.setValue("tool-test", 1)  # the tools' list now shows tool-test
assert mask.addBeamstop("mask-bs-add", (10, 19, 20, 29), "rectangle") == "mask-bs-add"
assert zeros("mask-bs-add") == {(r, c) for r in range(20, 30) for c in range(10, 20)}, len(zeros("mask-bs-add"))
assert zeros("tool-test") == set()
mask.addBeamstop("mask-bs-add", (50, 59, 50, 59))  # a second beam stop, the first kept
assert len(zeros("mask-bs-add")) == 200
raises(lambda: mask.addBeamstop("tool-test"), "no mask tool-test")
raises(lambda: mask.addBeamstop("mask-bs-add", (0, 5, 1, 5)), "beamstop: pixels 1..144")

# saveMask: the file loadMask reads back
folder = tempfile.mkdtemp()
try:
    path = mask.saveMask("mask-bs-add", os.path.join(folder, "bs.mask"))
    assert path == os.path.join(folder, "bs.mask") and os.path.isfile(path), path
    assert mask.loadMask(path, "mask-bs-back") == "mask-bs-back"
    assert zeros("mask-bs-back") == zeros("mask-bs-add")
    raises(lambda: mask.saveMask("tool-test", path), "no mask tool-test")
    raises(lambda: mask.saveMask("mask-bs-add", os.path.join(folder, "no", "x.mask")), "no folder")
finally:
    shutil.rmtree(folder)

# a mask's label keeps the edge and beam-stop switches and shapes; selecting the mask brings them back; an older
# label (without them) leaves them as they are
edge, bs = Dan.findChild(QGroupBox, "groupBoxMask"), Dan.findChild(QGroupBox, "groupBoxMaskBS")
edgeShape = Dan.findChild(QComboBox, "comboBoxMaskEdgeShape")
mask.newMask("mask-label", edge=(2, 140, 3, 141), beamstop=0, edgeShape="ellipse", shape="rectangle")
label = matrix("mask-label").windowLabel()
assert label.endswith("|Edge:Yes|BeamStop:No|EdgeShape:ellipse|BeamStopShape:rectangle|Transmission:No|"), label
assert label.startswith("DAN::Mask::144|2|140|3|141|"), label
mask.newMask("mask-label-tr", beamstop=(60, 70, 60, 70), transmission=True)
assert matrix("mask-label-tr").windowLabel().endswith("|BeamStop:Yes|EdgeShape:ellipse|BeamStopShape:rectangle|"
                                                      "Transmission:Yes|"), matrix("mask-label-tr").windowLabel()
bs.setChecked(True)
edgeShape.setCurrentIndex(0)
QMetaObject.invokeMethod(Dan, "readMaskParaFromMaskMatrix", DIRECT, Q_ARG(str, "mask-label"))
assert (edge.isChecked(), bs.isChecked(), edgeShape.currentIndex()) == (True, False, 1)
matrix("mask-label").setWindowLabel("DAN::Mask::144|5|139|6|138|60|80|60|80| 1-2| | |")  # the older format
QMetaObject.invokeMethod(Dan, "readMaskParaFromMaskMatrix", DIRECT, Q_ARG(str, "mask-label"))
assert (edge.isChecked(), bs.isChecked(), edgeShape.currentIndex()) == (True, False, 1)  # as they were
assert Dan.findChild(QSpinBox, "spinBoxLTx").value() == 5
assert Dan.findChild(QLineEdit, "lineEditDeadRows").text() == "1-2"
assert Dan.findChild(QLineEdit, "lineEditDeadCols").text() == ""  # " " of the label read as empty

