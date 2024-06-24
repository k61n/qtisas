/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Matrix dimensions dialog
 ******************************************************************************/

#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

#include "DoubleSpinBox.h"
#include "MatrixCommand.h"
#include "MatrixSizeDialog.h"

MatrixSizeDialog::MatrixSizeDialog( Matrix *m, QWidget* parent, Qt::WindowFlags fl )
	: QDialog( parent, fl ),
	d_matrix(m)
{
	setWindowTitle(tr("QtiSAS - Matrix Dimensions"));
	setAttribute(Qt::WA_DeleteOnClose);
	setSizeGripEnabled(true);

	groupBox1 = new QGroupBox(tr("Dimensions"));
	QHBoxLayout *topLayout = new QHBoxLayout(groupBox1);
	topLayout->addWidget( new QLabel(tr( "Rows" )) );
	boxRows = new QSpinBox();
	boxRows->setRange(1, INT_MAX);
	topLayout->addWidget(boxRows);
	topLayout->addStretch();
	topLayout->addWidget( new QLabel(tr( "Columns" )) );
	boxCols = new QSpinBox();
	boxCols->setRange(1, INT_MAX);
	topLayout->addWidget(boxCols);

	groupBox2 = new QGroupBox(tr("Coordinates"));
	QGridLayout *centerLayout = new QGridLayout(groupBox2);
	centerLayout->addWidget( new QLabel(tr( "X (Columns)" )), 0, 1 );
	centerLayout->addWidget( new QLabel(tr( "Y (Rows)" )), 0, 2 );

	centerLayout->addWidget( new QLabel(tr( "First" )), 1, 0 );

	QLocale locale = m->locale();
	boxXStart = new DoubleSpinBox();
	boxXStart->setLocale(locale);
	centerLayout->addWidget( boxXStart, 1, 1 );

	boxYStart = new DoubleSpinBox();
	boxYStart->setLocale(locale);
	centerLayout->addWidget( boxYStart, 1, 2 );

	centerLayout->addWidget( new QLabel(tr( "Last" )), 2, 0 );
	boxXEnd = new DoubleSpinBox();
	boxXEnd->setLocale(locale);
	centerLayout->addWidget( boxXEnd, 2, 1 );

	boxYEnd = new DoubleSpinBox();
	boxYEnd->setLocale(locale);
	centerLayout->addWidget( boxYEnd, 2, 2 );
	centerLayout->setRowStretch(3, 1);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	bottomLayout->addStretch();
	buttonApply = new QPushButton(tr("&Apply"));
	buttonApply->setDefault( true );
	bottomLayout->addWidget(buttonApply);
	buttonOk = new QPushButton(tr("&OK"));
	bottomLayout->addWidget( buttonOk );
	buttonCancel = new QPushButton(tr("&Cancel"));
	bottomLayout->addWidget( buttonCancel );

	QVBoxLayout * mainLayout = new QVBoxLayout( this );
	mainLayout->addWidget(groupBox1);
	mainLayout->addWidget(groupBox2);
	mainLayout->addLayout(bottomLayout);

	boxRows->setValue(m->numRows());
	boxCols->setValue(m->numCols());

    //+++ test correction 2019-11-20
    double correctX=(m->xEnd()-m->xStart())/(m->numCols()-1)*0.5;
    double correctY=(m->yEnd()-m->yStart())/(m->numRows()-1)*0.5;
    
    
	boxXStart->setValue(m->xStart()-correctX);
	boxYStart->setValue(m->yStart()+correctY);
	boxXEnd->setValue(m->xEnd()-correctX);
	boxYEnd->setValue(m->yEnd()+correctY);

	connect( buttonApply, SIGNAL(clicked()), this, SLOT(apply()));
	connect( buttonOk, SIGNAL(clicked()), this, SLOT(accept() ));
	connect( buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void MatrixSizeDialog::apply()
{
	double fromX = boxXStart->value();
	double toX = boxXEnd->value();
	double fromY = boxYStart->value();
	double toY = boxYEnd->value();
	double oxs = d_matrix->xStart();
	double oxe = d_matrix->xEnd();
	double oys = d_matrix->yStart();
	double oye = d_matrix->yEnd();
	if(oxs != fromX || oxe != toX || oys != fromY || oye != toY){
		d_matrix->undoStack()->push(new MatrixSetCoordinatesCommand(d_matrix,
						oxs, oxe, oys, oye, fromX, toX, fromY, toY,
						tr("Set Coordinates x[%1 : %2], y[%3 : %4]").arg(fromX).arg(toX).arg(fromY).arg(toY)));
		d_matrix->setCoordinates(fromX, toX, fromY, toY);
	}
	d_matrix->setDimensions(boxRows->value(), boxCols->value());
}

void MatrixSizeDialog::accept()
{
	apply();
	close();
}
