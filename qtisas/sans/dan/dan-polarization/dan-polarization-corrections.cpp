/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: polarized-neutron corrections (PN mode): x2, x4 and the two-file x421..x424 on GSL matrices
 ******************************************************************************/

#include "dan-polarization-corrections.h"

#include <cmath>
#include <iostream>

namespace DanPolarization
{
double square(double x)
{
    return std::pow(std::fabs(x), 2);
}
// TrDown -> Tr-; TrUp -> Tr+
bool transmissionCorrectionPNx2(double &TrDown, double &TrDownSigma, double &TrUp, double &TrUpSigma, double P,
                                double Pf, bool infoTerminalYN)
{
    if (P == 0 || Pf == -1)
        return false;

    double TrMinus = (P * Pf + 1) / (P * (Pf + 1)) * TrDown + (P - 1) / (P * (Pf + 1)) * TrUp;
    double TrPlus = (P * Pf - 1) / (P * (Pf + 1)) * TrDown + (P + 1) / (P * (Pf + 1)) * TrUp;

    // the sigmas are relative (sigma / Tr), in and out: the linear combination is propagated in absolute sigmas
    const double downSigma = TrDownSigma * TrDown;
    const double upSigma = TrUpSigma * TrUp;
    double TrMinusSigma = sqrt(square((P * Pf + 1) * downSigma) + square((P - 1) * upSigma)) / std::fabs(P * (Pf + 1));
    double TrPlusSigma = sqrt(square((P * Pf - 1) * downSigma) + square((P + 1) * upSigma)) / std::fabs(P * (Pf + 1));
    TrMinusSigma = TrMinus != 0 ? TrMinusSigma / std::fabs(TrMinus) : 0.0;
    TrPlusSigma = TrPlus != 0 ? TrPlusSigma / std::fabs(TrPlus) : 0.0;

    if (infoTerminalYN)
    {
        std::cout << " TrDown: " << TrDown << " TrMinus: " << TrMinus << "\n";
        std::cout << " TrDownSigma: " << TrDownSigma << " TrMinusSigma: " << TrMinusSigma << "\n";
        std::cout << " TrUp: " << TrUp << " TrPlus: " << TrPlus << "\n";
        std::cout << " TrUpSigma: " << TrUpSigma << " TrPlusSigma: " << TrPlusSigma << "\n";
    }

    TrDown = TrMinus;
    TrUp = TrPlus;
    TrDownSigma = TrMinusSigma;
    TrUpSigma = TrPlusSigma;

    return true;
}
//+++ SampleDown -> I-; SampleUp -> I+
bool matrixCorrectionPNx2(gsl_matrix *Sdown, gsl_matrix *SdownSigma, gsl_matrix *Sup, gsl_matrix *SupSigma,
                          gsl_matrix *mask, double TrMinus, double TrMinusSigma, double TrPlus, double TrPlusSigma,
                          double P, double Pf, int MD)
{
    if (P == 0 || Pf == -1 || TrPlus == 0 || TrMinus == 0)
        return false;

    double minus, plus, down, up;
    double minusSigma2, plusSigma2, downSigma2, upSigma2;

    for (int ix = 0; ix < MD; ix++)
        for (int iy = 0; iy < MD; iy++)
            if (gsl_matrix_get(mask, iy, ix) != 0)
            {
                down = gsl_matrix_get(Sdown, iy, ix);
                up = gsl_matrix_get(Sup, iy, ix);

                minus = (P * Pf + 1) / (P * (Pf + 1)) * down / TrMinus + (P - 1) / (P * (Pf + 1)) * up / TrPlus;
                plus = (P * Pf - 1) / (P * (Pf + 1)) * down / TrMinus + (P + 1) / (P * (Pf + 1)) * up / TrPlus;

                gsl_matrix_set(Sdown, iy, ix, minus);
                gsl_matrix_set(Sup, iy, ix, plus);

                downSigma2 = gsl_matrix_get(SdownSigma, iy, ix);
                upSigma2 = gsl_matrix_get(SupSigma, iy, ix);

                minusSigma2 = square((P * Pf + 1) * down / TrMinus) * (TrMinusSigma * TrMinusSigma + downSigma2);
                minusSigma2 += square((P - 1) * up / TrPlus) * (TrPlusSigma * TrPlusSigma + upSigma2);
                minusSigma2 /= square(P * (Pf + 1));

                plusSigma2 = square((P * Pf - 1) * down / TrMinus) * (TrMinusSigma * TrMinusSigma + downSigma2);
                plusSigma2 += square((P + 1) * up / TrPlus) * (TrPlusSigma * TrPlusSigma + upSigma2);
                plusSigma2 /= square(P * (Pf + 1));

                gsl_matrix_set(SdownSigma, iy, ix, minusSigma2);
                gsl_matrix_set(SupSigma, iy, ix, plusSigma2);
            }
    return true;
}
bool matrixCorrectionPNx4(gsl_matrix *Sdd, gsl_matrix *SddSigma, gsl_matrix *Sdu, gsl_matrix *SduSigma, gsl_matrix *Suu,
                          gsl_matrix *SuuSigma, gsl_matrix *Sud, gsl_matrix *SudSigma, gsl_matrix *mask, double TrMinus,
                          double TrMinusSigma, double TrPlus, double TrPlusSigma, double A1, double A2, double A3,
                          double A4, double P, double f, double MD)
{
    double Pf = 2 * f - 1.0;
    double P0 = (1 + P) / 2;
    double fxP = f * P;

    if (fxP == 0 || TrPlus == 0 || TrMinus == 0 || (A1 + A3) == 0 || (A2 + A4) == 0)
        return false;

    double dd, du, uu, ud;
    double mm, mp, pp, pm;
    double ddSigma, duSigma, uuSigma, udSigma;
    double mmSigma, mpSigma, ppSigma, pmSigma;

    for (int ix = 0; ix < MD; ix++)
        for (int iy = 0; iy < MD; iy++)
            if (gsl_matrix_get(mask, iy, ix) != 0)
            {
                //+++ corrected matrixes
                dd = gsl_matrix_get(Sdd, iy, ix);
                du = gsl_matrix_get(Sdu, iy, ix);
                ud = gsl_matrix_get(Sud, iy, ix);
                uu = gsl_matrix_get(Suu, iy, ix);

                mm = dd * (fxP - (P0 - 1.0)) * (A2 + A4) * (A3 + 1);
                mm += du * (P0 - 1 - fxP) * (A2 + A4) * (1 - A1);
                mm += ud * (P0 - 1) * (A1 + A3) * (A4 + 1);
                mm += uu * (P0 - 1) * (A1 + A3) * (A2 - 1);
                mm /= (2 * fxP * (A1 + A3) * (A2 + A4) * TrMinus);

                mp = dd * (fxP - P0 + 1) * (A2 + A4) * (A3 - 1);
                mp += du * (fxP - P0 + 1) * (A2 + A4) * (1 + A1);
                mp += ud * (P0 - 1) * (A1 + A3) * (A4 - 1);
                mp += uu * (P0 - 1) * (A1 + A3) * (A2 + 1);
                mp /= (2 * fxP * (A1 + A3) * (A2 + A4) * TrMinus);

                pp = dd * (fxP - P0) * (A2 + A4) * (A3 - 1);
                pp += du * (fxP - P0) * (A2 + A4) * (1 + A1);
                pp += ud * P0 * (A1 + A3) * (A4 - 1);
                pp += uu * P0 * (A1 + A3) * (A2 + 1);
                pp /= (2 * fxP * (A1 + A3) * (A2 + A4) * TrPlus);

                pm = dd * (fxP - P0) * (A2 + A4) * (A3 + 1);
                pm += du * (P0 - fxP) * (A2 + A4) * (1 - A1);
                pm += ud * P0 * (A1 + A3) * (A4 + 1);
                pm += uu * P0 * (A1 + A3) * (A2 - 1);
                pm /= (2 * fxP * (A1 + A3) * (A2 + A4) * TrPlus);

                gsl_matrix_set(Sdd, iy, ix, mm);
                gsl_matrix_set(Sdu, iy, ix, mp);
                gsl_matrix_set(Sud, iy, ix, pm);
                gsl_matrix_set(Suu, iy, ix, pp);

                //+++ sigma matrixes

                ddSigma = gsl_matrix_get(SddSigma, iy, ix);
                duSigma = gsl_matrix_get(SduSigma, iy, ix);
                udSigma = gsl_matrix_get(SudSigma, iy, ix);
                uuSigma = gsl_matrix_get(SuuSigma, iy, ix);

                mmSigma = square(dd * (fxP - (P0 - 1.0)) * (A2 + A4) * (A3 + 1)) * ddSigma;
                mmSigma += square(du * (P0 - 1 - fxP) * (A2 + A4) * (1 - A1)) * duSigma;
                mmSigma += square(ud * (P0 - 1) * (A1 + A3) * (A4 + 1)) * udSigma;
                mmSigma += square(uu * (P0 - 1) * (A1 + A3) * (A2 - 1)) * uuSigma;
                mmSigma /= square(2 * fxP * (A1 + A3) * (A2 + A4) * TrMinus);
                mmSigma += square(mm * TrMinusSigma);

                mpSigma = square(dd * (fxP - P0 + 1) * (A2 + A4) * (A3 - 1)) * ddSigma;
                mpSigma += square(du * (fxP - P0 + 1) * (A2 + A4) * (1 + A1)) * duSigma;
                mpSigma += square(ud * (P0 - 1) * (A1 + A3) * (A4 - 1)) * udSigma;
                mpSigma += square(uu * (P0 - 1) * (A1 + A3) * (A2 + 1)) * uuSigma;
                mpSigma /= square(2 * fxP * (A1 + A3) * (A2 + A4) * TrMinus);
                mpSigma += square(mp * TrMinusSigma);

                ppSigma = square(dd * (fxP - P0) * (A2 + A4) * (A3 - 1)) * ddSigma;
                ppSigma += square(du * (fxP - P0) * (A2 + A4) * (1 + A1)) * duSigma;
                ppSigma += square(ud * P0 * (A1 + A3) * (A4 - 1)) * udSigma;
                ppSigma += square(uu * P0 * (A1 + A3) * (A2 + 1)) * uuSigma;
                ppSigma /= square(2 * fxP * (A1 + A3) * (A2 + A4) * TrPlus);
                ppSigma += square(pp * TrPlusSigma);

                pmSigma = square(dd * (fxP - P0) * (A2 + A4) * (A3 + 1)) * ddSigma;
                pmSigma += square(du * (P0 - fxP) * (A2 + A4) * (1 - A1)) * duSigma;
                pmSigma += square(ud * P0 * (A1 + A3) * (A4 + 1)) * udSigma;
                pmSigma += square(uu * P0 * (A1 + A3) * (A2 - 1)) * uuSigma;
                pmSigma /= square(2 * fxP * (A1 + A3) * (A2 + A4) * TrPlus);
                pmSigma += square(pm * TrPlusSigma);

                gsl_matrix_set(SddSigma, iy, ix, mmSigma);
                gsl_matrix_set(SduSigma, iy, ix, mpSigma);
                gsl_matrix_set(SudSigma, iy, ix, pmSigma);
                gsl_matrix_set(SuuSigma, iy, ix, ppSigma);
            }
    return true;
}
//+++
// new gsl_matrix function for MD x MD matrix:
// a = a / b (only for non-masked pixels defined in mask)
//+++
void divideWithMask(gsl_matrix *a, gsl_matrix *b, gsl_matrix *mask, int MD)
{
    for (int i = 0; i < MD; i++)
        for (int j = 0; j < MD; j++)
            if (gsl_matrix_get(mask, i, j) != 0.0)
            {
                if (gsl_matrix_get(b, i, j) != 0.0)
                    gsl_matrix_set(a, i, j, gsl_matrix_get(a, i, j) / gsl_matrix_get(b, i, j));
                else
                    gsl_matrix_set(a, i, j, 0.0);
            }
}
//+++
// new gsl_matrix function for MD x MD matrix:
// a = a + scale * b
//+++
void addScaled(gsl_matrix *a, gsl_matrix *b, double scale, int MD)
{
    gsl_matrix *m = gsl_matrix_calloc(MD, MD);
    gsl_matrix_memcpy(m, b);
    gsl_matrix_scale(m, scale);
    gsl_matrix_add(a, m);
    gsl_matrix_free(m);
}

bool invertTwoByTwo(const double matrix[2][2], double inverse[2][2])
{
    const double determinant = matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
    const double scale =
        std::fmax(std::fabs(matrix[0][0]) + std::fabs(matrix[0][1]), std::fabs(matrix[1][0]) + std::fabs(matrix[1][1]));
    if (!std::isfinite(determinant) || scale <= 0.0 || std::fabs(determinant) < 1e-6 * scale * scale)
        return false;
    inverse[0][0] = matrix[1][1] / determinant;
    inverse[0][1] = -matrix[0][1] / determinant;
    inverse[1][0] = -matrix[1][0] / determinant;
    inverse[1][1] = matrix[0][0] / determinant;
    return true;
}

// The x4 forward matrix is the incident-spin row multiplied by (1 +/- A).
// Divide each measured channel by 2, as the x4 correction does (its ideal-optics limit is 1/2).
bool correctTwoChannelPair(const std::array<TwoChannel, 2> &channels, bool fixedAnalyzer, int matrixSize,
                           TwoChannelPair &result)
{
    double response[2][2] = {};
    double incident[2][2] = {};
    for (int j = 0; j < 2; ++j)
    {
        const TwoChannel &channel = channels[j];
        const double effectivePolarization = channel.state == 3 || channel.state == 4
                                                 ? channel.polarization * (2.0 * channel.flipperEfficiency - 1.0)
                                                 : -channel.polarization;
        incident[j][0] = (1.0 + effectivePolarization) / 2.0; // incident up
        incident[j][1] = (1.0 - effectivePolarization) / 2.0; // incident down
        if (!std::isfinite(channel.transmission) || channel.transmission <= 0.0 || !std::isfinite(channel.thickness) ||
            channel.thickness <= 0.0 || !std::isfinite(channel.analyzerTransmission) ||
            channel.analyzerTransmission <= 0.0 || !std::isfinite(channel.analyzerEfficiency) ||
            channel.analyzerEfficiency <= 0.0 || channel.analyzerEfficiency > 1.0 || !std::isfinite(channel.factor) ||
            channel.factor <= 0.0)
            return false;

        if (!fixedAnalyzer)
        {
            const double a = channel.analyzerEfficiency;
            const bool analyzerUp = channel.state == 3 || channel.state == 6;
            response[j][0] = (1.0 + (analyzerUp ? a : -a)) / 2.0; // outgoing up
            response[j][1] = 1.0 - response[j][0];                // outgoing down
        }
    }

    if (fixedAnalyzer)
    {
        double inverseIncident[2][2];
        if (!invertTwoByTwo(incident, inverseIncident))
            return false;
        const double pureUpTransmission =
            inverseIncident[0][0] * channels[0].transmission + inverseIncident[0][1] * channels[1].transmission;
        const double pureDownTransmission =
            inverseIncident[1][0] * channels[0].transmission + inverseIncident[1][1] * channels[1].transmission;
        if (!std::isfinite(pureUpTransmission) || !std::isfinite(pureDownTransmission) || pureUpTransmission <= 0.0 ||
            pureDownTransmission <= 0.0)
            return false;
        for (int j = 0; j < 2; ++j)
        {
            response[j][0] = incident[j][0] * pureUpTransmission / channels[j].transmission;
            response[j][1] = incident[j][1] * pureDownTransmission / channels[j].transmission;
        }
    }

    double inverse[2][2];
    if (!invertTwoByTwo(response, inverse))
        return false;

    result.first.reset(gsl_matrix_calloc(matrixSize, matrixSize));
    result.second.reset(gsl_matrix_calloc(matrixSize, matrixSize));
    result.firstVariance.reset(gsl_matrix_calloc(matrixSize, matrixSize));
    result.secondVariance.reset(gsl_matrix_calloc(matrixSize, matrixSize));
    result.mask.reset(gsl_matrix_alloc(matrixSize, matrixSize));
    if (!result.first || !result.second || !result.firstVariance || !result.secondVariance || !result.mask)
        return false;
    gsl_matrix_memcpy(result.mask.get(), channels[0].mask.get());
    gsl_matrix_mul_elements(result.mask.get(), channels[1].mask.get());

    const double scales[2] = {channels[0].factor / (2.0 * channels[0].thickness * channels[0].analyzerTransmission *
                                                    channels[0].transmission),
                              channels[1].factor / (2.0 * channels[1].thickness * channels[1].analyzerTransmission *
                                                    channels[1].transmission)};
    for (int row = 0; row < matrixSize; ++row)
        for (int col = 0; col < matrixSize; ++col)
        {
            if (gsl_matrix_get(result.mask.get(), row, col) == 0.0)
                continue;
            double values[2];
            double variances[2];
            for (int j = 0; j < 2; ++j)
            {
                values[j] = scales[j] * gsl_matrix_get(channels[j].image.get(), row, col);
                const double relativeVariance = gsl_matrix_get(channels[j].relativeVariance.get(), row, col);
                if (!std::isfinite(values[j]) || !std::isfinite(relativeVariance) || relativeVariance < 0.0)
                {
                    gsl_matrix_set(result.mask.get(), row, col, 0.0);
                    break;
                }
                // Transmission and optical uncertainties are shared across pixels and output channels.
                // Keep the image variance statistical rather than adding them as independent pixel noise.
                variances[j] = values[j] * values[j] * relativeVariance;
            }
            if (gsl_matrix_get(result.mask.get(), row, col) == 0.0)
                continue;
            const double first = inverse[0][0] * values[0] + inverse[0][1] * values[1];
            const double second = inverse[1][0] * values[0] + inverse[1][1] * values[1];
            for (int k = 0; k < 2; ++k)
                for (int j = 0; j < 2; ++j)
                    result.channelShares[k][j] += inverse[k][j] * values[j];
            gsl_matrix_set(result.first.get(), row, col, first);
            gsl_matrix_set(result.second.get(), row, col, second);
            gsl_matrix_set(result.firstVariance.get(), row, col,
                           inverse[0][0] * inverse[0][0] * variances[0] + inverse[0][1] * inverse[0][1] * variances[1]);
            gsl_matrix_set(result.secondVariance.get(), row, col,
                           inverse[1][0] * inverse[1][0] * variances[0] + inverse[1][1] * inverse[1][1] * variances[1]);
        }
    return true;
}
} // namespace DanPolarization
