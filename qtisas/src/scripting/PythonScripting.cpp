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

#include <algorithm>
#include <atomic>
#include <chrono>

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

// For a NameError on a bare method call ("setDecimalSeparators(2)" meant as "t.setDecimalSeparators(2)"): the
// script objects that have a method of that name. Confirmed live, a weak model repeated the bare call until it gave up;
// the plain "name '...' is not defined" never told it the name was right and only the object missing
static QString methodOwnerHint(PyObject *value)
{
    PyObject *nameObj = PyObject_GetAttrString(value, "name");
    const QString name = nameObj && PyUnicode_Check(nameObj) ? QString::fromUtf8(PyUnicode_AsUTF8(nameObj)) : QString();
    Py_XDECREF(nameObj);
    PyErr_Clear();
    if (name.isEmpty() || name.startsWith("_"))
        return {};
    struct Owner
    {
        const char *module;
        const char *object;
        const char *call;
    };
    static const Owner owners[] = {{"qti", "Table", "t"},
                                   {"qti", "Matrix", "m"},
                                   {"qti", "Note", "n"},
                                   {"qti", "GraphWindow", "g"},
                                   {"qti", "Graph", "layer"},
                                   {"qti", "Graph3D", "g3"},
                                   {"builtins", "Compiler", "Compiler"},
                                   {"builtins", "Fittable", "Fittable"},
                                   {"builtins", "Dan", "Dan"},
                                   {"builtins", "Ascii1d", "Ascii1d"}};
    QStringList types;
    QString example;
    for (const Owner &o : owners)
    {
        PyObject *module = PyImport_ImportModule(o.module);
        PyObject *object = module ? PyObject_GetAttrString(module, o.object) : nullptr;
        if (object && PyObject_HasAttrString(object, name.toUtf8().constData()))
        {
            types << o.object;
            if (example.isEmpty())
                example = QString("%1.%2(...)").arg(o.call, name);
        }
        Py_XDECREF(object);
        Py_XDECREF(module);
        PyErr_Clear();
    }
    if (types.isEmpty())
        return {};
    return QString("'%1' is not a global function but a method of %2 — call it on that object, e.g. %3\n")
        .arg(name, types.join(", "), example);
}

// For an AttributeError: the object's methods whose names contain the missing one ("text" → toPlainText, setText),
// or, on None, that the lookup before it found nothing. Confirmed live: n.tab("C").text() was retried as is, and
// n.tab("Main").setText() on a tab that never existed sent the retries hunting setText(). Python's own "Did you
// mean" only reaches the traceback display, never str(e), which is what the AI and the console get
static QString attributeHint(PyObject *value)
{
    PyObject *obj = PyObject_GetAttrString(value, "obj");
    PyObject *nameObj = PyObject_GetAttrString(value, "name");
    PyErr_Clear();
    const QString name = nameObj && PyUnicode_Check(nameObj) ? QString::fromUtf8(PyUnicode_AsUTF8(nameObj)) : QString();
    Py_XDECREF(nameObj);
    QString hint;
    if (obj == Py_None)
        hint = "The object is None: the call that should have returned it found nothing — e.g. table(\"…\"), "
               "matrix(\"…\") or n.tab(\"…\") for a name that does not exist.\n";
    else if (obj && name.length() >= 3)
    {
        // the missing name as a word of a camelCase name: "text" matches setText/toPlainText, not contextMenuEvent
        QStringList similar;
        PyObject *names = PyObject_Dir(obj);
        for (Py_ssize_t i = 0; names && i < PyList_Size(names); i++)
        {
            PyObject *item = PyList_GetItem(names, i);
            const QString candidate = PyUnicode_Check(item) ? QString::fromUtf8(PyUnicode_AsUTF8(item)) : QString();
            const qsizetype at = candidate.indexOf(name, 0, Qt::CaseInsensitive);
            if (at >= 0 && candidate != name && !candidate.startsWith("_") && (at == 0 || candidate.at(at).isUpper()))
                similar << candidate;
        }
        Py_XDECREF(names);
        PyErr_Clear();
        std::stable_sort(similar.begin(), similar.end(),
                         [](const QString &a, const QString &b) { return a.length() < b.length(); });
        if (!similar.isEmpty())
            hint = QString("'%1' has no '%2'; its methods with a similar name: %3()\n")
                       .arg(QString::fromUtf8(Py_TYPE(obj)->tp_name), name, similar.mid(0, 6).join("(), "));
    }
    Py_XDECREF(obj);
    return hint;
}

