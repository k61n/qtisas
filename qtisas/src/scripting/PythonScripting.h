/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Execute Python code from within QtiPlot
 ******************************************************************************/

#ifndef PYTHONSCRIPTING_H
#define PYTHONSCRIPTING_H

#include "PythonScript.h"
#include "ScriptingEnv.h"

typedef struct _object PyObject;

class PythonScripting: public ScriptingEnv
{
	Q_OBJECT

	public:
		static const char *langName;
		PythonScripting(ApplicationWindow *parent);
		~PythonScripting();
		static ScriptingEnv *constructor(ApplicationWindow *parent) { return new PythonScripting(parent); }
    bool initialize() override;

		void write(const QString &text) { emit print(text); }

		//! like str(object) in Python
		/**
		 * Convert object to a string.
		 * Steals a reference to object if decref is true; borrows otherwise.
		 */
		QString toString(PyObject *object, bool decref=false);
		//! evaluate a Python expression
		/**
		 * Evaluates code, using argDict (borrowed reference) as local dictionary
		 * or an empty one if argDict==nullptr. name is the filename Python uses when
		 * reporting errors. Returns a new reference; nullptr means caller has to do
		 * exception handling.
		 */
		PyObject* eval(const QString &code, PyObject *argDict=nullptr, const char *name="<qtiplot>");
		//! execute a sequence of Python statements
		/**
		 * Executes code, using argDict (borrowed reference) as local dictionary
		 * or an empty one if argDict==nullptr. name is the filename Python uses when
		 * reporting errors. A false return value means caller has to do exception
		 * handling.
		 */
		bool exec(const QString &code, PyObject *argDict=nullptr, const char *name="<qtiplot>");
		QString errorMsg();

    bool isRunning() const override;
    Script *newScript(const QString &code, QObject *context, const QString &name) override
		{
			return new PythonScript(this, code, context, name);
		}

		bool setQObject(QObject*, const char*, PyObject *dict);
    bool setQObject(QObject *val, const char *name) override
    {
        return setQObject(val, name, nullptr);
    }
    bool setInt(int, const char *, PyObject *dict = nullptr);
		bool setDouble(double, const char*, PyObject *dict=nullptr);

    const QStringList mathFunctions() const override;
    QString mathFunctionDoc(const QString &name) const override;
    const QStringList fileExtensions() const override;

		PyObject *globalDict() { return globals; }
		PyObject *sysDict() { return sys; }

	private:
		bool loadInitFile(const QString &path);

		PyObject *globals;		// PyDict of global environment
		PyObject *math;		// PyDict of math functions
		PyObject *sys;		// PyDict of sys module
};

#endif
