/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Jonas Bähr <Jonas.Baehr@web.de>
    2006 Knut Franke <knut.franke@gmx.de>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: Specifications for Python3 bindings
 ******************************************************************************/

%Module(name=qti, language="C++", keyword_arguments="Optional")
%Import QtCore/QtCoremod.sip
%Import QtWidgets/QtWidgetsmod.sip

class ScriptEdit: QTextEdit
{
%TypeHeaderCode
#include "ScriptEdit.h"
%End
public slots:
  void executeAll() /PyName=execute/;
  void print();
  void insertFunction(const QString &);
  void exportPDF(const QString& fileName);
  QString save();
  QString exportASCII(const QString &file=QString())/PyName=saveAs/;
  QString importASCII(const QString &file=QString());

private:
  ScriptEdit(ScriptEdit&);
};

class MdiSubWindow: QMdiSubWindow /PyName=MDIWindow/
{
%TypeHeaderCode
#include "MdiSubWindow.h"
%End
public:
  enum CaptionPolicy{Name = 0, Label = 1, Both = 2};

  SIP_PYOBJECT windowLabel();
%MethodCode
  sipRes = PyUnicode_FromString(sipCpp->windowLabel().toLocal8Bit().constData());
%End
  void setWindowLabel(const QString&);

  CaptionPolicy captionPolicy();
  void setCaptionPolicy(CaptionPolicy);

  virtual void restore(const QStringList& );
  Folder* folder();

  void askOnCloseEvent(bool)/PyName=confirmClose/;

	virtual void setHidden();
	void setNormal();
	void setMinimized();
	void setMaximized();

private:
  MdiSubWindow(const MdiSubWindow&);
};

class Table: MdiSubWindow
{
%TypeHeaderCode
#include "Table.h"
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <datetime.h> // python include
#define CHECK_TABLE_COL(arg)\
    int col;\
    if (PyLong_Check(arg)) {\
      col = (int)PyLong_AsLong(arg) - 1;\
		if (col < 0 || col >= sipCpp->numCols()) {\
			sipIsErr = 1;\
            PyErr_Format(PyExc_ValueError, "There's no column %d in table %s!", col+1, sipCpp->objectName().toLocal8Bit().constData());\
		}\
	 } else {\
      PyObject *tmp = PyObject_Str(arg);\
		if (!tmp) {\
			sipIsErr = 1;\
			PyErr_Format(PyExc_TypeError, "Column argument must be either int or string.");\
		} else {\
			col = sipCpp->colIndex(PyUnicode_AsUTF8(tmp));\
			if (col < 0) {\
				sipIsErr = 1;\
				PyErr_Format(PyExc_ValueError, "There's no column named %s in table %s!", PyUnicode_AsUTF8(tmp),\
                        sipCpp->objectName().toLocal8Bit().constData());\
				Py_DECREF(tmp);\
			}\
		}\
	 }

#define CHECK_TABLE_ROW(arg)\
   int row = arg-1;\
	if (row < 0 || row >= sipCpp->numRows()) {\
		sipIsErr = 1;\
        PyErr_Format(PyExc_ValueError, "There's no row %d in table %s!", row+1, sipCpp->objectName().toLocal8Bit().constData());\
	}

static int setCellDataHelper(Table* table, int row, int col, PyObject* item) {
  const sipTypeDef* sipType_QDateTime = sipFindType("QDateTime");
  const sipTypeDef* sipType_QTime = sipFindType("QTime");
  PyTypeObject* item_type = item->ob_type;
  if (item == Py_None) {
    table->clearCell(row, col);
    return 0;
  }
  switch (table->columnType(col)) {
  case Table::Numeric:
    item = PyNumber_Float(item);
    if (item) {
      table->setCell(row, col, PyFloat_AsDouble(item));
      Py_DECREF(item);
    }
    else {
      if (PyType_Ready(item_type) == 0)
        PyErr_Format(PyExc_TypeError, "Can't convert <%s> into a float!", item_type->tp_name);
      return 1;
    }
    break;
  case Table::Text:
    item = PyObject_Str(item);
    if (item) {
      table->setText(row, col, PyUnicode_AsUTF8(item));
      Py_DECREF(item);
    }
    else {
      if (PyType_Ready(item_type) == 0)
        PyErr_Format(PyExc_TypeError, "Can't convert <%s> into a string!", item_type->tp_name);
      return 1;
    }
    break;
  case Table::Date:
    if (sipCanConvertToType(item, sipType_QDateTime, 0)) {
      int isErr = 0;
      QDateTime* dateTime = reinterpret_cast<QDateTime*>(sipConvertToType(item, sipType_QDateTime,
                            0/*sipTransferObj*/, 0/*flags*/, 0/*state*/, &isErr));
      if (isErr)
        return 1;
      table->setText(row, col, dateTime->toString(table->columnFormat(col)));
      delete dateTime;
    }
    else {
      if (PyType_Ready(item_type) == 0)
        PyErr_Format(PyExc_TypeError, "Can't convert <%s> into a datetime!", item_type->tp_name);
      return 1;
    }
    break;
  case Table::Time:
    if (sipCanConvertToType(item, sipType_QTime, 0)) {
      int isErr = 0;
      QTime* time = reinterpret_cast<QTime*>(sipConvertToType(item, sipType_QTime,
                            0/*sipTransferObj*/, 0/*flags*/, 0/*state*/, &isErr));
      if (isErr)
        return 1;
      table->setText(row, col, time->toString(table->columnFormat(col)));
      delete time;
    }
    else {
      if (PyType_Ready(item_type) == 0)
        PyErr_Format(PyExc_TypeError, "Can't convert <%s> into a time!", item_type->tp_name);
      return 1;
    }
    break;
  case Table::Month:
    item = PyNumber_Long(item);
    if (item) {
      int m = PyLong_AsLong(item) % 12; // no type checking required since we casted above
      if (!m)
        m = 12;
      QString format = table->columnFormat(col);
      Py_DECREF(item);
      if (format == "M")
          table->setText(row, col, QLocale().monthName(m, QLocale::ShortFormat).left(1));
      else if (format == "MMM")
          table->setText(row, col, QLocale().monthName(m, QLocale::ShortFormat));
      else if (format == "MMMM")
          table->setText(row, col, QLocale().monthName(m));
    }
    else {
      if (PyType_Ready(item_type) == 0)
        PyErr_Format(PyExc_TypeError, "Can't convert <%s> into a number!", item_type->tp_name);
      return 1;
    }
    break;
  case Table::Day:
    item = PyNumber_Long(item);
    if (item) {
      int day = PyLong_AsLong(item) % 7; // no type checking required since we casted above
      if (!day)
        day = 7;
      QString format = table->columnFormat(col);
      Py_DECREF(item);
      if (format == "d")
          table->setText(row, col, QLocale().dayName(day, QLocale::ShortFormat).left(1));
      else if (format == "ddd")
          table->setText(row, col, QLocale().dayName(day, QLocale::ShortFormat));
      else if (format == "dddd")
          table->setText(row, col, QLocale().dayName(day));
    }
    else {
      if (PyType_Ready(item_type) == 0)
        PyErr_Format(PyExc_TypeError, "Can't convert <%s> into a number!", item_type->tp_name);
      return 1;
    }
    break;
  default:
    PyErr_Format(PyExc_ValueError, "Type %d of column %d in table %s not supported!",
                 table->columnType(col), col+1, table->name().toLocal8Bit().constData());
    return 1;
    break;
  } // switch column type
  return 0; // everything ok
}

static int cellDataHelper(Table* table, int row, int col, PyObject** item) {
  switch (table->columnType(col)) {
  case Table::Numeric:
    if (table->text(row, col).isEmpty())
      *item = Py_None;
    else
      *item = PyFloat_FromDouble(table->cell(row, col));
    break;
  case Table::Text:
    *item = PyUnicode_FromString(table->text(row, col).toLocal8Bit().constData());
    break;
  case Table::Date:
    if (table->text(row, col).isEmpty())
      *item = Py_None;
    else {
      if (!PyDateTimeAPI)
        PyDateTime_IMPORT;
      QDateTime dateTime = QDateTime::fromString(table->text(row, col), table->columnFormat(col));
      QDate date = dateTime.date();
      QTime time = dateTime.time();
      *item = PyDateTime_FromDateAndTime(date.year(), date.month(), date.day(),
                                         time.hour(), time.minute(), time.second(), time.msec()*1000);
    }
    break;
  case Table::Time:
    if (table->text(row, col).isEmpty())
      *item = Py_None;
    else {
      if (!PyDateTimeAPI)
        PyDateTime_IMPORT;
      QTime time = QTime::fromString(table->text(row, col), table->columnFormat(col));
      *item = PyTime_FromTime(time.hour(), time.minute(), time.second(), time.msec()*1000);
    }
    break;
  case Table::Month:
    if (table->text(row, col).isEmpty())
      *item = Py_None;
    else {
      QDate date = QDate::fromString(table->text(row, col), table->columnFormat(col));
      *item = PyLong_FromLong(date.month());
    }
    break;
  case Table::Day:
    if (table->text(row, col).isEmpty())
      *item = Py_None;
    else {
      QDate date = QDate::fromString(table->text(row, col), table->columnFormat(col));
      *item = PyLong_FromLong(date.dayOfWeek());
    }
    break;
  default:
    *item = nullptr;
    PyErr_Format(PyExc_ValueError, "Type %d of column %d in table %s not supported!",
                 table->columnType(col), col+1, table->name().toLocal8Bit().constData());
    return 1;
    break;
  } // switch column type
  return 0; // everything ok
}
%End

public:
  enum PlotDesignation{None = 0, X = 1, Y = 2, Z = 3, xErr = 4, yErr = 5, Label = 6};
  enum ImportMode{NewColumns, NewRows, Overwrite};
  enum NumericFormat{Default = 0, Decimal = 1, Scientific = 2};

  int numRows();
  Py_ssize_t __len__();
%MethodCode
  sipRes = sipCpp->numRows();
%End
  int numCols();
  void setNumRows(int);
  void setNumCols(int);

  void deleteRows(int, int);
  void __delitem__(int);
%MethodCode
  int len = sipCpp->numRows();
  if ((a0 = (int)sipConvertFromSequenceIndex(a0, len)) < 0)
    sipIsErr = 1;
  else
    sipCpp->deleteRows(a0 + 1, a0 + 1); // start and stop are equal, counting starts at 1
%End
  void __delitem__(SIP_PYSLICE);
%MethodCode
  Py_ssize_t len, start, stop, step, slicelength;
  len = sipCpp->numRows();
  if (PySlice_GetIndicesEx((PyObject*)a0, len, &start, &stop, &step, &slicelength) < 0)
    sipIsErr = 1;
  else {
    start += 1; // row index starts at 1 (python at 0); the stop is inclusive (python exclusive) so no need to change
    if (step == 1)
      sipCpp->deleteRows(start, stop);
    else {
      for (int i = 0; i < slicelength; i++) {
        sipCpp->deleteRows(start, start);
        start += step - 1; // the "- 1" is needed since the removal also shifts the index
      }
    }
  }
%End

  SIP_PYOBJECT text(SIP_PYOBJECT, int);
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
  if (sipIsErr == 0) {
		PyObject *encstr = PyUnicode_FromString(sipCpp->text(row, col).toLocal8Bit().constData());
		if (encstr) {
			sipRes = PyUnicode_FromEncodedObject(encstr, "utf8", 0);
			Py_DECREF(encstr);
		} else {
			sipRes = nullptr;
			sipIsErr = 1;
		}
	}
%End

    void removeCol(SIP_PYOBJECT);
%MethodCode
    sipIsErr=0;
    CHECK_TABLE_COL(a0);
	if (sipIsErr == 0){
        QStringList list;
		list << sipCpp->colName(col);
        sipCpp->removeCol(list);
    }
%End

  double cell(SIP_PYOBJECT, int);
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
	if (sipIsErr == 0){
		if (sipCpp->text(row, col) != "")
			sipRes = sipCpp->cell(row, col);
		else {
			sipRes = 0;
			PyErr_SetString(PyExc_ValueError, "Empty table cell");
		}
	}
%End
  SIP_PYOBJECT cellData(SIP_PYOBJECT, int);
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
	if (sipIsErr == 0)
    sipIsErr = cellDataHelper(sipCpp, row, col, &sipRes);
%End
  SIP_PYTUPLE rowData(int);
%MethodCode
  PyObject *item;
  sipIsErr = 0;
  CHECK_TABLE_ROW(a0);
	if (sipIsErr == 0){
    int size = sipCpp->numCols();
    sipRes = PyTuple_New(size);
    for(int col = 0; col < size; col++) {
      sipIsErr = cellDataHelper(sipCpp, row, col, &item);
      if (sipIsErr == 0)
        PyTuple_SET_ITEM(sipRes, col, item);
      else {
        Py_DECREF(sipRes);
        sipRes = nullptr;
        break;
      }
    } // for each col
	} // if not sip error
%End
  SIP_PYTUPLE __getitem__(int); // this is operator[]
%MethodCode
  Py_ssize_t idx = sipConvertFromSequenceIndex(a0, sipCpp->numRows());
  if (idx < 0)
    sipIsErr = 1;
  else {
    sipRes = PyObject_CallMethod(sipSelf, "rowData", "i", idx + 1); // rowData's index starts at 1
    if (sipRes == nullptr)
      sipIsErr = 1;
  }
%End
  SIP_PYTUPLE __getitem__(SIP_PYSLICE); // this is operator[]
%MethodCode
  Py_ssize_t len, start, stop, step, slicelength;
  len = sipCpp->numRows();
  if (PySlice_GetIndicesEx((PyObject*)a0, len, &start, &stop, &step, &slicelength) < 0)
    sipIsErr = 1;
  else {
    PyObject *item;
    sipRes = PyList_New(slicelength);
    for(int i = 0; i < slicelength; i++) {
      item = PyObject_CallMethod(sipSelf, "rowData", "i", start + 1); // rowData's index starts at 1
      start += step;
      if (item)
        PyList_SET_ITEM(sipRes, i, item);
      else {
        Py_DECREF(sipRes);
        sipRes = nullptr;
        sipIsErr = 1;
        break;
      }
    } // for
  }
%End
  SIP_PYLIST colData(SIP_PYOBJECT);
%MethodCode
  PyObject *item;
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
	if (sipIsErr == 0){
    int size = sipCpp->numRows();
    sipRes = PyList_New(size);
    for(int row = 0; row < size; row++) {
      sipIsErr = cellDataHelper(sipCpp, row, col, &item);
      if (sipIsErr == 0)
        PyList_SET_ITEM(sipRes, row, item);
      else {
        Py_DECREF(sipRes);
        sipRes = nullptr;
        break;
      }
    } // for
	} // if not sip error
%End
  void setRowData(int, SIP_PYTUPLE);
%MethodCode
  PyObject *item;
  sipIsErr = 0;
  CHECK_TABLE_ROW(a0);
  int n = PyTuple_Size(a1);
  int size = sipCpp->numCols();
  if (n != size) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, "The argument length must match the table's column count!");
  }
	if (sipIsErr == 0){
    for(int col = 0; col < size; col++) {
      item = PyTuple_GET_ITEM(a1, col);
      sipIsErr = setCellDataHelper(sipCpp, row, col, item);
      if (sipIsErr)
        break;
    } // for each col
	} // if not sip error
%End
  void __setitem__(int, SIP_PYTUPLE);
%MethodCode
  Py_ssize_t idx = sipConvertFromSequenceIndex(a0, sipCpp->numRows());
  if (idx < 0)
    sipIsErr = 1;
  else {
    PyObject* methodName = PyUnicode_FromString("setRowData");
    PyObject* rowNumber = PyLong_FromLong(idx + 1); // setRowData's index starts at 1
    PyObject* ret = PyObject_CallMethodObjArgs(sipSelf, methodName, rowNumber, a1, nullptr);
    if (ret == nullptr)
      sipIsErr = 1;
    else
      Py_DECREF(ret);
    Py_DECREF(rowNumber);
    Py_DECREF(methodName);
  }
%End
  int appendRowData(SIP_PYTUPLE);
%MethodCode
  sipRes = sipCpp->numRows() + 1;
  sipCpp->resizeRows(sipRes);
  PyObject* methodName = PyUnicode_FromString("setRowData");
  PyObject* rowNumber = PyLong_FromLong(sipRes);
  PyObject* ret = PyObject_CallMethodObjArgs(sipSelf, methodName, rowNumber, a0, nullptr);
  if (ret == nullptr)
    sipIsErr = 1;
  else
    Py_DECREF(ret);
  Py_DECREF(rowNumber);
  Py_DECREF(methodName);
%End
  void setColData(SIP_PYOBJECT col, SIP_PYOBJECT data, int offset = 0); // offset can be used as a start offset
%MethodCode
  PyObject *item;
  PyObject *iterator = PyObject_GetIter(a1);
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  int tableLength = sipCpp->numRows();
  if (iterator == nullptr) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_TypeError, "Object needs to be iterable");
  }
  else {
    while (a2 < 0) { // a negative offset should skip the first values in the iterator
      item = PyIter_Next(iterator);
      if (item)
        Py_DECREF(item); // not needed any more
      else {
        sipIsErr = 1;
        if (! PyErr_Occurred())
          PyErr_SetString(PyExc_StopIteration, "Iterator ended before any row could be filled!");
      }
      a2++;
    } //while
  }
	if (sipIsErr == 0){
    for(int row = a2; row < tableLength; row++) {
      item = PyIter_Next(iterator);
      if (item) {
        sipIsErr = setCellDataHelper(sipCpp, row, col, item);
        Py_DECREF(item);
      }
      else {
        sipIsErr = 1;
        if (! PyErr_Occurred())
          PyErr_SetString(PyExc_StopIteration, "Iterator ended before all rows were filled!");
      }
      if (sipIsErr)
        break;
    } // for each row
	} // if not sip error
  if (iterator)
    Py_DECREF(iterator);
%End
  void setText(SIP_PYOBJECT, int, const QString&);
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
	if (sipIsErr == 0)
		sipCpp->setText(row, col, *a2);
%End
  void setCell(SIP_PYOBJECT, int, double);
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
	if (sipIsErr == 0)
		sipCpp->setCell(row, col, a2);
%End
  void setCellData(SIP_PYOBJECT, int, SIP_PYOBJECT);
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
	if (sipIsErr == 0)
		sipIsErr = setCellDataHelper(sipCpp, row, col, a2);
