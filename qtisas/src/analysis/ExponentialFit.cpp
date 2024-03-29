/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Exponential fit classes
 ******************************************************************************/

#include "ExponentialFit.h"
#include "fit_gsl.h"

/*****************************************************************************
 *
 * Class ExponentialFit
 *
 *****************************************************************************/

ExponentialFit::ExponentialFit(ApplicationWindow *parent, QwtPlotCurve *c, bool expGrowth)
: Fit(parent, c),
	is_exp_growth(expGrowth)
{
	init();
	setDataFromCurve(c);
}

ExponentialFit::ExponentialFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end, bool expGrowth)
: Fit(parent, c),
	is_exp_growth(expGrowth)
{
	init();
	setDataFromCurve(c, start, end);
}

ExponentialFit::ExponentialFit(ApplicationWindow *parent, Graph *g, bool expGrowth)
: Fit(parent, g),
	is_exp_growth(expGrowth)
{
	init();
}

ExponentialFit::ExponentialFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, bool expGrowth)
: Fit(parent, g),
  is_exp_growth(expGrowth)
{
	init();
	setDataFromCurve(curveTitle);
}

ExponentialFit::ExponentialFit(ApplicationWindow *parent, Graph *g,
		const QString& curveTitle, double start, double end, bool expGrowth)
: Fit(parent, g),
	is_exp_growth(expGrowth)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

ExponentialFit::ExponentialFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow, bool expGrowth)
: Fit(parent, t), is_exp_growth(expGrowth)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void ExponentialFit::init()
{
	d_f = exp_f;
	d_df = exp_df;
	d_fdf = exp_fdf;
	d_fsimplex = exp_d;
	d_p = 3;
	initWorkspace(d_p);

	d_param_names << "A" << "t" << "y0";

	if (is_exp_growth) {
		setObjectName(tr("ExpGrowth"));
		d_explanation = tr("Exponential growth");
		d_formula = "y0+A*exp(x/t)";
		d_param_explain << tr("amplitude") << tr("lifetime") << tr("offset");
	} else {
		setObjectName(tr("ExpDecay1"));
		d_explanation = tr("Exponential decay");
		d_formula = "y0+A*exp(-x/t)";
		d_param_explain << tr("amplitude") << tr("e-folding time") << tr("offset");
	}
}

void ExponentialFit::customizeFitResults()
{
	if (is_exp_growth)
		d_results[1] = -d_results[1];
}

void ExponentialFit::calculateFitCurveData(double *X, double *Y)
{
	double a = d_results[0];
	double l = -d_results[1];
	double y0 = d_results[2];
	if (d_gen_function){
		double X0 = d_x[0];
		double step = (d_x[d_n-1] - X0)/(d_points - 1);
		for (int i = 0; i < d_points; i++) {
			double x = X0 + i*step;
			X[i] = x;
			Y[i] = a*exp(x/l) + y0;
		}
	} else {
		for (int i = 0; i < d_points; i++) {
		    double x = d_x[i];
			X[i] = x;
			Y[i] = a*exp(x/l) + y0;
		}
	}
}

/*****************************************************************************
 *
 * Class TwoExpFit
 *
 *****************************************************************************/

TwoExpFit::TwoExpFit(ApplicationWindow *parent, QwtPlotCurve *c)
: Fit(parent, c)
{
	init();
	setDataFromCurve(c);
}

TwoExpFit::TwoExpFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end)
: Fit(parent, c)
{
	init();
	setDataFromCurve(c, start, end);
}

TwoExpFit::TwoExpFit(ApplicationWindow *parent, Graph *g)
: Fit(parent, g)
{
	init();
}

TwoExpFit::TwoExpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

TwoExpFit::TwoExpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

TwoExpFit::TwoExpFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: Fit(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void TwoExpFit::init()
{
	setObjectName(tr("ExpDecay2"));
	d_f = expd2_f;
	d_df = expd2_df;
	d_fdf = expd2_fdf;
	d_fsimplex = expd2_d;
	d_p = 5;
    initWorkspace(d_p);
	d_param_names << "A1" << "t1" << "A2" << "t2" << "y0";
	d_explanation = tr("Exponential decay");
	d_formula = "A1*exp(-x/t1)+A2*exp(-x/t2)+y0";
	d_param_explain << tr("first amplitude") << tr("first lifetime") << tr("second amplitude") << tr("second lifetime") << tr("offset");
}

void TwoExpFit::calculateFitCurveData(double *X, double *Y)
{
	double a1 = d_results[0];
	double l1 = -d_results[1];
	double a2 = d_results[2];
	double l2 = -d_results[3];
	double y0 = d_results[4];

	if (d_gen_function){
		double X0 = d_x[0];
		double step = (d_x[d_n - 1] - X0)/(d_points - 1);
		for (int i = 0; i < d_points; i++){
		    double x = X0 + i*step;
			X[i] = x;
			Y[i] = a1*exp(x/l1) + a2*exp(x/l2) + y0;
		}
	} else {
		for (int i = 0; i < d_points; i++){
		    double x = d_x[i];
			X[i] = x;
			Y[i] = a1*exp(x/l1) + a2*exp(x/l2) + y0;
		}
	}
}

/*****************************************************************************
 *
 * Class ThreeExpFit
 *
 *****************************************************************************/

ThreeExpFit::ThreeExpFit(ApplicationWindow *parent, QwtPlotCurve *c)
: Fit(parent, c)
{
	init();
	setDataFromCurve(c);
}

ThreeExpFit::ThreeExpFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end)
: Fit(parent, c)
{
	init();
	setDataFromCurve(c, start, end);
}

ThreeExpFit::ThreeExpFit(ApplicationWindow *parent, Graph *g)
: Fit(parent, g)
{
	init();
}

ThreeExpFit::ThreeExpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

ThreeExpFit::ThreeExpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

ThreeExpFit::ThreeExpFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: Fit(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void ThreeExpFit::init()
{
	setObjectName(tr("ExpDecay3"));
	d_f = expd3_f;
	d_df = expd3_df;
	d_fdf = expd3_fdf;
	d_fsimplex = expd3_d;
	d_p = 7;
    initWorkspace(d_p);
	d_param_names << "A1" << "t1" << "A2" << "t2" << "A3" << "t3" << "y0";
	d_explanation = tr("Exponential decay");
	d_formula = "A1*exp(-x/t1)+A2*exp(-x/t2)+A3*exp(-x/t3)+y0";
	d_param_explain << tr("first amplitude") << tr("first lifetime") << tr("second amplitude") << tr("second lifetime") << tr("third amplitude") << tr("third lifetime") << tr("offset");
}

void ThreeExpFit::calculateFitCurveData(double *X, double *Y)
{
	double a1 = d_results[0];
	double l1 = -d_results[1];
	double a2 = d_results[2];
	double l2 = -d_results[3];
	double a3 = d_results[4];
	double l3 = -d_results[5];
	double y0 = d_results[6];

	if (d_gen_function){
		double X0 = d_x[0];
		double step = (d_x[d_n - 1] - X0)/(d_points - 1);
		for (int i = 0; i < d_points; i++){
		    double x = X0 + i*step;
			X[i] = x;
			Y[i] = a1*exp(x/l1) + a2*exp(x/l2) + a3*exp(x/l3) + y0;
		}
	} else {
		for (int i = 0; i < d_points; i++){
		    double x = d_x[i];
			X[i] = x;
			Y[i] = a1*exp(x/l1) + a2*exp(x/l2) + a3*exp(x/l3) + y0;
		}
	}
}
