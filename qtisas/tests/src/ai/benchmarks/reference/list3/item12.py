names = ["ai/RefLine", "ai/RefDecay"]
for name in names:
    Compiler.delete(name)
scriptPrint("R12: %s" % [name for name in names if name in Compiler.functionNames("ALL")])
