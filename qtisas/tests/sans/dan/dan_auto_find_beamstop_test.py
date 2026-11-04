"""Auto mode finds the beam stop's shadow where no direct beam is measured (masks "configuration", no configuration
measuring the transmissions): the beam-stop mask, of the instrument's box size, is centered on the dark box of the
empty beam wherever the instrument's box is (a detector moved sideways); an empty beam without a shadow (the direct
beam), or the auto option findBeamstop off, keeps the instrument's box (skipped without the qtisas.com example files).

The empty beam: KWS3-2020's 1.2 m sample run 00082646 as "EB", in a copy with an 11 x 11 beam stop over its beam.
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtWidgets import QSpinBox
except ImportError:
    from PyQt5.QtWidgets import QSpinBox

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

SPINS = ("spinBoxLTxBS", "spinBoxRBxBS", "spinBoxLTyBS", "spinBoxRByBS")
ELSEWHERE = [20, 30, 200, 210]  # the instrument's box far from the beam: only its size (11 x 11) counts


def bs_box():
    return [Dan.findChild(QSpinBox, n).value() for n in SPINS]


def auto_bs_mask(work, find=True):
    """The beam-stop mask auto mode makes for the folder's only configuration: (x1, x2, y1, y2)."""
    dr.new_session("KWS3-2020", work)
    for n, v in zip(SPINS, ELSEWHERE):
        Dan.findChild(QSpinBox, n).setValue(v)
    assert Dan.Options.Config.autoOptions()["findBeamstop"] is True  # on by default
    Dan.Options.Config.setAutoOptions({"center": "beamstop", "masks": "configuration", "sensitivity": "flat",
                                       "transmission": "", "findBeamstop": find})
    made = Dan.Mask.autoMasks()
    assert made == ["mask", "mask-bs-1.2m"], (made, Dan.Mask.lastError())
    assert bs_box() == ELSEWHERE  # the instrument's box again
    return [int(v) for v in matrix("mask-bs-1.2m").windowLabel().split("|")[5:9]]


src = dr.data_folder("kws3", "*.yaml")
work = tempfile.mkdtemp()
try:
    for ext in ("det", "yaml"):
        shutil.copy(os.path.join(src, "00082646_0000_H-J_HRD_standard." + ext),
                    os.path.join(work, "00082646_0000_EB_HRD_standard." + ext))
    det = os.path.join(work, "00082646_0000_EB_HRD_standard.det")
    with open(det) as f:
        rows = [line.split() for line in f]
    # the direct beam through the sample: no shadow, the instrument's box
    dr.new_session("KWS3-2020", work)
    Dan.Rawdata.setHeaderValue("00082646", "Sample-Title", "EB")
    assert auto_bs_mask(work) == ELSEWHERE

    # an 11 x 11 beam stop over the beam's brightest pixel
    peak = max((float(v), r, c) for r, row in enumerate(rows) for c, v in enumerate(row))
    for r in range(peak[1] - 5, peak[1] + 6):
        for c in range(peak[2] - 5, peak[2] + 6):
            rows[r][c] = "0"
    with open(det, "w") as f:
        f.write("\n".join(" ".join(row) for row in rows) + "\n")
    # centered on the beam stop, of the instrument's box size: all its pixels dark in DAN's image of the run (the axes
    # as the instrument reads the file)
    box = auto_bs_mask(work)
    assert box[1] - box[0] == 10 and box[3] - box[2] == 10, box
    image = matrix(Dan.Rawdata.newInfoMatrix("image-eb", ["00082646"], 1))
    inside = [image.cell(row, col) for row in range(box[2], box[3] + 1) for col in range(box[0], box[1] + 1)]
    assert len(inside) == 121 and max(inside) == 0, (box, max(inside))
    # the auto option findBeamstop off: the instrument's box as it is
    assert auto_bs_mask(work, find=False) == ELSEWHERE
finally:
    shutil.rmtree(work)
