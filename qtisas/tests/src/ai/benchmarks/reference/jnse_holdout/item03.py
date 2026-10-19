Jnse.fit("psInfo", "simpleDiffusion", 1, 0, ["A", "D"], {"A": 1, "D": 1}, {"w": 0})
scriptPrint("R3:", "%.4f" % (Fittable.paramValue("D") * max(Jnse.header("psInfo", "q")) ** 2))
