j = Jnse
j.reset()
j.setInputFolder("~/benchdata/jnse")
header = j.importFiles(["report90001_1.2.dtr", "report90002_1.2.dtr"], "pepInfo")
temps = sorted(set(j.header(header, "temp")))
scriptPrint("R4:", len(j.datasets(header)), "%g" % temps[0], "%g" % temps[1])
