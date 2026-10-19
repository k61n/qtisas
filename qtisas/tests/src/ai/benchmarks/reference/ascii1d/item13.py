a = Ascii1d
a.reset()
a.setOutputFolder("~/benchdata/converted")
a.setExtension("RAD")
a.setResolution(True)
a.setResolutionGeometry(800, 400, 0.525, 10, 10, 30, 30, 6, 0.1)  # cm, cm, cm, mm, mm, mm, mm, Å, Δλ/λ
files = a.convertFiles("~/benchdata/multi/*.dat")
scriptPrint("R13:", len(files), len(open(files[0]).readline().split()))
