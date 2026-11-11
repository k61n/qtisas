t = table("Lyso-sub")
pts = [(q * q, log(i)) for q, i in zip(t.colData(1), t.colData(2)) if i > 0]
rg = 30.0
for _ in range(5):  # Guinier range q*Rg < 1.3, refined with the fitted Rg
    sel = [p for p in pts if sqrt(p[0]) * rg < 1.3]
    n = len(sel)
    mx = sum(p[0] for p in sel) / n
    my = sum(p[1] for p in sel) / n
    slope = sum((p[0] - mx) * (p[1] - my) for p in sel) / sum((p[0] - mx) ** 2 for p in sel)
    rg = sqrt(-3 * slope)
i0 = exp(my - slope * mx)
scriptPrint("R3: %d %d" % (round(rg), round(i0)))
