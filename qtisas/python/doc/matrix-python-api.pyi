# QtiSAS Matrix
# m=matrix("name")|newMatrix("name",rows,cols)|currentMatrix()
# Cell: (row,col) 1-based — opposite of Table(col,row). SIP: required params positional-only; named optional params also accept keyword syntax.
# ViewType:Matrix.ViewType. TableView=0 ImageView=1
# HeaderViewType:Matrix.HeaderViewType. ColumnRow=0 XY=1
# ResamplingMethod:Matrix.ResamplingMethod. Bilinear=0 Bicubic=1

numRows()->int  numCols()->int  setNumRows(n)  setNumCols(n)  setDimensions(rows,cols)
cell(row,col)->float  setCell(row,col,value)  text(row,col)->str  setText(row,col,value)
setCoordinates(xStart,xEnd,yStart,yEnd)
xStart()->float  xEnd()->float  yStart()->float  yEnd()->float  dx()->float  dy()->float
# Formula vars: i/row(1-based) j/col(1-based) x y. Use pow(x,n) not ^; ln(x) natural log.
setFormula(expr)  calculate(startRow=0,endRow=-1,startCol=0,endCol=-1,forceMuParser=False)
setNumericPrecision(prec)
transpose()  invert()  determinant()->float  integrate()->float
flipVertically()  flipHorizontally()  rotate90(clockwise=True)  smooth()
resample(rows,cols,Matrix.ResamplingMethod.*)
setViewType(Matrix.ViewType.*)  setHeaderViewType(Matrix.HeaderViewType.*)
setGrayScale()  setRainbowColorMap()  setDefaultColorMap()  colorMap()->LinearColorMap
setColorMap(lc)  resetView()
# Import/Export (positional only — parameters are unnamed in the SIP binding):
# importASCII(file,sep,ignoredLines=0,stripSpaces=False,simplifySpaces=False,commentString="#",mode=2)
# exportASCII(file,sep="\t",exportSelection=False)->bool
# exportRasterImage(file,quality=100,dpi=0,compression=0)  export(file)  exportVector(file)
