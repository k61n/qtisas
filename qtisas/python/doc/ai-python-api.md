# QtiSAS AI Assistant

The AI assistant is built into QtiSAS and can generate Python scripts, explain the API,
and help automate data analysis workflows.

---

## For Users: How to Use the AI Agent

---

### Entry points

The AI can be invoked from three places:

---

**1. Status bar input line** — bottom of the main window

| Key | Action |
|-----|--------|
| `Enter` | Run as a direct command / short script |
| `Ctrl+Enter` (`Cmd+Enter` on macOS) | Send to AI — code shown in Scripting Console, **not** run |
| `Ctrl+Shift+Enter` | Send to AI — code **auto-executed** |
| `?` | Show help summary in the console |

---

**2. Script Window** — **Ask AI** button (top-right)

- Sends the **selected text** to the AI; if nothing is selected, sends the entire script.
- Click → generate only; `Ctrl+Shift`+Click → generate and run.

---

**3. Note window** — **Ask AI** button (tab corner)

- Sends the **full content of the current tab** to the AI.
- Click → generate only; `Ctrl+Shift`+Click → generate and run.

---

### Setup

Before using the AI for the first time, configure the provider in **Settings → AI**:
set the API key, model, and base URL.

---

### Modes

**Generate code only** — use when you want to review the script before running it.
The code appears in the Scripting Console; copy it to the Script Window to edit and run.

**Generate and run** — generated code is executed immediately after it arrives.
Use for well-defined, trusted requests.

---

### What the AI can do

- Generate Python scripts using the QtiSAS API (tables, matrices, graphs, fitting).
- Explain any QtiSAS Python function or workflow.
- Create datasets, run fits, and plot results in a single script.
- Batch-process multiple datasets.

### What the AI cannot do

- **Create or compile new fitting functions** — `configure()` selects functions by name from the
  pre-compiled library. Custom function authoring will be available in a future Compiler interface.
- Access files outside the QtiSAS workspace or make network requests.

---

### Tips for better results

- Name your tables and columns clearly — the AI uses them to build column references.
- Mention the fitting function name explicitly (e.g. `"Guinier"`, `"sphere_SANS"`).
- Use `f.fitFunctions()` to list available function names, `f.datasets()` to list Y-columns.

---

## For AI: QtiSAS Python API Rules

The following rules **must** be followed when generating Python code for QtiSAS.
Violations produce runtime errors that are hard for users to diagnose.

---

### Graph3D

**One `plot3D(…)` call = one surface = one window.** A second call opens a new window — there is no multi-surface overlay.

```python
# WRONG — tries to merge two windows; setMatrix(g2) fails (Graph3D ≠ Matrix)
g  = plot3D("X1","Y1","Z1", ul, ur, vl, vr, nu, nv, True, True)
g2 = plot3D("X2","Y2","Z2", ul, ur, vl, vr, nu, nv, True, True)
g.setMatrix(g2)    # TypeError

# CORRECT — one plot3D() call with a formula that covers the whole surface
g = plot3D("X(u,v)", "Y(u,v)", "Z(u,v)", ul, ur, vl, vr, nu, nv, uClosed, vClosed)
```

**muParser formula syntax** — formula strings are **not** Python:

| Rule | Correct | Wrong |
|------|---------|-------|
| Power | `x^2` | `x**2` |
| Conditional | `sign(threshold - v)` | `a ? b : c` or `if(cond, a, b)` |

Available functions: `sin cos tan asin acos atan atan2 exp log log2 log10 sqrt abs sign floor ceil round`

**Shared sub-expression rule** — when X and Y share a radius `r(v)`, define it as a Python string and interpolate into both. Never inline the same expression twice with different trig factors — the expressions diverge when edited:

```python
# WRONG — r(v) written twice; one will be wrong
g = plot3D(f"sqrt(4.0 - v^2)*cos(u)", f"sqrt(4.0 - v^2)*sin(u)", "v", ...)

# CORRECT — r defined once, reused
r  = "sqrt(4.0 - v^2)"
g = plot3D(f"({r})*cos(u)", f"({r})*sin(u)", "v",
           0, 2*pi, -2, 2, 60, 60, False, True)
```

**`setMatrix(matrix)`** takes a `Matrix` object only — **not** another `Graph3D`. Use `matrix("name")` or `currentMatrix()`.

**`setDataColors` and `rescaleColorMap()` are mutually exclusive.** `rescaleColorMap()` reloads the system color map and discards any `setDataColors()` gradient. Call one or the other, never both:

```python
g3.setDataColors(QColor("navy"), QColor("firebrick"))  # custom gradient — no rescaleColorMap()
# OR
g3.rescaleColorMap()                                   # system map — no setDataColors()
```

---

### Plotting

**`plot()` always creates its own new graph window.**
Never call `newGraph()` before `plot()`.

