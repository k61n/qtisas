p = Dan.Processing
p.setConfigurations(2)
for i, (eb, tag) in enumerate((("00082558", "9m"), ("00082645", "1m")), 1):
    p.setConfiguration(i, emptyBeam=eb, darkCurrent="00000002", calibrantEmptyBeam=eb, calibrantDarkCurrent="00000002",
                       center=eb, cellEmptyBeam=eb, mask="mask-bs-" + tag, transmissionMask="mask-tr-" + tag,
                       transmission=True)
p.calculate()
c = [p.configuration(i) for i in (1, 2)]
scriptPrint("R10:", *["%.0f" % x["factor"] for x in c], *["%.2f %.2f" % (x["xCenter"], x["yCenter"]) for x in c])
