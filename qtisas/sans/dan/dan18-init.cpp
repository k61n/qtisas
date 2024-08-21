/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2024 Artem Feoktystov <a.feoktystov@fz-juelich.de>
Description: SANS init tools
 ******************************************************************************/

#include <map>

#include <QScreen>
#include <QStringList>
#include <QToolBox>

#include "Folder.h"
#include "dan18.h"

//*******************************************
//+++ connect Slots
//*******************************************
void dan18::connectSlot()
{
    // top panel buttons
    connect(pushButtonNewSession, SIGNAL(clicked()), this, SLOT(newSession()));
    connect(pushButtonOpenSession, SIGNAL(clicked()), this, SLOT(openSession()));
    connect(pushButtonInstrLabel, SIGNAL(clicked()), this, SLOT(instrumentSelectedByButton()));

    // instrument buttons
    connect(comboBoxInstrument, SIGNAL(activated(int)), this, SLOT(instrumentSelected()));
    connect( pushButtonsaveCurrentSaveInstr, SIGNAL( clicked() ), this, SLOT( saveInstrumentAs() ) );
    connect( pushButtonDeleteCurrentInstr, SIGNAL( clicked() ), this, SLOT( deleteCurrentInstrument() ) );
    connect( pushButtonInstrColor , SIGNAL( clicked() ), this, SLOT( selectInstrumentColor() ) );

    // experimental mode
    connect(comboBoxMode, SIGNAL(activated(int)), this, SLOT(experimentalModeSelected()));

    // sanstab
    connect( sansTab, SIGNAL( currentChanged(int) ), this, SLOT(tabSelected() ) );
    
    connect( lineEditAsymetry, SIGNAL( textChanged(const QString&) ), lineEditAsymetryMatrix, SLOT( setText(const QString&) ) );
}

void dan18::changeFixedSizeH(QWidget *obj, int H)
{
    obj->setMinimumSize(0,H);
    obj->setMaximumSize(32767,H);
}

void dan18::changeFixedSizeHW(QWidget *obj, int H, int W)
{
    obj->setMinimumSize(W,H);
    obj->setMaximumSize(W,H);
}


//*******************************************
//*initScreenResolusionDependentParameters
//*******************************************
void dan18::initScreenResolusionDependentParameters(int hResolusion, double sasResoScale)
{

#ifdef Q_OS_LINUX
    return;
#endif
    
    int labelHight= int(hResolusion*sasResoScale/45); //int(hResolusion*sasResoScale/50);
    int fontIncr=app()->sasFontIncrement+1;
    
    int newH=labelHight;
    int newW=int(double(labelHight)/1.5);
    
    
    if (labelHight<30) {newH=30; newW=15;};
    
    int rowDelta=0;
#ifdef Q_OS_MACOS
    if (QGuiApplication::primaryScreen()->availableGeometry().width() < 1700)
        rowDelta = 4;
#endif
    
    //+++ tables
    double tableFactor=0.75;
#ifdef Q_OS_MACOS
    tableFactor=1.0;
#endif
    foreach( QTableWidget *obj, this->findChildren< QTableWidget * >( ) )
    {
        /*
        obj->verticalHeader()->setDefaultSectionSize(int(tableFactor*(labelHight+4+rowDelta))); //->setDefaultSectionSize(labelHight+4+rowDelta); <2020
        obj->verticalHeader()->setMinimumWidth(int(tableFactor*(labelHight+4+rowDelta)));
        
        obj->setIconSize(QSize(int(0.5*labelHight),int(0.5*labelHight)));
        obj->verticalHeader()->setIconSize(QSize(int(0.5*labelHight),int(0.5*labelHight)));
         */
    }
    
    //+++ QToolBox
    foreach( QToolBox *obj, this->findChildren< QToolBox * >( ) )
    {
        if(obj->baseSize().height()>0)
        {
            obj->setMinimumHeight(int(obj->baseSize().height()*sasResoScale));
            obj->setMaximumHeight(int(obj->baseSize().height()*sasResoScale));
        }
    }

    
    //+++ Labels
    foreach( QLabel *obj, this->findChildren< QLabel * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
        
        //obj->setMinimumWidth(3*labelHight);
    }
    
    textLabelInfo_2->setMaximumHeight(3000);
    
    //+++ QLineEdit
    foreach( QLineEdit *obj, this->findChildren< QLineEdit * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
        //+++
        obj->setStyleSheet("background-color: rgb(255, 255, 195);");
    }

    //+++ QTextEdit

    textEditPattern->setMinimumHeight(labelHight);
    textEditPattern->setMaximumHeight(labelHight);
    /*
    textEditPattern->setMinimumWidth(5*labelHight);
    textEditPattern->setMaximumWidth(5*labelHight);
    */
    
    //+++ QPushButton
    foreach( QPushButton *obj, this->findChildren< QPushButton * >( ) )
    {
        if(obj->baseSize().width()==25)
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            obj->setMinimumWidth(labelHight);
            obj->setMaximumWidth(labelHight);
            
            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
        }
        else if(obj->baseSize().width()==50)
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            obj->setMinimumWidth(2*labelHight+2);
            obj->setMaximumWidth(2*labelHight+2);
            
            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
        }
        else if(obj->baseSize().width()==14)
        {
            QFont fonts=obj->font();
            obj->setMaximumWidth(fonts.pointSize()+5);
            obj->setMaximumWidth(fonts.pointSize()+5);
            obj->setMinimumHeight(fonts.pointSize()+5);
            obj->setMaximumHeight(fonts.pointSize()+5);
        }
        else if(obj->baseSize().width()==10)
        {
            obj->setMinimumHeight(newH);
            obj->setMaximumHeight(newH);
        }
        else
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
        }
    }
    //+++ QToolButton
    foreach( QToolButton *obj, this->findChildren< QToolButton * >( ) )
    {
        if(obj->baseSize().width()==25)
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            obj->setMinimumWidth(labelHight);
            obj->setMaximumWidth(labelHight);
            
            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
        }
        else if(obj->baseSize().width()==50)
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            obj->setMinimumWidth(2*labelHight+5);
            obj->setMaximumWidth(2*labelHight+5);
            
            obj->setIconSize(QSize(int(0.65*labelHight),int(0.65*labelHight)));
        }
        else if(obj->baseSize().width()==14)
        {
            QFont fonts=obj->font();
            obj->setMaximumWidth(fonts.pointSize()+5);
            obj->setMaximumWidth(fonts.pointSize()+5);
            obj->setMinimumHeight(fonts.pointSize()+5);
            obj->setMaximumHeight(fonts.pointSize()+5);
        }
        else if(obj->baseSize().height()==25 && obj->baseSize().width()==75)
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            obj->setMinimumWidth(3*labelHight);
            obj->setMaximumWidth(3*labelHight);

            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));

        }
        else if(obj->baseSize().height()==50)
        {
            obj->setMinimumHeight(2*labelHight+1);
            obj->setMaximumHeight(2*labelHight+1);
            
            obj->setMinimumWidth(2*labelHight+1);
            obj->setMaximumWidth(2*labelHight+1);
            
            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
        }
        else
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            
            if (obj->sizeIncrement().width()>0)
            {
                int baseWidth=obj->sizeIncrement().width();
                if (fontIncr>0)  baseWidth=baseWidth + 2*fontIncr*obj->text().length();
                
                obj->setMinimumWidth(baseWidth);
                obj->setMaximumWidth(baseWidth);
            }
            
            
            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
        }
    }
    
    
    //+++ QCheckBox
    foreach( QCheckBox *obj, this->findChildren< QCheckBox * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }

    int comboInc=0;
#ifdef Q_OS_MACOS
    comboInc=5;
#endif
    //+++ QComboBox
    foreach(QComboBox *obj, this->findChildren< QComboBox * >( ) )
    {
        obj->setMinimumHeight(labelHight+comboInc);
        obj->setMaximumHeight(labelHight+comboInc);
    }

    //+++ QSpinBox
    foreach(QSpinBox *obj, this->findChildren< QSpinBox * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }
    //+++ QToolBox
    toolBox->setMinimumHeight(2*labelHight+25);
    toolBox->setMaximumHeight(2*labelHight+25);


}



//*******************************************
//+++   init at start
//*******************************************
void dan18::initDAN()
{

    spinBoxRebinQmin->setHidden(true);
    spinBoxRebinQmax->setHidden(true);
    toolBoxMerge->setCurrentIndex(0);
    mergeJoinVsRebin(0);
    checkBoxMergeFromTable->setChecked(true);
    mergeFromProjectTable(false, false);

    sansTab->setCurrentIndex(0);
    toolBoxOptions->setCurrentIndex(0);
    stackedWidgetConfig->setCurrentIndex(0);
    comboBoxConfig->setCurrentIndex(0);
    toolBoxAdv->setCurrentIndex(0);
    toolBoxMask->setCurrentIndex(0);
    toolBoxSens->setCurrentIndex(0);
    toolBoxProcess->setCurrentIndex(0);
    stackedWidgetDpOptions->setCurrentIndex(0);
    stackedWidgetDpOptions2D->setCurrentIndex(0);
    stackedWidgetDpOptions1D->setCurrentIndex(0);
    
    //+++ HIDE instrumet button
    pushButtonInstrLabel->hide();
    
    //+++ show first Info Page
    expandModeSelection(true);

    //+++ Processing Table

    extractorInit(); //2017

    //+++ new / 2023 / Files Manager
    filesManager =
        new FilesManager(lineEditPathDAT, textEditPattern, checkBoxDirsIndir, pushButtonDATpath, lineEditPathRAD,
                         pushButtonRADpath, lineEditWildCard, lineEditWildCard2ndHeader, checkBoxYes2ndHeader);

    //+++ new / 2023 / Header Parser
    parserHeader = new ParserHeader(filesManager, tableHeaderPosNew, comboBoxHeaderFormat, buttonGroupXMLbase,
                                    lineEditXMLbase, buttonGroupFlexibleHeader, checkBoxHeaderFlexibility,
                                    lineEditFlexiStop, spinBoxHeaderNumberLines, spinBoxHeaderNumberLines2ndHeader,
                                    spinBoxDataHeaderNumberLines, checkBoxRemoveNonePrint);

    //+++ new / 2023 / Detector-Related-Parameter's Parser
    detector = new Detector(parserHeader, comboBoxUnitsD, radioButtonDetRotHeaderX, doubleSpinBoxDetRotX,
                            checkBoxInvDetRotX, radioButtonDetRotHeaderY, doubleSpinBoxDetRotY, checkBoxInvDetRotY,
                            checkBoxBeamcenterAsPara, checkBoxDetRotAsPara);

    //+++ new / 2023 / Collimation-Related-Parameter's Parser
    collimation = new Collimation(parserHeader, comboBoxUnitsC, checkBoxResoCAround, checkBoxResoSAround,
                                  comboBoxUnitsBlends, checkBoxAttenuatorAsPara, checkBoxPolarizationAsPara);

    //+++ new / 2023 / Sample-Related-Parameter's Parser
    sample = new Sample(parserHeader, comboBoxThicknessUnits, checkBoxRecalculateUseNumber);

    //+++ new / 2023 / Selector-Related-Parameter's Parser
    selector = new Selector(parserHeader, comboBoxUnitsLambda, comboBoxUnitsSelector, radioButtonLambdaF,
                            radioButtonLambdaHeader, lineEditSel1, lineEditSel2);

    //+++ new / 2023 / Monitor-Related-Parameter's Parser
    monitors = new Monitors(parserHeader, comboBoxUnitsTime, lineEditDeadTimeM1, lineEditDeadTimeM2, lineEditDeadTimeM2,
                            lineEditDeadTime, lineEditDBdeadtime, radioButtonDeadTimeDet, spinBoxNorm, comboBoxNorm);

    //+++ new / 2023 / TOF/RT-Related-Parameter's Parser
    tofrt = new Tofrt(parserHeader, comboBoxUnitsTimeRT);

    //+++ new / 2024 / CONFIGURATION / Polarization Widget
    polarizationSelector = new ConfigurationSelector(parserHeader, radioButtonPolarizerConst, radioButtonPolarizerTable,
                                                     radioButtonPolarizerHeader, doubleSpinPolarization,
                                                     lineEditPolarizationTable, "Polarizer-Polarization");
    //+++ new / 2024 / CONFIGURATION / Polarization Transmission Widget
    polTransmissionSelector =
        new ConfigurationSelector(parserHeader, radioButtonPolTransmissionConst, radioButtonPolTransmissionTable,
                                  radioButtonPolTransmissionHeader, doubleSpinPolTransmission,
                                  lineEditPolTransmissionTable, "Polarizer-Transmission");
    //+++ new / 2024 / CONFIGURATION / Polarization Flipper Efficiency Widget
    polFlipperEfficiencySelector = new ConfigurationSelector(
        parserHeader, radioButtonPolFlipperEfficiencyConst, radioButtonPolFlipperEfficiencyTable,
        radioButtonPolFlipperEfficiencyHeader, doubleSpinPolFlipperEfficiency, lineEditPolFlipperEfficiencyTable,
        "Polarizer-Flipper-Efficiency");
    //+++ new / 2024 / CONFIGURATION / Analyzer Transmission Widget
    analyzerTransmissionSelector = new ConfigurationSelector(
        parserHeader, radioButtonAnalyzerTransmissionConst, radioButtonAnalyzerTransmissionTable,
        radioButtonAnalyzerTransmissionHeader, doubleSpinAnalyzerTransmission, lineEditAnalyzerTransmissionTable,
        "Analyzer-Transmission");
    //+++ new / 2024 / CONFIGURATION / Analyzer Efficiency Widget
    analyzerEfficiencySelector =
        new ConfigurationSelector(parserHeader, radioButtonAnalyzerEfficiencyConst, radioButtonAnalyzerEfficiencyTable,
                                  radioButtonAnalyzerEfficiencyHeader, doubleSpinAnalyzerEfficiency,
                                  lineEditAnalyzerEfficiencyTable, "Analyzer-Efficiency");

    //+++ new / 2024 / ScriptTableManager
    scriptTableManager = new ScriptTableManager(comboBoxMode, comboBoxPolReductionMode);

    tableEC->horizontalHeader()->setVisible(true);
    tableEC->verticalHeader()->setVisible(true);
    for (int i=0; i<tableEC->rowCount();i++) tableEC->setItem(i, 0, new QTableWidgetItem);

    tableEC->setHorizontalHeaderItem(0,new QTableWidgetItem("1"));
    tableEC->horizontalHeader()->setStretchLastSection(true);
    
    
    
    QAbstractButton *cornerButton = tableEC->findChild<QAbstractButton*>();
    // Since it's not part of the API, there is no guarantee it exists
    if (cornerButton)
    {
        cornerButton->disconnect();
        connect(cornerButton, SIGNAL(clicked()),this,SLOT(tableECcorner()));
        cornerButton->setStyleSheet("background-color: rgba(0,0,0,0);border: 1px solid rgb(137, 137, 183);padding:0px;padding-left:5px;");
        cornerButton->setText("all");
        //cornerButton->setToolTip("All refresh buttons...");
    }
}

void dan18::tableECcorner()
{
    vertHeaderTableECPressed(dptC);
    vertHeaderTableECPressed(dptD);
    vertHeaderTableECPressed(dptWL);
    vertHeaderTableECPressed(dptBSIZE);
    vertHeaderTableECPressed(dptPOL);
    vertHeaderTableECPressed(dptDAC);
    vertHeaderTableECPressed(dptACMU);
    vertHeaderTableECPressed(dptACTR);
    vertHeaderTableECPressed(dptACFAC);
    vertHeaderTableECPressed(dptCENTERX);
    vertHeaderTableECPressed(dptCENTERY);
    vertHeaderTableECPressed(dptECTR);
}

//*******************************************
//+++  New Session
//*******************************************
void dan18::newSession()
{
    findSANSinstruments();
    newInfoExtractor("");

    tableEC->setColumnCount(1);
    tableEC->setHorizontalHeaderItem(0, new QTableWidgetItem(QString::number(1)));
    for (int ii = 0; ii < tableEC->rowCount(); ii++)
        tableEC->setItem(ii, 0, new QTableWidgetItem);

    //+++ show first Info Page
    expandModeSelection(false);

    //+++ Calibrant
    comboBoxCalibrant->setEnabled(true);
    comboBoxCalibrant->setCurrentIndex(0);
    calibrantselected();

    updateMaskList();
    updateSensList();
    addMaskAndSens(tableEC->columnCount());

    instrumentSelected();

    findCalibrators();
    updateScriptTables();
    updatePolScriptTables();

    pushButtonInstrLabel->show();
    pushButtonNewSession->setMaximumWidth(pushButtonNewSession->maximumHeight());
    pushButtonOpenSession->setMaximumWidth(pushButtonOpenSession->maximumHeight());

    sliderConfigurations->setValue(1);
    
    stackedWidgetDpOptions1D->setCurrentIndex(0);
    stackedWidgetDpOptions2D->setCurrentIndex(0);

    //+++ hide info lables
    textLabelInfoSAS->hide();
    textLabelInfo->hide();
    textLabelAuthor->hide();
}

//*******************************************
//+++  Mode selection
//*******************************************
void dan18::openSession()
{
    updateScriptTables();
    updatePolScriptTables();
    newInfoExtractor("");

    QStringList lst;
    for (int i = 0; i < comboBoxMakeScriptTable->count(); i++)
        lst << comboBoxMakeScriptTable->itemText(i);

    if (lst.count() == 0)
    {
        QMessageBox::warning(this, tr("QtiSAS"), tr("No SAVED session exists. Just start NEW session!"));
        return;
    }

    bool ok;
    QString res = QInputDialog::getItem(this, 
	    "QtiSAS", "Select SAVED session:", lst, 0, false, &ok);
    if ( !ok )
        return;

    newSession();

    comboBoxMakeScriptTable->setCurrentIndex(comboBoxMakeScriptTable->findText(res));

    pushButtonInstrLabel->show();

    pushButtonNewSession->setMaximumWidth(pushButtonNewSession->maximumHeight());
    pushButtonNewSession->setText("");

    pushButtonOpenSession->setMaximumWidth(pushButtonNewSession->maximumHeight());
    pushButtonOpenSession->setText("");

    activeScriptTableSelected(comboBoxMakeScriptTable->currentIndex());
}

void dan18::expandModeSelection( bool YN)
{
    if (YN)
    {
        sansTab->hide();
        buttonGroupMode->show();
        textLabelInfo_2->show();

        buttonGroupMode->setMaximumHeight(3000);
    }
    else
    {
        buttonGroupMode->hide();
        sansTab->show();
    }    
}

void dan18::instrumentSelectedByButton()
{
    int oldInstr = comboBoxInstrument->currentIndex();
    int numberInstruments = comboBoxInstrument->count();
    int newInstr;

    if (oldInstr + 1 < numberInstruments)
        newInstr = oldInstr + 1;
    else
        newInstr = 0;

    comboBoxInstrument->setCurrentIndex(newInstr);

    instrumentSelected();
}

void dan18::tabSelected()
{  
    int MD = lineEditMD->text().toInt();

    app()->lv->setFocus();
    
    int index = sansTab->currentIndex();
    
    if (index>0 && !checkDataPath() )
    {
	sansTab->setCurrentIndex(0);
	index = 0;
	QMessageBox::warning(this,tr("qtiSAS"), tr("Select correct \"Input Folder\"!"));
    }  
    
    if ( index==1 )
    {
	QString activeTable=comboBoxInfoTable->currentText();
	
	QStringList infoTablesList;
	findTableListByLabel("Info::Table", infoTablesList);    
	infoTablesList.sort();
	
	comboBoxInfoTable->clear();	
	comboBoxInfoTable->insertItems(0, infoTablesList);
	comboBoxInfoTable->setCurrentIndex(infoTablesList.indexOf(activeTable));	
	
	
	QString activeMatrix=comboBoxInfoMatrix->currentText();
	
	QStringList infoMatrixList;    
	findMatrixListByLabel("[1,1]", infoMatrixList);
	infoMatrixList.sort();
	
	comboBoxInfoMatrix->clear();	
	comboBoxInfoMatrix->insertItems(0, infoMatrixList);
	comboBoxInfoMatrix->setCurrentIndex(infoMatrixList.indexOf(activeMatrix));
        
    updateComboBoxActiveFolders();
    updateComboBoxActiveFile();
        
    }
    
    if ( index==2 )
    {
        updateMaskList();
        matrixList();
    }

    if (index == 3)
    {
        updateSensList();
        updateMaskList();
        SensitivityLineEditCheck();
    }

    if ( index==4 )
    {
        updateSensList();
        updateMaskList();
        
        //mask
        QStringList lst0, lst;
        findMatrixListByLabel("DAN::Mask::"+QString::number(MD),lst0);
        lst=lst0;
        if (!lst.contains("mask")) lst.prepend("mask");
        QString currentMask;
        
        for(int i=0;i<tableEC->columnCount();i++)
        {
            QComboBoxInTable *mask =(QComboBoxInTable*)tableEC->cellWidget(dptMASK,i);
            currentMask=mask->currentText();
            mask->clear();
            mask->addItems(lst);
            mask->setCurrentIndex(lst.indexOf(currentMask));
        }
        
        for(int i=0;i<tableEC->columnCount();i++)
        {
            QComboBoxInTable *mask =(QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,i);
            currentMask=mask->currentText();
            mask->clear();
            mask->addItems(lst);
            mask->setCurrentIndex(lst.indexOf(currentMask));
        }
        
        //sens
        findMatrixListByLabel("DAN::Sensitivity::"+QString::number(MD),lst);
        if (!lst.contains("sens")) lst.prepend("sens");
        QString currentSens;
        
        for(int i=0;i<tableEC->columnCount();i++)
        {
            QComboBoxInTable *sens =(QComboBoxInTable*)tableEC->cellWidget(dptSENS,i);
            currentSens=sens->currentText();
            sens->clear();
            sens->addItems(lst);
            sens->setCurrentIndex(lst.indexOf(currentSens));
        }
    }
    

    sansTab->setFocus();
}

