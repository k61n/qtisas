Jnse.fit("pepInfo", "simpleDiffusion", 7, 10, ["A", "D"], {"A": 1, "D": 1}, {"w": 0})
scriptPrint("R6:", "%.2f" % Fittable.paramValue("D"))
