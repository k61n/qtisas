/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plugin fit class
 ******************************************************************************/

#ifndef PLUGINFIT_H
#define PLUGINFIT_H

#include "Fit.h"

class PluginFit : public Fit
{
	Q_OBJECT

	public:
	    PluginFit(ApplicationWindow *parent, QwtPlotCurve *);
	    PluginFit(ApplicationWindow *parent, QwtPlotCurve *, double, double);
		PluginFit(ApplicationWindow *parent, Graph *g);
		PluginFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		PluginFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		PluginFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

		bool load(const QString& pluginName);
        double eval(double *par, double x){return f_eval(x, par);};

	private:
		void init();
		typedef double (*fitFunctionEval)(double, double *);
		void calculateFitCurveData(double *X, double *Y);
		fitFunctionEval f_eval;
};
#endif
