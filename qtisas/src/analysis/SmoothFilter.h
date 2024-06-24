/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2009 Jonas Bähr <jonas@fs.ei.tum.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Numerical smoothing of data sets
 ******************************************************************************/

#ifndef SMOOTHFILTER_H
#define SMOOTHFILTER_H

#include <gsl/gsl_matrix_double.h>

#include "Filter.h"

class SmoothFilter : public Filter
{
Q_OBJECT

public:
	SmoothFilter(ApplicationWindow *parent, QwtPlotCurve *c, int m = 3);
	SmoothFilter(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end, int m = 3);
	SmoothFilter(ApplicationWindow *parent, Graph *g, const QString& curveTitle, int m = 3);
	SmoothFilter(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end, int m = 3);
	SmoothFilter(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int start = 0, int end = -1, int m = 3);

    enum SmoothMethod{SavitzkyGolay = 1, FFT = 2, Average = 3, Lowess = 4};

    int method(){return (int)d_method;};
    void setMethod(int m);

    void setSmoothPoints(int points, int left_points = 0);
    //! Sets the polynomial order in the Savitky-Golay algorithm.
    void setPolynomOrder(int order);
    void setLowessParameter(double f, int iterations);

private:
    void init(int m);
    void calculateOutputData(double *x, double *y);
    void smoothFFT(double *x, double *y);
    void smoothAverage(double *x, double *y);
    void smoothSavGol(double *x, double *y);
    void smoothLowess(double *x, double *y);
	static int savitzkyGolayCoefficients(int points, int polynom_order, gsl_matrix *h);

    //! The smooth method.
    SmoothMethod d_method;

    //! The number of adjacents points used to smooth the data set.
    int d_smooth_points;

    //! The number of left adjacents points used by the Savitky-Golay algorithm.
    int d_sav_gol_points;

    //! Polynomial order in the Savitky-Golay algorithm (see Numerical Receipes in C for details).
    int d_polynom_order;

    //! Parameter f for the Lowess algorithm
    double d_f;

    //! Number of iterations for the Lowess algorithm
    int d_iterations;
};

#endif
