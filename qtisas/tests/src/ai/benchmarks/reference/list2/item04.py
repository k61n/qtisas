c = Compiler
for params, code in ((["scale", "rate"], "y = scale*exp(-rate*x);"),
                     (["scale", "rate", "offset"], "y = scale*exp(-rate*x) + offset;")):
    c.new()
    c.setFunctionName("ai/ExpDecay")
    c.setGroup("reference")
    c.setXName("x")
    c.setYName("y")
    c.setParamCount(len(params))
    c.setParamNamesAll(params)
    c.setParamInitsAll([1.0] * len(params))
    c.setParamVariesAll([True] * len(params))
    c.setCode(code)
    c.compile()
c.scanGroups()
c.open("ai/ExpDecay")
scriptPrint("R4: %s %d" % (" ".join(c.paramNames()), c.functionNames("ALL").count("ai/ExpDecay")))
