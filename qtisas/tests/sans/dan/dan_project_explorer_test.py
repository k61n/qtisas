"""The project explorer after DAN made a table in its folder ("Sort output to folders"): the folder of the new table is
the one the explorer shows, the table listed there with its label. Before, the buttons "New Info Table" and "New Info
Extractor" made the table in "DAN :: script, info, ..." when that folder already existed, but the explorer still
showed (and listed the table under) the folder before: ApplicationWindow::changeFolder(name) only set the current
folder for an existing one.
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtCore import QTimer
    from PyQt6.QtWidgets import QAbstractButton, QApplication, QCheckBox, QInputDialog, QTreeWidget
except ImportError:
    from PyQt5.QtCore import QTimer
    from PyQt5.QtWidgets import QAbstractButton, QApplication, QCheckBox, QInputDialog, QTreeWidget

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

FOLDER = "DAN :: script, info, ..."


def explorer():
    """The folder tree (Folder column) and the window list (Name, Type, View, Created, Label) of the explorer."""
    trees = [w for w in QApplication.allWidgets() if isinstance(w, QTreeWidget)]
    windows = [t for t in trees if t.columnCount() == 5][0]
    folders = [t for t in trees if t.columnCount() == 2 and t is not windows][0]
    return folders, windows


def shown():
    folders, windows = explorer()
    items = {windows.topLevelItem(i).text(0): windows.topLevelItem(i).text(4)
             for i in range(windows.topLevelItemCount())}
    return folders.currentItem().text(0), items


def folder_of(name):
    found = []

    def walk(f):
        if any(w.objectName() == name for w in f.windows()):
            found.append(f.name())
        for g in f.folders():
            walk(g)

    walk(rootFolder())
    return found


def click_new(button, name):
    """The button's name dialog answered with name."""
    def answer():
        for w in QApplication.topLevelWidgets():
            if isinstance(w, QInputDialog) and w.isVisible():
                w.setTextValue(name)
                w.accept()

    QTimer.singleShot(200, answer)
    Dan.findChild(QAbstractButton, button).click()


work = tempfile.mkdtemp()
try:
    dr.new_session("KWS1-2020", work)
    Dan.findChild(QCheckBox, "checkBoxSortOutputToFolders").setChecked(True)
    for button, label in (("pushButtonNewInfoTable", "Info::Table"), ("pushButtonNewInfoExtractor", "Info::Extractor")):
        for k in (1, 2):  # the folder made by the first table; the second finds it
            name = "%s-%d" % (label.split("::")[1].lower(), k)
            changeFolder(rootFolder(), True)
            click_new(button, name)
            current, items = shown()
            assert folder_of(name) == [FOLDER] and activeFolder().name() == FOLDER, (name, folder_of(name))
            assert current == FOLDER and items.get(name) == label, (name, current, items)
finally:
    shutil.rmtree(work)
