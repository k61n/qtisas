"""Exercise Table::setCellOrText through its Qt slot in the running application."""

import os
import sys

try:
    from PyQt6.QtCore import Q_ARG, QLocale, Qt
except ImportError:
    from PyQt5.QtCore import Q_ARG, QLocale, Qt

number_options = getattr(QLocale, "NumberOption", QLocale)
connection_type = getattr(Qt, "ConnectionType", Qt)

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import locale_german

source = QLocale("C")
source.setNumberOptions(number_options.RejectGroupSeparator)
german = locale_german()

table = newTable("CellOrTextTest", 2, 2)
table.setLocale(german)
table.setColTextFormat(2)

slot = next(
    (table.metaObject().method(i) for i in range(table.metaObject().methodCount())
     if table.metaObject().method(i).methodSignature() == b"setCellOrText(int,int,QString,QLocale)"),
    None,
)
assert slot is not None, "Table::setCellOrText is not a Qt slot"


def write(row, column, value, input_locale=source):
    slot.invoke(
        table,
        connection_type.DirectConnection,
        Q_ARG(int, row),
        Q_ARG(int, column),
        Q_ARG(str, value),
        Q_ARG(QLocale, input_locale),
    )


# Qt's slot uses zero-based indices; QtiSAS's Python table API uses one-based indices.
write(0, 0, "12.5")
assert table.text(1, 1) == "12,5", table.text(1, 1)
assert table.cell(1, 1) == 12.5, table.cell(1, 1)

write(0, 0, "---")
assert table.text(1, 1) == "", table.text(1, 1)

write(0, 0, "1,333")  # C source locale rejects grouping.
assert table.text(1, 1) == "", table.text(1, 1)

write(1, 0, "0")
assert table.text(1, 2) == "0", table.text(1, 2)

table.setLocale(source)
write(0, 0, "1,25", german)
assert table.text(1, 1) == "1.25", table.text(1, 1)

write(0, 1, "not a number")
assert table.text(2, 1) == "not a number", table.text(2, 1)
