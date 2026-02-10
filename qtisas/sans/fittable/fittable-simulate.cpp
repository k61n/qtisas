/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s)'s after fit tools
 ******************************************************************************/

#include <QPainter>

#include "fittable18.h"
#include "ImageWidget.h"

//*******************************************
// slot: fit results to "res" winwow !OB
//*******************************************
void fittable18::resToLogWindow(){
    int M=spinBoxNumberCurvesToFit->value();
    int p=spinBoxPara->value();
    QString F_name=textLabelFfunc->text();
    
    QDateTime dt = QDateTime::currentDateTime ();
    QLocale l = app()->locale();
    QString info = "[ " + l.toString(dt, QLocale::ShortFormat) + " ]\n";
    info += tr("Fit Method") + ": " +comboBoxFitMethod->currentText() +"\n";
    info += tr("Using Function") + ": " + F_name + "\n";
    for (int mm=0;mm<M;mm++){
        info+=((QComboBoxInTable*)tableCurves->cellWidget(0,2*mm+1))->currentText()+ " : \n";
        for (int pp=0;pp<p;pp++){
            info+= F_paraList[pp]+" = "+tablePara->item(pp,3*mm+2)->text();
            
            if (tablePara->item(pp,3*mm+3)->text()!="---"){
                info+=" ";
                info+=QChar(177);
                info+=" "+tablePara->item(pp,3*mm+3)->text().remove(QChar(177));
            }
            info+="\n";
        }
    }
    info+="chi^2 = "+textLabelChi->text()+"\n";
    info+="R^2 = "+textLabelR2->text()+"\n";
    info+="time = "+textLabelTime->text()+"\n";
    info=info+"-------------------------------------------------------------"+"\n";
    
    toResLog(info);
}
//*******************************************
// slot: fit results to "res" window (one line) !OB
//*******************************************
void fittable18::resToLogWindowOne(){
    int p=spinBoxPara->value();
    int M=spinBoxNumberCurvesToFit->value();
    QString F_name=textLabelFfunc->text();
    
    QDateTime dt = QDateTime::currentDateTime ();
    QLocale l = app()->locale();
    QString info = "[ " + l.toString(dt, QLocale::ShortFormat) + " ]\n";
    info += tr("Fit Method") + ": " +comboBoxFitMethod->currentText() +"\n";
    info += tr("Using Function") + ": " + F_name + "\n";

    for (int pp=0;pp<p;pp++){
        info+= F_paraList[pp]+"\t"+F_paraList[pp]+"-error\t";
    }
    info+="\n";
    for (int mm=0;mm<M;mm++){
        for (int pp=0;pp<p;pp++){
            info+= tablePara->item(pp,3*mm+2)->text()+"\t "+tablePara->item(pp,3*mm+3)->text().remove(QChar(177))+"\t";
        }
        info+="\n";
        info+="("+((QComboBoxInTable*)tableCurves->cellWidget(0,2*mm+1))->currentText()+ ")  \n";
    }
    info+="chi^2 = "+textLabelChi->text()+"\n";
    info+="R^2 = "+textLabelR2->text()+"\n";
    info+="time = "+textLabelTime->text()+"\n";
    info=info+"-------------------------------------------------------------"+"\n";
    
    toResLog(info);
}
//*******************************************
// slot: make NEW table with fit results !OB
//*******************************************
void fittable18::newTabRes(){
    //app(this)->changeFolder("FIT :: 1D");
    
    QStringList RES;
    QString F_name=textLabelFfunc->text();
    
    RES<<F_name<<QString::number(spinBoxPara->value())<<QString::number(spinBoxNumberCurvesToFit->value());
    //	RES<<comboBoxHorV->currentText ();
    RES+=d_param_names;
    RES<<"chi^2="+textLabelChi->text()<<"time="+textLabelTime->text();
    
    for (int mm=0;mm<spinBoxNumberCurvesToFit->value();mm++)
        for (int pp=0;pp<spinBoxPara->value();pp++){
            RES<< tablePara->item(pp, 3*mm+2)->text()<<tablePara->item(pp, 3*mm+2)->text();
        }
    
    // +++ NewTabRes(RES)
    int p=spinBoxPara->value();
    int M=spinBoxNumberCurvesToFit->value();
    Table* w;
    
    QString temp=app()->generateUniqueName(tr(QString("newFitTableRow"+F_name).toLocal8Bit().constData()));
    
    w=app()->newTable(temp, GSL_MAX(p, 6), 3+2*M);
    w->setTextFormat(true);
    w->setColName(0,"Characteristics");w->setTextFormat(0);
    w->setColName(1,"Conditions"); w->setTextFormat(1);
    w->setColName(2,"Parameters"); w->setTextFormat(2);
    // First Col
    w->setText(0,0,"Fitting Function");
    w->setText(0,1,"->   "+F_name);
    //
    w->setText(1,0,"Number of Parameters");
    w->setText(1,1, "->   "+QString::number(p) );
    //
    w->setText(2,0,"Number of data sets M");
    w->setText(2,1,"->   "+QString::number(M));
    //
    w->setText(3,0,"chi^2");
    w->setText(3,1,"->   "+textLabelChi->text());
    //
    w->setText(4,0,"R^2");
    w->setText(4,1,"->   "+textLabelR2->text());
    //
    w->setText(5,0,"Time of Fit");
    w->setText(5,1,"->   "+textLabelTime->text());
    
    QString yn0="";
    
    for (int pp=0;pp<p;pp++){w->setText(pp,2,F_paraList[pp]);};
    
    for (int mm=0;mm<M;mm++) {
        if (mm>=9) yn0=""; else yn0="0";
        w->setColName(2*mm+3,"Value"+yn0+QString::number(mm+1));w->setTextFormat(2*mm+3);
        w->setColName(2*mm+4,"Error"+yn0+QString::number(mm+1));w->setTextFormat(2*mm+4);
        for (int pp=0;pp<p;pp++){
            w->setText(pp, 3+2*mm, tablePara->item(pp, 3*mm+2)->text());
            w->setText(pp, 4+2*mm, tablePara->item(pp, 3*mm+3)->text().remove(QChar(177)));
        }
    }
    w->adjustColumnsWidth(false);
}

