/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Vitaliy Pipich <v.pipich@gmail.com>
Description: DAN configuration selector widget: const/table/header
 ******************************************************************************/

#ifndef DAN_CONFIGURATIN_SELECTOR_H
#define DAN_CONFIGURATIN_SELECTOR_H

#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QString>
#include <QStringList>

#include "compat.h"
#include "dan-parser-header.h"

class ConfigurationSelector : public QObject
{
    Q_OBJECT
  protected:
    ParserHeader *parserHeader;
    QRadioButton *selectConst;
    QRadioButton *selectTable;
    QRadioButton *selectHeader;
    QDoubleSpinBox *spinBoxValue;
    QLineEdit *lineEditTable;
    QString qstringHeader;

  public:
    ConfigurationSelector(ParserHeader *parserHeader, QRadioButton *selectConst, QRadioButton *selectTable,
                          QRadioButton *selectHeader, QDoubleSpinBox *spinBoxValue, QLineEdit *lineEditTable,
                          const QString &qstringHeader);

    bool readSettingsString(QString string);
    QString writeSettingsString();
    double getValue(double x = -1001, const QString &number = "-1");

  public slots:
};

#endif
