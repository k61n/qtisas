"""Smart merging (Dan.Merge.join(smart=True)) of curves without Q beyond their neighbour's ends.

The overlap scans walked past the filled rows: GSL's error handler is off, so a row past them read as 0, and a curve
whose points the join all drops (inside its neighbour's range) looped forever.
"""

import math

Dan.Options.newSession()


def curve(name, q_values, intensity):
    t = newTable(name, len(q_values), 2)
    for row, q in enumerate(q_values, start=1):
        t.setCell(1, row, q)
        t.setCell(2, row, intensity)
    return t


same = [0.01 * i for i in range(1, 11)]  # 0.01 .. 0.10
curve("same-1", same, 1.0)
curve("same-2", same, 2.0)
curve("wide", [0.01 * i for i in range(1, 21)], 1.0)  # 0.01 .. 0.20
# 0.05 .. 0.10, 5000 points: all inside the overlap the join drops, so the scan starts past the filled rows, in a
# large (zeroed) block
curve("inside", [0.05 + 0.00001 * i for i in range(0, 5001)], 3.0)

template = newTable("smart-mergingTemplate", 2, 3)
for col in (1, 2, 3):
    template.setColTextFormat(col)
for row, cells in enumerate((("S-same", "same-1", "same-2"), ("S-inside", "wide", "inside")), start=1):
    for col, text in enumerate(cells, start=1):
        template.setText(col, row, text)

merged = Dan.Merge.join("smart-mergingTemplate", smart=True)
assert merged == ["S-same", "S-inside"], merged
for name in merged:
    t = table(name)
    assert t.numRows() > 0, name
    values = [t.cell(2, row) for row in range(1, t.numRows() + 1)]
    assert all(math.isfinite(v) and v > 0 for v in values), (name, values)
