/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
Description: Dialog for changing the current scripting
 ******************************************************************************/

#ifndef SCRIPTING_LANG_DIALOG_H
#define SCRIPTING_LANG_DIALOG_H

#include "ScriptingEnv.h"
#include "Script.h"

#include <QDialog>

class ApplicationWindow;
class QListWidget;
class QPushButton;

class ScriptingLangDialog: public QDialog, public scripted
{
	Q_OBJECT

	public:
		ScriptingLangDialog(ScriptingEnv *env, ApplicationWindow *parent, Qt::WindowFlags fl = 0);

	public slots:
		void updateLangList();
		void accept();

	private:
		QListWidget *langList;
		QPushButton *btnOK, *btnCancel;
};

#endif

