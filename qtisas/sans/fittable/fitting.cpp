/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Data structures and fitting functions
 ******************************************************************************/

#include "fitting.h"

//+++ inversion
int inversion(int n, const gsl_matrix *m, gsl_matrix *inverse)
{
    // Create a copy since LU decomposition modifies the matrix
    gsl_matrix *m_copy = gsl_matrix_alloc(n, n);
    gsl_matrix_memcpy(m_copy, m);

    gsl_permutation *perm = gsl_permutation_alloc(n);

    // Perform LU decomposition
    int s;
    int status = 0;
    status = gsl_linalg_LU_decomp(m_copy, perm, &s);
    if (status != 0)
    {
        gsl_permutation_free(perm);
        gsl_matrix_free(m_copy);
        return status;
    }

    // Compute the inverse
    status = gsl_linalg_LU_invert(m_copy, perm, inverse);

    gsl_permutation_free(perm);
    gsl_matrix_free(m_copy);

    return status;
}

// +++ poly-Function Uniform
double polyFunctionUniform(double P, void *poly1)
{
    auto *ps = (struct poly1_SANS *)poly1;

    gsl_vector *para = ((functionT *)ps->function->params)->para;

    const double P0 = gsl_vector_get(para, ps->polyNumber);

    gsl_vector_set(para, ps->polyNumber, P);
    const double F_val = GSL_FN_EVAL(ps->function, ps->Q);
    gsl_vector_set(para, ps->polyNumber, P0);

    return F_val;
}

//+++ poly-Function SZ   
double polyFunctionSZ(double P, void *poly1) 
{
    auto *ps = (const struct poly1_SANS *)poly1;

    gsl_vector *para = ((functionT *)ps->function->params)->para;

    const int p = (int)para->size;
    const double P0 = gsl_vector_get(para, ps->polyNumber);
    const double sigma = gsl_vector_get(para, p - 2);

    if (P0 <= 0.0 || P <= 0.0 || sigma == 0.0)
        return 0.0;

    const double F_val = polyFunctionUniform(P, poly1);

    const double k = 1.0 / (sigma * sigma);
    const double t = k * P / P0;

    const double PD = k / P0 * exp((k - 1) * log(t) - t - gsl_sf_lngamma(k));

    return PD * F_val;
}

//+++ poly-Function Gamma 
double polyFunctionGamma(double P, void *poly1)
{
    auto *ps = (struct poly1_SANS *)poly1;

    gsl_vector *para = ((functionT *)ps->function->params)->para;

    const int p = (int)para->size;
    const double P0 = gsl_vector_get(para, ps->polyNumber);
    const double sigma = gsl_vector_get(para, p - 2);

    if (P == 0 || sigma == 0 || P0 == 0)
        return 0;

    const double F_val = polyFunctionUniform(P, poly1);

    const double theta = sigma * sigma * P0;
    const double k = P0 / theta;
    const double t = P / theta;

    const double PD = 1.0 / theta * exp((k - 1) * log(t) - P / theta - gsl_sf_lngamma(k));

    return PD * F_val;
}

//+++ poly-Function Log-Normal
double polyFunctionLogNormal(double P, void *poly1)
{
    auto *ps = (struct poly1_SANS *)poly1;

    gsl_vector *para = ((functionT *)ps->function->params)->para;

    const int p = (int)para->size;
    const double P0 = gsl_vector_get(para, ps->polyNumber);
    const double sigma = gsl_vector_get(para, p - 2);

    if (P0 <= 0.0 || P <= 0.0 || sigma <= 0.0)
        return 0.0;

    const double C = sqrt(2.0 * M_PI) * sigma * P;

    const double F_val = polyFunctionUniform(P, poly1);

    const double PD = exp(-log(P / P0) * log(P / P0) / 2.0 / sigma / sigma) / C;

    return PD * F_val;
}

//+++ poly-Function Triangular
double polyFunctionTriangular(double P, void *poly1)
{
    auto *ps = (struct poly1_SANS *)poly1;

    gsl_vector *para = ((functionT *)ps->function->params)->para;

    const int p = (int)para->size;
    const double P0 = gsl_vector_get(para, ps->polyNumber);
    const double A = gsl_vector_get(para, p - 2);
    const double B = gsl_vector_get(para, p - 1);

    const double F_val = polyFunctionUniform(P, poly1);

    double PD;

    if (P < P0)
        PD = 2 * (P - A) / (B - A) / (P0 - A);
    else
        PD = 2 * (B - P) / (B - A) / (B - P0);

    return PD * F_val;
}

//+++ poly-Function-Gauss
double polyFunctionGauss(double P, void *poly1)
{
    auto *ps = (struct poly1_SANS *)poly1;

    gsl_vector *para = ((functionT *)ps->function->params)->para;

    const int p = (int)para->size;
    const double P0 = gsl_vector_get(para, ps->polyNumber);
    const double sigma = gsl_vector_get(para, p - 2) * P0;

    const double F_val = polyFunctionUniform(P, poly1);

    const double PD = exp(-0.5 * (P - P0) * (P - P0) / (sigma * sigma));

    return PD * F_val;
}

