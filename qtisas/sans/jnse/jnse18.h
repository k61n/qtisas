/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: JNSE data reading interface
 ******************************************************************************/

#ifndef JNSE18_H
#define JNSE18_H

#include "ApplicationWindow.h"
#include "ui_jnse.h"

class jnse18 : public QWidget, private Ui::jnse
{
    Q_OBJECT

  public:
    explicit jnse18(QWidget *parent = nullptr);
    ~jnse18() override;

    ApplicationWindow *app();

    void connectSlot();
    void readSettings();
    void writeSettings();

  public slots:
    void findHeaderTables();
    void buttomRADpath();
    void readNseData();
    void startNseFit();
    void headerTableSeleted();
    void filterFitFunctions(const QStringList &lst = {}, bool local = true);

  signals:
    void signalJnseFit(const QString &tableName, const QString &function, int from, int to);

  private:
};

#endif
