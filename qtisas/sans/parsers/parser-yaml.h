/***************************************************************************
    File                 : parser-yaml.h
    Project              : QtiSAS
    --------------------------------------------------------------------
    Copyright            : (C) 2023 by Vitaliy Pipich
    Email (use @ for *)  : v.pipich*gmail.com
    Description          : YAML parser

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
#ifndef PARSER_YAML_H
#define PARSER_YAML_H

#include <QString>
#include <QStringList>

#include <fstream>
#include <iostream>

#include "yaml-cpp/yaml.h"
#include <gsl/gsl_matrix.h>

class ParserYAML
{
  protected:
    static YAML::Node nextNode(YAML::Node initialNode, const QString &nextName);
    static bool checkNextNode(const YAML::Node &initialNode, const QString &nextName);
    static QString returnString(YAML::Node node);

  public:
    static bool singleLevelParsing(YAML::Node &node, const QString &nodeNameString, bool lastLevel, QString &result);
    static QString readEntry(const QString &file, QString code);
    static bool readMatrix(const QString &file, const QString &code, int numberFrames, int dimX, int dimY,
                           gsl_matrix *&matrix);
    static bool nodeModify(YAML::Node &node, QStringList lst, bool numerical, const QString &newValue,
                           int lstValue = 0);
};
#endif
