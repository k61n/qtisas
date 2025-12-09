/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2024 Artem Feoktystov <a.feoktystov@fz-juelich.de>
Description: SANS init tools
 ******************************************************************************/

#include <QScreen>
#include <QStringList>
#include <QToolBox>

#include "Folder.h"
#include "dan18.h"
#include "dan-sans-instruments.h"

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

    //+++ Files Manager
    filesManager = new FilesManager(lineEditPathDAT, textEditPattern, checkBoxDirsIndir, pushButtonDATpath,
                                    lineEditPathRAD, pushButtonRADpath, lineEditWildCard, lineEditWildCard2ndHeader,
                                    checkBoxYes2ndHeader, textEditPattern);

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
    
    if (auto *cornerButton = tableEC->findChild<QAbstractButton *>())
    {
        disconnect(cornerButton, nullptr, this, nullptr);
        connect(cornerButton, &QAbstractButton::clicked, this, &dan18::tableECcorner);

        cornerButton->setStyleSheet("background-color: transparent;"
                                    "border: 1px solid rgb(137, 137, 183);"
                                    "padding: 0px 0px 0px 5px;");

        cornerButton->setText("all");
    }
}

void dan18::tableECcorner()
{
    tableEC->clearSelection();
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
void dan18::openSession(QString scriptName)
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

    if (comboBoxMakeScriptTable->findText(scriptName) < 0)
        scriptName = "";

    if (scriptName.isEmpty())
    {
        bool ok;
        scriptName = QInputDialog::getItem(this, "QtiSAS", "Select SAVED session:", lst, 0, false, &ok);
        if (!ok)
            return;
    }

    newSession();

    comboBoxMakeScriptTable->setCurrentIndex(comboBoxMakeScriptTable->findText(scriptName));

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

    if (index > 0 && !QDir(filesManager->pathInString()).exists())
    {
	sansTab->setCurrentIndex(0);
	index = 0;
	QMessageBox::warning(this,tr("qtiSAS"), tr("Select correct \"Input Folder\"!"));
    }  
    
    if ( index==1 )
    {
	QString activeTable=comboBoxInfoTable->currentText();
	
        QStringList infoTablesList = app()->findTableListByLabel("Info::Table");
	infoTablesList.sort();
	
	comboBoxInfoTable->clear();	
	comboBoxInfoTable->insertItems(0, infoTablesList);
	comboBoxInfoTable->setCurrentIndex(infoTablesList.indexOf(activeTable));	
	
	
	QString activeMatrix=comboBoxInfoMatrix->currentText();

        QStringList infoMatrixList = app()->findMatrixListByLabel("[1,1]");
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
        QStringList lst0 = app()->findMatrixListByLabel("DAN::Mask::" + QString::number(MD));
        QStringList lst = lst0;

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
        lst = app()->findMatrixListByLabel("DAN::Sensitivity::" + QString::number(MD));

        if (!lst.contains("sens"))
            lst.prepend("sens");

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
//+++ Instrument Selected
void dan18::instrumentSelected(QString instrName)
{
    bool inFocus = instrName.isEmpty();

    int index = comboBoxInstrument->findText(instrName);
    if (instrName.isEmpty() || index < 0)
        instrName = comboBoxInstrument->currentText();
    else
        comboBoxInstrument->setCurrentIndex(index);

    imageOffsetX->setValue(0.0);
    imageOffsetY->setValue(0.0);

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

    checkBoxResoCAround->setChecked(false);
    checkBoxResoSAround->setChecked(false);

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

    checkBoxTranspose->setChecked(false);
    checkBoxMatrixX2mX->setChecked(false);
    checkBoxMatrixY2mY->setChecked(false);

    checkBoxParallax->setChecked(true);
    checkBoxParallaxTr->setChecked(true);
    comboBoxParallax->setCurrentIndex(0);

    QStringList lst;

    // +++ optional dead time models for kws instruments ...
    comboBoxDTtype->setVisible(instrName == "KWS1" || instrName == "KWS2");
    comboBoxDTtype->setCurrentIndex(0);

    if (instrName == "KWS1")
    {
        spinBoxWLS->setValue(10);
        lst = InstrumentSANS::KWS1();
    }
    else if (instrName == "KWS1-2020")
    {
        spinBoxWLS->setValue(20);
        lst = InstrumentSANS::KWS1_2020();
    }
    else if (instrName == "KWS1-He3")
    {
        spinBoxWLS->setValue(20);
        lst = InstrumentSANS::KWS1_HE3();
    }
    else if (instrName == "KWS2")
    {
        spinBoxWLS->setValue(20);
        lst = InstrumentSANS::KWS2();
    }
    else if (instrName == "KWS2-He3-20%")
    {
        spinBoxWLS->setValue(20);
        lst = InstrumentSANS::KWS2_HE3_20();
    }
    else if (instrName == "KWS2-He3-10%")
    {
        spinBoxWLS->setValue(20);
        lst = InstrumentSANS::KWS2_He3_10();
    }
    else if (instrName == "KWS2-HRD")
    {
        spinBoxWLS->setValue(20);
        lst = InstrumentSANS::KWS2_HRD();
    }
    else if (instrName == "KWS3-2021")
    {
        lst = InstrumentSANS::KWS3_2021();
    }
    else if (instrName == "KWS3-VHRD-2021")
    {
        lst = InstrumentSANS::KWS3_VHRD_2021();
    }
    else if (instrName == "KWS3")
    {
        lst = InstrumentSANS::KWS3();
    }
    else if (instrName == "kws3-2016")
    {
        lst = InstrumentSANS::KWS3_2016();
    }
    else if (instrName == "kws3-2017-nicos")
    {
        lst = InstrumentSANS::KWS3_2017_NICOS();
    }
    else if (instrName == "KWS3-2020")
    {
        lst = InstrumentSANS::KWS3_2020();
    }
    else if (instrName == "KWS3-2018")
    {
        lst = InstrumentSANS::KWS3_2018();
    }
    else if (instrName == "KWS3-VHRD")
    {
        lst = InstrumentSANS::KWS3_VHRD();
    }
    else if (instrName == "KWS3-VHRD-2020")
    {
        lst = InstrumentSANS::KWS3_VHRD_2020();
    }
    else if (instrName == "KWS3-VHRD-2018")
    {
        lst = InstrumentSANS::KWS3_VHRD_2018();
    }
    else if (instrName == "MARIA")
    {
        lst = InstrumentSANS::MARIA();
    }
    else if (instrName == "SANS1")
    {
        lst = InstrumentSANS::SANS1();
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
            QMessageBox::warning(this, "Could not read file:: " + instrName + ".SANS", "QtiSAS::DAN");
            return;
        }

        QTextStream t(&f);
        while(!t.atEnd())
            lst << t.readLine();

        f.close();
    }

    for (int i = 0; i < lst.count(); i++)
    {
        if (lst[i].contains("[Info]"))
            lst[i] = lst[i].replace("[Info]", "[Sample-Title]");
        else if (lst[i].contains("[Sample-Nr]"))
            lst[i] = lst[i].replace("[Sample-Nr]", "[Sample-Position-Number]");
        else if (lst[i].contains("[DeltaLambda]"))
            lst[i] = lst[i].replace("[DeltaLambda]", "[Delta-Lambda]");
        else if (lst[i].contains("[Offset]"))
            lst[i] = lst[i].replace("[Offset]", "[C,D-Offset]");
        else if (lst[i].contains("[Time-Factor]"))
            lst[i] = lst[i].replace("[Time-Factor]", "[Duration-Factor]");
        else if (lst[i].contains("[Thickness]"))
            lst[i] = lst[i].replace("[Thickness]", "[Sample-Thickness]");
        else if (lst[i].contains("[Run]"))
            lst[i] = lst[i].replace("[Run]", "[Sample-Run-Number]");
        else if (lst[i].contains("[Who]"))
            lst[i] = lst[i].replace("[Who]", "[User-Name]");
        else if (lst[i].contains("[Tiff-Data]"))
            lst[i] = lst[i].replace("[Tiff-Data]", "[Image-Data]");
    }

    QStringList stringList;
    QString line, selectPattern;
    int binning = 0;
    int DD = 0;
    int RoI = 0;
    int md;

    selectPattern = "";

    for (int i = 0; i < lst.count(); i++)
    {
        //+++ header :: map
        for (int j = 0; j < parserHeader->listOfHeaders.count(); j++)
            if (lst[i].contains(parserHeader->listOfHeaders[j]))
            {
                lst[i] = lst[i].remove(parserHeader->listOfHeaders[j]).simplified();
                int pos = lst[i].indexOf(";;;");
                tableHeaderPosNew->item(j, 0)->setText(lst[i].left(pos));
                tableHeaderPosNew->item(j, 1)->setText(lst[i].right(lst[i].length() - 3 - pos));
                break;
            }

        stringList = lst[i].split("] ");
        stringList[0].remove('[');
        line = (stringList.size() > 1) ? stringList[1].simplified() : " ";

        switch (InstrumentSANS::stringToEnumMap[stringList[0]])
        {
        case InstrumentSANS::Parameter::Instrument:
            break;
        case InstrumentSANS::Parameter::Instrument_Mode:
            comboBoxMode->setCurrentIndex(comboBoxMode->findText(line));
            break;
        case InstrumentSANS::Parameter::DataFormat:
            parserHeader->dataFormatChanged(line.toInt());
            break;
        case InstrumentSANS::Parameter::Color:
            pushButtonInstrColor->setStyleSheet("background-color: " + line + ";");
            pushButtonInstrLabel->setStyleSheet("background-color: " + line + ";");
            break;
        case InstrumentSANS::Parameter::Input_Folder:
            if (line.left(4) != "home")
                lineEditPathDAT->setText(line);
            break;
        case InstrumentSANS::Parameter::Include_Sub_Foldes:
            checkBoxDirsIndir->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Output_Folder:
            if (line.left(4) != "home")
                lineEditPathRAD->setText(line);
            break;
        case InstrumentSANS::Parameter::Units_Lambda:
            comboBoxUnitsLambda->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Units_Appertures:
            comboBoxUnitsBlends->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Units_Thickness:
            comboBoxThicknessUnits->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Units_Time:
            comboBoxUnitsTime->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Units_Time_RT:
            comboBoxUnitsTimeRT->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Units_C:
            comboBoxUnitsC->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Units_C_D_Offset:
            comboBoxUnitsD->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Units_Selector:
            comboBoxUnitsSelector->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Second_Header_OK:
            checkBoxYes2ndHeader->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Second_Header_Pattern:
            lineEditWildCard2ndHeader->setText(line);
            break;
        case InstrumentSANS::Parameter::Second_Header_Lines:
            spinBoxHeaderNumberLines2ndHeader->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Data_Header_Lines:
            spinBoxDataHeaderNumberLines->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Lines_Between_Frames:
            spinBoxDataLinesBetweenFrames->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Pattern:
            lineEditWildCard->setText(line);
            textEditPattern->setText(line.replace("#", "*").replace("**", "*"));
            break;
        case InstrumentSANS::Parameter::Pattern_Select_Data:
            selectPattern = line;
            break;
        case InstrumentSANS::Parameter::Header_Number_Lines:
            spinBoxHeaderNumberLines->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Flexible_Header:
            checkBoxHeaderFlexibility->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Flexible_Stop:
            lineEditFlexiStop->setText(line);
            break;
        case InstrumentSANS::Parameter::Remove_None_Printable_Symbols:
            checkBoxRemoveNonePrint->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Image_Offset_X:
            imageOffsetX->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Image_Offset_Y:
            imageOffsetY->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Image_Data:
            if (line.contains("Yes"))
            {
                radioButtonDetectorFormatImage->setChecked(true);
                radioButtonDetectorFormatHDF->setChecked(false);
                radioButtonDetectorFormatYAML->setChecked(false);
                radioButtonDetectorFormatAscii->setChecked(false);
            }
            break;
        case InstrumentSANS::Parameter::HDF_Data:
            if (line.contains("Yes"))
            {
                radioButtonDetectorFormatHDF->setChecked(true);
                radioButtonDetectorFormatYAML->setChecked(false);
                radioButtonDetectorFormatImage->setChecked(false);
                radioButtonDetectorFormatAscii->setChecked(false);
            }
            break;
        case InstrumentSANS::Parameter::YAML_Data:
            if (line.contains("Yes"))
            {
                radioButtonDetectorFormatYAML->setChecked(true);
                radioButtonDetectorFormatHDF->setChecked(false);
                radioButtonDetectorFormatImage->setChecked(false);
                radioButtonDetectorFormatAscii->setChecked(false);
            }
            break;
        case InstrumentSANS::Parameter::HDF_detector_entry:
            lineEditHdfDetectorEntry->setText(line);
            break;
        case InstrumentSANS::Parameter::HDF_data_structure:
            comboBoxDxDyN->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::YAML_detector_entry:
            lineEditYamlDetectorEntry->setText(line);
            break;
        case InstrumentSANS::Parameter::XML_base:
            lineEditXMLbase->setText(line);
            break;
        case InstrumentSANS::Parameter::Selector_Read_from_Header:
            radioButtonLambdaHeader->setChecked(line.contains("Yes"));
            radioButtonLambdaF->setChecked(!line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Selector_P1:
            lineEditSel1->setText(line);
            break;
        case InstrumentSANS::Parameter::Selector_P2:
            lineEditSel2->setText(line);
            break;
        case InstrumentSANS::Parameter::Detector_Data_Dimension:
            DD = line.toInt();
            break;
        case InstrumentSANS::Parameter::Detector_Data_Focus:
            RoI = line.toInt();
            break;
        case InstrumentSANS::Parameter::Detector_Binning:
            binning = line.toInt();
            md = RoI / comboBoxBinning->itemText(binning).toInt();
            break;
        case InstrumentSANS::Parameter::Detector_Pixel_Size:
            lineEditResoPixelSize->setText(line);
            break;
        case InstrumentSANS::Parameter::Detector_Pixel_Size_Asymetry:
            lineEditAsymetry->setText(line);
            break;
        case InstrumentSANS::Parameter::Detector_Data_Numbers_Per_Line:
            spinBoxReadMatrixNumberPerLine->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Detector_Data_Tof_Numbers_Per_Line:
            spinBoxReadMatrixTofNumberPerLine->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Detector_Data_Transpose:
            checkBoxTranspose->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Detector_X_to_Minus_X:
            checkBoxMatrixX2mX->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Detector_Y_to_Minus_Y:
            checkBoxMatrixY2mY->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Detector_Dead_Time:
            lineEditDeadTime->setText(line);
            break;
        case InstrumentSANS::Parameter::Detector_Dead_Time_DB:
            lineEditDBdeadtime->setText(line);
            break;
        case InstrumentSANS::Parameter::Monitor1_Dead_Time:
            lineEditDeadTimeM1->setText(line);
            break;
        case InstrumentSANS::Parameter::Monitor2_Dead_Time:
            lineEditDeadTimeM2->setText(line);
            break;
        case InstrumentSANS::Parameter::Monitor3_Dead_Time:
            lineEditDeadTimeM3->setText(line);
            break;
        case InstrumentSANS::Parameter::Options_2D_DeadTimeModel_NonPara:
            radioButtonDeadTimeCh->setChecked(line.contains("Yes"));
            radioButtonDeadTimeDet->setChecked(!line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_2D_CenterMethod:
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
        case InstrumentSANS::Parameter::DetRotation_X_Read_from_Header:
            radioButtonDetRotHeaderX->setChecked(line.contains("Yes"));
            radioButtonDetRotConstX->setChecked(!line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::DetRotation_Angle_X:
            doubleSpinBoxDetRotX->setValue(line.toDouble());
            break;
        case InstrumentSANS::Parameter::DetRotation_Invert_Angle_X:
            checkBoxInvDetRotX->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::DetRotation_Y_Read_from_Header:
            radioButtonDetRotHeaderY->setChecked(line.contains("Yes"));
            radioButtonDetRotConstY->setChecked(!line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::DetRotation_Angle_Y:
            doubleSpinBoxDetRotY->setValue(line.toDouble());
            break;
        case InstrumentSANS::Parameter::DetRotation_Invert_Angle_Y:
            checkBoxInvDetRotY->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Calibrant_Type:
            if (line.contains("Direct Beam"))
                comboBoxACmethod->setCurrentIndex(1);
            else if (line.contains("Flat Scatter + Transmission"))
                comboBoxACmethod->setCurrentIndex(2);
            else if (line.contains("Counts per Channel"))
                comboBoxACmethod->setCurrentIndex(3);
            else
                comboBoxACmethod->setCurrentIndex(0);
            break;
        case InstrumentSANS::Parameter::Calibrant: {
            QStringList lst;
            for (int i = 0; i < comboBoxCalibrant->count(); i++)
                lst << comboBoxCalibrant->itemText(i);
            if (lst.contains(line))
                comboBoxCalibrant->setCurrentIndex(lst.indexOf(line));
            calibrantselected();
        }
            break;
        case InstrumentSANS::Parameter::Use_Active_Mask_and_Sensitivity_Matrixes:
            checkBoxACDBuseActive->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Calculate_Calibrant_Transmission_by_Equation:
            checkBoxTransmissionPlexi->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Mask_Edge_Shape:
            if (line.contains("Rectangle"))
                comboBoxMaskEdgeShape->setCurrentIndex(0);
            else
                comboBoxMaskEdgeShape->setCurrentIndex(1);
            break;
        case InstrumentSANS::Parameter::Mask_BeamStop_Shape:
            if (line.contains("Rectangle"))
                comboBoxMaskBeamstopShape->setCurrentIndex(0);
            else
                comboBoxMaskBeamstopShape->setCurrentIndex(1);
            break;
        case InstrumentSANS::Parameter::Mask_Edge:
            groupBoxMask->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Mask_BeamStop:
            groupBoxMaskBS->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Mask_Edge_Left_X:
            spinBoxLTx->setMaximum(md);
            spinBoxLTx->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Mask_Edge_Left_Y:
            spinBoxLTy->setMaximum(md);
            spinBoxLTy->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Mask_Edge_Right_X:
            spinBoxRBx->setMaximum(md + 51);
            spinBoxRBx->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Mask_Edge_Right_Y:
            spinBoxRBy->setMaximum(md + 51);
            spinBoxRBy->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Mask_BeamStop_Left_X:
            spinBoxLTxBS->setMaximum(md);
            spinBoxLTxBS->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Mask_BeamStop_Left_Y:
            spinBoxLTyBS->setMaximum(md);
            spinBoxLTyBS->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Mask_BeamStop_Right_X:
            spinBoxRBxBS->setMaximum(md);
            spinBoxRBxBS->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Mask_BeamStop_Right_Y:
            spinBoxRByBS->setMaximum(md);
            spinBoxRByBS->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Mask_Dead_Rows:
            lineEditDeadRows->setText(line);
            break;
        case InstrumentSANS::Parameter::Mask_Dead_Cols:
            lineEditDeadCols->setText(line);
            break;
        case InstrumentSANS::Parameter::Mask_Triangular:
            lineEditMaskPolygons->setText(line);
            break;
        case InstrumentSANS::Parameter::Sensitivity_SpinBoxErrRightLimit:
            spinBoxErrRightLimit->setValue(line.toDouble());
            break;
        case InstrumentSANS::Parameter::Sensitivity_SpinBoxErrLeftLimit:
            spinBoxErrLeftLimit->setValue(line.toDouble());
            break;
        case InstrumentSANS::Parameter::Sensitivity_CheckBoxSensError:
            checkBoxSensError->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Sensitivity_Tr_Option:
            checkBoxSensTr->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Sensitivity_in_Use:
            buttonGroupSensanyD->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Sensitivity_Masked_Pixels_Value:
            lineEditSensMaskedPixels->setText(line);
            break;
        case InstrumentSANS::Parameter::Transmission_Method:
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
        case InstrumentSANS::Parameter::Options_2D_HighQ:
            checkBoxParallax->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_2D_HighQ_Parallax_Type:
            comboBoxParallax->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_2D_HighQ_Tr:
            checkBoxParallaxTr->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_2D_Polar_Resolusion:
            spinBoxPolar->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_2D_Mask_Negative_Points:
            checkBoxMaskNegative->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_2D_Normalization_Type:
            comboBoxNorm->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_2D_Normalization_Factor:
            spinBoxNorm->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_2D_Mask_Normalization_BC:
            checkBoxBCTimeNormalization->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_2D_xyDimension_Pixel:
            radioButtonXYdimPixel->setChecked(line.contains("Yes"));
            radioButtonXYdimQ->setChecked(!line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_2D_Output_Format:
            comboBoxIxyFormat->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_2D_Header_Output_Format:
            checkBoxASCIIheaderIxy->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_2D_Header_SASVIEW:
            checkBoxASCIIheaderSASVIEW->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_1D_RADmethod_HF:
            radioButtonRadHF->setChecked(line.contains("Yes"));
            radioButtonRadStd->setChecked(!line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_1D_RAD_LinearFactor:
            spinBoxAvlinear->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_1D_RAD_ProgressiveFactor:
            doubleSpinBoxAvLog->setValue(line.toDouble());
            break;
        case InstrumentSANS::Parameter::Options_1D_RemoveFirst:
            spinBoxRemoveFirst->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_1D_RemoveLast:
            spinBoxRemoveLast->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_1D_RemoveNegativePoints:
            checkBoxMaskNegativeQ->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_1D_QxQy_From:
            spinBoxFrom->setMaximum(md);
            spinBoxFrom->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_1D_QxQy_To:
            spinBoxTo->setMaximum(md);
            spinBoxTo->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_1D_QxQy_BS:
            checkBoxSlicesBS->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_1D_OutputFormat:
            comboBox4thCol->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_1D_OutputFormat_PlusHeader:
            checkBoxASCIIheader->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_1D_Anisotropy:
            checkBoxAnisotropy->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Options_1D_AnisotropyAngle:
            spinBoxAnisotropyOffset->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Options_1D_QI_Presentation:
            comboBoxSelectPresentation->setCurrentIndex(line.toInt());
            break;
        case InstrumentSANS::Parameter::Sample_Position_As_Condition:
            checkBoxRecalculateUseNumber->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Attenuator_as_Condition:
            checkBoxAttenuatorAsPara->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Beam_Center_as_Condition:
            checkBoxBeamcenterAsPara->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Polarization_as_Condition:
            checkBoxPolarizationAsPara->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::DetectorAngle_as_Condition:
            checkBoxDetRotAsPara->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Reread_Existing_Runs:
            checkBoxRecalculate->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Find_Center_For_EveryFile:
            checkBoxFindCenter->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Tr_Force_Copy_Paste:
            checkBoxForceCopyPaste->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Sampe_Name_As_RunTableName:
            checkBoxNameAsTableName->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Generate_MergingTable:
            checkBoxMergingTable->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Auto_Merging:
            checkBoxAutoMerging->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Overlap_Merging:
            spinBoxOverlap->setValue(line.toInt());
            break;
        case InstrumentSANS::Parameter::Rewrite_Output:
            checkBoxRewriteOutput->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Skipt_Tr_Configurations:
            checkBoxSkiptransmisionConfigurations->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Skipt_Output_Folders:
            checkBoxSortOutputToFolders->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Resolusion_Focusing:
            checkBoxResoFocus->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Resolusion_Detector:
            lineEditDetReso->setText(line);
            break;
        case InstrumentSANS::Parameter::Resolusion_CA_Round:
            checkBoxResoCAround->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::Resolusion_SA_Round:
            checkBoxResoSAround->setChecked(line.contains("Yes"));
            break;
        case InstrumentSANS::Parameter::File_Ext:
            lineEditFileExt->setText(line);
            break;
        case InstrumentSANS::Parameter::POL_ALIAS_UP:
            lineEditUp->setText(line);
            break;
        case InstrumentSANS::Parameter::POL_ALIAS_DOWN:
            lineEditDown->setText(line);
            break;
        case InstrumentSANS::Parameter::POL_ALIAS_UP_UP:
            lineEditUpUp->setText(line);
            break;
        case InstrumentSANS::Parameter::POL_ALIAS_UP_DOWN:
            lineEditUpDown->setText(line);
            break;
        case InstrumentSANS::Parameter::POL_ALIAS_DOWN_DOWN:
            lineEditDownDown->setText(line);
            break;
        case InstrumentSANS::Parameter::POL_ALIAS_DOWN_UP:
            lineEditDownUp->setText(line);
            break;
        case InstrumentSANS::Parameter::POLARIZATION:
            polarizationSelector->readSettingsString(line);
            break;
        case InstrumentSANS::Parameter::POL_TRANSMISSION:
            polTransmissionSelector->readSettingsString(line);
            break;
        case InstrumentSANS::Parameter::POL_FLIPPER_EFFICIENCY:
            polFlipperEfficiencySelector->readSettingsString(line);
            break;
        case InstrumentSANS::Parameter::ANALYZER_TRANSMISSION:
            analyzerTransmissionSelector->readSettingsString(line);
            break;
        case InstrumentSANS::Parameter::ANALYZER_EFFICIENCY:
            analyzerEfficiencySelector->readSettingsString(line);
            break;
        }
    }
    
    doubleSpinBoxXcenter->setValue((spinBoxLTxBS->value() + spinBoxRBxBS->value()) / 2.0);
    doubleSpinBoxYcenter->setValue((spinBoxLTyBS->value() + spinBoxRByBS->value()) / 2.0);

    if (!selectPattern.isEmpty())
        textEditPattern->setText(selectPattern);

    {
        QSignalBlocker blocker1(lineEditMD);
        QSignalBlocker blocker2(spinBoxRegionOfInteres);
        QSignalBlocker blocker3(comboBoxMDdata);
        QSignalBlocker blocker4(comboBoxBinning);

        comboBoxMDdata->setCurrentIndex(DD);
        dataDimensionChanged(comboBoxMDdata->currentText());

        spinBoxRegionOfInteres->setValue(RoI);
        dataRangeOfInteresChanged(spinBoxRegionOfInteres->value());

        comboBoxBinning->setCurrentIndex(binning);
        binningChanged(comboBoxBinning->currentText());
    }

    pushButtonInstrLabel->setText(comboBoxInstrument->currentText());

    SensitivityLineEditCheck();

    secondHeaderExist(checkBoxYes2ndHeader->isChecked());
    selector->readLambdaFromHeader(radioButtonLambdaHeader->isChecked());

    sasPresentation();

    pushButtonDeleteCurrentInstr->setEnabled(comboBoxInstrument->currentIndex() > 18);

    experimentalModeSelected();

    if (inFocus)
        comboBoxInstrument->setFocus();
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

    //+++ Image-Offset-X
    s += "lst<<\"[Image-Offset-X] " + QString::number(imageOffsetX->value()) + "\";\n";

    //+++ Image-Offset-Y
    s += "lst<<\"[Image-Offset-Y] " + QString::number(imageOffsetY->value()) + "\";\n";

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

    //+++ Image-Offset-X
    s += "[Image-Offset-X] " + QString::number(imageOffsetX->value()) + "\n";

    //+++ Image-Offset-Y
    s += "[Image-Offset-Y] " + QString::number(imageOffsetY->value()) + "\n";

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
    if (!app())
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
    if (!app())
        return;
    
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
void dan18::experimentalModeSelected(QString mode)
{
    if (mode.isEmpty() || comboBoxMode->findText(mode) < 0)
        mode = comboBoxMode->currentText();
    else
        comboBoxMode->setCurrentIndex(comboBoxMode->findText(mode));

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