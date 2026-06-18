# QtiSAS AI Assistant
# Entry: Status bar (Ctrl+Enter=show code, Ctrl+Shift+Enter=auto-run) | Script Window Ask AI | Note Ask AI
# Setup: Settings → AI  (API key, model, base URL, unlimited docs toggle)

# Rules for AI code generation — violations produce runtime errors:

# PLOTTING
# plot() ALWAYS creates its own new graph window — do NOT call newGraph() before plot()
# WRONG: newGraph() then plot() — plot() creates its own window, newGraph() is never needed before it
# plot() takes bare column names: t=table("data"); g=plot(t,("x","y"),style=0)
# importASCII is a Table METHOD not a global: t=newTable("D",100,3); t.importASCII("f.txt"); g=plot(t,("x","y"),style=0)
# GraphWindow (MultiLayer) has NO replot() — use g.activeLayer().replot()
# GraphWindow layers are 1-based: g.layer(1) is first, g.layer(0) is invalid
# addCurve(t,col,style): col is SINGLE Y column str — g.activeLayer().addCurve(t,"y",0)
# addCurves(t,cols_tuple,style): tuple — g.activeLayer().addCurves(t,("y1","y2"),0)

# WIDGET NAMES
# Names are case-sensitive: table("Data") != table("data")
# Use exact name from window title bar
# Lookup vs create: table("n")/newTable("n",rows,cols)  note("n")/newNote("n")
#   matrix("n")/newMatrix("n",rows,cols)  graphWindow("n")/newGraph("n",layers,rows,cols)

# COLUMN ADDRESSING
# Table: col FIRST, row second — both 1-based: t.setCell(col,row,value)  t.cell(col,row)
# Matrix: row FIRST, col second — both 1-based: m.setCell(row,col,value)  m.cell(row,col)
# plot/addCurve: bare column names ("x","y") — NOT full tableName_col names
# Fittable datasets: "tablename_colname" (table "data", col "I" → "data_I")
# colNames() returns full names: t.colNames() → ("MyData_x","MyData_y")

# FITTABLE
# configure()/setFunction() select BY NAME — do NOT pass formula string
#   WRONG: f.configure("a*x+b")   RIGHT: f.configure("Guinier",["data_I"])
# datasets= takes Y-column names NOT table names: ["data_I"] not ["data"]
# setCurveColor(name_or_index): call AFTER configure(), BEFORE fit()/plotResults()
# do NOT import PyQt5/PyQt6 — use f.setCurveColor("blue")
# fit()/simulate()/plotResults()/beforeFit()/afterFit() require stage=="fit"
# simulateCurve() requires stage=="simulate"

# META COMMANDS (type in AI input — answered instantly, no API call)
# ai status     — model, URL, docs mode, history turns, max tokens
# ai model      — active model name
# ai url        — base URL
# history       — list Q&A turns from this session
# clear history — reset conversation context (same as Settings → AI → Clear)

# QT BINDINGS
# macOS/Windows: PyQt6 only. Linux: PyQt5 or PyQt6 depending on Qt version.
# Prefer QtiSAS API over direct Qt calls
# NEVER import QApplication from PyQt5/PyQt6 — it transfers ownership to Python GC and CRASHES the app
