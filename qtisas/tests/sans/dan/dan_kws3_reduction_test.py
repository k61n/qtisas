"""The KWS-3 example of qtisas.com/dan-sans reduced through Dan.Options, Dan.Processing, ... (dan_reduction.py;
skipped without its files).

- KWS3-2020's transmission method is the direct beam (its "Direct Beam [dead-time +]" was read as ROI in Header).
- In the order of the tabs (masks, sensitivity, configurations with their beam-stop masks, then calculate()): the
  direct-beam factors and the centers come through the full mask "mask", I[Q] through each configuration's own.
- A run given where a file is asked: the error says it is a run (newSensitivity makes it), its extension guessed too.
- Absolute factors, transmissions and every I[Q] and merged table as in dan-example-kws3.qti (the 1.2 m data scaled
  to the 9.2 m transmission of the same sample).
"""

import os
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

folder = dr.data_folder("kws3", "*.yaml")
script = dr.kws3(folder)
assert Dan.Options.Config.transmissionMethod() == "Direct Beam [dead-time +]", Dan.Options.Config.transmissionMethod()
config = [Dan.Processing.configuration(i) for i in (1, 2)]
assert [round(c["factor"]) for c in config] == [280100, 2816], config
assert [c["C"] for c in config] == [11.0, 11.0], config  # the instrument's collimation: 2f of the mirror
assert [c["mask"] for c in config] == ["mask-bs-9m", "mask-bs-1m"], config  # set back after calculate()
assert [(c["xCenter"], c["yCenter"]) for c in config] == [(120.044, 140.661), (119.972, 130.853)], config
s = table(script)
tr = [s.text(dr.column(s, "Transmission-Sample"), r).split("[")[0].strip() for r in range(1, 7)]
assert tr == ["0.5380", "0.5471", "0.4829", "0.8232", "0.7811", "0.8087"], tr
Dan.Processing.process(script)
assert Dan.Merge.join(script + "-mergingTemplate") == ["H-M", "H-J", "H-L"]
reference = {  # dan-example-kws3.qti: rows, (Q, I) of rows 1, 20 and the last (merged tables saved to 7 digits);
    # the 1.2 m tables' last point (its ring leaves the detector) differs: the HF averaging's corner pixel of a
    # point at x in (-1, 0) was pixel 0, weights 1.5 and -0.5 (a masked pixel 0 left a negative weight); now floor();
    # and the parallax correction around the row's beam center, not the mask's beam-stop box (2e-5 there); the 1.2 m
    # rows' direct-beam transmissions with the solved dead-time root (one Newton step before: H-M +3.5e-4)
    "QI-SM-00082557-H-M": (132, ((0.000108956598, 11716934), (0.000453985689, 145859.18), (0.00248781846, 1251.67575))),
    "QI-SM-00082559-H-J": (132, ((0.000108956598, 11797939.9), (0.000453985689, 124618.121),
                                 (0.00248781846, 972.144445))),
    "QI-SM-00082561-H-L": (132, ((0.000108956598, 14213305.1), (0.000453985689, 207155.057),
                                 (0.00248781846, 1915.47445))),
    "QI-SM-00082646-H-J": (122, ((0.00139221903, 8485.03175), (0.00403734514, 195.872155), (0.0182287105, 3.18523525))),
    "QI-SM-00082647-H-L": (122, ((0.00139221903, 23269.6257), (0.00403734514, 470.741965), (0.0182287105, 5.78666938))),
    "QI-SM-00082648-H-M": (122, ((0.00139221903, 11373.5726), (0.00403734514, 317.50691), (0.0182287105, 2.66107676))),
    "H-M": (229, ((0.0001089566, 11716930), (0.0004539857, 145859.2), (0.01822871, 2.661077))),
    "H-L": (229, ((0.0001089566, 14213310), (0.0004539857, 207155.1), (0.01822871, 5.786669))),
}
for name, (count, values) in reference.items():
    dr.check_rows(name, count, values, rel=1e-6 if name.startswith("QI") else 5e-6)

for given in ("00000001", "00000001.sens"):
    try:
        Dan.Sensitivity.loadSensitivity(given)
        raise AssertionError("no ValueError: " + given)
    except ValueError as e:
        assert '"00000001" is a run; Dan.Sensitivity.newSensitivity(run) makes it' in str(e), str(e)
