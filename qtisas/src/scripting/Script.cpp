/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Implementations of generic scripting classes
 ******************************************************************************/

#include "ScriptingEnv.h"
#include "Script.h"

#include <string.h>

#include "muParserScript.h"
#include "muParserScripting.h"

#ifdef SCRIPTING_PYTHON
#include "PythonScript.h"
#include "PythonScripting.h"
#endif

ScriptingLangManager::ScriptingLang ScriptingLangManager::langs[] = {
	{ muParserScripting::langName, muParserScripting::constructor },
#ifdef SCRIPTING_PYTHON
	{ PythonScripting::langName, PythonScripting::constructor },
#endif
	{ nullptr, nullptr }
};

ScriptingEnv *ScriptingLangManager::newEnv(ApplicationWindow *parent)
{
	if (!langs[0].constructor)
		return nullptr;
	else
		return langs[0].constructor(parent);
}

ScriptingEnv *ScriptingLangManager::newEnv(const char *name, ApplicationWindow *parent)
{
	for (ScriptingLang *i = langs; i->constructor; i++)
		if (!strcmp(name, i->name))
			return i->constructor(parent);
	return nullptr;
}

QStringList ScriptingLangManager::languages()
{
	QStringList l;
	for (ScriptingLang *i = langs; i->constructor; i++)
		l << i->name;
	return l;
}

bool Script::compile(bool for_eval)
{
	Q_UNUSED(for_eval);
	
	emit_error("Script::compile called!", 0);
	return false;
}

QVariant Script::eval()
{
	emit_error("Script::eval called!",0);
	return QVariant();
}

bool Script::exec()
{
	emit_error("Script::exec called!",0);
	return false;
}

scripted::scripted(ScriptingEnv *env)
{
	env->incref();
	scriptEnv = env;
}

scripted::~scripted()
{
	scriptEnv->decref();
}

void scripted::scriptingChangeEvent(ScriptingChangeEvent *sce)
{
	scriptEnv->decref();
	sce->scriptingEnv()->incref();
	scriptEnv = sce->scriptingEnv();
}
