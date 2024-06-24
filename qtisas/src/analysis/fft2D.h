/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: FFT for matrices
 ******************************************************************************/

#ifndef FIT2D_H
#define FIT2D_H

void fft2d(double **xtre, double **xtim, int width, int height, bool shift = true);
void fft2d_inv(double **xtre, double **xtim, double **xrec_re, double **xrec_im, int width, int height, bool undoShift = true);

bool isPower2(int n);
int next2Power(int n);

#endif
