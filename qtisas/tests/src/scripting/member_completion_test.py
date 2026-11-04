"""After "Name." a script editor offers only that object's members (Dan.M -> Mask, Merge), not every keyword with the
same start; for a name that is no object the keyword list completes as before."""

import os
import sys

try:
    from PyQt6.QtCore import QCoreApplication, QEvent, Qt
    from PyQt6.QtGui import QKeyEvent, QTextCursor
    from PyQt6.QtWidgets import QCompleter
except ImportError:
    from PyQt5.QtCore import QCoreApplication, QEvent, Qt
    from PyQt5.QtGui import QKeyEvent, QTextCursor
    from PyQt5.QtWidgets import QCompleter

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import quiet  # noqa: F401  (the tests' common set-up)

KEY = getattr(Qt, "Key", Qt)
NO_MODIFIER = getattr(Qt, "KeyboardModifier", Qt).NoModifier
KEY_PRESS = getattr(QEvent, "Type", QEvent).KeyPress
END = getattr(QTextCursor, "MoveOperation", QTextCursor).End

note = newNote("completion")
editor = note.currentEditor()
editor.setFocus()


def type_text(text):
    for ch in text:
        key = KEY.Key_Period if ch == "." else getattr(KEY, "Key_" + ch.upper(), 0)
        QCoreApplication.sendEvent(editor, QKeyEvent(KEY_PRESS, key, NO_MODIFIER, ch))
    QCoreApplication.processEvents()


def member_popup():
    """The visible completer of the editor's own member list, if any: (its strings)."""
    for c in editor.findChildren(QCompleter):
        if c.popup().isVisible():
            return [c.model().index(i, 0).data() for i in range(c.model().rowCount())]
    return None


editor.setPlainText("")
type_text("Dan.M")
assert member_popup() == ["Mask", "Merge"], member_popup()

editor.setPlainText("")
editor.moveCursor(END)
type_text("Dan.Mask.")
assert member_popup() is not None and "newMask" in member_popup() and "Merge" not in member_popup(), member_popup()

editor.setPlainText("")
type_text("Dan.Processing.Options.set")
expected = ["setCurveFormat", "setMatrixFormat", "setNormalization", "setPresentation", "setRadialAveraging",
            "setRemovePoints", "setSmoothing", "setWideAngle"]
assert member_popup() == expected, member_popup()

