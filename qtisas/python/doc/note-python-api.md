# QtiSAS Note Python API

Python scripting interface for the `Note` class.
Obtain a note via `note("name")`, `newNote()`, or `currentNote()`.

> **Note:** All SIP-wrapped methods accept positional arguments. Named optional parameters also accept keyword syntax; required parameters are always positional.

---

## Quick Start

```python
# create a new note and set its content
n = newNote("MyScript")
n.setText("# my analysis script\nprint('hello')")
n.setAutoexec(True)
```

```python
# access an existing note
n = note("Analysis")
print(n.text())
```

---

## Getting a Note

| Call | Returns | Notes |
|------|---------|-------|
| `note("name")` | `Note` or `None` | look up existing note by object name; returns `None` if no note with that name exists |
| `newNote()` | `Note` | new empty note |
| `newNote("name")` | `Note` | new note with given name — if a note named `"name"` ALREADY exists, returns it completely unmodified (a genuinely safe reuse, unlike `newTable`'s destructive resize+clear) |
| `currentNote()` | `Note` or `None` | active note window; returns `None` if no Note is focused |
| `closeWindow(n)` | — | delete the note — that is "close and delete"; never `deleteFolder()` |

> **Warning:** `currentNote()` returns `None` whenever the active MDI window is not a Note — including while a script is running (focus is on the console). Always prefer `note("name")` to look up a note by its object name.
>
> **Warning:** `note("name")` also returns `None` (confirmed), not an error and not a placeholder object, when no note with that name exists. Always check `if n is None:` before using the result.

---

## Text Content

| Method | Returns | Notes |
|--------|---------|-------|
| `text()` | `str` | get full text of the active editor tab |
| `setText(s)` | — | replace full text in the active editor tab |

```python
n = note("Analysis")
script = n.text()
print(script)

n.setText("# pi, sin, cos, exp etc. are global in QtiSAS\nprint(pi)")
```

---

## Auto-Execute

When `autoexec` is enabled, the note's script is executed automatically when the project FILE is
opened, **or when the scripting environment restarts** — not on note-open or any per-tab action.
For a **multi-tab** note, only the tab that is/was the currently *active* tab gets executed —
scripts in the note's other tabs do NOT run, even though `autoexec` is a note-level (not per-tab)
setting.

A tab's text is stored now and only runs later (autoexec, `execute()`), so nothing checks it when it is
written: use only documented calls in it, and an f-string inside it uses single braces
(`f"{t.numRows()}"`), not `{{...}}` — doubled braces print the literal text.

| Method | Returns | Notes |
|--------|---------|-------|
| `autoexec()` | `bool` | whether auto-execute is on |
| `setAutoexec(on=True)` | — | enable or disable auto-execute |

```python
n = note("Startup")
n.setAutoexec(True)
print(n.autoexec())   # True
```

> **`setAutoexec()`/`autoexec()` are `Note`-level methods only — `ScriptEdit` has no
> autoexec-related methods at all.** Call them on the `Note` object itself (`n.setAutoexec(True)`),
> never on `n.currentEditor()`/`n.tab(...)`/`n.editorAt(...)`. Confirmed live:
> `n.currentEditor().setAutoexec(True)` raised `AttributeError: 'ScriptEdit' object has no
> attribute 'setAutoexec'` — the next retry guessed a second, equally nonexistent method
> (`setAutoRun`) on the same wrong object instead of just moving `setAutoexec()` to `n`.

---

## Tabs

A note can have multiple editor tabs. Tab indices are **0-based**.

| Method | Returns | Notes |
|--------|---------|-------|
| `tabs()` | `int` | number of tabs |
| `addTab()` | — | add a new empty tab |
| `removeTab(index=-1)` | — | remove tab at `index`; default (`-1`) removes the **currently active** tab (Qt convention), NOT the last tab. A note always keeps one tab: removing the last one raises `ValueError` — clear tabs with `while n.tabs() > 1`. |
| `renameTab(index, name)` | — | rename tab at `index` — **0-based**, unlike `tab(index)` below, which is 1-based. |

```python
n = newNote("MultiTab")
n.addTab()
n.addTab()
print(n.tabs())          # 3  (1 initial + 2 added)
n.renameTab(0, "Init")
n.renameTab(1, "Analysis")
n.renameTab(2, "Plot")
n.removeTab(2)           # remove last tab
```

### Tabs — renameTab() is 0-based, tab() is 1-based

A tab's script is written with `n.tab("make").setText(code)` (or `n.tab(1).setText(code)`) and read with
`n.tab("make").toPlainText()` — **`Note` has no `setTabText()`, `setTabScript()` or `tabText()`**: the text belongs
to the tab's editor, not to the note.

A new note has one tab, named **`Tab 1`**; `addTab()` adds `Tab 2`, `Tab 3`, … — there is no tab called `Main`.
A first tab plus a second named tab: `n.tab(1).setText(...)`, `n.addTab()`, `n.renameTab(1, "Calc")`,
`n.tab("Calc").setText(...)` — after a rename, address the tab **by name**: `renameTab(1)` is the SECOND tab, `tab(1)`
the FIRST.

> **`renameTab()` is 0-based; `tab()` is 1-based — do not mix them up.** Confirmed live: a script
> called `n.renameTab(n.tabs(), "Configuration")` right after `addTab()`, treating the tab COUNT as
> if it were a valid 1-based "next index" — but `renameTab()` needs the 0-based index instead
> (`n.tabs() - 1`), so the new tab was never actually renamed. The later `n.tab("Configuration")`
> lookup then silently returned `None`, with no error pointing back at the real cause.

### Tabs — addTab() already activates the new tab

> **`addTab()` already makes the newly added tab the active/current one — no extra step needed.**
> Confirmed against source: `addTab()` internally calls Qt's `setCurrentIndex()` on the tab it just
> created. There is no Python-exposed way to reactivate a DIFFERENT, already-existing tab —
> `setCurrentIndex()`/`setActiveTab()`/`setCurrentTab()` do **not** exist on `Note` (confirmed
> against the SIP bindings: `Note` exposes only `tabs()`/`addTab()`/`removeTab()`/`renameTab()`/
> `indexOf()`/`editorAt()`/`tab()`/`currentEditor()` for tabs — nothing else). Confirmed live: not
> knowing `addTab()` already does this, a script invented `n.setCurrentIndex(...)`, which raises
> `AttributeError: 'Note' object has no attribute 'setCurrentIndex'` — and repeated the same
> invented call across multiple retries, eventually exhausting the retry budget and failing the
> whole request. If the task is "add a tab and make it the auto-exec one," `addTab()` alone
> already satisfies "make it active" — do not add any further step to achieve that.

---

## Editor Access

Each tab has a `ScriptEdit` editor object. Use these to work with a specific tab's content.

| Method | Returns | Notes |
|--------|---------|-------|
| `currentEditor()` | `ScriptEdit` | editor for the currently visible tab |
| `tab(index)` | `ScriptEdit` | editor for tab at `index` (**1-based**) |
| `tab(name)` | `ScriptEdit` | editor for tab matching `name` (case-insensitive) |
| `editorAt(index)` | `ScriptEdit` | editor for tab at `index` (0-based) |
| `indexOf(editor)` | `int` | tab index of the given `ScriptEdit` object (**0-based**, same convention as `editorAt()`) |

```python
n = note("Scripts")
n.tab(3).execute()          # execute tab 3 (1-based)
n.tab("Fit").execute()      # execute tab named "Fit"
```

---

## ScriptEdit Methods

`ScriptEdit` objects are returned by `currentEditor()`, `tab()` and `editorAt()`.
`ScriptEdit` extends `QTextEdit` — QTextEdit methods are available directly.

| Method | Returns | Notes |
|--------|---------|-------|
| `setText(s)` | — | set tab content (inherited from QTextEdit) |
| `toPlainText()` | `str` | get tab content as plain text (inherited from QTextEdit) |
| `execute()` | — | execute all code in this editor tab; raises `RuntimeError("the tab's script failed: <its error>")` when the tab's own script fails — the error is in the tab's text |
| `print()` | — | opens the system print dialog to print the tab on paper — never in a script; the tab's text is `toPlainText()` |
| `insertFunction(name)` | — | insert a function name at the current cursor position |
| `save()` | `str` | save to the previously used file; returns file path — **if this tab has never been saved** (no known file path, e.g. a fresh `newNote()`/`addTab()`), this opens a BLOCKING file-choice dialog instead. Use `saveAs(file)` with an explicit path in automated scripts. |
| `saveAs(file="")` | `str` | save to `file`; opens dialog if empty; returns file path |
| `importASCII(file="")` | `str` | load text from `file`; opens dialog if empty; returns file path |
| `exportPDF(fileName)` | — | export editor content to PDF |

```python
n = note("Scripts")
n.editorAt(0).setText("print('hello')")   # set content of tab 0 (0-based)
n.tab("Fit").execute()                    # run tab named "Fit"
n.tab(2).execute()                        # run second tab (1-based)
n.currentEditor().saveAs("/tmp/script.py")
```

---

## Display Settings

| Method | Notes |
|--------|-------|
| `showLineNumbers(on=True)` | show or hide line-number gutter |
| `setFont(font)` | set editor font (pass a `QFont` object) |
| `setTabStopDistance(length)` | tab width in pixels |

```python
from PyQt6.QtGui import QFont

n = note("Code")
n.showLineNumbers(True)
n.setTabStopDistance(32)

f = QFont("Monospace", 11)
n.setFont(f)
```

---

## Import / Export

### importASCII

Load a text file into the active editor tab.

```python
path = n.importASCII("/path/to/script.py")
print("loaded:", path)
```

If called with no argument (or empty string), a file-open dialog appears.

### saveAs

Save the active editor tab to a text file.

```python
path = n.saveAs("/path/to/script.py")
print("saved:", path)
```

### exportPDF

```python
n.exportPDF("/path/to/note.pdf")
```

---

## Window Properties (inherited from MDIWindow)

| Method | Returns | Notes |
|--------|---------|-------|
| `windowLabel()` | `str` | display label |
| `setWindowLabel(label)` | — | set display label |
| `captionPolicy()` | `CaptionPolicy` | current caption display policy |
| `setCaptionPolicy(policy)` | — | set caption display policy |
| `confirmClose(on)` | — | ask for confirmation before closing |
| `setHidden()` | — | hide the window |
| `setNormal()` | — | restore to normal size |
| `setMinimized()` | — | minimize |
| `setMaximized()` | — | maximize |

### CaptionPolicy Constants

Access via `MDIWindow.CaptionPolicy.<name>`:

| Constant | Meaning |
|----------|---------|
| `MDIWindow.CaptionPolicy.Name` | show object name |
| `MDIWindow.CaptionPolicy.Label` | show window label |
| `MDIWindow.CaptionPolicy.Both` | show name and label |

```python
n = note("Analysis")
n.setWindowLabel("My Analysis Script")
n.setCaptionPolicy(MDIWindow.CaptionPolicy.Both)
```

---

## Examples

### Create a note with multiple tabs

```python
n = newNote("Workflow")
n.renameTab(0, "Setup")

n.addTab()
n.renameTab(1, "Fit")

n.addTab()
n.renameTab(2, "Plot")

n.editorAt(0).importASCII("/project/setup.py")
n.editorAt(1).importASCII("/project/fit.py")
n.editorAt(2).importASCII("/project/plot.py")
```

### Auto-execute startup script

```python
n = newNote("Startup")
n.setText("""
# math functions (exp, sin, cos, sqrt, pi, ...) are global in QtiSAS — no import needed
print("QtiSAS startup complete")
""")
n.setAutoexec(True)
```

### Save and reload a note

```python
n = note("Analysis")

# save current content to file
path = n.saveAs("/project/analysis.py")

# later, reload it
n2 = newNote("Reload")
n2.importASCII(path)
```

### Set editor appearance

```python
from PyQt6.QtGui import QFont

n = note("Code")
n.showLineNumbers(True)
n.setTabStopDistance(28)
n.setFont(QFont("Courier New", 10))
n.setWindowLabel("Analysis Code")
n.setCaptionPolicy(MDIWindow.CaptionPolicy.Label)
```

---

---

## For AI: Common Mistakes

### Rule 1 — `currentNote()` returns None when a script is running

While a script executes, the console window has focus — not any Note. `currentNote()` will return
`None`. Always look up a note by name.

**WRONG:**
```python
n = currentNote()
n.setText("...")   # AttributeError: 'NoneType' has no attribute 'setText'
```

**RIGHT:**
```python
n = note("MyScript")
n.setText("...")
```

### Rule 2 — `tab()` is 1-based; `editorAt()` is 0-based

The two accessors use different index bases. Mixing them writes to the wrong tab.

| Method | Index base | First tab |
|--------|-----------|-----------|
| `tab(int)` | 1-based | `tab(1)` |
| `editorAt(int)` | 0-based | `editorAt(0)` |

Both `tab(1)` and `editorAt(0)` refer to the same (first) tab.

**WRONG:**
```python
n.tab(0).execute()      # 0 is out of range for tab() — no tab 0
n.editorAt(1).execute() # editorAt(1) is the second tab, not the first
```

**RIGHT:**
```python
n.tab(1).execute()      # first tab (1-based)
n.editorAt(0).execute() # first tab (0-based)
```

### Rule 3 — Never `import math` in Note script content

Math functions (`pi`, `sin`, `cos`, `exp`, `sqrt`, …) are global in QtiSAS. Any script content
set via `setText()` that contains `import math` will fail when executed.

**WRONG:**
```python
n.setText("import math\nprint(math.pi)")
```

**RIGHT:**
```python
n.setText("print(pi)")
```
