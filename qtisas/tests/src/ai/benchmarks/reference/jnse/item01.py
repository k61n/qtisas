j = Jnse
j.reset()
j.setInputFolder("~/benchdata/jnse")
header = j.importFiles("report90001_1.2.dtr")
q = j.header(header, "q")
scriptPrint("R1:", header, len(q), "%.4f" % max(q))
