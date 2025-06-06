/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Fit polynomial dialog
 ******************************************************************************/

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

#include "ApplicationWindow.h"
#include "ColorButton.h"
#include "DoubleSpinBox.h"
#include "Graph.h"
#include "PolynomFitDialog.h"
#include "PolynomialFit.h"

PolynomFitDialog::PolynomFitDialog( QWidget* parent, Qt::WindowFlags fl )
: QDialog( parent, fl )
{
    setObjectName( "PolynomFitDialog" );
	setWindowTitle(tr("QtiSAS - Polynomial Fit Options"));
	setAttribute(Qt::WA_DeleteOnClose);
    setSizeGripEnabled( true );

    QGroupBox *gb1 = new QGroupBox();
	QGridLayout *gl1 = new QGridLayout(gb1);
	gl1->addWidget(new QLabel(tr("Polynomial Fit of")), 0, 0);

	boxName = new QComboBox();
    gl1->addWidget(boxName, 0, 1);

    gl1->addWidget(new QLabel( tr("Order (1 - 9, 1 = linear)")), 1, 0);
	boxOrder = new QSpinBox();
    boxOrder->setRange(1, 9);
	boxOrder->setValue(2);
    gl1->addWidget(boxOrder, 1, 1);

    gl1->addWidget(new QLabel( tr("Fit curve # pts")), 2, 0);
	boxPoints = new QSpinBox();
    boxPoints->setRange(1, 1000);
    boxPoints->setSingleStep(50);
    boxPoints->setSpecialValueText(tr("Not enough points"));
    gl1->addWidget(boxPoints, 2, 1);

	ApplicationWindow *app = (ApplicationWindow *)parent;

    gl1->addWidget(new QLabel( tr("Fit curve Xmin")), 3, 0);
	boxStart = new DoubleSpinBox();
	boxStart->setDecimals(app->d_decimal_digits);
	boxStart->setLocale(app->locale());
    gl1->addWidget(boxStart, 3, 1);

    gl1->addWidget(	new QLabel( tr("Fit curve Xmax")), 4, 0);
	boxEnd = new DoubleSpinBox();
	boxEnd->setDecimals(app->d_decimal_digits);
	boxEnd->setLocale(app->locale());
    gl1->addWidget(boxEnd, 4, 1);

    gl1->addWidget(new QLabel( tr("Color")), 5, 0);
	boxColor = new ColorButton();
	boxColor->setColor(Qt::red);
    gl1->addWidget(boxColor, 5, 1);

	boxShowFormula = new QCheckBox(tr( "Show Formula on Graph?" ));
	boxShowFormula->setChecked( false );
    gl1->addWidget(boxShowFormula, 6, 1);
    gl1->setRowStretch(7, 1);
	gl1->setColumnStretch(1, 1);

	buttonFit = new QPushButton(tr( "&Fit" ));
	buttonFit->setDefault( true );

	buttonCancel = new QPushButton(tr( "&Close" ));

    QVBoxLayout* vl = new QVBoxLayout();
    vl->addWidget(buttonFit);
    vl->addWidget(buttonCancel);
    vl->addStretch();

	QHBoxLayout* hlayout = new QHBoxLayout(this);
	hlayout->addWidget(gb1, 1);
	hlayout->addLayout(vl);

	connect( buttonFit, SIGNAL( clicked() ), this, SLOT( fit() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( close() ) );
    connect(boxName, &QComboBox::textActivated, this, &PolynomFitDialog::activateCurve);
}

void PolynomFitDialog::fit()
{
	QString curveName = boxName->currentText();
	QStringList curvesList = graph->analysableCurvesList();
	if (!curvesList.contains(curveName)){
		QMessageBox::critical(this, tr("QtiSAS - Warning"),
				tr("The curve <b> %1 </b> doesn't exist anymore! Operation aborted!").arg(curveName));
		boxName->clear();
		boxName->insertItems(0, curvesList);
		return;
	}

	curveName = curveName.left(curveName.indexOf(" ["));

	ApplicationWindow *app = (ApplicationWindow *)this->parent();
    PolynomialFit *fitter = new PolynomialFit(app, graph, boxOrder->value(), boxShowFormula->isChecked());
	if (fitter->setDataFromCurve((QwtPlotCurve *)graph->curve(curveName), boxStart->value(), boxEnd->value())){
		fitter->setColor(boxColor->color());
        fitter->setOutputPrecision(app->fit_output_precision);
		fitter->generateFunction(app->generateUniformFitPoints, app->fitPoints);
        fitter->fit();
        delete fitter;
	}
}

void PolynomFitDialog::setGraph(Graph *g)
{
	if (!g)
		return;

	graph = g;
	boxName->addItems (g->analysableCurvesList());

	if (g->rangeSelectorsEnabled())
		boxName->setCurrentIndex(boxName->findText(g->curveRange(g->rangeSelectorTool()->selectedCurve())));

	activateCurve(boxName->currentText());

	connect (graph, SIGNAL(destroyed()), this, SLOT(close()));
	connect (graph, SIGNAL(dataRangeChanged()), this, SLOT(changeDataRange()));
}

void PolynomFitDialog::activateCurve(const QString& s)
{
	double start, end;
	int n_points = graph->range((QwtPlotCurve *)graph->curve(s.left(s.indexOf(" ["))), &start, &end);

	boxStart->setValue(start);
	boxEnd->setValue(end);
	boxPoints->setValue(qMax(n_points, 100));
}

void PolynomFitDialog::changeDataRange()
{
	double start = graph->selectedXStartValue();
	double end = graph->selectedXEndValue();
	boxStart->setValue(qMin(start, end));
	boxEnd->setValue(qMax(start, end));
}
