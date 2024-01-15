/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
Description: Execute Python code from within QtiPlot
 ******************************************************************************/

#ifndef PYTHON_SCRIPT_H
#define PYTHON_SCRIPT_H

#include "Script.h"

class QObject;
class QString;

typedef struct _object PyObject;
class PythonScripting;
class ScriptingEnv;

class PythonScript : public Script
{
	Q_OBJECT

	public:
		PythonScript(PythonScripting *env, const QString &code, QObject *context=0, const QString &name="<input>");
		~PythonScript();

		void write(const QString &text) { emit print(text); }

		public slots:
		bool compile(bool for_eval=true);
		QVariant eval();
		bool exec();
		bool setQObject(QObject *val, const char *name);
		bool setInt(int val, const char* name);
		bool setDouble(double val, const char* name);
		void setContext(QObject *context);

	private:
		PythonScripting *env() { return (PythonScripting*)Env; }
		void beginStdoutRedirect();
		void endStdoutRedirect();

		PyObject *PyCode, *modLocalDict, *modGlobalDict, *stdoutSave, *stderrSave;
		bool isFunction, hasOldGlobals;
};


#endif
