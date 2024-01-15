/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Numerical integration of data sets
 ******************************************************************************/

#ifndef INTEGRATION_H
#define INTEGRATION_H

#include "Filter.h"

class Integration : public Filter
{
Q_OBJECT

public:
	enum Integrand{DataSet, AnalyticalFunction};

	Integration(ApplicationWindow *parent, Graph *g = 0);
	Integration(ApplicationWindow *parent, QwtPlotCurve *c);
	Integration(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end);
	Integration(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
	Integration(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
	Integration(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int start = 0, int end = -1, bool sort = false);
	Integration(const QString& formula, const QString& var, ApplicationWindow *parent, Graph *g, double start, double end);

	void setWorkspaceSize(int size){d_workspace_size = size;};

	double area(){return d_area;};
	QString variable(){return d_variable;};
	QString formula(){return d_formula;};

private:
    void init();
    QString logInfo();

    void output();

	double trapez();
	double gslIntegration();

    //! the value of the integral
    double d_area;
	//! the value of the estimated error in GSL integration
	double d_error;
	//! the value of the workspace size in GSL integration
	int d_workspace_size;

	//! the type of the integrand
	Integrand d_integrand;
	//! Analytical function to be integrated
	QString d_formula;
	//! Variable name for the function to be integrated
	QString d_variable;
};

#endif
