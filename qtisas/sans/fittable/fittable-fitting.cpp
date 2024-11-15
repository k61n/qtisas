/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s)'s fitting buttons functions
 ******************************************************************************/

#include "fittable18.h"

//*******************************************
//  saveUndo
//*******************************************
void fittable18::saveUndo()
{
    int M = spinBoxNumberCurvesToFit->value(); // Number of Curves
    int p = spinBoxPara->value();              // Number of Parameters per Curve

    QString undo, undoErrorbars, undoFittable, undoShareble, undoRange;
    
    for (int mm = 0; mm < M; mm++)
        for (int pp = 0; pp < p; pp++)
        {
            auto itS = (QTableWidgetItem *)tablePara->item(pp, 3 * mm + 1);
            undoFittable += QString::number(Qt::CheckState(itS->checkState())) + " ";
            undoRange += itS->text().remove(" ") + " ";
            undo += tablePara->item(pp, 3 * mm + 2)->text() + " ";
            undoErrorbars += tablePara->item(pp, 3 * mm + 3)->text() + " ";
        }

    if (M > 1)
        for (int pp = 0; pp < p; pp++)
        {
            auto itA = (QTableWidgetItem *)tablePara->item(pp, 0);
            undoShareble += QString::number(Qt::CheckState(itA->checkState())) + " ";
        }

    undoRedo << undo;
    undoRedoErrorbars << undoErrorbars;
    undoRedoFittable << undoFittable;
    undoRedoRange << undoRange;
    undoRedoShareble << undoShareble;

    pushButtonRedo->setEnabled(false);
    pushButtonUndo->setEnabled(true);

    undoRedoActive = undoRedo.count();
}

//*******************************************
//  undo
//*******************************************
void fittable18::undo()
{
    if (undoRedo.count() == 0 || undoRedoErrorbars.count() == 0 || undoRedoFittable.count() == 0 ||
        undoRedoRange.count() == 0 || undoRedoActive < 1)
    {
        undoRedoActive = 0;
        pushButtonUndo->setEnabled(false);
        return;
    }

    int M = spinBoxNumberCurvesToFit->value(); // Number of Curves
    int p = spinBoxPara->value();              // Number of Parameters per Curve

    QStringList lstPara = undoRedo[undoRedoActive - 1].simplified().split(" ");
    QStringList lstParaErrorbars = undoRedoErrorbars[undoRedoActive - 1].simplified().split(" ");
    QStringList lstParaFittable = undoRedoFittable[undoRedoActive - 1].simplified().split(" ");
    QStringList lstParaRange = undoRedoRange[undoRedoActive - 1].simplified().split(" ");
    QStringList lstParaShareble = undoRedoShareble[undoRedoActive - 1].simplified().split(" ");

    if (lstPara.count() != p * M || lstParaErrorbars.count() != p * M || lstParaFittable.count() != p * M ||
        lstParaRange.count() != p * M)
        return;

    undoRedoActive--;
    pushButtonRedo->setEnabled(true);

    tablePara->blockSignals(true);

    for (int mm = 0; mm < M; mm++)
        for (int pp = 0; pp < p; pp++)
        {
            auto itS = (QTableWidgetItem *)tablePara->item(pp, 3 * mm + 1);
            itS->setCheckState(Qt::CheckState(lstParaFittable[mm * p + pp].toInt()));
            itS->setText(lstParaRange[mm * p + pp]);
            tablePara->item(pp, 3 * mm + 2)->setText(lstPara[mm * p + pp]);
            tablePara->item(pp, 3 * mm + 3)->setText(lstParaErrorbars[mm * p + pp]);
        }

    if (M > 1 && lstParaShareble.count() == p)
        for (int pp = 0; pp < p; pp++)
        {
            auto itA = (QTableWidgetItem *)tablePara->item(pp, 0);
            itA->setCheckState(Qt::CheckState(lstParaShareble[pp].toInt()));
        }

    if (undoRedoActive < 1)
        pushButtonUndo->setEnabled(false);

    pushButtonUndo->setFocus();
    if (checkBoxAutoRecalculate->isChecked())
        plotSwitcher();

    tablePara->blockSignals(false);
}

