"""Radial averaging I[Q] ("I[Q] :: Radial Averaging :: Method"), KWS-3 example runs (skipped without the qtisas.com
example files):

- Dan.Processing.Options.setRadialAveraging(method, linear, progressive) / radialAveraging(); a wrong method or a
  factor outside its range is a ValueError.
- Merged points (linear and progressive, both methods): Q is the mean of their pixels, inside the Q range of the
  unmerged curve (it was the plain mean of the rings, also of rings without pixels: the first point at 5.4e-5 below
  the first pixel at 9.1e-5); the points' Q increase.
- Progressive merging: each point the factor times wider than the one before (the widths grew with the ring index:
  137 rings gave 19 points at 1.05, 5 at 1.2).
- MS mode with progressive merging: no merged-away ring left as a point with I = 0 (118 of 137 at 1.05).
- A session keeps the method, also "Standard (Shells)" (opening it left the button of the session before).
"""

import os
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
o = p.Options
RUN = "00082561"  # H-L at 9.2 m

dr.new_session("KWS3-2020", dr.data_folder("kws3", "*.yaml"))
p.autoConfig("averaging-base")
assert o.radialAveraging() == {"method": "hf", "linear": 1, "progressive": 1.0}, o.radialAveraging()
o.setRadialAveraging(method="shells")  # the others as they are
assert o.radialAveraging() == {"method": "shells", "linear": 1, "progressive": 1.0}, o.radialAveraging()
for wrong in ({"method": "rings"}, {"linear": 0}, {"linear": 101}, {"progressive": 1.5}, {"progressive": 0.5}):
    try:
        o.setRadialAveraging(**wrong)
        raise AssertionError("no ValueError: %s" % wrong)
    except ValueError:
        pass
assert o.radialAveraging()["method"] == "shells"

count = [0]


def curve(mode="(SM) Standard Mode"):
    """(Q, I) of the run's I[Q] tables (MS mode: two), as the options are."""
    Dan.Options.selectMode(mode)
    count[0] += 1
    s = p.newScriptTable("averaging-%d" % count[0], [RUN])
    curves = []
    for name in [n for n in p.process(s, "I-Q") if n.startswith("QI")]:
        t = table(name)
        curves.append([(float(t.text(1, r)), float(t.text(2, r))) for r in range(1, t.numRows() + 1)])
        closeWindow(t)
    assert curves, p.lastMessages()
    return curves


for method in ("shells", "hf"):
    o.setRadialAveraging(method=method, linear=1, progressive=1.0)
    full = curve()[0]
    q_min, q_max = full[0][0], full[-1][0]
    for linear, progressive in ((3, 1.0), (7, 1.0), (1, 1.05), (1, 1.2)):
        o.setRadialAveraging(linear=linear, progressive=progressive)
        merged = curve()[0]
        qs = [q for q, _ in merged]
        assert q_min <= qs[0] and qs[-1] <= q_max * (1 + 1e-9), \
            (method, linear, progressive, qs[0], qs[-1], q_min, q_max)
        assert all(a < b for a, b in zip(qs, qs[1:])), (method, linear, progressive)
        if progressive > 1:
            # widths 1, f, f^2, ... (rounded) over the ~137 rings: 38 points at 1.05, 15 at 1.2 (were 19 and 5)
            assert len(merged) >= {1.05: 35, 1.2: 13}[progressive], (method, progressive, len(merged))

# MS mode: no point with I = 0 from merged-away rings
o.setRadialAveraging(method="hf", linear=1, progressive=1.05)
for ms in curve("(MS) Magnetic Samples"):
    assert sum(1 for _, i in ms if i == 0) == 0, ms[:5]
    assert all(a[0] < b[0] for a, b in zip(ms, ms[1:]))
Dan.Options.selectMode("(SM) Standard Mode")

# the session keeps the method: Standard too
o.setRadialAveraging(method="shells", linear=1, progressive=1.0)
p.newScriptTable("averaging-session", [RUN])
o.setRadialAveraging(method="hf")
Dan.Options.openSession("averaging-session")
assert o.radialAveraging()["method"] == "shells", o.radialAveraging()
o.setRadialAveraging(method="hf")
