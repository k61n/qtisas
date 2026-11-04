"""Auto mode's mask modes (skipped without the qtisas.com example files):

- configuration: a beam-stop mask is centered on its direct beam, found without the instrument's beam stop (through
  "mask" the beam under the instrument's box was hidden and the center drifted), in 1-based pixels (it was one pixel
  off), the box rounded around the center; its size scaled by the beam size / the reference beam size, the masks
  without a direct beam too; a transmission mask only where the transmissions are calculated with the direct beam;
- none: with transmissions from the direct beam (SANS-1's Direct Beam method) a "mask-tr" of the instrument's box;
  "mask" hid the beam and autoConfig() failed ("no transmission could be calculated");
- wavelength: a mask shared by configurations of another beam-stop position is moved by the one with the beam stop
  in, not by the first (a transmission configuration's detector position moved it);
- files: a configuration takes the mask file named as its mask ("mask-bs-9.2m.mask"); without one "mask", told;
- the instrument's beam-stop box is the same after autoMasks().
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtWidgets import QComboBox, QLineEdit, QSpinBox
except ImportError:
    from PyQt5.QtWidgets import QComboBox, QLineEdit, QSpinBox

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

SPINS = ("spinBoxLTxBS", "spinBoxRBxBS", "spinBoxLTyBS", "spinBoxRByBS")
p = Dan.Processing


def bs_box():
    return [Dan.findChild(QSpinBox, n).value() for n in SPINS]


def set_bs_box(box):
    for n, v in zip(SPINS, box):
        Dan.findChild(QSpinBox, n).setValue(v)


def mask_box(name):
    return [int(v) for v in matrix(name).windowLabel().split("|")[5:9]]


kws3 = dr.data_folder("kws3", "*.yaml")

# --- configuration: the instrument's box over the 9.2 m direct beam (KWS3-2020 has it beside the beam)
dr.new_session("KWS3-2020", kws3)
on_beam = [115, 125, 135, 145]
set_bs_box(on_beam)
Dan.Options.Config.setAutoOptions({"center": "maxflux", "masks": "configuration", "sensitivity": "flat"})
made = Dan.Mask.autoMasks()
assert "mask-bs-9.2m" in made and "mask-auto-edge" not in made, made
assert bs_box() == on_beam
assert all(not n.startswith("mask-auto") for n in Dan.Mask.masks()), Dan.Mask.masks()  # the edge-only mask removed
Dan.Mask.newMask("edge-only", beamstop=0)
x, y = p.runCenter("00082558", "edge-only", "sens")
box = mask_box("mask-bs-9.2m")
assert box[1] - box[0] == 10 and box[3] - box[2] == 10, box  # the instrument's size
assert abs((box[0] + box[1]) / 2.0 - x) <= 0.5 and abs((box[2] + box[3]) / 2.0 - y) <= 0.5, (box, x, y)

# --- none: SANS-1 (transmissions with the direct beam, the instrument's beam stop on the beam) end to end
dr.new_session("SANS1", dr.data_folder("sans1", "D*.001"))
options = Dan.Options.Config.autoOptions()
assert options["masks"] == "none" and Dan.Options.Config.transmissionMethod().startswith("Direct Beam"), options
assert Dan.Mask.autoMasks() == ["mask", "mask-tr"]
s = p.autoConfig("auto-sans1")
assert s == "auto-sans1", p.lastError()
assert {p.configuration(i)["transmissionMask"] for i in range(1, p.configurations() + 1)} == {"mask-tr"}
# KWS1-2020 (the optional method, below 9.5 A: from the header): no mask-tr
kws1 = dr.data_folder("kws1", "*.DAT")
dr.new_session("KWS1-2020", kws1)
assert Dan.Mask.autoMasks() == ["mask"]

# --- configuration, the beam size 50x50 against the reference beam size 30: every beam-stop mask 5/3 of the
# instrument's 11 x 11, with a direct beam (7.68 m) or without (19.68 m, 1.98 m: the instrument's box, the beam seen
# through KWS1's beam stop)
dr.new_session("KWS1-2020", kws1)
Dan.Options.Config.setAutoOptions({"masks": "configuration", "beamSize": 30})
made = Dan.Mask.autoMasks()
# no transmission mask: KWS1-2020's transmissions below 9.5 A are read from the header (the optional method)
assert made == ["mask", "mask-bs-19.68m", "mask-bs-7.68m", "mask-bs-1.98m"], made
for name in ("mask-bs-19.68m", "mask-bs-7.68m", "mask-bs-1.98m"):
    box = mask_box(name)
    assert box[1] - box[0] == 17 and box[3] - box[2] == 17, (name, box)
assert (sum(mask_box("mask-bs-19.68m")[:2]) / 2.0, sum(mask_box("mask-bs-19.68m")[2:]) / 2.0) == (73.5, 85.5)

# --- wavelength: KWS3's 9.2 m twice, the empty beam 00082558 (the direct beam: it measures the transmissions, listed
# first) and a copy of a sample as "EB" at another beam center (the beam stop in); the shared beam-stop mask is moved by
# the copy's detector position
work = tempfile.mkdtemp()
try:
    for f in os.listdir(kws3):
        shutil.copy(os.path.join(kws3, f), work)
    for ext in ("det", "yaml"):
        shutil.copy(os.path.join(kws3, "00082561_0000_H-L_HRD_standard." + ext),
                    os.path.join(work, "00082599_0000_EB_HRD_standard." + ext))
    dr.new_session("KWS3-2020", work)
    Dan.Rawdata.setHeaderValue("00082599", "Sample-Title", "EB")
    Dan.Rawdata.setHeaderValue("00082599", "Detector-X || Beamcenter-X", 130)
    reference = Dan.Rawdata.info(["00082558"])[0]["detectorX"]
    Dan.Options.Config.setAutoOptions({"masks": "wavelength", "detectorX": int(round(reference)), "detectorY": 0})
    chosen = [(c["emptyBeam"], c["transmission"]) for c in p.autoConfigurations()]
    assert chosen[:2] == [("00082558", True), ("00082599", False)], chosen
    instrument = bs_box()
    made = Dan.Mask.autoMasks()
    shared = "mask-bs-9.2m-12.8A"  # two empty beams at 9.2 m: the wavelength in the name
    assert made.count(shared) == 1, made
    pixel = float(Dan.findChild(QComboBox, "comboBoxBinning").currentText()) * \
        float(Dan.findChild(QLineEdit, "lineEditResoPixelSize").text())
    dx = round((130 - int(round(reference))) / pixel / 10.0)
    assert dx != 0
    assert mask_box(shared) == [instrument[0] + dx, instrument[1] + dx, instrument[2], instrument[3]], \
        (mask_box(shared), instrument, dx)
finally:
    shutil.rmtree(work)

# --- files: a configuration takes the mask file named as auto mode names its mask; without one "mask", and told
work = tempfile.mkdtemp()
try:
    for f in os.listdir(kws3):
        shutil.copy(os.path.join(kws3, f), work)
    dr.new_session("KWS3-2020", work)
    Dan.Mask.newMask("mask-bs-9.2m", beamstop=[110, 130, 130, 150])
    Dan.Mask.saveMask("mask-bs-9.2m")  # <input folder>/mask-bs-9.2m.mask
    Dan.Mask.newMask("mask", beamstop=[123, 133, 123, 133])
    Dan.Mask.saveMask("mask")
    dr.new_session("KWS3-2020", work)
    Dan.Options.Config.setAutoOptions({"masks": "files"})
    made = Dan.Mask.autoMasks()
    assert sorted(made) == ["mask", "mask-bs-9.2m"], (made, Dan.Mask.lastError())
    Dan.Sensitivity.autoSensitivity()
    p.autoConfigurations()
    # (a mask-bs-1.2m of the sections above is in the project: not this folder's, no file of it)
    chosen = [p.configuration(i)["mask"] for i in (1, 2)]
    assert chosen == ["mask-bs-9.2m", "mask"], chosen
    notes = [m for m in p.lastMessages() if "no mask file" in m]
    assert len(notes) == 1 and "configuration 2" in notes[0] and "mask-bs-1.2m.mask" in notes[0], p.lastMessages()
finally:
    shutil.rmtree(work)
