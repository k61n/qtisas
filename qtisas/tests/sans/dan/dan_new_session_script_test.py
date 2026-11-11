"""A new session leaves the script tables of earlier sessions as they are (skipped without the qtisas.com example
files).

KWS-1 reduced in auto mode (script table auto1), then a new KWS-3 session: its calculate() pressed the configuration
headers, which wrote the KWS-3 values (D, factor, centers of configurations 1 and 2) into auto1, the first script
table of the project, still selected after the list was refilled. Now a new session selects no script table until
it makes or opens one; opening auto1 again reduces it as before.
"""

import os
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

from qtisas_testing import quiet

COLUMNS = ("C", "D", "Lambda", "Factor", "X-center", "Y-center", "Mask", "Sens")


def snapshot(name):
    t = table(name)
    assert t is not None, name
    cols = [dr.column(t, c) for c in COLUMNS]
    return [[t.text(c, r) for c in cols] for r in range(1, t.numRows() + 1)]


kws1 = dr.data_folder("kws1", "*.DAT")
kws3 = dr.data_folder("kws3", "*")
p = Dan.Processing

dr.new_session("KWS1-2020", kws1)
with quiet():
    s1 = p.autoConfig("auto1")
before = snapshot(s1)

dr.new_session("KWS3-2020", kws3)
with quiet():
    s3 = p.autoConfig("auto3")
after = snapshot(s1)
changed = [(r + 1, a, b) for r, (a, b) in enumerate(zip(before, after)) if a != b]
assert not changed, changed
assert snapshot(s3) != before[:len(snapshot(s3))]

# the earlier session again: its own script table, reduced with its own values
with quiet():
    Dan.Options.openSession(s1)
assert Dan.Options.instrument() == "KWS1-2020", Dan.Options.instrument()
with quiet():
    qi = p.process(s1)
assert len([n for n in qi if n.startswith("QI-SM-")]) == len(before), qi
assert snapshot(s1) == before