%End
  SIP_PYOBJECT colName(int);
%MethodCode
   if (a0 < 1 || a0 > sipCpp->numCols()) {\
		sipIsErr = 1;\
		PyErr_SetString(PyExc_ValueError, "Invalid column argument");\
	} else
		sipRes = PyUnicode_FromString(sipCpp->colLabel(a0-1).toLocal8Bit().constData());
%End
  SIP_PYTUPLE colNames();
%MethodCode
  QStringList l = sipCpp->colNames();
  int size = l.count();
  sipRes = PyTuple_New(size);
  if(sipRes) {
    for(int i=0; i<size; i++)
      PyTuple_SET_ITEM(sipRes, i, PyUnicode_FromString(l.at(i).toLocal8Bit().constData()));
  } else
    sipIsErr = 1;
%End
  void setColName(SIP_PYOBJECT col, const QString& name, bool updateTableDesignations = false);
%MethodCode
	sipIsErr = 0;
   CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setColName(col, *a1, a2);
%End
  void setHeader(QStringList header)/PyName=setColNames/;
  void notifyChanges();

  void importASCII(const QString&, const QString&="\t", int=0, bool=false,
  					bool=true, bool=false, bool=false, const QString&="#", bool=false,
					ImportMode = Overwrite, const QLocale& = QLocale(), int = 0, int = -1, const QList<int>& = QList<int>(), const QStringList& = QStringList());
  bool exportASCII(const QString& fileName, const QString& sep = "\t", bool exportSelection = false, bool exportLabels = false, bool exportComments = false);

  void setDecimalSeparators(int format, bool omitGroupSeparator = true);
%MethodCode
	QLocale locale;
    switch (a0){
        case 0:
            locale = QLocale::system();
        break;
        case 1:
            locale = QLocale::c();
        break;
        case 2:
            locale = QLocale(QLocale::German);
        break;
        case 3:
            locale = QLocale(QLocale::French);
        break;
    }
 	if (a1==true)
		locale.setNumberOptions(QLocale::OmitGroupSeparator);

    sipCpp->updateDecimalSeparators(locale);
%End

  void normalizeCol(SIP_PYOBJECT) /PyName=normalize/;
%MethodCode
	sipIsErr = 0;
	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->normalizeCol(col);
%End
  void normalize();

  void sortColumn(SIP_PYOBJECT, int order = 0);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->sortColumn(col, a1);
%End
  void sort(int type = 0, int order = 0, const QString& leadCol = QString());
  void sortColumns(SIP_PYTUPLE, int=0, int=0, const QString&=QString());
%MethodCode
  QStringList l;
  int n = PyTuple_Size(a0);
  for (int i=0; i<n; i++) {
    PyObject *str = PyObject_Str(PyTuple_GET_ITEM(a0,i));
    if (str) {
      l << PyUnicode_AsUTF8(str);
		Py_DECREF(str);
	 } else {
      sipIsErr = 1;
      break;
    }
  }
  sipCpp->sortColumns(l, a1, a2, *a3);
%End

void setRandomValues(SIP_PYOBJECT, int startRow = 1, int endRow = -1);
%MethodCode
sipIsErr = 0;
CHECK_TABLE_COL(a0);
if (sipIsErr == 0)
	sipCpp->setRandomValues(col, a1 - 1, a2 - 1);
%End

void setNormalRandomValues(SIP_PYOBJECT, int startRow = 1, int endRow = -1, double sd = 1.0);
%MethodCode
sipIsErr = 0;
CHECK_TABLE_COL(a0);
if (sipIsErr == 0)
	sipCpp->setNormalRandomValues(col, a1 - 1, a2 - 1, a3);
%End

	void setCommand(SIP_PYOBJECT, const QString&);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setCommand(col, *a1);
%End
  bool calculate(SIP_PYOBJECT col, int startRow = 1, int endRow = -1, bool forceMuParser = false, bool notifyChanges = true)/PyName=recalculate/;
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  if (a2 == -1)
    a2 = sipCpp->numRows();
  if (sipIsErr == 0)
    sipCpp->calculate(col, a1 - 1, a2 - 1, a3, a4); // row indes starts at 0
%End

	QString comment(SIP_PYOBJECT);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
        return Py_BuildValue("s", sipCpp->comment(col).toLocal8Bit().constData());
%End

	void setComment(SIP_PYOBJECT, const QString&);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setColComment(col, *a1);
%End

	void showComments(bool on = true);

    void setReadOnlyColumn(SIP_PYOBJECT col, bool on = true);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setReadOnlyColumn(col, a1);
%End

    void setColumnRole(SIP_PYOBJECT, PlotDesignation);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0){
		sipCpp->setColPlotDesignation(col, a1);
		sipCpp->setHeaderColType();
	}
%End

	void setColTextFormat(SIP_PYOBJECT);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setTextFormat(col);
%End

	void setColNumericFormat(SIP_PYOBJECT col, NumericFormat format, int prec, bool updateCells = true);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setColNumericFormat((int)a1, a2, col, a3);
%End

	void setColDateFormat(SIP_PYOBJECT col, const QString& format, bool updateCells = true);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setDateFormat(*a1, col, a2);
%End

	void setColTimeFormat(SIP_PYOBJECT col, const QString& format, bool updateCells = true);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setTimeFormat(*a1, col, a2);
%End

	void setColMonthFormat(SIP_PYOBJECT col, const QString& format, bool updateCells = true);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setMonthFormat(*a1, col, a2);
%End

	void setColDayFormat(SIP_PYOBJECT col, const QString& format, bool updateCells = true);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setDayFormat(*a1, col, a2);
%End

	void setColumnWidth(SIP_PYOBJECT, int);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setColumnWidth(col, a1);
%End

	void showAllColumns();
	void hideColumn(SIP_PYOBJECT col, bool on = true);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->hideColumn(col, a1);
%End

	bool isRowSelected(int row, bool full = false);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_ROW(a0);
	if (sipIsErr == 0)
		return Py_BuildValue("b",sipCpp->isRowSelected(row, a1));

%End

	bool isColumnSelected(SIP_PYOBJECT col, bool full = false)/PyName=isColSelected/;
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		return Py_BuildValue("b",sipCpp->isColumnSelected(col, a1));
%End

    int firstSelectedColumn();
	int numSelectedRows();

	void setSelectedCol(SIP_PYOBJECT);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setSelectedCol(col);
%End

	int selectedColumn();

	void addCol(PlotDesignation pd = Y)/PyName=addColumn/;
  int addRow();
%MethodCode
  sipRes = sipCpp->numRows() + 1;
  sipCpp->resizeRows(sipRes);
%End
	void insertColumns(SIP_PYOBJECT, int);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->insertCols(col, a1);
%End

	void swapColumns(SIP_PYOBJECT, SIP_PYOBJECT);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);

  	int col2;
    if (PyLong_Check(a1)) {
      col2 = (int)PyLong_AsLong(a1) - 1;
		if (col2 < 0 || col2 >= sipCpp->numCols()) {
			sipIsErr = 1;
            PyErr_Format(PyExc_ValueError, "There's no column %d in table %s!", col2+1, sipCpp->objectName().toLocal8Bit().constData());
		}
	 } else {
      PyObject *tmp = PyObject_Str(a1);
		if (!tmp) {
			sipIsErr = 1;
			PyErr_Format(PyExc_TypeError, "Column argument must be either int or string.");
		} else {
			col2 = sipCpp->colNames().indexOf(PyUnicode_AsUTF8(tmp));
			if (col2 < 0) {
				sipIsErr = 1;
                PyErr_Format(PyExc_ValueError, "There's no column named %s in table %s!", PyUnicode_AsUTF8(tmp),sipCpp->objectName().toLocal8Bit().constData());
				Py_DECREF(tmp);
			}
		}
	 }

	if (sipIsErr == 0)
		sipCpp->swapColumns(col, col2);
%End

void scrollToCell(SIP_PYOBJECT, int);
%MethodCode
    sipIsErr = 0;
    CHECK_TABLE_COL(a0);
    CHECK_TABLE_ROW(a1);
    if (sipIsErr == 0) {
        QTableWidgetItem *item = sipCpp->table()->item(row, col);
        sipCpp->table()->scrollToItem(item, QAbstractItemView::EnsureVisible); // for some reason these indices seem to start at 1
    }
%End

private:
  Table(const Table&);
};

class QwtLinearColorMap
{
%TypeHeaderCode
#include <qwt/qwt_color_map.h>
%End
public:

  enum Format{RGB, Indexed};
  enum Mode {FixedColors, ScaledColors};
  QwtLinearColorMap(const QColor &, const QColor &, QwtLinearColorMap::Format=QwtColorMap::RGB);

  void addColorStop (double, const QColor &);
  void setMode (Mode);

  void  setColorInterval (const QColor &, const QColor &);
  QColor color1() const;
  QColor color2() const;

private:
  QwtLinearColorMap(const QwtLinearColorMap&);
};

class LinearColorMap : QwtLinearColorMap
{
%TypeHeaderCode
#include <LinearColorMap.h>
%End
public:
  LinearColorMap();
  LinearColorMap(LinearColorMap *colorMap);
  LinearColorMap(const QColor &, const QColor &);
  void setIntensityRange(double vmin, double vmax);
  double lowerBound();
  double upperBound();
  QColor color(int index) const;
};

class Matrix: MdiSubWindow
{
%TypeHeaderCode
#include "Matrix.h"
#define CHECK_MATRIX_COL(arg)\
	int col = arg-1;\
   if (col < 0 || col >= sipCpp->numCols()) {\
		sipIsErr = 1;\
        PyErr_Format(PyExc_ValueError, "There's no column %d in matrix %s!", col+1, sipCpp->objectName().toLocal8Bit().constData());\
	}
#define CHECK_MATRIX_ROW(arg)\
   int row = arg-1;\
	if (row < 0 || row >= sipCpp->numRows()) {\
		sipIsErr = 1;\
        PyErr_Format(PyExc_ValueError, "There's no row %d in matrix %s!", row+1, sipCpp->objectName().toLocal8Bit().constData());\
	}
%End
public:
  enum HeaderViewType{ColumnRow, XY};
  enum ViewType{TableView, ImageView};
  enum ImportMode{NewColumns, NewRows, Overwrite};
  enum ResamplingMethod{Bilinear, Bicubic};

  int numRows();
  void setNumRows(int);
  int numCols();
  void setNumCols(int);
  void setDimensions(int rows, int cols);
  void resample(int rows, int cols, const ResamplingMethod& method = Matrix::Bilinear);
  void smooth();

  SIP_PYOBJECT text(int, int);
%MethodCode
	sipIsErr = 0;
	CHECK_MATRIX_ROW(a0);
	CHECK_MATRIX_COL(a1);
	if (sipIsErr == 0)
		sipRes = PyUnicode_FromString(sipCpp->text(row, col).toLocal8Bit().constData());
%End
  double cell(int, int);
%MethodCode
	sipIsErr = 0;
	CHECK_MATRIX_ROW(a0);
	CHECK_MATRIX_COL(a1);
	if (sipIsErr == 0)
		sipRes = sipCpp->cell(row, col);
%End
  void setText(int, int, const QString&);
%MethodCode
	sipIsErr = 0;
	CHECK_MATRIX_ROW(a0);
	CHECK_MATRIX_COL(a1);
	if (sipIsErr == 0)
		sipCpp->setText(row, col, *a2);
%End
  void setCell(int, int, double);
%MethodCode
	sipIsErr = 0;
	CHECK_MATRIX_ROW(a0);
	CHECK_MATRIX_COL(a1);
	if (sipIsErr == 0)
		sipCpp->setCell(row, col, a2);
%End

	double dx();
	double dy();
    double xStart();
	double xEnd();
	double yStart();
	double yEnd();
    void setCoordinates(double xs, double xe, double ys, double ye);

    void setFormula(const QString &);
    bool calculate(int startRow = 0, int endRow = -1, int startCol = 0, int endCol = -1, bool muParser = true);

    void setNumericPrecision(int prec);

    void transpose();
	void invert();
	void flipVertically();
	void flipHorizontally();
	void rotate90(bool clockwise = true);
	double determinant();
	double integrate();

	void setViewType(ViewType, bool renderImage = true);
	void setHeaderViewType(HeaderViewType);

	void setDefaultColorMap();
	void setGrayScale();
	void setRainbowColorMap();
    LinearColorMap *colorMap();
    void setColorMap(LinearColorMap*);

	void resetView();

	QImage image();
	void importImage(const QString&);
	void exportRasterImage(const QString& fileName, int quality = 100, int dpi = 0, int compression = 0);
	void exportToFile(const QString&)  /PyName=export/;
	void exportVector(const QString& fileName, int res = 0, bool color = true);

	void importASCII(const QString&, const QString& = "\t", int = 0, bool = false, bool = false,
			const QString& = "#", ImportMode = Overwrite, const QLocale& = QLocale(), int = 0, int = -1);
	bool exportASCII(const QString& fileName, const QString& sep = "\t", bool exportSelection = false);

private:
  Matrix(const Matrix&);
};

class ArrowMarker // : QwtPlotMarker
{
%TypeHeaderCode
#include "ArrowMarker.h"
%End
public:

  ArrowMarker();

  void setStartPoint(double, double) /PyName=setStart/;
  void setEndPoint(double, double) /PyName=setEnd/;

  void setStyle(Qt::PenStyle);
  void setColor(const QColor&);
  void setWidth(double);
  void drawStartArrow(bool = true);
  void drawEndArrow(bool = true);
  void setHeadLength(int);
  void setHeadAngle(int);
  void fillArrowHead(bool = true);

private:
  ArrowMarker(const ArrowMarker&);
};

class FrameWidget : QWidget  /PyName=Frame/
{
%TypeHeaderCode
#include "FrameWidget.h"
%End

public:
    enum FrameStyle{None = 0, Line = 1, Shadow = 2};
    enum Unit{Inch = 0, Millimeter, Centimeter, Point, Pixel, Scale};

    double xValue();
	double yValue();
	void setOriginCoord(double, double);
	void setOrigin(int x, int y);
	void move(const QPoint&);

	QRectF boundingRect();
	void setCoordinates(double left, double top, double right, double bottom);
    void setSize(int w, int h);
    void setSize(const QSize& newSize);
	void setRect(int x, int y, int w, int h);

	double right();
	double bottom();

	int frameStyle();
	void setFrameStyle(int);

	QPen framePen();
	void setFramePen(const QPen&);

	Qt::PenStyle frameLineStyle();
	void setFrameLineStyle(const Qt::PenStyle&);

	QColor frameColor();
	void setFrameColor(const QColor&);

	double frameWidth();
	void setFrameWidth(double);

	QColor backgroundColor();
	void setBackgroundColor(const QColor&);

	QBrush brush();
	void setBrush(const QBrush& );

private:
  FrameWidget(const FrameWidget&);
};

class ImageWidget : FrameWidget /PyName=Image/
{
%TypeHeaderCode
#include "ImageWidget.h"
%End
public:

  ImageWidget(Graph *, const QString&);
  QString fileName();
  bool load(const QString& fn);

private:
  ImageWidget(const ImageWidget&);
};

class LegendWidget : FrameWidget /PyName=Legend/
{
%TypeHeaderCode
#include "LegendWidget.h"
%End
public:

  void setText(const QString&);
  void setTextColor(const QColor&);
  void setFont(const QFont&);
  void setAngle(int);
  int angle();

  bool isAutoUpdateEnabled();
  void setAutoUpdate(bool);

private:
  LegendWidget(const LegendWidget&);
};

class RectangleWidget : FrameWidget /PyName=Rectangle/
{
%TypeHeaderCode
#include "RectangleWidget.h"
%End
public:

  RectangleWidget(Graph *);

private:
  RectangleWidget(const RectangleWidget&);
};

class EllipseWidget : FrameWidget /PyName=Ellipse/
{
%TypeHeaderCode
#include "EllipseWidget.h"
%End
public:

  EllipseWidget(Graph *);

private:
  EllipseWidget(const EllipseWidget&);
};

class QwtSymbol /PyName = PlotSymbol/
{
%TypeHeaderCode
#include <qwt/qwt_symbol.h>
%End
	public:
	enum Style
	{
		NoSymbol = -1,
		Ellipse,
		Rect,
		Diamond,
		Triangle,
		DTriangle,
		UTriangle,
		LTriangle,
		RTriangle,
		Cross,
		XCross,
		HLine,
		VLine,
		Star1,
		Star2,
		Hexagon,
        Path,
        Pixmap,
        Graphic,
        SvgDocument,
        UserStyle = 1000
	};

	QwtSymbol();
	QwtSymbol(Style st, const QBrush &bd, const QPen &pn, const QSize &s);

	void setSize(const QSize &s);
	void setSize(int a, int b = -1);
	void setBrush(const QBrush& b);
	void setPen(const QPen &p);
	void setStyle (Style s);

	const QBrush& brush() const;
	const QPen& pen() const;
	const QSize& size() const;
	Style style() const;

	private:
		QwtSymbol(const QwtSymbol&);
};

class ImageSymbol: QwtSymbol
{
%TypeHeaderCode
#include <ImageSymbol.h>
%End

public:
	ImageSymbol(const QString& fileName);
	ImageSymbol(const QPixmap& pixmap, const QString& fileName = QString());

	QPixmap pixmap();
	QString imagePath();

private:
	ImageSymbol(const ImageSymbol&);
};

class QwtPlotCurve
{
%TypeHeaderCode
#include <qwt/qwt_plot_curve.h>
%End
	public:
		int dataSize() const;
    QPointF sample(int index) const;
		double minXValue() const;
		double maxXValue() const;
		double minYValue() const;
		double maxYValue() const;

		void setPen(const QPen &);
		const QPen &pen() const;

		void setBrush(const QBrush &);
		const QBrush &brush() const;

    void setSymbol(QwtSymbol *s);
    const QwtSymbol* symbol() const;

		void setVisible(bool on);
		bool isVisible();

		void setTitle (const QString &title);

	private:
		QwtPlotCurve(const QwtPlotCurve&);
};

class PlotCurve : QwtPlotCurve
{
%TypeHeaderCode
#include "PlotCurve.h"
%End
public:
	PlotCurve(const QString& name = QString());