//+++++SLOT::select Selector+++++++++++++++++++++++++++++++++++++++++++++++++++++
void dan18::instrumentSelected()
{
    doubleSpinPolarization->setValue(1.0);
    lineEditPolarizationTable->setText("4.5,1.000;");
    radioButtonPolarizerConst->setChecked(true);

    doubleSpinPolTransmission->setValue(0.5);
    lineEditPolTransmissionTable->setText("4.5,0.500;");
    radioButtonPolTransmissionConst->setChecked(true);

    doubleSpinPolFlipperEfficiency->setValue(1.0);
    lineEditPolFlipperEfficiencyTable->setText("4.5,1.000;");
    radioButtonPolFlipperEfficiencyConst->setChecked(true);

    doubleSpinAnalyzerTransmission->setValue(0.5);
    lineEditAnalyzerTransmissionTable->setText("4.5,0.500;");
    radioButtonAnalyzerTransmissionConst->setChecked(true);

    doubleSpinAnalyzerEfficiency->setValue(1.0);
    lineEditAnalyzerEfficiencyTable->setText("4.5,1.000;");
    radioButtonAnalyzerEfficiencyConst->setChecked(true);

    lineEditUp->setText("up");
    lineEditDown->setText("down");
    lineEditUpUp->setText("up-up");
    lineEditUpDown->setText("up-down");
    lineEditDownDown->setText("down-down");
    lineEditDownUp->setText("down-up");

    radioButtonDetectorFormatAscii->setChecked(true);

    comboBoxUnitsC->setCurrentIndex(0);

    lineEditDeadTimeM1->setText("0");
    lineEditDeadTimeM2->setText("0");
    lineEditDeadTimeM3->setText("0");

    comboBoxMode->setCurrentIndex(0);
    checkBoxASCIIheader->setChecked(true);

    checkBoxAnisotropy->setChecked(false);
    spinBoxAnisotropyOffset->setValue(0.00);
    
    spinBoxAvlinear->setValue(1);
    doubleSpinBoxAvLog->setValue(1.000);
    
    checkBoxDetRotAsPara->setChecked(false);
    
    checkBoxInvDetRotX->setChecked(false);
    checkBoxInvDetRotY->setChecked(false);
    
    radioButtonDetRotConstX->setChecked(true);
    radioButtonDetRotHeaderX->setChecked(false);
    doubleSpinBoxDetRotX->setValue(0.00);
    
    radioButtonDetRotConstY->setChecked(true);
    radioButtonDetRotHeaderY->setChecked(false);
    doubleSpinBoxDetRotY->setValue(0.00);
    
    checkBoxWaTrDet->setChecked(false);
    checkBoxAutoMerging->setChecked(false);
    spinBoxOverlap->setValue(100);
    
    lineEditSensMaskedPixels->setText("0.000");
    
    checkBoxSlicesBS->setChecked(false);
    checkBoxASCIIheaderSASVIEW->setChecked(false);
    
    comboBoxMatrixConvolusion->setCurrentIndex(0);
    
    comboBoxIxyFormat->setCurrentIndex(0);
    checkBoxASCIIheaderIxy->setChecked(false);
    
    checkBoxSkipPolar->setChecked(true);
    
    radioButtonXYdimPixel->setChecked(true);
    radioButtonXYdimQ->setChecked(false);
    
    comboBoxUnitsTimeRT->setCurrentIndex(0);
    lineEditDeadRows->setText("");
    lineEditDeadCols->setText("");
    lineEditMaskPolygons->setText("");
    
    spinBoxReadMatrixTofNumberPerLine->setValue(1);
    spinBoxDataHeaderNumberLines->setValue(0);
    spinBoxDataLinesBetweenFrames->setValue(0);
    
    //+++
    checkBoxResoCAround->setChecked(false);
    checkBoxResoSAround->setChecked(false);
    //+++
    lineEditDetReso->setText("0.0");
    checkBoxResoFocus->setChecked(false);
    comboBoxUnitsSelector->setCurrentIndex(0);
    checkBoxSortOutputToFolders->setChecked(true);
    checkBoxSensTr->setChecked(false);
    
    lineEditFileExt->setText("");

    for (int i = 0; i < parserHeader->listOfHeaders.count(); i++)
    {
        tableHeaderPosNew->item(i, 0)->setText("");
        tableHeaderPosNew->item(i, 1)->setText("");
    }

    checkBoxBCTimeNormalization->setChecked(false);
    checkBoxSkiptransmisionConfigurations->setChecked(false);

    parserHeader->dataFormatChanged(0);
    
    lineEditXMLbase->setText("");
    lineEditFlexiStop->setText("");
    checkBoxHeaderFlexibility->setChecked(false);

    radioButtonDetectorFormatAscii->setChecked(true);
    radioButtonDetectorFormatImage->setChecked(false);
    radioButtonDetectorFormatHDF->setChecked(false);
    radioButtonDetectorFormatYAML->setChecked(false);

    lineEditYamlDetectorEntry->setText("");
    lineEditYamlDetectorEntry->setHidden(true);

    lineEditHdfDetectorEntry->setText("");
    comboBoxDxDyN->setCurrentIndex(1);
    lineEditHdfDetectorEntry->setHidden(true);
    comboBoxDxDyN->setHidden(true);

    checkBoxRemoveNonePrint->setChecked(false);
    checkBoxRemoveNonePrint->setHidden(true);
    
    checkBoxTranspose->setChecked(false);
    checkBoxMatrixX2mX->setChecked(false);
    checkBoxMatrixY2mY->setChecked(false);
    
    checkBoxParallax->setChecked(true);
    checkBoxParallaxTr->setChecked(true);
    comboBoxParallax->setCurrentIndex(0);

    QString instrName = comboBoxInstrument->currentText();
    QStringList lst;
    
    // +++ optional dead time models for kws instruments ...
    if (comboBoxInstrument->currentText() == "KWS1" || comboBoxInstrument->currentText() == "KWS2")
        comboBoxDTtype->show();
    else
        comboBoxDTtype->hide();
    
    comboBoxDTtype->setCurrentIndex(0);
    
    if (instrName=="KWS1")
    {
        spinBoxWLS->setValue(10);
        
        lst << "[Instrument] KWS1";
        lst<< "[Color] #ff0000";
        lst<< "[Input-Folder] home";
        lst<< "[Output-Folder] home";
        lst << "[DataFormat] 0";
        lst << "[Include-Sub-Foldes] No";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 3";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] No";
        lst << "[2nd-Header-Pattern] ";
        lst << "[2nd-Header-Lines] 0";
        lst << "[Pattern] *_#_*.DAT";
        lst << "[Pattern-Select-Data] *_*_*.DAT";
        lst << "[Header-Number-Lines] 100";
        lst << "[Data-Header-Lines] 1";
        lst << "[Lines-Between-Frames] 1";
        lst << "[Flexible-Header] Yes";
        lst << "[Flexible-Stop] $|(* Detector Data for";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] 3;;;s_1";
        lst << "[User-Name] 5;;;s 6";
        lst << "[Sample-Run-Number] 11;;;1";
        lst << "[Sample-Title] 16;;;s|1";
        lst << "[Sample-Thickness] 34;;;3";
        lst << "[Sample-Position-Number] 34;;;1";
        lst << "[Date] 5;;;s 8";
        lst << "[Time] 5;;;s 9";
        lst << "[C] 21;;;1";
        lst << "[D] 28;;;2";
        lst << "[D-TOF] 28;;;2";
        lst << "[C,D-Offset] 28;;;1";
        lst << "[CA-X] 21;;;3";
        lst << "[CA-Y] 21;;;4";
        lst << "[SA-X] 34;;;4";
        lst << "[SA-Y] 34;;;5";
        lst << "[Sum] 62;;;1";
        lst << "[Selector] 49;;;1";
        lst << "[Lambda] {lambda=};;;sA1";
        lst << "[Delta-Lambda] const;;;0.1";
        lst << "[Duration] 58;;;1";
        lst << "[Duration-Factor] 34;;;6";
        lst << "[Monitor-1] 49;;;2";
        lst << "[Monitor-2] 49;;;3";
        lst << "[Monitor-3|Tr|ROI] 49;;;4";
        lst << "[Comment1] 14;;;s!1";
        lst << "[Comment2] 15;;;s!1";
        lst << "[Detector-X || Beamcenter-X] 28;;;3";
        lst << "[Detector-Y || Beamcenter-Y] 28;;;4";
        lst << "[Sample-Motor-1] 34;;;2";
        lst << "[Sample-Motor-2] 35;;;2";
        lst << "[Sample-Motor-3] 0;;;0";
        lst << "[Sample-Motor-4] 0;;;0";
        lst << "[Sample-Motor-5] 0;;;0";
        lst << "[SA-Pos-X] 35;;;4";
        lst << "[SA-Pos-Y] 35;;;5";
        lst << "[Field-1] 38;;;s 4";
        lst << "[Field-2] 39;;;s 4";
        lst << "[Field-3] 40;;;s 4";
        lst << "[Field-4] 41;;;s 4";
        lst << "[RT-Number-Repetitions] {RT-Number-Repetitions=};;;s 1";
        lst << "[RT-Time-Factor] {RT-Time-Factor=};;;s 1";
        lst << "[RT-Current-Number] {RT-Current-Number=};;;s 1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] 21;;;s 5";
        lst << "[Lenses] 21;;;s 6";
        lst << "[Slices-Count] {(* Detector Time Slices};;;s 2";
        lst << "[Slices-Duration] {(* Detector Time Slices}+;;;2";
        lst << "[Slices-Current-Number] {(* TOF-SPLITTED:};;;s 4";
        lst << "[Slices-Current-Duration] {Slices-Current-Duration=};;;s 1";
        lst << "[Slices-Current-Monitor1] {Slices-Current-Monitor1=};;;s 1";
        lst << "[Slices-Current-Monitor2] {Slices-Current-Monitor2=};;;s 1";
        lst << "[Slices-Current-Monitor3] {Slices-Current-Monitor3=};;;s 1";
        lst << "[Slices-Current-Sum] {Slices-Current-Sum=};;;s 1";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 2227.5";
        lst << "[Selector-P2] 0.00";
        lst << "[Detector-Data-Dimension] 1";
        lst << "[Detector-Data-Focus] 128";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.525";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 8";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 128";
        lst << "[Detector-Data-Transpose] No";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] No";
        lst << "[Detector-Dead-Time] 6.5e-7";
        lst << "[Detector-Dead-Time-DB] 2.6e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] SYM";
        lst << "[Calibrant-Type] Flat Scatter [FS]";
        lst << "[Calibrant] Plexi-1.5mm[KWS-1&2]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] Yes";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Rectangle :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 8";
        lst << "[Mask-Edge-Left-Y] 8";
        lst << "[Mask-Edge-Right-X] 121";
        lst << "[Mask-Edge-Right-Y] 121";
        lst << "[Mask-BeamStop-Left-X] 58";
        lst << "[Mask-BeamStop-Left-Y] 58";
        lst << "[Mask-BeamStop-Right-X] 71";
        lst << "[Mask-BeamStop-Right-Y] 71";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] Yes";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Monitor-3 [dead-time -]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 0";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 128";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] No";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] No";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.75";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS1-2020")
    {
        spinBoxWLS->setValue(20);
        
        lst << "[Instrument] KWS1-2020";
        lst << "[DataFormat] 0";
        lst << "[Color] #ff0000";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] No";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 3";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] No";
        lst << "[2nd-Header-Pattern] ";
        lst << "[2nd-Header-Lines] 0";
        lst << "[Pattern] *_#_*.DAT";
        lst << "[Pattern-Select-Data] *_*_*.DAT";
        lst << "[Header-Number-Lines] 100";
        lst << "[Data-Header-Lines] 1";
        lst << "[Lines-Between-Frames] 1";
        lst << "[Flexible-Header] Yes";
        lst << "[Flexible-Stop] $|(* Detector Data for";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] 3;;;s_1";
        lst << "[User-Name] 5;;;s 6";
        lst << "[Sample-Run-Number] 11;;;1";
        lst << "[Sample-Title] 16;;;s|1";
        lst << "[Sample-Thickness] 34;;;3";
        lst << "[Sample-Position-Number] 34;;;1";
        lst << "[Date] 5;;;s 8";
        lst << "[Time] 5;;;s 9";
        lst << "[C] 21;;;1";
        lst << "[D] 28;;;2";
        lst << "[D-TOF] 28;;;2";
        lst << "[C,D-Offset] 28;;;1";
        lst << "[CA-X] 21;;;3";
        lst << "[CA-Y] 21;;;4";
        lst << "[SA-X] 34;;;4";
        lst << "[SA-Y] 34;;;5";
        lst << "[Sum] 62;;;1";
        lst << "[Selector] 49;;;1";
        lst << "[Lambda] {lambda=};;;sA1";
        lst << "[Delta-Lambda] const;;;0.1";
        lst << "[Duration] 58;;;1";
        lst << "[Duration-Factor] 34;;;6";
        lst << "[Monitor-1] 49;;;2";
        lst << "[Monitor-2] 49;;;3";
        lst << "[Monitor-3|Tr|ROI] 49;;;4";
        lst << "[Comment1] 14;;;s!1";
        lst << "[Comment2] 15;;;s!1";
        lst << "[Detector-X || Beamcenter-X] 28;;;3";
        lst << "[Detector-Y || Beamcenter-Y] 28;;;4";
        lst << "[Sample-Motor-1] 34;;;2";
        lst << "[Sample-Motor-2] 35;;;2";
        lst << "[Sample-Motor-3] 0;;;0";
        lst << "[Sample-Motor-4] 0;;;0";
        lst << "[Sample-Motor-5] 0;;;0";
        lst << "[SA-Pos-X] 35;;;4";
        lst << "[SA-Pos-Y] 35;;;5";
        lst << "[Field-1] 38;;;s 4";
        lst << "[Field-2] 39;;;s 4";
        lst << "[Field-3] 40;;;s 4";
        lst << "[Field-4] 41;;;s 4";
        lst << "[RT-Number-Repetitions] {RT-Number-Repetitions=};;;s 1";
        lst << "[RT-Time-Factor] {RT-Time-Factor=};;;s 1";
        lst << "[RT-Current-Number] {RT-Current-Number=};;;s 1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] 21;;;s 5";
        lst << "[Lenses] 21;;;s 6";
        lst << "[Slices-Count] {(* Detector Time Slices};;;s 2";
        lst << "[Slices-Duration] {(* Detector Time Slices}+;;;2";
        lst << "[Slices-Current-Number] {(* TOF-SPLITTED:};;;s 4";
        lst << "[Slices-Current-Duration] {Slices-Current-Duration=};;;s 1";
        lst << "[Slices-Current-Monitor1] {Slices-Current-Monitor1=};;;s 1";
        lst << "[Slices-Current-Monitor2] {Slices-Current-Monitor2=};;;s 1";
        lst << "[Slices-Current-Monitor3] {Slices-Current-Monitor3=};;;s 1";
        lst << "[Slices-Current-Sum] {Slices-Current-Sum=};;;s 1";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 2196.7";
        lst << "[Selector-P2] 0.00";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 144";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.836"; // September 2018: 0.832x0.840=0.836x0.836  ... old 0.8x0.8cm2
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 8";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] No";
        lst << "[Detector-Dead-Time] 70e-9";
        lst << "[Detector-Dead-Time-DB] 70e-9";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Flat Scatter [FS]";
        lst << "[Calibrant] Plexi-1.5mm[KWS-1&2]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] Yes";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Rectangle :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 1";
        lst << "[Mask-Edge-Left-Y] 14";
        lst << "[Mask-Edge-Right-X] 144";
        lst << "[Mask-Edge-Right-Y] 131";
        lst << "[Mask-BeamStop-Left-X] 68";
        lst << "[Mask-BeamStop-Left-Y] 80";
        lst << "[Mask-BeamStop-Right-X] 78";
        lst << "[Mask-BeamStop-Right-Y] 90";
        lst << "[Mask-Dead-Rows] "
               "1-51-1-24;1-51-121-144;94-144-1-24;94-144-121-144;1-25-1-48;1-25-97-144;120-144-1-48;120-144-97-144;44-"
               "49-120-121;44-49-24-25;";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 1000";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] Yes";
        lst << "[Sensitivity-Masked-Pixels-Value] 1.000";
        lst << "[Transmission-Method] <9.5A: ROI in Header; >=9.5A: Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 1";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 0";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] No";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 144";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] No";
        lst << "[Beam-Center-as-Condition] No";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] No";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.84";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS1-He3")
    {
        spinBoxWLS->setValue(20);
        
        lst << "[Instrument] KWS1-He3";
        lst << "[DataFormat] 0";
        lst << "[Color] #ff0000";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] No";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 3";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] No";
        lst << "[2nd-Header-Pattern] ";
        lst << "[2nd-Header-Lines] 0";
        lst << "[Pattern] *_#_*.DAT";
        lst << "[Pattern-Select-Data] *_*_*.DAT";
        lst << "[Header-Number-Lines] 100";
        lst << "[Data-Header-Lines] 1";
        lst << "[Lines-Between-Frames] 1";
        lst << "[Flexible-Header] Yes";
        lst << "[Flexible-Stop] $|(* Detector Data for";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] 3;;;s_1";
        lst << "[User-Name] 5;;;s 6";
        lst << "[Sample-Run-Number] 11;;;1";
        lst << "[Sample-Title] 16;;;s|1";
        lst << "[Sample-Thickness] 34;;;3";
        lst << "[Sample-Position-Number] 34;;;1";
        lst << "[Date] 5;;;s 8";
        lst << "[Time] 5;;;s 9";
        lst << "[C] 21;;;1";
        lst << "[D] 28;;;2";
        lst << "[D-TOF] 28;;;2";
        lst << "[C,D-Offset] 28;;;1";
        lst << "[CA-X] 21;;;3";
        lst << "[CA-Y] 21;;;4";
        lst << "[SA-X] 34;;;4";
        lst << "[SA-Y] 34;;;5";
        lst << "[Sum] 62;;;1";
        lst << "[Selector] 49;;;1";
        lst << "[Lambda] {lambda=};;;sA1";
        lst << "[Delta-Lambda] const;;;0.1";
        lst << "[Duration] 58;;;1";
        lst << "[Duration-Factor] 34;;;6";
        lst << "[Monitor-1] 49;;;2";
        lst << "[Monitor-2] 49;;;3";
        lst << "[Monitor-3|Tr|ROI] 49;;;4";
        lst << "[Comment1] 14;;;s!1";
        lst << "[Comment2] 15;;;s!1";
        lst << "[Detector-X || Beamcenter-X] 28;;;3";
        lst << "[Detector-Y || Beamcenter-Y] 28;;;4";
        lst << "[Sample-Motor-1] 34;;;2";
        lst << "[Sample-Motor-2] 35;;;2";
        lst << "[Sample-Motor-3] 0;;;0";
        lst << "[Sample-Motor-4] 0;;;0";
        lst << "[Sample-Motor-5] 0;;;0";
        lst << "[SA-Pos-X] 35;;;4";
        lst << "[SA-Pos-Y] 35;;;5";
        lst << "[Field-1] 38;;;s 4";
        lst << "[Field-2] 39;;;s 4";
        lst << "[Field-3] 40;;;s 4";
        lst << "[Field-4] 41;;;s 4";
        lst << "[RT-Number-Repetitions] {RT-Number-Repetitions=};;;s 1";
        lst << "[RT-Time-Factor] {RT-Time-Factor=};;;s 1";
        lst << "[RT-Current-Number] {RT-Current-Number=};;;s 1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] 21;;;s 5";
        lst << "[Lenses] 21;;;s 6";
        lst << "[Slices-Count] {(* Detector Time Slices};;;s 2";
        lst << "[Slices-Duration] {(* Detector Time Slices}+;;;2";
        lst << "[Slices-Current-Number] {(* TOF-SPLITTED:};;;s 4";
        lst << "[Slices-Current-Duration] {Slices-Current-Duration=};;;s 1";
        lst << "[Slices-Current-Monitor1] {Slices-Current-Monitor1=};;;s 1";
        lst << "[Slices-Current-Monitor2] {Slices-Current-Monitor2=};;;s 1";
        lst << "[Slices-Current-Monitor3] {Slices-Current-Monitor3=};;;s 1";
        lst << "[Slices-Current-Sum] {Slices-Current-Sum=};;;s 1";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 2196.7";
        lst << "[Selector-P2] 0.00";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 144";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.836"; // September 2018: 0.832x0.840=0.836x0.836  ... old 0.8x0.8cm2
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 8";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] No";
        lst << "[Detector-Dead-Time] 70e-9";
        lst << "[Detector-Dead-Time-DB] 70e-9";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Flat Scatter [FS]";
        lst << "[Calibrant] Plexi-1.5mm[KWS-1&2]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] Yes";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Rectangle :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 1";
        lst << "[Mask-Edge-Left-Y] 14";
        lst << "[Mask-Edge-Right-X] 144";
        lst << "[Mask-Edge-Right-Y] 131";
        lst << "[Mask-BeamStop-Left-X] 68";
        lst << "[Mask-BeamStop-Left-Y] 80";
        lst << "[Mask-BeamStop-Right-X] 78";
        lst << "[Mask-BeamStop-Right-Y] 90";
        lst << "[Mask-Dead-Rows] "
               "1-51-1-24;1-51-121-144;94-144-1-24;94-144-121-144;1-25-1-48;1-25-97-144;120-144-1-48;120-144-97-144;44-"
               "49-120-121;44-49-24-25;";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] Yes";
        lst << "[Sensitivity-Masked-Pixels-Value] 1.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 1";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 0";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] No";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 144";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] No";
        lst << "[Beam-Center-as-Condition] No";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 100";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] No";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.84";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS2")
    {
        spinBoxWLS->setValue(20);
        
        lst << "[Instrument] KWS2";
        lst<< "[Color] #0000ff";
        lst<< "[Input-Folder] home";
        lst<< "[Output-Folder] home";
        lst << "[DataFormat] 0";
        lst << "[[Include-Sub-Foldes] No";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 3";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] No";
        lst << "[2nd-Header-Pattern] ";
        lst << "[2nd-Header-Lines] 0";
        lst << "[Pattern] *_#_*.DAT";
        lst << "[Pattern-Select-Data] *_*_*.DAT";
        lst << "[Header-Number-Lines] 100";
        lst << "[Data-Header-Lines] 1";
        lst << "[Lines-Between-Frames] 1";
        lst << "[Flexible-Header] Yes";
        lst << "[Flexible-Stop] $|(* Detector Data for";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] 3;;;s_1";
        lst << "[User-Name] 5;;;s 6";
        lst << "[Sample-Run-Number] 11;;;1";
        lst << "[Sample-Title] 16;;;s!1";
        lst << "[Sample-Thickness] 34;;;3";
        lst << "[Sample-Position-Number] 34;;;1";
        lst << "[Date] 5;;;s 8";
        lst << "[Time] 5;;;s 9";
        lst << "[C] 21;;;1";
        lst << "[D] 28;;;2";
        lst << "[D-TOF] 28;;;2";
        lst << "[C,D-Offset] 28;;;1";
        lst << "[CA-X] 21;;;3";
        lst << "[CA-Y] 21;;;4";
        lst << "[SA-X] 34;;;4";
        lst << "[SA-Y] 34;;;5";
        lst << "[Sum] 62;;;1";
        lst << "[Selector] 49;;;1";
        lst << "[Lambda] {Lambda=} ;;;1";
        lst << "[Delta-Lambda] const;;;0.2";
        lst << "[Duration] 58;;;1";
        lst << "[Duration-Factor] 34;;;6";
        lst << "[Monitor-1] 49;;;2";
        lst << "[Monitor-2] 49;;;3";
        lst << "[Monitor-3|Tr|ROI] 49;;;4";
        lst << "[Comment1] 14;;;s!1";
        lst << "[Comment2] 15;;;s!1";
        lst << "[Detector-X || Beamcenter-X] 28;;;3";
        lst << "[Detector-Y || Beamcenter-Y] 28;;;4";
        lst << "[Sample-Motor-1] 34;;;2";
        lst << "[Sample-Motor-2] 35;;;2";
        lst << "[Sample-Motor-3] 0;;;0";
        lst << "[Sample-Motor-4] 0;;;0";
        lst << "[Sample-Motor-5] 0;;;0";
        lst << "[SA-Pos-X] 35;;;4";
        lst << "[SA-Pos-Y] 35;;;5";
        lst << "[Field-1] 38;;;s 4";
        lst << "[Field-2] 39;;;s 4";
        lst << "[Field-3] 40;;;s 4";
        lst << "[Field-4] 41;;;s 4";
        lst << "[RT-Number-Repetitions] 45;;;s 5";
        lst << "[RT-Time-Factor] {RT-Time-Factor=};;;s 1";
        lst << "[RT-Current-Number] {RT-Current-Number=};;;s 1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] const;;;out";
        lst << "[Lenses] const;;;out-out-out";
        lst << "[Slices-Count] {(* Detector Time Slices};;;s 2";
        lst << "[Slices-Duration] {(* Detector Time Slices}+;;;2";
        lst << "[Slices-Current-Number] {(* TOF-SPLITTED:};;;s 4";
        lst << "[Slices-Current-Duration] {Slices-Current-Duration=};;;s 1";
        lst << "[Slices-Current-Monitor1] {Slices-Current-Monitor1=};;;s 1";
        lst << "[Slices-Current-Monitor2] {Slices-Current-Monitor2=};;;s 1";
        lst << "[Slices-Current-Monitor3] {Slices-Current-Monitor3=};;;s 1";
        lst << "[Slices-Current-Sum] {Slices-Current-Sum=};;;s 1";
        lst << "[Selector-Read-from-Header] No";
        lst << "[Selector-P1] 1992.97";
        lst << "[Selector-P2] 0.56993";
        lst << "[Detector-Data-Dimension] 1";
        lst << "[Detector-Data-Focus] 128";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.525";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 8";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 128";
        lst << "[Detector-Data-Transpose] No";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] No";
        lst << "[Detector-Dead-Time] 6.5e-7";
        lst << "[Detector-Dead-Time-DB] 2.6e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] SYM";
        lst << "[Calibrant-Type] Flat Scatter [FS]";
        lst << "[Calibrant] Plexi-1.5mm[KWS-1&2]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] Yes";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Rectangle :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 8";
        lst << "[Mask-Edge-Left-Y] 8";
        lst << "[Mask-Edge-Right-X] 121";
        lst << "[Mask-Edge-Right-Y] 121";
        lst << "[Mask-BeamStop-Left-X] 58";
        lst << "[Mask-BeamStop-Left-Y] 58";
        lst << "[Mask-BeamStop-Right-X] 71";
        lst << "[Mask-BeamStop-Right-Y] 71";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] Yes";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Monitor-3 [dead-time -]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 0";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 128";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] No";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Beam-Center-as-Condition] No";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] No";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] No";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.75";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS2-He3-20%")
    {
        spinBoxWLS->setValue(20);
        
        lst << "[Instrument] KWS2-He3-20%";
        lst << "[DataFormat] 0";
        lst << "[Color] #0000ff";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[[Include-Sub-Foldes] No";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 3";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] No";
        lst << "[2nd-Header-Pattern] ";
        lst << "[2nd-Header-Lines] 0";
        lst << "[Pattern] *_#_*.DAT";
        lst << "[Pattern-Select-Data] *_*_*.DAT";
        lst << "[Header-Number-Lines] 100";
        lst << "[Data-Header-Lines] 1";
        lst << "[Lines-Between-Frames] 1";
        lst << "[Flexible-Header] Yes";
        lst << "[Flexible-Stop] $|(* Detector Data for";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] 3;;;s_1";
        lst << "[User-Name] 5;;;s 6";
        lst << "[Sample-Run-Number] 11;;;1";
        lst << "[Sample-Title] 16;;;s|1";
        lst << "[Sample-Thickness] 34;;;3";
        lst << "[Sample-Position-Number] 34;;;1";
        lst << "[Date] 5;;;s 8";
        lst << "[Time] 5;;;s 9";
        lst << "[C] 21;;;1";
        lst << "[D] 28;;;2";
        lst << "[D-TOF] 28;;;2";
        lst << "[C,D-Offset] 28;;;1";
        lst << "[CA-X] 21;;;3";
        lst << "[CA-Y] 21;;;4";
        lst << "[SA-X] 34;;;4";
        lst << "[SA-Y] 34;;;5";
        lst << "[Sum] 62;;;1";
        lst << "[Selector] 49;;;1";
        lst << "[Lambda] {lambda=};;;sA1";
        lst << "[Delta-Lambda] const;;;0.2";
        lst << "[Duration] 58;;;1";
        lst << "[Duration-Factor] 34;;;6";
        lst << "[Monitor-1] 49;;;2";
        lst << "[Monitor-2] 49;;;3";
        lst << "[Monitor-3|Tr|ROI] 49;;;4";
        lst << "[Comment1] 14;;;s!1";
        lst << "[Comment2] 15;;;s!1";
        lst << "[Detector-X || Beamcenter-X] 28;;;3";
        lst << "[Detector-Y || Beamcenter-Y] 28;;;4";
        lst << "[Sample-Motor-1] 34;;;2";
        lst << "[Sample-Motor-2] 35;;;2";
        lst << "[Sample-Motor-3] 0;;;0";
        lst << "[Sample-Motor-4] 0;;;0";
        lst << "[Sample-Motor-5] 0;;;0";
        lst << "[SA-Pos-X] 35;;;4";
        lst << "[SA-Pos-Y] 35;;;5";
        lst << "[Field-1] 38;;;s 4";
        lst << "[Field-2] 39;;;s 4";
        lst << "[Field-3] 40;;;s 4";
        lst << "[Field-4] 41;;;s 4";
        lst << "[RT-Number-Repetitions] {RT-Number-Repetitions=};;;s 1";
        lst << "[RT-Time-Factor] {RT-Time-Factor=};;;s 1";
        lst << "[RT-Current-Number] {RT-Current-Number=};;;s 1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] 21;;;s 5";
        lst << "[Lenses] 21;;;s 6";
        lst << "[Slices-Count] {(* Detector Time Slices};;;s 2";
        lst << "[Slices-Duration] {(* Detector Time Slices}+;;;2";
        lst << "[Slices-Current-Number] {(* TOF-SPLITTED:};;;s 4";
        lst << "[Slices-Current-Duration] {Slices-Current-Duration=};;;s 1";
        lst << "[Slices-Current-Monitor1] {Slices-Current-Monitor1=};;;s 1";
        lst << "[Slices-Current-Monitor2] {Slices-Current-Monitor2=};;;s 1";
        lst << "[Slices-Current-Monitor3] {Slices-Current-Monitor3=};;;s 1";
        lst << "[Slices-Current-Sum] {Slices-Current-Sum=};;;s 1";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 1027.589";
        lst << "[Selector-P2] 0.579";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 144";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.836"; // September 2018: 0.832x0.840=0.836x0.836  ... old 0.8x0.8cm2
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 8";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] No";
        lst << "[Detector-Dead-Time] 70e-9";
        lst << "[Detector-Dead-Time-DB] 70e-9";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Flat Scatter [FS]";
        lst << "[Calibrant] Plexi-1.5mm[KWS-1&2]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] Yes";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Rectangle :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 1";
        lst << "[Mask-Edge-Left-Y] 16";
        lst << "[Mask-Edge-Right-X] 144";
        lst << "[Mask-Edge-Right-Y] 130";
        lst << "[Mask-BeamStop-Left-X] 67";
        lst << "[Mask-BeamStop-Left-Y] 68";
        lst << "[Mask-BeamStop-Right-X] 77";
        lst << "[Mask-BeamStop-Right-Y] 79";
        lst << "[Mask-Dead-Rows]  "
               "1-52-1-24;1-52-121-144;93-144-1-24;93-144-121-144;1-26-1-48;1-26-97-144;119-144-1-48;119-144-97-144;43-"
               "49-23-25;43-49-120-123;";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] Yes";
        lst << "[Sensitivity-Masked-Pixels-Value] 1.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 1";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 0";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] No";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 144";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] No";
        lst << "[Beam-Center-as-Condition] No";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] No";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] No";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.84";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS2-He3-10%")
    {
        spinBoxWLS->setValue(20);
        
        lst << "[Instrument] KWS2-He3-10%";
        lst << "[DataFormat] 0";
        lst << "[Color] #0000ff";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[[Include-Sub-Foldes] No";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 3";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] No";
        lst << "[2nd-Header-Pattern] ";
        lst << "[2nd-Header-Lines] 0";
        lst << "[Pattern] *_#_*.DAT";
        lst << "[Pattern-Select-Data] *_*_*.DAT";
        lst << "[Header-Number-Lines] 100";
        lst << "[Data-Header-Lines] 1";
        lst << "[Lines-Between-Frames] 1";
        lst << "[Flexible-Header] Yes";
        lst << "[Flexible-Stop] $|(* Detector Data for";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] 3;;;s_1";
        lst << "[User-Name] 5;;;s 6";
        lst << "[Sample-Run-Number] 11;;;1";
        lst << "[Sample-Title] 16;;;s|1";
        lst << "[Sample-Thickness] 34;;;3";
        lst << "[Sample-Position-Number] 34;;;1";
        lst << "[Date] 5;;;s 8";
        lst << "[Time] 5;;;s 9";
        lst << "[C] 21;;;1";
        lst << "[D] 28;;;2";
        lst << "[D-TOF] 28;;;2";
        lst << "[C,D-Offset] 28;;;1";
        lst << "[CA-X] 21;;;3";
        lst << "[CA-Y] 21;;;4";
        lst << "[SA-X] 34;;;4";
        lst << "[SA-Y] 34;;;5";
        lst << "[Sum] 62;;;1";
        lst << "[Selector] 49;;;1";
        lst << "[Lambda] {lambda=};;;sA1";
        lst << "[Delta-Lambda] const;;;0.1";
        lst << "[Duration] 58;;;1";
        lst << "[Duration-Factor] 34;;;6";
        lst << "[Monitor-1] 49;;;2";
        lst << "[Monitor-2] 49;;;3";
        lst << "[Monitor-3|Tr|ROI] 49;;;4";
        lst << "[Comment1] 14;;;s!1";
        lst << "[Comment2] 15;;;s!1";
        lst << "[Detector-X || Beamcenter-X] 28;;;3";
        lst << "[Detector-Y || Beamcenter-Y] 28;;;4";
        lst << "[Sample-Motor-1] 34;;;2";
        lst << "[Sample-Motor-2] 35;;;2";
        lst << "[Sample-Motor-3] 0;;;0";
        lst << "[Sample-Motor-4] 0;;;0";
        lst << "[Sample-Motor-5] 0;;;0";
        lst << "[SA-Pos-X] 35;;;4";
        lst << "[SA-Pos-Y] 35;;;5";
        lst << "[Field-1] 38;;;s 4";
        lst << "[Field-2] 39;;;s 4";
        lst << "[Field-3] 40;;;s 4";
        lst << "[Field-4] 41;;;s 4";
        lst << "[RT-Number-Repetitions] {RT-Number-Repetitions=};;;s 1";
        lst << "[RT-Time-Factor] {RT-Time-Factor=};;;s 1";
        lst << "[RT-Current-Number] {RT-Current-Number=};;;s 1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] 21;;;s 5";
        lst << "[Lenses] 21;;;s 6";
        lst << "[Slices-Count] {(* Detector Time Slices};;;s 2";
        lst << "[Slices-Duration] {(* Detector Time Slices}+;;;2";
        lst << "[Slices-Current-Number] {(* TOF-SPLITTED:};;;s 4";
        lst << "[Slices-Current-Duration] {Slices-Current-Duration=};;;s 1";
        lst << "[Slices-Current-Monitor1] {Slices-Current-Monitor1=};;;s 1";
        lst << "[Slices-Current-Monitor2] {Slices-Current-Monitor2=};;;s 1";
        lst << "[Slices-Current-Monitor3] {Slices-Current-Monitor3=};;;s 1";
        lst << "[Slices-Current-Sum] {Slices-Current-Sum=};;;s 1";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 2094.328";
        lst << "[Selector-P2] 0.0583";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 144";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.836"; // September 2018: 0.832x0.840=0.836x0.836  ... old 0.8x0.8cm2
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 8";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] No";
        lst << "[Detector-Dead-Time] 70e-9";
        lst << "[Detector-Dead-Time-DB] 70e-9";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Flat Scatter [FS]";
        lst << "[Calibrant] Plexi-1.5mm[KWS-1&2]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] Yes";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Rectangle :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 1";
        lst << "[Mask-Edge-Left-Y] 16";
        lst << "[Mask-Edge-Right-X] 144";
        lst << "[Mask-Edge-Right-Y] 130";
        lst << "[Mask-BeamStop-Left-X] 67";
        lst << "[Mask-BeamStop-Left-Y] 68";
        lst << "[Mask-BeamStop-Right-X] 77";
        lst << "[Mask-BeamStop-Right-Y] 79";
        lst << "[Mask-Dead-Rows]  "
               "1-52-1-24;1-52-121-144;93-144-1-24;93-144-121-144;1-26-1-48;1-26-97-144;119-144-1-48;119-144-97-144;43-"
               "49-23-25;43-49-120-123;";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] Yes";
        lst << "[Sensitivity-Masked-Pixels-Value] 1.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 1";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 0";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] No";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 144";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] No";
        lst << "[Beam-Center-as-Condition] No";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] No";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] No";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.84";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS2-HRD")
    {
        spinBoxWLS->setValue(20);
        
        lst << "[Instrument] KWS2-HRD";
        lst << "[DataFormat] 0";
        lst << "[Color] #0000ff";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] No";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 3";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] No";
        lst << "[2nd-Header-Pattern] ";
        lst << "[2nd-Header-Lines] 0";
        lst << "[Pattern] *_#_*.DAT";
        lst << "[Pattern-Select-Data] *_*_*.DAT";
        lst << "[Header-Number-Lines] 100";
        lst << "[Data-Header-Lines] 1";
        lst << "[Lines-Between-Frames] 1";
        lst << "[Flexible-Header] Yes";
        lst << "[Flexible-Stop] $|(* Detector Data for";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] 3;;;s_1";
        lst << "[User-Name] 5;;;s 6";
        lst << "[Sample-Run-Number] 11;;;1";
        lst << "[Sample-Title] 16;;;s|1";
        lst << "[Sample-Thickness] 34;;;3";
        lst << "[Sample-Position-Number] 34;;;1";
        lst << "[Date] 5;;;s 8";
        lst << "[Time] 5;;;s 9";
        lst << "[C] 21;;;1";
        lst << "[D] 28;;;2";
        lst << "[D-TOF] 28;;;2";
        lst << "[C,D-Offset] 28;;;1";
        lst << "[CA-X] 21;;;3";
        lst << "[CA-Y] 21;;;4";
        lst << "[SA-X] 34;;;4";
        lst << "[SA-Y] 34;;;5";
        lst << "[Sum] 62;;;1";
        lst << "[Selector] 49;;;1";
        lst << "[Lambda] {lambda=};;;sA1";
        lst << "[Delta-Lambda] const;;;0.2";
        lst << "[Duration] 58;;;1";
        lst << "[Duration-Factor] 34;;;6";
        lst << "[Monitor-1] 49;;;2";
        lst << "[Monitor-2] 49;;;3";
        lst << "[Monitor-3|Tr|ROI] 49;;;4";
        lst << "[Comment1] 14;;;s!1";
        lst << "[Comment2] 15;;;s!1";
        lst << "[Detector-X || Beamcenter-X] 28;;;3";
        lst << "[Detector-Y || Beamcenter-Y] 28;;;4";
        lst << "[Sample-Motor-1] 34;;;2";
        lst << "[Sample-Motor-2] 35;;;2";
        lst << "[Sample-Motor-3] 0;;;0";
        lst << "[Sample-Motor-4] 0;;;0";
        lst << "[Sample-Motor-5] 0;;;0";
        lst << "[SA-Pos-X] 35;;;4";
        lst << "[SA-Pos-Y] 35;;;5";
        lst << "[Field-1] 38;;;s 4";
        lst << "[Field-2] 39;;;s 4";
        lst << "[Field-3] 40;;;s 4";
        lst << "[Field-4] 41;;;s 4";
        lst << "[RT-Number-Repetitions] {RT-Number-Repetitions=};;;s 1";
        lst << "[RT-Time-Factor] {RT-Time-Factor=};;;s 1";
        lst << "[RT-Current-Number] {RT-Current-Number=};;;s 1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] 21;;;s 5";
        lst << "[Lenses] 21;;;s 6";
        lst << "[Slices-Count] {(* Detector Time Slices};;;s 2";
        lst << "[Slices-Duration] {(* Detector Time Slices}+;;;2";
        lst << "[Slices-Current-Number] {(* TOF-SPLITTED:};;;s 4";
        lst << "[Slices-Current-Duration] {Slices-Current-Duration=};;;s 1";
        lst << "[Slices-Current-Monitor1] {Slices-Current-Monitor1=};;;s 1";
        lst << "[Slices-Current-Monitor2] {Slices-Current-Monitor2=};;;s 1";
        lst << "[Slices-Current-Monitor3] {Slices-Current-Monitor3=};;;s 1";
        lst << "[Slices-Current-Sum] {Slices-Current-Sum=};;;s 1";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 2161.87";
        lst << "[Selector-P2] 0.05231";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 180";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.039";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 8";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Transpose] No";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] No";
        lst << "[Detector-Dead-Time] 6.5e-7";
        lst << "[Detector-Dead-Time-DB] 6.5e-7";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam [DB]";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] Yes";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Ellipse :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 1";
        lst << "[Mask-Edge-Left-Y] 8";
        lst << "[Mask-Edge-Right-X] 180";
        lst << "[Mask-Edge-Right-Y] 180";
        lst << "[Mask-BeamStop-Left-X] 83";
        lst << "[Mask-BeamStop-Left-Y] 87";
        lst << "[Mask-BeamStop-Right-X] 97";
        lst << "[Mask-BeamStop-Right-Y] 101";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] Yes";
        lst << "[Sensitivity-Masked-Pixels-Value] 1.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 0";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] No";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 180";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] No";
        lst << "[Beam-Center-as-Condition] No";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] No";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.039";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS3-2021")
    {
        lst << "[Instrument] KWS3-2021";
        lst << "[DataFormat] 2";
        lst << "[Color] #8989b7";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] Yes";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 1";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] #_*_DHRD_*.KWS3.yaml";
        lst << "[2nd-Header-Lines] 2000";
        lst << "[Pattern] #_*_DHRD_*.KWS3.array.gz";
        lst << "[Pattern-Select-Data] *_*_DHRD_*.KWS3.yaml";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] No";
        lst << "[Flexible-Stop] ";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] Yes";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] experiment:title;;;";
        lst << "[User-Name] experiment:authors:name;;;s<1";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] measurement:sample:description:name;;;";
        lst << "[Sample-Thickness] measurement:sample:thickness;;;";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] measurement:history:started;;;sT1";
        lst << "[Time] measurement:history:started;;;sT2";
        lst << "[C] const;;;10.0";
        lst << "[D] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[D-TOF] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[C,D-Offset] measurement:sample:detoffset;;;";
        lst << "[CA-X] measurement:devices:name|sel_ap2|value;;;s;3";
        lst << "[CA-Y] measurement:devices:name|sel_ap2|value;;;s;4";
        lst << "[SA-X] measurement:devices:name|sam_ap|value;;;s;3";
        lst << "[SA-Y] measurement:devices:name|sam_ap|value;;;s;4";
        lst << "[Sum] measurement:detectors:type|position_sensitive_detector|results|total;;;s;1";
        lst << "[Selector] measurement:devices:name|sel_speed|value;;;s;1";
        lst << "[Lambda] measurement:devices:name|sel_lambda|value;;;s;1";
        lst << "[Delta-Lambda] const;;;0.15";
        lst << "[Duration] measurement:detectors:type|position_sensitive_detector|results|timer;;;s;1";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] measurement:detectors:type|position_sensitive_detector|results|mon1;;;s;1";
        lst << "[Monitor-2] measurement:detectors:type|position_sensitive_detector|results|mon2;;;s;1";
        lst << "[Monitor-3|Tr|ROI] measurement:detectors:type|position_sensitive_detector|results|roi.in;;;s;1";
        lst << "[Comment1] experiment:number;;;";
        lst << "[Comment2] experiment:title;;;";
        lst << "[Detector-X || Beamcenter-X] measurement:devices:name|det_beamstop_x|value;;;s;1";
        lst << "[Detector-Y || Beamcenter-Y] measurement:devices:name|det_y|value;;;s;1";
        lst << "[Sample-Motor-1] measurement:devices:name|sam_x|value;;;s;1";
        lst << "[Sample-Motor-2] measurement:devices:name|sam_y|value;;;s;1";
        lst << "[Sample-Motor-3] measurement:devices:name|sam_rot|value;;;s;1";
        lst << "[Sample-Motor-4] measurement:devices:name|sam_phi|value;;;s;1";
        lst << "[Sample-Motor-5] measurement:devices:name|sam_chi|value;;;s;1";
        lst << "[SA-Pos-X] measurement:devices:name|sam_ap|value;;;s;1";
        lst << "[SA-Pos-Y] measurement:devices:name|sam_ap|value;;;s;2";
        lst << "[Field-1] measurement:devices:name|T|value;;;s;1";
        lst << "[Field-2] measurement:devices:name|T_julabo_10m_extern|value;;;s;1";
        lst << "[Field-3] measurement:devices:name|T_julabo_1m_extern|value;;;s;1";
        lst << "[Field-4] measurement:devices:name|P_hpc1|value;;;s;1";
        lst << "[RT-Number-Repetitions] const;;;1.0";
        lst << "[RT-Time-Factor] const;;;1.0";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] measurement:devices:name|polarizer|value;;;";
        lst << "[Lenses] const;;;out";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 3133.4";
        lst << "[Selector-P2] -0.00195";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 256";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.034";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] Yes";
        lst << "[Detector-Dead-Time] 2.7e-6";
        lst << "[Detector-Dead-Time-DB] 8.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam [DB]";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] No";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Ellipse :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Ellipse :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 2";
        lst << "[Mask-Edge-Left-Y] 3";
        lst << "[Mask-Edge-Right-X] 254";
        lst << "[Mask-Edge-Right-Y] 255";
        lst << "[Mask-BeamStop-Left-X] 110";
        lst << "[Mask-BeamStop-Left-Y] 119";
        lst << "[Mask-BeamStop-Right-X] 131";
        lst << "[Mask-BeamStop-Right-Y] 142";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 1";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-2D-Header-SASVIEW] No";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 5";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 256";
        lst << "[Options-1D-QxQy-BS] No";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] No";
        lst << "[Beam-Center-as-Condition] No";
        lst << "[Polarization-as-Condition] No";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.08";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS3-VHRD-2021")
    {
        lst << "[Instrument] KWS3-VHRD-2021";
        lst << "[DataFormat] 2";
        lst << "[Color] #8989b7";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] Yes";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 1";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] #_*_DVHRD_*.KWS3.yaml";
        lst << "[2nd-Header-Lines] 2000";
        lst << "[Pattern] #_*_DVHRD_*.KWS3.array.gz";
        lst << "[Pattern-Select-Data] *_*_DVHRD_*.KWS3.yaml";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] No";
        lst << "[Flexible-Stop] ";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] Yes";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] experiment:title;;;";
        lst << "[User-Name] experiment:authors:name;;;s<1";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] measurement:sample:description:name;;;";
        lst << "[Sample-Thickness] measurement:sample:thickness;;;";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] measurement:history:started;;;sT1";
        lst << "[Time] measurement:history:started;;;sT2";
        lst << "[C] const;;;10.0";
        lst << "[D] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[D-TOF] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[C,D-Offset] measurement:sample:detoffset;;;";
        lst << "[CA-X] measurement:devices:name|sel_ap2|value;;;s;3";
        lst << "[CA-Y] measurement:devices:name|sel_ap2|value;;;s;4";
        lst << "[SA-X] measurement:devices:name|sam_ap|value;;;s;3";
        lst << "[SA-Y] measurement:devices:name|sam_ap|value;;;s;4";
        lst << "[Sum] measurement:detectors:type|position_sensitive_detector|results|total;;;s;1";
        lst << "[Selector] measurement:devices:name|sel_speed|value;;;s;1";
        lst << "[Lambda] measurement:devices:name|sel_lambda|value;;;s;1";
        lst << "[Delta-Lambda] const;;;0.15";
        lst << "[Duration] measurement:detectors:type|position_sensitive_detector|results|timer;;;s;1";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] measurement:detectors:type|position_sensitive_detector|results|mon1;;;s;1";
        lst << "[Monitor-2] measurement:detectors:type|position_sensitive_detector|results|mon2;;;s;1";
        lst << "[Monitor-3|Tr|ROI] measurement:detectors:type|position_sensitive_detector|results|roi.in;;;s;1";
        lst << "[Comment1] experiment:number;;;";
        lst << "[Comment2] experiment:title;;;";
        lst << "[Detector-X || Beamcenter-X] measurement:devices:name|det_beamstop_x|value;;;s;1";
        lst << "[Detector-Y || Beamcenter-Y] measurement:devices:name|det_y|value;;;s;1";
        lst << "[Sample-Motor-1] measurement:devices:name|sam_x|value;;;s;1";
        lst << "[Sample-Motor-2] measurement:devices:name|sam_y|value;;;s;1";
        lst << "[Sample-Motor-3] measurement:devices:name|sam_rot|value;;;s;1";
        lst << "[Sample-Motor-4] measurement:devices:name|sam_phi|value;;;s;1";
        lst << "[Sample-Motor-5] measurement:devices:name|sam_chi|value;;;s;1";
        lst << "[SA-Pos-X] measurement:devices:name|sam_ap|value;;;s;1";
        lst << "[SA-Pos-Y] measurement:devices:name|sam_ap|value;;;s;2";
        lst << "[Field-1] measurement:devices:name|T|value;;;s;1";
        lst << "[Field-2] measurement:devices:name|T_julabo_10m_extern|value;;;s;1";
        lst << "[Field-3] measurement:devices:name|T_julabo_1m_extern|value;;;s;1";
        lst << "[Field-4] measurement:devices:name|P_hpc1|value;;;s;1";
        lst << "[RT-Number-Repetitions] const;;;1.0";
        lst << "[RT-Time-Factor] const;;;1.0";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] measurement:devices:name|polarizer|value;;;";
        lst << "[Lenses] const;;;out";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 3133.4";
        lst << "[Selector-P2] -0.00195";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 256";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.0116";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] Yes";
        lst << "[Detector-Dead-Time] 2.7e-6";
        lst << "[Detector-Dead-Time-DB] 8.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam [DB]";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] No";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-BeamStop-Shape] Ellipse :: Shape of Edge";
        lst << "[Mask-Edge-Left-X] 2";
        lst << "[Mask-Edge-Left-Y] 3";
        lst << "[Mask-Edge-Right-X] 254";
        lst << "[Mask-Edge-Right-Y] 255";
        lst << "[Mask-BeamStop-Left-X] 121";
        lst << "[Mask-BeamStop-Left-Y] 121";
        lst << "[Mask-BeamStop-Right-X] 136";
        lst << "[Mask-BeamStop-Right-Y] 136";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 1";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-2D-Header-SASVIEW] No";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 5";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 256";
        lst << "[Options-1D-QxQy-BS] No";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] No";
        lst << "[Beam-Center-as-Condition] No";
        lst << "[Polarization-as-Condition] No";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.08";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS3")
    {
        lst << "[Instrument] KWS3";
        lst << "[DataFormat] 2";
        lst << "[Color] #8989b7";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[[Include-Sub-Foldes] Yes";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] *.dev";
        lst << "[2nd-Header-Lines] 3000";
        lst << "[Pattern] *_detector";
        lst << "[Pattern-Select-Data] *_detector";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] No";
        lst << "[Flexible-Stop] ";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] data:comment;;;";
        lst << "[User-Name] user;;;";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] data:comment;;;";
        lst << "[Sample-Thickness] SampleThickness:position;;;";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] created;;;";
        lst << "[Time] saved;;;";
        lst << "[C] const;;;10.0";
        lst << "[D] D:position;;;";
        lst << "[D-TOF] D:position;;;";
        lst << "[C,D-Offset] const;;;0";
        lst << "[CA-X] Slit_Selector_Width:position;;;";
        lst << "[CA-Y] Slit_Selector_Hight:position;;;";
        lst << "[SA-X] Slit_10m_Width:position;;;0.2";
        lst << "[SA-Y] Slit_10m_Hight:position;;;0.2";
        lst << "[Sum] detector:position;;;";
        lst << "[Selector] const;;;1";
        lst << "[Lambda] Lambda:position;;;";
        lst << "[Delta-Lambda] const;;;0.15";
        lst << "[Duration] time:position;;;";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] Monitor_1:position;;;";
        lst << "[Monitor-2] Monitor_1:position;;;";
        lst << "[Monitor-3|Tr|ROI] const;;;1";
        lst << "[Comment1] command;;;";
        lst << "[Comment2] ;;;";
        lst << "[Detector-X || Beamcenter-X] Detector_x:position;;;";
        lst << "[Detector-Y || Beamcenter-Y] Detector_y:position;;;";
        lst << "[Sample-Motor-1] Sample_10m_x:position;;;";
        lst << "[Sample-Motor-2] Sample_10m_y:position;;;";
        lst << "[Sample-Motor-3] Sample_01m_x:position;;;";
        lst << "[Sample-Motor-4] Sample_01m_y:position;;;";
        lst << "[Sample-Motor-5] ;;;";
        lst << "[SA-Pos-X] Slit_10m_X0:position;;;";
        lst << "[SA-Pos-Y] Slit_10m_Y0:position;;;";
        lst << "[Field-1] tlc40:position;;;";
        lst << "[Field-2] julabo11:position;;;";
        lst << "[Field-3] Lakeshore:position;;;";
        lst << "[Field-4] Huber:position;;;";
        lst << "[RT-Number-Repetitions] const;;;1.0";
        lst << "[RT-Time-Factor] const;;;1.0";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] const;;;out";
        lst << "[Lenses] const;;;out";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 3133.4";
        lst << "[Selector-P2] -0.00195";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 256";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.034";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] No";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] Yes";
        lst << "[Detector-Dead-Time] 2.7e-6";
        lst << "[Detector-Dead-Time-DB] 4.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] No";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Ellipse :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Ellipse :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 2";
        lst << "[Mask-Edge-Left-Y] 3";
        lst << "[Mask-Edge-Right-X] 254";
        lst << "[Mask-Edge-Right-Y] 255";
        lst << "[Mask-BeamStop-Left-X] 121";
        lst << "[Mask-BeamStop-Left-Y] 121";
        lst << "[Mask-BeamStop-Right-X] 133";
        lst << "[Mask-BeamStop-Right-Y] 133";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] No";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 2";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-1D-RADmethod-HF] No";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 256";
        lst << "[Options-1D-OutputFormat] 1";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] Yes";
        lst << "[Beam-Center-as-Condition] No";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.08";
        lst << "[File-Ext] ";
    }
    else if (instrName=="kws3-2016")
    {
        lst << "[Instrument] kws3-2016";
        lst << "[DataFormat] 2";
        lst << "[Color] #8989b7";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] Yes";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] *.dev";
        lst << "[2nd-Header-Lines] 2000";
        lst << "[Pattern] *_detector";
        lst << "[Pattern-Select-Data] *_detector";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] No";
        lst << "[Flexible-Stop] ";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] data:comment;;;";
        lst << "[User-Name] user;;;";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] data:comment;;;";
        lst << "[Sample-Thickness] SampleThickness:position;;;";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] created;;;";
        lst << "[Time] saved;;;";
        lst << "[C] const;;;10.0";
        lst << "[D] D:position;;;";
        lst << "[D-TOF] D:position;;;";
        lst << "[C,D-Offset] const;;;0";
        lst << "[CA-X] sel_ap2_width:position;;;";
        lst << "[CA-Y] sel_ap2_hight:position;;;";
        lst << "[SA-X] ActiveSlitHight:position;;;0.2";
        lst << "[SA-Y] ActiveSlitWidth:position;;;0.2";
        lst << "[Sum] detector:position;;;";
        lst << "[Selector] const;;;1";
        lst << "[Lambda] Lambda:position;;;";
        lst << "[Delta-Lambda] const;;;0.15";
        lst << "[Duration] time:position;;;";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] Monitor_1:position;;;";
        lst << "[Monitor-2] Monitor_1:position;;;";
        lst << "[Monitor-3|Tr|ROI] const;;;1";
        lst << "[Comment1] command;;;";
        lst << "[Comment2] ;;;";
        lst << "[Detector-X || Beamcenter-X] det_beamstop_x:position;;;";
        lst << "[Detector-Y || Beamcenter-Y] det_y:position;;;";
        lst << "[Sample-Motor-1] sam10_vac_x:position;;;";
        lst << "[Sample-Motor-2] sam10_vac_y:position;;;";
        lst << "[Sample-Motor-3] sam01_vac_x:position;;;";
        lst << "[Sample-Motor-4] sam01_vac_y:position;;;";
        lst << "[Sample-Motor-5] ;;;";
        lst << "[SA-Pos-X] mir_ap2_x0:position;;;";
        lst << "[SA-Pos-Y] mir_ap2_y0:position;;;";
        lst << "[Field-1] tlc40:position;;;";
        lst << "[Field-2] julabo11:position;;;";
        lst << "[Field-3] Lakeshore:position;;;";
        lst << "[Field-4] Huber:position;;;";
        lst << "[RT-Number-Repetitions] const;;;1.0";
        lst << "[RT-Time-Factor] const;;;1.0";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[P-Flipper] const;;;0";
        lst << "[A-Flipper] const;;;0";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 3133.4";
        lst << "[Selector-P2] -0.00195";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 256";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.034";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] No";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] Yes";
        lst << "[Detector-Dead-Time] 2.7e-6";
        lst << "[Detector-Dead-Time-DB] 8.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam [DB]";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] No";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Ellipse :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 2";
        lst << "[Mask-Edge-Left-Y] 3";
        lst << "[Mask-Edge-Right-X] 254";
        lst << "[Mask-Edge-Right-Y] 255";
        lst << "[Mask-BeamStop-Left-X] 125";
        lst << "[Mask-BeamStop-Left-Y] 120";
        lst << "[Mask-BeamStop-Right-X] 137";
        lst << "[Mask-BeamStop-Right-Y] 133";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 2";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 256";
        lst << "[Options-1D-OutputFormat] 1";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] Yes";
        lst << "[Beam-Center-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.08";
        lst << "[File-Ext] ";
    }
    else if (instrName=="kws3-2017-nicos")
    {
        lst << "[Instrument] kws3-2017-nicos";
        lst << "[DataFormat] 2";
        lst << "[Color] #8989b7";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] Yes";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 1";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] #_*.yaml";
        lst << "[2nd-Header-Lines] 2000";
        lst << "[Pattern] #_*.det";
        lst << "[Pattern-Select-Data] *_*_*.yaml";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] No";
        lst << "[Flexible-Stop] ";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] instrument:proposal;;;";
        lst << "[User-Name] experiment:authors:name;;;s<1";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] measurement:sample:comment;;;";
        lst << "[Sample-Thickness] measurement:sample:thickness;;;";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] measurement:history:started;;;sT1";
        lst << "[Time] measurement:history:started;;;sT2";
        lst << "[C] const;;;10.0";
        lst << "[D] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[D-TOF] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[C,D-Offset] measurement:sample:detoffset;;;";
        lst << "[CA-X] measurement:devices:name|sel_ap2|value;;;s;3";
        lst << "[CA-Y] measurement:devices:name|sel_ap2|value;;;s;4";
        lst << "[SA-X] measurement:devices:name|sam_ap|value;;;s;3";
        lst << "[SA-Y] measurement:devices:name|sam_ap|value;;;s;4";
        lst << "[Sum] measurement:devices:name|det_img|value;;;s;1";
        lst << "[Selector] measurement:devices:name|sel_speed|value;;;s;1";
        lst << "[Lambda] measurement:devices:name|sel_lambda|value;;;s;1";
        lst << "[Delta-Lambda] const;;;0.15";
        lst << "[Duration] measurement:devices:name|timer|value;;;s;1";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] measurement:devices:name|mon1|value;;;s;1";
        lst << "[Monitor-2] measurement:devices:name|mon2|value;;;s;1";
        lst << "[Monitor-3|Tr|ROI] measurement:devices:name|mon3|value;;;s;1";
        lst << "[Comment1] experiment:number;;;";
        lst << "[Comment2] experiment:title;;;";
        lst << "[Detector-X || Beamcenter-X] measurement:devices:name|det_beamstop_x|value;;;s;1";
        lst << "[Detector-Y || Beamcenter-Y] measurement:devices:name|det_y|value;;;s;1";
        lst << "[Sample-Motor-1] measurement:devices:name|sam_x|value;;;s;1";
        lst << "[Sample-Motor-2] measurement:devices:name|sam_y|value;;;s;1";
        lst << "[Sample-Motor-3] measurement:devices:name|sam_rot|value;;;s;1";
        lst << "[Sample-Motor-4] measurement:devices:name|sam_phi|value;;;s;1";
        lst << "[Sample-Motor-5] measurement:devices:name|sam_chi|value;;;s;1";
        lst << "[SA-Pos-X] measurement:devices:name|sam_ap|value;;;s;1";
        lst << "[SA-Pos-Y] measurement:devices:name|sam_ap|value;;;s;2";
        lst << "[Field-1] measurement:devices:name|T|value;;;s;1";
        lst << "[Field-2] measurement:devices:name|B|value;;;s;1";
        lst << "[Field-3] measurement:devices:name|julabo12|value;;;s;1";
        lst << "[Field-4] measurement:devices:name|julabo22|value;;;s;1";
        lst << "[RT-Number-Repetitions] const;;;1.0";
        lst << "[RT-Time-Factor] const;;;1.0";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] const;;;out";
        lst << "[Lenses] const;;;out";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 3133.4";
        lst << "[Selector-P2] -0.00195";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 256";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.034";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] Yes";
        lst << "[Detector-Dead-Time] 2.7e-6";
        lst << "[Detector-Dead-Time-DB] 8.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam [DB]";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] No";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Ellipse :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 2";
        lst << "[Mask-Edge-Left-Y] 3";
        lst << "[Mask-Edge-Right-X] 254";
        lst << "[Mask-Edge-Right-Y] 255";
        lst << "[Mask-BeamStop-Left-X] 126";
        lst << "[Mask-BeamStop-Left-Y] 139";
        lst << "[Mask-BeamStop-Right-X] 131";
        lst << "[Mask-BeamStop-Right-Y] 145";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 1";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 256";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] Yes";
        lst << "[Beam-Center-as-Condition] Yes";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.08";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS3-2020")
    {
        lst << "[Instrument] KWS3-2020";
        lst << "[DataFormat] 2";
        lst << "[Color] #8989b7";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[[Include-Sub-Foldes] Yes";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 1";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] #_*_HRD_standard.yaml";
        lst << "[2nd-Header-Lines] 2000";
        lst << "[Pattern] #_*.det";
        lst << "[Pattern-Select-Data] *_*_*_HRD_standard.yaml";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] No";
        lst << "[Flexible-Stop] ";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] experiment:title;;;";
        lst << "[User-Name] experiment:authors:name;;;s<1";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] measurement:sample:description:name;;;";
        lst << "[Sample-Thickness] measurement:sample:thickness;;;";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] measurement:history:started;;;sT1";
        lst << "[Time] measurement:history:started;;;sT2";
        lst << "[C] const;;;10.0";
        lst << "[D] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[D-TOF] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[C,D-Offset] measurement:sample:detoffset;;;";
        lst << "[CA-X] measurement:devices:name|sel_ap2|value;;;s;3";
        lst << "[CA-Y] measurement:devices:name|sel_ap2|value;;;s;4";
        lst << "[SA-X] measurement:devices:name|sam_ap|value;;;s;3";
        lst << "[SA-Y] measurement:devices:name|sam_ap|value;;;s;4";
        lst << "[Sum] measurement:devices:name|det_img|value;;;s;1";
        lst << "[Selector] measurement:devices:name|sel_speed|value;;;s;1";
        lst << "[Lambda] measurement:devices:name|sel_lambda|value;;;s;1";
        lst << "[Delta-Lambda] const;;;0.15";
        lst << "[Duration] measurement:devices:name|timer|value;;;s;1";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] measurement:devices:name|mon1|value;;;s;1";
        lst << "[Monitor-2] measurement:devices:name|mon2|value;;;s;1";
        lst << "[Monitor-3|Tr|ROI] measurement:devices:name|det_roi|value;;;s;1";
        lst << "[Comment1] experiment:number;;;";
        lst << "[Comment2] experiment:title;;;";
        lst << "[Detector-X || Beamcenter-X] measurement:devices:name|det_beamstop_x|value;;;s;1";
        lst << "[Detector-Y || Beamcenter-Y] measurement:devices:name|det_y|value;;;s;1";
        lst << "[Sample-Motor-1] measurement:devices:name|sam_x|value;;;s;1";
        lst << "[Sample-Motor-2] measurement:devices:name|sam_y|value;;;s;1";
        lst << "[Sample-Motor-3] measurement:devices:name|sam_rot|value;;;s;1";
        lst << "[Sample-Motor-4] measurement:devices:name|sam_phi|value;;;s;1";
        lst << "[Sample-Motor-5] measurement:devices:name|sam_chi|value;;;s;1";
        lst << "[SA-Pos-X] measurement:devices:name|sam_ap|value;;;s;1";
        lst << "[SA-Pos-Y] measurement:devices:name|sam_ap|value;;;s;2";
        lst << "[Field-1] measurement:devices:name|T|value;;;s;1";
        lst << "[Field-2] measurement:devices:name|T_julabo_10m_extern|value;;;s;1";
        lst << "[Field-3] measurement:devices:name|T_julabo_1m_extern|value;;;s;1";
        lst << "[Field-4] measurement:devices:name|P_hpc1|value;;;s;1";
        lst << "[RT-Number-Repetitions] const;;;1.0";
        lst << "[RT-Time-Factor] const;;;1.0";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] measurement:devices:name|polarizer|value;;;";
        lst << "[Lenses] const;;;out";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 3133.4";
        lst << "[Selector-P2] -0.00195";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 256";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.034";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] Yes";
        lst << "[Detector-Dead-Time] 2.7e-6";
        lst << "[Detector-Dead-Time-DB] 8.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam [DB]";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] No";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Ellipse :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Ellipse :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 2";
        lst << "[Mask-Edge-Left-Y] 3";
        lst << "[Mask-Edge-Right-X] 254";
        lst << "[Mask-Edge-Right-Y] 255";
        lst << "[Mask-BeamStop-Left-X] 123";
        lst << "[Mask-BeamStop-Left-Y] 123";
        lst << "[Mask-BeamStop-Right-X] 133";
        lst << "[Mask-BeamStop-Right-Y] 133";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 1000";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 1";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 256";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] Yes";
        lst << "[Beam-Center-as-Condition] Yes";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.08";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS3-2018")
    {
        lst << "[Instrument] KWS3-2018";
        lst << "[DataFormat] 2";
        lst << "[Color] #8989b7";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[[Include-Sub-Foldes] Yes";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 1";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] #_*.yaml";
        lst << "[2nd-Header-Lines] 2000";
        lst << "[Pattern] #_*.det";
        lst << "[Pattern-Select-Data] *_*_*.yaml";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] No";
        lst << "[Flexible-Stop] ";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] instrument:proposal;;;";
        lst << "[User-Name] experiment:authors:name;;;s<1";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] measurement:sample:comment;;;";
        lst << "[Sample-Thickness] measurement:sample:thickness;;;";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] measurement:history:started;;;sT1";
        lst << "[Time] measurement:history:started;;;sT2";
        lst << "[C] const;;;10.0";
        lst << "[D] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[D-TOF] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[C,D-Offset] measurement:sample:detoffset;;;";
        lst << "[CA-X] measurement:devices:name|sel_ap2|value;;;s;3";
        lst << "[CA-Y] measurement:devices:name|sel_ap2|value;;;s;4";
        lst << "[SA-X] measurement:devices:name|sam_ap|value;;;s;3";
        lst << "[SA-Y] measurement:devices:name|sam_ap|value;;;s;4";
        lst << "[Sum] measurement:devices:name|det_img|value;;;s;1";
        lst << "[Selector] measurement:devices:name|sel_speed|value;;;s;1";
        lst << "[Lambda] measurement:devices:name|sel_lambda|value;;;s;1";
        lst << "[Delta-Lambda] const;;;0.15";
        lst << "[Duration] measurement:devices:name|timer|value;;;s;1";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] measurement:devices:name|mon1|value;;;s;1";
        lst << "[Monitor-2] measurement:devices:name|mon2|value;;;s;1";
        lst << "[Monitor-3|Tr|ROI] measurement:devices:name|det_roi|value;;;s;1";
        lst << "[Comment1] experiment:number;;;";
        lst << "[Comment2] experiment:title;;;";
        lst << "[Detector-X || Beamcenter-X] measurement:devices:name|det_beamstop_x|value;;;s;1";
        lst << "[Detector-Y || Beamcenter-Y] measurement:devices:name|det_y|value;;;s;1";
        lst << "[Sample-Motor-1] measurement:devices:name|sam_x|value;;;s;1";
        lst << "[Sample-Motor-2] measurement:devices:name|sam_y|value;;;s;1";
        lst << "[Sample-Motor-3] measurement:devices:name|sam_rot|value;;;s;1";
        lst << "[Sample-Motor-4] measurement:devices:name|sam_phi|value;;;s;1";
        lst << "[Sample-Motor-5] measurement:devices:name|sam_chi|value;;;s;1";
        lst << "[SA-Pos-X] measurement:devices:name|sam_ap|value;;;s;1";
        lst << "[SA-Pos-Y] measurement:devices:name|sam_ap|value;;;s;2";
        lst << "[Field-1] measurement:devices:name|T|value;;;s;1";
        lst << "[Field-2] measurement:devices:name|B|value;;;s;1";
        lst << "[Field-3] measurement:devices:name|P|value;;;s;1";
        lst << "[Field-4] measurement:devices:name|X|value;;;s;1";
        lst << "[RT-Number-Repetitions] const;;;1.0";
        lst << "[RT-Time-Factor] const;;;1.0";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] const;;;out";
        lst << "[Lenses] const;;;out";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 3133.4";
        lst << "[Selector-P2] -0.00195";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 256";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.034";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] Yes";
        lst << "[Detector-Dead-Time] 2.7e-6";
        lst << "[Detector-Dead-Time-DB] 8.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam [DB]";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] No";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Ellipse :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Ellipse :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 2";
        lst << "[Mask-Edge-Left-Y] 3";
        lst << "[Mask-Edge-Right-X] 254";
        lst << "[Mask-Edge-Right-Y] 255";
        lst << "[Mask-BeamStop-Left-X] 123";
        lst << "[Mask-BeamStop-Left-Y] 123";
        lst << "[Mask-BeamStop-Right-X] 133";
        lst << "[Mask-BeamStop-Right-Y] 133";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 1";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 256";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] Yes";
        lst << "[Beam-Center-as-Condition] Yes";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.08";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS3-VHRD")
    {
        lst << "[Instrument] KWS3-VHRD";
        lst << "[DataFormat] 2";
        lst << "[Color] #8989b7";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] Yes";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] *.dev";
        lst << "[2nd-Header-Lines] 3000";
        lst << "[Pattern] *_detector";
        lst << "[Pattern-Select-Data] *_detector";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] No";
        lst << "[Flexible-Stop] ";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] data:comment;;;";
        lst << "[User-Name] user;;;";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] data:comment;;;";
        lst << "[Sample-Thickness] SampleThickness:position;;;";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] created;;;";
        lst << "[Time] saved;;;";
        lst << "[C] const;;;10.0";
        lst << "[D] D:position;;;";
        lst << "[D-TOF] D:position;;;";
        lst << "[C,D-Offset] const;;;0";
        lst << "[CA-X] Slit_Selector_Width:position;;;";
        lst << "[CA-Y] Slit_Selector_Hight:position;;;";
        lst << "[SA-X] Slit_10m_Width:position;;;0.2";
        lst << "[SA-Y] Slit_10m_Hight:position;;;0.2";
        lst << "[Sum] detector:position;;;";
        lst << "[Selector] const;;;1";
        lst << "[Lambda] Lambda:position;;;";
        lst << "[Delta-Lambda] const;;;0.15";
        lst << "[Duration] time:position;;;";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] Monitor_1:position;;;";
        lst << "[Monitor-2] Monitor_1:position;;;";
        lst << "[Monitor-3|Tr|ROI] const;;;1";
        lst << "[Comment1] command;;;";
        lst << "[Comment2] ;;;";
        lst << "[Detector-X || Beamcenter-X] det_x:position;;;";
        lst << "[Detector-Y || Beamcenter-Y] det_y:position;;;";
        lst << "[Sample-Motor-1] sam10_vac_x:position;;;";
        lst << "[Sample-Motor-2] sam10_vac_y:position;;;";
        lst << "[Sample-Motor-3] sam01_vac_x:position;;;";
        lst << "[Sample-Motor-4] sam01_vac_y:position;;;";
        lst << "[Sample-Motor-5] ;;;";
        lst << "[SA-Pos-X] mir_ap2_x0:position;;;";
        lst << "[SA-Pos-Y] mir_ap2_y0:position;;;";
        lst << "[Field-1] tlc40:position;;;";
        lst << "[Field-2] julabo11:position;;;";
        lst << "[Field-3] Lakeshore:position;;;";
        lst << "[Field-4] Huber:position;;;";
        lst << "[RT-Number-Repetitions] const;;;1.0";
        lst << "[RT-Time-Factor] const;;;1.0";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] const;;;out";
        lst << "[Lenses] const;;;out-out-out";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 3133.4";
        lst << "[Selector-P2] -0.00195";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 256";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.0116";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] No";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] Yes";
        lst << "[Detector-Dead-Time] 2.7e-6";
        lst << "[Detector-Dead-Time-DB] 8.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam [DB]";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] No";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Rectangle :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Ellipse :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 2";
        lst << "[Mask-Edge-Left-Y] 2";
        lst << "[Mask-Edge-Right-X] 255";
        lst << "[Mask-Edge-Right-Y] 255";
        lst << "[Mask-BeamStop-Left-X] 121";
        lst << "[Mask-BeamStop-Left-Y] 121";
        lst << "[Mask-BeamStop-Right-X] 133";
        lst << "[Mask-BeamStop-Right-Y] 133";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] No";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 2";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 256";
        lst << "[Options-1D-OutputFormat] 1";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] Yes";
        lst << "[Beam-Center-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.03";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS3-VHRD-2020")
    {
        lst << "[Instrument] KWS3-VHRD-2020";
        lst << "[DataFormat] 2";
        lst << "[Color] #8989b7";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] Yes";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 1";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] #_*_VHRD_standard.yaml";
        lst << "[2nd-Header-Lines] 2000";
        lst << "[Pattern] #_*.det";
        lst << "[Pattern-Select-Data] *_*_*_VHRD_standard.yaml";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] No";
        lst << "[Flexible-Stop] ";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] experiment:title;;;";
        lst << "[User-Name] experiment:authors:name;;;s<1";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] measurement:sample:description:name;;;";
        lst << "[Sample-Thickness] measurement:sample:thickness;;;";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] measurement:history:started;;;sT1";
        lst << "[Time] measurement:history:started;;;sT2";
        lst << "[C] const;;;10.0";
        lst << "[D] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[D-TOF] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[C,D-Offset] measurement:sample:detoffset;;;";
        lst << "[CA-X] measurement:devices:name|sel_ap2|value;;;s;3";
        lst << "[CA-Y] measurement:devices:name|sel_ap2|value;;;s;4";
        lst << "[SA-X] measurement:devices:name|sam_ap|value;;;s;3";
        lst << "[SA-Y] measurement:devices:name|sam_ap|value;;;s;4";
        lst << "[Sum] measurement:devices:name|det_img|value;;;s;1";
        lst << "[Selector] measurement:devices:name|sel_speed|value;;;s;1";
        lst << "[Lambda] measurement:devices:name|sel_lambda|value;;;s;1";
        lst << "[Delta-Lambda] const;;;0.15";
        lst << "[Duration] measurement:devices:name|timer|value;;;s;1";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] measurement:devices:name|mon1|value;;;s;1";
        lst << "[Monitor-2] measurement:devices:name|mon2|value;;;s;1";
        lst << "[Monitor-3|Tr|ROI] measurement:devices:name|det_roi|value;;;s;1";
        lst << "[Comment1] experiment:number;;;";
        lst << "[Comment2] experiment:title;;;";
        lst << "[Detector-X || Beamcenter-X] measurement:devices:name|det_beamstop_x|value;;;s;1";
        lst << "[Detector-Y || Beamcenter-Y] measurement:devices:name|det_y|value;;;s;1";
        lst << "[Sample-Motor-1] measurement:devices:name|sam_x|value;;;s;1";
        lst << "[Sample-Motor-2] measurement:devices:name|sam_y|value;;;s;1";
        lst << "[Sample-Motor-3] measurement:devices:name|sam_rot|value;;;s;1";
        lst << "[Sample-Motor-4] measurement:devices:name|sam_phi|value;;;s;1";
        lst << "[Sample-Motor-5] measurement:devices:name|sam_chi|value;;;s;1";
        lst << "[SA-Pos-X] measurement:devices:name|sam_ap|value;;;s;1";
        lst << "[SA-Pos-Y] measurement:devices:name|sam_ap|value;;;s;2";
        lst << "[Field-1] measurement:devices:name|T|value;;;s;1";
        lst << "[Field-2] measurement:devices:name|T_julabo_10m_extern|value;;;s;1";
        lst << "[Field-3] measurement:devices:name|T_julabo_1m_extern|value;;;s;1";
        lst << "[Field-4] measurement:devices:name|P_hpc1|value;;;s;1";
        lst << "[RT-Number-Repetitions] const;;;1.0";
        lst << "[RT-Time-Factor] const;;;1.0";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] const;;;out";
        lst << "[Lenses] const;;;out";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 3133.4";
        lst << "[Selector-P2] -0.00195";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 256";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.0116";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] Yes";
        lst << "[Detector-Dead-Time] 2.7e-6";
        lst << "[Detector-Dead-Time-DB] 8.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam [DB]";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] No";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-BeamStop-Shape] Ellipse :: Shape of Edge";
        lst << "[Mask-Edge-Left-X] 2";
        lst << "[Mask-Edge-Left-Y] 3";
        lst << "[Mask-Edge-Right-X] 254";
        lst << "[Mask-Edge-Right-Y] 255";
        lst << "[Mask-BeamStop-Left-X] 121";
        lst << "[Mask-BeamStop-Left-Y] 121";
        lst << "[Mask-BeamStop-Right-X] 136";
        lst << "[Mask-BeamStop-Right-Y] 136";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 1000";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 1";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 256";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] Yes";
        lst << "[Beam-Center-as-Condition] Yes";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.03";
        lst << "[File-Ext] ";
    }
    else if (instrName=="KWS3-VHRD-2018")
    {
        lst << "[Instrument] KWS3-VHRD-2018";
        lst << "[DataFormat] 2";
        lst << "[Color] #8989b7";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] Yes";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 1";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] #_*.yaml";
        lst << "[2nd-Header-Lines] 2000";
        lst << "[Pattern] #_*.det";
        lst << "[Pattern-Select-Data] *_*_*.yaml";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] No";
        lst << "[Flexible-Stop] ";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] instrument:proposal;;;";
        lst << "[User-Name] experiment:authors:name;;;s<1";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] measurement:sample:comment;;;";
        lst << "[Sample-Thickness] measurement:sample:thickness;;;";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] measurement:history:started;;;sT1";
        lst << "[Time] measurement:history:started;;;sT2";
        lst << "[C] const;;;10.0";
        lst << "[D] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[D-TOF] measurement:devices:name|sample_pos|value;;;sm1";
        lst << "[C,D-Offset] measurement:sample:detoffset;;;";
        lst << "[CA-X] measurement:devices:name|sel_ap2|value;;;s;3";
        lst << "[CA-Y] measurement:devices:name|sel_ap2|value;;;s;4";
        lst << "[SA-X] measurement:devices:name|sam_ap|value;;;s;3";
        lst << "[SA-Y] measurement:devices:name|sam_ap|value;;;s;4";
        lst << "[Sum] measurement:devices:name|det_img|value;;;s;1";
        lst << "[Selector] measurement:devices:name|sel_speed|value;;;s;1";
        lst << "[Lambda] measurement:devices:name|sel_lambda|value;;;s;1";
        lst << "[Delta-Lambda] const;;;0.15";
        lst << "[Duration] measurement:devices:name|timer|value;;;s;1";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] measurement:devices:name|mon1|value;;;s;1";
        lst << "[Monitor-2] measurement:devices:name|mon2|value;;;s;1";
        lst << "[Monitor-3|Tr|ROI] measurement:devices:name|det_roi|value;;;s;1";
        lst << "[Comment1] experiment:number;;;";
        lst << "[Comment2] experiment:title;;;";
        lst << "[Detector-X || Beamcenter-X] measurement:devices:name|det_beamstop_x|value;;;s;1";
        lst << "[Detector-Y || Beamcenter-Y] measurement:devices:name|det_y|value;;;s;1";
        lst << "[Sample-Motor-1] measurement:devices:name|sam_x|value;;;s;1";
        lst << "[Sample-Motor-2] measurement:devices:name|sam_y|value;;;s;1";
        lst << "[Sample-Motor-3] measurement:devices:name|sam_rot|value;;;s;1";
        lst << "[Sample-Motor-4] measurement:devices:name|sam_phi|value;;;s;1";
        lst << "[Sample-Motor-5] measurement:devices:name|sam_chi|value;;;s;1";
        lst << "[SA-Pos-X] measurement:devices:name|sam_ap|value;;;s;1";
        lst << "[SA-Pos-Y] measurement:devices:name|sam_ap|value;;;s;2";
        lst << "[Field-1] measurement:devices:name|T|value;;;s;1";
        lst << "[Field-2] measurement:devices:name|B|value;;;s;1";
        lst << "[Field-3] measurement:devices:name|P|value;;;s;1";
        lst << "[Field-4] measurement:devices:name|X|value;;;s;1";
        lst << "[RT-Number-Repetitions] const;;;1.0";
        lst << "[RT-Time-Factor] const;;;1.0";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] const;;;out";
        lst << "[Lenses] const;;;out";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 3133.4";
        lst << "[Selector-P2] -0.00195";
        lst << "[Detector-Data-Dimension] 2";
        lst << "[Detector-Data-Focus] 256";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.0116";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 256";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] Yes";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] Yes";
        lst << "[Detector-Dead-Time] 2.7e-6";
        lst << "[Detector-Dead-Time-DB] 8.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Direct Beam [DB]";
        lst << "[Calibrant] DirectBeam[KWS-3]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] No";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Ellipse :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 2";
        lst << "[Mask-Edge-Left-Y] 3";
        lst << "[Mask-Edge-Right-X] 254";
        lst << "[Mask-Edge-Right-Y] 255";
        lst << "[Mask-BeamStop-Left-X] 126";
        lst << "[Mask-BeamStop-Left-Y] 139";
        lst << "[Mask-BeamStop-Right-X] 131";
        lst << "[Mask-BeamStop-Right-Y] 145";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 1";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-2D-xyDimension-Pixel] Yes";
        lst << "[Options-2D-Output-Format] 0";
        lst << "[Options-2D-Header-Output-Format] No";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 256";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] Yes";
        lst << "[Beam-Center-as-Condition] Yes";
        lst << "[Polarization-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 30";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] Yes";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.03";
        lst << "[File-Ext] ";
    }
    else if (instrName=="MARIA")
    {
        lst << "[Instrument] MARIA";
        lst << "[DataFormat] 2";
        lst << "[Color] #c0c0c0";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] No";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 1";
        lst << "[Units-Thickness] 1";
        lst << "[Units-Time] 0";
        lst << "[Units-Time-RT] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 0";
        lst << "[2nd-Header-OK] Yes";
        lst << "[2nd-Header-Pattern] *.dev";
        lst << "[2nd-Header-Lines] 10000";
        lst << "[Pattern] *";
        lst << "[Pattern-Select-Data] *[0-9]";
        lst << "[Header-Number-Lines] 0";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] Yes";
        lst << "[Flexible-Stop] %Counts";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] data:name;;;";
        lst << "[User-Name] user:name;;;";
        lst << "[Sample-Run-Number] data:name;;;";
        lst << "[Sample-Title] data:name;;;";
        lst << "[Sample-Thickness] const;;;1";
        lst << "[Sample-Position-Number] const;;;1";
        lst << "[Date] created;;;";
        lst << "[Time] saved;;;s 4";
        lst << "[C] const;;;4";
        lst << "[D] const;;;2";
        lst << "[D-TOF] const;;;2";
        lst << "[C,D-Offset] const;;;0";
        lst << "[CA-X] const;;;8";
        lst << "[CA-Y] const;;;8";
        lst << "[SA-X] const;;;8";
        lst << "[SA-Y] const;;;8";
        lst << "[Sum] full:position;;;";
        lst << "[Selector] const;;;1";
        lst << "[Lambda] wavelength:position;;;";
        lst << "[Delta-Lambda] const;;;0.1";
        lst << "[Duration] time:position;;;";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] monitor1:position;;;";
        lst << "[Monitor-2] monitor2:position;;;";
        lst << "[Monitor-3|Tr|ROI] monitor3:position;;;";
        lst << "[Comment1] command;;;";
        lst << "[Comment2] instrument;;;";
        lst << "[Detector-X || Beamcenter-X] const;;;0";
        lst << "[Detector-Y || Beamcenter-Y] const;;;0";
        lst << "[Sample-Motor-1] const;;;0";
        lst << "[Sample-Motor-2] const;;;0";
        lst << "[Sample-Motor-3] const;;;0";
        lst << "[Sample-Motor-4] const;;;0";
        lst << "[Sample-Motor-5] const;;;0";
        lst << "[SA-Pos-X] const;;;0";
        lst << "[SA-Pos-Y] const;;;0";
        lst << "[Field-1] const;;;0";
        lst << "[Field-2] const;;;0";
        lst << "[Field-3] const;;;0";
        lst << "[Field-4] const;;;0";
        lst << "[RT-Number-Repetitions] const;;;1";
        lst << "[RT-Time-Factor] const;;;1";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] const;;;out";
        lst << "[Polarization] const;;;out";
        lst << "[Lenses] const;;;out-out-out";
        lst << "[Slices-Count] const;;;1";
        lst << "[Slices-Duration] const;;;1";
        lst << "[Slices-Current-Number] const;;;1";
        lst << "[Slices-Current-Duration] const;;;1";
        lst << "[Slices-Current-Monitor1] const;;;0";
        lst << "[Slices-Current-Monitor2] const;;;0";
        lst << "[Slices-Current-Monitor3] const;;;0";
        lst << "[Slices-Current-Sum] const;;;0";
        lst << "[Selector-Read-from-Header] Yes";
        lst << "[Selector-P1] 2096";
        lst << "[Selector-P2] 0.054";
        lst << "[Detector-Data-Dimension] 4";
        lst << "[Detector-Data-Focus] 600";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.066";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 1024";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] No";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] No";
        lst << "[Detector-Dead-Time] 1e-6";
        lst << "[Detector-Dead-Time-DB] 2.6e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Flat Scatter [FS]";
        lst << "[Calibrant] Plexi-1.5mm-[Current]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] Yes";
        lst << "[Mask-Edge] Yes";
        lst<< "[Mask-Edge-Shape]Rectangle";
        lst<< "[Mask-BeamStop-Shape]Rectangle";
        lst << "[Mask-Edge-Left-X] 1";
        lst << "[Mask-Edge-Left-Y] 1";
        lst << "[Mask-Edge-Right-X] 600";
        lst << "[Mask-Edge-Right-Y] 600";
        lst << "[Mask-BeamStop-Left-X] 296";
        lst << "[Mask-BeamStop-Left-Y] 258";
        lst << "[Mask-BeamStop-Right-X] 312";
        lst << "[Mask-BeamStop-Right-Y] 298";
        lst << "[Mask-Dead-Ros] ";
        lst << "[Mask-Dead-Cols] ";
        lst << "[Mask-Triangular] ";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 100";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] No";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 0.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 0";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 0";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-1D-RADmethod-HF] Yes";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 600";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] No";
        lst << "[Beam-Center-as-Condition] No";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 100";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] No";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] No";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.0";
        lst << "[File-Ext] ";
    }
    else if (instrName == "SANS1")
    {
        lst << "[Instrument] SANS1";
        lst << "[DataFormat] 0";
        lst << "[Color] #D02C79";
        lst << "[Input-Folder] home";
        lst << "[Output-Folder] home";
        lst << "[Include-Sub-Foldes] No";
        lst << "[Units-Lambda] 0";
        lst << "[Units-Appertures] 0";
        lst << "[Units-Thickness] 0";
        lst << "[Units-Time] 0";
        lst << "[Units-C] 0";
        lst << "[Units-C-D-Offset] 0";
        lst << "[Units-Selector] 2";
        lst << "[2nd-Header-OK] No";
        lst << "[2nd-Header-Pattern] ";
        lst << "[2nd-Header-Lines] 0";
        lst << "[Pattern] D0*.001";
        lst << "[Pattern-Select-Data] D0*.001";
        lst << "[Header-Number-Lines] 1200";
        lst << "[Data-Header-Lines] 0";
        lst << "[Lines-Between-Frames] 0";
        lst << "[Flexible-Header] Yes";
        lst << "[Flexible-Stop] %Counts";
        lst << "[Remove-None-Printable-Symbols] No";
        lst << "[Image-Data] No";
        lst << "[XML-base] ";
        lst << "[Experiment-Title] {Title=};;;s=1";
        lst << "[User-Name] {User=};;;s=1";
        lst << "[Sample-Run-Number] {FileName=};;;s=1";
        lst << "[Sample-Title] {SampleName=};;;s=1";
        lst << "[Sample-Thickness] {Thickness};;;s=1";
        lst << "[Sample-Position-Number] {Position=};;;s=1";
        lst << "[Date] {FromDate=};;;s=1";
        lst << "[Time] {FromTime=};;;s=1";
        lst << "[C] {Collimation=};;;s=1";
        lst << "[D] {SD=};;;1";
        lst << "[D-TOF] {SD=};;;1";
        lst << "[C,D-Offset] const;;;0";
        lst << "[CA-X] const;;;3";
        lst << "[CA-Y] const;;;5";
        lst << "[SA-X] {SlitWidth=};;;s=1";
        lst << "[SA-Y] {SlitHeight=};;;s=1";
        lst << "[Sum] {Sum=};;;s=1";
        lst << "[Selector] {LambdaC=};;;s=1";
        lst << "[Lambda] {Lambda=};;;s=1";
        lst << "[Delta-Lambda] const;;;0.1";
        lst << "[Duration] {Time=};;;s=1";
        lst << "[Duration-Factor] const;;;1";
        lst << "[Monitor-1] {Moni1=};;;s=1";
        lst << "[Monitor-2] {Moni2=};;;s=1";
        lst << "[Monitor-3|Tr|ROI] {Transmission=};;;s=1";
        lst << "[Comment1] {Environment=};;;s=1";
        lst << "[Comment2] {Type=};;;s=1";
        lst << "[Detector-X || Beamcenter-X] {BeamstopX=};;;s=1";
        lst << "[Detector-Y || Beamcenter-Y] {BeamstopY=};;;s=1";
        lst << "[Sample-Motor-1] {Position=};;;s=1";
        lst << "[Sample-Motor-2] {Omega=};;;s=1";
        lst << "[Sample-Motor-3] {Chi};;;s=1";
        lst << "[Sample-Motor-4] ;;;";
        lst << "[Sample-Motor-5] ;;;";
        lst << "[SA-Pos-X] const;;;0";
        lst << "[SA-Pos-Y] const;;;0";
        lst << "[Field-1] {Temperature=};;;s=1";
        lst << "[Field-2] {TempDev=};;;s=1";
        lst << "[Field-3] {Temp1=};;;s=1";
        lst << "[Field-4] ;;;";
        lst << "[RT-Number-Repetitions] const;;;1";
        lst << "[RT-Time-Factor] const;;;1";
        lst << "[RT-Current-Number] const;;;1";
        lst << "[Attenuator] {Attenuator=};;;s=1";
        lst << "[Polarization] const;;;out";
        lst << "[Lenses] const;;;out";
        lst << "[Slices-Count] ;;;";
        lst << "[Slices-Duration] ;;;";
        lst << "[Slices-Current-Number] ;;;";
        lst << "[Slices-Current-Duration] ;;;";
        lst << "[Slices-Current-Monitor1] ;;;";
        lst << "[Slices-Current-Monitor2] ;;;";
        lst << "[Slices-Current-Monitor3] ;;;";
        lst << "[Slices-Current-Sum] ;;;";
        lst << "[Selector-Read-from-Header] No";
        lst << "[Selector-P1] 2053.65";
        lst << "[Selector-P2] 0.196";
        lst << "[Detector-Data-Dimension] 1";
        lst << "[Detector-Data-Focus] 128";
        lst << "[Detector-Binning] 0";
        lst << "[Detector-Pixel-Size] 0.80";
        lst << "[Detector-Pixel-Size-Asymetry] 1.0";
        lst << "[Detector-Data-Numbers-Per-Line] 128";
        lst << "[Detector-Data-Tof-Numbers-Per-Line] 1";
        lst << "[Detector-Data-Transpose] No";
        lst << "[Detector-X-to-Minus-X] No";
        lst << "[Detector-Y-to-Minus-Y] No";
        lst << "[Detector-Dead-Time] 1.0e-6";
        lst << "[Detector-Dead-Time-DB] 1.0e-6";
        lst << "[Options-2D-DeadTimeModel-NonPara] Yes";
        lst << "[Options-2D-CenterMethod] HF";
        lst << "[Calibrant-Type] Flat Scatter [FS]";
        lst << "[Calibrant] H2O[SANS-1]";
        lst << "[Use-Active-Mask-and-Sensitivity-Matrixes] No";
        lst << "[Calculate-Calibrant-Transmission-by-Equation] Yes";
        lst << "[Mask-BeamStop] Yes";
        lst << "[Mask-Edge] Yes";
        lst << "[Mask-Edge-Shape] Rectangle :: Shape of Edge";
        lst << "[Mask-BeamStop-Shape] Rectangle :: Shape of Beam-Stop";
        lst << "[Mask-Edge-Left-X] 1";
        lst << "[Mask-Edge-Left-Y] 9";
        lst << "[Mask-Edge-Right-X] 128";
        lst << "[Mask-Edge-Right-Y] 125";
        lst << "[Mask-BeamStop-Left-X] 58";
        lst << "[Mask-BeamStop-Left-Y] 60";
        lst << "[Mask-BeamStop-Right-X] 70";
        lst << "[Mask-BeamStop-Right-Y] 72";
        lst << "[Sensitivity-SpinBoxErrLeftLimit] 0.1";
        lst << "[Sensitivity-SpinBoxErrRightLimit] 99.9";
        lst << "[Sensitivity-CheckBoxSensError] No";
        lst << "[Sensitivity-in-Use] Yes";
        lst << "[Sensitivity-Tr-Option] No";
        lst << "[Sensitivity-Masked-Pixels-Value] 1.000";
        lst << "[Transmission-Method] Direct Beam  [dead-time +]";
        lst << "[Options-2D-HighQ] Yes";
        lst << "[Options-2D-HighQ-Parallax-Type] 1";
        lst << "[Options-2D-HighQ-Tr] Yes";
        lst << "[Options-2D-Polar-Resolusion] 50";
        lst << "[Options-2D-Mask-Negative-Points] No";
        lst << "[Options-2D-Normalization-Type] 0";
        lst << "[Options-2D-Normalization-Factor] 1";
        lst << "[Options-2D-Mask-Normalization-BC] Yes";
        lst << "[Options-1D-RADmethod-HF] No";
        lst << "[Options-1D-RemoveFirst] 0";
        lst << "[Options-1D-RemoveLast] 0";
        lst << "[Options-1D-RemoveNegativePoints] No";
        lst << "[Options-1D-QxQy-From] 1";
        lst << "[Options-1D-QxQy-To] 128";
        lst << "[Options-1D-OutputFormat] 0";
        lst << "[Options-1D-QI-Presentation] 0";
        lst << "[Sample-Position-As-Condition] No";
        lst << "[Attenuator-as-Condition] Yes";
        lst << "[Beam-Center-as-Condition] Yes";
        lst << "[Reread-Existing-Runs] No";
        lst << "[Find-Center-For-EveryFile] No";
        lst << "[Tr-Force-Copy-Paste] Yes";
        lst << "[Sampe-Name-As-RunTableName] No";
        lst << "[Generate-MergingTable] Yes";
        lst << "[Auto-Merging] No";
        lst << "[Overlap-Merging] 100";
        lst << "[Rewrite-Output] Yes";
        lst << "[Skipt-Tr-Configurations] Yes";
        lst << "[Skipt-Output-Folders] Yes";
        lst << "[Resolusion-Focusing] No";
        lst << "[Resolusion-CA-Round] No";
        lst << "[Resolusion-SA-Round] No";
        lst << "[Resolusion-Detector] 0.8";
        lst << "[File-Ext] ";
    }
    else
    {
        QString instrPath;

        if (!QDir(app()->sasPath + "/SANSinstruments/").exists())
            QDir().mkdir(app()->sasPath + "/SANSinstruments/");

        instrPath = app()->sasPath + "/SANSinstruments/";
        instrPath = instrPath.replace("//", "/");

        QFile f(instrPath + "/" + instrName + ".SANS");

        if (!f.open(QIODevice::ReadOnly))
        {
            //*************************************Log Window Output
            QMessageBox::warning(this,"Could not read file:: "+instrName+".SANS", tr("qtiSAS::DAN"));
            //*************************************Log Window Output
            return;
        }
        
        //+++
        QTextStream t( &f );
        while(!t.atEnd())
        {
            lst<<t.readLine();
        }
        f.close();
    }
    

    
    
    for (int i=0; i<lst.count(); i++)
    {
        if (lst[i].contains("[Info]")) {lst[i]=lst[i].replace("[Info]","[Sample-Title]"); continue;};
        if (lst[i].contains("[Sample-Nr]")) {lst[i]=lst[i].replace("[Sample-Nr]","[Sample-Position-Number]"); continue;};
        if (lst[i].contains("[DeltaLambda]")) {lst[i]=lst[i].replace("[DeltaLambda]","[Delta-Lambda]"); continue;};
        if (lst[i].contains("[Offset]")) {lst[i]=lst[i].replace("[Offset]","[C,D-Offset]"); continue;};
        if (lst[i].contains("[Time-Factor]")) {lst[i]=lst[i].replace("[Time-Factor]","[Duration-Factor]"); continue;};
        if (lst[i].contains("[Thickness]")) {lst[i]=lst[i].replace("[Thickness]","[Sample-Thickness]"); continue;};
        if (lst[i].contains("[Run]")) {lst[i]=lst[i].replace("[Run]","[Sample-Run-Number]"); continue;};
        if (lst[i].contains("[Who]")) {lst[i]=lst[i].replace("[Who]","[User-Name]"); continue;};
        if (lst[i].contains("[Tiff-Data]"))
        {
            lst[i] = lst[i].replace("[Tiff-Data]", "[Image-Data]");
            continue;
        }
    }
    
    QStringList stringList;
    QString line, selectPattern;
    int binning=0;
    int DD=0;
    int RoI=0;
    int md;
    
    selectPattern="";
    
    enum class Parameter
    {
        Instrument,
        Instrument_Mode,
        DataFormat,
        Color,
        Input_Folder,
        Include_Sub_Foldes,
        Output_Folder,
        Units_Lambda,
        Units_Appertures,
        Units_Thickness,
        Units_Time,
        Units_Time_RT,
        Units_C,
        Units_C_D_Offset,
        Units_Selector,
        Second_Header_OK,
        Second_Header_Pattern,
        Second_Header_Lines,
        Data_Header_Lines,
        Lines_Between_Frames,
        Pattern,
        Pattern_Select_Data,
        Header_Number_Lines,
        Flexible_Header,
        Flexible_Stop,
        Remove_None_Printable_Symbols,
        Image_Data,
        HDF_Data,
        YAML_Data,
        HDF_detector_entry,
        HDF_data_structure,
        YAML_detector_entry,
        XML_base,
        Selector_Read_from_Header,
        Selector_P1,
        Selector_P2,
        Detector_Data_Dimension,
        Detector_Data_Focus,
        Detector_Binning,
        Detector_Pixel_Size,
        Detector_Pixel_Size_Asymetry,
        Detector_Data_Numbers_Per_Line,
        Detector_Data_Tof_Numbers_Per_Line,
        Detector_Data_Transpose,
        Detector_X_to_Minus_X,
        Detector_Y_to_Minus_Y,
        Detector_Dead_Time,
        Detector_Dead_Time_DB,
        Monitor1_Dead_Time,
        Monitor2_Dead_Time,
        Monitor3_Dead_Time,
        Options_2D_DeadTimeModel_NonPara,
        Options_2D_CenterMethod,
        DetRotation_X_Read_from_Header,
        DetRotation_Angle_X,
        DetRotation_Invert_Angle_X,
        DetRotation_Y_Read_from_Header,
        DetRotation_Angle_Y,
        DetRotation_Invert_Angle_Y,
        Calibrant_Type,
        Calibrant,
        Use_Active_Mask_and_Sensitivity_Matrixes,
        Calculate_Calibrant_Transmission_by_Equation,
        Mask_Edge_Shape,
        Mask_BeamStop_Shape,
        Mask_Edge,
        Mask_BeamStop,
        Mask_Edge_Left_X,
        Mask_Edge_Left_Y,
        Mask_Edge_Right_X,
        Mask_Edge_Right_Y,
        Mask_BeamStop_Left_X,
        Mask_BeamStop_Left_Y,
        Mask_BeamStop_Right_X,
        Mask_BeamStop_Right_Y,
        Mask_Dead_Rows,
        Mask_Dead_Cols,
        Mask_Triangular,
        Sensitivity_SpinBoxErrRightLimit,
        Sensitivity_SpinBoxErrLeftLimit,
        Sensitivity_CheckBoxSensError,
        Sensitivity_Tr_Option,
        Sensitivity_in_Use,
        Sensitivity_Masked_Pixels_Value,
        Transmission_Method,
        Options_2D_HighQ,
        Options_2D_HighQ_Parallax_Type,
        Options_2D_HighQ_Tr,
        Options_2D_Polar_Resolusion,
        Options_2D_Mask_Negative_Points,
        Options_2D_Normalization_Type,
        Options_2D_Normalization_Factor,
        Options_2D_Mask_Normalization_BC,
        Options_2D_xyDimension_Pixel,
        Options_2D_Output_Format,
        Options_2D_Header_Output_Format,
        Options_2D_Header_SASVIEW,
        Options_1D_RADmethod_HF,
        Options_1D_RAD_LinearFactor,
        Options_1D_RAD_ProgressiveFactor,
        Options_1D_RemoveFirst,
        Options_1D_RemoveLast,
        Options_1D_RemoveNegativePoints,
        Options_1D_QxQy_From,
        Options_1D_QxQy_To,
        Options_1D_QxQy_BS,
        Options_1D_OutputFormat,
        Options_1D_OutputFormat_PlusHeader,
        Options_1D_Anisotropy,
        Options_1D_AnisotropyAngle,
        Options_1D_QI_Presentation,
        Sample_Position_As_Condition,
        Attenuator_as_Condition,
        Beam_Center_as_Condition,
        Polarization_as_Condition,
        DetectorAngle_as_Condition,
        Reread_Existing_Runs,
        Find_Center_For_EveryFile,
        Tr_Force_Copy_Paste,
        Sampe_Name_As_RunTableName,
        Generate_MergingTable,
        Auto_Merging,
        Overlap_Merging,
        Rewrite_Output,
        Skipt_Tr_Configurations,
        Skipt_Output_Folders,
        Resolusion_Focusing,
        Resolusion_Detector,
        Resolusion_CA_Round,
        Resolusion_SA_Round,
        File_Ext,
        POL_ALIAS_UP,
        POL_ALIAS_DOWN,
        POL_ALIAS_UP_UP,
        POL_ALIAS_UP_DOWN,
        POL_ALIAS_DOWN_DOWN,
        POL_ALIAS_DOWN_UP,
        POLARIZATION,
        POL_TRANSMISSION,
        POL_FLIPPER_EFFICIENCY,
        ANALYZER_TRANSMISSION,
        ANALYZER_EFFICIENCY
    };

    std::map<QString, Parameter> stringToEnumMap = {
        {"Instrument", Parameter::Instrument},
        {"Instrument-Mode", Parameter::Instrument_Mode},
        {"DataFormat", Parameter::DataFormat},
        {"Color", Parameter::Color},
        {"Input-Folder", Parameter::Input_Folder},
        {"Include-Sub-Foldes", Parameter::Include_Sub_Foldes},
        {"Output-Folder", Parameter::Output_Folder},
        {"Units-Lambda", Parameter::Units_Lambda},
        {"Units-Appertures", Parameter::Units_Appertures},
        {"Units-Thickness", Parameter::Units_Thickness},
        {"Units-Time", Parameter::Units_Time},
        {"Units-Time-RT", Parameter::Units_Time_RT},
        {"Units-C", Parameter::Units_C},
        {"Units-C-D-Offset", Parameter::Units_C_D_Offset},
        {"Units-Selector", Parameter::Units_Selector},
        {"2nd-Header-OK", Parameter::Second_Header_OK},
        {"2nd-Header-Pattern", Parameter::Second_Header_Pattern},
        {"2nd-Header-Lines", Parameter::Second_Header_Lines},
        {"Data-Header-Lines", Parameter::Data_Header_Lines},
        {"Lines-Between-Frames", Parameter::Lines_Between_Frames},
        {"Pattern", Parameter::Pattern},
        {"Pattern-Select-Data", Parameter::Pattern_Select_Data},
        {"Header-Number-Lines", Parameter::Header_Number_Lines},
        {"Flexible-Header", Parameter::Flexible_Header},
        {"Flexible-Stop", Parameter::Flexible_Stop},
        {"Remove-None-Printable-Symbols", Parameter::Remove_None_Printable_Symbols},
        {"Image-Data", Parameter::Image_Data},
        {"HDF-Data", Parameter::HDF_Data},
        {"YAML-Data", Parameter::YAML_Data},
        {"HDF-detector-entry", Parameter::HDF_detector_entry},
        {"HDF-data-structure", Parameter::HDF_data_structure},
        {"YAML-detector-entry", Parameter::YAML_detector_entry},
        {"XML-base", Parameter::XML_base},
        {"Selector-Read-from-Header", Parameter::Selector_Read_from_Header},
        {"Selector-P1", Parameter::Selector_P1},
        {"Selector-P2", Parameter::Selector_P2},
        {"Detector-Data-Dimension", Parameter::Detector_Data_Dimension},
        {"Detector-Data-Focus", Parameter::Detector_Data_Focus},
        {"Detector-Binning", Parameter::Detector_Binning},
        {"Detector-Pixel-Size", Parameter::Detector_Pixel_Size},
        {"Detector-Pixel-Size-Asymetry", Parameter::Detector_Pixel_Size_Asymetry},
        {"Detector-Data-Numbers-Per-Line", Parameter::Detector_Data_Numbers_Per_Line},
        {"Detector-Data-Tof-Numbers-Per-Line", Parameter::Detector_Data_Tof_Numbers_Per_Line},
        {"Detector-Data-Transpose", Parameter::Detector_Data_Transpose},
        {"Detector-X-to-Minus-X", Parameter::Detector_X_to_Minus_X},
        {"Detector-Y-to-Minus-Y", Parameter::Detector_Y_to_Minus_Y},
        {"Detector-Dead-Time", Parameter::Detector_Dead_Time},
        {"Detector-Dead-Time-DB", Parameter::Detector_Dead_Time_DB},
        {"Monitor1-Dead-Time", Parameter::Monitor1_Dead_Time},
        {"Monitor2-Dead-Time", Parameter::Monitor2_Dead_Time},
        {"Monitor3-Dead-Time", Parameter::Monitor3_Dead_Time},
        {"Options-2D-DeadTimeModel-NonPara", Parameter::Options_2D_DeadTimeModel_NonPara},
        {"Options-2D-CenterMethod", Parameter::Options_2D_CenterMethod},
        {"DetRotation-X-Read-from-Header", Parameter::DetRotation_X_Read_from_Header},
        {"DetRotation-Angle-X", Parameter::DetRotation_Angle_X},
        {"DetRotation-Invert-Angle-X", Parameter::DetRotation_Invert_Angle_X},
        {"DetRotation-Y-Read-from-Header", Parameter::DetRotation_Y_Read_from_Header},
        {"DetRotation-Angle-Y", Parameter::DetRotation_Angle_Y},
        {"DetRotation-Invert-Angle-Y", Parameter::DetRotation_Invert_Angle_Y},
        {"Calibrant-Type", Parameter::Calibrant_Type},
        {"Calibrant", Parameter::Calibrant},
        {"Use-Active-Mask-and-Sensitivity-Matrixes", Parameter::Use_Active_Mask_and_Sensitivity_Matrixes},
        {"Calculate-Calibrant-Transmission-by-Equation", Parameter::Calculate_Calibrant_Transmission_by_Equation},
        {"Mask-Edge-Shape", Parameter::Mask_Edge_Shape},
        {"Mask-BeamStop-Shape", Parameter::Mask_BeamStop_Shape},
        {"Mask-Edge", Parameter::Mask_Edge},
        {"Mask-BeamStop", Parameter::Mask_BeamStop},
        {"Mask-Edge-Left-X", Parameter::Mask_Edge_Left_X},
        {"Mask-Edge-Left-Y", Parameter::Mask_Edge_Left_Y},
        {"Mask-Edge-Right-X", Parameter::Mask_Edge_Right_X},
        {"Mask-Edge-Right-Y", Parameter::Mask_Edge_Right_Y},
        {"Mask-BeamStop-Left-X", Parameter::Mask_BeamStop_Left_X},
        {"Mask-BeamStop-Left-Y", Parameter::Mask_BeamStop_Left_Y},
        {"Mask-BeamStop-Right-X", Parameter::Mask_BeamStop_Right_X},
        {"Mask-BeamStop-Right-Y", Parameter::Mask_BeamStop_Right_Y},
        {"Mask-Dead-Rows", Parameter::Mask_Dead_Rows},
        {"Mask-Dead-Cols", Parameter::Mask_Dead_Cols},
        {"Mask-Triangular", Parameter::Mask_Triangular},
        {"Sensitivity-SpinBoxErrRightLimit", Parameter::Sensitivity_SpinBoxErrRightLimit},
        {"Sensitivity-SpinBoxErrLeftLimit", Parameter::Sensitivity_SpinBoxErrLeftLimit},
        {"Sensitivity-CheckBoxSensError", Parameter::Sensitivity_CheckBoxSensError},
        {"Sensitivity-Tr-Option", Parameter::Sensitivity_Tr_Option},
        {"Sensitivity-in-Use", Parameter::Sensitivity_in_Use},
        {"Sensitivity-Masked-Pixels-Value", Parameter::Sensitivity_Masked_Pixels_Value},
        {"Transmission-Method", Parameter::Transmission_Method},
        {"Options-2D-HighQ", Parameter::Options_2D_HighQ},
        {"Options-2D-HighQ-Parallax-Type", Parameter::Options_2D_HighQ_Parallax_Type},
        {"Options-2D-HighQ-Tr", Parameter::Options_2D_HighQ_Tr},
        {"Options-2D-Polar-Resolusion", Parameter::Options_2D_Polar_Resolusion},
        {"Options-2D-Mask-Negative-Points", Parameter::Options_2D_Mask_Negative_Points},
        {"Options-2D-Normalization-Type", Parameter::Options_2D_Normalization_Type},
        {"Options-2D-Normalization-Factor", Parameter::Options_2D_Normalization_Factor},
        {"Options-2D-Mask-Normalization-BC", Parameter::Options_2D_Mask_Normalization_BC},
        {"Options-2D-xyDimension-Pixel", Parameter::Options_2D_xyDimension_Pixel},
        {"Options-2D-Output-Format", Parameter::Options_2D_Output_Format},
        {"Options-2D-Header-Output-Format", Parameter::Options_2D_Header_Output_Format},
        {"Options-2D-Header-SASVIEW", Parameter::Options_2D_Header_SASVIEW},
        {"Options-1D-RADmethod-HF", Parameter::Options_1D_RADmethod_HF},
        {"Options-1D-RAD-LinearFactor", Parameter::Options_1D_RAD_LinearFactor},
        {"Options-1D-RAD-ProgressiveFactor", Parameter::Options_1D_RAD_ProgressiveFactor},
        {"Options-1D-RemoveFirst", Parameter::Options_1D_RemoveFirst},
        {"Options-1D-RemoveLast", Parameter::Options_1D_RemoveLast},
        {"Options-1D-RemoveNegativePoints", Parameter::Options_1D_RemoveNegativePoints},
        {"Options-1D-QxQy-From", Parameter::Options_1D_QxQy_From},
        {"Options-1D-QxQy-To", Parameter::Options_1D_QxQy_To},
        {"Options-1D-QxQy-BS", Parameter::Options_1D_QxQy_BS},
        {"Options-1D-OutputFormat", Parameter::Options_1D_OutputFormat},
        {"Options-1D-OutputFormat-PlusHeader", Parameter::Options_1D_OutputFormat_PlusHeader},
        {"Options-1D-Anisotropy", Parameter::Options_1D_Anisotropy},
        {"Options-1D-AnisotropyAngle", Parameter::Options_1D_AnisotropyAngle},
        {"Options-1D-QI-Presentation", Parameter::Options_1D_QI_Presentation},
        {"Sample-Position-As-Condition", Parameter::Sample_Position_As_Condition},
        {"Attenuator-as-Condition", Parameter::Attenuator_as_Condition},
        {"Beam-Center-as-Condition", Parameter::Beam_Center_as_Condition},
        {"Polarization-as-Condition", Parameter::Polarization_as_Condition},
        {"DetectorAngle-as-Condition", Parameter::DetectorAngle_as_Condition},
        {"Reread-Existing-Runs", Parameter::Reread_Existing_Runs},
        {"Find-Center-For-EveryFile", Parameter::Find_Center_For_EveryFile},
        {"Tr-Force-Copy-Paste", Parameter::Tr_Force_Copy_Paste},
        {"Sampe-Name-As-RunTableName", Parameter::Sampe_Name_As_RunTableName},
        {"Generate-MergingTable", Parameter::Generate_MergingTable},
        {"Auto-Merging", Parameter::Auto_Merging},
        {"Overlap-Merging", Parameter::Overlap_Merging},
        {"Rewrite-Output", Parameter::Rewrite_Output},
        {"Skipt-Tr-Configurations", Parameter::Skipt_Tr_Configurations},
        {"Skipt-Output-Folders", Parameter::Skipt_Output_Folders},
        {"Resolusion-Focusing", Parameter::Resolusion_Focusing},
        {"Resolusion-Detector", Parameter::Resolusion_Detector},
        {"Resolusion-CA-Round", Parameter::Resolusion_CA_Round},
        {"Resolusion-SA-Round", Parameter::Resolusion_SA_Round},
        {"File-Ext", Parameter::File_Ext},
        {"POL-ALIAS-UP", Parameter::POL_ALIAS_UP},
        {"POL-ALIAS-DOWN", Parameter::POL_ALIAS_DOWN},
        {"POL-ALIAS-UP-UP", Parameter::POL_ALIAS_UP_UP},
        {"POL-ALIAS-UP-DOWN", Parameter::POL_ALIAS_UP_DOWN},
        {"POL-ALIAS-DOWN-DOWN", Parameter::POL_ALIAS_DOWN_DOWN},
        {"POL-ALIAS-DOWN-UP", Parameter::POL_ALIAS_DOWN_UP},
        {"POLARIZATION", Parameter::POLARIZATION},
        {"POL-TRANSMISSION", Parameter::POL_TRANSMISSION},
        {"POL-FLIPPER-EFFICIENCY", Parameter::POL_FLIPPER_EFFICIENCY},
        {"ANALYZER-TRANSMISSION", Parameter::ANALYZER_TRANSMISSION},
        {"ANALYZER-EFFICIENCY", Parameter::ANALYZER_EFFICIENCY}};

    for (int i = 0; i < lst.count(); i++)
    {
        //++++++++++++++++++++++
        //+++ header :: map                        +
        //++++++++++++++++++++++
        for (int j = 0; j < parserHeader->listOfHeaders.count(); j++)
        {
            if (lst[i].contains(parserHeader->listOfHeaders[j]))
            {
                lst[i] = lst[i].remove(parserHeader->listOfHeaders[j]).simplified();
                int pos = lst[i].indexOf(";;;");
                tableHeaderPosNew->item(j, 0)->setText(lst[i].left(pos));
                tableHeaderPosNew->item(j, 1)->setText(lst[i].right(lst[i].length() - 3 - pos));
                break;
            }
        }

        stringList = lst[i].split("] ");
        stringList[0].remove('[');
        if (stringList.size() != 1)
            line = stringList[1].simplified();
        else
            line = " ";

        Parameter param = stringToEnumMap[stringList[0]];

        switch (param)
        {
        case Parameter::Instrument:
            break;
        case Parameter::Instrument_Mode:
            comboBoxMode->setCurrentIndex(comboBoxMode->findText(line));
            break;
        case Parameter::DataFormat:
            parserHeader->dataFormatChanged(line.toInt());
            break;
        case Parameter::Color:
            pushButtonInstrColor->setStyleSheet("background-color: " + line + ";");
            pushButtonInstrLabel->setStyleSheet("background-color: " + line + ";");
            break;
        case Parameter::Input_Folder:
            if (line.left(4) != "home")
                lineEditPathDAT->setText(line);
            break;
        case Parameter::Include_Sub_Foldes:
            if (line.contains("Yes"))
                checkBoxDirsIndir->setChecked(true);
            else
                checkBoxDirsIndir->setChecked(false);
            break;
        case Parameter::Output_Folder:
            if (line.left(4) != "home")
                lineEditPathRAD->setText(line);
            break;
        case Parameter::Units_Lambda:
            comboBoxUnitsLambda->setCurrentIndex(line.toInt());
            break;
        case Parameter::Units_Appertures:
            comboBoxUnitsBlends->setCurrentIndex(line.toInt());
            break;
        case Parameter::Units_Thickness:
            comboBoxThicknessUnits->setCurrentIndex(line.toInt());
            break;
        case Parameter::Units_Time:
            comboBoxUnitsTime->setCurrentIndex(line.toInt());
            break;
        case Parameter::Units_Time_RT:
            comboBoxUnitsTimeRT->setCurrentIndex(line.toInt());
            break;
        case Parameter::Units_C:
            comboBoxUnitsC->setCurrentIndex(line.toInt());
            break;
        case Parameter::Units_C_D_Offset:
            comboBoxUnitsD->setCurrentIndex(line.toInt());
            break;
        case Parameter::Units_Selector:
            comboBoxUnitsSelector->setCurrentIndex(line.toInt());
            break;
        case Parameter::Second_Header_OK:
            if (line.contains("Yes"))
                checkBoxYes2ndHeader->setChecked(true);
            else
                checkBoxYes2ndHeader->setChecked(false);
            break;
        case Parameter::Second_Header_Pattern:
            lineEditWildCard2ndHeader->setText(line);
            break;
        case Parameter::Second_Header_Lines:
            spinBoxHeaderNumberLines2ndHeader->setValue(line.toInt());
            break;
        case Parameter::Data_Header_Lines:
            spinBoxDataHeaderNumberLines->setValue(line.toInt());
            break;
        case Parameter::Lines_Between_Frames:
            spinBoxDataLinesBetweenFrames->setValue(line.toInt());
            break;
        case Parameter::Pattern:
            lineEditWildCard->setText(line);
            textEditPattern->setText(line.replace("#", "*").replace("**", "*"));
            break;
        case Parameter::Pattern_Select_Data:
            selectPattern = line;
            break;
        case Parameter::Header_Number_Lines:
            spinBoxHeaderNumberLines->setValue(line.toInt());
            break;
        case Parameter::Flexible_Header:
            if (line.contains("Yes"))
                checkBoxHeaderFlexibility->setChecked(true);
            else
                checkBoxHeaderFlexibility->setChecked(false);
            break;
        case Parameter::Flexible_Stop:
            lineEditFlexiStop->setText(line);
            break;
        case Parameter::Remove_None_Printable_Symbols:
            if (line.contains("Yes"))
                checkBoxRemoveNonePrint->setChecked(true);
            else
                checkBoxRemoveNonePrint->setChecked(false);
            break;
        case Parameter::Image_Data:
            if (line.contains("Yes"))
            {
                radioButtonDetectorFormatImage->setChecked(true);
                radioButtonDetectorFormatHDF->setChecked(false);
                radioButtonDetectorFormatYAML->setChecked(false);
                radioButtonDetectorFormatAscii->setChecked(false);
            }
            break;
        case Parameter::HDF_Data:
            if (line.contains("Yes"))
            {
                radioButtonDetectorFormatHDF->setChecked(true);
                radioButtonDetectorFormatYAML->setChecked(false);
                radioButtonDetectorFormatImage->setChecked(false);
                radioButtonDetectorFormatAscii->setChecked(false);
            }
            break;
        case Parameter::YAML_Data:
            if (line.contains("Yes"))
            {
                radioButtonDetectorFormatYAML->setChecked(true);
                radioButtonDetectorFormatHDF->setChecked(false);
                radioButtonDetectorFormatImage->setChecked(false);
                radioButtonDetectorFormatAscii->setChecked(false);
            }
            break;
        case Parameter::HDF_detector_entry:
            lineEditHdfDetectorEntry->setText(line);
            break;
        case Parameter::HDF_data_structure:
            comboBoxDxDyN->setCurrentIndex(line.toInt());
            break;
        case Parameter::YAML_detector_entry:
            lineEditYamlDetectorEntry->setText(line);
            break;
        case Parameter::XML_base:
            lineEditXMLbase->setText(line);
            break;
        case Parameter::Selector_Read_from_Header:
            if (line.contains("Yes"))
            {
                radioButtonLambdaHeader->setChecked(true);
                radioButtonLambdaF->setChecked(false);
            }
            else
            {
                radioButtonLambdaHeader->setChecked(false);
                radioButtonLambdaF->setChecked(true);
            }
            break;
        case Parameter::Selector_P1:
            lineEditSel1->setText(line);
            break;
        case Parameter::Selector_P2:
            lineEditSel2->setText(line);
            break;
        case Parameter::Detector_Data_Dimension:
            DD = line.toInt();
            break;
        case Parameter::Detector_Data_Focus:
            RoI = line.toInt();
            break;
        case Parameter::Detector_Binning:
            binning = line.toInt();
            md = RoI / comboBoxBinning->itemText(binning).toInt();
            break;
        case Parameter::Detector_Pixel_Size:
            lineEditResoPixelSize->setText(line);
            break;
        case Parameter::Detector_Pixel_Size_Asymetry:
            lineEditAsymetry->setText(line);
            break;
        case Parameter::Detector_Data_Numbers_Per_Line:
            spinBoxReadMatrixNumberPerLine->setValue(line.toInt());
            break;
        case Parameter::Detector_Data_Tof_Numbers_Per_Line:
            spinBoxReadMatrixTofNumberPerLine->setValue(line.toInt());
            break;
        case Parameter::Detector_Data_Transpose:
            if (line.contains("Yes"))
                checkBoxTranspose->setChecked(true);
            else
                checkBoxTranspose->setChecked(false);
            break;
        case Parameter::Detector_X_to_Minus_X:
            if (line.contains("Yes"))
                checkBoxMatrixX2mX->setChecked(true);
            else
                checkBoxMatrixX2mX->setChecked(false);
            break;
        case Parameter::Detector_Y_to_Minus_Y:
            if (line.contains("Yes"))
                checkBoxMatrixY2mY->setChecked(true);
            else
                checkBoxMatrixY2mY->setChecked(false);
            break;
        case Parameter::Detector_Dead_Time:
            lineEditDeadTime->setText(line);
            break;
        case Parameter::Detector_Dead_Time_DB:
            lineEditDBdeadtime->setText(line);
            break;
        case Parameter::Monitor1_Dead_Time:
            lineEditDeadTimeM1->setText(line);
            break;
        case Parameter::Monitor2_Dead_Time:
            lineEditDeadTimeM2->setText(line);
            break;
        case Parameter::Monitor3_Dead_Time:
            lineEditDeadTimeM3->setText(line);
            break;
        case Parameter::Options_2D_DeadTimeModel_NonPara:
            if (line.contains("Yes"))
            {
                radioButtonDeadTimeCh->setChecked(true);
                radioButtonDeadTimeDet->setChecked(false);
            }
            else
            {
                radioButtonDeadTimeCh->setChecked(false);
                radioButtonDeadTimeDet->setChecked(true);
            }
            break;
        case Parameter::Options_2D_CenterMethod:
            if (line.contains("HF"))
            {
                radioButtonCenterHF->setChecked(true);
                radioButtonRadStdSymm->setChecked(false);
                radioButtonCenterReadFromHeader->setChecked(false);
            }
            else if (line.contains("SYM"))
            {
                radioButtonCenterHF->setChecked(false);
                radioButtonRadStdSymm->setChecked(true);
                radioButtonCenterReadFromHeader->setChecked(false);
            }
            else
            {
                radioButtonCenterHF->setChecked(false);
                radioButtonRadStdSymm->setChecked(false);
                radioButtonCenterReadFromHeader->setChecked(true);
            }
            break;
        case Parameter::DetRotation_X_Read_from_Header:
            if (line.contains("Yes"))
            {
                radioButtonDetRotHeaderX->setChecked(true);
                radioButtonDetRotConstX->setChecked(false);
            }
            else
            {
                radioButtonDetRotHeaderX->setChecked(false);
                radioButtonDetRotConstX->setChecked(true);
            }
            break;
        case Parameter::DetRotation_Angle_X:
            doubleSpinBoxDetRotX->setValue(line.toDouble());
            break;
        case Parameter::DetRotation_Invert_Angle_X:
            if (line.contains("Yes"))
            {
                checkBoxInvDetRotX->setChecked(true);
            }
            else
            {
                checkBoxInvDetRotX->setChecked(false);
            }
            break;
        case Parameter::DetRotation_Y_Read_from_Header:
            if (line.contains("Yes"))
            {
                radioButtonDetRotHeaderY->setChecked(true);
                radioButtonDetRotConstY->setChecked(false);
            }
            else
            {
                radioButtonDetRotHeaderY->setChecked(false);
                radioButtonDetRotConstY->setChecked(true);
            }
            break;
        case Parameter::DetRotation_Angle_Y:
            doubleSpinBoxDetRotY->setValue(line.toDouble());
            break;
        case Parameter::DetRotation_Invert_Angle_Y:
            if (line.contains("Yes"))
            {
                checkBoxInvDetRotY->setChecked(true);
            }
            else
            {
                checkBoxInvDetRotY->setChecked(false);
            }
            break;
        case Parameter::Calibrant_Type:
            if (line.contains("Direct Beam"))
                comboBoxACmethod->setCurrentIndex(1);
            else if (line.contains("Flat Scatter + Transmission"))
                comboBoxACmethod->setCurrentIndex(2);
            else if (line.contains("Counts per Channel"))
                comboBoxACmethod->setCurrentIndex(3);
            else
            {
                comboBoxACmethod->setCurrentIndex(0);
            }
            break;
        case Parameter::Calibrant: {
            QStringList lst;
            for (int i = 0; i < comboBoxCalibrant->count(); i++)
                lst << comboBoxCalibrant->itemText(i);
            if (lst.contains(line))
                comboBoxCalibrant->setCurrentIndex(lst.indexOf(line));
            calibrantselected();
        }
        break;
        case Parameter::Use_Active_Mask_and_Sensitivity_Matrixes:
            if (line.contains("Yes"))
                checkBoxACDBuseActive->setChecked(true);
            else
                checkBoxACDBuseActive->setChecked(false);
            break;
        case Parameter::Calculate_Calibrant_Transmission_by_Equation:
            if (line.contains("Yes"))
                checkBoxTransmissionPlexi->setChecked(true);
            else
                checkBoxTransmissionPlexi->setChecked(false);
            break;
        case Parameter::Mask_Edge_Shape:
            if (line.contains("Rectangle"))
                comboBoxMaskEdgeShape->setCurrentIndex(0);
            else
                comboBoxMaskEdgeShape->setCurrentIndex(1);
            break;
        case Parameter::Mask_BeamStop_Shape:
            if (line.contains("Rectangle"))
                comboBoxMaskBeamstopShape->setCurrentIndex(0);
            else
                comboBoxMaskBeamstopShape->setCurrentIndex(1);
            break;
        case Parameter::Mask_Edge:
            if (line.contains("Yes"))
                groupBoxMask->setChecked(true);
            else
                groupBoxMask->setChecked(false);
            break;
        case Parameter::Mask_BeamStop:
            if (line.contains("Yes"))
                groupBoxMaskBS->setChecked(true);
            else
                groupBoxMaskBS->setChecked(false);
            break;
        case Parameter::Mask_Edge_Left_X:
            spinBoxLTx->setMaximum(md);
            spinBoxLTx->setValue(line.toInt());
            break;
        case Parameter::Mask_Edge_Left_Y:
            spinBoxLTy->setMaximum(md);
            spinBoxLTy->setValue(line.toInt());
            break;
        case Parameter::Mask_Edge_Right_X:
            spinBoxRBx->setMaximum(md + 51);
            spinBoxRBx->setValue(line.toInt());
            break;
        case Parameter::Mask_Edge_Right_Y:
            spinBoxRBy->setMaximum(md + 51);
            spinBoxRBy->setValue(line.toInt());
            break;
        case Parameter::Mask_BeamStop_Left_X:
            spinBoxLTxBS->setMaximum(md);
            spinBoxLTxBS->setValue(line.toInt());
            break;
        case Parameter::Mask_BeamStop_Left_Y:
            spinBoxLTyBS->setMaximum(md);
            spinBoxLTyBS->setValue(line.toInt());
            break;
        case Parameter::Mask_BeamStop_Right_X:
            spinBoxRBxBS->setMaximum(md);
            spinBoxRBxBS->setValue(line.toInt());
            break;
        case Parameter::Mask_BeamStop_Right_Y:
            spinBoxRByBS->setMaximum(md);
            spinBoxRByBS->setValue(line.toInt());
            break;
        case Parameter::Mask_Dead_Rows:
            lineEditDeadRows->setText(line);
            break;
        case Parameter::Mask_Dead_Cols:
            lineEditDeadCols->setText(line);
            break;
        case Parameter::Mask_Triangular:
            lineEditMaskPolygons->setText(line);
            break;
        case Parameter::Sensitivity_SpinBoxErrRightLimit:
            spinBoxErrRightLimit->setValue(line.toDouble());
            break;
        case Parameter::Sensitivity_SpinBoxErrLeftLimit:
            spinBoxErrLeftLimit->setValue(line.toDouble());
            break;
        case Parameter::Sensitivity_CheckBoxSensError:
            if (line.contains("Yes")) 
                checkBoxSensError->setChecked(true);
            else 
                checkBoxSensError->setChecked(false);
            break;
        case Parameter::Sensitivity_Tr_Option:
            if (line.contains("Yes")) 
                checkBoxSensTr->setChecked(true);
            else 
                checkBoxSensTr->setChecked(false);
            break;
        case Parameter::Sensitivity_in_Use:
            if (line.contains("Yes")) 
                buttonGroupSensanyD->setChecked(true);
            else 
                buttonGroupSensanyD->setChecked(false);
            break;
        case Parameter::Sensitivity_Masked_Pixels_Value:
            lineEditSensMaskedPixels->setText(line);
            break;
        case Parameter::Transmission_Method:
            if (line.contains("9.5A: ROI in Header;"))
                comboBoxTransmMethod->setCurrentIndex(4);
            else if (line.contains("Direct Beam"))
                comboBoxTransmMethod->setCurrentIndex(1);
            else if (line.contains("Monitor-3"))
                comboBoxTransmMethod->setCurrentIndex(0);
            else if (line.contains("Tr in Header"))
                comboBoxTransmMethod->setCurrentIndex(2);
            else
                comboBoxTransmMethod->setCurrentIndex(3);
            break;
        case Parameter::Options_2D_HighQ:
            if (line.contains("Yes")) 
                checkBoxParallax->setChecked(true);
            else 
                checkBoxParallax->setChecked(false);
            break;
        case Parameter::Options_2D_HighQ_Parallax_Type:
            comboBoxParallax->setCurrentIndex(line.toInt());
            break;
        case Parameter::Options_2D_HighQ_Tr:
            if (line.contains("Yes"))
                checkBoxParallaxTr->setChecked(true);
            else
                checkBoxParallaxTr->setChecked(false);
            break;
        case Parameter::Options_2D_Polar_Resolusion:
            spinBoxPolar->setValue(line.toInt());
            break;
        case Parameter::Options_2D_Mask_Negative_Points:
            if (line.contains("Yes")) 
                checkBoxMaskNegative->setChecked(true);
            else 
                checkBoxMaskNegative->setChecked(false);
            break;
        case Parameter::Options_2D_Normalization_Type:
            comboBoxNorm->setCurrentIndex(line.toInt());
            break;
        case Parameter::Options_2D_Normalization_Factor:
            spinBoxNorm->setValue(line.toInt());
            break;
        case Parameter::Options_2D_Mask_Normalization_BC:
            if (line.contains("Yes")) 
                checkBoxBCTimeNormalization->setChecked(true);
            else 
                checkBoxBCTimeNormalization->setChecked(false);
            break;
        case Parameter::Options_2D_xyDimension_Pixel:
            if (line.contains("Yes"))
            {
                radioButtonXYdimPixel->setChecked(true);
                radioButtonXYdimQ->setChecked(false);		
            }
            else
            {
                radioButtonXYdimQ->setChecked(true);		
                radioButtonXYdimPixel->setChecked(false);
            }
            break;
        case Parameter::Options_2D_Output_Format:
            comboBoxIxyFormat->setCurrentIndex(line.toInt());
            break;
        case Parameter::Options_2D_Header_Output_Format:
            if (line.contains("Yes")) 
                checkBoxASCIIheaderIxy->setChecked(true);
            else 
                checkBoxASCIIheaderIxy->setChecked(false);
            break;
        case Parameter::Options_2D_Header_SASVIEW:
            if (line.contains("Yes"))
                checkBoxASCIIheaderSASVIEW->setChecked(true);
            else
                checkBoxASCIIheaderSASVIEW->setChecked(false);
            break;
        case Parameter::Options_1D_RADmethod_HF:
            if (line.contains("Yes"))
            {
                radioButtonRadHF->setChecked(true);
                radioButtonRadStd->setChecked(false);
            }
            else
            {
                radioButtonRadHF->setChecked(false);
                radioButtonRadStd->setChecked(true);
            }
            break;
        case Parameter::Options_1D_RAD_LinearFactor:
            spinBoxAvlinear->setValue(line.toInt());
            break;
        case Parameter::Options_1D_RAD_ProgressiveFactor:
            doubleSpinBoxAvLog->setValue(line.toDouble());
            break;
        case Parameter::Options_1D_RemoveFirst:
            spinBoxRemoveFirst->setValue(line.toInt());
            break;
        case Parameter::Options_1D_RemoveLast:
            spinBoxRemoveLast->setValue(line.toInt());
            break;
        case Parameter::Options_1D_RemoveNegativePoints:
            if (line.contains("Yes"))
            {
                checkBoxMaskNegativeQ->setChecked(true);
            }
            else
            {
                checkBoxMaskNegativeQ->setChecked(false);
            }
            break;
        case Parameter::Options_1D_QxQy_From:
            spinBoxFrom->setMaximum(md);
            spinBoxFrom->setValue(line.toInt());
            break;
        case Parameter::Options_1D_QxQy_To:
            spinBoxTo->setMaximum(md);
            spinBoxTo->setValue(line.toInt());
            break;
        case Parameter::Options_1D_QxQy_BS:
            if (line.contains("Yes"))
            {
                checkBoxSlicesBS->setChecked(true);
            }
            else
            {
                checkBoxSlicesBS->setChecked(false);
            }
            break;
        case Parameter::Options_1D_OutputFormat:
            comboBox4thCol->setCurrentIndex(line.toInt());
            break;
        case Parameter::Options_1D_OutputFormat_PlusHeader:
            if (line.contains("Yes"))
                checkBoxASCIIheader->setChecked(true);
            else
                checkBoxASCIIheader->setChecked(false);
            break;
        case Parameter::Options_1D_Anisotropy:
            if (line.contains("Yes"))
                checkBoxAnisotropy->setChecked(true);
            else
                checkBoxAnisotropy->setChecked(false);
            break;
        case Parameter::Options_1D_AnisotropyAngle:
            spinBoxAnisotropyOffset->setValue(line.toInt());
            break;
        case Parameter::Options_1D_QI_Presentation:
            comboBoxSelectPresentation->setCurrentIndex(line.toInt());
            break;
        case Parameter::Sample_Position_As_Condition:
            if (line.contains("Yes")) 
                checkBoxRecalculateUseNumber->setChecked(true);
            else 
                checkBoxRecalculateUseNumber->setChecked(false);
            break;
        case Parameter::Attenuator_as_Condition:
            if (line.contains("Yes")) 
                checkBoxAttenuatorAsPara->setChecked(true);
            else 
                checkBoxAttenuatorAsPara->setChecked(false);
            break;
        case Parameter::Beam_Center_as_Condition:
            if (line.contains("Yes")) 
                checkBoxBeamcenterAsPara->setChecked(true);
            else 
                checkBoxBeamcenterAsPara->setChecked(false);
            break;
        case Parameter::Polarization_as_Condition:
            if (line.contains("Yes")) 
                checkBoxPolarizationAsPara->setChecked(true);
            else 
                checkBoxPolarizationAsPara->setChecked(false);
            break;
        case Parameter::DetectorAngle_as_Condition:
            if (line.contains("Yes"))
                checkBoxDetRotAsPara->setChecked(true);
            else
                checkBoxDetRotAsPara->setChecked(false);
            break;
        case Parameter::Reread_Existing_Runs:
            if (line.contains("Yes")) 
                checkBoxRecalculate->setChecked(true);
            else 
                checkBoxRecalculate->setChecked(false);
            break;
        case Parameter::Find_Center_For_EveryFile:
            if (line.contains("Yes")) 
                checkBoxFindCenter->setChecked(true);
            else 
                checkBoxFindCenter->setChecked(false);
            break;
        case Parameter::Tr_Force_Copy_Paste:
            if (line.contains("Yes")) 
                checkBoxForceCopyPaste->setChecked(true);
            else 
                checkBoxForceCopyPaste->setChecked(false);
            break;
        case Parameter::Sampe_Name_As_RunTableName:
            if (line.contains("Yes")) 
                checkBoxNameAsTableName->setChecked(true);
            else 
                checkBoxNameAsTableName->setChecked(false);
            break;
        case Parameter::Generate_MergingTable:
            if (line.contains("Yes")) 
                checkBoxMergingTable->setChecked(true);
            else 
                checkBoxMergingTable->setChecked(false);
            break;
        case Parameter::Auto_Merging:
            if (line.contains("Yes"))
                checkBoxAutoMerging->setChecked(true);
            else
                checkBoxAutoMerging->setChecked(false);
            break;
        case Parameter::Overlap_Merging:
            spinBoxOverlap->setValue(line.toInt());
            break;
        case Parameter::Rewrite_Output:
            if (line.contains("Yes")) 
                checkBoxRewriteOutput->setChecked(true);
            else 
                checkBoxRewriteOutput->setChecked(false);
            break;
        case Parameter::Skipt_Tr_Configurations:
            if (line.contains("Yes")) 
                checkBoxSkiptransmisionConfigurations->setChecked(true);
            else 
                checkBoxSkiptransmisionConfigurations->setChecked(false);
            break;
        case Parameter::Skipt_Output_Folders:
            if (line.contains("Yes")) 
                checkBoxSortOutputToFolders->setChecked(true);
            else 
                checkBoxSortOutputToFolders->setChecked(false);
            break;
        case Parameter::Resolusion_Focusing:
            if (line.contains("Yes")) 
                checkBoxResoFocus->setChecked(true);
            else 
                checkBoxResoFocus->setChecked(false);
            break;
        case Parameter::Resolusion_Detector:
            lineEditDetReso->setText(line);
            break;
        case Parameter::Resolusion_CA_Round:
            if (line.contains("Yes")) 
                checkBoxResoCAround->setChecked(true);
            else 
                checkBoxResoCAround->setChecked(false);
            break;
        case Parameter::Resolusion_SA_Round:
            if (line.contains("Yes")) 
                checkBoxResoSAround->setChecked(true);
            else 
                checkBoxResoSAround->setChecked(false);
            break;
        case Parameter::File_Ext:
            lineEditFileExt->setText(line);
            break;
        case Parameter::POL_ALIAS_UP:
            lineEditUp->setText(line);
            break;
        case Parameter::POL_ALIAS_DOWN:
            lineEditDown->setText(line);
            break;
        case Parameter::POL_ALIAS_UP_UP:
            lineEditUpUp->setText(line);
            break;
        case Parameter::POL_ALIAS_UP_DOWN:
            lineEditUpDown->setText(line);
            break;
        case Parameter::POL_ALIAS_DOWN_DOWN:
            lineEditDownDown->setText(line);
            break;
        case Parameter::POL_ALIAS_DOWN_UP:
            lineEditDownUp->setText(line);
            break;
        case Parameter::POLARIZATION:
            polarizationSelector->readSettingsString(line);
            break;
        case Parameter::POL_TRANSMISSION:
            polTransmissionSelector->readSettingsString(line);
            break;
        case Parameter::POL_FLIPPER_EFFICIENCY:
            polFlipperEfficiencySelector->readSettingsString(line);
            break;
        case Parameter::ANALYZER_TRANSMISSION:
            analyzerTransmissionSelector->readSettingsString(line);
            break;
        case Parameter::ANALYZER_EFFICIENCY:
            analyzerEfficiencySelector->readSettingsString(line);
            break;
        }
    }
    
    //+++ 2020-07
    doubleSpinBoxXcenter->setValue((spinBoxLTxBS->value()+spinBoxRBxBS->value())/2.0);
    doubleSpinBoxYcenter->setValue((spinBoxLTyBS->value()+spinBoxRByBS->value())/2.0);
    //---
    
    //+++ select data pattern
    if (selectPattern!="") textEditPattern->setText(selectPattern);
    
    
    disconnect( lineEditMD, SIGNAL( textChanged(const QString&) ), this, SLOT( MDchanged() ) );
    disconnect( spinBoxRegionOfInteres, SIGNAL( valueChanged(int) ), this, SLOT( dataRangeOfInteresChanged(int) ) );
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    disconnect( comboBoxMDdata, SIGNAL( activated(const QString&) ), this, SLOT( dataDimensionChanged(const QString&) ) );      
    disconnect( comboBoxBinning, SIGNAL( activated(const QString&) ), this, SLOT( binningChanged(const QString&) ) );      
