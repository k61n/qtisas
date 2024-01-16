/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: TOF/RT Information
 ******************************************************************************/
#ifndef TOFRT_H
#define TOFRT_H

#include "dan-parser-header.h"

#include <QComboBox>
#include <cmath>

class Tofrt : public QObject
{
    Q_OBJECT
  protected:
    ParserHeader *parserHeader;
    QComboBox *unitsDuration;

    //+++ Duration units converter to [sec]
    double unitsConverterDurationToSec(double duration);

  public:
    Tofrt(ParserHeader *parserHeader, QComboBox *unitsDuration);

    //+++ read Rt Current Number
    int readRtCurrentNumber(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Slices-Count
    int readSlicesCount(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Slices-Duration
    QString readSlicesDuration(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Slices-Current-Number
    int readSlicesCurrentNumber(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Slices-Current-Duration
    QString readSlicesCurrentDuration(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Slices-Current-Monitor1
    QString readSlicesCurrentMonitor1(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Slices-Current-Monitor2
    QString readSlicesCurrentMonitor2(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Slices-Current-Monitor3
    QString readSlicesCurrentMonitor3(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Slices-Current-Sum
    QString readSlicesCurrentSum(const QString &Number, const QStringList &lst = QStringList());

  public slots:
};
#endif