	int skipSymbolsCount();
	void setSkipSymbolsCount(int);

private:
	PlotCurve(const PlotCurve&);
};

class FunctionCurve : PlotCurve
{
%TypeHeaderCode
#include "FunctionCurve.h"
%End
public:
	enum FunctionType{Normal = 0, Parametric = 1, Polar = 2};

	FunctionCurve(const QString& name = QString());

	double startRange();
	double endRange();
	void setRange(double, double);

	QStringList formulas();
	void setFormula(const QString& s);
	void setFormulas(const QString&, const QString&);
%MethodCode
		QStringList l;
		l << *a0;
		l << *a1;
		sipCpp->setFormulas(l);
%End

	QString variable();
	void setVariable(const QString& s);

	FunctionType functionType();
	void setFunctionType(const FunctionType& t);

	//! Returns the number of parameters in your function formula
	int parametersCount();
	//! Returns the name of the parameter of rang index
	QString parameterName(int index);
	//! Returns the value of the parameter of rang index
	double parameterValue(int index);
	//! Returns the value of the name parameter
	double parameterValue(const QString& name);

	bool loadData(int points = 0, bool xLog10Scale = false);

private:
	FunctionCurve(const FunctionCurve&);
};

class DataCurve : PlotCurve
{
%TypeHeaderCode
#include "PlotCurve.h"
%End
public:
	DataCurve(Table *t, const QString& xColName, const QString& name, int startRow = 0, int endRow = -1);

	bool hasLabels();
	QString labelsColumnName();
	void setLabelsColumnName(const QString& name);

    int labelsAlignment();
    void setLabelsAlignment(int flags);

    int labelsXOffset();
    int labelsYOffset();
    void setLabelsOffset(int x, int y);

    double labelsRotation();
    void setLabelsRotation(double angle);

    QFont labelsFont();
    void setLabelsFont(const QFont& font);

    QColor labelsColor();
    void setLabelsColor(const QColor& c);

    bool labelsWhiteOut();
    void setLabelsWhiteOut(bool whiteOut = true);

	Table* table();

	int startRow();
	int endRow();
	void setRowRange(int startRow, int endRow);

	bool isFullRange();
	void setFullRange();

	bool updateData(Table *t, const QString& colName);
	void loadData();

	//! The list of attached error bars.
	QList<ErrorBarsCurve *> errorBarsList();
	//! Remove a single error bars curve from the list of attached error bars.
	void removeErrorBars(ErrorBarsCurve *);
	//! Clears the list of attached error bars.
	void clearErrorBars();

	void clearLabels();

private:
	DataCurve(const DataCurve&);
};

class ErrorBarsCurve : DataCurve
{
%TypeHeaderCode
#include "ErrorBarsCurve.h"
%End
public:
	enum Orientation{Horizontal = 0, Vertical = 1};

	ErrorBarsCurve(int orientation, Table *t, const QString& name);
	ErrorBarsCurve(Table *t, const QString& name);

	double errorValue(int i);

	int capLength();
	void setCapLength(int t);

	double width();
	void setWidth(double w);

	QColor color();
	void setColor(const QColor& c);

	int direction();
	void setDirection(int);

	bool xErrors();
	void setXErrors(bool);

	bool throughSymbol();
	void drawThroughSymbol(bool);

	bool plusSide();
	void drawPlusSide(bool);

	bool minusSide();
	void drawMinusSide(bool);

	//! Returns the master curve to which this error bars curve is attached.
	DataCurve* masterCurve();
	void setMasterCurve(DataCurve *);

	//! Causes the master curve to delete this curve from its managed error bars list.
	void detachFromMasterCurve();

	void loadData();

private:
	ErrorBarsCurve(const ErrorBarsCurve&);
};

class PieCurve : DataCurve
{
%TypeHeaderCode
#include "PieCurve.h"
%End
public:
	PieCurve(Table *t, const QString& name, int startRow, int endRow);
	void clone(PieCurve* c);

	double viewAngle();
	void setViewAngle(double);

	double thickness();
	void setThickness(double);

	double horizontalOffset();
	void setHorizontalOffset(double);

	bool counterClockwise();
	void setCounterClockwise(bool);

	double startAzimuth();
	void setStartAzimuth(double);

	double labelsEdgeDistance();
	void setLabelsEdgeDistance(double);

	bool labelsAutoFormat();
	void setLabelsAutoFormat(bool);

	bool labelsValuesFormat();
	void setLabelValuesFormat(bool);

	bool labelsPercentagesFormat();
	void setLabelPercentagesFormat(bool);

	bool labelCategories();
	void setLabelCategories(bool);

	bool fixedLabelsPosition();
	void setFixedLabelsPosition(bool);

	QColor color(int i) const;

	int radius();
	void setRadius(int);

	Qt::BrushStyle pattern();
	void setBrushStyle(const Qt::BrushStyle&);

	void setFirstColor(int);
	int firstColor();

	void loadData();
	void clearLabels();

	private:
		PieCurve(const PieCurve&);
};

class VectorCurve : DataCurve
{
%TypeHeaderCode
#include "VectorCurve.h"
%End
public:
	enum VectorStyle{XYXY, XYAM};
	enum Position{Tail, Middle, Head};

	VectorCurve(VectorStyle style, Table *t, const QString& xColName, const char *name,
				const QString& endCol1, const QString& endCol2, int startRow, int endRow);

	QString vectorEndXAColName();
	QString vectorEndYMColName();
	void setVectorEnd(const QString& xColName, const QString& yColName);

	double width();
	void setWidth(double);

	QColor color();
	void setColor(const QColor&);

	int headLength();
	void setHeadLength(int);

	int headAngle();
	void setHeadAngle(int);

	bool filledArrowHead();
	void fillArrowHead(bool);

	int position();
	void setPosition(int);

	int vectorStyle();
	void setVectorStyle(int);

	void loadData();

	QPen vectorPen();
	void setVectorPen(const QPen&);

	private:
		VectorCurve(const VectorCurve&);
};

class QwtHistogram : DataCurve /PyName=Histogram/
{
%TypeHeaderCode
#include "QwtHistogram.h"
%End
public:
	QwtHistogram(Table *t, const QString& name, int startRow = 0, int endRow = -1);
	QwtHistogram(Matrix *m);

    void setBinning(double size, double begin, double end);
    void setAutoBinning(bool autoBin = true);
    bool autoBinning();

    double begin();
    double end();
    double binSize();
    double ratio();
    void setRatio(double r);

    bool skipEmptyBins();
    void setSkipEmptyBins(bool on);

    void loadData();

    double mean();
    double standardDeviation();
    double minimum();
    double maximum();

    Matrix* matrix();

private:
	QwtHistogram(const QwtHistogram&);
};

class BoxCurve : DataCurve
{
%TypeHeaderCode
#include "BoxCurve.h"
%End
public:
	enum BoxStyle{NoBox, Rect, Diamond, WindBox, Notch};
	enum Range{None, SD, SE, r25_75, r10_90, r5_95, r1_99, MinMax, UserDef};

	BoxCurve(Table *t, const QString& name, int startRow = 0, int endRow = -1);

	QwtSymbol::Style minStyle();
	void setMinStyle(QwtSymbol::Style s);

	QwtSymbol::Style maxStyle();
	void setMaxStyle(QwtSymbol::Style s);

	void setMeanStyle(QwtSymbol::Style s);
	QwtSymbol::Style meanStyle();

	void setP99Style(QwtSymbol::Style s);
	QwtSymbol::Style p99Style();

	void setP1Style(QwtSymbol::Style s);
	QwtSymbol::Style p1Style();

	int boxStyle();
	void setBoxStyle(int);

	int boxWidth();
	void setBoxWidth(int);

	double boxRange();
	int boxRangeType();
	void setBoxRange(int type, double coeff);

	double whiskersRange();
	int whiskersRangeType();
	void setWhiskersRange(int type, double coeff = 0.0);

	void loadData();

	QString statistics();
	double median();
	double quantile(double);

private:
	BoxCurve(const BoxCurve&);
};

class QwtPlotSpectrogram // : QwtPlotItem
{
%TypeHeaderCode
#include <qwt/qwt_plot_spectrogram.h>
%End
public:
	enum DisplayMode {
		ImageMode = 1,
		ContourMode = 2
	};

	QwtPlotSpectrogram (const QString &title = QString());
	void setDefaultContourPen (const QPen &);
	void setDisplayMode (DisplayMode mode, bool on = true);

private:
	QwtPlotSpectrogram(const QwtPlotSpectrogram&);
};

class Spectrogram : QwtPlotSpectrogram
{
%TypeHeaderCode
#include "Spectrogram.h"
%End
public:

	Spectrogram(Graph *graph, Matrix *m);

	Matrix * matrix();
	bool setMatrix(Matrix *, bool = false);

	int levels();
	void setLevelsNumber(int);
	void setContourLevels (SIP_PYTUPLE);
%MethodCode
  QList<double> l;
  int n = PyTuple_Size(a0);
  for (int i=0; i<n; i++) {
    PyObject *element = PyTuple_GET_ITEM(a0,i);
    if (element) {
		l << PyFloat_AS_DOUBLE(element);
		Py_DECREF(element);
	 } else {
		sipIsErr = 1;
		break;
    }
  }
  sipCpp->setContourLevels(l);
%End

	bool hasColorScale();
	int colorScaleAxis();
	void showColorScale(int axis, bool on = true);

	int colorBarWidth();
	void setColorBarWidth(int width);

	void setGrayScale();
	void setDefaultColorMap();

    LinearColorMap *colorMap();
    void setCustomColorMap(LinearColorMap *map);

	bool hasLabels();
	void showContourLineLabels(bool show = true);

	QFont labelsFont();
	void setLabelsFont(const QFont& font);

	QColor labelsColor();
	void setLabelsColor(const QColor& c);

	bool labelsWhiteOut();
	void setLabelsWhiteOut(bool whiteOut = true);

	double labelsXOffset();
    double labelsYOffset();
    void setLabelsOffset(double x, double y);

	double labelsRotation();
    void setLabelsRotation(double angle);

    bool useMatrixFormula();
    bool setUseMatrixFormula(bool on = true);

    void setColorMapPen(bool on = true);
    void setContourLinePen(int, const QPen &);
	void updateData();

private:
	Spectrogram(const Spectrogram&);
};

class Grid // : QwtPlotGrid
{
%TypeHeaderCode
#include "Grid.h"
%End
public:

	Grid();

	void enableX(bool = true) /PyName=enableXMax/;
	bool xEnabled() /PyName=xMaxEnabled/;
	void enableXMin(bool = true);
	bool xMinEnabled();

	void enableY(bool = true) /PyName=enableYMax/;
	bool yEnabled() /PyName=yMaxEnabled/;
	void enableYMin(bool = true);
	bool yMinEnabled();

	bool xZeroLineEnabled();
	void enableZeroLineX(bool = true);
	bool yZeroLineEnabled();
	void enableZeroLineY(bool = true);
    QPen xZeroLinePen();
	void setXZeroLinePen(const QPen &p);
    QPen yZeroLinePen();
	void setYZeroLinePen(const QPen &p);

	void setMajPenX(const QPen &p);
	const QPen& majPenX();

	void setMinPenX(const QPen &p);
	const QPen& minPenX();

	void setMajPenY(const QPen &p);
	const QPen& majPenY();

	void setMinPenY(const QPen &p);
	const QPen& minPenY();

private:
	Grid(const Grid&);
};

class QwtPlot : QFrame
{
%TypeHeaderCode
#include "qwt/qwt_plot.h"
%End

private:
  QwtPlot(const QwtPlot&);
};

class Graph : QwtPlot
{
%TypeHeaderCode
#include "Graph.h"
#include "LegendWidget.h"
%End
public:
  enum Scale{Linear, Log10, Ln, Log2, Reciprocal, Probability, Logit};
  enum Axis{Left, Right, Bottom, Top};
  enum Ticks{NoTicks = 0, Out = 1, InOut = 2, In = 3};
  enum CurveType{Line, Scatter, LineSymbols, VerticalBars, Area, Pie, VerticalDropLines,
			Spline, HorizontalSteps, Histogram, HorizontalBars, VectXYXY, ErrorBars,
			Box, VectXYAM, VerticalSteps, ColorMap, GrayScale, Contour, Function, ImagePlot,
			StackBar, StackColumn};

  bool isPiePlot();
  SIP_PYOBJECT pieLegendText() /PyName=pieLegend/;
%MethodCode
  sipRes = PyUnicode_FromString(sipCpp->pieLegendText().toLocal8Bit().constData());
%End

  DataCurve* insertCurve(Table*, const QString&, int style = 1, int startRow = 0, int endRow = -1);
  DataCurve* insertCurve(Table*, const QString&, const QString&, int style = 1, int startRow = 0, int endRow = -1);
  bool addCurves(Table* table, SIP_PYTUPLE cols, int style=0, double lineWidth=1, int pointSize=3, int startRow=0, int endRow=-1);
%MethodCode
  QStringList l;
  int n = PyTuple_Size(a1);
  for (int i=0; i<n; i++) {
    PyObject *str = PyObject_Str(PyTuple_GET_ITEM(a1,i));
    if (str) {
      l << PyUnicode_AsUTF8(str);
		Py_DECREF(str);
	 } else {
      sipIsErr = 1;
      break;
    }
  }
  sipRes = sipCpp->addCurves(a0, l, a2, a3, a4, a5, a6);
%End

  bool addCurve(Table* table, const QString& col, int style=0, double lineWidth=1, int pointSize=3, int startRow=0, int endRow=-1);
%MethodCode
	if (a0 == 0) {
	   sipIsErr = 1;
	   PyErr_Format(PyExc_ValueError, "Invalid table in addCurve().");
	} else {
	  QStringList l;
	  l << *a1;
	  sipRes = sipCpp->addCurves(a0, l, a2, a3, a4, a5, a6);
	}
%End
  void removeCurve(int);
  void removeCurve(const QString&);
  void removeCurve(QwtPlotCurve*);
  void deleteFitCurves();
  int curveCount() /PyName=numCurves/;
  PlotCurve* curve(int index);
  PlotCurve* curve(const QString &title);
  QString curveTitle(int);
  DataCurve* dataCurve(int index);

  void plotBox(Table *, const QStringList& names, int startRow = 0, int endRow = -1);
  BoxCurve * boxCurve(int index);

  void changeCurveIndex(int fromIndex, int toIndex);
  void reverseCurveOrder();

  void setCurveLineColor(int, int);
  void setCurveLineColor(int, QColor);
  void setCurveLineStyle(int, Qt::PenStyle);
  void setCurveLineWidth(int, double);

  void setCurveSymbolColor(int, int);
  void setCurveSymbolColor(int, QColor);
  void setCurveSymbolBorderColor(int, int);
  void setCurveSymbolBorderColor(int, QColor);
  void setCurveSymbolBorderWidth(int, double);
  void setCurveSymbolOpacity(int, int);
  void setCurveSymbolStyle(int, int);
  void setCurveSymbolSize(int, int);

  void showMissingDataGap(bool on = true, bool update = true);
  bool isMissingDataGapEnabled();

  void setGrayScale();
  void setIndexedColors();

	FunctionCurve * functionCurve(int index);
	FunctionCurve* addFunction(const QString& formula, double xStart, double xEnd, int points=100);
%MethodCode
  QStringList l;
  l << *a0;
  sipRes = sipCpp->addFunction(l, a1, a2, a3);
%End
	FunctionCurve* addPolarFunction(const QString& r, const QString& theta, double tStart, double tEnd, int points=100, const QString& paramName = "t");
%MethodCode
  QStringList l;
  l << *a0;
  l << *a1;
  sipRes = sipCpp->addFunction(l, a2, a3, a4, *a5, 2);
%End
	FunctionCurve* addParametricFunction(const QString& xFormula, const QString& yFormula, double tStart, double tEnd, int points=100, const QString& paramName = "m");
%MethodCode
  QStringList l;
  l << *a0;
  l << *a1;
  sipRes = sipCpp->addFunction(l, a2, a3, a4, *a5, 1);
%End

  ErrorBarsCurve* addErrorBars(const QString&, Table *, const QString&,
		  int type = 1, double width = 1, int cap = 8, const QColor& color = QColor(Qt::black),
		  bool through = true, bool minus = true, bool plus = true);
  ErrorBarsCurve* addErrorBars(DataCurve *, Table *, const QString&,
		  int type = 1, double width = 1, int cap = 8, const QColor& color = QColor(Qt::black),
		  bool through = true, bool minus = true, bool plus = true);

  QwtHistogram* addHistogram(Matrix*);
  Spectrogram* plotSpectrogram(Matrix *m, CurveType type);
  Spectrogram* plotSpectrogram(Matrix *m, int type);
%MethodCode
    sipRes = sipCpp->plotSpectrogram(a0, (Graph::CurveType)a1);
%End
  Spectrogram* spectrogram(Matrix *m);

  PieCurve* plotPie(Table*,const QString&, int startRow = 0, int endRow = -1);
  VectorCurve* plotVectors(Table*, const QStringList& colList, int style, int startRow = 0, int endRow = -1);

  ArrowMarker* addArrow(ArrowMarker*);
  void remove(ArrowMarker*);
  QList<ArrowMarker *> arrowsList();
  int numArrows();

  ImageWidget* addImage(ImageWidget*);
  ImageWidget* addImage(const QString&);
  ImageWidget* addImage(const QImage&);
  void remove(ImageWidget*);

  void setTitle(const QString& t);
  void setTitleFont(const QFont &fnt);
  void setTitleColor(const QColor &c);
  void setTitleAlignment(int align);
  void removeTitle();

  LegendWidget* newLegend(const QString& = QString());
  void setLegend(const QString&);

  LegendWidget* legend();
  void removeLegend();

  LegendWidget* addText(LegendWidget*);
  void remove(LegendWidget*);

  LegendWidget* addTimeStamp();

  FrameWidget* add(FrameWidget*, bool = true);
  void remove(FrameWidget*);

  void setXAxisTitle(const QString& text) /PyName=setXTitle/;
  void setYAxisTitle(const QString& text) /PyName=setYTitle/;

  void enableAxis(int axis, bool on = true);
  void setAxisColor(int axis, const QColor& color);
  void setAxisFont(int axis, const QFont &fnt);

  void setAxisTitle(int axis, const QString& text);
  void setAxisTitleFont(int axis,const QFont &fnt);
  void setAxisTitleColor(int axis, const QColor& c);
  void setAxisTitleAlignment(int axis, int align);

