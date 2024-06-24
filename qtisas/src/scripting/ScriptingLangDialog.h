/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Dialog for changing the current scripting
 ******************************************************************************/

#ifndef SCRIPTINGLANGDIALOG_H
#define SCRIPTINGLANGDIALOG_H

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
    ScriptingLangDialog(ScriptingEnv *env, ApplicationWindow *parent, Qt::WindowFlags fl = Qt::WindowFlags());

	public slots:
		void updateLangList();
		void accept();

	private:
		QListWidget *langList;
		QPushButton *btnOK, *btnCancel;
};

#endif
