/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Custom QTableWidget class
 ******************************************************************************/

#include "Header.h"
#include "Mytable.h"

MyTable::MyTable(QWidget *parent) : QTableWidget(parent)
{
    Header *vh = new Header(Qt::Vertical, this);
    vh->setEnabled(true);
    vh->setSectionsClickable(true);
    vh->setSectionsMovable(true);
    setVerticalHeader(vh);
    Header *hh = new Header(Qt::Horizontal, this);
    hh->setEnabled(true);
    hh->setSectionsClickable(true);
    hh->setSectionsMovable(true);
    setHorizontalHeader(hh);
}

MyTable::MyTable(int numRows, int numCols, QWidget *parent)
    : QTableWidget(numRows, numCols, parent)
{
    Header *vh = new Header(Qt::Vertical, this);
    vh->setEnabled(true);
    vh->setSectionsClickable(true);
    vh->setSectionsMovable(true);
    setVerticalHeader(vh);
    Header *hh = new Header(Qt::Horizontal, this);
    hh->setEnabled(true);
    hh->setSectionsClickable(true);
    hh->setSectionsMovable(true);
    setHorizontalHeader(hh);
}

bool MyTable::isRowSelected(int row, bool full)
{
    QList<MySelection> selectedRanges = this->selectedRanges();
        foreach (const MySelection& range, selectedRanges)
            if (row >= range.topRow() && row <= range.bottomRow()) {
                if (full) {
                    bool rowSelected = true;
                    for (int col = range.leftColumn(); col <= range.columnCount(); col++) {
                        QTableWidgetItem *item = this->item(row, col);
                        if (!item || !item->isSelected()) {
                            rowSelected = false;
                            break;
                        }
                    }
                    return rowSelected;
                } else {
                    for (int col = range.leftColumn(); col <= range.columnCount(); col++) {
                        QTableWidgetItem *item = this->item(row, col);
                        if (item && item->isSelected())
                            return true;
                    }
                }
            }
    return false;
}

bool MyTable::isColumnSelected(int column, bool full)
{
    bool isSelected;
    if (full) {
        isSelected = true;
        for (int row = 0; row < rowCount(); row++) {
            if (!this->item(row, column))
                setItem(row, column,new QTableWidgetItem());
            QTableWidgetItem *it = this->item(row, column);
            if (!it || !it->isSelected()) {
                isSelected = false;
                break;
            }
        }
        return isSelected;
    } else {
        for (int row = 0; row < rowCount(); row++) {
            QTableWidgetItem *it = this->item(row, column);
            if (it && it->isSelected())
                return true;
        }
    }
}

QList<MySelection> MyTable::selectedRanges() const
{
    QList<QTableWidgetSelectionRange> ranges = QTableWidget::selectedRanges();
    QList<MySelection> myRanges;
    foreach (const QTableWidgetSelectionRange& range, ranges)
        myRanges.append(MySelection(range));
    return myRanges;
}

MySelection MyTable::currentSelection()
{
    MySelection currentRange;
    QList<MySelection> ranges = this->selectedRanges();
    if (!ranges.empty())
        foreach (const MySelection& range, ranges)
            if (currentColumn() >= range.leftColumn() && currentColumn() <= range.rightColumn() && currentRow() <= range.bottomRow() && currentRow() >= range.topRow()) {
                currentRange = range;
                break;
            }
    return currentRange;
}

bool MyTable::isColumnReadOnly(int col)
{
    bool isReadOnly = true;
    for (int row = 0; row < rowCount(); row++) {
        if (!item(row, col))
            setItem(row, col, new QTableWidgetItem());
        if (item(row, col)->flags().testFlag(Qt::ItemIsEditable)) {
            isReadOnly = false;
            break;
        }
    }
    return isReadOnly;
}

void MyTable::swapColumns(int col1, int col2)
{
    QTableWidgetItem *it;
    for (int row = 0; row < rowCount(); row++) {
        it = item(row, col1);
        setItem(row, col1, item(row, col2));
        setItem(row, col2, it);
    }
}

void MyTable::swapRows(int row1, int row2)
{
    QTableWidgetItem *it;
    for (int col = 0; col < columnCount(); col++) {
        it = item(row1, col);
        setItem(row1, col, item(row2, col));
        setItem(row2, col, it);
    }
}

void MyTable::setColumnReadOnly(int col, bool ro)
{
    for (int row = 0; row < rowCount(); row++)
        if (ro)
            item(row, col)->setFlags(item(row, col)->flags() & ~Qt::ItemIsEditable);
        else
            item(row, col)->setFlags(item(row, col)->flags() | Qt::ItemIsEditable);
}

QString MyTable::text(int row, int col)
{
    if (item(row, col))
        return item(row, col)->text();
    else
        return {};
}

void MyTable::setText(int row, int col, const QString& text)
{
    if (!item(row, col)) {
        QTableWidgetItem *it = new QTableWidgetItem(text);
        setItem(row, col, it);
    } else
        item(row, col)->setText(text);
}

bool MyTable::eventFilter(QObject *object, QEvent *e)
{
    return parent()->eventFilter(object, e);
}
