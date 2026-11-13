# QtiSAS Application — global functions auto-imported into every script
# SIP: required params are positional-only; named optional params also accept keyword syntax.

# table("name")/matrix("name")/note("name")/graphWindow("name") all return None if no
# window with that name exists — always check "if x is None:" before use (see class-specific docs).

# Command line: qtisas -x script.py (run, stay open) | -x --quit script.py (exit 0/1) | -X script.py (no GUI, exit 0/1)
#   __file__ = script path; cwd = launch dir; sys.argv == ['']; print() -> QtiSAS console, not terminal
#   globals is QtiSAS's shared module, NOT builtin globals() -> use builtins.globals()
#   sys.executable is QtiSAS itself -> plain Python: os.path.join(sys.base_prefix, "bin/python3"|"python.exe")

# Tables:
table("name")->Table|None  newTable()->Table  newTable("name",rows,cols)->Table
# newTable on an EXISTING name is DESTRUCTIVE, not a safe "reuse" — it resizes to rows/cols AND
#   unconditionally clears() every cell, even if rows/cols already match the existing size. Never
#   assume a table survives a second newTable() call with the same name.
currentTable()->Table
tables()->list  tableNames()->list[str]

# Matrices:
matrix("name")->Matrix|None  newMatrix()->Matrix  newMatrix("name",rows,cols)->Matrix
# newMatrix on an EXISTING name resizes in place (no blanket clear like newTable) — safe/no-op only
#   if rows/cols match the current size; SHRINKING permanently truncates rows/cols beyond the new size.
currentMatrix()->Matrix
matrices()->list  matrixNames()->list[str]

# 2D Graphs:
graphWindow("name")->MultiLayer|None  currentGraph()->MultiLayer  graphs()->list
newGraph("name"="Graph1",layers=1,rows=1,cols=1)->MultiLayer
# newGraph on an EXISTING name IS safe by default (layers=1 returns it completely untouched) —
#   unlike newTable/newMatrix above. Only rearranges the layer grid (never curve/plot data) if
#   called with layers>1 and a different row/col grid than the existing graph already has.
#   Blank graph only — do NOT use with plot()
# plot() ALWAYS creates its own new graph window with auto-generated name — do NOT call newGraph() before plot()
# For a NAMED graph use newGraph("name") + addCurve(), NOT plot():
#   g = newGraph("Template"); g.activeLayer().addCurve(t, "y", Graph.CurveType.LineSymbols)
plot(table,("x","y"),style=0)->MultiLayer   # style: Line=0 Scatter=1 LineSymbols=2 VerticalBars=3 HorizontalBars=10
plot(table,"col",style=0)->MultiLayer
plot(matrix,Graph.CurveType.ColorMap)->MultiLayer   # ColorMap=16 GrayScale=17 Contour=18
plot("f(x)",xmin,xmax,points=100)->MultiLayer
# GraphWindow has no replot() — use g.activeLayer().replot()
# to add curves to existing graph: g.activeLayer().addCurve(t,"y",0)  ("y" is bare col name)
waterfallPlot(table,("col1","col2"))->MultiLayer|None  plotImageProfiles(matrix)->MultiLayer
# waterfallPlot returns None (no dialog) if the column tuple is empty
# stemPlot(table,"col",power=1001,startRow=0,endRow=-1)->str
saveGraphAsProject(graph,path,name,compress=False,format="",resolution=-1)
# format: "PNG" "PDF" "SVG" "TEX" "PS" "EPS" "Archive Graph"; "" uses global setting

# 3D Plots:
# PREFER newPlot3D(name, ...) over plot3D(...)+setWindowName(g,name) — plot3D(...)'s creating
#   forms always make a NEW window with no name param, so a separate setWindowName() raises
#   ValueError: name already exists on any re-run with a taken name, AND plot3D(...) already
#   created another orphaned window before that error even fired (only the rename fails, not the
#   creation). newPlot3D(name, ...) checks for the existing window FIRST — updates it in place if
#   found, creates it fresh (already named) otherwise — never leaks an orphaned window before
#   failing. If name belongs to a NON-Graph3D window instead (names are unique across ALL window
#   types), raises ValueError: "newPlot3D: name '<name>' already exists but is not a Graph3D
#   window" immediately, before creating anything.
plot3D("name")->Graph3D  newPlot3D("name"="")->Graph3D
newPlot3D(name,table,"zCol",style=0)->Graph3D   # recommended, same style codes as below
plot3D(table,"zCol",style=0)->Graph3D   # style: 0=ribbons 1=bars 2=cones
newPlot3D(name,matrix,style=5)->Graph3D         # recommended, same style codes as below
plot3D(matrix,style=5)->Graph3D         # style: 5=no-mesh surface
newPlot3D(name,"z(x,y)",xl,xr,yl,yr,zl,zr,cols=40,rows=40)->Graph3D  # recommended
plot3D("z(x,y)",xl,xr,yl,yr,zl,zr,cols=40,rows=40)->Graph3D
plot3D("fx","fy","fz",umin,umax,vmin,vmax,cols=40,rows=40,openU=True,openV=True)->Graph3D
#   g3 = newPlot3D("Surface3D", "sin(x)*cos(y)", -pi,pi,-pi,pi,-1,1,60,60)
#   Confirmed live: after a setWindowName() collision, a retry tried graphWindow(name) instead —
#   that's 2D-GraphWindow-ONLY, always None for a Graph3D name -> AttributeError next line.

