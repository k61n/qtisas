# QtiSAS Note
# n=note("name")|newNote("name")|currentNote()
# currentNote() returns None if active window is not a Note — use note("name") instead.
text()->str  setText(s)  autoexec()->bool  setAutoexec(on=True)

# Tabs — tab(int) is 1-based; editorAt(int) is 0-based:
tabs()->int  addTab()  removeTab(index=-1)  renameTab(index,name)
currentEditor()->ScriptEdit
tab(int_1based)->ScriptEdit    # n.tab(1) → first tab  (1-based)
tab(str_name)->ScriptEdit      # n.tab("Main") → tab by name
editorAt(index_0based)->ScriptEdit   # n.editorAt(0) → same as tab(1)  (0-based)
indexOf(editor)->int

# ScriptEdit methods (call on currentEditor() or tab()):
#   execute()  print()  save()->str  saveAs(file="")->str
#   importASCII(file="")->str  exportPDF(fileName)  insertFunction(name)
# e.g.: n.currentEditor().execute()

importASCII(file="")->str  saveAs(file="")->str  exportPDF(fileName)
showLineNumbers(on=True)  setFont(QFont)  setTabStopDistance(length)
windowLabel()->str  setWindowLabel(label)  captionPolicy()  setCaptionPolicy(policy)
confirmClose(on)  setHidden()  setNormal()  setMinimized()  setMaximized()
# MDIWindow.CaptionPolicy: Name=0 Label=1 Both=2
