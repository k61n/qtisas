/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Subtract baseline dialog
 ******************************************************************************/

#include <QCloseEvent>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

#include <gsl/gsl_interp.h>
#include <gsl/gsl_sort.h>
#include <gsl/gsl_spline.h>
#include <qwt/qwt_symbol.h>

#include "ApplicationWindow.h"
#include "BaselineDialog.h"
#include "DataPickerTool.h"
#include "FunctionCurve.h"
#include "Graph.h"
#include "Interpolation.h"
#include "PlotCurve.h"
#include "RangeSelectorTool.h"

BaselineDialog::BaselineDialog( QWidget* parent, Qt::WindowFlags fl )
	: QDialog( parent, fl ),
	d_baseline(nullptr),
	d_table(nullptr),
	d_picker_tool(nullptr)
{
	setObjectName( "BaselineDialog" );
	setWindowTitle(tr("QtiSAS") + " - " + tr("Baseline"));
	setAttribute(Qt::WA_DeleteOnClose);
	setSizeGripEnabled( true );

    auto gb1 = new QGroupBox();
    auto gl1 = new QGridLayout(gb1);

	gl1->addWidget(new QLabel(tr("Curve")), 0, 0);

	boxInputName = new QComboBox();
	gl1->addWidget(boxInputName, 0, 1);

	boxPoints = new QSpinBox();
	boxPoints->setMinimum(2);
	boxPoints->setMaximum(INT_MAX);

	gl1->addWidget(new QLabel(tr("Points")), 1, 0);
	gl1->addWidget(boxPoints, 1, 1);

	btnAutomatic = new QRadioButton(tr("&Interpolation"));
	btnAutomatic->setChecked(true);
	gl1->addWidget(btnAutomatic, 2, 0);

	boxInterpolationMethod = new QComboBox();
	boxInterpolationMethod->addItems(QStringList() << tr("Linear") << tr("Cubic") << tr("Non-rounded Akima"));
	gl1->addWidget(boxInterpolationMethod, 2, 1);

	btnEquation = new QRadioButton(tr("User Defined &Equation Y ="));
	btnEquation->setChecked(false);
	gl1->addWidget(btnEquation, 3, 0);

	boxEquation = new QLineEdit();
	gl1->addWidget(boxEquation, 3, 1);

	btnDataset = new QRadioButton(tr("Existing &Dataset"));
	btnDataset->setChecked(false);
	gl1->addWidget(btnDataset, 4, 0);

	boxTableName = new QComboBox();
	boxColumnName = new QComboBox();

    auto hb0 = new QHBoxLayout();
	hb0->addWidget(boxTableName);
	hb0->addWidget(boxColumnName);
	gl1->addLayout(hb0, 4, 1);

	gl1->setColumnStretch(1, 1);
	gl1->setRowStretch(5, 1);

    auto app = (ApplicationWindow *)parent;
	boxTableName->addItems(app->tableNames());
	updateTableColumns(0);

	buttonCreate = new QPushButton(tr( "Create &Baseline" ));
	buttonCreate->setDefault( true );
	buttonSubtract = new QPushButton(tr( "&Subtract" ));
	buttonUndo = new QPushButton(tr( "&Undo Subtraction" ));
	buttonModify = new QPushButton(tr( "&Modify" ));
	buttonModify->setCheckable(true);
	buttonCancel = new QPushButton(tr( "&Close" ));

    auto vl = new QVBoxLayout();
	vl->addWidget(buttonCreate);
	vl->addWidget(buttonModify);
	vl->addWidget(buttonSubtract);
	vl->addWidget(buttonUndo);
	vl->addStretch();
	vl->addWidget(buttonCancel);

    auto hb = new QHBoxLayout(this);
	hb->addWidget(gb1);
	hb->addLayout(vl);

	enableBaselineOptions();

	connect(boxTableName, SIGNAL(activated(int)), this, SLOT(updateTableColumns(int)));
	connect(buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
	connect(buttonCreate, SIGNAL(clicked()), this, SLOT(createBaseline()));
	connect(buttonSubtract, SIGNAL(clicked()), this, SLOT(subtractBaseline()));
	connect(buttonUndo, SIGNAL(clicked()), this, SLOT(undo()));
	connect(buttonModify, SIGNAL(clicked()), this, SLOT(modifyBaseline()));

	connect(btnAutomatic, SIGNAL(toggled(bool)), this, SLOT(enableBaselineOptions()));
	connect(btnEquation, SIGNAL(toggled(bool)), this, SLOT(enableBaselineOptions()));
	connect(btnDataset, SIGNAL(toggled(bool)), this, SLOT(enableBaselineOptions()));
}

void BaselineDialog::enableBaselineOptions()
{
	boxInterpolationMethod->setEnabled(false);
	boxEquation->setEnabled(false);
	boxColumnName->setEnabled(false);
	boxTableName->setEnabled(false);

	if (btnAutomatic->isChecked()){
		boxInterpolationMethod->setEnabled(true);
	} else if (btnEquation->isChecked()){
		boxEquation->setEnabled(true);
	} else if (btnDataset->isChecked()){
		boxTableName->setEnabled(true);
		boxColumnName->setEnabled(true);
	}
}

void BaselineDialog::disableBaselineTool()
{
	if (d_picker_tool){
		graph->setActiveTool(nullptr);
		d_picker_tool = nullptr;
		buttonModify->setChecked(false);
	}
}

void BaselineDialog::modifyBaseline()
{
	if (!d_baseline)
		createBaseline();

	disableBaselineTool();

	if (d_baseline->type() == Graph::Function){
        auto app = (ApplicationWindow *)parent();
		int points = d_baseline->dataSize();
		d_table = app->newTable(points, 2);
		app->setWindowName(d_table, tr("Baseline"));
		app->hideWindow(d_table);
		for (int i = 0; i < points; i++){
			d_table->setCell(i, 0, d_baseline->x(i));
			d_table->setCell(i, 1, d_baseline->y(i));
		}

		QPen pen = d_baseline->pen();
		graph->removeCurve(d_baseline->title().text());
		delete d_baseline;

		d_baseline = graph->insertCurve(d_table, d_table->objectName() + "_2", Graph::Line);
		d_baseline->setPen(pen);
	}

	d_baseline->setSymbol(QwtSymbol(QwtSymbol::Rect, QBrush(Qt::black), d_baseline->pen(), QSize(7, 7)));

	d_picker_tool = new BaselineTool(d_baseline, graph, (ApplicationWindow *)parent());
	graph->setActiveTool(d_picker_tool);
	graph->replot();
}

void BaselineDialog::createBaseline()
{
    auto app = (ApplicationWindow *)parent();
	QPen pen = QPen(Qt::red);

	if (d_baseline){
		pen = d_baseline->pen();
		graph->removeCurve(d_baseline->title().text());
		delete d_baseline;
		d_baseline = nullptr;

		if (d_table){
			d_table->askOnCloseEvent(false);
			app->closeWindow(d_table);
			d_table = nullptr;
		}
	}

	disableBaselineTool();

	if (btnAutomatic->isChecked()){
		QString name = boxInputName->currentText();
        auto i =
            new Interpolation(app, graph->curve(name.left(name.indexOf(" ["))), boxInterpolationMethod->currentIndex());
		i->setOutputPoints(boxPoints->value());
		i->run();
		delete i;
		d_baseline = graph->dataCurve(graph->curveCount() - 1);
		d_table = ((DataCurve *)d_baseline)->table();
	} else if (btnEquation->isChecked()){
		double start = graph->axisScaleDiv(QwtPlot::xBottom)->lowerBound();
		double end = graph->axisScaleDiv(QwtPlot::xBottom)->upperBound();
		QString name = boxInputName->currentText();
		DataCurve *c = graph->dataCurve(graph->curveIndex(name.left(name.indexOf(" ["))));
		if (c){
			start = c->minXValue();
			end = c->maxXValue();
		}
		d_baseline = graph->addFunction(QStringList() << boxEquation->text(), start, end, boxPoints->value(), "x", FunctionCurve::Normal, tr("Baseline"));
	} else if (btnDataset->isChecked()){
		Table *t = app->table(boxTableName->currentText());
		if (t){
			int rows = t->numRows();
			d_table = app->newTable(rows, 2);
			app->setWindowName(d_table, tr("Baseline"));
			app->hideWindow(d_table);
			int ycol = t->colIndex(boxColumnName->currentText());
			int xcol = t->colX(ycol);
			for (int i = 0; i < rows; i++){
				d_table->setText(i, 0, t->text(i, xcol));
				d_table->setText(i, 1, t->text(i, ycol));
			}
			d_baseline = graph->insertCurve(d_table, d_table->objectName() + "_2", Graph::Line, 0, -1);
			graph->setAutoScale();
		}
	}

	if (d_baseline){
		d_baseline->setPen(pen);
		graph->replot();
	}
}

void BaselineDialog::undo()
{
	subtractBaseline(true);
}

void BaselineDialog::subtractBaseline(bool add)
{
	if (!d_baseline)
		return;

	disableBaselineTool();

	if (!graph)
		return;

	QString name = boxInputName->currentText();
	DataCurve *c = graph->dataCurve(graph->curveIndex(name.left(name.indexOf(" ["))));
	if (!c)
		return;

    auto app = (ApplicationWindow *)parent();
	if (!app)
		return;

	Table *inputTable = c->table();
	if (!inputTable)
		return;

	int startRow = c->startRow(), endRow = c->endRow();
	if (startRow < 0)
		startRow = 0;
	if (endRow < 0)
		endRow = c->dataSize() - 1;

	int xCol = inputTable->colIndex(c->xColumnName());
	int yCol = inputTable->colIndex(c->title().text());

	int refPoints = d_baseline->dataSize();
    auto x = (double *)malloc(refPoints * sizeof(double));
	if (!x)
		return;

    auto y = (double *)malloc(refPoints * sizeof(double));
	if (!y){
		free (x);
		return;
	}

	for (int i = 0; i < refPoints; i++){
		x[i] = d_baseline->x(i);
		y[i] = d_baseline->y(i);
	}

	//sort data with respect to x value
    auto p = (size_t *)malloc(refPoints * sizeof(size_t));
	if (!p){
		free(x); free(y);
		return;
	}

	gsl_sort_index(p, x, 1, refPoints);

    auto xtemp = (double *)malloc(refPoints * sizeof(double));
	if (!xtemp){
		free(x); free(y); free(p);
		return;
	}

    auto ytemp = (double *)malloc(refPoints * sizeof(double));
	if (!ytemp){
		free(x); free(y); free(p); free(xtemp);
		return;
	}

	for (int i = 0; i < refPoints; i++){
		xtemp[i] = x[p[i]];
		ytemp[i] = y[p[i]];
	}
	free(x);
	free(y);
	free(p);

	//make linear interpolation on sorted data
	gsl_interp_accel *acc = gsl_interp_accel_alloc();
	gsl_spline *interp = gsl_spline_alloc(gsl_interp_linear, refPoints);
	gsl_spline_init (interp, xtemp, ytemp, refPoints);

	for (int i = startRow; i <= endRow; i++){
		if (!inputTable->text(i, yCol).isEmpty() && !inputTable->text(i, xCol).isEmpty())
			inputTable->setCell(i, yCol, combineValues(inputTable->cell(i, yCol), gsl_spline_eval(interp, inputTable->cell(i, xCol), acc), add));
	}
	inputTable->notifyChanges(c->title().text());

	gsl_spline_free (interp);
	gsl_interp_accel_free (acc);
	free(xtemp);
	free(ytemp);
}

double BaselineDialog::combineValues(double v1, double v2, bool add)
{
	double res = v1;
	if (add)
		res += v2;
	else
		res -= v2;
	return res;
}

void BaselineDialog::updateTableColumns(int tabnr)
{
	boxColumnName->clear();

    auto app = (ApplicationWindow *)parent();
	if (!app)
		return;

	Table *t = app->table(boxTableName->itemText(tabnr));
	if (!t)
		return;

	QStringList columns;
	for (int i = 0; i < t->numCols(); i++)
		if (t->colPlotDesignation(i) != Table::X)
			columns << t->colLabel(i);

	boxColumnName->addItems(columns);
}

void BaselineDialog::setGraph(Graph *g)
{
	if (!g)
		return;

	graph = g;
	DataCurve *c = g->dataCurve(0);
	if (c)
		boxPoints->setValue(c->dataSize());

	boxInputName->addItems(g->analysableCurvesList());
	if (g->rangeSelectorsEnabled())
		boxInputName->setCurrentIndex(boxInputName->findText(g->curveRange(g->rangeSelectorTool()->selectedCurve())));

	connect (graph, SIGNAL(destroyed()), this, SLOT(close()));
	connect (graph, SIGNAL(modifiedGraph()), this, SLOT(updateGraphCurves()));
}

void BaselineDialog::updateGraphCurves()
{
	QStringList lst = graph->analysableCurvesList();
	if (d_baseline)
		lst.removeAll(d_baseline->title().text());

	if (lst.isEmpty()){
		close();
		return;
	}

	boxInputName->clear();
	boxInputName->addItems(lst);
}

void BaselineDialog::closeEvent(QCloseEvent* e)
{
	if (d_picker_tool)
		graph->disableTools();

	e->accept();
}
