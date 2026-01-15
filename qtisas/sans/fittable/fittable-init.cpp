/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s)'s init functions
 ******************************************************************************/

#include <QPainter>

#include "fittable18.h"
#include "DoubleSpinBox.h"
#include "RangeSelectorTool.h"

//*******************************************
//*  Connect Slot
//*******************************************
void fittable18::connectSlot()
{
    connect( textLabelGroupName, SIGNAL( clicked() ), this, SLOT( scanGroup() ) );
    
    connect(tableCurves, SIGNAL(cellChanged(int,int) ), this, SLOT( tableCurvechanged(int,int) ) );
    connect(comboBoxFitMethod, &QComboBox::textActivated, this, [this](const QString &) { this->algorithmSelected(); });
    connect(comboBoxLevenberg, &QComboBox::textActivated, this, [this](const QString &) { this->levenbergSelected(); });
    connect(comboBoxSpeedControlPoly, &QComboBox::textActivated, this,
            [this](const QString &) { this->speedControlPoly(); });
    connect(comboBoxSpeedControlReso, &QComboBox::textActivated, this,
            [this](const QString &) { this->speedControlReso(); });
    connect(comboBoxPolyFunction, &QComboBox::textActivated, this,
            [this](const QString &) { this->speedControlPoly(); });
    connect(comboBoxResoFunction, &QComboBox::textActivated, this,
            [this](const QString &) { this->speedControlReso(); });
    connect(comboBoxInstrument, &QComboBox::textActivated, this, [this](const QString &) { this->SANSsupportYN(); });

    connect(comboBoxWeightingMethod, SIGNAL( activated(int) ), this, SLOT( weightChanged() ) );
    connect(toolButtonResetLimits, SIGNAL( clicked() ), this, SLOT( initLimits() ) );
    connect(lineEditAbsErr, SIGNAL( editingFinished() ), this, SLOT( lineValidator() ) );
    connect(lineEditRelErr, SIGNAL( editingFinished() ), this, SLOT( lineValidator() ) );
    connect(lineEditTolerance, SIGNAL( editingFinished() ), this, SLOT( lineValidator() ) );
    connect(textLabelRangeFirst, SIGNAL( editingFinished() ), this, SLOT( rangeFirstCheck() ) );
    connect(textLabelRangeLast, SIGNAL( editingFinished() ), this, SLOT( rangeLastCheck() ) );
    connect(pushButtonFitPrev, SIGNAL( pressed() ), this, SLOT( slotStackFitPrev() ) );
    connect(pushButtonFitNext, SIGNAL( pressed() ), this, SLOT( slotStackFitNext() ) );
    connect(checkBoxShowEFIT, SIGNAL( clicked() ), this, SLOT( updateEFunctions() ) );
    connect(pushButtonSaveSession, SIGNAL( clicked() ), this, SLOT( saveFittingSession() ) );
    connect(pushButtonLoadFittingSession, SIGNAL( clicked() ), this, SLOT( readSettingsTable() ) );
    connect(pushButtonIFIT, SIGNAL( clicked() ), this, SLOT( iFit() ) );
    connect(pushButtonIFITadv, SIGNAL( clicked() ), this, SLOT( iFitAdv() ) );
    
//+++ fittable-data
    connect(radioButtonDataShowAll, &QRadioButton::toggled, this, &fittable18::changeDataTableView);
    connect(radioButtonDataOnlyDatasets, &QRadioButton::toggled, this, &fittable18::changeDataTableView);
    connect(radioButtonDataOnlyChecks, &QRadioButton::toggled, this, &fittable18::changeDataTableView);

    connect(radioButtonParaShowAll, &QRadioButton::toggled, this, &fittable18::changeParaTableView);
    connect(radioButtonParaOnlyVary, &QRadioButton::toggled, this, &fittable18::changeParaTableView);
    connect(radioButtonParaValues, &QRadioButton::toggled, this, &fittable18::changeParaTableView);
    connect(radioButtonParaValuesErrors, &QRadioButton::toggled, this, &fittable18::changeParaTableView);

    connect(checkBoxMultiData, SIGNAL( clicked() ), this, SLOT( initMultiParaTable() ) );
    connect(spinBoxNumberCurvesToFit, SIGNAL( valueChanged(int) ), this, SLOT( initMultiParaTable() ) );
    connect(checkBoxSANSsupport, SIGNAL( clicked() ), this, SLOT( SANSsupportYN() ) );
    connect(comboBoxDatasetSim, SIGNAL( activated(int) ), this, SLOT( datasetChangedSim(int) ) );
    
    connect(tablePara->horizontalHeader(), SIGNAL(sectionPressed(int)), this, SLOT(headerPressedTablePara(int)));
    connect(tablePara->verticalHeader(), SIGNAL(sectionPressed(int)), this, SLOT(vertHeaderPressedTablePara(int)));
    connect(tableCurves->verticalHeader(), SIGNAL(sectionPressed(int)), this, SLOT(vertHeaderTableCurves(int)));
    connect(tableCurves->horizontalHeader(), SIGNAL(sectionPressed(int)), this, SLOT(horizHeaderCurves(int)));
    connect(tableMultiFit->horizontalHeader(), SIGNAL(sectionPressed(int)), this, SLOT(headerTableMultiFit(int)));
    connect(tableMultiFit, SIGNAL(itemSelectionChanged()), this, SLOT(selectRowsTableMultiFit()));
    connect(tableMultiFit, SIGNAL( cellChanged(int,int) ), this, SLOT( changedSetToSet(int,int) ) );
    connect(tablePara, SIGNAL( cellChanged(int,int) ), this, SLOT( checkGlobalParameters(int,int) ) );
    
    connect(radioButtonSelectWeight, SIGNAL( toggled(bool) ), this, SLOT( optionSelected()) );
    connect(radioButtonSelectReso, SIGNAL( toggled(bool) ), this, SLOT( optionSelected()) );
    connect(radioButtonSelectPoly, SIGNAL( toggled(bool) ), this, SLOT( optionSelected()) );
    
//+++ fittable-fitting
    connect(pushButtonUndo, SIGNAL( clicked() ), this, SLOT( undo() ) );
    connect(pushButtonRedo, SIGNAL( clicked() ), this, SLOT( redo() ) );
    
    connect(comboBoxPolyFunction, SIGNAL( activated(int) ), this, SLOT( SDchanged(int) ) );
    connect(comboBoxPolyFunction_2, SIGNAL( activated(int) ), this, SLOT( SDchanged(int) ) );
    connect(comboBoxFunction, &QComboBox::textActivated, this, &fittable18::changeFunctionLocal);
    
    connect(pushButtonChiSqr, SIGNAL( clicked() ), this, SLOT( plotSwitcher() ) );
    connect(pushButtonMultiFit, SIGNAL( clicked() ), this, SLOT( fitSwitcher() ) );
    connect(pushButtonINITbefore, SIGNAL( clicked() ), this, SLOT( initParametersBeforeFit() ) );
    connect(pushButtonINITafter, SIGNAL( clicked() ), this, SLOT( initParametersAfterFit() ) );
    
//+++ fittable-simulate
    connect(radioButtonUniform_Q, SIGNAL( toggled(bool) ), this, SLOT( uniformSimulChanged(bool) ) );
    connect(radioButtonSameQrange, SIGNAL( toggled(bool) ), this, SLOT( theSameSimulChanged(bool) ) );
    connect(comboBoxSimQN, SIGNAL( highlighted(int) ), this, SLOT( dataLimitsSimulation(int) ) );

    connect(checkBoxLogStep, SIGNAL( toggled(bool) ), this, SLOT( logStepChanged(bool) ) );
    
    connect(lineEditSetBySetFit, SIGNAL( returnPressed() ), this, SLOT( setBySetFit() ) );
    
    connect(pushButtonPattern, SIGNAL( clicked() ), this, SLOT( selectPattern() ) );
    connect(pushButtonSelectFromTable, SIGNAL( clicked() ), this, SLOT( selectMultyFromTable() ) );
    connect(pushButtonSetBySetFit, SIGNAL( clicked() ), this, SLOT( setBySetFit() ) );
    connect(pushButtonresToLogWindow, SIGNAL( clicked() ), this, SLOT( resToLogWindow() ) );
    connect(pushButtonNewTabRes, SIGNAL( clicked() ), this, SLOT( newTabRes() ) );
    connect(pushButtonNewTabResCol, SIGNAL( clicked() ), this, SLOT( newTabResCol() ) );
    connect(pushButtonresToLogWindowOne, SIGNAL( clicked() ), this, SLOT( resToLogWindowOne() ) );
    connect(pushButtonresToActiveGraph, SIGNAL( clicked() ), this, SLOT( addFitResultToActiveGraph() ) );
    
//+++ cleanning
    connect(pushButtonFitCurveDelete, SIGNAL( clicked() ), this, SLOT( removeFitCurve() ) );
    connect(pushButtonSimulatedCurveDelete, SIGNAL( clicked() ), this, SLOT( removeSimulatedCurve() ) );
    connect(pushButtonGlobalDelete, SIGNAL( clicked() ), this, SLOT( removeGlobal() ) );
    connect(pushButtonSimulateDelete, SIGNAL( clicked() ), this, SLOT( removeSimulatedDatasets() ) );
    connect(pushButtonDeleteCurves, SIGNAL( clicked() ), this, SLOT( removeSimulatedDatasets() ) );
    connect(pushButtonSimulate, SIGNAL( clicked() ), this, SLOT( simulateSwitcher() ) );
    connect(pushButtonSimulateSuperpositional, SIGNAL( clicked() ), this, SLOT( simulateSuperpositional() ) );
    connect(pushButtonSimulateSuperpositionalRes, SIGNAL( clicked() ), this, SLOT( simulateSuperpositional() ) );
    connect(pushButtonSimulateMulti, SIGNAL( clicked() ), this, SLOT( simulateMultifitTables() ) );
    
    connect(tableParaSimulate, SIGNAL( cellChanged(int,int) ), this, SLOT( autoSimulateCurveInSimulations(int,int) ) );
}

void fittable18::changeFixedSizeH(QWidget *obj, int H)
{
    obj->setMinimumSize(0,H);
    obj->setMaximumSize(32767,H);
}

