/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2009 masantos <masantos@5a6a7de5-fb12-0410-b871-c33778c25c60>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Built-in data fit models for QtiPlot
 ******************************************************************************/

#include <cmath>

#include <QMessageBox>

#include <gsl/gsl_math.h>

#include "fit_gsl.h"
#include "NonLinearFit.h"
#include "MyParser.h"

int expd3_f (const gsl_vector * x, void *params, gsl_vector * f){
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double A1 = gsl_vector_get(x, 0);
    double t1 = gsl_vector_get(x, 1);
    double A2 = gsl_vector_get(x, 2);
    double t2 = gsl_vector_get(x, 3);
    double A3 = gsl_vector_get(x, 4);
    double t3 = gsl_vector_get(x, 5);
    double y0 = gsl_vector_get(x, 6);
    for (int i = 0; i < n; i++) {
    	double t = -X[i];
        double Yi = A1 * exp(t/t1) + A2 * exp(t/t2) + A3 * exp(t/t3) + y0;
		double s = 1.0/sqrt(sigma[i]);
        gsl_vector_set (f, i, (Yi - Y[i])/s);
    }
    return GSL_SUCCESS;
}
double expd3_d (const gsl_vector * x, void *params){
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double A1 = gsl_vector_get(x, 0);
    double t1 = gsl_vector_get(x, 1);
    double A2 = gsl_vector_get(x, 2);
    double t2 = gsl_vector_get(x, 3);
    double A3 = gsl_vector_get(x, 4);
    double t3 = gsl_vector_get(x, 5);
    double y0 = gsl_vector_get(x, 6);
    double val = 0;
    for (int i = 0; i < n; i++) {
    	double t = -X[i];
		double s = 1.0/sqrt(sigma[i]);
        double dYi = ((A1 * exp(t/t1) + A2 * exp(t/t2) + A3 * exp(t/t3) + y0) - Y[i])/s;
        val += dYi * dYi;
    }
    return val;
}
int expd3_df (const gsl_vector * x, void *params, gsl_matrix * J){
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;
    double A1 = gsl_vector_get(x, 0);
    double l1 = gsl_vector_get(x, 1);
    double A2 = gsl_vector_get(x, 2);
    double l2 = gsl_vector_get(x, 3);
    double A3 = gsl_vector_get(x, 4);
    double l3 = gsl_vector_get(x, 5);
    for (int i = 0; i < n; i++) {
        /* Jacobian matrix J(i,j) = dfi / dxj, */
        /* where fi = (Yi - yi)/sigma[i],      */
        /*       Yi = A1 * exp(-xi/l1) + A2 * exp(-xi/l2) + A3 * exp(-xi/l3) + y0  */
        /* and the xj are the parameters (A1, l1 ,A2, l2, A3, l3, y0) */
        double t = X[i];
        double s = 1.0/sqrt(sigma[i]);
        double e1 = exp(-t*l1)/s;
        double e2 = exp(-t*l2)/s;
        double e3 = exp(-t*l3)/s;
        gsl_matrix_set (J, i, 0, e1);
        gsl_matrix_set (J, i, 1, t * A1 * e1/(l1*l1));
        gsl_matrix_set (J, i, 2, e2);
        gsl_matrix_set (J, i, 3, t * A2 * e2/(l2*l2));
        gsl_matrix_set (J, i, 4, e3);
        gsl_matrix_set (J, i, 5, t * A3 * e3/(l3*l3));
        gsl_matrix_set (J, i, 6, 1/s);
    }
    return GSL_SUCCESS;
}
int expd3_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J){
    expd3_f (x, params, f);
    expd3_df (x, params, J);
    return GSL_SUCCESS;
}
int expd2_f (const gsl_vector * x, void *params, gsl_vector * f){
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double A1 = gsl_vector_get(x, 0);
    double t1 = gsl_vector_get(x, 1);
    double A2 = gsl_vector_get(x, 2);
    double t2 = gsl_vector_get(x, 3);
    double y0 = gsl_vector_get(x, 4);
    for (int i = 0; i < n; i++) {
    	double t = -X[i];
        double Yi = A1 * exp (t/t1) + A2 * exp (t/t2) + y0;
		double s = 1.0/sqrt(sigma[i]);
        gsl_vector_set (f, i, (Yi - Y[i])/s);
    }
    return GSL_SUCCESS;
}
double expd2_d (const gsl_vector * x, void *params){
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double A1 = gsl_vector_get(x, 0);
    double t1 = gsl_vector_get(x, 1);
    double A2 = gsl_vector_get(x, 2);
    double t2 = gsl_vector_get(x, 3);
    double y0 = gsl_vector_get(x, 4);
    double val = 0;
    for (int i = 0; i < n; i++) {
    	double t = -X[i];
		double s = 1.0/sqrt(sigma[i]);
        double dYi = ((A1 * exp (t/t1) + A2 * exp (t/t2) + y0) - Y[i])/s;
        val += dYi * dYi;
    }
    return val;
}
int expd2_df (const gsl_vector * x, void *params, gsl_matrix * J){
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;
    double A1 = gsl_vector_get(x, 0);
    double l1 = gsl_vector_get(x, 1);
    double A2 = gsl_vector_get(x, 2);
    double l2 = gsl_vector_get(x, 3);
    for (int i = 0; i < n; i++) {
        /* Jacobian matrix J(i,j) = dfi / dxj, */
        /* where fi = (Yi - yi)/sigma[i],      */
        /*       Yi = A1 * exp(-xi/l1) + A2 * exp(-xi/l2) + y0  */
        /* and the xj are the parameters (A1, l1, A2, l2, y0) */
        double s = 1.0/sqrt(sigma[i]);
        double t = X[i];
        double e1 = exp(-t/l1)/s;
        double e2 = exp(-t/l2)/s;
        gsl_matrix_set (J, i, 0, e1);
        gsl_matrix_set (J, i, 1, t * A1 * e1/(l1*l1));
        gsl_matrix_set (J, i, 2, e2);
        gsl_matrix_set (J, i, 3, t * A2 * e2/(l2*l2));
        gsl_matrix_set (J, i, 4, 1/s);
    }
    return GSL_SUCCESS;
}
int expd2_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J){
    expd2_f (x, params, f);
    expd2_df (x, params, J);
    return GSL_SUCCESS;
}
int exp_f (const gsl_vector * x, void *params, gsl_vector * f) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double A = gsl_vector_get (x, 0);
    double lambda = gsl_vector_get (x, 1);
    double b = gsl_vector_get (x, 2);
    for (int i = 0; i < n; i++) {
        double Yi = A * exp (-X[i]/lambda) + b;
        double s = 1.0/sqrt(sigma[i]);
        gsl_vector_set (f, i, (Yi - Y[i])/s);
    }
    return GSL_SUCCESS;
}
double exp_d (const gsl_vector * x, void *params) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double A = gsl_vector_get (x, 0);
    double lambda = gsl_vector_get (x, 1);
    double b = gsl_vector_get (x, 2);
    double val=0;
    for (int i = 0; i < n; i++) {
        double s = 1.0/sqrt(sigma[i]);
        double dYi = ((A * exp(-X[i]/lambda) + b) - Y[i])/s;
        val += dYi * dYi;
    }
    return val;
}
int exp_df (const gsl_vector * x, void *params, gsl_matrix * J) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;
    double A = gsl_vector_get (x, 0);
    double lambda = gsl_vector_get (x, 1);
    for (int i = 0; i < n; i++) {
        /* Jacobian matrix J(i,j) = dfi / dxj, */
        /* where fi = (Yi - yi)/sigma[i],      */
        /*       Yi = A * exp(-i/lambda) + b  */
        /* and the xj are the parameters (A, lambda, b) */
        double t = X[i];
        double s = 1.0/sqrt(sigma[i]);
        double e = exp(-t/lambda);
        gsl_matrix_set (J, i, 0, e/s);
        gsl_matrix_set (J, i, 1, t * A * e/(s*lambda*lambda));
        gsl_matrix_set (J, i, 2, 1/s);
    }
    return GSL_SUCCESS;
}

