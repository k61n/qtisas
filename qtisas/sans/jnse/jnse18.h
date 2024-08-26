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

#include <fstream>
#include <iostream>
#include <string>

#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMdiArea>
#include <QProgressDialog>
#include <QSettings>
#include <QTableWidget>
#include <QTextStream>

#include "ui_jnse.h"

#include "ApplicationWindow.h"

class jnse18 : public QWidget, private Ui::jnse
{
    Q_OBJECT
public:
    jnse18(QWidget *parent = 0);
    ~jnse18();

    ApplicationWindow* app();
    void toResLog( QString text );

    // init
    void connectSlot();
/*
    void changeFixedSizeH(QWidget *obj, int H);
    void changeFixedSizeHW(QWidget *obj, int H, int W);
    void initScreenResolusionDependentParameters(int hResolusion, double sasResoScale);
*/
    
    // settings
    void readSettings();
    void writeSettings();
    
 public slots:
    
    void findHeaderTables();
    void buttomRADpath();
    void slotMakeList();
    void nseFit();
    void headerTableSeleted();
    void slotMakeListCohIncoh();
    void forceReadSettings();
    void filterFitFunctions(QStringList lst= QStringList(), bool local=true);
    
signals:
    void signalJnseFit(QString,QString,int,int);
    
private:
    
};

#endif
