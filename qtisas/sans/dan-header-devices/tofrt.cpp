/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Individual Header Parser for Tof/Rt information
 ******************************************************************************/

#include "tofrt.h"

Tofrt::Tofrt(ParserHeader *parserHeaderDAN, QComboBox *unitsDurationDAN)
{
    parserHeader = parserHeaderDAN;
    unitsDuration = unitsDurationDAN;
}
//+++ Tof/Rt time units converter to [sec]
double Tofrt::unitsConverterDurationToSec(double duration)
{
    if (unitsDuration->currentIndex() == 1)
        duration /= 10.0;
    if (unitsDuration->currentIndex() == 2)
        duration /= 1000.0;
    if (unitsDuration->currentIndex() == 3)
        duration /= 1000000.0;
    return duration;
}
//+++ read Rt Current Number
int Tofrt::readRtCurrentNumber(const QString &Number, const QStringList &lst)
{
    int currentNumber =
        QString::number(parserHeader->readNumberString(Number, "[RT-Current-Number]", lst).toDouble(), 'f', 0).toInt();
    if (currentNumber < 1)
        return 1;
    return currentNumber;
}
//+++ read  Slices-Count
int Tofrt::readSlicesCount(const QString &Number, const QStringList &lst)
{
    int slicesCount =
        QString::number(parserHeader->readNumberString(Number, "[Slices-Count]", lst).toDouble(), 'f', 0).toInt();
    if (slicesCount < 1)
        slicesCount = 1;
    return slicesCount;
}
//+++ read  Slices-Duration
QString Tofrt::readSlicesDuration(const QString &Number, const QStringList &lst)
{
    double slicesDuration =
        unitsConverterDurationToSec(parserHeader->readNumberString(Number, "[Slices-Duration]", lst).toDouble());
    if (slicesDuration < 0.0)
        slicesDuration = 0.0;

    return QString::number(slicesDuration);
}
//+++ read  Slices-Current-Number
int Tofrt::readSlicesCurrentNumber(const QString &Number, const QStringList &lst)
{
    int SlicesCurrentNumber =
        QString::number(parserHeader->readNumberString(Number, "[Slices-Current-Number]", lst).toDouble(), 'f', 0)
            .toInt();
    if (SlicesCurrentNumber < 0)
        SlicesCurrentNumber = 0;
    return SlicesCurrentNumber;
}
//+++ read  Slices-Current-Duration
QString Tofrt::readSlicesCurrentDuration(const QString &Number, const QStringList &lst)
{
    double SlicesCurrentDuration = unitsConverterDurationToSec(
        parserHeader->readNumberString(Number, "[Slices-Current-Duration]", lst).toDouble());
    if (SlicesCurrentDuration < 0)
        SlicesCurrentDuration = 0;
    return QString::number(SlicesCurrentDuration);
}
//+++ read  Slices-Current-Monitor1
QString Tofrt::readSlicesCurrentMonitor1(const QString &Number, const QStringList &lst)
{
    double SlicesCurrentMonitor1 = parserHeader->readNumberString(Number, "[Slices-Current-Monitor1]", lst).toDouble();
    if (SlicesCurrentMonitor1 < 0)
        SlicesCurrentMonitor1 = 0;
    return QString::number(SlicesCurrentMonitor1);
}
//+++ read  Slices-Current-Monitor2
QString Tofrt::readSlicesCurrentMonitor2(const QString &Number, const QStringList &lst)
{
    double SlicesCurrentMonitor2 = parserHeader->readNumberString(Number, "[Slices-Current-Monitor2]", lst).toDouble();
    if (SlicesCurrentMonitor2 < 0)
        SlicesCurrentMonitor2 = 0;
    return QString::number(SlicesCurrentMonitor2);
}
//+++ read  Slices-Current-Monitor3
QString Tofrt::readSlicesCurrentMonitor3(const QString &Number, const QStringList &lst)
{
    double SlicesCurrentMonitor3 = parserHeader->readNumberString(Number, "[Slices-Current-Monitor3]", lst).toDouble();
    if (SlicesCurrentMonitor3 < 0)
        SlicesCurrentMonitor3 = 0;
    return QString::number(SlicesCurrentMonitor3);
}
//+++ read  Slices-Current-Sum
QString Tofrt::readSlicesCurrentSum(const QString &Number, const QStringList &lst)
{
    double SlicesCurrentSum = parserHeader->readNumberString(Number, "[Slices-Current-Sum]", lst).toDouble();
    if (SlicesCurrentSum < 0)
        SlicesCurrentSum = 0;
    return QString::number(SlicesCurrentSum);
}