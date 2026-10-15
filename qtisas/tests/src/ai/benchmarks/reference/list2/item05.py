f = Fittable
f.configure("ai/ExpDecay", ["Baseline_y"])
chi2_dof = f.fit()
scriptPrint("chi2/dof = %g" % chi2_dof)
scriptPrint("R5: ai/ExpDecay Baseline_y")
