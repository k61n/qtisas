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

//+++ connect Slots
void dan18::connectSlot()
{
    // top panel buttons
    connect(pushButtonNewSession, &QPushButton::clicked, this, &dan18::newSession);
    connect(pushButtonOpenSession, &QPushButton::clicked, this, [this](bool) { openSession(); });
    connect(pushButtonInstrLabel, &QPushButton::clicked, this, &dan18::instrumentSelectedByButton);

    // instrument buttons
    connect(comboBoxInstrument, QOverload<int>::of(&QComboBox::activated), this, [this](int) { instrumentSelected(); });
    connect(pushButtonsaveCurrentSaveInstr, &QPushButton::clicked, this, &dan18::saveInstrumentAs);
    connect(pushButtonDeleteCurrentInstr, &QPushButton::clicked, this, &dan18::deleteCurrentInstrument);
    connect(pushButtonInstrColor, &QPushButton::clicked, this, &dan18::selectInstrumentColor);

    // experiment mode
    connect(comboBoxMode, QOverload<int>::of(&QComboBox::activated), this,
            [this](int index) { experimentalModeSelected(); });

    // sanstab
    connect(sansTab, &QTabWidget::currentChanged, this, [this](int) { tabSelected(); });

    // line edit mirroring
    connect(lineEditAsymetry, &QLineEdit::textChanged, lineEditAsymetryMatrix, &QLineEdit::setText);
}
//+++
void dan18::initScreenResolusionDependentParameters(int hResolusion, double sasResoScale)
{
#ifdef Q_OS_LINUX
    return;
#endif

    int labelHight = int(hResolusion * sasResoScale / 45);
    int fontIncr = app()->sasFontIncrement + 1;

    int newH = labelHight;
    int newW = int(double(labelHight) / 1.5);
    if (labelHight < 30)
        newH = 30, newW = 15;

    //+++ QToolBox
    foreach (QToolBox *obj, this->findChildren<QToolBox *>())
        if (obj->baseSize().height() > 0)
            obj->setFixedHeight(int(obj->baseSize().height() * sasResoScale));

    //+++ Labels
    foreach (QLabel *obj, this->findChildren<QLabel *>())
        obj->setFixedHeight(labelHight);
    textLabelInfo_2->setMaximumHeight(3000);

    //+++ QLineEdit
    foreach (QLineEdit *obj, this->findChildren<QLineEdit *>())
    {
        obj->setFixedHeight(labelHight);
        obj->setStyleSheet("background-color: rgb(255, 255, 195);");
    }

    //+++ QTextEdit
    textEditPattern->setFixedHeight(labelHight);

    //+++ QPushButton and QToolButton
    for (QAbstractButton *btn : findChildren<QAbstractButton *>())
    {
        const QSize base = btn->baseSize();
        const int bw = base.width();
        const int bh = base.height();
        const bool isPush = qobject_cast<QPushButton *>(btn);
        const bool isTool = qobject_cast<QToolButton *>(btn);

        auto icon = [&](double f) { btn->setIconSize(QSize(int(f * labelHight), int(f * labelHight))); };

        //+++ QPushButton and QToolButton
        if (bw == 25)
        {
            btn->setFixedSize(labelHight, labelHight);
            icon(0.75);
        }
        else if (bw == 14)
        {
            const int s = btn->font().pointSize() + 5;
            btn->setFixedSize(s, s);
        }
        else if (bw == 50)
        {
            btn->setFixedHeight(labelHight);

            if (isPush)
            {
                btn->setFixedWidth(2 * labelHight + 2);
                icon(0.75);
            }
            else if (isTool)
            {
                btn->setFixedWidth(2 * labelHight + 5);
                icon(0.65);
            }
        }
        //+++ QPushButton-specific
        else if (isPush && bw == 10)
        {
            btn->setFixedHeight(newH);
        }
        //+++ QToolButton-specific
        else if (isTool && bw == 75 && bh == 25)
        {
            btn->setFixedSize(3 * labelHight, labelHight);
            icon(0.75);
        }
        else if (isTool && bh == 50)
        {
            btn->setFixedSize(2 * labelHight + 1, 2 * labelHight + 1);
            icon(0.75);
        }
        //+++ default
        else
        {
            btn->setFixedHeight(labelHight);

            if (isTool && btn->sizeIncrement().width() > 0)
            {
                int w = btn->sizeIncrement().width();
                if (fontIncr > 0)
                    w += 2 * fontIncr * int(btn->text().length());

                btn->setFixedWidth(w);
            }
            icon(0.75);
        }
    }

    //+++ QCheckBox
    foreach (QCheckBox *obj, this->findChildren<QCheckBox *>())
        obj->setFixedHeight(labelHight);

    //+++ QSpinBox
    foreach (QSpinBox *obj, this->findChildren<QSpinBox *>())
        obj->setFixedHeight(labelHight);

    //+++ QToolBox
    toolBox->setFixedHeight(2 * labelHight + 25);

#ifdef Q_OS_MACOS
    int comboInc = 5;
#else
    int comboInc = 0;
#endif
    //+++ QComboBox
    foreach (QComboBox *obj, this->findChildren<QComboBox *>())
        obj->setFixedHeight(labelHight + comboInc);
}
//+++ init at the start
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
    extractorInit();

    //+++ Files Manager
    filesManager = new FilesManager(lineEditPathDAT, textEditPattern, checkBoxDirsIndir, pushButtonDATpath,
                                    lineEditPathRAD, pushButtonRADpath, lineEditWildCard, lineEditWildCard2ndHeader,
                                    checkBoxYes2ndHeader, textEditPattern);

    //+++ Header Parser
    parserHeader = new ParserHeader(filesManager, tableHeaderPosNew, comboBoxHeaderFormat, buttonGroupXMLbase,
                                    lineEditXMLbase, buttonGroupFlexibleHeader, checkBoxHeaderFlexibility,
                                    lineEditFlexiStop, spinBoxHeaderNumberLines, spinBoxHeaderNumberLines2ndHeader,
                                    spinBoxDataHeaderNumberLines, checkBoxRemoveNonePrint);

    //+++ Detector-Related-Parameter's Parser
    detector = new Detector(parserHeader, comboBoxUnitsD, radioButtonDetRotHeaderX, doubleSpinBoxDetRotX,
                            checkBoxInvDetRotX, radioButtonDetRotHeaderY, doubleSpinBoxDetRotY, checkBoxInvDetRotY,
                            checkBoxBeamcenterAsPara, checkBoxDetRotAsPara);

    //+++ Collimation-Related-Parameter's Parser
    collimation = new Collimation(parserHeader, comboBoxUnitsC, checkBoxResoCAround, checkBoxResoSAround,
                                  comboBoxUnitsBlends, checkBoxAttenuatorAsPara, checkBoxPolarizationAsPara);

    //+++ Sample-Related-Parameter's Parser
    sample = new Sample(parserHeader, comboBoxThicknessUnits, checkBoxRecalculateUseNumber);

    //+++ Selector-Related-Parameter's Parser
    selector = new Selector(parserHeader, comboBoxUnitsLambda, comboBoxUnitsSelector, radioButtonLambdaF,
                            radioButtonLambdaHeader, lineEditSel1, lineEditSel2);

    //+++ Monitor-Related-Parameter's Parser
    monitors = new Monitors(parserHeader, comboBoxUnitsTime, lineEditDeadTimeM1, lineEditDeadTimeM2, lineEditDeadTimeM2,
                            lineEditDeadTime, lineEditDBdeadtime, radioButtonDeadTimeDet, spinBoxNorm, comboBoxNorm);

    //+++ TOF/RT-Related-Parameter's Parser
    tofrt = new Tofrt(parserHeader, comboBoxUnitsTimeRT);

    //+++ CONFIGURATION / Polarization Widget
    polarizationSelector = new ConfigurationSelector(parserHeader, radioButtonPolarizerConst, radioButtonPolarizerTable,
                                                     radioButtonPolarizerHeader, doubleSpinPolarization,
                                                     lineEditPolarizationTable, "Polarizer-Polarization");

    //+++ CONFIGURATION / Polarization Transmission Widget
    polTransmissionSelector =
        new ConfigurationSelector(parserHeader, radioButtonPolTransmissionConst, radioButtonPolTransmissionTable,
                                  radioButtonPolTransmissionHeader, doubleSpinPolTransmission,
                                  lineEditPolTransmissionTable, "Polarizer-Transmission");

    //+++ CONFIGURATION / Polarization Flipper Efficiency Widget
    polFlipperEfficiencySelector = new ConfigurationSelector(
        parserHeader, radioButtonPolFlipperEfficiencyConst, radioButtonPolFlipperEfficiencyTable,
        radioButtonPolFlipperEfficiencyHeader, doubleSpinPolFlipperEfficiency, lineEditPolFlipperEfficiencyTable,
        "Polarizer-Flipper-Efficiency");

    //+++ CONFIGURATION / Analyzer Transmission Widget
    analyzerTransmissionSelector = new ConfigurationSelector(
        parserHeader, radioButtonAnalyzerTransmissionConst, radioButtonAnalyzerTransmissionTable,
        radioButtonAnalyzerTransmissionHeader, doubleSpinAnalyzerTransmission, lineEditAnalyzerTransmissionTable,
        "Analyzer-Transmission");

    //+++ CONFIGURATION / Analyzer Efficiency Widget
    analyzerEfficiencySelector =
        new ConfigurationSelector(parserHeader, radioButtonAnalyzerEfficiencyConst, radioButtonAnalyzerEfficiencyTable,
                                  radioButtonAnalyzerEfficiencyHeader, doubleSpinAnalyzerEfficiency,
                                  lineEditAnalyzerEfficiencyTable, "Analyzer-Efficiency");

    //+++ ScriptTableManager
    scriptTableManager = new ScriptTableManager(comboBoxMode, comboBoxPolReductionMode);

    tableEC->horizontalHeader()->setVisible(true);
    tableEC->verticalHeader()->setVisible(true);

    for (int i = 0; i < tableEC->rowCount(); i++)
        tableEC->setItem(i, 0, new QTableWidgetItem);

    tableEC->setHorizontalHeaderItem(0, new QTableWidgetItem("1"));
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
//+++  New Session
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
//+++  Mode selection
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