//+++ Poly-Integral
double polyIntegral(double Q, void *poly2)
{
    auto *ps = (struct poly2_SANS *)poly2;

    gsl_function *F = ps->function;
    gsl_vector *F_para = ((functionT *)F->params)->para;
    integralControl *ic = ps->polyIntegralControl;

    const double absErrPoly = ic->absErr;
    const double relErrPoly = ic->relErr;
    const int intWorkspasePoly = ic->intWorkspase;
    const int numberSigmaPoly = ic->numberSigma;
    const int n_function = ic->n_function;

    const double P = gsl_vector_get(F_para, ps->polyNumber);

    const int p = (int)F_para->size;
	
    poly1_SANS poly1 = {ps->polyNumber, Q, F};

    gsl_function FIpoly;
    FIpoly.params = &poly1;

    const double polySigma = gsl_vector_get(F_para, p - 2);

    gsl_vector *x;
    gsl_vector *w;

    double i0 = 0.0;
    double i = 0.0;
    int nodes = 15;
    if (absErrPoly == 0.0 && relErrPoly == 0.0)
        nodes = intWorkspasePoly;
	
	//+++ Integration Range
    double Pmin = 0.0;
    if (polySigma * numberSigmaPoly < 1.0)
        Pmin = P * (1.0 - polySigma * numberSigmaPoly);
    double Pmax = P * (1 + polySigma * numberSigmaPoly);

    // +++
	switch (n_function)
	{
	case 0:		
        //+++ Gauss
        FIpoly.function = &polyFunctionGauss;

        //+++ Integral: Gauss-Legendre-Quadrature
        do
        {
            i0 = i;
            i = 0.0;

            x = gsl_vector_alloc(nodes);
            w = gsl_vector_alloc(nodes);

            GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);

            for (int ii = 0; ii < nodes; ii++)
                i += gsl_vector_get(w, ii) * GSL_FN_EVAL(&FIpoly, gsl_vector_get(x, ii));

            nodes *= 2;

            gsl_vector_free(x);
            gsl_vector_free(w);
        } while (fabs(i - i0) > absErrPoly && fabs(i - i0) > relErrPoly * fabs(i0) && nodes <= intWorkspasePoly);

        i /= (M_SQRT2 * M_SQRTPI) * (P * polySigma) * (1 - 0.5 * gsl_sf_erfc(1.0 / polySigma / M_SQRT2));

        break;

	case 1:
        //+++ Schultz-Zimm
        FIpoly.function = &polyFunctionSZ;

        //+++ Integral: Gauss-Legendre-Quadrature
        do
        {
            i0 = i;
            i = 0.0;

            x = gsl_vector_alloc(nodes);
            w = gsl_vector_alloc(nodes);

            GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);
            //
            for (int ii = 0; ii < nodes; ii++)
                i += gsl_vector_get(w, ii) * GSL_FN_EVAL(&FIpoly, gsl_vector_get(x, ii));

            nodes *= 2;

            gsl_vector_free(x);
            gsl_vector_free(w);
        } while (fabs(i - i0) > absErrPoly && fabs(i - i0) > relErrPoly * fabs(i0) && nodes <= intWorkspasePoly);

        break;

	case 2:
        //+++ Gamma
        FIpoly.function = &polyFunctionGamma;

        //+++ Integral: Gauss-Legendre-Quadrature
        do
        {
            i0 = i;
            i = 0;

            x = gsl_vector_alloc(nodes);
            w = gsl_vector_alloc(nodes);

            GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);

            for (int ii = 0; ii < nodes; ii++)
                i += gsl_vector_get(w, ii) * GSL_FN_EVAL(&FIpoly, gsl_vector_get(x, ii));

            nodes *= 2;

            gsl_vector_free(x);
            gsl_vector_free(w);
        } while (fabs(i - i0) > absErrPoly && fabs(i - i0) > relErrPoly * fabs(i0) && nodes <= intWorkspasePoly);

        break;

	case 3:  
        //+++ Log-Normal
        FIpoly.function = &polyFunctionLogNormal;

        //+++ Integral: Gauss-Legendre-Quadrature
        do
        {
            i0 = i;
            i = 0.0;

            x = gsl_vector_alloc(nodes);
            w = gsl_vector_alloc(nodes);

            GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);

            for (int ii = 0; ii < nodes; ii++)
                i += gsl_vector_get(w, ii) * GSL_FN_EVAL(&FIpoly, gsl_vector_get(x, ii));

            nodes *= 2;

            gsl_vector_free(x);
            gsl_vector_free(w);
        } while (fabs(i - i0) > absErrPoly && fabs(i - i0) > relErrPoly * fabs(i0) && nodes <= intWorkspasePoly);

        break;

	case 4: 
        //+++ Uniform
        FIpoly.function = &polyFunctionUniform;

        //+++ Integration Range
        Pmin = 0.0;
        if (polySigma < 1.0)
            Pmin = P * (1.0 - polySigma);
        Pmax = P * (1 + polySigma);

        //+++ Integral: Gauss-Legendre-Quadrature
        do
        {
            i0 = i;
            i = 0;

            x = gsl_vector_alloc(nodes);
            w = gsl_vector_alloc(nodes);

            GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);

            for (int ii = 0; ii < nodes; ii++)
                i += gsl_vector_get(w, ii) * GSL_FN_EVAL(&FIpoly, gsl_vector_get(x, ii));

            nodes *= 2;

            gsl_vector_free(x);
            gsl_vector_free(w);
        } while (fabs(i - i0) > absErrPoly && fabs(i - i0) > relErrPoly * fabs(i0) && nodes <= intWorkspasePoly);

        i /= (Pmax - Pmin);

        break;

	case 5:
        //+++ Triangular
        FIpoly.function = &polyFunctionTriangular;

        Pmin = gsl_vector_get(F_para, p - 2);
        Pmax = gsl_vector_get(F_para, p - 1);

        if (Pmin >= Pmax || (P > Pmin && P > Pmax) || (P < Pmin && P < Pmax))
            i = GSL_FN_EVAL(F, Q);
        else
        {
            //+++ Integral: Gauss-Legendre-Quadrature
            do
            {
                i0 = i;
                i = 0;

                x = gsl_vector_alloc(nodes);
                w = gsl_vector_alloc(nodes);

                GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);

                for (int ii = 0; ii < nodes; ii++)
                    i += gsl_vector_get(w, ii) * GSL_FN_EVAL(&FIpoly, gsl_vector_get(x, ii));

                nodes *= 2;

                gsl_vector_free(x);
                gsl_vector_free(w);
            } while (fabs(i - i0) > absErrPoly && fabs(i - i0) > relErrPoly * fabs(i0) && nodes <= intWorkspasePoly);
        }

        break;
    default:
        break;
}
	return i;
}

//+++ reso-poly-Function
double resoPolyFunction(double Q, void *polyReso0)
{
    auto *prs = (struct polyReso0_SANS *)polyReso0;
    auto *poly2 = prs->poly2;

    const double resoSigma = prs->resoSigma;
    const double Q0 = prs->Q0;

    const double result = exp(-0.5 * (Q - Q0) * (Q - Q0) / (resoSigma * resoSigma)) * polyIntegral(Q, poly2);

    return result;
}

//+++ Reso-Poly-Integral
double resoPolyFunctionNew(double Q0, void *polyReso1)
{
    auto *prs = (struct polyReso1_SANS *)polyReso1;
    auto *poly2 = prs->poly2;
    auto *resoIntegralControl = prs->resoIntegralControl;

    const double resoSigma = prs->resoSigma;

    gsl_function polyFunctionLocal;
    polyFunctionLocal.function = &polyIntegral;
    polyFunctionLocal.params = poly2;

    resoSANS resoSANS_poly = {resoSigma, Q0, &polyFunctionLocal, resoIntegralControl};

    return resoIntegral(Q0, &resoSANS_poly);
}

//+++ Reso-Poly-Integral
double resoPolyIntegral(double Q0, void *polyReso1)
{
    auto *prs = (struct polyReso1_SANS *)polyReso1;
    auto *poly2 = prs->poly2;
    auto *resoIntegralControl = prs->resoIntegralControl;

    const double resoSigma = prs->resoSigma;

    const double absErrReso = resoIntegralControl->absErr;
    const double relErrReso = resoIntegralControl->relErr;
    const int intWorkspaseReso = resoIntegralControl->intWorkspase;
    const int numberSigmaReso = resoIntegralControl->numberSigma;

    polyReso0_SANS polyReso0 = {poly2, resoSigma, Q0};

    gsl_function FresoPoly;
    FresoPoly.function = &resoPolyFunction;
    FresoPoly.params = &polyReso0;

    double result, error;

    gsl_integration_workspace *w = gsl_integration_workspace_alloc(intWorkspaseReso);
    double Q0min = 0.0;
    if ((Q0 - (numberSigmaReso * resoSigma)) > 0)
        Q0min = Q0 - (numberSigmaReso * resoSigma);

    gsl_integration_qags(&FresoPoly, Q0min, Q0 + (numberSigmaReso * resoSigma), absErrReso, relErrReso,
                         intWorkspaseReso, w, &result, &error);
    gsl_integration_workspace_free(w);

    result /= M_SQRT2 * M_SQRTPI * resoSigma * (1 - 0.5 * gsl_sf_erfc(Q0 / resoSigma / M_SQRT2));
    return result;
}

