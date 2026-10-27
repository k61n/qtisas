o, p = Dan.Options, Dan.Processing
o.newSession()
o.selectInstrument("KWS1-2020")
o.setInputPath("~/benchdata/dan/kws1")
s = p.autoConfig("auto1")
c = p.configuration(2)
scriptPrint("R17:", table(s).numRows(), c["emptyBeam"], c["calibrant"], c["center"])
