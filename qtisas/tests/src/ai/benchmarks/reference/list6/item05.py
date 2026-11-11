c = Compiler
hits = [n for n in c.functionNames("ALL") if n.startswith("ai/") and "decay" in n.lower()]
c.open(hits[0])
scriptPrint("R5: %s %s" % (hits[0], " ".join(c.paramNames())))