//+++ Reso-Function:: Gauss
double ResoFunction(double Q, void *paraM)
{
    const double resoSigma = ((struct resoSANS *)paraM)->resoSigma;
    const double Q0 = ((struct resoSANS *)paraM)->Q0;
    gsl_function *F = ((struct resoSANS *)paraM)->function;

    return exp(-0.5 * (Q - Q0) * (Q - Q0) / (resoSigma * resoSigma)) * GSL_FN_EVAL(F, Q);
}

//+++ Reso-Function:: Triangular
double ResoFunctionTriangle(double Q, void *paraM)
{
    const double resoSigma = ((struct resoSANS *)paraM)->resoSigma;
    const double Q0 = ((struct resoSANS *)paraM)->Q0;
    gsl_function *F = ((struct resoSANS *)paraM)->function;

    double result = 0.0;

    if (Q >= (Q0 - 2 * resoSigma) && Q <= Q0)
        result = (Q - Q0 + 2 * resoSigma) / (resoSigma * resoSigma) / 4.0 * GSL_FN_EVAL(F, Q);
    if (Q <= (Q0 + 2 * resoSigma) && Q > Q0)
        result = -(Q - Q0 - 2 * resoSigma) / (resoSigma * resoSigma) / 4.0 * GSL_FN_EVAL(F, Q);

    return result;
}

//+++ Reso-Function:: Bessel I0
double ResoFunctionBessel(double Q, void *paraM)
{
    const double resoSigma = ((struct resoSANS *)paraM)->resoSigma;
    const double Q0 = ((struct resoSANS *)paraM)->Q0;
    gsl_function *F = ((struct resoSANS *)paraM)->function;

    if (resoSigma < 0 || Q < 0 || Q0 <= 0)
        return 0;
    
    double result = Q / (resoSigma * resoSigma) * exp(-0.5 * (Q * Q + Q0 * Q0) / (resoSigma * resoSigma));
    result *= gsl_sf_bessel_I0(Q * Q0 / (resoSigma * resoSigma));

    return result * GSL_FN_EVAL(F, Q);
}

//+++ Reso-Integral
double resoIntegral(double Q, void *paraM)
{
    auto *resoIntegralControl = ((struct resoSANS *)paraM)->resoIntegralControl;

    double absErr = resoIntegralControl->absErr;
    double relErr = resoIntegralControl->relErr;
    int intWorkspase = resoIntegralControl->intWorkspase;
    int numberSigma = resoIntegralControl->numberSigma;
    int n_function = resoIntegralControl->n_function;

    const double resoSigma = ((struct resoSANS *)paraM)->resoSigma;

    if (resoSigma <= 0 || n_function == 10)
    {
        gsl_function *function = ((struct resoSANS *)paraM)->function;
        return GSL_FN_EVAL(function, Q);
    }

    int nodes = 10;
    if (absErr == 0 && relErr == 0)
        nodes = intWorkspase;

    double Qmin, Qmax, i0, i;
    gsl_vector *x, *w;

    gsl_function Freso;

    switch (n_function)
    {
        case 0:
        //+++ Gauss
        Freso.function = &ResoFunction;
        break;

        case 1:
        //+++ Triangular
        Freso.function = &ResoFunctionTriangle;
        numberSigma = 2;
        break;

        case 2:
        //+++ Bessel
        Freso.function = &ResoFunctionBessel;
        break;

        case 3:
        //+++ Gauss
        Freso.function = &ResoFunction;
        break;

    default:
        break;
    }

    Freso.params = (void *)paraM;

    Qmin = Q - numberSigma * resoSigma;
    if (Qmin < 0 && n_function < 3 && n_function != 1)
        Qmin = 0;

    Qmax = Q + numberSigma * resoSigma;

    i = 0.0;

    // Integral: Gauss-Legendre-Quadrature
    double ww, xx, ff;
    do
    {
        i0 = i;
        i = 0;

        x = gsl_vector_alloc(nodes);
        w = gsl_vector_alloc(nodes);

        GaussLegendreQuadrature(nodes, Qmin, Qmax, x, w);

        for (int ii = 0; ii < nodes; ii++)
        {
            ww = gsl_vector_get(w, ii);
            xx = gsl_vector_get(x, ii);
            ff = GSL_FN_EVAL(&Freso, xx);
            i += ww * ff;
        }
        nodes++;

        gsl_vector_free(x);
        gsl_vector_free(w);
    } while (fabs(i - i0) > absErr && fabs(i - i0) > relErr * fabs(i0) && nodes < intWorkspase);

    switch (n_function)
    {
    case 0:
        //+++ Gauss
        i /= M_SQRT2 * M_SQRTPI * resoSigma * (1 - 0.5 * gsl_sf_erfc(Q / resoSigma / M_SQRT2));
        break;
    case 3:
        //+++ Gauss
        i /= M_SQRT2 * M_SQRTPI * resoSigma;
        break;
    default:
        break;
    }
    return i;
}

//+++ function_fmPoly
int function_fmPoly(const gsl_vector *x, void *params, gsl_vector *f)
{
    double bayesianTerm;
    return function_fmPoly_bayesian(x, params, f, bayesianTerm);
}

