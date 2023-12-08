/***************************************************************************
 File : dan18-rawdata-tools.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : SANS Data Analisys Interface
 
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

#include <QDebug>

#include "dan18.h"
#include <Note.h>
//+++ connect Slots
void dan18::rawdataConnectSlot()
{
    connect( pushButtonNewInfoTable, SIGNAL( clicked() ), this, SLOT( newInfoTable() ) );
    connect( pushButtonMakeList, SIGNAL( clicked() ), this, SLOT( addToInfoTable() ) );
    connect( pushButtonNewInfoMatrix, SIGNAL( clicked() ), this, SLOT( newInfoMatrix() ) );
    connect( pushButtonMakeBigMatrix, SIGNAL( clicked() ), this, SLOT( slotMakeBigMatrix() ) );
    connect( pushButtonNewInfoMatrixFromTable, SIGNAL( clicked() ), this, SLOT( slotMakeBigMatrixFromTable() ) );
    
    connect( comboBoxCheck, SIGNAL( activated(const QString&) ), this, SLOT( check() ) );
    connect( lineEditCheck, SIGNAL( returnPressed() ), this, SLOT( check() ) );
    connect( comboBoxActiveFile, SIGNAL( activated(const QString&) ), this, SLOT( checkInList() ) );
    // 2022
    connect( checkBoxDirsIndir, SIGNAL( toggled(bool) ), this, SLOT( updateComboBoxActiveFolders() ) );
    connect( comboBoxActiveFolder, SIGNAL( activated(const QString&) ), this, SLOT( updateComboBoxActiveFile() ) );
    
    connect( checkBoxBigMatrixASCII, SIGNAL( stateChanged(int) ), this, SLOT( updateComboBoxActiveFile() ) );
    connect( pushButtonHeader, SIGNAL( clicked() ), this, SLOT( selectFileToHeader() ) );
    // 2017
    connect(pushButtonExtractData, SIGNAL( clicked() ), this, SLOT( extractRawData() ) );
    // 2021
    connect( checkBoxBigMatrixASCII, SIGNAL( stateChanged(int) ), this, SLOT( asciiOrRawdata() ) );
}

//++++++SLOT::newInfoTable++
void dan18::newInfoTable()
{
    bool ok;
    QString TableName = QInputDialog::getText(this,
                                              "Table's Generation: all header info", "Enter name of Table:",
                                              QLineEdit::Normal,
                                              "info-table", &ok);
    if ( !ok ||  TableName.isEmpty() )
    {
        return;
    }
    newInfoTable(TableName);
}

void dan18::newInfoTable(QString TableName)
{
    //+++ vry IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    QStringList infoTablesList;
    findTableListByLabel("Info::Table", infoTablesList);
    
    if (infoTablesList.contains(TableName)) TableName=app()->generateUniqueName(TableName+"-");
    infoTablesList<<TableName;
    infoTablesList.sort();
    comboBoxInfoTable->clear();
    
    comboBoxInfoTable->insertItems(0, infoTablesList);
    comboBoxInfoTable->setCurrentIndex(infoTablesList.indexOf(TableName));
    
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    //+++ create table
    Table* tableDat;
    
    tableDat=app()->newTable(TableName, 0,56);
    
    //+++
    tableDat->setWindowLabel("Info::Table");
    app()->setListViewLabel(tableDat->name(), "Info::Table");
    app()->updateWindowLists(tableDat);

    QStringList colType;
    //+++ Cols Names
    tableDat->setColName(itSample, "Sample");
    tableDat->setColPlotDesignation(itSample, Table::None);
    colType << "1";
    tableDat->setColName(itPolarization, "Polarization");
    tableDat->setColPlotDesignation(itPolarization, Table::None);
    colType << "1";
    tableDat->setColName(itRuns, "Runs");
    tableDat->setColPlotDesignation(itRuns, Table::X);
    colType << "1";
    tableDat->setColName(itC, "C");
    tableDat->setColPlotDesignation(itC, Table::None);
    colType << "0";
    tableDat->setColComment(itC, "[m]");
    tableDat->setColName(itD, "D");
    tableDat->setColPlotDesignation(itD, Table::None);
    colType << "0";
    tableDat->setColComment(itD, "[m]");
    tableDat->setColName(itLambda, "lambda");
    tableDat->setColPlotDesignation(itLambda, Table::None);
    colType << "0";
    tableDat->setColComment(itLambda, "[A]");
    tableDat->setColName(itBeam, "Beam");
    tableDat->setColPlotDesignation(itBeam, Table::None);
    colType << "1";
    tableDat->setColName(itSum, "Sum");
    tableDat->setColPlotDesignation(itSum, Table::Y);
    colType << "0";
    tableDat->setColName(itDuration, "Duration");
    tableDat->setColPlotDesignation(itDuration, Table::None);
    colType << "0";
    tableDat->setColComment(itDuration, "[sec]");
    tableDat->setColName(itCps, "cps");
    tableDat->setColPlotDesignation(itCps, Table::Y);
    colType << "0";
    tableDat->setColName(itDate, "Date");
    tableDat->setColPlotDesignation(itDate, Table::None);
    colType << "1";
    tableDat->setColName(itTime, "Time");
    tableDat->setColPlotDesignation(itTime, Table::None);
    colType << "1";
    tableDat->setColName(itField1, "Field-1");
    tableDat->setColPlotDesignation(itField1, Table::None);
    colType << "0";
    tableDat->setColName(itField2, "Field-2");
    tableDat->setColPlotDesignation(itField2, Table::None);
    colType << "0";
    tableDat->setColName(itField3, "Field-3");
    tableDat->setColPlotDesignation(itField3, Table::None);
    colType << "0";
    tableDat->setColName(itField4, "Field-4");
    tableDat->setColPlotDesignation(itField4, Table::None);
    colType << "0";
    tableDat->setColName(itBeamwindowX, "Beamwindow-X");
    tableDat->setColPlotDesignation(itBeamwindowX, Table::None);
    colType << "0";
    tableDat->setColName(itBeamwindowY, "Beamwindow-Y");
    tableDat->setColPlotDesignation(itBeamwindowY, Table::None);
    colType << "0";
    tableDat->setColName(itOffset, "Offset");
    tableDat->setColPlotDesignation(itOffset, Table::None);
    colType << "0";
    tableDat->setColName(itXposition, "X-Position");
    tableDat->setColPlotDesignation(itXposition, Table::None);
    colType << "0";
    tableDat->setColName(itYposition, "Y-Position");
    tableDat->setColPlotDesignation(itYposition, Table::None);
    colType << "0";
    tableDat->setColName(itSampleNr, "Sample-Nr");
    tableDat->setColPlotDesignation(itSampleNr, Table::None);
    colType << "0";
    tableDat->setColName(itMotor1, "Motor-1");
    tableDat->setColPlotDesignation(itMotor1, Table::None);
    colType << "0";
    tableDat->setColName(itMotor2, "Motor-2");
    tableDat->setColPlotDesignation(itMotor2, Table::None);
    colType << "0";
    tableDat->setColName(itThickness, "Thickness");
    tableDat->setColPlotDesignation(itThickness, Table::None);
    colType << "0";
    tableDat->setColComment(itThickness, "[cm]");
    tableDat->setColName(itBeamWinXs, "BeamWin-Xs");
    tableDat->setColPlotDesignation(itBeamWinXs, Table::None);
    colType << "0";
    tableDat->setColName(itBeamWinYs, "BeamWin-Ys");
    tableDat->setColPlotDesignation(itBeamWinYs, Table::None);
    colType << "0";
    tableDat->setColName(itBeamWinXpos, "Beamwin-X-Pos");
    tableDat->setColPlotDesignation(itBeamWinXpos, Table::None);
    colType << "0";
    tableDat->setColName(itBeamWinYpos, "Beamwin-Y-Pos");
    tableDat->setColPlotDesignation(itBeamWinYpos, Table::None);
    colType << "0";
    tableDat->setColName(itTimeFactor, "Time-Factor");
    tableDat->setColPlotDesignation(itTimeFactor, Table::None);
    colType << "0";
    tableDat->setColName(itComment1, "Comment1");
    tableDat->setColPlotDesignation(itComment1, Table::None);
    colType << "1";
    tableDat->setColName(itComment2, "Comment2");
    tableDat->setColPlotDesignation(itComment2, Table::None);
    colType << "1";
    tableDat->setColName(itName, "Name");
    tableDat->setColPlotDesignation(itName, Table::None);
    colType << "1";
    tableDat->setColName(itWho, "Who");
    tableDat->setColPlotDesignation(itWho, Table::None);
    colType << "1";
    tableDat->setColName(itSelector, "Selector");
    tableDat->setColPlotDesignation(itSelector, Table::None);
    colType << "0";
    tableDat->setColComment(itSelector, "[Hz]");
    tableDat->setColName(itMonitor1, "Monitor-1");
    tableDat->setColPlotDesignation(itMonitor1, Table::Y);
    colType << "0";
    tableDat->setColName(itMonitor2, "Monitor-2");
    tableDat->setColPlotDesignation(itMonitor2, Table::Y);
    colType << "0";
    tableDat->setColName(itMonitor3, "Monitor-3");
    tableDat->setColPlotDesignation(itMonitor3, Table::Y);
    colType << "0";
    tableDat->setColName(itFiles, "Files");
    tableDat->setColPlotDesignation(itFiles, Table::None);
    colType << "1";
    tableDat->setColName(itRTnumber, "RT-number");
    tableDat->setColPlotDesignation(itRTnumber, Table::None);
    colType << "1";
    tableDat->setColName(itRTtimefactor, "RT-Time-Factor");
    tableDat->setColPlotDesignation(itRTtimefactor, Table::None);
    colType << "1";
    tableDat->setColName(itRTrepetitions, "RT-Repetitions");
    tableDat->setColPlotDesignation(itRTrepetitions, Table::None);
    colType << "1";
    tableDat->setColName(itRTframeduration, "RT-Frame-Duration");
    tableDat->setColPlotDesignation(itRTframeduration, Table::None);
    colType << "1";
    tableDat->setColName(itSumvsmask, "Sum-vs-Mask");
    tableDat->setColPlotDesignation(itSumvsmask, Table::Y);
    colType << "0";
    tableDat->setColName(itSumvsmaskdeadtimecorrected, "Sum-vs-Mask-Dead-Time-Corrected");
    tableDat->setColPlotDesignation(itSumvsmaskdeadtimecorrected, Table::Y);
    colType << "0";
    tableDat->setColName(itQ2vsmask, "Q2-vs-Mask");
    tableDat->setColPlotDesignation(itQ2vsmask, Table::Y);
    colType << "0";
    tableDat->setColName(itAttenuator, "Attenuator");
    tableDat->setColPlotDesignation(itAttenuator, Table::None);
    colType << "1";
    tableDat->setColName(itLenses, "Lenses");
    tableDat->setColPlotDesignation(itLenses, Table::None);
    colType << "1";
    tableDat->setColName(itSlicesCount, "Slices-Count");
    tableDat->setColPlotDesignation(itSlicesCount, Table::Y);
    colType << "0";
    tableDat->setColName(itSlicesDuration, "Slices-Duration");
    tableDat->setColPlotDesignation(itSlicesDuration, Table::Y);
    colType << "0";
    tableDat->setColName(itSlicesCurrentNumber, "Slices-Current-Number");
    tableDat->setColPlotDesignation(itSlicesCurrentNumber, Table::Y);
    colType << "0";
    tableDat->setColName(itSlicesCurrentDuration, "Slices-Current-Duration");
    tableDat->setColPlotDesignation(itSlicesCurrentDuration, Table::Y);
    colType << "0";
    tableDat->setColName(itSlicesCurrentMonitor1, "Slices-Current-Monitor1");
    tableDat->setColPlotDesignation(itSlicesCurrentMonitor1, Table::Y);
    colType << "0";
    tableDat->setColName(itSlicesCurrentMonitor2, "Slices-Current-Monitor2");
    tableDat->setColPlotDesignation(itSlicesCurrentMonitor2, Table::Y);
    colType << "0";
    tableDat->setColName(itSlicesCurrentMonitor3, "Slices-Current-Monitor3");
    tableDat->setColPlotDesignation(itSlicesCurrentMonitor3, Table::Y);
    colType << "0";
    tableDat->setColName(itSlicesCurrentSum, "Slices-Current-Sum");
    tableDat->setColPlotDesignation(itSlicesCurrentSum, Table::Y);
    colType << "0";

    tableDat->setColumnTypes(colType);
    //+++
    tableDat->show();
    app()->maximizeWindow(tableDat);
    
    app()->modifiedProject();
}

//++++++SLOT::addToInfoTable++
void dan18::addToInfoTable()
{
    //+++
    QString activeTable=comboBoxInfoTable->currentText();

    //+++ vry IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    QStringList infoTablesList;
    findTableListByLabel("Info::Table", infoTablesList);
    
    
    QString TableName;
    
    if ( activeTable=="new-info-table" || infoTablesList.count()==0 || !infoTablesList.contains(activeTable))
    {
        bool ok;
        TableName = QInputDialog::getText(this,
                                          "Table's Generation: all header info", "Enter name of Table:",
                                          QLineEdit::Normal,
                                          "info-table", &ok);
        
        if ( !ok ||  TableName.isEmpty() )
        {
            return;
        }
        
        if (infoTablesList.contains(TableName)) TableName=app()->generateUniqueName(TableName+"-");
        infoTablesList<<TableName;
        infoTablesList.sort();
        comboBoxInfoTable->clear();
        
        comboBoxInfoTable->insertItems(0, infoTablesList);
        comboBoxInfoTable->setCurrentIndex(infoTablesList.indexOf(TableName));
    }
    else TableName=activeTable;
    
    
    int i;
    
    //+++ create table
    Table* tableDat;
    
    QString wildCardLocal=wildCard;
    
    if (wildCardLocal.count("#")<3)
        wildCardLocal=wildCardLocal.replace("#","*");
    else if (wildCard.count("#")>2)
    {
        QMessageBox::critical( 0, "qtiSAS", "Check >> File : Pattern (#>1)");
        return;
    }
    else if (wildCardLocal.count("*")!=1)
    {
        QMessageBox::critical( 0, "qtiSAS", "Check >> File : Pattern (* != 1)");
        return;
    }
    
    QString filter=textEditPattern->text();
    
    //+++ select files

    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Getting File Information"));
    fd->setNameFilter(filter +";; All (*.*)");
#ifdef Q_OS_MACOS
    //fd->setOptions(QFileDialog::DontUseNativeDialog); // 2021-12:: test to get select all option
#endif
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    QStringList selectedDat=fd->selectedFiles();
     
    int filesNumber= selectedDat.count();
    
    if (filesNumber==0)
    {
        QMessageBox::critical( 0, "qtiSAS", "Nothing was selected");
        return;
    }
    
    QString test=selectedDat[0];
    if ( test.contains(Dir) )
    {
        test=test.remove(Dir);
        if (!dirsInDir && test.contains("/") )
        {
            QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
            return;
        }
        else if (dirsInDir && test.count("/")>1 )
        {
            QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder:: + sub-folders");
            return;
        }
    }
    else
    {
        QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
        return;
    }
    
    int startRaw=0;
    
    wildCardLocal=wildCard;
    
    // app()->ws->hide();
    // app()->ws->blockSignals ( true );
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    //+++
    if (checkTableExistence(TableName))
    {
        //+++ Find table
        QList<MdiSubWindow *> tableList=app()->tableList();
        foreach (MdiSubWindow *t, tableList) if (t->name()==TableName)  tableDat=(Table *)t;
        //+++
        
        if (tableDat->numCols()!=56)
        {
            QMessageBox::critical( 0, "qtiSAS", "Create new table (# cols)");
            //app()->ws->show();
            //app()->ws->blockSignals ( false );
            return;
        }
        
        startRaw=tableDat->numRows();
        
    }
    else
    {
        
        tableDat=app()->newTable(TableName, filesNumber,56);
        
        //+++
        tableDat->setWindowLabel("Info::Table");
        app()->setListViewLabel(tableDat->name(), "Info::Table");
        app()->updateWindowLists(tableDat);
        
        QStringList colType;
        //+++ Cols Names
        tableDat->setColName(itSample, "Sample");
        tableDat->setColPlotDesignation(itSample, Table::None);
        colType << "1";
        tableDat->setColName(itPolarization, "Polarization");
        tableDat->setColPlotDesignation(itPolarization, Table::None);
        colType << "1";
        tableDat->setColName(itRuns, "Runs");
        tableDat->setColPlotDesignation(itRuns, Table::X);
        colType << "1";
        tableDat->setColName(itC, "C");
        tableDat->setColPlotDesignation(itC, Table::None);
        colType << "0";
        tableDat->setColComment(itC, "[m]");
        tableDat->setColName(itD, "D");
        tableDat->setColPlotDesignation(itD, Table::None);
        colType << "0";
        tableDat->setColComment(itD, "[m]");
        tableDat->setColName(itLambda, "lambda");
        tableDat->setColPlotDesignation(itLambda, Table::None);
        colType << "0";
        tableDat->setColComment(itLambda, "[A]");
        tableDat->setColName(itBeam, "Beam");
        tableDat->setColPlotDesignation(itBeam, Table::None);
        colType << "1";
        tableDat->setColName(itSum, "Sum");
        tableDat->setColPlotDesignation(itSum, Table::Y);
        colType << "0";
        tableDat->setColName(itDuration, "Duration");
        tableDat->setColPlotDesignation(itDuration, Table::None);
        colType << "0";
        tableDat->setColComment(itDuration, "[sec]");
        tableDat->setColName(itCps, "cps");
        tableDat->setColPlotDesignation(itCps, Table::Y);
        colType << "0";
        tableDat->setColName(itDate, "Date");
        tableDat->setColPlotDesignation(itDate, Table::None);
        colType << "1";
        tableDat->setColName(itTime, "Time");
        tableDat->setColPlotDesignation(itTime, Table::None);
        colType << "1";
        tableDat->setColName(itField1, "Field-1");
        tableDat->setColPlotDesignation(itField1, Table::None);
        colType << "0";
        tableDat->setColName(itField2, "Field-2");
        tableDat->setColPlotDesignation(itField2, Table::None);
        colType << "0";
        tableDat->setColName(itField3, "Field-3");
        tableDat->setColPlotDesignation(itField3, Table::None);
        colType << "0";
        tableDat->setColName(itField4, "Field-4");
        tableDat->setColPlotDesignation(itField4, Table::None);
        colType << "0";
        tableDat->setColName(itBeamwindowX, "Beamwindow-X");
        tableDat->setColPlotDesignation(itBeamwindowX, Table::None);
        colType << "0";
        tableDat->setColName(itBeamwindowY, "Beamwindow-Y");
        tableDat->setColPlotDesignation(itBeamwindowY, Table::None);
        colType << "0";
        tableDat->setColName(itOffset, "Offset");
        tableDat->setColPlotDesignation(itOffset, Table::None);
        colType << "0";
        tableDat->setColName(itXposition, "X-Position");
        tableDat->setColPlotDesignation(itXposition, Table::None);
        colType << "0";
        tableDat->setColName(itYposition, "Y-Position");
        tableDat->setColPlotDesignation(itYposition, Table::None);
        colType << "0";
        tableDat->setColName(itSampleNr, "Sample-Nr");
        tableDat->setColPlotDesignation(itSampleNr, Table::None);
        colType << "0";
        tableDat->setColName(itMotor1, "Motor-1");
        tableDat->setColPlotDesignation(itMotor1, Table::None);
        colType << "0";
        tableDat->setColName(itMotor2, "Motor-2");
        tableDat->setColPlotDesignation(itMotor2, Table::None);
        colType << "0";
        tableDat->setColName(itThickness, "Thickness");
        tableDat->setColPlotDesignation(itThickness, Table::None);
        colType << "0";
        tableDat->setColComment(itThickness, "[cm]");
        tableDat->setColName(itBeamWinXs, "BeamWin-Xs");
        tableDat->setColPlotDesignation(itBeamWinXs, Table::None);
        colType << "0";
        tableDat->setColName(itBeamWinYs, "BeamWin-Ys");
        tableDat->setColPlotDesignation(itBeamWinYs, Table::None);
        colType << "0";
        tableDat->setColName(itBeamWinXpos, "Beamwin-X-Pos");
        tableDat->setColPlotDesignation(itBeamWinXpos, Table::None);
        colType << "0";
        tableDat->setColName(itBeamWinYpos, "Beamwin-Y-Pos");
        tableDat->setColPlotDesignation(itBeamWinYpos, Table::None);
        colType << "0";
        tableDat->setColName(itTimeFactor, "Time-Factor");
        tableDat->setColPlotDesignation(itTimeFactor, Table::None);
        colType << "0";
        tableDat->setColName(itComment1, "Comment1");
        tableDat->setColPlotDesignation(itComment1, Table::None);
        colType << "1";
        tableDat->setColName(itComment2, "Comment2");
        tableDat->setColPlotDesignation(itComment2, Table::None);
        colType << "1";
        tableDat->setColName(itName, "Name");
        tableDat->setColPlotDesignation(itName, Table::None);
        colType << "1";
        tableDat->setColName(itWho, "Who");
        tableDat->setColPlotDesignation(itWho, Table::None);
        colType << "1";
        tableDat->setColName(itSelector, "Selector");
        tableDat->setColPlotDesignation(itSelector, Table::None);
        colType << "0";
        tableDat->setColComment(itSelector, "[Hz]");
        tableDat->setColName(itMonitor1, "Monitor-1");
        tableDat->setColPlotDesignation(itMonitor1, Table::Y);
        colType << "0";
        tableDat->setColName(itMonitor2, "Monitor-2");
        tableDat->setColPlotDesignation(itMonitor2, Table::Y);
        colType << "0";
        tableDat->setColName(itMonitor3, "Monitor-3");
        tableDat->setColPlotDesignation(itMonitor3, Table::Y);
        colType << "0";
        tableDat->setColName(itFiles, "Files");
        tableDat->setColPlotDesignation(itFiles, Table::None);
        colType << "1";
        tableDat->setColName(itRTnumber, "RT-number");
        tableDat->setColPlotDesignation(itRTnumber, Table::None);
        colType << "1";
        tableDat->setColName(itRTtimefactor, "RT-Time-Factor");
        tableDat->setColPlotDesignation(itRTtimefactor, Table::None);
        colType << "1";
        tableDat->setColName(itRTrepetitions, "RT-Repetitions");
        tableDat->setColPlotDesignation(itRTrepetitions, Table::None);
        colType << "1";
        tableDat->setColName(itRTframeduration, "RT-Frame-Duration");
        tableDat->setColPlotDesignation(itRTframeduration, Table::None);
        colType << "1";
        tableDat->setColName(itSumvsmask, "Sum-vs-Mask");
        tableDat->setColPlotDesignation(itSumvsmask, Table::Y);
        colType << "0";
        tableDat->setColName(itSumvsmaskdeadtimecorrected, "Sum-vs-Mask-Dead-Time-Corrected");
        tableDat->setColPlotDesignation(itSumvsmaskdeadtimecorrected, Table::Y);
        colType << "0";
        tableDat->setColName(itQ2vsmask, "Q2-vs-Mask");
        tableDat->setColPlotDesignation(itQ2vsmask, Table::Y);
        colType << "0";
        tableDat->setColName(itAttenuator, "Attenuator");
        tableDat->setColPlotDesignation(itAttenuator, Table::None);
        colType << "1";
        tableDat->setColName(itLenses, "Lenses");
        tableDat->setColPlotDesignation(itLenses, Table::None);
        colType << "1";
        tableDat->setColName(itSlicesCount, "Slices-Count");
        tableDat->setColPlotDesignation(itSlicesCount, Table::Y);
        colType << "0";
        tableDat->setColName(itSlicesDuration, "Slices-Duration");
        tableDat->setColPlotDesignation(itSlicesDuration, Table::Y);
        colType << "0";
        tableDat->setColName(itSlicesCurrentNumber, "Slices-Current-Number");
        tableDat->setColPlotDesignation(itSlicesCurrentNumber, Table::Y);
        colType << "0";
        tableDat->setColName(itSlicesCurrentDuration, "Slices-Current-Duration");
        tableDat->setColPlotDesignation(itSlicesCurrentDuration, Table::Y);
        colType << "0";
        tableDat->setColName(itSlicesCurrentMonitor1, "Slices-Current-Monitor1");
        tableDat->setColPlotDesignation(itSlicesCurrentMonitor1, Table::Y);
        colType << "0";
        tableDat->setColName(itSlicesCurrentMonitor2, "Slices-Current-Monitor2");
        tableDat->setColPlotDesignation(itSlicesCurrentMonitor2, Table::Y);
        colType << "0";
        tableDat->setColName(itSlicesCurrentMonitor3, "Slices-Current-Monitor3");
        tableDat->setColPlotDesignation(itSlicesCurrentMonitor3, Table::Y);
        colType << "0";
        tableDat->setColName(itSlicesCurrentSum, "Slices-Current-Sum");
        tableDat->setColPlotDesignation(itSlicesCurrentSum, Table::Y);
        colType << "0";

        tableDat->setColumnTypes(colType);
    }

    tableDat->setNumRows(startRaw + filesNumber);

    //+++
    tableDat->show();
    //+++
    QString fnameOnly;
    int skip;
    QString s,ss,nameMatrix,name2ndHeader;
    int iter;
    QStringList lst;
    int index;
    //
    QString pos,num;
    

    //+++ Progress Dialog +++
    QProgressDialog progress;
    progress.setMinimumDuration(10);
    progress.setWindowModality(Qt::WindowModal);
    progress.setRange(0,filesNumber);
    progress.setCancelButtonText("Abort");
    progress.setMinimumWidth(400);
    progress.setMaximumWidth(4000);
    progress.setLabelText("DAN - Getting File Information ...");
    progress.show();
    
    

    for(iter=startRaw; iter<(startRaw+filesNumber);iter++)
    {
        progress.setValue( iter-startRaw);
        if ( progress.wasCanceled() ) break;
        

        tableDat->setText( iter, itFiles, selectedDat[iter-startRaw]);
        lst.clear();

        //+++ header
        nameMatrix = FilesManager::findFileNumberInFileName(wildCard, selectedDat[iter - startRaw].remove(Dir));
        progress.setLabelText("current file: " + nameMatrix);

        int index=-1;
        if (nameMatrix.contains("["))
        {
            index=nameMatrix.right(nameMatrix.length()-nameMatrix.indexOf("[")).remove("[").remove("]").toInt();
        }
        if (comboBoxHeaderFormat->currentIndex()==0) readHeaderNumberFull ( nameMatrix, lst );
        
        int indexInHeader;
        name2ndHeader = nameMatrix;
        //+++ Info [itSample]
        tableDat->setText(iter, itSample, sample->readName(name2ndHeader, lst));
        //+++ Run Nuber [itRuns]
        tableDat->setText(iter,itRuns, nameMatrix);
        //+++ Collimation [itC]
        tableDat->setText(iter, itC, QString::number(int(collimation->readCinM(name2ndHeader, lst))));
        //+++ Detector Z [itD]
        tableDat->setText(iter, itD, QString::number(detector->readDinM(name2ndHeader, lst), 'f', 3));
        //+++ Lambda [itLambda]
        tableDat->setText(
            iter, itLambda,
            QString::number(selector->readLambda(name2ndHeader, monitors->readDuration(name2ndHeader, lst), lst), 'f',
                            3));
        //+++ Sum [itSum]
        tableDat->setText(iter, itSum, QString::number(monitors->readSum(name2ndHeader, 0.0, lst)));
        //+++ Duration [itDuration]
        tableDat->setText(iter, itDuration, QString::number(monitors->readDuration(name2ndHeader, lst)));
        //+++ Date [itDate]
        tableDat->setText(iter, itDate, sample->readDate(name2ndHeader, lst));
        //+++ Time [itTime]
        tableDat->setText(iter, itTime, sample->readTime(name2ndHeader, lst));
        //+++ Field-1 [itField1]
        tableDat->setText(iter, itField1, parserHeader->readNumberString(name2ndHeader, "[Field-1]", lst));
        //+++ Field-2 [itField2]
        tableDat->setText(iter, itField2, parserHeader->readNumberString(name2ndHeader, "[Field-2]", lst));
        //+++ Field-3 [itField3]
        tableDat->setText(iter, itField3, parserHeader->readNumberString(name2ndHeader, "[Field-3]", lst));
        //+++ Field-4 [itField4]
        tableDat->setText(iter, itField4, parserHeader->readNumberString(name2ndHeader, "[Field-4]", lst));
        //+++ Beamwindow_X 	[itBeamwindowX]
        tableDat->setText(
            iter, itBeamwindowX,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[CA-X]", lst).toDouble(), 'f', 2));
        //+++ Beamwindow_Y 	[itBeamwindowY]
        tableDat->setText(
            iter, itBeamwindowY,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[CA-Y]", lst).toDouble(), 'f', 2));
        //+++ Detector Offet [itOffset]
        tableDat->setText(iter, itOffset, parserHeader->readNumberString(name2ndHeader, "[C,D-Offset]", lst));
        //+++ DetectorX [itXposition]
        tableDat->setText(iter, itXposition, QString::number(detector->readDetectorX(name2ndHeader, lst), 'f', 3));
        //+++ DetectorY [itYposition]
        tableDat->setText(iter, itYposition, QString::number(detector->readDetectorY(name2ndHeader, lst), 'f', 3));
        //+++ Sample discription 	[itSampleNr]
        ss = QString::number(sample->readPositionNumber(name2ndHeader, lst).toDouble(), 'f', 0);
        tableDat->setText(iter, itSampleNr, ss);
        //+++ SMotor-1 [itMotor1]
        tableDat->setText(iter, itMotor1, sample->readMotor1(name2ndHeader, lst));
        //+++ SMotor-2 [itMotor2]
        tableDat->setText(iter, itMotor2, sample->readMotor2(name2ndHeader, lst));
        //+++ Thickness [itThickness]
        tableDat->setText(iter, itThickness, QString::number(sample->readThickness(name2ndHeader, lst), 'f', 2));
        //+++ BeamWin-Xs 	[itBeamWinXs]
        tableDat->setText(
            iter, itBeamWinXs,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[SA-X]", lst).toDouble(), 'f', 2));
        //+++ BeamWin-Ys 	[itBeamWinYs]
        tableDat->setText(
            iter, itBeamWinYs,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[SA-Y]", lst).toDouble(), 'f', 2));
        //+++ SA-Pos-X 	[itBeamWinXpos]
        tableDat->setText(iter, itBeamWinXpos, parserHeader->readNumberString(name2ndHeader, "[SA-Pos-X]", lst));
        //+++ SA-Pos-Y 	[itBeamWinYpos]
        tableDat->setText(iter, itBeamWinXpos, parserHeader->readNumberString(name2ndHeader, "[SA-Pos-Y]", lst));
        //+++ Time-Factor [itTimeFactor]
        tableDat->setText(iter, itTimeFactor, parserHeader->readNumberString(name2ndHeader, "[Duration-Factor]", lst));
        //+++ Comment1 [itComment1]
        tableDat->setText(iter, itComment1, sample->readComment1(name2ndHeader, lst));
        //+++ Comment2 		[itComment2]
        tableDat->setText(iter, itComment2, sample->readComment2(name2ndHeader, lst));
        //+++ Experimental Title 	[itName]
        tableDat->setText(iter, itName, sample->readExperimentTitle(name2ndHeader, lst));
        //+++ User Name [itWho]
        tableDat->setText(iter, itWho, sample->readUserName(name2ndHeader, lst));
        //+++ Selector [itSelector]
        tableDat->setText(iter, itSelector, parserHeader->readNumberString(name2ndHeader, "[Selector]", lst));
        //+++ Monitor1 [itMonitor1]
        tableDat->setText(iter, itMonitor1, parserHeader->readNumberString(name2ndHeader, "[Monitor-1]", lst));
        //+++ Monitor2 		[itMonitor2]
        tableDat->setText(iter, itMonitor2, parserHeader->readNumberString(name2ndHeader, "[Monitor-2]", lst));
        //+++ Monitor3 		[itMonitor3]
        tableDat->setText(iter, itMonitor3, parserHeader->readNumberString(name2ndHeader, "[Monitor-3|Tr|ROI]", lst));
        //+++ RT-Current-Number [itRTnumber]
        tableDat->setText(iter, itRTnumber, QString::number(readRtCurrentNumber(lst, index, name2ndHeader)));
        //+++ RT-Time-Factor [itRTtimefactor]
        tableDat->setText(iter, itRTtimefactor, QString::number(monitors->readTimefactor(name2ndHeader, lst)));
        //+++ RT-Number-Frames [itRTrepetitions]
        tableDat->setText(iter, itRTrepetitions, QString::number(monitors->readNumberRepetitions(name2ndHeader, lst)));
        //+++ Attenuator
        tableDat->setText(iter, itAttenuator, parserHeader->readNumberString(name2ndHeader, "[Attenuator]", lst));
        //+++ Polarization
        tableDat->setText(iter, itPolarization, parserHeader->readNumberString(name2ndHeader, "[Polarization]", lst));
        //+++ Lenses
        tableDat->setText(iter, itLenses, parserHeader->readNumberString(name2ndHeader, "[Lenses]", lst));
        //+++ Slices-Count
        tableDat->setText(
            iter, itSlicesCount,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[Slices-Count]", lst).toDouble(), 'f', 0));
        //+++ Slices-Duration
        tableDat->setText(iter, itSlicesDuration, readSlicesDuration(lst, index, name2ndHeader));
        //+++ Slices-Current-Number
        tableDat->setText(
            iter, itSlicesCurrentNumber,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Number]", lst).toDouble(),
                            'f', 0));
        //+++ Slices-Current-Duration
        tableDat->setText(
            iter, itSlicesCurrentDuration,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Duration]", lst).toDouble(),
                            'f', 0));
        //+++ Slices-Current-Monitor1
        tableDat->setText(
            iter, itSlicesCurrentMonitor1,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Monitor1]", lst).toDouble(),
                            'f', 0));
        //+++ Slices-Current-Monitor2
        tableDat->setText(
            iter, itSlicesCurrentMonitor2,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Monitor2]", lst).toDouble(),
                            'f', 0));
        //+++ Slices-Current-Monitor3
        tableDat->setText(
            iter, itSlicesCurrentMonitor3,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Monitor3]", lst).toDouble(),
                            'f', 0));
        //+++ Slices-Current-Sum
        tableDat->setText(
            iter, itSlicesCurrentSum,
            QString::number(parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Sum]", lst).toDouble(), 'f',
                            0));
        // cps
        double cps = tableDat->text(iter, itSum).toDouble() / tableDat->text(iter, itDuration).toDouble() *
                     tableDat->text(iter, itRTtimefactor).toDouble(); // 2015 RT-TOF
        tableDat->setText(iter, itCps, QString::number(cps));         //+1

        // integral
        if (checkBoxSumVsMask->isChecked())
        {
            tableDat->setText( iter, itSumvsmask,QString::number(integralVSmaskSimmetrical( nameMatrix )) );// integralVSmaskUniByName( selectedDat[iter-startRaw]) );
            tableDat->setText( iter, itSumvsmaskdeadtimecorrected, QString::number(integralVSmaskUniDeadTimeCorrected( nameMatrix )));
            tableDat->setText( iter, itQ2vsmask, QString::number(Q2_VS_maskSimmetrical( nameMatrix )));
        }
        else
        {
            tableDat->setText( iter, itSumvsmask, "" );
            tableDat->setText( iter, itSumvsmaskdeadtimecorrected, "" );	    
            tableDat->setText( iter, itQ2vsmask, "" ); 
        }
        // Frame Duration
        double frameDuration=tableDat->text(iter,itDuration).toDouble();
        frameDuration*=tableDat->text(iter,itRTrepetitions).toDouble();
        if (tableDat->text(iter,itRTtimefactor).toDouble()!=0) frameDuration/=tableDat->text(iter,itRTtimefactor).toDouble();
        tableDat->setText(iter,itRTframeduration,QString::number(frameDuration));//new
        
        // Beam Size
        s = collimation->readCA(name2ndHeader, lst) + "|" + collimation->readSA(name2ndHeader, lst);
        tableDat->setText(iter, itBeam, s);
        
        // real D
        //tableDat->setText(iter,3,QString::number(tableDat->text(iter,3).toDouble()+tableDat->text(iter,17).toDouble(),'f',2));
        
            
        if(checkBoxDatYesNo->isChecked())    
        {
            if (checkBoxSortOutputToFolders->isChecked())
            {
                app()->changeFolder("DAN :: rawdata");
            }
            
            gsl_matrix *data=gsl_matrix_alloc(MD,MD);
            gsl_matrix_set_all(data, 0.0);
            
            readMatrix( nameMatrix, data );
            
            nameMatrix=nameMatrix.replace("_","-").replace("/","-").replace("\\","-").replace("%","-").replace(",","-");	    
            
            makeMatrixSymmetric(data,"Matrix-"+nameMatrix, "Rawdata::Matrix", MD );
            
            gsl_matrix_free(data);
        }
        
    }
    progress.cancel();
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    //app()->ws->show();
    //app()->ws->blockSignals ( false );
    
    maximizeWindow(TableName);
    
    int tt;    
    // adjust columns
    if (checkBoxShortList->isChecked()) 
    {
        
        for (tt=14; tt<21; tt++)
        {
            tableDat->table()->hideColumn(tt);
        }
        for (tt=24; tt<tableDat->numCols(); tt++)
        {
            tableDat->table()->hideColumn(tt);
        }
    }
    for (tt=0; tt<tableDat->numCols(); tt++)
    {
        if (!checkBoxShortList->isChecked()) tableDat->table()->showColumn(tt);
        tableDat->table()->resizeColumnToContents(tt);
        tableDat->table()->setColumnWidth(tt, tableDat->table()->columnWidth(tt)+10); 
    }
    
    int sampleLength=0;
    for(i=0;i<tableDat->numRows();i++)
    {
        s=tableDat->text(i,0);
        if (s.length()>sampleLength ) sampleLength=s.length();
    }
    
    
    for(i=0;i<tableDat->numRows();i++)
    {
        s=tableDat->text(i,0);
        while(s.length()<sampleLength)s+=" ";
        tableDat->setText(i,0,s);
    }    
    
}

//++++++SLOT::newInfoMatrix +++
void dan18::newInfoMatrix()
{
    //+++
    QString activeMatrix=comboBoxInfoMatrix->currentText();
    
    bool ok;
    QString TableName = QInputDialog::getText(this,
                                              "Big-Matrix's Generation: All-in-one", "Enter name of Matrix:",
                                              QLineEdit::Normal,
                                              "info-matrix", &ok);
    if ( !ok ||  TableName.isEmpty() )
    {
        return;
    }
    
    //+++ vry IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    QStringList infoMatrixList;
    findMatrixListByLabel("[1,1]", infoMatrixList);
    
    if (infoMatrixList.contains(TableName)) TableName=app()->generateUniqueName(TableName+"-");
    infoMatrixList<<TableName;
    infoMatrixList.sort();
    comboBoxInfoMatrix->clear();
    
    comboBoxInfoMatrix->insertItems(0, infoMatrixList);
    comboBoxInfoMatrix->setCurrentIndex(infoMatrixList.indexOf(TableName));
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: rawdata");
    }
    
    // cols &rows
    int cols=spinBoxBigMatrixCols->value();
    int rows=1;
    
    //ROI
    int xFirst=0;
    int yFirst=0;
    int xLast=MD-1;
    int yLast=MD-1;
    if (checkBoxBigMatrixROI->isChecked() && groupBoxMask->isChecked())
    {
        
        xFirst=spinBoxLTx->value()-1;
        if (xFirst<0) xFirst=0;
        yFirst=spinBoxLTy->value()-1;
        if (yFirst<0) yFirst=0;
        
        xLast=spinBoxRBx->value()-1;
        if (xLast>=MD) xLast=MD-1;
        
        yLast=spinBoxRBy->value()-1;
        if (yLast>=MD) yLast=MD-1;
    }
    
    
    gsl_matrix *bigMatrix=gsl_matrix_calloc(rows*(yLast-yFirst+1), cols*(xLast-xFirst+1));
    
    
    
    QString nameMatrix=TableName.replace("_","-").replace("/","-").replace("\\","-").replace("%","-").replace(",","-");
    
    
    // generate label
    QString label="[1,1]";
    
    makeMatrixUni(bigMatrix,nameMatrix, label, bigMatrix->size2, bigMatrix->size1,false, true);
    
    gsl_matrix_free(bigMatrix);
}

//++++++SLOT::Make Table++
void dan18::slotMakeBigMatrix()
{
    //+++ very IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: rawdata");
    }
    
    
    QString wildCardLocal=wildCard;
    
    if (wildCardLocal.count("#")<3)
        wildCardLocal=wildCardLocal.replace("#","*");
    else if (wildCard.count("#")>1)
    {
        QMessageBox::critical( 0, "qtiSAS", "Check >> File : Pattern (#>1)");
        return;
    }
    else if (wildCardLocal.count("*")!=1)
    {
        QMessageBox::critical( 0, "qtiSAS", "Check >> File : Pattern (* != 1)");
        return;
    }
    
    QString filter=textEditPattern->text();
    if (filter.contains("[0-9]"))
    {
        filter=filter+" "+textEditPattern->text().replace("[0-9]","[0-9][0-9]");
        filter=filter+" "+textEditPattern->text().replace("[0-9]","[0-9][0-9][0-9]");
        filter=filter+" "+textEditPattern->text().replace("[0-9]","[0-9][0-9][0-9][0-9]");
    }
    
    QString DirLocal=Dir;
    
    if (checkBoxBigMatrixASCII->isChecked())
    {
        filter="*.DAT";
        DirLocal= lineEditPathRAD->text()+"/ASCII-I/";
        DirLocal=DirLocal.replace("//","/");
    }
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",DirLocal,"*");
    
    fd->setDirectory(DirLocal);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Getting File Information"));
    if (checkBoxBigMatrixASCII->isChecked()) fd->setNameFilter("I-*-*-*.DAT;;"+filter + ";;"+ textEditPattern->text());
    else fd->setNameFilter(filter + ";;"+ textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    QStringList selectedDat=fd->selectedFiles();
    
    slotMakeBigMatrix(selectedDat);
}

//++++++SLOT::Make Table++
void dan18::slotMakeBigMatrix(QStringList selectedDat)
{
    //+++
    QString activeMatrix=comboBoxInfoMatrix->currentText();
    if (activeMatrix=="") return;
    
    QStringList infoMatrixList;
    findMatrixListByLabel("[1,1]", infoMatrixList);
    
    QString TableName;
    
    if (activeMatrix=="new-info-matrix" || infoMatrixList.count()==0 || !infoMatrixList.contains(activeMatrix))
    {
        bool ok;
        TableName = QInputDialog::getText(this,
                                          "Big-Matrix's Generation: All-in-one", "Enter name of Matrix:",
                                          QLineEdit::Normal,
                                          "info-matrix", &ok);
        if ( !ok ||  TableName.isEmpty() )
        {
            return;
        }
        
        if (infoMatrixList.contains(TableName)) TableName=app()->generateUniqueName(TableName+"-");
        infoMatrixList<<TableName;
        infoMatrixList.sort();
        infoMatrixList.prepend("new-info-matrix");
        comboBoxInfoMatrix->clear();
        
        comboBoxInfoMatrix->insertItems(0, infoMatrixList);
        comboBoxInfoMatrix->setCurrentIndex(infoMatrixList.indexOf(TableName));
    }
    else TableName=activeMatrix;
    
    QString filter=textEditPattern->text();
    if (filter.contains("[0-9]"))
    {
        filter=filter+" "+textEditPattern->text().replace("[0-9]","[0-9][0-9]");
        filter=filter+" "+textEditPattern->text().replace("[0-9]","[0-9][0-9][0-9]");
        filter=filter+" "+textEditPattern->text().replace("[0-9]","[0-9][0-9][0-9][0-9]");
    }
    
    QString DirLocal=Dir;
    
    if (checkBoxBigMatrixASCII->isChecked())
    {
        filter="*.DAT";
        DirLocal= lineEditPathRAD->text()+"/ASCII-I/";
        DirLocal=DirLocal.replace("//","/");
    }
    
    int i;
    

    int filesNumber= selectedDat.count();
    
    if (filesNumber==0)
    {
        QMessageBox::critical( 0, "qtiSAS", "Nothing was selected");
        return;
    }
    
    QString test=selectedDat[0];
    if ( test.contains(DirLocal) )
    {
        test=test.remove(DirLocal);
        if (!dirsInDir && test.contains("/") )
        {
            QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
            return;
        }
        else if (dirsInDir && test.count("/")>1 )
        {
            QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder:: + sub-folders");
            return;
        }
    }
    else
    {
        QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
        return;
    }
    
    
    int xFirst=0;
    int yFirst=0;
    int xLast=MD-1;
    int yLast=MD-1;
    // cols
    int cols=spinBoxBigMatrixCols->value();
    //ROI
    if (checkBoxBigMatrixROI->isChecked() && groupBoxMask->isChecked())
    {
        
        xFirst=spinBoxLTx->value()-1;
        if (xFirst<0) xFirst=0;
        yFirst=spinBoxLTy->value()-1;
        if (yFirst<0) yFirst=0;
        
        xLast=spinBoxRBx->value()-1;
        if (xLast>=MD) xLast=MD-1;
        
        yLast=spinBoxRBy->value()-1;
        if (yLast>=MD) yLast=MD-1;
    }
    
    
    
    // +++ initial matrix reading +++
    gsl_matrix * bigMatrixInit;
    int xDimInit;
    int yDimInit;
    QString labelInit;
    if (!make_GSL_Matrix_Uni( activeMatrix, bigMatrixInit, xDimInit, yDimInit, labelInit))
    {
        QMessageBox::critical( 0, "qtiSAS", "Could not read initial matrix");
        return;
    }
    
    int xx=1;
    int yy=1;
    
    int numberMatrixesInit=0;
    
    if (labelInit!="[1,1]")
    {
        QString ss;
        ss=labelInit.right(labelInit.length()-labelInit.lastIndexOf("["));
        ss=ss.remove(" ");
        ss=ss.remove("[");
        ss=ss.remove("]");
        
        QStringList lstXY = ss.split(",", QString::SkipEmptyParts);
        
        if (lstXY.count()!=2)
        {
            gsl_matrix_free(bigMatrixInit);
            return;
        }
        
        xx=lstXY[0].toInt();
        yy=lstXY[1].toInt();
        
        if (xx<1 || yy<1)
        {
            gsl_matrix_free(bigMatrixInit);
            return;
        }
        
        if (cols*(xLast-xFirst+1)!=xDimInit)
        {
            gsl_matrix_free(bigMatrixInit);
            QMessageBox::critical( 0, "qtiSAS", "Wrong dimension of initial matrix (x) ! Please create new matrix!");
            return;
        }
        
        if ((yy==1 &&  yy*(yLast-yFirst+1)!=yDimInit ) || (xx==1 && yy!=1 &&  (yy-1)*(yLast-yFirst+1)!=yDimInit ))
        {
            gsl_matrix_free(bigMatrixInit);
            QMessageBox::critical( 0, "qtiSAS", "Wrong dimension of initial matrix (y) ! Please create new matrix!");
            return;
        }
        
        numberMatrixesInit=cols*(yy-1)+(xx-1);
    }
    
    // ---
    int numberMatrixes=selectedDat.count()+numberMatrixesInit;
    int rows=int(numberMatrixes/cols);
    if (rows*cols<numberMatrixes) rows++;
    
    gsl_matrix *bigMatrix=gsl_matrix_calloc(rows*(yLast-yFirst+1), cols*(xLast-xFirst+1));
    
    if(xx>1|| yy>1||numberMatrixesInit>0)
    {
        for (int ix=0;ix<xDimInit;ix++) for (int iy=0;iy<yDimInit;iy++)
        {
            gsl_matrix_set(bigMatrix, iy, ix,  gsl_matrix_get(bigMatrixInit,iy,ix) ); //Matrix transfer
        }
        
    }
    
    
    if (!genetateMatrixInMatrix(selectedDat, bigMatrix, xFirst, yFirst, xLast, yLast, cols, yy-1, xx-1, numberMatrixesInit))
    {
        QMessageBox::critical( 0, "qtiSAS", "Could not generate/add final matrix");
        return;
    }
    
    QString nameMatrix=TableName.replace("_","-").replace("/","-").replace("\\","-").replace("%","-").replace(",","-");	   

    // generate label
    QString label = labelInit;

    for (int i = 0; i < selectedDat.count(); i++)
    {
        if (checkBoxBigMatrixASCII->isChecked())
            label +=
                " " +
                selectedDat[i].remove(DirLocal).remove("I-").remove(".DAT").remove("-" + comboBoxSel->currentText()) +
                "; ";
        else
            label += " " + FilesManager::findFileNumberInFileName(wildCard, selectedDat[i].remove(Dir)) + "; ";
        xx++;
        if ((double)xx / (double)cols > 1.0)
        {
            xx = 1;
            yy++;
        }
        label += "[" + QString::number(xx) + "," + QString::number(yy) + "]";
    }

    makeMatrixUni(bigMatrix,nameMatrix, label, bigMatrix->size2, bigMatrix->size1, false, true );
    
    
    gsl_matrix_free(bigMatrix);
    gsl_matrix_free(bigMatrixInit);    
    
}


//++++++SLOT::Make Table++
void dan18::slotMakeBigMatrixFromTable()
{
    //+++ very IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "Table") return;
    
    Table* t = (Table*)app()->activeWindow();
    
    int N=t->numCols();
    int M=t->numRows();
    
    QStringList list;
    QString text;
    //+++ Set Data-Sets List +++
    for (int mm = 0; mm < M; mm++)
        for (int nn = 0; nn < N; nn++)
        {
            text = t->text(mm, nn).simplified();
            qDebug() << text;
            if (text == "")
                continue;
            if (!filesManager->checkFileNumber(text))
                continue;
            list << filesManager->fileNameFull(text, wildCard);
        }
    slotMakeBigMatrix(list);
}
//+++ AdvOpt:: Check Built Functions
void dan18::check()
{
    QString Number=lineEditCheck->text().simplified(); //+++ uni-sas
    
    if(Number.contains(";"))
    {
        QStringList lstNumberIn = Number.split(";", QString::SkipEmptyParts);
        Number=lstNumberIn[0];
    }
    if (comboBoxActiveFolder->currentIndex()>0) Number=comboBoxActiveFolder->currentText()+"/"+Number;
    check(Number, false);
    
    lineEditCheck->setFocus();
}

void dan18::checkInList()
{
    QString Number=comboBoxActiveFile->currentText().simplified(); //+++ uni-sas
    
    QString oldNumber=lineEditCheck->text();
    if(oldNumber.contains(";"))
    {
        Number+=oldNumber.replace(0,oldNumber.indexOf(";"),"");
    }
    lineEditCheck->setText(Number);
    if (comboBoxActiveFolder->currentText()!=".") Number=comboBoxActiveFolder->currentText()+"/"+Number;
    check(Number, true);
    
    comboBoxActiveFile->setFocus();
}

void dan18::check(QString Number, bool fromComboBox)
{
    //+++
    QString whatToCheck=comboBoxCheck->currentText();
    check(Number, fromComboBox, whatToCheck);
}


void dan18::check(QString NumberIn, bool fromComboBox, QString whatToCheck)
{
    //+++
    bool asciiYN=checkBoxBigMatrixASCII->isChecked();
    if (!fromComboBox) asciiYN=false;
    
    QStringList lstNumberIn = NumberIn.split(";", QString::SkipEmptyParts);
    
    if (lstNumberIn.count()>1) NumberIn = lstNumberIn[0];
    
    //+++
    QString Number=NumberIn;
    //+++
    ImportantConstants();
    //+++
    if (!dirsInDir && Number.contains("/") )
    {
        QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
        return;
    }
    else if (dirsInDir && Number.count("/")>1 )
    {
        QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder:: + sub-folders");
        return;
    }

    //+++ Check File +++
    if (!fromComboBox && !filesManager->checkFileNumber(Number))
    {
        lineEditCheckRes->setText("! not existing !");
        return;
    }

    if (fromComboBox && !asciiYN && !filesManager->checkFileNumber(Number))
    {
        lineEditCheckRes->setText("! not existing !");
        return;
    }

    lineEditCheckRes->setText("OK");
    
    if (whatToCheck == "R2 [cm]")
    {
        double r2 = collimation->readR2(Number);
        lineEditCheckRes->setText(QString::number(r2));
    }
    else if (whatToCheck == "R1 [cm]")
    {
        double r1 = collimation->readR1(Number);
        lineEditCheckRes->setText(QString::number(r1));
    }
    else if (whatToCheck == "D [cm]")
    {
        double D = detector->readD(Number); // [cm]
        lineEditCheckRes->setText(QString::number(D));
    }
    else if (whatToCheck == "C [cm]")
    {
        lineEditCheckRes->setText(QString::number(int(collimation->readC(Number))));
    }
    else if (whatToCheck == "Lambda")
    {
        lineEditCheckRes->setText(QString::number(selector->readLambda(Number, monitors->readDuration(Number))));
    }
    else if (whatToCheck == "f [Hz]")
    {
        lineEditCheckRes->setText(QString::number(selector->readFrequency(Number, monitors->readDuration(Number))));
    }
    else if (whatToCheck == "Dead-Time-Factor [1]")
    {
        lineEditCheckRes->setText(QString::number(monitors->deadTimeFactorDetector(Number)));
    }
    else if (whatToCheck == "Monitor-1 [cps]")
    {
        lineEditCheckRes->setText(QString::number(monitors->readMonitor1(Number) / monitors->readDuration(Number)));
    }
    else if (whatToCheck == "Monitor-2 [cps]")
    {
        lineEditCheckRes->setText(QString::number(monitors->readMonitor2(Number) / monitors->readDuration(Number)));
    }
    else if (whatToCheck == "Monitor-3 [cps]")
    {
        lineEditCheckRes->setText(QString::number(monitors->readMonitor3(Number) / monitors->readDuration(Number)));
    }
    else if (whatToCheck=="Integral [cps]")
    {
        lineEditCheckRes->setText(QString::number(monitors->readSum(Number) / monitors->readDuration(Number)));
    }
    else if (whatToCheck == "Thickness [cm]")
    {
        lineEditCheckRes->setText(QString::number(sample->readThickness(Number)));
    }
    else if (whatToCheck=="SA")
    {
        lineEditCheckRes->setText(collimation->readSA(Number));
    }
    else if (whatToCheck=="CA")
    {
        lineEditCheckRes->setText(collimation->readCA(Number));
    }
    else if (whatToCheck == "Integral-vs-Mask[cps]")
    {
        lineEditCheckRes->setText(QString::number(integralVSmaskSimmetrical(Number) / monitors->readDuration(Number)));
    }
    else if (whatToCheck=="Q2-vs-Mask")
    {
        lineEditCheckRes->setText(QString::number(Q2_VS_maskSimmetrical( Number, true )));
    }
    else if (whatToCheck == "Duration[sec]")
    {
        lineEditCheckRes->setText(QString::number(monitors->readDuration(Number)));
    }
    else if (whatToCheck == "RT-normalization")
    {
        lineEditCheckRes->setText(QString::number(monitors->normalizationFactorRT(Number)));
    }
    else if (whatToCheck.contains("View Header"))
    {
        if (checkBoxSortOutputToFolders->isChecked())
        {
            app()->changeFolder("DAN :: rawdata");
        }
        bool activeYN=false;
        if (whatToCheck.contains("-Active]")) activeYN=true;
        openHeaderInNote( Number, activeYN );
    }
    else if (whatToCheck.contains("Sample-Title"))
    {
        lineEditCheckRes->setText(sample->readName(Number));
    }
    else if (whatToCheck == "Monitor-1")
    {
        lineEditCheckRes->setText(QString::number(monitors->readMonitor1(Number)));
    }
    else if (whatToCheck == "Monitor-2")
    {
        lineEditCheckRes->setText(QString::number(monitors->readMonitor2(Number)));
    }
    else if (whatToCheck == "Monitor-3")
    {
        lineEditCheckRes->setText(QString::number(monitors->readMonitor3(Number)));
    }
    else if (whatToCheck.contains("View Matrix") || whatToCheck.contains("Plot Matrix") )
    {
        if (checkBoxSortOutputToFolders->isChecked())
        {
            app()->changeFolder("DAN :: rawdata");
        }
 
        bool activeYN=false;
        if (whatToCheck.contains("-Active]")) activeYN=true;
        
        if (lstNumberIn.count()>1)
        {
             viewMatrixReduction(Number,lstNumberIn, activeYN);
        }
        else
        {
            gsl_matrix* data=gsl_matrix_alloc(MD,MD);
            
            
            int DD=comboBoxMDdata->currentText().toInt();
            int RegionOfInteres=spinBoxRegionOfInteres->value();
            int binning=comboBoxBinning->currentText().toInt();
            
            int pixelPerLine=spinBoxReadMatrixNumberPerLine->value();
            int pixelsInHeader=spinBoxHeaderNumberLines->value()+spinBoxDataHeaderNumberLines->value();
            
            bool XY=checkBoxTranspose->isChecked();
            // 2012 ::
            bool X2mX=checkBoxMatrixX2mX->isChecked();
            bool Y2mY=checkBoxMatrixY2mY->isChecked();
            
            if (asciiYN)
            {
                readMatrixByNameGSL (lineEditPathRAD->text()+"/ASCII-I/"+NumberIn+".DAT", data );
            }
            else
            {
                if (!readMatrix( Number, DD, RegionOfInteres, binning, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, data) )
                {
                    lineEditCheckRes->setText("Problem To READ Matrix");return;
                }
                
                //+++ mask gsl matrix
                QString maskName=comboBoxMaskFor->currentText();
                gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
                gsl_matrix_set_all(mask, 1.0);
                
                if (checkBoxBigMatrixMask->isChecked())
                {
                    make_GSL_Matrix_Symmetric( maskName, mask, MD);
                    gsl_matrix_mul_elements(data,mask);
                    
                }
                //+++ sens gsl matrix
                QString sensName=comboBoxSensFor->currentText();
                gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
                gsl_matrix_set_all(sens, 1.0);
                if (checkBoxBigMatrixSens->isChecked())
                {
                    make_GSL_Matrix_Symmetric( sensName, sens, MD);
                    gsl_matrix_mul_elements(data,sens);
                    
                }
                
                matrixConvolusion(data,mask,MD);
            }

            if (!asciiYN)
                lineEditCheckRes->setText(sample->readName(Number));

            Number=Number.replace("/","-").replace("[","-r").remove("]"); //new 2017

            QString matrixName="Matrix-"+Number;
            if (activeYN) matrixName="Matrix-Active";

            bool exist=existWindow(matrixName);

            QString sLabel;

            if (!asciiYN)
                sLabel = "raw-matrix :: " + sample->readName(NumberIn);
            else
                sLabel = "ascii-matrix :: " + NumberIn + ".DAT";

            makeMatrixSymmetric(data,matrixName,sLabel, MD);

            if (!exist ||  !whatToCheck.contains("Plot-Active]"))
            {
                app()->activeWindow()->setNormal();
                maximizeWindow(matrixName);
            }
            gsl_matrix_free(data);
        }
        

    }
    else if (whatToCheck.contains("View I(Q)"))
    {
        viewIQ(whatToCheck, Number, lstNumberIn);

    }
    
    if (whatToCheck.contains("Plot Matrix") && existWindow("Matrix-Active") )
    {
        if (!existWindow("Plot-Active"))
        {
            maximizeWindow("Matrix-Active");
            app()->plotColorMap();
            QString oldName=((MdiSubWindow* )app()->activeWindow())->name();
            app()->setWindowName((MdiSubWindow* )app()->activeWindow(),  "Plot-Active");
            app()->renameListViewItem(oldName, "Plot-Active");
            app()->updateWindowStatus ((MdiSubWindow * )app()->activeWindow());
            
        }
        app()->activeWindow()->setNormal();
        maximizeWindow("Plot-Active");

        QString title = "";

        if (asciiYN)
            title = "[" + comboBoxActiveFile->currentText() + "]";
        else
            title = "[" + NumberIn + "]: " + sample->readName(NumberIn);

        if (checkBoxSortOutputToFolders->isChecked())
            app()->changeFolder("DAN :: rawdata");

        MultiLayer *plot = (MultiLayer *)(app()->current_folder)->findWindow("Plot-Active", true, false, true, false);

        if (plot)
        {
            Graph *gr = plot->activeLayer();
            if (gr)
                gr->setTitle(title);
        }

        app()->setAutoScale();
    }
}

void dan18::viewMatrixReduction(QString Number, QStringList lstNumberIn, bool activeYN)
{
    QString label = "Matrix-";
    if (activeYN)
        label += "Active";
    else
        label += Number;

    QString NEC = "";
    QString NBC = "";
    QString Nbuffer = "";

    QString maskName = comboBoxMaskFor->currentText();
    QString sensName = comboBoxSensFor->currentText();
    double Detector = detector->readD(Number); // [cm]
    double C = int(collimation->readC(Number)); // [cm]
    double Lambda = selector->readLambda(Number, monitors->readDuration(Number));

    double trans = 1.0;
    double transBuffer = 1.0;
    double fractionBuffer = 0.0;
    double thickness = 1.0;
    double abs0 = 1.0;

    double scale = 1.0;
    double BackgroundConst = 0.0;
    double VShift = 0.0;
    double HShift = 0.0;

    lineEditCheckRes->setText(sample->readName(Number));

    if (!checkBoxBigMatrixMask->isChecked())
        maskName = "m1m2m3m4m5m098";
    if (!checkBoxBigMatrixSens->isChecked())
        sensName = "m1m2m3m4m5m099";
    if (!checkBoxBigMatrixNorm->isChecked())
    {
        double normalization = monitors->normalizationFactor(Number);
        if (normalization != 0)
            scale = 1.0 / normalization;
    }

    for (int i=1; i<lstNumberIn.count();i++ )
    {
        if (lstNumberIn[i].contains("SDD=")) { Detector=lstNumberIn[i].remove("SDD=").toDouble(); continue;};
        if (lstNumberIn[i].contains("COL=")) { C=lstNumberIn[i].remove("COL=").toDouble(); continue;};
        if (lstNumberIn[i].contains("LAMBDA=")) { Lambda=lstNumberIn[i].remove("LAMBDA=").toDouble(); continue;};
        if (lstNumberIn[i].contains("SCALE=")) { scale=lstNumberIn[i].remove("SCALE=").toDouble(); continue;};
        
        
        if (lstNumberIn[i].contains("EC=")) { NEC=lstNumberIn[i].remove("EC=").remove(" "); continue;};
        if (lstNumberIn[i].contains("BC=")) { NBC=lstNumberIn[i].remove("BC=").remove(" "); continue;};
        if (lstNumberIn[i].contains("TR=")) { trans=lstNumberIn[i].remove("TR=").toDouble(); continue;};
        if (lstNumberIn[i].contains("BGD=")) { BackgroundConst=lstNumberIn[i].remove("BGD=").toDouble(); continue;};
        
        if (lstNumberIn[i].contains("MASK=")) { maskName=lstNumberIn[i].remove("MASK=").remove(" "); continue;};
        if (lstNumberIn[i].contains("SENS=")) { sensName=lstNumberIn[i].remove("SENS=").remove(" "); continue;};
    }
    
    
    double Xcenter=MD/2;
    double Ycenter=MD/2;
    
    readCenterfromMaskName( maskName, Xcenter, Ycenter, MD );
    Xcenter++;
    Ycenter++;

    danDanMultiButtonSingleLine("I-x-y", label, Number, NEC, NBC, Nbuffer, maskName, sensName, Detector, C, Lambda,
                                trans, transBuffer, fractionBuffer, thickness, abs0, Xcenter, Ycenter, scale,
                                BackgroundConst, VShift, HShift);
}


void dan18::viewIQ(QString whatToCheck, QString Number, QStringList lstNumberIn)
{
    QString Nsample=Number;
    QString label = sample->readName(Number);
    QString NEC="";
    QString NBC="";
    QString Nbuffer="";
    
    
    QString maskName=comboBoxMaskFor->currentText();
    QString sensName=comboBoxSensFor->currentText();
    double Detector = detector->readD(Number);  // [cm]
    double C = int(collimation->readC(Number)); // [cm]
    double Lambda = selector->readLambda(Number, monitors->readDuration(Number));

    double trans=1.0;
    double transBuffer=1.0;
    double fractionBuffer=0.0;
    double thickness=1.0;
    double abs0=1.0;
    
    double scale=1.0;
    double BackgroundConst=0.0;
    double VShift=0.0;
    double HShift=0.0;

    lineEditCheckRes->setText(sample->readName(Number));

    if (!checkBoxBigMatrixMask->isChecked()) maskName="m1m2m3m4m5m098";
    if (!checkBoxBigMatrixSens->isChecked()) sensName="m1m2m3m4m5m099";
    if (!checkBoxBigMatrixNorm->isChecked())
    {
        double normalization = monitors->normalizationFactor(Number);
        if (normalization != 0)
            scale = 1.0 / normalization;
    }

    for (int i=1; i<lstNumberIn.count();i++ )
    {
        if (lstNumberIn[i].contains("SDD=")) { Detector=lstNumberIn[i].remove("SDD=").toDouble(); continue;};
        if (lstNumberIn[i].contains("COL=")) { C=lstNumberIn[i].remove("COL=").toDouble(); continue;};
        if (lstNumberIn[i].contains("LAMBDA=")) { Lambda=lstNumberIn[i].remove("LAMBDA=").toDouble(); continue;};
        if (lstNumberIn[i].contains("SCALE=")) { scale=lstNumberIn[i].remove("SCALE=").toDouble(); continue;};
        
        
        if (lstNumberIn[i].contains("EC=")) { NEC=lstNumberIn[i].remove("EC=").remove(" "); continue;};
        if (lstNumberIn[i].contains("BC=")) { NBC=lstNumberIn[i].remove("BC=").remove(" "); continue;};
        if (lstNumberIn[i].contains("TR=")) { trans=lstNumberIn[i].remove("TR=").toDouble(); continue;};
        if (lstNumberIn[i].contains("BGD=")) { BackgroundConst=lstNumberIn[i].remove("BGD=").toDouble(); continue;};

        if (lstNumberIn[i].contains("MASK=")) { maskName=lstNumberIn[i].remove("MASK=").remove(" "); continue;};
        if (lstNumberIn[i].contains("SENS=")) { sensName=lstNumberIn[i].remove("SENS=").remove(" "); continue;};
    }
    
    
    double Xcenter=MD/2;
    double Ycenter=MD/2;
    
    readCenterfromMaskName( maskName, Xcenter, Ycenter, MD );
    Xcenter++;
    Ycenter++;
    
    danDanMultiButtonSingleLine("I-Q",
                                label, Nsample, NEC, NBC, Nbuffer, maskName, sensName,
                                Detector, C, Lambda,
                                trans, transBuffer, fractionBuffer, thickness, abs0,
                                Xcenter, Ycenter,
                                scale, BackgroundConst,VShift, HShift
                                );
    
    bool plotData=false;
    
    if (plotData)
    {
        Graph *g;
        MdiSubWindow *w;
        
        plotData=findActiveGraph(g);
        bool maximaizedYN=false;
        
        if ( plotData )
        {
            w= ( MdiSubWindow *) app()->activeWindow();
            if (w->status() == MdiSubWindow::Maximized && QString(w->metaObject()->className()) == "MultiLayer")
            {
                maximaizedYN=true;
                // w->showMinimized();
            }
            
            QString tableName2plot="raw-QI";
            if (!whatToCheck.contains("raw-QI")) tableName2plot+="-SM-"+Nsample;
                
                AddCurve(g, tableName2plot);
                }
        
        
    }
}



void dan18::updateComboBoxActiveFile()
{
    ImportantConstants();
    QString activeFile=comboBoxActiveFile->currentText();

    QDir dir( lineEditPathDAT->text() + "/"+comboBoxActiveFolder->currentText()+"/");
    
    if (checkBoxBigMatrixASCII->isChecked()) dir.setPath(lineEditPathRAD->text()+"/ASCII-I/");

    QStringList activeFileList;
    if (checkBoxBigMatrixASCII->isChecked()) activeFileList= dir.entryList(QStringList() << "*.DAT");
    else activeFileList= dir.entryList(QStringList() << textEditPattern->text());
    QStringList activeNumberList;

    QString filter=wildCard;
    if (checkBoxBigMatrixASCII->isChecked()) filter="*.DAT";
    
    //+++ Ext
    QString currentExt=lineEditFileExt->text().remove(" ");
    if(currentExt!="") currentExt+="-";

    for (int i = 0; i < activeFileList.count(); i++)
    {
        QString sss;
        if (checkBoxBigMatrixASCII->isChecked())
            sss = FilesManager::findFileNumberInFileName(filter, activeFileList[i]);
        else
            sss = FilesManager::findFileNumberInFileName(wildCard, activeFileList[i]);
        if (sss != "")
            activeNumberList << sss;
    }
    comboBoxActiveFile->clear();
    comboBoxActiveFile->insertItems(0, activeNumberList);
    if (activeNumberList.contains(activeFile)) comboBoxActiveFile->setCurrentIndex(activeNumberList.indexOf(activeFile));
}

void dan18::updateComboBoxActiveFolders()
{
    bool checkBoxActiveFooldersIsChecked=checkBoxDirsIndir->isChecked();
    QString activeFolder=comboBoxActiveFolder->currentText();
    QStringList activeFolderList;
    
    if (checkBoxActiveFooldersIsChecked)
    {
        QDir dir( lineEditPathDAT->text());
        activeFolderList << dir.entryList( QDir::AllDirs );
        comboBoxActiveFolder->show();
    }
    else
    {
        activeFolderList<<".";
        comboBoxActiveFolder->hide();
    }
    activeFolderList.removeAll("..");
    
    comboBoxActiveFolder->clear();
    
    comboBoxActiveFolder->insertItems(0, activeFolderList);
    if (activeFolderList.contains(activeFolder)) comboBoxActiveFolder->setCurrentIndex(activeFolderList.indexOf(activeFolder));
}

void dan18::selectFileToHeader()
{
    QString commandLine="";
    if(lineEditCheck->text().contains(";"))
    {
        QStringList lstNumberIn = lineEditCheck->text().split(";", QString::SkipEmptyParts);
        for(int i=1; i<lstNumberIn.count();i++)commandLine+=";"+lstNumberIn[i];
    }
    
    QString fileNumber="";
    if (selectFile(fileNumber))
    {
        
        lineEditCheck->setText(fileNumber+commandLine);
        check();
    }
    else {
        lineEditCheck->setText(commandLine);
        lineEditCheckRes->setText("! not correct file-name format!");
    }
    
}


void dan18::openHeaderInNote( QString Number, bool activeYN )
{
    QStringList header;
    readHeaderNumberFull ( Number, header);
    
    //+++
    QString s, ss;
    
    //+++
    for(int i=0;i<header.count(); i++) s+=header[i]+"\n";

    QString tableName="Header-";
    if (activeYN)tableName+="Active";
    else tableName+=Number;
    tableName=tableName.replace("/", "-");
    if (!checkNoteExistence(tableName))
    {
        app()->newNoteText(tableName,s);
    }
    else
    {
        QList<MdiSubWindow*> windows = app()->windowsList();
        foreach(MdiSubWindow *w, windows) if (QString(w->metaObject()->className()) == "Note" && w->name()==tableName) ((Note *)w)->currentEditor()->setText(s);
    }
    app()->activeWindow()->setNormal();
    maximizeWindow(tableName);
}




//++++++SLOT::extractRawData++
void dan18::extractRawData()
{
    //+++ vry IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    QString filter=textEditPattern->text();
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Getting File Information"));
    fd->setNameFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    if (filesNumber==0)
    {
        QMessageBox::critical( 0, "qtiSAS", "Nothing was selected");
        return;
    }
    
    QString test=selectedDat[0];
    if ( test.contains(Dir) )
    {
        test=test.remove(Dir);
        if (!dirsInDir && test.contains("/") )
        {
            QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
            return;
        }
        else if (dirsInDir && test.count("/")>1 )
        {
            QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder:: + sub-folders");
            return;
        }
    }
    else
    {
        QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
        return;
    }
    
    
    //+++
    QString fnameOnly;
    QString nameMatrix;
    //
    QString pos,num;
    
    QString newName;
    QString DirOut 	= lineEditPathRAD->text();
    QDir dd;
    if (!dd.cd(DirOut+"/raw-matrix"))
    {
        dd.mkdir(DirOut+"/raw-matrix");
    }

    for (int iter = 0; iter < filesNumber; iter++)
    {
        //+++ header
        nameMatrix = FilesManager::findFileNumberInFileName(wildCard, selectedDat[iter].remove(Dir));
        gsl_matrix *data = gsl_matrix_alloc(MD, MD);
        gsl_matrix_set_all(data, 0.0);
        readMatrix(nameMatrix, data);

        newName = DirOut + "raw-matrix/only_" + nameMatrix + "_rawdata.matrix";

        saveMatrixToFileInteger(newName, data, MD);
        gsl_matrix_free(data);
    }
}

bool dan18::callFromTerminal(QString commandLine)
{
    commandLine=commandLine.simplified();
    commandLine=commandLine.trimmed();
    
    QStringList lst;
    lst.clear();
    lst = commandLine.split(" ", QString::SkipEmptyParts);
    
    if (lst.count()==0) return false;
    
    QString command=lst[0];

    if(command=="fast-QI")

    {
        sansTab->setCurrentIndex(1);
        commandLine=commandLine.replace(" ", ";").remove("fast-QI;");
        check(commandLine, false, "View I(Q)");
        
    }
}

void dan18::asciiOrRawdata()
{
    comboBoxCheck->blockSignals(true);
    QString whatToCheck=comboBoxCheck->currentText();
    comboBoxCheck->clear();
    
    QStringList lst;
    if (checkBoxBigMatrixASCII->isChecked())
    {
        lst<<"View Matrix";
        lst<<"View Matrix [Matrix-Active]";
        lst<<"Plot Matrix [Plot-Active]";
    }
    else
    {
        lst<<"View I(Q)";
        lst<<"View I(Q) [in raw-QI  table]";
        lst<<"View Matrix";
        lst<<"View Matrix [Matrix-Active]";
        lst<<"Plot Matrix [Plot-Active]";
        lst<<"View Header";
        lst<<"Monitor-1";
        lst<<"Monitor-2";
        lst<<"Monitor-3";
        lst<<"Monitor-1 [cps]";
        lst<<"Monitor-2 [cps]";
        lst<<"Monitor-3 [cps]";
        lst<<"Duration[sec]";
        lst<<"Integral [cps]";
        lst<<"Integral-vs-Mask[cps]";
        lst<<"Dead-Time-Factor [1]";
        lst<<"C [cm]";
        lst<<"D [cm]";
        lst<<"f [Hz]";
        lst<<"Lambda";
        lst<<"R1 [cm]";
        lst<<"R2 [cm]";
        lst<<"Thickness [cm]";
        lst<<"SA";
        lst<<"CA";
        lst<<"[Info]";
        lst<<"RT-normalization";
        lst<<"Q2-vs-Mask";
    }

    comboBoxCheck->addItems(lst);
    if (lst.contains(whatToCheck))
        comboBoxCheck->setItemText(comboBoxCheck->currentIndex(), whatToCheck);
    else
        comboBoxCheck->setItemText(comboBoxCheck->currentIndex(), "Plot Matrix [Plot-Active]");
    comboBoxCheck->blockSignals(false);
}
