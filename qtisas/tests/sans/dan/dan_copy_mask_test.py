"""Dan.Mask.copyMask(): a copy of a mask is a mask too and sits with the masks, in "DAN :: mask, sens" (skipped
without the qtisas.com example files).

Confirmed live (KWS-3 in auto mode, horizontal sectors): masks copied with clone() landed in the active folder, with
the script tables, not with "mask" and the auto masks.
"""

import os
import sys

import qti

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

from qtisas_testing import quiet


def folder_of(name):
    w = next(w for w in qti.app.windows() if w.objectName() == name)
    return w.folder().path()


def refused(call):
    try:
        call()
    except ValueError as e:
        return str(e)
    return ""


dr.new_session("KWS3-2020", dr.data_folder("kws3", "*"))
with quiet():
    s = Dan.Processing.autoConfig("auto3")
source = "mask-bs-9.2m"
assert source in Dan.Mask.masks(), Dan.Mask.masks()

assert Dan.Mask.copyMask(source, "auto3-hor-1") == "auto3-hor-1"
assert folder_of("auto3-hor-1") == folder_of(source), (folder_of("auto3-hor-1"), folder_of(source))
assert "auto3-hor-1" in Dan.Mask.masks(), Dan.Mask.masks()  # a mask, usable in a script table's Mask column
m, copy = matrix(source), matrix("auto3-hor-1")
assert all(m.cell(i, j) == copy.cell(i, j)
           for i in range(1, m.numRows() + 1, 17) for j in range(1, m.numCols() + 1, 17))
Dan.Mask.sector("auto3-hor-1", (128, 128), (-75, 75))  # the copy changes, the source not
assert sum(copy.cell(i, j) for i in range(1, 257) for j in range(1, 257)) < sum(
    m.cell(i, j) for i in range(1, 257) for j in range(1, 257))

# a name in use, an invalid name, no mask: refused, nothing left behind
windows = len(qti.app.windows())
assert "exists already" in refused(lambda: Dan.Mask.copyMask(source, "auto3-hor-1"))
assert "invalid name" in refused(lambda: Dan.Mask.copyMask(source, "mask-bs-9.2m-hor"))
assert "no mask" in refused(lambda: Dan.Mask.copyMask("no-such-mask", "auto3-x"))
assert len(qti.app.windows()) == windows, len(qti.app.windows())