int function_fmPoly_bayesian(const gsl_vector *x, void *params, gsl_vector *f, double &bayesianTerm)
{
    double *fdata = f->data;
    size_t fstride = f->stride;

    auto *SPOLYparams = static_cast<fitDataSANSpoly *>(params);

    sizetNumbers *sizetNumbers = SPOLYparams->SizetNumbers;
    sansData *sansData = SPOLYparams->SansData;
    int *controlM = SPOLYparams->controlM;
    gsl_vector *para = SPOLYparams->para;
    gsl_vector_int *paraF = SPOLYparams->paraF;
    gsl_function *F = SPOLYparams->function;
    integralControl *resoIntegralControl = SPOLYparams->resoIntegralControl;
    int *polyNumber = SPOLYparams->polyNumber;
    integralControl *polyIntegralControl = SPOLYparams->polyIntegralControl;
    gsl_vector *limitLeft = SPOLYparams->limitLeft;
    gsl_vector *limitRight = SPOLYparams->limitRight;
    gsl_vector_int *limitBayesian = SPOLYparams->bayesian;

    size_t M = sizetNumbers->M;
    size_t N = sizetNumbers->N;
    size_t p = sizetNumbers->p;

    double *Q = sansData->Q;
    double *I = sansData->I;
    double *Weight = sansData->Weight;
    double *sigmaReso = sansData->sigmaReso;

    auto *Fparams = static_cast<functionT *>(F->params);
    gsl_vector *paraP = Fparams->para;
    size_t pPoly = paraP->size;

    size_t pFit = 0; // number adjustable parameters
    size_t ii = 0;

    bayesianTerm = 0.0;

    for (size_t pp = 0; pp < p; ++pp)
    {
        for (size_t mm = 0; mm < M; ++mm, ++ii)
        {
            const int flag = gsl_vector_int_get(paraF, ii);

            if (flag == 0)
            { // fittable
                double xx = gsl_vector_get(x, pFit);
                double left = gsl_vector_get(limitLeft, pFit);
                double right = gsl_vector_get(limitRight, pFit);
                bool bayesian = bool(gsl_vector_int_get(limitBayesian, pFit));
                if (!bayesian)
                    xx = std::clamp(xx, left, right);
                else if (right > 0)
                    bayesianTerm += (xx - left) * (xx - left) / right / right;

                gsl_vector_set(para, ii, xx);
                ++pFit;
            }
            else if (flag == 2)
            { // global
                gsl_vector_set(para, ii, gsl_vector_get(para, M * pp));
            }
        }
    }

    if (bayesianTerm > 0.0 && N > 0)
        bayesianTerm /= static_cast<double>(N);

    size_t iMstart = 0, iMfinish = 0;
    double Ii;

    for (int mm = 0; mm < M; mm++)
    {
        if (mm == 0)
        {
            iMstart = 0;
            iMfinish = controlM[0];
        }
        else
        {
            iMstart = iMfinish;
            iMfinish += controlM[mm];
        }

        Fparams->currentFirstPoint = static_cast<int>(iMstart);
        Fparams->currentLastPoint = static_cast<int>(iMfinish - 1);
        Fparams->currentPoint = static_cast<int>(iMstart);

        for (size_t pp = 0; pp < p; pp++)
            gsl_vector_set(paraP, pp, gsl_vector_get(para, M * pp + mm));

        double polySigma;
        if (resoIntegralControl->n_function > 5)
            polySigma = -1.0;
        else if (resoIntegralControl->n_function == 5)
            polySigma = gsl_vector_get(paraP, pPoly - 1) + gsl_vector_get(paraP, pPoly - 2);
        else
            polySigma = gsl_vector_get(paraP, pPoly - 2);
        
        Fparams->polyYN = (polyNumber[mm] >= 0 && polySigma > 0.0);

        //____before integrals
        Fparams->beforeIter = true;
        Fparams->currentPoint = static_cast<int>(iMstart);
        Fparams->currentInt = -1;
        GSL_FN_EVAL(F, Q[iMstart]);

        if (polyNumber[mm] >= 0 && polySigma > 0.0)
        {
            poly2_SANS poly2 = {F, polyNumber[mm], polyIntegralControl};

            // integral1
            Fparams->currentInt = 1;
            Fparams->Int1 = polyIntegral(Q[iMstart], &poly2);
            // integral2
            Fparams->currentInt = 2;
            Fparams->Int2 = polyIntegral(Q[iMstart], &poly2);
            // integral3
            Fparams->currentInt = 3;
            Fparams->Int3 = polyIntegral(Q[iMstart], &poly2);
        }
        else
        {
            // integral1
            Fparams->currentInt = 1;
            Fparams->Int1 = GSL_FN_EVAL(F, Q[iMstart]);
            // integral2
            Fparams->currentInt = 2;
            Fparams->Int2 = GSL_FN_EVAL(F, Q[iMstart]);
            // integral3
            Fparams->currentInt = 3;
            Fparams->Int3 = GSL_FN_EVAL(F, Q[iMstart]);
        }

        //___ after integrals
        Fparams->currentInt = 0;
        GSL_FN_EVAL(F, Q[iMstart]);

        Fparams->beforeIter = false;

        //+++ update global parameters
        if (M > 1 && mm == 0)
        {
            for (size_t pp = 0; pp < p; pp++)
            {
                if (gsl_vector_int_get(paraF, pp * M + 1) == 2)
                    for (size_t mmm = 1; mmm < M; mmm++)
                        gsl_vector_set(para, pp * M + mmm, gsl_vector_get(paraP, pp));
            }
        }

        //+++
        for (size_t im = iMstart; im < iMfinish; im++)
        {
            Fparams->currentPoint = static_cast<int>(im);
            if (sigmaReso[im] > 0) // reso Yes
            {
                if (polyNumber[mm] < 0 || polySigma <= 0.0)
                {
                    resoSANS paraReso = {sigmaReso[im], Q[im], F, resoIntegralControl};
                    Ii = resoIntegral(Q[im], &paraReso);
                }
                else
                {
                    poly2_SANS poly2 = {F, polyNumber[mm], polyIntegralControl};
                    polyReso1_SANS polyReso1 = {&poly2, sigmaReso[im], resoIntegralControl};
                    Ii = resoPolyFunctionNew(Q[im], &polyReso1);
                }
            }
            else
            {
                if (polyNumber[mm] < 0 || polySigma <= 0.0)
                {
                    Ii = GSL_FN_EVAL(F, Q[im]);
                }
                else
                {
                    poly2_SANS poly2 = {F, polyNumber[mm], polyIntegralControl};
                    Ii = polyIntegral(Q[im], &poly2);
                }
            }

            Ii = (Ii - I[im]) / Weight[im];

            if (bayesianTerm > 0)
                Ii = std::copysign(1.0, Ii) * sqrt(Ii * Ii + bayesianTerm);

            fdata[im * fstride] = Ii;
        }

        Fparams->afterIter = true;
        Fparams->currentPoint = static_cast<int>(iMfinish - 1);
        GSL_FN_EVAL(F, Q[iMfinish - 1]);
        Fparams->afterIter = false;
        Fparams->polyYN = false;
    }

    bayesianTerm *= static_cast<double>(N);

    return GSL_SUCCESS;
}