void fittable18::changeFixedSizeHW(QWidget *obj, int H, int W)
{
    obj->setMinimumSize(W,H);
    obj->setMaximumSize(W,H);
}


//*******************************************
//*initScreenResolusionDependentParameters
//*******************************************
void fittable18::initScreenResolusionDependentParameters(int hResolusion, double sasResoScale)
{
    
    int labelHight = int(hResolusion * sasResoScale / 50.0);
    
    //+++ ALL tables :: vertical heeader
    foreach( QTableWidget *obj, this->findChildren< QTableWidget * >( ) )
    {
        //obj->verticalHeader()->setDefaultSectionSize(labelHight+5);
        obj->verticalHeader()->setMinimumWidth(4*labelHight+40);
        obj->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    }
    
    //+++ Labels
    foreach( QLabel *obj, this->findChildren< QLabel * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
        
        //obj->setMinimumWidth(3*labelHight);
    }
    
    //+++ QLineEdit
    foreach( QLineEdit *obj, this->findChildren< QLineEdit * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }
    //+++ QPushButton
    foreach( QPushButton *obj, this->findChildren< QPushButton * >( ) )
    {
        
        if(obj->baseSize().width()==25)
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            obj->setMinimumWidth(labelHight);
            obj->setMaximumWidth(labelHight);
        }
        else
        {
            if(obj->baseSize().width()==14)
            {
                obj->setMaximumWidth(int(0.6*labelHight));
                obj->setMaximumWidth(int(0.6*labelHight));
                obj->setMinimumHeight(int(0.6*labelHight));
                obj->setMaximumHeight(int(0.6*labelHight));
            }
            else
            {
                obj->setMinimumHeight(labelHight);
                obj->setMaximumHeight(labelHight);
            }
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
        else
        {
            if(obj->baseSize().width()==14)
            {
                obj->setMaximumWidth(int(0.6*labelHight));
                obj->setMaximumWidth(int(0.6*labelHight));
                obj->setMinimumHeight(int(0.6*labelHight));
                obj->setMaximumHeight(int(0.6*labelHight));
                
            }
            else
            {
                obj->setMinimumHeight(labelHight);
                obj->setMaximumHeight(labelHight);
                
                obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
            }
        }
        
    }
    //+++ QCheckBox
    foreach( QCheckBox *obj, this->findChildren< QCheckBox * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }
    //+++ QComboBox
    foreach(QComboBox *obj, this->findChildren< QComboBox * >( ) )
    {
        obj->setMinimumHeight(labelHight+5);
        obj->setMaximumHeight(labelHight+5);
    }
    //+++ QSpinBox
    foreach(QSpinBox *obj, this->findChildren< QSpinBox * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }
    
    //+++
    changeFixedSizeH(buttonGroupPrevNext, labelHight+5);
    changeFixedSizeH(buttonGroupNavigator, 2*labelHight+35);
    
    
    changeFixedSizeHW(pushButtonFitPrev, labelHight, labelHight);
    changeFixedSizeHW(pushButtonFitNext, labelHight, labelHight);
    
    pushButtonFitPrev->setIconSize(QSize(labelHight,labelHight));
    pushButtonFitNext->setIconSize(QSize(labelHight,labelHight));

    changeFixedSizeH(pushButtonLoadFittingSession, labelHight + 5);
    changeFixedSizeH(pushButtonSaveSession, labelHight + 5);

    changeFixedSizeH(textLabelGroupName, labelHight + 5);
    changeFixedSizeH(textLabelFunctionName, labelHight + 5);
    changeFixedSizeH(pushButtonIFIT, labelHight + 5);
    changeFixedSizeH(pushButtonIFITadv, labelHight + 5);
    //+++
    textLabelChiLabel->setMinimumWidth(labelHight*3);
    
    //+++ simulation...
    theSameSimulChanged(radioButtonSameQrange->isChecked());
    
    changeFixedSizeH(checkBoxWeightSim, 2*labelHight+15);
    changeFixedSizeH(checkBoxResoSim, 2*labelHight+15);
    changeFixedSizeH(checkBoxPolySim, 2*labelHight+15);
    changeFixedSizeH(buttonGroupOutputControl, 4*labelHight+35);
    
    frameSimLeft->setMinimumWidth(int(hResolusion/5));
}




//*******************************************
//*Initiation-of-Fitting-Dialog
//*******************************************
void fittable18::initFITTABLE()
{
    widgetStackFit->setCurrentIndex(0);
    tabWidgetFit->setCurrentIndex(0);
    tabWidgetGenResults->setCurrentIndex(0);
    tabWidgetinfo->setCurrentIndex(0);

    
    eFitWeight=false;
    //+++
    libName="";
    //+++
    XQ="x";
    //+++
    QDir dd;

    if (!QDir(app()->sasPath + "/FitFunctions/").exists())
        QDir().mkdir(app()->sasPath + "/FitFunctions/");

    libPath = app()->sasPath + "/FitFunctions/";
    libPath = libPath.replace("//", "/");

    scanGroup();
    
    //+++ Interface of first page
    textLabelFfunc->hide();
    comboBoxPolyFunction_2->hide();
    comboBoxFunction->hide();
    spinBoxCurrentFunction->hide();
    
    textLabelLeft->setEnabled(false);
    pushButtonFitPrev->setEnabled(false);
    spinBoxPara->hide();
    
    //+++ initial options
    pF=0;
    int p=0;
    pSANS=0;
    
    //+++ Para-Table******************
    tablePara->hideColumn(0);
    //tablePara->setColumnWidth(0,1);
    tablePara->setColumnWidth(1,45);
    tablePara->setColumnWidth(2,95);
    tablePara->setColumnWidth(3,105);
    
    //+++ Para-Comment******************
    tableParaComments->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Stretch );
    tableParaComments00->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Stretch );
    
    // +++ color boxes
    initColorBox();
    
    weightChanged();
    pushButtonLoadFittingSession->setGeometry(10, 3, 490, 30);
    setToSetProgressControl=false;
    algorithmSelected();
    
    
    //+++ Control-Table******************
    tableControl->setColumnWidth(0,85);
    tableControl->setColumnWidth(1,140);
    tableControl->setColumnWidth(2,120);
    tableControl->setColumnWidth(3,60);
    tableControl->setColumnWidth(4,85);

    tableControl->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch );
    
    spinBoxNumberCurvesToFit->setEnabled(false);
    pushButtonSaveSession->hide();
    comboBoxInstrument->setEnabled(false);
    
    widgetStackFit->setMinimumHeight(500);
    
    const QModelIndexList indexes = listViewGroup->model()->match(listViewGroup->model()->index(0,0),Qt::DisplayRole,"ALL",1,Qt::MatchExactly);

    if (auto *cornerButton = tableCurves->findChild<QAbstractButton *>())
    {
        disconnect(cornerButton, nullptr, this, nullptr);
        connect(cornerButton, &QAbstractButton::clicked, this, [this]() { tableScreenshotToActiveGraph(tableCurves); });
    }

    if (auto *cornerButton = tablePara->findChild<QAbstractButton *>())
    {
        disconnect(cornerButton, nullptr, this, nullptr);
        connect(cornerButton, &QAbstractButton::clicked, this, [this]() { tableScreenshotToActiveGraph(tablePara); });
    }
}

//*******************************************
// +++ init Color Boxes !OB
//*******************************************
void fittable18::initColorBox()
{
    comboBoxColor->clear();

    QPixmap icon = QPixmap(14, 14);

    for (int i = 0; i < app()->indexedColors().count(); i++)
    {
        icon.fill(app()->indexedColors()[i]);
        comboBoxColor->addItem(icon, app()->indexedColorNames()[i]);
    }
}

QColor fittable18::getColor(int pos)
{
    return app()->indexedColors()[pos];
}

//*******************************************
// +++ weightChanged
//*******************************************
void fittable18::weightChanged()
{
    int currentWeight=comboBoxWeightingMethod->currentIndex();
    //+++ info labels

    if (currentWeight == 0 || currentWeight == 2)
        textLabelInfoWeight->setText("σ - error bar column [weighting]  ");
    else
        textLabelInfoWeight->setText("Y - independent variable to be fit");

    //+++ parameters
    spinBoxWA->hide();
    spinBoxWB->hide();
    spinBoxWC->hide();
    spinBoxWXMAX->hide();

    switch (currentWeight)
    {
    case 4:
        spinBoxWA->show();
        spinBoxWB->hide();
        spinBoxWC->hide();
        spinBoxWXMAX->hide();
        break;
    case 5:
        spinBoxWA->show();
        spinBoxWB->show();
        spinBoxWC->show();
        spinBoxWXMAX->hide();
        break;
    case 6:
        spinBoxWA->show();
        spinBoxWB->hide();
        spinBoxWC->show();
        spinBoxWXMAX->show();
        break;
    default:
        spinBoxWA->hide();
        spinBoxWB->hide();
        spinBoxWC->hide();
        spinBoxWXMAX->hide();
        break;
    }
}

//*******************************************
// +++ algorithmSelected
//*******************************************
void fittable18::algorithmSelected()
{
    QString newAlgorithm=comboBoxFitMethod->currentText();
    
    if (newAlgorithm.contains("Simplex"))
    {
        buttonGroupGenMin->hide();
        buttonGroupSimplex->show();
        
        buttonGroupSimplex->setTitle("Simplex Options");
        
        lineEditToleranceAbs->hide();
        textLabel2_2->hide();
        
        lineEditTolerance->show();
        textLabelTolerance->show();
        
        checkBoxConstChi2->hide();
        labelConstChi2->hide();
        
        lineEditSTEP->hide();
        textLabelSTEP->hide();
        
        comboBoxConvRate->show();
        textLabelConvRate->show();

        comboBoxSimplex->show();
        textLabelSimplex->show();
        
        comboBoxLevenberg->hide();
        textLabelLevenberg->hide();
        
    }
    else if (newAlgorithm.contains("Levenberg"))
    {
        buttonGroupGenMin->hide();
        buttonGroupSimplex->show();
        
        buttonGroupSimplex->setTitle("Levenberg-Marquardt Options");
        
        lineEditToleranceAbs->show();
        textLabel2_2->show();
        
        levenbergSelected();

        lineEditSTEP->show();
        textLabelSTEP->show();
        
        comboBoxConvRate->hide();
        textLabelConvRate->hide();
        
        comboBoxSimplex->hide();
        textLabelSimplex->hide();
        
        comboBoxLevenberg->show();
        textLabelLevenberg->show();

    }
    else  if (newAlgorithm.contains("[GenMin]"))
    {
        buttonGroupGenMin->show();
        buttonGroupSimplex->hide();
        
        lineEditToleranceAbs->hide();
        textLabel2_2->hide();
        
    }
    
}

