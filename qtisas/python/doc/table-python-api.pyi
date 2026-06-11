# QtiSAS Table
# t=table("name")|newTable()|newTable("name",rows,cols)|currentTable()
# Column: int(1-based) or str; Row: int(1-based). Avoid "_" in labels.
# PlotDesignation: Table.PlotDesignation. None_=0 X=1 Y=2 Z=3 xErr=4 yErr=5 Label=6
# NumericFormat:   Table.NumericFormat. Default=0 Decimal=1 Scientific=2

# Size:
numRows()->int  numCols()->int  len(t)->int
setNumRows(n)  setNumCols(n)  addRow()->int  addColumn(pd=Y)  removeCol(col)
deleteRows(start,end)  insertColumns(col,n)  swapColumns(col1,col2)

# Column names:
colName(col)->str  colNames()->tuple  setColName(col,name,updateTableDesignations=False)  setColNames([...])

# Read:
cell(col,row)->float  text(col,row)->str  cellData(col,row)->str|float
colData(col)->list  rowData(row)->tuple
# t[n]→rowData(n+1) 0-based negative-ok;  t[a:b]→list of tuples

# Write:
setCell(col,row,value)  setText(col,row,value)  setCellData(col,row,value)
setColData(col,iterable,startRow=0)  appendRowData(tuple)->int
setRowData(row,tuple)  # t[row]=tuple

# Column role / format:
setColumnRole(col,Table.PlotDesignation.*)
setColNumericFormat(col,Table.NumericFormat.*,prec,updateCells=True)
setColTextFormat(col)  setColDateFormat(col,fmt,updateCells=True)  setColTimeFormat(col,fmt,updateCells=True)
setColMonthFormat(col,fmt,updateCells=True)  setColDayFormat(col,fmt,updateCells=True)
setColumnWidth(col,px)  hideColumn(col,on=True)  showAllColumns()  setDecimalSeparators(format,omitGroupSeparator=True)
setReadOnlyColumn(col,on=True)  isRowSelected(row,full=False)->bool  isColSelected(col,full=False)->bool
firstSelectedColumn()->int  numSelectedRows()->int  selectedColumn()->int  setSelectedCol(col)

# Comments:
comment(col)->str  setComment(col,text)  showComments(on=True)

# Formula:
setCommand(col,expr)  recalculate(col,startRow=1,endRow=-1)->bool

# Sort:
sort(type=0,order=0,leadCol="")  # type:0=single 1=grouped; order:0=asc 1=desc
sortColumn(col,order=0)  sortColumns(cols_tuple,type=0,order=0,leadCol="")

# Random/normalize:
setRandomValues(col,startRow=1,endRow=-1)
setNormalRandomValues(col,startRow=1,endRow=-1,sd=1.0)
normalize(col)  normalize()

importASCII(file,sep="\t",ignoredLines=0,renameCols=False,stripSpaces=True,simplifySpaces=False,importComments=False,commentString="#",readOnly=False,mode=2)
# mode: 0=NewColumns 1=NewRows 2=Overwrite
exportASCII(fileName,sep="\t",exportSelection=False,exportLabels=False,exportComments=False)->bool
notifyChanges()  scrollToCell(col,row)
