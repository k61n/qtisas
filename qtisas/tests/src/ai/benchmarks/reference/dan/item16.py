o, p = Dan.Options, Dan.Processing
o.newSession()
o.selectInstrument("KWS1-2020")
o.setInputPath("~/benchdata/dan/kws1")
Dan.Mask.newMask()
Dan.Sensitivity.loadSensitivity("~/benchdata/dan/kws1/sens-20200117.sens")
p.setConfigurations(3)
runs = (("53216", "53229", "53230", "53218"), ("53220", "53224", "53225", "53222"), ("53225", "53224", "53225", "53227"))
for i, (eb, plexi, plexiEB, center) in enumerate(runs, 1):
    p.setConfiguration(i, emptyBeam=eb, darkCurrent="48462", calibrant=plexi, calibrantEmptyBeam=plexiEB,
                       calibrantDarkCurrent="48462", center=center, cellEmptyBeam=eb, transmission=(i == 2))
o.Config.setCenterMethod("symmetry")
p.calculate()
c = p.configuration(3)
scriptPrint("R16:", "%.2f %.2f" % (c["xCenter"], c["yCenter"]))