# Notes:
note("name")->Note|None  newNote("name"="")->Note
# newNote on an EXISTING name IS a genuinely safe reuse (like newGraph, unlike newTable/newMatrix
#   above) — returns the existing Note completely untouched, no resize, no clear.
currentNote()->Note  notes()->list

# Folders:
activeFolder()->Folder  rootFolder()->Folder
addFolder("name",parent=None)->Folder  deleteFolder(folder,ask=False)->bool
changeFolder(folder,force=False)->bool  copyFolder(src,dest)->bool
saveFolder(folder,"path",compress=False)  appendProject("path",parent=None)->Folder

# Project / window lifecycle:
newProject(save=None)->bool  closeProject(save=None)->bool  openProject("path",save=None)->bool
saveProject(compress=False)->bool  saveProjectAs("path",compress=False)
# save: None asks to save unsaved changes (a dialog; Cancel keeps the project and returns False), False discards
#   them, True saves first (ValueError if the project has no file yet: saveProjectAs(path) first).
# All three stay in this window: a running script keeps its application. newProject = an empty project here.
# openProject: closes the current project, then opens a .qti/.qti.gz/.qti.svg file; ValueError if it is missing or
#   not a project. appendProject() instead ADDS a file's windows to the current project as a sub-folder.
# NameError for openProject: an older qtisasrc.py in the python config folder — use qti.app.openProject(...)
existWindow("name")->bool  existTable("name")->bool  existNote("name")->bool
existMatrix("name")->bool  existGraph("name")->bool   # each type-specific: a matrix is never a table
existPlot3D("name")->bool   # a 3D plot (Graph3D); existGraph() is 2D only
# A fit function: Compiler.functionExists("ai/Quad") (also Fittable.functionExists) — no exist…() for functions
closeWindow(window)  activateWindow(window)  activateWindow("name")->bool
# closeWindow deletes outright, no "hide or delete?" dialog (unlike the UI close button)
# closeWindow(w) IS "close and delete": it deletes the window outright — no separate delete call exists
#   (deleteFolder() is for project folders, never for a window); closeWindow(None) does nothing — no existence check needed
# closeWindow(t) leaves the Python variable t bound — still not None: check by name afterwards,
#   existTable("T") / table("T") is None (existNote/note, existMatrix/matrix, existGraph/graphWindow)
# "whether X still exists": print existTable("X") itself — True = it still exists; never not existTable("X")
hideWindow(window)  maximizeWindow(window)  # call activateWindow(window) before maximizeWindow
openTemplate("path")->MdiSubWindow  saveAsTemplate(window,"path")
clone(window)->MdiSubWindow  setWindowName(window,"name")->bool
# setWindowName raises ValueError (no dialog) if name is empty/invalid/already used by another window
# Allowed characters: letters, digits, underscore, hyphen — confirmed against source
#   (ApplicationWindow::setWindowName() strips "-" before checking, and the check itself is \W,
#   which already excludes "_"). The runtime error text ("only letters and digits are allowed") is
#   misleading — "_"/"-" are fine, e.g. "SphereMap_3D" is valid, not just "SphereMap3D". Confirmed
#   live: a script avoided an underscore-suffixed name out of a mistaken belief it'd be rejected
#   too, and settled for a disconnected generic name ("surface") instead of a traceable one.
autoArrangeLayers()  setPreferences(graph)  # graph: Graph layer (NOT GraphWindow); auto-applied at creation
windows()->list  workspace()->QMdiArea

# Conversions:
tableToMatrix(table)->Matrix  tableToMatrixRegularXYZ(table,"zCol")->Matrix
matrixToTable(matrix,mode=Direct)->Table
# ApplicationWindow.MatrixToTableConversion: Direct XYZ YXZ