int exp_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J) {
    exp_f (x, params, f);
    exp_df (x, params, J);
    return GSL_SUCCESS;
}

int gauss_f (const gsl_vector * x, void *params, gsl_vector * f) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double Y0 = gsl_vector_get (x, 0);
    double A = gsl_vector_get (x, 1);
    double C = gsl_vector_get (x, 2);
    double w = gsl_vector_get (x, 3);
    for (int i = 0; i < n; i++) {
        double diff = X[i]-C;
        double Yi = A*exp(-0.5*diff*diff/(w*w)) + Y0;
		double s = 1.0/sqrt(sigma[i]);
		gsl_vector_set (f, i, (Yi - Y[i])/s);
    }
    return GSL_SUCCESS;
}
double gauss_d (const gsl_vector * x, void *params) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double Y0 = gsl_vector_get (x, 0);
    double A = gsl_vector_get (x, 1);
    double C = gsl_vector_get (x, 2);
    double w = gsl_vector_get (x, 3);
    double val=0;
    for (int i = 0; i < n; i++) {
        double diff = X[i] - C;
        double s = 1.0/sqrt(sigma[i]);
        double dYi = ((A*exp(-0.5*diff*diff/(w*w)) + Y0) - Y[i])/s;
        val += dYi * dYi;
    }
    return val;
}
int gauss_df (const gsl_vector * x, void *params, gsl_matrix * J) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;
    double A = gsl_vector_get (x, 1);
    double C = gsl_vector_get (x, 2);
    double w = gsl_vector_get (x, 3);
    for (int i = 0; i < n; i++) {
        /* Jacobian matrix J(i,j) = dfi / dxj,	 */
        /* where fi = Yi - yi,					*/
        /* Yi = y=A*exp[-(Xi-xc)^2/(2*w*w)]+B		*/
        /* and the xj are the parameters (B,A,C,w) */
        //double s = sigma[i];
        double s = 1.0/sqrt(sigma[i]);
        double diff = X[i]-C;
        double e = exp(-0.5*diff*diff/(w*w))/s;
        gsl_matrix_set (J, i, 0, 1/s);
        gsl_matrix_set (J, i, 1, e);
        gsl_matrix_set (J, i, 2, diff*A*e/(w*w));
        gsl_matrix_set (J, i, 3, diff*diff*A*e/(w*w*w));
    }
    return GSL_SUCCESS;
}
int gauss_fdf (const gsl_vector * x, void *params,
               gsl_vector * f, gsl_matrix * J) {
    gauss_f (x, params, f);
    gauss_df (x, params, J);
    return GSL_SUCCESS;
}
int gauss_multi_peak_f (const gsl_vector * x, void *params, gsl_vector * f) {
    int n = ((struct FitData *)params)->n;
    int p = ((struct FitData *)params)->p;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    int peaks = (p-1)/3;
    double *a = new double[peaks];
    double *xc = new double[peaks];
    double *w2 = new double[peaks];
    double offset = gsl_vector_get (x, p-1);
    for (int i = 0; i < peaks; i++) {
        xc[i] = gsl_vector_get(x, 3*i+1);
        double wi = gsl_vector_get(x, 3*i+2);
        a[i] = sqrt(M_2_PI)*gsl_vector_get(x, 3*i)/wi;
        w2[i] = wi*wi;
    }
    for (int i = 0; i < n; i++) {
        double res = 0;
        for (int j = 0; j < peaks; j++) {
            double diff=X[i]-xc[j];
            res += a[j]*exp(-2*diff*diff/w2[j]);
        }
		double s = 1.0/sqrt(sigma[i]);
        gsl_vector_set(f, i, (res + offset - Y[i])/s);
    }
    delete[] a;
    delete[] xc;
    delete[] w2;
    return GSL_SUCCESS;
}
double gauss_multi_peak_d (const gsl_vector * x, void *params) {
    int n = ((struct FitData *)params)->n;
    int p = ((struct FitData *)params)->p;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    int peaks = (p-1)/3;
    double *a = new double[peaks];
    double *xc = new double[peaks];
    double *w2 = new double[peaks];
    double offset = gsl_vector_get (x, p-1);

    for (int i = 0; i < peaks; i++) {
        xc[i] = gsl_vector_get(x, 3*i+1);
        double wi = gsl_vector_get(x, 3*i+2);
        a[i] = sqrt(M_2_PI)*gsl_vector_get(x, 3*i)/wi;
        w2[i] = wi*wi;
    }
	double val=0;
    for (int i = 0; i < n; i++) {
        double res = 0;
        for (int j = 0; j < peaks; j++) {
            double diff=X[i]-xc[j];
            res+= a[j]*exp(-2*diff*diff/w2[j]);
        }
		double s = 1.0/sqrt(sigma[i]);
        double t = (res + offset - Y[i])/s;
        val += t*t;
    }
    delete[] a;
    delete[] xc;
    delete[] w2;
    return val;
}
int gauss_multi_peak_df (const gsl_vector * x, void *params, gsl_matrix * J) {
    int n = ((struct FitData *)params)->n;
    int p = ((struct FitData *)params)->p;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;
    int peaks = (p-1)/3;
    double *a = new double[peaks];
    double *xc = new double[peaks];
    double *w = new double[peaks];
    for (int i = 0; i<peaks; i++) {
        a[i] = gsl_vector_get (x, 3*i);
        xc[i] = gsl_vector_get (x, 3*i+1);
        w[i] = gsl_vector_get (x, 3*i+2);
    }
    for (int i = 0; i<n; i++) {
        double s = 1.0/sqrt(sigma[i]);
        for (int j = 0; j<peaks; j++) {
            double diff = X[i]-xc[j];
            double w2 = w[j]*w[j];
            double e = sqrt(M_2_PI)/s*exp(-2*diff*diff/w2);
            gsl_matrix_set (J, i, 3*j, e/w[j]);
            gsl_matrix_set (J, i, 3*j+1, 4*diff*a[j]*e/(w2*w[j]));
            gsl_matrix_set (J, i, 3*j+2, a[j]/w2*e*(4*diff*diff/w2 - 1));
        }
        gsl_matrix_set (J, i, p-1, 1.0/s);
    }
    delete[] a;
    delete[] xc;
    delete[] w;
    return GSL_SUCCESS;
}
int gauss_multi_peak_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J) {
    gauss_multi_peak_f (x, params, f);
    gauss_multi_peak_df (x, params, J);
    return GSL_SUCCESS;
}
int lorentz_multi_peak_f (const gsl_vector * x, void *params, gsl_vector * f) {
    int n = ((struct FitData *)params)->n;
    int p = ((struct FitData *)params)->p;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    int peaks = (p-1)/3;
    double *a = new double[peaks];
    double *xc = new double[peaks];
    double *w = new double[peaks];
    double offset = gsl_vector_get (x, p-1);
    for (int i = 0; i < peaks; i++) {
        a[i] = gsl_vector_get(x, 3*i);
        xc[i] = gsl_vector_get(x, 3*i+1);
        w[i] = gsl_vector_get(x, 3*i+2);
    }
    for (int i = 0; i < n; i++) {
        double res = 0;
        for (int j = 0; j < peaks; j++) {
            double diff=X[i]-xc[j];
            res += a[j]*w[j]/(4*diff*diff+w[j]*w[j]);
        }
		double s = 1.0/sqrt(sigma[i]);
        gsl_vector_set(f, i, (M_2_PI*res + offset - Y[i])/s);
    }
    delete[] a;
    delete[] xc;
    delete[] w;
    return GSL_SUCCESS;
}
double lorentz_multi_peak_d (const gsl_vector * x, void *params) {
    int n = ((struct FitData *)params)->n;
    int p = ((struct FitData *)params)->p;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    int peaks = (p-1)/3;
    double *a = new double[peaks];
    double *xc = new double[peaks];
    double *w = new double[peaks];
    double offset = gsl_vector_get (x, p-1);
    for (int i = 0; i < peaks; i++) {
        a[i] = gsl_vector_get(x, 3*i);
        xc[i] = gsl_vector_get(x, 3*i+1);
        w[i] = gsl_vector_get(x, 3*i+2);
    }

	double val = 0;
    for (int i = 0; i < n; i++) {
        double res = 0;
        for (int j = 0; j < peaks; j++) {
            double diff = X[i]-xc[j];
            res += a[j]*w[j]/(4*diff*diff+w[j]*w[j]);
        }
		double s = 1.0/sqrt(sigma[i]);
        double t = (M_2_PI*res + offset - Y[i])/s;
        val += t*t;
    }
    delete[] a;
    delete[] xc;
    delete[] w;
    return GSL_SUCCESS;
}
int lorentz_multi_peak_df (const gsl_vector * x, void *params, gsl_matrix * J) {
    int n = ((struct FitData *)params)->n;
    int p = ((struct FitData *)params)->p;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;
    int peaks = (p-1)/3;
    double *a = new double[peaks];
    double *xc = new double[peaks];
    double *w = new double[peaks];
    for (int i = 0; i<peaks; i++) {
        a[i] = gsl_vector_get (x, 3*i);
        xc[i] = gsl_vector_get (x, 3*i+1);
        w[i] = gsl_vector_get (x, 3*i+2);
    }
    for (int i = 0; i<n; i++) {
        double s = 1.0/sqrt(sigma[i]);
        for (int j = 0; j<peaks; j++) {
            double diff = X[i]-xc[j];
            double diff2 = diff*diff;
            double w2 = w[j]*w[j];
            double num = 1.0/(4*diff2+w2);
            double num2 = num*num;
            double den = 4*diff2-w2;
            gsl_matrix_set (J, i, 3*j, M_2_PI*w[j]*num/s);
            gsl_matrix_set (J, i, 3*j+1, M_2_PI*8*diff*a[j]*w[j]*num2/s);
            gsl_matrix_set (J, i, 3*j+2, M_2_PI*den*a[j]*num2/s);
        }
        gsl_matrix_set (J, i, p-1, 1.0/s);
    }
    delete[] a;
    delete[] xc;
    delete[] w;
    return GSL_SUCCESS;
}
int lorentz_multi_peak_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J) {
    lorentz_multi_peak_f (x, params, f);
    lorentz_multi_peak_df (x, params, J);
    return GSL_SUCCESS;
}

