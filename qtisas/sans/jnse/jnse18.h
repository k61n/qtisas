/***************************************************************************
 File : jnse18.h
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : JNSE Data Reading Interface
 
 ***************************************************************************/

/***************************************************************************
 * *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or  *
 *  (at your option) any later version.*
 * *
 *  This program is distributed in the hope that it will be useful,*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  *
 *  GNU General Public License for more details.   *
 * *
 *   You should have received a copy of the GNU General Public License *
 *   along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,*
 *   Boston, MA  02110-1301  USA   *
 * *
 ***************************************************************************/


#ifndef JNSE18_H
#define JNSE18_H

#include "ApplicationWindow.h"
#include "ui_jnse.h"

#include <QTableWidget>
#include <QSettings>
#include <QLabel>
#include <QDir>
#include <QInputDialog>
#include <QTextStream>
#include <QFileDialog>
#include <QProgressDialog>
#include <QMdiArea>

#include <iostream>
#include <fstream>
#include <string>

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
    void setFontForce(QFont font);
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
