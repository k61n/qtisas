/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Polynomial fit and linear fit classes
 ******************************************************************************/

#ifndef POLYNOMIALFIT_H
#define POLYNOMIALFIT_H

#include "Fit.h"

class PolynomialFit : public Fit
{
	Q_OBJECT

	public:
		PolynomialFit(ApplicationWindow *parent, QwtPlotCurve *c, int order = 2, bool legend = false);
		PolynomialFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end, int order = 2, bool legend = false);
		PolynomialFit(ApplicationWindow *parent, Graph *g, int order = 2, bool legend = false);
		PolynomialFit(ApplicationWindow *parent, Graph *g, QString& curveTitle, int order = 2, bool legend = false);
		PolynomialFit(ApplicationWindow *parent, Graph *g, QString& curveTitle, double start, double end, int order = 2, bool legend = false);
		PolynomialFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1, int order = 2, bool legend = false);

		virtual QString legendInfo();
		void fit();

		int order(){return d_order;};
		void setOrder(int order);

		static QString generateFormula(int order);
		static QStringList generateParameterList(int order);

		virtual double eval(double *par, double x);

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);

		int d_order;
		bool show_legend;
};

class LinearFit : public Fit
{
	Q_OBJECT

	public:
		LinearFit(ApplicationWindow *parent, Graph *g);
		LinearFit(ApplicationWindow *parent, QwtPlotCurve *c);
		LinearFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end);
		LinearFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		LinearFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		LinearFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

		void fit();
	    virtual double eval(double *par, double x){return par[0] + par[1]*x;};

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);
};

class LinearSlopeFit : public Fit
{
	Q_OBJECT

	public:
		LinearSlopeFit(ApplicationWindow *parent, Graph *g);
		LinearSlopeFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		LinearSlopeFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		LinearSlopeFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

		void fit();
	    virtual double eval(double *par, double x){return par[0]*x;};

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);
};
#endif
