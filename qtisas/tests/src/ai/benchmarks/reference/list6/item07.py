n = newNote("Report6")
n.tab(1).setText("Sample: lysozyme 5 mg/ml\nRg = 23 A (Guinier)\nSphere radius = 30 A")
text = note("Report6").tab(1).toPlainText()
scriptPrint("R7: %s %s" % (existNote("Report6"), "lysozyme" in text.lower()))
