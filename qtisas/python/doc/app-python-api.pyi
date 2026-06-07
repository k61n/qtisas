# QtiSAS Application — global functions auto-imported into every script
# SIP: required params are positional-only; named optional params also accept keyword syntax.

# Tables:
table("name")->Table  newTable()->Table  newTable("name",rows,cols)->Table  currentTable()->Table
tables()->list  tableNames()->list[str]

# Matrices:
matrix("name")->Matrix  newMatrix()->Matrix  newMatrix("name",rows,cols)->Matrix  currentMatrix()->Matrix
matrices()->list  matrixNames()->list[str]

# 2D Graphs:
graphWindow("name")->MultiLayer  currentGraph()->MultiLayer  graphs()->list
newGraph("name"="Graph1",numPanels=1,rows=1,cols=1)->MultiLayer
plot(table,("col1","col2"),style=Graph.CurveType.Line)->MultiLayer  plot(table,"col",style=Graph.CurveType.Line)->MultiLayer
plot(matrix,Graph.CurveType.ColorMap)->MultiLayer  plot(matrix,int)->MultiLayer  plot("f(x)",xmin,xmax,points=100)->MultiLayer
# Table style = Graph.CurveType int: Line=0 Scatter=1 LineSymbols=2 VerticalBars=3 HorizontalBars=10
# Matrix spectrogram style: Graph.CurveType.ColorMap=16 Graph.CurveType.GrayScale=17 Graph.CurveType.Contour=18
waterfallPlot(table,("col1","col2"))->MultiLayer  plotImageProfiles(matrix)->MultiLayer
stemPlot(table,"col",power=1001,r0=0,r1=-1)->str
saveGraphAsProject(graph,path,name,compress=False,format="",resolution=-1)
# format: "PNG" "PDF" "SVG" "TEX" "PS" "EPS" "Archive Graph"; "" uses global setting
# compress=True → .qti.gz; resolution=-1 uses global DPI setting

# 3D Plots:
plot3D("name")->Graph3D  newPlot3D("name"="")->Graph3D
plot3D(table,"zCol",style=0)->Graph3D  # style: 0=ribbons 1=bars 2=cones
plot3D(matrix,style=5)->Graph3D  # style: 5=no-mesh surface
plot3D("z(x,y)",xl,xr,yl,yr,zl,zr,cols=40,rows=40)->Graph3D
plot3D("fx","fy","fz",umin,umax,vmin,vmax,cols=40,rows=40,openU=True,openV=True)->Graph3D

# Notes:
note("name")->Note  newNote("name"="")->Note  currentNote()->Note  notes()->list

# Folders:
activeFolder()->Folder  rootFolder()->Folder
addFolder("name",parent=None)->Folder  deleteFolder(folder)->bool
changeFolder(folder,force=False)->bool  copyFolder(src,dest)->bool
saveFolder(folder,"path",compress=False)  appendProject("path",parent=None)->Folder

# Project / window lifecycle:
newProject()  closeProject()  saveProject(compress=False)->bool  saveProjectAs("path",compress=False)
existWindow("name")->bool  existTable("name")->bool  existNote("name")->bool
closeWindow(window)
activateWindow(window)  activateWindow("name")->bool
hideWindow(window)  maximizeWindow(window)
openTemplate("path")->MdiSubWindow  saveAsTemplate(window,"path")
clone(window)->MdiSubWindow  setWindowName(window,"name")
autoArrangeLayers()  setPreferences(graph)
windows()->list  workspace()->QMdiArea

# Conversions:
tableToMatrix(table)->Matrix  tableToMatrixRegularXYZ(table,"zCol")->Matrix
matrixToTable(matrix,mode=Direct)->Table
# ApplicationWindow.MatrixToTableConversion: Direct XYZ YXZ

# Output / display:
scriptPrint("text")  scriptPrintReplace("text")
displayInfo("text")  resultsLog()->QTextEdit  infoLineEdit()->QLineEdit

# Data state:
autoUpdateTableValues()->bool  setAutoUpdateTableValues(on)
columnsList()->list[str]  columnsList(Table.PlotDesignation.Y)->list[str]

# Data import:
importWaveFile()->Table

# Module widget singletons:
Dan  Fittable  Compiler
