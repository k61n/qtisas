/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: sorting a QStringList naturally
 ******************************************************************************/

#pragma once

#include <QFileInfo>
#include <QStringList>
#include <algorithm>

namespace NaturalSort
{
// Compare two strings naturally (numbers as integers, not lexically)
inline bool compare(const QString &a, const QString &b)
{
    QString strA = QFileInfo(a).baseName();
    QString strB = QFileInfo(b).baseName();

    int i = 0, j = 0;

    while (i < strA.length() && j < strB.length())
    {
        if (strA[i].isDigit() && strB[j].isDigit())
        {
            int startI = i, startJ = j;

            while (i < strA.length() && strA[i].isDigit())
                i++;
            while (j < strB.length() && strB[j].isDigit())
                j++;

            qlonglong numA = strA.mid(startI, i - startI).toLongLong();
            qlonglong numB = strB.mid(startJ, j - startJ).toLongLong();

            if (numA != numB)
                return numA < numB;
        }
        else
        {
            if (strA[i].toLower() != strB[j].toLower())
                return strA[i].toLower() < strB[j].toLower();

            ++i;
            ++j;
        }
    }

    return strA.length() < strB.length();
}

// Sort a QStringList in-place
inline void sort(QStringList &list)
{
    std::sort(list.begin(), list.end(), compare);
}

// Return a sorted copy
inline QStringList sorted(const QStringList &list)
{
    QStringList copy = list;
    sort(copy);
    return copy;
}
}
