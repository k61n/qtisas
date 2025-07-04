/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2010 cnicolai <cnicolai@5a6a7de5-fb12-0410-b871-c33778c25c60>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Execute Python code from within QtiSAS
 ******************************************************************************/

#include <Python.h>

#include "PythonScript.h"
#include "PythonScripting.h"
#include "ApplicationWindow.h"

PythonScript::PythonScript(PythonScripting *env, const QString &code, QObject *context, const QString &name)
: Script(env, code, context, name)
{
	PyGILState_STATE state = PyGILState_Ensure();
	PyCode = nullptr;
	// Old: All scripts share a global namespace, and module top-level has its own nonstandard local namespace
	modLocalDict = PyDict_New();
	// A bit of a hack, but we need either IndexError or len() from __builtins__.
	PyDict_SetItemString(modLocalDict, "__builtins__",
			     PyDict_GetItemString(this->env()->globalDict(), "__builtins__"));
	// New: Each script gets its own copy of the global namespace.
	// it is passed as both globals and locals, so top-level assignments are global to this script.
	modGlobalDict = PyDict_Copy(this->env()->globalDict());
	// To read and write program-wide globals, we provide "globals"
	// e.g. ">>> globals.remote_ctl_server = server"
	PyObject *ret;
	ret = PyRun_String(
			   "import __main__\n"
			   "globals = __main__",
			   Py_file_input, modLocalDict, modLocalDict);
	if (ret)
		Py_DECREF(ret);
	else
		PyErr_Print();
	ret = PyRun_String(
			   "import __main__\n"
			   "globals = __main__",
			   Py_file_input, modGlobalDict, modGlobalDict);
	if (ret)
		Py_DECREF(ret);
	else
		PyErr_Print();
	PyGILState_Release(state);
	// "self" is unique to each script, so they can't all run in the __main__ namespace
	setQObject(Context, "self");
}

PythonScript::~PythonScript()
{
	PyGILState_STATE state = PyGILState_Ensure();
	Py_DECREF(modLocalDict);
	Py_DECREF(modGlobalDict);
	Py_XDECREF(PyCode);
	PyGILState_Release(state);
}

void PythonScript::setContext(QObject *context)
{
	Script::setContext(context);
	setQObject(Context, "self");
}

bool PythonScript::compile(bool for_eval)
{
	// Support for the convenient col() and cell() functions.
	// This can't be done anywhere else, because we need access to the local
	// variables self, i and j.
	PyGILState_STATE state = PyGILState_Ensure();
	hasOldGlobals = Code.contains("\nglobal ") || (0 == Code.indexOf("global "));
	PyObject *topLevelLocal = hasOldGlobals ? modLocalDict : modGlobalDict;

	if(Context->inherits("Table")) {
		PyObject *ret = PyRun_String(
				"def col(c,*arg):\n"
				"\ttry: return self.cell(c,arg[0])\n"
				"\texcept(IndexError): return self.cell(c,i)\n"
				"def cell(c,r):\n"
				"\treturn self.cell(c,r)\n"
				"def cellData(c,r):\n"
				"\treturn self.cellData(c,r)\n"
				"def tablecol(t,c):\n"
				"\treturn table(t).cell(c,i)\n"
				"def _meth_table_col_(t,c):\n"
				"\treturn t.cell(c,i)\n"
				"self.__class__.col = _meth_table_col_",
				Py_file_input, topLevelLocal, topLevelLocal);
		if (ret)
			Py_DECREF(ret);
		else
			PyErr_Print();
	} else if(Context->inherits("Matrix")) {
		PyObject *ret = PyRun_String(
				"def cell(*arg):\n"
				"\ttry: return self.cell(arg[0],arg[1])\n"
				"\texcept(IndexError): return self.cell(i,j)\n",
				Py_file_input, topLevelLocal, topLevelLocal);
		if (ret)
			Py_DECREF(ret);
		else
			PyErr_Print();
	}
	bool success=false;
	Py_XDECREF(PyCode);
	
	// Simplest case: Code is a single expression
	PyCode = Py_CompileString(Code.toLocal8Bit().constData(), Name.toLocal8Bit().constData(), Py_eval_input);

	if (PyCode)
		success = true;
	else if (for_eval) {
		// Code contains statements (or errors) and we want to get a return
		// value from it.
		// So we wrap the code into a function definition,
		// execute that (as Py_file_input) and store the function object in PyCode.
		// See http://mail.python.org/pipermail/python-list/2001-June/046940.html
		// for why there isn't an easier way to do this in Python.
		PyErr_Clear(); // silently ignore errors
		PyObject *key, *value;
        Py_ssize_t i = 0;
		QString signature = "";
		while(PyDict_Next(topLevelLocal, &i, &key, &value))
			signature.append(PyUnicode_AsUTF8(key)).append(",");
		signature.truncate(signature.length()-1);
		QString fdef = "def __doit__("+signature+"):\n";
		fdef.append(Code);
		fdef.replace('\n',"\n\t");
		PyCode = Py_CompileString(fdef.toLocal8Bit().constData(), Name.toLocal8Bit().constData(), Py_file_input);
		if (PyCode){
			PyObject *tmp = PyDict_New();
			Py_XDECREF(PyEval_EvalCode((PyObject*)PyCode, topLevelLocal, tmp));
			Py_DECREF(PyCode);
			PyCode = PyDict_GetItemString(tmp,"__doit__");
			Py_XINCREF(PyCode);
			Py_DECREF(tmp);
		}
		success = (PyCode != nullptr);
	} else {
		// Code contains statements (or errors), but we do not need to get
		// a return value.
		PyErr_Clear(); // silently ignore errors
		PyCode = Py_CompileString(Code.toLocal8Bit().constData(), Name.toLocal8Bit().constData(), Py_file_input);
		success = (PyCode != nullptr);
	}

	if (!success){
		compiled = compileErr;
		emit_error(env()->errorMsg(), 0);
	} else
		compiled = isCompiled;
	PyGILState_Release(state);
	return success;
}

