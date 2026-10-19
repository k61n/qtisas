Jnse.fit("pepInfo", "simpleDiffusion", 7, 12, ["A", "D"], {"A": 1, "D": 1}, {"w": 0})
scriptPrint("R7:", "%.4f" % (Fittable.paramValue("D") * 0.1 ** 2))
