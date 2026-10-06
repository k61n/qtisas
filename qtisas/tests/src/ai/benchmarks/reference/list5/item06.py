names = ["ai/Quad", "ai/Gauss5", "ai/Lor5"]
for name in names:
    Compiler.delete(name)
Compiler.scanGroups()
closeWindow(table("Peak5"))
closeWindow(note("Pipeline"))
left = [n for n in names if n in Compiler.functionNames("ALL")]
scriptPrint("R6: %s %s %s" % (left, existTable("Peak5"), existNote("Pipeline")))
