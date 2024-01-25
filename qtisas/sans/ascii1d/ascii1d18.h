/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SAS data (radially averaged) Import/Export interface
 ******************************************************************************/

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
