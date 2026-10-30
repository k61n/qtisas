/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS data analysis interface
 ******************************************************************************/

#include <QDebug>

#include "dan18.h"
#include "Note.h"

//+++ connect Slots
void dan18::rawdataConnectSlot()
{
    connect( pushButtonNewInfoTable, SIGNAL( clicked() ), this, SLOT( newInfoTable() ) );
    connect( pushButtonMakeList, SIGNAL( clicked() ), this, SLOT( addToInfoTable() ) );
    connect( pushButtonNewInfoMatrix, SIGNAL( clicked() ), this, SLOT( newInfoMatrix() ) );
    connect( pushButtonMakeBigMatrix, SIGNAL( clicked() ), this, SLOT( slotMakeBigMatrix() ) );
    connect( pushButtonNewInfoMatrixFromTable, SIGNAL( clicked() ), this, SLOT( slotMakeBigMatrixFromTable() ) );

    connect(comboBoxCheck, &QComboBox::textActivated, this, [this](const QString &) { this->check(); });
    connect(comboBoxActiveFile, &QComboBox::textActivated, this, &dan18::checkInList);
    connect(comboBoxActiveFolder, &QComboBox::textActivated, this,
            [this](const QString &) { this->updateComboBoxActiveFile(); });

    connect( lineEditCheck, SIGNAL( returnPressed() ), this, SLOT( check() ) );
    // 2022
    connect( checkBoxDirsIndir, SIGNAL( toggled(bool) ), this, SLOT( updateComboBoxActiveFolders() ) );
    
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
    QStringList infoTablesList = app()->findTableListByLabel("Info::Table");

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

    //+++ Cols Names
    tableDat->setColName(itSample, "Sample");
    tableDat->setColPlotDesignation(itSample, Table::None);
    tableDat->setColumnType(itSample, Table::Text);

    tableDat->setColName(itPolarization, "Polarization");
    tableDat->setColPlotDesignation(itPolarization, Table::None);
    tableDat->setColumnType(itPolarization, Table::Text);

    tableDat->setColName(itRuns, "Runs");
    tableDat->setColPlotDesignation(itRuns, Table::X);
    tableDat->setColumnType(itRuns, Table::Text);

    tableDat->setColName(itC, "C");
    tableDat->setColPlotDesignation(itC, Table::None);
    tableDat->setColumnType(itC, Table::Numeric);

    tableDat->setColName(itD, "D");
    tableDat->setColComment(itC, "[m]");
    tableDat->setColPlotDesignation(itD, Table::None);
    tableDat->setColumnType(itD, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 3, itD, false);

    tableDat->setColName(itLambda, "lambda");
    tableDat->setColComment(itD, "[m]");
    tableDat->setColPlotDesignation(itLambda, Table::None);
    tableDat->setColumnType(itLambda, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 3, itLambda, false);

    tableDat->setColName(itBeam, "Beam");
    tableDat->setColComment(itLambda, "[A]");
    tableDat->setColPlotDesignation(itBeam, Table::None);
    tableDat->setColumnType(itBeam, Table::Text);

    tableDat->setColName(itSum, "Sum");
    tableDat->setColPlotDesignation(itSum, Table::Y);
    tableDat->setColumnType(itSum, Table::Numeric);
    tableDat->setColNumericFormat(Table::Default, 13, itSum, false);

    tableDat->setColName(itDuration, "Duration");
    tableDat->setColComment(itDuration, "[sec]");
    tableDat->setColPlotDesignation(itDuration, Table::None);
    tableDat->setColumnType(itDuration, Table::Numeric);
    tableDat->setColNumericFormat(Table::Default, 6, itDuration, false);

    tableDat->setColName(itCps, "cps");
    tableDat->setColPlotDesignation(itCps, Table::Y);
    tableDat->setColumnType(itCps, Table::Numeric);
    tableDat->setColNumericFormat(Table::Default, 13, itCps, false);

    tableDat->setColName(itDate, "Date");
    tableDat->setColPlotDesignation(itDate, Table::None);
    tableDat->setColumnType(itDate, Table::Text);

    tableDat->setColName(itTime, "Time");
    tableDat->setColPlotDesignation(itTime, Table::None);
    tableDat->setColumnType(itTime, Table::Text);

    tableDat->setColName(itField1, "Field-1");
    tableDat->setColPlotDesignation(itField1, Table::None);
    tableDat->setColumnType(itField1, Table::Text);

    tableDat->setColName(itField2, "Field-2");
    tableDat->setColPlotDesignation(itField2, Table::None);
    tableDat->setColumnType(itField2, Table::Text);

    tableDat->setColName(itField3, "Field-3");
    tableDat->setColPlotDesignation(itField3, Table::None);
    tableDat->setColumnType(itField3, Table::Text);

    tableDat->setColName(itField4, "Field-4");
    tableDat->setColumnType(itField4, Table::Text);
    tableDat->setColPlotDesignation(itField4, Table::None);

    tableDat->setColName(itBeamwindowX, "Beamwindow-X");
    tableDat->setColPlotDesignation(itBeamwindowX, Table::None);
    tableDat->setColumnType(itBeamwindowX, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 2, itBeamwindowX, false);

    tableDat->setColName(itBeamwindowY, "Beamwindow-Y");
    tableDat->setColPlotDesignation(itBeamwindowY, Table::None);
    tableDat->setColumnType(itBeamwindowY, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 2, itBeamwindowY, false);

    tableDat->setColName(itOffset, "Offset");
    tableDat->setColPlotDesignation(itOffset, Table::None);
    tableDat->setColumnType(itOffset, Table::Numeric);

    tableDat->setColName(itXposition, "X-Position");
    tableDat->setColPlotDesignation(itXposition, Table::None);
    tableDat->setColumnType(itXposition, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 3, itXposition, false);

    tableDat->setColName(itYposition, "Y-Position");
    tableDat->setColPlotDesignation(itYposition, Table::None);
    tableDat->setColumnType(itYposition, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 3, itYposition, false);

    tableDat->setColName(itSampleNr, "Sample-Nr");
    tableDat->setColPlotDesignation(itSampleNr, Table::None);
    tableDat->setColumnType(itSampleNr, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 0, itSampleNr, false);

    tableDat->setColName(itMotor1, "Motor-1");
    tableDat->setColPlotDesignation(itMotor1, Table::None);
    tableDat->setColumnType(itMotor1, Table::Numeric);

    tableDat->setColName(itMotor2, "Motor-2");
    tableDat->setColPlotDesignation(itMotor2, Table::None);
    tableDat->setColumnType(itMotor2, Table::Numeric);

    tableDat->setColName(itThickness, "Thickness");
    tableDat->setColComment(itThickness, "[cm]");
    tableDat->setColPlotDesignation(itThickness, Table::None);
    tableDat->setColumnType(itThickness, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 2, itThickness, false);

    tableDat->setColName(itBeamWinXs, "BeamWin-Xs");
    tableDat->setColPlotDesignation(itBeamWinXs, Table::None);
    tableDat->setColumnType(itBeamWinXs, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 2, itBeamWinXs, false);

    tableDat->setColName(itBeamWinYs, "BeamWin-Ys");
    tableDat->setColPlotDesignation(itBeamWinYs, Table::None);
    tableDat->setColumnType(itBeamWinYs, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 2, itBeamWinYs, false);

    tableDat->setColName(itBeamWinXpos, "Beamwin-X-Pos");
    tableDat->setColPlotDesignation(itBeamWinXpos, Table::None);
    tableDat->setColumnType(itBeamWinXpos, Table::Numeric);

    tableDat->setColName(itBeamWinYpos, "Beamwin-Y-Pos");
    tableDat->setColPlotDesignation(itBeamWinYpos, Table::None);
    tableDat->setColumnType(itBeamWinYpos, Table::Numeric);

    tableDat->setColName(itTimeFactor, "Time-Factor");
    tableDat->setColPlotDesignation(itTimeFactor, Table::None);
    tableDat->setColumnType(itTimeFactor, Table::Numeric);

    tableDat->setColName(itComment1, "Comment1");
    tableDat->setColPlotDesignation(itComment1, Table::None);
    tableDat->setColumnType(itComment1, Table::Text);

    tableDat->setColName(itComment2, "Comment2");
    tableDat->setColPlotDesignation(itComment2, Table::None);
    tableDat->setColumnType(itComment2, Table::Text);

    tableDat->setColName(itName, "Name");
    tableDat->setColPlotDesignation(itName, Table::None);
    tableDat->setColumnType(itName, Table::Text);

    tableDat->setColName(itWho, "Who");
    tableDat->setColPlotDesignation(itWho, Table::None);
    tableDat->setColumnType(itWho, Table::Text);

    tableDat->setColName(itSelector, "Selector");
    tableDat->setColPlotDesignation(itSelector, Table::None);
    tableDat->setColumnType(itSelector, Table::Numeric);

    tableDat->setColComment(itSelector, "[Hz]");
    tableDat->setColName(itMonitor1, "Monitor-1");
    tableDat->setColPlotDesignation(itMonitor1, Table::Y);
    tableDat->setColumnType(itMonitor1, Table::Numeric);

    tableDat->setColName(itMonitor2, "Monitor-2");
    tableDat->setColPlotDesignation(itMonitor2, Table::Y);
    tableDat->setColumnType(itMonitor2, Table::Numeric);

    tableDat->setColName(itMonitor3, "Monitor-3");
    tableDat->setColPlotDesignation(itMonitor3, Table::Y);
    tableDat->setColumnType(itMonitor3, Table::Numeric);

    tableDat->setColName(itFiles, "Files");
    tableDat->setColPlotDesignation(itFiles, Table::None);
    tableDat->setColumnType(itFiles, Table::Text);

    tableDat->setColName(itRTnumber, "RT-number");
    tableDat->setColPlotDesignation(itRTnumber, Table::None);
    tableDat->setColumnType(itRTnumber, Table::Text);

    tableDat->setColName(itRTtimefactor, "RT-Time-Factor");
    tableDat->setColPlotDesignation(itRTtimefactor, Table::None);
    tableDat->setColumnType(itRTtimefactor, Table::Text);

    tableDat->setColName(itRTrepetitions, "RT-Repetitions");
    tableDat->setColPlotDesignation(itRTrepetitions, Table::None);
    tableDat->setColumnType(itRTrepetitions, Table::Text);

    tableDat->setColName(itRTframeduration, "RT-Frame-Duration");
    tableDat->setColPlotDesignation(itRTframeduration, Table::None);
    tableDat->setColumnType(itRTframeduration, Table::Text);

    tableDat->setColName(itSumvsmask, "Sum-vs-Mask");
    tableDat->setColPlotDesignation(itSumvsmask, Table::Y);
    tableDat->setColumnType(itSumvsmask, Table::Numeric);
    tableDat->setColNumericFormat(Table::Default, 13, itSumvsmask, false);

    tableDat->setColName(itSumvsmaskdeadtimecorrected, "Sum-vs-Mask-Dead-Time-Corrected");
    tableDat->setColPlotDesignation(itSumvsmaskdeadtimecorrected, Table::Y);
    tableDat->setColumnType(itSumvsmaskdeadtimecorrected, Table::Numeric);
    tableDat->setColNumericFormat(Table::Default, 13, itSumvsmaskdeadtimecorrected, false);

    tableDat->setColName(itQ2vsmask, "Q2-vs-Mask");
    tableDat->setColPlotDesignation(itQ2vsmask, Table::Y);
    tableDat->setColumnType(itQ2vsmask, Table::Numeric);
    tableDat->setColNumericFormat(Table::Default, 13, itQ2vsmask, false);

    tableDat->setColName(itAttenuator, "Attenuator");
    tableDat->setColPlotDesignation(itAttenuator, Table::None);
    tableDat->setColumnType(itAttenuator, Table::Text);

    tableDat->setColName(itLenses, "Lenses");
    tableDat->setColPlotDesignation(itLenses, Table::None);
    tableDat->setColumnType(itLenses, Table::Text);

    tableDat->setColName(itSlicesCount, "Slices-Count");
    tableDat->setColPlotDesignation(itSlicesCount, Table::Y);
    tableDat->setColumnType(itSlicesCount, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCount, false);

    tableDat->setColName(itSlicesDuration, "Slices-Duration");
    tableDat->setColPlotDesignation(itSlicesDuration, Table::Y);
    tableDat->setColumnType(itSlicesDuration, Table::Numeric);
    tableDat->setColNumericFormat(Table::Default, 6, itSlicesDuration, false);

    tableDat->setColName(itSlicesCurrentNumber, "Slices-Current-Number");
    tableDat->setColPlotDesignation(itSlicesCurrentNumber, Table::Y);
    tableDat->setColumnType(itSlicesCurrentNumber, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentNumber, false);

    tableDat->setColName(itSlicesCurrentDuration, "Slices-Current-Duration");
    tableDat->setColPlotDesignation(itSlicesCurrentDuration, Table::Y);
    tableDat->setColumnType(itSlicesCurrentDuration, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentDuration, false);

    tableDat->setColName(itSlicesCurrentMonitor1, "Slices-Current-Monitor1");
    tableDat->setColPlotDesignation(itSlicesCurrentMonitor1, Table::Y);
    tableDat->setColumnType(itSlicesCurrentMonitor1, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentMonitor1, false);

    tableDat->setColName(itSlicesCurrentMonitor2, "Slices-Current-Monitor2");
    tableDat->setColPlotDesignation(itSlicesCurrentMonitor2, Table::Y);
    tableDat->setColumnType(itSlicesCurrentMonitor2, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentMonitor2, false);

    tableDat->setColName(itSlicesCurrentMonitor3, "Slices-Current-Monitor3");
    tableDat->setColPlotDesignation(itSlicesCurrentMonitor3, Table::Y);
    tableDat->setColumnType(itSlicesCurrentMonitor3, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentMonitor3, false);

    tableDat->setColName(itSlicesCurrentSum, "Slices-Current-Sum");
    tableDat->setColPlotDesignation(itSlicesCurrentSum, Table::Y);
    tableDat->setColumnType(itSlicesCurrentSum, Table::Numeric);
    tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentSum, false);

    tableDat->show();

    tableDat->setMaximized();
    
    app()->modifiedProject();
}

