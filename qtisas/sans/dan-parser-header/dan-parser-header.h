/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Header Parser used in DAN-SANS interface
 ******************************************************************************/
#ifndef PARSER_HEADER_H
#define PARSER_HEADER_H

#include "dan-files-manager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QTableWidget>

#include "parser-hdf5.h"
#include "parser-xml.h"
#include "parser-yaml.h"
#include "parser-ascii.h"

class ParserHeader : public QObject
{
    Q_OBJECT
  protected:
    FilesManager *filesManager;
    QTableWidget *tableHeader;
    QComboBox *HeaderFormat;
    QGroupBox *XMLgroup;
    QLineEdit *XMLbase;
    QGroupBox *FLEXIgroup;
    QCheckBox *FLEXIcheck;
    QLineEdit *FLEXIline;
    QSpinBox *HEADERlength;
    QSpinBox *HEADER2NDlength;
    QSpinBox *HEADERDATAlength;
    void initListOfHeaders();

  public:
    ParserHeader(FilesManager *filesManager, QTableWidget *tableHeader, QComboBox *HeaderFormat, QGroupBox *XMLgroup,
                 QLineEdit *XMLbase, QGroupBox *FLEXIgroup, QCheckBox *FLEXIcheck, QLineEdit *FLEXIline,
                 QSpinBox *HEADERlength, QSpinBox *HEADER2NDlength, QSpinBox *HEADERDATAlength);
    QStringList listOfHeaders;
    QString readNumberString(const QString &Number, const QString &headerName, const QStringList &lst = QStringList());
    QString readNumberString(const QString &Number, QString &pos, QString &num, const QStringList &lst = QStringList());
    ~ParserHeader() override
    {
        listOfHeaders.clear();
    };
  public slots:
    void dataFormatChanged(int format);
};
#endif
