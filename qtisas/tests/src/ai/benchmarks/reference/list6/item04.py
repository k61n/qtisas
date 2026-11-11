f = Fittable
f.configure("sasviewmodels/sphere", ["Lyso-sub_I"])
for name in f.paramNames():
    f.setParamVaries(name, name in ("scale", "radius", "background"))
f.setParamValue("scale", 0.3)
f.setParamValue("radius", 28.0)
f.setParamValue("background", 0.0)
f.setWeightingEnabled(1, True)
f.setWeightingDataset(1, "Lyso-sub_dI")
f.fit()
scriptPrint("R4: %d" % round(f.results()["radius"][0]))
