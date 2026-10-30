/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: XML parser
 ******************************************************************************/

#include <QDomElement>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStringList>

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

    QStringList lst = xmlCode.split(":", Qt::SkipEmptyParts);

    QDomDocument doc;
    QDomElement root;
    QDomElement element;

    //+++
    auto *xmlFile = new QFile(fileName);
    if (!xmlFile->open(QIODevice::ReadOnly))
        return "";
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    if (!doc.setContent(xmlFile, true))
#else
    if (!doc.setContent(xmlFile, QDomDocument::ParseOption::UseNamespaceProcessing))
#endif
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
                qWarning("XML file error: level # %d, %s", i, lst[i].toLocal8Bit().constData());
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
                qWarning("XML(2) file error: level # %d, %s", ii, lst[ii].toLocal8Bit().constData());
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

//+++ readEntry() the other way: its path (the first element of each name, as readEntry() finds it) walked through the
//+++ file's text; only the element's text replaced
bool ParserXML::setEntry(const QString &fileName, QString xmlCode, QString xmlBase, const QString &newValue)
{
    xmlBase = xmlBase.remove(" ");
    if (xmlBase != "")
        xmlBase = (xmlBase + ":").replace("::", ":").replace("::", ":");
    xmlCode = (xmlBase + xmlCode).remove(" ").replace("::", ":").replace("::", ":");
    if (xmlCode.count("{") > 1)
        return false;
    const QStringList lst = xmlCode.remove("{").remove("}").split(":", Qt::SkipEmptyParts);
    if (lst.isEmpty())
        return false;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QByteArray raw = file.readAll();
    file.close();
    // the offsets below are those of this text: a file in another encoding is not rewritten
    if (QRegularExpression(R"(^\s*<\?xml[^>]*encoding\s*=\s*["'](?!utf-?8["'])[^"']*["'])",
                           QRegularExpression::CaseInsensitiveOption)
            .match(QString::fromLatin1(raw.left(200)))
            .hasMatch())
        return false;
    QString text = QString::fromUtf8(raw);

    QXmlStreamReader reader(text);
    int depth = 0;
    int matched = 0;
    qint64 start = -1;
    QString qualifiedName;
    while (!reader.atEnd())
    {
        const QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement)
        {
            depth++;
            if (start >= 0)
                return false; // elements in it: no text value
            if (matched == depth - 1 && depth <= lst.count() && reader.name() == lst[depth - 1])
            {
                matched = depth;
                if (matched == lst.count())
                {
                    start = reader.characterOffset();
                    qualifiedName = reader.qualifiedName().toString();
                }
            }
        }
        else if (token == QXmlStreamReader::EndElement)
        {
            if (start >= 0)
            {
                const qint64 endTag = text.lastIndexOf("<", reader.characterOffset() - 1);
                QString value = newValue;
                value.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
                if (endTag < start) // <name/>
                {
                    const qint64 slash = text.lastIndexOf("/", start - 1);
                    text.replace(slash, start - slash, ">" + value + "</" + qualifiedName + ">");
                }
                else
                    text.replace(start, endTag - start, value);
                QSaveFile out(fileName);
                const QByteArray data = text.toUtf8();
                if (!out.open(QIODevice::WriteOnly) || out.write(data) != data.size())
                    return false;
                return out.commit();
            }
            if (matched >= depth)
                return false; // the first element of that name has not the rest of the path
            depth--;
        }
    }
    return false;
}

XMLParser::XMLParser(const QString &filePath) : filePath(filePath)
{
    file = new QFile(filePath);
    reader = new QXmlStreamReader(file);
    if (!file->open(QFile::ReadOnly | QFile::Text))
        errorStr = "Cannot open file for reading";
    else
        init();
}

XMLParser::~XMLParser()
{
    reader->clear();
    file->close();
}

QString XMLParser::errorString()
{
    if (reader && reader->hasError())
        return reader->errorString();
    return errorStr;
}

void XMLParser::init()
{
    QString key;
    while (!reader->atEnd() && !reader->hasError())
    {
        QXmlStreamReader::TokenType token = reader->readNext();
        if (token == QXmlStreamReader::StartElement)
        {
            key = reader->name().toString();
            if (!content.contains(key))
                content[key] = QList<QString>();
        }
        else if (token == QXmlStreamReader::Characters)
            content[key] << reader->text().toString();
        else if (token == QXmlStreamReader::EndElement)
            reader->readNext();
    }

}

QList<QString> XMLParser::readElement(const QString &element)
{
    return content.contains(element) ? content[element] : QList<QString>();
}

bool XMLParser::hasElement(const QString &element)
{
    return content.contains(element);
}