//*******************************************
// slot: make NEW table with fit results (Col) 2
//*******************************************
void fittable18::newTabResCol(){
    //app(this)->changeFolder("FIT :: 1D");
    
    QStringList RES;
    QString F_name=textLabelFfunc->text();
    
    RES<<F_name<<QString::number(spinBoxPara->value())<<QString::number(spinBoxNumberCurvesToFit->value());
    //	RES<<comboBoxHorV->currentText ();
    RES+=d_param_names;
    RES<<"chi^2="+textLabelChi->text()<<"time="+textLabelTime->text();
 
    for (int mm=0;mm<spinBoxNumberCurvesToFit->value();mm++)
        for (int pp=0;pp<spinBoxPara->value();pp++){
            RES<< tablePara->item(pp, 3*mm+2)->text()<<tablePara->item(pp, 3*mm+2)->text();
        }
    
    // +++ NewTabRes(RES)
    
    int p=spinBoxPara->value();
    int M=spinBoxNumberCurvesToFit->value();
    Table* w;
    
    QString temp=app()->generateUniqueName(tr(QString("newFitTableCol"+F_name).toLocal8Bit().constData()));
       
    w=app()->newTable(temp, GSL_MAX(M, 6), 2+1+2*p);
    w->setTextFormat(true);
 
    w->setColName(0,"Characteristics");
    w->setColPlotDesignation(0,Table::None);
    w->setTextFormat(0);
    
    w->setColName(1,"Conditions");
    w->setColPlotDesignation(1,Table::None);
    w->setTextFormat(1);
    
    w->setColName(2,"Nr");
    w->setColPlotDesignation(2,Table::X);
    w->setColNumericFormat(0, 6, 2);
    w->setTextFormat(2);
    
    // First Col
    w->setText(0,0,"Fitting Function");
    w->setText(0,1,"->   "+F_name);
    //
    w->setText(1,0,"Number of Parameters");
    w->setText(1,1, "->   "+QString::number(p) );
    //
    w->setText(2,0,"Number of data sets M");
    w->setText(2,1,"->   "+QString::number(M));
    //
    w->setText(3,0,"chi^2");
    w->setText(3,1,"->   "+textLabelChi->text());
    //
    w->setText(4,0,"R^2");
    w->setText(4,1,"->   "+textLabelR2->text());
    //
    w->setText(5,0,"Time of Fit");
    w->setText(5,1,"->   "+textLabelTime->text());
    //
    
    for (int pp=0;pp<p;pp++){
        w->setColName(3+2*pp,F_paraList[pp]);
        w->setColPlotDesignation(3+2*pp,Table::Y);
        w->setTextFormat(3+2*pp);
        
        w->setColName(3+2*pp+1,F_paraList[pp]+"Error");
        w->setColPlotDesignation(3+2*pp+1,Table::yErr);
        w->setTextFormat(3+2*pp+1);
        
        for (int mm=0;mm<M;mm++){
            w->setText(mm, 3+2*pp, tablePara->item(pp, 3*mm+2)->text());
            w->setText(mm, 3+2*pp+1, tablePara->item(pp, 3*mm+3)->text().remove(QChar(177)));
        }
    }
    for (int mm=0;mm<M;mm++) w->setText(mm, 2,QString::number(mm+1));
    
    w->adjustColumnsWidth(false);
}
//*******************************************
//+++ Fit results in graph +++
//*******************************************
void fittable18::addFitResultToActiveGraph(){
    int p=spinBoxPara->value();
    int M=spinBoxNumberCurvesToFit->value();

    int prec = spinBoxSignDigits->value();

    QString F_name=textLabelFfunc->text();
    
    QDateTime dt = QDateTime::currentDateTime ();
    QLocale l = app()->locale();
    QString info = "[" + l.toString(dt, QLocale::ShortFormat) + " ]\n";
    info = info+ "<b>Fit Method</b>" + ": " +comboBoxFitMethod->currentText() +"\n";
    info = info+ "<b>Using Function</b>" + ": " + F_name + "\n";
    
    for (int mm=0;mm<M;mm++){
        info+="\n<b>"+((QComboBoxInTable*)tableCurves->cellWidget(0,2*mm+1))->currentText()+"</b>"+ " : \n";
        
        for (int pp=0;pp<p;pp++){
            info+= "<b>"+F_paraList[pp]+"</b>"+" = "+tablePara->item(pp,3*mm+2)->text();
            if (tablePara->item(pp,3*mm+3)->text()!="---"){
                info+=" ";
                info+=QChar(177);
                info+=" "+tablePara->item(pp,3*mm+3)->text().remove(QChar(177));
            }
            info+="\n";
        }
    }
    QString sChi = QString::fromUtf8("χ");
    QString sChi2 = QString::fromUtf8("χ²");
    QString sRoot = QString::fromUtf8("√");

    info += "\n<b>" + sChi2 + "/dof </b> = " + QString::number(textLabelChi->text().toDouble(), 'E', 10);
    info += "\n<b>R<sup>2</sup></b> = " + textLabelR2->text();

    info += "\n\n<b>Residual Sum of Squares:</b> " + sChi + " = \t" +
            QString::number(textLabelChi->text().toDouble() * lastDoF, 'E', 10);
    info += "\n<b>Residual Standard Deviation:</b> " + sChi + "/" + sRoot + "dof = \t" +
            QString::number(sqrt(textLabelChi->text().toDouble()), 'E', 10);
    info += "\n<b>Degrees of Freedom:</b> dof = " + QString::number(lastDoF);

    info += "\n\n<b>Q-Factor</b> = " +
            QString::number(gsl_sf_gamma_inc_Q(lastDoF / 2.0, lastCHi2 * lastDoF / 2.0), 'E', prec + 2) + "\n";

    info += "\n<b>time</b> = " + textLabelTime->text() + "\n";

    if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "MultiLayer")
        return;
    
    MultiLayer* plot = (MultiLayer*)app()->activeWindow() ;
    
    if (plot->isEmpty()){
        QMessageBox::warning(this, "QtiSAS - Warning",
                             tr("<h4>There are no plot layers available in this window.</h4>"
                                "<p><h4>Please add a layer and try again!</h4>"));
        return;
    }
    
    Graph *g = (Graph*)plot->activeLayer();
    if (g) g->newLegend(info);
    g->replot();
}