//*******************************************
// +++ algorithmSelected
//*******************************************
void fittable18::levenbergSelected()
{
    QString newLevenberg=comboBoxLevenberg->currentText();
    
    checkBoxConstChi2->show();
    labelConstChi2->show();
    
    if (newLevenberg.contains("delta"))
    {
        lineEditTolerance->show();
        textLabelTolerance->show();
    }
    else
    {
        lineEditTolerance->hide();
        textLabelTolerance->hide();
    }
}
//*******************************************
// slot: SANS support Y / N !OB
//*******************************************
void fittable18::SANSsupportYN()
{
    bool YN=checkBoxSANSsupport->isChecked();
    int p=spinBoxPara->value();
    
    //+++
    if (YN && pF==0 )
    {
        QMessageBox::warning(this ,tr("QtiKWS "),
                             tr("Select Function !"));
        checkBoxSANSsupport->setChecked(false);

        comboBoxInstrument->setEnabled(false);
        return;
    }
    
    if (YN) comboBoxInstrument->setEnabled(true);
    else comboBoxInstrument->setEnabled(false);
    
    
    //comboBoxPolyFunction->setCurrentIndex(0);
    //
    
    //
    F_paraList=F_paraListF;
    
    //
    QString currentInstrument=comboBoxInstrument->currentText();
    QStringList resoMethods;
    
    if (YN && currentInstrument.contains("SANS") )
    {
        SDchanged(0);
        
        p=pF+pSANS;
        F_paraList+=SANS_param_names;
        comboBoxPolyFunction->setEnabled(true);
        comboBoxPolyFunction_2->setEnabled(true);
        comboBoxResoFunction->setEnabled(true);
        
        resoMethods<<"Gauss-SANS"<<"Triangular"<<"Bessel-SANS"<<"Gauss";
        
        comboBoxResoFunction->clear();
        comboBoxResoFunction->addItems(resoMethods);
        comboBoxResoFunction->setEnabled(true);
        comboBoxSpeedControlReso->setEnabled(true);
        comboBoxSpeedControlReso->setCurrentIndex(3);
        speedControlReso();
        
        comboBoxPolyFunction->setEnabled(true);
        comboBoxPolyFunction_2->setEnabled(true);
        comboBoxSpeedControlPoly->setEnabled(true);
        comboBoxSpeedControlPoly->setCurrentIndex(3);
        speedControlPoly();
        
    }
    else     if (YN && currentInstrument.contains("Back") )
    {
        p=pF;
        comboBoxPolyFunction->setEnabled(false);
        comboBoxPolyFunction_2->setEnabled(false);
        comboBoxResoFunction->setEnabled(true);
        
        resoMethods<<"In Function";
        
        comboBoxResoFunction->clear();
        comboBoxResoFunction->addItems(resoMethods);
        comboBoxResoFunction->setEnabled(true);
        comboBoxSpeedControlReso->setEnabled(true);
        comboBoxSpeedControlReso->setCurrentIndex(3);
        speedControlReso();
        
        comboBoxPolyFunction->setCurrentIndex(0);
        comboBoxPolyFunction_2->setCurrentIndex(0);
        comboBoxPolyFunction->setEnabled(false);
        comboBoxPolyFunction_2->setEnabled(false);
        comboBoxSpeedControlPoly->setEnabled(false);
        comboBoxSpeedControlPoly->setCurrentIndex(3);
        speedControlPoly();
    }
    else
    {
        p=pF;
        comboBoxPolyFunction->setEnabled(false);
        comboBoxPolyFunction_2->setEnabled(false);
        comboBoxResoFunction->setEnabled(false);
        
        comboBoxResoFunction->setCurrentIndex(0);
        comboBoxResoFunction->setEnabled(false);
        comboBoxSpeedControlReso->setEnabled(false);
        comboBoxSpeedControlReso->setCurrentIndex(3);
        speedControlReso();
        
        comboBoxPolyFunction->setCurrentIndex(0);
        comboBoxPolyFunction_2->setCurrentIndex(0);
        comboBoxPolyFunction->setEnabled(false);
        comboBoxPolyFunction_2->setEnabled(false);
        comboBoxSpeedControlPoly->setEnabled(false);
        comboBoxSpeedControlPoly->setCurrentIndex(3);
        speedControlPoly();
    }
    spinBoxPara->setValue(p);
    
}

