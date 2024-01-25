/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Filter options dialog
 ******************************************************************************/

#include "FilterDialog.h"
#include "FFTFilter.h"
#include <Graph.h>
#include <ColorButton.h>
#include <DoubleSpinBox.h>

#include <QGroupBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

FilterDialog::FilterDialog(int type, QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl ), filter_type(type)
{
	setObjectName( "FilterDialog" );
	setWindowTitle(tr("QtiSAS - Filter options"));
	setSizeGripEnabled( true );
	setAttribute(Qt::WA_DeleteOnClose);

    QGroupBox *gb1 = new QGroupBox();
    QGridLayout *gl1 = new QGridLayout(gb1);
	gl1->addWidget(new QLabel(tr("Filter curve: ")), 0, 0);

	boxName = new QComboBox();
	gl1->addWidget(boxName, 0, 1);

	if (type <= FFTFilter::HighPass)
		gl1->addWidget(new QLabel(tr("Frequency cutoff (Hz)")), 1, 0);
	else
		gl1->addWidget(new QLabel(tr("Low Frequency (Hz)")), 1, 0);

	ApplicationWindow *app = (ApplicationWindow *)parent;

	boxStart = new DoubleSpinBox();
	boxStart->setValue(0.0);
	boxStart->setDecimals(app->d_decimal_digits);
	boxStart->setLocale(app->locale());
	boxStart->setMinimum(0.0);
	gl1->addWidget(boxStart, 1, 1);

	boxColor = new ColorButton();
	boxColor->setColor(Qt::red);
	if (type >= FFTFilter::BandPass){
	    gl1->addWidget(new QLabel(tr("High Frequency (Hz)")), 2, 0);

		boxEnd = new DoubleSpinBox();
		boxEnd->setValue(0.0);
		boxEnd->setDecimals(app->d_decimal_digits);
		boxEnd->setLocale(app->locale());
		boxEnd->setMinimum(0.0);
        gl1->addWidget(boxEnd, 2, 1);

		if (type == FFTFilter::BandPass)
		    gl1->addWidget(new QLabel(tr("Add DC Offset")), 3, 0);
		else
		    gl1->addWidget(new QLabel(tr("Substract DC Offset")), 3, 0);

		boxOffset = new QCheckBox();
		gl1->addWidget(boxOffset, 3, 1);

		gl1->addWidget(new QLabel(tr("Color")), 4, 0);
		gl1->addWidget(boxColor, 4, 1);
        gl1->setRowStretch(5, 1);
	} else {
        gl1->addWidget(new QLabel(tr("Color")), 2, 0);
		gl1->addWidget(boxColor, 2, 1);
        gl1->setRowStretch(3, 1);
	}
	gl1->setColumnStretch(1, 1);

	buttonFilter = new QPushButton(tr( "&Filter" ));
    buttonFilter->setDefault( true );
    buttonCancel = new QPushButton(tr( "&Close" ));

    QVBoxLayout *vl = new QVBoxLayout();
 	vl->addWidget(buttonFilter);
	vl->addWidget(buttonCancel);
    vl->addStretch();

    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->addWidget(gb1, 1);
    hb->addLayout(vl);

	connect( buttonFilter, SIGNAL( clicked() ), this, SLOT( filter() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( close() ) );
}

void FilterDialog::filter()
{
	double from = boxStart->value();
	double to = 0.0;
	if (filter_type >= FFTFilter::BandPass){
		to = boxEnd->value();
		if (from >= to){
			QMessageBox::critical(this, tr("QtiSAS - Frequency input error"),
				tr("Please enter frequency limits that satisfy: Low < High !"));
			boxEnd->setFocus();
			return;
		}
	}

	QString name = boxName->currentText();
	name = name.left(name.indexOf(" ["));
	FFTFilter *f = new FFTFilter((ApplicationWindow *)parent(), (QwtPlotCurve *)graph->curve(name), filter_type);
	if (filter_type == FFTFilter::BandPass){
    	f->setBand(from, to);
    	f->enableOffset(boxOffset->isChecked());
    } else if (filter_type == FFTFilter::BandBlock){
    	f->setBand(from, to);
    	f->enableOffset(!boxOffset->isChecked());
    } else
    	f->setCutoff(from);

	f->setColor(boxColor->color());
	f->run();
	delete f;
}

void FilterDialog::setGraph(Graph *g)
{
	if (!g)
		return;
	graph = g;
	boxName->addItems (g->analysableCurvesList());
}
