/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Sigmoidal (Boltzmann) fit class
 ******************************************************************************/

#include "fit_gsl.h"
#include "SigmoidalFit.h"

SigmoidalFit::SigmoidalFit(ApplicationWindow *parent, QwtPlotCurve *c)
: Fit(parent, c)
{
	init();
	setDataFromCurve(c);
}

SigmoidalFit::SigmoidalFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end)
: Fit(parent, c)
{
	init();
	setDataFromCurve(c, start, end);
}

SigmoidalFit::SigmoidalFit(ApplicationWindow *parent, Graph *g)
: Fit(parent, g)
{
	init();
}

SigmoidalFit::SigmoidalFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

SigmoidalFit::SigmoidalFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

SigmoidalFit::SigmoidalFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: Fit(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void SigmoidalFit::init()
{
	setObjectName(tr("Boltzmann"));
	d_f = boltzmann_f;
	d_df = boltzmann_df;
	d_fdf = boltzmann_fdf;
	d_fsimplex = boltzmann_d;
	d_param_explain << tr("init value") << tr("final value") << tr("center") << tr("time constant");
	d_param_names << "A1" << "A2" << "x0" << "dx";
	d_explanation = tr("Boltzmann (Sigmoidal) Fit");
	d_formula = "A2+(A1-A2)/(1+exp((x-x0)/dx))";
	d_p = 4;
    initWorkspace(d_p);
}

void SigmoidalFit::calculateFitCurveData(double *X, double *Y)
{
	double a1 = d_results[0];
	double a2 = d_results[1];
	double x0 = d_results[2];
	double p = d_results[3];

	if (d_gen_function){
		double X0 = d_x[0];
		double step = (d_x[d_n-1]-X0)/(d_points-1);
        for (int i=0; i<d_points; i++){
			double x = X0+i*step;
        	X[i] = x;
        	Y[i] = (a1 - a2)/(1 + exp((x - x0)/p)) + a2;
		}
	} else {
        for (int i=0; i<d_points; i++){
			double x = d_x[i];
        	X[i] = x;
        	Y[i] = (a1 - a2)/(1 + exp((x - x0)/p)) + a2;
		}
	}
}

void SigmoidalFit::guessInitialValues()
{
	gsl_vector_view x = gsl_vector_view_array (d_x, d_n);
	gsl_vector_view y = gsl_vector_view_array (d_y, d_n);

	double min_out, max_out;
	gsl_vector_minmax (&y.vector, &min_out, &max_out);

	gsl_vector_set(d_param_init, 0, min_out);
	gsl_vector_set(d_param_init, 1, max_out);
	gsl_vector_set(d_param_init, 2, gsl_vector_get (&x.vector, d_n/2));
	gsl_vector_set(d_param_init, 3, 1.0);
}