//*******************************************
// slot: Polydispersity function changed !OB
//*******************************************
void fittable18::SDchanged(int poly)
{
    //+++2018
    tablePara->blockSignals(true);
    
    int mm;
    int M=spinBoxNumberCurvesToFit->value();
    
    int oldPoly=poly;
    
    if (oldPoly!=comboBoxPolyFunction->currentIndex()) oldPoly=comboBoxPolyFunction->currentIndex();
    else if (oldPoly!=comboBoxPolyFunction_2->currentIndex()) oldPoly=comboBoxPolyFunction_2->currentIndex();
    
    
    comboBoxPolyFunction->setCurrentIndex(poly);
    comboBoxPolyFunction_2->setCurrentIndex(poly);
    
    int p=spinBoxPara->value();

    QString F_name=textLabelFfunc->text();
    
    //+++
    SANS_param_names.clear();
    SANS_initValues.clear();
    SANS_paraListComments.clear();
    SANS_adjustPara.clear();

    switch (poly)
    {
        case 0:
            //"Gauss"
            SANS_param_names<<"Sigma_Gauss_SD";
            SANS_initValues<<"0.0";
            SANS_paraListComments<<"Standard Deviation [1]";
            SANS_adjustPara<<"0";
            pSANS= 1;
            break;
        case 1:
            //"Schultz-Zimm"
            SANS_param_names<<"Sigma_Schultz_SD";
            SANS_initValues<<"0.0";
            SANS_paraListComments<<"Standard Deviation [1]";
            SANS_adjustPara<<"0";
            pSANS= 1;
            break;
        case 2:
            //"Gamma" ?OB
            SANS_param_names<<"Sigma_Gamma_SD";
            SANS_initValues<<"0.0";
            SANS_paraListComments<<"Sigma [1]";
            SANS_adjustPara<<"0";
            pSANS= 1;
            break;
        case 3:
            //"Log-Normal" ?OB
            SANS_param_names<<"Sigma_LogNormal_SD";
            SANS_initValues<<"0.0";
            SANS_paraListComments<<"Standard Deviation [1]";
            SANS_adjustPara<<"0";
            pSANS= 1;
            break;
        case 4:
            //"Uniform"
            SANS_param_names<<"Sigma_uniform_SD";
            SANS_initValues<<"0.0";
            SANS_paraListComments<<"Relative width of uniform SD [1]";
            SANS_adjustPara<<"0";
            pSANS= 1;
            break;
        case 5:
            //"Triangular"
            SANS_param_names<<"Sigma_A_SD"<<"Sigma_B_SD";
            SANS_initValues<<"1"<<"1";
            SANS_paraListComments<<"Relative left-width of triangular SD [.]"<<"Relative right-width of triangular SD [.]";
            SANS_adjustPara<<"0"<<"0";
            pSANS=2;
            break;
        default :  break;
    }
    
    bool sdNumberChanged=false;
    
    if (p==pF)
    {
        tablePara->setRowCount(p);
        tableParaComments->setRowCount(p);
        tableParaSimulate->setRowCount(p);
        tableControl->setRowCount(p);
        
        p=pF+pSANS;
        tablePara->setRowCount(p);
        tableParaComments->setRowCount(p);
        tableParaSimulate->setRowCount(p);
        tableControl->setRowCount(p);
        
        for (mm=0; mm<M;mm++)
        {
            tablePara->setItem(p-pSANS, 3*mm+2,new QTableWidgetItem(SANS_initValues[0]));
            if (pSANS==2)
                tablePara->setItem(p-pSANS+1, 3*mm+2,new QTableWidgetItem(SANS_initValues[1]));
            
            tablePara->setItem(p-pSANS,3*mm+3,new QTableWidgetItem("---"));
            tablePara->item(p-pSANS,3*mm+3)->setFlags(tablePara->item(p-pSANS,3*mm+3)->flags() & ~Qt::ItemIsEditable);
            tablePara->item(p-pSANS,3*mm+3)->setBackground(QColor(236,236,236));
            if (pSANS==2)
            {
                tablePara->setItem(p-pSANS+1,3*mm+3,new QTableWidgetItem("---"));
                tablePara->item(p-pSANS+1,3*mm+3)->setFlags(tablePara->item(p-pSANS+1,3*mm+3)->flags() & ~Qt::ItemIsEditable);
                tablePara->item(p-pSANS+1,3*mm+3)->setBackground(QColor(236,236,236));
            }
            
            QTableWidgetItem *cb = new QTableWidgetItem();
            cb->setText("..");
            tablePara->setItem(p-pSANS,3*mm+1, cb);
            cb->setCheckState(Qt::Unchecked);
            if (pSANS==2)
            {
                QTableWidgetItem *cb = new QTableWidgetItem();
                cb->setText("..");
                tablePara->setItem(p-pSANS+1,3*mm+1, cb);
                cb->setCheckState(Qt::Unchecked);
            }
        }
        
        QTableWidgetItem *cb = new QTableWidgetItem();
        tablePara->setItem(p-pSANS,0,cb);
        cb->setCheckState(Qt::Unchecked);
        if (pSANS==2)
        {
            QTableWidgetItem *cb = new QTableWidgetItem();
            tablePara->setItem(p-pSANS+1,0,cb);
            cb->setCheckState(Qt::Unchecked);
        }

        tableParaComments->setRowCount(p);
        tableParaComments->setItem(p-pSANS, 0, new QTableWidgetItem(SANS_paraListComments[0]));
        if (pSANS==2)
            tableParaComments->setItem(p-pSANS+1, 0, new QTableWidgetItem(SANS_paraListComments[1]));
    }
    else if (p!=(pF+pSANS))
    {
        sdNumberChanged=true;
        tablePara->setRowCount(pF+pSANS);
        tableParaComments->setRowCount(pF+pSANS);
        tableParaSimulate->setRowCount(pF+pSANS);
        tableControl->setRowCount(pF+pSANS);
        
        tableParaSimulate->setItem(pF,0,new QTableWidgetItem(SANS_initValues[0]));
        tableParaComments->setItem(pF,0,new QTableWidgetItem(SANS_paraListComments[0]));

        if (pSANS==2)
        {
            for (mm=0; mm<M;mm++)
            {
                tablePara->setItem(pF+pSANS-1,3*mm+2,new QTableWidgetItem(SANS_initValues[1]));
                tablePara->setItem(pF+pSANS-1,3*mm+3,new QTableWidgetItem("---"));
                tablePara->item(pF+pSANS-1,3*mm+3)->setFlags(tablePara->item(pF+pSANS-1,3*mm+3)->flags() & ~Qt::ItemIsEditable); //+++2019
                tablePara->item(pF+pSANS-1,3*mm+3)->setBackground(QColor(236,236,236));
                
                QTableWidgetItem *cb = new QTableWidgetItem();
                cb->setText("..");
                tablePara->setItem(pF+pSANS-1,3*mm+1, cb);
                cb->setCheckState(Qt::Unchecked);
                
                tablePara->item(pF+pSANS-2, 3*mm+2)->setText(SANS_initValues[1]);
            }
            
            tableParaComments->setItem(pF+pSANS-1,0,new QTableWidgetItem(SANS_paraListComments[1]));
            tableParaSimulate->setItem(pF+pSANS-1,0,new QTableWidgetItem(SANS_initValues[1]));
            
            QTableWidgetItem *cb = new QTableWidgetItem();
            tablePara->setItem(pF+pSANS-1,0,cb);
            cb->setCheckState(Qt::Unchecked);
            
            
        }

        
        p=pF+pSANS;
    }
    else if (p==pF+pSANS)
    {
        tableParaComments->setItem(p-1,0,new QTableWidgetItem(SANS_paraListComments[0]));
        sdNumberChanged=true;
    }
    
    F_paraList=F_paraListF;
    F_paraList+=SANS_param_names;
    tablePara->setVerticalHeaderLabels(F_paraList);
    tableParaSimulate->setVerticalHeaderLabels(F_paraList);
    //
    F_initValues=F_initValuesF;
    F_initValues+=SANS_initValues;
    //
    F_adjustPara=F_adjustParaF;
    F_adjustPara+=SANS_adjustPara;
    //
    F_paraListComments=F_paraListCommentsF;
    F_paraListComments+=SANS_paraListComments;
    
    tableParaComments->setVerticalHeaderLabels(F_paraList);
    
    spinBoxPara->setValue(p);
    initMultiTable();
    
    if(sdNumberChanged)
    {
        // +++
        int colsNumber=tableMultiFit->columnCount();
        tableMultiFit->setHorizontalHeaderItem(colsNumber-pSANS,new QTableWidgetItem(SANS_param_names[0]));
        if (pSANS==2) tableMultiFit->setHorizontalHeaderItem(colsNumber-pSANS+1,new QTableWidgetItem(SANS_param_names[1]));
        //+++
        tableControl->setItem(p-1,2, new QTableWidgetItem(F_paraList[p-1]));
        
        tableControl->item(p-1,2)->setFlags(tableControl->item(p-1,2)->flags() & ~Qt::ItemIsEditable); //+++2019
        tableControl->item(p-1,2)->setBackground(QColor(236,236,236));
        
        QStringList cccL; cccL<<"=<"<<"<"+F_paraList[p-1]+">";
        QString stmp=QChar(177);stmp+=QChar(963);
        QStringList cccR; cccR<<"=<"<<stmp;
        
        QStringList lst0;
        QComboBox *cL = new QComboBox();
        tableControl->setCellWidget(p-1,1, cL);
        cL->addItems(cccL);
        
        //***
        QComboBox *cR = new QComboBox();
        tableControl->setCellWidget(p-1,3, cR);
        cR->addItems(cccR);
        
        connect(cR, SIGNAL( activated(int) ), cL, SLOT( setCurrentIndex(int) ) );
        connect(cL, SIGNAL( activated(int) ), cR, SLOT( setCurrentIndex(int) ) );
        
        
        tableControl->setItem(p-1,0,new QTableWidgetItem("0"));
        //+++
        tableControl->setItem(p-1,4,new QTableWidgetItem("inf"));

        
        if (pSANS==2)
        {

            //+++
            tableControl->setItem(p-2,2, new QTableWidgetItem(F_paraList[p-2]));
            tableControl->item(p-2,2)->setFlags(tableControl->item(p-2,2)->flags() & ~Qt::ItemIsEditable); //+++2019
            tableControl->item(p-2,2)->setBackground(QColor(236,236,236));
            
            QStringList cccL; cccL<<"=<"<<"<"+F_paraList[p-2]+">";
            QString stmp=QChar(177);stmp+=QChar(963);
            QStringList cccR; cccR<<"=<"<<stmp;
            
            QStringList lst0;
            QComboBox *cL = new QComboBox();
            tableControl->setCellWidget(p-2,1, cL);
            cL->addItems(cccL);
            
            //***
            QComboBox *cR = new QComboBox();
            tableControl->setCellWidget(p-2,3, cR);
            cR->addItems(cccR);
            
            connect(cR, SIGNAL( activated(int) ), cL, SLOT( setCurrentIndex(int) ) );
            connect(cL, SIGNAL( activated(int) ), cR, SLOT( setCurrentIndex(int) ) );
            
            
            tableControl->setItem(p-2,0,new QTableWidgetItem("0"));
            
            //+++
            tableControl->setItem(p-2,4,new QTableWidgetItem("inf"));

        }
    }
    tablePara->blockSignals(false);
}

//***************************************************
//*** speedControlReso
//***************************************************
void fittable18::speedControlReso()
{
    if (comboBoxSpeedControlReso->currentIndex()==7)
    {
        lineEditAbsErr->setEnabled(true);
        lineEditRelErr->setEnabled(true);
        spinBoxIntWorkspase->setEnabled(true);
        spinBoxIntLimits->setEnabled(true);
    }
    else
    {
        lineEditAbsErr->setText("0");
        lineEditRelErr->setText("0");
        spinBoxIntLimits->setValue(3);
        
        lineEditAbsErr->setEnabled(false);
        lineEditRelErr->setEnabled(false);
        spinBoxIntWorkspase->setEnabled(false);
        spinBoxIntLimits->setEnabled(false);
    }
    
    int speed=comboBoxSpeedControlReso->currentIndex();
    
    switch (speed)
    {
        case 0:
            spinBoxIntWorkspase->setValue(5);  // 99.00%
            break;
        case 1:
            spinBoxIntWorkspase->setValue(6);  // 99.80%
            break;
        case 2:
            spinBoxIntWorkspase->setValue(7);  // 99.90%
            break;
        case 3:
            spinBoxIntWorkspase->setValue(8);  // 99.95%
            break;
        case 4:
            spinBoxIntWorkspase->setValue(15);  // 99.98%
            break;
        case 5:
            spinBoxIntWorkspase->setValue(20);  // 99.99%
            break;
        case 6:
            spinBoxIntWorkspase->setValue(50);  // >99.99%
            break;
    }
    
    if (comboBoxResoFunction->currentIndex()==1)
    {
        spinBoxIntLimits->hide();
    }
    else
    {
        spinBoxIntLimits->show();
    }
}

//***************************************************
//*** speedControlPoly
//***************************************************
void fittable18::speedControlPoly()
{
    if (comboBoxSpeedControlPoly->currentIndex()==7)
    {
        lineEditAbsErrPoly->setEnabled(true);
        lineEditRelErrPoly->setEnabled(true);
        spinBoxIntWorkspasePoly->setEnabled(true);
        spinBoxIntLimitsPoly->setEnabled(true);
    }
    else
    {
        lineEditAbsErrPoly->setText("0");
        lineEditRelErrPoly->setText("0");
        spinBoxIntLimitsPoly->setValue(3);
        
        lineEditAbsErrPoly->setEnabled(false);
        lineEditRelErrPoly->setEnabled(false);
        spinBoxIntWorkspasePoly->setEnabled(false);
        spinBoxIntLimitsPoly->setEnabled(false);
    }
    
    int speed=comboBoxSpeedControlPoly->currentIndex();
    
    switch (speed)
    {
        case 0:
            spinBoxIntWorkspasePoly->setValue(5);  // 99.00%
            break;
        case 1:
            spinBoxIntWorkspasePoly->setValue(6);  // 99.80%
            break;
        case 2:
            spinBoxIntWorkspasePoly->setValue(7);  // 99.90%
            break;
        case 3:
            spinBoxIntWorkspasePoly->setValue(8);  // 99.95%
            break;
        case 4:
            spinBoxIntWorkspasePoly->setValue(15);  // 99.98%
            break;
        case 5:
            spinBoxIntWorkspasePoly->setValue(20);  // 99.99%
            break;
        case 6:
            spinBoxIntWorkspasePoly->setValue(50);  // >99.99%
            break;
    }
    
    if (speed == 7)
        spinBoxIntWorkspasePoly->setPrefix("Max. Number of points:   ");
    else
        spinBoxIntWorkspasePoly->setPrefix("Number of points:   ");
    
    if (comboBoxPolyFunction->currentIndex() >= 4)
        spinBoxIntLimitsPoly->hide();
    else
        spinBoxIntLimitsPoly->show();
}

