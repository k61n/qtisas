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
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QString>
#include <cmath>

class Detector : public QObject
{
    Q_OBJECT
  protected:
    ParserHeader *parserHeader;
    QComboBox *unitsDistance;
    QRadioButton *rotationX;
    QDoubleSpinBox *constValueDetRotX;
    QCheckBox *invDetRotX;
    QRadioButton *rotationY;
    QDoubleSpinBox *constValueDetRotY;
    QCheckBox *invDetRotY;
    QCheckBox *detRotAsPara;

    double unitsConverterDtoCM(double D);
    QString readDlikeInHeader(const QString &Number, const QStringList &lst = QStringList());
    QString readOffsetDlikeInHeader(const QString &Number, const QStringList &lst = QStringList());

  public:
    Detector(ParserHeader *parserHeader, QComboBox *unitsDistance, QRadioButton *rotationX,
             QDoubleSpinBox *constValueDetRotX, QCheckBox *invDetRotX, QRadioButton *rotationY,
             QDoubleSpinBox *constValueDetRotY, QCheckBox *invDetRotY, QCheckBox *detRotAsPara);
    //+++ read D = D + offset + add-Offset   [cm]
    double readD(const QString &Number, const QStringList &lst = QStringList());
    //+++ read D = D + offset + add-Offset   [m]
    double readDinM(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  DetectorX
    double readDetectorX(const QString &Number, const QStringList &lst = QStringList());
    //+++ read  DetectorY
    double readDetectorY(const QString &Number, const QStringList &lst = QStringList());
    //+++ read DetRotationX
    double readDetRotationX(const QString &Number, const QStringList &lst = QStringList());
    //+++ read DetRotationY
    double readDetRotationY(const QString &Number, const QStringList &lst = QStringList());
    //+++ compare DetRotationX/Y Position
    bool compareDetRotationPosition(const QString &n1, const QString &n2);
    //+++ read :: [Sum]
    double readSum(const QString &Number, const QStringList &lst = QStringList());
  public slots:
};
#endif