"""Table.setDecimalSeparators(format, omitGroupSeparator): the table takes the format and its cells are converted.

0 = system, 1 = C (.), 2 = German (,), 3 = French; afterwards text() shows the new format, setText() accepts it and
cell() is the number. Switching back converts the cells again.
"""

t = newTable("DecimalSeparators", 3, 1)
t.setColData(1, [1234.5, 0.25, -3.0])
shown = lambda: [t.text(1, row) for row in range(1, 4)]
assert shown() == ["1234.5", "0.25", "-3"], shown()

t.setDecimalSeparators(2)  # German, no group separators
assert shown() == ["1234,5", "0,25", "-3"], shown()
assert [t.cell(1, row) for row in range(1, 4)] == [1234.5, 0.25, -3.0], [t.cell(1, row) for row in range(1, 4)]
t.setText(1, 2, "2,5")
assert t.cellOrNone(1, 2) == 2.5, (t.text(1, 2), t.cellOrNone(1, 2))

t.setDecimalSeparators(2, False)  # German with group separators
assert t.text(1, 1) == "1.234,5", t.text(1, 1)

t.setDecimalSeparators(1)  # back to C
assert shown() == ["1234.5", "2.5", "-3"], shown()
