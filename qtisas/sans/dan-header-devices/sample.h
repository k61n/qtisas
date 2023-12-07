/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Extraction Collimation Information
 ******************************************************************************/
#ifndef SAMPLE_H
#define SAMPLE_H

#include "dan-parser-header.h"

#include <QComboBox>
#include <QCheckBox>
#include <cmath>

class Sample : public QObject
{
    Q_OBJECT
  protected:
    ParserHeader *parserHeader;
    QComboBox *unitsThickness;
    QCheckBox *sampleNumberAsPara;

    double unitsConverterThicknessToCM(double thickness);
  public:
    Sample(ParserHeader *parserHeader, QComboBox *unitsThickness, QCheckBox *sampleNumberAsPara);

    //+++ read  CA (collimation aperture)
    QString readName(const QString &Number, const QStringList &lst = QStringList());
    //+++ read Sample Position Number
    QString readPositionNumber(const QString &Number, const QStringList &lst = QStringList());
    //+++ read Experiment Title
    QString readExperimentTitle(const QString &Number, const QStringList &lst = QStringList());
    //+++ read User Name
    QString readUserName(const QString &Number, const QStringList &lst = QStringList());
    //+++ read Comment #1
    QString readComment1(const QString &Number, const QStringList &lst = QStringList());
    //+++ read Comment #2
    QString readComment2(const QString &Number, const QStringList &lst = QStringList());
    //+++ read Time
    QString readTime(const QString &Number, const QStringList &lst = QStringList());
    //+++ read Date
    QString readDate(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Thickness [cm]
    double readThickness(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Sample Motor #1
    QString readMotor1(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Sample Motor #2
    QString readMotor2(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Sample Motor #3
    QString readMotor3(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Sample Motor #4
    QString readMotor4(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Sample Motor #5
    QString readMotor5(const QString &Number, const QStringList &lst = QStringList());
    //+++ compare Sample Positions
    bool compareSamplePositions(const QString &n1, const QString &n2);
  public slots:
};
#endif
