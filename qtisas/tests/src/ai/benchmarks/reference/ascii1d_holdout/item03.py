a = Ascii1d
a.reset()
a.setOutputFolder("~/benchdata/converted")
a.setPresentation("Guinier")
scriptPrint("R3:", len(a.convertFiles("~/benchdata/multi/*.dat")))