#else
    disconnect(comboBoxMDdata, &QComboBox::textActivated, this, &dan18::dataDimensionChanged);
    disconnect(comboBoxBinning, &QComboBox::textActivated, this, &dan18::binningChanged);
#endif
    
    comboBoxMDdata->setCurrentIndex(DD); dataDimensionChanged(comboBoxMDdata->currentText());
    spinBoxRegionOfInteres->setValue(RoI); dataRangeOfInteresChanged(spinBoxRegionOfInteres->value());
    comboBoxBinning->setCurrentIndex(binning); binningChanged(comboBoxBinning->currentText());
    
    connect( lineEditMD, SIGNAL( textChanged(const QString&) ), this, SLOT( MDchanged() ) );
    connect( spinBoxRegionOfInteres, SIGNAL( valueChanged(int) ), this, SLOT( dataRangeOfInteresChanged(int) ) );
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    connect( comboBoxMDdata, SIGNAL( activated(const QString&) ), this, SLOT( dataDimensionChanged(const QString&) ) );      
    connect( comboBoxBinning, SIGNAL( activated(const QString&) ), this, SLOT( binningChanged(const QString&) ) );      
#else
    connect(comboBoxMDdata, &QComboBox::textActivated, this, &dan18::dataDimensionChanged);
    connect(comboBoxBinning, &QComboBox::textActivated, this, &dan18::binningChanged);
