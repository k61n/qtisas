/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Individual Header Parser for Selector
 ******************************************************************************/

#include "selector.h"

Selector::Selector(ParserHeader *parserHeaderDan, QComboBox *unitsLambdaDan, QComboBox *unitsSelectorDan,
                   QRadioButton *lambdaCalculateDan, QRadioButton *lambdaReadDan, QLineEdit *selectorFormula1Dan,
                   QLineEdit *selectorFormula2Dan)
{
    parserHeader = parserHeaderDan;
    unitsLambda = unitsLambdaDan;
    unitsSelector = unitsSelectorDan;
    lambdaCalculate = lambdaCalculateDan;
    lambdaRead = lambdaReadDan;
    selectorFormula1 = selectorFormula1Dan;
    selectorFormula2 = selectorFormula2Dan;

    connect(lambdaRead, SIGNAL(toggled(bool)), this, SLOT(readLambdaFromHeader(bool)));
}
//+++ Lambda units converter to [A]
double Selector::unitsConverterLambdaToA(double lambda)
{
    if (unitsLambda->currentIndex() == 1)
        return lambda * 10.0;
    return lambda;
}
//+++ Frequency units converter to [Hz]
double Selector::unitsConverterFrequencyToHz(double frequency, double duration)
{
    if (unitsSelector->currentIndex() == 1)
        duration = 1.0;
    else if (unitsSelector->currentIndex() == 2)
        duration = 60.0;
    return frequency / duration;
}
//+++ read Lambda in units like in Header
double Selector::readLambdalikeInHeader(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Lambda]", lst).toDouble();
}
//+++ read from DAT-file:: Delta Lambda [1]
double Selector::readDeltaLambda(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Delta-Lambda]", lst).toDouble();
}
//+++ read Frequency in units like in Header
double Selector::readFrequencylikeInHeader(const QString &Number, const QStringList &lst)
{
    return parserHeader->readNumberString(Number, "[Selector]", lst).toDouble();
}
//+++ read Lambda
double Selector::readLambda(const QString &Number, double duration, const QStringList &lst)
{
    double lambda;
    if (lambdaRead->isChecked())
        lambda = readLambdalikeInHeader(Number, lst);
    else
    {
        double f = readFrequency(Number, duration, lst); // [Hz]

        double para1 = selectorFormula1->text().toDouble();
        double para2 = selectorFormula2->text().toDouble();

        lambda = para1 / f + para2;
    }

    return unitsConverterLambdaToA(lambda); // [A]
}

//+++ read Frequency [Hz]
double Selector::readFrequency(const QString &Number, double duration, const QStringList &lst)
{
    double f = readFrequencylikeInHeader(Number, lst);
    if (f <= 0.0)
        return 0.0;
    return unitsConverterFrequencyToHz(f, duration);
}
//+++ read Rotations [1]
double Selector::readRotations(const QString &Number, double duration, const QStringList &lst)
{
    return readFrequency(Number, duration, lst) * duration;
}
// slot
void Selector::readLambdaFromHeader(bool YN)
{
    if (!YN)
    {
        selectorFormula1->setEnabled(true);
        selectorFormula2->setEnabled(true);
    }
    else
    {
        selectorFormula1->setEnabled(false);
        selectorFormula2->setEnabled(false);
    }
}