//+++  function_simplyFit_derivative
double function_sasFit_derivative(double x, void *params)
{
    auto *SPOLYparams = static_cast<sasFitDerivative *>(params);

    double Q = SPOLYparams->Q;
    size_t currentPoint = SPOLYparams->currentPoint;
    size_t indexX = SPOLYparams->indexX;
    size_t indexXfunction = SPOLYparams->indexXfunction;
    size_t mm = SPOLYparams->mData;
    bool localFitOrGlobal = SPOLYparams->localFitOrGlobal;

    fitDataSANSpoly *sansPoly = SPOLYparams->sansPoly;
    gsl_vector *para = sansPoly->para;
    int *polyNumber = sansPoly->polyNumber;
    integralControl *resoIntegralControl = sansPoly->resoIntegralControl;
    integralControl *polyIntegralControl = sansPoly->polyIntegralControl;

    gsl_function *F = sansPoly->function;
    gsl_vector *paraP = ((functionT *)F->params)->para;
    size_t p = paraP->size;

    sansData *sansData = sansPoly->SansData;
    double *sigmaReso = sansData->sigmaReso;

    double oldX, oldXglobal;
    if (localFitOrGlobal)
    {
        oldX = gsl_vector_get(paraP, indexXfunction);
        gsl_vector_set(paraP, indexXfunction, x);

        oldXglobal = gsl_vector_get(para, indexX);
        gsl_vector_set(para, indexX, x);
    }

    double polySigma;
    if (resoIntegralControl->n_function > 5)
        polySigma = -1.0;
    else if (resoIntegralControl->n_function == 5)
        polySigma = gsl_vector_get(paraP, p - 1) + gsl_vector_get(paraP, p - 2);
    else
        polySigma = gsl_vector_get(paraP, p - 2);

    double res = 0;

    if (sigmaReso[currentPoint] > 0) // reso Yes
    {
        if (polyNumber[mm] < 0 || polySigma <= 0.0)
        {
            resoSANS paraReso = {sigmaReso[currentPoint], Q, F, resoIntegralControl};
            res = resoIntegral(Q, &paraReso);
        }
        else
        {
            poly2_SANS poly2 = {F, polyNumber[mm], polyIntegralControl};
            polyReso1_SANS polyReso1 = {&poly2, sigmaReso[currentPoint], resoIntegralControl};
            res = resoPolyFunctionNew(Q, &polyReso1);
        }
    }
    else
    {
        if (polyNumber[mm] < 0 || polySigma <= 0.0)
        {
            res = GSL_FN_EVAL(F, Q);
        }
        else
        {
            poly2_SANS poly2 = {F, polyNumber[mm], polyIntegralControl};
            res = polyIntegral(Q, &poly2);
        }
    }

    if (localFitOrGlobal)
    {
        gsl_vector_set(paraP, indexXfunction, oldX);
        gsl_vector_set(para, indexX, oldXglobal);
    }

    return res;
}

//+++ function_dfmPoly
int function_dfmPoly(const gsl_vector *x, void *params, gsl_matrix *J)
{
    auto *SPOLYparams = static_cast<fitDataSANSpoly *>(params);

    sizetNumbers *sizetNumbers = SPOLYparams->SizetNumbers;

    size_t N = sizetNumbers->N;
    size_t M = sizetNumbers->M;
    size_t p = sizetNumbers->p;
    size_t np = sizetNumbers->np;
    double STEP = sizetNumbers->STEP;
    double STEPlocal = STEP;

    sansData *sansData = SPOLYparams->SansData;

    double *Q = sansData->Q;
    double *I = sansData->I;
    double *Weight = sansData->Weight;
    double *sigmaReso = sansData->sigmaReso;

    gsl_function *F = SPOLYparams->function;

    auto *Fparams = static_cast<functionT *>(F->params);
    gsl_vector *paraP = Fparams->para;

    int *controlM = SPOLYparams->controlM;

    gsl_vector *para = SPOLYparams->para;
    gsl_vector_int *paraF = SPOLYparams->paraF;

    sasFitDerivative sasFitDerivativeL = {SPOLYparams, 0.0, 0, 0, 0, 0, true};
    gsl_function FD;
    FD.function = function_sasFit_derivative;
    FD.params = &sasFitDerivativeL;

    gsl_vector *limitLeft = SPOLYparams->limitLeft;
    gsl_vector *limitRight = SPOLYparams->limitRight;
    gsl_vector_int *limitBayesian = SPOLYparams->bayesian;

    std::vector<int> npCurrent(np);
    std::vector<int> mCurrent(np);

    size_t pFit = 0;
    size_t npnp = 0;

    double bayesianTerm = 0.0;

    for (size_t ppp = 0; ppp < p; ++ppp)
    {
        for (size_t mm = 0; mm < M; ++mm, ++npnp)
        {
            const int flag = gsl_vector_int_get(paraF, npnp);

            if (flag == 0)
            { // fittable
                double xx = gsl_vector_get(x, pFit);
                double left = gsl_vector_get(limitLeft, pFit);
                double right = gsl_vector_get(limitRight, pFit);
                bool bayesian = bool(gsl_vector_int_get(limitBayesian, pFit));
                if (!bayesian)
                    xx = std::clamp(xx, left, right);
                else if (right > 0)
                    bayesianTerm += (xx - left) * (xx - left) / right / right;

                gsl_vector_set(para, npnp, xx);

                npCurrent[pFit] = static_cast<int>(ppp);
                mCurrent[pFit] = static_cast<int>(mm);
                ++pFit;
            }
            else if (flag == 2)
            { // global
                gsl_vector_set(para, npnp, gsl_vector_get(para, M * ppp));
            }
        }
    }

    if (bayesianTerm > 0.0 && N > 0)
        bayesianTerm /= static_cast<double>(N);

    size_t iMstart = 0, iMfinish = 0;

    double FDerivative, FFunction, deriv, weight, resid, abserr;

    double pOld;

    for (size_t pp = 0; pp < pFit; pp++)
    {
        double xxx = gsl_vector_get(x, pp);
        double xxleft = gsl_vector_get(limitLeft, pp);
        double xxright = gsl_vector_get(limitRight, pp);
        bool bayesian = bool(gsl_vector_int_get(limitBayesian, pp));

        double bayesianDer = 0;
        if (!bayesian)
            xxx = std::clamp(xxx, xxleft, xxright);
        else if (xxright > 0.0 && N > 0)
            bayesianDer = 2 * (xxx - xxleft) / xxright / xxright / static_cast<double>(N);

        // Adaptive step size
        STEPlocal = (xxx == 0.0) ? STEP : STEP * std::fabs(xxx);
        double distLeft = xxx - xxleft;
        double distRight = xxright - xxx;

        for (int mm = 0; mm < M; mm++)
        {
            if (mm == 0)
            {
                iMstart = 0;
                iMfinish = controlM[0];
            }
            else
            {
                iMstart = iMfinish;
                iMfinish += controlM[mm];
            }

            // +++ move parameters to function
            for (int ppp = 0; ppp < p; ppp++)
                gsl_vector_set(paraP, ppp, gsl_vector_get(para, M * ppp + mm));

            pOld = gsl_vector_get(paraP, npCurrent[pp]);

            bool localFitOrGlobal = mCurrent[pp] == mm || gsl_vector_int_get(paraF, M * npCurrent[pp] + mm) == 2;

            sasFitDerivativeL.Q = Q[iMstart];
            sasFitDerivativeL.currentPoint = iMstart;
            sasFitDerivativeL.indexX = pp;
            sasFitDerivativeL.indexXfunction = npCurrent[pp];
            sasFitDerivativeL.mData = mm;
            sasFitDerivativeL.localFitOrGlobal = localFitOrGlobal;
            // +++ call function before
            Fparams->beforeIter = true;
            Fparams->currentPoint = static_cast<int>(iMstart);
            Fparams->currentInt = -1;
            GSL_FN_EVAL(F, Q[iMstart]);

            //+++ calculate integrals
            // integral1
            Fparams->currentInt = 1;
            Fparams->Int1 = GSL_FN_EVAL(&FD, pOld);
            // integral2
            Fparams->currentInt = 2;
            Fparams->Int2 = GSL_FN_EVAL(&FD, pOld);
            // integral3
            Fparams->currentInt = 3;
            Fparams->Int3 = GSL_FN_EVAL(&FD, pOld);

            Fparams->currentInt = 0;
            GSL_FN_EVAL(F, Q[iMstart]);
            Fparams->beforeIter = false;

            //+++ update global parameters
            if (M > 1 && mm == 0)
            {
                for (int ppp = 0; ppp < p; ppp++)
                {
                    if (gsl_vector_int_get(paraF, ppp * M + 1) == 2)
                        for (int mmm = 1; mmm < M; mmm++)
                            gsl_vector_set(para, ppp * M + mmm, gsl_vector_get(paraP, ppp));
                }
            }

            for (size_t current = iMstart; current < iMfinish; current++)
            {
                //+++ derivative function filling
                Fparams->currentPoint = static_cast<int>(current);

                sasFitDerivativeL.Q = Q[current];
                sasFitDerivativeL.currentPoint = current;

                // Derivative: dF/dx
                if (bayesianTerm > 0)
                    gsl_deriv_central(&FD, xxx, STEPlocal, &FDerivative, &abserr);
                else
                {
                    if ((xxx - xxleft) < STEPlocal)
                        gsl_deriv_forward(&FD, xxx, STEPlocal, &FDerivative, &abserr);
                    else if ((xxright - xxx) < STEPlocal)
                        gsl_deriv_backward(&FD, xxx, STEPlocal, &FDerivative, &abserr);
                    else
                        gsl_deriv_central(&FD, xxx, STEPlocal, &FDerivative, &abserr);
                }

                weight = (Weight[current] != 0.0) ? Weight[current] : 1.0;
                if (bayesianTerm > 0)
                {
                    // Function:F
                    FFunction = GSL_FN_EVAL(&FD, xxx);
                    double resid = (FFunction - I[current]) / weight;
                    deriv = std::copysign(1.0, resid) / sqrt(resid * resid + bayesianTerm) *
                            (resid * FDerivative / weight + 0.5 * bayesianDer);
                }
                else if (!localFitOrGlobal)
                    deriv = 0;
                else
                {
                    // Weighted derivative
                    deriv = FDerivative / weight;
                }

                gsl_matrix_set(J, current, pp, deriv);
            }

            Fparams->afterIter = true;
            Fparams->currentPoint = static_cast<int>(iMfinish - 1);

            GSL_FN_EVAL(F, Q[iMfinish - 1]);
            Fparams->afterIter = false;
        }
    }

    return GSL_SUCCESS;
}

