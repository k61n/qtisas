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
  // A failure inside the tab's own script raises here, at the call — confirmed live (series 4/5, local, LIST 5 item 5):
  // a tab's script failed with nothing reaching the calling script, which then read an empty table twice.
  void executeAll() /PyName=execute/;
%MethodCode
    sipCpp->executeAll();
    if (sipCpp->lastRunFailed())
    {
        sipIsErr = 1;
        const QString msg = "the tab's script failed: " + sipCpp->errorMessage().trimmed();
        PyErr_SetString(PyExc_RuntimeError, msg.toUtf8().constData());
    }
%End
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
#include "numericLocale.h"
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
            PyErr_Format(PyExc_ValueError, "There's no column %d in table %s! It has %d columns: %s.", col+1,\
                         sipCpp->objectName().toLocal8Bit().constData(), sipCpp->numCols(),\
                         sipCpp->colNames().join(", ").toLocal8Bit().constData());\
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
				PyErr_Format(PyExc_ValueError, "There's no column named %s in table %s! Its columns: %s.",\
                        PyUnicode_AsUTF8(tmp), sipCpp->objectName().toLocal8Bit().constData(),\
                        sipCpp->colNames().join(", ").toLocal8Bit().constData());\
				Py_DECREF(tmp);\
			}\
		}\
	 }

#define CHECK_TABLE_ROW(arg)\
   int row = arg-1;\
	if (row < 0 || row >= sipCpp->numRows()) {\
		sipIsErr = 1;\
        PyErr_Format(PyExc_ValueError, "There's no row %d in table %s! It has %d rows.", row+1,\
                     sipCpp->objectName().toLocal8Bit().constData(), sipCpp->numRows());\
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
    {
      Py_INCREF(Py_None); // a new reference: the caller owns what it gets
      *item = Py_None;
    }
    else
      *item = PyFloat_FromDouble(table->cell(row, col));
    break;
  case Table::Text:
    *item = PyUnicode_FromString(table->text(row, col).toLocal8Bit().constData());
    break;
  case Table::Date:
    if (table->text(row, col).isEmpty())
    {
      Py_INCREF(Py_None); // a new reference: the caller owns what it gets
      *item = Py_None;
    }
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
    {
      Py_INCREF(Py_None); // a new reference: the caller owns what it gets
      *item = Py_None;
    }
    else {
      if (!PyDateTimeAPI)
        PyDateTime_IMPORT;
      QTime time = QTime::fromString(table->text(row, col), table->columnFormat(col));
      *item = PyTime_FromTime(time.hour(), time.minute(), time.second(), time.msec()*1000);
    }
    break;
  case Table::Month:
    if (table->text(row, col).isEmpty())
    {
      Py_INCREF(Py_None); // a new reference: the caller owns what it gets
      *item = Py_None;
    }
    else {
      QDate date = QDate::fromString(table->text(row, col), table->columnFormat(col));
      *item = PyLong_FromLong(date.month());
    }
    break;
  case Table::Day:
    if (table->text(row, col).isEmpty())
    {
      Py_INCREF(Py_None); // a new reference: the caller owns what it gets
      *item = Py_None;
    }
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
%MethodCode
  // interactive=false: a blocking QMessageBox on read-only columns would hang an automated
  // script — raise ValueError instead of silently aborting the deletion.
  if (!sipCpp->deleteRows(a0, a1, false)) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError,
        QString("deleteRows: table '%1' contains read-only columns, operation aborted.")
            .arg(sipCpp->objectName()).toLocal8Bit().constData());
  }
%End
  void __delitem__(int);
%MethodCode
  int len = sipCpp->numRows();
  if ((a0 = (int)sipConvertFromSequenceIndex(a0, len)) < 0)
    sipIsErr = 1;
  else if (!sipCpp->deleteRows(a0 + 1, a0 + 1, false)) { // start and stop are equal, counting starts at 1
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError,
        QString("del: table '%1' contains read-only columns, operation aborted.")
            .arg(sipCpp->objectName()).toLocal8Bit().constData());
  }
%End
  void __delitem__(SIP_PYSLICE);
%MethodCode
  Py_ssize_t len, start, stop, step, slicelength;
  len = sipCpp->numRows();
  if (PySlice_GetIndicesEx((PyObject*)a0, len, &start, &stop, &step, &slicelength) < 0)
    sipIsErr = 1;
  else {
    start += 1; // row index starts at 1 (python at 0); the stop is inclusive (python exclusive) so no need to change
    bool ok = true;
    if (step == 1)
      ok = sipCpp->deleteRows(start, stop, false);
    else {
      for (int i = 0; i < slicelength && ok; i++) {
        ok = sipCpp->deleteRows(start, start, false);
        start += step - 1; // the "- 1" is needed since the removal also shifts the index
      }
    }
    if (!ok) {
      sipIsErr = 1;
      PyErr_SetString(PyExc_ValueError,
          QString("del: table '%1' contains read-only columns, operation aborted.")
              .arg(sipCpp->objectName()).toLocal8Bit().constData());
    }
  }
%End

  SIP_PYOBJECT text(SIP_PYOBJECT, int);
%MethodCode
  // Was: PyUnicode_FromString(...) followed by PyUnicode_FromEncodedObject(encstr, "utf8", 0) on
  // the result — the second call re-decodes an object that's ALREADY a Python str, which CPython
  // explicitly rejects (PyUnicode_Check(obj) guard in unicodeobject.c) with "TypeError: decoding
  // str is not supported". This made text() raise unconditionally for every non-empty cell, on
  // ANY column, confirmed live. cellData()/colLabel() elsewhere in this file already use the
  // single-call form below with no issue — this brings text() in line with them.
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
  if (sipIsErr == 0) {
		sipRes = PyUnicode_FromString(sipCpp->text(row, col).toLocal8Bit().constData());
		if (!sipRes)
			sipIsErr = 1;
	}
%End

    void removeCol(SIP_PYOBJECT);
%MethodCode
    sipIsErr=0;
    CHECK_TABLE_COL(a0);
	if (sipIsErr == 0){
        QStringList list;
		list << sipCpp->colName(col);
		// interactive=false: a blocking QMessageBox on a read-only column would hang an
		// automated script — read-only columns are silently skipped either way.
        sipCpp->removeCol(list, false);
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
			sipIsErr = 1;
		}
	}
%End
  SIP_PYOBJECT cellOrNone(SIP_PYOBJECT, int);
%MethodCode
  // Like cell(), but an empty or unparsable cell gives None instead of 0.0 / ValueError
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
  if (sipIsErr == 0)
  {
    double value = 0.0;
    const int type = sipCpp->columnType(col);
    bool ok = false;
    if (type == Table::Date || type == Table::Time)
    {
      ok = !sipCpp->text(row, col).trimmed().isEmpty();
      if (ok)
        value = sipCpp->cell(row, col);
    }
    else
      ok = sipCpp->tryNumericCell(row, col, value);
    if (ok)
      sipRes = PyFloat_FromDouble(value);
    else
    {
      Py_INCREF(Py_None);
      sipRes = Py_None;
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
  {
    // Python writes numbers in C ("1.5"): a Numeric column stores them in its own locale, like setCellData();
    // other text goes to Table::cellEdited(), which evaluates it as an expression ("2*pi") and empties the cell if
    // it is none ("beta") — a Text column keeps it as it is
    double value = 0.0;
    if (sipCpp->columnType(col) == Table::Numeric && NumericLocale::toDouble(*a2, QLocale::c(), value))
      sipCpp->setCell(row, col, value);
    else
      sipCpp->setText(row, col, *a2);
  }
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
	if (sipIsErr == 0) {
		// warn=false: a blocking QMessageBox on a name collision would hang an automated
		// script — raise ValueError instead so the caller can catch/handle it.
		if (!sipCpp->setColName(col, *a1, a2, false)) {
			sipIsErr = 1;
			PyErr_SetString(PyExc_ValueError,
				QString("setColName: name '%1' is empty or already used by another column.")
					.arg(*a1).toLocal8Bit().constData());
		}
	}
%End
  void setHeader(QStringList header)/PyName=setColNames/;
  void notifyChanges();

  void importASCII(const QString&, const QString&="\t", int=0, bool=false,
  					bool=true, bool=false, bool=false, const QString&="#", bool=false,
					ImportMode = Overwrite, const QLocale& = QLocale(), int = 0, int = -1, const QList<int>& = QList<int>(), const QStringList& = QStringList());
  bool exportASCII(const QString& fileName, const QString& sep = "\t", bool exportSelection = false, bool exportLabels = false, bool exportComments = false);
%MethodCode
  // interactive=false: a blocking QMessageBox on a write failure would hang an automated
  // script — raise ValueError instead of silently returning false.
  sipRes = sipCpp->exportASCII(*a0, *a1, a2, a3, a4, false);
  if (!sipRes) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError,
        QString("exportASCII: could not write to file '%1'.").arg(*a0).toLocal8Bit().constData());
  }
%End

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

    // as the Preferences do: the table gets the new format, its cells are converted from the old one
    const QLocale oldLocale = sipCpp->locale();
    sipCpp->setLocale(locale);
    sipCpp->updateDecimalSeparators(oldLocale);
%End

  void normalizeCol(SIP_PYOBJECT) /PyName=normalize/;
%MethodCode
	sipIsErr = 0;
	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->normalizeCol(col);
%End
  void normalize();
%MethodCode
	// interactive=false: a blocking QMessageBox on read-only columns would hang an automated
	// script — read-only columns are silently skipped either way.
	sipCpp->normalize(false);
%End

  void sortColumn(SIP_PYOBJECT, int order = 0);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->sortColumn(col, a1);
%End
  bool sort(int type = 0, int order = 0, const QString& leadCol = QString());
%MethodCode
  // interactive=false: a blocking QMessageBox (missing/empty leading column) would hang an
  // automated script — raise ValueError instead of silently aborting the sort.
  sipRes = sipCpp->sort(a0, a1, *a2, false);
  if (!sipRes) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, "sort: leading column name is missing or empty; sort aborted.");
  }
