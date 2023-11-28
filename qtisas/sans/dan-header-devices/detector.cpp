/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Individual Header Parser for Detector Distance device
 ******************************************************************************/

#include "detector.h"

Detector::Detector(ParserHeader *parserHeaderDan, QComboBox *unitsDistanceDan)
{
    unitsDistance = unitsDistanceDan;
    parserHeader = parserHeaderDan;
}
//+++ D units converter to [cm]
double Detector::unitsConverterDtoCM(double D)
{
    if (unitsDistance->currentText() == "m")
        return D * 100.0;
    if (unitsDistance->currentText() == "mm")
        return D / 10.0;
    return D;
}
//+++ read D in units like in Header
QString Detector::readDlikeInHeader(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[D]", lst);
}
//+++ read D's offset in units like in Header
QString Detector::readOffsetDlikeInHeader(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[C,D-Offset]", lst);
}
//+++ read D = D + offset + add-Offset   [cm]
double Detector::readD(const QString &Number, const QStringList &lst)
{
    double D;
    D = readDlikeInHeader(Number, lst).toDouble();
    D += readOffsetDlikeInHeader(Number, lst).toDouble();
    D = unitsConverterDtoCM(D);
    return D;
}
//+++ read D = D + offset + add-Offset   [m]
double Detector::readDinM(const QString &Number, const QStringList &lst)
{
    return readD(Number, lst) / 100.0;
}