#endif

    pushButtonInstrLabel->setText(comboBoxInstrument->currentText());

    SensitivityLineEditCheck();

    secondHeaderExist(checkBoxYes2ndHeader->isChecked());
    selector->readLambdaFromHeader(radioButtonLambdaHeader->isChecked());

    sasPresentation();

    if (comboBoxInstrument->currentIndex() > 18)
        pushButtonDeleteCurrentInstr->setEnabled(true);
    else
        pushButtonDeleteCurrentInstr->setEnabled(false);

    experimentalModeSelected();
}

void dan18::saveInstrumentAsCpp(QString instrPath, QString instrName  )
{
    
    QDir dd;
    instrPath+="/cpp";
    instrPath=instrPath.replace("//","/");
    
    if (!dd.cd(instrPath)) dd.mkdir( instrPath);
    
    
    QString s;
    s="else if (instrName==\""+instrName+"\")\n{\n";
    
    //+++ instrument
    s+="lst<<\"[Instrument] "+instrName+"\";\n";
    
    //+++ mode
    s+="lst<<\"[Instrument-Mode] "+comboBoxMode->currentText()+"\";\n";
    
    //+++ data format
    s+="lst<<\"[DataFormat] "+QString::number(comboBoxHeaderFormat->currentIndex())+"\";\n";
    
    //+++ color
    s += "lst<<\"[Color] " + pushButtonInstrColor->palette().color(QPalette::Window).name() + "\";\n";
    
    //++++++++++++++++++++++
    //+++ data :: input&output folder    +
    //++++++++++++++++++++++
    s+="lst<<\"[Input-Folder] home\";\n";
    s+="lst<<\"[Output-Folder] home\";\n";
    //+++ sub folders
    if (checkBoxDirsIndir->isChecked())
        s+="lst<<\"[Include-Sub-Foldes] Yes\";\n";
    else
        s+="lst<<\"[Include-Sub-Foldes] No\";\n";
    
    //++++++++++++++++++++++
    //+++  units
    //++++++++++++++++++++++
    s+="lst<<\"[Units-Lambda] "+QString::number(comboBoxUnitsLambda->currentIndex())+"\";\n";
    s+="lst<<\"[Units-Appertures] "+QString::number(comboBoxUnitsBlends->currentIndex())+"\";\n";
    s+="lst<<\"[Units-Thickness] "+QString::number(comboBoxThicknessUnits->currentIndex())+"\";\n";
    s+="lst<<\"[Units-Time] "+QString::number(comboBoxUnitsTime->currentIndex())+"\";\n";
    s+="lst<<\"[Units-Time-RT] "+QString::number(comboBoxUnitsTimeRT->currentIndex())+"\";\n";
    s += "lst<<\"[Units-C] " + QString::number(comboBoxUnitsC->currentIndex()) + "\";\n";
    s += "lst<<\"[Units-C-D-Offset] " + QString::number(comboBoxUnitsD->currentIndex()) + "\";\n";
    s += "lst<<\"[Units-Selector] " + QString::number(comboBoxUnitsSelector->currentIndex()) + "\";\n";

    //++++++++++++++++++++++
    //+++ file(s) :: structure
    //++++++++++++++++++++++
    
    //+++ 2ND header
    if (checkBoxYes2ndHeader->isChecked())
        s+="lst<<\"[2nd-Header-OK] Yes\";\n";
    else
        s+="lst<<\"[2nd-Header-OK] No\";\n";
    s+="lst<<\"[2nd-Header-Pattern] "+lineEditWildCard2ndHeader->text()+"\";\n";
    s+="lst<<\"[2nd-Header-Lines] "+QString::number(spinBoxHeaderNumberLines2ndHeader->value())+"\";\n";
    
    //+++ 1ND header
    s+="lst<<\"[Pattern] "+lineEditWildCard->text()+"\";\n";
    s+="lst<<\"[Pattern-Select-Data] "+textEditPattern->text()+"\";\n";
    s+="lst<<\"[Header-Number-Lines] "+QString::number(spinBoxHeaderNumberLines->value())+"\";\n";
    
    //+++ TOF/RT Headers
    s+="lst<<\"[Data-Header-Lines] "+QString::number(spinBoxDataHeaderNumberLines->value())+"\";\n";
    s+="lst<<\"[Lines-Between-Frames] "+QString::number(spinBoxDataLinesBetweenFrames->value())+"\";\n";
    
    //+++ Flexible-Header
    if (checkBoxHeaderFlexibility->isChecked())
        s+="lst<<\"[Flexible-Header] Yes\";\n";
    else
        s+="lst<<\"[Flexible-Header] No\";\n";
    
    //+++ Flexible-Stop
    s+="lst<<\"[Flexible-Stop] "+lineEditFlexiStop->text()+"\";\n";
    
    //+++ Remove-None-Printable-Symbols
    if (checkBoxRemoveNonePrint->isChecked())
        s+="lst<<\"[Remove-None-Printable-Symbols] Yes\";\n";
    else
        s+="lst<<\"[Remove-None-Printable-Symbols] No\";\n";

    //+++ Image-Data
    if (radioButtonDetectorFormatImage->isChecked())
        s += "lst<<\"[Image-Data] Yes\";\n";
    else
        s += "lst<<\"[Image-Data] No\";\n";

    //+++ HDF-Data
    if (radioButtonDetectorFormatHDF->isChecked())
        s += "lst<<\"[HDF-Data] Yes\";\n";
    else
        s += "lst<<\"[HDF-Data] No\";\n";

    //+++ YAML-Data
    if (radioButtonDetectorFormatYAML->isChecked())
        s += "lst<<\"[YAML-Data] Yes\";\n";
    else
        s += "lst<<\"[YAML-Data] No\";\n";

    //+++ HDF-detector-entry
    s += "lst<<\"[HDF-detector-entry] " + lineEditHdfDetectorEntry->text() + "\";\n";

    //+++ HDF-data-structure
    s += "lst<<\"[HDF-data-structure] " + QString::number(comboBoxDxDyN->currentIndex()) + "\";\n";

    //+++ YAML-detector-entry
    s += "lst<<\"[YAML-detector-entry] " + lineEditYamlDetectorEntry->text() + "\";\n";

    //+++ XML-base
    s += "lst<<\"[XML-base] " + lineEditXMLbase->text() + "\";\n";

    //++++++++++++++++++++++
    //+++ header :: map                        +
    //++++++++++++++++++++++
    for (int i = 0; i < parserHeader->listOfHeaders.count(); i++)
    {
        s += "lst<<\"" + parserHeader->listOfHeaders[i] + " ";
        s += tableHeaderPosNew->item(i, 0)->text() + ";;;";
        s += tableHeaderPosNew->item(i, 1)->text() + "\";\n";
    }
    //++++++++++++++++++++++
    //+++ selector :: wave length           +
    //++++++++++++++++++++++
    if (radioButtonLambdaHeader->isChecked())
        s+="lst<<\"[Selector-Read-from-Header] Yes\";\n";
    else
        s+="lst<<\"[Selector-Read-from-Header] No\";\n";
    
    s+="lst<<\"[Selector-P1] "+lineEditSel1->text()+"\";\n";
    s+="lst<<\"[Selector-P2] "+lineEditSel2->text()+"\";\n";
    
    //++++++++++++++++++++++
    //+++ detector :: image                    +
    //++++++++++++++++++++++
    s+="lst<<\"[Detector-Data-Dimension] "+QString::number(comboBoxMDdata->currentIndex())+"\";\n";
    s+="lst<<\"[Detector-Data-Focus] "+QString::number(spinBoxRegionOfInteres->value())+"\";\n";
    s+="lst<<\"[Detector-Binning] "+QString::number(comboBoxBinning->currentIndex())+"\";\n";
    s+="lst<<\"[Detector-Pixel-Size] "+lineEditResoPixelSize->text()+"\";\n";
    s+="lst<<\"[Detector-Pixel-Size-Asymetry] "+lineEditAsymetry->text()+"\";\n";
    s+="lst<<\"[Detector-Data-Numbers-Per-Line] "
    +QString::number(spinBoxReadMatrixNumberPerLine->value())+"\";\n";
    s+="lst<<\"[Detector-Data-Tof-Numbers-Per-Line] "
    +QString::number(spinBoxReadMatrixTofNumberPerLine->value())+"\";\n";
    
    if (checkBoxTranspose->isChecked())
        s+="lst<<\"[Detector-Data-Transpose] Yes\";\n";
    else
        s+="lst<<\"[Detector-Data-Transpose] No\";\n";
    
    if (checkBoxMatrixX2mX->isChecked())
        s+="lst<<\"[Detector-X-to-Minus-X] Yes\";\n";
    else
        s+="lst<<\"[Detector-X-to-Minus-X] No\";\n";
    
    if (checkBoxMatrixY2mY->isChecked())
        s+="lst<<\"[Detector-Y-to-Minus-Y] Yes\";\n";
    else
        s+="lst<<\"[Detector-Y-to-Minus-Y] No\";\n";
    
    //++++++++++++++++++++++
    //+++ detector :: dead-time              +
    //++++++++++++++++++++++
    s+="lst<<\"[Detector-Dead-Time] "+lineEditDeadTime->text()+"\";\n";
    s+="lst<<\"[Detector-Dead-Time-DB] "+lineEditDBdeadtime->text()+"\";\n";
    
    if (radioButtonDeadTimeCh->isChecked())
        s+="lst<<\"[Options-2D-DeadTimeModel-NonPara] Yes\";\n";
    else
        s+="lst<<\"[Options-2D-DeadTimeModel-NonPara] No\";\n";
    
    //++++++++++++++++++++++
    //+++ monitors :: dead-time
    //++++++++++++++++++++++
    s += "lst<<\"[Monitor1-Dead-Time] " + lineEditDeadTimeM1->text() + "\";\n";
    s += "lst<<\"[Monitor2-Dead-Time] " + lineEditDeadTimeM2->text() + "\";\n";
    s += "lst<<\"[Monitor3-Dead-Time] " + lineEditDeadTimeM3->text() + "\";\n";

    //++++++++++++++++++++++
    //+++ detector :: center                    +
    //++++++++++++++++++++++
    if (radioButtonCenterHF->isChecked())
        s+="lst<<\"[Options-2D-CenterMethod] HF\";\n";
    else if (radioButtonRadStdSymm->isChecked())
        s+="lst<<\"[Options-2D-CenterMethod] SYM\";\n";
    else
        s+="lst<<\"[Options-2D-CenterMethod] Header\";\n";
    
    
    //++++++++++++++++++++++++++++++
    //+++ detector :: rotation :: x
    //++++++++++++++++++++++++++++++
    if (radioButtonDetRotHeaderX->isChecked())
        s+="lst<<\"[DetRotation-X-Read-from-Header] Yes\";\n";
    else
        s+="lst<<\"[DetRotation-X-Read-from-Header] No\";\n";
    
    s+="lst<<\"[DetRotation-Angle-X] "+QString::number(doubleSpinBoxDetRotX->value(), 'f', 2)+"\";\n";

    if (checkBoxInvDetRotX->isChecked())
        s+="lst<<\"[DetRotation-Invert-Angle-X] Yes\";\n";
    else
        s+="lst<<\"[DetRotation-Invert-Angle-X] No\";\n";
    
    //++++++++++++++++++++++++++++++
    //+++ detector :: rotation :: y
    //++++++++++++++++++++++++++++++
    if (radioButtonDetRotHeaderY->isChecked())
        s+="lst<<\"[DetRotation-Y-Read-from-Header] Yes\";\n";
    else
        s+="lst<<\"[DetRotation-Y-Read-from-Header] No\";\n";
    
    s+="lst<<\"[DetRotation-Angle-Y] "+QString::number(doubleSpinBoxDetRotY->value(), 'f', 2)+"\";\n";
    
    if (checkBoxInvDetRotY->isChecked())
        s+="lst<<\"[DetRotation-Invert-Angle-Y] Yes\";\n";
    else
        s+="lst<<\"[DetRotation-Invert-Angle-Y] No\";\n";
    
    //++++++++++++++++++++++
    //+++ absolute calibration                +
    //++++++++++++++++++++++
    s=s+"lst<<\"[Calibrant-Type] "+comboBoxACmethod->currentText()+  "\";\n";
    
    
    s+="lst<<\"[Calibrant] "+comboBoxCalibrant->currentText()+"\";\n";
    
    if (checkBoxACDBuseActive->isChecked())
        s+="lst<<\"[Use-Active-Mask-and-Sensitivity-Matrixes] Yes\";\n";
    else
        s+="lst<<\"[Use-Active-Mask-and-Sensitivity-Matrixes] No\";\n";
    
    if (checkBoxTransmissionPlexi->isChecked())
        s+="lst<<\"[Calculate-Calibrant-Transmission-by-Equation] Yes\";\n";
    else
        s+="lst<<\"[Calculate-Calibrant-Transmission-by-Equation] No\";\n";
    
    //++++++++++++++++++++++
    //+++ mask :: options                      +
    //++++++++++++++++++++++
    if (groupBoxMaskBS->isChecked())
        s+="lst<<\"[Mask-BeamStop] Yes\";\n";
    else
        s+="lst<<\"[Mask-BeamStop] No\";\n";
    
    if (groupBoxMask->isChecked())
        s+="lst<<\"[Mask-Edge] Yes\";\n";
    else
        s+="lst<<\"[Mask-Edge] No\";\n";
    
    s+="lst<<\"[Mask-Edge-Shape] "
    +comboBoxMaskEdgeShape->currentText()+"\";\n";
    s+="lst<<\"[Mask-BeamStop-Shape] "
    +comboBoxMaskBeamstopShape->currentText()+"\";\n";
    
    s+="lst<<\"[Mask-Edge-Left-X] "
    +QString::number(spinBoxLTx->value())+"\";\n";
    s+="lst<<\"[Mask-Edge-Left-Y] "
    +QString::number(spinBoxLTy->value())+"\";\n";
    s+="lst<<\"[Mask-Edge-Right-X] "
    +QString::number(spinBoxRBx->value())+"\";\n";
    s+="lst<<\"[Mask-Edge-Right-Y] "
    +QString::number(spinBoxRBy->value())+"\";\n";
    s+="lst<<\"[Mask-BeamStop-Left-X] "
    +QString::number(spinBoxLTxBS->value())+"\";\n";
    s+="lst<<\"[Mask-BeamStop-Left-Y] "
    +QString::number(spinBoxLTyBS->value())+"\";\n";
    s+="lst<<\"[Mask-BeamStop-Right-X] "
    +QString::number(spinBoxRBxBS->value())+"\";\n";
    s+="lst<<\"[Mask-BeamStop-Right-Y] "
    +QString::number(spinBoxRByBS->value())+"\";\n";
    s+="lst<<\"[Mask-Dead-Ros] "
    +lineEditDeadRows->text()+"\";\n";
    s+="lst<<\"[Mask-Dead-Cols] "
    +lineEditDeadCols->text()+"\";\n";
    s+="lst<<\"[Mask-Triangular] "
    +lineEditMaskPolygons->text()+"\";\n";
    
    //++++++++++++++++++++++
    //+++ sensitivity :: options               +
    //++++++++++++++++++++++
    s+="lst<<\"[Sensitivity-SpinBoxErrLeftLimit] "
    +QString::number(spinBoxErrLeftLimit->value())+"\";\n";
    s+="lst<<\"[Sensitivity-SpinBoxErrRightLimit] "
    +QString::number(spinBoxErrRightLimit->value())+"\";\n";
    
    if (checkBoxSensError->isChecked())
        s+="lst<<\"[Sensitivity-CheckBoxSensError] Yes\";\n";
    else
        s+="lst<<\"[Sensitivity-CheckBoxSensError] No\";\n";
    
    if (buttonGroupSensanyD->isChecked())
        s+="lst<<\"[Sensitivity-in-Use] Yes\";\n";
    else
        s+="lst<<\"[Sensitivity-in-Use] No\";\n";
    
    if (checkBoxSensTr->isChecked())
        s+="lst<<\"[Sensitivity-Tr-Option] Yes\";\n";
    else
        s+="lst<<\"[Sensitivity-Tr-Option] No\";\n";
    s+="lst<<\"[Sensitivity-Masked-Pixels-Value] "
    +lineEditSensMaskedPixels->text()+"\";\n";
    
    //++++++++++++++++++++++
    //+++ transmission :: method          +
    //++++++++++++++++++++++
    s+="lst<<\"[Transmission-Method] "
    +comboBoxTransmMethod->currentText()+"\";\n";
    
    //++++++++++++++++++++++
    //+++ [2D] :: options                      +
    //++++++++++++++++++++++
    // 1
    if (checkBoxParallax->isChecked())
        s+="lst<<\"[Options-2D-HighQ] Yes\";\n";
    else
        s+="lst<<\"[Options-2D-HighQ] No\";\n";
    // 1a
    s+="lst<<\"[Options-2D-HighQ-Parallax-Type] "+QString::number(comboBoxParallax->currentIndex())+"\";\n";
    // 1c
    if (checkBoxParallaxTr->isChecked())
        s+="lst<<\"[Options-2D-HighQ-Tr] Yes\";\n";
    else
        s+="lst<<\"[Options-2D-HighQ-Tr] No\";\n";
    // 2
    s+="lst<<\"[Options-2D-Polar-Resolusion] "
    +QString::number(spinBoxPolar->value())+"\";\n";
    // 3
    if (checkBoxMaskNegative->isChecked())
        s+="lst<<\"[Options-2D-Mask-Negative-Points] Yes\";\n";
    else
        s+="lst<<\"[Options-2D-Mask-Negative-Points] No\";\n";
    // 4
    s+="lst<<\"[Options-2D-Normalization-Type] "
    +QString::number(comboBoxNorm->currentIndex())+"\";\n";
    // 5
    s+="lst<<\"[Options-2D-Normalization-Factor] "
    +QString::number(spinBoxNorm->value())+"\";\n";
    // 6
    if (checkBoxBCTimeNormalization->isChecked())
        s+="lst<<\"[Options-2D-Mask-Normalization-BC] Yes\";\n";
    else
        s+="lst<<\"[Options-2D-Mask-Normalization-BC] No\";\n";
    // 7
    if (radioButtonXYdimPixel->isChecked())
        s+="lst<<\"[Options-2D-xyDimension-Pixel] Yes\";\n";
    else
        s+="lst<<\"[Options-2D-xyDimension-Pixel] No\";\n";
    // 8
    s+="lst<<\"[Options-2D-Output-Format] "
    +QString::number(comboBoxIxyFormat->currentIndex())+"\";\n";
    //  9
    if (checkBoxASCIIheaderIxy->isChecked())
        s+="lst<<\"[Options-2D-Header-Output-Format] Yes\";\n";
    else
        s+="lst<<\"[Options-2D-Header-Output-Format] No\";\n";
    // 10
    if (checkBoxASCIIheaderSASVIEW->isChecked())
        s+="lst<<\"[Options-2D-Header-SASVIEW] Yes\";\n";
    else
        s+="lst<<\"[Options-2D-Header-SASVIEW] No\";\n";
    
    //++++++++++++++++++++++
    //+++ [1D] :: options                      +
    //++++++++++++++++++++++
    // 1
    if (radioButtonRadHF->isChecked())
        s+="lst<<\"[Options-1D-RADmethod-HF] Yes\";\n";
    else
        s+="lst<<\"[Options-1D-RADmethod-HF] No\";\n";
    // 1a
    s+="lst<<\"[Options-1D-RAD-LinearFactor] "
    +QString::number(spinBoxAvlinear->value())+"\";\n";
    // 1b
    s+="lst<<\"[Options-1D-RAD-ProgressiveFactor] "
    +QString::number(doubleSpinBoxAvLog->value())+"\";\n";
    // 2
    s+="lst<<\"[Options-1D-RemoveFirst] "
    +QString::number(spinBoxRemoveFirst->value())+"\";\n";
    // 3
    s+="lst<<\"[Options-1D-RemoveLast] "
    +QString::number(spinBoxRemoveLast->value())+"\";\n";
    // 4
    if (checkBoxMaskNegativeQ->isChecked())
        s+="lst<<\"[Options-1D-RemoveNegativePoints] Yes\";\n";
    else
        s+="lst<<\"[Options-1D-RemoveNegativePoints] No\";\n";
    // 5
    s+="lst<<\"[Options-1D-QxQy-From] "
    +QString::number(spinBoxFrom->value())+"\";\n";
    // 6
    s+="lst<<\"[Options-1D-QxQy-To] "
    +QString::number(spinBoxTo->value())+"\";\n";
    // 6a
    if (checkBoxSlicesBS->isChecked())
        s+="lst<<\"[Options-1D-QxQy-BS] Yes\";\n";
    else
        s+="lst<<\"[Options-1D-QxQy-BS] No\";\n";
    
    // 7
    s+="lst<<\"[Options-1D-OutputFormat] "
    +QString::number(comboBox4thCol->currentIndex())+"\";\n";
    // 7a
    if (checkBoxASCIIheader->isChecked())
        s+="lst<<\"[Options-1D-OutputFormat-PlusHeader] Yes\";\n";
    else
        s+="lst<<\"[Options-1D-OutputFormat-PlusHeader] No\";\n";
    // 7b
    if (checkBoxAnisotropy->isChecked())
        s+="lst<<\"[Options-1D-Anisotropy] Yes\";\n";
    else
        s+="lst<<\"[Options-1D-Anisotropy] No\";\n";
    // 7c
    s+="lst<<\"[Options-1D-AnisotropyAngle] "+QString::number(spinBoxAnisotropyOffset->value())+"\";\n";
    
    // 8
    s+="lst<<\"[Options-1D-QI-Presentation] "
    +QString::number(comboBoxSelectPresentation->currentIndex())+"\";\n";
    
    //++++++++++++++++++++++
    //+++ script table options                +
    //++++++++++++++++++++++
    if (checkBoxRecalculateUseNumber->isChecked())
        s+="lst<<\"[Sample-Position-As-Condition] Yes\";\n";
    else
        s+="lst<<\"[Sample-Position-As-Condition] No\";\n";
    
    if (checkBoxAttenuatorAsPara->isChecked())
        s+="lst<<\"[Attenuator-as-Condition] Yes\";\n";
    else
        s+="lst<<\"[Attenuator-as-Condition] No\";\n";
    
    if (checkBoxBeamcenterAsPara->isChecked())
        s+="lst<<\"[Beam-Center-as-Condition] Yes\";\n";
    else
        s+="lst<<\"[Beam-Center-as-Condition] No\";\n";
    
    if (checkBoxPolarizationAsPara->isChecked())
        s+="lst<<\"[Polarization-as-Condition] Yes\";\n";
    else
        s+="lst<<\"[Polarization-as-Condition] No\";\n";
    
    if (checkBoxDetRotAsPara->isChecked())
        s+="lst<<\"[DetectorAngle-as-Condition] Yes\";\n";
    else
        s+="lst<<\"[DetectorAngle-as-Condition] No\";\n";
    
    if (checkBoxRecalculate->isChecked())
        s+="lst<<\"[Reread-Existing-Runs] Yes\";\n";
    else
        s+="lst<<\"[Reread-Existing-Runs] No\";\n";
    
    if (checkBoxFindCenter->isChecked())
        s+="lst<<\"[Find-Center-For-EveryFile] Yes\";\n";
    else
        s+="lst<<\"[Find-Center-For-EveryFile] No\";\n";
    
    if (checkBoxForceCopyPaste->isChecked())
        s+="lst<<\"[Tr-Force-Copy-Paste] Yes\";\n";
    else
        s+="lst<<\"[Tr-Force-Copy-Paste] No\";\n";
    
    if (checkBoxNameAsTableName->isChecked())
        s+="lst<<\"[Sampe-Name-As-RunTableName] Yes\";\n";
    else
        s+="lst<<\"[Sampe-Name-As-RunTableName] No\";\n";
    
    if (checkBoxMergingTable->isChecked())
        s+="lst<<\"[Generate-MergingTable] Yes\";\n";
    else
        s+="lst<<\"[Generate-MergingTable] No\";\n";

    if (checkBoxAutoMerging->isChecked())
        s+="lst<<\"[Auto-Merging] Yes\";\n";
    else
        s+="lst<<\"[Auto-Merging] No\";\n";
    
    s+="lst<<\"[Overlap-Merging] "+QString::number(spinBoxOverlap->value())+"\";\n";
    
    if (checkBoxRewriteOutput->isChecked())
        s+="lst<<\"[Rewrite-Output] Yes\";\n";
    else
        s+="lst<<\"[Rewrite-Output] No\";\n";
    
    
    if (checkBoxSkiptransmisionConfigurations->isChecked())
        s+="lst<<\"[Skipt-Tr-Configurations] Yes\";\n";
    else
        s+="lst<<\"[Skipt-Tr-Configurations] No\";\n";
    
    
    if (checkBoxSortOutputToFolders->isChecked())
        s+="lst<<\"[Skipt-Output-Folders] Yes\";\n";
    else
        s+="lst<<\"[Skipt-Output-Folders] No\";\n";
    
    if (checkBoxResoFocus->isChecked())
        s+="lst<<\"[Resolusion-Focusing] Yes\";\n";
    else
        s+="lst<<\"[Resolusion-Focusing] No\";\n";
    
    if (checkBoxResoCAround->isChecked())
        s+="lst<<\"[Resolusion-CA-Round] Yes\";\n";
    else
        s+="lst<<\"[Resolusion-CA-Round] No\";\n";
    
    if (checkBoxResoSAround->isChecked())
        s+="lst<<\"[Resolusion-SA-Round] Yes\";\n";
    else
        s+="lst<<\"[Resolusion-SA-Round] No\";\n";
    
    
    s+="lst<<\"[Resolusion-Detector] "+lineEditDetReso->text()+"\";\n";
    
    s+="lst<<\"[File-Ext] "+lineEditFileExt->text()+"\";\n";

    s += "lst<<\"[POL-ALIAS-UP] " + lineEditUp->text() + "\";\n";
    s += "lst<<\"[POL-ALIAS-DOWN] " + lineEditDown->text() + "\";\n";
    s += "lst<<\"[POL-ALIAS-UP-UP] " + lineEditUpUp->text() + "\";\n";
    s += "lst<<\"[POL-ALIAS-UP-DOWN] " + lineEditUpDown->text() + "\";\n";
    s += "lst<<\"[POL-ALIAS-DOWN-DOWN] " + lineEditDownDown->text() + "\";\n";
    s += "lst<<\"[POL-ALIAS-DOWN-UP] " + lineEditDownUp->text() + "\";\n";

    s += "lst<<\"[POLARIZATION] " + polarizationSelector->writeSettingsString() + "\";\n";
    s += "lst<<\"[POL-TRANSMISSION] " + polTransmissionSelector->writeSettingsString() + "\";\n";
    s += "lst<<\"[POL-FLIPPER-EFFICIENCY] " + polFlipperEfficiencySelector->writeSettingsString() + "\";\n";

    s += "lst<<\"[ANALYZER-TRANSMISSION] " + analyzerTransmissionSelector->writeSettingsString() + "\";\n";
    s += "lst<<\"[ANALYZER-EFFICIENCY] " + analyzerEfficiencySelector->writeSettingsString() + "\";\n";

    s+="}\n";
    
    
    QFile f(instrPath+"/"+instrName+".cpp");
    
    
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        //*************************************Log Window Output
        QMessageBox::warning(this,"Could not write to file:: "+instrName+".cpp", tr("qtiSAS::DAN"));
        //*************************************Log Window Output
        return;
    }
    
    
    QTextStream stream( &f );
    stream<<s;
    f.close();
}