%End
  bool sortColumns(SIP_PYTUPLE, int=0, int=0, const QString&=QString());
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
  if (sipIsErr == 0) {
    sipRes = sipCpp->sortColumns(l, a1, a2, *a3, false);
    if (!sipRes) {
      sipIsErr = 1;
      PyErr_SetString(PyExc_ValueError, "sortColumns: leading column name is missing or empty; sort aborted.");
    }
  }
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
  if (sipIsErr == 0) {
    // interactive=false: a blocking QMessageBox (read-only column, or the multiline-expression
    // confirmation) would hang an automated script — raise ValueError instead on real failure.
    // row index starts at 0; step 1; interactive=false
    sipRes = sipCpp->calculate(col, a1 - 1, a2 - 1, 1, a3, a4, false);
    if (!sipRes) {
      sipIsErr = 1;
      // the formula's first error, if any (e.g. "name 'x' is not defined"), after the general text
      QString message = QString("recalculate: column %1 is read-only or its formula failed to evaluate.").arg(col + 1);
      if (!sipCpp->formulaError().isEmpty())
        message += "\n" + sipCpp->formulaError();
      PyErr_SetString(PyExc_ValueError, message.toUtf8().constData());
    }
  }
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
#include "numericLocale.h"
#include <cmath>
#define CHECK_MATRIX_COL(arg)\
	int col = arg-1;\
   if (col < 0 || col >= sipCpp->numCols()) {\
		sipIsErr = 1;\
        PyErr_Format(PyExc_ValueError,\
                     "There's no column %d in matrix %s! It has %d rows and %d columns; "\
                     "cell(row, col) takes the row first.",\
                     col+1, sipCpp->objectName().toLocal8Bit().constData(), sipCpp->numRows(), sipCpp->numCols());\
	}
#define CHECK_MATRIX_ROW(arg)\
   int row = arg-1;\
	if (row < 0 || row >= sipCpp->numRows()) {\
		sipIsErr = 1;\
        PyErr_Format(PyExc_ValueError,\
                     "There's no row %d in matrix %s! It has %d rows and %d columns; "\
                     "cell(row, col) takes the row first.",\
                     row+1, sipCpp->objectName().toLocal8Bit().constData(), sipCpp->numRows(), sipCpp->numCols());\
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
  {
    // Python writes numbers in C ("1.5"); the matrix's own format is accepted too, anything else leaves it empty
    double value = 0.0;
    if (NumericLocale::toDoubleCOrLocale(*a2, sipCpp->locale(), value))
      sipCpp->setCell(row, col, value);
    else
      sipCpp->setText(row, col, *a2);
  }
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
	bool invert();
%MethodCode
  // interactive=false: a blocking QMessageBox on a non-square matrix would hang an automated
  // script — raise ValueError instead of silently returning false.
  sipRes = sipCpp->invert(false);
  if (!sipRes) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, "invert: the matrix is not square.");
  }
%End
	void flipVertically();
	void flipHorizontally();
	void rotate90(bool clockwise = true);
	double determinant();
%MethodCode
  // interactive=false: a blocking QMessageBox on a non-square matrix would hang an automated
  // script — raise ValueError instead of silently returning GSL_POSINF.
  sipRes = sipCpp->determinant(false);
  if (!std::isfinite(sipRes)) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, "determinant: the matrix is not square.");
  }
%End
	double integrate();
  // The smallest and the largest cell value (Matrix::range()). Confirmed live (benchmark LIST 5, local model): asked
  // for the smallest and the largest value of all cells, the model guessed m.min()/m.max(), minValue(), minCell()
  // and colData() in turn, each an AttributeError.
  double min();
%MethodCode
  double largest = 0.0;
  sipCpp->range(&sipRes, &largest);
%End
  double max();
%MethodCode
  double smallest = 0.0;
  sipCpp->range(&smallest, &sipRes);
%End

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
%MethodCode
  // interactive=false: a blocking QMessageBox (empty filename / unhandled format) would hang
  // an automated script — raise ValueError instead of silently returning.
  if (!sipCpp->exportToFile(*a0, false)) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError,
        QString("export: could not export to '%1' — empty filename or unhandled file format.")
            .arg(*a0).toLocal8Bit().constData());
  }
%End
	void exportVector(const QString& fileName, int res = 0, bool color = true);

	void importASCII(const QString&, const QString& = "\t", int = 0, bool = false, bool = false,
			const QString& = "#", ImportMode = Overwrite, const QLocale& = QLocale(), int = 0, int = -1);
	bool exportASCII(const QString& fileName, const QString& sep = "\t", bool exportSelection = false);
%MethodCode
  // interactive=false: a blocking QMessageBox on a write failure would hang an automated
  // script — raise ValueError instead of silently returning false.
  sipRes = sipCpp->exportASCII(*a0, *a1, a2, false);
  if (!sipRes) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError,
        QString("exportASCII: could not write to file '%1'.").arg(*a0).toLocal8Bit().constData());
  }
%End

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
%MethodCode
  // Confirmed live: a script called insertCurve() against a fitCurve-<name> table (whose real
  // columns are the fixed x/y/weight/sigma/residues/... set, never the SOURCE data table's own
  // column names) using a column name that only existed on the source table. Graph::insertCurve()
  // itself just returns nullptr when Table::colIndex() can't find the name (Graph.cpp) — no
  // exception, no curve, no visible sign anything went wrong; the script completed as if it had
  // succeeded, with the fit curve/residuals silently never plotted. Raising here, at the SIP
  // boundary, turns that silent no-op into an immediate, catchable Python error instead.
  if (a0 == 0) {
    sipIsErr = 1;
    PyErr_Format(PyExc_ValueError, "insertCurve(): the table is None - the table(\"...\") lookup before it found "
                 "no table of that name (a fit's curve table is \"fitCurve-\" + the full function name, e.g. "
                 "\"fitCurve-ai/GaussPeak\").");
  } else if (a0->colIndex(*a1) < 0) {
    sipIsErr = 1;
    PyErr_Format(PyExc_ValueError, "insertCurve(): column '%s' not found in table '%s'.",
                 a1->toUtf8().constData(), a0->objectName().toUtf8().constData());
  } else {
    sipRes = sipCpp->insertCurve(a0, *a1, a2, a3, a4);
  }
%End
  DataCurve* insertCurve(Table*, const QString&, const QString&, int style = 1, int startRow = 0, int endRow = -1);
%MethodCode
  // See the single-column overload above for the confirmed-live rationale — same fix, checking
  // BOTH the X and Y column names here.
  if (a0 == 0) {
    sipIsErr = 1;
    PyErr_Format(PyExc_ValueError, "insertCurve(): the table is None - the table(\"...\") lookup before it found "
                 "no table of that name (a fit's curve table is \"fitCurve-\" + the full function name, e.g. "
                 "\"fitCurve-ai/GaussPeak\").");
  } else if (a0->colIndex(*a1) < 0) {
    sipIsErr = 1;
    PyErr_Format(PyExc_ValueError, "insertCurve(): X column '%s' not found in table '%s'.",
                 a1->toUtf8().constData(), a0->objectName().toUtf8().constData());
  } else if (a0->colIndex(*a2) < 0) {
    sipIsErr = 1;
    PyErr_Format(PyExc_ValueError, "insertCurve(): Y column '%s' not found in table '%s'.",
                 a2->toUtf8().constData(), a0->objectName().toUtf8().constData());
  } else {
    sipRes = sipCpp->insertCurve(a0, *a1, *a2, a3, a4, a5);
  }
%End
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
  // as addCurve() below: a column that does not exist is an error, not a silently missing curve
  if (sipIsErr == 0 && a0 == 0) {
    sipIsErr = 1;
    PyErr_Format(PyExc_ValueError, "Invalid table in addCurves().");
  }
  for (int i = 0; sipIsErr == 0 && i < l.size(); i++)
    if (a0->colIndex(l[i]) < 0) {
      sipIsErr = 1;
      PyErr_Format(PyExc_ValueError, "addCurves(): no column '%s' in table '%s'; columns: %s",
                   l[i].toUtf8().constData(), a0->objectName().toUtf8().constData(),
                   a0->colNames().join(", ").toUtf8().constData());
    }
  if (sipIsErr == 0)
    sipRes = sipCpp->addCurves(a0, l, a2, a3, a4, a5, a6);
%End

  bool addCurve(Table* table, const QString& col, int style=0, double lineWidth=1, int pointSize=3, int startRow=0, int endRow=-1);
