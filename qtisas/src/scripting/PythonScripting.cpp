/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Execute Python code from within QtiSAS
 ******************************************************************************/

#include <Python.h>
#include <compile.h>
#if PY_VERSION_HEX < 0x030B0000
#include <eval.h>
#endif
#include <frameobject.h>
#include <traceback.h>

#include "ApplicationWindow.h"
#include "PythonScript.h"
#include "PythonScripting.h"

#include "sipAPIqti.h"


extern "C" PyObject* PyInit_qti();

const char* PythonScripting::langName = "Python";

QString PythonScripting::toString(PyObject *object, bool decref)
{
	PyGILState_STATE state = PyGILState_Ensure();
	QString ret("");
	if (object)
	{
		PyObject *repr = PyObject_Str(object);
		if (decref) Py_DECREF(object);
		if (repr)
		{
			ret = PyUnicode_AsUTF8(repr);
			Py_DECREF(repr);
		}
	}
	PyGILState_Release(state);
	return ret;
}

PyObject *PythonScripting::eval(const QString &code, PyObject *argDict, const char *name)
{
	PyGILState_STATE state = PyGILState_Ensure();
	PyObject *args;
	if (argDict)
		args = argDict;
	else
		args = globals;
	PyObject *ret=nullptr;
    PyObject *co = Py_CompileString(code.toLocal8Bit().constData(), name, Py_eval_input);
	if (co)
	{
		ret = PyEval_EvalCode((PyObject*)co, globals, args);
		Py_DECREF(co);
	}
	PyGILState_Release(state);
	return ret;
}

bool PythonScripting::exec (const QString &code, PyObject *argDict, const char *name)
{
	PyGILState_STATE state = PyGILState_Ensure();
	PyObject *args;
	if (argDict)
		args = argDict;
	else
		// "local" variable assignments automatically become global:
		args = globals;
	PyObject *tmp = nullptr;
    PyObject *co = Py_CompileString(code.toLocal8Bit().constData(), name, Py_file_input);
	if (co)
	{
		tmp = PyEval_EvalCode((PyObject*)co, globals, args);
		Py_DECREF(co);
	}
	if (tmp) Py_DECREF(tmp);
	PyGILState_Release(state);
	return (bool) tmp;
}

QString PythonScripting::errorMsg()
{
	PyGILState_STATE state = PyGILState_Ensure();
	PyObject *exception=0, *value=0, *traceback=0;
	PyTracebackObject *excit=0;
	PyCodeObject *code;
	const char *fname;
	QString msg;
	if (!PyErr_Occurred())
	{
		PyGILState_Release(state);
		return "";
	}
	PyErr_Fetch(&exception, &value, &traceback);
	PyErr_NormalizeException(&exception, &value, &traceback);
	if(PyErr_GivenExceptionMatches(exception, PyExc_SyntaxError))
	{
		QString text = toString(PyObject_GetAttrString(value, "text"), true);
		msg.append(text + "\n");
		PyObject *offset = PyObject_GetAttrString(value, "offset");
		for (int i=0; i<(PyLong_AsLong(offset)-1); i++)
			if (text[i] == '\t')
				msg.append("\t");
			else
				msg.append(" ");
		msg.append("^\n");
		Py_DECREF(offset);
		msg.append("SyntaxError: ");
		msg.append(toString(PyObject_GetAttrString(value, "msg"), true) + "\n");
		msg.append("at ").append(toString(PyObject_GetAttrString(value, "filename"), true));
		msg.append(":").append(toString(PyObject_GetAttrString(value, "lineno"), true));
		msg.append("\n");
		Py_DECREF(exception);
		Py_DECREF(value);
	} else {
		msg.append(toString(exception,true)).remove("exceptions.").append(": ");
		msg.append(toString(value,true));
		msg.append("\n");
	}

	if (traceback) {
		excit = (PyTracebackObject*)traceback;
		while (excit && (PyObject*)excit != Py_None)
		{
			code = PyFrame_GetCode(excit->tb_frame);
			msg.append("at ").append(PyUnicode_AsUTF8(code->co_filename));
			msg.append(":").append(QString::number(excit->tb_lineno));
			if (code->co_name && *(fname = PyUnicode_AsUTF8(code->co_name)) != '?')
				msg.append(" in ").append(fname);
			msg.append("\n");
            Py_DECREF(code);
			excit = excit->tb_next;
		}
		Py_DECREF(traceback);
	}

	PyGILState_Release(state);
	return msg;
}

