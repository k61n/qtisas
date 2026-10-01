"""Hidden windows stay hidden across folder switches and project save; shown windows leave the hidden list; a hidden
window does not stay the active one."""

import os
import shutil
import sys
import tempfile

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import show_windows_of_active_folder

show_windows_of_active_folder()  # the folder switch below shows the windows that are not hidden

hidden = newTable("HiddenStateHidden", 3, 2)
shown = newTable("HiddenStateShown", 3, 2)
hideWindow(hidden)
hideWindow(shown)
activateWindow(shown)

root = activeFolder()
other = addFolder("HiddenStateOther")
changeFolder(other)
changeFolder(root)
assert not hidden.isVisible(), "hidden table reappeared after a folder switch"
assert shown.isVisible(), "re-shown table disappeared after a folder switch"

folder = tempfile.mkdtemp()
saveProjectAs(os.path.join(folder, "hidden_state.qti"))
with open(os.path.join(folder, "hidden_state.qti"), encoding="utf-8", errors="replace") as f:
    lines = f.read().split("\n")
shutil.rmtree(folder)


def geometry_of(name):
    for i, line in enumerate(lines):
        if line.startswith(name + "\t"):
            return lines[i + 1]
    raise AssertionError("%s not saved" % name)


assert geometry_of("HiddenStateHidden").endswith("hidden"), geometry_of("HiddenStateHidden")
assert not geometry_of("HiddenStateShown").endswith("hidden"), geometry_of("HiddenStateShown")

# A window opened over a maximized one and hidden again (as FitTable's statistics note): the maximized window is
# active again, and the next new window opens maximized
base = newTable("HiddenStateMaximized", 3, 2)
base.showMaximized()
note = newNote("HiddenStateNote")
hideWindow(note)
area = workspace()
active = lambda: area.activeSubWindow().objectName()  # the MDI area's active window
assert active() == base.objectName() and base.isMaximized(), (active(), base.isMaximized())
graph = newGraph("HiddenStateGraph")
states = (active(), graph.isMaximized(), base.isMaximized())
assert states == (graph.objectName(), True, False), states