//++++++SLOT::addToInfoTable++
void dan18::addToInfoTable()
{
    QString Dir = filesManager->pathInString();
    QString wildCard = filesManager->wildCardDetector();
    bool dirsInDir = filesManager->subFoldersYN();
    int MD = lineEditMD->text().toInt();

    QString activeTable = comboBoxInfoTable->currentText();

    QStringList infoTablesList = app()->findTableListByLabel("Info::Table");

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
    
    QString wildCardLocal = wildCard;
    
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
    
    if (fd->exec() == QDialog::Rejected)
        return;
    
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

    addToInfoTable(TableName, selectedDat);
}
//+++ the files (of the input folder) added to the info table TableName, made when it does not exist
void dan18::addToInfoTable(const QString &TableName, QStringList selectedDat)
{
    QString Dir = filesManager->pathInString();
    QString wildCard = filesManager->wildCardDetector();
    int MD = lineEditMD->text().toInt();
    int filesNumber = static_cast<int>(selectedDat.count());
    int i;
    Table *tableDat;
    QString wildCardLocal;

    int startRaw=0;
    
    wildCardLocal=wildCard;
    
    // app()->ws->hide();
    // app()->ws->blockSignals ( true );
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }

    if (app()->checkTableExistence(TableName))
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
        tableDat = app()->newTable(TableName, filesNumber, 56);

        //+++
        tableDat->setWindowLabel("Info::Table");
        app()->setListViewLabel(tableDat->name(), "Info::Table");
        app()->updateWindowLists(tableDat);
        
        //+++ Cols Names
        tableDat->setColName(itSample, "Sample");
        tableDat->setColPlotDesignation(itSample, Table::None);
        tableDat->setColumnType(itSample, Table::Text);

        tableDat->setColName(itPolarization, "Polarization");
        tableDat->setColPlotDesignation(itPolarization, Table::None);
        tableDat->setColumnType(itPolarization, Table::Text);

        tableDat->setColName(itRuns, "Runs");
        tableDat->setColPlotDesignation(itRuns, Table::X);
        tableDat->setColumnType(itRuns, Table::Text);

        tableDat->setColName(itC, "C");
        tableDat->setColPlotDesignation(itC, Table::None);
        tableDat->setColumnType(itC, Table::Numeric);

        tableDat->setColName(itD, "D");
        tableDat->setColComment(itC, "[m]");
        tableDat->setColPlotDesignation(itD, Table::None);
        tableDat->setColumnType(itD, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 3, itD, false);

        tableDat->setColName(itLambda, "lambda");
        tableDat->setColComment(itD, "[m]");
        tableDat->setColPlotDesignation(itLambda, Table::None);
        tableDat->setColumnType(itLambda, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 3, itLambda, false);

        tableDat->setColName(itBeam, "Beam");
        tableDat->setColComment(itLambda, "[A]");
        tableDat->setColPlotDesignation(itBeam, Table::None);
        tableDat->setColumnType(itBeam, Table::Text);

        tableDat->setColName(itSum, "Sum");
        tableDat->setColPlotDesignation(itSum, Table::Y);
        tableDat->setColumnType(itSum, Table::Numeric);
        tableDat->setColNumericFormat(Table::Default, 13, itSum, false);

        tableDat->setColName(itDuration, "Duration");
        tableDat->setColComment(itDuration, "[sec]");
        tableDat->setColPlotDesignation(itDuration, Table::None);
        tableDat->setColumnType(itDuration, Table::Numeric);
        tableDat->setColNumericFormat(Table::Default, 6, itDuration, false);

        tableDat->setColName(itCps, "cps");
        tableDat->setColPlotDesignation(itCps, Table::Y);
        tableDat->setColumnType(itCps, Table::Numeric);
        tableDat->setColNumericFormat(Table::Default, 13, itCps, false);

        tableDat->setColName(itDate, "Date");
        tableDat->setColPlotDesignation(itDate, Table::None);
        tableDat->setColumnType(itDate, Table::Text);

        tableDat->setColName(itTime, "Time");
        tableDat->setColPlotDesignation(itTime, Table::None);
        tableDat->setColumnType(itTime, Table::Text);

        tableDat->setColName(itField1, "Field-1");
        tableDat->setColPlotDesignation(itField1, Table::None);
        tableDat->setColumnType(itField1, Table::Text);

        tableDat->setColName(itField2, "Field-2");
        tableDat->setColPlotDesignation(itField2, Table::None);
        tableDat->setColumnType(itField2, Table::Text);

        tableDat->setColName(itField3, "Field-3");
        tableDat->setColPlotDesignation(itField3, Table::None);
        tableDat->setColumnType(itField3, Table::Text);

        tableDat->setColName(itField4, "Field-4");
        tableDat->setColumnType(itField4, Table::Text);
        tableDat->setColPlotDesignation(itField4, Table::None);

        tableDat->setColName(itBeamwindowX, "Beamwindow-X");
        tableDat->setColPlotDesignation(itBeamwindowX, Table::None);
        tableDat->setColumnType(itBeamwindowX, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 2, itBeamwindowX, false);

        tableDat->setColName(itBeamwindowY, "Beamwindow-Y");
        tableDat->setColPlotDesignation(itBeamwindowY, Table::None);
        tableDat->setColumnType(itBeamwindowY, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 2, itBeamwindowY, false);

        tableDat->setColName(itOffset, "Offset");
        tableDat->setColPlotDesignation(itOffset, Table::None);
        tableDat->setColumnType(itOffset, Table::Numeric);

        tableDat->setColName(itXposition, "X-Position");
        tableDat->setColPlotDesignation(itXposition, Table::None);
        tableDat->setColumnType(itXposition, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 3, itXposition, false);

        tableDat->setColName(itYposition, "Y-Position");
        tableDat->setColPlotDesignation(itYposition, Table::None);
        tableDat->setColumnType(itYposition, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 3, itYposition, false);

        tableDat->setColName(itSampleNr, "Sample-Nr");
        tableDat->setColPlotDesignation(itSampleNr, Table::None);
        tableDat->setColumnType(itSampleNr, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 0, itSampleNr, false);

        tableDat->setColName(itMotor1, "Motor-1");
        tableDat->setColPlotDesignation(itMotor1, Table::None);
        tableDat->setColumnType(itMotor1, Table::Numeric);

        tableDat->setColName(itMotor2, "Motor-2");
        tableDat->setColPlotDesignation(itMotor2, Table::None);
        tableDat->setColumnType(itMotor2, Table::Numeric);

        tableDat->setColName(itThickness, "Thickness");
        tableDat->setColComment(itThickness, "[cm]");
        tableDat->setColPlotDesignation(itThickness, Table::None);
        tableDat->setColumnType(itThickness, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 2, itThickness, false);

        tableDat->setColName(itBeamWinXs, "BeamWin-Xs");
        tableDat->setColPlotDesignation(itBeamWinXs, Table::None);
        tableDat->setColumnType(itBeamWinXs, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 2, itBeamWinXs, false);

        tableDat->setColName(itBeamWinYs, "BeamWin-Ys");
        tableDat->setColPlotDesignation(itBeamWinYs, Table::None);
        tableDat->setColumnType(itBeamWinYs, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 2, itBeamWinYs, false);

        tableDat->setColName(itBeamWinXpos, "Beamwin-X-Pos");
        tableDat->setColPlotDesignation(itBeamWinXpos, Table::None);
        tableDat->setColumnType(itBeamWinXpos, Table::Numeric);

        tableDat->setColName(itBeamWinYpos, "Beamwin-Y-Pos");
        tableDat->setColPlotDesignation(itBeamWinYpos, Table::None);
        tableDat->setColumnType(itBeamWinYpos, Table::Numeric);

        tableDat->setColName(itTimeFactor, "Time-Factor");
        tableDat->setColPlotDesignation(itTimeFactor, Table::None);
        tableDat->setColumnType(itTimeFactor, Table::Numeric);

        tableDat->setColName(itComment1, "Comment1");
        tableDat->setColPlotDesignation(itComment1, Table::None);
        tableDat->setColumnType(itComment1, Table::Text);

        tableDat->setColName(itComment2, "Comment2");
        tableDat->setColPlotDesignation(itComment2, Table::None);
        tableDat->setColumnType(itComment2, Table::Text);

        tableDat->setColName(itName, "Name");
        tableDat->setColPlotDesignation(itName, Table::None);
        tableDat->setColumnType(itName, Table::Text);

        tableDat->setColName(itWho, "Who");
        tableDat->setColPlotDesignation(itWho, Table::None);
        tableDat->setColumnType(itWho, Table::Text);

        tableDat->setColName(itSelector, "Selector");
        tableDat->setColPlotDesignation(itSelector, Table::None);
        tableDat->setColumnType(itSelector, Table::Numeric);

        tableDat->setColComment(itSelector, "[Hz]");
        tableDat->setColName(itMonitor1, "Monitor-1");
        tableDat->setColPlotDesignation(itMonitor1, Table::Y);
        tableDat->setColumnType(itMonitor1, Table::Numeric);

        tableDat->setColName(itMonitor2, "Monitor-2");
        tableDat->setColPlotDesignation(itMonitor2, Table::Y);
        tableDat->setColumnType(itMonitor2, Table::Numeric);

        tableDat->setColName(itMonitor3, "Monitor-3");
        tableDat->setColPlotDesignation(itMonitor3, Table::Y);
        tableDat->setColumnType(itMonitor3, Table::Numeric);

        tableDat->setColName(itFiles, "Files");
        tableDat->setColPlotDesignation(itFiles, Table::None);
        tableDat->setColumnType(itFiles, Table::Text);

        tableDat->setColName(itRTnumber, "RT-number");
        tableDat->setColPlotDesignation(itRTnumber, Table::None);
        tableDat->setColumnType(itRTnumber, Table::Text);

        tableDat->setColName(itRTtimefactor, "RT-Time-Factor");
        tableDat->setColPlotDesignation(itRTtimefactor, Table::None);
        tableDat->setColumnType(itRTtimefactor, Table::Text);

        tableDat->setColName(itRTrepetitions, "RT-Repetitions");
        tableDat->setColPlotDesignation(itRTrepetitions, Table::None);
        tableDat->setColumnType(itRTrepetitions, Table::Text);

        tableDat->setColName(itRTframeduration, "RT-Frame-Duration");
        tableDat->setColPlotDesignation(itRTframeduration, Table::None);
        tableDat->setColumnType(itRTframeduration, Table::Text);

        tableDat->setColName(itSumvsmask, "Sum-vs-Mask");
        tableDat->setColPlotDesignation(itSumvsmask, Table::Y);
        tableDat->setColumnType(itSumvsmask, Table::Numeric);

        tableDat->setColName(itSumvsmaskdeadtimecorrected, "Sum-vs-Mask-Dead-Time-Corrected");
        tableDat->setColPlotDesignation(itSumvsmaskdeadtimecorrected, Table::Y);
        tableDat->setColumnType(itSumvsmaskdeadtimecorrected, Table::Numeric);

        tableDat->setColName(itQ2vsmask, "Q2-vs-Mask");
        tableDat->setColPlotDesignation(itQ2vsmask, Table::Y);
        tableDat->setColumnType(itQ2vsmask, Table::Numeric);

        tableDat->setColName(itAttenuator, "Attenuator");
        tableDat->setColPlotDesignation(itAttenuator, Table::None);
        tableDat->setColumnType(itAttenuator, Table::Text);

        tableDat->setColName(itLenses, "Lenses");
        tableDat->setColPlotDesignation(itLenses, Table::None);
        tableDat->setColumnType(itLenses, Table::Text);

        tableDat->setColName(itSlicesCount, "Slices-Count");
        tableDat->setColPlotDesignation(itSlicesCount, Table::Y);
        tableDat->setColumnType(itSlicesCount, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCount, false);

        tableDat->setColName(itSlicesDuration, "Slices-Duration");
        tableDat->setColPlotDesignation(itSlicesDuration, Table::Y);
        tableDat->setColumnType(itSlicesDuration, Table::Numeric);
        tableDat->setColNumericFormat(Table::Default, 6, itSlicesDuration, false);

        tableDat->setColName(itSlicesCurrentNumber, "Slices-Current-Number");
        tableDat->setColPlotDesignation(itSlicesCurrentNumber, Table::Y);
        tableDat->setColumnType(itSlicesCurrentNumber, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentNumber, false);

        tableDat->setColName(itSlicesCurrentDuration, "Slices-Current-Duration");
        tableDat->setColPlotDesignation(itSlicesCurrentDuration, Table::Y);
        tableDat->setColumnType(itSlicesCurrentDuration, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentDuration, false);

        tableDat->setColName(itSlicesCurrentMonitor1, "Slices-Current-Monitor1");
        tableDat->setColPlotDesignation(itSlicesCurrentMonitor1, Table::Y);
        tableDat->setColumnType(itSlicesCurrentMonitor1, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentMonitor1, false);

        tableDat->setColName(itSlicesCurrentMonitor2, "Slices-Current-Monitor2");
        tableDat->setColPlotDesignation(itSlicesCurrentMonitor2, Table::Y);
        tableDat->setColumnType(itSlicesCurrentMonitor2, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentMonitor2, false);

        tableDat->setColName(itSlicesCurrentMonitor3, "Slices-Current-Monitor3");
        tableDat->setColPlotDesignation(itSlicesCurrentMonitor3, Table::Y);
        tableDat->setColumnType(itSlicesCurrentMonitor3, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentMonitor3, false);

        tableDat->setColName(itSlicesCurrentSum, "Slices-Current-Sum");
        tableDat->setColPlotDesignation(itSlicesCurrentSum, Table::Y);
        tableDat->setColumnType(itSlicesCurrentSum, Table::Numeric);
        tableDat->setColNumericFormat(Table::Decimal, 0, itSlicesCurrentSum, false);
    }

    tableDat->setNumRows(startRaw + filesNumber);

    //+++
    tableDat->show();
    //+++
    QString fnameOnly;
    int skip;
    QString s, nameMatrix, name2ndHeader;
    int iter;
    QStringList lst;
    int index;
    //
    QString pos,num;
    

    //+++ Progress Dialog +++
    QProgressDialog progress(app());
    progress.setMinimumDuration(10);
    progress.setWindowModality(Qt::WindowModal);
    progress.setRange(0,filesNumber);
    progress.setCancelButtonText("Abort");
    progress.setMinimumWidth(400);
    progress.setMaximumWidth(4000);
    progress.setLabelText("DAN - Getting File Information ...");
    startProgress(progress);

    QLocale headerLocale = QLocale::c();
    headerLocale.setNumberOptions(QLocale::RejectGroupSeparator);

    for(iter=startRaw; iter<(startRaw+filesNumber);iter++)
    {
        stepProgress(progress, iter - startRaw);
        if ( progress.wasCanceled() ) break;
        

        tableDat->setText(iter, itFiles, selectedDat[iter - startRaw]);
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
        tableDat->setText(iter, itRuns, nameMatrix);
        //+++ Collimation [itC]
        tableDat->setCell(iter, itC, int(collimation->readCinM(name2ndHeader, lst)));
        //+++ Detector Z [itD]
        tableDat->setCell(iter, itD, detector->readDinM(name2ndHeader, lst));
        //+++ Lambda [itLambda]
        tableDat->setCell(iter, itLambda,
                          selector->readLambda(name2ndHeader, monitors->readDuration(name2ndHeader, lst), lst));
        //+++ Sum [itSum]
        tableDat->setCell(iter, itSum, monitors->readSum(name2ndHeader, 0.0, lst));
        //+++ Duration [itDuration]
        tableDat->setCell(iter, itDuration, monitors->readDuration(name2ndHeader, lst));
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
        tableDat->setCellOrText(iter, itBeamwindowX, parserHeader->readNumberString(name2ndHeader, "[CA-X]", lst),
                                headerLocale);
        //+++ Beamwindow_Y 	[itBeamwindowY]
        tableDat->setCellOrText(iter, itBeamwindowY, parserHeader->readNumberString(name2ndHeader, "[CA-Y]", lst),
                                headerLocale);
        //+++ Detector Offet [itOffset]
        tableDat->setCellOrText(iter, itOffset, parserHeader->readNumberString(name2ndHeader, "[C,D-Offset]", lst),
                                headerLocale);
        //+++ DetectorX [itXposition]
        tableDat->setCell(iter, itXposition, detector->readDetectorX(name2ndHeader, lst));
        //+++ DetectorY [itYposition]
        tableDat->setCell(iter, itYposition, detector->readDetectorY(name2ndHeader, lst));
        //+++ Sample discription 	[itSampleNr]
        tableDat->setCellOrText(iter, itSampleNr, sample->readPositionNumber(name2ndHeader, lst), headerLocale);
        //+++ SMotor-1 [itMotor1]
        tableDat->setCellOrText(iter, itMotor1, sample->readMotor1(name2ndHeader, lst), headerLocale);
        //+++ SMotor-2 [itMotor2]
        tableDat->setCellOrText(iter, itMotor2, sample->readMotor2(name2ndHeader, lst), headerLocale);
        //+++ Thickness [itThickness]
        tableDat->setCell(iter, itThickness, sample->readThickness(name2ndHeader, lst));
        //+++ BeamWin-Xs 	[itBeamWinXs]
        tableDat->setCellOrText(iter, itBeamWinXs, parserHeader->readNumberString(name2ndHeader, "[SA-X]", lst),
                                headerLocale);
        //+++ BeamWin-Ys 	[itBeamWinYs]
        tableDat->setCellOrText(iter, itBeamWinYs, parserHeader->readNumberString(name2ndHeader, "[SA-Y]", lst),
                                headerLocale);
        //+++ SA-Pos-X 	[itBeamWinXpos]
        tableDat->setCellOrText(iter, itBeamWinXpos, parserHeader->readNumberString(name2ndHeader, "[SA-Pos-X]", lst),
                                headerLocale);
        //+++ SA-Pos-Y 	[itBeamWinYpos]
        tableDat->setCellOrText(iter, itBeamWinYpos, parserHeader->readNumberString(name2ndHeader, "[SA-Pos-Y]", lst),
                                headerLocale);
        //+++ Time-Factor [itTimeFactor]
        tableDat->setCellOrText(iter, itTimeFactor,
                                parserHeader->readNumberString(name2ndHeader, "[Duration-Factor]", lst), headerLocale);
        //+++ Comment1 [itComment1]
        tableDat->setText(iter, itComment1, sample->readComment1(name2ndHeader, lst));
        //+++ Comment2 		[itComment2]
        tableDat->setText(iter, itComment2, sample->readComment2(name2ndHeader, lst));
        //+++ Experimental Title 	[itName]
        tableDat->setText(iter, itName, sample->readExperimentTitle(name2ndHeader, lst));
        //+++ User Name [itWho]
        tableDat->setText(iter, itWho, sample->readUserName(name2ndHeader, lst));
        //+++ Selector [itSelector]
        tableDat->setCellOrText(iter, itSelector, parserHeader->readNumberString(name2ndHeader, "[Selector]", lst),
                                headerLocale);
        //+++ Monitor1 [itMonitor1]
        tableDat->setCellOrText(iter, itMonitor1, parserHeader->readNumberString(name2ndHeader, "[Monitor-1]", lst),
                                headerLocale);
        //+++ Monitor2 		[itMonitor2]
        tableDat->setCellOrText(iter, itMonitor2, parserHeader->readNumberString(name2ndHeader, "[Monitor-2]", lst),
                                headerLocale);
        //+++ Monitor3 		[itMonitor3]
        tableDat->setCellOrText(iter, itMonitor3,
                                parserHeader->readNumberString(name2ndHeader, "[Monitor-3|Tr|ROI]", lst), headerLocale);
        //+++ RT-Current-Number [itRTnumber]
        tableDat->setText(iter, itRTnumber, QString::number(tofrt->readRtCurrentNumber(name2ndHeader, lst)));
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
        tableDat->setCellOrText(iter, itSlicesCount,
                                parserHeader->readNumberString(name2ndHeader, "[Slices-Count]", lst), headerLocale);
        //+++ Slices-Duration
        tableDat->setCellOrText(iter, itSlicesDuration, tofrt->readSlicesDuration(name2ndHeader, lst), headerLocale);
        //+++ Slices-Current-Number
        tableDat->setCellOrText(iter, itSlicesCurrentNumber,
                                parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Number]", lst),
                                headerLocale);
        //+++ Slices-Current-Duration
        tableDat->setCellOrText(iter, itSlicesCurrentDuration,
                                parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Duration]", lst),
                                headerLocale);
        //+++ Slices-Current-Monitor1
        tableDat->setCellOrText(iter, itSlicesCurrentMonitor1,
                                parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Monitor1]", lst),
                                headerLocale);
        //+++ Slices-Current-Monitor2
        tableDat->setCellOrText(iter, itSlicesCurrentMonitor2,
                                parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Monitor2]", lst),
                                headerLocale);
        //+++ Slices-Current-Monitor3
        tableDat->setCellOrText(iter, itSlicesCurrentMonitor3,
                                parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Monitor3]", lst),
                                headerLocale);
        //+++ Slices-Current-Sum
        tableDat->setCellOrText(iter, itSlicesCurrentSum,
                                parserHeader->readNumberString(name2ndHeader, "[Slices-Current-Sum]", lst),
                                headerLocale);
        // cps
        double sumValue = 0.0;
        double durationValue = 0.0;
        const bool hasSum = tableDat->tryNumericCell(iter, itSum, sumValue);
        const bool hasDuration = tableDat->tryNumericCell(iter, itDuration, durationValue);
        bool hasTimeFactor = false;
        const double timeFactor = tableDat->text(iter, itRTtimefactor).toDouble(&hasTimeFactor); // C-style Text column.
        if (hasSum && hasDuration && durationValue != 0.0 && hasTimeFactor)
        {
            const double cps = sumValue / durationValue * timeFactor; // 2015 RT-TOF
            tableDat->setCell(iter, itCps, cps);
        }
        else
            tableDat->clearCell(iter, itCps);

        // integral
        if (checkBoxSumVsMask->isChecked())
        {
            tableDat->setCell(iter, itSumvsmask, integralVSmaskSimmetrical(nameMatrix));
            tableDat->setCell(iter, itSumvsmaskdeadtimecorrected, integralVSmaskUniDeadTimeCorrected(nameMatrix));
            tableDat->setCell(iter, itQ2vsmask, Q2_VS_maskSimmetrical(nameMatrix));
        }
        else
        {
            tableDat->clearCell(iter, itSumvsmask);
            tableDat->clearCell(iter, itSumvsmaskdeadtimecorrected);
            tableDat->clearCell(iter, itQ2vsmask);
        }
        // Frame Duration
        bool hasRepetitions = false;
        const double repetitions = tableDat->text(iter, itRTrepetitions).toDouble(&hasRepetitions);
        if (hasDuration && hasRepetitions && hasTimeFactor)
        {
            double frameDuration = durationValue * repetitions;
            if (timeFactor != 0.0)
                frameDuration /= timeFactor;
            tableDat->setText(iter, itRTframeduration, QString::number(frameDuration));
        }
        else
            tableDat->clearCell(iter, itRTframeduration);
        
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

    tableDat->setMaximized();

    int tt;    
    // adjust columns
    tableDat->adjustColumnsWidth(false);

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

    for (tt = 0; tt < tableDat->numCols(); tt++)
        if (!checkBoxShortList->isChecked())
            tableDat->table()->showColumn(tt);

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
    bool ok;
    QString TableName = QInputDialog::getText(this,
                                              "Big-Matrix's Generation: All-in-one", "Enter name of Matrix:",
                                              QLineEdit::Normal,
                                              "info-matrix", &ok);
    if ( !ok ||  TableName.isEmpty() )
    {
        return;
    }
    newInfoMatrix(TableName);
}
//+++ a new (empty) info-matrix, the active one; its name (made unique, "_/\\%," as "-")
QString dan18::newInfoMatrix(QString TableName)
{
    int MD = lineEditMD->text().toInt();

    // the name as the matrix gets it, so that the list selects the matrix made
    TableName = TableName.replace("_", "-").replace("/", "-").replace("\\", "-").replace("%", "-").replace(",", "-");

    QStringList infoMatrixList = app()->findMatrixListByLabel("[1,1]");

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
    
    
    
    QString nameMatrix = TableName;

    // generate label
    QString label="[1,1]";
    
    makeMatrixUni(bigMatrix,nameMatrix, label, bigMatrix->size2, bigMatrix->size1,false, true);
    
    gsl_matrix_free(bigMatrix);
    return nameMatrix;
}