void dan18::expandModeSelection(bool YN)
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
    int newInstr = (oldInstr + 1 < numberInstruments) ? oldInstr + 1 : 0;

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
        QMessageBox::warning(this, "QtiSAS", "Select correct \"Input Folder\"!");
    }  

    if (index == 1)
    {
        QString activeTable = comboBoxInfoTable->currentText();
        QStringList infoTablesList = app()->findTableListByLabel("Info::Table");
        infoTablesList.sort();

        comboBoxInfoTable->clear();
        comboBoxInfoTable->insertItems(0, infoTablesList);
        comboBoxInfoTable->setCurrentIndex(int(infoTablesList.indexOf(activeTable)));

        QString activeMatrix = comboBoxInfoMatrix->currentText();

        QStringList infoMatrixList = app()->findMatrixListByLabel("[1,1]");
        infoMatrixList.sort();

        comboBoxInfoMatrix->clear();
        comboBoxInfoMatrix->insertItems(0, infoMatrixList);
        comboBoxInfoMatrix->setCurrentIndex(int(infoMatrixList.indexOf(activeMatrix)));

        updateComboBoxActiveFolders();
        updateComboBoxActiveFile();
    }
    else if (index == 2)
    {
        updateMaskList();
        matrixList();
    }
    else if (index == 3)
    {
        updateSensList();
        updateMaskList();
        SensitivityLineEditCheck();
    }
    else if (index == 4)
    {
        updateSensList();
        updateMaskList();

        //mask
        QStringList lst0 = app()->findMatrixListByLabel("DAN::Mask::" + QString::number(MD));
        QStringList lst = lst0;

        if (!lst.contains("mask")) lst.prepend("mask");
        QString currentMask;

        for (int i = 0; i < tableEC->columnCount(); i++)
        {
            auto *mask = (QComboBoxInTable *)tableEC->cellWidget(dptMASK, i);
            currentMask = mask->currentText();
            mask->clear();
            mask->addItems(lst);
            mask->setCurrentIndex(lst.indexOf(currentMask));
        }

        for (int i = 0; i < tableEC->columnCount(); i++)
        {
            auto *mask = (QComboBoxInTable *)tableEC->cellWidget(dptMASKTR, i);
            currentMask = mask->currentText();
            mask->clear();
            mask->addItems(lst);
            mask->setCurrentIndex(lst.indexOf(currentMask));
        }

        //sens
        lst = app()->findMatrixListByLabel("DAN::Sensitivity::" + QString::number(MD));

        if (!lst.contains("sens"))
            lst.prepend("sens");

        QString currentSens;
        
        for (int i = 0; i < tableEC->columnCount(); i++)
        {
            auto *sens = (QComboBoxInTable *)tableEC->cellWidget(dptSENS, i);
            currentSens = sens->currentText();
            sens->clear();
            sens->addItems(lst);
            sens->setCurrentIndex(lst.indexOf(currentSens));
        }
    }

    sansTab->setFocus();
}

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
    instrPath += "/cpp";
    instrPath = instrPath.replace("//", "/");

    if (!dd.cd(instrPath))
        dd.mkdir(instrPath);

    QString s;
    s = "else if (instrName==\"" + instrName + "\")\n{\n";

    //+++ instrument
    s += "lst<<\"[Instrument] " + instrName + "\";\n";

    //+++ mode
    s += "lst<<\"[Instrument-Mode] " + comboBoxMode->currentText() + "\";\n";

    //+++ data format
    s += "lst<<\"[DataFormat] " + QString::number(comboBoxHeaderFormat->currentIndex()) + "\";\n";

    //+++ color
    s += "lst<<\"[Color] " + pushButtonInstrColor->palette().color(QPalette::Window).name() + "\";\n";

    //+++ data :: input&output folder
    s += "lst<<\"[Input-Folder] home\";\n";
    s += "lst<<\"[Output-Folder] home\";\n";

    //+++ sub folders
    s += QString("lst<<\"[Include-Sub-Foldes] %1\";\n").arg(checkBoxDirsIndir->isChecked() ? "Yes" : "No");

    //+++  units
    s += "lst<<\"[Units-Lambda] " + QString::number(comboBoxUnitsLambda->currentIndex()) + "\";\n";
    s += "lst<<\"[Units-Appertures] " + QString::number(comboBoxUnitsBlends->currentIndex()) + "\";\n";
    s += "lst<<\"[Units-Thickness] " + QString::number(comboBoxThicknessUnits->currentIndex()) + "\";\n";
    s += "lst<<\"[Units-Time] " + QString::number(comboBoxUnitsTime->currentIndex()) + "\";\n";
    s += "lst<<\"[Units-Time-RT] " + QString::number(comboBoxUnitsTimeRT->currentIndex()) + "\";\n";
    s += "lst<<\"[Units-C] " + QString::number(comboBoxUnitsC->currentIndex()) + "\";\n";
    s += "lst<<\"[Units-C-D-Offset] " + QString::number(comboBoxUnitsD->currentIndex()) + "\";\n";
    s += "lst<<\"[Units-Selector] " + QString::number(comboBoxUnitsSelector->currentIndex()) + "\";\n";

    //+++ file(s) :: structure

    //+++ 2ND header
    if (checkBoxYes2ndHeader->isChecked())
        s += "lst<<\"[2nd-Header-OK] Yes\";\n";
    else
        s += "lst<<\"[2nd-Header-OK] No\";\n";
    s += "lst<<\"[2nd-Header-Pattern] " + lineEditWildCard2ndHeader->text() + "\";\n";
    s += "lst<<\"[2nd-Header-Lines] " + QString::number(spinBoxHeaderNumberLines2ndHeader->value()) + "\";\n";

    //+++ 1ND header
    s += "lst<<\"[Pattern] " + lineEditWildCard->text() + "\";\n";
    s += "lst<<\"[Pattern-Select-Data] " + textEditPattern->text() + "\";\n";
    s += "lst<<\"[Header-Number-Lines] " + QString::number(spinBoxHeaderNumberLines->value()) + "\";\n";

    //+++ TOF/RT Headers
    s += "lst<<\"[Data-Header-Lines] " + QString::number(spinBoxDataHeaderNumberLines->value()) + "\";\n";
    s += "lst<<\"[Lines-Between-Frames] " + QString::number(spinBoxDataLinesBetweenFrames->value()) + "\";\n";

    //+++ Flexible-Header
    if (checkBoxHeaderFlexibility->isChecked())
        s += "lst<<\"[Flexible-Header] Yes\";\n";
    else
        s += "lst<<\"[Flexible-Header] No\";\n";

    //+++ Flexible-Stop
    s += "lst<<\"[Flexible-Stop] " + lineEditFlexiStop->text() + "\";\n";

    //+++ Remove-None-Printable-Symbols
    s += QString("lst<<\"[Remove-None-Printable-Symbols] %1\";\n")
             .arg(checkBoxRemoveNonePrint->isChecked() ? "Yes" : "No");

    //+++ Image-Offset-X
    s += "lst<<\"[Image-Offset-X] " + QString::number(imageOffsetX->value()) + "\";\n";

    //+++ Image-Offset-Y
    s += "lst<<\"[Image-Offset-Y] " + QString::number(imageOffsetY->value()) + "\";\n";

    //+++ Image-Data
    s += QString("lst<<\"[Image-Data] %1\";\n").arg(radioButtonDetectorFormatImage->isChecked() ? "Yes" : "No");

    //+++ HDF-Data
    s += QString("lst<<\"[HDF-Data] %1\";\n").arg(radioButtonDetectorFormatHDF->isChecked() ? "Yes" : "No");

    //+++ YAML-Data
    s += QString("lst<<\"[YAML-Data] %1\";\n").arg(radioButtonDetectorFormatYAML->isChecked() ? "Yes" : "No");

    //+++ HDF-detector-entry
    s += "lst<<\"[HDF-detector-entry] " + lineEditHdfDetectorEntry->text() + "\";\n";

    //+++ HDF-data-structure
    s += "lst<<\"[HDF-data-structure] " + QString::number(comboBoxDxDyN->currentIndex()) + "\";\n";

    //+++ YAML-detector-entry
    s += "lst<<\"[YAML-detector-entry] " + lineEditYamlDetectorEntry->text() + "\";\n";

    //+++ XML-base
    s += "lst<<\"[XML-base] " + lineEditXMLbase->text() + "\";\n";

    //+++ header :: map
    for (int i = 0; i < parserHeader->listOfHeaders.count(); i++)
    {
        s += "lst<<\"" + parserHeader->listOfHeaders[i] + " ";
        s += tableHeaderPosNew->item(i, 0)->text() + ";;;";
        s += tableHeaderPosNew->item(i, 1)->text() + "\";\n";
    }
    //+++ selector :: wave length
    s += QString("lst<<\"[Selector-Read-from-Header] %1\";\n").arg(radioButtonLambdaHeader->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Selector-P1] " + lineEditSel1->text() + "\";\n";
    s += "lst<<\"[Selector-P2] " + lineEditSel2->text() + "\";\n";

    //+++ detector :: image
    s += "lst<<\"[Detector-Data-Dimension] " + QString::number(comboBoxMDdata->currentIndex()) + "\";\n";
    s += "lst<<\"[Detector-Data-Focus] " + QString::number(spinBoxRegionOfInteres->value()) + "\";\n";
    s += "lst<<\"[Detector-Binning] " + QString::number(comboBoxBinning->currentIndex()) + "\";\n";
    s += "lst<<\"[Detector-Pixel-Size] " + lineEditResoPixelSize->text() + "\";\n";
    s += "lst<<\"[Detector-Pixel-Size-Asymetry] " + lineEditAsymetry->text() + "\";\n";

    s +=
        "lst<<\"[Detector-Data-Numbers-Per-Line] " + QString::number(spinBoxReadMatrixNumberPerLine->value()) + "\";\n";

    s += "lst<<\"[Detector-Data-Tof-Numbers-Per-Line] " + QString::number(spinBoxReadMatrixTofNumberPerLine->value()) +
         "\";\n";

    s += QString("lst<<\"[Detector-Data-Transpose] %1\";\n").arg(checkBoxTranspose->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Detector-X-to-Minus-X] %1\";\n").arg(checkBoxMatrixX2mX->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Detector-Y-to-Minus-Y] %1\";\n").arg(checkBoxMatrixY2mY->isChecked() ? "Yes" : "No");

    //+++ detector :: dead-time              +
    s += "lst<<\"[Detector-Dead-Time] " + lineEditDeadTime->text() + "\";\n";
    s += "lst<<\"[Detector-Dead-Time-DB] " + lineEditDBdeadtime->text() + "\";\n";

    s += QString("lst<<\"[Options-2D-DeadTimeModel-NonPara] %1\";\n")
             .arg(radioButtonDeadTimeCh->isChecked() ? "Yes" : "No");

    //+++ monitors :: dead-time
    s += "lst<<\"[Monitor1-Dead-Time] " + lineEditDeadTimeM1->text() + "\";\n";
    s += "lst<<\"[Monitor2-Dead-Time] " + lineEditDeadTimeM2->text() + "\";\n";
    s += "lst<<\"[Monitor3-Dead-Time] " + lineEditDeadTimeM3->text() + "\";\n";

    //+++ detector :: center
    if (radioButtonCenterHF->isChecked())
        s += "lst<<\"[Options-2D-CenterMethod] HF\";\n";
    else if (radioButtonRadStdSymm->isChecked())
        s += "lst<<\"[Options-2D-CenterMethod] SYM\";\n";
    else
        s += "lst<<\"[Options-2D-CenterMethod] Header\";\n";

    //+++ detector :: rotation :: x
    s += QString("lst<<\"[DetRotation-X-Read-from-Header] %1\";\n")
             .arg(radioButtonDetRotHeaderX->isChecked() ? "Yes" : "No");

    s += "lst<<\"[DetRotation-Angle-X] " + QString::number(doubleSpinBoxDetRotX->value(), 'f', 2) + "\";\n";

    s += QString("lst<<\"[DetRotation-Invert-Angle-X] %1\";\n").arg(checkBoxInvDetRotX->isChecked() ? "Yes" : "No");

    //+++ detector :: rotation :: y
    s += QString("lst<<\"[DetRotation-Y-Read-from-Header] %1\";\n")
             .arg(radioButtonDetRotHeaderY->isChecked() ? "Yes" : "No");

    s += "lst<<\"[DetRotation-Angle-Y] " + QString::number(doubleSpinBoxDetRotY->value(), 'f', 2) + "\";\n";

    s += QString("lst<<\"[DetRotation-Invert-Angle-Y] %1\";\n").arg(checkBoxInvDetRotY->isChecked() ? "Yes" : "No");

    //+++ absolute calibration                +
    s += "lst<<\"[Calibrant-Type] " + comboBoxACmethod->currentText() + "\";\n";

    s += "lst<<\"[Calibrant] " + comboBoxCalibrant->currentText() + "\";\n";

    s += QString("lst<<\"[Use-Active-Mask-and-Sensitivity-Matrixes] %1\";\n")
             .arg(checkBoxACDBuseActive->isChecked() ? "Yes" : "No");

    s += QString("lst<<\"[Calculate-Calibrant-Transmission-by-Equation] %1\";\n")
             .arg(checkBoxTransmissionPlexi->isChecked() ? "Yes" : "No");
    //+++ mask :: options
    s += QString("lst<<\"[Mask-BeamStop] %1\";\n").arg(groupBoxMaskBS->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Mask-Edge] %1\";\n").arg(groupBoxMask->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Mask-Edge-Shape] " + comboBoxMaskEdgeShape->currentText() + "\";\n";
    s += "lst<<\"[Mask-BeamStop-Shape] " + comboBoxMaskBeamstopShape->currentText() + "\";\n";

    s += "lst<<\"[Mask-Edge-Left-X] " + QString::number(spinBoxLTx->value()) + "\";\n";
    s += "lst<<\"[Mask-Edge-Left-Y] " + QString::number(spinBoxLTy->value()) + "\";\n";
    s += "lst<<\"[Mask-Edge-Right-X] " + QString::number(spinBoxRBx->value()) + "\";\n";
    s += "lst<<\"[Mask-Edge-Right-Y] " + QString::number(spinBoxRBy->value()) + "\";\n";
    s += "lst<<\"[Mask-BeamStop-Left-X] " + QString::number(spinBoxLTxBS->value()) + "\";\n";
    s += "lst<<\"[Mask-BeamStop-Left-Y] " + QString::number(spinBoxLTyBS->value()) + "\";\n";
    s += "lst<<\"[Mask-BeamStop-Right-X] " + QString::number(spinBoxRBxBS->value()) + "\";\n";
    s += "lst<<\"[Mask-BeamStop-Right-Y] " + QString::number(spinBoxRByBS->value()) + "\";\n";

    s += "lst<<\"[Mask-Dead-Rows] " + lineEditDeadRows->text() + "\";\n";
    s += "lst<<\"[Mask-Dead-Cols] " + lineEditDeadCols->text() + "\";\n";
    s += "lst<<\"[Mask-Triangular] " + lineEditMaskPolygons->text() + "\";\n";

    //+++ sensitivity :: options               +
    s += "lst<<\"[Sensitivity-SpinBoxErrLeftLimit] " + QString::number(spinBoxErrLeftLimit->value()) + "\";\n";
    s += "lst<<\"[Sensitivity-SpinBoxErrRightLimit] " + QString::number(spinBoxErrRightLimit->value()) + "\";\n";

    s += QString("lst<<\"[Sensitivity-CheckBoxSensError] %1\";\n").arg(checkBoxSensError->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Sensitivity-in-Use] %1\";\n").arg(buttonGroupSensanyD->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Sensitivity-Tr-Option] %1\";\n").arg(checkBoxSensTr->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Sensitivity-Masked-Pixels-Value] " + lineEditSensMaskedPixels->text() + "\";\n";

    //+++ transmission :: method
    s += "lst<<\"[Transmission-Method] " + comboBoxTransmMethod->currentText() + "\";\n";

    //+++ [2D] :: options
    s += QString("lst<<\"[Options-2D-HighQ] %1\";\n").arg(checkBoxParallax->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Options-2D-HighQ-Parallax-Type] " + QString::number(comboBoxParallax->currentIndex()) + "\";\n";

    s += QString("lst<<\"[Options-2D-HighQ-Tr] %1\";\n").arg(checkBoxParallaxTr->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Options-2D-Polar-Resolusion] " + QString::number(spinBoxPolar->value()) + "\";\n";

    s += QString("lst<<\"[Options-2D-Mask-Negative-Points] %1\";\n")
             .arg(checkBoxMaskNegative->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Options-2D-Normalization-Type] " + QString::number(comboBoxNorm->currentIndex()) + "\";\n";
    s += "lst<<\"[Options-2D-Normalization-Factor] " + QString::number(spinBoxNorm->value()) + "\";\n";

    s += QString("lst<<\"[Options-2D-Mask-Normalization-BC] %1\";\n")
             .arg(checkBoxBCTimeNormalization->isChecked() ? "Yes" : "No");

    s +=
        QString("lst<<\"[Options-2D-xyDimension-Pixel] %1\";\n").arg(radioButtonXYdimPixel->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Options-2D-Output-Format] " + QString::number(comboBoxIxyFormat->currentIndex()) + "\";\n";

    s += QString("lst<<\"[Options-2D-Header-Output-Format] %1\";\n")
             .arg(checkBoxASCIIheaderIxy->isChecked() ? "Yes" : "No");

    s += QString("lst<<\"[Options-2D-Header-SASVIEW] %1\";\n")
             .arg(checkBoxASCIIheaderSASVIEW->isChecked() ? "Yes" : "No");

    //+++ [1D] :: options
    s += QString("lst<<\"[Options-1D-RADmethod-HF] %1\";\n").arg(radioButtonRadHF->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Options-1D-RAD-LinearFactor] " + QString::number(spinBoxAvlinear->value()) + "\";\n";
    s += "lst<<\"[Options-1D-RAD-ProgressiveFactor] " + QString::number(doubleSpinBoxAvLog->value()) + "\";\n";
    s += "lst<<\"[Options-1D-RemoveFirst] " + QString::number(spinBoxRemoveFirst->value()) + "\";\n";
    s += "lst<<\"[Options-1D-RemoveLast] " + QString::number(spinBoxRemoveLast->value()) + "\";\n";

    s += QString("lst<<\"[Options-1D-RemoveNegativePoints] %1\";\n")
             .arg(checkBoxMaskNegativeQ->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Options-1D-QxQy-From] " + QString::number(spinBoxFrom->value()) + "\";\n";

    s += "lst<<\"[Options-1D-QxQy-To] " + QString::number(spinBoxTo->value()) + "\";\n";

    s += QString("lst<<\"[Options-1D-QxQy-BS] %1\";\n").arg(checkBoxSlicesBS->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Options-1D-OutputFormat] " + QString::number(comboBox4thCol->currentIndex()) + "\";\n";

    s += QString("lst<<\"[Options-1D-OutputFormat-PlusHeader] %1\";\n")
             .arg(checkBoxASCIIheader->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Options-1D-Anisotropy] %1\";\n").arg(checkBoxAnisotropy->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Options-1D-AnisotropyAngle] " + QString::number(spinBoxAnisotropyOffset->value()) + "\";\n";
    s += "lst<<\"[Options-1D-QI-Presentation] " + QString::number(comboBoxSelectPresentation->currentIndex()) + "\";\n";

    //+++ script table options
    s += QString("lst<<\"[Sample-Position-As-Condition] %1\";\n")
             .arg(checkBoxRecalculateUseNumber->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Attenuator-as-Condition] %1\";\n").arg(checkBoxAttenuatorAsPara->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Beam-Center-as-Condition] %1\";\n").arg(checkBoxBeamcenterAsPara->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Polarization-as-Condition] %1\";\n")
             .arg(checkBoxPolarizationAsPara->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[DetectorAngle-as-Condition] %1\";\n").arg(checkBoxDetRotAsPara->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Reread-Existing-Runs] %1\";\n").arg(checkBoxRecalculate->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Find-Center-For-EveryFile] %1\";\n").arg(checkBoxFindCenter->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Tr-Force-Copy-Paste] %1\";\n").arg(checkBoxForceCopyPaste->isChecked() ? "Yes" : "No");
    s +=
        QString("lst<<\"[Sampe-Name-As-RunTableName] %1\";\n").arg(checkBoxNameAsTableName->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Generate-MergingTable] %1\";\n").arg(checkBoxMergingTable->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Auto-Merging] %1\";\n").arg(checkBoxAutoMerging->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Overlap-Merging] " + QString::number(spinBoxOverlap->value()) + "\";\n";

    s += QString("lst<<\"[Rewrite-Output] %1\";\n").arg(checkBoxRewriteOutput->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Skipt-Tr-Configurations] %1\";\n")
             .arg(checkBoxSkiptransmisionConfigurations->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Skipt-Output-Folders] %1\";\n").arg(checkBoxSortOutputToFolders->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Resolusion-Focusing] %1\";\n").arg(checkBoxResoFocus->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Resolusion-CA-Round] %1\";\n").arg(checkBoxResoCAround->isChecked() ? "Yes" : "No");
    s += QString("lst<<\"[Resolusion-SA-Round] %1\";\n").arg(checkBoxResoSAround->isChecked() ? "Yes" : "No");

    s += "lst<<\"[Resolusion-Detector] " + lineEditDetReso->text() + "\";\n";

    s += "lst<<\"[File-Ext] " + lineEditFileExt->text() + "\";\n";

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

    s += "}\n";

    QFile f(instrPath + "/" + instrName + ".cpp");

    if (!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "Could not write to file:: " + instrName + ".cpp", "QtiSAS::DAN");
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
        fileName = QInputDialog::getText(this, "QtiSAS", "Create a SAS Instrument", QLineEdit::Normal, fileName, &ok);
        if (!ok || fileName.isEmpty())
            return;

        if (InstrumentSANS::defaultInstruments.contains(fileName))
        {
            ok = false;
            fileName = "Please do not use STANDARD instrument names";
        }
    }

    saveInstrumentAsCpp(instrPath,fileName);

    QString s;
    //+++ instrument
    s += "[Instrument] " + fileName + "\n";

    //+++ mode
    s += "[Instrument-Mode] " + comboBoxMode->currentText() + "\n";

    //+++ data format
    s += "[DataFormat] " + QString::number(comboBoxHeaderFormat->currentIndex()) + "\n";

    //+++ color
    s += "[Color] " + pushButtonInstrColor->palette().color(QPalette::Window).name() + "\n";

    //+++ data :: input&output folder
    s += "[Input-Folder] " + lineEditPathDAT->text() + "\n";
    s += "[Output-Folder] " + lineEditPathRAD->text() + "\n";

    //+++ sub folders
    s += QString("[Include-Sub-Foldes] %1\n").arg(checkBoxDirsIndir->isChecked() ? "Yes" : "No");

    //+++ units
    s += "[Units-Lambda] " + QString::number(comboBoxUnitsLambda->currentIndex()) + "\n";
    s += "[Units-Appertures] " + QString::number(comboBoxUnitsBlends->currentIndex()) + "\n";
    s += "[Units-Thickness] " + QString::number(comboBoxThicknessUnits->currentIndex()) + "\n";
    s += "[Units-Time] " + QString::number(comboBoxUnitsTime->currentIndex()) + "\n";
    s += "[Units-Time-RT] " + QString::number(comboBoxUnitsTimeRT->currentIndex()) + "\n";
    s += "[Units-C] " + QString::number(comboBoxUnitsC->currentIndex()) + "\n";
    s += "[Units-C-D-Offset] " + QString::number(comboBoxUnitsD->currentIndex()) + "\n";
    s += "[Units-Selector] " + QString::number(comboBoxUnitsSelector->currentIndex()) + "\n";

    //+++ file(s) :: structure

    //+++ 2ND header
    s += QString("[2nd-Header-OK] %1\n").arg(checkBoxYes2ndHeader->isChecked() ? "Yes" : "No");
    s += "[2nd-Header-Pattern] " + lineEditWildCard2ndHeader->text() + "\n";
    s += "[2nd-Header-Lines] " + QString::number(spinBoxHeaderNumberLines2ndHeader->value()) + "\n";

    //+++ 1ND header
    s += "[Pattern] " + lineEditWildCard->text() + "\n";
    s += "[Pattern-Select-Data] " + textEditPattern->text() + "\n";
    s += "[Header-Number-Lines] " + QString::number(spinBoxHeaderNumberLines->value()) + "\n";

    //+++ TOF/RT Headers
    s += "[Data-Header-Lines] " + QString::number(spinBoxDataHeaderNumberLines->value()) + "\n";
    s += "[Lines-Between-Frames] " + QString::number(spinBoxDataLinesBetweenFrames->value()) + "\n";

    //+++ Flexible-Header
    s += QString("[Flexible-Header] %1\n").arg(checkBoxHeaderFlexibility->isChecked() ? "Yes" : "No");

    //+++ Flexible-Stop
    s += "[Flexible-Stop] " + lineEditFlexiStop->text() + "\n";

    //+++ Remove-None-Printable-Symbols
    s += QString("[Remove-None-Printable-Symbols] %1\n").arg(checkBoxRemoveNonePrint->isChecked() ? "Yes" : "No");

    //+++ Image-Offset-X
    s += "[Image-Offset-X] " + QString::number(imageOffsetX->value()) + "\n";

    //+++ Image-Offset-Y
    s += "[Image-Offset-Y] " + QString::number(imageOffsetY->value()) + "\n";

    //+++ Image-Data
    s += QString("[Image-Data] %1\n").arg(radioButtonDetectorFormatImage->isChecked() ? "Yes" : "No");

    //+++ HDF-Data
    s += QString("[HDF-Data] %1\n").arg(radioButtonDetectorFormatHDF->isChecked() ? "Yes" : "No");

    //+++ YAML-Data
    s += QString("[YAML-Data] %1\n").arg(radioButtonDetectorFormatYAML->isChecked() ? "Yes" : "No");

    //+++ HDF-detector-entry
    s += "[HDF-detector-entry] " + lineEditHdfDetectorEntry->text() + "\n";

    //+++ HDF-data-structure
    s += "[HDF-data-structure] " + QString::number(comboBoxDxDyN->currentIndex()) + "\n";

    //+++ YAML-detector-entry
    s += "[YAML-detector-entry] " + lineEditYamlDetectorEntry->text() + "\n";

    //+++ XML-base
    s += "[XML-base] " + lineEditXMLbase->text() + "\n";

    //+++ header :: map                        +
    for (int i = 0; i < parserHeader->listOfHeaders.count(); i++)
    {
        s += parserHeader->listOfHeaders[i] + " ";
        s += tableHeaderPosNew->item(i, 0)->text() + ";;;";
        s += tableHeaderPosNew->item(i, 1)->text() + "\n";
    }
    //+++ selector :: wave length           +
    s += QString("[Selector-Read-from-Header] %1\n").arg(radioButtonLambdaHeader->isChecked() ? "Yes" : "No");
    s += "[Selector-P1] " + lineEditSel1->text() + "\n";
    s += "[Selector-P2] " + lineEditSel2->text() + "\n";

    //+++ detector :: image                    +
    s += "[Detector-Data-Dimension] " + QString::number(comboBoxMDdata->currentIndex()) + "\n";
    s += "[Detector-Data-Focus] " + QString::number(spinBoxRegionOfInteres->value()) + "\n";
    s += "[Detector-Binning] " + QString::number(comboBoxBinning->currentIndex()) + "\n";
    s += "[Detector-Pixel-Size] " + lineEditResoPixelSize->text() + "\n";
    s += "[Detector-Pixel-Size-Asymetry] " + lineEditAsymetry->text() + "\n";
    s += "[Detector-Data-Numbers-Per-Line] " + QString::number(spinBoxReadMatrixNumberPerLine->value()) + "\n";
    s += "[Detector-Data-Tof-Numbers-Per-Line] " + QString::number(spinBoxReadMatrixTofNumberPerLine->value()) + "\n";
    s += QString("[Detector-Data-Transpose] %1\n").arg(checkBoxTranspose->isChecked() ? "Yes" : "No");
    s += QString("[Detector-X-to-Minus-X] %1\n").arg(checkBoxMatrixX2mX->isChecked() ? "Yes" : "No");
    s += QString("[Detector-Y-to-Minus-Y] %1\n").arg(checkBoxMatrixY2mY->isChecked() ? "Yes" : "No");

    //+++ detector :: dead-time              +
    s += "[Detector-Dead-Time] " + lineEditDeadTime->text() + "\n";
    s += "[Detector-Dead-Time-DB] " + lineEditDBdeadtime->text() + "\n";
    s += QString("[Options-2D-DeadTimeModel-NonPara] %1\n").arg(radioButtonDeadTimeCh->isChecked() ? "Yes" : "No");

    //+++ monitors :: dead-time
    s += "[Monitor1-Dead-Time] " + lineEditDeadTimeM1->text() + "\n";
    s += "[Monitor2-Dead-Time] " + lineEditDeadTimeM2->text() + "\n";
    s += "[Monitor3-Dead-Time] " + lineEditDeadTimeM3->text() + "\n";

    //+++ detector :: center                    +
    if (radioButtonCenterHF->isChecked())
        s += "[Options-2D-CenterMethod] HF\n";
    else if (radioButtonRadStdSymm->isChecked())
        s += "[Options-2D-CenterMethod] SYM\n";
    else
        s += "[Options-2D-CenterMethod] Header\n";

    //+++ detector :: rotation :: x
    s += QString("[DetRotation-X-Read-from-Header] %1\n").arg(radioButtonDetRotHeaderX->isChecked() ? "Yes" : "No");
    s += "[DetRotation-Angle-X] " + QString::number(doubleSpinBoxDetRotX->value(), 'f', 2) + "\n";
    s += QString("[DetRotation-Invert-Angle-X] %1\n").arg(checkBoxInvDetRotX->isChecked() ? "Yes" : "No");

    //+++ detector :: rotation :: y
    s += QString("[DetRotation-Y-Read-from-Header] %1\n").arg(radioButtonDetRotHeaderY->isChecked() ? "Yes" : "No");
    s += "[DetRotation-Angle-Y] " + QString::number(doubleSpinBoxDetRotY->value(), 'f', 2) + "\n";
    s += QString("[DetRotation-Invert-Angle-Y] %1\n").arg(checkBoxInvDetRotY->isChecked() ? "Yes" : "No");

    //+++ absolute calibration
    s += "[Calibrant-Type] " + comboBoxACmethod->currentText() + "\n";
    s += "[Calibrant] " + comboBoxCalibrant->currentText() + "\n";
    s += QString("[Use-Active-Mask-and-Sensitivity-Matrixes] %1\n")
             .arg(checkBoxACDBuseActive->isChecked() ? "Yes" : "No");
    s += QString("[Calculate-Calibrant-Transmission-by-Equation] %1\n")
             .arg(checkBoxTransmissionPlexi->isChecked() ? "Yes" : "No");

    //+++ mask :: options
    s += QString("[Mask-BeamStop] %1\n").arg(groupBoxMaskBS->isChecked() ? "Yes" : "No");
    s += QString("[Mask-Edge] %1\n").arg(groupBoxMask->isChecked() ? "Yes" : "No");

    s += "[Mask-Edge-Shape] " + comboBoxMaskEdgeShape->currentText() + "\n";
    s += "[Mask-BeamStop-Shape] " + comboBoxMaskBeamstopShape->currentText() + "\n";
    s += "[Mask-Edge-Left-X] " + QString::number(spinBoxLTx->value()) + "\n";
    s += "[Mask-Edge-Left-Y] " + QString::number(spinBoxLTy->value()) + "\n";
    s += "[Mask-Edge-Right-X] " + QString::number(spinBoxRBx->value()) + "\n";
    s += "[Mask-Edge-Right-Y] " + QString::number(spinBoxRBy->value()) + "\n";
    s += "[Mask-BeamStop-Left-X] " + QString::number(spinBoxLTxBS->value()) + "\n";
    s += "[Mask-BeamStop-Left-Y] " + QString::number(spinBoxLTyBS->value()) + "\n";
    s += "[Mask-BeamStop-Right-X] " + QString::number(spinBoxRBxBS->value()) + "\n";
    s += "[Mask-BeamStop-Right-Y] " + QString::number(spinBoxRByBS->value()) + "\n";
    s += "[Mask-Dead-Rows] " + lineEditDeadRows->text() + "\n";
    s += "[Mask-Dead-Cols] " + lineEditDeadCols->text() + "\n";
    s += "[Mask-Triangular] " + lineEditMaskPolygons->text() + "\n";

    //+++ sensitivity :: options               +
    s += "[Sensitivity-SpinBoxErrLeftLimit] " + QString::number(spinBoxErrLeftLimit->value()) + "\n";
    s += "[Sensitivity-SpinBoxErrRightLimit] " + QString::number(spinBoxErrRightLimit->value()) + "\n";
    s += QString("[Sensitivity-CheckBoxSensError] %1\n").arg(checkBoxSensError->isChecked() ? "Yes" : "No");
    s += QString("[Sensitivity-in-Use] %1\n").arg(buttonGroupSensanyD->isChecked() ? "Yes" : "No");
    s += QString("[Sensitivity-Tr-Option] %1\n").arg(checkBoxSensTr->isChecked() ? "Yes" : "No");
    s += "[Sensitivity-Masked-Pixels-Value] " + lineEditSensMaskedPixels->text() + "\n";

    //+++ transmission :: method          +
    s += "[Transmission-Method] " + comboBoxTransmMethod->currentText() + "\n";

    //+++ [2D] :: options
    s += QString("[Options-2D-HighQ] %1\n").arg(checkBoxParallax->isChecked() ? "Yes" : "No");
    s += "[Options-2D-HighQ-Parallax-Type] " + QString::number(comboBoxParallax->currentIndex()) + "\n";
    s += QString("[Options-2D-HighQ-Tr] %1\n").arg(checkBoxParallaxTr->isChecked() ? "Yes" : "No");
    s += "[Options-2D-Polar-Resolusion] " + QString::number(spinBoxPolar->value()) + "\n";
    s += QString("[Options-2D-Mask-Negative-Points] %1\n").arg(checkBoxMaskNegative->isChecked() ? "Yes" : "No");
    s += "[Options-2D-Normalization-Type] " + QString::number(comboBoxNorm->currentIndex()) + "\n";
    s += "[Options-2D-Normalization-Factor] " + QString::number(spinBoxNorm->value()) + "\n";
    s +=
        QString("[Options-2D-Mask-Normalization-BC] %1\n").arg(checkBoxBCTimeNormalization->isChecked() ? "Yes" : "No");
    s += QString("[Options-2D-xyDimension-Pixel] %1\n").arg(radioButtonXYdimPixel->isChecked() ? "Yes" : "No");
    s += "[Options-2D-Output-Format] " + QString::number(comboBoxIxyFormat->currentIndex()) + "\n";
    s += QString("[Options-2D-Header-Output-Format] %1\n").arg(checkBoxASCIIheaderIxy->isChecked() ? "Yes" : "No");
    s += QString("[Options-2D-Header-SASVIEW] %1\n").arg(checkBoxASCIIheaderSASVIEW->isChecked() ? "Yes" : "No");

    //+++ [1D] :: options                      +
    s += QString("[Options-1D-RADmethod-HF] %1\n").arg(radioButtonRadHF->isChecked() ? "Yes" : "No");
    s += QString("[Options-1D-RAD-LinearFactor] %1\n").arg(spinBoxAvlinear->value());
    s += "[Options-1D-RAD-LinearFactor] " + QString::number(spinBoxAvlinear->value()) + "\n";
    s += "[Options-1D-RAD-ProgressiveFactor] " + QString::number(doubleSpinBoxAvLog->value()) + "\n";
    s += "[Options-1D-RemoveFirst] " + QString::number(spinBoxRemoveFirst->value()) + "\n";
    s += "[Options-1D-RemoveLast] " + QString::number(spinBoxRemoveLast->value()) + "\n";
    s += QString("[Options-1D-RemoveNegativePoints] %1\n").arg(checkBoxMaskNegativeQ->isChecked() ? "Yes" : "No");
    s += "[Options-1D-QxQy-From] " + QString::number(spinBoxFrom->value()) + "\n";
    s += "[Options-1D-QxQy-To] " + QString::number(spinBoxTo->value()) + "\n";
    s += QString("[Options-1D-QxQy-BS] %1\n").arg(checkBoxSlicesBS->isChecked() ? "Yes" : "No");
    s += "[Options-1D-OutputFormat] " + QString::number(comboBox4thCol->currentIndex()) + "\n";
    s += QString("[Options-1D-OutputFormat-PlusHeader] %1\n").arg(checkBoxASCIIheader->isChecked() ? "Yes" : "No");
    s += QString("[Options-1D-Anisotropy] %1\n").arg(checkBoxAnisotropy->isChecked() ? "Yes" : "No");
    s += "[Options-1D-AnisotropyAngle] " + QString::number(spinBoxAnisotropyOffset->value()) + "\n";
    s += "[Options-1D-QI-Presentation] " + QString::number(comboBoxSelectPresentation->currentIndex()) + "\n";

    //+++ script table options
    s += QString("[Sample-Position-As-Condition] %1").arg(checkBoxRecalculateUseNumber->isChecked() ? "Yes" : "No");
    s += QString("[Attenuator-as-Condition] %1").arg(checkBoxAttenuatorAsPara->isChecked() ? "Yes" : "No");
    s += QString("[Beam-Center-as-Condition] %1").arg(checkBoxBeamcenterAsPara->isChecked() ? "Yes" : "No");
    s += QString("[Polarization-as-Condition] %1").arg(checkBoxPolarizationAsPara->isChecked() ? "Yes" : "No");
    s += QString("[DetectorAngle-as-Condition] %1").arg(checkBoxDetRotAsPara->isChecked() ? "Yes" : "No");
    s += QString("[Reread-Existing-Runs] %1").arg(checkBoxRecalculate->isChecked() ? "Yes" : "No");
    s += QString("[Find-Center-For-EveryFile] %1").arg(checkBoxFindCenter->isChecked() ? "Yes" : "No");
    s += QString("[Tr-Force-Copy-Paste] %1").arg(checkBoxForceCopyPaste->isChecked() ? "Yes" : "No");
    s += QString("[Sampe-Name-As-RunTableName] %1").arg(checkBoxNameAsTableName->isChecked() ? "Yes" : "No");
    s += QString("[Generate-MergingTable] %1").arg(checkBoxMergingTable->isChecked() ? "Yes" : "No");
    s += QString("[Auto-Merging] %1").arg(checkBoxAutoMerging->isChecked() ? "Yes" : "No");

    s += "[Overlap-Merging] " + QString::number(spinBoxOverlap->value()) + "\n";

    s += QString("[Rewrite-Output] %1\n").arg(checkBoxRewriteOutput->isChecked() ? "Yes" : "No");
    s += QString("[Skipt-Tr-Configurations] %1\n")
             .arg(checkBoxSkiptransmisionConfigurations->isChecked() ? "Yes" : "No");
    s += QString("[Skipt-Output-Folders] %1\n").arg(checkBoxSortOutputToFolders->isChecked() ? "Yes" : "No");
    s += QString("[Resolusion-Focusing] %1\n").arg(checkBoxResoFocus->isChecked() ? "Yes" : "No");
    s += QString("[Resolusion-CA-Round] %1\n").arg(checkBoxResoCAround->isChecked() ? "Yes" : "No");
    s += QString("[Resolusion-SA-Round] %1\n").arg(checkBoxResoSAround->isChecked() ? "Yes" : "No");

    s += "[Resolusion-Detector] " + lineEditDetReso->text() + "\n";
    s += "[File-Ext] " + lineEditFileExt->text() + "\n";

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

    QFile f(instrPath + "/" + fileName + ".SANS");

    if (!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "Could not write to file:: " + fileName + ".SANS", "QtiSAS::DAN");
        return;
    }	

    QTextStream stream(&f);
    stream << s;
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

    QColor color = QColorDialog::getColor(initialColor, app(), "Select Instrument Color");

    if (color.isValid())
        pushButtonInstrColor->setStyleSheet("background-color: " + color.name() + ";");
}

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