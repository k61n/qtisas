"""Column formulas run in the global scripting language (Python here); only an explicit request takes muParser.

- Table.recalculate(col, start, end, forceMuParser): False -> Python ("i**2"), True -> muParser ("i^2"; "i**2" is
  no muParser and fails, leaving the column unchanged).
- "Recalculate" (Table.calculate(), the menu action) and the automatic recalculation after an edit use Python too;
  muParser runs only on request (forceMuParser, or "Apply muParser" in "Set column values").
- A syntax error leaves the column unchanged; a row without a finite result (an error, nan, inf) is left empty, in
  both engines.
- "Set column values": "Apply Python" and "Apply muParser" (with Python global, both are shown; with muParser global
  only "Apply muParser" is, not tested here); the default button is the one pressed last. Errors are shown in a
  QErrorMessage (not in the console); a syntax error restores the old formula, rows without a result keep the new one.
  A read-only column and Cancel on the multiline question give one message or none.
- A failing formula reports its first error once: in the console, the dialog's message and the ValueError's second
  line (also a multiline muParser formula, which fails row by row).
"""

import os
import sys

try:
    from PyQt6.QtCore import Q_RETURN_ARG, QMetaObject, Qt, QTimer
    from PyQt6.QtWidgets import (QApplication, QCheckBox, QErrorMessage, QMessageBox, QPushButton, QTableWidget,
                                 QTextEdit)
except ImportError:
    from PyQt5.QtCore import Q_RETURN_ARG, QMetaObject, Qt, QTimer
    from PyQt5.QtWidgets import (QApplication, QCheckBox, QErrorMessage, QMessageBox, QPushButton, QTableWidget,
                                 QTextEdit)

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import DIRECT, answering

column = lambda t, col: [t.cellOrNone(col, row) for row in range(1, t.numRows() + 1)]

t = newTable("RecalculateEngine", 3, 2)
t.setColData(2, [1.0, 2.0, 3.0])

t.setCommand(1, "i**2")
t.recalculate(1, 1, -1, False)
assert column(t, 1) == [1, 4, 9], column(t, 1)

try:  # muParser has no "**": an error, the column stays
    t.recalculate(1, 1, -1, True)
    failed = False
except ValueError:
    failed = True
assert failed and column(t, 1) == [1, 4, 9], (failed, column(t, 1))
t.setCommand(1, "i^2 + 1")
t.recalculate(1, 1, -1, True)
assert column(t, 1) == [2, 5, 10], column(t, 1)

# "Recalculate" (menu): the selected cells, in Python
t.setCommand(1, "i**3")
t.findChild(QTableWidget).selectColumn(0)
assert QMetaObject.invokeMethod(t, "calculate", DIRECT, Q_RETURN_ARG(bool)), "Recalculate failed"
assert column(t, 1) == [1, 8, 27], column(t, 1)

# automatic recalculation when a column the formula uses changes: in Python
setAutoUpdateTableValues(True)
t.setCommand(1, 'col("2")**2')
t.findChild(QTableWidget).item(0, 1).setText("5")  # as the cell editor does
assert column(t, 1)[0] == 25, column(t, 1)

# the console and the script window's output (plain QTextEdits, not the editors): error headings there
console_errors = lambda: sum(e.toPlainText().count("script Error") for e in QApplication.allWidgets()
                             if type(e) is QTextEdit)

# a row without a result is left empty, in both engines (1/0 at i = 2, a root of -1 at i = 1); the call reports it
for force, root, infinite in ((False, "math.sqrt(i-2)", "float('inf') if i == 2 else i"),
                              (True, "sqrt(i-2)", "i == 2 ? 1/0 : i")):
    for formula, expected in (("1/(i-2)", [-1, None, 1]), (root, [None, 0, 1]), (infinite, [1, None, 3])):
        t.setCommand(1, formula)
        console_before = console_errors()
        try:
            t.recalculate(1, 1, -1, force)
            failed = False
        except ValueError:
            failed = True
        assert failed and column(t, 1) == expected, (force, formula, failed, column(t, 1), expected)
        if formula == "1/(i-2)" and not force:  # a Python error: reported once, not for every row
            assert console_errors() == console_before + 1, (console_errors(), console_before)

