#include <cmath>
#include <cstdio>

#include <QDebug>
#include <QProgressDialog>

#include "genmin.h"
#include "get_options.h"

inline void checkLimitsLocal(int p, gsl_vector *x, MatrixG xleft, MatrixG xright)
{
    for (int i = 0; i < p; ++i)
    {
        const double xx = gsl_vector_get(x, i);
        const double xxleft = xleft[i];
        const double xxright = xright[i];
        
        if (xx < xxleft)
            gsl_vector_set(x, i, xxleft);
        else if (xx > xxright)
            gsl_vector_set(x, i, xxright);
    }
}

double GenMin::localSearchGSL(DataG &x, int prec, QProgressDialog *progress)
{
    // Levenberg: gradient, delta

    bool const_chi = levenberg_constchi;
    double STEP = levenberg_step;
    int d_max_iterations = levenberg_iters;
    double d_tolerance = levenberg_rel;
    double absError = levenberg_abs;    
    bool test = levenberg_unscaled;

    int dimension = problem->getDimension();
    int N = problem->fln.n;

    gsl_vector *xx = gsl_vector_alloc(dimension);

    for(int i = 0; i < dimension; i++)
        gsl_vector_set(xx, i, x[i]);

    const gsl_multifit_fdfsolver_type *Tln = gsl_multifit_fdfsolver_lmsder;

    gsl_multifit_fdfsolver *sln = gsl_multifit_fdfsolver_alloc(Tln, N, dimension);
    
    int status = gsl_multifit_fdfsolver_set(sln, &problem->fln, xx);
    status = GSL_CONTINUE;    

    MatrixG limitLeft, limitRight;
    problem->getLeftMargin(limitLeft);
    problem->getRightMargin(limitRight);

    double chi2 = std::numeric_limits<double>::infinity();
    double chi2prev = std::numeric_limits<double>::infinity();

    double ssizeDelta = 0;
    int countConstChi2 = 0;

    int iter = 0;
    QString st, stOld;

    if (progress)
        stOld = progress->labelText();

    do
    {
        status = gsl_multifit_fdfsolver_iterate(sln);
        if (status > 1 && !const_chi)
        {
            qDebug() << "stopped by GSL error:" << gsl_strerror(status);
            break;
        }

        checkLimitsLocal(dimension, sln->x, limitLeft, limitRight);

        if (problem->sansSupportYN())
            chi2 = function_dmPoly(sln->x, problem->fln.params);
        else
            chi2 = function_dm(sln->x, problem->fln.params);
        
        //+++ Delta Stop
        for (int i = 0; i < dimension; ++i)
        {
            const double dx = fabs(gsl_vector_get(sln->dx, i));
            const double x = fabs(gsl_vector_get(sln->x, i));
            const double v = fabs(dx - d_tolerance * x);
            if (v > ssizeDelta)
                ssizeDelta = v;
        }
        status = (iter > 1 && ssizeDelta < absError) ? GSL_SUCCESS : GSL_CONTINUE;

        iter++;

        st = QString("# local: levenberg %1[<%2]\tchi^2\t%3\t%4 [<%5]")
            .arg(iter - 1, 4)
            .arg(d_max_iterations, 4)
            .arg(chi2 / (N - dimension), 15, 'e', 10)
            .arg(ssizeDelta, 10, 'g', 5)
            .arg(absError, 10, 'g', 5);

        if (progress)
        {
            if (progress->wasCanceled())
                break;
            progress->setValue(progress->value() - 1);
            progress->setLabelText(stOld + "\n" + st);
            progress->setValue(progress->value() + 1);
            progress->update();
        }

        st += "\t[";

        for (int i = 0; i < GSL_MIN(dimension, 8); ++i)
            st += QString("%1  ").arg(gsl_vector_get(sln->x, i), 10, 'g', 5);

        st += (dimension > 8 ? "...]\n" : "]\n");

        printf("%s", st.toLatin1().constData());


        if (iter > 2 && chi2 == 0.0)
            break;

        if (const_chi)
            countConstChi2 = (chi2prev <= chi2) ? countConstChi2 + 1 : 0;
        else if (iter > 2 && chi2prev < chi2)
            break;

        chi2prev = chi2;

        if (status == GSL_SUCCESS)
            break;

        if (iter > 1 && chi2prev == 0.0)
            break;

        if (const_chi && countConstChi2 > 0)
        {
            if (countConstChi2 == 10)
                break;

            double factor = 1.0;

            if (countConstChi2 == 2 || countConstChi2 == 3)
                factor = 10.0;
            else if (countConstChi2 == 6 || countConstChi2 == 7)
                factor = 1000.0;

            if (problem->sansSupportYN())
            {
                struct sizetNumbers *sn = ((struct fitDataSANSpoly *)problem->fln.params)->SizetNumbers;
                sn->STEP = STEP * factor;
            }
            else
                ((struct simplyFitP *)problem->fln.params)->STEP = STEP * factor;

            gsl_multifit_fdfsolver_set(sln, &problem->fln, sln->x);
            status = GSL_CONTINUE;
        }
    }
    while (status == GSL_CONTINUE  && iter < d_max_iterations);

    if (progress && !progress->wasCanceled())
        progress->setLabelText(stOld);

    for (int i = 0; i < dimension; i++)
        x[i] = gsl_vector_get(sln->x, i);

    gsl_multifit_fdfsolver_free(sln);
    gsl_vector_free(xx);

    return chi2;
}