void fittable18::updateDatasets()
{
    int M = spinBoxNumberCurvesToFit->value();

    //+++ Set Data-Sets List
    for (int mm = 0; mm < M; mm++)
    {
        //+++ Data sets selection ComboBox
        auto *xxx = (QComboBox *)tableCurves->cellWidget(0, 2 * mm + 1);
        QString oldYcol = xxx->currentText();
        QStringList lst = app()->columnsList(Table::Y);
        QStringList lstNew;
        
        for (int i = 0; i < lst.count(); i++)
            if (lst[i].length() > 14 && lst[i].right(9) == "_residues" &&
                (lst[i].left(14) == "simulatedCurve" || lst[i].left(8) == "fitCurve"))
                continue;
            else
                lstNew << lst[i];

        xxx->blockSignals(true);
        xxx->clear();
        xxx->addItems(lstNew);
        if (oldYcol != "" && xxx->findText(oldYcol) >= 0)
        {
            xxx->setCurrentIndex(xxx->findText(oldYcol));
            xxx->blockSignals(false);
        }
        else
        {
            xxx->blockSignals(false);
            tableCurvechanged(0, 2 * mm + 1);
        }
    }
}

//*******************************************
// +++ init Multi Table  !OB
//*******************************************
void fittable18::initMultiTable()
{
    int labelHight= int(app()->screenResoHight*app()->sasResoScale/50);
    
    if (spinBoxNumberCurvesToFit->value()==1)
    {
        int cols=spinBoxPara->value()+3;
        
        if (checkBoxSANSsupport->isChecked())
        {
            cols++;
        }
        
        tableMultiFit->setColumnCount(cols);
        tableMultiFit->setRowCount(1);
        tableMultiFit->setColumnWidth(0,labelHight+10);
        
        int i;
        
        tableMultiFit->setItem(0,0,new QTableWidgetItem("")); // new 2018
        
        for (i=1;i<cols;i++)
        {
            tableMultiFit->setColumnWidth(i,3*labelHight);
            // +++
            QTableWidgetItem *item = new QTableWidgetItem();
            item->setCheckState(Qt::Unchecked);
            tableMultiFit->setItem(0,i,item);
            
            
        }
        //+++ mke Buttoms active
        tabWidgetGenResults->setTabVisible(2, true);
    }
    else
    {
        tableMultiFit->setColumnCount(0);
        tableMultiFit->setRowCount(1);

        //+++ mke Buttoms active
        tabWidgetGenResults->setTabVisible(2, false);
    }
    
    
}


//*******************************************
//*Init Fitting Page
//*******************************************
void fittable18::initLimits()
{
    //+++
    int PP=spinBoxPara->value();
    
    //+++
    tableControl->setRowCount(0);
    tableControl->setRowCount(PP);
    
    //+++
    int digits=spinBoxSignDigits->value();
    
    //+++
    QStringList cccL;
    QStringList cccR;
    
    double leftLimit, rightLimit;
    QString s;
    
    for(int pp=0; pp<PP; pp++)
    {
        QString stmp=QChar(177);stmp+=QChar(963);
        cccL.clear(); cccL<<"=<"<<"<"+F_paraList[pp]+">";
        cccR.clear();cccR<<"=<"<<stmp;
        
        tableControl->setItem(pp,0,new QTableWidgetItem("-inf")); //2018:: -inf instead of -1.0E308
        
        tableControl->setItem(pp,1, new QTableWidgetItem);
        tableControl->item(pp,1)->setBackground(QColor(236,236,236));
        
        QStringList lst0;
        QComboBox *cL = new QComboBox();
        tableControl->setCellWidget(pp,1, cL);
        cL->addItems(cccL);
        
        tableControl->setItem(pp,3, new QTableWidgetItem);
        tableControl->item(pp,3)->setBackground(QColor(236,236,236));
        
        //***
        QComboBox *cR = new QComboBox();
        tableControl->setCellWidget(pp,3, cR);
        cR->addItems(cccR);
        
        connect(cR, SIGNAL( activated(int) ), cL, SLOT( setCurrentIndex(int) ) );
        connect(cL, SIGNAL( activated(int) ), cR, SLOT( setCurrentIndex(int) ) );

        //***
        tableControl->setItem(pp,4,new QTableWidgetItem("inf")); //2018:: -inf instead of 1.0E308

        tableControl->setItem(pp,2, new QTableWidgetItem(F_paraList[pp])); //qt4
        
        tableControl->item(pp,2)->setFlags(tableControl->item(pp,2)->flags() & ~Qt::ItemIsEditable); //+++2019
        tableControl->item(pp,2)->setBackground(QColor(236,236,236));
        
        s=F_initValues[pp];
        
        if (s.contains('[') && s.contains("..") && s.contains(']'))
        {
            leftLimit=s.mid(s.indexOf("[")+1,s.indexOf("..")-s.indexOf("[") - 1 ).toDouble();
            rightLimit=s.mid(s.indexOf("..")+2,s.indexOf("]")-s.indexOf("..") - 2 ).toDouble();
            
            tableControl->item(pp,0)->setText(QString::number(leftLimit));
            tableControl->item(pp,4)->setText(QString::number(rightLimit));
        }
    }
    if (checkBoxSANSsupport->isChecked() && comboBoxInstrument->currentIndex()==0)
    {
        tableControl->setItem(PP-1,0,new QTableWidgetItem("0"));
        if (pSANS==2) tableControl->setItem(PP-2,0,new QTableWidgetItem("0"));
    }
}

//*******************************************
//*Init Fitting Page
//*******************************************
void fittable18::chekLimits()
{
    //+++
    int PP=spinBoxPara->value();
    
    double leftV;
    double rightV;
    
    for(int pp=0; pp<PP; pp++)
    {
        
        if (tableControl->item(pp,0)->text().contains("-inf")) leftV=-1.0E308;
        else leftV=tableControl->item(pp,0)->text().toDouble();
        if (tableControl->item(pp,4)->text().contains("inf")) rightV= 1.0E308;
        else rightV=tableControl->item(pp,4)->text().toDouble();
        
        if (rightV<leftV)
        {
            leftV=	-1.0E308;
            rightV=1.0E308;
        }
        
        if (leftV>-1.0E308) tableControl->item(pp,0)->setText(QString::number(leftV));
        else tableControl->item(pp,0)->setText("-inf");
        if (rightV<1.0E308) tableControl->item(pp,4)->setText(QString::number(rightV));
        else tableControl->item(pp,4)->setText("inf");
    }
}

//*******************************************
//*Init Fitting Page
//*******************************************
void fittable18::chekLimitsAndFittedParameters()
{
    tablePara->blockSignals(true);
    
    int M=spinBoxNumberCurvesToFit->value();        // Number of Curves
    int p=spinBoxPara->value();                //Number of Parameters per Curve
    int prec=spinBoxSignDigits->value();
    
    //+++
    double left;
    double right;
    double value;
    QString txtVary;
    QStringList lstTmpLimits;
    
    //+++
    for (int pp=0; pp<p;pp++)
    {
        for (int mm=0; mm<M;mm++)
        {
            if(tableControl->item(pp,0)->text().contains("-inf")) left=-1.0e308;
            else left=tableControl->item(pp,0)->text().toDouble();
            if(tableControl->item(pp,4)->text().contains("inf")) right=1.0e308;
            else right=tableControl->item(pp,4)->text().toDouble();
            
            
            QTableWidgetItem *itA0 = (QTableWidgetItem *)tablePara->item(pp,3*mm+1); // Vary?
            
            txtVary=itA0->text().remove(" ");
            
            if (txtVary.contains("..") && txtVary!="..")
            {
                lstTmpLimits = txtVary.split("..");
                if(lstTmpLimits.count()==2)
                {
                    double leftNew;
                    if (lstTmpLimits[0]!="") leftNew=lstTmpLimits[0].toDouble();
                    else leftNew=left;
                    
                    double rightNew;
                    if (lstTmpLimits[1]!="") rightNew=lstTmpLimits[1].toDouble();
                    else rightNew=right;
                    
                    if (rightNew>leftNew)
                    {
                        if (leftNew>left && leftNew<right) left=leftNew;
                        if (rightNew<right && rightNew>left) right=rightNew;
                    }
                }
            }
            
            
            QTableWidgetItem *itValue = (QTableWidgetItem *)tablePara->item(pp,3*mm+2);
            value=itValue->text().toDouble();
            if (value<left) itValue->setText(QString::number(left,'G',prec));
            else if (value>right) itValue->setText(QString::number(right,'G',prec));
        }
    }
    
    tablePara->blockSignals(false); //+++2019
}

