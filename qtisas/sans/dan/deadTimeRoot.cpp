/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Dead time correction, paralysable model N = n exp(-n tau)
 ******************************************************************************/

#include <cmath>

#include "deadTimeRoot.h"

double deadTimeRoot_f(double x, void *params)
{
    auto *p = (struct deadTimeRoot_params *)params;
    double I = p->I;
    double tau = p->tau;

    return x * exp(-tau * x) - I;
}

double deadTimeRoot_deriv(double x, void *params)
{
    auto *p = (struct deadTimeRoot_params *)params;
    double I = p->I;
    double tau = p->tau;

    return (1 - tau * x) * exp(-tau * x);
}

void deadTimeRoot_fdf(double x, void *params, double *y, double *dy)
{
    auto *p = (struct deadTimeRoot_params *)params;
    double I = p->I;
    double tau = p->tau;

    *y = x * exp(-tau * x) - I;
    *dy = (1 - tau * x) * exp(-tau * x);
}
