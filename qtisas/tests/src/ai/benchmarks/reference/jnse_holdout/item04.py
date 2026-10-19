j = Jnse
q = j.header("psInfo", "q")
t = table(j.datasets("psInfo")[q.index(min(q))])
scriptPrint("R4:", "%.4f" % t.cell(t.colNames().index("Sqt-To-Sq") + 1, t.numRows()))
