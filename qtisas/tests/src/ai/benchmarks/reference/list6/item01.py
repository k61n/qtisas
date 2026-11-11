names = tableNames()
curves = [n for n in names if n in ("Table1", "Table2")]  # the two tables with Q, I, dI columns
scriptPrint("R1: %d %d" % (len(names), sum(table(n).numRows() for n in curves)))