//*******************************************
//*Init Fitting Page
//*******************************************
void fittable18::initFitPage()
{
    tableCurves->blockSignals(true);
    tablePara->blockSignals(true);
    
    QStringList lst0;
    
    int id=widgetStackFit->currentIndex();
    int M=spinBoxNumberCurvesToFit->value();
    
    int mm,pp;
    QStringList colNames, iQlist;
    iQlist<<"N"<<XQ;
    
    textLabelfromQsim->setText(XQ+"[min]=");
    textLabelToQsim->setText(XQ+"[max]=");
    radioButtonUniform_Q->setTitle(" ... Uniform "+XQ+":: Range ... ");
    
    textLabelfromQ->setText(XQ+"[min]=");
    textLabelToQ->setText(XQ+"[max]=");
    radioButtonSameQrange->setTitle(" ... Same "+XQ+" as Fitting Data ... ");
    
    QString s;
    double temp;

    //+++ Initiation of multi-Set Tables
    initMultiParaTable();
    
    
    if (id==0)
    {
        //+++ tableCurves  SANS supportf
        QStringList tableCurvesNames;
        QStringList tableCurvesNamesSimulate;
        
        //+++
        tableCurvesNames<<"Data Set(s)"<<"N"<<"First Point"<<"Last Point"<<"Weighting";
        tableCurvesNamesSimulate<<"Begin"<<"End"<<"# points"<<"Data Set(s)";
        
        if (checkBoxSANSsupport->isChecked())
        {
            QString currentInstrument=comboBoxInstrument->currentText();
            if ( currentInstrument.contains("SANS") )
            {
                //+++
                tableCurvesNames<<"Resolution"<<"Polydispersity";
                tableCurvesNamesSimulate<<"Resolution"<<"Polydispersity";
                tableCurves->setRowCount(7);
                tableCurves->horizontalHeader()->setMinimumSectionSize(3*int(app()->screenResoHight/50*app()->sasResoScale));
            }
            else if ( currentInstrument.contains("Back-Scattering") )
            {
                //+++
                tableCurvesNames<<"Resolution";
                tableCurvesNamesSimulate<<"Resolution";
                tableCurves->setRowCount(6);
                tableCurves->horizontalHeader()->setMinimumSectionSize(3*int(app()->screenResoHight/50*app()->sasResoScale));
            }
        }
        else
        {
            //+++
            tableCurves->setRowCount(5);
            tableCurves->horizontalHeader()->setMinimumSectionSize(3*int(app()->screenResoHight/50*app()->sasResoScale)-20);
        }

        //+++
        tableCurves->setVerticalHeaderLabels(tableCurvesNames);
        tableCurves->setColumnCount(2*M);
        
        //+++ Set Data-Sets List
        for(mm=0;mm<M;mm++)
        {
#ifdef Q_OS_WIN
            tableCurves->setColumnWidth(2*mm,3*int(app()->screenResoHight/50*app()->sasResoScale));
#else
            tableCurves->setColumnWidth(2*mm,3.5*int(app()->screenResoHight/50*app()->sasResoScale));
#endif
            
            if (M==1)
            {
                tableCurves->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::Stretch );
            }
            else  if (M==2)
            {
                tableCurves->horizontalHeader()->setSectionResizeMode( 2*mm+1, QHeaderView::Stretch );
            }
            else
            {
                tableCurves->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive );
                tableCurves->horizontalHeader()->setSectionResizeMode(2*mm+1, QHeaderView::Interactive );
                tableCurves->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive );
                tableCurves->setColumnWidth(2*mm+1,5*int(app()->screenResoHight/50*app()->sasResoScale));
            }
            
             if (M==1) colNames<<"Checks"<<"Dataset";
             else colNames<<"Checks #"+QString::number(mm+1)<<"Dataset #"+QString::number(mm+1);
        
            //++ tableCurves: 2*mm
            
            //+++ Q or N select

            tableCurves->setItem(1, 2*mm, new QTableWidgetItem);
            tableCurves->item(1, 2*mm)->setBackground(QColor(236,236,236));
            
            QComboBoxInTable *iQ = new QComboBoxInTable(1,2*mm,tableCurves);
            iQ->addItems(iQlist);
            connect(iQ, SIGNAL( activated(int,int) ), this, SLOT( tableCurvechanged(int,int) ) );
            tableCurves->setCellWidget(1,2*mm,iQ );
 
            //+++ CheckTableItem: N/Qmin fix?
            QTableWidgetItem *cbImin = new QTableWidgetItem();
            cbImin->setCheckState(Qt::Unchecked);
            tableCurves->setItem(2,2*mm, cbImin);
            tableCurves->item(2,2*mm)->setBackground(QColor(236,236,236));
            tableCurves->item(2,2*mm)->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);

            //+++ CheckTableItem: N/Qmax Fix?
            QTableWidgetItem *cbImax = new QTableWidgetItem();
            cbImax->setCheckState(Qt::Unchecked);
            tableCurves->setItem(3,2*mm, cbImax);
            tableCurves->item(3,2*mm)->setBackground(QColor(236,236,236));
            tableCurves->item(3,2*mm)->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
            
            //+++ CheckTableItem: Weighting
            QTableWidgetItem *cbW = new QTableWidgetItem();
            cbW->setCheckState(Qt::Unchecked);
            tableCurves->setItem(4,2*mm, cbW);
            tableCurves->item(4,2*mm)->setBackground(QColor(236,236,236));
            tableCurves->item(4,2*mm)->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
            
            if (eFitWeight) cbW->setCheckState(Qt::Checked);
            
            //+++ If with SANS support
            if (checkBoxSANSsupport->isChecked())
            {
                //+++ CheckTableItem: IncludeResolution
                QTableWidgetItem *cbR = new QTableWidgetItem();
                cbR->setCheckState(Qt::Unchecked);
                tableCurves->setItem(5,2*mm, cbR);
                tableCurves->item(5,2*mm)->setBackground(QColor(236,236,236));
                tableCurves->item(5,2*mm)->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
                QString currentInstrument=comboBoxInstrument->currentText();
                cbR->setText("off");
                
                if ( currentInstrument.contains("SANS") )
                {
                    //+++ CheckTableItem: Include polydispersity
                    QTableWidgetItem *cbP = new QTableWidgetItem();
                    cbP->setCheckState(Qt::Unchecked);
                    tableCurves->setItem(6,2*mm, cbP);
                    tableCurves->item(6,2*mm)->setBackground(QColor(236,236,236));
                    tableCurves->item(6,2*mm)->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
                    cbP->setText("off");
                }
            }
            
            //+++ tableCurves: 2*mm+1
            
            //+++ Data sets selection ComboBox
            QString oldYcol="";
            
            QComboBoxInTable* curve;
            
            tableCurves->setItem(0, 2*mm+1, new QTableWidgetItem);
            tableCurves->item(0, 2*mm+1)->setBackground(QColor(236,236,236));

            if ((QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1))
            {
                curve =(QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1);
                curve->blockSignals(true);
                oldYcol=curve->currentText();
                curve->clear();
                curve->addItems(app()->columnsList(Table::Y));
                if (oldYcol!="") curve->setCurrentIndex(curve->findText(oldYcol));
                curve->blockSignals(false);
            }
            else
            {
                 curve = new QComboBoxInTable(0,2*mm+1,tableCurves);
                 tableCurves->setCellWidget(0,2*mm+1, curve);
                 curve->addItems(app()->columnsList(Table::Y));
                 connect(curve, SIGNAL( activated(int,int) ), this, SLOT( tableCurvechanged(int,int) ) );
            }
            
            //+++ Weight Set selection CimboBox
            tableCurves->setItem(4, 2*mm+1, new QTableWidgetItem);
            tableCurves->item(4, 2*mm+1)->setBackground(QColor(236,236,236));
            
            QComboBoxInTable* weight = new QComboBoxInTable(4,2*mm+1,tableCurves);
            tableCurves->setCellWidget(4,2*mm+1, weight);
            
            connect(weight, SIGNAL( activated(int,int) ), this, SLOT( tableCurvechanged(int,int) ) );
            
            if (eFitWeight)
            {
                cbW->setText("on");
                
                weight->setMaximumWidth(((QComboBoxInTable*)tableCurves->cellWidget(4,2*mm+1))->width());
                weight->setMinimumWidth(((QComboBoxInTable*)tableCurves->cellWidget(4,2*mm+1))->width());
                weight->setMaximumWidth(10000);
                weight->setMinimumWidth(0);
                tableCurves->item(4,2*mm+1)->setText("");
            }
            else
            {
                cbW->setText("off");
                
                weight->setMinimumWidth(0);
                weight->setMaximumWidth(0);
                tableCurves->item(4,2*mm+1)->setText("... [w=1] ... ");
            }
            
            //+++ If with  SANS support
            if (checkBoxSANSsupport->isChecked())
            {
                tableCurves->setItem(5, 2*mm+1, new QTableWidgetItem);
                tableCurves->item(5, 2*mm+1)->setBackground(QColor(236,236,236));
                
                //+++ select resolution data-set
                QComboBoxInTable* reso = new QComboBoxInTable(5,2*mm+1,tableCurves);
                tableCurves->setCellWidget(5,2*mm+1, reso);
                connect(reso, SIGNAL( activated(int,int) ), this, SLOT( tableCurvechanged(int,int) ) );
                
                reso->setMinimumWidth(0);
                reso->setMaximumWidth(0);
                tableCurves->item(5,2*mm+1)->setText("... no resolution integration");
                
                QString currentInstrument=comboBoxInstrument->currentText();
                if ( currentInstrument.contains("SANS") )
                {
                    tableCurves->setItem(6, 2*mm+1, new QTableWidgetItem);
                    tableCurves->item(6, 2*mm+1)->setBackground(QColor(236,236,236));
                    
                    //+++ Select polydispersity data-set
                    QComboBoxInTable* poly = new QComboBoxInTable(6,2*mm+1,tableCurves);
                    poly->addItems(F_paraListF);
                    tableCurves->setCellWidget(6,2*mm+1, poly);
                    connect(poly, SIGNAL( activated(int,int) ), this, SLOT( tableCurvechanged(int,int) ) );
                    
                    poly->setMinimumWidth(0);
                    poly->setMaximumWidth(0);
                    tableCurves->item(6,2*mm+1)->setText("... no polydispersity integration");
                }
            }
            
            QString table=curve->currentText();
            table=table.left(table.lastIndexOf("_"));
            
            tableCurves->setItem(0,2*mm, new QTableWidgetItem("") );
            tableCurves->setItem(1,2*mm+1, new QTableWidgetItem("") );
            tableCurves->item(1,2*mm+1)->setFlags(tableCurves->item(1,2*mm+1)->flags() & ~Qt::ItemIsEditable);
            tableCurves->item(1,2*mm+1)->setBackground(QColor(236,236,236));
            
            tableCurves->setItem(2,2*mm+1, new QTableWidgetItem("1") );
            tableCurves->item(2,2*mm+1)->setFlags(tableCurves->item(2,2*mm+1)->flags() & ~Qt::ItemIsEditable);
            tableCurves->item(2,2*mm+1)->setBackground(QColor(236,236,236));
            
            tableCurves->setItem(3,2*mm+1, new QTableWidgetItem("") );
            tableCurves->item(3,2*mm+1)->setFlags(tableCurves->item(3,2*mm+1)->flags() & ~Qt::ItemIsEditable);
            tableCurves->item(3,2*mm+1)->setBackground(QColor(236,236,236));
            
            //+++ Fill weight, reso, poly , data sets combo-boxes
            colList(table,2*mm+1);
        }
        
        //+++ Set Column Labels
        tableCurves->setHorizontalHeaderLabels(colNames);
    }
    else
    {
        
        //+++ Set Data-Sets List
        for(mm=0;mm<M;mm++)
        {
            //+++ Q or N select
            QComboBoxInTable* iQ =(QComboBoxInTable*)tableCurves->cellWidget(1, 2*mm);
            iQ->blockSignals(true);
            int currentItem=iQ->currentIndex();
            iQ->clear();
            iQ->addItems(iQlist);
            iQ->setCurrentIndex(currentItem);
            iQ->blockSignals(false);
            
            //+++ If with  SANS support
            if (checkBoxSANSsupport->isChecked())
            {
                QString currentInstrument=comboBoxInstrument->currentText();
                if ( currentInstrument.contains("SANS") )
                {
                    //+++ Select polydispersity data-set
                    QComboBoxInTable* poly = new QComboBoxInTable(6,2*mm+1,tableCurves);
                    poly->addItems(F_paraListF);
                    tableCurves->setCellWidget(6,2*mm+1, poly);
                    connect(poly, SIGNAL( activated(int,int) ), this, SLOT( tableCurvechanged(int,int) ) );
                }
            }
            
        }
    }

    
    //
    int PP=spinBoxPara->value();
    
    
    //+++ Set Table Labels
    tablePara->setVerticalHeaderLabels(F_paraList);
    //tablePara->verticalHeader()->adjustHeaderSize();
    //tablePara->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    
    //+++ Set Table Labels Simulate
    tableParaSimulate->setRowCount(0);
    tableParaSimulate->setRowCount(PP);
    tableParaSimulate->setVerticalHeaderLabels(F_paraList);
    tableParaSimulate->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Stretch );

    
    //+++ Function description
    if (F_paraListComments.count() ==0)
        textBrowserFunctionDescription->setText("<center>...sorry, but no description of Function is available...</center>");
    else
        textBrowserFunctionDescription->setText(F_paraListComments[0]);
    

    
    //+++ Comment table
    tableParaComments->setRowCount(0);
    tableParaComments->setRowCount(PP);
    tableParaComments->setVerticalHeaderLabels(F_paraList);
    
    //
    int digits=spinBoxSignDigits->value()-1;
    
    tablePara->blockSignals(true);
    for(pp=0; pp<PP; pp++)
    {
        s=F_initValues[pp];
        
        if (s.contains('[') && s.contains("..") && s.contains(']'))
        {
            temp=s.left(s.indexOf("[")).toDouble();
        }
        else
            temp=s.toDouble();
        
        for (mm=0; mm<M;mm++)
        {
            tablePara->setItem(pp,3*mm+2,new QTableWidgetItem(QString::number(temp, 'G', digits+1)));
            tablePara->setItem(pp,3*mm+3,new QTableWidgetItem("---"));
            tablePara->item(pp,3*mm+3)->setFlags(tablePara->item(pp,3*mm+3)->flags() & ~Qt::ItemIsEditable); //+++2019
            tablePara->item(pp,3*mm+3)->setBackground(QColor(236,236,236));
            
            QTableWidgetItem *cb = new QTableWidgetItem();
            cb->setText("..");
            tablePara->setItem(pp,3*mm+1, cb);
            
            if (F_adjustPara[pp]=="1") cb->setCheckState(Qt::Checked); else cb->setCheckState(Qt::Unchecked);
        }
        gsl_vector_set(F_para,pp,s.toDouble());
        gsl_vector_int_set(F_para_fit_yn,pp,F_adjustPara[pp].toInt());

        double leftLimit=-1e308;
        double rightLimit=1e308;
        
        if (s.contains('[') && s.contains("..") && s.contains(']'))
        {
            
            if (s.mid(s.indexOf("[")+1,s.indexOf("..")-s.indexOf("[") - 1 ).remove(" ")!="") leftLimit=s.mid(s.indexOf("[")+1,s.indexOf("..")-s.indexOf("[") - 1 ).remove(" ").toDouble();
            if (s.mid(s.indexOf("..")+2,s.indexOf("]")-s.indexOf("..") - 2 ).remove(" ")!="")
                rightLimit=s.mid(s.indexOf("..")+2,s.indexOf("]")-s.indexOf("..") - 2 ).remove(" ").toDouble();
        }
        gsl_vector_set(F_para_limit_left,pp,leftLimit);
        gsl_vector_set(F_para_limit_right,pp,rightLimit);
        
        //Comment table
        if (F_paraListComments.count() <= 1) tableParaComments->setItem(pp,0,new QTableWidgetItem("sorry, but no description available"));
        else tableParaComments->setItem(pp,0,new QTableWidgetItem(F_paraListComments[pp+1]));
    }
    tablePara->blockSignals(false);
    
    //+++
    bool polyYN=false;
    int polyFunction=comboBoxPolyFunction->currentIndex();
    //+++
    bool beforeFit=false;
    bool afterFit=false;
    bool beforeIter=false;
    bool afterIter=false;
    
    double *Qq=new double[1]; Qq[0]=0.0;
    double *Ii=new double[1]; Ii[0]=0.0;
    double *dIi=new double[1]; dIi[0]=0.0;
    double *SIGMAsigma=new double[1];SIGMAsigma[0]=0.0;
    
    int  *listLastPoints=new int[1]; listLastPoints[0]=1;
    int currentM=0;
    
    int currentFirstPoint=0;
    int currentLastPoint=0;
    int currentPoint=0;
    
    int prec=spinBoxSignDigits->value();
    
    //+++ ,tableName,tableColNames,tableColDestinations,mTable
    char *tableName = nullptr;
    char **tableColNames=0;
    int *tableColDestinations=0;
    gsl_matrix * mTable=0;
    //+++ 2019-09: new superpositional function number
    int currentFunction=spinBoxCurrentFunction->value();
    //+++
    double Int1=1.0;
    double Int2=1.0;
    double Int3=1.0;
    //+++
    int currentInt=0;
        
    functionT paraT={F_para, F_para_limit_left, F_para_limit_right, F_para_fit_yn, Qq, Ii, dIi, SIGMAsigma,listLastPoints, currentM, currentFirstPoint, currentLastPoint, currentPoint, polyYN, polyFunction, beforeFit, afterFit, beforeIter, afterIter, Int1, Int2, Int3, currentInt, prec,tableName,tableColNames,tableColDestinations,mTable,currentFunction};
    
    
    F.params=&paraT;

    if (tableColNames!=0) delete[] tableColNames;
    if (tableColDestinations!=0) delete[] tableColDestinations;
    if (mTable!=0) gsl_matrix_free(mTable);

    
    delete[] Qq;
    delete[] Ii;
    delete[] SIGMAsigma;
    
    tablePara->blockSignals(false);
    tableCurves->blockSignals(false);
}