//+++ Add a QTableWidget Screenshot to active graph
void fittable18::tableScreenshotToActiveGraph(QTableWidget *table)
{
    int M = spinBoxNumberCurvesToFit->value();
    table->clearSelection();

    QHeaderView *vheader = table->verticalHeader();
    QHeaderView *hheader = table->horizontalHeader();

    if (M > 1)
        hheader->setSectionResizeMode(QHeaderView::ResizeToContents);

    vheader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    hheader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    int iWidth = vheader->width();
    for (int i = 0; i < table->columnCount(); ++i)
        iWidth += hheader->sectionSize(i);

    int iHeight = hheader->height();
    for (int i = 0; i < table->rowCount(); ++i)
        iHeight += vheader->sectionSize(i);

    QSize originalSize = table->size();
    table->resize(iWidth + 10, iHeight + 10);

    QPixmap screen_shot = table->grab();

    table->resize(originalSize);
    vheader->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    hheader->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "MultiLayer")
        return;

    auto *plot = dynamic_cast<MultiLayer *>(app()->activeWindow());
    if (plot->isEmpty())
    {
        QMessageBox::warning(this, tr("QtiSAS - Warning"),
                             tr("<h4>There are no plot layers available in this window.</h4>"
                                "<p><h4>Please add a layer and try again!</h4>"));
        return;
    }

    Graph *g = plot->activeLayer();
    if (!g)
        return;

    ImageWidget *iw = g->addImage(screen_shot.toImage());
    if (iw)
    {
        iw->resize(iWidth, iHeight);
        iw->setAttachPolicy(FrameWidget::AttachPolicy(0));
        g->multiLayer()->notifyChanges();
    }
    g->replot();
}

