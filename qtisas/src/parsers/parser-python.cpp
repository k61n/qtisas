/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: python code parser: information extraction
 ******************************************************************************/

#include <QFile>
#include <QTextStream>

#include "parser-python.h"

//++++++++++++++++++++++++++++++++++++
//+++ Python Code: extract text block
//++++++++++++++++++++++++++++++++++++
QStringList ParserPython::extractBlock(const QString &filePath, const QString &start, const QString &finish,
                                       bool skipStartLine, bool finishFromStart)
{
    QFile file(filePath);
    QStringList result;
    bool capturing = false;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();

        if (start == finish && line.startsWith(start))
        {
            result << line.remove(start).trimmed();
            break;
        }

        if (!capturing && line.startsWith(start))
        {
            capturing = true;
            if (!skipStartLine)
                result << line.remove(start);
            if (!finishFromStart && line.endsWith(finish))
                break;
            continue;
        }

        if (capturing)
        {
            if ((finishFromStart && line.startsWith(finish)))
                break;

            result << line;
            if (!finishFromStart && line.endsWith(finish))
                break;
        }
    }

    return result;
}