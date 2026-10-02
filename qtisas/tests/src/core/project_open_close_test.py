"""Projects from Python: closeProject(save), newProject(save), openProject(path, save) — all in this window.

save=None asks (not tested here: a dialog), False discards unsaved changes, True saves first (ValueError while the
project has no file). openProject() closes the current project, then opens .qti / .qti.gz / .qti.svg here; a missing
file or another file type is a ValueError. The application object stays the same throughout, so a running script
(and the script server) keeps working. openProject is called as qti.app.openProject: the builtin of the same name
comes from qtisasrc.py, whose copy in the user's python folder is refreshed only with a new version.
"""

import os
import shutil
import tempfile

import qti

app = qti.app
folder = tempfile.mkdtemp(prefix="qtisas-project-test-")
path = os.path.join(folder, "p1.qti")
names = lambda: sorted(w.objectName() for w in app.windows())

closeProject(save=False)
assert names() == [], names()

# save=True needs a file
newTable("ProjectA", 3, 2)
try:
    closeProject(save=True)
    raised = False
except ValueError as e:
    raised = "saveProjectAs" in str(e)
assert raised and names() == ["ProjectA"], (raised, names())

saveProjectAs(path)
assert os.path.isfile(path), path

# save=True saves the changes before closing; openProject brings them back
newTable("ProjectB", 2, 1)
assert closeProject(save=True) and names() == [], names()
assert app.openProject(path) and names() == ["ProjectA", "ProjectB"], names()
assert qti.app is app  # the same application: this script keeps running

# save=False discards: the table added now is gone after reopening
newTable("ProjectC", 2, 1)
assert app.openProject(path, save=False) and names() == ["ProjectA", "ProjectB"], names()

# newProject keeps this window too: an empty project
assert newProject(save=False) and names() == [] and qti.app is app, names()

# errors: a missing file, a file that is no project
for wrong, expected in ((os.path.join(folder, "missing.qti"), "no readable file"), (__file__, "not a project file")):
    try:
        app.openProject(wrong, save=False)
        message = ""
    except ValueError as e:
        message = str(e)
    assert expected in message, (wrong, message)

shutil.rmtree(folder)
