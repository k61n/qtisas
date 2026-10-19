a = Ascii1d
a.reset()
made = a.importFiles("~/benchdata/multi/*.dat")
scriptPrint("R5:", len(made), sum(table([n for n in made if "102" in n][0]).colData(2)))