QVariant PythonScript::eval()
{
	if (!isFunction) compiled = notCompiled;
	if (compiled != isCompiled && !compile(true))
		return QVariant();

	PyGILState_STATE state = PyGILState_Ensure();
	PyObject *topLevelGlobal = hasOldGlobals ? env()->globalDict() : modGlobalDict;
	PyObject *topLevelLocal = hasOldGlobals ? modLocalDict : modGlobalDict;
	PyObject *pyret;
	beginStdoutRedirect();
	if (PyCallable_Check(PyCode)){
		PyObject *empty_tuple = PyTuple_New(0);
		pyret = PyObject_Call(PyCode, empty_tuple, topLevelLocal);
		Py_DECREF(empty_tuple);
	} else
		pyret = PyEval_EvalCode((PyObject*)PyCode, topLevelGlobal, topLevelLocal);
	endStdoutRedirect();

    if (!pyret)
    {
        if (!Context->inherits("Table"))
            emit_error(env()->errorMsg(), 0);
        PyErr_Clear(); // silently ignore errors
        PyGILState_Release(state);
        return {};
    }

	QVariant qret = QVariant();
	/* None */
	if (pyret == Py_None)
		qret = QVariant("");
    /* string */
    else if (PyUnicode_Check(pyret))
        qret = QVariant(QString::fromUtf8(PyUnicode_AsUTF8(pyret)));
	/* numeric types */
	else if (PyFloat_Check(pyret))
		qret = QVariant(PyFloat_AS_DOUBLE(pyret));
	else if (PyLong_Check(pyret))
		qret = QVariant((qlonglong)PyLong_AsLong(pyret));
	else if (PyLong_Check(pyret))
		qret = QVariant((qlonglong)PyLong_AsLongLong(pyret));
	else if (PyNumber_Check(pyret)){
		PyObject *number = PyNumber_Float(pyret);
		if (number){
			qret = QVariant(PyFloat_AS_DOUBLE(number));
			Py_DECREF(number);
		}
		/* bool */
	} else if (PyBool_Check(pyret))
		qret = QVariant(pyret==Py_True);
    /* List types */
    else if (PyList_Check(pyret))
    {
        QVariantList list;
        for (Py_ssize_t i = 0; i < PyList_Size(pyret); ++i)
        {
            PyObject *item = PyList_GetItem(pyret, i); // Borrowed reference
            if (item == Py_None)
                list << QVariant();
            else if (PyUnicode_Check(item))
                list << QString::fromUtf8(PyUnicode_AsUTF8(item));
            else if (PyFloat_Check(item))
                list << PyFloat_AS_DOUBLE(item);
            else if (PyLong_Check(item))
                list << (qlonglong)PyLong_AsLongLong(item);
            else if (PyBool_Check(item))
                list << (item == Py_True);
            else
            {
                // fallback
                PyObject *strObj = PyObject_Str(item);
                if (strObj)
                {
                    list << QString::fromUtf8(PyUnicode_AsUTF8(strObj));
                    Py_DECREF(strObj);
                }
            }
        }
        qret = QVariant(list);
    }
    /* Dictionary types */
    else if (PyDict_Check(pyret))
    {
        QVariantMap map;
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(pyret, &pos, &key, &value))
        {
            QString keyStr;
            QVariant valueVar;
            if (PyUnicode_Check(key))
                keyStr = QString::fromUtf8(PyUnicode_AsUTF8(key));
            else
            {
                PyObject *keyStrObj = PyObject_Str(key);
                if (keyStrObj)
                {
                    keyStr = QString::fromUtf8(PyUnicode_AsUTF8(keyStrObj));
                    Py_DECREF(keyStrObj);
                }
            }
            if (value == Py_None)
                valueVar = QVariant();
            else if (PyUnicode_Check(value))
                valueVar = QString::fromUtf8(PyUnicode_AsUTF8(value));
            else if (PyFloat_Check(value))
                valueVar = PyFloat_AS_DOUBLE(value);
            else if (PyLong_Check(value))
                valueVar = (qlonglong)PyLong_AsLongLong(value);
            else if (PyBool_Check(value))
                valueVar = (value == Py_True);
            else
            {
                PyObject *valueStrObj = PyObject_Str(value);
                if (valueStrObj)
                {
                    valueVar = QString::fromUtf8(PyUnicode_AsUTF8(valueStrObj));
                    Py_DECREF(valueStrObj);
                }
            }
            map.insert(keyStr, valueVar);
        }
        qret = QVariant(map);
    }
	if(!qret.isValid()) {
		PyObject *pystring = PyObject_Str(pyret);
		if (pystring) {
            PyObject *asUTF8 = PyUnicode_AsUTF8String(pystring);
			Py_DECREF(pystring);
			if (asUTF8) {
				qret = QVariant(QString::fromUtf8(PyUnicode_AsUTF8(asUTF8)));
				Py_DECREF(asUTF8);
			} else if ( (pystring = PyObject_Str(pyret)) ) {
				// single '=' to assign and test null
				qret = QVariant(QString(PyUnicode_AsUTF8(pystring)));
				Py_DECREF(pystring);
			}
		}
	}

	Py_DECREF(pyret);

	if (PyErr_Occurred()){
		if (PyErr_ExceptionMatches(PyExc_ValueError) ||
			PyErr_ExceptionMatches(PyExc_ZeroDivisionError)){
			PyErr_Clear(); // silently ignore errors
			PyGILState_Release(state);
			return  QVariant("");
		} else {
			emit_error(env()->errorMsg(), 0);
			PyGILState_Release(state);
			return QVariant();
		}
	} else {
		PyGILState_Release(state);
		return qret;
	}
}

