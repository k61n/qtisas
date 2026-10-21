o = Dan.Options
o.newSession()
o.selectInstrument("KWS1-2020")
o.setInputPath("~/benchdata/dan/kws1")
runs = Dan.Rawdata.runs()
scriptPrint("R1:", len(runs), runs[0], runs[-1])
