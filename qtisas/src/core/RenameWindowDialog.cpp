/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Rename window dialog
 ******************************************************************************/

#include "RenameWindowDialog.h"
#include "ApplicationWindow.h"

#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QRadioButton>
#include <QMessageBox>
#include <QButtonGroup>
#include <QRegExp>
#include <QHBoxLayout>
#include <QGridLayout>

RenameWindowDialog::RenameWindowDialog(QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
	setWindowTitle(tr("QtiSAS - Rename Window"));
	setAttribute(Qt::WA_DeleteOnClose);

	QGridLayout * leftLayout = new QGridLayout();
	QVBoxLayout * rightLayout = new QVBoxLayout();

	groupBox1 = new QGroupBox(tr("Window Title"));
	groupBox1->setLayout(leftLayout);

	boxName = new QRadioButton(tr("&Name (single word)"));
	leftLayout->addWidget(boxName, 0, 0);
	boxNameLine = new QLineEdit();
	leftLayout->addWidget(boxNameLine, 0, 1);
	setFocusProxy(boxNameLine);

	boxLabel = new QRadioButton(tr("&Label"));
	leftLayout->addWidget(boxLabel, 2, 0);
	boxLabelEdit = new QTextEdit();
	leftLayout->addWidget(boxLabelEdit, 1, 1, 3, 1);
	boxLabelEdit->setMaximumHeight(100);
	boxLabelEdit->setMinimumHeight(100);

	boxBoth = new QRadioButton(tr("&Both Name and Label"));
	leftLayout->addWidget(boxBoth, 4, 0);

	buttons = new QButtonGroup(this);
	buttons->addButton(boxName);
	buttons->addButton(boxLabel);
	buttons->addButton(boxBoth);

	buttonOk = new QPushButton(tr( "&OK" ));
    buttonOk->setAutoDefault( true );
    buttonOk->setDefault( true );
	rightLayout->addWidget(buttonOk);

    buttonCancel = new QPushButton(tr( "&Cancel" ));
    buttonCancel->setAutoDefault( true );
	rightLayout->addWidget(buttonCancel);
	rightLayout->addStretch();

	QHBoxLayout * mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(groupBox1);
	mainLayout->addLayout(rightLayout);

    // signals and slots connections
    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

void RenameWindowDialog::setWidget(MdiSubWindow *w)
{
	window = w;
	boxNameLine->setText(w->objectName());
    boxNameLine->setSelection(0, w->objectName().length());
	boxLabelEdit->setText(w->windowLabel());
	switch (w->captionPolicy())
	{
		case MdiSubWindow::Name:
			boxName->setChecked(true);
			break;

		case MdiSubWindow::Label:
			boxLabel->setChecked(true);
			break;

		case MdiSubWindow::Both:
			boxBoth->setChecked(true);
			break;
	}
}

MdiSubWindow::CaptionPolicy RenameWindowDialog::getCaptionPolicy()
{
	MdiSubWindow::CaptionPolicy policy = MdiSubWindow::Name;
	if (boxLabel->isChecked())
		policy = MdiSubWindow::Label;
	else if (boxBoth->isChecked())
		policy = MdiSubWindow::Both;

	return policy;
}

void RenameWindowDialog::accept()
{
	QString name = window->name();
	QString text = boxNameLine->text().remove("=").remove(QRegExp("\\s"));
	QString label = boxLabelEdit->toPlainText();

	MdiSubWindow::CaptionPolicy policy = getCaptionPolicy();
	if (text == name && label == window->windowLabel() && window->captionPolicy() == policy)
		close();

	ApplicationWindow *app = (ApplicationWindow *)parentWidget();
	if (!app || !app->setWindowName(window, text))
		return;

	label.replace("\n", " ").replace("\t", " ");
	window->setWindowLabel(label);
	window->setCaptionPolicy(policy);
	app->modifiedProject(window);
	close();
}
