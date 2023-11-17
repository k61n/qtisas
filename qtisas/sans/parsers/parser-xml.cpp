/***************************************************************************
    File                 : parser-xml.cpp
    Project              : QtiSAS
    --------------------------------------------------------------------
    Copyright            : (C) 2023 by Vitaliy Pipich
    Email (use @ for *)  : v.pipich*gmail.com
    Description          : XML parser

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

#include "parser-xml.h"

QString ParserXML::readEntry(const QString &fileName, QString xmlCode, QString xmlBase)
{
    int order = 0;
    xmlBase = xmlBase.remove(" ");
    if (xmlBase != "")
    {
        xmlBase += ":";
        xmlBase = xmlBase.replace("::", ":");
        xmlBase = xmlBase.replace("::", ":");
    }

    xmlCode = xmlBase + xmlCode;

    xmlCode = xmlCode.remove(" ");
    xmlCode = xmlCode.replace("::", ":");
    xmlCode = xmlCode.replace("::", ":");

    QStringList lst = xmlCode.split(":", QString::SkipEmptyParts);

    QDomDocument doc;
    QDomElement root;
    QDomElement element;

    QString errorStr;
    int errorLine;
    int errorColumn;

    //+++
    auto *xmlFile = new QFile(fileName);
    if (!xmlFile->open(QIODevice::ReadOnly))
        return "";
    if (!doc.setContent(xmlFile, true, &errorStr, &errorLine, &errorColumn))
        return "";

    root = doc.documentElement();

    int number = lst.count();
    if (number < 1)
    {
        xmlFile->close();
        return "";
    }

    if (number == 1)
    {
        if (root.tagName() != lst[0])
        {
            xmlFile->close();
            return "";
        }
        element = root;
        return element.text().simplified();
    }
    int numberCycles = 0;
    QList<int> positions;

    //+++ number of cycles ...
    for (int i = 1; i < number; i++)
    {
        if (lst[i].contains("{") && lst[i].contains("}"))
        {
            lst[i] = lst[i].remove("}");
            lst[i] = lst[i].remove("{");
            positions << i;
            numberCycles++;
        }
    }

    if (numberCycles > 2)
    {
        xmlFile->close();
        return "";
    }

    if (numberCycles <= 1)
    {
        if (root.tagName() != lst[0])
        {
            qWarning("XML(0-1) file error: level # 0");
            xmlFile->close();
            return "";
        }

        QDomNode node = root.firstChild();
        bool nodeFound = false;

        bool repeatedNote;
        int repeatCounter;

        for (int i = 1; i < number; i++)
        {
            repeatedNote = false;
            repeatCounter = 0;

            if (lst[i].contains("{") && lst[i].contains("}"))
            {
                repeatedNote = true;
                lst[i] = lst[i].remove("{");
                lst[i] = lst[i].remove("}");
            }

            while (!node.isNull() && !nodeFound)
            {
                if (node.toElement().tagName() == lst[i])
                {
                    if (repeatCounter < order && repeatedNote)
                    {
                        repeatCounter++;
                        node = node.nextSibling();
                    }
                    else
                    {
                        if (i < (number - 1))
                            node = node.firstChild();
                        nodeFound = true;
                    }
                }
                else
                    node = node.nextSibling();
            }
            if (!nodeFound)
            {
                qWarning("XML file error: level # %d, %s", i, lst[i].toLatin1().constData());
                xmlFile->close();
                return "";
            }
            nodeFound = false;
        }
        if (node.toElement().tagName() == lst[number - 1])
        {
            element = node.toElement();
            xmlFile->close();
            return element.text().simplified();
        }
        else
        {
            xmlFile->close();
            return "";
        }
    }

    if (numberCycles == 2)
    {
        int i = positions[0];
        if (root.tagName() != lst[0])
        {
            xmlFile->close();
            return "";
        }

        QDomNode node = root.firstChild();

        bool nodeFound = false;
        bool repeatedNote;
        int repeatCounter2 = 0;

        for (int ii = 1; ii <= positions[0]; ii++)
        {
            nodeFound = false;
            repeatedNote = false;

            if (ii == positions[0])
                repeatedNote = true;

            while (!node.isNull() && !nodeFound)
            {
                if (node.toElement().tagName() == lst[ii])
                {
                    if (repeatedNote)
                    {
                        QDomNode node2 = node;
                        bool nodeFound2 = false;
                        bool repeatedNote2;

                        for (int iii = ii; iii <= positions[1]; iii++)
                        {
                            nodeFound2 = false;
                            repeatedNote2 = false;

                            if (iii == positions[1])
                                repeatedNote2 = true;

                            while (!node2.isNull() && !nodeFound2)
                            {
                                if (node2.toElement().tagName() == lst[iii])
                                {
                                    if (repeatedNote2)
                                    {
                                        if (repeatCounter2 == order)
                                        {
                                            if (iii < number - 1)
                                            {
                                                for (int iiii = iii + 1; iiii < number; iiii++)
                                                {
                                                    node2 = node2.firstChild();
                                                    while (!node2.isNull() && node2.toElement().tagName() != lst[iiii])
                                                        node2 = node2.nextSibling();
                                                }
                                            }
                                            element = node2.toElement();
                                            xmlFile->close();
                                            return element.text().simplified();
                                        }

                                        node2 = node2.nextSibling();
                                        repeatCounter2++;
                                        if (node2.toElement().tagName() != lst[positions[1]])
                                            break;
                                    }
                                    else
                                    {
                                        nodeFound2 = true;
                                        if (iii < positions[1])
                                            node2 = node2.firstChild();
                                    }
                                }
                                else
                                {
                                    node2 = node2.nextSibling();
                                }
                            }
                            nodeFound2 = false;
                        }
                        node = node.nextSibling();
                        if (node.toElement().tagName() != lst[positions[0]])
                        {
                            xmlFile->close();
                            return "";
                        }
                    }
                    else
                    {
                        nodeFound = true;
                        if (ii < positions[0])
                            node = node.firstChild();
                    }
                }
                else
                {
                    node = node.nextSibling();
                }
            }
            if (!nodeFound)
            {
                qWarning("XML(2) file error: level # %d, %s", ii, lst[ii].toLatin1().constData());
                xmlFile->close();
                return "";
            }
            if (repeatedNote)
            {
                xmlFile->close();
                return "";
            }
            nodeFound = false;
        }
        xmlFile->close();
        return "";
    }

    xmlFile->close();
    return "";
}