  int axisTitleDistance(int axis);
  void setAxisTitleDistance(int axis, int dist);

  void setAxisTicksLength(int axis, int majTicksType, int minTicksType, int minLength, int majLength);
  void setAxisLabelRotation(int axis, int rotation);

  void setAxesLinewidth(int width);
  void drawAxesBackbones(bool yes);
  void setTicksLength(int minLength, int majLength);

  void setMajorTicksType(int axis, int type);
  void setMinorTicksType(int axis, int type);

  void enableAxisLabels(int axis, bool on);
  void setAxisLabelsColor(int axis, const QColor& color);
  void setLabelsNumericFormat(int axis, int format, int = 6, const QString& = QString()) /PyName=setAxisNumericFormat/;
  void setScale(int axis, double start, double end, double step = 0.0,
                int majorTicks = 5, int minorTicks = 5, int type = 0, bool inverted = false,
				double left_break = -DBL_MAX, double right_break = DBL_MAX, int pos = 50,
				double stepBeforeBreak = 0.0, double stepAfterBreak = 0.0, int minTicksBeforeBreak = 4,
				int minTicksAfterBreak = 4, bool log10AfterBreak = false, int breakWidth = 4, bool breakDecoration = true);
  void setAutoScale();
  void setFrame(int width = 1, const QColor& color = QColor(Qt::black));
  void setCanvasFrame(int width = 1, const QColor& color =  QColor(Qt::black));
  void setBackgroundColor(const QColor& color);
  void setCanvasBackground(const QColor& color) /PyName=setCanvasColor/;

  QPixmap backgroundPixmap();
  QString canvasBackgroundFileName();
  void setCanvasBackgroundImage (const QString & = QString(), bool = true);

  QRect canvasGeometry() const;
  void setCanvasGeometry(int x, int y, int w, int h);
  void setCanvasGeometry(const QRect &canvasRect);
  void setCanvasSize(int w, int h);
  void setCanvasSize(const QSize &size);

  Grid* grid();
  void showGrid(int);
  void showGrid();
  void setGridOnTop(bool on = true, bool update = true);
  bool hasGridOnTop();

  void replot();

  void exportTeX(const QString& fname, bool color = true, bool escapeStrings = true, bool fontSizes = true,
				const QSizeF& customSize = QSizeF(), int unit = FrameWidget::Pixel, double fontsFactor = 1.0)/PyName=exportTex/;
  void exportImage(const QString& fileName, int quality = 100, bool transparent = false, int dpi = 72,
		const QSizeF& customSize = QSizeF(), int unit = FrameWidget::Pixel, double fontsFactor = 1.0, int compression = 0);
  void exportVector(const QString&, bool, int = 72, bool = true,
				const QSizeF& customSize = QSizeF(), int unit = FrameWidget::Pixel, double fontsFactor = 1.0);
  void exportToFile(const QString& fileName) /PyName=export/;

  void enableAutoscaling(bool on = true);
  void setAutoscaleFonts(bool on = true);
  void setAntialiasing(bool on = true, bool update = true);

    void setCurveAxes(int,int,int);
%MethodCode
	sipIsErr = 0;
	if ((a1)>1 || (a1<0) || (a2>1) || (a2<0))
    {
         sipIsErr=1;
         PyErr_Format(PyExc_ValueError, "Invalid axis attachment!");\
    }
	if (a0>=sipCpp->curveCount())
	{
         sipIsErr=1;
         PyErr_Format(PyExc_ValueError, "Invalid curve index! is %d, should be less than %d",a0,sipCpp->curveCount());\
	}
	if (sipIsErr == 0)
	{
        QwtPlotItem* pItem;
		pItem = sipCpp->plotItem(a0);
        pItem->setAxes(a1 + 2, a2);
		sipCpp->setAutoScale();
	}
%End

  void setWaterfallSideLines(bool on = true);
  void setWaterfallFillColor(const QColor&);
  void setWaterfallOffset(int x, int y, bool update = true);

  void setLinOrLogAxis(int axis, bool logYN, bool changeAxisFormat = true, bool forceRescale = false);

  void setLogLog();
%MethodCode
    for (int ax = 0; ax < 4; ++ax)
        sipCpp->setLinOrLogAxis(ax, true, true);
%End

  void setLinLin();
%MethodCode
    for (int ax = 0; ax < 4; ++ax)
        sipCpp->setLinOrLogAxis(ax, false, true);
%End

private:
  Graph(const Graph&);
};

class MultiLayer : MdiSubWindow /PyName=GraphWindow/
{
%TypeHeaderCode
#include "MultiLayer.h"
%End
public:

  enum HorAlignement{HCenter, Left, Right};
  enum VertAlignement{VCenter, Top, Bottom};
  enum AlignPolicy{AlignLayers = 0, AlignCanvases};

  Graph *activeLayer();
  void setActiveLayer(Graph*);
  int numLayers();
  void setNumLayers(int n);
  Graph* layer(int num);
  QList<Graph *> layersList();
  Graph* addLayer(int x = 0, int y = 0, int width = 0, int height = 0, bool setPreferences = true);

  bool removeLayer(Graph *g);
  bool removeActiveLayer();

  void setCols(int);
  void setRows(int);
  void setSpacing (int, int);
  void setMargins (int, int, int, int);
  void setLayerCanvasSize (int, int);
  void setAlignement (int, int);
  void arrangeLayers(bool fit = true, bool userSize = false);
  bool swapLayers(int, int);

  void setScaleLayersOnResize(bool on = true);
  void setEqualSizedLayers();

  void setAlignPolicy(const AlignPolicy& policy);
  void setCommonLayerAxes(bool verticalAxis = true, bool horizontalAxis = true);
  void linkXLayerAxes(bool on = true);
  void keepAligned(bool on = true);
  bool isKeepAligned() const;
  void exportToFile(const QString& fileName) /PyName=export/;
  void exportImage(const QString& fileName, int quality = 100, bool transparent = false,
		int dpi = 72, const QSizeF& customSize = QSizeF(), int unit = FrameWidget::Pixel,
		double fontsFactor = 1.0, int compression = 0);
  void exportVector(const QString&, bool, int = 72, bool = true,
		const QSizeF& customSize = QSizeF(), int unit = FrameWidget::Pixel, double fontsFactor = 1.0);
  void exportTeX(const QString& fname, bool color = true, bool escapeStrings = true, bool fontSizes = true,
		const QSizeF& customSize = QSizeF(), int unit = FrameWidget::Pixel, double fontsFactor = 1.0)/PyName=exportTex/;

  void reverseWaterfallOrder();

private:
  MultiLayer(const MultiLayer&);
};

class Note: MdiSubWindow
{
%TypeHeaderCode
#include "Note.h"
%End
public:
  bool autoexec() const;
  void setAutoexec(bool on = true);

  QString text();
  void setText(const QString &s);

  void exportPDF(const QString& fileName);
  QString exportASCII(const QString &file=QString())/PyName=saveAs/;
  QString importASCII(const QString &file=QString());

  void showLineNumbers(bool on = true);

  void setFont(const QFont& f);
  void setTabStopDistance(qreal length);

  int tabs();
  void addTab();
  void removeTab(int index = -1);
  void renameTab(int, const QString&);

  int indexOf(ScriptEdit*);
  ScriptEdit* editorAt(int index);
  ScriptEdit* tab(int index);
  ScriptEdit* tab(const QString &name);
  ScriptEdit* currentEditor();

private:
  Note(const Note&);
};

class Graph3D: MdiSubWindow
{
%TypeHeaderCode
#include "Graph3D.h"
%End
public:
	enum PlotType{Scatter = 0, Trajectory = 1, Bars = 2, Ribbon = 3};
	enum AxisNumericFormat{Default = 0, Decimal = 1, Scientific = 2, Engineering = 3};

    void exportToFile(const QString& fileName) /PyName=export/;
    void exportVector(const QString& fileName, int = 0, int = 1,
		const QSizeF& customSize = QSizeF(), int unit = FrameWidget::Pixel, double fontsFactor = 1.0);
	void exportImage(const QString& fileName, int quality = 100, bool transparent = false, int dpi = 72,
		const QSizeF& customSize = QSizeF(), int unit = FrameWidget::Pixel, double fontsFactor = 1.0, int compression = 0);

    void setRotation(double xVal, double yVal, double zVal);
	void setScale(double xVal, double yVal, double zVal);
	void setShift(double xVal, double yVal, double zVal);
    void setZoom(double val);
    void setOrthogonal(bool = true);
    void setMeshLineWidth(int);
	void setMeshColor(const QColor&);
	void setAxesColor(const QColor&);
	void setNumbersColor(const QColor&);
	void setLabelsColor(const QColor&);
    void setLabelsDistance(int);
	void setBackgroundColor(const QColor&);
	void setGridColor(const QColor&);
    void setDataColors(const QColor&, const QColor&);
	QString colorMapFile();
    void setDataColorMap(const QString& fileName);
    LinearColorMap *colorMap();
    void setDataColorMap(LinearColorMap *);

    void changeTransparency(double) /PyName=setOpacity/;
    void setResolution(int);
    void showColorLegend(bool = true) /PyName=showLegend/;
    void setTitle(const QString&, const QColor& color = QColor(Qt::black), const QFont& font = QFont());
    void setAntialiasing(bool = true);
    void setLeftGrid(bool = true);
	void setRightGrid(bool = true);
	void setCeilGrid(bool = true);
	void setFloorGrid(bool = true);
	void setFrontGrid(bool = true);
	void setBackGrid(bool = true);

    void setFramed();
	void setBoxed();
	void setNoAxes();

	void setFloorData() /PyName=showFloorProjection/;
	void setFloorIsolines() /PyName=showFloorIsolines/;
	void setEmptyFloor();

    void setPolygonStyle();
	void setHiddenLineStyle();
	void setWireframeStyle();
	void setFilledMeshStyle();
	void setDotStyle();
	void setBarStyle();
	void setPlotStyle(int style);
	void rescaleColorMap();

    Graph3D* curve(int index);
%MethodCode
    (void)a0;
    sipRes = sipCpp;
%End
    void setConeStyle();
    void setCrossStyle();

    void setConeOptions(double rad, int quality);
    void setCrossOptions(double rad, double linewidth, bool smooth, bool boxed);
    void setDotOptions(double size, bool smooth);
    void setBarRadius(double rad);
    void setBarLines(bool lines = true);
    void setFilledBars(bool filled = true);

    void animate(bool = true);
    void findBestLayout();

    void changeDataColumn(Table* table, const QString& colName, int = 0) /PyName=setData/;
    void addMatrixData(Matrix* ) /PyName=setMatrix/;
    void addFunction(const QString&, double, double, double, double, double, double, int = 40, int = 40) /PyName=setFunction/;
    void addParametricSurface(const QString&, const QString&, const QString&, double,
                        double, double, double, int=40, int=40, bool=true, bool=true) /PyName=setParametricSurface/;

    void update();

    void setXAxisLabel(const QString&);
    void setYAxisLabel(const QString&);
	void setZAxisLabel(const QString&);

	void setXAxisTickLength(double majorLength, double minorLength);
    void setYAxisTickLength(double majorLength, double minorLength);
	void setZAxisTickLength(double majorLength, double minorLength);

	void setScales(double xl, double xr, double yl, double yr, double zl, double zr, int axis = -1);

    int axisNumericFormat(int axis);
    int axisNumericPrecision(int axis);
    void setAxisNumericFormat(int axis, int format, int precision);
    void setAxisNumericFormat(int axis, AxisNumericFormat format, int precision);
%MethodCode
    sipCpp->setAxisNumericFormat(a0, (int)a1, a2);
%End

    void setXAxisNumericFormat(int format, int precision);
%MethodCode
    sipCpp->setAxisNumericFormat(0, a0, a1);
%End
    void setXAxisNumericFormat(AxisNumericFormat format, int precision);
%MethodCode
    sipCpp->setAxisNumericFormat(0, (int)a0, a1);
%End

    void setYAxisNumericFormat(int format, int precision);
%MethodCode
    sipCpp->setAxisNumericFormat(1, a0, a1);
%End
    void setYAxisNumericFormat(AxisNumericFormat format, int precision);
%MethodCode
    sipCpp->setAxisNumericFormat(1, (int)a0, a1);
%End

    void setZAxisNumericFormat(int format, int precision);
%MethodCode
    sipCpp->setAxisNumericFormat(2, a0, a1);
%End
    void setZAxisNumericFormat(AxisNumericFormat format, int precision);
%MethodCode
    sipCpp->setAxisNumericFormat(2, (int)a0, a1);
%End

private:
  Graph3D(const Graph3D&);
};

class compile18 : QWidget
{
%TypeHeaderCode
#include "compile18.h"
%End

public:
    compile18(QWidget *parent = nullptr);

    // --- PATH ---
    QString pathFIF /PyName=path/;
    void setPath(const QString &path = QString()) /PyName=setPath/;
%MethodCode
    if (!a0->isEmpty() && !QDir(*a0).exists()) {
        PyErr_SetString(PyExc_ValueError,
            QString("setPath: path '%1' does not exist.").arg(*a0).toLocal8Bit().constData());
        sipError = sipErrorFail;
    } else {
        sipCpp->setPath(*a0);
    }
%End

