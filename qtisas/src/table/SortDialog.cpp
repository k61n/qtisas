/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Sorting options dialog
 ******************************************************************************/

#include "SortDialog.h"

#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QLayout>
#include <QApplication>

SortDialog::SortDialog( QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
	setWindowTitle(tr("QtiSAS - Sorting Options"));
	setSizeGripEnabled(true);
	setAttribute(Qt::WA_DeleteOnClose);

	QGroupBox *groupBox1 = new QGroupBox();
	QGridLayout * topLayout = new QGridLayout(groupBox1);
	QHBoxLayout * hl = new QHBoxLayout();
	hl->addStretch();

	topLayout->addWidget( new QLabel(tr("Sort columns")), 0, 0 );
	boxType = new QComboBox();
	boxType->addItem(tr("Separately"));
	boxType->addItem(tr("Together"));
	topLayout->addWidget(boxType, 0, 1 );

	topLayout->addWidget( new QLabel( tr("Order")), 1, 0 );
	boxOrder = new QComboBox();
	boxOrder->addItem(QIcon(":/sort_ascending.png"), tr("Ascending"));
	boxOrder->addItem(QIcon(":/sort_descending.png"), tr("Descending"));
	topLayout->addWidget(boxOrder, 1, 1 );

	topLayout->addWidget( new QLabel(tr("Leading column")), 2, 0 );
	columnsList = new QComboBox();
	topLayout->addWidget(columnsList, 2, 1);
	topLayout->setRowStretch(3, 1);

	buttonOk = new QPushButton(tr("&Sort"));
    buttonOk->setDefault( true );
	hl->addWidget(buttonOk);

    buttonCancel = new QPushButton(tr("&Close"));
	hl->addWidget(buttonCancel);

	QVBoxLayout * mainlayout = new QVBoxLayout(this);
    mainlayout->addWidget(groupBox1);
	mainlayout->addLayout(hl);

    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
    connect( boxType, SIGNAL( activated(int) ), this, SLOT(changeType(int)));
}

void SortDialog::accept()
{
	emit sort(boxType->currentIndex(),boxOrder->currentIndex(),columnsList->currentText());
}

void SortDialog::insertColumnsList(const QStringList& cols)
{
	columnsList->addItems(cols);
	columnsList->setCurrentIndex(0);

	boxType->setCurrentIndex(1);
}

void SortDialog::changeType(int Type)
{
	boxType->setCurrentIndex(Type);
	if(Type==1)
		columnsList->setEnabled(true);
	else
		columnsList->setEnabled(false);
}
