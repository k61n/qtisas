/***************************************************************************
    File                 : parser-ascii.h
    Project              : QtiSAS
    --------------------------------------------------------------------
    Copyright            : (C) 2023 by Vitaliy Pipich
    Email (use @ for *)  : v.pipich*gmail.com
    Description          : ASCII parser(s)

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef PARSER_ASCII_H
#define PARSER_ASCII_H

#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <iostream>
class ParserASCII
{
  public:
    static QString readEntryFlexy(const QString &file, QString code, int maxLinesNumberInHeader);
    static QString readEntry(const QString &file, QString line, QStringList lst = QStringList());
    static QString stringParser(QString initText, const QString &action);
    static QString findNumberInString(QString line, int digitNumber, QString &num);
    static QString findStringInString(const QString &initialString, int stringPosisionInString,
                                      const QString &separationSymbol, QString &indexOfString);
};
#endif
