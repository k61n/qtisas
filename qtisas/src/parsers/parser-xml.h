/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: XML parser
 ******************************************************************************/

#ifndef PARSER_XML_H
#define PARSER_XML_H

#include <QFile>
#include <QMap>
#include <QString>
#include <QXmlStreamReader>

class ParserXML
{
  public:
    static QString readEntry(const QString &file, QString code, QString xmlBase);
};


class XMLParser
{
  public:
    explicit XMLParser(const QString &filepath);
    ~XMLParser();
    QString errorString();
    QList<QString> readElement(const QString &element);
    bool hasElement(const QString &element);

  private:
    void init();

    QFile *file;
    QXmlStreamReader *reader;
    QString filePath, errorStr;
    QMap<QString, QList<QString>> content;
};

#endif
