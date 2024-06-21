/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2007 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Implementations of generic scripting classes
 ******************************************************************************/

#include "ScriptingEnv.h"
#include "Script.h"


ScriptingEnv::ScriptingEnv(ApplicationWindow *parent, const char *langName)
    : QObject(nullptr), d_parent(parent)
{
    setObjectName(langName);
	d_initialized=false;
	d_refcount=0;
}

const QString ScriptingEnv::fileFilter() const
{
	QStringList extensions = fileExtensions();
	if (extensions.isEmpty())
		return "";
	else
		return tr("%1 Source (*.%2);").arg(objectName()).arg(extensions.join(" *."));
}

void ScriptingEnv::incref()
{
	d_refcount++;
}

void ScriptingEnv::decref()
{
	d_refcount--;
	if (d_refcount==0)
		delete this;
}

