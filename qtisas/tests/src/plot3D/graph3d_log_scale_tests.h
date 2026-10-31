/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: test functions of graph3d_log_scale_tests.cpp (a header, so that AUTOMOC needs no .moc include)
 ******************************************************************************/

#ifndef GRAPH3D_LOG_SCALE_TESTS_H
#define GRAPH3D_LOG_SCALE_TESTS_H

#include <QObject>

class Graph3DLogScaleTests : public QObject
{
    Q_OBJECT

  private slots:
    static void positiveValuesAsExponents();
    static void nonPositiveValuesAtLowerLimit();
    static void linearAxisOnlyClamps();
    static void formulaSurfaces();
};

#endif