int user_f(const gsl_vector * x, void *params, gsl_vector * f) {
    int n = ((struct FitData *)params)->n;
    int p = ((struct FitData *)params)->p;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;

	NonLinearFit *fitter = (NonLinearFit *)((struct FitData *) params)->fitter;
    const char *function = fitter->formula().toLocal8Bit().constData();
	QStringList parNames = fitter->parameterNames();

	MyParser parser;
    try {
        double *parameters = new double[p];
        double xvar;
        parser.DefineVar("x", &xvar);
        for (int i = 0; i < p; i++) {
            parameters[i] = gsl_vector_get(x,i);
            parser.DefineVar(parNames[i].toLocal8Bit().constData(), &parameters[i]);
        }

		QMapIterator<QString, double> i(fitter->constants());
 		while (i.hasNext()){
     		i.next();
            parser.DefineConst(i.key().toLocal8Bit().constData(), i.value());
 		}

        parser.SetExpr(function);
        for (int j = 0; j < n; j++) {
            xvar = X[j];
			double s = 1.0/sqrt(sigma[j]);
			try {
			    gsl_vector_set (f, j, (parser.EvalRemoveSingularity(&xvar) - Y[j])/s);
			} catch (MyParser::Pole) {
			    return GSL_ESING;
			}
         }
        delete[] parameters;
    } catch (mu::ParserError &e) {
        QMessageBox::critical(0, "QtiSAS - Input function error", QString::fromStdString(e.GetMsg()));
        return GSL_EINVAL;
    }
    return GSL_SUCCESS;
}