%MethodCode
	if (a0 == 0) {
	   sipIsErr = 1;
	   PyErr_Format(PyExc_ValueError, "Invalid table in addCurve().");
	} else if (a0->colIndex(*a1) < 0) {
	  // Confirmed live: addCurve(t, "I", ...) on a table with columns x, y, dy added no curve and raised nothing
	  sipIsErr = 1;
	  PyErr_Format(PyExc_ValueError, "addCurve(): no column '%s' in table '%s'; columns: %s",
	               a1->toUtf8().constData(), a0->objectName().toUtf8().constData(),
	               a0->colNames().join(", ").toUtf8().constData());
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
  bool updateErrorBars(const QString&, Table *, const QString&,
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

  int axesLinewidth() const;
  void setAxesLinewidth(int width);
  void drawAxesBackbones(bool yes);
  void setTicksLength(int minLength, int majLength);

  void setMajorTicksType(int axis, int type);
  void setMinorTicksType(int axis, int type);

  void enableAxisLabels(int axis, bool on);
  void setAxisLabelsColor(int axis, const QColor& color);
  void setLabelsNumericFormat(int axis, int format, int = 6, const QString& = QString()) /PyName=setAxisNumericFormat/;
  // 15th param was WRONGLY declared as "bool log10AfterBreak" here — Graph.h/Graph.cpp both
  // consistently declare/use it as "int typeAfterBreak" (a Graph.Scale enum selector for the
  // scale type used after an axis break: Linear/Log10/Ln/Log2/Reciprocal/Probability/Logit, not
  // a log10-vs-not toggle). The mismatched bool made every value except Linear/Log10 unreachable
  // from Python. Fixed to match the real C++ signature exactly.
  void setScale(int axis, double start, double end, double step = 0.0,
                int majorTicks = 5, int minorTicks = 5, int type = 0, bool inverted = false,
				double left_break = -DBL_MAX, double right_break = DBL_MAX, int pos = 50,
				double stepBeforeBreak = 0.0, double stepAfterBreak = 0.0, int minTicksBeforeBreak = 4,
				int minTicksAfterBreak = 4, int typeAfterBreak = 0, int breakWidth = 4, bool breakDecoration = true);
  SIP_PYTUPLE axisScale(int axis);
%MethodCode
double start = 0.0, end = 0.0;
sipCpp->axisScale(a0, start, end);
sipRes = Py_BuildValue("(dd)", start, end);
%End

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
  bool exportMatplotlibScript(const QString &fileName);
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
  bool exportMatplotlibScript(const QString &fileName);

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
%MethodCode
  // The UI keeps a note's last tab without a word (Note::removeTab()); a script must hear it — confirmed live, an AI
  // script's "while n.tabs() > 0: n.removeTab(0)" never ended
  if (sipCpp->tabs() <= 1)
  {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError,
                    "removeTab: a note always keeps one tab — its last tab can't be removed; "
                    "setText() or renameTab() it");
  }
  else
    sipCpp->removeTab(a0);
%End
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

    QString formula() /PyName=dataSource/;
    Matrix* matrix() /PyName=sourceMatrix/;

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
    // "ALL" by default: confirmed live (LIST 5 item 6, 2026-10-11), Compiler.functionNames() without an argument raised
    // "not enough arguments" twice and the item was given up — the natural call now works.
    QStringList functionNames(const QString &categoryName = "ALL") /PyName=functionNames/;
    // The call models guess (18 times on 2026-10-11, AttributeError each, the doc pointing to fitFunctions() ignored):
    // whether a compiled function exists. groupFunctions("ALL") — no category selection, no UI side effect.
    bool functionExists(const QString &name) /PyName=functionExists/;
%MethodCode
    sipRes = sipCpp->groupFunctions("ALL").contains(*a0);
%End
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
%MethodCode
    // interactive=false: a blocking QMessageBox on a bad name would hang an automated script —
    // raise ValueError instead. '(' / ')' break the function list's keyboard navigation and can
    // silently overwrite a different function's files on a later save/compile.
    const QString stem = a0->isEmpty() ? sipCpp->functionName() : *a0;
    const QString collision = sipqti_caseInsensitiveCollision(sipCpp, stem);
    if (stem.trimmed().isEmpty())
    {
        sipIsErr = 1;
        PyErr_SetString(PyExc_ValueError,
            "save: function name is empty — call setFunctionName(\"name\") before save() for a new "
            "function (or pass a name to save() directly).");
    }
    else if (stem.contains('(') || stem.contains(')'))
    {
        sipIsErr = 1;
        PyErr_SetString(PyExc_ValueError,
            QString("save: function name '%1' must not contain '(' or ')'.").arg(stem).toLocal8Bit().constData());
    }
    else if (!collision.isEmpty())
    {
        sipIsErr = 1;
        PyErr_SetString(PyExc_ValueError,
            QString("save: a function named '%1' already exists — names differing only in letter case are "
                    "not allowed, this filesystem cannot tell them apart and saving would corrupt both.")
                .arg(collision).toLocal8Bit().constData());
    }
    else if (sipCpp->group().trimmed().isEmpty())
    {
        sipIsErr = 1;
        PyErr_SetString(PyExc_ValueError,
            "save: group is empty — call setGroup(\"name\") before save() for a new function. The "
            "explorer's internal state-restore logic (run after saving a brand-new function) matches "
            "against the group name to find the function again; with no group set, that match fails "
            "and can silently clear the function name/code, producing a confusing downstream failure.");
    }
    else if (sipCpp->parameterCount() == 0)
    {
        sipIsErr = 1;
        PyErr_SetString(PyExc_ValueError,
            "save: parameter count is 0 — call setParamCount(n) (and the matching setParamNamesAll/"
            "setParamInitsAll/etc.) before save(). The interactive C++ path shows a blocking 'Check "
            "function!' dialog for this instead, which would hang a script with no one to click it.");
    }
    else
    {
        sipRes = new QString(sipCpp->saveFIF(*a0, *a1, a2));
    }
%End
    void deleteFIF(const QString &name = QString(), const QString &customPath = QString()) /PyName=delete/;
    void scanGroups() /PyName=scanGroups/;
    void downloadFIF(const QString &repoUrl = "https://iffgit.fz-juelich.de/qtisas/fitfunctions.git",
                    bool silent = true) /PyName=download/;

    // --- CODE GENERATION ---
    QString makeCPP() /PyName=makeCPP/;
    QString makeCompileScript() /PyName=makeScript/;

    // --- BUILD ---
    bool buildSharedLibrary(bool compileAll = false) /PyName=compile/;
%MethodCode
    // interactive=false: same guards as save() — compile() saves the FIF internally too.
    const QString collision = sipqti_caseInsensitiveCollision(sipCpp, sipCpp->functionName());
    if (sipCpp->functionName().trimmed().isEmpty())
    {
        sipIsErr = 1;
        PyErr_SetString(PyExc_ValueError,
            "compile: function name is empty — call setFunctionName(\"name\") before compile() for a "
            "new function. Without it, compile() attempts to build an empty-named file, failing deep "
            "inside the shelled-out clang invocation with an unhelpful \"no such file or directory: "
            "'.cpp'\" error instead of this clear message. Confirmed live: setGroup() was called but "
            "setFunctionName() never was, and the fix attempted was renaming the group instead of "
            "adding the missing setFunctionName() call — the same error then repeated unchanged.");
    }
    else if (sipCpp->functionName().contains('(') || sipCpp->functionName().contains(')'))
    {
        sipIsErr = 1;
        PyErr_SetString(PyExc_ValueError,
            QString("compile: function name '%1' must not contain '(' or ')'.")
                .arg(sipCpp->functionName()).toLocal8Bit().constData());
    }
    else if (!collision.isEmpty())
    {
        sipIsErr = 1;
        PyErr_SetString(PyExc_ValueError,
            QString("compile: a function named '%1' already exists — names differing only in letter case are "
                    "not allowed, this filesystem cannot tell them apart and compiling would corrupt both.")
                .arg(collision).toLocal8Bit().constData());
    }
    else if (sipCpp->group().trimmed().isEmpty())
    {
        sipIsErr = 1;
        PyErr_SetString(PyExc_ValueError,
            "compile: group is empty — call setGroup(\"name\") before compile() for a new function. "
            "compile() saves the FIF internally first, and the explorer's internal state-restore logic "
            "that runs afterward matches against the group name to find the function again; with no "
            "group set, that match fails and can silently clear the function name/code, so compile() "
            "then fails deep inside the shelled-out clang invocation with an unhelpful "
            "'no such file or directory: <name>.cpp' error instead of this clear message.");
    }
    else if (sipCpp->parameterCount() == 0)
    {
        sipIsErr = 1;
        PyErr_SetString(PyExc_ValueError,
            "compile: parameter count is 0 — call setParamCount(n) (and the matching setParamNamesAll/"
            "setParamInitsAll/etc.) before compile(). The interactive C++ path shows a blocking 'Check "
            "function!' dialog for this instead, which would hang a script with no one to click it.");
    }
    else
    {
        sipRes = sipCpp->buildSharedLibrary(a0);
        if (!sipRes)
        {
            sipIsErr = 1;
            QString errText = sipCpp->lastCompileErrors.trimmed().isEmpty()
                                         ? QString("no compiler output captured — see qti.app.resultsLog().")
                                         : sipCpp->lastCompileErrors.trimmed();
            // Confirmed live: code assigning "I = ..." (the SANS habit) to a function whose y is named "f" — clang's
            // "undeclared identifier 'I'" alone was repeated until the request gave up; name the real variables
            // The parameters by name too: confirmed live (local, benchmark LIST 2 item 4), "undeclared identifier
            // 'background'" for a function with scale, rate and offset came back five times.
            if (errText.contains("undeclared identifier"))
                errText += QString("\nThis function's variables are named x = '%1' and y = '%2' "
                                   "(setXName()/setYName()); its parameters are %3 (setParamNamesAll()) — the code "
                                   "can use only these names.")
                               .arg(sipCpp->xName(), sipCpp->yName(),
                                    sipCpp->paramNames().isEmpty() ? QString("none")
                                                                   : "'" + sipCpp->paramNames().join("', '") + "'");
            PyErr_SetString(PyExc_RuntimeError,
                QString("compile: compilation of '%1' failed:\n%2")
                    .arg(sipCpp->functionName(), errText).toLocal8Bit().constData());
        }
    }
%End
    void compileAll() /PyName=compileAll/;

    // --- SASVIEW ---
    void openSasViewPy() /PyName=openSasView/;

    // --- PARAMETERS ---
    void setNumberparameters(int paraNumber) /PyName=setParamCount/;
    QStringList paramNames() const /PyName=paramNames/;
    QString paramName(int i) const /PyName=paramName/;
    void setParamName(int i, const QString &name) /PyName=setParamName/;
%MethodCode
    if (sipqti_isCppReservedWord(*a1)) {
        PyErr_Format(PyExc_ValueError,
            "setParamName: '%s' is a C++ reserved keyword and cannot be used as a parameter name.",
            a1->toUtf8().constData());
        sipIsErr = 1;
    } else {
        sipCpp->setParamName(a0, *a1);
    }
%End
    void setParamNamesAll(SIP_PYLIST names) /PyName=setParamNamesAll/;
%MethodCode
    // Each item MUST be checked with PyUnicode_Check() before PyUnicode_AsUTF8() — calling it on
    // a non-string (e.g. a float) leaves a Python exception set internally without raising here,
    // so the method would return normally with an exception pending, which Python's own C-API
    // safety net catches as an opaque "SystemError: ... returned a result with an exception set"
    // — confirmed live, passing floats instead of range strings to setParamLimitsAll (same
    // unchecked pattern, fixed below too) produced exactly that unhelpful message.
    QStringList lst;
    Py_ssize_t n = PyList_Size(a0);
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject *item = PyList_GET_ITEM(a0, i);
        if (!PyUnicode_Check(item)) {
            PyErr_Format(PyExc_TypeError, "setParamNamesAll: item %zd is not a string.", i);
            sipIsErr = 1;
            break;
        }
        lst << QString::fromUtf8(PyUnicode_AsUTF8(item));
    }
    if (sipIsErr) {
        // bad item already reported above; nothing more to do
    } else if (lst.size() != sipCpp->parameterCount()) {
        PyErr_Format(PyExc_ValueError,
            "setParamNamesAll: got %d name(s) but setParamCount(%d) was declared - they must "
            "match exactly.",
            (int)lst.size(), sipCpp->parameterCount());
        sipIsErr = 1;
    } else {
        QString bad;
        for (const QString &nm : lst)
            if (sipqti_isCppReservedWord(nm)) { bad = nm; break; }
        if (!bad.isEmpty()) {
            PyErr_Format(PyExc_ValueError,
                "setParamNamesAll: '%s' is a C++ reserved keyword and cannot be used as a "
                "parameter name.",
                bad.toUtf8().constData());
            sipIsErr = 1;
        } else {
            sipCpp->setParamNamesAll(lst);
        }
    }
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
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject *item = PyList_GET_ITEM(a0, i);
        if (!PyUnicode_Check(item)) {
            // PyErr_Format's format string must be pure ASCII (unlike PyErr_SetString) — an em-dash here
            // previously crashed this with "ValueError: PyUnicode_FromFormatV() expects an ASCII-encoded
            // format string..." instead of raising the intended TypeError. Confirmed live (twice now,
            // across two different methods — see setWeightingDataset's %MethodCode for the other one).
            PyErr_Format(PyExc_TypeError,
                "setParamLimitsAll: item %zd is not a string - pass range strings like \"0..100\" "
                "or \"..\", not raw numbers.",
                i);
            sipIsErr = 1;
            break;
        }
        lst << QString::fromUtf8(PyUnicode_AsUTF8(item));
    }
    if (!sipIsErr)
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
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject *item = PyList_GET_ITEM(a0, i);
        if (!PyUnicode_Check(item)) {
            PyErr_Format(PyExc_TypeError, "setParamDescrsAll: item %zd is not a string.", i);
            sipIsErr = 1;
            break;
        }
        lst << QString::fromUtf8(PyUnicode_AsUTF8(item));
    }
    if (!sipIsErr)
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
        // The C++ side only scriptPrint()s the specific reason (missing name + what's actually
        // available) and returns false — that text never reached the raised exception before,
        // so a caller (human or AI) reading only the exception message had no way to know WHAT
        // was wrong, only THAT something was. Confirmed live: an AI retry loop cycled through 10
        // guessed dataset names in a row because the actual available name was never visible to
        // it anywhere except this now-added detail.
        QString detail;
        const QStringList functions = sipCpp->fitFunctions();
        if (!functions.contains(*a0)) {
            // A name without its folder: confirmed live (LIST 5 local pyi, every run), best = "Lor5" for the compiled
            // "ai/Lor5" — "not found" alone was retried unchanged until the item was given up. One function whose
            // name ends in "/<name>" is named; several or none leave the message as it was.
            QStringList withFolder;
            for (const QString &f : functions)
                if (f.endsWith("/" + *a0))
                    withFolder << f;
            if (withFolder.size() == 1)
                detail = QString(" Function '%1' not found. Did you mean '%2'? A fit function is named with its "
                                 "folder, in every call.")
                             .arg(*a0, withFolder.first());
            else
                detail = QString(" Function '%1' not found — use fitFunctions() to list available names.").arg(*a0);
        } else if (sipCpp->stage() == "init") {
            detail = QString(" Function '%1' is listed but could not be opened: its compiled library is missing "
                             "or does not load. Compile it first (Compiler.open(name), then Compiler.compile()), or "
                             "use another function from fitFunctions().")
                         .arg(*a0);
        } else {
            const QStringList avail = sipCpp->availableDatasets();
            QStringList missing;
            for (const QString &d : *a1)
                if (!avail.contains(d))
                    missing << d;
            if (!missing.isEmpty())
                detail = QString(" Dataset(s) not found: %1. Available: %2.")
                             .arg(missing.join(", "), avail.join(", "));
            // A dataset is "<table>_<column name>". Confirmed live: a table created with the default column names
            // 1, 2 was asked for as "Peak_y" three times in a row — the list above alone did not make the reason
            // visible.
            for (const QString &d : missing)
            {
                const qsizetype cut = d.lastIndexOf('_');
                if (cut <= 0)
                    continue;
                const QString tablePrefix = d.left(cut + 1);
                QStringList columns;
                for (const QString &a : avail)
                    if (a.startsWith(tablePrefix))
                        columns << a.mid(tablePrefix.size());
                if (!columns.isEmpty())
                    detail += QString(" Table \"%1\" has no Y column named \"%2\" — its Y columns are named %3 (name "
                                      "them with setColNames(), or use \"%4\").")
                                  .arg(d.left(cut), d.mid(cut + 1), columns.join(", "), tablePrefix + columns.last());
            }
        }
        PyErr_SetString(PyExc_ValueError,
            QString("configure: '%1' failed.%2").arg(*a0, detail).toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End

    QStringList fitFunctions(const QString &pattern = QString()) /PyName=fitFunctions/;
    // Whether a fit function exists — the call models guess (see Compiler.functionExists()).
    bool functionExists(const QString &name) /PyName=functionExists/;
%MethodCode
    sipRes = sipCpp->fitFunctions().contains(*a0);
%End
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
    // m is 1-indexed (like setRange/setDataset above) — confirmed live: passing 0 (a natural
    // mistake coming from Python's 0-based indexing elsewhere) silently no-ops downstream
    // (fittable18::setWeightingEnabled/setWeightingDataset do "m = m - 1" with no bounds check,
    // landing on a negative table column index that Qt just returns null for) — the fit then
    // silently runs unweighted with no error, no warning, and a plausible-looking wrong chi2/dof.
    bool weightingEnabled(int m) const /PyName=weightingEnabled/;
%MethodCode
    if (a0 < 1 || a0 > sipCpp->datasetCount()) {
        PyErr_Format(PyExc_ValueError, "weightingEnabled: dataset %d out of range (1..%d).", a0,
                     sipCpp->datasetCount());
        sipIsErr = 1;
    } else {
        sipRes = sipCpp->weightingEnabled(a0);
    }
%End
    void setWeightingEnabled(int m, bool enabled) /PyName=setWeightingEnabled/;
%MethodCode
    if (a0 < 1 || a0 > sipCpp->datasetCount()) {
        PyErr_Format(PyExc_ValueError, "setWeightingEnabled: dataset %d out of range (1..%d).", a0,
                     sipCpp->datasetCount());
        sipIsErr = 1;
    } else {
        sipCpp->setWeightingEnabled(a0, a1);
    }
%End
    QString weightingDataset(int m) const /PyName=weightingDataset/;
%MethodCode
    if (a0 < 1 || a0 > sipCpp->datasetCount()) {
        PyErr_Format(PyExc_ValueError, "weightingDataset: dataset %d out of range (1..%d).", a0,
                     sipCpp->datasetCount());
        sipIsErr = 1;
    } else {
        sipRes = new QString(sipCpp->weightingDataset(a0));
    }
%End
    bool setWeightingDataset(int m, const QString &name) /PyName=setWeightingDataset/;
%MethodCode
    if (a0 < 1 || a0 > sipCpp->datasetCount()) {
        PyErr_Format(PyExc_ValueError, "setWeightingDataset: dataset %d out of range (1..%d).", a0,
                     sipCpp->datasetCount());
        sipIsErr = 1;
    } else if (!sipCpp->setWeightingDataset(a0, *a1)) {
        // Confirmed live: a bare column name (e.g. "dI" instead of the required fully-qualified
        // "tablename_dI") used to silently no-op here — the previous/default selection stayed in
        // place, weighting never actually applied, and the fit ran unweighted with no error at all.
        // Confirmed live, separately: an ALREADY-correctly-qualified name (e.g. "table_dy") can
        // still land here if that column's PlotDesignation was never set to yErr — this combobox
        // only ever lists yErr-role columns, regardless of naming. Naming that second cause
        // explicitly here matters: a script that burned 12+ retries chasing the naming-format
        // explanation alone never once considered the column's role, because nothing told it to.
        PyErr_SetString(PyExc_ValueError,
            QString("setWeightingDataset: '%1' is not one of the available weighting datasets for "
                    "index %2 - either use the fully-qualified \"tablename_colname\" form (matching "
                    "configure()'s dataset names), not a bare column name, OR - if the name is "
                    "already qualified correctly - that column's role was never set to "
                    "Table.PlotDesignation.yErr (only yErr-role columns are ever listed here; "
                    "setting the role AFTER configure() was already called requires calling "
                    "configure() again for it to take effect).")
                .arg(*a1).arg(a0).toLocal8Bit().constData());
        sipIsErr = 1;
    } else {
        sipRes = true;
    }
%End

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
    bool setResoDataset(int m, const QString &name) /PyName=setResoDataset/;
%MethodCode
    if (a0 < 1 || a0 > sipCpp->datasetCount()) {
        PyErr_Format(PyExc_ValueError, "setResoDataset: dataset %d out of range (1..%d).", a0,
                     sipCpp->datasetCount());
        sipIsErr = 1;
    } else if (!sipCpp->setResoDataset(a0, *a1)) {
        // Same silent-no-op bug as setWeightingDataset used to have: an unmatched name left the
        // previous/default selection in place, with no error, so resolution smearing never actually
        // applied as requested.
        PyErr_SetString(PyExc_ValueError,
            QString("setResoDataset: '%1' is not one of the available datasets for index %2 - use "
                    "the fully-qualified \"tablename_colname\" form.")
                .arg(*a1).arg(a0).toLocal8Bit().constData());
        sipIsErr = 1;
    } else {
        sipRes = true;
    }
%End

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
    bool setPolyDataset(int m, const QString &name) /PyName=setPolyDataset/;
%MethodCode
    if (a0 < 1 || a0 > sipCpp->datasetCount()) {
        PyErr_Format(PyExc_ValueError, "setPolyDataset: dataset %d out of range (1..%d).", a0,
                     sipCpp->datasetCount());
        sipIsErr = 1;
    } else if (!sipCpp->setPolyDataset(a0, *a1)) {
        // Same silent-no-op bug as setWeightingDataset used to have: an unmatched name left the
        // previous/default selection in place, with no error, so polydispersity averaging never
        // actually applied as requested.
        PyErr_SetString(PyExc_ValueError,
            QString("setPolyDataset: '%1' is not one of the available datasets for index %2 - use "
                    "the fully-qualified \"tablename_colname\" form.")
                .arg(*a1).arg(a0).toLocal8Bit().constData());
        sipIsErr = 1;
    } else {
        sipRes = true;
    }
%End

    // parameters (dataset: 1=first .. N; limits: -1=global  1..N=per-dataset)
    int parameterCount() const /PyName=paramCount/;
    QStringList parameterNames() const /PyName=paramNames/;
    double parameterValue(const QString &name, int dataset = 1) const /PyName=paramValue/;
%MethodCode
    sipRes = 0.0;
    if (!sipCpp->parameterNames().contains(*a0)) {
        PyErr_SetString(PyExc_ValueError,
            QString("paramValue: no such parameter '%1'. Available: %2.")
                .arg(*a0, sipCpp->parameterNames().join(", "))
                .toLocal8Bit().constData());
        sipError = sipErrorFail;
    } else {
        sipRes = sipCpp->parameterValue(*a0, a1);
    }
%End
    bool setParameterValue(const QString &name, double value, int dataset = -1) /PyName=setParamValue/;
%MethodCode
    sipRes = sipCpp->setParameterValue(*a0, a1, a2);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError,
            QString("setParamValue: no such parameter '%1'. Available: %2.")
                .arg(*a0, sipCpp->parameterNames().join(", "))
                .toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End
    void setParameterValueAll(SIP_PYLIST values, int dataset = -1) /PyName=setParamValueAll/;
%MethodCode
    QVector<double> vec;
    Py_ssize_t n = PyList_Size(a0);
    for (Py_ssize_t i = 0; i < n; ++i)
        vec.append(PyFloat_AsDouble(PyList_GET_ITEM(a0, i)));
    sipCpp->setParameterValueAll(vec, a1);
%End
    bool parameterVaries(const QString &name, int dataset = 1) const /PyName=paramVaries/;
    bool setParameterVaries(const QString &name, bool varies, int dataset = -1) /PyName=setParamVaries/;
%MethodCode
    sipRes = sipCpp->setParameterVaries(*a0, a1, a2);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError,
            QString("setParamVaries: no such parameter '%1'. Available: %2.")
                .arg(*a0, sipCpp->parameterNames().join(", "))
                .toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End
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
        // Confirmed live: forgetting setParamVaries()/setParamVariesAll() after configure() leaves
        // every parameter fixed (that's the default — see compile18::setNumberparameters), and
        // fitOrCalculate() then aborts cleanly with "No adjustible parameters" printed to the
        // console. But that specific, correct reason never reaches this wrapper: fit() only ever
        // returns NaN for ANY failure reason, and the code below used to blindly report that NaN as
        // "failed to read data ... check for empty or invalid rows" — flatly wrong here, and
        // confusing enough that a retry loop burned through several rounds "fixing" data that was
        // never the problem. Check for this exact, common cause upfront with an accurate message.
        bool anyVaries = false;
        for (const QString &pname : sipCpp->parameterNames())
            if (sipCpp->parameterVaries(pname))
            {
                anyVaries = true;
                break;
            }
        if (!anyVaries)
        {
            PyErr_SetString(PyExc_RuntimeError,
                "fit(): no adjustable parameters — every parameter is fixed. Call setParamVaries(name, "
                "True) or setParamVariesAll([...]) for at least one parameter before fit().");
            sipIsErr = 1;
        }
        else
        {
            sipRes = sipCpp->fit();
            if (PyErr_Occurred()) {
                if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
                    sipCpp->app()->scriptPrint("Fit interrupted");
                PyErr_Clear();
            } else if (std::isnan(sipRes) && sipCpp->lastFitRan) {
                // The fit ran and diverged. Confirmed live (local LIST 1, item 12): a Gaussian with no initial
                // values (width 0, a division by zero) ended "chi2/dof: NAN" after 100 iterations, the error below
                // said "empty or invalid rows", and the retries looked for a data fault that was not there.
                PyErr_SetString(PyExc_RuntimeError,
                    "fit(): the fit ran but its chi2/dof is NaN — the function gives no number for the starting "
                    "values: most likely a parameter starts at 0 where it divides (a width) or inside log()/sqrt(). "
                    "Set nonzero initial values with setParamValue() before fit().");
                sipIsErr = 1;
            } else if (std::isnan(sipRes)) {
                // fit() now returns NaN (instead of a stale previous chi2) when the underlying data
                // read failed (e.g. an empty/unreadable dataset) — see fittable18::fit(). Without
                // this check that NaN would just silently look like a real, if odd, fit result.
                PyErr_SetString(PyExc_RuntimeError,
                    "fit(): failed to read data for the selected dataset(s) — check for empty or invalid rows.");
                sipIsErr = 1;
            }
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
                "selectStage(): no function is set yet (init stage) — call configure(function, datasets) first, "
                "and fix the error it raises if it raised one.");
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
    } else if (sipCpp->selectedDataset(1).isEmpty()) {
        // configure(f, []) sets the dataset COUNT to 1 but selects nothing, so datasetCount() is a
        // useless test here — the dataset row itself is empty. simulateSwitcher() then reads
        // comboBoxDatasetSim->currentIndex(), which is -1 while nothing is selected (the combo is
        // only filled once a dataset exists), and hands that -1 to generateSimulatedTable(). The
        // failure surfaced as a bare "SystemError: ... returned a result with an exception set"
        // with nothing to act on. A dataset is needed even for a uniform X range, which supplies
        // only the X points, not the row the simulation is built from.
        PyErr_SetString(PyExc_ValueError,
            "simulateCurve(): no dataset is selected — pass one to configure(), e.g. "
            "configure(function, [\"MyTable_y\"]). A dataset is required even with setSimUniformX(True): "
            "the uniform range supplies the X points, but the simulation still reads the selected "
            "dataset row. Create a small seed table if you have no real data to simulate against.");
        sipIsErr = 1;
    } else {
        sipRes = sipCpp->simulateCurve();
        // A simulation that made no table used to return a number all the same. Confirmed live (BlaBlaDor, LIST 1
        // item 8, 4 of 4 runs) and reproduced: setSimUniformX(False) takes the x values from the selected dataset's
        // X column — on a table without one (both columns Y) the curve is not made, table("simulatedCurve-...")
        // returned None, and every retry failed on that None without a word about why.
        const QString base = "simulatedCurve-" + sipCpp->currentFunction();
        bool made = false;
        for (const QString &name : sipCpp->app()->tableNames())
            if (name == base || (name.startsWith(base + "-") && !name.endsWith("-session")))
                made = true;
        if (!made) {
            PyErr_SetString(PyExc_RuntimeError,
                QString("simulateCurve(): no simulated curve was made (no table \"%1\"). With "
                        "setSimUniformX(False) the x values come from the X column of the selected dataset \"%2\" — "
                        "its table needs a column with the X role; or use setSimUniformX(True) with "
                        "setSimFromX()/setSimToX()/setSimNumPoints() for an x range of its own.")
                    .arg(base, sipCpp->selectedDataset(1))
                    .toUtf8()
                    .constData());
            sipIsErr = 1;
        }
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

// ASCII.SANS.1D: import and export of radially averaged 1D SAS data (Q, I, dI, sigma) — the "Process Data" button in
// four directions, the interface's options as setters. Options stay set until changed or reset(). A run returns what
// it made (table names, or file paths); it raises ValueError when it made nothing, and prints the datasets it skipped.
class ascii1d18 : public QWidget
{
%TypeHeaderCode
#include "ascii1d18.h"
%End

%TypeCode
// one name or a list of names: "a.dat", ["a.dat", "b.dat"], ("a.dat",) — None for none
static bool ascii1dStrings(PyObject *obj, QStringList *out, const char *call, const char *what)
{
    if (!obj || obj == Py_None)
        return true;
    if (PyUnicode_Check(obj)) {
        *out << QString::fromUtf8(PyUnicode_AsUTF8(obj));
        return true;
    }
    PyObject *seq = PySequence_Check(obj) ? PySequence_Fast(obj, "") : nullptr;
    if (!seq) {
        PyErr_Format(PyExc_TypeError, "%s: %s must be a string or a list of strings", call, what);
        return false;
    }
    const Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = PySequence_Fast_GET_ITEM(seq, i);
        if (!PyUnicode_Check(item)) {
            Py_DECREF(seq);
            PyErr_Format(PyExc_TypeError, "%s: %s must be a string or a list of strings", call, what);
            return false;
        }
        *out << QString::fromUtf8(PyUnicode_AsUTF8(item));
    }
    Py_DECREF(seq);
    return true;
}
%End

public:
    ascii1d18(QWidget *parent = 0);

    void reset();
    bool setFormat(const QString &format);
%MethodCode
    sipRes = sipCpp->setFormat(*a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    QString format() const;
    bool setHeader(const QString &linesOrMarker);
%MethodCode
    sipRes = sipCpp->setHeader(*a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    // a number of header lines as an int too: setHeader(5)
    bool setHeader(int lines);
%MethodCode
    sipRes = sipCpp->setHeader(QString::number(a0));
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    QString header() const;
    bool setColumns(int n);
%MethodCode
    sipRes = sipCpp->setColumns(a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    int columns() const;
    void setInputFolder(const QString &folder);
    QString inputFolder() const;
    void setOutputFolder(const QString &folder);
    QString outputFolder() const;
    void setExtension(const QString &extension);
    QString extension() const;
    void setIndexing(bool on);
    void setAutoPrefix(bool on);
    bool setNameNumber(int index);
%MethodCode
    sipRes = sipCpp->setNameNumber(a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    bool setRemovePoints(int first = 0, int last = 0, int from = 0, int to = 0, bool negative = false);
%MethodCode
    sipRes = sipCpp->setRemovePoints(a0, a1, a2, a3, a4);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    bool setPresentation(const QString &to, const QString &from = "QI");
%MethodCode
    sipRes = sipCpp->setPresentation(*a0, *a1);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    QStringList presentations() const;
    bool setMath(const QString &op, double factor = 1, const QString &op2 = QString(), double factor2 = 1);
%MethodCode
    sipRes = sipCpp->setMath(*a0, a1, *a2, a3);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    bool setCalculator(const QString &op, const QString &column = QString());
%MethodCode
    sipRes = sipCpp->setCalculator(*a0, *a1);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    bool setBinning(int every = 1, double ratio = 1);
%MethodCode
    sipRes = sipCpp->setBinning(a0, a1);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    void setResolution(bool on);
    // the tool's units, in the names: collimation, detector distance, pixel size in cm; apertures in mm; lambda in Å
    bool setResolutionGeometry(double collimationCm, double detectorCm, double pixelCm = 0.525,
                               double sampleApertureWmm = 10, double sampleApertureHmm = 10,
                               double collimationApertureWmm = 30, double collimationApertureHmm = 30,
                               double lambdaA = 6, double deltaLambda = 0.1);
%MethodCode
    sipRes = sipCpp->setResolutionGeometry(a0, a1, a2, a3, a4, a5, a6, a7, a8);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    void setResolutionLenses(bool on, double focus = 0);
    double resolutionSigma(double q);
    void setPlot(bool on, bool xError = false, bool yError = true);
    QStringList importFiles(SIP_PYOBJECT sources, SIP_PYOBJECT names = Py_None);
%MethodCode
    QStringList sources, names;
    if (!ascii1dStrings(a0, &sources, "importFiles", "sources") || !ascii1dStrings(a1, &names, "importFiles", "names")) {
        sipError = sipErrorFail;
    } else {
        sipRes = new QStringList(sipCpp->importFiles(sources, names));
        if (sipRes->isEmpty()) {
            PyErr_SetString(PyExc_ValueError, ("importFiles: " + sipCpp->lastError()).toUtf8().constData());
            sipError = sipErrorFail;
        } else {
            for (const QString &m : sipCpp->lastMessages())
                PySys_WriteStdout("%s\n", ("importFiles: " + m).toUtf8().constData());
        }
    }
%End
    QStringList convertFiles(SIP_PYOBJECT sources, SIP_PYOBJECT names = Py_None);
%MethodCode
    QStringList sources, names;
    if (!ascii1dStrings(a0, &sources, "convertFiles", "sources") || !ascii1dStrings(a1, &names, "convertFiles", "names")) {
        sipError = sipErrorFail;
    } else {
        sipRes = new QStringList(sipCpp->convertFiles(sources, names));
        if (sipRes->isEmpty()) {
            PyErr_SetString(PyExc_ValueError, ("convertFiles: " + sipCpp->lastError()).toUtf8().constData());
            sipError = sipErrorFail;
        } else {
            for (const QString &m : sipCpp->lastMessages())
                PySys_WriteStdout("%s\n", ("convertFiles: " + m).toUtf8().constData());
        }
    }
%End
    QStringList exportTables(SIP_PYOBJECT sources, SIP_PYOBJECT names = Py_None);
%MethodCode
    QStringList sources, names;
    if (!ascii1dStrings(a0, &sources, "exportTables", "sources") || !ascii1dStrings(a1, &names, "exportTables", "names")) {
        sipError = sipErrorFail;
    } else {
        sipRes = new QStringList(sipCpp->exportTables(sources, names));
        if (sipRes->isEmpty()) {
            PyErr_SetString(PyExc_ValueError, ("exportTables: " + sipCpp->lastError()).toUtf8().constData());
            sipError = sipErrorFail;
        } else {
            for (const QString &m : sipCpp->lastMessages())
                PySys_WriteStdout("%s\n", ("exportTables: " + m).toUtf8().constData());
        }
    }
%End
    QStringList convertTables(SIP_PYOBJECT sources, SIP_PYOBJECT names = Py_None);
%MethodCode
    QStringList sources, names;
    if (!ascii1dStrings(a0, &sources, "convertTables", "sources") || !ascii1dStrings(a1, &names, "convertTables", "names")) {
        sipError = sipErrorFail;
    } else {
        sipRes = new QStringList(sipCpp->convertTables(sources, names));
        if (sipRes->isEmpty()) {
            PyErr_SetString(PyExc_ValueError, ("convertTables: " + sipCpp->lastError()).toUtf8().constData());
            sipError = sipErrorFail;
        } else {
            for (const QString &m : sipCpp->lastMessages())
                PySys_WriteStdout("%s\n", ("convertTables: " + m).toUtf8().constData());
        }
    }
%End
    QStringList formats() const;
    bool loadFormat(const QString &name);
%MethodCode
    sipRes = sipCpp->loadFormat(*a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    bool saveFormat(const QString &name);
%MethodCode
    sipRes = sipCpp->saveFormat(*a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    QStringList lastMessages() const;
};


class svd : public QWidget
{
%TypeHeaderCode
#include "svd.h"
%End

%TypeCode
// a sequence of tuples (str, number, number), the last number optional when optional = 1 (NaN then)
static bool svdTuples(PyObject *obj, int optional, QStringList *names, QVector<double> *a, QVector<double> *b,
                      const char *message)
{
    PyObject *seq = obj && PySequence_Check(obj) && !PyUnicode_Check(obj) ? PySequence_Fast(obj, "") : nullptr;
    bool ok = seq != nullptr;
    for (Py_ssize_t i = 0; ok && i < PySequence_Fast_GET_SIZE(seq); i++) {
        PyObject *item = PySequence_Fast_GET_ITEM(seq, i);
        const Py_ssize_t n = PySequence_Check(item) && !PyUnicode_Check(item) ? PySequence_Size(item) : -1;
        ok = n >= 3 - optional && n <= 3;
        if (!ok)
            break;
        PyObject *name = PySequence_GetItem(item, 0);
        ok = PyUnicode_Check(name);
        if (ok)
            *names << QString::fromUtf8(PyUnicode_AsUTF8(name));
        Py_DECREF(name);
        QVector<double> *fields[] = {a, b};
        for (Py_ssize_t k = 1; ok && k <= 2; k++) {
            if (k >= n) {
                *fields[k - 1] << qQNaN();
                continue;
            }
            PyObject *v = PySequence_GetItem(item, k);
            ok = PyNumber_Check(v) && !PyUnicode_Check(v);
            if (ok)
                *fields[k - 1] << PyFloat_AsDouble(v);
            Py_DECREF(v);
        }
    }
    Py_XDECREF(seq);
    if (!ok)
        PyErr_SetString(PyExc_TypeError, message);
    return ok;
}

// a number or a sequence of numbers
static bool svdNumbers(PyObject *obj, QVector<double> *out, const char *message)
{
    if (obj && PyNumber_Check(obj) && !PySequence_Check(obj)) {
        *out << PyFloat_AsDouble(obj);
        return true;
    }
    PyObject *seq = obj && PySequence_Check(obj) && !PyUnicode_Check(obj) ? PySequence_Fast(obj, "") : nullptr;
    bool ok = seq != nullptr;
    for (Py_ssize_t i = 0; ok && i < PySequence_Fast_GET_SIZE(seq); i++) {
        PyObject *v = PySequence_Fast_GET_ITEM(seq, i);
        ok = PyNumber_Check(v) && !PyUnicode_Check(v);
        if (ok)
            *out << PyFloat_AsDouble(v);
    }
    Py_XDECREF(seq);
    if (!ok)
        PyErr_SetString(PyExc_TypeError, message);
    return ok;
}
%End

public:
    void reset();
    bool setSolvent(double sldH2Ocm2, double sldD2Ocm2);
%MethodCode
    sipRes = sipCpp->setSolvent(a0, a1);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    // [(name, sldH2Ocm2, sldD2Ocm2), ...]
    bool setComponents(SIP_PYOBJECT components);
%MethodCode
    QStringList names;
    QVector<double> h, d;
    if (!svdTuples(a0, 0, &names, &h, &d,
                   "setComponents: give a list of (name, sldH2Ocm2, sldD2Ocm2), e.g. [(\"a\", 1.48e10, 2.374e10)]")) {
        sipError = sipErrorFail;
    } else {
        sipRes = sipCpp->setComponents(names, h, d);
        if (!sipRes) {
            PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
            sipError = sipErrorFail;
        }
    }
%End
    // [(table, d2oFraction), ...] or [(table, d2oFraction, solventSldCm2), ...]
    bool setContrasts(SIP_PYOBJECT contrasts);
%MethodCode
    QStringList tables;
    QVector<double> fractions, sld;
    if (!svdTuples(a0, 1, &tables, &fractions, &sld,
                   "setContrasts: give a list of (table, d2oFraction) or (table, d2oFraction, solventSldCm2), "
                   "e.g. [(\"mix0\", 0.0), (\"mix34\", 0.34)]")) {
        sipError = sipErrorFail;
    } else {
        sipRes = sipCpp->setContrasts(tables, fractions, sld);
        if (!sipRes) {
            PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
            sipError = sipErrorFail;
        }
    }
%End
    QString decompose(const QString &name = QString(), int qPoints = 0);
%MethodCode
    sipRes = new QString(sipCpp->decompose(*a0, a1));
    if (sipRes->isEmpty()) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    } else {
        for (const QString &m : sipCpp->lastMessages())
            PySys_WriteStdout("%s\n", ("decompose: " + m).toUtf8().constData());
    }
%End
    double value(const QString &svdTable, const QString &column, double q);
%MethodCode
    sipRes = sipCpp->value(*a0, *a1, a2);
    if (!sipCpp->lastError().isEmpty()) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    bool readTable(const QString &svdTable);
%MethodCode
    sipRes = sipCpp->readTable(*a0);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    }
%End
    // [(table, d2oFraction, solventSldCm2), ...] as set: setContrasts() takes it back as it is
    SIP_PYLIST contrasts() const;
%MethodCode
    const QStringList tables = sipCpp->contrastTables();
    const QVector<double> fractions = sipCpp->contrastFractions();
    const QVector<double> solvent = sipCpp->contrastSolventSlds();
    sipRes = PyList_New(tables.size());
    for (int i = 0; i < tables.size(); i++)
        PyList_SET_ITEM(sipRes, i, Py_BuildValue("(sdd)", tables[i].toUtf8().constData(), fractions[i], solvent[i]));
%End
    // the Q points of an SVD table: len() is the number of Q points
    SIP_PYLIST qValues(const QString &svdTable);
%MethodCode
    const QVector<double> q = sipCpp->qValues(*a0);
    if (!sipCpp->lastError().isEmpty()) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    } else {
        sipRes = PyList_New(q.size());
        for (int i = 0; i < q.size(); i++)
            PyList_SET_ITEM(sipRes, i, PyFloat_FromDouble(q[i]));
    }
%End
    QStringList components() const;
%MethodCode
    sipRes = new QStringList(sipCpp->componentNames());
%End
    double matchPoint(double sldH2Ocm2, double sldD2Ocm2);
%MethodCode
    sipRes = sipCpp->matchPoint(a0, a1);
    if (!sipCpp->lastError().isEmpty()) {
        PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
        sipError = sipErrorFail;
    } else {
        for (const QString &m : sipCpp->lastMessages())
            PySys_WriteStdout("%s\n", m.toUtf8().constData());
    }
%End
    int rank(SIP_PYOBJECT d2oFractions);
%MethodCode
    QVector<double> fractions;
    if (!svdNumbers(a0, &fractions, "rank: give the D2O fractions as a list of numbers, e.g. [0, 0.5, 1]")) {
        sipError = sipErrorFail;
    } else {
        sipRes = sipCpp->rank(fractions);
        if (sipRes < 0) {
            PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
            sipError = sipErrorFail;
        }
    }
%End
    SIP_PYLIST plan(SIP_PYOBJECT d2oFractions, SIP_PYOBJECT partials, double relError = 0.01, double background = 0);
%MethodCode
    QVector<double> fractions, values;
    if (!svdNumbers(a0, &fractions, "plan: give the D2O fractions as a list of numbers, e.g. [0, 0.5, 1]") ||
        !svdNumbers(a1, &values, "plan: give the partial structure factors S11, S12, ... at one Q as a list")) {
        sipError = sipErrorFail;
    } else {
        const QVector<double> errors = sipCpp->plan(fractions, values, a2, a3);
        if (errors.isEmpty()) {
            PyErr_SetString(PyExc_ValueError, sipCpp->lastError().toUtf8().constData());
            sipError = sipErrorFail;
        } else {
            for (const QString &m : sipCpp->lastMessages())
                PySys_WriteStdout("%s\n", m.toUtf8().constData());
            sipRes = PyList_New(errors.size());
            for (int i = 0; i < errors.size(); i++)
                PyList_SET_ITEM(sipRes, i, PyFloat_FromDouble(errors[i]));
        }
    }
%End
    QStringList lastMessages() const;
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
  ascii1d18 *ascii1dWidget /PyName=Ascii1d/;
  svd *svdWidget /PyName=Svd/;

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
        if (a1 > 1)
        {
            // Confirmed live (Python-side): a script calling newGraph(name, layers>1, rows, cols)
            // a second time (a retry, or simply re-running the same script) against a window that
            // ALREADY has the requested grid used to be returned COMPLETELY untouched, curves and
            // all — and even when the grid genuinely differed, only the layer geometry was
            // rearranged, never the curve data. Either way, curves already on the layers stayed in
            // place while the caller's own insertCurve()/addCurve() calls piled MORE on top, with
            // no error at all — a two-panel fit-plus-residuals graph ended up with 4 curves in the
            // top layer instead of 2, each one duplicated exactly once. newGraph() is documented as
            // the "build from scratch" entry point (graphWindow(name) is the lookup-to-extend one),
            // so clearing every existing layer's curves here — regardless of whether the grid also
            // needs rearranging — matches that contract instead of leaving it to luck.
            if (existing->getRows() != a2 || existing->getCols() != a3)
            {
                existing->setRows(a2);
                existing->setCols(a3);
                existing->arrangeLayers(false, true);
            }
            for (int li = 1; li <= existing->numLayers(); ++li)
            {
                Graph *g = existing->layer(li);
                if (g)
                    for (int ci = g->curveCount() - 1; ci >= 0; --ci)
                        g->removeCurve(ci);
            }
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
	  // interactive=false: a blocking QMessageBox (no Y column selected) would hang an
	  // automated script — the nullptr return already signals failure to the caller.
	  sipRes = sipCpp->waterfallPlot(a0, l, false);
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
    // Resolves a matrix by NAME — fixes the confirmed-live footgun where plot3D("SphereMap", 5)
    // (passing a matrix's name string instead of the Matrix object) silently did the wrong thing.
    // No default for style: a call with exactly one string argument must stay plot3D(name)'s
    // window-lookup form below, never this one.
    Graph3D* plot3DMatrix(const QString&, int style) /PyName=plot3D/;
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

    // Upsert family: name is ALWAYS the first argument. Updates the existing Graph3D window named
    // `name` in place (reusing setMatrix()/setFunction()/setParametricSurface()'s underlying
    // add*Data() calls) if one already exists, otherwise creates a fresh window and names it
    // `name` atomically — no separate setWindowName() call, so no "name already exists" collision
    // is possible for a script that always reaches for newPlot3D() instead of plot3D()+
    // setWindowName(). Confirmed live: a script created via plot3D(formula, ...) then
    // setWindowName(g3, "SinCos3D") raised ValueError whenever an earlier turn had already used
    // that name — this family removes the failure mode at the API level instead of relying on the
    // caller to check plot3D(name) first.
    //
    // Each overload is %MethodCode-wrapped to check plot3DNameCollisionError() FIRST: if `name`
    // is already used by a window that is NOT a Graph3D, raise ValueError immediately rather than
    // creating an orphaned Graph3D first and only then failing on the rename, the same way
    // plot3D(...)+setWindowName() does.
    Graph3D* newPlot3D(const QString& name, Matrix*, int style = 5);
%MethodCode
    const QString collisionError = sipCpp->plot3DNameCollisionError(*a0);
    if (!collisionError.isEmpty()) {
        PyErr_SetString(PyExc_ValueError, collisionError.toLocal8Bit().constData());
        sipError = sipErrorFail;
    } else
        sipRes = sipCpp->newPlot3D(*a0, a1, a2);
%End
    Graph3D* newPlot3D(const QString& name, const QString& matrixName, int style = 5);
%MethodCode
    const QString collisionError = sipCpp->plot3DNameCollisionError(*a0);
    if (!collisionError.isEmpty()) {
        PyErr_SetString(PyExc_ValueError, collisionError.toLocal8Bit().constData());
        sipError = sipErrorFail;
    } else
        sipRes = sipCpp->newPlot3D(*a0, *a1, a2);
%End
    Graph3D* newPlot3D(const QString& name, Table* table, const QString&, int = 0);
%MethodCode
    const QString collisionError = sipCpp->plot3DNameCollisionError(*a0);
    if (!collisionError.isEmpty()) {
        PyErr_SetString(PyExc_ValueError, collisionError.toLocal8Bit().constData());
        sipError = sipErrorFail;
    } else
        sipRes = sipCpp->newPlot3D(*a0, a1, *a2, a3);
%End
    Graph3D* newPlot3D(const QString& name, const QString& formula, double xl, double xr,
            double yl, double yr, double zl, double zr, int=40, int=40);
%MethodCode
    const QString collisionError = sipCpp->plot3DNameCollisionError(*a0);
    if (!collisionError.isEmpty()) {
        PyErr_SetString(PyExc_ValueError, collisionError.toLocal8Bit().constData());
        sipError = sipErrorFail;
    } else
        sipRes = sipCpp->newPlot3D(*a0, *a1, a2, a3, a4, a5, a6, a7, a8, a9);
%End
    Graph3D* newPlot3D(const QString& name, const QString&, const QString&, const QString&,
            double, double, double, double, int=40, int=40, bool=true, bool=true);
%MethodCode
    const QString collisionError = sipCpp->plot3DNameCollisionError(*a0);
    if (!collisionError.isEmpty()) {
        PyErr_SetString(PyExc_ValueError, collisionError.toLocal8Bit().constData());
        sipError = sipErrorFail;
    } else
        sipRes = sipCpp->newPlot3D(*a0, *a1, *a2, *a3, a4, a5, a6, a7, a8, a9, a10, a11);
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

  bool setWindowName(MdiSubWindow *, const QString &) /PyName=setWindowName/;
%MethodCode
    // interactive=false: no QMessageBox — a blocking modal dialog would hang an automated
    // script waiting for a human to dismiss it, instead of letting an exception propagate.
    sipRes = sipCpp->setWindowName(a0, *a1, false);
    if (!sipRes) {
        const QString reason = sipCpp->alreadyUsedName(*a1)
            ? "already exists"
            : "is empty or contains invalid characters (only letters and digits are allowed)";
        PyErr_SetString(PyExc_ValueError,
            QString("setWindowName: name '%1' %2.").arg(*a1, reason).toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End
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
%MethodCode
  // interactive=false: a blocking "hide or delete?" QMessageBox would hang an automated
  // script — delete the window outright instead, matching removeWindows()'s bulk-close behavior.
  sipCpp->closeWindow(a0, true, false);
%End
  void activateWindow(MdiSubWindow *);
  bool activateWindow(QString);
  void hideWindow(MdiSubWindow *);
  void maximizeWindow(MdiSubWindow *);
  void maximizeWindow(const QString &);

  // project lifecycle
  // save: None asks to save unsaved changes (Cancel keeps the project), False discards them, True saves first.
  // Both keep this window, unlike the menu's "New Project": a running script keeps its application.
  bool closeProject(SIP_PYOBJECT save = Py_None);
%MethodCode
  int policy = sipqti_savePolicy(a0);
  if (policy == 1 && !sipCpp->saved && sipCpp->projectname == "untitled") {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, "closeProject: the project has no file yet: saveProjectAs(path) first, "
                                      "or save=False to discard it");
  } else
    sipRes = sipCpp->closeProject(policy);
%End
  bool newProject(SIP_PYOBJECT save = Py_None);
%MethodCode
  int policy = sipqti_savePolicy(a0);
  if (policy == 1 && !sipCpp->saved && sipCpp->projectname == "untitled") {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, "newProject: the project has no file yet: saveProjectAs(path) first, "
                                      "or save=False to discard it");
  } else
    sipRes = sipCpp->closeProject(policy);
%End
  // Closes the current project (save as for closeProject), then opens fileName (.qti, .qti.gz, .qti.svg) here.
  bool openProject(const QString &fileName, SIP_PYOBJECT save = Py_None);
%MethodCode
  int policy = sipqti_savePolicy(a1);
  const QFileInfo fi(*a0);
  const QString lower = a0->toLower();
  if (!fi.isFile() || !fi.isReadable()) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, QString("openProject: no readable file %1").arg(*a0).toUtf8().constData());
  } else if (!lower.endsWith(".qti") && !lower.endsWith(".qti.gz") && !lower.endsWith(".qti.svg")) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError,
                    QString("openProject: not a project file (.qti, .qti.gz, .qti.svg): %1").arg(*a0).toUtf8().constData());
  } else if (policy == 1 && !sipCpp->saved && sipCpp->projectname == "untitled") {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, "openProject: the current project has no file yet: saveProjectAs(path) first, "
                                      "or save=False to discard it");
  } else if (!sipCpp->closeProject(policy))
    sipRes = false; // the user kept the current project
  else if (!sipCpp->openProjectInPlace(fi.absoluteFilePath())) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, QString("openProject: %1 could not be opened").arg(*a0).toUtf8().constData());
  } else
    sipRes = true;
