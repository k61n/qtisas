/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: YAML parser
 ******************************************************************************/

#include <fstream>
#include <iostream>

#include "compat.h"
#include "parser-yaml.h"

//+++ next Node: initialNode[nextName]
YAML::Node ParserYAML::nextNode(YAML::Node initialNode, const QString &nextName)
{
    return initialNode[nextName.toLocal8Bit().constData()];
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
        QStringList sLst = nodeNameString.split("|", Qt::SkipEmptyParts);
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

    QStringList lst = yamlCode.split(":", Qt::SkipEmptyParts);

    int countLevels = lst.count();

    if (countLevels > 10)
        return "> 10 levels";

    std::ifstream fin(fileNameString.toLocal8Bit().constData());

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

    QStringList lst = singleLineMatrix.split("; ", Qt::SkipEmptyParts);

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

#define cs(x) (lst[x].toLocal8Bit().constData())
#define cl(x) (lstLast[x].toLocal8Bit().constData())

//+++ modification of a value insine node
bool ParserYAML::nodeModify(YAML::Node &node, QStringList lst, bool numerical, const QString &newValue, int lstValue)
{
    int count = lst.count();
    if (count == 0)
        return false;
    QStringList lstLast = lst[count - 1].split("|");

    switch (lst.count())
    {
    case 1:
        if (lstLast.count() == 1 && node[cs(0)])
        {
            if (node[cs(0)].IsSequence() || node[cs(0)].size() > 0)
            {
                if (numerical)
                    node[cs(0)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][lstValue] = newValue.toLocal8Bit().constData();
            }
            else
            {
                if (numerical)
                    node[cs(0)] = newValue.toDouble();
                else
                    node[cs(0)] = newValue.toLocal8Bit().constData();
            }
            return true;
        }
        if (lstLast.count() <= 0 || lstLast.count() > 3)
            return false;
        if (!node.IsSequence())
            return false;
        for (std::size_t i = 0; i < node.size(); i++)
        {
            if (!node[i].IsMap())
                continue;
            if (!node[i][cl(0)])
                continue;
            if (node[i][cl(0)].as<std::string>() == cl(1))
            {
                if (!node[i][cl(2)])
                    continue;
                if (node[i][cl(2)].IsScalar())
                {
                    if (numerical)
                        node[i][cl(2)] = newValue.toDouble();
                    else
                        node[i][cl(2)] = newValue.toLocal8Bit().constData();
                    return true;
                }
                if (!node[i][cl(2)][lstValue].IsScalar())
                    continue;
                if (numerical)
                    node[i][cl(2)][lstValue] = newValue.toDouble();
                else
                    node[i][cl(2)][lstValue] = newValue.toLocal8Bit().constData();
                return true;
            }
        }
        break;
    case 2:
        if (lstLast.count() == 1 && node[cs(0)][cs(1)])
        {
            if (node[cs(0)][cs(1)].IsSequence() || node[cs(0)][cs(1)].size() > 0)
            {
                if (numerical)
                    node[cs(0)][cs(1)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][lstValue] = newValue.toLocal8Bit().constData();
            }
            else
            {
                if (numerical)
                    node[cs(0)][cs(1)] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)] = newValue.toLocal8Bit().constData();
            }
            return true;
        }
        if (lstLast.count() <= 0 || lstLast.count() > 3)
            return false;
        if (!node[cs(0)].IsSequence())
            return false;
        for (std::size_t i = 0; i < node[cs(0)].size(); i++)
        {
            if (!node[cs(0)][i].IsMap())
                continue;
            if (!node[cs(0)][i][cl(0)])
                continue;
            if (node[cs(0)][i][cl(0)].as<std::string>() == cl(1))
            {
                if (!node[cs(0)][i][cl(2)])
                    continue;
                if (node[cs(0)][i][cl(2)].IsScalar())
                {
                    if (numerical)
                        node[cs(0)][i][cl(2)] = newValue.toDouble();
                    else
                        node[cs(0)][i][cl(2)] = newValue.toLocal8Bit().constData();
                    return true;
                }
                if (!node[cs(0)][i][cl(2)][lstValue].IsScalar())
                    continue;
                if (numerical)
                    node[cs(0)][i][cl(2)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][i][cl(2)][lstValue] = newValue.toLocal8Bit().constData();
                return true;
            }
        }
        break;
    case 3:
        if (lstLast.count() == 1 && node[cs(0)][cs(1)][cs(2)])
        {
            if (node[cs(0)][cs(1)][cs(2)].IsSequence() || node[cs(0)][cs(1)][cs(2)].size() > 0)
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][lstValue] = newValue.toLocal8Bit().constData();
            }
            else
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)] = newValue.toLocal8Bit().constData();
            }
            return true;
        }
        if (lstLast.count() <= 0 || lstLast.count() > 3)
            return false;
        if (!node[cs(0)][cs(1)].IsSequence())
            return false;
        for (std::size_t i = 0; i < node[cs(0)][cs(1)].size(); i++)
        {
            if (!node[cs(0)][cs(1)][i].IsMap())
                continue;
            if (!node[cs(0)][cs(1)][i][cl(0)])
                continue;
            if (node[cs(0)][cs(1)][i][cl(0)].as<std::string>() == cl(1))
            {
                if (!node[cs(0)][cs(1)][i][cl(2)])
                    continue;
                if (node[cs(0)][cs(1)][i][cl(2)].IsScalar())
                {
                    if (numerical)
                        node[cs(0)][cs(1)][i][cl(2)] = newValue.toDouble();
                    else
                        node[cs(0)][cs(1)][i][cl(2)] = newValue.toLocal8Bit().constData();
                    return true;
                }
                if (!node[cs(0)][cs(1)][i][cl(2)][lstValue].IsScalar())
                    continue;
                if (numerical)
                    node[cs(0)][cs(1)][i][cl(2)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][i][cl(2)][lstValue] = newValue.toLocal8Bit().constData();
                return true;
            }
        }
        break;
    case 4:
        if (lstLast.count() == 1 && node[cs(0)][cs(1)][cs(2)][cs(3)])
        {
            if (node[cs(0)][cs(1)][cs(2)][cs(3)].IsSequence() || node[cs(0)][cs(1)][cs(2)][cs(3)].size() > 0)
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][lstValue] = newValue.toLocal8Bit().constData();
            }
            else
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)] = newValue.toLocal8Bit().constData();
            }
            return true;
        }
        if (lstLast.count() <= 0 || lstLast.count() > 3)
            return false;
        if (!node[cs(0)][cs(1)][cs(2)].IsSequence())
            return false;
        for (std::size_t i = 0; i < node[cs(0)][cs(1)][cs(2)].size(); i++)
        {
            if (!node[cs(0)][cs(1)][cs(2)][i].IsMap())
                continue;
            if (!node[cs(0)][cs(1)][cs(2)][i][cl(0)])
                continue;
            if (node[cs(0)][cs(1)][cs(2)][i][cl(0)].as<std::string>() == cl(1))
            {
                if (!node[cs(0)][cs(1)][cs(2)][i][cl(2)])
                    continue;
                if (node[cs(0)][cs(1)][cs(2)][i][cl(2)].IsScalar())
                {
                    if (numerical)
                        node[cs(0)][cs(1)][cs(2)][i][cl(2)] = newValue.toDouble();
                    else
                        node[cs(0)][cs(1)][cs(2)][i][cl(2)] = newValue.toLocal8Bit().constData();
                    return true;
                }
                if (!node[cs(0)][cs(1)][cs(2)][i][cl(2)][lstValue].IsScalar())
                    continue;
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][i][cl(2)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][i][cl(2)][lstValue] = newValue.toLocal8Bit().constData();
                return true;
            }
        }
        break;
    case 5:
        if (lstLast.count() == 1 && node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)])
        {
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)].IsSequence() ||
                node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)].size() > 0)
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][lstValue] = newValue.toLocal8Bit().constData();
            }
            else
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)] = newValue.toLocal8Bit().constData();
            }
            return true;
        }
        if (lstLast.count() <= 0 || lstLast.count() > 3)
            return false;
        if (!node[cs(0)][cs(1)][cs(2)][cs(3)].IsSequence())
            return false;
        for (std::size_t i = 0; i < node[cs(0)][cs(1)][cs(2)][cs(3)].size(); i++)
        {
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][i].IsMap())
                continue;
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][i][cl(0)])
                continue;
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][i][cl(0)].as<std::string>() == cl(1))
            {
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][i][cl(2)])
                    continue;
                if (node[cs(0)][cs(1)][cs(2)][cs(3)][i][cl(2)].IsScalar())
                {
                    if (numerical)
                        node[cs(0)][cs(1)][cs(2)][cs(3)][i][cl(2)] = newValue.toDouble();
                    else
                        node[cs(0)][cs(1)][cs(2)][cs(3)][i][cl(2)] = newValue.toLocal8Bit().constData();
                    return true;
                }
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][i][cl(2)][lstValue].IsScalar())
                    continue;
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][i][cl(2)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][i][cl(2)][lstValue] = newValue.toLocal8Bit().constData();
                return true;
            }
        }
        break;
    case 6:
        if (lstLast.count() == 1 && node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)])
        {
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)].IsSequence() ||
                node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)].size() > 0)
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][lstValue] = newValue.toLocal8Bit().constData();
            }
            else
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)] = newValue.toLocal8Bit().constData();
            }
            return true;
        }
        if (lstLast.count() <= 0 || lstLast.count() > 3)
            return false;
        if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)].IsSequence())
            return false;
        for (std::size_t i = 0; i < node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)].size(); i++)
        {
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][i].IsMap())
                continue;
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][i][cl(0)])
                continue;
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][i][cl(0)].as<std::string>() == cl(1))
            {
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][i][cl(2)])
                    continue;
                if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][i][cl(2)].IsScalar())
                {
                    if (numerical)
                        node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][i][cl(2)] = newValue.toDouble();
                    else
                        node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][i][cl(2)] = newValue.toLocal8Bit().constData();
                    return true;
                }
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][i][cl(2)][lstValue].IsScalar())
                    continue;
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][i][cl(2)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][i][cl(2)][lstValue] = newValue.toLocal8Bit().constData();
                return true;
            }
        }
        break;
    case 7:
        if (lstLast.count() == 1 && node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)])
        {
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)].IsSequence() ||
                node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)].size() > 0)
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][lstValue] =
                        newValue.toLocal8Bit().constData();
            }
            else
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)] = newValue.toLocal8Bit().constData();
            }
            return true;
        }
        if (lstLast.count() <= 0 || lstLast.count() > 3)
            return false;
        if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)].IsSequence())
            return false;
        for (std::size_t i = 0; i < node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)].size(); i++)
        {
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][i].IsMap())
                continue;
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][i][cl(0)])
                continue;
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][i][cl(0)].as<std::string>() == cl(1))
            {
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][i][cl(2)])
                    continue;
                if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][i][cl(2)].IsScalar())
                {
                    if (numerical)
                        node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][i][cl(2)] = newValue.toDouble();
                    else
                        node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][i][cl(2)] = newValue.toLocal8Bit().constData();
                    return true;
                }
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][i][cl(2)][lstValue].IsScalar())
                    continue;
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][i][cl(2)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][i][cl(2)][lstValue] =
                        newValue.toLocal8Bit().constData();
                return true;
            }
        }
        break;
    case 8:
        if (lstLast.count() == 1 && node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)])
        {
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)].IsSequence() ||
                node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)].size() > 0)
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][lstValue] =
                        newValue.toLocal8Bit().constData();
            }
            else
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)] = newValue.toLocal8Bit().constData();
            }
            return true;
        }
        if (lstLast.count() <= 0 || lstLast.count() > 3)
            return false;
        if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)].IsSequence())
            return false;
        for (std::size_t i = 0; i < node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)].size(); i++)
        {
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][i].IsMap())
                continue;
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][i][cl(0)])
                continue;
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][i][cl(0)].as<std::string>() == cl(1))
            {
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][i][cl(2)])
                    continue;
                if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][i][cl(2)].IsScalar())
                {
                    if (numerical)
                        node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][i][cl(2)] = newValue.toDouble();
                    else
                        node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][i][cl(2)] =
                            newValue.toLocal8Bit().constData();
                    return true;
                }
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][i][cl(2)][lstValue].IsScalar())
                    continue;
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][i][cl(2)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][i][cl(2)][lstValue] =
                        newValue.toLocal8Bit().constData();
                return true;
            }
        }
        break;
    case 9:
        if (lstLast.count() == 1 && node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)])
        {
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)].IsSequence() ||
                node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)].size() > 0)
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][lstValue] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][lstValue] =
                        newValue.toLocal8Bit().constData();
            }
            else
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)] =
                        newValue.toLocal8Bit().constData();
            }
            return true;
        }
        if (lstLast.count() <= 0 || lstLast.count() > 3)
            return false;
        if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)].IsSequence())
            return false;
        for (std::size_t i = 0; i < node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)].size(); i++)
        {
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][i].IsMap())
                continue;
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][i][cl(0)])
                continue;
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][i][cl(0)].as<std::string>() == cl(1))
            {
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][i][cl(2)])
                    continue;
                if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][i][cl(2)].IsScalar())
                {
                    if (numerical)
                        node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][i][cl(2)] = newValue.toDouble();
                    else
                        node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][i][cl(2)] =
                            newValue.toLocal8Bit().constData();
                    return true;
                }
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][i][cl(2)][lstValue].IsScalar())
                    continue;
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][i][cl(2)][lstValue] =
                        newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][i][cl(2)][lstValue] =
                        newValue.toLocal8Bit().constData();
                return true;
            }
        }
        break;
    case 10:
        if (lstLast.count() == 1 && node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][cs(9)])
        {
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][cs(9)].IsSequence() ||
                node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][cs(9)].size() > 0)
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][cs(9)][lstValue] =
                        newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][cs(9)][lstValue] =
                        newValue.toLocal8Bit().constData();
            }
            else
            {
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][cs(9)] = newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][cs(9)] =
                        newValue.toLocal8Bit().constData();
            }
            return true;
        }
        if (lstLast.count() <= 0 || lstLast.count() > 3)
            return false;
        if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)].IsSequence())
            return false;
        for (std::size_t i = 0; i < node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)].size(); i++)
        {
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][i].IsMap())
                continue;
            if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][i][cl(0)])
                continue;
            if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][i][cl(0)].as<std::string>() ==
                cl(1))
            {
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][i][cl(2)])
                    continue;
                if (node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][i][cl(2)].IsScalar())
                {
                    if (numerical)
                        node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][i][cl(2)] =
                            newValue.toDouble();
                    else
                        node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][i][cl(2)] =
                            newValue.toLocal8Bit().constData();
                    return true;
                }
                if (!node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][i][cl(2)][lstValue].IsScalar())
                    continue;
                if (numerical)
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][i][cl(2)][lstValue] =
                        newValue.toDouble();
                else
                    node[cs(0)][cs(1)][cs(2)][cs(3)][cs(4)][cs(5)][cs(6)][cs(7)][cs(8)][i][cl(2)][lstValue] =
                        newValue.toLocal8Bit().constData();
                return true;
            }
        }
        break;
    default:
        break;
    }
    return false;
}