double user_d(const gsl_vector * x, void *params) {
    int n = ((struct FitData *)params)->n;
    int p = ((struct FitData *)params)->p;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;

	NonLinearFit *fitter = (NonLinearFit *)((struct FitData *) params)->fitter;
    const char *function = fitter->formula().toLocal8Bit().constData();
	QStringList parNames = fitter->parameterNames();

    double val=0;
    MyParser parser;
    try {
        double *parameters = new double[p];
        double xvar;
        parser.DefineVar("x", &xvar);
        for (int i=0; i < p; i++) {
            parameters[i]=gsl_vector_get(x,i);
            parser.DefineVar(parNames[i].toLocal8Bit().constData(), &parameters[i]);
        }

		QMapIterator<QString, double> i(fitter->constants());
 		while (i.hasNext()){
     		i.next();
            parser.DefineConst(i.key().toLocal8Bit().constData(), i.value());
 		}

        parser.SetExpr(function);
        for (int j = 0; j < n; j++) {
            xvar = X[j];
			double s = 1.0/sqrt(sigma[j]);
            try {
				double t = (parser.EvalRemoveSingularity(&xvar) - Y[j])/s;
				val += t*t;
			} catch (MyParser::Pole) {
				return GSL_POSINF; //weird, I know. blame gsl.
			}
        }
        delete[] parameters;
    } catch (mu::ParserError &e) {
        QMessageBox::critical(0,"QtiSAS - Input function error",QString::fromStdString(e.GetMsg()));
        return GSL_EINVAL;
    }
    return val;
}

