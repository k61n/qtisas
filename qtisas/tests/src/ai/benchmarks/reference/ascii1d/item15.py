a = Ascii1d
a.reset()
a.setAutoPrefix(True)
a.setPresentation("Kratky")
made = a.convertTables(["QI-alpha", "QI-beta"])
scriptPrint("R15:", len(made), round(table([n for n in made if "beta" in n][0]).cell(2, 2), 6))
