/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Custom QTableWidgetSelectionRange class having isEmpty() method
 ******************************************************************************/

#ifndef MYSELECTION_H
#define MYSELECTION_H

#include <QTableWidgetSelectionRange>

class MySelection : public QTableWidgetSelectionRange
{
public:
    MySelection() : QTableWidgetSelectionRange() {}
    MySelection(int top, int left, int bottom, int right)
        : QTableWidgetSelectionRange(top, left, bottom, right) {}
    explicit MySelection(const QTableWidgetSelectionRange& range)
        : QTableWidgetSelectionRange(range.topRow(), range.leftColumn(),
                                     range.bottomRow(), range.rightColumn()) {}

    bool isEmpty() const;
};

#endif
