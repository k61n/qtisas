/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Selector Information
 ******************************************************************************/

#ifndef SELECTOR_H
#define SELECTOR_H

#include <QComboBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QString>

#include "dan-parser-header.h"

class Selector : public QObject
{
    Q_OBJECT
  protected:
    ParserHeader *parserHeader;
    QComboBox *unitsLambda;
    QComboBox *unitsSelector;
    QRadioButton *lambdaCalculate;
    QRadioButton *lambdaRead;
    QLineEdit *selectorFormula1;
    QLineEdit *selectorFormula2;

    double unitsConverterLambdaToA(double lambda);
    double unitsConverterFrequencyToHz(double frequency, double duration);
    double calculateLambdalikeInHeader(const QString &Number, const QStringList &lst = QStringList());

  public:
    Selector(ParserHeader *parserHeaderDan, QComboBox *unitsLambdaDan, QComboBox *unitsSelectorDan,
             QRadioButton *lambdaCalculateDan, QRadioButton *lambdaReadDan, QLineEdit *selectorFormula1Dan,
             QLineEdit *selectorFormula2Dan);
    double readLambdalikeInHeader(const QString &Number, const QStringList &lst = QStringList());
    double readDeltaLambda(const QString &Number, const QStringList &lst = QStringList());
    double readFrequencylikeInHeader(const QString &Number, const QStringList &lst = QStringList());
    double readLambda(const QString &Number, double duration, const QStringList &lst = QStringList());
    double readFrequency(const QString &Number, double duration, const QStringList &lst = QStringList());
    double readRotations(const QString &Number, double duration, const QStringList &lst = QStringList());
  public slots:
    void readLambdaFromHeader(bool YN);
};

#endif
