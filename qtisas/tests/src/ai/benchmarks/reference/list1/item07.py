import random
def sphere(q, R=50.0):
    qr = q * R
    return (3 * (sin(qr) - qr * cos(qr)) / qr**3) ** 2
t = newTable("SAXS_data", 100, 3)
t.setColNames(["Q", "I", "dI"])
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(2, Table.PlotDesignation.Y)
t.setColumnRole(3, Table.PlotDesignation.yErr)
qs = [0.005 * (i + 1) for i in range(100)]
clean = [sphere(q) for q in qs]
t.setColData(1, qs)
t.setColData(2, [v + random.gauss(0, 0.03 * v) for v in clean])
t.setColData(3, [0.03 * v for v in clean])
scriptPrint("R7: %d %.6f" % (t.numRows(), sphere(0.1)))