// +++
void fittable18::uniformSimulChanged(bool status){
    
    if (status){
        radioButtonSameQrange->setChecked(false);
        
        textLabelChi2Sim->hide();
        textLabelChi2dofSim->hide();
        textLabelR2sim->hide();
        
        textLabelChiLabelSim->hide();
        textLabelChiLabelDofSim->hide();
        textLabelR2simInt->hide();
        
        groupBoxPointsPara->hide();
        groupBoxSimRange->hide();

        checkBoxWeightSim->hide();
        
        groupBoxQrange->show();
    } else{
        radioButtonSameQrange->setChecked(true);
        
        textLabelChi2Sim->show();
        textLabelChi2dofSim->show();
        textLabelR2sim->show();
        
        textLabelChiLabelSim->show();
        textLabelChiLabelDofSim->show();
        textLabelR2simInt->show();
        
        groupBoxPointsPara->show();
        groupBoxSimRange->show();
        
        checkBoxWeightSim->show();
        
        groupBoxQrange->hide();
    }
}

//***************************************************
// +++ logStepChanged
//***************************************************
void  fittable18::logStepChanged( bool status ){
    lineEditImin->setText("0");
    if (status ){
        textLabelmin->show();
        lineEditImin->show();
        
        if (lineEditFromQsim->text().toDouble()<=0) lineEditFromQsim->setText("0.001");
    } else{
        textLabelmin->hide();
        lineEditImin->hide();
    }
}
//***************************************************
// +++ theSameSimulChanged
//***************************************************
void fittable18::theSameSimulChanged(bool status){
    if (status){
        radioButtonUniform_Q->setChecked(false);
        textLabelChi2Sim->show();
        textLabelChi2dofSim->show();
        textLabelR2sim->show();
        
        textLabelChiLabelSim->show();
        textLabelChiLabelDofSim->show();
        textLabelR2simInt->show();
        
        groupBoxPointsPara->show();
        groupBoxSimRange->show();
        radioButtonUniform_Q->setMinimumHeight(int(app()->screenResoHight*app()->sasResoScale/50)+5);
        radioButtonUniform_Q->setMaximumHeight(int(app()->screenResoHight*app()->sasResoScale/50)+5);
        checkBoxWeightSim->show();
        
        groupBoxQrange->hide();
        radioButtonSameQrange->setMinimumHeight(3*int(app()->screenResoHight*app()->sasResoScale/50)+45);
        radioButtonSameQrange->setMaximumHeight(3*int(app()->screenResoHight*app()->sasResoScale/50)+45);
    } else{
        radioButtonUniform_Q->setChecked(true);
        
        textLabelChi2Sim->hide();
        textLabelChi2dofSim->hide();
        textLabelR2sim->hide();
        
        textLabelChiLabelSim->hide();
        textLabelChiLabelDofSim->hide();
        textLabelR2simInt->hide();
        
        groupBoxPointsPara->hide();
        groupBoxSimRange->hide();
        radioButtonUniform_Q->setMinimumHeight(5*int(app()->screenResoHight*app()->sasResoScale/50)+55);
        radioButtonUniform_Q->setMaximumHeight(5*int(app()->screenResoHight*app()->sasResoScale/50)+55);
        
        checkBoxWeightSim->hide();
        
        groupBoxQrange->show();
        //radioButtonSameQrange->setMaximumHeight(20);
        radioButtonSameQrange->setMinimumHeight(int(app()->screenResoHight*app()->sasResoScale/50)+5);
        radioButtonSameQrange->setMaximumHeight(int(app()->screenResoHight*app()->sasResoScale/50)+5);
    }
}

