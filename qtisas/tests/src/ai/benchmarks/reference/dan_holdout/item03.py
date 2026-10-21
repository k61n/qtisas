p = Dan.Processing
p.setConfigurations(1)
p.setConfiguration(1, emptyBeam="497844", darkCurrent="497847", calibrant="497846", calibrantEmptyBeam="497844",
                   calibrantDarkCurrent="497847", center="497842", mask="mask-4m", sens="sens", cellEmptyBeam="497844")
p.calculate()
c = p.configuration(1)
scriptPrint("R3:", "%.3f" % c["factor"], "%.2f" % c["xCenter"], "%.2f" % c["yCenter"])
