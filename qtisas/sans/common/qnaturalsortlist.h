/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: QStringList subclass that adds support for natural sorting of
             strings via sortNatural() and sortedNatural() methods
 ******************************************************************************/

#ifndef QNATURALSORTLIST_H
#define QNATURALSORTLIST_H

#include <QFileInfo>
#include <QStringList>

class QNaturalSortList : public QStringList
{
  public:
    using QStringList::QStringList;

    static bool compareNatural(const QString &a, const QString &b)
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

    void sortNatural()
    {
        std::sort(begin(), end(), [](const QString &a, const QString &b) { return compareNatural(a, b); });
    }

    QNaturalSortList sortedNatural() const
    {
        auto copy = *this;
        copy.sortNatural();
        return copy;
    }
};
#endif
