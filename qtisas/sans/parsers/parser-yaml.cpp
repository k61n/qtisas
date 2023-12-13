/***************************************************************************
    File                 : parser-yaml.cpp
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

#include "parser-yaml.h"

//+++ next Node: initialNode[nextName]
YAML::Node ParserYAML::nextNode(YAML::Node initialNode, const QString &nextName)
{
    return initialNode[nextName.toLatin1().constData()];
}
bool ParserYAML::checkNextNode(const YAML::Node &initialNode, const QString &nextName)
{
    if (!nextNode(initialNode, nextName).IsMap())
        if (!nextNode(initialNode, nextName).IsSequence())
            if (!nextNode(initialNode, nextName).IsScalar())
                return false;
    return true;
}
//+++ return String: "1.00" or "1.00; 2.00; ... ; 1.01"
QString ParserYAML::returnString(YAML::Node node)
{
    QString s = "";
    if (node.IsSequence())
        for (std::size_t nodeIndex = 0; nodeIndex < node.size(); nodeIndex++)
            if (node[nodeIndex].IsSequence() || node[nodeIndex].IsMap())
            {
                for (std::size_t i = 0; i < node[nodeIndex].size(); i++)
                {
                    s += node[nodeIndex][i].as<std::string>().c_str();
                    s += "; ";
                }
            }
            else
            {
                s += node[nodeIndex].as<std::string>().c_str();
                s += "; ";
            }
    else if (node.IsMap())
    {
        for (std::size_t i = 0; i < node.size(); i++)
            if (node[i].IsSequence() || node[i].IsMap())
            {
                for (std::size_t nodeIndex = 0; nodeIndex < node[i].size(); nodeIndex++)
                {
                    s += node[i][nodeIndex].as<std::string>().c_str();
                    s += "; ";
                }
            }
            else
            {
                s += node[i].as<std::string>().c_str();
                s += "; ";
            }
    }
    else
        s = node.as<std::string>().c_str();

    if (s.right(2) == "; ")
        s = s.left(s.length() - 2);

    return s.simplified();
}
//+++ parsing single level
bool ParserYAML::singleLevelParsing(YAML::Node &node, const QString &nodeNameString, bool lastLevel, QString &result)
{
    result = "";

    if (node.IsSequence())
    {
        QStringList sLst = nodeNameString.split("|", QString::SkipEmptyParts);
        if (sLst.count() == 1)
        {
            for (std::size_t i = 0; i < node.size(); i++)
            {
                YAML::Node sequence = node[i];
                if (nextNode(sequence, nodeNameString).IsScalar())
                {
                    result = returnString(nextNode(sequence, nodeNameString));
                    return true;
                }

                if (nextNode(sequence, nodeNameString).IsDefined())
                {
                    if (lastLevel)
                        result = returnString(nextNode(sequence, nodeNameString));
                    node = nextNode(sequence, nodeNameString);
                    return true;
                }
            }
        }
        else if (sLst.count() == 3)
        {
            for (std::size_t i = 0; i < node.size(); i++)
            {
                YAML::Node sequence = node[i];

                if (nextNode(sequence, sLst[0]).IsDefined() &&
                    nextNode(sequence, sLst[0]).as<std::string>().c_str() == sLst[1])
                {
                    if (lastLevel)
                        result = returnString(nextNode(sequence, sLst[2]));
                    else
                        node = nextNode(sequence, sLst[2]);
                    return true;
                }
                else
                    continue;

                if (nextNode(sequence, sLst[2]).IsDefined())
                {
                    result = returnString(nextNode(sequence, nodeNameString));
                    node = nextNode(sequence, sLst[2]);
                    return true;
                }
            }
        }
        else
            return true;
    }
    else
    {
        if (!checkNextNode(node, nodeNameString))
            return false;

        node = nextNode(node, nodeNameString);

        if (node.IsScalar())
            result = returnString(node);
        else if (lastLevel && node.IsDefined())
            for (std::size_t i = 0; i < node.size(); i++)
            {
                result += returnString(node[i]);
                result += "; ";
            }

        if (result.right(2) == "; ")
            result = result.left(result.length() - 2);

        return true;
    }
    return false;
}
//+++ parsing file
QString ParserYAML::readEntry(const QString &fileNameString, QString yamlCode)
{
    yamlCode = yamlCode.remove(" ");
    yamlCode = yamlCode.replace("::", ":");
    yamlCode = yamlCode.replace("::", ":");
    yamlCode = yamlCode.replace("||", "|");
    yamlCode = yamlCode.replace("||", "|");

    QStringList lst = yamlCode.split(":", QString::SkipEmptyParts);

    int countLevels = lst.count();

    if (countLevels > 10)
        return "> 10 levels";

    std::ifstream fin(fileNameString.toLatin1().constData());

    QString result = "";

    try
    {
        auto documents = YAML::LoadAll(fin);

        bool lastLevel;

        for (std::size_t docIndex = 0; docIndex < documents.size(); docIndex++)
        {
            YAML::Node doc = documents[docIndex];

            for (int levelIndex = 0; levelIndex < countLevels; levelIndex++)
            {
                lastLevel = (levelIndex == countLevels - 1) ? true : false;

                if (!singleLevelParsing(doc, lst[levelIndex], lastLevel, result))
                    break;
                if (countLevels == levelIndex + 1)
                    return result;
            }
        }
    }
    catch (const YAML::Exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    return "";
}
bool ParserYAML::readMatrix(const QString &file, const QString &code, int numberFrames, int dimX, int dimY,
                            gsl_matrix *&matrix)
{
    QString singleLineMatrix = readEntry(file, code);

    QStringList lst = singleLineMatrix.split("; ", QString::SkipEmptyParts);

    if (lst.count() != dimX * dimY)
        return false;

    int counter = 0;
    for (int i = 0; i < dimX; i++)
        for (int j = 0; j < dimY; j++)
        {
            gsl_matrix_set(matrix, j, i, lst[counter].toDouble());
            counter++;
        }

    return true;
}