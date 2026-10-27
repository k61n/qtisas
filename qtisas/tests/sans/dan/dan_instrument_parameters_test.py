"""Dan.Options.Config.parameters(): the instrument's whole configuration as its .SANS file holds it; setParameters()
applies some of them as selecting the instrument does (a value the interface does not take is a ValueError), and
Dan.Options.saveInstrument() keeps them in an instrument of its own — e.g. the I[Q] points always removed. The test
instrument is removed afterwards.
"""

import glob
import os
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import quiet

NAME = "ParametersTest"
folder = os.path.join(os.path.expanduser("~"), ".config", "qtisas", "SANSinstruments")
if sys.platform.startswith("win"):
    folder = os.path.join(os.path.expanduser("~"), "AppData", "Local", "qtisas", "SANSinstruments")
config = Dan.Options.Config


def raises(call, text):
    try:
        call()
    except ValueError as e:
        assert text in str(e), (text, str(e))
        return
    raise AssertionError("no ValueError: " + text)


try:
    with quiet():
        Dan.Options.newSession()
        Dan.Options.selectInstrument("KWS1-2020")
    p = config.parameters()
    assert p["Instrument"] == "KWS1-2020" and "Options-1D-RemoveLast" in p and "AutoMode-EB" in p, sorted(p)[:10]
    assert len(p) > 100, len(p)

    config.setParameters({"Options-1D-RemoveLast": 5, "Options-2D-CenterMethod": "SYM"})
    assert Dan.Processing.Options.removePoints() == (0, 5), Dan.Processing.Options.removePoints()
    assert config.centerMethod() == "symmetry"
    assert config.parameters()["Options-1D-RemoveLast"] == "5"
    raises(lambda: config.setParameters({"No-Such-Key": 1}), "no parameter No-Such-Key")
    raises(lambda: config.setParameters({"Instrument-Mode": "no such mode"}), "not taken: Instrument-Mode")
    raises(lambda: Dan.Options.saveInstrument("KWS1-2020"), "built-in instrument")

    Dan.Options.saveInstrument(NAME)
    assert Dan.Options.instrument() == NAME
    with quiet():
        Dan.Options.selectInstrument("KWS1-2020")
    assert Dan.Processing.Options.removePoints() == (0, 0)
    with quiet():
        Dan.Options.selectInstrument(NAME)
    assert Dan.Processing.Options.removePoints() == (0, 5) and config.centerMethod() == "symmetry"
finally:
    for f in glob.glob(os.path.join(folder, NAME + ".SANS")) + glob.glob(os.path.join(folder, "cpp", NAME + ".cpp")):
        os.remove(f)
    with quiet():
        Dan.Options.selectInstrument("KWS1-2020")
