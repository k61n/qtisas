made = Dan.Processing.process("script")
qi = [n for n in made if n.startswith("QI-")]
scriptPrint("R6:", len(qi), table("QI-SM-53227-H-L").numRows())
