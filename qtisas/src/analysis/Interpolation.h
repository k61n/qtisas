/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Numerical interpolation of data sets
 ******************************************************************************/

#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <qwt/qwt_plot_curve.h>

#include "Filter.h"

class Interpolation : public Filter
{
Q_OBJECT

public:
	enum InterpolationMethod{Linear, Cubic, Akima};

	Interpolation(ApplicationWindow *parent, QwtPlotCurve *c, int m = 0);
	Interpolation(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end, int m = 0);
	Interpolation(ApplicationWindow *parent, Graph *g, const QString& curveTitle, int m = 0);
	Interpolation(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end, int m = 0);
	Interpolation(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int start = 0, int end = -1, int m = 0);

    int method(){return d_method;};
    void setMethod(int m);

private:
    void init(int m);
    void calculateOutputData(double *x, double *y);
    int sortedCurveData(QwtPlotCurve *c, double start, double end, double **x, double **y);

    //! the interpolation method
    int d_method;
};

#endif