    // --- STATE GETTERS ---
    QString functionName() const /PyName=functionName/;
    void setFunctionName(const QString &name) /PyName=setFunctionName/;
    QString yName() const /PyName=yName/;
    void setYName(const QString &name) /PyName=setYName/;
    QString xName() const /PyName=xName/;
    void setXName(const QString &name) /PyName=setXName/;
    int parameterCount() const /PyName=paramCount/;
    QString code() const /PyName=code/;
    void setCode(const QString &code) /PyName=setCode/;
    QString info() const /PyName=info/;
    void setInfo(const QString &text) /PyName=setInfo/;
    QString hHeaders() const /PyName=hHeaders/;
    void setHHeaders(const QString &text) /PyName=setHHeaders/;
    QString includedFunctions() const /PyName=includedFunctions/;
    void setIncludedFunctions(const QString &text) /PyName=setIncludedFunctions/;
    // --- GROUP (lineEditGroupName — [group] field saved in .fif file) ---
    QString group() const /PyName=group/;
    void setGroup(const QString &name) /PyName=setGroup/;
%MethodCode
    if (a0->trimmed().isEmpty()) {
        PyErr_SetString(PyExc_ValueError,
            "setGroup: group name must not be empty — use a plain name like \"custom\"");
        sipError = sipErrorFail;
    } else if (a0->contains('/')) {
        PyErr_SetString(PyExc_ValueError,
            "setGroup: group name must not contain '/' — use a plain name like \"custom\", not a subfolder path");
        sipError = sipErrorFail;
    } else {
        sipCpp->setGroup(*a0);
    }
%End
    QStringList groups() const /PyName=groups/;
    // --- CATEGORY (listViewGroup — explorer filter) ---
    QString category() const /PyName=category/;
    QStringList categories() const /PyName=categories/;
    // --- CATEGORY / FUNCTION SELECTION ---
    bool selectCategory(const QString &name) /PyName=selectCategory/;
%MethodCode
    sipRes = sipCpp->selectCategory(*a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError,
            QString("selectCategory: category '%1' not found.").arg(*a0).toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End
    QStringList functionNames(const QString &categoryName) /PyName=functionNames/;
    bool openFunction(const QString &name) /PyName=open/;
%MethodCode
    sipRes = sipCpp->openFunction(*a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError,
            QString("open: function '%1' not found.").arg(*a0).toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End
    void categoryChanged(const QString &category = QString()) /PyName=categoryChanged/;

    // --- FIF MANAGEMENT ---
    void newFIF() /PyName=new/;
    QString saveFIF(const QString &name = QString(), const QString &path = QString(), bool askYN = false) /PyName=save/;
    void deleteFIF(const QString &name = QString(), const QString &customPath = QString()) /PyName=delete/;
    void scanGroups() /PyName=scanGroups/;
    void downloadFIF(const QString &repoUrl = "https://iffgit.fz-juelich.de/qtisas/fitfunctions.git",
                    bool silent = true) /PyName=download/;

    // --- CODE GENERATION ---
    QString makeCPP() /PyName=makeCPP/;
    QString makeCompileScript() /PyName=makeScript/;

    // --- BUILD ---
    bool buildSharedLibrary(bool compileAll = false) /PyName=compile/;
    void compileAll() /PyName=compileAll/;

    // --- SASVIEW ---
    void openSasViewPy() /PyName=openSasView/;

    // --- PARAMETERS ---
    void setNumberparameters(int paraNumber) /PyName=setParamCount/;
    QStringList paramNames() const /PyName=paramNames/;
    QString paramName(int i) const /PyName=paramName/;
    void setParamName(int i, const QString &name) /PyName=setParamName/;
    void setParamNamesAll(SIP_PYLIST names) /PyName=setParamNamesAll/;
%MethodCode
    QStringList lst;
    Py_ssize_t n = PyList_Size(a0);
    for (Py_ssize_t i = 0; i < n; ++i)
        lst << QString::fromUtf8(PyUnicode_AsUTF8(PyList_GET_ITEM(a0, i)));
    sipCpp->setParamNamesAll(lst);
%End
    double paramInit(int i) const /PyName=paramInit/;
    void setParamInit(int i, double value) /PyName=setParamInit/;
    void setParamInitsAll(SIP_PYLIST values) /PyName=setParamInitsAll/;
%MethodCode
    QVector<double> vec;
    Py_ssize_t n = PyList_Size(a0);
    for (Py_ssize_t i = 0; i < n; ++i)
        vec.append(PyFloat_AsDouble(PyList_GET_ITEM(a0, i)));
    sipCpp->setParamInitsAll(vec);
%End
    QString paramLimit(int i) const /PyName=paramLimit/;
    void setParamLimit(int i, const QString &limit) /PyName=setParamLimit/;
    void setParamLimitsAll(SIP_PYLIST limits) /PyName=setParamLimitsAll/;
%MethodCode
    QStringList lst;
    Py_ssize_t n = PyList_Size(a0);
    for (Py_ssize_t i = 0; i < n; ++i)
        lst << QString::fromUtf8(PyUnicode_AsUTF8(PyList_GET_ITEM(a0, i)));
    sipCpp->setParamLimitsAll(lst);
%End
    bool paramVaries(int i) const /PyName=paramVaries/;
    void setParamVaries(int i, bool varies) /PyName=setParamVaries/;
    void setParamVariesAll(SIP_PYLIST varies) /PyName=setParamVariesAll/;
%MethodCode
    QVector<bool> vec;
    Py_ssize_t n = PyList_Size(a0);
    for (Py_ssize_t i = 0; i < n; ++i)
        vec.append(PyObject_IsTrue(PyList_GET_ITEM(a0, i)));
    sipCpp->setParamVariesAll(vec);
%End
    QString paramDescr(int i) const /PyName=paramDescr/;
    void setParamDescr(int i, const QString &descr) /PyName=setParamDescr/;
    void setParamDescrsAll(SIP_PYLIST descrs) /PyName=setParamDescrsAll/;
%MethodCode
    QStringList lst;
    Py_ssize_t n = PyList_Size(a0);
    for (Py_ssize_t i = 0; i < n; ++i)
        lst << QString::fromUtf8(PyUnicode_AsUTF8(PyList_GET_ITEM(a0, i)));
    sipCpp->setParamDescrsAll(lst);
%End

    // --- FIT.CONTROL: X-RANGE ---
    void setXRange(bool uniform, double xMin, double xMax, int nPoints, bool logStep, double yMin = 0.0) /PyName=setXRange/;
    bool customXrange() const /PyName=customXrange/;
    void setCustomXrange(bool on) /PyName=setCustomXrange/;
    double xMin() const /PyName=xMin/;
    void setXMin(double v) /PyName=setXMin/;
    double xMax() const /PyName=xMax/;
    void setXMax(double v) /PyName=setXMax/;
    int xPoints() const /PyName=xPoints/;
    void setXPoints(int n) /PyName=setXPoints/;
    double yMin() const /PyName=yMin/;
    void setYMin(double v) /PyName=setYMin/;
    bool logStep() const /PyName=logStep/;
    void setLogStep(bool on) /PyName=setLogStep/;

    // --- FIT.CONTROL: FIT OPTIONS ---
    bool eFitEnabled() const /PyName=eFitEnabled/;
    void setEFitEnabled(bool on) /PyName=setEFitEnabled/;
    QString eFit() const /PyName=eFit/;
    void setEFit(const QString &text) /PyName=setEFit/;
    bool weight() const /PyName=weight/;
    void setWeight(bool on) /PyName=setWeight/;
    int weightMethod() const /PyName=weightMethod/;
    void setWeightMethod(int idx) /PyName=setWeightMethod/;
    bool superpositional() const /PyName=superpositional/;
    void setSuperpositional(bool on) /PyName=setSuperpositional/;
    int subFitCount() const /PyName=subFitCount/;
    void setSubFitCount(int n) /PyName=setSubFitCount/;
    bool useAlgorithm() const /PyName=useAlgorithm/;
    void setUseAlgorithm(bool on) /PyName=setUseAlgorithm/;
    int fitMethod() const /PyName=fitMethod/;
    void setFitMethod(int idx) /PyName=setFitMethod/;
    QString fitMethodPara() const /PyName=fitMethodPara/;
    void setFitMethodPara(const QString &text) /PyName=setFitMethodPara/;

    // --- LOGGING ---
    void toResLog(QString text) /PyName=log/;
};

class fittable18 : QWidget
{
%TypeHeaderCode
#include "fittable18.h"
%End

public:
    fittable18(QWidget *parent = nullptr);
    QString libPath /PyName=Path/;

    // --- PRE-FUNCTION SETUP (call before setFunction) ---

    // instrument
    bool instrumentalFit() const /PyName=instrumentalFit/;
    bool setInstrumentalFit(bool on) /PyName=setInstrumentalFit/;
%MethodCode
    sipRes = sipCpp->setInstrumentalFit(a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_RuntimeError,
            "setInstrumentalFit: must be called before setFunction().");
        sipError = sipErrorFail;
    }
%End
    QString instrument() const /PyName=instrument/;
    bool setInstrument(const QString &name) /PyName=setInstrument/;
%MethodCode
    sipRes = sipCpp->setInstrument(*a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_RuntimeError,
            QString("setInstrument: '%1' not found or must be called before setFunction().")
                .arg(*a0).toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End

    // number of datasets (set before setFunction)
    int datasetCount() const /PyName=datasetCount/;
    void setDatasetCount(int n) /PyName=setDatasetCount/;

    // fit method: 0=Simplex  1=Levenberg-Marquardt  2=GenMin
    int fitMethod() const /PyName=fitMethod/;
    void setFitMethod(int index) /PyName=setFitMethod/;
    void setFitMethod(const QString &name) /PyName=setFitMethod/;
    int maxIterations() const /PyName=maxIterations/;
    void setMaxIterations(int n) /PyName=setMaxIterations/;
    double tolerance() const /PyName=tolerance/;
    void setTolerance(double v) /PyName=setTolerance/;
    double toleranceAbs() const /PyName=toleranceAbs/;
    void setToleranceAbs(double v) /PyName=setToleranceAbs/;

    // Levenberg-Marquardt: lmControl 0=[gradient,scaled] 1=[gradient,unscaled] 2=[delta,scaled] 3=[delta,unscaled]
    int lmControl() const /PyName=lmControl/;
    void setLmControl(int index) /PyName=setLmControl/;
    void setLmControl(const QString &name) /PyName=setLmControl/;
    bool constChi2() const /PyName=lmAdaptStep/;
    void setConstChi2(bool on) /PyName=setLmAdaptStep/;
    double derivStep() const /PyName=lmDerivStep/;
    void setDerivStep(double v) /PyName=setLmDerivStep/;

    // Simplex: variant 0=nmsimplex2  1=nmsimplex2rand  2=simplex
    //          convRate 0=fast(×10)  1=normal(×1)  2=slow(×0.1)
    int simplexVariant() const /PyName=simplexVariant/;
    void setSimplexVariant(int index) /PyName=setSimplexVariant/;
    void setSimplexVariant(const QString &name) /PyName=setSimplexVariant/;
    int simplexConvRate() const /PyName=simplexConvRate/;
    void setSimplexConvRate(int index) /PyName=setSimplexConvRate/;
    bool constChi2Simplex() const /PyName=simplexRestart/;
    void setConstChi2Simplex(bool on) /PyName=setSimplexRestart/;

    // GenMin: localSearch 0=genmin  1=levenberg  2=genmin+levenberg
    //         setGenMinLocalSearch also accepts strings: "genmin", "levenberg", "genmin+levenberg"
    int genMinPopulation() const /PyName=genMinPopulation/;
    void setGenMinPopulation(int n) /PyName=setGenMinPopulation/;
    int genMinGenomeSize() const /PyName=genMinGenomeSize/;
    void setGenMinGenomeSize(int n) /PyName=setGenMinGenomeSize/;
    int genMinGenerations() const /PyName=genMinGenerations/;
    void setGenMinGenerations(int n) /PyName=setGenMinGenerations/;
    double genMinSelectionRate() const /PyName=genMinSelectionRate/;
    void setGenMinSelectionRate(double v) /PyName=setGenMinSelectionRate/;
    double genMinMutationRate() const /PyName=genMinMutationRate/;
    void setGenMinMutationRate(double v) /PyName=setGenMinMutationRate/;
    int genMinRandomSeed() const /PyName=genMinRandomSeed/;
    void setGenMinRandomSeed(int n) /PyName=setGenMinRandomSeed/;
    int genMinLocalSearch() const /PyName=genMinLocalSearch/;
    void setGenMinLocalSearch(int index) /PyName=setGenMinLocalSearch/;
    void setGenMinLocalSearch(const QString &name) /PyName=setGenMinLocalSearch/;

    // --- FUNCTION SELECTION (triggers fit page) ---

    // configure() handles the full sequence: setDatasetCount → setFunction → selectDatasets
    bool configure(const QString &function, const QStringList &datasets = QStringList()) /PyName=configure/;
%MethodCode
    sipRes = sipCpp->configure(*a0, *a1);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError,
            QString("configure: '%1' failed. Check function name and dataset names.")
                .arg(*a0).toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End

    QStringList fitFunctions(const QString &pattern = QString()) /PyName=fitFunctions/;
    QString currentFunction() const;
    bool setFunction(const QString &name);
%MethodCode
    sipRes = sipCpp->setFunction(*a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError,
            QString("setFunction: '%1' not found.").arg(*a0).toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End

    // --- POST-FUNCTION SETUP (call after setFunction) ---

    // dataset selection (m is 1-indexed)
    QStringList availableDatasets() /PyName=datasets/;
    QString selectedDataset(int m) const /PyName=selectedDataset/;
    QStringList selectedDatasets() const /PyName=selectedDatasets/;
    bool setDataset(int m, const QString &name) /PyName=selectDataset/;
%MethodCode
    sipRes = sipCpp->setDataset(a0, *a1);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError,
            QString("selectDataset: '%1' not found.").arg(*a1).toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End
    void setRange(int m, double min, double max, bool byIndex = false) /PyName=setRange/;
    void setRangeAll(double min, double max, bool byIndex = false) /PyName=setRangeAll/;

    // weighting: method 0=1/σ²  1=1/Y  2=σ  3=1/Y²  4=1/Y^a  5=1/|c^a+b·Y^a|  6=1/Y^a·c^|Xmax-X|
    int weightingMethod() const /PyName=weightingMethod/;
    void setWeightingMethod(int method) /PyName=setWeightingMethod/;
    double weightingParamA() const /PyName=weightingA/;
    void setWeightingParamA(double v) /PyName=setWeightingA/;
    double weightingParamB() const /PyName=weightingB/;
    void setWeightingParamB(double v) /PyName=setWeightingB/;
    double weightingParamC() const /PyName=weightingC/;
    void setWeightingParamC(double v) /PyName=setWeightingC/;
    double weightingParamXmax() const /PyName=weightingXmax/;
    void setWeightingParamXmax(double v) /PyName=setWeightingXmax/;
    bool weightingEnabled(int m) const /PyName=weightingEnabled/;
    void setWeightingEnabled(int m, bool enabled) /PyName=setWeightingEnabled/;
    QString weightingDataset(int m) const /PyName=weightingDataset/;
    void setWeightingDataset(int m, const QString &name) /PyName=setWeightingDataset/;

    // resolution — global: function 0=Gauss-SANS  1=Triangular  2=Bessel-SANS  3=Gauss  4=InFunction
    //                       speedControl 0=Fastest .. 6=Perfect  7=Custom
    int resoFunction() const /PyName=resoFunction/;
    void setResoFunction(int index) /PyName=setResoFunction/;
    void setResoFunction(const QString &name) /PyName=setResoFunction/;
    int resoSpeedControl() const /PyName=resoSpeedControl/;
    void setResoSpeedControl(int index) /PyName=setResoSpeedControl/;
    int resoWorkspace() const /PyName=resoWorkspace/;
    void setResoWorkspace(int n) /PyName=setResoWorkspace/;
    int resoSigmaLimit() const /PyName=resoSigmaLimit/;
    void setResoSigmaLimit(int n) /PyName=setResoSigmaLimit/;
    // resolution — per dataset (m is 1-indexed)
    bool resoEnabled(int m) const /PyName=resoEnabled/;
    void setResoEnabled(int m, bool enabled) /PyName=setResoEnabled/;
    QString resoDataset(int m) const /PyName=resoDataset/;
    void setResoDataset(int m, const QString &name) /PyName=setResoDataset/;

    // polydispersity — global: function 0=Gauss  1=Schultz-Zimm  2=Gamma  3=Log-Normal  4=Uniform  5=Triangular
    //                          speedControl 0=Fastest .. 6=Perfect  7=Custom
    int polyFunction() const /PyName=polyFunction/;
    void setPolyFunction(int index) /PyName=setPolyFunction/;
    void setPolyFunction(const QString &name) /PyName=setPolyFunction/;
    int polySpeedControl() const /PyName=polySpeedControl/;
    void setPolySpeedControl(int index) /PyName=setPolySpeedControl/;
    int polyWorkspace() const /PyName=polyWorkspace/;
    void setPolyWorkspace(int n) /PyName=setPolyWorkspace/;
    int polySigmaLimit() const /PyName=polySigmaLimit/;
    void setPolySigmaLimit(int n) /PyName=setPolySigmaLimit/;
    // polydispersity — per dataset (m is 1-indexed)
    bool polyEnabled(int m) const /PyName=polyEnabled/;
    void setPolyEnabled(int m, bool enabled) /PyName=setPolyEnabled/;
    QString polyDataset(int m) const /PyName=polyDataset/;
    void setPolyDataset(int m, const QString &name) /PyName=setPolyDataset/;

    // parameters (dataset: 1=first .. N; limits: -1=global  1..N=per-dataset)
    int parameterCount() const /PyName=paramCount/;
    QStringList parameterNames() const /PyName=paramNames/;
    double parameterValue(const QString &name, int dataset = 1) const /PyName=paramValue/;
    void setParameterValue(const QString &name, double value, int dataset = -1) /PyName=setParamValue/;
    void setParameterValueAll(SIP_PYLIST values, int dataset = -1) /PyName=setParamValueAll/;
%MethodCode
    QVector<double> vec;
    Py_ssize_t n = PyList_Size(a0);
    for (Py_ssize_t i = 0; i < n; ++i)
        vec.append(PyFloat_AsDouble(PyList_GET_ITEM(a0, i)));
    sipCpp->setParameterValueAll(vec, a1);
%End
    bool parameterVaries(const QString &name, int dataset = 1) const /PyName=paramVaries/;
    void setParameterVaries(const QString &name, bool varies, int dataset = -1) /PyName=setParamVaries/;
    void setParameterVariesAll(SIP_PYLIST values, int dataset = -1) /PyName=setParamVariesAll/;
%MethodCode
    QVector<bool> vec;
    Py_ssize_t n = PyList_Size(a0);
    for (Py_ssize_t i = 0; i < n; ++i)
        vec.append(PyObject_IsTrue(PyList_GET_ITEM(a0, i)));
    sipCpp->setParameterVariesAll(vec, a1);
%End
    bool parameterShared(const QString &name) const /PyName=paramShared/;
    void setParameterShared(const QString &name, bool shared) /PyName=setParamShared/;
    double parameterError(const QString &name, int dataset = 1) const /PyName=paramError/;
    bool parameterBayesian(const QString &name, int dataset = -1) const /PyName=paramBayesian/;
    double parameterLimitLeft(const QString &name, int dataset = -1) const /PyName=paramLimitLeft/;
    double parameterLimitRight(const QString &name, int dataset = -1) const /PyName=paramLimitRight/;
    void setParameterLimits(const QString &name, double left, double right, bool bayesianYN = false, int dataset = -1) /PyName=setParamLimits/;

    // --- EXECUTE ---
    void setScaleErrors(bool on);
    void setStatisticsNotes(bool on);
    void setSaveSession(bool on);
    double fit();
%MethodCode
    if (sipCpp->stage() != "fit") {
        PyErr_SetString(PyExc_RuntimeError,
            "fit(): call setFunction() and selectDataset() first (stage must be \"fit\")");
        sipIsErr = 1;
    } else {
        sipRes = sipCpp->fit();
        if (PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
                sipCpp->app()->scriptPrint("Fit interrupted");
            PyErr_Clear();
        }
    }
%End
    double r2Factor() const /PyName=r2/;
    double qFactor() const /PyName=qFactor/;

    void initParametersBeforeFit() /PyName=beforeFit/;
%MethodCode
    if (sipCpp->stage() != "fit") {
        PyErr_SetString(PyExc_RuntimeError,
            "beforeFit(): stage must be \"fit\"");
        sipIsErr = 1;
    } else {
        sipCpp->initParametersBeforeFit();
    }
%End

    void initParametersAfterFit() /PyName=afterFit/;
%MethodCode
    if (sipCpp->stage() != "fit") {
        PyErr_SetString(PyExc_RuntimeError,
            "afterFit(): stage must be \"fit\"");
        sipIsErr = 1;
    } else {
        sipCpp->initParametersAfterFit();
    }
%End

    // --- SIMULATE ---
    bool simUniformX() const;
    void setSimUniformX(bool uniform);
    double simFromX() const;
    void setSimFromX(double v);
    double simToX() const;
    void setSimToX(double v);
    int simNumPoints() const;
    void setSimNumPoints(int n);
    bool simLogStep() const;
    void setSimLogStep(bool on);
    QString simDataset() const;
    bool setSimDataset(const QString &name /In/);
    int curveColor() const;
    bool setCurveColor(int index);
    bool setCurveColor(const QString &name /In/);
    QString stage() const;
    bool selectStage(const QString &s /In/);
%MethodCode
    if (sipCpp->stackPage() == 0) {
        if (*a0 == "init") {
            sipRes = false;  // already in init, silently do nothing
        } else {
            PyErr_SetString(PyExc_RuntimeError,
                "selectStage(): call setFunction() first (currently in init stage)");
            sipIsErr = 1;
        }
    } else {
        sipRes = sipCpp->selectStage(*a0);
        if (!sipIsErr && !sipRes) {
            PyErr_Format(PyExc_ValueError,
                "selectStage(): unknown stage \"%s\" - valid: init, fit, simulate, results, batch-fit",
                a0->toUtf8().constData());
            sipIsErr = 1;
        }
    }
%End

    double simulate();
%MethodCode
    if (sipCpp->stage() != "fit") {
        PyErr_SetString(PyExc_RuntimeError,
            "simulate(): call setFunction() and selectDataset() first (stage must be \"fit\")");
        sipIsErr = 1;
    } else {
        sipRes = sipCpp->simulate();
    }
%End

    double simulateCurve();
%MethodCode
    if (sipCpp->stage() != "simulate") {
        PyErr_SetString(PyExc_RuntimeError,
            "simulateCurve(): navigate to Simulate Curve tab first (stage must be \"simulate\")");
        sipIsErr = 1;
    } else {
        sipRes = sipCpp->simulateCurve();
    }
%End

    MultiLayer* plotResults(const QString &graphName /In/ = QString(), const QString &graphTemplate /In/ = QString());
%MethodCode
    if (sipCpp->stage() != "fit") {
        PyErr_SetString(PyExc_RuntimeError,
            "plotResults(): stage must be \"fit\"");
        sipIsErr = 1;
    } else {
        sipRes = sipCpp->plotResults(*a0, *a1);
    }
%End

    // --- RESULTS ---
    double lastChi2;
    double lastDoF;
    QString lastFitReport;
    QString fitReport();

    // results() → {name: (value, error|None)}  for M=1
    //           → {name: [(v1,e1|None), ...]}   for M>1
    SIP_PYOBJECT results();
%MethodCode
    int M = sipCpp->datasetCount();
    QStringList names = sipCpp->parameterNames();

    auto makeTuple = [](double v, double e, bool varies) -> PyObject * {
        PyObject *t = PyTuple_New(2);
        PyTuple_SET_ITEM(t, 0, PyFloat_FromDouble(v));  // steals ref
        PyObject *err = (varies && !std::isnan(e)) ? PyFloat_FromDouble(e)
                                                   : (Py_INCREF(Py_None), Py_None);
        PyTuple_SET_ITEM(t, 1, err);                    // steals ref
        return t;
    };

    PyObject *d = PyDict_New();
    for (const QString &name : names)
    {
        QByteArray key = name.toUtf8();
        if (M == 1)
        {
            PyObject *t = makeTuple(sipCpp->parameterValue(name, 1),
                                    sipCpp->parameterError(name, 1),
                                    sipCpp->parameterVaries(name, 1));
            PyDict_SetItemString(d, key.constData(), t);
            Py_DECREF(t);
        }
        else
        {
            PyObject *lst = PyList_New(M);
            for (int m = 1; m <= M; m++)
                PyList_SET_ITEM(lst, m - 1,
                    makeTuple(sipCpp->parameterValue(name, m),
                              sipCpp->parameterError(name, m),
                              sipCpp->parameterVaries(name, m)));
            PyDict_SetItemString(d, key.constData(), lst);
            Py_DECREF(lst);
        }
    }
    sipRes = d;
%End

    // --- STATE INTROSPECTION ---
    SIP_PYOBJECT state();
%MethodCode
    int stage = sipCpp->fittingStage();
    const char *stageName = (stage == 0) ? "init" : (stage == 1) ? "ready" : "fitted";

    PyObject *d = PyDict_New();
    PyDict_SetItemString(d, "stage",        PyUnicode_FromString(stageName));
    PyDict_SetItemString(d, "function",     PyUnicode_FromString(sipCpp->currentFunction().toUtf8().constData()));
    PyDict_SetItemString(d, "datasets",     PyLong_FromLong(sipCpp->datasetCount()));

    QStringList sel = sipCpp->selectedDatasets();
    PyObject *selList = PyList_New(sel.size());
    for (int i = 0; i < sel.size(); i++)
        PyList_SET_ITEM(selList, i, PyUnicode_FromString(sel[i].toUtf8().constData()));
    PyDict_SetItemString(d, "selected", selList);
    Py_DECREF(selList);

    PyDict_SetItemString(d, "fitMethod",    PyLong_FromLong(sipCpp->fitMethod()));
    PyDict_SetItemString(d, "params",       PyLong_FromLong(sipCpp->parameterCount()));
    PyDict_SetItemString(d, "instrumental", PyBool_FromLong(sipCpp->instrumentalFit()));

    if (stage >= 2) {
        PyDict_SetItemString(d, "chi2", PyFloat_FromDouble(sipCpp->lastChi2));
        PyDict_SetItemString(d, "dof",  PyFloat_FromDouble(sipCpp->lastDoF));
    } else {
        PyDict_SetItemString(d, "chi2", Py_None); Py_INCREF(Py_None);
        PyDict_SetItemString(d, "dof",  Py_None); Py_INCREF(Py_None);
    }

    sipRes = d;
%End
};

class dan18 : public QWidget
{
%TypeHeaderCode
#include "dan18.h"
%End

public:
    explicit dan18(QWidget *parent = nullptr);

    void newSession();
    void openSession(QString scriptName);
    void instrumentSelected(QString instrumentName) /PyName=selectInstrument/;
    void experimentalModeSelected(QString mode) /PyName=selectMode/;
    bool setPathIn(QString path);
    bool setPathOut(QString path);
    bool infoExtractorScript(
        QString tableName,
        QString columnNumberList,
        QString runsRange = QString()
    ) /PyName=infoExtractor/;
};


class ApplicationWindow: QMainWindow
{
%TypeHeaderCode
#include "ApplicationWindow.h"
%End
%ConvertToSubClassCode
// we have to do this to override casting in qt/qobject.sip
 sipType = sipFindType(sipCpp->metaObject()->className());
%End

public:
  enum MatrixToTableConversion{Direct, XYZ, YXZ};

  void autoArrangeLayers();
  Table* table(const QString&);
%MethodCode
  sipRes = sipCpp->current_folder->table(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->table(*a0, true);
%End
  compile18	*compileWidget /PyName=Compiler/;
  fittable18	*fittableWidget /PyName=Fittable/;
  dan18 *danWidget /PyName=Dan/;

  Table* newTable();
  Table* currentTable();
  Table* newTable(const QString& name, int rows=30, int cols=2);
%MethodCode
  {
    Table *existing = sipCpp->current_folder->table(*a0, false);
    if (!existing)
        existing = sipCpp->projectFolder()->table(*a0, true);
    if (existing)
    {
        existing->setNumRows(a1);
        existing->setNumCols(a2);
        existing->clear();
        sipRes = existing;
    }
    else
        sipRes = sipCpp->newTable(*a0, a1, a2);
  }
%End
  Matrix* matrix(const QString&);
%MethodCode
  sipRes = sipCpp->current_folder->matrix(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->matrix(*a0, true);
%End
  Matrix* newMatrix();
  Matrix* currentMatrix();
  Matrix* newMatrix(const QString& name, int rows=32, int cols=32);
%MethodCode
  {
    Matrix *existing = sipCpp->current_folder->matrix(*a0, false);
    if (!existing)
        existing = sipCpp->projectFolder()->matrix(*a0, true);
    if (existing)
    {
        existing->setNumRows(a1);
        existing->setNumCols(a2);
        sipRes = existing;
    }
    else
        sipRes = sipCpp->newMatrix(*a0, a1, a2);
  }
%End
  MultiLayer *plot(const QString&) /PyName=graphWindow/;
%MethodCode
  sipRes = sipCpp->current_folder->graph(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->graph(*a0, true);
%End
  MultiLayer* currentPlot() /PyName=currentGraph/;

  MultiLayer* newGraph(const QString& name = "Graph1", int layers = 1, int rows = 1, int cols = 1, bool applyPreferences = true);
%MethodCode
  {
    MultiLayer *existing = sipCpp->current_folder->graph(*a0, false);
    if (!existing)
        existing = sipCpp->projectFolder()->graph(*a0, true);
    if (existing)
    {
        if (a1 > 1 && (existing->getRows() != a2 || existing->getCols() != a3))
        {
            existing->setRows(a2);
            existing->setCols(a3);
            existing->arrangeLayers(false, true);
        }
        sipRes = existing;
    }
    else if (a1 == 1)
        sipRes = sipCpp->newGraph(*a0);
    else
        sipRes = sipCpp->multilayerPlot(*a0, a1, a2, a3, a4);
  }
%End

  Note *note(const QString&);
%MethodCode
  sipRes = sipCpp->current_folder->note(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->note(*a0, true);
%End
  Note* newNote(const QString& name = QString());
%MethodCode
  {
    Note *existing = nullptr;
    if (!a0->isEmpty())
    {
        existing = sipCpp->current_folder->note(*a0, false);
        if (!existing)
            existing = sipCpp->projectFolder()->note(*a0, true);
    }
    sipRes = existing ? existing : sipCpp->newNote(*a0);
  }
%End
  Note* currentNote();
  MultiLayer *multilayerPlot(Table* table, SIP_PYTUPLE cols, int style=1) /PyName=plot/;
%MethodCode
	if (a0 == 0) {
	   sipIsErr = 1;
	   PyErr_Format(PyExc_ValueError, "Invalid table in argument to plot().");
	} else {
	  QStringList l;
	  int n = PyTuple_Size(a1);
	  for (int i=0; i<n; i++) {
		PyObject *str = PyObject_Str(PyTuple_GET_ITEM(a1,i));
		if (str) {
		  l << PyUnicode_AsUTF8(str);
			Py_DECREF(str);
		 } else {
		  sipIsErr = 1;
		  break;
		}
	  }
	  sipRes = sipCpp->multilayerPlot(a0, l, a2);
	}
%End
  MultiLayer *multiLayerPlot(Table* table, const QString& col, int style=1) /PyName=plot/;
%MethodCode
	if (a0 == 0) {
	   sipIsErr = 1;
	   PyErr_Format(PyExc_ValueError, "Invalid table in argument to plot().");
	} else {
	  QStringList l;
	  l << *a1;
	  sipRes = sipCpp->multilayerPlot(a0, l, a2);
	}
%End

	MultiLayer *waterfallPlot(Table*, SIP_PYTUPLE);
%MethodCode
	if (a0 == 0) {
	   sipIsErr = 1;
	   PyErr_Format(PyExc_ValueError, "Invalid table in argument to waterfallPlot().");
	} else {
	  QStringList l;
	  int n = PyTuple_Size(a1);
	  for (int i=0; i<n; i++) {
		PyObject *str = PyObject_Str(PyTuple_GET_ITEM(a1,i));
		if (str) {
		  l << PyUnicode_AsUTF8(str);
			Py_DECREF(str);
		 } else {
		  sipIsErr = 1;
		  break;
		}
	  }
	  sipRes = sipCpp->waterfallPlot(a0, l);
	}
%End

	Matrix* importImage(const QString&, bool);
	MultiLayer* plotSpectrogram(Matrix* matrix, Graph::CurveType style = Graph::ColorMap) /PyName=plot/;
	MultiLayer* plotSpectrogram(Matrix*, int) /PyName=plot/;
%MethodCode
    sipRes = sipCpp->plotSpectrogram(a0, (Graph::CurveType)a1);
%End
	MultiLayer* plotImageProfiles(Matrix*);
	MultiLayer* plot(const QString& formula, double xStart, double xEnd, int points = 100);
%MethodCode
  QStringList l;
  l << *a0;
  sipRes = sipCpp->newFunctionPlot(l, a1, a2, a3);
%End

    Graph3D* newPlot3D(const QString& = QString());
    Graph3D* plotXYZ(Table* table, const QString&, int = 0) /PyName=plot3D/;
    Graph3D* plot3DMatrix(Matrix *, int style = 5) /PyName=plot3D/;
    Graph3D* plotSurface(const QString&, double xl, double xr, double yl, double yr,
            double zl, double zr, int=40, int=40) /PyName=plot3D/;
    Graph3D* plotParametricSurface(const QString&, const QString&, const QString&,
            double, double, double, double, int=40, int=40, bool=true, bool=true) /PyName=plot3D/;
    Graph3D *plot3D(const QString&);
%MethodCode
    sipRes = sipCpp->current_folder->plot3D(*a0, false);
    if (!sipRes)
        sipRes = sipCpp->projectFolder()->plot3D(*a0, true);
%End

  QList<MdiSubWindow*> windowsList() /PyName=windows/;

  // folders
  Folder *activeFolder();
%MethodCode
  sipRes = sipCpp->current_folder;
%End

  Folder* appendProject(const QString& file_name, Folder* parentFolder = 0);
  void saveFolder(Folder *folder, const QString& fn, bool compress = false);
  Folder* projectFolder() /PyName=rootFolder/;

  Folder* addFolder(QString name, Folder* parent = 0);
  bool deleteFolder(Folder *);
  bool changeFolder(Folder *, bool force = false);
  bool copyFolder(Folder *src, Folder *dest);

  MdiSubWindow* openTemplate(const QString&);
  void saveAsTemplate(MdiSubWindow*, const QString&);
  QStringList graphTemplates() const;
%MethodCode
    QStringList result;
    for (const QString &fn : sipCpp->magicList)
    {
        QString name = fn;
        if (name.endsWith(QLatin1String(".qpt"), Qt::CaseInsensitive))
            name.chop(4);
        result << name;
    }
    sipRes = new QStringList(result);
%End

  void setWindowName(MdiSubWindow *, const QString &);
  void setPreferences(Graph*);

  // graph default preference getters — read the values stored in QtiSAS Preferences dialog
  bool graphTitleOn() const;
  %MethodCode
      sipRes = sipCpp->titleOn;
  %End

  bool graphDrawBackbones() const;
  %MethodCode
      sipRes = sipCpp->drawBackbones;
  %End

  bool graphAntialiasing() const;
  %MethodCode
      sipRes = sipCpp->antialiasing2DPlots;
  %End

  bool graphAutoscale() const;
  %MethodCode
      sipRes = sipCpp->autoscale2DPlots;
  %End

  int graphAxesLineWidth() const;
  %MethodCode
      sipRes = sipCpp->axesLineWidth;
  %End

  int graphMajTicksStyle() const;
  %MethodCode
      sipRes = sipCpp->majTicksStyle;
  %End

  int graphMinTicksStyle() const;
  %MethodCode
      sipRes = sipCpp->minTicksStyle;
  %End

  int graphMajTicksLength() const;
  %MethodCode
      sipRes = sipCpp->majTicksLength;
  %End

  int graphMinTicksLength() const;
  %MethodCode
      sipRes = sipCpp->minTicksLength;
  %End

  int graphCanvasFrameWidth() const;
  %MethodCode
      sipRes = sipCpp->canvasFrameWidth;
  %End

  int graphDefaultMargin() const;
  %MethodCode
      sipRes = sipCpp->defaultPlotMargin;
  %End

  int graphTickLabelsDist() const;
  %MethodCode
      sipRes = sipCpp->d_graph_tick_labels_dist;
  %End

  int graphAxesLabelsDist() const;
  %MethodCode
      sipRes = sipCpp->d_graph_axes_labels_dist;
  %End

  int graphDefaultCurveStyle() const;
  %MethodCode
      sipRes = sipCpp->defaultCurveStyle;
  %End

  int graphDefaultSymbolSize() const;
  %MethodCode
      sipRes = sipCpp->defaultSymbolSize;
  %End

  double graphDefaultCurveLineWidth() const;
  %MethodCode
      sipRes = (double)sipCpp->defaultCurveLineWidth;
  %End

  QColor* graphCanvasColor() const;
  %MethodCode
      sipRes = new QColor(sipCpp->d_graph_canvas_color);
  %End

  QColor* graphBackgroundColor() const;
  %MethodCode
      sipRes = new QColor(sipCpp->d_graph_background_color);
  %End

  QColor* graphBorderColor() const;
  %MethodCode
      sipRes = new QColor(sipCpp->d_graph_border_color);
  %End

  QFont* graphAxesFont() const;
  %MethodCode
      sipRes = new QFont(sipCpp->plotAxesFont);
  %End

  QFont* graphTitleFont() const;
  %MethodCode
      sipRes = new QFont(sipCpp->plotTitleFont);
  %End

  QFont* graphLegendFont() const;
  %MethodCode
      sipRes = new QFont(sipCpp->plotLegendFont);
  %End

  QFont* graphNumbersFont() const;
  %MethodCode
      sipRes = new QFont(sipCpp->plotNumbersFont);
  %End

  void saveProjectAs(const QString& fileName = QString(), bool compress = false);
  MdiSubWindow* clone(MdiSubWindow*);

  Matrix* tableToMatrix(Table* t);
  Matrix* tableToMatrixRegularXYZ(Table* t, const QString& colName);
  Table* matrixToTable(Matrix* m, MatrixToTableConversion = Direct);

  QTextEdit *resultsLog();
  void displayInfo(const QString& text);
  QLineEdit *infoLineEdit();
  QString stemPlot(Table *t, const QString& colName, int power = 1001, int startRow = 0, int endRow = -1);

  QMdiArea* workspace();
  Table* importWaveFile();

  // window listing
  QList<MdiSubWindow*> tableList() /PyName=tables/;
  QList<MdiSubWindow*> matrixList() /PyName=matrices/;
  QList<MdiSubWindow*> multilayerList() /PyName=graphs/;
  QList<MdiSubWindow*> noteList() /PyName=notes/;
  QStringList tableNames();
  QStringList matrixNames();

  // window management
  void closeWindow(MdiSubWindow *);
  void activateWindow(MdiSubWindow *);
  bool activateWindow(QString);
  void hideWindow(MdiSubWindow *);
  void maximizeWindow(MdiSubWindow *);
  void maximizeWindow(const QString &);

  // project lifecycle
  void newProject();
  void closeProject();
  bool saveProject(bool compress = false);
  void saveGraphAsProject(MultiLayer *graph, const QString &path, const QString &name, bool compress = false, const QString &format = QString(), int resolution = -1);

  // existence
  bool existWindow(const QString &);
  bool checkTableExistence(const QString &tableName) /PyName=existTable/;
  bool checkNoteExistence(const QString &noteName) /PyName=existNote/;

  // custom actions
  bool addCustomAction(const QString &name, const QString &scriptName,
                       const QString &menuName = QString(),
                       const QString &iconName = QString(),
                       const QString &shortcut = QString(),
                       const QString &tooltip = QString());
  bool updateCustomActionName(const QString &name, const QString &newName);
  bool updateCustomActionScript(const QString &name, const QString &newScriptName);
  bool updateCustomActionIcon(const QString &name, const QString &newIconName);
  bool updateCustomActionMenu(const QString &name, const QString &newMenuName);
  bool removeCustomActionByName(const QString &name) /PyName=removeCustomAction/;
  bool runCustomActionByName(const QString &name) /PyName=runCustomAction/;
  QStringList customActionNames() const;
  QString customActionsPath() const;

  // console output
  void scriptPrint(const QString &);
  void scriptPrintReplace(const QString &);

  // indexed colour palette
  SIP_PYLIST colorNames() const;
  %MethodCode
    {
        QStringList names = sipCpp->indexedColorNames();
        PyObject *lst = PyList_New(names.size());
        for (int i = 0; i < names.size(); i++)
            PyList_SET_ITEM(lst, i,
                PyUnicode_FromString(names[i].toUtf8().constData()));
        sipRes = lst;
    }
  %End

  int colorIndex(const QString& name) const;
  %MethodCode
    {
        QStringList names = sipCpp->indexedColorNames();
        sipRes = names.indexOf(*a0);
    }
  %End

  // indexed symbol palette
  SIP_PYLIST symbolNames() const;
  %MethodCode
    {
        struct { int style; const char *name; } map[] = {
            {-1, "no symbol"}, {0, "ellipse"}, {1, "rectangle"},
            {2, "diamond"}, {3, "triangle"}, {4, "down triangle"},
            {5, "up triangle"}, {6, "left triangle"}, {7, "right triangle"},
            {8, "cross"}, {9, "diagonal cross"}, {10, "horizontal line"},
            {11, "vertical line"}, {12, "star 1"}, {13, "star 2"}, {14, "hexagon"}
        };
        QList<int> syms = sipCpp->indexedSymbols();
        PyObject *lst = PyList_New(syms.size());
        for (int i = 0; i < syms.size(); i++)
        {
            const char *n = "unknown";
            for (int j = 0; j < 16; j++)
                if (map[j].style == syms[i]) { n = map[j].name; break; }
            PyList_SET_ITEM(lst, i, PyUnicode_FromString(n));
        }
        sipRes = lst;
    }
  %End

  int symbolIndex(const QString& name) const;
  %MethodCode
    {
        struct { int style; const char *name; } map[] = {
            {-1, "no symbol"}, {0, "ellipse"}, {0, "circle"},
            {1, "rectangle"}, {2, "diamond"}, {3, "triangle"},
            {4, "down triangle"}, {5, "up triangle"}, {6, "left triangle"},
            {7, "right triangle"}, {8, "cross"}, {9, "diagonal cross"},
            {10, "horizontal line"}, {11, "vertical line"},
            {12, "star 1"}, {13, "star 2"}, {14, "hexagon"}
        };
        QString lower = a0->toLower();
        sipRes = -1;
        for (int j = 0; j < 17; j++)
        {
            if (lower == QLatin1String(map[j].name))
            {
                sipRes = map[j].style;
                break;
            }
        }
    }
  %End

  // table/data control
  bool autoUpdateTableValues();
  void setAutoUpdateTableValues(bool on = true);
  QStringList columnsList(Table::PlotDesignation type = Table::All);

private:
  ApplicationWindow(const ApplicationWindow&);
};

class Fit : Filter
{
%TypeHeaderCode
#include "Fit.h"
%End
public:
  enum Algorithm{ScaledLevenbergMarquardt, UnscaledLevenbergMarquardt, NelderMeadSimplex};
  enum WeightingMethod{NoWeighting, Instrumental, Statistical, Dataset, Direct};

  Fit(ApplicationWindow* /TransferThis/, Graph*=0, const char*=0);
  ~Fit();

  virtual void fit();
  virtual bool run();

  bool setWeightingData(WeightingMethod, const QString&=QString());
  bool setDataFromTable(Table *, const QString&, const QString&, int = 1, int = -1, bool = false);

  void setInterval(double from, double to);

  QString formula();
  QString resultFormula();
  int numParameters();
  QStringList parameterNames();

  void setInitialGuess(int, double) /PyName=setInitialValue/;
  void setInitialGuesses(...) /PyName=setInitialValues/;
%MethodCode
int n = PyTuple_GET_SIZE(a0);
double *values = new double[n];
for (int i=0; i<n; i++) {
  PyObject *item = PyTuple_GET_ITEM(a0, i);
  if (PyNumber_Check(item)) {
    item=PyNumber_Float(item);
    if (!item) {
      sipIsErr=1;
      break;
    }
    values[i] = PyFloat_AS_DOUBLE(item);
    Py_DECREF(item);
  } else
    values[i] = 0;
}
sipCpp->setInitialGuesses(values);
delete[] values;
%End

  virtual void guessInitialValues();
  void setParameterRange(int, double, double);

  void setAlgorithm(Algorithm);
  void setOutputPrecision(int);
  void generateFunction(bool, int=100);

  void showLegend();
  virtual QString legendInfo();

  void scaleErrors(bool yes = true);

  SIP_PYTUPLE results();
%MethodCode
double *results = sipCpp->results();
int size=sipCpp->numParameters();
sipRes = PyTuple_New(size);
if(sipRes)
{
  for(int i=0; i<size; i++)
    PyTuple_SET_ITEM(sipRes, i, PyFloat_FromDouble(results[i]));
} else
  sipIsErr = 1;
%End

  SIP_PYTUPLE errors();
%MethodCode
double *errors = sipCpp->errors();
int size=sipCpp->numParameters();
sipRes = PyTuple_New(size);
if(sipRes)
{
  for(int i=0; i<size; i++)
    PyTuple_SET_ITEM(sipRes, i, PyFloat_FromDouble(errors[i]));
} else
  sipIsErr = 1;
%End

	SIP_PYTUPLE residuals();
%MethodCode
double *residuals = sipCpp->residuals();
int size = sipCpp->dataSize();
sipRes = PyTuple_New(size);
if(sipRes)
{
  for(int i=0; i<size; i++)
    PyTuple_SET_ITEM(sipRes, i, PyFloat_FromDouble(residuals[i]));
} else
  sipIsErr = 1;
%End

  double chiSquare();
  double rSquare();
  double adjustedRSquare();
  double rss();
  double rmse();

  void showPredictionLimits(double);
  void showConfidenceLimits(double);
  double lcl(int, double);
  double ucl(int, double);

  Table* parametersTable(const QString&);
  Matrix* covarianceMatrix(const QString&);
};

%ModuleCode
ApplicationWindow *sipqti_app()
{
  int iserr = 0;
  PyObject *me = PyImport_ImportModule("qti");
  PyObject *mydict = PyModule_GetDict(me);
  PyObject *pyapp = PyDict_GetItemString(mydict,"app");
  Py_DECREF(me);
  if (sipCanConvertToType(pyapp, sipType_ApplicationWindow, SIP_NOT_NONE))
    return (ApplicationWindow*) sipConvertToType(pyapp, sipType_ApplicationWindow, nullptr, SIP_NOT_NONE, nullptr, &iserr);
  else
    return nullptr;
}
%End
%ModuleHeaderCode
class ApplicationWindow;
ApplicationWindow *sipqti_app();
#define SIPQTI_APP(sipcppexpr)\
ApplicationWindow *app = sipqti_app();\
if (app) sipCpp = sipcppexpr;\
else { sipCpp = nullptr; }
#include "Graph.h"
%End

%PostInitialisationCode
{
    PyObject *graphType   = (PyObject *)sipTypeAsPyTypeObject(sipType_Graph);
    PyObject *types_mod   = PyImport_ImportModule("types");
    PyObject *sns_class   = PyObject_GetAttrString(types_mod, "SimpleNamespace");
    PyObject *empty_tuple = PyTuple_New(0);
    PyObject *d, *ns;
    Py_DECREF(types_mod);

    // Graph.Axis
    d = PyDict_New();
    PyDict_SetItemString(d, "Left",   PyLong_FromLong(::Graph::Left));
    PyDict_SetItemString(d, "Right",  PyLong_FromLong(::Graph::Right));
    PyDict_SetItemString(d, "Bottom", PyLong_FromLong(::Graph::Bottom));
    PyDict_SetItemString(d, "Top",    PyLong_FromLong(::Graph::Top));
    ns = PyObject_Call(sns_class, empty_tuple, d); Py_DECREF(d);
    PyObject_SetAttrString(graphType, "Axis", ns); Py_DECREF(ns);

    // Graph.Scale
    d = PyDict_New();
    PyDict_SetItemString(d, "Linear",      PyLong_FromLong(::Graph::Linear));
    PyDict_SetItemString(d, "Log10",       PyLong_FromLong(::Graph::Log10));
    PyDict_SetItemString(d, "Ln",          PyLong_FromLong(::Graph::Ln));
    PyDict_SetItemString(d, "Log2",        PyLong_FromLong(::Graph::Log2));
    PyDict_SetItemString(d, "Reciprocal",  PyLong_FromLong(::Graph::Reciprocal));
    PyDict_SetItemString(d, "Probability", PyLong_FromLong(::Graph::Probability));
    PyDict_SetItemString(d, "Logit",       PyLong_FromLong(::Graph::Logit));
    ns = PyObject_Call(sns_class, empty_tuple, d); Py_DECREF(d);
    PyObject_SetAttrString(graphType, "Scale", ns); Py_DECREF(ns);

    // Graph.Ticks
    d = PyDict_New();
    PyDict_SetItemString(d, "NoTicks", PyLong_FromLong(::Graph::NoTicks));
    PyDict_SetItemString(d, "Out",     PyLong_FromLong(::Graph::Out));
    PyDict_SetItemString(d, "InOut",   PyLong_FromLong(::Graph::InOut));
    PyDict_SetItemString(d, "In",      PyLong_FromLong(::Graph::In));
    ns = PyObject_Call(sns_class, empty_tuple, d); Py_DECREF(d);
    PyObject_SetAttrString(graphType, "Ticks", ns); Py_DECREF(ns);

    // Graph.CurveType — plain-int SimpleNamespace so values pass Python 3.12+ strict int checks
    d = PyDict_New();
    PyDict_SetItemString(d, "Line",              PyLong_FromLong(::Graph::Line));
    PyDict_SetItemString(d, "Scatter",           PyLong_FromLong(::Graph::Scatter));
    PyDict_SetItemString(d, "LineSymbols",       PyLong_FromLong(::Graph::LineSymbols));
    PyDict_SetItemString(d, "VerticalBars",      PyLong_FromLong(::Graph::VerticalBars));
    PyDict_SetItemString(d, "Area",              PyLong_FromLong(::Graph::Area));
    PyDict_SetItemString(d, "Pie",               PyLong_FromLong(::Graph::Pie));
    PyDict_SetItemString(d, "VerticalDropLines", PyLong_FromLong(::Graph::VerticalDropLines));
    PyDict_SetItemString(d, "Spline",            PyLong_FromLong(::Graph::Spline));
    PyDict_SetItemString(d, "HorizontalSteps",   PyLong_FromLong(::Graph::HorizontalSteps));
    PyDict_SetItemString(d, "Histogram",         PyLong_FromLong(::Graph::Histogram));
    PyDict_SetItemString(d, "HorizontalBars",    PyLong_FromLong(::Graph::HorizontalBars));
    PyDict_SetItemString(d, "VectXYXY",          PyLong_FromLong(::Graph::VectXYXY));
    PyDict_SetItemString(d, "ErrorBars",         PyLong_FromLong(::Graph::ErrorBars));
    PyDict_SetItemString(d, "Box",               PyLong_FromLong(::Graph::Box));
    PyDict_SetItemString(d, "VectXYAM",          PyLong_FromLong(::Graph::VectXYAM));
    PyDict_SetItemString(d, "VerticalSteps",     PyLong_FromLong(::Graph::VerticalSteps));
    PyDict_SetItemString(d, "ColorMap",          PyLong_FromLong(::Graph::ColorMap));
    PyDict_SetItemString(d, "GrayScale",         PyLong_FromLong(::Graph::GrayScale));
    PyDict_SetItemString(d, "Contour",           PyLong_FromLong(::Graph::Contour));
    PyDict_SetItemString(d, "Function",          PyLong_FromLong(::Graph::Function));
    PyDict_SetItemString(d, "ImagePlot",         PyLong_FromLong(::Graph::ImagePlot));
    PyDict_SetItemString(d, "StackBar",          PyLong_FromLong(::Graph::StackBar));
    PyDict_SetItemString(d, "StackColumn",       PyLong_FromLong(::Graph::StackColumn));
    ns = PyObject_Call(sns_class, empty_tuple, d); Py_DECREF(d);
    PyObject_SetAttrString(graphType, "CurveType", ns); Py_DECREF(ns);

    Py_DECREF(empty_tuple);
    Py_DECREF(sns_class);

    // Qti3D — module-level namespace with PLOTSTYLE constants
    {
        PyObject *qti3d_d = PyDict_New();
        PyDict_SetItemString(qti3d_d, "NOPLOT",     PyLong_FromLong(0));
        PyDict_SetItemString(qti3d_d, "WIREFRAME",  PyLong_FromLong(1));
        PyDict_SetItemString(qti3d_d, "HIDDENLINE", PyLong_FromLong(2));
        PyDict_SetItemString(qti3d_d, "FILLED",     PyLong_FromLong(3));
        PyDict_SetItemString(qti3d_d, "FILLEDMESH", PyLong_FromLong(4));
        PyDict_SetItemString(qti3d_d, "POINTS",     PyLong_FromLong(5));
        PyDict_SetItemString(qti3d_d, "USER",       PyLong_FromLong(6));
        PyObject *qti3d_name  = PyUnicode_FromString("Qti3D");
        PyObject *qti3d_bases = PyTuple_New(0);
        PyObject *qti3d_args  = PyTuple_Pack(3, qti3d_name, qti3d_bases, qti3d_d);
        PyObject *qti3d_cls   = PyObject_Call((PyObject *)&PyType_Type, qti3d_args, NULL);
        Py_DECREF(qti3d_name);
        Py_DECREF(qti3d_bases);
        Py_DECREF(qti3d_args);
        Py_DECREF(qti3d_d);
        if (qti3d_cls)
            PyModule_AddObject(sipModule, "Qti3D", qti3d_cls);
    }
}
%End

class ExponentialFit : Fit
{
%TypeHeaderCode
#include "ExponentialFit.h"
%End
public:
  ExponentialFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, bool);
  ExponentialFit(Table *, const QString&, const QString&, int = 1, int = -1, bool = false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipExponentialFit(app, a0, *a1, *a2, a3, a4, a5))
%End

  ExponentialFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, bool=false);
  ExponentialFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double, bool=false);
  ExponentialFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, bool=false);
  ExponentialFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double, bool=false);
  ExponentialFit(QwtPlotCurve *, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipExponentialFit(app, a0, a1))
%End
  ExponentialFit(QwtPlotCurve *, double, double, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipExponentialFit(app, a0, a1, a2, a3))
%End
  ExponentialFit(Graph *, const QString&, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipExponentialFit(app, a0, *a1, a2))
%End
  ExponentialFit(Graph *, const QString&, double, double, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipExponentialFit(app, a0, *a1, a2, a3, a4))
%End
};

class TwoExpFit : Fit
{
%TypeHeaderCode
#include "ExponentialFit.h"
%End
public:
  TwoExpFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  TwoExpFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipTwoExpFit(app, a0, *a1, *a2, a3, a4))
%End

  TwoExpFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  TwoExpFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  TwoExpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  TwoExpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  TwoExpFit(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipTwoExpFit(app, a0))
%End
  TwoExpFit(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipTwoExpFit(app, a0, a1, a2))
%End
  TwoExpFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipTwoExpFit(app, a0, *a1))
%End
  TwoExpFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipTwoExpFit(app, a0, *a1, a2, a3))
