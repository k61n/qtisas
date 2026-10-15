# QtiSAS Table
# t=table("name")|newTable("name",rows,cols)|currentTable()
# table("name") returns None (confirmed) if no table with that name exists — ALWAYS
# check "if t is None:" before use; t.cell(...) on None raises AttributeError.
# Column: int(1-based) or str label; Row: int(1-based). Avoid "_" in labels — setColName() does
#   NOT reject it, but downstream dataset names (Fittable, etc.) are "tablename_label" split at the
#   LAST "_". A table "Data" + label "x_err" gives qualified name "Data_x_err", which splits back
#   to table="Data_x" (doesn't exist) + col="err" (wrong) — a genuine parsing break, not just a
#   readability issue. Table names CAN safely contain "_" (the split is always at the LAST one);
#   labels containing "_" are what actually breaks it.
# PlotDesignation: Table.PlotDesignation. None_=0 X=1 Y=2 Z=3 xErr=4 yErr=5 Label=6
# NumericFormat:   Table.NumericFormat. Default=0 Decimal=1 Scientific=2

# Size:
numRows()->int  numCols()->int  len(t)->int
setNumRows(n)  setNumCols(n)  addRow()->int  addColumn(pd=Y)  removeCol(col)
deleteRows(start,end)  insertColumns(col,n)  swapColumns(col1,col2)
# deleteRows / del t[...] raise ValueError if the table has any read-only column (whole op aborted)
#   — INCONSISTENT with removeCol()/normalize() below, which silently SKIP read-only columns
#   instead for the same underlying condition. Don't assume one behavior implies the other.

# Column names:
colName(col)->str         # t.colName(2) → "y"  (short label only)
colNames()->tuple         # t.colNames() → ("x","y")  BARE labels only — NEVER "tableName_label"
# IMPORTANT: colNames()/colName() as exposed to Python BOTH return the bare label — confirmed
#   against source, the SIP bindings call colLabel(), not the differently-named C++ colName(int)
#   that DOES prefix with the table name (that one is never invoked from Python at all). The
#   "tableName_label" form is a separate, Fittable-only convention (configure()/datasets()/
#   setWeightingDataset()) — build it yourself as t.objectName()+"_"+label, never expect it FROM
#   colNames(). Checking for it inside colNames() ALWAYS fails, even when the column exists.
#   WRONG: if "MyData_y" not in t.colNames(): ...     # never true — colNames() has no prefix
#   RIGHT: if "y" not in t.colNames(): ...             # check the bare label
#   Confirmed live: this exact wrong check silently skipped an entire comparison+plotting stage,
#   printing "missing required columns" while the columns genuinely existed, no error raised.
setColName(col,name,updateTableDesignations=False)
# raises ValueError if name is empty or already used by another column in this table
setColNames(["x","y","dy"])   # list length must equal numCols()
# newTable() labels its columns "1","2",... — a column the request calls "x"/"y" exists only after
#   t.setColNames([...]); until then col("y"), colData("y") and Fittable's "Peak_y" are not found.
#   Confirmed live: "y (roles X, Y)" without setColNames() → configure(): "Dataset(s) not found: Peak_y".

# Read — col FIRST, row second — both 1-based:
cell(col,row)->float      # t.cell(2,3) → col 2, row 3
cellOrNone(col,row)->float|None  # None if empty or not a number — use when a parse failure must be visible
text(col,row)->str        # displayed text; Numeric cells follow the numerical format ("1,5") — use cell() for numbers
cellData(col,row)->str|float|None  # None for an EMPTY cell (Numeric/Date/Time) — test "is None", never == ""; or cellOrNone()
colData(col)->list        # t.colData(1) → [0.1, 0.2, ...]  all rows of col 1
rowData(row)->tuple       # t[n]→rowData(n+1) 0-based, negative ok;  t[a:b]→list of tuples
# cell()/text()/cellData()/setCell() do NOT support negative indices (only t[n]/t[a:b] do):
#   WRONG: t.cell(1, -1)   ← ValueError: There's no row -1 in table <name>!
#   RIGHT: t.cell(1, t.numRows())   or   t[-1]  (bracket access supports -1)
# cell() SILENTLY returns 0.0 when the cell's text can't be parsed as a number — never raises for
#   this case (only a genuinely EMPTY cell raises), indistinguishable from a real 0.0 result. Never
#   use cell() on a column you haven't confirmed is numeric — use cellOrNone() (None on failure);
#   never float(t.text(...)): Numeric cells follow the numerical format.
#   (text() had a Python-3-porting TypeError bug on every non-empty cell; fixed at the source as of
#   this build.)

