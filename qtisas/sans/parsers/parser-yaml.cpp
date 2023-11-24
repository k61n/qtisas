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

QString ParserYAML::readEntry(const QString &fileNameString, QString yamlCode)
{
    yamlCode = yamlCode.remove(" ");
    yamlCode = yamlCode.replace("::", ":");
    yamlCode = yamlCode.replace("::", ":");
    yamlCode = yamlCode.replace("||", "|");
    yamlCode = yamlCode.replace("||", "|");

    QStringList lst = yamlCode.split(":", QString::SkipEmptyParts);

    int countLevels = lst.count();

    if (countLevels > 4)
        return "-4";

    std::ifstream fin(fileNameString.toLatin1().constData());

    try
    {
        YAML::Parser parser(fin);
        YAML::Node doc;

        while (parser.GetNextDocument(doc))
        {
            // first level
            std::string key, value;

            for (YAML::Iterator it = doc.begin(); it != doc.end(); ++it)
            {
                key = "";
                value = "";
                it.first() >> key;
                if (key != lst[0].toLatin1().constData())
                    continue;
                if (countLevels == 1)
                {
                    it.second() >> value;
                    return value.c_str();
                }
                // second level
                std::string key2, value2;
                for (YAML::Iterator it2 = it.second().begin(); it2 != it.second().end(); ++it2)
                {
                    key2 = "";
                    value2 = "";
                    it2.first() >> key2;
                    if (key2 != lst[1].toLatin1().constData())
                        continue;
                    if (countLevels == 2)
                    {
                        it2.second() >> value2;
                        return value2.c_str();
                    }
                    // third level
                    std::string key3, value3;

                    if (it2.second().Type() == YAML::NodeType::Sequence)
                    {
                        bool foundYN = false;
                        std::string key3a, value3a;
                        std::string tvalue;
                        QStringList sLst = lst[2].split("|", QString::SkipEmptyParts);
                        if (countLevels > 3)
                            return "limit-3-levels";
                        if (sLst.size() == 1)
                        {
                            for (unsigned int i = 0; i < it2.second().size(); i++)
                            {
                                it2.second()[i].begin().first() >> key3a;
                                it2.second()[i].begin().second() >> key3;
                                if (key3a == sLst[0].toLatin1().constData())
                                    return key3.c_str();
                            }
                            return "not-found";
                        }
                        else
                        {
                            for (unsigned int i = 0; i < it2.second().size(); i++)
                            {
                                tvalue = "";
                                for (YAML::Iterator it3 = it2.second()[i].begin(); it3 != it2.second()[i].end(); ++it3)
                                {
                                    key3a = "";
                                    key3 = "";
                                    if (it3.second().Type() == YAML::NodeType::Sequence)
                                    {
                                        it3.first() >> key3a;
                                        if (sLst.count() < 3 || key3a != sLst[2].toLatin1().constData())
                                            continue;
                                        if (it3.second()[0].Type() != YAML::NodeType::Scalar)
                                            continue;
                                        for (unsigned int i = 0; i < it3.second().size(); i++)
                                        {
                                            it3.second()[i] >> key3;
                                            tvalue += key3 + "; ";
                                        }
                                        continue;
                                    }

                                    if (it3.second().Type() != YAML::NodeType::Scalar && sLst.count() < 4)
                                        continue;
                                    it3.first() >> key3a;
                                    if (it3.second().Type() == YAML::NodeType::Scalar)
                                        it3.second() >> key3;
                                    if (key3a == sLst[0].toLatin1().constData() &&
                                        key3 == sLst[1].toLatin1().constData() && sLst.count() == 3)
                                        foundYN = true;
                                    if (sLst.count() > 2 && key3a == sLst[2].toLatin1().constData())
                                        tvalue = key3;
                                    if (sLst.count() == 2)
                                        tvalue += key3 + "; ";
                                    if (key3a == sLst[2].toLatin1().constData() && sLst.count() == 4)
                                    {
                                        // third level
                                        std::string key4, value4;
                                        tvalue = "";
                                        for (YAML::Iterator it4 = it3.second().begin(); it4 != it3.second().end();
                                             ++it4)
                                        {
                                            it4.first() >> key4;
                                            // if (it4.second().Type() != YAML::NodeType::Scalar) continue;
                                            if (key4 != sLst[3].toLatin1().constData())
                                                continue;
                                            it4.second() >> value4;
                                            return value4.c_str();
                                        }
                                    }
                                }
                                if (foundYN)
                                    return tvalue.c_str();
                            }
                        }
                    }
                    else
                    {
                        for (YAML::Iterator it3 = it2.second().begin(); it3 != it2.second().end(); ++it3)
                        {
                            key3 = "";
                            value3 = "";
                            it3.first() >> key3;
                            if (key3 != lst[2].toLatin1().constData())
                                continue;
                            if (countLevels == 3)
                            {
                                it3.second() >> value3;
                                return value3.c_str();
                            }
                            // 4th level
                            std::string key4, value4;
                            if (it3.second().Type() == YAML::NodeType::Sequence)
                            {
                                bool foundYN = false;
                                std::string key4a, value4a;
                                std::string tvalue;
                                QStringList sLst = lst[3].split("|", QString::SkipEmptyParts);
                                if (countLevels > 4)
                                    return "limit-4-levels";
                                if (sLst.size() == 1)
                                {
                                    for (unsigned int i = 0; i < it3.second().size(); i++)
                                    {
                                        it3.second()[i].begin().first() >> key4a;
                                        it3.second()[i].begin().second() >> key4;
                                        if (key4a == sLst[0].toLatin1().constData())
                                            return key4.c_str();
                                    }
                                    return "not-found";
                                }
                                else
                                {
                                    for (unsigned int i = 0; i < it3.second().size(); i++)
                                    {
                                        tvalue = "";
                                        for (YAML::Iterator it4 = it3.second()[i].begin(); it4 != it3.second()[i].end();
                                             ++it4)
                                        {
                                            key4a = "";
                                            key4 = "";
                                            if (it4.second().Type() == YAML::NodeType::Sequence)
                                            {
                                                it4.first() >> key4a;
                                                if (sLst.count() < 3 || key4a != sLst[2].toLatin1().constData())
                                                    continue;
                                                if (it4.second()[0].Type() != YAML::NodeType::Scalar)
                                                    continue;
                                                for (unsigned int i = 0; i < it4.second().size(); i++)
                                                {
                                                    it4.second()[i] >> key4;
                                                    tvalue += key4 + "; ";
                                                }
                                                continue;
                                            }
                                            if (it4.second().Type() != YAML::NodeType::Scalar && sLst.count() < 4)
                                                continue;
                                            it4.first() >> key4a;
                                            if (it4.second().Type() == YAML::NodeType::Scalar)
                                                it4.second() >> key4;
                                            if (key4a == sLst[0].toLatin1().constData() &&
                                                key4 == sLst[1].toLatin1().constData() && sLst.count() == 3)
                                                foundYN = true;
                                            if (sLst.count() > 2 && key4a == sLst[2].toLatin1().constData())
                                                tvalue = key4;
                                            if (sLst.count() == 2)
                                                tvalue += key4 + "; ";

                                            if (key4a == sLst[2].toLatin1().constData() && sLst.count() == 4)
                                            {
                                                // third level
                                                std::string key5, value5;
                                                tvalue = "";
                                                for (YAML::Iterator it5 = it4.second().begin();
                                                     it5 != it4.second().end(); ++it5)
                                                {
                                                    it5.first() >> key5;
                                                    // if (it4.second().Type() != YAML::NodeType::Scalar) continue;
                                                    if (key5 != sLst[3].toLatin1().constData())
                                                        continue;
                                                    it5.second() >> value5;
                                                    return value5.c_str();
                                                }
                                            }
                                        }
                                        if (foundYN)
                                            return tvalue.c_str();
                                    }
                                }
                            }
                            else
                            {
                                for (YAML::Iterator it4 = it3.second().begin(); it4 != it3.second().end(); ++it4)
                                {
                                    if (countLevels > 4)
                                        return "limit-4-levels";
                                    key4 = "";
                                    value4 = "";
                                    it4.first() >> key4;
                                    if (key4 != lst[3].toLatin1().constData())
                                        continue;
                                    if (countLevels == 4)
                                    {
                                        it4.second() >> value4;
                                        return value4.c_str();
                                    }
                                }
                            }
                        }
                    }
                    
                }
            }
        }
    }
    catch (const YAML::Exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    return "";
}