%End
};

class ThreeExpFit : Fit
{
%TypeHeaderCode
#include "ExponentialFit.h"
%End
public:
  ThreeExpFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  ThreeExpFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipThreeExpFit(app, a0, *a1, *a2, a3, a4))
%End

  ThreeExpFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  ThreeExpFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  ThreeExpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  ThreeExpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  ThreeExpFit(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipThreeExpFit(app, a0))
%End
  ThreeExpFit(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipThreeExpFit(app, a0, a1, a2))
%End
  ThreeExpFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipThreeExpFit(app, a0, *a1))
%End
  ThreeExpFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipThreeExpFit(app, a0, *a1, a2, a3))
%End
};

class SigmoidalFit : Fit
{
%TypeHeaderCode
#include "SigmoidalFit.h"
%End
public:
  SigmoidalFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  SigmoidalFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSigmoidalFit(app, a0, *a1, *a2, a3, a4))
%End

  SigmoidalFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  SigmoidalFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  SigmoidalFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  SigmoidalFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  SigmoidalFit(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSigmoidalFit(app, a0))
%End
  SigmoidalFit(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSigmoidalFit(app, a0, a1, a2))
%End
  SigmoidalFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSigmoidalFit(app, a0, *a1))
%End
  SigmoidalFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSigmoidalFit(app, a0, *a1, a2, a3))
