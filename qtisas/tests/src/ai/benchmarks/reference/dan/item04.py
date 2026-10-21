p = Dan.Processing
p.setConfigurations(3)
runs = (("53216", "53229", "53230", "53218"), ("53220", "53224", "53225", "53222"), ("53225", "53224", "53225", "53227"))
for i, (eb, plexi, plexiEB, center) in enumerate(runs, 1):
    p.setConfiguration(i, emptyBeam=eb, darkCurrent="48462", calibrant=plexi, calibrantEmptyBeam=plexiEB,
                       calibrantDarkCurrent="48462", center=center, cellEmptyBeam=eb, transmission=(i == 2))
p.calculate()
c = [p.configuration(i) for i in (1, 2, 3)]
scriptPrint("R4:", *["%.0f" % x["factor"] for x in c], *["%.2f" % x["xCenter"] for x in c])
