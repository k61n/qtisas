/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: the PN-mode corrections (x2, x4, the two-file x421..x424) on synthetic one-pixel data: the cross
             sections go through one forward model (polarizer P, flipper f, analyzers, pure-spin transmissions) and
             must come back
 ******************************************************************************/

#include "dan_polarization_corrections_tests.h"

#include "dan-polarization-corrections.h"

#include <QtTest/QtTest>

#include <array>
#include <cmath>

using namespace DanPolarization;

namespace
{
constexpr double CrossSections[2][2] = {{8.0, 2.0}, {3.0, 7.0}}; // incident up/down, outgoing up/down

// two analyzed channels of one pixel (states: 3 up-up, 4 up-down, 5 down-down, 6 down-up) and the values the
// correction must return
std::array<TwoChannel, 2> makePair(const std::array<int, 2> &states, bool fixedAnalyzer,
                                   bool differentAnalyzerEfficiency, bool masked, double expected[2])
{
    std::array<TwoChannel, 2> channels;
    for (int j = 0; j < 2; ++j)
    {
        TwoChannel &channel = channels[j];
        channel.state = states[j];
        channel.factor = 3.0;
        channel.thickness = 2.0;
        channel.analyzerTransmission = 0.2;
        channel.analyzerEfficiency = differentAnalyzerEfficiency && j == 1 ? 0.85 : 0.9;
        channel.polarization = 0.8;
        channel.flipperEfficiency = 0.9;
        channel.image.reset(gsl_matrix_calloc(1, 1));
        channel.relativeVariance.reset(gsl_matrix_calloc(1, 1));
        channel.mask.reset(gsl_matrix_calloc(1, 1));
        gsl_matrix_set(channel.mask.get(), 0, 0, masked && j == 1 ? 0.0 : 1.0);
        gsl_matrix_set(channel.relativeVariance.get(), 0, 0, 0.01);

        const bool incidentUp = channel.state == 3 || channel.state == 4;
        const bool analyzerUp = channel.state == 3 || channel.state == 6;
        const double incidentUpFraction = incidentUp ? (1.0 + 0.8 * 0.8) / 2.0 : (1.0 - 0.8) / 2.0;
        const double incidentFractions[2] = {incidentUpFraction, 1.0 - incidentUpFraction};
        const double pureTransmissions[2] = {0.9, 0.8};
        const double analyzedUpFraction =
            (1.0 + (analyzerUp ? channel.analyzerEfficiency : -channel.analyzerEfficiency)) / 2.0;
        const double analyzerFractions[2] = {analyzedUpFraction, 1.0 - analyzedUpFraction};
        channel.transmission =
            incidentFractions[0] * pureTransmissions[0] + incidentFractions[1] * pureTransmissions[1];

        double detectorSignal = 0.0;
        for (int incident = 0; incident < 2; ++incident)
            for (int outgoing = 0; outgoing < 2; ++outgoing)
                detectorSignal += incidentFractions[incident] * pureTransmissions[incident] *
                                  CrossSections[incident][outgoing] * analyzerFractions[outgoing];
        const double rawDetectorValue =
            detectorSignal * 2.0 * channel.thickness * channel.analyzerTransmission / channel.factor;
        gsl_matrix_set(channel.image.get(), 0, 0, rawDetectorValue);
    }

    if (fixedAnalyzer)
    {
        const bool analyzerUp = states[0] == 3 || states[0] == 6;
        const double analyzerUpFraction = analyzerUp ? 0.95 : 0.05;
        for (int incident = 0; incident < 2; ++incident)
            expected[incident] = CrossSections[incident][0] * analyzerUpFraction +
                                 CrossSections[incident][1] * (1.0 - analyzerUpFraction);
    }
    else
    {
        const bool incidentUp = states[0] == 3 || states[0] == 4;
        const double incidentUpFraction = incidentUp ? 0.82 : 0.1;
        const double weights[2] = {incidentUpFraction * 0.9 / channels[0].transmission,
                                   (1.0 - incidentUpFraction) * 0.8 / channels[0].transmission};
        for (int outgoing = 0; outgoing < 2; ++outgoing)
            expected[outgoing] = weights[0] * CrossSections[0][outgoing] + weights[1] * CrossSections[1][outgoing];
    }
    return channels;
}

// the pair corrected: both cross sections back within 1e-10, both variances positive
void verifyPair(const std::array<int, 2> &states, bool fixedAnalyzer, bool differentAnalyzerEfficiency)
{
    double expected[2] = {};
    const std::array<TwoChannel, 2> channels =
        makePair(states, fixedAnalyzer, differentAnalyzerEfficiency, false, expected);
    TwoChannelPair corrected;
    QVERIFY(correctTwoChannelPair(channels, fixedAnalyzer, 1, corrected));
    const double first = gsl_matrix_get(corrected.first.get(), 0, 0);
    const double second = gsl_matrix_get(corrected.second.get(), 0, 0);
    QVERIFY2(std::fabs(first - expected[0]) < 1e-10,
             qPrintable(QString("first %1, expected %2").arg(first, 0, 'g', 15).arg(expected[0], 0, 'g', 15)));
    QVERIFY2(std::fabs(second - expected[1]) < 1e-10,
             qPrintable(QString("second %1, expected %2").arg(second, 0, 'g', 15).arg(expected[1], 0, 'g', 15)));
    QVERIFY(gsl_matrix_get(corrected.firstVariance.get(), 0, 0) > 0.0);
    QVERIFY(gsl_matrix_get(corrected.secondVariance.get(), 0, 0) > 0.0);
}
// incident fractions (up, down) of a state: up (flipper on) P(2f-1), down (flipper off) -P
void incidentFractions(bool incidentUp, double P, double f, double fractions[2])
{
    fractions[0] = incidentUp ? (1.0 + P * (2.0 * f - 1.0)) / 2.0 : (1.0 - P) / 2.0;
    fractions[1] = 1.0 - fractions[0];
}

gsl_matrix *pixel(double value)
{
    gsl_matrix *m = gsl_matrix_alloc(1, 1);
    gsl_matrix_set(m, 0, 0, value);
    return m;
}

bool close(double actual, double expected)
{
    return std::fabs(actual - expected) < 1e-10;
}

bool close(const gsl_matrix *m, double expected)
{
    return close(gsl_matrix_get(m, 0, 0), expected);
}
} // namespace