# Write — col FIRST, row second — both 1-based:
setCell(col,row,value)    # t.setCell(2,3,1.5) → col 2, row 3
setText(col,row,value)    # Numeric column: a number as text ("1.5", or "2,5" with German separators) is stored as that number — NO text format needed; ANY other text is evaluated as an expression ("2*pi") and leaves the cell EMPTY if it is none — setColTextFormat(col) first for labels (words)
setCellData(col,row,value)  # ONE cell: setCellData(3, 1, 0.05) — a whole column from a list is setColData(3, [0.05]*21)
#   writes by the column's type: a str into a Numeric column raises TypeError — setColTextFormat(col) first
setColData(col,iterable,offset=0)   # offset 0-based: t.setColData(1,[0.1,0.2,0.3])
appendRowData(tuple)->int
setRowData(row,tuple)     # row is 1-based
# t[row]=tuple ALSO works, but is 0-BASED (negative ok) — same convention as t[n]/t[a:b] reads
#   above, NOT the same as setRowData()'s own 1-based `row` right next to it: t[0]=(...) writes
#   row 1; t[-1]=(...) writes the LAST row. Verified in source (__setitem__ calls setRowData(idx+1,...)).

# Column role / format:
setColumnRole(col,Table.PlotDesignation.X)   # col can be int or str label
# Columns start as NumericFormat.Default with 13 significant digits: 3 -> 3, 3.000001 -> 3.000001, 1234.5 -> 1234.5.
# Change the format only when the request asks for one (a format or a number of decimals).
setColNumericFormat(col,Table.NumericFormat.Scientific,4,updateCells=True)   # prec positional (prec=4 -> TypeError)
#   prec = significant digits for Default, digits after the point for Decimal/Scientific; reset: (col,Default,13)
setColTextFormat(col)
setColDateFormat(col,fmt,updateCells=True)   # fmt e.g. "yyyy-MM-dd"
setColTimeFormat(col,fmt,updateCells=True)   # fmt e.g. "hh:mm:ss"
setColMonthFormat(col,fmt,updateCells=True)  setColDayFormat(col,fmt,updateCells=True)
setColumnWidth(col,px)  hideColumn(col,on=True)  showAllColumns()
setDecimalSeparators(format,omitGroupSeparator=True)   # 0=system 1=English(.) 2=German(,) 3=French(,)
setReadOnlyColumn(col,on=True)
isRowSelected(row,full=False)->bool  isColSelected(col,full=False)->bool
firstSelectedColumn()->int  numSelectedRows()->int  selectedColumn()->int  setSelectedCol(col)

# Comments:
comment(col)->str  setComment(col,text)  showComments(on=True)

# Formula:
setCommand(col,expr)      # t.setCommand("y2","pow(col(\"y\"),2)")
# ONE call for the whole column, outside any loop: the formula refers to col("x"), never to a Python row value
recalculate(col,startRow=1,endRow=-1,forceMuParser=False,notifyChanges=True)->bool
# raises ValueError if the column is read-only or the formula fails; 2nd line: the formula's first error
#   its name: str(e).splitlines()[1].split(":")[0]   → e.g. "ZeroDivisionError" (e is that ValueError)
# after the raise the column is still filled: rows that evaluated hold their values (only failing rows stay
#   empty); a read-only column keeps its old values — count/sum AFTER the try/except, whatever happened
# expr runs in the global language (Python by default); forceMuParser=True -> muParser
# powers: pow(a,b) in both; ^ only muParser, ** only Python. Bare names fail -- always col("x") or col(idx)
# col("1") is the column NAMED "1" — after setColNames() that name is gone; col(1) is column number 1
# The row number in a formula is i (1, 2, ...), in both languages: x = 0, 0.1, ... is "0.1*(i-1)"; no row / row()

# Sort:
sort(type=0,order=0,leadCol="")   # type:0=single 1=grouped; order:0=asc 1=desc
sortColumn(col,order=0)  sortColumns(cols_tuple,type=0,order=0,leadCol="")
# sort/sortColumns raise ValueError if type=1 (grouped) and leadCol is missing/empty/blank column

# Random / normalize:
setRandomValues(col,startRow=1,endRow=-1)
setNormalRandomValues(col,startRow=1,endRow=-1,sd=1.0)
normalize(col)  normalize()
# removeCol()/normalize() silently skip read-only columns (no dialog, no exception) — unlike
#   deleteRows()/del t[...] above, which raise ValueError for the same condition instead.

# Import / Export:
importASCII(file,sep="\t",ignoredLines=0,renameCols=False,stripSpaces=True,
            simplifySpaces=False,importComments=False,commentString="#",readOnly=False,mode=2)
# mode: 0=NewColumns 1=NewRows 2=Overwrite
exportASCII(fileName,sep="\t",exportSelection=False,exportLabels=False,exportComments=False)->bool
# raises ValueError if the file cannot be opened for writing (bad path/permissions)
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