PythonScripting::PythonScripting(ApplicationWindow *parent)
	: ScriptingEnv(parent, langName)
{
	PyGILState_STATE state;
	PyObject *mainmod=nullptr, *qtimod=nullptr, *sysmod=nullptr;
	math = nullptr;
	sys = nullptr;
	d_initialized = false;
	if (Py_IsInitialized())
	{
		state = PyGILState_Ensure();
		mainmod = PyImport_ImportModule("__main__");
		if (!mainmod)
		{
			PyErr_Print();
			PyGILState_Release(state);
			return;
		}
		globals = PyModule_GetDict(mainmod);
		Py_DECREF(mainmod);
		PyGILState_Release(state);
	} else {
        PyImport_AppendInittab("qti", &PyInit_qti);
		Py_Initialize ();
		if (!Py_IsInitialized ())
			return;

		mainmod = PyImport_AddModule("__main__");
		if (!mainmod)
		{
			PyErr_Print();
			PyEval_SaveThread();
			return;
		}
		globals = PyModule_GetDict(mainmod);

		/* Swap out and return current thread state and release the GIL */
		/*PyThreadState *tstate =*/
		PyEval_SaveThread();
	}

	state = PyGILState_Ensure();
	if (!globals)
	{
		PyErr_Print();
		PyGILState_Release(state);
		return;
	}
	Py_INCREF(globals);

	math = PyDict_New();
	if (!math)
		PyErr_Print();

	qtimod = PyImport_ImportModule("qti");
	if (qtimod)
	{
		PyDict_SetItemString(globals, "qti", qtimod);
		PyObject *qtiDict = PyModule_GetDict(qtimod);
		if (!setQObject(d_parent, "app", qtiDict))
			QMessageBox::warning(d_parent, tr("Failed to export QtiPlot API"),
			tr("Accessing QtiPlot functions or objects from Python code won't work. "\
			"Probably your version of Qt/SIP/PyQt differs from the one QtiPlot was compiled against."));

		PyDict_SetItemString(qtiDict, "mathFunctions", math);
		Py_DECREF(qtimod);
	} else
		PyErr_Print();

	sysmod = PyImport_ImportModule("sys");
	if (sysmod)
	{
		sys = PyModule_GetDict(sysmod);
		Py_INCREF(sys);
	} else
		PyErr_Print();

	PyGILState_Release(state);
	d_initialized = true;
}