void DanPolarizationCorrectionsTests::transmissionsX2()
{
    // the analyzer-out transmissions measured in the down and up states, from the pure-spin ones
    const double P = 0.8;
    const double f = 0.9;
    const double T[2] = {0.9, 0.8}; // pure up (+), down (-)
    double down[2];
    double up[2];
    incidentFractions(false, P, f, down);
    incidentFractions(true, P, f, up);
    double trDown = down[0] * T[0] + down[1] * T[1];
    double trUp = up[0] * T[0] + up[1] * T[1];
    // relative sigmas in and out: the absolute ones go through the inverse, -/+ = C (down, up)
    const double Pf = 2.0 * f - 1.0;
    const double det = P * (Pf + 1);
    const double absDown = 0.01 * trDown;
    const double absUp = 0.02 * trUp;
    const double expectedMinus = std::hypot((P * Pf + 1) * absDown, (P - 1) * absUp) / det / T[1];
    const double expectedPlus = std::hypot((P * Pf - 1) * absDown, (P + 1) * absUp) / det / T[0];
    double sigmaDown = 0.01;
    double sigmaUp = 0.02;
    QVERIFY(transmissionCorrectionPNx2(trDown, sigmaDown, trUp, sigmaUp, P, Pf));
    QVERIFY2(close(trDown, T[1]) && close(trUp, T[0]), qPrintable(QString("T- %1, T+ %2").arg(trDown).arg(trUp)));
    QVERIFY2(close(sigmaDown, expectedMinus) && close(sigmaUp, expectedPlus),
             qPrintable(QString("sigma- %1 (%2), sigma+ %3 (%4)")
                            .arg(sigmaDown)
                            .arg(expectedMinus)
                            .arg(sigmaUp)
                            .arg(expectedPlus)));
    QVERIFY(!transmissionCorrectionPNx2(trDown, sigmaDown, trUp, sigmaUp, 0.0, 0.8)); // P = 0: no inverse
}

void DanPolarizationCorrectionsTests::crossSectionsX2()
{
    // no analyzer: down and up each mix sigma- and sigma+; equal pure-spin transmissions, as only then x2's order
    // (measured channel divided by the transmission, then the polarizer inverse) and the physical one agree
    const double P = 0.8;
    const double f = 0.9;
    const double T = 0.85;
    const double sigma[2] = {9.0, 4.0}; // +, -
    double down[2];
    double up[2];
    incidentFractions(false, P, f, down);
    incidentFractions(true, P, f, up);
    gsl_matrix *sDown = pixel(T * (down[0] * sigma[0] + down[1] * sigma[1]));
    gsl_matrix *sUp = pixel(T * (up[0] * sigma[0] + up[1] * sigma[1]));
    gsl_matrix *eDown = pixel(0.0);
    gsl_matrix *eUp = pixel(0.0);
    gsl_matrix *mask = pixel(1.0);
    const bool ok = matrixCorrectionPNx2(sDown, eDown, sUp, eUp, mask, T, 0.0, T, 0.0, P, 2.0 * f - 1.0, 1);
    const bool recovered = close(sDown, sigma[1]) && close(sUp, sigma[0]);
    const QString got = QString("I- %1, I+ %2").arg(gsl_matrix_get(sDown, 0, 0)).arg(gsl_matrix_get(sUp, 0, 0));
    for (gsl_matrix *m : {sDown, sUp, eDown, eUp, mask})
        gsl_matrix_free(m);
    QVERIFY(ok);
    QVERIFY2(recovered, qPrintable(got));
}

