/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Polynomial fit and linear fit classes
 ******************************************************************************/

#include <QLocale>
#include <QMessageBox>

#include <gsl/gsl_fit.h>
#include <gsl/gsl_multifit.h>

#include "PolynomialFit.h"

PolynomialFit::PolynomialFit(ApplicationWindow *parent, Graph *g, int order, bool legend)
: Fit(parent, g), d_order(order), show_legend(legend)
{
	init();
}

PolynomialFit::PolynomialFit(ApplicationWindow *parent, QwtPlotCurve *c, int order, bool legend)
: Fit(parent, c), d_order(order), show_legend(legend)
{
	init();
	setDataFromCurve(c);
}

PolynomialFit::PolynomialFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end, int order, bool legend)
: Fit(parent, c), d_order(order), show_legend(legend)
{
	init();
	setDataFromCurve(c, start, end);
}

PolynomialFit::PolynomialFit(ApplicationWindow *parent, Graph *g, QString& curveTitle, int order, bool legend)
: Fit(parent, g), d_order(order), show_legend(legend)
{
	init();
	setDataFromCurve(curveTitle);
}

PolynomialFit::PolynomialFit(ApplicationWindow *parent, Graph *g, QString& curveTitle, double start, double end, int order, bool legend)
: Fit(parent, g), d_order(order), show_legend(legend)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

PolynomialFit::PolynomialFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow, int order, bool legend)
: Fit(parent, t), d_order(order), show_legend(legend)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void PolynomialFit::init()
{
	setObjectName(tr("Polynomial"));
	is_non_linear = false;
	d_explanation = tr("Polynomial Fit");
	setOrder(d_order);
	d_scale_errors = false;
}

void PolynomialFit::setOrder(int order)
{
	d_order = order;
	d_p = d_order + 1;

	freeWorkspace();
	initWorkspace(d_p);

	d_formula = generateFormula(d_order);
	d_param_names = generateParameterList(d_order);

	d_param_explain.clear();
	for (int i=0; i<d_p; i++)
		d_param_explain << "";
}

QString PolynomialFit::generateFormula(int order)
{
    QString formula = QString();
	for (int i = 0; i < order+1; i++){
		QString par = "a" + QString::number(i);
		formula += par;
		if (i>0)
			formula +="*x";
		if (i>1)
			formula += "^"+QString::number(i);
		if (i != order)
			formula += "+";
	}
	return formula;
}

QStringList PolynomialFit::generateParameterList(int order)
{
	QStringList lst;
	for (int i = 0; i < order+1; i++)
		lst << "a" + QString::number(i);
	return lst;
}

void PolynomialFit::calculateFitCurveData(double *X, double *Y)
{
	if (d_gen_function){
		double X0 = d_x[0];
		double step = (d_x[d_n-1]-X0)/(d_points-1);
		for (int i=0; i<d_points; i++){
		    double x = X0+i*step;
			X[i] = x;
			double 	yi = 0.0;
			for (int j=0; j<d_p;j++)
				yi += d_results[j]*pow(x, j);
			Y[i] = yi;
		}
	} else {
		for (int i=0; i<d_points; i++) {
		    double x = d_x[i];
			X[i] = x;
			double 	yi = 0.0;
			for (int j=0; j<d_p; j++)
				yi += d_results[j]*pow(x, j);
			Y[i] = yi;
		}
	}
}

double PolynomialFit::eval(double *par, double x)
{
	double 	y = 0.0;
	for (int j=0; j<d_p; j++)
		y += par[j]*pow(x, j);
	return y;
}

void PolynomialFit::fit()
{
    if (d_init_err)
        return;

	if (d_p > d_n){
  		QMessageBox::critical((ApplicationWindow *)parent(), tr("QtiSAS - Fit Error"),
  	    tr("You need at least %1 data points for this fit operation. Operation aborted!").arg(d_p));
  		return;
  	}

	gsl_matrix *X = gsl_matrix_alloc (d_n, d_p);

	for (int i = 0; i <d_n; i++){
		for (int j= 0; j < d_p; j++)
			gsl_matrix_set (X, i, j, pow(d_x[i],j));
	}

	gsl_vector_view y = gsl_vector_view_array (d_y, d_n);
	gsl_vector_view w = gsl_vector_view_array (d_w, d_n);
	gsl_multifit_linear_workspace * work = gsl_multifit_linear_alloc (d_n, d_p);

	if (d_weighting == NoWeighting)
		gsl_multifit_linear (X, &y.vector, d_param_init, covar, &chi_2, work);
	else
		gsl_multifit_wlinear (X, &w.vector, &y.vector, d_param_init, covar, &chi_2, work);

	for (int i = 0; i < d_p; i++)
		d_results[i] = gsl_vector_get(d_param_init, i);

	gsl_multifit_linear_free (work);
	gsl_matrix_free (X);

	generateFitCurve();

	if (show_legend)
		showLegend();

	ApplicationWindow *app = (ApplicationWindow *)parent();
	if (app->writeFitResultsToLog)
		app->updateLog(logFitInfo(0, 0));
}

QString PolynomialFit::legendInfo()
{
    ApplicationWindow *app = (ApplicationWindow *)parent();
    QLocale locale = app->locale();
	QString legend = "Y=" + locale.toString(d_results[0], 'g', d_prec);
	for (int j = 1; j < d_p; j++){
		double cj = d_results[j];
		if (cj>0 && !legend.isEmpty())
			legend += "+";

        QString s = QString::asprintf("%.5f", cj);
		if (s != "1.00000")
			legend += locale.toString(cj, 'g', d_prec);

		legend += "X";
		if (j>1)
			legend += "<sup>" + QString::number(j) + "</sup>";
	}
	return legend;
}

