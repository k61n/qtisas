o = Dan.Options
o.newSession()
o.selectInstrument("SANS1")
o.setInputPath("~/benchdata/dan/sans1")
Dan.Mask.newMask("mask-4m", edge=(1, 128, 4, 127), beamstop=(57, 71, 58, 71), shape="rectangle", polygons="")
Dan.Mask.newMask("mask-tr", edge=(1, 128, 4, 127), beamstop=(57, 71, 58, 71), shape="rectangle", polygons="", transmission=True)
tr, dtr = Dan.Sensitivity.transmission("497866", "497864", "mask-tr")
scriptPrint("R1:", "%.4f" % tr)
