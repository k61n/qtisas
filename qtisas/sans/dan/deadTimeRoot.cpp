/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Dead time correction Non paralysable N=n exp(-nt)
 ******************************************************************************/

#include "deadTimeRoot.h"
#include <math.h>

double
deadTimeRoot_f (double x, void *params)
{
  struct deadTimeRoot_params *p
    = (struct deadTimeRoot_params *) params;
  double I = p->I;
  double tau = p->tau;

  return x*exp(-tau*x) - I;
}

double
deadTimeRoot_deriv (double x, void *params)
{
  struct deadTimeRoot_params *p
    = (struct deadTimeRoot_params *) params;
  double I = p->I;
  double tau = p->tau;
 
  return (1-tau*x)*exp(-tau*x);
}

void
deadTimeRoot_fdf (double x, void *params,
               double *y, double *dy)
{
  struct deadTimeRoot_params *p
    = (struct deadTimeRoot_params *) params;
  double I = p->I;
  double tau = p->tau;

  *y = (1-tau*x)*exp(-tau*x);
  *dy = (1-tau*x)*exp(-tau*x);
}