void DanPolarizationCorrectionsTests::crossSectionsX4()
{
    // the four analyzed channels from the same forward model as the two-file cases (factor 2 included), four
    // different analyzer efficiencies, different pure-spin transmissions given as DAN measures them (analyzer out)
    const double P = 0.8;
    const double f = 0.9;
    const double T[2] = {0.9, 0.8};               // pure up (+), down (-)
    const double A[4] = {0.97, 0.85, 0.88, 0.93}; // down-down, up-down, down-up, up-up
    auto channel = [&](bool incidentUp, bool analyzerUp, double a) {
        double incident[2];
        incidentFractions(incidentUp, P, f, incident);
        const double analyzed[2] = {(1.0 + (analyzerUp ? a : -a)) / 2.0, (1.0 - (analyzerUp ? a : -a)) / 2.0};
        double signal = 0.0;
        for (int i = 0; i < 2; ++i)
            for (int o = 0; o < 2; ++o)
                signal += incident[i] * T[i] * CrossSections[i][o] * analyzed[o];
        return pixel(2.0 * signal);
    };
    gsl_matrix *dd = channel(false, false, A[0]);
    gsl_matrix *ud = channel(true, false, A[1]);
    gsl_matrix *du = channel(false, true, A[2]);
    gsl_matrix *uu = channel(true, true, A[3]);
    gsl_matrix *errors[4] = {pixel(0.0), pixel(0.0), pixel(0.0), pixel(0.0)};
    gsl_matrix *mask = pixel(1.0);
    double down[2];
    double up[2];
    incidentFractions(false, P, f, down);
    incidentFractions(true, P, f, up);
    double trMinus = down[0] * T[0] + down[1] * T[1];
    double trPlus = up[0] * T[0] + up[1] * T[1];
    double sigmaMinus = 0.0;
    double sigmaPlus = 0.0;
    const bool ok = transmissionCorrectionPNx2(trMinus, sigmaMinus, trPlus, sigmaPlus, P, 2.0 * f - 1.0) &&
                    matrixCorrectionPNx4(dd, errors[0], du, errors[1], uu, errors[2], ud, errors[3], mask, trMinus,
                                         sigmaMinus, trPlus, sigmaPlus, A[0], A[1], A[2], A[3], P, f, 1);
    // CrossSections[incident][outgoing], up first: ++ 8, +- 2, -+ 3, -- 7
    const bool recovered = close(uu, CrossSections[0][0]) && close(ud, CrossSections[0][1]) &&
                           close(du, CrossSections[1][0]) && close(dd, CrossSections[1][1]);
    const QString got = QString("++ %1, +- %2, -+ %3, -- %4")
                            .arg(gsl_matrix_get(uu, 0, 0))
                            .arg(gsl_matrix_get(ud, 0, 0))
                            .arg(gsl_matrix_get(du, 0, 0))
                            .arg(gsl_matrix_get(dd, 0, 0));
    for (gsl_matrix *m : {dd, ud, du, uu, errors[0], errors[1], errors[2], errors[3], mask})
        gsl_matrix_free(m);
    QVERIFY(ok);
    QVERIFY2(recovered, qPrintable(got));
}

void DanPolarizationCorrectionsTests::fixedAnalyzerUpUpDownUp()
{
    verifyPair({3, 6}, true, false); // PNx421
}

void DanPolarizationCorrectionsTests::fixedIncidentUpUpUpDown()
{
    verifyPair({3, 4}, false, true); // PNx422
}

void DanPolarizationCorrectionsTests::fixedAnalyzerUpDownDownDown()
{
    verifyPair({4, 5}, true, false); // PNx423
}

void DanPolarizationCorrectionsTests::fixedIncidentDownUpDownDown()
{
    verifyPair({6, 5}, false, true); // PNx424
}

void DanPolarizationCorrectionsTests::maskedPixelStaysZero()
{
    double expected[2] = {};
    const std::array<TwoChannel, 2> channels = makePair({3, 6}, true, false, true, expected);
    TwoChannelPair corrected;
    QVERIFY(correctTwoChannelPair(channels, true, 1, corrected));
    QCOMPARE(gsl_matrix_get(corrected.mask.get(), 0, 0), 0.0);
    QCOMPARE(gsl_matrix_get(corrected.first.get(), 0, 0), 0.0);
    QCOMPARE(gsl_matrix_get(corrected.second.get(), 0, 0), 0.0);
}

void DanPolarizationCorrectionsTests::rejectsSingularIncidentPair()
{
    std::array<TwoChannel, 2> channels;
    for (int j = 0; j < 2; ++j)
    {
        channels[j].state = j == 0 ? 3 : 4; // same incident preparation
        channels[j].polarization = 0.8;
        channels[j].flipperEfficiency = 0.9;
        channels[j].factor = 3.0;
        channels[j].thickness = 2.0;
        channels[j].analyzerTransmission = 0.2;
        channels[j].analyzerEfficiency = 0.9;
        channels[j].transmission = 0.85;
    }
    TwoChannelPair corrected;
    QVERIFY(!correctTwoChannelPair(channels, true, 1, corrected));
}

QTEST_APPLESS_MAIN(DanPolarizationCorrectionsTests)
