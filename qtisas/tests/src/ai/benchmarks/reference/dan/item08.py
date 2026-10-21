for s in ("H-J", "H-L", "H-M"):
    setWindowName(table(s), "KWS1-" + s)
scriptPrint("R8:", *sorted(n for n in tableNames() if n.startswith("KWS1-")))
