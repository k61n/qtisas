/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: FFT for matrices
 ******************************************************************************/

#ifndef FOURIER_H
#define FOURIER_H

void fft2d(double **xtre, double **xtim, int width, int height, bool shift = true);
void fft2d_inv(double **xtre, double **xtim, double **xrec_re, double **xrec_im, int width, int height, bool undoShift = true);

bool isPower2(int n);
int next2Power(int n);
#endif
