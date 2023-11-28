/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Individual Header Parser for Collimation Distance device
 ******************************************************************************/

#include "collimation.h"

Collimation::Collimation(ParserHeader *parserHeaderDan, QComboBox *unitsCollimationDan)
{
    unitsCollimation = unitsCollimationDan;
    parserHeader = parserHeaderDan;
}
//+++ C units converter to [cm]
double Collimation::unitsConverterCtoCM(double C)
{
    if (unitsCollimation->currentText() == "m")
        return C * 100.0;
    if (unitsCollimation->currentText() == "mm")
        return C / 10.0;
    return C;
}
//+++ read C in units like in Header
QString Collimation::readClikeInHeader(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[C]", lst);
}
//+++ read C [cm]
double Collimation::readC(const QString &Number, const QStringList &lst)
{
    double C = readClikeInHeader(Number, lst).toDouble();
    C = unitsConverterCtoCM(C);
    return C;
}
//+++ read C   [m]
double Collimation::readCinM(const QString &Number, const QStringList &lst)
{
    return readC(Number, lst) / 100.0;
}