n = note("Batch")
n.addTab()  # addTab() makes the new tab the active one, the one autoexec runs
last = n.tabs()
n.tab(last).setText('t = table("RawScan")\nprint(t.numRows())\n')
n.setAutoexec(True)
scriptPrint("R3: %d %s %d" % (n.tabs(), n.autoexec(), table("RawScan").numRows()))
