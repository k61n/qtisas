/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Logistic Fit class
 ******************************************************************************/

#ifndef LOGISTICFIT_H
#define LOGISTICFIT_H

#include "Fit.h"

class LogisticFit : public Fit
{
	Q_OBJECT

	public:
		LogisticFit(ApplicationWindow *parent, QwtPlotCurve *);
		LogisticFit(ApplicationWindow *parent, QwtPlotCurve *, double, double);
		LogisticFit(ApplicationWindow *parent, Graph *g);
		LogisticFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		LogisticFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		LogisticFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

		void guessInitialValues();
        double eval(double *par, double x){return (par[0]-par[1])/(1+pow(x/par[2], par[3]))+par[1];};

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);
};

#endif
