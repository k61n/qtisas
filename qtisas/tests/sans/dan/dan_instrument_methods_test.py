"""Selecting an instrument sets its transmission method, calibrant type and center method as its definition says."""

import os
import sys

try:
    from PyQt6.QtWidgets import QComboBox, QRadioButton
except ImportError:
    from PyQt5.QtWidgets import QComboBox, QRadioButton

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import quiet

transmission = Dan.findChild(QComboBox, "comboBoxTransmMethod")
calibrant = Dan.findChild(QComboBox, "comboBoxACmethod")

# instrument: (transmission method, calibrant type) by their combo-box indices
expected = {
    "KWS3-2020": (1, 1),  # "Direct Beam [dead-time +]" was read as ROI in Header since 1a205940
    "SANS1": (1, 0),
    "KWS1-2020": (4, 0),  # "<9.5A: ROI in Header; >=9.5A: Direct Beam [dead-time +]"
    "KWS1": (0, 0),  # Monitor-3
    "KWS3": (1, 1),  # bare "[Calibrant-Type] Direct Beam [DB]" was read as Flat Scatter since 1a205940
    "KWSX-init": (3, 4),  # ROI in Header; Direct Beam from Monitor3
}
with quiet():
    for name, (tr, ac) in expected.items():
        Dan.Options.selectInstrument(name)
        got = (transmission.currentIndex(), calibrant.currentIndex())
        assert got == (tr, ac), (name, got, transmission.currentText(), calibrant.currentText())

# beam-center method: SANS1 uses X-Y symmetrization (stable for its weakly scattering 20 m data), KWS1-2020 H.F.
with quiet():
    for name, method in (("SANS1", "radioButtonRadStdSymm"), ("KWS1-2020", "radioButtonCenterHF")):
        Dan.Options.selectInstrument(name)
        assert Dan.findChild(QRadioButton, method).isChecked(), (name, method)

# The overall PN mode and its correction mode are separate controls in the GUI and Python API.
with quiet():
    Dan.Options.selectMode("(SM) Standard Mode")
try:
    Dan.Options.selectPolarizationReductionMode("x421")
    raise AssertionError("accepted a PN correction outside PN mode")
except ValueError as error:
    assert "select polarized-neutron mode first" in str(error), str(error)
with quiet():
    Dan.Options.selectMode("(PN) Polarized Neutrons")
    assert Dan.Options.polarizationReductionModes() == ["none", "x2", "x4", "x421", "x422", "x423", "x424"]
    for mode in ("x421", "x422", "x423", "x424", "x4", "none"):
        Dan.Options.selectPolarizationReductionMode(mode)
        assert Dan.Options.polarizationReductionMode() == mode
        assert Dan.findChild(QComboBox, "comboBoxPolReductionMode").currentIndex() == \
               Dan.Options.polarizationReductionModes().index(mode)
