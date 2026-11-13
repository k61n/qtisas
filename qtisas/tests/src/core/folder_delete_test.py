"""deleteFolder() from Python asks nothing: a script deleting folders runs through.

Confirmed (2026-11-13): a script removing every "DAN ::" folder recursively stopped at a "Delete folder ...?" message
box for each folder (the "confirm close folder" preference is on by default). deleteFolder(folder, ask=True) still
asks, as the menu does.
"""

import os
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import answering

root = rootFolder()


def names(folder):
    """The names of all folders below folder."""
    return [name for child in folder.folders() for name in [child.name()] + names(child)]


def subfolder(folder, name):
    """The subfolder called name, looked up anew: the Python object of a deleted folder must not be used."""
    return next((child for child in folder.folders() if child.name() == name), None)


keep = addFolder("FolderDeleteKeep", root)
addFolder("DAN :: FolderDeleteB", keep)
changeFolder(addFolder("DAN :: FolderDeleteA", root))
newTable("FolderDeleteT", 3, 2)
changeFolder(root)


def remove_matching_folders(folder):  # the user's script
    for child in list(folder.folders()):
        remove_matching_folders(child)
        if child.name().startswith("DAN :: FolderDelete"):
            deleteFolder(child)


with answering() as messages:
    remove_matching_folders(root)
assert messages == [], messages  # no "Delete folder ...?" box
assert "FolderDeleteKeep" in names(root), names(root)
assert not any(name.startswith("DAN :: FolderDelete") for name in names(root)), names(root)
assert table("FolderDeleteT") is None  # the folder's windows went with it

# asked for: the box comes, as from the menu, if the preference is on (the tests share the user's settings)
with answering() as messages:
    deleteFolder(addFolder("FolderDeleteAsk", root), ask=True)
assert len(messages) <= 1 and all("Delete folder" in text for text in messages), messages

for name in ("FolderDeleteAsk", "FolderDeleteKeep"):
    if subfolder(root, name):
        deleteFolder(subfolder(root, name))
assert not any(name.startswith("FolderDelete") for name in names(root)), names(root)