//+++ function_fdfmPoly
int function_fdfmPoly(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J)
{
    function_fmPoly(x, params, f);
    function_dfmPoly(x, params, J);
    return GSL_SUCCESS;
}

//+++ function_dmPoly
double function_dmPoly(const gsl_vector *x, void *params)
{
    double bayesianTerm;
    return function_dmPoly_bayesian(x, params, bayesianTerm);
}

double function_dmPoly_bayesian(const gsl_vector *x, void *params, double &bayesianTerm)
{
    auto *sizetNumbers = ((struct fitDataSANSpoly *)params)->SizetNumbers;
    size_t N = sizetNumbers->N;

    gsl_vector *fu = gsl_vector_alloc(N);

    function_fmPoly_bayesian(x, params, fu, bayesianTerm);

    double Ii = 0.0;
    for (size_t i = 0; i < N; i++)
        Ii += gsl_vector_get(fu, i) * gsl_vector_get(fu, i);

    gsl_vector_free(fu);

    return Ii;
}

double function_r2Poly(const gsl_vector *x, void *params)
{
    auto *Sparams = (fitDataSANSpoly *)params;
    auto *sizetNumbers = Sparams->SizetNumbers;
    size_t N = sizetNumbers->N;
    if (N == 0)
        return 0.0;

    gsl_vector *fu = gsl_vector_alloc(N);
    function_fmPoly(x, params, fu);

    auto *sansData = Sparams->SansData;

    // weighted mean, w_i = 1/sigma_i^2
    double mean_y = 0.0, sum_w = 0.0;
    for (size_t i = 0; i < N; ++i)
    {
        double wi = 1.0 / (sansData->Weight[i] * sansData->Weight[i]);
        mean_y += wi * sansData->I[i];
        sum_w += wi;
    }
    mean_y /= sum_w;

    double ss_res = 0.0, ss_tot = 0.0;
    for (size_t i = 0; i < N; ++i)
    {
        double res = gsl_vector_get(fu, i); // = (f_i - y_i) / sigma_i
        ss_res += res * res;                // = (f_i - y_i)² / sigma_i²

        double wi = 1.0 / (sansData->Weight[i] * sansData->Weight[i]);
        double dev = sansData->I[i] - mean_y;
        ss_tot += wi * dev * dev; // = (y_i - y_w)² / sigma_i²
    }

    gsl_vector_free(fu);

    if (ss_tot == 0.0)
        return 1.0;

    return 1.0 - (ss_res / ss_tot);
}

/***
 ***
 *** Global Fit :: no instrumental options
 ***
 *** last update : ... 2025-11-05 ...
 ***
 ***/

//+++function_fdfm: Combined function that computes both residuals and Jacobian
int function_fdfm(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J)
{
    function_fm(x, params, f);
    function_dfm(x, params, J);
    return GSL_SUCCESS;
}

//+++ function_dm: Computes the sum of squares of residuals for the nonlinear fit
double function_dm(const gsl_vector *x, void *params)
{
    double bayesianTerm;
    return function_dm_bayesian(x, params, bayesianTerm);
}

double function_dm_bayesian(const gsl_vector *x, void *params, double &bayesianTerm)
{
    auto *Sparams = (simplyFitP *)params;
    size_t N = Sparams->N;

    gsl_vector *fu = gsl_vector_alloc(N);

    function_fm_bayesian(x, Sparams, fu, bayesianTerm);

    double sumSq = 0.0;
    for (size_t i = 0; i < N; ++i)
    {
        double val = gsl_vector_get(fu, i);
        sumSq += val * val;
    }

    gsl_vector_free(fu);
    return sumSq;
}

//+++ Computes R² (coefficient of determination) for the nonlinear fit
double function_r2(const gsl_vector *x, void *params)
{
    auto *Sparams = (simplyFitP *)params;
    size_t N = Sparams->N;
    if (N == 0)
        return 0.0;

    gsl_vector *fu = gsl_vector_alloc(N);
    function_fm(x, Sparams, fu);

    // w_i = 1/sigma_i^2
    double mean_y = 0.0, sum_w = 0.0;
    for (size_t i = 0; i < N; ++i)
    {
        double wi = 1.0 / (Sparams->Weight[i] * Sparams->Weight[i]);
        mean_y += wi * Sparams->I[i];
        sum_w += wi;
    }
    mean_y /= sum_w;

    double ss_res = 0.0, ss_tot = 0.0;
    for (size_t i = 0; i < N; ++i)
    {
        double res = gsl_vector_get(fu, i); // = (f_i - y_i) / sigma_i
        ss_res += res * res;                // = (f_i - y_i)² / sigma_i²

        double wi = 1.0 / (Sparams->Weight[i] * Sparams->Weight[i]);
        double dev = Sparams->I[i] - mean_y;
        ss_tot += wi * dev * dev; // = (y_i - y_w)² / sigma_i²
    }

    gsl_vector_free(fu);

    if (ss_tot == 0.0)
        return 1.0;

    return 1.0 - (ss_res / ss_tot);
}

