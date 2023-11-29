/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Individual Header Parser for Collimation Distance device
 ******************************************************************************/

#include "collimation.h"

Collimation::Collimation(ParserHeader *parserHeaderDan, QComboBox *unitsCollimationDan, QCheckBox *apertureCAroundDAN,
                         QCheckBox *apertureSAroundDAN, QComboBox *unitsApertureDAN)
{
    unitsCollimation = unitsCollimationDan;
    parserHeader = parserHeaderDan;
    apertureCAround = apertureCAroundDAN;
    apertureSAround = apertureSAroundDAN;
    unitsAperture = unitsApertureDAN;
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
//+++ read  CA (collimation aperture)
QString Collimation::readCA(const QString &Number, const QStringList &lst)
{
    double cax = parserHeader->readNumberString(Number, "[CA-X]", lst).toDouble();
    if (cax <= 0.0)
        return "x";
    double cay = parserHeader->readNumberString(Number, "[CA-Y]", lst).toDouble();
    if (cay <= 0.0)
        return "x";

    QString CA = "";
    CA += QString::number(cax, 'f', 1);
    CA += "x";
    CA += QString::number(cay, 'f', 1);

    return CA.remove(".0");
}
//+++ read  SA (sample aperture)
QString Collimation::readSA(const QString &Number, const QStringList &lst)
{
    double sax = parserHeader->readNumberString(Number, "[SA-X]", lst).toDouble();
    if (sax <= 0.0)
        return "x";
    double say = parserHeader->readNumberString(Number, "[SA-Y]", lst).toDouble();
    if (say <= 0.0)
        return "x";

    QString SA = "";
    SA += QString::number(sax, 'f', 1);
    SA += "x";
    SA += QString::number(say, 'f', 1);

    return SA.remove(".0");
}
//+++ Apertures units converter to [cm]
double Collimation::unitsConverterAperturetoCM(double R)
{
    if (unitsAperture->currentIndex() == 1)
        return R * 0.1;
    if (unitsAperture->currentIndex() == 2)
        return R * 1.0e-4;
    return R;
}
//+++ read  R1 source aperture [CA-X] x [CA-X] in [cm]
double Collimation::readR1(const QString &Number, const QStringList &lst)
{
    double cax = parserHeader->readNumberString(Number, "[CA-X]", lst).toDouble();
    if (cax <= 0.0)
        return 0.0;
    cax = unitsConverterAperturetoCM(cax);
    double cay = parserHeader->readNumberString(Number, "[CA-Y]", lst).toDouble();
    if (cay <= 0.0)
        return 0.0;
    cay = unitsConverterAperturetoCM(cay);

    double r1 = cax * cay;

    if (apertureCAround->isChecked())
        r1 = sqrt(r1 / 4.0);
    else
        r1 = sqrt(r1 / M_PI);

    return r1;
}
//+++ read  R2 sample aperture [SA-X] x [SA-X] in [cm]
double Collimation::readR2(const QString &Number, const QStringList &lst)
{
    double sax = parserHeader->readNumberString(Number, "[SA-X]", lst).toDouble();
    if (sax <= 0.0)
        return 0.0;
    sax = unitsConverterAperturetoCM(sax);
    double say = parserHeader->readNumberString(Number, "[SA-Y]", lst).toDouble();
    if (say <= 0.0)
        return 0.0;
    say = unitsConverterAperturetoCM(say);

    double r2 = sax * say;

    if (apertureSAround->isChecked())
        r2 = sqrt(r2 / 4.0);
    else
        r2 = sqrt(r2 / M_PI);

    return r2;
}