void dan18::saveInstrumentAs()
{
    if (app()->sasPath == "")
        return;

    QString instrPath;
    if (!QDir(app()->sasPath + "/SANSinstruments/").exists())
        QDir().mkdir(app()->sasPath + "/SANSinstruments/");
    instrPath = app()->sasPath + "/SANSinstruments/";
    instrPath = instrPath.replace("//", "/");

    bool ok = false;

    QString fileName = comboBoxInstrument->currentText();

    if (comboBoxInstrument->currentIndex() < 19)
        fileName = "Create Your SAS-Instrument: Input instrument Name";

    while (ok == false)
    {
        fileName = QInputDialog::getText(this,
                                         "qtiSAS", "Create Your SAS Instrument", QLineEdit::Normal,
                                         fileName, &ok);
        if ( !ok ||  fileName.isEmpty() )
        {
            return;
        }
        
        if (fileName=="KWS1"|| fileName=="KWS1-2020" || fileName=="KWS1-He3" || fileName=="KWS2" || fileName=="KWS2-He3-10%" || fileName=="KWS2-He3-20%" || fileName=="KWS2-HRD" || fileName=="KWS3" || fileName=="kws3-2016" || fileName=="kws3-2017-nicos" || fileName=="KWS3-VHRD" || fileName=="KWS3-VHRD-2018" || fileName=="KWS3-VHRD-2020" || fileName=="KWS3-2018" || fileName=="KWS3-2020" || fileName=="MARIA"|| fileName=="SANS1")
        {
            ok=false;
            fileName="Please do not use STANDARD instrument names";
        }
    }
    
    saveInstrumentAsCpp(instrPath,fileName);
    
    QString s;
    //+++ instrument
    s+="[Instrument] "+fileName+"\n";
    
    //+++ mode
    s+="[Instrument-Mode] "+comboBoxMode->currentText()+"\n";
    
    //+++ data format
    s+="[DataFormat] "+QString::number(comboBoxHeaderFormat->currentIndex())+"\n";
    
    //+++ color
    s += "[Color] " + pushButtonInstrColor->palette().color(QPalette::Window).name() + "\n";
    
    //++++++++++++++++++++++
    //+++ data :: input&output folder    +
    //++++++++++++++++++++++
    s+="[Input-Folder] "+lineEditPathDAT->text()+"\n";
    s+="[Output-Folder] "+lineEditPathRAD->text()+"\n";
    //+++ sub folders
    if (checkBoxDirsIndir->isChecked())
        s+="[Include-Sub-Foldes] Yes\n";
    else
        s+="[Include-Sub-Foldes] No\n";
    
    //++++++++++++++++++++++
    //+++  units                                      +
    //++++++++++++++++++++++
    s+="[Units-Lambda] "+QString::number(comboBoxUnitsLambda->currentIndex())+"\n";
    s+="[Units-Appertures] "+QString::number(comboBoxUnitsBlends->currentIndex())+"\n";
    s+="[Units-Thickness] "+QString::number(comboBoxThicknessUnits->currentIndex())+"\n";
    s+="[Units-Time] "+QString::number(comboBoxUnitsTime->currentIndex())+"\n";
    s+="[Units-Time-RT] "+QString::number(comboBoxUnitsTimeRT->currentIndex())+"\n";
    s += "[Units-C] " + QString::number(comboBoxUnitsC->currentIndex()) + "\n";
    s += "[Units-C-D-Offset] " + QString::number(comboBoxUnitsD->currentIndex()) + "\n";
    s += "[Units-Selector] " + QString::number(comboBoxUnitsSelector->currentIndex()) + "\n";

    //++++++++++++++++++++++
    //+++ file(s) :: structure                   +
    //++++++++++++++++++++++
    
    //+++ 2ND header
    if (checkBoxYes2ndHeader->isChecked())
        s+="[2nd-Header-OK] Yes\n";
    else
        s+="[2nd-Header-OK] No\n";
    s+="[2nd-Header-Pattern] "+lineEditWildCard2ndHeader->text()+"\n";
    s+="[2nd-Header-Lines] "+QString::number(spinBoxHeaderNumberLines2ndHeader->value())+"\n";
    
    //+++ 1ND header
    s+="[Pattern] "+lineEditWildCard->text()+"\n";
    s+="[Pattern-Select-Data] "+textEditPattern->text()+"\n";
    s+="[Header-Number-Lines] "+QString::number(spinBoxHeaderNumberLines->value())+"\n";
    
    //+++ TOF/RT Headers
    s+="[Data-Header-Lines] "+QString::number(spinBoxDataHeaderNumberLines->value())+"\n";
    s+="[Lines-Between-Frames] "+QString::number(spinBoxDataLinesBetweenFrames->value())+"\n";
    
    //+++ Flexible-Header
    if (checkBoxHeaderFlexibility->isChecked())
        s+="[Flexible-Header] Yes\n";
    else
        s+="[Flexible-Header] No\n";
    
    //+++ Flexible-Stop
    s+="[Flexible-Stop] "+lineEditFlexiStop->text()+"\n";
    
    //+++ Remove-None-Printable-Symbols
    if (checkBoxRemoveNonePrint->isChecked())
        s+="[Remove-None-Printable-Symbols] Yes\n";
    else
        s+="[Remove-None-Printable-Symbols] No\n";

    //+++ Image-Data
    if (radioButtonDetectorFormatImage->isChecked())
        s += "[Image-Data] Yes\n";
    else
        s += "[Image-Data] No\n";

    //+++ HDF-Data
    if (radioButtonDetectorFormatHDF->isChecked())
        s += "[HDF-Data] Yes\n";
    else
        s += "[HDF-Data] No\n";

    //+++ YAML-Data
    if (radioButtonDetectorFormatYAML->isChecked())
        s += "[YAML-Data] Yes\n";
    else
        s += "[YAML-Data] No\n";

    //+++ HDF-detector-entry
    s += "[HDF-detector-entry] " + lineEditHdfDetectorEntry->text() + "\n";

    //+++ HDF-data-structure
    s += "[HDF-data-structure] " + QString::number(comboBoxDxDyN->currentIndex()) + "\n";

    //+++ YAML-detector-entry
    s += "[YAML-detector-entry] " + lineEditYamlDetectorEntry->text() + "\n";

    //+++ XML-base
    s+="[XML-base] "+lineEditXMLbase->text()+"\n";
    
    //++++++++++++++++++++++
    //+++ header :: map                        +
    //++++++++++++++++++++++
    for (int i = 0; i < parserHeader->listOfHeaders.count(); i++)
    {
        s += parserHeader->listOfHeaders[i] + " ";
        s += tableHeaderPosNew->item(i, 0)->text() + ";;;";
        s += tableHeaderPosNew->item(i, 1)->text() + "\n";
    }
    //++++++++++++++++++++++
    //+++ selector :: wave length           +
    //++++++++++++++++++++++
    if (radioButtonLambdaHeader->isChecked())
        s+="[Selector-Read-from-Header] Yes\n";
    else
        s+="[Selector-Read-from-Header] No\n";
    
    s+="[Selector-P1] "+lineEditSel1->text()+"\n";
    s+="[Selector-P2] "+lineEditSel2->text()+"\n";
    
    //++++++++++++++++++++++
    //+++ detector :: image                    +
    //++++++++++++++++++++++
    s+="[Detector-Data-Dimension] "+QString::number(comboBoxMDdata->currentIndex())+"\n";
    s+="[Detector-Data-Focus] "+QString::number(spinBoxRegionOfInteres->value())+"\n";
    s+="[Detector-Binning] "+QString::number(comboBoxBinning->currentIndex())+"\n";
    s+="[Detector-Pixel-Size] "+lineEditResoPixelSize->text()+"\n";
    s+="[Detector-Pixel-Size-Asymetry] "+lineEditAsymetry->text()+"\n";
    s+="[Detector-Data-Numbers-Per-Line] "
    +QString::number(spinBoxReadMatrixNumberPerLine->value())+"\n";
    s+="[Detector-Data-Tof-Numbers-Per-Line] "
    +QString::number(spinBoxReadMatrixTofNumberPerLine->value())+"\n";
    
    if (checkBoxTranspose->isChecked())
        s+="[Detector-Data-Transpose] Yes\n";
    else
        s+="[Detector-Data-Transpose] No\n";
    
    if (checkBoxMatrixX2mX->isChecked())
        s+="[Detector-X-to-Minus-X] Yes\n";
    else
        s+="[Detector-X-to-Minus-X] No\n";
    
    if (checkBoxMatrixY2mY->isChecked())
        s+="[Detector-Y-to-Minus-Y] Yes\n";
    else
        s+="[Detector-Y-to-Minus-Y] No\n";
    
    //++++++++++++++++++++++
    //+++ detector :: dead-time              +
    //++++++++++++++++++++++
    s+="[Detector-Dead-Time] "+lineEditDeadTime->text()+"\n";
    s+="[Detector-Dead-Time-DB] "+lineEditDBdeadtime->text()+"\n";
    
    if (radioButtonDeadTimeCh->isChecked())
        s+="[Options-2D-DeadTimeModel-NonPara] Yes\n";
    else
        s+="[Options-2D-DeadTimeModel-NonPara] No\n";

    //++++++++++++++++++++++
    //+++ monitors :: dead-time
    //++++++++++++++++++++++
    s += "[Monitor1-Dead-Time] " + lineEditDeadTimeM1->text() + "\n";
    s += "[Monitor2-Dead-Time] " + lineEditDeadTimeM2->text() + "\n";
    s += "[Monitor3-Dead-Time] " + lineEditDeadTimeM3->text() + "\n";

    //++++++++++++++++++++++
    //+++ detector :: center                    +
    //++++++++++++++++++++++
    if (radioButtonCenterHF->isChecked())
        s+="[Options-2D-CenterMethod] HF\n";
    else if (radioButtonRadStdSymm->isChecked())
        s+="[Options-2D-CenterMethod] SYM\n";
    else
        s+="[Options-2D-CenterMethod] Header\n";
    
    //+++++++++++++++++++++++++++++
    //+++ detector :: rotation :: x
    //+++++++++++++++++++++++++++++
    if (radioButtonDetRotHeaderX->isChecked())
        s+="[DetRotation-X-Read-from-Header] Yes\n";
    else
        s+="[DetRotation-X-Read-from-Header] No\n";
    
    s+="[DetRotation-Angle-X] "+QString::number(doubleSpinBoxDetRotX->value(), 'f', 2)+"\n";

    if (checkBoxInvDetRotX->isChecked())
        s+="[DetRotation-Invert-Angle-X] Yes\n";
    else
        s+="[DetRotation-Invert-Angle-X] No\n";
    
    //+++++++++++++++++++++++++++++
    //+++ detector :: rotation :: y
    //+++++++++++++++++++++++++++++
    if (radioButtonDetRotHeaderY->isChecked())
        s+="[DetRotation-Y-Read-from-Header] Yes\n";
    else
        s+="[DetRotation-Y-Read-from-Header] No\n";
    
    s+="[DetRotation-Angle-Y] "+QString::number(doubleSpinBoxDetRotY->value(), 'f', 2)+"\n";
    
    if (checkBoxInvDetRotY->isChecked())
        s+="[DetRotation-Invert-Angle-Y] Yes\n";
    else
        s+="[DetRotation-Invert-Angle-Y] No\n";
    
    //++++++++++++++++++++++
    //+++ absolute calibration                +
    //++++++++++++++++++++++
    
    s=s+"[Calibrant-Type] "+comboBoxACmethod->currentText()+"\n";
    
    s+="[Calibrant] "+comboBoxCalibrant->currentText()+"\n";
    
    if (checkBoxACDBuseActive->isChecked())
        s+="[Use-Active-Mask-and-Sensitivity-Matrixes] Yes\n";
    else
        s+="[Use-Active-Mask-and-Sensitivity-Matrixes] No\n";
    
    if (checkBoxTransmissionPlexi->isChecked())
        s+="[Calculate-Calibrant-Transmission-by-Equation] Yes\n";
    else
        s+="[Calculate-Calibrant-Transmission-by-Equation] No\n";
    
    //++++++++++++++++++++++
    //+++ mask :: options                      +
    //++++++++++++++++++++++
    if (groupBoxMaskBS->isChecked())
        s+="[Mask-BeamStop] Yes\n";
    else
        s+="[Mask-BeamStop] No\n";
    
    if (groupBoxMask->isChecked())
        s+="[Mask-Edge] Yes\n";
    else
        s+="[Mask-Edge] No\n";
    
    s+="[Mask-Edge-Shape] "
    +comboBoxMaskEdgeShape->currentText()+"\n";
    s+="[Mask-BeamStop-Shape] "
    +comboBoxMaskBeamstopShape->currentText()+"\n";
    
    s+="[Mask-Edge-Left-X] "
    +QString::number(spinBoxLTx->value())+"\n";
    s+="[Mask-Edge-Left-Y] "
    +QString::number(spinBoxLTy->value())+"\n";
    s+="[Mask-Edge-Right-X] "
    +QString::number(spinBoxRBx->value())+"\n";
    s+="[Mask-Edge-Right-Y] "
    +QString::number(spinBoxRBy->value())+"\n";
    s+="[Mask-BeamStop-Left-X] "
    +QString::number(spinBoxLTxBS->value())+"\n";
    s+="[Mask-BeamStop-Left-Y] "
    +QString::number(spinBoxLTyBS->value())+"\n";
    s+="[Mask-BeamStop-Right-X] "
    +QString::number(spinBoxRBxBS->value())+"\n";
    s+="[Mask-BeamStop-Right-Y] "
    +QString::number(spinBoxRByBS->value())+"\n";
    s+="[Mask-Dead-Rows] "
    +lineEditDeadRows->text()+"\n";
    s+="[Mask-Dead-Cols] "
    +lineEditDeadCols->text()+"\n";
    s+="[Mask-Triangular] "
    +lineEditMaskPolygons->text()+"\n";
    
    //++++++++++++++++++++++
    //+++ sensitivity :: options               +
    //++++++++++++++++++++++
    s+="[Sensitivity-SpinBoxErrLeftLimit] "
    +QString::number(spinBoxErrLeftLimit->value())+"\n";
    s+="[Sensitivity-SpinBoxErrRightLimit] "
    +QString::number(spinBoxErrRightLimit->value())+"\n";
    
    if (checkBoxSensError->isChecked())
        s+="[Sensitivity-CheckBoxSensError] Yes\n";
    else
        s+="[Sensitivity-CheckBoxSensError] No\n";
    
    if (buttonGroupSensanyD->isChecked())
        s+="[Sensitivity-in-Use] Yes\n";
    else
        s+="[Sensitivity-in-Use] No\n";
    
    if (checkBoxSensTr->isChecked())
        s+="[Sensitivity-Tr-Option] Yes\n";
    else
        s+="[Sensitivity-Tr-Option] No\n";
    s+="[Sensitivity-Masked-Pixels-Value] "
    +lineEditSensMaskedPixels->text()+"\n";
    
    //++++++++++++++++++++++
    //+++ transmission :: method          +
    //++++++++++++++++++++++
    s+="[Transmission-Method] "
    +comboBoxTransmMethod->currentText()+"\n";
    
    //++++++++++++++++++++++
    //+++ [2D] :: options                      +
    //++++++++++++++++++++++
    // 1
    if (checkBoxParallax->isChecked())
        s+="[Options-2D-HighQ] Yes\n";
    else
        s+="[Options-2D-HighQ] No\n";
    // 1a
    s+="[Options-2D-HighQ-Parallax-Type] "
    +QString::number(comboBoxParallax->currentIndex())+"\n";
    // 1c
    if (checkBoxParallaxTr->isChecked())
        s+="[Options-2D-HighQ-Tr] Yes\n";
    else
        s+="[Options-2D-HighQ-Tr] No\n";
    // 2
    s+="[Options-2D-Polar-Resolusion] "
    +QString::number(spinBoxPolar->value())+"\n";
    // 3
    if (checkBoxMaskNegative->isChecked())
        s+="[Options-2D-Mask-Negative-Points] Yes\n";
    else
        s+="[Options-2D-Mask-Negative-Points] No\n";
    // 4
    s+="[Options-2D-Normalization-Type] "
    +QString::number(comboBoxNorm->currentIndex())+"\n";
    // 5
    s+="[Options-2D-Normalization-Factor] "
    +QString::number(spinBoxNorm->value())+"\n";
    // 6
    if (checkBoxBCTimeNormalization->isChecked())
        s+="[Options-2D-Mask-Normalization-BC] Yes\n";
    else
        s+="[Options-2D-Mask-Normalization-BC] No\n";
    // 7
    if (radioButtonXYdimPixel->isChecked())
        s+="[Options-2D-xyDimension-Pixel] Yes\n";
    else
        s+="[Options-2D-xyDimension-Pixel] No\n";
    // 8
    s+="[Options-2D-Output-Format] "
    +QString::number(comboBoxIxyFormat->currentIndex())+"\n";
    // 9
    if (checkBoxASCIIheaderIxy->isChecked())
        s+="[Options-2D-Header-Output-Format] Yes\n";
    else
        s+="[Options-2D-Header-Output-Format] No\n";
    // 10
    if (checkBoxASCIIheaderSASVIEW->isChecked())
        s+="[Options-2D-Header-SASVIEW] Yes\n";
    else
        s+="[Options-2D-Header-SASVIEW] No\n";
    
    //++++++++++++++++++++++
    //+++ [1D] :: options                      +
    //++++++++++++++++++++++
    // 1
    if (radioButtonRadHF->isChecked())
        s+="[Options-1D-RADmethod-HF] Yes\n";
    else
        s+="[Options-1D-RADmethod-HF] No\n";
    // 1a
    s+="[Options-1D-RAD-LinearFactor] "
    +QString::number(spinBoxAvlinear->value())+"\n";
    // 1b
    s+="[Options-1D-RAD-ProgressiveFactor] "
    +QString::number(doubleSpinBoxAvLog->value())+"\n";
    // 2
    s+="[Options-1D-RemoveFirst] "
    +QString::number(spinBoxRemoveFirst->value())+"\n";
    // 3
    s+="[Options-1D-RemoveLast] "
    +QString::number(spinBoxRemoveLast->value())+"\n";
    // 4
    if (checkBoxMaskNegativeQ->isChecked())
        s+="[Options-1D-RemoveNegativePoints] Yes\n";
    else
        s+="[Options-1D-RemoveNegativePoints] No\n";
    // 5
    s+="[Options-1D-QxQy-From] "
    +QString::number(spinBoxFrom->value())+"\n";
    // 6
    s+="[Options-1D-QxQy-To] "
    +QString::number(spinBoxTo->value())+"\n";
    // 6a
    if (checkBoxSlicesBS->isChecked())
        s+="[Options-1D-QxQy-BS] Yes\n";
    else
        s+="[Options-1D-QxQy-BS] No\n";
    // 7
    s+="[Options-1D-OutputFormat] "
    +QString::number(comboBox4thCol->currentIndex())+"\n";
    // 7a
    if (checkBoxASCIIheader->isChecked())
        s+="[Options-1D-OutputFormat-PlusHeader] Yes\n";
    else
        s+="[Options-1D-OutputFormat-PlusHeader] No\n";
    // 7b
    if (checkBoxAnisotropy->isChecked())
        s+="[Options-1D-Anisotropy] Yes\n";
    else
        s+="[Options-1D-Anisotropy] No\n";
    // 7c
    s+="[Options-1D-AnisotropyAngle] "
    +QString::number(spinBoxAnisotropyOffset->value())+"\n";
    
    // 8
    s+="[Options-1D-QI-Presentation] "
    +QString::number(comboBoxSelectPresentation->currentIndex())+"\n";
    
    //++++++++++++++++++++++
    //+++ script table options                +
    //++++++++++++++++++++++
    if (checkBoxRecalculateUseNumber->isChecked())
        s+="[Sample-Position-As-Condition] Yes\n";
    else
        s+="[Sample-Position-As-Condition] No\n";
    
    if (checkBoxAttenuatorAsPara->isChecked())
        s+="[Attenuator-as-Condition] Yes\n";
    else
        s+="[Attenuator-as-Condition] No\n";
    
    if (checkBoxBeamcenterAsPara->isChecked())
        s+="[Beam-Center-as-Condition] Yes\n";
    else
        s+="[Beam-Center-as-Condition] No\n";
    
    if (checkBoxPolarizationAsPara->isChecked())
        s+="[Polarization-as-Condition] Yes\n";
    else
        s+="[Polarization-as-Condition] No\n";
    
    if (checkBoxDetRotAsPara->isChecked())
        s+="[DetectorAngle-as-Condition] Yes\n";
    else
        s+="[DetectorAngle-as-Condition] No\n";
    
    if (checkBoxRecalculate->isChecked())
        s+="[Reread-Existing-Runs] Yes\n";
    else
        s+="[Reread-Existing-Runs] No\n";
    
    if (checkBoxFindCenter->isChecked())
        s+="[Find-Center-For-EveryFile] Yes\n";
    else
        s+="[Find-Center-For-EveryFile] No\n";
    
    if (checkBoxForceCopyPaste->isChecked())
        s+="[Tr-Force-Copy-Paste] Yes\n";
    else
        s+="[Tr-Force-Copy-Paste] No\n";
    
    if (checkBoxNameAsTableName->isChecked())
        s+="[Sampe-Name-As-RunTableName] Yes\n";
    else
        s+="[Sampe-Name-As-RunTableName] No\n";
    
    if (checkBoxMergingTable->isChecked())
        s+="[Generate-MergingTable] Yes\n";
    else
        s+="[Generate-MergingTable] No\n";
    
    if (checkBoxAutoMerging->isChecked())
        s+="[Auto-Merging] Yes\n";
    else
        s+="[Auto-Merging] No\n";
    
    s+="[Overlap-Merging] "+QString::number(spinBoxOverlap->value())+"\n";
    
    if (checkBoxRewriteOutput->isChecked())
        s+="[Rewrite-Output] Yes\n";
    else
        s+="[Rewrite-Output] No\n";
    
    
    if (checkBoxSkiptransmisionConfigurations->isChecked())
        s+="[Skipt-Tr-Configurations] Yes\n";
    else
        s+="[Skipt-Tr-Configurations] No\n";
    
    
    if (checkBoxSortOutputToFolders->isChecked())     
        s+="[Skipt-Output-Folders] Yes\n";
    else 
        s+="[Skipt-Output-Folders] No\n";
    
    if (checkBoxResoFocus->isChecked())     
        s+="[Resolusion-Focusing] Yes\n";
    else 
        s+="[Resolusion-Focusing] No\n";	
    
    if (checkBoxResoCAround->isChecked())     
        s+="[Resolusion-CA-Round] Yes\n";
    else 
        s+="[Resolusion-CA-Round] No\n";	
    
    if (checkBoxResoSAround->isChecked())     
        s+="[Resolusion-SA-Round] Yes\n";
    else 
        s+="[Resolusion-SA-Round] No\n";	
    
    
    s+="[Resolusion-Detector] "+lineEditDetReso->text()+"\n";
    
    s+="[File-Ext] "+lineEditFileExt->text()+"\n";

    s += "[POL-ALIAS-UP] " + lineEditUp->text() + "\n";
    s += "[POL-ALIAS-DOWN] " + lineEditDown->text() + "\n";
    s += "[POL-ALIAS-UP-UP] " + lineEditUpUp->text() + "\n";
    s += "[POL-ALIAS-UP-DOWN] " + lineEditUpDown->text() + "\n";
    s += "[POL-ALIAS-DOWN-DOWN] " + lineEditDownDown->text() + "\n";
    s += "[POL-ALIAS-DOWN-UP] " + lineEditDownUp->text() + "\n";

    s += "[POLARIZATION] " + polarizationSelector->writeSettingsString() + "\n";
    s += "[POL-TRANSMISSION] " + polTransmissionSelector->writeSettingsString() + "\n";
    s += "[POL-FLIPPER-EFFICIENCY] " + polFlipperEfficiencySelector->writeSettingsString() + "\n";

    s += "[ANALYZER-TRANSMISSION] " + analyzerTransmissionSelector->writeSettingsString() + "\n";
    s += "[ANALYZER-EFFICIENCY] " + analyzerEfficiencySelector->writeSettingsString() + "\n";

    QFile f(instrPath+"/"+fileName+".SANS");
    
    
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        //*************************************Log Window Output
        QMessageBox::warning(this,"Could not write to file:: "+fileName+".SANS", tr("qtiSAS::DAN"));
        //*************************************Log Window Output
        return;
    }	
    
    QTextStream stream( &f );
    stream<<s;
    f.close();	

    findSANSinstruments();
    if (comboBoxInstrument->findText(fileName) >= 0)
        comboBoxInstrument->setCurrentIndex(comboBoxInstrument->findText(fileName));

    instrumentSelected();
    return;
}