QString PythonScripting::errorMsg(const QString &sourceCode, const QString &sourceName)
{
    PyGILState_STATE state = PyGILState_Ensure();
    PyObject *exception = nullptr, *value = nullptr, *traceback = nullptr;
    QString msg;
    if (!PyErr_Occurred())
    {
        PyGILState_Release(state);
        return "";
    }
    PyErr_Fetch(&exception, &value, &traceback);
    PyErr_NormalizeException(&exception, &value, &traceback);
    if (PyErr_GivenExceptionMatches(exception, PyExc_SyntaxError))
    {
        QString text = toString(PyObject_GetAttrString(value, "text"), true);
        msg.append(text + "\n");
        PyObject *offset = PyObject_GetAttrString(value, "offset");
        for (int i = 0; i < (PyLong_AsLong(offset) - 1); i++)
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
    }
    else
    {
        // before toString(value, true) below releases value
        QString hint;
        if (PyErr_GivenExceptionMatches(exception, PyExc_NameError))
            hint = methodOwnerHint(value);
        else if (PyErr_GivenExceptionMatches(exception, PyExc_AttributeError))
            hint = attributeHint(value);
        PyObject *nameAttr = PyObject_GetAttrString(exception, "__name__");
        if (nameAttr)
        {
            msg.append(PyUnicode_AsUTF8(nameAttr));
            Py_DECREF(nameAttr);
        }
        else
        {
            PyErr_Clear();
            msg.append(toString(exception, true)).remove("exceptions.");
        }
        msg.append(": ").append(toString(value, true)).append("\n").append(hint);
    }

    if (traceback)
    {
        PyObject *tbMod = PyImport_ImportModule("traceback");
        if (tbMod)
        {
            PyObject *extractFn = PyObject_GetAttrString(tbMod, "extract_tb");
            if (extractFn)
            {
                PyObject *frames = PyObject_CallFunctionObjArgs(extractFn, traceback, nullptr);
                if (frames)
                {
                    Py_ssize_t nFrames = PySequence_Size(frames);
                    for (Py_ssize_t i = 0; i < nFrames; i++)
                    {
                        PyObject *f = PySequence_GetItem(frames, i);
                        if (!f)
                            continue;
                        PyObject *pyFile = PyObject_GetAttrString(f, "filename");
                        PyObject *pyLine = PyObject_GetAttrString(f, "lineno");
                        PyObject *pyName = PyObject_GetAttrString(f, "name");
                        if (pyFile && pyLine && pyName)
                        {
                            const char *filename = PyUnicode_AsUTF8(pyFile);
                            long lineno = PyLong_AsLong(pyLine);
                            QString fn = filename ? QString::fromUtf8(filename) : QString();
                            bool showFile = fn.contains('/') || fn.contains('\\') ||
                                            (fn.startsWith('<') && fn.endsWith('>') && fn.size() > 2);
                            if (showFile)
                            {
                                msg.append("at ").append(fn).append(":").append(QString::number(lineno));
                                if (!sourceCode.isEmpty() && fn == sourceName)
                                {
                                    const QStringList lines = sourceCode.split('\n');
                                    if (lineno >= 1 && lineno <= lines.size())
                                    {
                                        const QString srcLine = lines[lineno - 1].trimmed();
                                        if (!srcLine.isEmpty())
                                            msg.append(" — ").append(srcLine);
                                    }
                                }
                            }
                            else
                                msg.append("at line:").append(QString::number(lineno));
                            const char *frameName = PyUnicode_AsUTF8(pyName);
                            if (frameName && *frameName != '?' && strcmp(frameName, "<module>") != 0)
                                msg.append(" in ").append(frameName);
                            msg.append("\n");
                        }
                        Py_XDECREF(pyFile);
                        Py_XDECREF(pyLine);
                        Py_XDECREF(pyName);
                        Py_DECREF(f);
                    }
                    Py_DECREF(frames);
                }
                else
                {
                    PyErr_Clear();
                }
                Py_DECREF(extractFn);
            }
            Py_DECREF(tbMod);
        }
        else
        {
            PyErr_Clear();
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

    // Override sys.excepthook so PyQt5/PyQt6 slot exceptions print a traceback
    // instead of calling qFatal() → abort().  Both PyQt5 and PyQt6 only escalate
    // to QMessageLogger::fatal when sys.excepthook is still the built-in default;
    // any custom hook breaks that path and keeps the application alive.
    PyRun_SimpleString("import sys, traceback\n"
                       "def _qtisas_excepthook(exc_type, exc_value, exc_tb):\n"
                       "    traceback.print_exception(exc_type, exc_value, exc_tb)\n"
                       "sys.excepthook = _qtisas_excepthook\n");

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

static std::atomic<long> s_activeTimeLimit{0};
static QByteArray s_timeLimitMessage;

static int raiseTimeLimit(void *id)
{
    // the limit that queued this call may have ended (its script finished) before the call ran
    if (reinterpret_cast<intptr_t>(id) != s_activeTimeLimit.load())
        return 0;
    PyErr_SetString(PyExc_KeyboardInterrupt, s_timeLimitMessage.constData());
    return -1;
}

PythonTimeLimit::PythonTimeLimit(int seconds, const QString &message)
{
    static std::atomic<long> lastId{0};
    const long id = ++lastId;
    s_timeLimitMessage = message.toUtf8();
    s_activeTimeLimit = id;
    d_thread = std::thread([this, seconds, id]() {
        std::unique_lock<std::mutex> lock(d_mutex);
        if (d_wake.wait_for(lock, std::chrono::seconds(seconds), [this]() { return d_done; }))
            return;
        do
            Py_AddPendingCall(raiseTimeLimit, reinterpret_cast<void *>(static_cast<intptr_t>(id)));
        while (!d_wake.wait_for(lock, std::chrono::seconds(5), [this]() { return d_done; }));
    });
}

PythonTimeLimit::~PythonTimeLimit()
{
    s_activeTimeLimit = 0;
    {
        std::lock_guard<std::mutex> lock(d_mutex);
        d_done = true;
    }
    d_wake.notify_all();
    d_thread.join();
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
        // A __doc__ that is None (or not a str) made PyUnicode_AsUTF8() fail and leave its TypeError set; the next
        // __doc__ lookup, numpy's ufunc doc formatter, then ran with that exception pending — confirmed: a cascade of
        // "SystemError: ... returned a result with an exception set" in the console when a ScriptEdit listed the
        // math functions' docs. Only a str is read, and whatever the lookup raised is cleared here.
        if (pydocstr && PyUnicode_Check(pydocstr))
            qdocstr = QString::fromUtf8(PyUnicode_AsUTF8(pydocstr));
        Py_XDECREF(pydocstr);
        PyErr_Clear();
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
