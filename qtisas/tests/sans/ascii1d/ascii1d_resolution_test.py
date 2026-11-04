"""ASCII.1D resolution sigma (Ascii1d.resolutionSigma, the column "Sigma"; also Fittable's "ASCII.1D.SANS"): the
square root of the sum of the variances of the contributions, as DAN's pinhole sigma (Mildner & Carpenter 1984,
NIST), with the angle factors of Pedersen et al. (1990):

- the wavelength: the selector's triangle of FWHM dlambda, Q^2 dlambda^2 / 6 (it was a Gaussian of that FWHM);
- the apertures: uniformly lit, r^2 / 4 each with r^2 = (x^2 + y^2) / 6 for a rectangle x by y, the mean of its
  x^2 / 12 and y^2 / 12 (it was 0.4247 times the FWHM of their overlap, 10-25 % more, of circles of the same area);
- the detector: the pixel as the detector's resolution (its real pixel) and as the averaging ring, a box each,
  w^2 / 12 (both were w / sqrt(pi) as a FWHM).
- "Focused?" (it was "Lenses?") as DAN's focusing: source and detector at 2f = C, the sample at D (the lenses had a
  focus of their own and the source over C); the source's image r1^2 / 4 / D^2, the same detector and wavelength
  terms; the optic beside it, "lenses" (the chromatic blur 16 (dl^2 / 6) (r2 C / D)^2 / 4 / D^2 and gravity
  (4 A lambda^2 C^2)^2 (dl^2 / 6) / 2 / D^2) or "mirror" (neither), shown
  only when focused, as is "Colimation = 2f:"; Ascii1d.setResolutionFocusing(on, optic), a wrong optic a ValueError.
- A format file keeps every resolution option: focusing and its optic, the units of Q and of the wavelength (they
  were not saved) and the geometry; an older format's "{Reso-Lens-Yes-No} Yes" reads as focused lenses.
"""

import math
import os
import sys

try:
    from PyQt6.QtWidgets import QApplication, QCheckBox, QComboBox, QLabel, QLineEdit, QToolButton
except ImportError:
    from PyQt5.QtWidgets import QApplication, QCheckBox, QComboBox, QLabel, QLineEdit, QToolButton

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import accept_text, answering, locale_c, set_app_locale

set_app_locale(locale_c())
a = Ascii1d
widget = [w for w in QApplication.allWidgets() if w.metaObject().className() == "ascii1d18"][0]
child = lambda cls, name: widget.findChild(cls, name)
focused, optic = child(QCheckBox, "checkBoxFocused"), child(QComboBox, "comboBoxFocusType")
collimation = child(QLabel, "textLabelResoCol")


def expected(q, C, D, pixel, sa, ca, lam, dl):
    """The pinhole sigma: lengths in cm, apertures (w, h) in mm, lambda in A."""
    r1, r2 = 0.1 * math.sqrt((ca[0] ** 2 + ca[1] ** 2) / 6), 0.1 * math.sqrt((sa[0] ** 2 + sa[1] ** 2) / 6)
    k = 2 * math.pi / lam
    theta = math.asin(q / 2 / k)
    c = math.cos(2 * theta) ** 2
    detector = (k * math.cos(theta) * c / D) ** 2 * (2 / 12) * pixel ** 2
    apertures = (k * math.cos(theta)) ** 2 * ((r1 / C) ** 2 + (r2 * (1 / C + c / D)) ** 2) / 4
    return math.sqrt(q * q * dl * dl / 6 + apertures + detector)