void dan18::findSANSinstruments()
{
    if (!app() || app()->sasPath == "")
        return;

    QString instrPath;

    if (!QDir(app()->sasPath + "/SANSinstruments/").exists())
        QDir().mkdir(app()->sasPath + "/SANSinstruments/");

    instrPath = app()->sasPath + "/SANSinstruments/";
    instrPath = instrPath.replace("//", "/");

    QStringList lst = QDir(instrPath).entryList(QStringList() << "*.SANS");
    lst.replaceInStrings(".SANS", "");
    lst.prepend("KWS3-VHRD-2020");
    lst.prepend("KWS3-2020");
    lst.prepend("KWS3-VHRD-2018");
    lst.prepend("KWS3-VHRD");
    lst.prepend("KWS3-2018");
    lst.prepend("kws3-2017-nicos");
    lst.prepend("kws3-2016");
    lst.prepend("KWS3");
    lst.prepend("KWS2");
    lst.prepend("KWS1-He3");
    lst.prepend("KWS1");
    lst.prepend("SANS1");
    lst.prepend("MARIA");
    lst.prepend("KWS3-VHRD-2021");
    lst.prepend("KWS3-2021");
    lst.prepend("KWS2-HRD");
    lst.prepend("KWS2-He3-10%");
    lst.prepend("KWS2-He3-20%");
    lst.prepend("KWS1-2020");

    QString ct = comboBoxInstrument->currentText();
    
    comboBoxInstrument->clear();
    comboBoxInstrument->addItems(lst);

    if (comboBoxInstrument->findText(ct) >= 0)
    {
        comboBoxInstrument->setCurrentIndex(comboBoxInstrument->findText(ct));
        instrumentSelected();
    }
}