bool PythonScripting::initialize()
{
    if (!d_initialized)
        return false;

    // Redirect output to print(const QString&) via Python's sys.stdout and sys.stderr
    setQObject(this, "stdout", sys);
    setQObject(this, "stderr", sys);

    const QString initFilePath = d_parent->d_python_config_folder + "/qtisasrc";

    // Try to load the init file
    if (loadInitFile(initFilePath))
        return true;

    // Attempt to restore config files and try again
    if (QMessageBox::question(d_parent, "QtiSAS", "Python initialization failed.\n\nReset to default files?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        d_parent->copyPythonConfigurationFiles(true);
        if (loadInitFile(initFilePath))
            return true;
    }

    // If it still fails, notify the user
    QMessageBox::critical(d_parent, "QtiSAS",
                          "Python initialization failed again.\n\n"
                          "Try to set the correct Python config path in Preferences.");

    return false;
}

PythonScripting::~PythonScripting()
{
	PyGILState_STATE state = PyGILState_Ensure();
	Py_XDECREF(globals);
	Py_XDECREF(math);
	Py_XDECREF(sys);
	PyGILState_Release(state);
}

bool PythonScripting::loadInitFile(const QString &path)
{
	PyGILState_STATE state = PyGILState_Ensure();
	QFileInfo pyFile(path+".py"), pycFile(path+".pyc");
	bool success = false;
	if (pycFile.isReadable() && (pycFile.lastModified() >= pyFile.lastModified())) {
		// if we have a recent pycFile, use it
		FILE *f = fopen(pycFile.filePath().toLocal8Bit().constData(), "rb");
		success = PyRun_SimpleFileEx(f, pycFile.filePath().toLocal8Bit().constData(), false) == 0;
		fclose(f);
	} else if (pyFile.isReadable() && pyFile.exists()) {
		// try to compile pyFile to pycFile
		PyObject *compileModule = PyImport_ImportModule("py_compile");
		if (compileModule) {
			PyObject *compile = PyDict_GetItemString(PyModule_GetDict(compileModule), "compile");
			if (compile) {
				PyObject *tmp = PyObject_CallFunctionObjArgs(compile,
						PyUnicode_FromString(pyFile.filePath().toLocal8Bit().constData()),
						PyUnicode_FromString(pycFile.filePath().toLocal8Bit().constData()),
						nullptr);
				if (tmp)
					Py_DECREF(tmp);
				else
					PyErr_Print();
			} else
				PyErr_Print();
			Py_DECREF(compileModule);
		} else
			PyErr_Print();
		pycFile.refresh();
		if (pycFile.isReadable() && (pycFile.lastModified() >= pyFile.lastModified())) {
			// run the newly compiled pycFile
			FILE *f = fopen(pycFile.filePath().toLocal8Bit().constData(), "rb");
			success = PyRun_SimpleFileEx(f, pycFile.filePath().toLocal8Bit().constData(), false) == 0;
			fclose(f);
		} else {
			// fallback: just run pyFile
            FILE *f = fopen(pyFile.filePath().toLocal8Bit().constData(), "r");
            success = PyRun_SimpleFileEx(f, pyFile.filePath().toLocal8Bit().constData(), false) == 0;
            fclose(f);
		}
	}
	PyGILState_Release(state);
	return success;
}

bool PythonScripting::isRunning() const
{
	PyGILState_STATE state = PyGILState_Ensure();
	bool isinit = Py_IsInitialized();
	PyGILState_Release(state);
	return isinit;
}

bool PythonScripting::setQObject(QObject *val, const char *name, PyObject *dict)
{
	if(!val) return false;
	PyObject *pyobj=nullptr;

	PyGILState_STATE state = PyGILState_Ensure();

    const sipTypeDef *klass = sipFindType(val->metaObject()->className());
	if (klass) pyobj = sipConvertFromType(val, klass, nullptr);

	if (pyobj) {
		if (dict)
			PyDict_SetItemString(dict,name,pyobj);
		else
			PyDict_SetItemString(globals,name,pyobj);
		Py_DECREF(pyobj);
	}

	PyGILState_Release(state);
	return (bool) pyobj;
}

bool PythonScripting::setInt(int val, const char *name, PyObject *dict)
{
	PyGILState_STATE state = PyGILState_Ensure();
	PyObject *pyobj = Py_BuildValue("i",val);
	if (pyobj) {
		if (dict)
			PyDict_SetItemString(dict,name,pyobj);
		else
			PyDict_SetItemString(globals,name,pyobj);
		Py_DECREF(pyobj);
	}
	PyGILState_Release(state);
	return (bool) pyobj;
}

bool PythonScripting::setDouble(double val, const char *name, PyObject *dict)
{
	PyGILState_STATE state = PyGILState_Ensure();
	PyObject *pyobj = Py_BuildValue("d",val);
	if (pyobj) {
		if (dict)
			PyDict_SetItemString(dict,name,pyobj);
		else
			PyDict_SetItemString(globals,name,pyobj);
		Py_DECREF(pyobj);
	}
	PyGILState_Release(state);
	return (bool) pyobj;
}

const QStringList PythonScripting::mathFunctions() const
{
	PyGILState_STATE state = PyGILState_Ensure();
	QStringList flist;
	PyObject *key, *value;
    Py_ssize_t i = 0;
	while(PyDict_Next(math, &i, &key, &value))
		if (PyCallable_Check(value))
			flist << PyUnicode_AsUTF8(key);
	PyGILState_Release(state);
	flist.sort();
	return flist;
}

QString PythonScripting::mathFunctionDoc(const QString &name) const
{
	PyGILState_STATE state = PyGILState_Ensure();
	PyObject *mathf = PyDict_GetItemString(math, name.toLocal8Bit().constData()); // borrowed
	QString qdocstr("");

	if (mathf) {
		PyObject *pydocstr = PyObject_GetAttrString(mathf, "__doc__"); // new
		if (pydocstr) qdocstr = PyUnicode_AsUTF8(pydocstr);
		Py_XDECREF(pydocstr);
	}
	PyGILState_Release(state);

	return qdocstr;
}

const QStringList PythonScripting::fileExtensions() const
{
	QStringList extensions;
	extensions << "py";
	return extensions;
}