/*****************************************************************************
 *
 * Class LinearFit
 *
 *****************************************************************************/

LinearFit::LinearFit(ApplicationWindow *parent, QwtPlotCurve *c)
: Fit(parent, c)
{
	init();
	setDataFromCurve(c);
}

LinearFit::LinearFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end)
: Fit(parent, c)
{
	init();
	setDataFromCurve(c, start, end);
}

LinearFit::LinearFit(ApplicationWindow *parent, Graph *g)
: Fit(parent, g)
{
	init();
}

LinearFit::LinearFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

LinearFit::LinearFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

LinearFit::LinearFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: Fit(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void LinearFit::init()
{
	d_scale_errors = false;

	d_p = 2;
    d_min_points = d_p;

	covar = gsl_matrix_alloc (d_p, d_p);
	d_results = new double[d_p];

    d_param_init = gsl_vector_alloc(d_p);
	gsl_vector_set_all (d_param_init, 1.0);

	is_non_linear = false;
	d_formula = "A*x+B";
	d_param_names << "B" << "A";
	d_param_explain << "y-intercept" << "slope";
	d_explanation = tr("Linear Regression");
	setObjectName(tr("Linear"));
}

void LinearFit::fit()
{
    if (d_init_err)
        return;

	if (d_p > d_n){
  		QMessageBox::critical((ApplicationWindow *)parent(), tr("QtiSAS - Fit Error"),
  	    tr("You need at least %1 data points for this fit operation. Operation aborted!").arg(d_p));
  		return;
  	}

	double c0, c1, cov00, cov01, cov11;
	if (d_weighting == NoWeighting)
		gsl_fit_linear(d_x, 1, d_y, 1, d_n, &c0, &c1, &cov00, &cov01, &cov11, &chi_2);
	else
		gsl_fit_wlinear(d_x, 1, d_w, 1, d_y, 1, d_n, &c0, &c1, &cov00, &cov01, &cov11, &chi_2);

	d_results[0] = c0;
	d_results[1] = c1;

	gsl_matrix_set(covar, 0, 0, cov00);
	gsl_matrix_set(covar, 0, 1, cov01);
	gsl_matrix_set(covar, 1, 1, cov11);
	gsl_matrix_set(covar, 1, 0, cov01);

	generateFitCurve();

	ApplicationWindow *app = (ApplicationWindow *)parent();
	if (app->writeFitResultsToLog)
		app->updateLog(logFitInfo(0, 0));
}

void LinearFit::calculateFitCurveData(double *X, double *Y)
{
	if (d_gen_function){
		double X0 = d_x[0];
		double step = (d_x[d_n-1]-X0)/(d_points-1);
		for (int i=0; i<d_points; i++){
		    double x = X0+i*step;
			X[i] = x;
			Y[i] = d_results[0] + d_results[1]*x;
		}
	} else {
		for (int i=0; i<d_points; i++) {
		    double x = d_x[i];
			X[i] = x;
			Y[i] = d_results[0] + d_results[1]*x;
		}
	}
}


/*****************************************************************************
 *
 * Class LinearSlopeFit
 *
 *****************************************************************************/

LinearSlopeFit::LinearSlopeFit(ApplicationWindow *parent, Graph *g)
: Fit(parent, g)
{
	init();
}

LinearSlopeFit::LinearSlopeFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

LinearSlopeFit::LinearSlopeFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

LinearSlopeFit::LinearSlopeFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: Fit(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void LinearSlopeFit::init()
{
	d_scale_errors = false;

	d_p = 1;
    d_min_points = d_p;

	covar = gsl_matrix_alloc (d_p, d_p);
	d_results = new double[d_p];

    d_param_init = gsl_vector_alloc(d_p);
	gsl_vector_set_all (d_param_init, 1.0);

	is_non_linear = false;
	d_formula = "A*x";
	d_param_names << "A";
	d_param_explain << "slope";
	d_explanation = tr("Linear Regression");
	setObjectName(tr("LinearSlope"));
}

void LinearSlopeFit::fit()
{
    if (d_init_err)
        return;

	if (d_p > d_n){
  		QMessageBox::critical((ApplicationWindow *)parent(), tr("QtiSAS - Fit Error"),
  	    tr("You need at least %1 data points for this fit operation. Operation aborted!").arg(d_p));
  		return;
  	}

	double c1, cov11;
	if (d_weighting == NoWeighting)
		gsl_fit_mul(d_x, 1, d_y, 1, d_n, &c1, &cov11, &chi_2);
	else
		gsl_fit_wmul(d_x, 1, d_w, 1, d_y, 1, d_n, &c1, &cov11, &chi_2);

	d_results[0] = c1;

	gsl_matrix_set(covar, 0, 0, cov11);
	generateFitCurve();

	ApplicationWindow *app = (ApplicationWindow *)parent();
	if (app->writeFitResultsToLog)
		app->updateLog(logFitInfo(0, 0));
}

void LinearSlopeFit::calculateFitCurveData(double *X, double *Y)
{
	if (d_gen_function){
		double X0 = d_x[0];
		double step = (d_x[d_n-1] - X0)/(d_points - 1);
		for (int i=0; i<d_points; i++){
		    double x = X0 + i*step;
			X[i] = x;
			Y[i] = d_results[0]*x;
		}
	} else {
		for (int i=0; i<d_points; i++) {
		    double x = d_x[i];
			X[i] = x;
			Y[i] = d_results[0]*x;
		}
	}
}