void dan18::deleteCurrentInstrument()
{
    if (!app() || app()->sasPath=="") return;
    
    if (comboBoxInstrument->currentIndex() < 19)
        return;

    QString fileName = comboBoxInstrument->currentText();

    QString instrPath;

    if (!QDir(app()->sasPath + "/SANSinstruments/").exists())
        QDir().mkdir(app()->sasPath + "/SANSinstruments/");

    instrPath = app()->sasPath + "/SANSinstruments/";
    instrPath = instrPath.replace("//", "/");

    QDir(instrPath).remove(fileName + ".SANS");

    findSANSinstruments();
}

void dan18::selectInstrumentColor()
{
    QColor initialColor = pushButtonInstrColor->palette().color(QPalette::Window);
    
    QColor color = QColorDialog::getColor(initialColor,app(), "Select Instrument Color" );

    if ( color.isValid() )
    {
        pushButtonInstrColor->setStyleSheet("background-color: "+color.name()+";");
    }
}

// experimental mode
void dan18::experimentalModeSelected()
{
    const QString mode = comboBoxMode->currentText();
    if (mode.contains("(PN)"))
    {
        tableEC->showRow(dptPOL);
        framePolarizationTable->show();
        buttonGroupdaNdAN->setTitle("Process active Polarization-Script Table");
    }
    else
    {
        tableEC->hideRow(dptPOL);
        framePolarizationTable->hide();
        buttonGroupdaNdAN->setTitle("Process active Script-Table");
    }
}

