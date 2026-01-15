/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s)'s simulate tools
 ******************************************************************************/

#include "fittable18.h"

//***************************************************
//  Switcher:  simulate with SANS support or not
//***************************************************
void fittable18::simulateSwitcher(){
    if (radioButtonSameQrange->isChecked() && comboBoxDatasetSim->currentText()==""){
        QMessageBox::warning(this,tr("QtiSAS"),
                             tr("Dataset does not exist !!!"));
        return;
    }
    
    bool toPlot=false;
    bool toPlotResidulas=false;
    bool toPlotResidulasRight=false;
    
    int plotDirector=comboBoxPlotActions->currentIndex();
    
    if (plotDirector<3) toPlot=true;
    if (plotDirector==1 || plotDirector==2) toPlotResidulas=true;
    if (plotDirector==2) toPlotResidulasRight=true;
    
    if (!radioButtonSameQrange->isChecked()) toPlotResidulas=false;
    if (!toPlotResidulas) toPlotResidulasRight=false;
    
    MultiLayer* plot;
    
    if (toPlot)
        toPlot = app()->findActivePlot(plot);

    pushButtonSimulate->setFocus();
    
    Graph *g;
    Graph *gR;
    
    if (toPlot && !toPlotResidulas &&  plot->numLayers()>=1){
        g = (Graph*)plot->activeLayer();
        if (!g) toPlot=false;
    } else 
        if (toPlot && toPlotResidulas && toPlotResidulasRight &&  plot->numLayers()>=1){
            g = (Graph*)plot->activeLayer();
            if (!g) toPlot=false; else gR=g;
        } else 
            if (toPlot && toPlotResidulas && !toPlotResidulasRight && plot->numLayers()>1){
                g=plot->layer(1);
                gR=plot->layer(2);
            } else 
                if (toPlot &&  plot->numLayers()>=1){
                    g = (Graph*)plot->activeLayer();
                    if (!g) toPlot=false;
                    toPlotResidulas=false;
                    toPlotResidulasRight=false;
                }
                else 
                    toPlot=false;
    
    bool maximaizedYN=false;
    if ( toPlot && plot->status() == MdiSubWindow::Maximized) maximaizedYN=true;
    
    int mm=comboBoxDatasetSim->currentIndex();
    
    Table *ttt;
    QString tableName;
    //np,chi2,TSS
    int np=0;
    double chi2=0;
    double TSS=0;

    if (!generateSimulatedTable(true,0,mm, true, tableName,ttt,np,chi2,TSS)){
        QMessageBox::warning(this,tr("QtiSAS"),
                             tr("Dataset does not exist !!!"));
        return;
    }
    // +++ constrains
    checkConstrains(-1);
    
    int mmm=0;
    if (!textLabelFfunc->text().contains("superpositional-") && checkBoxSimIndexing->isChecked()){
        QString tName=ttt->name();
        QStringList lst;
        lst.clear();
        lst = tName.split("-", Qt::SkipEmptyParts);
        mmm=lst[lst.count()-1].toInt();
    }

    if (checkBoxSaveSession->isChecked()) saveFittingSessionSimulation(mm, tableName+"-session") ;

    int indexingColor=0;
    if (checkBoxColorIndexing->isChecked() && !checkBoxSuperpositionalFit->isChecked()) indexingColor=mmm;
    
    if (toPlot) addGeneralCurve(g, tableName, comboBoxColor->currentIndex()+indexingColor,ttt);
    
    if (toPlot && toPlotResidulas && !toPlotResidulasRight && radioButtonSameQrange->isChecked()) addGeneralCurve(gR, tableName+"_residues", comboBoxColor->currentIndex()+indexingColor,ttt);
   
    if (toPlot && toPlotResidulas && toPlotResidulasRight && radioButtonSameQrange->isChecked()) addGeneralCurve(gR, tableName+"_residues", comboBoxColor->currentIndex()+indexingColor,ttt,true);
    
    if ( toPlot) app()->activateWindow((MdiSubWindow*)plot);
    
    if ( toPlot && maximaizedYN){
        //app()->updateWindowLists ( plot );
        app()->modifiedProject (plot);
        plot->showMaximized();
    }
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++ simulate table :: single interfacef
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// createTable :: (true) generate table :: (false) only calculate chi2, np, tss (2016)
// source :: (0) simulate interf :: (1) fit interf :: (2) set-to-set interf
// m :: number of curve
// progressShow :: show progress for individual curve
bool fittable18::generateSimulatedTable(bool createTable, int source, int m, bool progressShow, QString &simulatedTable,
                                        Table *&ttt, int &np, double &chi2, double &TSS)
{
    //+++ move marameters of fit table [m] to simulation interface
    if (source > 0)
        datasetChangedSim(m);

    //+++ uniform or data defined x-points
    bool uniform = radioButtonUniform_Q->isChecked();
    if (!createTable)
        uniform = false;

    if (!uniform && comboBoxSimQN->currentText() == "N" && textLabelRangeLastLimit->text().toInt() <= 0)
    {
        QMessageBox::warning(this, "QtiSAS", "A problem with Reading Data");
        return false;
    }

    //+++ generate vectors::
    double *Q, *Idata, *Isim, *dI, *sigma, *weight, *sigmaf;

    //+++ number points
    int N;

    //+++ read data :: in case uniform range
    if (uniform && !SetQandIuniform(N, Q, sigma, m))
    {
        QMessageBox::warning(this, "QtiSAS", "A problem with Reading Data");
        return false;
    }

    //+++ read data :: in case table defined case
    if (!uniform && !SetQandIgivenM(N, Q, Idata, dI, sigma, weight, sigmaf, m))
    {
        QMessageBox::warning(this, "QtiSAS", "A problem with Reading Data");
        return false;
    }

    //+++ Simulated vector
    Isim = new double[N];
    if (!uniform)
    {
        for (int i = 0; i < N; i++)
            Isim[i] = Idata[i];
    }
    else
    {
        dI = new double[N];
        Idata = new double[N];
        weight = new double[N];
        sigmaf = new double[N];

        for (int i = 0; i < N; i++)
        {
            Isim[i] =0.0;
            Idata[i] =0.0;
            dI[i] =0.0;
            weight[i] = 1.0;
            sigmaf[i] = sigma[i];
        }
    }

    //+++ time of calculation ...
    QTime dt = QTime::currentTime();

    //+++
    if (!simulateData(N, Q, Isim, dI, sigma, sigmaf, progressShow))
    {
        QMessageBox::warning(this, "QtiSAS", "A problem with Simulation");
        delete[] Q;
        delete[] Idata;
        delete[] Isim;
        delete[] dI;
        delete[] sigma;
        delete[] weight;
        delete[] sigmaf;
        return false;
    }

    //+++ constrains
    checkConstrains(-1);
    if (source >= 0)
        checkConstrains(m);

    //+++ time of calculation ...
    textLabelTimeSim->setText(QString::number(dt.msecsTo(QTime::currentTime()), 'G', 3) + " ms");

    //+++ makeTable:: Q-I-dI-Sigma-Residulas:: Full info
    if (createTable && !simulateDataTable(source, m, simulatedTable, N, Q, Idata, weight, sigma, Isim, ttt))
    {
        QMessageBox::warning(this, "QtiSAS", "A problem with Creation of a table");

        delete[] Q;
        delete[] Idata;
        delete[] Isim;
        delete[] dI;
        delete[] sigma;
        delete[] weight;
        delete[] sigmaf;
        return false;
    }

    if (!uniform)
    {
        np = 0;
        int p = spinBoxPara->value();
        
        for (int pp = 0; pp < p; pp++)
            if (((QTableWidgetItem *)tablePara->item(pp, 3 * m + 1))->checkState())
                np++;

        //+++
        chi2 = 0;
        TSS = 0;
        double residues = 0;
        double data = 0;
        double Imean = 0;

        for (int n = 0; n < N; n++)
            Imean += Idata[n];
        Imean /= double(N);

        for (int n = 0; n < N; n++)
        {
            residues = Idata[n] - Isim[n];
            if (weight[n] != 0)
                residues /= weight[n];

            data = Idata[n] - Imean;
            if (weight[n] != 0)
                data /= weight[n];

            chi2 += residues * residues;
            TSS += data * data;
        }

        int prec = spinBoxSignDigits->value();
        
        double R2 = 0.0;
        if (TSS > 0.0)
            R2 = 1.0 - (chi2 * 1e6) / (TSS * 1e6);
        
        textLabelDofSim->setText(QString::number(N));
        textLabelChi2Sim->setText(QString::number(chi2, 'E', prec + 2));
        textLabelChi2dofSim->setText(QString::number(chi2 / (N - np), 'E', prec + 2));
        textLabelnpSIM->setText(QString::number(np));
        textLabelR2sim->setText(QString::number(R2, 'E', prec + 2));

        int maxInfoCount = 11;
        if (checkBoxSANSsupport->isChecked())
            maxInfoCount = 16;

        ttt->blockSignals(true);

        if (ttt->numRows() < maxInfoCount + 9)
            ttt->setNumRows(maxInfoCount + 9);
        int currentLine = maxInfoCount;

        ttt->setText(currentLine,5,"N");
        ttt->setText(currentLine,6,"->   "+QString::number(N));
        currentLine++;
        ttt->setText(currentLine,5,"np");
        ttt->setText(currentLine,6,"->   "+QString::number(np));
        currentLine++;
        ttt->setText(currentLine,5,"DoF=N-np");
        ttt->setText(currentLine,6,"->   "+QString::number(N-np));
        currentLine++;
        ttt->setText(currentLine,5,"chi^2");
        ttt->setText(currentLine,6,"->   "+QString::number(chi2,'E',prec+2));
        currentLine++;
        ttt->setText(currentLine,5,"chi^2/DoF");
        ttt->setText(currentLine,6,"->   "+QString::number(chi2/(N-np),'E',prec+2));
        currentLine++;
        ttt->setText(currentLine,5,"TSS");
        ttt->setText(currentLine,6,"->   "+QString::number(TSS,'E',prec+2));
        currentLine++;
        ttt->setText(currentLine,5,"R^2=1-chi^2/TSS");
        ttt->setText(currentLine,6,"->   "+QString::number(R2,'E',prec+2));
        currentLine++;
        ttt->setText(currentLine,5,"R=1-sqrt(chi^2/TSS)");
        ttt->setText(currentLine,6,"->   "+QString::number(1-sqrt(chi2/TSS),'E',prec+2));

        currentLine++;
        ttt->setText(currentLine, 5, "Q-Factor");
        ttt->setText(currentLine, 6,
                     "->   " + QString::number(gsl_sf_gamma_inc_Q(double(N - np) / 2.0, chi2 / 2.0), 'E', prec + 2));
        currentLine++;
        ttt->blockSignals(false);
    }
    //+++ clear memory
    delete[] Idata;
    delete[] dI;
    delete[] weight;
    delete[] Q;
    delete[] sigma;
    delete[] sigmaf;
    delete[] Isim;

    return true;
}
//***************************************************
//  checkConstrains
//***************************************************
void fittable18::checkConstrains(int m){
    int M=spinBoxNumberCurvesToFit->value();
    int P=spinBoxPara->value();
    int prec=spinBoxSignDigits->value();
    
    if (m<-1) return;
    if (m>=M) return;
    
    tablePara->blockSignals(true);
    for (int pp=0; pp<P; pp++){
        if (m>-1)
            tablePara->item(pp,3*m+2)->setText(QString::number(gsl_vector_get(F_para,pp),'G',prec));
        else
            tableParaSimulate->item(pp,0)->setText(QString::number(gsl_vector_get(F_para,pp),'G',prec));
    }
    tablePara->blockSignals(false);
}
//++++++++++++++++++++++++++++++++++++++++
//+++ Add/update fitted/simulated lines
//++++++++++++++++++++++++++++++++++++++++
bool fittable18::addGeneralCurve(Graph *g, QString tableName, int m, Table *&table, bool rightYN)
{
    if (!g)
        return false;

    if (!tableName.endsWith("_y") && !tableName.endsWith("_residues"))
        tableName = tableName + "_y";

    int xColIndex, yColIndex;
    if (!findFitDataTableDirect(tableName, table, xColIndex, yColIndex))
        return false;

    m--;
    int mmax = int(app()->indexedColors().count());
    int color = !checkBoxColorIndexing->isChecked() ? comboBoxColor->currentIndex() : (m + 1) % mmax;

    CurveLayout cl;
    cl.connectType = 1;
    cl.lStyle = 0;
    cl.lWidth = app()->defaultCurveLineWidth + 1;
    cl.sSize = app()->defaultSymbolSize;
    cl.sType = 0;
    cl.filledArea = 0;
    cl.aCol =  getColor(color);
    cl.aStyle = 0;
    cl.lCol = getColor(color);
    cl.penWidth = 1;
    cl.symCol = getColor(color);
    cl.fillCol = getColor(color);

    int style = Graph::Line;

    if (table && !g->curveNamesList().contains(tableName))
    {
        g->insertCurve(table, tableName, style)->setAxis(0 + 2, int(rightYN));

        const auto index = g->curveNamesList().indexOf(tableName);
        if (index >= 0)
            g->updateCurveLayout(g->curve(static_cast<int>(index)), &cl);
    }
    else
    {
        const auto index = g->curveNamesList().indexOf(tableName);
        if (index < 0)
            return false;

        PlotCurve *c = g->curve(static_cast<int>(index));

        if (!c)
            return false;

        auto *dc = dynamic_cast<DataCurve *>(c);

        if (dc && !dc->isFullRange())
            g->setCurveFullRange(static_cast<int>(index));
    }

    return true;
}
//*******************************************
// slot: make NEW table with fit results
//*******************************************
void fittable18::saveFittingSessionSimulation(int m, const QString &table)
{
    bool ok;
    int p=spinBoxPara->value();
    int M=1;  //+++ single dataset
    QString F_name=textLabelFfunc->text();
    if (m < 0)
        m = 0;

    Table* w;

    if (app()->checkTableExistence(table, w))
    {
        if (w->windowLabel() != "FIT1D::Settings::Table")
            return;
        w->setNumRows(0);
        w->setNumCols(2);
    } else{
        w=app()->newHiddenTable(table,"FIT1D::Settings::Table", 0, 2);
        //+++ new
        w->setWindowLabel("FIT1D::Settings::Table");
        app()->setListViewLabel(w->name(), "FIT1D::Settings::Table");
        app()->updateWindowLists(w);
    }

    w->blockSignals(true);
    // Col-Names
    w->setColName(0, "Parameter");
    w->setColPlotDesignation(0, Table::None);
    w->setColumnType(0, Table::Text);

    w->setColName(1, "Parameter-Value");
    w->setColPlotDesignation(1, Table::None);
    w->setColumnType(1, Table::Text);

    int currentRow=0;
    QString s;
    //----- Function::Folder
    w->setNumRows(currentRow+1);
    s=libPath+" <";
    w->setText(currentRow, 0, "Function::Folder");
    w->setText(currentRow, 1, s);
    currentRow++;
    //----- Function::Name
    w->setNumRows(currentRow+1);
    s=textLabelFfunc->text()+" <";
    w->setText(currentRow, 0, "Function::Name");
    w->setText(currentRow, 1, s);
    currentRow++;
    //+++ Function::Parameters::Number
    w->setNumRows(currentRow+1);
    s=QString::number(p)+" ";
    w->setText(currentRow,0,"Function::Parameters::Number");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    //+++ Function::SANS::Support
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Function::SANS::Support");
    if (checkBoxSANSsupport->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    //+++ Function::Global::Fit
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Function::Global::Fit");
    w->setText(currentRow,1,"no <");  // non-global 2016
    currentRow++;
    //+++ Function::Global::Fit::Number
    w->setNumRows(currentRow+1);
    s=QString::number(M)+" ";
    w->setText(currentRow,0,"Function::Global::Fit::Number");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Data::Datasets   0
    w->setNumRows(currentRow+1);
    s=((QComboBoxInTable*)tableCurves->cellWidget(0, 2*m+1))->currentText()+" "; // 2016
    w->setText(currentRow,0,"Session::Data::Datasets");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Data::N 1a
    w->setNumRows(currentRow+1);
    s=((QComboBoxInTable*)tableCurves->cellWidget(1, 2*m))->currentText()+" "; // 2016
    w->setText(currentRow,0,"Session::Data::N");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Data::NN 1b
    w->setNumRows(currentRow+1);
    s=tableCurves->item(1,2*m+1)->text()+" ";
    w->setText(currentRow,0,"Session::Data::NN");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Data::From::Use
    w->setNumRows(currentRow+1);
    s="";
    if (( (QTableWidgetItem *)tableCurves->item (2,2*m) )->checkState()) s+="1 "; else s+="0 ";
    w->setText(currentRow,0,"Session::Data::From::Use");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Data::From::Number
    w->setNumRows(currentRow+1);
    s=tableCurves->item(2,2*m+1)->text()+" ";
    w->setText(currentRow,0,"Session::Data::From::Number");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Data::To::Use
    w->setNumRows(currentRow+1);
    s="";
    if (( (QTableWidgetItem *)tableCurves->item (3,2*m) )->checkState()) s+="1 "; else s+="0 ";
    w->setText(currentRow,0,"Session::Data::To::Use");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Data::To::Number
    w->setNumRows(currentRow+1);
    s=tableCurves->item(3,2*m+1)->text()+" ";
    w->setText(currentRow,0,"Session::Data::To::Number");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Weighting::Use
    w->setNumRows(currentRow+1);
    s="";
    if (( (QTableWidgetItem *)tableCurves->item (4,2*m) )->checkState()) s+="1 "; else s+="0 ";
    w->setText(currentRow,0,"Session::Weighting::Use");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Weighting::Dataset
    w->setNumRows(currentRow+1);
    s=((QComboBoxInTable*)tableCurves->cellWidget(4, 2*m+1))->currentText()+" ";
    w->setText(currentRow,0,"Session::Weighting::Dataset");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Limits::Left
    w->setNumRows(currentRow+1);
    s="";
    for (int pp=0; pp<p;pp++)
        s+=tableControl->item(pp,0)->text()+" ";
    w->setText(currentRow,0,"Session::Limits::Left");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Limits::Right
    w->setNumRows(currentRow+1);
    s="";
    for (int pp=0; pp<p;pp++)
        s+=tableControl->item(pp,4)->text()+" ";
    w->setText(currentRow,0,"Session::Limits::Right");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    if (checkBoxSANSsupport->isChecked()){
        //+++ Session::Resolution::Use
        w->setNumRows(currentRow+1);
        s="";
        if (checkBoxResoSim->isChecked()) s+="1 "; else s+="0 ";
        w->setText(currentRow,0,"Session::Resolution::Use");
        w->setText(currentRow,1,s+"<");
        currentRow++;
        //+++ Session::Resolution::Datasets
        w->setNumRows(currentRow+1);
        s=comboBoxResoSim->currentText()+" ";
        w->setText(currentRow,0,"Session::Resolution::Datasets");
        w->setText(currentRow,1,s+"<");
        currentRow++;
        //+++ Session::Polydispersity::Use
        w->setNumRows(currentRow+1);
        s="";
        if (checkBoxPolySim->isChecked()) s+="1 "; else s+="0 ";
        w->setText(currentRow,0,"Session::Polydispersity::Use");
        w->setText(currentRow,1,s+"<");
        currentRow++;
        //+++ Session::Polydispersity::Datasets
        w->setNumRows(currentRow+1);
        s=comboBoxPolySim->currentText()+" ";
        w->setText(currentRow,0,"Session::Polydispersity::Datasets");
        w->setText(currentRow,1,s+"<");
        currentRow++;
        //+++ Session::Options::Reso
        w->setNumRows(currentRow+1);
        s=QString::number(comboBoxResoFunction->currentIndex())+" ";
        s+=QString::number(comboBoxSpeedControlReso->currentIndex())+" ";
        s+=lineEditAbsErr->text()+" ";
        s+=lineEditRelErr->text()+" ";
        s+=QString::number(spinBoxIntWorkspase->value())+" ";
        s+=QString::number(spinBoxIntLimits->value());
        w->setText(currentRow,0,"Session::Options::Reso");
        w->setText(currentRow,1,s+" <");
        currentRow++;
        //+++ Session::Options::Poly
        w->setNumRows(currentRow+1);
        s=QString::number(comboBoxPolyFunction->currentIndex())+" ";
        s+=QString::number(comboBoxSpeedControlPoly->currentIndex())+" ";
        s+=lineEditAbsErrPoly->text()+" ";
        s+=lineEditRelErrPoly->text()+" ";
        s+=QString::number(spinBoxIntWorkspasePoly->value())+" ";
        s+=QString::number(spinBoxIntLimitsPoly->value());
        w->setText(currentRow,0,"Session::Options::Poly");
        w->setText(currentRow,1,s+" <");
        currentRow++;
    }
    //+++ Session::Options::Fit::Control
    w->setNumRows(currentRow+1);
    s=QString::number(comboBoxFitMethod->currentIndex())+" ";
    s+=QString::number(comboBoxSimplex->currentIndex())+" ";
    s+=QString::number(comboBoxLevenberg->currentIndex())+" ";
    s+=QString::number(spinBoxMaxIter->value())+" ";
    s+=lineEditTolerance->text()+" ";
    s+=lineEditToleranceAbs->text()+" ";
    s+=QString::number(spinBoxSignDigits->value())+" ";
    s+=QString::number(comboBoxWeightingMethod->currentIndex())+" ";
    if (checkBoxCovar->isChecked()) s+="1 "; else s+="0 ";
    s += QString::number(spinBoxWA->value()) + " ";
    s += QString::number(spinBoxWB->value()) + " ";
    s += QString::number(spinBoxWC->value()) + " ";
    s += QString::number(spinBoxWXMAX->value()) + " ";
    s+=QString::number(spinBoxGenomeCount->value())+" ";
    s+=QString::number(spinBoxGenomeSize->value())+" ";
    s+=QString::number(spinBoxMaxNumberGenerations->value())+" ";
    s+=lineEditSelectionRate->text()+" ";
    s+=lineEditMutationRate->text()+" ";
    s+=QString::number(spinBoxRandomSeed->value())+" ";
    w->setText(currentRow,0,"Session::Options::Fit::Control");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Options::Instrument::Reso
    w->setNumRows(currentRow+1);
    s=QString::number(comboBoxResoFunction->currentIndex())+" ";
    s+=QString::number(comboBoxSpeedControlReso->currentIndex())+" ";
    s+=lineEditAbsErr->text()+" ";
    s+=lineEditRelErr->text()+" ";
    s+=QString::number(spinBoxIntWorkspase->value())+" ";
    s+=QString::number(spinBoxIntLimits->value())+" ";
    w->setText(currentRow,0,"Session::Options::Instrument::Reso");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++ Session::Options::Instrument::Poly
    w->setNumRows(currentRow+1);
    s=QString::number(comboBoxPolyFunction->currentIndex())+" ";
    s+=QString::number(comboBoxSpeedControlPoly->currentIndex())+" ";
    s+=lineEditAbsErrPoly->text()+" ";
    s+=lineEditRelErrPoly->text()+" ";
    s+=QString::number(spinBoxIntWorkspasePoly->value())+" ";
    s+=QString::number(spinBoxIntLimitsPoly->value())+" ";
    w->setText(currentRow,0,"Session::Options::Instrument::Poly");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    //+++
    for (int pp=0;pp<p;pp++){
        QString uselName="Session::Parameters::Use::"+QString::number(pp+1);
        //+++ Session::Parameters::Use::pp+1
        w->setNumRows(currentRow+1);
        s="";
        if (( (QTableWidgetItem *)tablePara->item (pp,3*m+1) )->checkState()) s+="1 "; else s+="0 ";
        w->setText(currentRow,0,uselName);
        w->setText(currentRow,1,s+" <");
        currentRow++;
        QString cellName="Session::Parameters::Values::"+QString::number(pp+1);
        //+++Session::Parameters::Values::pp+1
        w->setNumRows(currentRow+1);
        s=tableParaSimulate->item(pp,0)->text()+" ";
        w->setText(currentRow,0,cellName);
        w->setText(currentRow,1,s+"<");
        currentRow++;
        QString rangeName="Session::Parameters::Ranges::"+QString::number(pp+1);
        //+++Session::Parameters::Ranges::pp+1
        w->setNumRows(currentRow+1);
        s="";
        QTableWidgetItem *range = (QTableWidgetItem *)tablePara->item(pp,3*m+1);
        s=range->text()+" ";
        w->setText(currentRow,0,rangeName);
        w->setText(currentRow,1,s+"<");
        currentRow++;
    }
    //+++Session::Parameters::Errors
    w->setNumRows(currentRow+1);
    w->setText(currentRow, 0, "Session::Parameters::Errors");
    s="";
    for (int pp=0;pp<p;pp++) s+=tablePara->item(pp,3*m+3)->text()+" ";
    w->setText(currentRow,1,s+" <");
    currentRow++;
    //+++ Session::Fit::SaveSession
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Session::Fit::SaveSession");
    if (checkBoxSaveSessionFit->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    //----- Session::Chi2
    w->setNumRows(currentRow+1);
    s=textLabelChi2dofSim->text()+" <";
    w->setText(currentRow, 0, "Session::Chi2");
    w->setText(currentRow, 1, s);
    currentRow++;
    //----- Session::R2
    w->setNumRows(currentRow+1);
    s=textLabelR2sim->text()+" <";
    w->setText(currentRow, 0, "Session::R2");
    w->setText(currentRow, 1, s);
    currentRow++;
    //----- Session::Time
    w->setNumRows(currentRow+1);
    s=textLabelTimeSim->text()+" <";
    w->setText(currentRow, 0, "Session::Time");
    w->setText(currentRow, 1, s);
    currentRow++;
    //----- Simulate::Control
    w->setNumRows(currentRow + 1);
    s = QString::number(comboBoxPlotActions->currentIndex()) + " <";
    w->setText(currentRow, 0, "Simulate::Control");
    w->setText(currentRow, 1, s);
    currentRow++;
    //----- Simulate::Color
    w->setNumRows(currentRow+1);
    s=QString::number(comboBoxColor->currentIndex())+" <";
    w->setText(currentRow, 0, "Simulate::Color");
    w->setText(currentRow, 1, s);
    currentRow++;
    //+++ Simulate::Color::Indexing
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::Color::Indexing");
    if (checkBoxColorIndexing->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    //+++ Simulate::Statistics
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::Statistics");
    if (checkBoxCovar->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    //+++ Simulate::SaveSession
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::SaveSession");
    if (checkBoxSaveSession->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    //+++ Simulate::Indexing
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::Indexing");
    if (checkBoxSimIndexing->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    //+++ Simulate::Uniform
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::Uniform");
    if (radioButtonUniform_Q->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    //+++ Simulate::Uniform::Parameters
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::Uniform::Parameters");
    s="";
    s+=lineEditFromQsim->text()+" ";
    s+=lineEditToQsim->text()+" ";
    s+=lineEditNumPointsSim->text()+" ";    
    if (checkBoxLogStep->isChecked()){
        s+="1 ";
        s+=lineEditImin->text()+" ";
    } else 
        s+="0 0 ";
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    w->adjustColumnsWidth(false);

    w->blockSignals(false);
}
//***************************************************
// +++  Set Q and I  :: uniform range
//***************************************************
bool fittable18::SetQandIuniform(int &N, double* &QQ, double* &sigmaQ, int m){
    int i;
    //+++
    bool SANSsupport=checkBoxSANSsupport->isChecked();
    //+++
    int M=spinBoxNumberCurvesToFit->value();  if(m>=M) return false;
    int p=spinBoxPara->value(); if (p==0) return false;
    //+++
    N=lineEditNumPointsSim->text ().toInt();
    
    sizetNumbers sizetNumbers={static_cast<size_t>(N),
                               1,
                               static_cast<size_t>(p),
                               static_cast<size_t>(p)};
    
    double Qmin=lineEditFromQsim->text ().toDouble();
    double Qmax=lineEditToQsim->text ().toDouble();
    //+++
    double *Q=new double[N];
    double *sigma=new double[N];
    //+++  Q [i]  calculation
    for (i=0;i<N;i++){
        if (checkBoxLogStep->isChecked()) 
            Q[i]=pow(10, (log10(Qmin)+(log10(Qmax)-log10(Qmin))/ (N-1)*i));
        else 
            Q[i]=Qmin+(Qmax-Qmin)/ (N-1) *i;
    }
    // sigmaReso[i] calculation
    if (SANSsupport)
    {
        if (comboBoxResoSim->currentText().contains("ASCII.1D.SANS"))
            for (int i = 0; i < N; i++)
                sigma[i] = app()->sigma(Q[i]);
        else if (comboBoxResoSim->currentText().contains("20%"))
            for (i = 0; i < N; i++)
                sigma[i] = 0.20 * Q[i];
        else if (comboBoxResoSim->currentText().contains("10%"))
            for (i = 0; i < N; i++)
                sigma[i] = 0.10 * Q[i];
        else if (comboBoxResoSim->currentText().contains("05%"))
            for (i = 0; i < N; i++)
                sigma[i] = 0.05 * Q[i];
        else if (comboBoxResoSim->currentText().contains("02%"))
            for (i = 0; i < N; i++)
                sigma[i] = 0.02 * Q[i];
        else if (comboBoxResoSim->currentText().contains("01%"))
            for (i = 0; i < N; i++)
                sigma[i] = 0.01 * Q[i];
        else if (comboBoxResoSim->currentText() == "from_SPHERES")
            for (i = 0; i < N; i++)
                sigma[i] = app()->sigma(Q[i]);
        else
        {
            //~~~ DEFINE table name
            QString tableName = comboBoxResoSim->currentText().left(comboBoxResoSim->currentText().indexOf('_'));
            QString sigmaName = comboBoxResoSim->currentText().right(comboBoxResoSim->currentText().indexOf('_') + 1);

            //~~~ check of existence of table
            bool exist = false;
            Table *t;

            exist = app()->checkTableExistence(tableName, t);
            if (!exist)
            {
                QMessageBox::warning(this, tr("QtiSAS"), "There is no table:: " + tableName);
                return false;
            }
            int colIndexSigma = t->colIndex(sigmaName);
            if (colIndexSigma < 1)
            {
                QMessageBox::warning(this, tr("QtiSAS"), "Problem with Sigma 2: " + sigmaName);
                return false;
            }
            //~~~ number of points
            int Nsigma = 0;
            static const QRegularExpression rx(R"(((\-|\+)?\d*(\.|\,)\d*((e|E)(\-|\+)\d*)?)|((\-|\+)?\d+))");
            for (i = 0; i < t->numRows(); i++)
                if (rx.match(t->text(i, 0)).hasMatch() && rx.match(t->text(i, colIndexSigma)).hasMatch())
                    Nsigma++;

            if (Nsigma < 3)
            {
                QMessageBox::warning(this, tr("QtiSAS"), "Sigma problem 3");
                return false;
            }

            auto *QQsigma = new double[Nsigma];
            auto *sigmaSigma = new double[Nsigma];
            Nsigma = 0;
            double Qmin = 1;
            double Qmax = 0;
            double sigmaMin, sigmaMax;

            for (i = 0; i < t->numRows(); i++)
                if (rx.match(t->text(i, 0)).hasMatch() && rx.match(t->text(i, colIndexSigma)).hasMatch())
                {
                    QQsigma[Nsigma] = t->text(i, 0).toDouble();
                    sigmaSigma[Nsigma] = t->text(i, colIndexSigma).toDouble();
                    if (QQsigma[Nsigma] < Qmin)
                    {
                        Qmin = QQsigma[Nsigma];
                        sigmaMin = sigmaSigma[Nsigma];
                    }
                    if (QQsigma[Nsigma] > Qmax)
                    {
                        Qmax = QQsigma[Nsigma];
                        sigmaMax = sigmaSigma[Nsigma];
                    }
                    Nsigma++;
                }

            gsl_interp_accel *acc = gsl_interp_accel_alloc();
            gsl_spline *spline = gsl_spline_alloc(gsl_interp_cspline, Nsigma);
            gsl_spline_init(spline, QQsigma, sigmaSigma, Nsigma);

            if (comboBoxInstrument->currentText().contains("Back"))
            {
                int Ntotal;
                double *Qtotal, *Itotal, *dItotal, *Sigmatotal, *Weighttotal, *Sigmaftotal;
                int m = comboBoxDatasetSim->currentIndex();

                SetQandIgivenM(Ntotal, Qtotal, Itotal, dItotal, Sigmatotal, Weighttotal, Sigmaftotal, m);

                if (Ntotal > 21)
                {
                    sigmaMin = Sigmatotal[0] + Sigmatotal[1] + Sigmatotal[2] + Sigmatotal[3] + Sigmatotal[4];
                    sigmaMin += Sigmatotal[5] + Sigmatotal[6] + Sigmatotal[7] + Sigmatotal[8] + Sigmatotal[9];
                    sigmaMin = sigmaMin / 10;

                    sigmaMax = Sigmatotal[Ntotal - 1] + Sigmatotal[Ntotal - 2] + Sigmatotal[Ntotal - 3] +
                               Sigmatotal[Ntotal - 4] + Sigmatotal[Ntotal - 5];
                    sigmaMax += Sigmatotal[Ntotal - 6] + Sigmatotal[Ntotal - 7] + Sigmatotal[Ntotal - 8] +
                                Sigmatotal[Ntotal - 9] + Sigmatotal[Ntotal - 10];
                    sigmaMax = sigmaMax / 10;
                }

                for (i = 0; i < N; i++)
                    if (Q[i] < Qtotal[0])
                        sigma[i] = sigmaMin;
                    else if (Q[i] > Qtotal[Ntotal - 1])
                        sigma[i] = sigmaMax;
                    else
                        sigma[i] = gsl_spline_eval(spline, Q[i], acc);

                delete[] Qtotal;
                delete[] Itotal;
                delete[] dItotal;
                delete[] Sigmatotal;
                delete[] Weighttotal;
            }
            else
            {
                for (i = 0; i < N; i++)
                {
                    if (Q[i] < Qmin)
                    {
                        if (Qmin != 0)
                            sigma[i] = sigmaMin / Qmin * Q[i];
                        else
                            sigma[i] = -911119.119911;
                    }
                    else if (Q[i] > Qmax)
                    {
                        if (Qmax != 0)
                            sigma[i] = sigmaMax / Qmax * Q[i];
                        else
                            sigma[i] = -911119.119911;
                    }
                    else
                        sigma[i] = gsl_spline_eval(spline, Q[i], acc);
                }
            }

            if (!checkBoxResoSim->isChecked())
                for (i = 0; i < N; i++)
                    sigma[i] = 0.0 - fabs(sigma[i]);

            delete[] QQsigma;
            delete[] sigmaSigma;
        }
    }
    else
        for (i = 0; i < N; i++)
            sigma[i] = 0.00;

    int Nfinal = 0;
    for (i = 0; i < N; i++)
        if (sigma[i] != -911119.119911)
            Nfinal++;

    if (Nfinal == 0)
    {
        delete[] Q;
        delete[] sigma;
        return false;
    }

    QQ = new double[Nfinal];
    sigmaQ = new double[Nfinal];
    int prec = spinBoxSignDigits->value();

    Nfinal=0;
    for (i=0;i<N;i++) if ( Q[i]!=-911119.119911)
    {
        QQ[Nfinal]	= Q[i];//round2prec( Q[i], prec);
        sigmaQ[Nfinal]	= sigma[i];//round2prec( sigma[i], prec);
        Nfinal++;
    }
    
    N=Nfinal;
    delete[] Q;
    delete[] sigma;
    return true;
}

//***************************************************
//*** NEW :: SetQandIgivenM :: 14-06-2019
//***************************************************
bool fittable18::SetQandIgivenM(int &Ntotal, double*&Qtotal, double*&Itotal, double*&dItotal, double*&Sigmatotal, double*&Weighttotal, double*&Sigmaftotal, int m)
{
    
    //++++++++++++++++++
    double min, max;
    int minN, maxN;
    bool weightYN, resoYN;
    //++++++++++++++++++
    
    //+++
    bool SANSsupport=checkBoxSANSsupport->isChecked();
    //+++
    int M=spinBoxNumberCurvesToFit->value();  if(m>=M) return false;

    //+++
    QComboBoxInTable *curve = (QComboBoxInTable*)tableCurves->cellWidget(0,2*m+1);
    if (curve->count() == 0)
        return false;

    QString curveName = curve->currentText();
    QString tableName = curveName.left(curveName.lastIndexOf("_"));
    QString colName = curveName.remove(tableName);

    Table *t;
    if (!app()->checkTableExistence(tableName, t))
        return false;

    int yColIndex = t->colIndex(colName);
    int xColIndex = t->colX(yColIndex);
    if (yColIndex < 0 || xColIndex < 0)
        return false;

    //+++
    size_t N=t->numRows();
    
    //+++
    bool NnotQ =true;
    if (((QComboBoxInTable*)tableCurves->cellWidget(1, 2*m))->currentText()!="N") NnotQ=false;
    
    //+++ Min Max
    min=tableCurves->item(2,2*m+1)->text().toDouble();
    max=tableCurves->item(3,2*m+1)->text().toDouble();
    minN=tableCurves->item(2,2*m+1)->text().toInt();
    maxN=tableCurves->item(3,2*m+1)->text().toInt();
    
    if (widgetStackFit->currentIndex()==2)
    {
        NnotQ =true;
        if (comboBoxSimQN->currentText()!="N") NnotQ=false;
        
        //+++ Min Max
        min=textLabelRangeFirst->text().toDouble();
        max=textLabelRangeLast->text().toDouble();
        minN=textLabelRangeFirst->text().toInt();
        maxN=textLabelRangeLast->text().toInt();
    }
    
    //+++  weight  dI
    weightYN=false;
    QString colWeight="";
    
    if (( (QTableWidgetItem *)tableCurves->item(4,2*m) )->checkState()) weightYN=true;
    colWeight=((QComboBoxInTable*)tableCurves->cellWidget(4, 2*m+1))->currentText();
    
    
    if (textLabelCenter->text().contains("Generate Results") && tabWidgetGenResults->currentIndex()==0 )
    {
        weightYN=false;
        colWeight="";
        
        if (checkBoxWeightSim->isChecked()) weightYN=true;
        colWeight=comboBoxWeightSim->currentText();
    }
    colWeight=colWeight.remove(tableName+"_");
    
    //+++ ResoSigma
    resoYN=false;
    QString colReso="";
    if (SANSsupport)
    {
        
        colReso="";
        if (( (QTableWidgetItem *)tableCurves->item(5,2*m) )->checkState()) resoYN=true;

        colReso=((QComboBoxInTable*)tableCurves->cellWidget(5, 2*m+1))->currentText();
        
        if (textLabelCenter->text().contains("Generate Results") && tabWidgetGenResults->currentIndex()==0 )
        {
            resoYN=false;
            colReso="";
            
            if (checkBoxResoSim->isChecked())
            {
                resoYN=true;
            }
            colReso=comboBoxResoSim->currentText();
        }
    }
    
    
    
    double *II=new double[N];
    double *QQ=new double[N];
    double *dII=new double[N];
    double *sigmaResoO=new double[N];
    double *Weight=new double[N];
    double *sigmaf=new double[N];
    
    int i;
    QString line;
    
    double wa = spinBoxWA->value();
    double wb = spinBoxWB->value();
    if (wb == 0)
        wb = 1.0;
    double wc = spinBoxWC->value();
    double wxmax = spinBoxWXMAX->value();
    
    int weightMethod=comboBoxWeightingMethod->currentIndex();
    
    for(i=0;i<N;i++)
    {
        //Q
        line=t->text(i,xColIndex);
        if ( checkCell(line) ) QQ[i]=line.toDouble();
        else QQ[i]=-911119.119911;
        
        //I
        line=t->text(i,yColIndex);
        if ( checkCell(line) ) II[i]=line.toDouble();
        else II[i]=-911119.119911;
        
        //dI
        line=t->text(i,t->colIndex(colWeight));
        if ( checkCell(line) ) dII[i]=line.toDouble();
        else if (!weightYN) dII[i]=0.0;
        else if (weightMethod!=0 && weightMethod!=2) dII[i]=0.0;
        else dII[i]=-911119.119911;

        if (!weightYN) Weight[i]=1.00;
        else
        {
            if (weightMethod==0)
            {
                Weight[i]=fabs(dII[i]);
            }
            else if (weightMethod==1)
            {
                Weight[i]=sqrt(fabs(II[i]));
            }
            else if (weightMethod==2)
            {
                if ( dII[i]!=-911119.119911  && dII[i]!=0.0) Weight[i]=1/sqrt(fabs(dII[i]));
                else Weight[i]=1.0;
            }
            else if (weightMethod==3)
            {
                Weight[i]= fabs(II[i]);
            }
            else if (weightMethod==4)
            {
                Weight[i]=sqrt(pow( fabs(II[i]),wa));
            }
            else if (weightMethod==5)
            {
                Weight[i]=sqrt(pow(wc,wa)+wb*pow( fabs(II[i]),wa));
            }
            else if (weightMethod==6)
            {
                Weight[i]=sqrt(pow(wc,fabs(wxmax-QQ[i]))*pow( fabs(II[i]),wa));
            }
        }
        // Sigma
        double sigmaValue = -911119.119911;
        double sign = 1.0;

        if (SANSsupport && !resoYN)
            sign = -1.0;

        if (resoYN || SANSsupport)
        {
            if (colReso.contains("ASCII.1D.SANS"))
            {
                double val = app()->sigma(QQ[i]);
                sigmaValue = (sign < 0.0) ? -fabs(val) : val;
            }
            else if (colReso.contains("20%"))
                sigmaValue = sign * QQ[i] * 0.20;
            else if (colReso.contains("10%"))
                sigmaValue = sign * QQ[i] * 0.10;
            else if (colReso.contains("05%"))
                sigmaValue = sign * QQ[i] * 0.05;
            else if (colReso.contains("02%"))
                sigmaValue = sign * QQ[i] * 0.02;
            else if (colReso.contains("01%"))
                sigmaValue = sign * QQ[i] * 0.01;
            else if (colReso == "from_SPHERES")
            {
                double val = app()->sigma(QQ[i]);
                sigmaValue = (sign < 0.0) ? -fabs(val) : val;
            }
            else
            {
                line = t->text(i, t->colIndex(colReso));
                if (checkCell(line))
                {
                    double val = line.toDouble();
                    sigmaValue = (sign < 0.0) ? -fabs(val) : val;
                }
                else
                    sigmaValue = -911119.119911;
            }
        }
        else
            sigmaValue = 0.0;

        sigmaResoO[i] = sigmaValue;
        sigmaf[i] = sigmaValue;
    }

    int Nfinal=0;
    bool yn;
    
    for (i=0;i<N;i++) if ( QQ[i]!=-911119.119911 && II[i]!=-911119.119911 && dII[i]!=-911119.119911 && sigmaResoO[i]!=-911119.119911)
    {
        yn=false;
        if (NnotQ)
        {
            if (i>=(minN-1) && i<=(maxN-1) ) yn=true;
        }
        else
        {
            if ( QQ[i]>=min && QQ[i] <= max ) yn=true;
        }
        if (yn) Nfinal++;

    }

    if (Nfinal==0) return false;

    Qtotal=new double[Nfinal];
    Itotal=new double[Nfinal];
    dItotal=new double[Nfinal];
    Sigmatotal=new double[Nfinal];
    Weighttotal=new double[Nfinal];
    Sigmaftotal=new double[Nfinal];
    
    int prec=spinBoxSignDigits->value();
    
    Nfinal=0;
    for (i=0;i<N;i++) if ( QQ[i]!=-911119.119911 && II[i]!=-911119.119911 && dII[i]!=-911119.119911 && sigmaResoO[i]!=-911119.119911 && sigmaf[i]!=-911119.119911)
    {
        yn=false;
        if (NnotQ){
            if (i>=(minN-1) && i<=(maxN-1) ) yn=true;
        } else{
            if ( QQ[i]>=min && QQ[i] <= max ) yn=true;
        }
        if (yn) {
            Qtotal[Nfinal]		= QQ[i];//round2prec(QQ[i],prec);
            Itotal[Nfinal]		= II[i];//round2prec(II[i],prec);
            dItotal[Nfinal]		= dII[i];//round2prec(dII[i],prec);
            Sigmatotal[Nfinal]	= sigmaResoO[i]; //round2prec(sigmaResoO[i],prec);
            Weighttotal[Nfinal]    = Weight[i];//round2prec(Weight[i],prec);
            Sigmaftotal[Nfinal]    = sigmaf[i];//round2prec(sigmaf[i],prec);
            
           /* qDebug(QString::number(Qtotal[Nfinal],'e',23)+"\t"+QString::number(Itotal[Nfinal],'e',23)+"\t"+QString::number(dItotal[Nfinal],'e',23)+"\t"+QString::number(Sigmatotal[Nfinal],'e',23)+"\t"+QString::number(Weighttotal[Nfinal],'e',23)+"\t"+QString::number(Sigmaftotal[Nfinal],'e',23));
            */
            Nfinal++;
            
 /*
            Idata[iiii]=QString::number(Idata[iiii] ,'E',prec-1).toDouble();
                Isim[iiii]=QString::number(Isim[iiii] ,'E',prec-1).toDouble();
                weight[iiii]=QString::number(weight[iiii] ,'E',prec-1).toDouble();
 */

        }
    }
    Ntotal=Nfinal;
    
    
    delete[] QQ;
    delete[] II;
    delete[] dII;
    delete[] sigmaResoO;
    delete[] Weight;
    delete[] sigmaf;
    return true;
}
//***************************************************
//*** NEW :: simulate Function :: table  :: 22-09-2009
//***************************************************
bool fittable18::simulateData( int &N, double *Q,  double *&I,  double *&dI, double *sigmaReso, double *sigmaf, bool progressShow){
    int prec=spinBoxSignDigits->value();
    
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++ init parameters of function
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    int p=spinBoxPara->value(); if (p==0) return false;
    
    double left;
    double right;
    double value;
    QString txtVary;
    QStringList lstTmpLimits;
    
    for (int pp=0; pp<p;pp++){
        int mm=comboBoxDatasetSim->currentIndex();
        if (mm < 0)
            mm = 0;
        if(tableControl->item(pp,0)->text().contains("-inf")) left=-1.0e308;
        else left=tableControl->item(pp,0)->text().toDouble();
        if(tableControl->item(pp,4)->text().contains("inf")) right=1.0e308;
        else right=tableControl->item(pp,4)->text().toDouble();
        
        QTableWidgetItem *itA0 = (QTableWidgetItem *)tablePara->item(pp,3*mm+1); // Vary?
        
        if (!itA0) return false;
        if ( itA0->checkState()) 
            gsl_vector_int_set(F_para_fit_yn, pp, 1);
        else 
            gsl_vector_int_set(F_para_fit_yn, pp, 0);
        
        txtVary=itA0->text().remove(" ");
        
        if (txtVary.contains("..") && txtVary!=".."){
            lstTmpLimits = txtVary.split("..");
            if(lstTmpLimits.count()==2){
                double leftNew;
                if (lstTmpLimits[0]!="") leftNew=lstTmpLimits[0].toDouble();
                else leftNew=left;
                
                double rightNew;
                if (lstTmpLimits[1]!="") 
                    rightNew=lstTmpLimits[1].toDouble();
                else   
                    rightNew=right;
                
                if (rightNew>leftNew){
                    if (leftNew>left && leftNew<right) left=leftNew;
                    if (rightNew<right && rightNew>left) right=rightNew;
                }
            }
        }
        
        QTableWidgetItem *itValue = (QTableWidgetItem *)tableParaSimulate->item(pp,0);
        value=itValue->text().toDouble();
        if (value<left) itValue->setText(QString::number(left,'G',prec));
        else if (value>right) itValue->setText(QString::number(right,'G',prec));
        
        gsl_vector_set(F_para_limit_left,pp,left);
        gsl_vector_set(F_para_limit_right,pp,right);
    }
    
    for (int i=0;i<p;i++) gsl_vector_set(F_para, i,  tableParaSimulate->item(i,0)->text().toDouble() );
    
    //+++ init parameters of function
    bool polyYN=false;
    if (checkBoxPolySim->isChecked()) polyYN=true;
    int polyFunction=comboBoxPolyFunction->currentIndex();
    
    bool beforeFit=false;
    bool afterFit=false;
    bool beforeIter=false;
    bool afterIter=false;
    
    int currentFirstPoint=0;
    int currentLastPoint=N-1;
    int currentPoint=0;
    
    //+++ tableName,tableColNames,tableColDestinations,mTable
    char *tableName = nullptr;
    char **tableColNames=0;
    int *tableColDestinations=0;
    gsl_matrix * mTable=0;
    
    //+++ new superpositional function number
    int currentFunction=spinBoxCurrentFunction->value();
    
    double *Idata= new double[N]; for (int i=0;i<N;i++) Idata[i]=I[i];
    
    //+++
    double Int1=1.0;
    double Int2=1.0;
    double Int3=1.0;
    //+++
    int currentInt=0;
    //+++
    int currentM=0;
    int  *listLastPoints=new int[1]; listLastPoints[0]=N-1;
    
    functionT paraT={F_para, F_para_limit_left, F_para_limit_right, F_para_fit_yn, Q, Idata, dI, sigmaf,listLastPoints, currentM, currentFirstPoint, currentLastPoint, currentPoint, polyYN, polyFunction, beforeFit, afterFit, beforeIter, afterIter, Int1, Int2, Int3, currentInt, prec,tableName,tableColNames,tableColDestinations,mTable,currentFunction};
    
    F.params=&paraT;
    
    
    if ( !checkBoxSANSsupport->isChecked() ){
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        //+++ no SANS support bare function
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        
        N=simulateNoSANS(N,Q,I, F, progressShow);
    } else{
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        //+++ SANS support ::
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        bool resoSim=checkBoxResoSim->isChecked();
        bool polySim=checkBoxPolySim->isChecked();
        //+++
        QString currentInstrument=comboBoxInstrument->currentText();
        //+++
        bool bsMode=false;
        if ( currentInstrument.contains("Back") ) bsMode=true;
        //+++
        double sigmaPoly=-1;
        if (!bsMode) sigmaPoly=gsl_vector_get(F_para, p-1);
        
        
        if (!resoSim && ( !polySim || sigmaPoly<=0) ){
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            //+++ with SANS support :: no reso no poly
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            
            N=simulateNoSANS(N,Q,I, F, progressShow);
        } else{
            
            double absErrReso=lineEditAbsErr->text().toDouble();
            double relErrReso=lineEditRelErr->text().toDouble();
            int intWorkspaseReso=spinBoxIntWorkspase->value();
            int numberSigmaReso=spinBoxIntLimits->value();
            int func_reso=comboBoxResoFunction->currentIndex();
            if ( bsMode ) func_reso+=10;
            //+++
            integralControl     resoIntegralControl={absErrReso,relErrReso, intWorkspaseReso, numberSigmaReso, func_reso};
            //+++ paraReso
            resoSANS paraReso= { sigmaReso[0], Q[0], &F, &resoIntegralControl };
            //+++
            if (checkBoxSANSsupport->isChecked() && resoSim &&  ( !polySim || sigmaPoly<=0) ){
                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                //+++ with SANS support :: only reso no poly
                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                N=simulateSANSreso(N, Q, sigmaReso, I, paraReso, progressShow);
            } else{
                double absErrPoly=lineEditAbsErrPoly->text().toDouble();
                double relErrPoly=lineEditRelErrPoly->text().toDouble();
                int intWorkspasePoly=spinBoxIntWorkspasePoly->value();
                int numberSigmaPoly=spinBoxIntLimitsPoly->value();
                int func_poly=comboBoxPolyFunction->currentIndex();
                //+++
                integralControl polyIntegralControl={absErrPoly,relErrPoly, intWorkspasePoly, numberSigmaPoly, func_poly};
                //+++
                int polyItem=comboBoxPolySim->currentIndex();
                //+++
                poly2_SANS poly2={&F, polyItem, &polyIntegralControl};
                //+++
                if (checkBoxSANSsupport->isChecked() &&  !resoSim && polySim ){
                    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                    //+++ with SANS support :: no reso only poly
                    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                    N=simulateSANSpoly(N, Q, I, poly2, progressShow);
                } else{
                    polyReso1_SANS polyReso1={&poly2,sigmaReso[0], &resoIntegralControl};
                    
                    if (checkBoxSANSsupport->isChecked() &&  resoSim && polySim ){
                        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                        //+++ with SANS support :: both reso and poly
                        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                        N=simulateSANSpolyReso(N, Q, sigmaReso, I, polyReso1, progressShow);
                    }
                    else{
                        //+++ Delete  Variables
                        if (tableColNames!=0) delete[] tableColNames;
                        if (tableColDestinations!=0) delete[] tableColDestinations;
                        if (mTable!=0) gsl_matrix_free(mTable);
                        
                        delete[] Idata;
                        return false;
                    }
                }
            }
        }
        
    }
    
    //+++ Delete  Variables
    if (tableColNames!=0) delete[] tableColNames;
    if (tableColDestinations!=0) delete[] tableColDestinations;
    if (mTable!=0) gsl_matrix_free(mTable);
       
    delete[] Idata;
    return true;
}
//***************************************************
//*** simulate Function :: table 
//***************************************************
bool fittable18::simulateDataTable( int source, int number, QString &simulatedTable, int N, double *Q,  double *Idata, double *Weight, double *sigma,  double *Isim, Table *&t){
    
    int prec=spinBoxSignDigits->value();
    int M=spinBoxNumberCurvesToFit->value();
    int p=spinBoxPara->value(); if (p==0) return false;
    int f=spinBoxSubFitNumber->value();
 
    bool uniform=radioButtonUniform_Q->isChecked();
    
    QString simulatedLabel;
    QString function=textLabelFfunc->text();   
    QString superposSuffix="";
    if (spinBoxCurrentFunction->value()>0) superposSuffix="-part-"+QString::number(spinBoxCurrentFunction->value());
    
    bool tableExist=false;
    
    switch (source){
        case 0: simulatedTable="simulatedCurve-"+function+superposSuffix;
            simulatedLabel="Simulated Curve";
            if (!checkBoxSimIndexing->isChecked()) break;
            simulatedTable=app()->generateUniqueName(simulatedTable+"-");
            break;
        case 1: simulatedTable="fitCurve-"+function;
            if (M>1) simulatedTable+="-global-"+QString::number(number+1);
            simulatedTable+=superposSuffix;
            simulatedLabel="Fitting Curve";
            break;
        case 2:
            if (setToSetSimulYN){
                simulatedTable="simulatedCurve-"+function+"-set-"+QString::number(setToSetNumber);
                simulatedTable=app()->generateUniqueName(simulatedTable+"-");
                simulatedLabel="Simulated Curve by Set-to-Set interface";
            } else{
                simulatedTable="fitCurve-"+function+"-set-"+QString::number(setToSetNumber);
                simulatedLabel="Fitted Curve by Set-to-Set interface";
            }
            break;
    }
    
    //    Table *t;
    int cols, rows;
    if (app()->checkTableExistence(simulatedTable, t))
        tableExist = true;

    if (tableExist)
        t->blockSignals(true);

    int increaseNumRows = N;
    if (tableExist && t->numRows() <= N)
        increaseNumRows = N - t->numRows();

    QProgressDialog *progress;
    if (source==0 && increaseNumRows>=10000){
        progress= new QProgressDialog("Creation of a very long table (rows>10000): "+QString::number(increaseNumRows), "Stop", 0, 0);
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumDuration(0);
        progress->setCancelButton(0);
        progress->setLabelText("Creation of a very long table (rows>=10000)");
        progress->setValue(0);
        QThread::msleep(100);
        progress->setValue(progress->value()+1);
        QThread::msleep(100);
        progress->setValue(progress->value()+2);
        QThread::msleep(100);
        progress->setValue(progress->value()+3);
        progress->show();
    }

    if (tableExist)
    {
        t->setNumRows(N);
        if (t->numCols() < 10)
            t->setNumCols(10);
    }
    else
    {
        t=app()->newHiddenTable(simulatedTable,simulatedLabel,N, 10);
        t->setWindowLabel("Simulated Curve");
        app()->setListViewLabel(t->name(), "Simulated Curve");
        app()->updateWindowLists(t);
    }

    if (source == 0 && increaseNumRows>=10000) { progress->close(); QApplication::restoreOverrideCursor();};

    t->blockSignals(true);

    t->setColName(0,"x"); t->setColPlotDesignation(0,Table::X); t->setColNumericFormat(2, prec+1, 0);
    t->setColName(1,"y");t->setColPlotDesignation(1,Table::Y); t->setColNumericFormat(2, prec+1, 1);
    t->setColName(2,"weight"); t->setColPlotDesignation(2,Table::yErr); if (uniform) t->setTextFormat(2); else t->setColNumericFormat(2, prec+1, 2);
    t->setColName(3,"sigma"); t->setColPlotDesignation(3,Table::xErr);t->setColNumericFormat(2, prec+1,3);
    t->setColName(4,"residues"); t->setColPlotDesignation(4,Table::Y);if (uniform) t->setTextFormat(4); else t->setColNumericFormat(2, prec+1, 4);
    t->setColName(5,"Characteristics");  t->setColPlotDesignation(5,Table::None);t->setTextFormat(5);
    t->setColName(6,"Conditions"); t->setColPlotDesignation(6,Table::None);t->setTextFormat(6);
    t->setColName(7,"Parameters");  t->setColPlotDesignation(7,Table::None);t->setTextFormat(7);
    t->setColName(8,"Values");  t->setColPlotDesignation(8,Table::None);t->setColNumericFormat(0, prec+1, 8);
    t->setColName(9,"Errors");  t->setColPlotDesignation(9,Table::None);t->setTextFormat(9);
    
    int maxInfoCount=11;
    if (checkBoxSANSsupport->isChecked()) maxInfoCount=17;
    
    if (t->numRows()<maxInfoCount) t->setNumRows(maxInfoCount);
    if (t->numRows()<p) t->setNumRows(p);

    double yMin=lineEditImin->text().toDouble();
    bool xLogScale=checkBoxLogStep->isChecked();

    for (int i=0; i<N;i++){
        t->setText(i,0,QString::number(Q[i],'E',prec));
        if (uniform && xLogScale && Isim[i]<yMin) t->setText(i,1,"");
        else t->setText(i,1,QString::number(Isim[i],'E',prec));
        if (uniform) t->setText(i,2,"---");
        else t->setText(i,2,QString::number(Weight[i],'E',prec));
        t->setText(i,3,QString::number(sigma[i],'E',prec));
        if (uniform) t->setText(i,4,"---");
        else t->setText(i,4,QString::number(Idata[i] - Isim[i],'E',prec));
    }
       
    int currentLine=0;
    // First Col
    t->setText(currentLine,5,"Fitting Function");
    t->setText(currentLine,6,"->   "+function);
    currentLine++;
    //
    t->setText(currentLine,5,"Number of Parameters");
    t->setText(currentLine,6, "->   "+QString::number(p) );
    currentLine++;
    //
    t->setText(currentLine,5,"Time of Simulation");
    t->setText(currentLine,6,"->   "+textLabelTimeSim->text());
    currentLine++;
    //
    t->setText(currentLine,5,"x-Range Source");
    if (radioButtonSameQrange->isChecked() )
        t->setText(currentLine,6,"->   Same x as Fitting Data");
    else
        t->setText(currentLine,6,"->   Uniform x");
    currentLine++;
    //
    t->setText(currentLine,5,"x-min");
    t->setText(currentLine,6,"->   "+lineEditFromQsim->text());
    currentLine++;
    //
    t->setText(currentLine,5,"x-max");
    t->setText(currentLine,6,"->   "+lineEditToQsim->text());
    currentLine++;
    //
    t->setText(currentLine,5,"Number Points");
    t->setText(currentLine,6,"->   "+lineEditNumPointsSim->text());
    currentLine++;
    //
    t->setText(currentLine,5,"Logarithmic Step");
    if (checkBoxLogStep->isChecked() )
        t->setText(currentLine,6,"->   Yes");
    else
        t->setText(currentLine,6,"->   No");
    currentLine++;
    //
    t->setText(currentLine,5,"y-min");
    if (!checkBoxLogStep->isChecked() )
        t->setText(currentLine,6,"->   0");
    else
        t->setText(currentLine,6,"->   "+lineEditImin->text());
    currentLine++;
    //
    t->setText(currentLine,5,"Dataset");
    t->setText(currentLine,6,"->   "+comboBoxDatasetSim->itemText(number));
    currentLine++;
    //
    t->setText(currentLine,5,"SANS mode");
    if (checkBoxSANSsupport->isChecked() )
        t->setText(currentLine,6,"->   Yes");
    else
        t->setText(currentLine,6,"->   No");
    currentLine++;
    //
    
    if (checkBoxSANSsupport->isChecked())
    {
        t->setText(currentLine,5,"Resolution On");
        if (checkBoxResoSim->isChecked() )
            t->setText(currentLine,6,"->   Yes");
        else
            t->setText(currentLine,6,"->   No");
        currentLine++;
        //
        t->setText(currentLine,5,"Resolution Source");
        t->setText(currentLine,6,"->   "+comboBoxResoSim->currentText());
        currentLine++;
        //
        t->setText(currentLine,5,"Resolution Integral");
        QString line="->   a "+lineEditAbsErr->text();
        line+=" r "+lineEditRelErr->text();
        line+=" m "+spinBoxIntWorkspase->text();
        line+=" n "+spinBoxIntLimits->text();
        line+=" f "+comboBoxResoFunction->currentText();
        t->setText(currentLine,6,line);
        currentLine++;
        //
        t->setText(currentLine,5,"Polydispersity On");
        if (checkBoxPolySim->isChecked() )
            t->setText(currentLine,6,"->   Yes");
        else
            t->setText(currentLine,6,"->   No");
        currentLine++;
        //
        t->setText(currentLine,5,"Polydisperse Parameter");
        t->setText(currentLine,6,"->   "+comboBoxPolySim->currentText());
        currentLine++;
        //
        t->setText(currentLine,5,"Polydispersity Integral");
        line="->   a "+lineEditAbsErrPoly->text();
        line+=" r "+lineEditRelErrPoly->text();
        line+=" m "+spinBoxIntWorkspasePoly->text();
        line+=" n "+spinBoxIntLimitsPoly->text();
        line+=" f "+comboBoxPolyFunction->currentText();
        t->setText(currentLine,6,line);
        currentLine++;
    }

    for (int pp=0;pp<p;pp++){
        t->setText(pp,7,F_paraList[pp]);
        t->setText(pp,8,tableParaSimulate->item(pp,0)->text());
        if (source==1) 
            t->setText(pp,9,tablePara->item(pp,3*number+3)->text());
        else 
            if (source==2) 
                t->setText(pp,9,tablePara->item(pp,3)->text());
            else 
                t->setText(pp,9,"");
    }

    t->adjustColumnsWidth(false);

    app()->setListViewLabel(t->name(), simulatedLabel);

    t->blockSignals(false);

    t->setAutoUpdateValues(false);
    t->modifiedData(t, simulatedTable + "_y");
    t->modifiedData(t, simulatedTable + "_x");
    app()->modifiedProject(t);

    if (tableExist && increaseNumRows > 0)
        app()->showFullRangeAllPlots(simulatedTable);

    t->setAutoUpdateValues(true);
    return true;
}
//***************************************************
//*** checkCell
//***************************************************
bool fittable18::checkCell(QString &line){
    static const QRegularExpression rx(R"(((\-|\+)?\d\d*(\.\d*)?((E\-|E\+)\d\d?\d?\d?)?))");
    
    line=line.trimmed();
    line.replace(",",".");
    line.replace("e","E");
    line.replace("E","E0");
    line.replace("E0+","E+0");
    line.replace("E0-","E-0");
    line.replace("E0","E+0");
    line = rx.match(line).captured(1);
    return rx.match(line).hasMatch();
}
//***************************************************
//*** Simulate No Reso
//***************************************************
int fittable18::simulateNoSANS(int N, double *Q,double *&I, gsl_function FF, bool progressShow){
    gsl_set_error_handler_off();
    
    functionT *functionTpara=(functionT *)FF.params;
    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++  ---process+dialog+control--- Show only xxx times
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    int setProcessNumber=100; //
    int procNumber=1, restNumber=0;
    if (N>setProcessNumber) { procNumber=N/setProcessNumber; restNumber=N-procNumber*setProcessNumber;}
    else setProcessNumber=N;
    
    int i=0,ii,iii;
    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++ Progress dialog
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    QProgressDialog *progress;
    
    if (progressShow){
        progress= new QProgressDialog("Function | Simulator | Started",  "Stop", 0, N);
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumDuration(4000);
    }
    
    //_______________________________________________
    ((functionT *)FF.params)->beforeIter=true;
    ((functionT *)FF.params)->currentPoint=0;
    ((functionT *)FF.params)->currentInt=-1;
    GSL_FN_EVAL(&FF,Q[0]);
    
    //+++
    // integral1
    ((functionT *)FF.params)->currentInt=1;
    double Int1=GSL_FN_EVAL(&FF,Q[0]);
    ((functionT *)FF.params)->Int1=Int1;
    // integral2
    ((functionT *)FF.params)->currentInt=2;
    double Int2=GSL_FN_EVAL(&FF,Q[0]);
    ((functionT *)FF.params)->Int2=Int2;
    // integral3
    ((functionT *)FF.params)->currentInt=3;
    double Int3=GSL_FN_EVAL(&FF,Q[0]);
    ((functionT *)FF.params)->Int3=Int3;
    
    //+++
    ((functionT *)FF.params)->currentInt=0;
    GSL_FN_EVAL(&FF,Q[0]);
    ((functionT *)FF.params)->beforeIter=false;
    //_______________________________________________
    
    for (i=0;  i<restNumber+procNumber;i++){
        ((functionT *)(FF.params))->currentPoint=i;
        I[i] = GSL_FN_EVAL (&FF,Q[i]);
        
        if (progressShow && i<10){
            //+++ Start +++  1
            progress->setValue(i);
            progress->setLabelText("Function | Simulator | Started: # "+QString::number(i+1)+" of "+QString::number(N));
            
            if ( progress->wasCanceled() ){
                progress->close();
                return i;
            }
        }
    }
    
    for (ii=1; ii<setProcessNumber;ii++){
        for (iii=0; iii<procNumber;iii++){
            ((functionT *)FF.params)->currentPoint=i;
            I[i] = GSL_FN_EVAL (&FF,Q[i]);
            i++;
        }
        if (progressShow){
            //+++ Start +++  1
            progress->setValue(i-1);
            progress->setLabelText("Function | Simulator | Started: # "+QString::number(i)+" of "+QString::number(N));
            
            if ( progress->wasCanceled() ){
                progress->close();
                return i;
            }
        }
    }
    
    ((functionT *)FF.params)->afterIter=true;
    ((functionT *)FF.params)->currentPoint=i-1;
    GSL_FN_EVAL(&FF, Q[i-1]);
    ((functionT *)FF.params)->afterIter=false;
    
    if (progressShow)
        progress->close();

    return N;
}
//***************************************************
//***  Simulate only Reso
//***************************************************
int fittable18::simulateSANSreso(int N, double *Q,double *sigma, double *&I, resoSANS &paraReso, bool progressShow){
    gsl_set_error_handler_off();
            
    gsl_function *FF =paraReso.function;
    
    functionT *functionTpara=(functionT *)paraReso.function->params;

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++  ---process+dialog+control--- Show only xxx times
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    int setProcessNumber=100; //
    int procNumber=1, restNumber=0;
    if (N>setProcessNumber) { procNumber=N/setProcessNumber; restNumber=N-procNumber*setProcessNumber;}
    else setProcessNumber=N;
    
    int i=0,ii,iii;
    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++ Progress dialog
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    QProgressDialog *progress;
    
    if (progressShow){
        progress= new QProgressDialog("Function | Simulator | Started",  "Stop", 0, N);
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumDuration(4000);
    }
    
    //___________________________________________________________
    ((struct functionT *) functionTpara)->beforeIter=true;
    ((struct functionT *) functionTpara)->currentPoint=0;
    //
    paraReso.resoSigma=sigma[0];;
    paraReso.Q0=Q[0];
    
    ((functionT *)functionTpara)->currentInt=-1;
    //resoIntegral(Q[0],&paraReso) ;
    GSL_FN_EVAL(FF,Q[0]);//+++2020.04
    
    // integral1
    ((functionT *)functionTpara)->currentInt=1;
    double Int1=resoIntegral(Q[0],&paraReso) ;
    ((functionT *)functionTpara)->Int1=Int1;
    // integral2
    ((functionT *)functionTpara)->currentInt=2;
    double Int2=resoIntegral(Q[0],&paraReso) ;
    ((functionT *)functionTpara)->Int2=Int2;
    // integral3
    ((functionT *)functionTpara)->currentInt=3;
    double Int3=resoIntegral(Q[0],&paraReso) ;
    ((functionT *)functionTpara)->Int3=Int3;
    
    //+++
    ((functionT *)functionTpara)->currentInt=0;
    //resoIntegral(Q[0],&paraReso) ;
    GSL_FN_EVAL(FF,Q[0]);//+++2020.04
    
    ((struct functionT *) functionTpara)->beforeIter=false;
    
    //___________________________________________________________
    
    
    for (i=0; i<restNumber+procNumber;i++){
        ((struct functionT *) functionTpara)->currentPoint=i;
        //+++ paraReso
        paraReso.resoSigma=sigma[i];
        paraReso.Q0=Q[i];
        
        I[i]=  resoIntegral(Q[i],&paraReso) ;
        
        if (progressShow && i<10){
            //+++ Start +++  1
            progress->setValue(i);
            progress->setLabelText("Function | Simulator | Started: # "+QString::number(i+1)+" of "+QString::number(N));
            
            if ( progress->wasCanceled() ){
                progress->close();
                return i;
            }
        }
    }
    
    for (ii=1; ii<setProcessNumber;ii++){
        for (iii=0; iii<procNumber;iii++){
            ((struct functionT *) functionTpara)->currentPoint=i;
            //+++ paraReso
            paraReso.resoSigma=sigma[i];
            paraReso.Q0=Q[i];
            //+++
            I[i]=resoIntegral(Q[i],&paraReso);
            i++;
        }
        if (progressShow){
            //+++ Start +++  1
            progress->setValue(i-1);
            progress->setLabelText("Function | Simulator | Started: # "+QString::number(i)+" of "+QString::number(N));
            
            if ( progress->wasCanceled() ){
                progress->close();
                return i;
            }
        }
    }
    
    //+++ new::  initAfterIteration 2011-08-19
    ((struct functionT *) functionTpara)->afterIter=true;
    ((struct functionT *) functionTpara)->currentPoint=i-1;
    paraReso.resoSigma=sigma[i-1];
    paraReso.Q0=Q[i-1];
    //resoIntegral(Q[i-1],&paraReso);
    GSL_FN_EVAL(FF,Q[i-1]);//+++2020.04
    ((struct functionT *) functionTpara)->afterIter=false;
    
    if ( progressShow ) 
        progress->close();
    
    return N;
}
//***************************************************
//*** Simulate only Poly
//***************************************************
int fittable18::simulateSANSpoly(int N, double *Q, double *&I, poly2_SANS poly2, bool progressShow){
    gsl_set_error_handler_off();
    
    functionT *functionTpara=(functionT *)poly2.function->params;
    gsl_function *FF =poly2.function;
    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++  ---process+dialog+control--- Show only xxx times
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    int setProcessNumber=100; //
    int procNumber=1, restNumber=0;
    
    if (N>setProcessNumber) { procNumber=N/setProcessNumber; restNumber=N-procNumber*setProcessNumber;}
    else setProcessNumber=N;
    
    int i=0,ii,iii;
    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++ Progress dialog
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    QProgressDialog *progress;
    
    if (progressShow){
        progress= new QProgressDialog("Function | Simulator | Started",  "Stop", 0, N);
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumDuration(4000);
    }
        
    //___________________________________________________________
    ((struct functionT *) functionTpara)->beforeIter=true;
    ((struct functionT *) functionTpara)->currentPoint=0;
    ((functionT *)functionTpara)->currentInt=-1;
    //polyIntegral(Q[0],&poly2);
    GSL_FN_EVAL(FF,Q[0]);//+++2020.04
    
    // integral1
    ((functionT *)functionTpara)->currentInt=1;
    double Int1=polyIntegral(Q[0],&poly2);
    ((functionT *)functionTpara)->Int1=Int1;
    // integral2
    ((functionT *)functionTpara)->currentInt=2;
    double Int2=polyIntegral(Q[0],&poly2);
    ((functionT *)functionTpara)->Int2=Int2;
    // integral3
    ((functionT *)functionTpara)->currentInt=3;
    double Int3=polyIntegral(Q[0],&poly2);
    ((functionT *)functionTpara)->Int3=Int3;
    //+++
    ((functionT *)functionTpara)->currentInt=0;
    //polyIntegral(Q[0],&poly2);
    GSL_FN_EVAL(FF,Q[0]);//+++2020.04
    ((struct functionT *) functionTpara)->beforeIter=false;
    //___________________________________________________________
    
    
    for (i=0; i<restNumber+procNumber;i++){
        ((struct functionT *) functionTpara)->currentPoint=i;
        //+++ poly
        I[i] = polyIntegral(Q[i],&poly2);
        if (progressShow && i<10){
            //+++ Start +++  1
            progress->setValue(i);
            progress->setLabelText("Function | Simulator | Started: # "+QString::number(i+1)+" of "+QString::number(N));
            
            if ( progress->wasCanceled() ){
                progress->close();
                return i;
            }
        }
    }
    
    for (ii=1; ii<setProcessNumber;ii++){
        for (iii=0; iii<procNumber;iii++){
            ((struct functionT *) functionTpara)->currentPoint=i;
            //+++ poly
            I[i] = polyIntegral(Q[i],&poly2);
            i++;
        }
        if (progressShow){

            //+++ Start +++  1
            progress->setValue(i-1);
            progress->setLabelText("Function | Simulator | Started: # "+QString::number(i)+" of "+QString::number(N));
            
            if ( progress->wasCanceled() ){
                progress->close();
                return i;
            }
        }
    }
    
    //+++ new::  initAfterIteration 2011-08-19
    ((struct functionT *) functionTpara)->afterIter=true;
    ((struct functionT *) functionTpara)->currentPoint=i-1;
    //polyIntegral(Q[i-1],&poly2);
    GSL_FN_EVAL(FF,Q[i-1]);//+++2020.04
    ((struct functionT *) functionTpara)->afterIter=false;
    
    if ( progressShow ) 
        progress->close();
    return N;
}
//***************************************************
//*** Simulate Reso && Poly
//***************************************************
int fittable18::simulateSANSpolyReso(int N, double *Q, double *sigma, double *&I, polyReso1_SANS &polyReso1, bool progressShow){
    gsl_set_error_handler_off();
    
    functionT *functionTpara=(functionT *)polyReso1.poly2->function->params;
    gsl_function *FF =polyReso1.poly2->function;
    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++  ---process+dialog+control--- Show only xxx times
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    int setProcessNumber=100; //
    int procNumber=1, restNumber=0;
    
    
    if (N>setProcessNumber) { procNumber=N/setProcessNumber; restNumber=N-procNumber*setProcessNumber;}
    else setProcessNumber=N;
    
    int i=0,ii,iii;
    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++ Progress dialog
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++
    QProgressDialog *progress;
    
    if (progressShow){
        progress= new QProgressDialog("Function | Simulator | Started",  "Stop", 0, N);
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumDuration(4000);
    }
    
    //___________________________________________________________
    ((struct functionT *) functionTpara)->beforeIter=true;
    ((struct functionT *) functionTpara)->currentPoint=0;
    ((functionT *)functionTpara)->currentInt=-1;
    //
    polyReso1.resoSigma=sigma[0];
    //resoPolyFunctionNew(Q[0], &polyReso1);
    GSL_FN_EVAL(FF,Q[0]);//+++2020.04
    
    // integral1
    ((functionT *)functionTpara)->currentInt=1;
    double Int1=resoPolyFunctionNew(Q[0], &polyReso1);
    ((functionT *)functionTpara)->Int1=Int1;
    // integral2
    ((functionT *)functionTpara)->currentInt=2;
    double Int2=resoPolyFunctionNew(Q[0], &polyReso1);
    ((functionT *)functionTpara)->Int2=Int2;
    // integral3
    ((functionT *)functionTpara)->currentInt=3;
    double Int3=resoPolyFunctionNew(Q[0], &polyReso1);
    ((functionT *)functionTpara)->Int3=Int3;
    //+++
    ((functionT *)functionTpara)->currentInt=0;
    //resoPolyFunctionNew(Q[0], &polyReso1);
    GSL_FN_EVAL(FF,Q[0]);//+++2020.04
    ((struct functionT *) functionTpara)->beforeIter=false;
    //___________________________________________________________
    
    for (i=0; i<restNumber+procNumber;i++){
        ((struct functionT *) functionTpara)->currentPoint=i;
        //+++ polyReso
        polyReso1.resoSigma=sigma[i];
        I[i]=resoPolyFunctionNew(Q[i], &polyReso1);
        
        if (progressShow && i<10){

            //+++ Start +++  1
            progress->setValue(i);
            progress->setLabelText("Function | Simulator | Started: # "+QString::number(i+1)+" of "+QString::number(N));
            
            if ( progress->wasCanceled() ){
                progress->close();
                return i;
            }
        }
    }
    
    for (ii=1; ii<setProcessNumber;ii++){
        for (iii=0; iii<procNumber;iii++){
            ((struct functionT *) functionTpara)->currentPoint=i;
            //+++ polyReso
            polyReso1.resoSigma=sigma[i];
            I[i]=resoPolyFunctionNew(Q[i], &polyReso1);
            i++;
        }
        if (progressShow){
            //+++ Start +++  1
            progress->setValue(i-1);
            progress->setLabelText("Function | Simulator | Started: # "+QString::number(i)+" of "+QString::number(N));
            
            if ( progress->wasCanceled() ){
                progress->close();
                return i;
            }
        }
    }
    
    //+++ new::  initAfterIteration 2011-08-19
    ((struct functionT *) functionTpara)->afterIter=true;
    ((struct functionT *) functionTpara)->currentPoint=0;

    //polyReso1.resoSigma=sigma[i-1];
    //resoPolyFunctionNew(Q[i-1], &polyReso1);
    GSL_FN_EVAL(FF,Q[i-1]);//+++2020.04
    ((struct functionT *) functionTpara)->afterIter=false;
    
    if ( progressShow )
        progress->close();

    return N;
}
//***************************************************
//*** simulateSuperpositional
//***************************************************
void fittable18::simulateSuperpositional(){
    bool fitYN=false;

    if (widgetStackFit->currentIndex()==1) fitYN=true;
    
    if (!checkBoxSuperpositionalFit->isChecked()) return;

    int M=spinBoxNumberCurvesToFit->value();
    if (!fitYN) M=1;
    
    int SFnumber=spinBoxSubFitNumber->value();
    
    int initColor=comboBoxColor->currentIndex();
    
    int indexingColor=0;
    
    for (int f = 0; f < SFnumber; f++)
    {
        if (checkBoxColorIndexing->isChecked())
        {
            indexingColor = (initColor + f * M) % comboBoxColor->count();
            comboBoxColor->setCurrentIndex(initColor + indexingColor);
        }

        bool changeBack=false;
        if (!checkBoxColorIndexing->isChecked() && f==0 && M>1){
            checkBoxColorIndexing->setChecked(true);
            changeBack=true;
        }
        spinBoxCurrentFunction->setValue(f);
        
        if (!fitYN) simulateSwitcher();
        else plotSwitcher();
        
        if(changeBack){
            comboBoxColor->setCurrentIndex(initColor);
            checkBoxColorIndexing->setChecked(false);
        }
    }
    if (checkBoxColorIndexing->isChecked())
        comboBoxColor->setCurrentIndex(initColor);

    spinBoxCurrentFunction->setValue(0);
}
//***************************************************
//*** setBySetFit
//***************************************************
void fittable18::setBySetFit(){
    setToSetSimulYN=false;
    setBySetFitOrSim(true);
}
//***************************************************
//*** setBySetFitOrSim
//***************************************************
void fittable18::setBySetFitOrSim(bool fitYN)
{
    Graph *g;
    bool currentGraphAutoscaled = false;
    if (app()->findActiveGraph(g) && g->isAutoscalingEnabled())
    {
        currentGraphAutoscaled = true;
        g->enableAutoscaling(false);
    }
    // timer
    QElapsedTimer time;
    time.start();
    qint64 pre_t = 0;

    int Nselected = 0;
    int p = spinBoxPara->value();

    QStringList tables, colList, weightColList,resoColList, commentList;
    QString s, sTmp;
    QString SANSsupport = "No";
    QString polyUse = "No";
    
    // +++ check #1
    int Ntot = tableMultiFit->rowCount() - 1; // number of Availeble Datasets in table
    
    if (Ntot == 0)
    {
        QMessageBox::warning(this, "QtiSAS", "There is no table | Select datasets! | Use [Select] button ");
        return;
    }

    setToSetProgressControl = true;

    // +++ weight
    bool weight = (QTableWidgetItem *)tableMultiFit->item(0, 2)->checkState();
    
    // +++ reso & poly
    bool reso = false, poly = false;
    int start = 3;
    if (checkBoxSANSsupport->isChecked())
    {
        reso = (QTableWidgetItem *)tableMultiFit->item(0, 3)->checkState();
        poly = (QTableWidgetItem *)tableCurves->item(6, 0)->checkState();
        if (poly)
            polyUse = "Yes";
        start++;
        SANSsupport = "Yes";
    }

    // +++ check #2
    for (int i = 0; i < Ntot; i++)
    {
        if (!(QTableWidgetItem *)tableMultiFit->item(i + 1, 0)->checkState())
            continue;

        tables << tableMultiFit->verticalHeaderItem(i + 1)->text();
        s = tableMultiFit->verticalHeaderItem(i + 1)->text() + " |t| ";

        sTmp = ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 1))->currentText();
        colList << sTmp;
        s += sTmp + " |y| ";

        if (weight)
        {
            sTmp = ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 2))->currentText();
            weightColList << sTmp;
            s += sTmp;
        }
        s += " |w| ";

        if (reso)
        {
            sTmp = ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 3))->currentText();
            resoColList << sTmp;
            s += sTmp;
        }
        s += " |r| ";

        commentList << s;
        Nselected++;
    }

    if (Nselected == 0)
    {
        QMessageBox::warning(this, "QtiSAS", "There are no SELECTED tables | Select datasets! ");
        return;
    }

    int prec = spinBoxSignDigits->value();
    
    // +++  create Table
    Table *t;

    s = app()->generateUniqueName(QString("Set-By-Set-Fit-" + lineEditSetBySetFit->text()).toLocal8Bit().constData());
    t = app()->newHiddenTable(s, "Fitting Results:: Set-By-Set", GSL_MAX(Nselected, 20), 2 + 1 + 1 + 3 + 2 * p);

    t->blockSignals(true);

    t->setWindowLabel("Fitting Results:: Set-By-Set");
    
    app()->setListViewLabel(t->name(), "Fitting Results:: Set-By-Set");
    app()->updateWindowLists(t);
    
    t->setColName(0, "Parameter");
    t->setColPlotDesignation(0, Table::None);
    t->setColumnType(0, Table::Text);

    t->setColName(1, "Value");
    t->setColPlotDesignation(1, Table::None);
    t->setColumnType(1, Table::Text);

    t->setColName(2, "X");
    t->setColPlotDesignation(2, Table::X);

    t->setColName(3, "Dataset");
    t->setColumnType(3, Table::Text);

    t->setColName(4, "Chi2");

    t->setColName(5, "R2");

    t->setColName(6, "Fit-Time");
    t->setColumnType(6, Table::Text);

    s = "-> ";
    for (int i = 0; i < p; i++)
    {
        s += (i < p - 1) ? F_paraList[i] + " , " : F_paraList[i];

        t->setColName(7 + 2 * i, F_paraList[i]);
        t->setColNumericFormat(2, prec + 1, 7 + 2 * i, true);

        t->setColName(7 + 2 * i + 1, "d" + F_paraList[i]);
        t->setColPlotDesignation(7 + 2 * i + 1, Table::yErr);
        t->setColNumericFormat(2, prec + 2, 7 + 2 * i + 1, true);
        t->setColumnType(7 + 2 * i + 1, Table::Text);
    }

    // Fit Conrtrol
    int currentChar = 0;
    
    // +++  Fitting Function
    t->setText(currentChar, 0, "Fitting Function");
    t->setText(currentChar, 1, "-> " + textLabelFfunc->text());
    currentChar++;
    // +++  Number of Parameters
    t->setText(currentChar, 0, "Number of Parameters");
    t->setText(currentChar, 1, "-> " + QString::number(p));
    currentChar++;
    // +++  Parameters
    t->setText(currentChar, 0, "Parameters");
    t->setText(currentChar, 1, s);
    currentChar++;
    //+++ Multi Mode
    t->setText(currentChar, 0, "Number of Datasets");
    t->setText(currentChar, 1, "-> 1");
    currentChar++;
    //+++ SANS Mode
    t->setText(currentChar, 0, "SANS Mode");
    t->setText(currentChar, 1, "-> " + SANSsupport);
    currentChar++;
    //+++ Resolusion On
    SANSsupport = (reso && checkBoxSANSsupport->isChecked()) ? "Yes" : "No";
    t->setText(currentChar, 0, "Resolution On");
    t->setText(currentChar, 1, "-> " + SANSsupport);
    currentChar++;
    //+++ Weight On
    SANSsupport = weight ? "Yes" : "No";
    t->setText(currentChar, 0, "Weight On");
    t->setText(currentChar, 1, "-> " + SANSsupport);
    currentChar++;
    //+++ Polydispersity On
    t->setText(currentChar, 0, "Polydispersity On");
    t->setText(currentChar, 1, "-> " + polyUse);
    currentChar++;
    //+++ Polydispersity Parameter
    t->setText(currentChar, 0, "Polydisperse Parameter");
    auto *pp = qobject_cast<QComboBoxInTable *>(tableCurves->cellWidget(6, 1));
    t->setText(currentChar, 1, checkBoxSANSsupport->isChecked() && pp ? "-> " + pp->currentText() : "-> No");
    currentChar++;
    //+++ Fitting Range: From x[min]
    t->setText(currentChar, 0, "Fitting Range: From x[min]");
    t->setText(currentChar, 1, "-> " + lineEditFromQ->text());
    currentChar++;
    //+++ Fitting Range: To x[max]
    t->setText(currentChar, 0, "Fitting Range: To x[max]");
    t->setText(currentChar, 1, "-> " + lineEditToQ->text());
    currentChar++;
    //+++ Simulation Range: x-Range Source
    t->setText(currentChar, 0, "Simulation Range: x-Range Source");
    if (radioButtonSameQrange->isChecked())
        t->setText(currentChar, 1, "-> Same Q as Fitting Data");
    else
        t->setText(currentChar,1,"-> Uniform Q");
    currentChar++;
    //+++ Simulation Range: x-min
    t->setText(currentChar, 0, "Simulation Range: x-min");
    t->setText(currentChar, 1, "-> " + lineEditFromQsim->text());
    currentChar++;
    //+++ Simulation Range: x-max
    t->setText(currentChar, 0, "Simulation Range: x-max");
    t->setText(currentChar, 1, "-> " + lineEditToQsim->text());
    currentChar++;
    //+++ Simulation Range: Number Points
    t->setText(currentChar, 0, "Simulation Range: Number Points");
    t->setText(currentChar, 1, "-> " + lineEditNumPointsSim->text());
    currentChar++;
    //+++ Simulation Range: Logarithmic Step
    t->setText(currentChar, 0, "Simulation Range: Logarithmic Step");
    if (checkBoxLogStep->isChecked())
        t->setText(currentChar, 1, "-> Yes");
    else
        t->setText(currentChar, 1, "-> No");
    currentChar++;
    //+++ Simulation Range: Logarithmic Step
    t->setText(currentChar, 0, "Simulation Range: y-min");
    if (checkBoxLogStep->isChecked())
        t->setText(currentChar, 1, "-> " + lineEditImin->text());
    else
        t->setText(currentChar, 1, "-> 0");
    currentChar++;
    //+++ Fit-Control
    QString line;
    line = QString::number(comboBoxFitMethod->currentIndex()) + " , ";
    line += spinBoxMaxIter->text() + " , ";
    line += lineEditTolerance->text() + " , ";
    line += QString::number(comboBoxColor->currentIndex()) + " , ";
    line += spinBoxSignDigits->text() + " , ";
    line += QString::number(comboBoxWeightingMethod->currentIndex()) + " , ";
    line += checkBoxCovar->isChecked() ? "1 , " : "0 , ";
    t->setText(currentChar, 0, "Fit-Control");
    t->setText(currentChar, 1, "-> " + line);
    currentChar++;
    //+++ Resolution Integral
    t->setText(currentChar, 0, "Resolution Integral");
    line = "-> " + lineEditAbsErr->text();
    line += " , " + lineEditRelErr->text();
    line += " , " + spinBoxIntWorkspase->text();
    line += " , " + spinBoxIntLimits->text();
    line += " , " + comboBoxResoFunction->currentText();
    t->setText(currentChar, 1, line);
    currentChar++;
    //+++ Polydispersity Integral
    t->setText(currentChar, 0, "Polydispersity Integral");
    line = "-> " + lineEditAbsErrPoly->text();
    line += " , " + lineEditRelErrPoly->text();
    line += " , " + spinBoxIntWorkspasePoly->text();
    line += " , " + spinBoxIntLimitsPoly->text();
    line += " , " + comboBoxPolyFunction->currentText();
    t->setText(currentChar, 1, line);
    currentChar++;
    // +++ Q/N
    auto *NQ = (QComboBoxInTable *)tableCurves->cellWidget(1, 0);
    if (NQ)
        NQ->setCurrentIndex(1);
    // +++ From
    auto *fromCheckItem = (QTableWidgetItem *)tableCurves->item(2, 0);
    if (fromCheckItem)
        fromCheckItem->setCheckState(Qt::Checked);
    tableCurves->item(2, 1)->setText(lineEditFromQ->text());
    // +++ To
    auto *toCheckItem = (QTableWidgetItem *)tableCurves->item(3, 0);
    if (toCheckItem)
        toCheckItem->setCheckState(Qt::Checked);
    tableCurves->item(3, 1)->setText(lineEditToQ->text());
    // +++ DataSet
    auto *dataSetItem = (QComboBoxInTable *)tableCurves->cellWidget(0, 1);
    // +++ weight check & Col
    auto *WrealYN = (QTableWidgetItem *)tableCurves->item(4, 0);
    auto *weightColItem = (QComboBoxInTable *)tableCurves->cellWidget(4, 1);
    // +++ reso check & Col
    QTableWidgetItem *RrealYN;
    QComboBoxInTable *resoColItem;
    if (checkBoxSANSsupport->isChecked())
    {
        RrealYN = (QTableWidgetItem *)tableCurves->item(5, 0);
        resoColItem = (QComboBoxInTable *)tableCurves->cellWidget(5, 1);
    }
    // +++Start values & adjustibility trasfer
    tablePara->blockSignals(true);
    for (int j = start; j < tableMultiFit->columnCount(); j++)
    {
        auto *fitYN = (QTableWidgetItem *)tableMultiFit->item(0, j);
        auto *fitYN0 = (QTableWidgetItem *)tablePara->item(j - start, 1);

        fitYN0->setCheckState(fitYN->checkState());
    }
    tablePara->blockSignals(false);
    
    int NselTot = Nselected;
    Nselected = 0;

    int firstColor = comboBoxColor->currentIndex();
    int indexingColor = 0;

    //+++ Progress dialog
    int progressIter = 0;
    auto *progress = new QProgressDialog("Set-to-Set Fit", "Abort Set-To-Set FIT", 0, NselTot);
    progress->setWindowModality(Qt::ApplicationModal);
    progress->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    progress->setMinimumDuration(0);

    for (int i = 0; i < Ntot; i++)
    {
        if (!(QTableWidgetItem *)tableMultiFit->item(i + 1, 0)->checkState())
            continue;

        QString infoStr =
            "Current data-set: # " + QString::number(progressIter + 1) + " of " + QString::number(NselTot);
        progress->setValue(progressIter);
        progress->setLabelText(infoStr);
        progressIter++;

        // +++ RESULT TABLE
        t->setText(Nselected, 2, QString::number(Nselected + 1));
        t->setText(Nselected, 3, commentList[Nselected]);

        // +++ TRANSFER INFO ABOUT FITTED DATASET
        s = tables[Nselected] + "_" + colList[Nselected];
        dataSetItem->setItemText(dataSetItem->currentIndex(), s);
        tableCurvechanged(0, 1);

        //+++ TRANSFER OF WEIGHT INFO
        std::cout << "Set-to-Set Fit" << "\n";
        std::cout << infoStr.toLocal8Bit().constData() << "\n";
        std::cout << "Dataset:" << s.toLocal8Bit().constData() << "\n";
        if (weight)
        {
            if (weightColList[Nselected].isEmpty() &&
                (comboBoxWeightingMethod->currentIndex() == 0 || comboBoxWeightingMethod->currentIndex() == 2))
                WrealYN->setCheckState(Qt::Unchecked);
            else
            {
                WrealYN->setCheckState(Qt::Checked);
                s = tables[Nselected] + "_" + weightColList[Nselected];
                weightColItem->setItemText(weightColItem->currentIndex(), s);
                tableCurvechanged(4, 1);
            }
        }
        else
            WrealYN->setCheckState(Qt::Unchecked);

        //+++ TRANSFER OF RESOLUTION INFO
        if (reso && !resoColList[Nselected].isEmpty())
        {
            RrealYN->setCheckState(Qt::Checked);

            if (resoColList[Nselected].contains("from DANP") || resoColList[Nselected].contains("ASCII.1D.SANS"))
                s = "calculated_in_\"ASCII.1D.SANS\"";
            else if (resoColList[Nselected].contains("20%"))
                s = "\"20%\":_sigma(Q)=0.20*Q";
            else if (resoColList[Nselected].contains("10%"))
                s = "\"10%\":_sigma(Q)=0.10*Q";
            else if (resoColList[Nselected].contains("05%"))
                s = "\"05%\":_sigma(Q)=0.05*Q";
            else if (resoColList[Nselected].contains("02%"))
                s = "\"02%\":_sigma(Q)=0.02*Q";
            else if (resoColList[Nselected].contains("01%"))
                s = "\"01%\":_sigma(Q)=0.01*Q";
            else if (resoColList[Nselected] == "from_SPHERES")
                s = "from_SPHERES";
            else
                s = tables[Nselected] + "_" + resoColList[Nselected];

            resoColItem->setItemText(resoColItem->currentIndex(), s);
            tableCurvechanged(5, 1);
        }

        //+++ MOVING OF PARAMETERS TO FITTING INTERFACE
        if (tableMultiFit->item(0, 0)->text() != "c" || progressIter == 1)
            for (int j = start; j < tableMultiFit->columnCount(); j++)
                tablePara->item(j - start, 2)->setText(tableMultiFit->item(i + 1, j)->text());

        //+++ FITTING OF CURRENT DATASET
        setToSetNumber = i + 1;

        fitOrCalculate(!fitYN, -2); // -2 means Set-to-Set Fit

        // +++ COLOR CONTROL
            if (checkBoxColorIndexing->isChecked())
            {
                const qsizetype count = app()->indexedColors().count();
                indexingColor = static_cast<int>((firstColor + setToSetNumber) % count);
                comboBoxColor->setCurrentIndex(indexingColor);
            }

        //+++ TRANSFER OF OBTEINED PARAMETERS TO SET-BY-SET TABLE AND TO RESULT TABLE
        for (int j = 0; j < p; j++)
        {
            t->setText(Nselected, 2 * j + 7, tablePara->item(j, 2)->text());
            t->setText(Nselected, 2 * j + 8, tablePara->item(j, 3)->text().remove(QString(QChar(0x00B1))));

            tableMultiFit->item(i + 1, j + start)->setText(tablePara->item(j, 2)->text());
        }
        t->setText(Nselected, 4, textLabelChi->text());
        t->setText(Nselected, 5, textLabelR2->text());
        t->setText(Nselected, 6, textLabelTime->text());
        Nselected++;

        if (progress->wasCanceled())
            break;
    }
    progress->close();

    t->adjustColumnsWidth(false);
    t->blockSignals(false);

    setToSetProgressControl = false;
    if (checkBoxColorIndexing->isChecked())
        comboBoxColor->setCurrentIndex(firstColor);

    if (currentGraphAutoscaled)
    {
        g->enableAutoscaling(true);
        g->setAutoScale();
    }

    if (app()->findActiveGraph(g))
    {
        g->replot();
        g->notifyChanges();
    }
}
//***************************************************
//*** Set to Set simlate
//***************************************************
void fittable18::simulateMultifitTables(){
    setToSetSimulYN=true;
    setBySetFitOrSim(false);
}
//***************************************************
//*** autoSimulateCurveInSimulations
//***************************************************
void fittable18::autoSimulateCurveInSimulations(int,int){
    if (!checkBoxAutoRecalculateSim->isChecked()) return;    

    tableParaSimulate->blockSignals(true);
    simulateSwitcher();
    tableParaSimulate->blockSignals(false);

    tableParaSimulate->setFocus();
}
