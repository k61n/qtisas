/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: z values of a 3D graph with a log10 z axis (Graph3DLogScale.h)
 ******************************************************************************/

#include <QtTest>

#include "Graph3DLogScale.h"
#include "graph3d_log_scale_tests.h"

using Graph3DLogScale::logZ;
using Graph3DLogScale::plotZ;

void Graph3DLogScaleTests::positiveValuesAsExponents()
{
    // the axis -3..2 (0.001..100)
    QCOMPARE(plotZ(10.0, true, -3.0, 2.0), 1.0);
    QCOMPARE(plotZ(1.0, true, -3.0, 2.0), 0.0);
    QCOMPARE(plotZ(0.01, true, -3.0, 2.0), -2.0);
    QCOMPARE(plotZ(1e5, true, -3.0, 2.0), 2.0);   // above: the upper limit
    QCOMPARE(plotZ(1e-6, true, -3.0, 2.0), -3.0); // below: the lower limit
}

void Graph3DLogScaleTests::nonPositiveValuesAtLowerLimit()
{
    // before: 0 kept as the exponent 0 (shown as 1), -0.5 as 10^-0.5
    QCOMPARE(plotZ(0.0, true, -3.0, 2.0), -3.0);
    QCOMPARE(plotZ(-0.5, true, -3.0, 2.0), -3.0);
    QCOMPARE(plotZ(-1e3, true, -3.0, 2.0), -3.0);
    // a range above 1 (exponents 1..4): 0 was clamped to 10, now too the lower limit
    QCOMPARE(plotZ(0.0, true, 1.0, 4.0), 1.0);
}

void Graph3DLogScaleTests::linearAxisOnlyClamps()
{
    QCOMPARE(plotZ(0.0, false, -1.0, 1.0), 0.0);
    QCOMPARE(plotZ(-0.5, false, -1.0, 1.0), -0.5);
    QCOMPARE(plotZ(-5.0, false, -1.0, 1.0), -1.0);
    QCOMPARE(plotZ(5.0, false, -1.0, 1.0), 1.0);
}

void Graph3DLogScaleTests::formulaSurfaces()
{
    QCOMPARE(logZ(100.0, true, -3.0), 2.0);
    QCOMPARE(logZ(0.0, true, -3.0), -3.0);
    QCOMPARE(logZ(-2.0, true, -3.0), -3.0);
    QCOMPARE(logZ(-2.0, false, -3.0), -2.0);
}

QTEST_GUILESS_MAIN(Graph3DLogScaleTests)