int user_df(const gsl_vector *x, void *params, gsl_matrix *J) {
    int n = ((struct FitData *)params)->n;
    int p = ((struct FitData *)params)->p;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;

	NonLinearFit *fitter = (NonLinearFit *)((struct FitData *) params)->fitter;
    const char *function = fitter->formula().toLocal8Bit().constData();
	QStringList parNames = fitter->parameterNames();

	try {
        double *param = new double[p];
        MyParser parser;
        double xvar;
        parser.DefineVar("x", &xvar);
        for (int k=0; k<p; k++) {
            param[k] = gsl_vector_get(x,k);
            parser.DefineVar(parNames[k].toLocal8Bit().constData(), &param[k]);
        }

		QMapIterator<QString, double> i(fitter->constants());
 		while (i.hasNext()){
     		i.next();
            parser.DefineConst(i.key().toLocal8Bit().constData(), i.value());
 		}

        parser.SetExpr(function);
        for (int i = 0; i < n; i++) {
            xvar = X[i];
			double s = 1.0/sqrt(sigma[i]);
            for (int j = 0; j < p; j++)
	        try {
				gsl_matrix_set (J, i, j, 1.0/s*parser.DiffRemoveSingularity(&xvar, &param[j], param[j]));
			} catch (MyParser::Pole) {
				return GSL_ESING;
			}
        }
        delete[] param;
    } catch (mu::ParserError &) {
        return GSL_EINVAL;
    }
    return GSL_SUCCESS;
}

