# QtiSAS Application — global functions auto-imported into every script
# SIP: required params are positional-only; named optional params also accept keyword syntax.

# Tables:
table("name")->Table  newTable()->Table  newTable("name",rows,cols)->Table  # reuses existing table if name matches
currentTable()->Table
tables()->list  tableNames()->list[str]

# Matrices:
matrix("name")->Matrix  newMatrix()->Matrix  newMatrix("name",rows,cols)->Matrix  # reuses existing matrix if name matches
currentMatrix()->Matrix
matrices()->list  matrixNames()->list[str]

# 2D Graphs:
graphWindow("name")->MultiLayer  currentGraph()->MultiLayer  graphs()->list
newGraph("name"="Graph1",layers=1,rows=1,cols=1)->MultiLayer  # reuses existing graph if name matches; blank graph only — do NOT use with plot()
# plot() ALWAYS creates its own new graph window with auto-generated name — do NOT call newGraph() before plot()
# For a NAMED graph use newGraph("name") + addCurve(), NOT plot():
#   g = newGraph("SANS_Template"); g.activeLayer().addCurve(t, "I", Graph.CurveType.LineSymbols)
plot(table,("x","y"),style=0)->MultiLayer   # style: Line=0 Scatter=1 LineSymbols=2 VerticalBars=3 HorizontalBars=10
plot(table,"col",style=0)->MultiLayer
plot(matrix,Graph.CurveType.ColorMap)->MultiLayer   # ColorMap=16 GrayScale=17 Contour=18
plot("f(x)",xmin,xmax,points=100)->MultiLayer
# GraphWindow has no replot() — use g.activeLayer().replot()
# to add curves to existing graph: g.activeLayer().addCurve(t,"y",0)  ("y" is bare col name)
waterfallPlot(table,("col1","col2"))->MultiLayer  plotImageProfiles(matrix)->MultiLayer
# stemPlot(table,"col",power=1001,startRow=0,endRow=-1)->str
saveGraphAsProject(graph,path,name,compress=False,format="",resolution=-1)
# format: "PNG" "PDF" "SVG" "TEX" "PS" "EPS" "Archive Graph"; "" uses global setting

# 3D Plots:
plot3D("name")->Graph3D  newPlot3D("name"="")->Graph3D
plot3D(table,"zCol",style=0)->Graph3D   # style: 0=ribbons 1=bars 2=cones
plot3D(matrix,style=5)->Graph3D         # style: 5=no-mesh surface
plot3D("z(x,y)",xl,xr,yl,yr,zl,zr,cols=40,rows=40)->Graph3D
plot3D("fx","fy","fz",umin,umax,vmin,vmax,cols=40,rows=40,openU=True,openV=True)->Graph3D

# Notes:
note("name")->Note  newNote("name"="")->Note  # reuses existing note if name matches
currentNote()->Note  notes()->list

# Folders:
activeFolder()->Folder  rootFolder()->Folder
addFolder("name",parent=None)->Folder  deleteFolder(folder)->bool
changeFolder(folder,force=False)->bool  copyFolder(src,dest)->bool
saveFolder(folder,"path",compress=False)  appendProject("path",parent=None)->Folder

# Project / window lifecycle:
newProject()  closeProject()  saveProject(compress=False)->bool  saveProjectAs("path",compress=False)
existWindow("name")->bool  existTable("name")->bool  existNote("name")->bool
closeWindow(window)  activateWindow(window)  activateWindow("name")->bool
hideWindow(window)  maximizeWindow(window)  # call activateWindow(window) before maximizeWindow
openTemplate("path")->MdiSubWindow  saveAsTemplate(window,"path")
clone(window)->MdiSubWindow  setWindowName(window,"name")
autoArrangeLayers()  setPreferences(graph)  # graph: Graph layer (NOT GraphWindow); auto-applied at creation
windows()->list  workspace()->QMdiArea

# Conversions:
tableToMatrix(table)->Matrix  tableToMatrixRegularXYZ(table,"zCol")->Matrix
matrixToTable(matrix,mode=Direct)->Table
# ApplicationWindow.MatrixToTableConversion: Direct XYZ YXZ

# Output / display:
scriptPrint("text")  scriptPrintReplace("text")
displayInfo("text")  resultsLog()->QTextEdit  infoLineEdit()->QLineEdit

# Colour palette (indexed colours used by setCurveLineColor / setCurveSymbolColor):
# colorNames() -> list[str]   — ordered list of colour names (user-configurable in prefs)
# colorIndex("red") -> int    — palette INDEX (int), NOT a QColor; -1 if not found
# Usage: layer.setCurveLineColor(0, colorIndex("red"))     # ok — int index accepted
# WRONG: QPen(colorIndex("black"))  QBrush(colorIndex("red"))  addErrorBars(...,colorIndex("gray"),...)
#        these expect QColor — use QColor("black") or QColor(r,g,b) instead
colorNames()->list[str]  colorIndex("name")->int

# Symbol palette (indexed symbols used by setCurveSymbolStyle):
# symbolNames() -> list[str]  — ordered list of symbol names (user-configurable in prefs)
# symbolIndex("ellipse") -> int — QwtSymbol::Style value for that name (-1 = no symbol)
# Names: "no symbol" "ellipse" "circle"(alias) "rectangle" "diamond" "triangle" "down triangle"
#        "up triangle" "left triangle" "right triangle" "cross" "diagonal cross"
#        "horizontal line" "vertical line" "star 1" "star 2" "hexagon"
# Usage: layer.setCurveSymbolStyle(0, symbolIndex("diamond"))
symbolNames()->list[str]  symbolIndex("name")->int

# Data state:
autoUpdateTableValues()->bool  setAutoUpdateTableValues(on)
columnsList()->list[str]  columnsList(Table.PlotDesignation.Y)->list[str]

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
