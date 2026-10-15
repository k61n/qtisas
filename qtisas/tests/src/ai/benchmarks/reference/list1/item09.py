f = Fittable
f.configure("sasviewmodels/sphere", ["SAXS_data_I"])
for p in ("scale", "background", "radius"):
    f.setParamVaries(p, True)
f.setWeightingEnabled(1, True)
f.setWeightingDataset(1, "SAXS_data_dI")
f.setFitMethod("LM")
chi2_dof = f.fit()
radius = f.results()["radius"][0]
scriptPrint("chi2/dof = %g" % chi2_dof)
scriptPrint("R9: %d %s" % (round(radius), chi2_dof < 3))
