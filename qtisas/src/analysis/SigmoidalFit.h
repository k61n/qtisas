/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Sigmoidal (Boltzmann) fit class
 ******************************************************************************/

#ifndef SIGMOIDALFIT_H
#define SIGMOIDALFIT_H

#include "Fit.h"

class SigmoidalFit : public Fit
{
	Q_OBJECT

	public:
		SigmoidalFit(ApplicationWindow *parent, QwtPlotCurve *);
		SigmoidalFit(ApplicationWindow *parent, QwtPlotCurve *, double, double);
		SigmoidalFit(ApplicationWindow *parent, Graph *g);
		SigmoidalFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		SigmoidalFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		SigmoidalFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

        double eval(double *par, double x){return (par[0]-par[1])/(1+exp((x-par[2])/par[3]))+par[1];};

		void guessInitialValues();
		void setLogistic(bool on = true);

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);

		bool d_logistic;
};

#endif
