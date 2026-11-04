"""(BS-SENS) Buffer Subtraction | Buffer as Sensitivity: a row with "Use-Buffer-as-Sensitivity" no gets the mask
combined with the sensitivity (masked where sens is 0), a row with "yes" not; two such rows with the same mask and
sens, in either order, give each the I[Q] it has alone (the second row got the first row's mask and sens: the
combined ones, or not combined; skipped without the qtisas.com example files).
"""

import math
import os
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
NO, YES, BUFFER = "00082559", "00082561", "00082557"  # H-J, H-L, H-M at 9.2 m

dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
p.autoConfig("buffer-base")
# a sensitivity with a band of zeros where the samples scatter: it masks the band for the "no" rows only
sens = matrix("sens")
for row in range(130, 141):
    for col in range(1, sens.numCols() + 1):
        sens.setCell(row, col, 0.0)
Dan.Options.selectMode([m for m in Dan.Options.modes() if m.startswith("(BS-SENS)")][0])


def reduce(name, rows):
    """I[Q] of each row of a new script table: {run: [(Q, I), ...]}."""
    s = p.newScriptTable(name, [run for run, _ in rows])
    t = table(s)
    cn = list(t.colNames())
    for r, (run, sensFromBuffer) in enumerate(rows, start=1):
        assert t.text(cn.index("#-Run") + 1, r).strip() == run
        t.setText(cn.index("#-Buffer") + 1, r, BUFFER)
        t.setText(cn.index("Buffer-Fraction") + 1, r, "1")
        t.setText(cn.index("Use-Buffer-as-Sensitivity") + 1, r, sensFromBuffer)
    curves = {}
    for made in p.process(s, "I-Q"):
        run = next((run for run, _ in rows if "-%s-" % run in made), None)
        if run:
            c = table(made)
            curves[run] = [(float(c.text(1, i)), float(c.text(2, i))) for i in range(1, c.numRows() + 1)]
            closeWindow(c)
    assert sorted(curves) == sorted(run for run, _ in rows), (curves.keys(), rows)
    return curves


def same(a, b):
    return len(a) == len(b) and all(math.isclose(x[1], y[1], rel_tol=1e-9, abs_tol=1e-12) for x, y in zip(a, b))


alone_no = reduce("buffer-no", [(NO, "no")])[NO]
alone_yes = reduce("buffer-yes", [(YES, "yes")])[YES]
assert not same(alone_no, reduce("buffer-no-as-yes", [(NO, "yes")])[NO])  # the combination matters here
for order in ([(NO, "no"), (YES, "yes")], [(YES, "yes"), (NO, "no")]):
    pair = reduce("buffer-pair-" + order[0][1], order)
    assert same(pair[NO], alone_no), order
    assert same(pair[YES], alone_yes), order