//*******************************************
//* init Multi-Table
//*******************************************
void fittable18::initMultiParaTable()
{
    tablePara->blockSignals(true);
    
    int i,j;
    
    int p=spinBoxPara->value();
    
    tablePara->setRowCount(p);
    
    int colReso=int(app()->screenResoHight/50*app()->sasResoScale);
    
    if (!checkBoxMultiData->isChecked())
    {
        
        
        //	spinBoxNumberCurvesToFit->hide();
        spinBoxNumberCurvesToFit->setEnabled(false);
        tablePara->hideColumn(0);
        tablePara->setColumnCount(4);
        tablePara->setColumnWidth(0,1);
        tablePara->setColumnWidth(1,5*colReso);
        tablePara->setColumnWidth(2,5*colReso);
        tablePara->setColumnWidth(3,6*colReso);
        spinBoxNumberCurvesToFit->setValue(1);
        
        tablePara->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::Stretch );
    }
    else
    {
        tablePara->showColumn(0);
        spinBoxNumberCurvesToFit->show();
        spinBoxNumberCurvesToFit->setEnabled(true);
        if (spinBoxNumberCurvesToFit->value()==1)tablePara->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::Stretch );
        else tablePara->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::Interactive);
        tablePara->setColumnWidth(2,5*colReso);
        
        tablePara->setColumnCount(4);
        tablePara->setColumnWidth(0,2*colReso+10);
        QStringList colNames;
        if (spinBoxNumberCurvesToFit->value()==1) colNames<<"Share?"<<"Vary?From..To"<<"Value"<<"Error";
        else colNames<<"Share?"<<"Vary?From..To"<<"Value #1"<<"Error #1";
        
        for (j=0;j<p;j++)
        {
            QTableWidgetItem *cb = new QTableWidgetItem();
            cb->setCheckState(Qt::Unchecked);
            tablePara->setItem(j,0, cb);

        }
        for (i=0;i<(spinBoxNumberCurvesToFit->value()-1); i++ )
        {
            //if ((i+2)>=10) colNames<<"Vary?From..To"<<"Value-"+QString::number(i+2)<<"Error-"+QString::number(i+2);
            //else colNames<<"Vary?From..To"<<"Value-0"+QString::number(i+2)<<"Error-0"+QString::number(i+2);
            colNames<<"Vary?From..To"<<"Value #"+QString::number(i+2)<<"Error #"+QString::number(i+2);
            tablePara->insertColumn(4+3*i);
            tablePara->insertColumn(4+3*i+1);
            tablePara->insertColumn(4+3*i+2);
            tablePara->setColumnWidth(3*(i+1)+1,5*colReso);
            tablePara->setColumnWidth(3*(i+1)+2,5*colReso);
            tablePara->setColumnWidth(3*(i+1)+3,6*colReso);
            for (j=0;j<p;j++)
            {
                QTableWidgetItem *cb = new QTableWidgetItem();
                cb->setText("..");
                tablePara->setItem(j,3*(i+1)+1, cb);
                
            }
        }
        tablePara->setHorizontalHeaderLabels(colNames);
    }
    tablePara->blockSignals(false);
}

//*******************************************
// validator: Fit-conditions
//*******************************************
void fittable18::lineValidator()
{
    QString s;
    double num;
    // AbsErr
    num=lineEditAbsErr->text().toDouble();
    if (num<0) 	num=0;
    lineEditAbsErr->setText(QString::number(num));
    //RelErr
    num=lineEditRelErr->text().toDouble();
    if (num<1e-20 &&num>0 ) num=1e-20;
    lineEditRelErr->setText(QString::number(num));
    //fit tolerance
    num=lineEditTolerance->text().toDouble();
    if (num<0) 	num=0;
    lineEditTolerance->setText(QString::number(num));
}