int user_fdf(const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J) {
    int status = user_f (x, params, f);
    if (status)
    	return status;
    status = user_df (x, params, J);
    return status;
}

int boltzmann_f (const gsl_vector * x, void *params, gsl_vector * f) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double A1 = gsl_vector_get (x, 0);
    double A2 = gsl_vector_get (x, 1);
    double x0 = gsl_vector_get (x, 2);
    double dx = gsl_vector_get (x, 3);
    for (int i = 0; i < n; i++) {
        double Yi = (A1-A2)/(1 + exp((X[i] - x0)/dx)) + A2;
		double s = 1.0/sqrt(sigma[i]);
        gsl_vector_set (f, i, (Yi - Y[i])/s);
    }
    return GSL_SUCCESS;
}
double boltzmann_d (const gsl_vector * x, void *params) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double A1 = gsl_vector_get (x, 0);
    double A2 = gsl_vector_get (x, 1);
    double x0 = gsl_vector_get (x, 2);
    double dx = gsl_vector_get (x, 3);
    double val=0;
    for (int i = 0; i < n; i++) {
		double s = 1.0/sqrt(sigma[i]);
        double dYi = ((A1 - A2)/(1 + exp((X[i] - x0)/dx)) + A2 - Y[i])/s;
        val += dYi * dYi;
    }
    return val;
}
int boltzmann_df (const gsl_vector * x, void *params, gsl_matrix * J) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;
    double A1 = gsl_vector_get (x, 0);
    double A2 = gsl_vector_get (x, 1);
    double x0 = gsl_vector_get (x, 2);
    double dx = gsl_vector_get (x, 3);
    for (int i = 0; i < n; i++) {
        /* Jacobian matrix J(i,j) = dfi / dxj,		*/
        /* where fi = Yi - yi,						*/
        /* Yi = (A1-A2)/(1+exp((X[i]-x0)/dx)) + A2	*/
        /* and the xj are the parameters (A1,A2,x0,dx)*/
        double s = 1.0/sqrt(sigma[i]);
        double diff = X[i]-x0;
        double e = exp(diff/dx);
        double r = 1/(1+e);
        double aux = (A1 - A2)*e*r*r/(dx*s);
        gsl_matrix_set (J, i, 0, r/s);
        gsl_matrix_set (J, i, 1, (1-r)/s);
        gsl_matrix_set (J, i, 2, aux);
        gsl_matrix_set (J, i, 3, aux*diff/dx);
    }
    return GSL_SUCCESS;
}
int boltzmann_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J) {
    boltzmann_f (x, params, f);
    boltzmann_df (x, params, J);
    return GSL_SUCCESS;
}

