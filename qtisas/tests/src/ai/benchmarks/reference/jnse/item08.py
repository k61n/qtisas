j = Jnse
row = 1 + min(range(6), key=lambda k: abs(j.header("pepInfo", "q")[k] - 0.1034))
j.fit("pepInfo", "simpleDiffusion", row, row, [], {"A": 1, "D": 1}, {"w": 0})
scriptPrint("R8:", "%.2f" % Fittable.paramValue("D"))
