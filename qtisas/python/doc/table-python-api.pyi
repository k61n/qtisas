# QtiSAS Table
# t=table("name")|newTable("name",rows,cols)|currentTable()
# table("name") returns None (confirmed) if no table with that name exists — ALWAYS
# check "if t is None:" before use; t.cell(...) on None raises AttributeError.
# Column: int(1-based) or str label; Row: int(1-based). Avoid "_" in labels.
# PlotDesignation: Table.PlotDesignation. None_=0 X=1 Y=2 Z=3 xErr=4 yErr=5 Label=6
# NumericFormat:   Table.NumericFormat. Default=0 Decimal=1 Scientific=2

# Size:
numRows()->int  numCols()->int  len(t)->int
setNumRows(n)  setNumCols(n)  addRow()->int  addColumn(pd=Y)  removeCol(col)
deleteRows(start,end)  insertColumns(col,n)  swapColumns(col1,col2)

# Column names:
colName(col)->str         # t.colName(2) → "y"  (short label only)
colNames()->tuple         # t.colNames() → ("MyData_x","MyData_y")  full "tableName_label" — used for Fittable datasets
setColName(col,name,updateTableDesignations=False)
setColNames(["x","y","dy"])   # list length must equal numCols()

# Read — col FIRST, row second — both 1-based:
cell(col,row)->float      # t.cell(2,3) → col 2, row 3
text(col,row)->str        # t.text(1,5) → col 1, row 5
cellData(col,row)->str|float
colData(col)->list        # t.colData(1) → [0.1, 0.2, ...]  all rows of col 1
rowData(row)->tuple       # t[n]→rowData(n+1) 0-based, negative ok;  t[a:b]→list of tuples

# Write — col FIRST, row second — both 1-based:
setCell(col,row,value)    # t.setCell(2,3,1.5) → col 2, row 3
setText(col,row,value)
setCellData(col,row,value)
setColData(col,iterable,offset=0)   # offset 0-based: t.setColData(1,[0.1,0.2,0.3])
appendRowData(tuple)->int
setRowData(row,tuple)     # t[row]=tuple also works

# Column role / format:
setColumnRole(col,Table.PlotDesignation.X)   # col can be int or str label
setColNumericFormat(col,Table.NumericFormat.Scientific,prec=4,updateCells=True)
setColTextFormat(col)
setColDateFormat(col,fmt,updateCells=True)   # fmt e.g. "yyyy-MM-dd"
setColTimeFormat(col,fmt,updateCells=True)   # fmt e.g. "hh:mm:ss"
setColMonthFormat(col,fmt,updateCells=True)  setColDayFormat(col,fmt,updateCells=True)
setColumnWidth(col,px)  hideColumn(col,on=True)  showAllColumns()
setDecimalSeparators(format,omitGroupSeparator=True)   # 0=system 1=English(.) 2=German(,)
setReadOnlyColumn(col,on=True)
isRowSelected(row,full=False)->bool  isColSelected(col,full=False)->bool
firstSelectedColumn()->int  numSelectedRows()->int  selectedColumn()->int  setSelectedCol(col)

# Comments:
comment(col)->str  setComment(col,text)  showComments(on=True)

# Formula:
setCommand(col,expr)      # t.setCommand("y2","pow(col(\"y\"),2)")
recalculate(col,startRow=1,endRow=-1)->bool

# Sort:
sort(type=0,order=0,leadCol="")   # type:0=single 1=grouped; order:0=asc 1=desc
sortColumn(col,order=0)  sortColumns(cols_tuple,type=0,order=0,leadCol="")

# Random / normalize:
setRandomValues(col,startRow=1,endRow=-1)
setNormalRandomValues(col,startRow=1,endRow=-1,sd=1.0)
normalize(col)  normalize()

# Import / Export:
importASCII(file,sep="\t",ignoredLines=0,renameCols=False,stripSpaces=True,
            simplifySpaces=False,importComments=False,commentString="#",readOnly=False,mode=2)
# mode: 0=NewColumns 1=NewRows 2=Overwrite
exportASCII(fileName,sep="\t",exportSelection=False,exportLabels=False,exportComments=False)->bool
notifyChanges()  scrollToCell(col,row)

# Quick example:
# t = newTable("Data", 5, 3)                         # 5 rows, 3 cols
# t.setColNames(["x","y","dy"])
# t.setColumnRole(1, Table.PlotDesignation.X)
# t.setColumnRole(2, Table.PlotDesignation.Y)
# t.setColumnRole(3, Table.PlotDesignation.yErr)
# t.setColData(1, [0.1*i for i in range(1,6)])       # fill col 1 from list
# for i in range(1, 6):
#     t.setCell(2, i, i**2 * 0.01)                   # col=2, row=i
#     t.setCell(3, i, 0.001)
# g = plot(t, ("x","y"), Graph.CurveType.LineSymbols)
