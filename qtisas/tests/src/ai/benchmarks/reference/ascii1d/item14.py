a = Ascii1d
a.reset()
a.setOutputFolder("~/benchdata/out")
a.setExtension("cut")
a.setRemovePoints(first=3)
files = sorted(a.convertFiles("~/benchdata/multi/*.dat"))
scriptPrint("R14:", len(files), *[len([l for l in open(f) if l.strip()]) for f in files])