//+++
bool dan18::checkDataPath()
{
    QString Dir = filesManager->pathInString();

    QDir dd;
    if (!dd.cd(Dir))
        return false;
    return true;
}

//+++
bool dan18::selectFile(QString &fileNumber)
{
    QString Dir = filesManager->pathInString();
    QString filter = textEditPattern->text(); // MOVE TO FILESMANAGER
    QString wildCard = filesManager->wildCardDetector();
    bool dirsInDir = filesManager->subFoldersYN();

    QFileDialog *fd = new QFileDialog(this,"Choose a file",Dir,"*");

    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFile);
    fd->setWindowTitle(tr("DAN - Getting File Information"));
    fd->setNameFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);

    if (fd->exec() == QDialog::Rejected)
        return false;

    QStringList selectedDat=fd->selectedFiles();
    
    if (selectedDat.count()==0)
    {
        QMessageBox::critical( 0, "qtiSAS", "Nothing was selected");
        return false;
    }

    fileNumber =selectedDat[0];
    
    /*
    fileNumber =
    QFileDialog::getOpenFileName(
                                 Dir,
                                 filter,
                                 this,
                                 "open file dialog",
                                 "Choose a file" );
    */
    
    fileNumber=fileNumber.replace('\\', '/');
    
    if ( fileNumber.contains(Dir) )
    {
        if (Dir.right(1)=="/") fileNumber=fileNumber.remove(Dir);
        else fileNumber=fileNumber.remove(Dir+"/");
        if (!dirsInDir )
        {
            if (fileNumber.contains("/") || fileNumber.contains('\\'))
            {
                fileNumber="";
                return false;
            }
        }
        else
        {
            if ( fileNumber.count("/")>1 )
            {
                fileNumber="";
                return false;
            }
        }
    }
    else
    {
        fileNumber="";
        return false;
    }
    fileNumber = FilesManager::findFileNumberInFileName(wildCard, fileNumber);
    if (fileNumber == "")
        return false;

    return true;
}

//*******************************************
//*** findFitDataTable
//*******************************************
bool dan18::findFitDataTable(QString curveName, Table* &table, int &xColIndex, int &yColIndex )
{
    int i, ixy;
    bool exist=false;
    
    QString tableName=curveName.left(curveName.indexOf("_",0));
    QString colName=curveName.remove(tableName+"_");
    
    
    QList<MdiSubWindow *> windows = app()->windowsList();
    
    foreach (MdiSubWindow *w, windows) {
        if (QString(w->metaObject()->className()) == "Table" && w->name()==tableName) {
            table=(Table*)w;
            yColIndex=table->colIndex(colName);
            xColIndex=0;
            
            bool xSearch=true;
            ixy=yColIndex-1;
            while(xSearch && ixy>0 )
            {
                if (table->colPlotDesignation(ixy)==1)
                {
                    xColIndex=ixy;
                    xSearch=false;
                }
                else ixy--;
            }
            exist=true;
        }
    }
    
    return exist;
}

//*********************************************************
// +++  Add Curve  +++
//*********************************************************
bool dan18::AddCurve(Graph* g,QString curveName)
{
    int ii;
    
    Table* table;
    
    int xColIndex, yColIndex;
    
    if ( !findFitDataTable(curveName, table, xColIndex,  yColIndex ) ) return false;
    
    // check Table
    int  nReal=0;
    for (ii=0; ii<table->numRows(); ii++) if ((table->text(ii,0))!="") nReal++;
    //
    if (nReal<=2)
    {
        QMessageBox::warning(this,tr("qtiSAS"),
                             tr("Check Data Sets"));
        return false;
    }
    
    QStringList contents;
    contents=g->curveNamesList();
    
    if (g && table && !contents.contains(curveName))
    {
        int scatterCounter=0;
        
        for (int i=0; i<contents.count();i++)
        {
            PlotCurve *c = (PlotCurve *)g->curve(i);
            if (!c) break;
            int curve_type = c->plotStyle();
            
            if ( curve_type == Graph::Scatter) scatterCounter++;
        }
        
        int style = Graph::Scatter;
        g->insertCurve(table, curveName, style);
        CurveLayout cl = Graph::initCurveLayout();
        
        
        int color =(scatterCounter)%15;
        if (color >= 13) color++;
        int shape=(scatterCounter)%15+1;
        
        if (scatterCounter==0)
        {
            color=0;
            shape=1;
        }
        
        cl.lCol=color;
        cl.symCol=color;
        cl.fillCol=color;
        cl.aCol=color;
        cl.lWidth = app()->defaultCurveLineWidth;
        cl.sSize = app()->defaultSymbolSize;
        
        cl.sType=shape;
        
        g->updateCurveLayout((PlotCurve *)g->curve(contents.count()), &cl);
        g->replot();
    }
    
    return true;
}
