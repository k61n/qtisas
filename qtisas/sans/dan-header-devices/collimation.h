/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Extraction Collimation Information
 ******************************************************************************/
#ifndef Collimation_H
#define Collimation_H

#include "dan-parser-header.h"

#include <QComboBox>
#include <QCheckBox>
#include <QString>
#include <cmath>

class Collimation : public QObject
{
    Q_OBJECT
  protected:
    ParserHeader *parserHeader;
    QComboBox *unitsCollimation;
    QCheckBox *apertureCAround;
    QCheckBox *apertureSAround;
    QComboBox *unitsAperture;

    double unitsConverterCtoCM(double C);
    QString readClikeInHeader(const QString &Number, const QStringList &lst = QStringList());

    double unitsConverterAperturetoCM(double R);
  public:
    Collimation(ParserHeader *parserHeader, QComboBox *unitsDistance, QCheckBox *arapertureCAround,
                QCheckBox *arapertureSAround, QComboBox *unitsAperture);

    double readC(const QString &Number, const QStringList &lst = QStringList());
    double readCinM(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  CA (collimation aperture)
    QString readCA(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  SA (sample aperture)
    QString readSA(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  R1 source aperture [CA-X] x [CA-X] in [cm]
    double readR1(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  R2 sample aperture [SA-X] x [SA-X] in [cm]
    double readR2(const QString &Number, const QStringList &lst = QStringList());
  public slots:
};
#endif