//+++ function_fm
int function_fm(const gsl_vector *x, void *params, gsl_vector *f)
{
    double bayesianTerm;
    return function_fm_bayesian(x, params, f, bayesianTerm);
}

int function_fm_bayesian(const gsl_vector *x, void *params, gsl_vector *f, double &bayesianTerm)
{
    auto *Sparams = static_cast<simplyFitP *>(params);
    const size_t N = Sparams->N;
    const size_t M = Sparams->M;
    const size_t p = Sparams->p;
    const size_t np = Sparams->np;

    double *Q = Sparams->Q;
    double *I = Sparams->I;
    double *Weight = Sparams->Weight;
    int *controlM = Sparams->controlM;

    gsl_vector *para = Sparams->para;
    gsl_vector_int *paraF = Sparams->paraF;
    gsl_function *F = Sparams->function;

    auto *Fparams = static_cast<functionT *>(F->params);
    const int prec = Fparams->prec;
    gsl_vector *paraP = Fparams->para;

    gsl_vector *limitLeft = Sparams->limitLeft;
    gsl_vector *limitRight = Sparams->limitRight;
    gsl_vector_int *limitBayesian = Sparams->bayesian;

    std::vector<int> npCurrent(np);
    std::vector<int> mCurrent(np);

    size_t ii = 0;
    size_t pFit = 0;
    bayesianTerm = 0.0;

    for (size_t pp = 0; pp < p; ++pp)
    {
        for (size_t mm = 0; mm < M; ++mm, ++ii)
        {
            const int flag = gsl_vector_int_get(paraF, ii);

            if (flag == 0)
            { // fittable
                double xx = gsl_vector_get(x, pFit);
                double left = gsl_vector_get(limitLeft, pFit);
                double right = gsl_vector_get(limitRight, pFit);
                bool bayesian = bool(gsl_vector_int_get(limitBayesian, pFit));
                if (!bayesian)
                    xx = std::clamp(xx, left, right);
                else if (right > 0)
                    bayesianTerm += (xx - left) * (xx - left) / right / right;

                gsl_vector_set(para, ii, xx);
                npCurrent[pFit] = static_cast<int>(pp);
                mCurrent[pFit] = static_cast<int>(mm);
                ++pFit;
            }
            else if (flag == 2)
            { // global
                gsl_vector_set(para, ii, gsl_vector_get(para, M * pp));
            }
        }
    }

    if (bayesianTerm > 0.0 && N > 0)
        bayesianTerm /= static_cast<double>(N);

    //--- Dataset loop
    size_t iMstart = 0;
    size_t iMfinish = 0;

    for (size_t mm = 0; mm < M; ++mm)
    {
        // Determine index range
        if (mm == 0)
        {
            iMstart = 0;
            iMfinish = controlM[0];
        }
        else
        {
            iMstart = iMfinish;
            iMfinish += controlM[mm];
        }

        // Update function parameters for this dataset
        Fparams->currentFirstPoint = static_cast<int>(iMstart);
        Fparams->currentLastPoint = static_cast<int>(iMfinish) - 1;
        Fparams->currentPoint = static_cast<int>(iMstart);

        // Copy current dataset parameters
        for (size_t pp = 0; pp < p; ++pp)
            gsl_vector_set(paraP, pp, gsl_vector_get(para, M * pp + mm));

        //--- Pre-evaluation setup
        Fparams->beforeIter = true;
        Fparams->currentInt = -1;
        GSL_FN_EVAL(F, Q[iMstart]);

        // Compute pre-integrals
        for (int k = 1; k <= 3; ++k)
        {
            Fparams->currentInt = k;
            double val = GSL_FN_EVAL(F, Q[iMstart]);
            if (k == 1)
                Fparams->Int1 = val;
            else if (k == 2)
                Fparams->Int2 = val;
            else if (k == 3)
                Fparams->Int3 = val;
        }

        Fparams->currentInt = 0;
        GSL_FN_EVAL(F, Q[iMstart]);
        Fparams->beforeIter = false;

        //--- Global parameter synchronization (only once)
        if (M > 1 && mm == 0)
        {
            for (size_t pp = 0; pp < p; ++pp)
            {
                if (gsl_vector_int_get(paraF, pp * M + 1) == 2)
                {
                    double val = gsl_vector_get(paraP, pp);
                    for (size_t mmm = 1; mmm < M; ++mmm)
                        gsl_vector_set(para, pp * M + mmm, val);
                }
            }
        }

        //--- Compute residuals for dataset mm
        if (bayesianTerm <= 0)
        {
            for (size_t im = iMstart; im < iMfinish; ++im)
            {
                Fparams->currentPoint = static_cast<int>(im);
                double Ii = GSL_FN_EVAL(F, Q[im]);
                double resid = (Ii - I[im]) / Weight[im];
                gsl_vector_set(f, im, resid);
            }
        }
        else
        {
            for (size_t im = iMstart; im < iMfinish; ++im)
            {
                Fparams->currentPoint = static_cast<int>(im);
                double Ii = GSL_FN_EVAL(F, Q[im]);
                double resid = (Ii - I[im]) / Weight[im];
                double residBayesian = std::copysign(1.0, resid) * sqrt(resid * resid + bayesianTerm);

                gsl_vector_set(f, im, residBayesian);
            }
        }


        //--- Post-iteration cleanup
        Fparams->afterIter = true;
        Fparams->currentPoint = static_cast<int>(iMfinish) - 1;
        GSL_FN_EVAL(F, Q[iMfinish - 1]);
        Fparams->afterIter = false;
    }

    bayesianTerm *= static_cast<double>(N);

    return GSL_SUCCESS;
}

//+++  function_simplyFit_derivative
double function_simplyFit_derivative(double x, void *params)
{
    auto *Sparams = (simplyFitDerivative *)params;
    size_t indexX = Sparams->indexX;
    double Q = Sparams->Q;
    gsl_function *F = Sparams->function;
    auto *Fparams = (functionT *)F->params;
    gsl_vector *para = Fparams->para;

    // Save old value
    double oldX = gsl_vector_get(para, indexX);

    // Temporarily set parameter
    gsl_vector_set(para, indexX, x);

    // Evaluate model
    double res = GSL_FN_EVAL(F, Q);

    // Restore original parameter
    gsl_vector_set(para, indexX, oldX);

    return res;
}

