/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Extruction Detector Information
 ******************************************************************************/
#ifndef DETECTOR_H
#define DETECTOR_H

#include "dan-parser-header.h"

#include <QComboBox>
#include <QString>

class Detector : public QObject
{
    Q_OBJECT
  protected:
    ParserHeader *parserHeader;
    QComboBox *unitsDistance;

    double unitsConverterDtoCM(double D);
    QString readDlikeInHeader(const QString &Number, const QStringList &lst = QStringList());
    QString readOffsetDlikeInHeader(const QString &Number, const QStringList &lst = QStringList());

  public:
    Detector(ParserHeader *parserHeader, QComboBox *unitsDistance);

    double readD(const QString &Number, const QStringList &lst = QStringList());
    double readDinM(const QString &Number, const QStringList &lst = QStringList());
  public slots:
};
#endif
