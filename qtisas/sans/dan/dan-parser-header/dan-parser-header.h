/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
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
#include <QTextStream>

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
    QCheckBox *removeNonePrint;
    void initListOfHeaders();

  public:
    ParserHeader(FilesManager *filesManager, QTableWidget *tableHeader, QComboBox *HeaderFormat, QGroupBox *XMLgroup,
                 QLineEdit *XMLbase, QGroupBox *FLEXIgroup, QCheckBox *FLEXIcheck, QLineEdit *FLEXIline,
                 QSpinBox *HEADERlength, QSpinBox *HEADER2NDlength, QSpinBox *HEADERDATAlength,
                 QCheckBox *removeNonePrint);
    QStringList listOfHeaders;
    //+++ read a string from run Number
    QString readNumberString(const QString &Number, const QString &headerName, const QStringList &lst = QStringList());
    //+++ read a string from run Number with  pos and  num codes
    QString readNumberString(const QString &Number, QString &pos, QString &num, const QStringList &lst = QStringList());
    //+++ read a Header as QStringList
    bool readHeader(const QString &fileName, const int &linesNumber, QStringList &header);
    //+++ replaceHeaderValue
    bool replaceHeaderValue(const QString &Number, const QString &headerName, QStringList &lst,
                            const QString &newValue);
    //+++ replace value
    bool replaceEntryYaml(const QString &fileNameString, const QString &headerName, bool numerical,
                          const QString &newValue);
    ~ParserHeader() override
    {
        listOfHeaders.clear();
    };
  public slots:
    void dataFormatChanged(int format);
};
#endif
