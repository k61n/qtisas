/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: test functions of dan_polarization_corrections_tests.cpp (a header, so that AUTOMOC needs no
             .moc include)
 ******************************************************************************/

#ifndef DAN_POLARIZATION_CORRECTIONS_TESTS_H
#define DAN_POLARIZATION_CORRECTIONS_TESTS_H

#include <QObject>

class DanPolarizationCorrectionsTests : public QObject
{
    Q_OBJECT

  private slots:
    static void transmissionsX2();
    static void crossSectionsX2();
    static void crossSectionsX4();
    static void fixedAnalyzerUpUpDownUp();
    static void fixedIncidentUpUpUpDown();
    static void fixedAnalyzerUpDownDownDown();
    static void fixedIncidentDownUpDownDown();
    static void maskedPixelStaysZero();
    static void rejectsSingularIncidentPair();
};

#endif
