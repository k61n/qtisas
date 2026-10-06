t = newTable("Format", 3, 1)
t.setColData(1, [1234.5, 0.25, -3])
t.setDecimalSeparators(2, True)
texts = [t.text(1, row) for row in range(1, 4)]
t.setText(1, 2, "2,5")
scriptPrint("R5: %s %g" % (" ".join(texts), t.cell(1, 2)))