%End
  bool saveProject(bool compress = false);
  void saveGraphAsProject(MultiLayer *graph, const QString &path, const QString &name, bool compress = false, const QString &format = QString(), int resolution = -1);

  // existence
  bool existWindow(const QString &);
  bool checkTableExistence(const QString &tableName) /PyName=existTable/;
  bool checkNoteExistence(const QString &noteName) /PyName=existNote/;
  bool checkMatrixExistence(const QString &matrixName) /PyName=existMatrix/;
  bool checkGraphExistence(const QString &graphName) /PyName=existGraph/;
  bool checkPlot3DExistence(const QString &plotName) /PyName=existPlot3D/;

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

  // console output — rendered in a distinct color (see ApplicationWindow::scriptPrintHighlighted()
  // in the C++ source) so a script's own reported results stand out from AI status lines, which
  // print via the plain, uncolored ApplicationWindow::scriptPrint()/scriptPrintReplace() directly.
  void scriptPrint(const QString &);
%MethodCode
    sipCpp->scriptPrintHighlighted(*a0);
%End
  void scriptPrintReplace(const QString &);
%MethodCode
    sipCpp->scriptPrintReplaceHighlighted(*a0);
%End

  // AI agents (the presets of Preferences > Ask AI) and the settings of that page
  QStringList aiAgents() const;
  static QString aiAgent();
  bool setAiAgent(const QString &name);
