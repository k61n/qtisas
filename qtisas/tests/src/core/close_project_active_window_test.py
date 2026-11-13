"""closeProject() with the active window in a subfolder: no window stays active afterwards.

Confirmed (2026-11-13): closeProject() deleted the windows of subfolders without closeWindow(); with one of them
active, the application's active window pointed to a deleted window, and the next look at it crashed QtiSAS
(currentTable() here; opening the Edit menu in the user's QtiSAS). newProject() and openProject() close the
project the same way.
"""

root = rootFolder()
sub = addFolder("CloseProjectSub", root)
changeFolder(sub)
newTable("CloseProjectT", 3, 2)
assert currentTable() is not None and currentTable().objectName() == "CloseProjectT"

assert closeProject(False)
assert windows() == [], windows()
assert currentTable() is None and currentNote() is None and currentMatrix() is None  # crashed here before
