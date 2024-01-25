/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plugin fit class
 ******************************************************************************/

#include "PluginFit.h"

#include <QLibrary>
#include <QMessageBox>

PluginFit::PluginFit(ApplicationWindow *parent, Graph *g)
: Fit(parent, g)
{
	init();
}

PluginFit::PluginFit(ApplicationWindow *parent, QwtPlotCurve *c)
: Fit(parent, c)
{
	init();
	setDataFromCurve(c);
}

PluginFit::PluginFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end)
: Fit(parent, c)
{
	init();
	setDataFromCurve(c, start, end);
}

PluginFit::PluginFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

PluginFit::PluginFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

PluginFit::PluginFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: Fit(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void PluginFit::init()
{
	d_explanation = tr("Plugin Fit");
    d_fit_type = Plugin;
}

bool PluginFit::load(const QString& pluginName)
{
	if (!QFile::exists (pluginName)){
		QMessageBox::critical((ApplicationWindow *)parent(), tr("QtiSAS - File not found"),
				tr("Plugin file: <p><b> %1 </b> <p>not found. Operation aborted!").arg(pluginName));
		return false;
	}

	QLibrary lib(pluginName);

	d_fsimplex = (fit_function_simplex) lib.resolve( "function_d" );
	if (!d_fsimplex)
		return false;

	d_f = (fit_function) lib.resolve( "function_f" );
	if (!d_f)
		return false;

	d_df = (fit_function_df) lib.resolve( "function_df" );
	if (!d_df)
		return false;

	d_fdf = (fit_function_fdf) lib.resolve( "function_fdf" );
	if (!d_fdf)
		return false;

	f_eval = (fitFunctionEval) lib.resolve("function_eval");
	if (!f_eval)
		return false;

	typedef char* (*fitFunc)();
	fitFunc fitFunction = (fitFunc) lib.resolve("parameters");
	if (fitFunction){
		d_param_names = QString(fitFunction()).split(",", QString::SkipEmptyParts);
		d_p = (int)d_param_names.count();
        initWorkspace(d_p);
	} else
		return false;

	fitFunc fitExplain = (fitFunc) lib.resolve("explanations");
	if (fitExplain)
		d_param_explain = QString(fitExplain()).split(",", QString::SkipEmptyParts);
	else
		for (int i=0; i<d_p; i++)
			d_param_explain << "";

	fitFunction = (fitFunc) lib.resolve( "name" );
	setObjectName(QString(fitFunction()));

	fitFunction = (fitFunc) lib.resolve( "function" );
	if (fitFunction)
		d_formula = QString(fitFunction());
	else
		return false;

	return true;
}

void PluginFit::calculateFitCurveData(double *X, double *Y)
{
	if (d_gen_function) {
		double X0 = d_x[0];
		double step = (d_x[d_n-1]-X0)/(d_points-1);
		for (int i=0; i<d_points; i++){
		    double x = X0+i*step;
			X[i] = x;
			Y[i]= f_eval(x, d_results);
		}
	} else {
		for (int i=0; i<d_points; i++) {
		    double x = d_x[i];
			X[i] = x;
			Y[i]= f_eval(x, d_results);
		}
	}
}