```python
# t = newTable("data", 100, 2); t.setColName(1, "x"); t.setColName(2, "y")
# plot() takes bare column names when the table object is passed

# CORRECT
t = table("data")
g = plot(t, ("x", "y"), style=Graph.CurveType.Line)
setWindowName(g, "MyGraph")
activateWindow(g)
maximizeWindow(g)

# WRONG — newGraph() creates an empty orphan window; plot() ignores it
g = newGraph("MyGraph")
plot(t, ("x", "y"), style=Graph.CurveType.Line)
```

**`GraphWindow` (`MultiLayer`) has no `replot()` method.**
Use `g.activeLayer().replot()`.

```python
g.activeLayer().replot()
g.activeLayer().addCurve(t, "y", Graph.CurveType.Line)  # single Y col str — NOT a tuple; X is auto-detected
```

---

### Widget names

**Widget names are case-sensitive.** `table("Data")` and `table("data")` are different objects.
Always use the exact name as shown in the window title bar.

**Lookup vs create — naming pattern:**

| Look up existing | Create new |
|------------------|------------|
| `table("name")` | `newTable("name", rows, cols)` |
| `note("name")` | `newNote("name")` |
| `matrix("name")` | `newMatrix("name", rows, cols)` |
| `graphWindow("name")` | `newGraph("name", layers, rows, cols)` |

---

### Column names

**When a table object is passed, use bare column names.**
When only a string is used (fittable `datasets=`, `addCurve` without a table object), use `"tablename_colname"`.
For a table named `data` with a column `I`: bare = `"I"`, string-only = `"data_I"`.

```python
t = newTable("data", 100, 2)
t.setColName(1, "q")
t.setColName(2, "I")
t.setColumnRole(1, Table.PlotDesignation.X)
t.setColumnRole(2, Table.PlotDesignation.Y)
# Y-column name for fitting: "data_I"   (tablename_colname)
```

Use `f.datasets()` to list all Y-column names available in the workspace.

---

### Column Addressing

Different objects use **different argument order** for cell access:

| Context | Argument order | Example |
|---------|----------------|---------|
| **Table** cell access | col **first**, row second — both 1-based | `t.setCell(col, row, value)` · `t.cell(col, row)` |
| **Matrix** cell access | row **first**, col second — both 1-based | `m.setCell(row, col, value)` · `m.cell(row, col)` |
| `plot()` / `addCurve()` with table object | bare column names | `"x"`, `"y"` — **not** `"MyData_x"` |
| Fittable `datasets=` (string-only context) | `"tablename_colname"` | table `"data"`, col `"I"` → `"data_I"` |

`colNames()` returns full names with the table prefix: `t.colNames()` → `("MyData_x", "MyData_y")`.

---

### Fittable

**`configure()` takes a function NAME, not a formula string.**

```python
# CORRECT
f = Fittable
f.configure("Guinier", ["data_I"])

# WRONG — raises ValueError
f.configure("a*exp(-x*x*Rg*Rg/3)+bgd", ["data_I"])
```

Use `f.fitFunctions()` to list all available function names.

---

**`datasets` are Y-column names, not table names.**

```python
f.configure("Guinier", ["data_I"])   # correct: column "I" in table "data"
f.configure("Guinier", ["data"])     # WRONG: raises ValueError
```

---

**`setCurveColor()` must be called after `configure()` and before `fit()` / `plotResults()`.**
Do NOT import PyQt5 or PyQt6 to set curve colors — use the API method.

```python
f.configure("Guinier", ["data_I"])
f.setCurveColor("blue")             # after configure(), before fit()
f.fit()
f.plotResults("FitResult")

# WRONG — PyQt5 is not available; PyQt6 would work but is not needed
from PyQt5.QtGui import QColor
g.activeLayer().setCurveLineColor(1, QColor(0, 0, 255))
```

---

**Stage requirements** — these methods raise `RuntimeError` if called in the wrong stage:

| Method | Required stage |
|--------|----------------|
| `fit()`, `simulate()`, `plotResults()`, `beforeFit()`, `afterFit()` | `"fit"` |
| `simulateCurve()` | `"simulate"` |
| `setCurveColor()` | after `configure()` (not `"init"`) |

---

### Meta commands

Type these directly into the AI input — they are answered instantly without an API call:

| Command | Result |
|---|---|
| `ai status` | model, URL, docs mode, history turns, max tokens |
| `ai model` | active model name |
| `ai url` | base URL |
| `history` | list Q&A turns from this session |
| `clear history` | reset conversation context |

---

### Qt bindings

QtiSAS embeds **PyQt6** on macOS and Windows. On Linux, the build may use **PyQt5** or **PyQt6**
depending on the Qt version installed — do not assume either.
Prefer the QtiSAS API over direct Qt calls whenever possible.
