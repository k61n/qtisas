j = Jnse
j.reset()
j.setInputFolder("~/benchdata/jnse")
try:
    j.importFiles("broken.dtr")
    scriptPrint("R10:", len(j.datasets("broken-info")))
except ValueError as e:
    print(e)
    scriptPrint("R10:", 0)
