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
#include <QString>
#include <QStringList>
#include <QDomElement>

#include <gsl/gsl_matrix.h>

class ParserXML
{
  public:
    static QString readEntry(const QString &file, QString code, QString xmlBase);
};
#endif
