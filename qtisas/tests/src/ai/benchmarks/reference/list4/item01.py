n = newNote("Steps")
n.addTab()
n.addTab()
for i, name in enumerate(["A", "B", "C"]):
    n.renameTab(i, name)  # renameTab() is 0-based, like editorAt()
    n.editorAt(i).setText("print('%s')" % name)
scriptPrint("R1: %d %s %s" % (n.tabs(), n.tab("C").toPlainText(), n.tab("A").toPlainText()))