//*******************************************
// headerPressedTablePara
//*******************************************
void fittable18::headerPressedTablePara( int col )
{
    if(col<0) return;
    
    if (col==0 || int((col-1)/3)*3==(col-1))
    {
        int p=spinBoxPara->value();
        int M=spinBoxNumberCurvesToFit->value();
        
        int pp, ppC=0;
        //+++ get np && params
        for (pp=0; pp<p;pp++)
        {
            QTableWidgetItem *itS = (QTableWidgetItem *)tablePara->item(pp,col); //
            if (itS->checkState()) ppC++;
        }
        
        for (pp=0; pp<p;pp++)
        {
            QTableWidgetItem *	itS = (QTableWidgetItem *)tablePara->item(pp,col); //
            if (ppC==p) itS->setCheckState(Qt::Unchecked);
            else itS->setCheckState(Qt::Checked);
        }
    }
    else if (int((col-2)/3)*3==(col-2))
    {
      fitOrCalculate(true, (int) ((col-1)/3));
    }
    else
    {
        Graph *g;
        if (!app()->findActiveGraph(g))
            return;

        bool maximaizedYN=false;
        MdiSubWindow *w;
        
        w= ( MdiSubWindow * ) app()->activeWindow();
        if (w->status() == MdiSubWindow::Maximized)
        {
            maximaizedYN=true;
        }
        
        QString nameesidular="fitCurve-"+textLabelFfunc->text();
        if (spinBoxNumberCurvesToFit->value()>1) nameesidular+="-global-"+QString::number(1+((int) ((col-1)/3)));
        nameesidular+="_residues";

        g->insertCurveScatter(nameesidular);
    }
}

//*******************************************
// vertHeaderPressedTablePara
//*******************************************
void fittable18::vertHeaderPressedTablePara( int raw)
{
    if(raw<0) return;
    
    int M=spinBoxNumberCurvesToFit->value();
    
    if (M>1)
    {
        QTableWidgetItem *itS = (QTableWidgetItem *)tablePara->item(raw,0); //
        
        if (itS->checkState()==Qt::Checked)
        {
            itS->setCheckState(Qt::Unchecked);
            
            for(int mm=0;mm<M;mm++)
            {
                QTableWidgetItem *itSm = (QTableWidgetItem *)tablePara->item(raw,3*mm+1);
                itSm->setCheckState(Qt::Checked);
                
            }
        }
        else
        {
            itS->setCheckState(Qt::Checked);
            
            
            for(int mm=0;mm<M;mm++)
            {
                QTableWidgetItem *itSm = (QTableWidgetItem *)tablePara->item(raw,3*mm+1);
                if (mm==0) itSm->setCheckState(Qt::Checked);
                else itSm->setCheckState(Qt::Unchecked);
                
            }
        }
    }
    
}

//*******************************************
// Multi-Fit-Table header
//*******************************************
void fittable18::headerTableMultiFit( int col )
{
    int numRows=tableMultiFit->rowCount();
    if (col>2 && numRows > 2 )
    {
        int i;
        for (i=2;i<numRows;i++)
        {
            tableMultiFit->item(i,col)->setText(tableMultiFit->item(1,col)->text());
        }
    }
}

//*******************************************
// Multi-Fit-Table header
//*******************************************
void fittable18::selectRowsTableMultiFit()
{
    return; // 2018 new
    int numRows=tableMultiFit->rowCount();
    int i;
    
    if (tableMultiFit->selectedRanges()[0].rightColumn()>0)return;
    for (i=1;i<numRows;i++)
    {
        if ( ((QTableWidgetItem *)tableMultiFit->item(i,0))->isSelected())
        {
            QTableWidgetItem *itS0 = (QTableWidgetItem *)tableMultiFit->item(i,0); //
            itS0->setCheckState(Qt::Checked);
        }
        
    }
}

//*******************************************
// rangeFirstCheck
//*******************************************
void fittable18::rangeFirstCheck()
{
    QString NQ=comboBoxSimQN->currentText();
    
    if (NQ=="N")
    {
        int min=textLabelRangeFirstLimit->text().toInt();
        int max=textLabelRangeLast->text().toInt();
        int value=textLabelRangeFirst->text().toInt();
        
        if (value<min || value >max) textLabelRangeFirst->setText(QString::number(min));
    }
    else
    {
        double min=textLabelRangeFirstLimit->text().toDouble();
        double max=textLabelRangeLast->text().toDouble();
        double value=textLabelRangeFirst->text().toDouble();
        
        if (value<min || value >max) textLabelRangeFirst->setText(QString::number(min));
    }
}

//*******************************************
// rangeLastCheck
//*******************************************
void fittable18::rangeLastCheck()
{
    QString NQ=comboBoxSimQN->currentText();
    
    if (NQ=="N")
    {
        int min=textLabelRangeFirst->text().toInt();
        int max=textLabelRangeLastLimit->text().toInt();
        int value=textLabelRangeLast->text().toInt();
        
        if (value<min || value >max) textLabelRangeLast->setText(QString::number(max));
    }
    else
    {
        double min=textLabelRangeFirst->text().toDouble();
        double max=textLabelRangeLastLimit->text().toDouble();
        double value=textLabelRangeLast->text().toDouble();
        
        if (value<min || value >max) textLabelRangeLast->setText(QString::number(max));
    }
}

//*******************************************
// selectActiveCurve
//*******************************************
bool fittable18::selectActiveCurve(int m)
{
    m=m-1;
    
    QString name;
    bool selectedRange;
    double min, max;

    if (!findActiveCurve(name, selectedRange, min, max))
        return false;

    tableCurves->item(0,2*m)->setText(name); tableCurvechanged(0,2*m); tableCurvechanged(0,2*m+1);
    tableCurves->item(0,2*m)->setText("");
    
    
    if (selectedRange)
    {
        QComboBoxInTable *iQ =(QComboBoxInTable*)tableCurves->cellWidget(1, 2*m);
        iQ->setCurrentIndex(1); tableCurvechanged(1,2*m);
        QTableWidgetItem *cbImin = (QTableWidgetItem *)tableCurves->item(2,2*m);
        cbImin->setCheckState(Qt::Checked); tableCurves->item(2,2*m+1)->setText(QString::number(min,'g',10));
        QTableWidgetItem *cbImax = (QTableWidgetItem *)tableCurves->item(3,2*m);
        cbImax->setCheckState(Qt::Checked); tableCurves->item(3,2*m+1)->setText(QString::number(max,'g',10));
    }
    else
    {
        QComboBoxInTable *iQ =(QComboBoxInTable*)tableCurves->cellWidget(1, 2*m);
        iQ->setCurrentIndex(0); tableCurvechanged(1,2*m);
    }
    
    return true;
}

//*******************************************
//+++ select data
//*******************************************
bool fittable18::selectData(int m, QString name)
{
    m=m-1;
    
    tableCurves->item(0,2*m)->setText(name); tableCurvechanged(0,2*m); tableCurvechanged(0,2*m+1);
    tableCurves->item(0,2*m)->setText("");
    if (((QComboBoxInTable*)tableCurves->cellWidget(0,2*m+1))->currentText().contains(name)) return true;
    return false;
}

//*******************************************
//+++ select range
//*******************************************
bool fittable18::selectRange(int m, QString QN, double min, double max)
{
    m=m-1;
    
    QComboBoxInTable *iQ =(QComboBoxInTable*)tableCurves->cellWidget(1, 2*m);
    if (QN=="N") iQ->setCurrentIndex(0); else iQ->setCurrentIndex(1);
    tableCurvechanged(1,2*m);
    
    if (min>max)
    {
        double tmp=max;
        max=min;
        min=tmp;
    }
    
    if (min==max) return false;
    
    
    QTableWidgetItem *cbImin = (QTableWidgetItem *)tableCurves->item(2,2*m);
    cbImin->setCheckState(Qt::Checked); tableCurves->item(2,2*m+1)->setText(QString::number(min,'g',10));
    QTableWidgetItem *cbImax = (QTableWidgetItem *)tableCurves->item(3,2*m);
    cbImax->setCheckState(Qt::Checked); tableCurves->item(3,2*m+1)->setText(QString::number(max,'g',10));
    
    tableCurvechanged(2,2*m+1);
    tableCurvechanged(3,2*m+1);
    
    return true;
}

//*******************************************
//+++ findActiveCurve
//*******************************************
bool fittable18::findActiveCurve(QString &name, bool &selectedRange, double &min, double &max)
{
    Graph *g;
    
    if (!app()->findActiveGraph(g))
    {
        QMessageBox::critical(this, tr("QtiSAS"), tr("Activate first GRAPH with data to fit !!!"));
        return false;
    };
    
    if (g->curveCount()==0) {QMessageBox::critical(this,tr("QtiKWS"), tr("Graph is EMPTY !!!")); return false;};
    
    
    selectedRange=true;
    
    if (g->rangeSelectorsEnabled())
    {
        // returns the curve range information as a string: "curve_name [start:end]"
        QString rangeInfo=g->curveRange(g->rangeSelectorTool()->selectedCurve());
        QStringList lst = rangeInfo.split(" ", Qt::SkipEmptyParts);
        name=lst[0];
        
        min=g->selectedXStartValue();
        max=g->selectedXEndValue();

    }
    else
    {
        QwtPlotItem *sp = (QwtPlotItem *)g->plotItem(0);
        if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
        {QMessageBox::critical(this,tr("QtiKWS"), tr("First curve is Spectrogram !!!")); return false;};
        
        selectedRange=false;
        
        // returns the curve range information as a string: "curve_name [start:end]"
        QString rangeInfo=g->curveRange((QwtPlotCurve *)g->curve(0));
        QStringList lst = rangeInfo.split(" ", Qt::SkipEmptyParts);
        name=lst[0];
        rangeInfo=lst[1].remove("[").remove("]");
        lst.clear();
        lst = rangeInfo.split(":", Qt::SkipEmptyParts);
        min=lst[0].toInt();
        max=lst[1].toInt();
    }
    
    if (min==max) return false;
    
    if (min>max)
    {
        double tmp=max;
        max=min;
        min=tmp;
    }
    
    return true;
}
