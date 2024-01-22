/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Interpolation options dialog
 ******************************************************************************/

#include "InterpolationDialog.h"
#include <Interpolation.h>
#include <Graph.h>
#include <ColorButton.h>
#include <DoubleSpinBox.h>
#include <RangeSelectorTool.h>

#include <QGroupBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLayout>

InterpolationDialog::InterpolationDialog( QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
    setObjectName( "InterpolationDialog" );
	setWindowTitle(tr("QtiSAS - Interpolation Options"));
	setAttribute(Qt::WA_DeleteOnClose);
	setSizeGripEnabled( true );

    QGroupBox *gb1 = new QGroupBox();
	QGridLayout *gl1 = new QGridLayout(gb1);
	gl1->addWidget(new QLabel(tr("Make curve from")), 0, 0);

	boxName = new QComboBox();
	gl1->addWidget(boxName, 0, 1);

	gl1->addWidget(new QLabel(tr("Spline")), 1, 0);
	boxMethod = new QComboBox();
	boxMethod->addItem(tr("Linear"));
    boxMethod->addItem(tr("Cubic"));
    boxMethod->addItem(tr("Non-rounded Akima"));
	gl1->addWidget(boxMethod, 1, 1);

	gl1->addWidget(new QLabel(tr("Points")), 2, 0);
	boxPoints = new QSpinBox();
	boxPoints->setRange(3, 100000);
	boxPoints->setSingleStep(10);
	boxPoints->setValue(1000);
	gl1->addWidget(boxPoints, 2, 1);

	ApplicationWindow *app = (ApplicationWindow *)parent;

	gl1->addWidget(new QLabel(tr("From Xmin")), 3, 0);
	boxStart = new DoubleSpinBox();
	boxStart->setDecimals(app->d_decimal_digits);
	boxStart->setLocale(app->locale());
	gl1->addWidget(boxStart, 3, 1);

	gl1->addWidget(new QLabel(tr("To Xmax")), 4, 0);
	boxEnd = new DoubleSpinBox();
	boxEnd->setDecimals(app->d_decimal_digits);
	boxEnd->setLocale(app->locale());
	gl1->addWidget(boxEnd, 4, 1);

	gl1->addWidget(new QLabel(tr("Color")), 5, 0);

	boxColor = new ColorButton();
	boxColor->setColor(Qt::red);
	gl1->addWidget(boxColor, 5, 1);
	gl1->setRowStretch(6, 1);
	gl1->setColumnStretch(1, 1);

	buttonFit = new QPushButton(tr( "&Make" ));
    buttonFit->setDefault( true );
    buttonCancel = new QPushButton(tr( "&Close" ));

	QVBoxLayout *vl = new QVBoxLayout();
 	vl->addWidget(buttonFit);
	vl->addWidget(buttonCancel);
    vl->addStretch();

    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->addWidget(gb1, 1);
    hb->addLayout(vl);

	connect(boxName, SIGNAL(activated(const QString&)), this, SLOT(activateCurve(const QString&)));
	connect(buttonFit, SIGNAL(clicked()), this, SLOT(interpolate()));
	connect(buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
}

void InterpolationDialog::interpolate()
{
	QString curveName = boxName->currentText();
	QStringList curvesList = graph->analysableCurvesList();
	if (!curvesList.contains(curveName)){
		QMessageBox::critical(this,tr("QtiSAS - Warning"),
		tr("The curve <b> %1 </b> doesn't exist anymore! Operation aborted!").arg(curveName));
		boxName->clear();
		boxName->addItems(curvesList);
		return;
	}
	curveName = curveName.left(curveName.indexOf(" ["));

	double from = boxStart->value();
	double to = boxEnd->value();
	if (from >= to){
		QMessageBox::critical(this, tr("QtiSAS - Input error"), tr("Please enter x limits that satisfy: from < to!"));
		boxEnd->setFocus();
		return;
	}

	Interpolation *i = new Interpolation((ApplicationWindow *)parent(), (QwtPlotCurve *)graph->curve(curveName), from, to, boxMethod->currentIndex());
	i->setOutputPoints(boxPoints->value());
	i->setColor(boxColor->color());
	i->run();
	delete i;
}

void InterpolationDialog::setGraph(Graph *g)
{
	if (!g)
		return;

	graph = g;
	boxName->addItems(g->analysableCurvesList());

	if (g->rangeSelectorsEnabled())
		boxName->setCurrentIndex(boxName->findText(g->curveRange(g->rangeSelectorTool()->selectedCurve())));

	activateCurve(boxName->currentText());

	connect (graph, SIGNAL(destroyed()), this, SLOT(close()));
	connect (graph, SIGNAL(dataRangeChanged()), this, SLOT(changeDataRange()));
};

void InterpolationDialog::activateCurve(const QString& s)
{
	QwtPlotCurve *c = (QwtPlotCurve *)graph->curve(s.left(s.indexOf(" [")));
	if (!c)
		return;

    ApplicationWindow *app = (ApplicationWindow *)parent();
    if(!app)
        return;

	double start, end;
	graph->range(c, &start, &end);
	boxStart->setValue(qMin(start, end));
	boxEnd->setValue(qMax(start, end));
};

void InterpolationDialog::changeDataRange()
{
	ApplicationWindow *app = (ApplicationWindow *)parent();
	if(!app)
		return;

	double start = graph->selectedXStartValue();
	double end = graph->selectedXEndValue();
	boxStart->setValue(qMin(start, end));
	boxEnd->setValue(qMax(start, end));
}
