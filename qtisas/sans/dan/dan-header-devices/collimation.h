/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Extraction Collimation Information
 ******************************************************************************/

#ifndef COLLIMATION_H
#define COLLIMATION_H

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
    QCheckBox *attenuatorAsPara;
    QCheckBox *polarizationAsPara;

    double unitsConverterCtoCM(double C);
    QString readClikeInHeader(const QString &Number, const QStringList &lst = QStringList());

    double unitsConverterAperturetoCM(double R);
  public:
    Collimation(ParserHeader *parserHeader, QComboBox *unitsDistance, QCheckBox *arapertureCAround,
                QCheckBox *arapertureSAround, QComboBox *unitsAperture, QCheckBox *attenuatorAsPara,
                QCheckBox *polarizationAsPara);
    //+++ read C [cm]
    double readC(const QString &Number, const QStringList &lst = QStringList());
    //+++ read C [m]
    double readCinM(const QString &Number, const QStringList &lst = QStringList());
    //+++ read CA (collimation aperture)
    QString readCA(const QString &Number, const QStringList &lst = QStringList());
    //+++ read SA (sample aperture)
    QString readSA(const QString &Number, const QStringList &lst = QStringList());
    //+++ read R1 source aperture [CA-X] x [CA-X] in [cm]
    double readR1(const QString &Number, const QStringList &lst = QStringList());
    //+++ read R2 sample aperture [SA-X] x [SA-X] in [cm]
    double readR2(const QString &Number, const QStringList &lst = QStringList());
    //+++ read Attenuator
    QString readAttenuator(const QString &Number, const QStringList &lst = QStringList());
    //+++ compare Attenuators
    bool compareAttenuators(const QString &n1, const QString &n2);
    //+++ read Polarization
    QString readPolarization(const QString &Number, const QStringList &lst = QStringList());
    //+++ compare Polarizations
    bool comparePolarization(const QString &n1, const QString &n2);
    //+++ read Lenses
    QString readLenses(const QString &Number, const QStringList &lst = QStringList());

  public slots:
};

#endif