# the reason of a failure is in the ValueError: the formula's first error message
# (a multiline muParser formula fails in every row: reported once too)
for force, formula, reason in ((False, "x + 1", "name 'x' is not defined"), (True, "x + 1", '"x"'),
                               (True, "x=i\ncell(9, i)", "no column 9")):
    t.setCommand(1, formula)
    console_before = console_errors()
    try:
        t.recalculate(1, 1, -1, force)
        error = ""
    except ValueError as e:
        error = str(e)
    assert error.startswith("recalculate: column 1") and reason in error, (force, formula, error)
    assert console_errors() == console_before + 1, (force, formula, console_errors(), console_before)

# "Set column values": "Apply Python" and "Apply muParser" (both shown, Python is global here); the blue default
# button is the one pressed last. An error is shown in a QErrorMessage, not in the console: after a syntax error the
# column and its formula stay as they were, after rows without a result the new formula is kept.
def apply_formula(formula, muparser, result, box_answer=None):
    def answer(dialog):
        edit = [e for e in dialog.findChildren(QTextEdit) if not e.isReadOnly()][0]
        result.append(edit.toPlainText())  # the column's formula
        buttons = {b.text(): b for b in dialog.findChildren(QPushButton)}
        assert not buttons["Apply &Python"].isHidden() and not buttons["Apply &muParser"].isHidden(), buttons
        result.append([name for name, b in buttons.items() if b.isDefault()])  # the button last used
        edit.setPlainText(formula)
        if box_answer is not None:  # the question before a multiline muParser formula
            def click():
                box = QApplication.activeModalWidget()
                if isinstance(box, QMessageBox):  # else: left to answering(), the test then fails on its text
                    box.button(box_answer).click()

            QTimer.singleShot(0, click)
        buttons["Apply &muParser" if muparser else "Apply &Python"].click()
        message = dialog.findChild(QErrorMessage)
        shown = message is not None and message.isVisible()
        result.append(message.findChild(QTextEdit).toPlainText() if shown else None)
        assert not shown or message.findChild(QCheckBox).isHidden(), "\"Show this message again\" is shown"
        if shown:
            message.close()
        QMetaObject.invokeMethod(dialog, "close", DIRECT)
        return True

    return answer


BUTTON = getattr(QMessageBox, "StandardButton", QMessageBox)
NOT_COMPILED, NO_RESULT, READ_ONLY = "could not be calculated", "no result for some or all rows", "is read-only"

setAutoUpdateTableValues(False)
t.setCommand(1, "i^2")
t.recalculate(1, 1, -1, True)
t.findChild(QTableWidget).selectColumn(0)
formula_before, last_button = "i^2", None  # the first dialog: the button as the user left it
for formula, muparser, expected, shown, box_answer, read_only in (
        ("i**2", True, [1, 4, 9], NOT_COMPILED, None, False),  # a syntax error in muParser, with the "^" hint
        ("i^3", True, [1, 8, 27], None, None, False),
        ("1/(i-2)", False, [-1, None, 1], (NO_RESULT, "ZeroDivisionError"), None, False),  # kept, with the reason
        ("i", True, [1, 2, 3], None, None, False),
        ("x=i\nx^2", True, [1, 2, 3], None, BUTTON.Cancel, False),  # multiline: not asked to continue
        ("x=i\nx^2", True, [1, 4, 9], None, BUTTON.Yes, False),
        ("i^3", True, [1, 4, 9], READ_ONLY, None, True)):  # read-only: one message, from the dialog
    result = []
    console_before = console_errors()
    t.setReadOnlyColumn(1, read_only)
    with answering(apply_formula(formula, muparser, result, box_answer)) as boxes:
        QMetaObject.invokeMethod(qti.app, "showColumnValuesDialog", DIRECT)
    t.setReadOnlyColumn(1, False)
    before, default, message = result
    assert not boxes, (formula, boxes)  # no other message box
    assert last_button is None or default == [last_button], (formula, default, last_button)
    assert before == formula_before, (formula, before, formula_before)
    assert column(t, 1) == expected, (formula, column(t, 1), expected)
    expected_texts = (shown,) if isinstance(shown, str) else shown or ()
    assert (message is None) if shown is None else all(x in (message or "") for x in expected_texts), (formula, message)
    assert shown != NOT_COMPILED or not muparser or "^" in message, message
    assert console_errors() == console_before, (formula, console_errors(), console_before)
    if not read_only and box_answer != BUTTON.Cancel:
        last_button = "Apply &muParser" if muparser else "Apply &Python"
    if (shown is None or NO_RESULT in expected_texts) and box_answer != BUTTON.Cancel:
        formula_before = formula