%MethodCode
  if (!sipCpp->setAiAgent(*a0)) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, QString("setAiAgent: no AI agent %1; agents: %2")
                                          .arg(*a0, sipCpp->aiAgents().join(", ")).toUtf8().constData());
  } else
    sipRes = true;
%End
  void aiClearHistory();
  SIP_PYOBJECT aiConfig();
%MethodCode
  // the "Ask AI" page: the agent's fields (read only here; setAiAgent() changes them) and the global settings
  const AiSettings &st = sipCpp->aiSettings;
  AiAssistant *ai = sipCpp->aiAssistant();
  sipRes = PyDict_New();
  auto put = [&](const char *key, PyObject *value) { PyDict_SetItemString(sipRes, key, value); Py_DECREF(value); };
  const QString agent = sipCpp->aiAgent();
  QSettings preset(sipCpp->sasPath + "ai/" + agent + ".ai", QSettings::IniFormat);
  const QString baseUrl = ai ? ai->baseUrl() : preset.value("AI/BaseUrl").toString();
  const QString model = ai ? ai->model() : preset.value("AI/Model").toString();
  const int maxTokens = ai ? ai->maxTokens() : preset.value("AI/MaxTokens", 4096).toInt();
  const int docsMode = static_cast<int>(ai ? ai->docsMode() : AiAssistant::docsModeFromSettings(preset));
  static const char *docsModes[] = {"compact", "full", "integrated"};
  put("agent", PyUnicode_FromString(agent.toUtf8().constData()));
  put("baseUrl", PyUnicode_FromString(baseUrl.toUtf8().constData()));
  put("model", PyUnicode_FromString(model.toUtf8().constData()));
  put("maxTokens", PyLong_FromLong(maxTokens));
  put("docsMode", PyUnicode_FromString(docsModes[qBound(0, docsMode, 2)]));
  put("bigModel", PyBool_FromLong(ai ? ai->bigModel() : AiAssistant::bigModelFromSettings(preset)));
  put("maxUserRules", PyLong_FromLong(st.maxUserRules));
  put("maxAutoContinue", PyLong_FromLong(st.maxAutoContinue));
  put("splitByDefault", PyBool_FromLong(st.splitByDefault));
  put("maxSplitParts", PyLong_FromLong(st.maxSplitParts));
  put("splitRequestMerge", PyBool_FromLong(st.splitRequestMerge));
  put("selfReviewEnabled", PyBool_FromLong(st.selfReviewEnabled));
  put("exploreEnabled", PyBool_FromLong(st.exploreEnabled));
  put("requestTimeoutSec", PyLong_FromLong(st.requestTimeoutSec));