int logistic_f (const gsl_vector * x, void *params, gsl_vector * f) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;

    double A1 = gsl_vector_get (x, 0);
    double A2 = gsl_vector_get (x, 1);
    double x0 = gsl_vector_get (x, 2);
    double p = gsl_vector_get (x, 3);
    for (int i = 0; i < n; i++) {
        double Yi = (A1 - A2)/(1 + pow(X[i]/x0, p)) + A2;
		double s = 1.0/sqrt(sigma[i]);
        gsl_vector_set (f, i, (Yi - Y[i])/s);
    }

    return GSL_SUCCESS;
}

double logistic_d (const gsl_vector * x, void *params) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;

    double A1 = gsl_vector_get (x, 0);
    double A2 = gsl_vector_get (x, 1);
    double x0 = gsl_vector_get (x, 2);
    double p = gsl_vector_get (x, 3);
    double val=0;
    for (int i = 0; i < n; i++) {
		double s = 1.0/sqrt(sigma[i]);
        double dYi = ((A1 - A2)/(1 + pow(X[i]/x0, p)) + A2 - Y[i])/s;
        val += dYi * dYi;
    }
    return val;
}

int logistic_df (const gsl_vector * x, void *params, gsl_matrix * J) {
    int n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;

    double A1 = gsl_vector_get (x, 0);
    double A2 = gsl_vector_get (x, 1);
    double x0 = gsl_vector_get (x, 2);
    double p = gsl_vector_get (x, 3);
    for (int i = 0; i < n; i++) {
        /* Jacobian matrix J(i,j) = dfi / dxj,		*/
        /* where fi = Yi - yi,						*/
        /* Yi = (A1-A2)/(1+(X[i]/x0)^p)) + A2	*/
        /* and the xj are the parameters (A1,A2,x0,p)*/
        double s = 1.0/sqrt(sigma[i]);
        double rap = X[i]/x0;
        double r = 1/(1 + pow(rap, p));
        double aux = (A1-A2)*r*r*pow(rap, p);
        gsl_matrix_set (J, i, 0, r/s);
        gsl_matrix_set (J, i, 1, (1-r)/s);
        gsl_matrix_set (J, i, 2, aux*p/(x0*s));
        gsl_matrix_set (J, i, 3, -aux*log(rap)/s);
    }
    return GSL_SUCCESS;
}

int logistic_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J) {
    logistic_f (x, params, f);
    logistic_df (x, params, J);
    return GSL_SUCCESS;
}
