/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: YAML parser
 ******************************************************************************/

#ifndef PARSERYAML_H
#define PARSERYAML_H

#include <QString>
#include <QStringList>

#include <gsl/gsl_matrix.h>

#include <yaml-cpp/yaml.h>

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