%End
  void setAiConfig(const QString &key, SIP_PYOBJECT value);
%MethodCode
  // the global settings of the "Ask AI" page, with the page's own ranges; the agent's fields via setAiAgent()
  AiSettings &st = sipCpp->aiSettings;
  struct IntField { const char *key; int *field; int min, max; };
  struct BoolField { const char *key; bool *field; };
  const IntField ints[] = {{"maxUserRules", &st.maxUserRules, 1, 200}, {"maxAutoContinue", &st.maxAutoContinue, 1, 50},
                           {"maxSplitParts", &st.maxSplitParts, 2, 30},
                           {"requestTimeoutSec", &st.requestTimeoutSec, 0, 3600}};
  const BoolField bools[] = {{"splitByDefault", &st.splitByDefault}, {"splitRequestMerge", &st.splitRequestMerge},
                             {"selfReviewEnabled", &st.selfReviewEnabled}, {"exploreEnabled", &st.exploreEnabled}};
  QString error;
  bool done = false;
  for (const IntField &f : ints)
    if (*a0 == f.key) {
      done = true;
      const long v = PyLong_Check(a1) && !PyBool_Check(a1) ? PyLong_AsLong(a1) : LONG_MIN;
      if (v < f.min || v > f.max)
        error = QString("setAiConfig: %1 must be an integer %2..%3").arg(f.key).arg(f.min).arg(f.max);
      else
        *f.field = static_cast<int>(v);
    }
  for (const BoolField &f : bools)
    if (*a0 == f.key) {
      done = true;
      if (!PyBool_Check(a1))
        error = QString("setAiConfig: %1 must be True or False").arg(f.key);
      else
        *f.field = a1 == Py_True;
    }
  if (!done) {
    const QStringList agentKeys = {"agent", "baseUrl", "model", "maxTokens", "docsMode", "bigModel"};
    error = agentKeys.contains(*a0) ? QString("setAiConfig: %1 belongs to the agent: use setAiAgent(name)").arg(*a0)
                                    : QString("setAiConfig: no setting %1").arg(*a0);
  }
  if (!error.isEmpty()) {
    sipIsErr = 1;
    PyErr_SetString(PyExc_ValueError, error.toUtf8().constData());
  }
