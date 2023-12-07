/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Individual Header Parser for Detector Distance device
 ******************************************************************************/

#include "detector.h"

Detector::Detector(ParserHeader *parserHeaderDan, QComboBox *unitsDistanceDan, QRadioButton *rotationXDan,
                   QDoubleSpinBox *constValueDetRotXDan, QCheckBox *invDetRotXDan, QRadioButton *rotationYDan,
                   QDoubleSpinBox *constValueDetRotYDan, QCheckBox *invDetRotYDan, QCheckBox *detectorCenterAsParaDAN,
                   QCheckBox *detRotAsParaDan)
{
    unitsDistance = unitsDistanceDan;
    parserHeader = parserHeaderDan;
    rotationX = rotationXDan;
    constValueDetRotX = constValueDetRotXDan;
    invDetRotX = invDetRotXDan;
    rotationY = rotationYDan;
    constValueDetRotY = constValueDetRotYDan;
    invDetRotY = invDetRotYDan;
    detectorCenterAsPara = detectorCenterAsParaDAN;
    detRotAsPara = detRotAsParaDan;
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
//+++ read  DetectorX
double Detector::readDetectorX(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Detector-X || Beamcenter-X]", lst).toDouble();
}
//+++ read  DetectorY
double Detector::readDetectorY(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Detector-Y || Beamcenter-Y]", lst).toDouble();
}
//+++ compare DetectorX/Y positions
bool Detector::compareBeamPosition(const QString &n1, const QString &n2)
{
    if (!detectorCenterAsPara->isChecked())
        return true;
    if (fabs(double(readDetectorX(n1) - readDetectorX(n2))) > 5.0)
        return false;
    if (fabs(double(readDetectorY(n1) - readDetectorY(n2))) > 5.0)
        return false;
    return true;
}
//+++ read DetRotationX
double Detector::readDetRotationX(const QString &Number, const QStringList &lst)
{
    double detRotX;
    if (rotationX->isChecked())
        detRotX = parserHeader->readNumberString(Number, "[Detector-Angle-X]", lst).toDouble();
    else
        detRotX = constValueDetRotX->value();

    if (invDetRotX->isChecked())
        detRotX *= -1.0;

    return detRotX;
}
//+++ read DetRotationY
double Detector::readDetRotationY(const QString &Number, const QStringList &lst)
{
    double detRotY;
    if (rotationY->isChecked())
        detRotY = parserHeader->readNumberString(Number, "[Detector-Angle-Y]", lst).toDouble();
    else
        detRotY = constValueDetRotY->value();

    if (invDetRotY->isChecked())
        detRotY *= -1.0;

    return detRotY;
}
//+++ compare DetRotationX/Y Position
bool Detector::compareDetRotationPosition(const QString &n1, const QString &n2)
{
    if (!detRotAsPara->isChecked())
        return true;
    if (fabs(double(readDetRotationX(n1) - readDetRotationX(n2))) > 0.015)
        return false;
    if (fabs(double(readDetRotationY(n1) - readDetRotationY(n2))) > 0.015)
        return false;
    return true;
}
//+++ read :: [Sum]
double Detector::readSum(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Sum]", lst).toDouble();
}