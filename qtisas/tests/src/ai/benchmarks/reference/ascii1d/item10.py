a = Ascii1d
a.reset()
a.setInputFolder("~/benchdata/in")
a.setResolution(True)
a.setResolutionGeometry(800, 400, 0.525, 10, 10, 30, 30, 6, 0.1)  # cm, cm, cm, mm, mm, mm, mm, Å, Δλ/λ
t = table(a.importFiles("opt.dat", "optSigma")[0])
scriptPrint("R10:", "%.6g" % t.cell(4, 1))
