"""Compiler.functionNames() is a query: it lists a category as the Compile window does, without selecting it.

Confirmed (2026-11-13): the chat's probes called functionNames("ALL") in nearly every message about fit functions;
each call selected "ALL" in the window's category list, which rebuilt the function list and cleared the open function,
and a folder category ("sasviewmodels/") gave [] (only the functions' [group] lines were matched). Uses the folder
NIST/ (NIST/Misra1b), else skipped. Nothing is written into the user's function library.
"""

import os
import sys

try:
    from PyQt6.QtWidgets import QListView
except ImportError:
    from PyQt5.QtWidgets import QListView

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import skip

c = Compiler
everything = c.functionNames("ALL")
if "NIST/Misra1b" not in everything or "NIST/" not in c.categories():
    skip("function NIST/Misra1b or the folder NIST/ is not in the library")

c.open("NIST/Misra1b")
c.selectCategory("NIST/")
groups = c.findChild(QListView, "listViewGroup")
functions = c.findChild(QListView, "listViewFunctions")
listed = [functions.model().index(i, 0).data() for i in range(functions.model().rowCount())]

# a folder lists its files, as the window does
nist = c.functionNames("NIST/")
assert nist == listed and "NIST/Misra1b" in nist, (nist[:5], listed[:5])
assert all(name.startswith("NIST/") for name in nist), nist[:5]

# no side effect: the selected category, the window's function list and the open function stay
c.open("NIST/Misra1b")
for category in ("ALL", "NIST/", c.groups()[0]):
    c.functionNames(category)
    assert c.category() == "NIST/", c.category()
    assert c.functionName() == "NIST/Misra1b", c.functionName()
assert [functions.model().index(i, 0).data() for i in range(functions.model().rowCount())] == listed

# "ALL" as before: every function, the top level first; no "(...)" copies
assert "NIST/Misra1b" in everything and not any("(" in name for name in everything), everything[:5]
assert c.functionNames("") == nist  # empty: the selected category

# no drag auto-scroll in the two lists: a drag over them started a timer that never stopped (the list jumped)
assert not groups.hasAutoScroll() and not functions.hasAutoScroll()
