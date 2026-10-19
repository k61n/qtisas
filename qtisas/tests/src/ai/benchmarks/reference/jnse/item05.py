Jnse.fit("pepInfo", "simpleDiffusion", 1, 6, ["A", "D"], {"A": 1, "D": 1}, {"w": 0})
scriptPrint("R5:", "%.2f" % Fittable.paramValue("D"))
