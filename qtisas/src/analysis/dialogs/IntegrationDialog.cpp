/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2011 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Integration options dialog
 ******************************************************************************/

#include <QComboBox>
#include <QGroupBox>
#include <QCloseEvent>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

#include "DoubleSpinBox.h"
#include "Graph.h"
#include "Integration.h"
#include "IntegrationDialog.h"


IntegrationDialog::IntegrationDialog(Graph *g, QWidget* parent, Qt::WindowFlags fl )
	: QDialog( parent, fl ),
	d_table(0)
{
	setObjectName( "IntegrationDialog" );
	setWindowTitle(tr("QtiSAS - Integration Options"));
	setAttribute(Qt::WA_DeleteOnClose);
	setSizeGripEnabled( true );

    QGroupBox *gb1 = new QGroupBox();
	QGridLayout *gl1 = new QGridLayout(gb1);
	gl1->addWidget(new QLabel(tr("Curve")), 0, 0);

	boxName = new QComboBox();
	gl1->addWidget(boxName, 0, 1);

	ApplicationWindow *app = (ApplicationWindow *)parent;

	gl1->addWidget(new QLabel(tr("From Xmin")), 1, 0);
	boxStart = new DoubleSpinBox();
	boxStart->setDecimals(app->d_decimal_digits);
	boxStart->setLocale(app->locale());
	gl1->addWidget(boxStart, 1, 1);

	gl1->addWidget(new QLabel(tr("To Xmax")), 2, 0);
	boxEnd = new DoubleSpinBox();
	boxEnd->setDecimals(app->d_decimal_digits);
	boxEnd->setLocale(app->locale());
	gl1->addWidget(boxEnd, 2, 1);

	boxSortData = new QCheckBox(tr("&Sort data"));
	boxSortData->setChecked(app->d_int_sort_data);
	gl1->addWidget(boxSortData, 3, 1);
	boxShowPlot = new QCheckBox(tr("Show &plot"));
	boxShowPlot->setChecked(app->d_int_show_plot);
	gl1->addWidget(boxShowPlot, 4, 1);

	gl1->setRowStretch(5, 1);
	gl1->setColumnStretch(1, 1);

	buttonFit = new QPushButton(tr( "&Integrate" ));
    buttonFit->setDefault( true );
    buttonCancel = new QPushButton(tr( "&Close" ));

	QVBoxLayout *vl = new QVBoxLayout();
 	vl->addWidget(buttonFit);
	vl->addWidget(buttonCancel);
    vl->addStretch();

    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->addWidget(gb1, 1);
    hb->addLayout(vl);

	setGraph(g);

    connect(boxName, &QComboBox::textActivated, this, &IntegrationDialog::activateCurve);

	connect(buttonFit, SIGNAL(clicked()), this, SLOT(integrate()));
	connect(buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
}

IntegrationDialog::IntegrationDialog(Table *t, QWidget* parent, Qt::WindowFlags fl )
	: QDialog( parent, fl ),
	d_graph(0)
{
	setObjectName( "IntegrationDialog" );
	setWindowTitle(tr("QtiSAS - Integration Options"));
	setAttribute(Qt::WA_DeleteOnClose);
	setSizeGripEnabled( true );

	QGroupBox *gb1 = new QGroupBox();
	QGridLayout *gl1 = new QGridLayout(gb1);

	ApplicationWindow *app = (ApplicationWindow *)parent;

	gl1->addWidget(new QLabel(tr("From row")), 0, 0);
	boxStartRow = new QSpinBox();
	gl1->addWidget(boxStartRow, 0, 1);

	gl1->addWidget(new QLabel(tr("To row")), 1, 0);
	boxEndRow = new QSpinBox();
	gl1->addWidget(boxEndRow, 1, 1);

	boxSortData = new QCheckBox(tr("&Sort data"));
	boxSortData->setChecked(app->d_int_sort_data);
	gl1->addWidget(boxSortData, 2, 1);
	boxShowPlot = new QCheckBox(tr("Show &plot"));
	boxShowPlot->setChecked(app->d_int_show_plot);
	gl1->addWidget(boxShowPlot, 3, 1);
	boxShowTable = new QCheckBox(tr("Show results &table"));
	boxShowTable->setChecked(app->d_int_results_table);
	gl1->addWidget(boxShowTable, 4, 1);

	gl1->setRowStretch(5, 1);
	gl1->setColumnStretch(1, 1);

	buttonFit = new QPushButton(tr( "&Integrate" ));
	buttonFit->setDefault( true );
	buttonCancel = new QPushButton(tr( "&Close" ));

	QVBoxLayout *vl = new QVBoxLayout();
	vl->addWidget(buttonFit);
	vl->addWidget(buttonCancel);
	vl->addStretch();

	QHBoxLayout *hb = new QHBoxLayout(this);
	hb->addWidget(gb1, 1);
	hb->addLayout(vl);

	setTable(t);

	connect(buttonFit, SIGNAL(clicked()), this, SLOT(integrate()));
	connect(buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
}

void IntegrationDialog::integrate()
{
	if (d_graph)
		integrateCurve();
	else if (d_table)
		integrateTable();
}

void IntegrationDialog::integrateCurve()
{
	QString curveName = boxName->currentText();
	QStringList curvesList = d_graph->analysableCurvesList();
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

	Integration *i = new Integration((ApplicationWindow *)parent());
	i->setSortData(boxSortData->isChecked());
	if (i->setDataFromCurve((QwtPlotCurve *)d_graph->curve(curveName), from, to)){
		i->enableGraphicsDisplay(boxShowPlot->isChecked());
		i->run();
	}
	delete i;
}

void IntegrationDialog::setGraph(Graph *g)
{
	if (!g)
		return;

	d_graph = g;
	boxName->addItems(g->analysableCurvesList());

	if (g->rangeSelectorsEnabled())
		boxName->setCurrentIndex(boxName->findText(g->curveRange(g->rangeSelectorTool()->selectedCurve())));

	activateCurve(boxName->currentText());

	connect (d_graph, SIGNAL(destroyed()), this, SLOT(close()));
	connect (d_graph, SIGNAL(dataRangeChanged()), this, SLOT(changeDataRange()));
}

void IntegrationDialog::activateCurve(const QString& s)
{
	if (!d_graph)
		return;

	QwtPlotCurve *c = (QwtPlotCurve *)d_graph->curve(s.left(s.indexOf(" [")));
	if (!c)
		return;

    ApplicationWindow *app = (ApplicationWindow *)parent();
    if(!app)
        return;

	double start, end;
	d_graph->range(c, &start, &end);
	boxStart->setValue(qMin(start, end));
	boxEnd->setValue(qMax(start, end));
}

void IntegrationDialog::changeDataRange()
{
	if (!d_graph)
		return;

	ApplicationWindow *app = (ApplicationWindow *)parent();
	if(!app)
		return;

	double start = d_graph->selectedXStartValue();
	double end = d_graph->selectedXEndValue();
	boxStart->setValue(qMin(start, end));
	boxEnd->setValue(qMax(start, end));
}

void IntegrationDialog::setTable(Table *t)
{
	if (!t)
		return;

	d_table = t;

	if (t->selectedYColumns().size() < 2)
		boxShowTable->hide();

	MySelection sel = t->getSelection();
	if (!sel.isEmpty()) {
		boxStartRow->setValue(sel.topRow() + 1);
		boxEndRow->setValue(sel.bottomRow() + 1);
	} else {
		boxStartRow->setValue(1);
		boxEndRow->setValue(t->numRows());
	}

	connect (d_table, SIGNAL(destroyed()), this, SLOT(close()));
}

void IntegrationDialog::integrateTable()
{
	if (!d_table)
		return;

	ApplicationWindow *app = (ApplicationWindow *)parent();
	if(!app)
		return;

	QStringList lst = d_table->selectedYColumns();
	int cols = lst.size();
	if (!cols)
		return;

	int from = boxStartRow->value();
	int to = boxEndRow->value();
	bool sortData = boxSortData->isChecked();

	Table *result = 0;
	if (boxShowTable->isVisible() && boxShowTable->isChecked()){
		result = app->newTable(cols, 2, "", tr("Integration of %1").arg(d_table->objectName()));
		result->setColName(0, tr("Column"));
		result->setColName(1, tr("Area"));
	}

	int aux = 0;
	foreach (QString yCol, lst){
		int xCol = d_table->colX(d_table->colIndex(yCol));
		Integration *i = new Integration(app, d_table, d_table->colName(xCol), yCol, from, to, sortData);
		i->enableGraphicsDisplay(boxShowPlot->isChecked());
		i->run();
		if (result){
			result->setText(aux, 0, yCol);
			result->setCell(aux, 1, i->area());
			aux++;
		}
		delete i;
	}

	if (result)
		result->show();
}

void IntegrationDialog::closeEvent (QCloseEvent * e)
{
	ApplicationWindow *app = (ApplicationWindow *)parent();
	if(app){
		app->d_int_sort_data = boxSortData->isChecked();
		app->d_int_show_plot = boxShowPlot->isChecked();
		if (d_table)
			app->d_int_results_table = boxShowTable->isChecked();
	}

	e->accept();
}
