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
#include <QString>

class Collimation : public QObject
{
    Q_OBJECT
  protected:
    ParserHeader *parserHeader;
    QComboBox *unitsCollimation;

    double unitsConverterCtoCM(double C);
    QString readClikeInHeader(const QString &Number, const QStringList &lst = QStringList());

  public:
    Collimation(ParserHeader *parserHeader, QComboBox *unitsDistance);

    double readC(const QString &Number, const QStringList &lst = QStringList());
    double readCinM(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  CA (collimation aperture)
    QString readCA(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  SA (sample aperture)
    QString readSA(const QString &Number, const QStringList &lst = QStringList());
  public slots:
};
#endif
