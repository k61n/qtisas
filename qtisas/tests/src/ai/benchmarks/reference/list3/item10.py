n = newNote("Report")
n.editorAt(0).setText("print('A')")
n.addTab()
n.renameTab(1, "Calc")  # renameTab() is 0-based
n.editorAt(1).setText("print(6*7)")
scriptPrint("R10: %d %s" % (n.tabs(), n.tab("Calc").toPlainText()))
