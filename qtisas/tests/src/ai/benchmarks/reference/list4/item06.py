closeWindow(table("Names"))
closeWindow(matrix("Grid"))
scriptPrint("R6: %s %s" % (table("Names") is not None, matrix("Grid") is not None))