//+++ function_dfm
int function_dfm(const gsl_vector *x, void *params, gsl_matrix *J)
{
    //--- Unpack parameters
    auto *Sparams = static_cast<simplyFitP *>(params);
    gsl_function *F = Sparams->function;
    auto *Fparams = static_cast<functionT *>(F->params);

    gsl_vector *paraP = Fparams->para;
    const int prec = Fparams->prec;

    simplyFitDerivative simplyFitDerivativeL = {F, 0, 0};
    gsl_function FD;
    FD.function = function_simplyFit_derivative;
    FD.params = &simplyFitDerivativeL;

    //--- Basic setup
    const size_t N = Sparams->N;
    const size_t M = Sparams->M;
    const size_t p = Sparams->p;
    const size_t np = Sparams->np;

    int *controlM = Sparams->controlM;
    gsl_vector *para = Sparams->para;
    gsl_vector_int *paraF = Sparams->paraF;
    double *Q = Sparams->Q;
    double *I = Sparams->I;
    double *Weight = Sparams->Weight;
    double STEP = Sparams->STEP;
    gsl_vector *limitLeft = Sparams->limitLeft;
    gsl_vector *limitRight = Sparams->limitRight;
    gsl_vector_int *limitBayesian = Sparams->bayesian;

    //--- Local storage
    std::vector<int> npCurrent(np);
    std::vector<int> mCurrent(np);

    //--- Fill parameter vector with current fit parameters
    size_t ii = 0;
    size_t pFit = 0;
    double bayesianTerm = 0.0;

    for (size_t pp = 0; pp < p; ++pp)
    {
        for (size_t mm = 0; mm < M; ++mm, ++ii)
        {
            int flag = gsl_vector_int_get(paraF, ii);

            if (flag == 0)
            { // fittable
                double xx = gsl_vector_get(x, pFit);
                double left = gsl_vector_get(limitLeft, pFit);
                double right = gsl_vector_get(limitRight, pFit);
                bool bayesian = bool(gsl_vector_int_get(limitBayesian, pFit));
                if (!bayesian)
                    xx = std::clamp(xx, left, right);
                else if (right > 0)
                {
                    bayesianTerm += (xx - left) * (xx - left) / (right * right);
                }
                gsl_vector_set(para, ii, xx);
                npCurrent[pFit] = static_cast<int>(pp);
                mCurrent[pFit] = static_cast<int>(mm);
                ++pFit;
            }
            else if (flag == 2)
            { // global
                gsl_vector_set(para, ii, gsl_vector_get(para, M * pp));
            }
        }
    }

    if (bayesianTerm > 0.0 && N > 0)
        bayesianTerm /= static_cast<double>(N);

    //--- Derivative calculation setup
    double FDerivative = 0.0, FFunction = 0.0;
    double abserr = 0.0;
    double STEPlocal = STEP;

    //--- Fill Jacobian matrix J
    for (size_t jj = 0; jj < pFit; ++jj)
    {
        double xxx = gsl_vector_get(x, jj);
        double xxleft = gsl_vector_get(limitLeft, jj);
        double xxright = gsl_vector_get(limitRight, jj);
        bool bayesian = bool(gsl_vector_int_get(limitBayesian, jj));
        double bayesianDer = 0;
        if (!bayesian)
            xxx = std::clamp(xxx, xxleft, xxright);
        else if (xxright > 0.0 && N > 0)
            bayesianDer = 2 * (xxx - xxleft) / (xxright * xxright) / static_cast<double>(N);

        size_t current = 0;

        for (size_t mm = 0; mm < M; ++mm)
        {
            // Determine range for dataset mm
            size_t iMstart = 0;
            size_t iMfinish = 0;

            if (mm == 0)
            {
                iMstart = 0;
                iMfinish = controlM[0];
            }
            else
            {
                iMfinish += controlM[mm];
                iMstart = iMfinish - controlM[mm];
            }

            // Copy dataset-specific parameters
            for (size_t pp = 0; pp < p; ++pp)
                gsl_vector_set(paraP, pp, gsl_vector_get(para, M * pp + mm));

            //--- Pre-iteration setup
            Fparams->beforeIter = true;
            Fparams->currentPoint = static_cast<int>(current);
            Fparams->currentInt = -1;
            GSL_FN_EVAL(F, Q[current]);

            // Compute pre-integrals
            for (int k = 1; k <= 3; ++k)
            {
                Fparams->currentInt = k;
                double val = GSL_FN_EVAL(F, Q[current]);
                if (k == 1)
                    Fparams->Int1 = val;
                else if (k == 2)
                    Fparams->Int2 = val;
                else if (k == 3)
                    Fparams->Int3 = val;
            }

            Fparams->currentInt = 0;
            GSL_FN_EVAL(F, Q[current]);
            Fparams->beforeIter = false;

            //--- Global parameter synchronization
            if (M > 1 && mm == 0)
            {
                for (size_t pp = 0; pp < p; ++pp)
                {
                    if (gsl_vector_int_get(paraF, pp * M + 1) == 2)
                    {
                        double val = gsl_vector_get(paraP, pp);
                        for (size_t mmm = 1; mmm < M; ++mmm)
                            gsl_vector_set(para, pp * M + mmm, val);
                    }
                }
            }

            //--- Derivatives per data point
            for (size_t im = iMstart; im < iMfinish; ++im)
            {
                // If parameter not related to dataset -> derivative = 0
                if (mCurrent[jj] != static_cast<int>(mm) && gsl_vector_int_get(paraF, M * npCurrent[jj] + mm) != 2)
                {
                    gsl_matrix_set(J, current, jj, 0.0);
                    ++current;
                    continue;
                }

                // Fill derivative function
                Fparams->currentPoint = static_cast<int>(current);
                simplyFitDerivativeL.Q = Q[current];
                simplyFitDerivativeL.indexX = npCurrent[jj];

                // Adaptive step size
                STEPlocal = (xxx == 0.0) ? STEP : STEP * std::fabs(xxx);

                // Derivative: dF/dx
                if (bayesian)
                    gsl_deriv_central(&FD, xxx, STEPlocal, &FDerivative, &abserr);
                else
                {
                    if ((xxx - xxleft) < STEPlocal)
                        gsl_deriv_forward(&FD, xxx, STEPlocal, &FDerivative, &abserr);
                    else if ((xxright - xxx) < STEPlocal)
                        gsl_deriv_backward(&FD, xxx, STEPlocal, &FDerivative, &abserr);
                    else
                        gsl_deriv_central(&FD, xxx, STEPlocal, &FDerivative, &abserr);
                }

                double deriv;
                double weight = (Weight[current] != 0.0) ? Weight[current] : 1.0;
                if (bayesianTerm > 0)
                {
                    // Function:F
                    FFunction = GSL_FN_EVAL(F, Q[current]);
                    double resid = (FFunction - I[current]) / weight;
                    deriv = std::copysign(1.0, resid) / sqrt(resid * resid + bayesianTerm) *
                            (resid * FDerivative / weight + 0.5 * bayesianDer);
                }
                else
                {
                    // Weighted derivative
                    deriv = FDerivative / weight;
                }

                gsl_matrix_set(J, current, jj, deriv);
                ++current;
            }

            //--- Post-iteration cleanup
            Fparams->afterIter = true;
            Fparams->currentPoint = static_cast<int>(current - 1);
            GSL_FN_EVAL(F, Q[current - 1]);
            Fparams->afterIter = false;
        }
    }

    return GSL_SUCCESS;
}
