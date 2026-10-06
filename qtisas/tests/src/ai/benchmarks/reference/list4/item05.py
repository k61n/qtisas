t = newTable("Names", 3, 2)
t.setColData(1, [1, 2, 3])
t.setColTextFormat(2)  # text into a Numeric column would be evaluated and lost
for row, name in enumerate(["alpha", "beta", "gamma"], 1):
    t.setText(2, row, name)
scriptPrint("R5: %s %g" % (t.text(2, 2), t.cell(1, 3)))
