# QtiSAS Matrix
# m=matrix("name")|newMatrix("name",rows,cols)|currentMatrix()
# matrix("name") returns None (confirmed) if no matrix with that name exists — ALWAYS
# check "if m is None:" before use; m.cell(...) on None raises AttributeError.
# IMPORTANT: Matrix uses (row,col) — ROW first — OPPOSITE of Table which uses (col,row)!
# ViewType:         Matrix.ViewType.TableView=0 ImageView=1
# HeaderViewType:   Matrix.HeaderViewType.ColumnRow=0 XY=1
# ResamplingMethod: Matrix.ResamplingMethod.Bilinear=0 Bicubic=1

numRows()->int  numCols()->int  setNumRows(n)  setNumCols(n)  setDimensions(rows,cols)
cell(row,col)->float      # m.cell(2,3) → row 2, col 3  (ROW first — opposite of Table!)
setCell(row,col,value)    # m.setCell(2,3,1.5)
text(row,col)->str  setText(row,col,value)
# cell()/setCell()/text()/setText() raise ValueError ("There's no row/column N in matrix X!")
#   on an out-of-range row or col — same mechanism as Table's equivalent methods.
setCoordinates(xStart,xEnd,yStart,yEnd)   # m.setCoordinates(-3.14,3.14,-3.14,3.14)
xStart()->float  xEnd()->float  yStart()->float  yEnd()->float  dx()->float  dy()->float

# Formula — vars: i=row(1-based) j=col(1-based) x y. Use pow(x,n) not ^; ln(x) natural log.
setFormula(expr)          # m.setFormula("sin(x)*cos(y)")
calculate(startRow=0,endRow=-1,startCol=0,endCol=-1,forceMuParser=False)->bool   # m.calculate()
# calculate() has NO error handling — an empty/malformed formula does NOT raise (unlike
#   Table.recalculate(), which raises ValueError on a bad formula); it just silently returns
#   False. Always check the return value if the formula came from anywhere less than certain.
setNumericPrecision(prec)

transpose()  invert()->bool  determinant()->float  integrate()->float
# invert()/determinant() raise ValueError if the matrix is not square
flipVertically()  flipHorizontally()  rotate90(clockwise=True)  smooth()
resample(rows,cols,Matrix.ResamplingMethod.Bilinear)

setViewType(Matrix.ViewType.ImageView)  setHeaderViewType(Matrix.HeaderViewType.XY)
setGrayScale()  setRainbowColorMap()  setDefaultColorMap()
colorMap()->LinearColorMap  setColorMap(lc)  resetView()
image()->QImage  importImage(fileName)   # load an image FILE directly as matrix data

importASCII(file,sep="\t",ignoredLines=0,stripSpaces=False,simplifySpaces=False,commentString="#",mode=2)
# mode: 0=NewColumns 1=NewRows 2=Overwrite
exportASCII(file,sep="\t",exportSelection=False)->bool
# raises ValueError if the file cannot be opened for writing (bad path/permissions)
exportRasterImage(fileName,quality=100,dpi=0,compression=0)
export(file)  exportVector(fileName,res=0,color=True)
# export() raises ValueError if file is empty or the extension is an unhandled format

# Quick example:
# m = newMatrix("Z", 64, 64)
# m.setCoordinates(-3.14, 3.14, -3.14, 3.14)
# m.setFormula("sin(x)*cos(y)")
# m.calculate()
# plot(m, Graph.CurveType.ColorMap)