%End

  // For the script server's AI modes (qtisas/python/server) — internal, deliberately not in the API docs: an AI
  // script must never ask the AI itself.
  void askAi(const QString &text, bool autoRun = false) /PyName=_askAi/;
%MethodCode
    sipCpp->askAi(*a0, a1, false);
%End
  bool askAiChat(const QString &text) /PyName=_askAiChat/;
  bool aiIdle() const /PyName=_aiIdle/;
  void aiStop() /PyName=_aiStop/;
  int _aiTranscriptSize() const;
%MethodCode
    sipRes = static_cast<int>(sipCpp->aiTranscript().size());
%End
  QStringList _aiTranscript(int from = 0) const;
%MethodCode
    sipRes = new QStringList(sipCpp->aiTranscript().mid(a0));
%End

  // AI system-prompt rules — manually curated, capped, shown in Help > Python API > Ai
  bool addRule(const QString &text) /PyName=addRule/;
%MethodCode
    QString err;
    sipRes = sipCpp->addRule(*a0, &err);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, err.toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End
  bool removeRule(int index) /PyName=removeRule/;
%MethodCode
    QString err;
    sipRes = sipCpp->removeRule(a0, &err);
    if (!sipRes) {
        PyErr_SetString(PyExc_ValueError, err.toLocal8Bit().constData());
        sipError = sipErrorFail;
    }
