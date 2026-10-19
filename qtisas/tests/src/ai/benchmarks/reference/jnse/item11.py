d = []
for rows in ((1, 6), (7, 12)):
    Jnse.fit("pepInfo", "simpleDiffusion", rows[0], rows[1], ["A", "D"], {"A": 1, "D": 1}, {"w": 0})
    d.append(Fittable.paramValue("D"))
scriptPrint("R11:", "%.2f" % (d[1] / d[0]))
