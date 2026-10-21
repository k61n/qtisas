o = Dan.Options
o.newSession()
o.selectInstrument("KWS3-2020")
o.setInputPath("~/benchdata/dan/kws3")
Dan.Mask.newMask()
for tag, bs, tr in (("9m", (114, 126, 134, 147), (117, 123, 137, 144)),
                    ("1m", (110, 131, 119, 142), (117, 123, 127, 135))):
    Dan.Mask.newMask("mask-bs-" + tag, beamstop=bs)
    Dan.Mask.newMask("mask-tr-" + tag, beamstop=tr, transmission=True)
Dan.Sensitivity.newSensitivity("00000001")
m, s = matrix("mask"), matrix("sens")
values = [s.cell(r, c) for r in range(1, s.numRows() + 1) for c in range(1, s.numCols() + 1) if m.cell(r, c) > 0]
scriptPrint("R9:", s.numRows(), "%.3f" % (sum(values) / len(values)))
