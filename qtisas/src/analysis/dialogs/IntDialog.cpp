/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Integration options dialog
 ******************************************************************************/

#include <QComboBox>
#include <QGroupBox>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

#include <gsl/gsl_math.h>

#include "ApplicationWindow.h"
#include "DoubleSpinBox.h"
#include "Graph.h"
#include "IntDialog.h"
#include "Integration.h"
#include "MyParser.h"
#include "ScriptEdit.h"

IntDialog::IntDialog(QWidget* parent, Graph *g, Qt::WindowFlags fl )
    : QDialog( parent, fl),
	d_graph(g)
{
	setObjectName("IntegrationDialog");
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(tr("QtiSAS - Integration Options"));
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

	QGroupBox *gb1 = new QGroupBox(tr("Function"));
	QGridLayout *gl1 = new QGridLayout(gb1);

	boxMathFunctions = new QComboBox();
	boxMathFunctions->addItems(MyParser::functionsList());

	QVBoxLayout *vl1 = new QVBoxLayout();
	vl1->addWidget(boxMathFunctions);

	addFunctionBtn = new QPushButton(tr("&Add" ));
	addFunctionBtn->setAutoDefault(false);
	connect(addFunctionBtn, SIGNAL(clicked()), this, SLOT(insertFunction()));
	vl1->addWidget(addFunctionBtn);

	buttonClear = new QPushButton(tr("Clea&r" ));
	connect(buttonClear, SIGNAL( clicked() ), this, SLOT(clearFunction()));
	vl1->addWidget(buttonClear);

	buttonFunctionLog = new QPushButton(tr("Rece&nt") );
	buttonFunctionLog->setToolTip(tr("Click here to select a recently typed expression"));
	connect(buttonFunctionLog, SIGNAL(clicked()), this, SLOT(showFunctionLog()));
	vl1->addWidget(buttonFunctionLog);

	vl1->addStretch();
	gl1->addLayout(vl1, 0, 0);

	boxFunction = new ScriptEdit(((ApplicationWindow *)parent)->scriptingEnv());
	boxFunction->enableShortcuts();
	gl1->addWidget(boxFunction, 0, 1);

	gl1->addWidget(new QLabel(tr("Variable")), 1, 0);
	boxVariable = new QLineEdit();
	boxVariable->setText("x");
	gl1->addWidget(boxVariable, 1, 1);

	gl1->addWidget(new QLabel(tr("Subintervals")), 2, 0);
	boxSteps = new QSpinBox();
	boxSteps->setRange(1, INT_MAX);
	boxSteps->setValue(1000);
	boxSteps->setSingleStep(100);
	gl1->addWidget(boxSteps, 2, 1);

	QLocale locale = ((ApplicationWindow *)parent)->locale();
	gl1->addWidget(new QLabel(tr("Tolerance")),3, 0);
	boxTol = new DoubleSpinBox();
	boxTol->setLocale(locale);
	boxTol->setValue(0.01);
	boxTol->setMinimum(0.0);
	boxTol->setSingleStep(0.001);
	gl1->addWidget(boxTol, 3, 1);

	gl1->addWidget(new QLabel(tr("Lower limit")), 4, 0);
	boxStart = new DoubleSpinBox();
	boxStart->setLocale(locale);
	gl1->addWidget(boxStart, 4, 1);

	gl1->addWidget(new QLabel(tr("Upper limit")), 5, 0);
	boxEnd = new DoubleSpinBox();
	boxEnd->setLocale(locale);
	boxEnd->setValue(1.0);
	gl1->addWidget(boxEnd, 5, 1);

	boxPlot = new QCheckBox(tr("&Plot area"));
	boxPlot->setChecked(true);
	gl1->addWidget(boxPlot, 6, 1);
	gl1->setRowStretch(0, 1);

	buttonOk = new QPushButton(tr( "&Integrate" ));
	buttonOk->setDefault( true );
	buttonCancel = new QPushButton(tr("&Close" ));

	QVBoxLayout *vl = new QVBoxLayout();
	vl->addWidget(buttonOk);
	vl->addWidget(buttonCancel);
	vl->addStretch();

	QHBoxLayout *hb = new QHBoxLayout(this);
	hb->addWidget(gb1);
	hb->addLayout(vl);

	connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

void IntDialog::accept()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (!app)
		return;

	QString function = boxFunction->toPlainText().simplified();
	if (!validInput(function))
		return;

	app->updateFunctionLists(0, QStringList() << function);

	Integration *i = new Integration(function, boxVariable->text(), app, d_graph, boxStart->value(), boxEnd->value());
	if (!i->error()){
		i->setTolerance(boxTol->text().toDouble());
		i->setWorkspaceSize(boxSteps->value());
		i->enableGraphicsDisplay(d_graph && boxPlot->isChecked(), d_graph);
		i->run();
	}
	delete i;
}

bool IntDialog::validInput(const QString& function)
{
	int points = 100;
	double start = boxStart->value();
	double end = boxEnd->value();
	double step = (end - start)/(double)(points - 1.0);
	double x = end;

	MyParser parser;
    parser.DefineVar(boxVariable->text().toLocal8Bit().constData(), &x);
    parser.SetExpr(function.toLocal8Bit().constData());

	try {
		parser.Eval();
	} catch(mu::ParserError &e) {
		QMessageBox::critical(this, tr("QtiSAS - Input error"), QString::fromStdString(e.GetMsg()));
		return false;
	}

	x = start;
	int lastButOne = points - 1;
	try {
		double xl = x, xr;
		double y = parser.Eval();
		bool wellDefinedFunction = true;
		if (!gsl_finite(y)){// try to find a first well defined point (might help for some not really bad functions)
			wellDefinedFunction = false;
			for (int i = 0; i < lastButOne; i++){
				xl = x;
				x += step;
				xr = x;
				y = parser.Eval();
				if (gsl_finite(y)){
					wellDefinedFunction = true;
					int iter = 0;
					double x0 = x, y0 = y;
					while(fabs(xr - xl)/step > 1e-15 && iter < points){
						x = 0.5*(xl + xr);
						y = parser.Eval();
						if (gsl_finite(y)){
							xr = x;
							x0 = x;
							y0 = y;
						} else
							xl = x;
						iter++;
					}
					start = x0;
					step = (start - end)/(double)(lastButOne);
					break;
				}
			}
			if (!wellDefinedFunction){
				QMessageBox::critical(0, QObject::tr("QtiSAS"),
				QObject::tr("The function %1 is not defined in the specified interval!").arg(function));
				return false;
			}
		}
	} catch (MyParser::Pole) {return false;}

	return true;
}

void IntDialog::insertFunction()
{
	QString fname = boxMathFunctions->currentText().remove("(").remove(")").remove(",").remove(";");
	boxFunction->insertFunction(fname);
	boxFunction->setFocus();
}

void IntDialog::clearFunction()
{
	boxFunction->clear();
}

void IntDialog::showFunctionLog()
{
	ApplicationWindow *d_app = (ApplicationWindow *)this->parent();
	if (!d_app)
		return;

	if (d_app->d_recent_functions.isEmpty()){
		QMessageBox::information(this, tr("QtiSAS"), tr("Sorry, there are no recent expressions available!"));
		return;
	}

	bool ok;
	QString s = QInputDialog::getItem(this, tr("QtiSAS") + " - " + tr("Recent Functions"), tr("Please, choose a function:"), d_app->d_recent_functions, 0, false, &ok);
	if (ok && !s.isEmpty())
		boxFunction->setText(s);
}