%End

  void guessInitialValues();
};

class LogisticFit : Fit
{
%TypeHeaderCode
#include "LogisticFit.h"
%End
public:
  LogisticFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  LogisticFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLogisticFit(app, a0, *a1, *a2, a3, a4))
%End

  LogisticFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  LogisticFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  LogisticFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  LogisticFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  LogisticFit(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLogisticFit(app, a0))
%End
  LogisticFit(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLogisticFit(app, a0, a1, a2))
%End
  LogisticFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLogisticFit(app, a0, *a1))
%End
  LogisticFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLogisticFit(app, a0, *a1, a2, a3))
%End

  void guessInitialValues();
};

class GaussAmpFit : Fit
{
%TypeHeaderCode
#include "MultiPeakFit.h"
%End
public:
  GaussAmpFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  GaussAmpFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussAmpFit(app, a0, *a1, *a2, a3, a4))
%End

  GaussAmpFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  GaussAmpFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  GaussAmpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  GaussAmpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  GaussAmpFit(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussAmpFit(app, a0))
%End
  GaussAmpFit(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussAmpFit(app, a0, a1, a2))
%End
  GaussAmpFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussAmpFit(app, a0, *a1))
%End
  GaussAmpFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussAmpFit(app, a0, *a1, a2, a3))
%End

  void guessInitialValues();
};

class NonLinearFit : Fit
{
%TypeHeaderCode
#include "NonLinearFit.h"
%End
public:
  NonLinearFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  NonLinearFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipNonLinearFit(app, a0, *a1, *a2, a3, a4))
%End

  NonLinearFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  NonLinearFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  NonLinearFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  NonLinearFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  NonLinearFit(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipNonLinearFit(app, a0))
%End
  NonLinearFit(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipNonLinearFit(app, a0, a1, a2))
%End
  NonLinearFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipNonLinearFit(app, a0, *a1))
%End
  NonLinearFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipNonLinearFit(app, a0, *a1, a2, a3))
%End
  void setParameters(...);
%MethodCode
  QStringList l;
  const char *item;
  for (int i=0; i<PyTuple_GET_SIZE(a0); i++)
    item = PyUnicode_AsUTF8(PyTuple_GET_ITEM(a0, i));
    if (item)
      l << item;
    else
      sipIsErr = 1;

  sipCpp->setParametersList(l);
%End
  // TODO: make it accept Python callables
  bool setFormula(const QString&, bool = true);
  bool save(const QString&);
  bool load(const QString&);
};

