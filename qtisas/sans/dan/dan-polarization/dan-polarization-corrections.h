/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: polarized-neutron corrections (PN mode): x2, x4 and the two-file x421..x424 on GSL matrices
 ******************************************************************************/

#ifndef DAN_POLARIZATION_CORRECTIONS_H
#define DAN_POLARIZATION_CORRECTIONS_H

#include <gsl/gsl_matrix.h>

#include <array>
#include <memory>

namespace DanPolarization
{
double square(double x);
//+++ a = a / b on the pixels the mask keeps (0 where b is 0)
void divideWithMask(gsl_matrix *a, gsl_matrix *b, gsl_matrix *mask, int MD);
//+++ a = a + scale * b
void addScaled(gsl_matrix *a, gsl_matrix *b, double scale, int MD);

//+++ x2: the analyzer-out down/up transmissions -> the pure-spin ones (-/+), their relative sigmas along
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

//+++ x421..x424: two analyzed channels (states 3 up-up, 4 up-down, 5 down-down, 6 down-up)
using MatrixPtr = std::unique_ptr<gsl_matrix, decltype(&gsl_matrix_free)>;

struct TwoChannel
{
    int row = -1;
    int state = -1;
    MatrixPtr image{nullptr, gsl_matrix_free};
    MatrixPtr relativeVariance{nullptr, gsl_matrix_free};
    MatrixPtr mask{nullptr, gsl_matrix_free};
    double factor = 0.0;
    double transmission = 1.0;
    double relativeTransmissionError = 0.0;
    double thickness = 0.0;
    double analyzerTransmission = 0.0;
    double analyzerEfficiency = 0.0;
    double polarization = 0.0;
    double flipperEfficiency = 0.0;
};

struct TwoChannelPair
{
    MatrixPtr first{nullptr, gsl_matrix_free};
    MatrixPtr second{nullptr, gsl_matrix_free};
    MatrixPtr firstVariance{nullptr, gsl_matrix_free};
    MatrixPtr secondVariance{nullptr, gsl_matrix_free};
    MatrixPtr mask{nullptr, gsl_matrix_free};
    // over the pair's mask: the share of measured channel j in output k (sum of inverse[k][j] * value_j), for the
    // outputs' transmission errors
    std::array<std::array<double, 2>, 2> channelShares{};
};

bool invertTwoByTwo(const double matrix[2][2], double inverse[2][2]);
bool correctTwoChannelPair(const std::array<TwoChannel, 2> &channels, bool fixedAnalyzer, int matrixSize,
                           TwoChannelPair &result);
} // namespace DanPolarization

#endif // DAN_POLARIZATION_CORRECTIONS_H
