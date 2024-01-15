/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Exponential fit classes
 ******************************************************************************/

#ifndef EXPONENTIALFIT_H
#define EXPONENTIALFIT_H

#include "Fit.h"

class ExponentialFit : public Fit
{
	Q_OBJECT

	public:
		ExponentialFit(ApplicationWindow *parent, QwtPlotCurve *c, bool expGrowth = false);
		ExponentialFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end, bool expGrowth = false);
		ExponentialFit(ApplicationWindow *parent, Graph *g,  bool expGrowth = false);
		ExponentialFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, bool expGrowth = false);
		ExponentialFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle,
				double start, double end, bool expGrowth = false);
		ExponentialFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1, bool expGrowth = false);

        double eval(double *par, double x){return par[0]*exp(-x/par[1]) + par[2];};
		bool isExponentialGrowth(){return is_exp_growth;};

	private:
		void init();
		void customizeFitResults();
		void calculateFitCurveData(double *X, double *Y);

		bool is_exp_growth;
};

class TwoExpFit : public Fit
{
	Q_OBJECT

	public:
	    TwoExpFit(ApplicationWindow *parent, QwtPlotCurve *c);
	    TwoExpFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end);
		TwoExpFit(ApplicationWindow *parent, Graph *g);
		TwoExpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		TwoExpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		TwoExpFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

        double eval(double *par, double x){return par[0]*exp(-x/par[1]) + par[2]*exp(-x/par[3]) + par[4];};

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);
};

class ThreeExpFit : public Fit
{
	Q_OBJECT

	public:
		ThreeExpFit(ApplicationWindow *parent, QwtPlotCurve *c);
	    ThreeExpFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end);
		ThreeExpFit(ApplicationWindow *parent, Graph *g);
		ThreeExpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		ThreeExpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		ThreeExpFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

        double eval(double *par, double x){return par[0]*exp(-x/par[1]) + par[2]*exp(-x/par[3]) + par[4]*exp(-x/par[5]) + par[6];};

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);
};
#endif