class PluginFit : Fit
{
%TypeHeaderCode
#include "PluginFit.h"
%End
public:
  PluginFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  PluginFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPluginFit(app, a0, *a1, *a2, a3, a4))
%End

  PluginFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  PluginFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  PluginFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  PluginFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  PluginFit(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPluginFit(app, a0))
%End
  PluginFit(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPluginFit(app, a0, a1, a2))
%End
  PluginFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPluginFit(app, a0, *a1))
%End
  PluginFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPluginFit(app, a0, *a1, a2, a3))
%End
  bool load(const QString&);
};

class MultiPeakFit : Fit
{
%TypeHeaderCode
#include "MultiPeakFit.h"
%End
public:
  enum PeakProfile{Gauss, Lorentz};
  MultiPeakFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, PeakProfile=Gauss, int=1);
  MultiPeakFit(ApplicationWindow * /TransferThis/, Graph *, PeakProfile=Gauss, int=1);

  int peaks() /PyName=numPeaks/;
  void setNumPeaks(int);

  void enablePeakCurves(bool);
  void setPeakCurvesColor(int);

  static QString generateFormula(int, PeakProfile);
  static QStringList generateParameterList(int);

  void guessInitialValues();
};

class LorentzFit : MultiPeakFit
{
%TypeHeaderCode
#include "MultiPeakFit.h"
%End
public:
  LorentzFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  LorentzFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLorentzFit(app, a0, *a1, *a2, a3, a4))
%End

  LorentzFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  LorentzFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  LorentzFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  LorentzFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  LorentzFit(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLorentzFit(app, a0))
%End
  LorentzFit(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLorentzFit(app, a0, a1, a2))
%End
  LorentzFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLorentzFit(app, a0, *a1))
%End
  LorentzFit(Graph *, const QString&, int, int) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLorentzFit(app, a0, *a1, a2, a3))
%End
};

class GaussFit : MultiPeakFit
{
%TypeHeaderCode
#include "MultiPeakFit.h"
%End
public:
  GaussFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  GaussFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussFit(app, a0, *a1, *a2, a3, a4))
%End

  GaussFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  GaussFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  GaussFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  GaussFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  GaussFit(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussFit(app, a0))
%End
  GaussFit(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussFit(app, a0, a1, a2))
%End
  GaussFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussFit(app, a0, *a1))
%End
  GaussFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussFit(app, a0, *a1, a2, a3))
%End
};

class PolynomialFit : Fit
{
%TypeHeaderCode
#include "PolynomialFit.h"
%End
public:
  PolynomialFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, int, bool);
  PolynomialFit(Table *, const QString&, const QString&, int = 1, int = -1, int = 2, bool = false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPolynomialFit(app, a0, *a1, *a2, a3, a4, a5, a6))
%End

  PolynomialFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, int=2, bool=false);
  PolynomialFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double, int=2, bool=false);
  PolynomialFit(ApplicationWindow * /TransferThis/, Graph *, QString&, int=2, bool=false);
  PolynomialFit(ApplicationWindow * /TransferThis/, Graph *, QString&, double, double, int=2, bool=false);
  PolynomialFit(QwtPlotCurve *, int=2, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPolynomialFit(app, a0, a1, a2))
%End
  PolynomialFit(QwtPlotCurve *, double, double, int=2, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPolynomialFit(app, a0, a1, a2, a3, a4))
%End
  PolynomialFit(Graph *, QString&, int=2, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPolynomialFit(app, a0, *a1, a2, a3))
%End
  PolynomialFit(Graph *, QString&, double, double, int=2, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPolynomialFit(app, a0, *a1, a2, a3, a4, a5))
%End

  virtual QString legendInfo();
  void fit();

  static QString generateFormula(int);
  static QStringList generateParameterList(int);
};

class LinearFit : Fit
{
%TypeHeaderCode
#include "PolynomialFit.h"
%End
public:
  LinearFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  LinearFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLinearFit(app, a0, *a1, *a2, a3, a4))
%End

  LinearFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  LinearFit(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  LinearFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  LinearFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  LinearFit(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLinearFit(app, a0))
%End
  LinearFit(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLinearFit(app, a0, a1, a2))
%End
  LinearFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLinearFit(app, a0, *a1))
%End
  LinearFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLinearFit(app, a0, *a1, a2, a3))
%End
  void fit();
};

class Filter : QObject
{
%TypeHeaderCode
#include "Filter.h"
%End
public:
  Filter(ApplicationWindow* /TransferThis/, Graph*=0, const char*=0);
  ~Filter();

  void setOutputPoints(int);
  void setTolerance(double);
  void setMaximumIterations(int);

  void setColor(int);
  void setColor(const QString&);
  void setColor(const QColor&);

  bool setDataFromCurve(QwtPlotCurve *c);
  bool setDataFromCurve(QwtPlotCurve *c, double from, double to);
  bool setDataFromCurve(const QString&, Graph*=0);
  bool setDataFromCurve(const QString& curveTitle, double from, double to, Graph *g = 0);
  virtual bool setDataFromTable(Table *t, const QString& xColName, const QString& yColName, int from = 1, int to = -1, bool = false);

  Table *resultTable();

  virtual void enableGraphicsDisplay(bool on = true, Graph *g = 0);
  virtual bool run();
};

class Differentiation : Filter
{
%TypeHeaderCode
#include "Differentiation.h"
%End
public:
  Differentiation(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  Differentiation(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipDifferentiation(app, a0, *a1, *a2, a3, a4))
%End

  Differentiation(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  Differentiation(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  Differentiation(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  Differentiation(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  Differentiation(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipDifferentiation(app, a0, a1, a2))
%End
  Differentiation(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipDifferentiation(app, a0))
%End
  Differentiation(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipDifferentiation(app, a0, *a1))
%End
  Differentiation(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipDifferentiation(app, a0, *a1, a2, a3))
%End
  bool run();
};

class Integration : Filter
{
%TypeHeaderCode
#include "Integration.h"
%End
public:
  Integration(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, bool);
  Integration(Table *, const QString&, const QString&, int = 1, int = -1, bool = false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipIntegration(app, a0, *a1, *a2, a3, a4, a5))
%End

  Integration(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  Integration(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  Integration(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  Integration(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  Integration(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipIntegration(app, a0))
%End
  Integration(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipIntegration(app, a0, a1, a2))
%End
  Integration(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipIntegration(app, a0, *a1))
%End
  Integration(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipIntegration(app, a0, *a1, a2, a3))
%End

  void enableGraphicsDisplay(bool on = true, Graph *g = 0);
  bool run();

  double area();
};

class Interpolation : Filter
{
%TypeHeaderCode
#include "Interpolation.h"
%End
public:
  enum InterpolationMethod{Linear, Cubic, Akima};

  Interpolation(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, int);
  Interpolation(Table *, const QString&, const QString&, int = 1, int = -1, int = 0) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipInterpolation(app, a0, *a1, *a2, a3, a4, a5))
%End

  Interpolation(ApplicationWindow * /TransferThis/, QwtPlotCurve *, int=0);
  Interpolation(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double, int=0);
  Interpolation(ApplicationWindow * /TransferThis/, Graph *, const QString&, int=0);
  Interpolation(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double, int=0);
  Interpolation(QwtPlotCurve *, int=0) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipInterpolation(app, a0, a1))
%End
  Interpolation(QwtPlotCurve *, double, double, int=0) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipInterpolation(app, a0, a1, a2, a3))
%End
  Interpolation(Graph *, const QString&, int=0) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipInterpolation(app, a0, *a1, a2))
%End
  Interpolation(Graph *, const QString&, double, double, int=0) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipInterpolation(app, a0, *a1, a2, a3, a4))
%End

  void setMethod(int n);
  bool run();
};

class SmoothFilter : Filter
{
%TypeHeaderCode
#include "SmoothFilter.h"
%End
public:
  enum SmoothMethod{SavitzkyGolay = 1, FFT = 2, Average = 3, Lowess = 4};

  SmoothFilter(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, int);
  SmoothFilter(Table *, const QString&, const QString&, int = 1, int = -1, int = 3) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSmoothFilter(app, a0, *a1, *a2, a3, a4, a5))
%End

  SmoothFilter(ApplicationWindow * /TransferThis/, QwtPlotCurve *, int = 3);
  SmoothFilter(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double, int = 3);
  SmoothFilter(ApplicationWindow * /TransferThis/, Graph *, const QString&, int=3);
  SmoothFilter(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double, int=3);
  SmoothFilter(QwtPlotCurve *, int=3) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSmoothFilter(app, a0, a1))
%End
  SmoothFilter(QwtPlotCurve *, double, double, int=3) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSmoothFilter(app, a0, a1, a2, a3))
%End
  SmoothFilter(Graph *, const QString&, int=3) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSmoothFilter(app, a0, *a1, a2))
%End
  SmoothFilter(Graph *, const QString&, double, double, int=3) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSmoothFilter(app, a0, *a1, a2, a3, a4))
%End

  void setMethod(int);
  void setSmoothPoints(int, int = 0);
  void setPolynomOrder(int);
  void setLowessParameter(double f, int iterations);
  bool run();
};

class FFTFilter : Filter
{
%TypeHeaderCode
#include "FFTFilter.h"
%End
public:
  enum FilterType{LowPass = 1, HighPass = 2, BandPass = 3, BandBlock = 4};

  FFTFilter(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, int);
  FFTFilter(Table *, const QString&, const QString&, int = 1, int = -1, int = 1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFTFilter(app, a0, *a1, *a2, a3, a4, a5))
%End

  FFTFilter(ApplicationWindow * /TransferThis/, QwtPlotCurve *, int = 1);
  FFTFilter(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double, int = 1);
  FFTFilter(ApplicationWindow * /TransferThis/, Graph *, const QString&, int=1);
  FFTFilter(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double, int=1);
  FFTFilter(QwtPlotCurve *, int=1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFTFilter(app, a0, a1))
%End
  FFTFilter(QwtPlotCurve *, double, double, int=1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFTFilter(app, a0, a1, a2, a3))
%End
  FFTFilter(Graph *, const QString&, int=1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFTFilter(app, a0, *a1, a2))
%End
  FFTFilter(Graph *, const QString&, double, double, int=1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFTFilter(app, a0, *a1, a2, a3, a4))
%End

  void setFilterType(int);
  void setCutoff(double);
  void setBand(double, double);
  void enableOffset(bool=true);

  bool run();
};

class FFT : Filter
{
%TypeHeaderCode
#include "../../../src/analysis/FFT.h"
%End
public:

  FFT(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString& = QString(), int = 1, int = -1);
  FFT(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  FFT(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  FFT(ApplicationWindow * /TransferThis/, QwtPlotCurve *);
  FFT(ApplicationWindow * /TransferThis/, QwtPlotCurve *, double, double);
  FFT(ApplicationWindow * /TransferThis/, Matrix *, Matrix * = nullptr, bool = false, bool = true, bool = false, bool = true);

  FFT(Matrix *, Matrix * = nullptr, bool = false, bool = true, bool = false, bool = true) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFT(app, a0, a1, a2, a3, a4, a5))
%End

  FFT(Table *, const QString&, const QString& = QString(), int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFT(app, a0, *a1, *a2, a3, a4))
%End
  FFT(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFT(app, a0, *a1))
%End
  FFT(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFT(app, a0, *a1, a2, a3))
%End
  FFT(QwtPlotCurve *) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFT(app, a0))
%End
  FFT(QwtPlotCurve *, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFT(app, a0, a1, a2))
%End

  void setInverseFFT(bool=true);
  void setSampling(double);
  void normalizeAmplitudes(bool=true);
  void shiftFrequencies(bool=true);

  Matrix *amplitudesMatrix();
  Matrix *realOutputMatrix();
  Matrix *imaginaryOutputMatrix();

  bool run();
};

class Correlation : Filter
{
%TypeHeaderCode
#include "../../../src/analysis/Correlation.h"
%End
public:

  Correlation(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  Correlation(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipCorrelation(app, a0, *a1, *a2, a3, a4))
%End

  bool setDataFromTable(Table *, const QString&, const QString&, int = 1, int = -1);
  bool run();
};

class Convolution : Filter
{
%TypeHeaderCode
#include "Convolution.h"
%End
public:

  Convolution(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&);
  Convolution(Table *, const QString&, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipConvolution(app, a0, *a1, *a2))
%End

  void setDataFromTable(Table *, const QString&, const QString&);
  bool run();
};

class Deconvolution : Filter
{
%TypeHeaderCode
#include "Convolution.h"
%End
public:

  Deconvolution(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&);
  Deconvolution(Table *, const QString&, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipDeconvolution(app, a0, *a1, *a2))
%End

  void setDataFromTable(Table *, const QString&, const QString&);
  bool run();
};

class Statistics : QObject
{
%TypeHeaderCode
#include <Statistics.h>
%End
public:
  Statistics(ApplicationWindow * /TransferThis/, const QString& = QString());
  Statistics(const QString& = QString()) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipStatistics(app, *a0))
%End
  ~Statistics();

  bool setData(const QString&);
  void showResultsLog(bool = true);

  //! Returns the size of the input data set
  unsigned int dataSize();
  //! Returns the degrees of freedom
  virtual int dof();

  double mean();
  double variance();
  double standardDeviation();
  double standardError();

  QString logInfo(bool header = true);
  virtual bool run();
};

class StatisticTest : Statistics
{
%TypeHeaderCode
#include <StatisticTest.h>
%End
public:
  StatisticTest(ApplicationWindow * /TransferThis/, double = 0.0, double = 0.05, const QString& = QString());
  StatisticTest(double = 0.0, double = 0.05, const QString& = QString()) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipStatisticTest(app, a0, a1, *a2))
%End
  enum Tail{Left = 0, Right = 1, Both = 2};

  void setTail(const Tail&);
  void setTestValue(double);
  void setSignificanceLevel(double);
  void showDescriptiveStatistics(bool show = true);

  Table* resultTable(const QString& = QString());
  virtual QString logInfo();
  virtual double statistic();
  virtual bool run();
};

class tTest : StatisticTest
{
%TypeHeaderCode
#include <tTest.h>
%End
public:

  tTest(ApplicationWindow * /TransferThis/, double, double, const QString& = QString(), const QString& = QString(), bool = false);
  tTest(double, double, const QString& = QString(), const QString& = QString(), bool = false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new siptTest(app, a0, a1, *a2, *a3, a4))
%End

  bool setSample1(const QString&);
  bool setSample2(const QString&, bool = false);

  virtual QString logInfo();
  double t();
  double statistic();
  double pValue();
  int dof();
  double power(double alpha, int size = 0);
  //! Lower Confidence Limit
  double lcl(double);
  //! Upper Confidence Limit
  double ucl(double);
};

class ChiSquareTest : StatisticTest
{
%TypeHeaderCode
#include <ChiSquareTest.h>
%End
public:
  ChiSquareTest(ApplicationWindow * /TransferThis/, double testValue, double level, const QString& = QString());
  ChiSquareTest(double testValue, double level, const QString& = QString()) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipChiSquareTest(app, a0, a1, *a2))
%End

  virtual QString logInfo();
  double chiSquare();
  double statistic();
  double pValue();
};

class ShapiroWilkTest : StatisticTest
{
%TypeHeaderCode
#include <ShapiroWilkTest.h>
%End
public:
  ShapiroWilkTest(ApplicationWindow * /TransferThis/, const QString& = QString());
  ShapiroWilkTest(const QString& = QString()) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipShapiroWilkTest(app, *a0))
%End

  virtual QString logInfo();
  double w();
  double statistic();
  double pValue();
};

class Anova : StatisticTest
{
%TypeHeaderCode
#include <Anova.h>
%End
public:
  Anova(ApplicationWindow * /TransferThis/, bool twoWay = false, double level = 0.05);
  Anova(bool twoWay = false, double level = 0.05) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipAnova(app, a0, a1))
%End

  bool run();
  bool addSample(const QString& colName, int aLevel = 1, int bLevel = 1);
  void setAnovaTwoWayModel(int);

  virtual QString logInfo();
  double fStat();
  double statistic();
  double pValue();
  //! Sum of squares
  double ssm();
  double sse();
  double sst();
  //! Mean square value
  double mse();
  //! F statistic for factor A
  double fStatA();
  //! F statistic for factor B
  double fStatB();
  //! F statistic for the interaction
  double fStatAB();
  //! P value for factor A
  double pValueA();
  //! P value for factor B
  double pValueB();
  //! P value for the interaction
  double pValueAB();
  //! Sum of squares for factor A
  double ssa();
  //! Sum of squares for factor B
  double ssb();
  //! Sum of squares for the interaction
  double ssab();
  //! Mean square value for factor A
  double msa();
  //! Mean square value for factor B
  double msb();
  //! Mean square value for the interaction
  double msab();
};

// used for output redirection
class PythonScripting
{
%TypeHeaderCode
#include "PythonScripting.h"
%End
public:
  void write(const QString&);
private:
  PythonScripting(const PythonScripting&);
};
class PythonScript
{
%TypeHeaderCode
#include "PythonScript.h"
%End
public:
  void write(const QString&);
private:
  PythonScript(const PythonScript&);
};

class Folder : QObject
{
%TypeHeaderCode
#include "Folder.h"
%End
public:
  QList<MdiSubWindow*> windowsList() /PyName=windows/;
// TODO: implement signal Folder::nameChanged and make it update the project explorer; adjust renaming from GUI accordingly
//  void setFolderName(const QString&) /PyName=setName/;
  QString name();
  QString path();

  QList<Folder*> folders();
  Folder *findSubfolder(const QString&, bool=true, bool=false)  /PyName=folder/;
  MdiSubWindow* findWindow(const QString&, bool=true, bool=true, bool=false, bool=true);

  MdiSubWindow *window(const QString &name, const char *cls="MdiSubWindow", bool recursive=false);
  Table *table(const QString &name, bool recursive=false);
  Matrix *matrix(const QString &name, bool recursive=false);
  MultiLayer *graph(const QString &name, bool recursive=false);
  Note *note(const QString &name, bool recursive=false);
  Graph3D *plot3D(const QString &name, bool recursive=false);

  Folder* rootFolder();

  QString logInfo();
  void appendLogInfo(const QString&);
  void clearLogInfo();

private:
  Folder(const Folder&);
};
