Jnse.fit("psInfo", "simpleDiffusion", 1, 0, ["A", "D"], {"A": 1, "D": 1}, {"w": 0})
scriptPrint("R2:", "%.2f" % Fittable.paramValue("D"))
