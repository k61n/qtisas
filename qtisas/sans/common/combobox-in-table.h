/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: QComboBox subclass for use in QTableWidget
 ******************************************************************************/

#pragma once
#include <QComboBox>
#include <QObject>
#include <QTabWidget>

class QComboBoxInTable : public QComboBox
{
    Q_OBJECT

public:
    QComboBoxInTable(int row, int column, QWidget *parent, const char *name = nullptr);
    
signals:
    void activated (int row, int col);
    
    public slots:
    void reEmitActivated(const QString &);
    
private:
    int row;
    int column;
};
