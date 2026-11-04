"""Dan.Processing.Options.setSmoothing()/smoothing(): the matrix smoothing ("Matrix convolusion") as the pixels of
its kernel, 1 (none), 5, 9, 25; another value is a ValueError. A session (the script table's Settings table) keeps
it: "Options::2D::Smoothing". The run: data/synthetic_0_00024.edf (KWSX-init).
"""

import os
import shutil
import sys
import tempfile

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

o = Dan.Processing.Options
work = tempfile.mkdtemp()
try:
    shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "synthetic_0_00024.edf"), work)
    dr.new_session("KWSX-init", work)
    assert o.smoothing() == 1  # none
    for pixels in (5, 9, 25, 1):
        o.setSmoothing(pixels)
        assert o.smoothing() == pixels
    for wrong in (0, 3, 49):
        try:
            o.setSmoothing(wrong)
        except ValueError as e:
            assert "1 (none), 5, 9 or 25" in str(e), str(e)
        else:
            raise AssertionError("no ValueError for %d" % wrong)

    # the session keeps it
    o.setSmoothing(9)
    assert Dan.Processing.newScriptTable("smoothing", ["00024"]) == "smoothing"
    settings = table("smoothing-Settings")
    keys = [settings.text(1, r) for r in range(1, settings.numRows() + 1)]
    row = keys.index("Options::2D::Smoothing") + 1
    assert settings.text(2, row).strip() == "9 <", settings.text(2, row)
    o.setSmoothing(1)
    Dan.Options.openSession("smoothing")
    assert o.smoothing() == 9
    # an older settings table without the line: none (opening a session selects its instrument, which resets it)
    settings.deleteRows(row, row)
    assert "Options::2D::Smoothing" not in [settings.text(1, r) for r in range(1, settings.numRows() + 1)]
    o.setSmoothing(5)
    Dan.Options.openSession("smoothing")
    assert o.smoothing() == 1, o.smoothing()
finally:
    shutil.rmtree(work)
