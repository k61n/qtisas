/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Matrix properties dialog
 ******************************************************************************/

#include "MatrixDialog.h"
#include "Matrix.h"
#include "MatrixCommand.h"

#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLayout>
#include <QGroupBox>
#include <QSpinBox>

MatrixDialog::MatrixDialog( QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl ),
    d_matrix(0)
{
    setWindowTitle( tr( "QtiSAS - Matrix Properties" ) );
    setAttribute(Qt::WA_DeleteOnClose);
	setSizeGripEnabled(true);

	QGridLayout * topLayout = new QGridLayout();
	QHBoxLayout * bottomLayout = new QHBoxLayout();

	topLayout->addWidget( new QLabel(tr( "Cell Width" )), 0, 0 );
	boxColWidth = new QSpinBox();
	boxColWidth->setRange(0,1000);
	boxColWidth->setSingleStep(10);
	topLayout->addWidget( boxColWidth, 0, 1 );

	topLayout->addWidget( new QLabel(tr( "Data Format" )), 1, 0 );
	boxFormat = new QComboBox();
    boxFormat->addItem( tr( "Decimal: 1000" ) );
	boxFormat->addItem( tr( "Scientific: 1E3" ) );

	topLayout->addWidget( boxFormat, 1, 1 );

	topLayout->addWidget( new QLabel( tr( "Numeric Display" )), 2, 0 );
	boxNumericDisplay = new QComboBox();
    boxNumericDisplay->addItem( tr( "Default Decimal Digits" ) );
	boxNumericDisplay->addItem( tr( "Significant Digits=" ) );

	topLayout->addWidget( boxNumericDisplay, 2, 1 );
	boxPrecision = new QSpinBox();
	boxPrecision->setRange(0, 13);
	boxPrecision->setEnabled( false );
	topLayout->addWidget( boxPrecision, 2, 2 );
	topLayout->setColumnStretch(3, 1);

	bottomLayout->addStretch();
	buttonApply = new QPushButton(tr( "&Apply" ));
	buttonApply->setAutoDefault( true );
	bottomLayout->addWidget( buttonApply );

	buttonOk = new QPushButton(tr( "&OK" ));
	buttonOk->setAutoDefault( true );
	buttonOk->setDefault( true );
	bottomLayout->addWidget( buttonOk );

	buttonCancel = new QPushButton(tr( "&Cancel" ));
	buttonCancel->setAutoDefault( true );
	bottomLayout->addWidget( buttonCancel );
	bottomLayout->addStretch();

	QVBoxLayout * mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(topLayout);
	mainLayout->addStretch();
	mainLayout->addLayout(bottomLayout);

	// signals and slots connections
	connect(buttonApply, SIGNAL(clicked()), this, SLOT(apply()));
	connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
	connect(buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
	connect(boxNumericDisplay, SIGNAL(activated(int)), this, SLOT(showPrecisionBox(int)));
}

void MatrixDialog::showPrecisionBox(int item)
{
	if (item)
		boxPrecision->setEnabled(true);
	else {
		boxPrecision->setValue(6);
		boxPrecision->setEnabled(false);
	}
}

void MatrixDialog::apply()
{
    int width = boxColWidth->value();
    if (d_matrix->columnsWidth() != width){
        d_matrix->undoStack()->push(new MatrixSetColWidthCommand(d_matrix, d_matrix->columnsWidth(),
                        width, tr("Set Columns Width") + " " + QString::number(width)));
        d_matrix->setColumnsWidth(width);
    }

    int prec = boxPrecision->value();
    QChar format = 'f';
    QString fmtName = tr("Decimal");
    if (boxFormat->currentIndex()){
        format = 'e';
        fmtName = tr("Scientific");
    }

    QChar oldFormat = d_matrix->textFormat();
    int oldPrec = d_matrix->precision();
    if (oldFormat != format || oldPrec != prec){
        d_matrix->undoStack()->push(new MatrixSetPrecisionCommand(d_matrix, oldFormat, format,
                        oldPrec, prec, tr("Set Data Format %1").arg(fmtName) + " - " +
                        tr("Precision %1 digits").arg(prec)));
        d_matrix->setNumericFormat(format, prec);
    }
}

void MatrixDialog::setMatrix(Matrix *m)
{
    if (!m)
        return;

    d_matrix = m;
    boxColWidth->setValue(m->columnsWidth());

    if (QString(m->textFormat()) == "f")
		boxFormat->setCurrentIndex(0);
	else
		boxFormat->setCurrentIndex(1);

	boxPrecision->setValue(m->precision());
	if (m->precision() != 6){
		boxPrecision->setEnabled( true );
		boxNumericDisplay->setCurrentIndex(1);
	}
}

void MatrixDialog::accept()
{
	apply();
	close();
}
