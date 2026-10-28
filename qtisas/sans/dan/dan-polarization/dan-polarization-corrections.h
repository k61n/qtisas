/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: polarized-neutron corrections (PN mode): x2 and x4 on GSL matrices
 ******************************************************************************/

#ifndef DAN_POLARIZATION_CORRECTIONS_H
#define DAN_POLARIZATION_CORRECTIONS_H

#include <gsl/gsl_matrix.h>

namespace DanPolarization
{
double square(double x);
//+++ a = a / b on the pixels the mask keeps (0 where b is 0)
void divideWithMask(gsl_matrix *a, gsl_matrix *b, gsl_matrix *mask, int MD);
//+++ a = a + scale * b
void addScaled(gsl_matrix *a, gsl_matrix *b, double scale, int MD);

//+++ x2: the analyzer-out down/up transmissions -> the pure-spin ones (-/+), errors along
bool transmissionCorrectionPNx2(double &TrDown, double &TrDownSigma, double &TrUp, double &TrUpSigma, double P,
                                double Pf, bool infoTerminalYN = false);
//+++ x2: SampleDown -> I-; SampleUp -> I+
bool matrixCorrectionPNx2(gsl_matrix *Sdown, gsl_matrix *SdownSigma, gsl_matrix *Sup, gsl_matrix *SupSigma,
                          gsl_matrix *mask, double TrMinus, double TrMinusSigma, double TrPlus, double TrPlusSigma,
                          double P, double Pf, int MD);
//+++ x4: the four analyzed channels -> the four cross sections
bool matrixCorrectionPNx4(gsl_matrix *Sdd, gsl_matrix *SddSigma, gsl_matrix *Sdu, gsl_matrix *SduSigma, gsl_matrix *Suu,
                          gsl_matrix *SuuSigma, gsl_matrix *Sud, gsl_matrix *SudSigma, gsl_matrix *mask, double TrMinus,
                          double TrMinusSigma, double TrPlus, double TrPlusSigma, double A1, double A2, double A3,
                          double A4, double P, double f, double MD);
} // namespace DanPolarization

#endif // DAN_POLARIZATION_CORRECTIONS_H
