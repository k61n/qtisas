/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Dead time correction Non paralysable N=n exp(-nt)
 ******************************************************************************/

#ifndef DEADTIMEROOT_H
#define DEADTIMEROOT_H

struct deadTimeRoot_params
{
    double I, tau;
};

double deadTimeRoot_f (double x, void *params);
double deadTimeRoot_deriv (double x, void *params);
void deadTimeRoot_fdf (double x, void *params, double *y, double *dy);

#endif
