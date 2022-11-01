/***************************************************************************
 File                   : fittable-fitting.cpp
 Project                : QtiSAS
 --------------------------------------------------------------------
 Copyright              : (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)    : v.pipich*gmail.com
 Description            : table(s) fitting interface: fitting buttons functions
 
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
#include "fittable18.h"

//*******************************************
//  saveUndo
//*******************************************
void fittable18::saveUndo()
{
    //+++
    int M=spinBoxNumberCurvesToFit->value();		// Number of Curves
    int p=spinBoxPara->value();				//Number of Parameters per Curve
    int pM=p*M;					//Total Numbe
    
    QString undo;
    int pp, mm;
    
    for (mm=0;mm<M;mm++) for (pp=0;pp<p;pp++) undo+=QString::number(tablePara->item(pp, 3*mm+2)->text().toDouble(),'E',spinBoxSignDigits->value()-1)+"  ";
    
    undoRedo<<undo;
    
    pushButtonRedo->setEnabled(false);
    pushButtonUndo->setEnabled(true);
    
    undoRedoActive=undoRedo.count();
}

//*******************************************
//  undo
//*******************************************
void fittable18::undo()
{
    tablePara->blockSignals(true);//+++2019
    
    if(undoRedoActive>1)
    {
        undoRedoActive-=1;
        pushButtonRedo->setEnabled(true);
        
        //+++
        int M=spinBoxNumberCurvesToFit->value();		// Number of Curves
        int p=spinBoxPara->value();				//Number of Parameters per Curve
        int pM=p*M;						//Total Numbe
        
        QString undo=undoRedo[undoRedoActive-1];
        int pp=0, mm=0;
        
        int pos=0;
        
        QRegExp rx( "((\\-|\\+)?\\d*(\\.|\\,)\\d*((e|E)(\\-|\\+)\\d*)?)" );
        
        QString sss;
        
        for (mm=0;mm<M;mm++) for (pp=0;pp<p;pp++)
        {
            pos = rx.search( undo, pos ); pos+=rx.matchedLength();
            sss=rx.cap(1);
            sss=sss.replace(",", ".");
            tablePara->item(pp,3*mm+2)->setText(QString::number(sss.toDouble(),'G',spinBoxSignDigits->value()));

        }
        if(undoRedoActive==1) pushButtonUndo->setEnabled(false);
        if (checkBoxAutoRecalculate->isChecked()) plotSwitcher();
    }
    else pushButtonUndo->setEnabled(false);
    
    tablePara->blockSignals(false);
}

//*******************************************
//  redo
//*******************************************
void fittable18::redo()
{
    tablePara->blockSignals(true);//+++2019
    
    if(undoRedoActive<undoRedo.count())
    {
        undoRedoActive+=1;
        pushButtonUndo->setEnabled(true);
        
        //+++
        int M=spinBoxNumberCurvesToFit->value();		// Number of Curves
        int p=spinBoxPara->value();				//Number of Parameters per Curve
        int pM=p*M;						//Total Numbe
        
        QString undo=undoRedo[undoRedoActive-1];
        int pp=0, mm=0;
        
        int pos=0;
        
        //QRegExp rx( "((\\-|\\+)?\\d*(\\.|\\,)\\d*((e|E)(\\-|\\+)\\d*)?)" );
        QRegExp rx( "((\\-|\\+)?\\d*(\\.|\\,)\\d*((e|E)(\\-|\\+)\\d*)?)" );
        
        QString sss;
        
        for (mm=0;mm<M;mm++) for (pp=0;pp<p;pp++)
        {
            pos = rx.search( undo, pos ); pos+=rx.matchedLength();
            sss=rx.cap(1);
            sss=sss.replace(",", ".");
            tablePara->item(pp,3*mm+2)->setText(QString::number(sss.toDouble(),'G',spinBoxSignDigits->value()));
            
        }
        
        
        if(undoRedoActive==undoRedo.count()) pushButtonRedo->setEnabled(false);
        if (checkBoxAutoRecalculate->isChecked()) plotSwitcher();
    }
    else pushButtonRedo->setEnabled(false);
    
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
    
    if (toPlot) toPlot=findActivePlot(plot);

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
    bool maximaizedYN=false;
    if ( toPlot && plot->status() == MdiSubWindow::Maximized) maximaizedYN=true;
    
    
    
    //+++ Limits check
    chekLimits();
    chekLimitsAndFittedParameters();

    //+++
    bool fitOK=true;
    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++
    if (!calculateYN)
    {
        //+++ Results to Interface
        int M=spinBoxNumberCurvesToFit->value();    // Number of Curves
        int p=spinBoxPara->value();            //Number of Parameters per Curve
        
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

        int indexingColor=0;
        if (checkBoxColorIndexing->isChecked()) indexingColor=mm;
        if (!checkBoxColorIndexing->isChecked() && spinBoxCurrentFunction->value()>0) indexingColor=mm;
        
        if (toPlot)
        {
            if ( ((QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1))->currentText()!= "" )
                if (!AddCurve(g, ((QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1))->currentText())) return;
            addGeneralCurve(g, tableName, comboBoxColor->currentIndex()+indexingColor,ttt);
        }
        if (toPlot && !toPlotResidulasRight && toPlotResidulas && radioButtonSameQrange->isChecked()) addGeneralCurve(gR, tableName+"_residues", comboBoxColor->currentIndex()+indexingColor,ttt);
        else if (toPlot && toPlotResidulas && toPlotResidulasRight  && radioButtonSameQrange->isChecked()) addGeneralCurve(gR, tableName+"_residues", comboBoxColor->currentIndex()+mm,ttt,true);
    }

    //+++ save session
    if (checkBoxSaveSessionFit->isChecked()) saveFittingSession("fitCurve-"+textLabelFfunc->text()+"-session");

    if ( toPlot) app()->activateWindow((MdiSubWindow*)plot);
    
    //+++ chi2
    chi2();
    
    //+++save Undo
    saveUndo();
    
    if (maximaizedYN) app()->maximizeWindow(plot);
    if (toPlot) plot->blockSignals(false);
}
