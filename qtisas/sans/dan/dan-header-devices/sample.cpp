/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Individual Header Parser for Sample Information
 ******************************************************************************/

#include "sample.h"

Sample::Sample(ParserHeader *parserHeaderDAN, QComboBox *unitsThicknessDAN, QCheckBox *sampleNumberAsParaDAN)
{
    parserHeader = parserHeaderDAN;
    unitsThickness = unitsThicknessDAN;
    sampleNumberAsPara = sampleNumberAsParaDAN;
}
//+++ Thickness units converter to [cm]
double Sample::unitsConverterThicknessToCM(double thickness)
{
    if (unitsThickness->currentIndex() == 1)
        return thickness / 10.0; // mm -> cm
    if (unitsThickness->currentIndex() == 2)
        return thickness / 10000.0; // mkm -> cm
    return thickness;
}
//+++ read  Info / Sample / Sample Title
QString Sample::readName(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Sample-Title]", lst);
}
//+++ read Sample Position Number
QString Sample::readPositionNumber(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Sample-Position-Number]", lst);
}
//+++ read Experiment Title
QString Sample::readExperimentTitle(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Experiment-Title]", lst);
}
//+++ read User Name
QString Sample::readUserName(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[User-Name]", lst);
}
//+++ read Comment #1
QString Sample::readComment1(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Comment1]", lst);
}
//+++ read Comment #2
QString Sample::readComment2(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Comment2]", lst);
}
//+++ read Time
QString Sample::readTime(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Time]", lst);
}
//+++ read Date
QString Sample::readDate(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Date]", lst);
}
//+++ read  Thickness [cm]
double Sample::readThickness(const QString &Number, const QStringList &lst)
{
    return unitsConverterThicknessToCM(parserHeader->readNumberString(Number, "[Sample-Thickness]", lst).toDouble());
}
//+++ read  Sample Motor #1
QString Sample::readMotor1(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Sample-Motor-1]", lst);
}
//+++ read  Sample Motor #2
QString Sample::readMotor2(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Sample-Motor-2]", lst);
}
//+++ read  Sample Motor #3
QString Sample::readMotor3(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Sample-Motor-3]", lst);
}
//+++ read  Sample Motor #4
QString Sample::readMotor4(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Sample-Motor-4]", lst);
}
//+++ read  Sample Motor #5
QString Sample::readMotor5(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Sample-Motor-5]", lst);
}
//+++ compare Sample Positions
bool Sample::compareSamplePositions(const QString &n1, const QString &n2)
{
    if (!sampleNumberAsPara->isChecked())
        return true;
    if (readPositionNumber(n1) == readPositionNumber(n2))
        return true;
    else
        return false;
}
