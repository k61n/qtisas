j = Jnse
j.reset()
j.setInputFolder("~/benchdata/jnse")
header = j.importFiles("report95001_2.5.dtr", "psInfo")
q = j.header(header, "q")
scriptPrint("R1:", len(q), "%.4f" % min(q))
