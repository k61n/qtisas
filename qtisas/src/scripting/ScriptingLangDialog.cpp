/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Dialog for changing the current scripting
 ******************************************************************************/

#include "ScriptingLangDialog.h"
#include <ApplicationWindow.h>

#include <QListWidget>
#include <QPushButton>
#include <QLayout>
#include <QMessageBox>

ScriptingLangDialog::ScriptingLangDialog(ScriptingEnv *env, ApplicationWindow *parent, Qt::WindowFlags fl )
: QDialog(parent, fl), scripted(env)
{
	setWindowTitle(tr("QTISAS - Select scripting language"));

	langList = new QListWidget(this);

	btnOK = new QPushButton(tr("OK"));
	btnCancel = new QPushButton(tr("Cancel"));

	QHBoxLayout *hbox1 = new QHBoxLayout();
    hbox1->addStretch();
	hbox1->addWidget(btnOK);
	hbox1->addWidget(btnCancel);

	QVBoxLayout *vl = new QVBoxLayout(this);
	vl->addWidget(langList);
	vl->addLayout(hbox1);

	connect(btnOK, SIGNAL(clicked()), this, SLOT(accept()));
	connect(btnCancel, SIGNAL(clicked()), this, SLOT(close()));
	connect(langList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(accept()));

	updateLangList();
}

void ScriptingLangDialog::updateLangList()
{
	langList->clear();
	langList->insertItems(0, ScriptingLangManager::languages());
	QListWidgetItem *current = langList->findItems(scriptEnv->objectName(), Qt::MatchExactly).first();
	if (current)
		langList->setCurrentItem(current);
}

void ScriptingLangDialog::accept()
{
	ApplicationWindow *app = (ApplicationWindow*) parent();
	if (app->setScriptingLanguage(langList->currentItem()->text()))
		close();
	else
		QMessageBox::critical(this, tr("QTISAS - Scripting Error"),
				tr("Scripting language \"%1\" failed to initialize.").arg(langList->currentItem()->text()));
}
