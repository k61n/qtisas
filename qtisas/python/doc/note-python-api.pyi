# QtiSAS Note
# n=note("name")|newNote("name")|currentNote()
# currentNote() returns None if active window is not a Note. Use note("name") instead.
# SIP: required params positional-only; named optional params also accept keyword syntax.
text()->str  setText(s)  autoexec()->bool  setAutoexec(on=True)
# Tabs (0-based for add/remove/rename/editorAt):
tabs()->int  addTab()  removeTab(index=-1)  renameTab(index,name)
# Editor access:
currentEditor()->ScriptEdit  tab(int_1based)->ScriptEdit  tab(str_name)->ScriptEdit
editorAt(index_0based)->ScriptEdit  indexOf(editor)->int
# ScriptEdit: execute() print() save()->str saveAs(file="")->str importASCII(file="")->str
#             exportPDF(fileName)  insertFunction(name)
importASCII(file="")->str  saveAs(file="")->str  exportPDF(fileName)
showLineNumbers(on=True)  setFont(QFont)  setTabStopDistance(length)
windowLabel()->str  setWindowLabel(label)  captionPolicy()  setCaptionPolicy(policy)
confirmClose(on)  setHidden()  setNormal()  setMinimized()  setMaximized()
# MDIWindow.CaptionPolicy: Name=0 Label=1 Both=2
