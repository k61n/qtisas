keep = {"Table1", "Table2", "Info", "Lyso-sub"}
for name in tableNames():
    if name not in keep:
        closeWindow(table(name))
scriptPrint("R8: " + " ".join(sorted(tableNames())))
