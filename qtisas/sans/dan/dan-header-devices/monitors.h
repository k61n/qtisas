/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Extraction Collimation Information
 ******************************************************************************/

#ifndef MONITORS_H
#define MONITORS_H

#include <cmath>

#include <QComboBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>

#include "dan-parser-header.h"

class Monitors : public QObject
{
    Q_OBJECT
  protected:
    ParserHeader *parserHeader;
    QComboBox *unitsDuration;
    QLineEdit *deadTimeM1;
    QLineEdit *deadTimeM2;
    QLineEdit *deadTimeM3;
    QLineEdit *deadTimeDetector;
    QLineEdit *deadTimeDetectorDB;
    QRadioButton *deadTimeDetectorParalysable;
    QSpinBox *normalizationConst;
    QComboBox *normalizationType;
    //! The last run reported without a normalization (one message per run, not per image read)
    QString unnormalizedReported;

  public:
    Monitors(ParserHeader *parserHeader, QComboBox *unitsDuration, QLineEdit *deadTimeM1, QLineEdit *deadTimeM2,
             QLineEdit *deadTimeM3, QLineEdit *deadTimeDetector, QLineEdit *deadTimeDetectorDB,
             QRadioButton *deadTimeDetectorParalysable, QSpinBox *normalizationConst, QComboBox *normalizationType);
    //+++ Duration units converter to [sec]
    double unitsConverterDurationToSec(double duration);
    //+++ read duration [sec]
    double readDuration(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  Monitor1
    double readMonitor1(const QString &Number, double deadTime = -1, const QStringList &lst = QStringList());
    //+++ read  Monitor2
    double readMonitor2(const QString &Number, double deadTime = -1, const QStringList &lst = QStringList());
    //+++ read  Monitor3
    double readMonitor3(const QString &Number, double deadTime = -1, const QStringList &lst = QStringList());
    //+++ read Normalized Monitor3
    double readMonitor3Normalized(const QString &Number, const QStringList &lst = QStringList());
    //+++ read Sum
    double readSum(const QString &Number, double deadTime = 0, const QStringList &lst = QStringList());
    //+++ exp- dead-time
    double deadTimeFactor(double I, double tau);
    //+++ Dead Time Correction factor (detector)
    double deadTimeFactorDetector(const QString &Number);
    //+++ Dead Time Correction factor (DB)
    double deadTimeFactorDetectorDB(const QString &Number);
    //+++ Normalization Factor
    double normalizationFactor(const QString &Number, const QStringList &lst = QStringList());
    //+++ A new processing reports every run that cannot be normalized again
    void clearReported();
    //+++ Timefactor  [1]
    double readTimefactor(const QString &Number, const QStringList &lst = QStringList());
    //+++ Timefactor numberRepetitions [1]
    int readNumberRepetitions(const QString &Number, const QStringList &lst = QStringList());
    //++++ Normalization Factor RT
    double normalizationFactorRT(const QString &Number, const QStringList &lst = QStringList());
  public slots:
  signals:
    //! A line for the results log (a run that cannot be normalized)
    void message(const QString &text);
};

#endif