//*******************************************
//  redo
//*******************************************
void fittable18::redo()
{
    if (undoRedo.count() == 0 || undoRedoErrorbars.count() == 0 || undoRedoFittable.count() == 0 ||
        undoRedoRange.count() == 0 || undoRedoActive >= undoRedo.count() - 1)
    {
        pushButtonRedo->setEnabled(false);
        return;
    }

    int M = spinBoxNumberCurvesToFit->value(); // Number of Curves
    int p = spinBoxPara->value();              // Number of Parameters per Curve

    QStringList lstPara = undoRedo[undoRedoActive + 1].simplified().split(" ");
    QStringList lstParaErrorbars = undoRedoErrorbars[undoRedoActive + 1].simplified().split(" ");
    QStringList lstParaFittable = undoRedoFittable[undoRedoActive + 1].simplified().split(" ");
    QStringList lstParaRange = undoRedoRange[undoRedoActive + 1].simplified().split(" ");
    QStringList lstParaShareble = undoRedoShareble[undoRedoActive + 1].simplified().split(" ");

    if (lstPara.count() != p * M || lstParaErrorbars.count() != p * M || lstParaFittable.count() != p * M ||
        lstParaRange.count() != p * M)
        return;

    undoRedoActive++;
    pushButtonUndo->setEnabled(true);

    tablePara->blockSignals(true);

    for (int mm = 0; mm < M; mm++)
        for (int pp = 0; pp < p; pp++)
        {
            auto itS = (QTableWidgetItem *)tablePara->item(pp, 3 * mm + 1);
            itS->setCheckState(Qt::CheckState(lstParaFittable[mm * p + pp].toInt()));
            itS->setText(lstParaRange[mm * p + pp]);
            tablePara->item(pp, 3 * mm + 2)->setText(lstPara[mm * p + pp]);
            tablePara->item(pp, 3 * mm + 3)->setText(lstParaErrorbars[mm * p + pp]);
        }

    if (M > 1 && lstParaShareble.count() == p)
        for (int pp = 0; pp < p; pp++)
        {
            auto itA = (QTableWidgetItem *)tablePara->item(pp, 0);
            itA->setCheckState(Qt::CheckState(lstParaShareble[pp].toInt()));
        }

    if (undoRedoActive >= undoRedo.count() - 1)
        pushButtonRedo->setEnabled(false);

    pushButtonRedo->setFocus();
    if (checkBoxAutoRecalculate->isChecked())
        plotSwitcher();
    tablePara->blockSignals(false);
}

//*******************************************
//  changeFunctionLocal
//*******************************************
void fittable18::changeFunctionLocal(const QString& newFunction)
{
    //+++
    QString oldFunction=textLabelFfunc->text();
    //+++
    if(oldFunction==newFunction) return;
    //+++
    openDLL( newFunction);

    //+++
    initLimits();

    //+++
    initFitPage();

    //+++
    if (widgetStackFit->currentIndex()==2) for (int i=0;i<spinBoxPara->value();i++) tableParaSimulate->setItem(i,0,new QTableWidgetItem(tablePara->item(i,2)->text()));
}

//*********************************************************
//***  plotSwitcher:: calculate button was pressed
//*********************************************************
void fittable18::plotSwitcher()
{
    fitOrCalculate(true);
}

//*********************************************************
//***  fitSwitcher
//*********************************************************
void fittable18::fitSwitcher()
{
    fitOrCalculate(false);
}

//***************************************************
//  fitOrCalculate
//***************************************************
void fittable18::fitOrCalculate(bool calculateYN)
{
    fitOrCalculate(calculateYN, -1);
}