bool PythonScript::exec()
{
	if (isFunction) compiled = notCompiled;
	if (compiled != Script::isCompiled && !compile(false))
		return false;
	PyGILState_STATE state = PyGILState_Ensure();
	PyObject *topLevelGlobal = hasOldGlobals ? env()->globalDict() : modGlobalDict;
	PyObject *topLevelLocal = hasOldGlobals ? modLocalDict : modGlobalDict;
	PyObject *pyret;
	beginStdoutRedirect();
	if (PyCallable_Check(PyCode)){
		PyObject *empty_tuple = PyTuple_New(0);
		if (!empty_tuple) {
			emit_error(env()->errorMsg(), 0);
			PyGILState_Release(state);
			return false;
		}
		pyret = PyObject_Call(PyCode,empty_tuple,topLevelLocal);
		Py_DECREF(empty_tuple);
	} else {
		pyret = PyEval_EvalCode((PyObject*)PyCode, topLevelGlobal, topLevelLocal);
	}

	endStdoutRedirect();
	if (pyret) {
		Py_DECREF(pyret);
		PyGILState_Release(state);
		return true;
	}
	emit_error(env()->errorMsg(), 0);
	PyGILState_Release(state);
	return false;
}

void PythonScript::beginStdoutRedirect()
{
	PyGILState_STATE state = PyGILState_Ensure();
	stdoutSave = PyDict_GetItemString(env()->sysDict(), "stdout");
	Py_XINCREF(stdoutSave);
	stderrSave = PyDict_GetItemString(env()->sysDict(), "stderr");
	Py_XINCREF(stderrSave);
	env()->setQObject(this, "stdout", env()->sysDict());
	env()->setQObject(this, "stderr", env()->sysDict());
	PyGILState_Release(state);
}

void PythonScript::endStdoutRedirect()
{
	PyGILState_STATE state = PyGILState_Ensure();
	PyDict_SetItemString(env()->sysDict(), "stdout", stdoutSave);
	Py_XDECREF(stdoutSave);
	PyDict_SetItemString(env()->sysDict(), "stderr", stderrSave);
	Py_XDECREF(stderrSave);
	PyGILState_Release(state);
}

bool PythonScript::setQObject(QObject *val, const char *name)
{
	PyGILState_STATE state = PyGILState_Ensure();
	if (!PyDict_Contains(modLocalDict, PyUnicode_FromString(name)))
		compiled = notCompiled;
	PyGILState_Release(state);
	return (env()->setQObject(val, name, modLocalDict) && env()->setQObject(val, name, modGlobalDict));
}

bool PythonScript::setInt(int val, const char *name)
{
	PyGILState_STATE state = PyGILState_Ensure();
	if (!PyDict_Contains(modLocalDict, PyUnicode_FromString(name)))
		compiled = notCompiled;
	PyGILState_Release(state);
	return (env()->setInt(val, name, modLocalDict) && env()->setInt(val, name, modGlobalDict));
}

bool PythonScript::setDouble(double val, const char *name)
{
	PyGILState_STATE state = PyGILState_Ensure();
	if (!PyDict_Contains(modLocalDict, PyUnicode_FromString(name)))
		compiled = notCompiled;
	PyGILState_Release(state);
	return (env()->setDouble(val, name, modLocalDict) && env()->setDouble(val, name, modGlobalDict));
}