# Output / display:
scriptPrint(*values,sep=" ")  scriptPrintReplace("text")   # scriptPrint like print(): values joined by spaces
displayInfo("text")  resultsLog()->QTextEdit  infoLineEdit()->QLineEdit
# NOTE: resultsLog() is an ApplicationWindow method — call as qti.app.resultsLog() or just resultsLog() (global).
#   Do NOT call it as c.resultsLog() on a Compiler object — Compiler has no resultsLog().

# Random numbers — math module has NO random(): math.random() raises AttributeError.
#   Use: import random; random.random()   OR   import numpy as np; np.random.normal()
#   For table columns: t.setRandomValues(col)  t.setNormalRandomValues(col,sd=1.0)  (no import needed)

# Colour palette (indexed colours used by setCurveLineColor / setCurveSymbolColor):
# colorNames() -> list[str]   — ordered list of colour names (user-configurable in prefs)
# colorIndex("red") -> int    — palette INDEX (int), NOT a QColor; -1 if not found
# Usage: layer.setCurveLineColor(0, colorIndex("red"))     # ok — int index accepted
# WRONG: QPen(colorIndex("black"))  QBrush(colorIndex("red"))  addErrorBars(...,colorIndex("gray"),...)
#        these expect QColor — use QColor("black") or QColor(r,g,b) instead
colorNames()->list[str]  colorIndex("name")->int

# Symbol palette (indexed symbols used by setCurveSymbolStyle):
# symbolNames() -> list[str]  — ordered list of symbol names (user-configurable in prefs)
# symbolIndex("ellipse") -> int — QwtSymbol::Style value for that name ("no symbol" -> -1); another name: ValueError
# Names: "no symbol" "ellipse" "circle"(alias) "rectangle" "diamond" "triangle" "down triangle"
#        "up triangle" "left triangle" "right triangle" "cross" "diagonal cross"
#        "horizontal line" "vertical line" "star 1" "star 2" "hexagon"; also "square" "rect" "box"
#        (= rectangle), "star" (= star 1), "plus" (= cross), "x" (= diagonal cross), "none" (= no symbol)
# Usage: layer.setCurveSymbolStyle(0, symbolIndex("diamond"))
symbolNames()->list[str]  symbolIndex("name")->int

# Data state:
autoUpdateTableValues()->bool  setAutoUpdateTableValues(on)   # automatic formula recalculation, project-wide (False = off)
columnsList()->list[str]  columnsList(Table.PlotDesignation.Y)->list[str]
# columnsList() with no args defaults to "all columns" internally (C++ Table.All) — but there is
#   no Table.PlotDesignation.All in Python (omitted from the exposed enum); it's reachable only by
#   leaving the argument out, never by naming it explicitly.

# Data import:
importWaveFile()->Table

# Custom actions — stored in customActionsPath() (Linux/macOS: ~/.config/qtisas/python-actions):
# Layout: *.qca in root | scripts/*.py | icons/*.png/svg
# HOW TO CREATE (follow these 3 steps in order):
#   STEP 1 — write the script:
#     path = customActionsPath()
#     with open(path + "/scripts/my_action.py", "w") as f:
#         f.write("# your QtiSAS Python code here\n")
#   STEP 2 — (optional) provide an icon:
#     copy an image file to customActionsPath() + "/icons/my_action.png"
#   STEP 3 — register the action:
#     addCustomAction("My Action", "my_action.py",
#                     menuName="scriptingMenu",
#                     iconName="my_action.png",   # omit if no icon
#                     shortcut="Ctrl+Shift+M",    # omit if no shortcut
#                     tooltip="What this action does")
#   IMPORTANT: scriptName and iconName are bare filenames only — NOT full paths.
# TO UPDATE AN EXISTING ACTION:
#   updateCustomActionName("My Action", "New Name")
#   updateCustomActionScript("My Action", "new_script.py")
#   updateCustomActionIcon("My Action", "new_icon.png")   # "" clears icon
#   updateCustomActionMenu("My Action", "analysisMenu")
addCustomAction(name,scriptName,menuName="",iconName="",shortcut="",tooltip="")->bool
updateCustomActionName(name,newName)->bool   updateCustomActionScript(name,newScriptName)->bool
updateCustomActionIcon(name,newIconName)->bool   updateCustomActionMenu(name,newMenuName)->bool
removeCustomAction(name)->bool   runCustomAction(name)->bool
customActionNames()->list[str]   customActionsPath()->str

# Module widget singletons:
Dan  Fittable  Compiler