%End
  bool removeAllRules() /PyName=removeAllRules/;
%MethodCode
    sipRes = sipCpp->removeAllRules();
    if (!sipRes) {
        PyErr_SetString(PyExc_RuntimeError, "removeAllRules: failed to write rules file.");
        sipError = sipErrorFail;
    }
%End
  QStringList showRules() /PyName=showRules/;

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
// save=None / False / True of closeProject(), newProject(), openProject(): -1 ask, 0 discard, 1 save first
int sipqti_savePolicy(PyObject *save)
{
  if (!save || save == Py_None)
    return -1;
  return PyObject_IsTrue(save) ? 1 : 0;
}
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

// Fit-function parameter names become plain C++ locals in the generated code (see
// compile-python-api.md) — a name that collides with a C++ reserved keyword (confirmed live:
// "switch") fails to compile, but with a *misattributed* error blaming some other, unrelated
// parameter declared later in the same boilerplate block, since the bad declaration corrupts
// everything that follows it. Checked at the Python boundary so the real cause is reported
// immediately instead of triggering a confusing multi-retry loop that never finds it.
bool sipqti_isCppReservedWord(const QString &name)
{
    static const QSet<QString> kCppKeywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
        "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept",
        "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await",
        "co_return", "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast",
        "else", "enum", "explicit", "export", "extern", "false", "float", "for", "friend", "goto",
        "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
        "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register",
        "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static",
        "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local",
        "throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using",
        "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
    };
    return kCppKeywords.contains(name);
}

// Shared by save()/compile()'s %MethodCode above — see their comments for why this matters
// (a case-insensitive filesystem silently splits one function's .fif/.dylib into a mismatched-
// case pair, which then vanishes from every Fittable function list under either name). Returns
// the existing, differently-cased name `stem` collides with, or an empty QString if none.
QString sipqti_caseInsensitiveCollision(compile18 *c, const QString &stem)
{
    for (const QString &existing : c->groupFunctions("ALL"))
        if (existing != stem && existing.compare(stem, Qt::CaseInsensitive) == 0)
            return existing;
    return QString();
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
#include "compile18.h"
#include <QSet>
bool sipqti_isCppReservedWord(const QString &name);
QString sipqti_caseInsensitiveCollision(compile18 *c, const QString &stem);
int sipqti_savePolicy(PyObject *save);
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
    // X/Y are convenience aliases for the common case (X=horizontal=Bottom, Y=vertical=Left) — added
    // because scripts repeatedly guess Graph.Axis.X/.Y by analogy with Table.PlotDesignation.X/.Y (a
    // different enum, on a different class); this makes that guess work instead of raising AttributeError.
    PyDict_SetItemString(d, "X",      PyLong_FromLong(::Graph::Bottom));
    PyDict_SetItemString(d, "Y",      PyLong_FromLong(::Graph::Left));
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
