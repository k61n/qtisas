/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: ASCII parser(s)
 ******************************************************************************/

#ifndef PARSERASCII_H
#define PARSERASCII_H

#include <QString>
#include <QStringList>

class ParserASCII
{
  public:
    static QString readEntryFlexy(const QString &file, QString &code, int &startingPosition,
                                  int maxLinesNumberInHeader);
    static QString readEntry(const QString &file, QString line, const QStringList &lst = QStringList());
    static QString stringParser(QString initText, QString &action);
    static QString findNumberInString(QString line, int digitNumber, QString &num);
    static QString findStringInString(const QString &initialString, int stringPosisionInString,
                                      const QString &separationSymbol, QString &indexOfString);
};

#endif
