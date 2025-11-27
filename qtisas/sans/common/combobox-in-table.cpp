/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: QComboBox subclass
 ******************************************************************************/

#include "combobox-in-table.h"

QComboBoxInTable::QComboBoxInTable (int row, int column, QWidget *parent, const char *name)
    : QComboBox(parent), row(row), column(column)
{
    setObjectName(name);
    connect(this, &QComboBox::currentTextChanged, this, &QComboBoxInTable::reEmitActivated);
}
 
void QComboBoxInTable::reEmitActivated(const QString & string)
{
    emit activated(row, column);
}