//***************************************************
//  fitOrCalculate
//***************************************************
void fittable18::fitOrCalculate(bool calculateYN, int mmm)
{
    // +++ Plot Fitting Curves via Simulate Interface
    
    bool toPlot=false;
    bool toPlotResidulas=false;
    bool toPlotResidulasRight=false;
    
    int plotDirector=comboBoxPlotActions->currentIndex();
    
    if (plotDirector<3) toPlot=true;
    if (plotDirector==1 || plotDirector==2) toPlotResidulas=true;
    if (plotDirector==2) toPlotResidulasRight=true;
    
    
    MultiLayer* plot;
    
    if (toPlot)
        toPlot = app()->findActivePlot(plot);

    if (toPlot) plot->blockSignals(true);

    //+++ to plot or not to plot
    Graph *g;
    Graph *gR;
    
    if (toPlot && plot->numLayers()<1) toPlot=false;
    
    if (toPlot && !toPlotResidulas)
    {
        g = (Graph*)plot->activeLayer();
        if (!g) toPlot=false;
    }
    else if (toPlot && toPlotResidulas && toPlotResidulasRight )
    {
        g = (Graph*)plot->activeLayer();
        if (g) gR=g;
        else {toPlot=false; toPlotResidulas=false;};
    }
    else if (toPlot && toPlotResidulas && !toPlotResidulasRight && plot->numLayers()>=2)
    {
        g=plot->layer(1);
        gR=plot->layer(2);
    }
    else toPlot=false;

    //+++ mazimaized plot?
    bool maximaizedYN = false;
    if (toPlot && plot->status() == MdiSubWindow::Maximized)
        maximaizedYN = true;

    bool toPlotAutoscaling = false;
    if (g && toPlot && g->isAutoscalingEnabled())
    {
        toPlotAutoscaling = true;
        g->enableAutoscaling(false);
    }

    bool toPlotResidulasAutoscaling = false;
    if (gR && toPlotResidulas && gR->isAutoscalingEnabled())
    {
        toPlotResidulasAutoscaling = true;
        gR->enableAutoscaling(false);
    }


    //+++ Limits check
    chekLimits();
    chekLimitsAndFittedParameters();

    //+++
    bool fitOK=true;

    int M=spinBoxNumberCurvesToFit->value();    // Number of Curves
    int p=spinBoxPara->value();            //Number of Parameters per Curve

    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++
    if (!calculateYN)
    {
        //+++ Results to Interface
        for (int pp=0; pp<p; pp++) for(int mm=0; mm<M; mm++) tablePara->item(pp,3*mm+3)->setText("---");
        
        if ( checkBoxSANSsupport->isChecked() )  fitOK = sansFit();
        else fitOK = simplyFit();
        
    }
    //++++++++++++++++++++++++++++++++++++++++++++++++++++
    
    // mmm=-1 all sets
    
    
    if (!fitOK)
    {
        if (toPlot) plot->blockSignals(false);
        return;
    }
  
    
    //+++ curves to generate/plot:
    //+++ mmm==-1  all [1..M]  (fitting interface)
    //
    int mmStart, mmFinish;
    
    if (mmm>-1)
    {
        mmStart=mmm;
        mmFinish=mmm+1;
    }
    else
    {
        mmStart=0;
        mmFinish=spinBoxNumberCurvesToFit->value();
    }

    //+++  currrent table namr
    QString tableName;
    //np,chi2,TSS
    int npAll=0; double chi2All=0; double TSSAll=0;
    
    //+++ finally actions
    for (int mm=mmStart; mm<mmFinish; mm++)
    {
        Table *ttt;
        int np=0; double chi2=0; double TSS=0;
        
        
        if (widgetStackFit->currentIndex()==1)
        {
            generateSimulatedTable(true,1,mm, false, tableName,ttt,np,chi2,TSS);
        }
        else
        {
            generateSimulatedTable(true,2,mm, false, tableName,ttt,np,chi2,TSS);
        }
        
        npAll+=np;
        chi2All+=chi2;
        TSSAll+=TSS;

        checkConstrains(mm);
        if (M>1 && mmm==-1 && mm==0 ) for (int pp=0;pp<p;pp++) checkGlobalParameters(pp,-2);
        
        int indexingColor=0;
        if (checkBoxColorIndexing->isChecked()) indexingColor=mm;
        if (!checkBoxColorIndexing->isChecked() && spinBoxCurrentFunction->value()>0) indexingColor=mm;
        
        if (toPlot)
        {
            auto cell = (QComboBoxInTable *)tableCurves->cellWidget(0, 2 * mm + 1);
            if (cell->currentText() != "")
                if (!g->insertCurveScatter(cell->currentText()))
                    return;
            addGeneralCurve(g, tableName, comboBoxColor->currentIndex()+indexingColor,ttt);
        }
        if (toPlot && !toPlotResidulasRight && toPlotResidulas && radioButtonSameQrange->isChecked()) addGeneralCurve(gR, tableName+"_residues", comboBoxColor->currentIndex()+indexingColor,ttt);
        else if (toPlot && toPlotResidulas && toPlotResidulasRight  && radioButtonSameQrange->isChecked()) addGeneralCurve(gR, tableName+"_residues", comboBoxColor->currentIndex()+mm,ttt,true);
    }

    //+++ save session
    if (checkBoxSaveSessionFit->isChecked()) saveFittingSession("fitCurve-"+textLabelFfunc->text()+"-session");
    
    //+++ chi2
    chi2();
    
    //+++save Undo
    if (!pushButtonUndo->hasFocus() && !pushButtonRedo->hasFocus())
        saveUndo();

    if (g && toPlot)
        g->enableAutoscaling(toPlotAutoscaling);

    if (gR && toPlotResidulas)
        gR->enableAutoscaling(toPlotAutoscaling);

    if (toPlot)
        plot->blockSignals(false);

    if (toPlot)
        app()->activateWindow((MdiSubWindow *)plot);

    if (toPlot && maximaizedYN)
        app()->maximizeWindow(plot);
}
