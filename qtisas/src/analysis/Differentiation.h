/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Numerical differentiation of data sets
 ******************************************************************************/

#ifndef DIFFERENTIATION_H
#define DIFFERENTIATION_H

#include "Filter.h"

class Differentiation : public Filter
{
Q_OBJECT

public:
	Differentiation(ApplicationWindow *parent, Graph *g);
	Differentiation(ApplicationWindow *parent, QwtPlotCurve *c);
	Differentiation(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end);
	Differentiation(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
	Differentiation(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
	Differentiation(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int start = 0, int end = -1);

private:
    void init();
    void output();
};
#endif
