/***************************************************************************
 File : ascii1d18.h
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : ASCII.1D... SA.S Data (Radially Avereged) Import/Export Interface
 
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


#ifndef ASCII1D18_H
#define ASCII1D18_H

#include "ApplicationWindow.h"
#include "ui_ascii1d.h"
#include <gsl/gsl_matrix.h>

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

class ascii1d18 : public QWidget, private Ui::ascii1d
{
    Q_OBJECT
public:
    ascii1d18(QWidget *parent = 0);
    ~ascii1d18();

    ApplicationWindow* app();
    void toResLog( QString text );

    //+++ init
    void connectSlot();
    void sourceSelected();
    
    //+++ settings
    void readSettings();
    void writeSettings();
    
    //+++ session
    void defaultASCII1D();
    void findASCII1DFormats();
    void saveCurrentASCII1D(QString fileName);

    //+++ modify
    void sasPresentation(QString &presentation );
    
    //+++
    void loadASCIIfromFiles();
    void loadASCIIfromFiles(QStringList fileNames, bool fastPlotYN);
    void loadASCIIfromTables();
    void loadASCIIfromTables(QStringList tableNames, bool fastPlotYN);
    
    int linesNumber(const QString fn);
    int rowsNumber(const QString table);
    bool loadASCIIfromFile(const QString fn, gsl_matrix* &data, int &N, bool &sigmaExist);
    bool loadASCIIfromTable(const QString table, gsl_matrix* &data, int &N, bool &sigmaExist);
    
    int removePoints(gsl_matrix* &data, int N);
    void convertFromQI(gsl_matrix* &data, int N, int &Nfinal);
    double sigma( double Q);
    void sigmaCalculation(gsl_matrix* &data, int N);
    void applyMath(gsl_matrix* &data, QString MathChar, double MathFactor, int N);
    void convertToQI(gsl_matrix* &data, int N, int &Nfinal);
    int columnCalculation(gsl_matrix* &data, int N, int Nfinal, QString action, QString columnName);
    bool linearBinning(gsl_matrix* &data, int N, int &Nfinal);
    bool logBinning(gsl_matrix* &data, int N, int &Nfinal);
    void dataMatrixSave(QString &fn, gsl_matrix* data, int N, int Nfinal, bool loadedReso, Table* &w);
    bool plotTable(Graph *g,Table *table, QString tableName);
    QString generateTableName(QString fn);
    QString generateTableName(QString fn,   QString prefix,  int findNumberIndex, bool indexing, bool tableOutput);
    
 public slots:
    
    void loadASCIIall();
    
    //+++ path
    void buttonPath();
    void buttonPathOut();
    
    //+++ options
    void asciiFormatSelected();
    
    //+++ reso
    void checkBoxLensesChanged();
    void detColDistanceValidator();
    void checkBoxResoSlot();
    
    //+++ session
    void saveCurrentASCII1D();
    void saveNewASCII1D();
    void readCurrentASCII1D();
    void deleteASCII1D();
    
    //+++ modify
    void presentationFromChanged();
    void presentationToChanged();
    void mathControl();
    void mathControl2();
    void slotMathYN();
    
    void removeRangeControl();
    void removeLastControl();
    void removeFirstControl();
    
    void slot1DcalcYN();
    
    //+++ tools
    void filterChangedFastPlotting();
    void fastPlot();
    
    
private:
    
};

#endif
