# QtiSAS Note
# n=note("name")|newNote("name")|currentNote()
# currentNote() returns None if active window is not a Note — use note("name") instead.
# note("name") ALSO returns None (confirmed) if no note with that name exists — ALWAYS
# check "if n is None:" before use.
text()->str  setText(s)  autoexec()->bool  setAutoexec(on=True)
# autoexec fires ONLY when the PROJECT FILE is opened (or the scripting env restarts) — NOT when
#   this note itself is opened/activated, not on any other trigger. Setting it True plants the
#   note to auto-run the next time the project loads — a real, persistent side effect; don't set it
#   unless that's genuinely intended. For a MULTI-TAB note, only the tab that is/was the currently
#   ACTIVE tab gets executed — scripts in the note's other tabs do NOT run, even though autoexec is
#   a note-level (not per-tab) setting.
# setAutoexec()/autoexec() are Note-level ONLY — ScriptEdit has no autoexec-related methods at
#   all. Call on n directly (n.setAutoexec(True)), never on n.currentEditor()/n.tab(...)/
#   n.editorAt(...). Confirmed live: n.currentEditor().setAutoexec(True) raised AttributeError;
#   the next retry guessed a second, equally nonexistent method (setAutoRun) on the same wrong
#   object instead of just moving setAutoexec() to n.

# Tabs — tab(int) is 1-based; editorAt(int) is 0-based; renameTab(index,...) is ALSO 0-based
#   (confirmed against source: passes index straight to Qt's QTabWidget.setTabText) — do not treat
#   it as 1-based like tab(). Confirmed live: renameTab(n.tabs(), "Configuration") right after
#   addTab() used the tab COUNT as if it were a valid 1-based "next index" — the new tab was never
#   actually renamed, so a later tab("Configuration") lookup silently returned None. Use
#   renameTab(n.tabs() - 1, name) instead.
tabs()->int  addTab()  removeTab(index=-1)  renameTab(index,name)
# A new note has ONE tab, named "Tab 1"; addTab() adds "Tab 2", ... — there is no tab "Main".
#   First tab + a second named tab: n.tab(1).setText(...); n.addTab(); n.renameTab(1, "Calc")
# removeTab's default -1 means "the CURRENTLY ACTIVE tab" (Qt convention) — NOT "the last tab"
#   (Python-style negative indexing, which nothing else here uses either). A no-op if only one
#   tab remains (refuses to remove the last one).
# addTab() ALREADY makes the new tab active/current (confirmed against source: it calls Qt's
#   setCurrentIndex() on the tab it just created) — no separate step needed. There is NO
#   Python-exposed way to reactivate a DIFFERENT, already-existing tab: setCurrentIndex()/
#   setActiveTab()/setCurrentTab() do NOT exist on Note (confirmed against the SIP bindings — only
#   tabs()/addTab()/removeTab()/renameTab()/indexOf()/editorAt()/tab()/currentEditor() exist for
#   tabs). Confirmed live: guessing n.setCurrentIndex(...) raised AttributeError repeatedly across
#   retries and eventually failed the whole request. "Add a tab and make it the auto-exec one"
#   needs ONLY addTab() — do not add any further "make it active" step.
currentEditor()->ScriptEdit
tab(int_1based)->ScriptEdit    # n.tab(1) → first tab  (1-based)
tab(str_name)->ScriptEdit      # n.tab("Main") → tab by name
editorAt(index_0based)->ScriptEdit   # n.editorAt(0) → same as tab(1)  (0-based)
indexOf(editor)->int

# ScriptEdit methods (call on currentEditor() or tab()):
#   execute()  print()  save()->str  saveAs(file="")->str
#   toPlainText()->str  setText(text)   # the tab's text — print() is NOT that: it opens the printer dialog
#   importASCII(file="")->str  exportPDF(fileName)  insertFunction(name)
# execute() runs the ENTIRE tab's text (bound to C++ executeAll() internally), NOT just the
#   selection or the line at the cursor — there's a same-named C++ method with that narrower
#   scope, but it isn't exposed to Python. execute() always means "run everything in this tab."
# e.g.: n.currentEditor().execute()

# Note-level shortcuts (genuinely separate bindings, NOT a duplicate of the ScriptEdit list above)
#   — thin delegators to currentEditor(), so n.saveAs(...) == n.currentEditor().saveAs(...):
importASCII(file="")->str  saveAs(file="")->str  exportPDF(fileName)
showLineNumbers(on=True)  setFont(QFont)  setTabStopDistance(length)
windowLabel()->str  setWindowLabel(label)  captionPolicy()  setCaptionPolicy(policy)
confirmClose(on)  setHidden()  setNormal()  setMinimized()  setMaximized()
# MDIWindow.CaptionPolicy: Name=0 Label=1 Both=2
