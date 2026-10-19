j = Jnse
header = "report90001_1-2-info"
scriptPrint("R2:", "%g" % j.header(header, "temp")[2], table(j.datasets(header)[2]).numRows())