try:
    a.setResolutionFocusing(False)
    for geometry in ((800, 400, 0.525, (10, 10), (30, 30), 6.0, 0.1), (2000, 2000, 0.8, (7, 10), (50, 50), 5.0, 0.2),
                     (400, 100, 0.836, (10, 10), (30, 30), 4.5, 0.1)):
        C, D, pixel, sa, ca, lam, dl = geometry
        a.setResolutionGeometry(C, D, pixel, sa[0], sa[1], ca[0], ca[1], lam, dl)
        for q in (0.001, 0.01, 0.05, 0.2, 0.5):
            s, e = a.resolutionSigma(q), expected(q, *geometry)
            assert abs(s / e - 1) < 1e-9, (geometry, q, s, e)

    # the option: the optic and "= 2f" only when focused
    assert not focused.isChecked() and optic.isHidden() and collimation.text() == "Colimation:", collimation.text()
    a.setResolutionFocusing(True, "mirror")
    assert focused.isChecked() and not optic.isHidden() and optic.currentText() == "mirror"
    assert collimation.text() == "Colimation = 2f:", collimation.text()
    try:
        a.setResolutionFocusing(True, "prism")
        raise AssertionError("no ValueError")
    except ValueError as e:
        assert "lenses" in str(e) and "mirror" in str(e), str(e)

    # focusing, as DAN's (2f = C, the sample at D): the source's image r1^2 / 4 / D^2, the detector as without it,
    # the wavelength Q^2 dl^2 / 6; lenses add the chromatic blur 16 (dl^2 / 6) (r2 C / D)^2 / 4 / D^2 and gravity
    for name in ("lenses", "mirror"):
        a.setResolutionFocusing(True, name)
        for C, D, pixel, sa, ca, lam, dl in ((2000, 2000, 0.5, (10, 10), (30, 20), 6.0, 0.1),
                                             (2000, 800, 1.0, (8, 12), (20, 20), 8.0, 0.15)):
            a.setResolutionGeometry(C, D, pixel, sa[0], sa[1], ca[0], ca[1], lam, dl)
            r1sq, r2sq = 0.01 * (ca[0] ** 2 + ca[1] ** 2) / 6, 0.01 * (sa[0] ** 2 + sa[1] ** 2) / 6
            k = 2 * math.pi / lam
            chromatic = 16 * dl * dl / 6 * r2sq * C * C / D / D / 4 / D / D if name == "lenses" else 0.0
            fall = 3.13311e-9 * lam * lam  # A lambda^2 [1/cm]
            chromatic += (4 * fall * C * C) ** 2 * dl * dl / 6 / 2 / D / D if name == "lenses" else 0.0
            for q in (0.001, 0.01, 0.05):
                e = math.sqrt(k * k * (r1sq / 4 / D / D + chromatic + 2 / 12 * pixel ** 2 / D / D)
                              + q * q * dl * dl / 6)
                s = a.resolutionSigma(q)
                assert abs(s / e - 1) < 1e-9, (name, C, D, q, s, e)

    # a format file keeps every resolution option
    fields = {"lineEditResoCol": "1234", "lineEditResoDet": "567", "lineEditResoPixelSize": "0.75",
              "lineEditResoSamAper": "7", "lineEditResoSamAper2": "8", "lineEditResoColAper": "20",
              "lineEditResoColAper2": "30", "lineEditResoLambda": "6.5", "lineEditResoDeltaLambda": "0.12"}
    for field, value in fields.items():
        child(QLineEdit, field).setText(value)
    q_units, lambda_units = child(QComboBox, "comboBoxQunits"), child(QComboBox, "comboBoxLambdaUnits")
    q_units.setCurrentIndex(1)
    lambda_units.setCurrentIndex(1)
    a.setResolution(True)
    a.setResolutionFocusing(True, "mirror")
    name = "QtisasTestAscii1dResolution"
    folder = os.path.join(os.path.expanduser("~"), ".config", "qtisas", "ascii1dFormats")
    path = os.path.join(folder, name + ".ASCII1D")
    try:
        with answering(accept_text(name)):
            child(QToolButton, "pushButtonNewASCII1D").click()
        text = open(path, encoding="utf-8").read()
        for line in ("{Reso-Focused-Yes-No} Yes", "{Reso-Focused-Optic} mirror", "{Reso-Q-Units} nm",
                     "{Reso-Lambda-Units} nm", "{Reso-Colimation} 1234", "{Reso-Delta-Lambda} 0.12"):
            assert line in text, (line, text)
        for field in fields:
            child(QLineEdit, field).setText("1")
        q_units.setCurrentIndex(0)
        lambda_units.setCurrentIndex(0)
        a.setResolutionFocusing(False, "lenses")
        formats = child(QComboBox, "comboBoxStructureASCII1D")
        formats.setCurrentIndex(formats.findText(name))
        formats.activated.emit(formats.currentIndex())  # as when chosen: reads the file
        read = {field: child(QLineEdit, field).text() for field in fields}
        assert read == fields, read
        assert (q_units.currentIndex(), lambda_units.currentIndex()) == (1, 1)
        assert focused.isChecked() and optic.currentText() == "mirror" and not optic.isHidden()
        assert collimation.text() == "Colimation = 2f:", collimation.text()
        # an older format: "{Reso-Lens-Yes-No} Yes" is focused lenses
        with open(path, "w", encoding="utf-8") as f:
            f.write(text.replace("{Reso-Focused-Yes-No} Yes\n", "{Reso-Lens-Yes-No} Yes\n")
                    .replace("{Reso-Focused-Optic} mirror\n", ""))
        a.setResolutionFocusing(False, "mirror")
        formats.activated.emit(formats.currentIndex())
        assert focused.isChecked() and optic.currentText() == "lenses"
    finally:
        if os.path.exists(path):
            os.remove(path)
        q_units.setCurrentIndex(0)
        lambda_units.setCurrentIndex(0)
finally:
    a.setResolutionFocusing(False)
    a.reset()