//***************************************************
//  removeSimulatedDatasets
//***************************************************
void fittable18::removeSimulatedDatasets(){
    QString F_name=textLabelFfunc->text();
    if ( tabWidgetGenResults->currentIndex()==0 ){
        removeTables( "simulatedCurve-"+textLabelFfunc->text()+"*");
    }
    if ( tabWidgetGenResults->currentIndex()==2 ){
        removeTables( "simulatedCurve-"+textLabelFfunc->text()+"-set*");
        removeTables( "fitCurve-"+textLabelFfunc->text()+"-set*");
    }
    updateDatasets();
}
//***************************************************
// +++ removeFitCurve
//***************************************************
void fittable18::removeFitCurve(){
    removeTables( "fitCurve-*");
    updateDatasets();
}
//***************************************************
// +++ removeSimulatedCurve
//***************************************************
void fittable18::removeSimulatedCurve(){
    removeTables( "simulatedCurve-*");
    updateDatasets();
}
//***************************************************
// +++ removeGlobal
//***************************************************
void fittable18::removeGlobal(){
    removeTables( "*-global-*");
    updateDatasets();
}
//*******************************************
//  dataLimitsSimulation
//*******************************************
void fittable18::dataLimitsSimulation(int value){
    QString NQ=comboBoxSimQN->currentText();

    //+++
    QString curveName = comboBoxDatasetSim->currentText();
    QString tableName = curveName.left(curveName.lastIndexOf("_"));
    QString colName = curveName.remove(tableName);

    Table *t;
    if (!app()->checkTableExistence(tableName, t))
        return;

    int yColIndex = t->colIndex(colName);
    int xColIndex = t->colX(yColIndex);
    if (yColIndex < 0 || xColIndex < 0)
        return;

    int N=t->numRows();
    int ii=0;
    while(t->text(ii,xColIndex) == "" && ii<N) ii++;
    // +++
    int Ntot=0;
    static const QRegularExpression rx(R"(((\-|\+)?\d*(\.|\,)\d*((e|E)(\-|\+)\d*)?)|((\-|\+)?\d+))");
    for (int j = 0; j < N; j++)
        if (rx.match(t->text(j, xColIndex)).hasMatch())
            Ntot++;
    if (Ntot<2) Ntot=1000;
    
    if (ii==N) return;
    
    double min=t->text(ii,xColIndex).toDouble();
    double max=t->text(ii,xColIndex).toDouble();
    
    for (int j=ii;j<N;j++){
        if ((t->text(j,xColIndex).toDouble())>max && t->text(j,xColIndex)!="") max=t->text(j,xColIndex).toDouble();
        if (t->text(j,xColIndex).toDouble()<min && t->text(j,xColIndex)!="") min=t->text(j,xColIndex).toDouble();
    }
    if(NQ=="Q"){
        textLabelRangeFirstLimit->setText(QString::number(GSL_MIN(1,Ntot)));
        textLabelRangeLastLimit->setText(QString::number(Ntot));
        textLabelRangeFirst->setText(QString::number(GSL_MIN(1,Ntot)));
        textLabelRangeLast->setText(QString::number(Ntot));
    }
    else
    {
        textLabelRangeFirstLimit->setText(QString::number(GSL_MIN(min, max), 'f', spinBoxSignDigits->value() + 2));
        textLabelRangeLastLimit->setText(QString::number(max, 'f', spinBoxSignDigits->value() + 2));
        textLabelRangeFirst->setText(QString::number(GSL_MIN(min, max), 'f', spinBoxSignDigits->value() + 2));
        textLabelRangeLast->setText(QString::number(max, 'f', spinBoxSignDigits->value() + 2));
    }
}