//++++++SLOT::Make Table++
void dan18::slotMakeBigMatrix()
{
    QString Dir = filesManager->pathInString();
    QString DirOut = filesManager->pathOutString();
    QString wildCard = filesManager->wildCardDetector();

    if (checkBoxSortOutputToFolders->isChecked())
        app()->changeFolder("DAN :: rawdata");

    QString wildCardLocal = wildCard;
    
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
    
    QString DirLocal = Dir;
    if (checkBoxBigMatrixASCII->isChecked())
    {
        filter="*.DAT";
        DirLocal = DirOut + "/ASCII-I/";
        DirLocal = DirLocal.replace("//", "/");
    }

    //+++ select files
    auto *fd = new QFileDialog(this, "Getting File Information", DirLocal, "*");
    
    fd->setDirectory(DirLocal);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Getting File Information"));
    if (checkBoxBigMatrixASCII->isChecked()) fd->setNameFilter("I-*-*-*.DAT;;"+filter + ";;"+ textEditPattern->text());
    else fd->setNameFilter(filter + ";;"+ textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (fd->exec() == QDialog::Rejected)
        return;
    
    QStringList selectedDat=fd->selectedFiles();
    
    slotMakeBigMatrix(selectedDat);
}

//++++++SLOT::Make Table++
void dan18::slotMakeBigMatrix(QStringList selectedDat)
{
    QString Dir = filesManager->pathInString();
    QString DirOut = filesManager->pathOutString();
    QString wildCard = filesManager->wildCardDetector();
    bool dirsInDir = filesManager->subFoldersYN();
    int MD = lineEditMD->text().toInt();

    QString activeMatrix=comboBoxInfoMatrix->currentText();
    if (activeMatrix=="") return;

    QStringList infoMatrixList = app()->findMatrixListByLabel("[1,1]");

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
    
    QString DirLocal = Dir;

    if (checkBoxBigMatrixASCII->isChecked())
    {
        filter = "*.DAT";
        DirLocal = DirOut + "/ASCII-I/";
        DirLocal = DirLocal.replace("//", "/");
    }

    int i;

    int filesNumber= selectedDat.count();
    
    if (filesNumber==0)
    {
        QMessageBox::critical( 0, "qtiSAS", "Nothing was selected");
        return;
    }

    QString test = selectedDat[0];
    if (test.contains(DirLocal))
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
        
        QStringList lstXY = ss.split(",", Qt::SkipEmptyParts);
        
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
                     selectedDat[i].remove(DirLocal).remove("I-").remove(".DAT").remove(
                         "-" + comboBoxInstrument->currentText()) +
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
    QString wildCard = filesManager->wildCardDetector();

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
            text = tableTextC(t, mm, nn).simplified();
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
        QStringList lstNumberIn = Number.split(";", Qt::SkipEmptyParts);
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
    bool dirsInDir = filesManager->subFoldersYN();
    int MD = lineEditMD->text().toInt();

    //+++
    bool asciiYN=checkBoxBigMatrixASCII->isChecked();
    if (!fromComboBox) asciiYN=false;
    
    QStringList lstNumberIn = NumberIn.split(";", Qt::SkipEmptyParts);
    
    if (lstNumberIn.count()>1) NumberIn = lstNumberIn[0];

    QString Number = NumberIn;

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
    else if (whatToCheck == "Polarization")
    {
        lineEditCheckRes->setText(collimation->readPolarization(Number));
    }
    else if (whatToCheck == "Polarization-Aliased")
    {
        QString polarization = collimation->readPolarization(Number);
        polarizedAlias(polarization);
        lineEditCheckRes->setText(polarization);
    }
    else if (whatToCheck == "Polarizer-Polarization")
    {
        lineEditCheckRes->setText(QString::number(
            polarizationSelector->getValue(selector->readLambda(Number, monitors->readDuration(Number)), Number)));
    }
    else if (whatToCheck == "Polarizer-Transmission")
    {
        lineEditCheckRes->setText(QString::number(
            polTransmissionSelector->getValue(selector->readLambda(Number, monitors->readDuration(Number)), Number)));
    }
    else if (whatToCheck == "Polarizer-Flipper-Efficiency")
    {
        lineEditCheckRes->setText(QString::number(polFlipperEfficiencySelector->getValue(
            selector->readLambda(Number, monitors->readDuration(Number)), Number)));
    }
    else if (whatToCheck == "Analyzer-Transmission")
    {
        lineEditCheckRes->setText(QString::number(analyzerTransmissionSelector->getValue(
            selector->readLambda(Number, monitors->readDuration(Number)), Number)));
    }
    else if (whatToCheck == "Analyzer-Efficiency")
    {
        lineEditCheckRes->setText(QString::number(analyzerEfficiencySelector->getValue(
            selector->readLambda(Number, monitors->readDuration(Number)), Number)));
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
                ParserASCII::readMatrixByNameGSL(lineEditPathRAD->text() + "/ASCII-I/" + NumberIn + ".DAT", data);
            }
            else
            {
                if (!readMatrix( Number, DD, RegionOfInteres, binning, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, data) )
                {
                    gsl_matrix_free(data);
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

                gsl_matrix_free(mask);
                gsl_matrix_free(sens);
            }

            if (!asciiYN)
                lineEditCheckRes->setText(sample->readName(Number));

            Number=Number.replace("/","-").replace("[","-r").remove("]"); //new 2017

            QString matrixName="Matrix-"+Number;
            if (activeYN) matrixName="Matrix-Active";

            bool exist = app()->existWindow(matrixName);

            QString sLabel;

            if (!asciiYN)
                sLabel = "raw-matrix :: " + sample->readName(NumberIn);
            else
                sLabel = "ascii-matrix :: " + NumberIn + ".DAT";

            makeMatrixSymmetric(data,matrixName,sLabel, MD);

            if (!exist)
            {
                app()->activeWindow()->setNormal();
                app()->maximizeWindow(matrixName);
            }
            gsl_matrix_free(data);
        }
    }
    else if (whatToCheck.contains("View I(Q)"))
    {
        viewIQ(whatToCheck, Number, lstNumberIn);

    }
    
    if (whatToCheck.contains("Plot Matrix") && app()->existWindow("Matrix-Active"))
    {
        if (!app()->existWindow("Plot-Active"))
        {
            app()->maximizeWindow("Matrix-Active");
            app()->plotColorMap();
            QString oldName=((MdiSubWindow* )app()->activeWindow())->name();
            app()->setWindowName((MdiSubWindow* )app()->activeWindow(),  "Plot-Active");
            app()->renameListViewItem(oldName, "Plot-Active");
            app()->updateWindowStatus ((MdiSubWindow * )app()->activeWindow());
            
        }
        app()->activeWindow()->setNormal();
        app()->maximizeWindow("Plot-Active");

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
    int MD = lineEditMD->text().toInt();

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
    double Detector = detector->readD(Number);  // [cm]
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
    int MD = lineEditMD->text().toInt();

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
    
    Graph *g;
    if (!app()->findActiveGraph(g))
        return;

    QString tableName2plot = "raw-QI";
    if (!whatToCheck.contains("raw-QI"))
        tableName2plot += "-SM-" + Nsample;

    g->insertCurveScatter(tableName2plot + "_I");
}

void dan18::updateComboBoxActiveFile()
{
    QString wildCard = filesManager->wildCardDetector();

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
        QStringList lstNumberIn = lineEditCheck->text().split(";", Qt::SkipEmptyParts);
        for(int i=1; i<lstNumberIn.count();i++)commandLine+=";"+lstNumberIn[i];
    }
    
    QString fileNumber="";
    if (filesManager->selectFile(fileNumber))
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
    if (!app()->checkNoteExistence(tableName))
    {
        app()->newNoteText(tableName,s);
    }
    else
    {
        QList<MdiSubWindow*> windows = app()->windowsList();
        foreach(MdiSubWindow *w, windows) if (QString(w->metaObject()->className()) == "Note" && w->name()==tableName) ((Note *)w)->currentEditor()->setText(s);
    }
    app()->activeWindow()->setNormal();
    app()->maximizeWindow(tableName);
}
//+++ extractRawData
void dan18::extractRawData()
{
    QString Dir = filesManager->pathInString();
    QString wildCard = filesManager->wildCardDetector();
    bool dirsInDir = filesManager->subFoldersYN();

    QString filter = textEditPattern->text();

    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Getting File Information"));
    fd->setNameFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (fd->exec() == QDialog::Rejected)
        return;
    
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
    
    
    QStringList runs;
    for (int iter = 0; iter < filesNumber; iter++)
        runs << FilesManager::findFileNumberInFileName(wildCard, selectedDat[iter].remove(Dir));
    extractRawData(runs);
}
//+++ the runs' raw matrices as files <output folder>/raw-matrix/only_<run>_rawdata.matrix; the files written
QStringList dan18::extractRawData(const QStringList &runs)
{
    int MD = lineEditMD->text().toInt();
    QString nameMatrix;
    QString newName;
    QString DirOut 	= lineEditPathRAD->text();
    QDir dd;
    if (!dd.cd(DirOut+"/raw-matrix"))
    {
        dd.mkdir(DirOut+"/raw-matrix");
    }

    QStringList written;
    for (int iter = 0; iter < runs.count(); iter++)
    {
        nameMatrix = runs[iter];
        gsl_matrix *data = gsl_matrix_alloc(MD, MD);
        gsl_matrix_set_all(data, 0.0);
        readMatrix(nameMatrix, data);

        newName = QDir(DirOut).filePath("raw-matrix/only_" + nameMatrix + "_rawdata.matrix");

        if (checkBoxSaveMatrixDouble->isChecked())
            saveMatrixToFile(newName, data, MD);
        else
            saveMatrixToFileInteger(newName, data, MD);
        gsl_matrix_free(data);
        written << newName;
    }
    return written;
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
        lst << "Polarization";
        lst << "Polarization-Aliased";
        lst<<"[Info]";
        lst<<"RT-normalization";
        lst<<"Q2-vs-Mask";
    }

    comboBoxCheck->addItems(lst);
    // the action chosen before selected again (not the first entry renamed to it)
    comboBoxCheck->setCurrentIndex(static_cast<int>(
        lst.contains(whatToCheck) ? lst.indexOf(whatToCheck) : lst.indexOf("Plot Matrix [Plot-Active]")));
    comboBoxCheck->blockSignals(false);
}
