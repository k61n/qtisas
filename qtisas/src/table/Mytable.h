/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Custom QTableWidget class
 ******************************************************************************/

#ifndef MYTABLE_H
#define MYTABLE_H

#include <QTableWidget>

#include "Myselection.h"

class MyTable : public QTableWidget
{
    Q_OBJECT

public:
    MyTable(QWidget *parent = nullptr);
    MyTable(int numRows, int numCols, QWidget *parent = nullptr);

    bool isRowSelected(int row, bool full=false);
    bool isColumnSelected(int column, bool full=false);
    QList<MySelection> selectedRanges() const;
    MySelection currentSelection();
    bool isColumnReadOnly(int col);
    void swapColumns(int col1, int col2);
    void swapRows(int row1, int row2);
    void setColumnReadOnly(int col, bool ro);
    QString text(int row, int col);
    void setText(int row, int col, const QString& text);
    bool eventFilter(QObject *object, QEvent *e);
};

#endif
