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
    info += "\n<b>chi<sup>2</sup></b> = " + textLabelChi->text();
    info += " [ <b>DoF</b> = " + QString::number(lastDoF) + " ]";
    info += "\n<b>R<sup>2</sup></b> = " + textLabelR2->text();
    info += "\n<b>Q-Factor</b> = " +
            QString::number(gsl_sf_gamma_inc_Q(lastDoF / 2.0, lastCHi2 * lastDoF / 2.0), 'E', prec + 2) + "\n";

    info += "\n<b>time</b> = " + textLabelTime->text() + "\n";

    if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "MultiLayer")
        return;
    
    MultiLayer* plot = (MultiLayer*)app()->activeWindow() ;
    
    if (plot->isEmpty()){
        QMessageBox::warning(this,tr("QtiKWS - Warning"),
                             tr("<h4>There are no plot layers available in this window.</h4>"
                                "<p><h4>Please add a layer and try again!</h4>"));
        return;
    }
    
    Graph *g = (Graph*)plot->activeLayer();
    if (g) g->newLegend(info);
    g->replot();
}
//*******************************************
//+++ Fit Table of Parameters Screenshot to graph +++
//*******************************************
void fittable18::addFitTableScreenshotToActiveGraph(){
    int M=spinBoxNumberCurvesToFit->value();
    tablePara->clearSelection();

    QHeaderView *vheader = tablePara->verticalHeader();
    QHeaderView *hheader = tablePara->horizontalHeader();
    // ask it to resize to size of all its text
    //vheader->setSectionResizeMode( QHeaderView::ResizeToContents );
    if(M>1) hheader->setSectionResizeMode( QHeaderView::ResizeToContents );
    // tell it we never want scrollbars so they are not shown disabled

    vheader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    hheader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tablePara->adjustSize();

    // loop all rows and cols and grap sizes
    int iWidth = 0;
    int iHeight = 0;
    for (int i = 0; i < tablePara->columnCount(); i++) {
        iWidth += hheader->sectionSize(i);
    }

    iWidth += vheader->width();
    for (int i = 0; i < tablePara->rowCount(); i++) {
        iHeight += vheader->sectionSize(i);
    }
    iHeight += hheader->height();
    QSize oldSize = tablePara->size();
    
    tablePara->resize(iWidth+10, iHeight+10);
    
    iWidth = 0;
    iHeight = 0;
    for (int i = 0; i < tablePara->columnCount(); i++) {
        iWidth += hheader->sectionSize(i);
    }
    
    iWidth += vheader->width();
    for (int i = 0; i < tablePara->rowCount(); i++) {
        iHeight += vheader->sectionSize(i);
    }
    iHeight += hheader->height();
    
    oldSize = tablePara->size();
    
    // now resize it to the size we just summed up
    tablePara->resize(iWidth+10, iHeight+10);
    tablePara->resize(iWidth, iHeight);
    
    // ask it to renader to a pixmap
    QPixmap screen_shot(tablePara->size()); //QSize(iWidth,iHeight));
    tablePara->render(&screen_shot);
    // restore org size

    tablePara->resize(oldSize);
    
    tablePara->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    tablePara->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    
    vheader->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    hheader->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    

    
    if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "MultiLayer")
        return;
    
    MultiLayer* plot = (MultiLayer*)app()->activeWindow() ;
    
    if (plot->isEmpty())
    {
        QMessageBox::warning(this,tr("QtiKWS - Warning"),
                             tr("<h4>There are no plot layers available in this window.</h4>"
                                "<p><h4>Please add a layer and try again!</h4>"));
        
        return;
    }
    
    Graph *g = (Graph*)plot->activeLayer();
    if (g)
    {
        ImageWidget* iw=g->addImage(screen_shot.toImage());
        if (iw)
        {
            iw->setSize(iw->pixmap().size());
            //displayCoordinates(unitBox->currentIndex());
            g->multiLayer()->notifyChanges();
            iw->setAttachPolicy((FrameWidget::AttachPolicy)0);
        }
    }
    g->replot();
}
//*******************************************
//+++ Fit Table of Parameters Screenshot to graph +++
//*******************************************
void fittable18::addDataScreenshotToActiveGraph(){
    int M=spinBoxNumberCurvesToFit->value();
    tableCurves->clearSelection();
    //QPixmap screen_shot(tablePara->size());
    //tablePara->render(&screen_shot);
    
    // for readability
    
    QHeaderView *vheader = tableCurves->verticalHeader();
    QHeaderView *hheader = tableCurves->horizontalHeader();
    // ask it to resize to size of all its text
    //vheader->setSectionResizeMode( QHeaderView::ResizeToContents );
    if(M>1) hheader->setSectionResizeMode( QHeaderView::ResizeToContents );
    // tell it we never want scrollbars so they are not shown disabled
    vheader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    hheader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // loop all rows and cols and grap sizes
    int iWidth = 0;
    int iHeight = 0;
    for (int i = 0; i < tableCurves->columnCount(); i++) {
        iWidth += hheader->sectionSize(i);
    }
    
    iWidth += vheader->width();
    for (int i = 0; i < tableCurves->rowCount(); i++) {
        iHeight += vheader->sectionSize(i);
    }
    iHeight += hheader->height();
    QSize oldSize = tableCurves->size();
    
    tableCurves->resize(iWidth+10, iHeight+10);
//    tableCurves->resize(oldSize);
    
    iWidth = 0;
    iHeight = 0;
    for (int i = 0; i < tableCurves->columnCount(); i++) {
        iWidth += hheader->sectionSize(i);
    }
    
    iWidth += vheader->width();
    for (int i = 0; i < tableCurves->rowCount(); i++) {
        iHeight += vheader->sectionSize(i);
    }
    iHeight += hheader->height();
    
    oldSize = tableCurves->size();
    
    // now resize it to the size we just summed up
    tableCurves->resize(iWidth+10, iHeight+10);
    tableCurves->resize(iWidth, iHeight);
    // ask it to renader to a pixmap
    QPixmap screen_shot(tableCurves->size()); //QSize(iWidth,iHeight));
    tableCurves->render(&screen_shot);
    // restore org size
    tableCurves->resize(oldSize);
    
    tableCurves->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    tableCurves->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    
    vheader->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    hheader->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    
    
    if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "MultiLayer")
        return;
    
    MultiLayer* plot = (MultiLayer*)app()->activeWindow() ;
    
    if (plot->isEmpty())
    {
        QMessageBox::warning(this,tr("QtiKWS - Warning"),
                             tr("<h4>There are no plot layers available in this window.</h4>"
                                "<p><h4>Please add a layer and try again!</h4>"));
        
        return;
    }
    
    Graph *g = (Graph*)plot->activeLayer();
    if (g)
    {
        ImageWidget* iw=g->addImage(screen_shot.toImage());
        if (iw)
        {
            iw->setSize(iw->pixmap().size());
            //displayCoordinates(unitBox->currentIndex());
            g->multiLayer()->notifyChanges();
            iw->setAttachPolicy((FrameWidget::AttachPolicy)0);
        }
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
//  Multi Table:: select Pattern
//***************************************************
void fittable18::selectPattern(){
    bool addAllColsSep=checkBoxBatchFitAllYCols->isChecked(); //+++ 2018-07 //@
    
    tableMultiFit->setRowCount(1);
    
    QStringList tables, tablesAll, tablesCols; //@
    //	tables<<"All";
    tablesAll=app()->tableNames();
    
    QRegExp rx( lineEditPattern->text());
    rx.setPatternSyntax(QRegExp::Wildcard);
    
    for (int i=0; i<tablesAll.count();i++) if (rx.exactMatch(tablesAll[i])) tables<<tablesAll[i];
    
    tables.prepend("All");
    tableMultiFit->setRowCount(tables.count());
//    tableMultiFit->setVerticalHeaderLabels(tables);

    tablesCols.prepend("All"); //@ new
    int currentRaw=1; //@ new
    
    for (int ii=1; ii<tables.count();ii++){
        tablesCols<<tables[ii]; //@ new
        
        // +++
        QTableWidgetItem *yn = new QTableWidgetItem();
        yn->setCheckState(Qt::Unchecked);
        tableMultiFit->setItem(currentRaw,0, yn);
        // +++
        QComboBox *yCol = new QComboBox();
        tableMultiFit->setCellWidget(currentRaw,1,yCol);

        // +++
        QComboBox *dYcol = new QComboBox();
        tableMultiFit->setCellWidget(currentRaw,2,dYcol);
        
        QRegExp rxCol(tables[ii]+"_*");
        rxCol.setPatternSyntax(QRegExp::Wildcard);
        
        // QStringList cols,colTemp; //@ new
        QStringList colsY, colsYerr, colsXerr, colTemp; //@ new
        
        // +++
        colTemp=app()->columnsList(Table::Y);
        for (int j=0; j<colTemp.count();j++){
            if (rxCol.exactMatch(colTemp[j])) colsY<<colTemp[j].remove(tables[ii]+"_");
        }
        yCol->addItems(colsY);

        // +++
        colTemp=app()->columnsList(Table::yErr);
        for (int j=0; j<colTemp.count();j++){
            if (rxCol.exactMatch(colTemp[j])) colsYerr<<colTemp[j].remove(tables[ii]+"_");
        }
        dYcol->addItems(colsYerr);

        int start=3;
        if (checkBoxSANSsupport->isChecked()){
            // +++
            start++;
            
            // +++
            QComboBox *xCol = new QComboBox();
            tableMultiFit->setCellWidget(currentRaw,3,xCol);

            QString currentInstrument=comboBoxInstrument->currentText();
            if ( currentInstrument.contains("Back") ) 
                colsXerr<<"from SPHERES";
            else{
                colsXerr<<"calculated in \"ASCII.1D.SANS\"";
                colsXerr<<"\"01%\":  sigma(Q)=0.01*Q";
                colsXerr<<"\"02%\":  sigma(Q)=0.02*Q";
                colsXerr<<"\"05%\":  sigma(Q)=0.05*Q";
                colsXerr<<"\"10%\":  sigma(Q)=0.10*Q";
                colsXerr<<"\"20%\":  sigma(Q)=0.20*Q";
            }
            colTemp=app()->columnsList(Table::xErr);
            for (int j=0; j<colTemp.count();j++){
                if (rxCol.exactMatch(colTemp[j])) colsXerr<<colTemp[j].remove(tables[ii]+"_");
            }
            
            xCol->addItems(colsXerr);
        }

        // +++Start values & adjustibility trasfer
        for (int j=start;j<tableMultiFit->columnCount();j++){
            tableMultiFit->setItem(currentRaw,j,new QTableWidgetItem(tablePara->item(j-start,2)->text()));
            QTableWidgetItem *fitYN = (QTableWidgetItem *)tableMultiFit->item (0,j);
            QTableWidgetItem *fitYN0 = (QTableWidgetItem *)tablePara->item(j-start,1);
            if (fitYN0->checkState()) fitYN->setCheckState(Qt::Checked); else fitYN->setCheckState(Qt::Unchecked);
        }
        currentRaw++;
        
        if ( addAllColsSep && colsY.count()>1){
            tableMultiFit->setRowCount(tableMultiFit->rowCount() + colsY.count()-1);
            
            for (int cols=1;cols<colsY.count();cols++){
                tablesCols<<tables[ii];
                // +++
                QTableWidgetItem *yn = new QTableWidgetItem();
                yn->setCheckState(Qt::Unchecked);
                tableMultiFit->setItem(currentRaw,0, yn);
                // +++
                QComboBox *yCol = new QComboBox();
                tableMultiFit->setCellWidget(currentRaw,1,yCol);
                // +++
                QComboBox *dYcol = new QComboBox();
                tableMultiFit->setCellWidget(currentRaw,2,dYcol);
                // +++
                yCol->addItems(colsY); yCol->setCurrentIndex(cols);
                // +++
                dYcol->addItems(colsYerr);
                int start=3;
                if (checkBoxSANSsupport->isChecked()){
                    // +++
                    start++;
                    
                    // +++
                    QComboBox *xCol = new QComboBox();
                    tableMultiFit->setCellWidget(currentRaw,3,xCol);
                    
                    xCol->addItems(colsXerr);
                }
                
                // +++Start values & adjustibility trasfer
                for (int j=start;j<tableMultiFit->columnCount();j++)
                {
                    tableMultiFit->setItem(currentRaw,j,new QTableWidgetItem(tablePara->item(j-start,2)->text()));
                    QTableWidgetItem *fitYN = (QTableWidgetItem *)tableMultiFit->item (0,j);
                    QTableWidgetItem *fitYN0 = (QTableWidgetItem *)tablePara->item(j-start,1);
                    if (fitYN0->checkState()) fitYN->setCheckState(Qt::Checked); else fitYN->setCheckState(Qt::Unchecked);
                }
                
                currentRaw++;
            }
        }
        
    }
    tableMultiFit->setVerticalHeaderLabels(tablesCols);   
    // +++Weiting option
    QTableWidgetItem *wYN = (QTableWidgetItem*)tableMultiFit->item (0,2);
    QTableWidgetItem *wYN0 = (QTableWidgetItem*)tableCurves->item (4,0);
    if (wYN0->checkState()) wYN->setCheckState(Qt::Checked); else wYN->setCheckState(Qt::Unchecked);
    // +++reso
    if (checkBoxSANSsupport->isChecked()){
        QTableWidgetItem *rYN = (QTableWidgetItem*)tableMultiFit->item (0,3);
        QTableWidgetItem *rYN0 = (QTableWidgetItem*)tableCurves->item (5,0);
        if (rYN0->checkState()) rYN->setCheckState(Qt::Checked); else rYN->setCheckState(Qt::Unchecked);
    }
    if (tableMultiFit->columnCount()>10) return; // no Stretch
    int startCol=3; if (checkBoxSANSsupport->isChecked()) startCol++;

    for (int tt = startCol; tt < tableMultiFit->columnCount(); tt++)
        tableMultiFit->resizeColumnToContents(tt);
}
//***************************************************
//  selectMultyFromTable
//***************************************************
void fittable18::selectMultyFromTable(){
    int p=spinBoxPara->value();
    // +++ FIRST LINE IS UNTOUCHED, REST  IS OUT
    tableMultiFit->setRowCount(1);
    //+++
    QStringList tablesAll,tablesSelected;
    //+++ ALL TABLES OF PROJECT
//    tablesAll=app()->tableNames();
    findTableListByLabel("Fitting Results:: Set-By-Set",tablesAll);
    
    //+++ WILD PATTERN FOR SKRIPT SELECTION
    QRegExp rx( lineEditPattern->text());
    rx.setPatternSyntax(QRegExp::Wildcard);
    
    for (int j=0; j<tablesAll.count(); j++){
        if (rx.exactMatch(tablesAll[j])) tablesSelected<<tablesAll[j];
    }
    //+++ SKRIPT TABLE SELECTION
    bool ok;
    QString skriptTable = QInputDialog::getItem(this,
                                                "QtiSAS", "Select a table with fitting results, you want to work again :", tablesSelected, 1, true, &ok);
    if ( !ok || skriptTable=="") return;
    
    //+++ FINDING SKRIPT TABLE OBJECT
    QList<MdiSubWindow *> windows = app()->windowsList();
    
    Table *skript;
    bool exist=false;
    
    foreach (MdiSubWindow *w, windows){
        if (QString(w->metaObject()->className()) == "Table" && w->name()==skriptTable){
            skript=(Table*)w;
            exist=true;
        }
    }
    
    if (!exist){
        QMessageBox::warning(this,tr("QtiKWS"),
                             "There is no table:: "+skriptTable);
        return;
    }
    //+++ COUNTER: NUMBER OF EXISTING TABLES IN SKRIPT TABLE
    int activeDatasets=0;
    //+++ LIST OF ALL Y-COLUMNS
    QStringList colTemp=app()->columnsList(Table::Y);
    //+++ LIST OF EXISTING COLUMNS IN SKRIPT TABLE
    QStringList tables;
    //+++ FIRST STEP OF DATA TRANSFET ANF MULTITABLE ARRANGMENT | USED FOR LEFT HEADER
    for (int i=0; i< skript->numRows();i++){
        colTemp=app()->columnsList(Table::Y);
        QString info=skript->text(i,3);
        QString currentTable=info.left(info.indexOf("|t|")).trimmed();
        QString currentY=info.mid(info.indexOf("|t|")+3, info.indexOf("|y|") - info.indexOf("|t|")-3).trimmed();
        QString currentWeight=info.mid(info.indexOf("|y|")+3, info.indexOf("|w|") - info.indexOf("|y|")-3).trimmed();
        QString currentReso =
            info.mid(info.indexOf("|w|") + 3, info.indexOf("|r|") - info.indexOf("|w|") - 3).trimmed();
        
        if (colTemp.contains(currentTable+"_"+currentY)){
            tableMultiFit->setRowCount(activeDatasets + 2); //+++ add row to table
            tables<<currentTable; //+++
            
            QStringList cols;
            //+++ CURRENT TABLE NAME
            QRegExp rxCol(currentTable+"_*"); //+++ WILD PATTERN OF Y-COLUMNS OF CURRENT DATASET
            rxCol.setPatternSyntax(QRegExp::Wildcard);
            // +++
            QTableWidgetItem *yn = new QTableWidgetItem();
            yn->setCheckState(Qt::Unchecked);
            tableMultiFit->setItem(activeDatasets+1,0, yn);
            // +++
            QComboBox *yCol = new QComboBox();
            tableMultiFit->setCellWidget(activeDatasets+1,1,yCol);
    
            for (int j=0; j<colTemp.count();j++){
                if (rxCol.exactMatch(colTemp[j])) cols<<colTemp[j].remove(currentTable+"_");
            }
            yCol->addItems(cols);
            yCol->setItemText(yCol->currentIndex(), currentY);
            // +++
            QComboBox *dYcol = new QComboBox();
            tableMultiFit->setCellWidget(activeDatasets+1,2,dYcol);
            colTemp=app()->columnsList(Table::yErr);
            cols.clear();
            for (int j=0; j<colTemp.count();j++){
                if (rxCol.exactMatch(colTemp[j])) cols<<colTemp[j].remove(currentTable+"_");
            }
            dYcol->addItems(cols);
            dYcol->setItemText(dYcol->currentIndex(), currentWeight);
            //+++
            int resoShift=0;
            if (checkBoxSANSsupport->isChecked()){
                // +++
                QComboBox *resoCol = new QComboBox();
                tableMultiFit->setCellWidget(activeDatasets+1,3,resoCol);
                
                colTemp=app()->columnsList(Table::xErr);
                cols.clear();
                for (int j=0; j<colTemp.count();j++){
                    if (rxCol.exactMatch(colTemp[j])) cols<<colTemp[j].remove(currentTable+"_");
                }
                resoCol->addItems(cols);
                resoCol->setItemText(resoCol->currentIndex(), currentReso);
                resoShift++;
            }
            
            for (int j=0;j<p;j++){
                tableMultiFit->setItem(activeDatasets+1,3+j+resoShift,new QTableWidgetItem(skript->text(i,7+2*j)) );
            }
            activeDatasets++;
        }
    }
    tables.prepend("All");
    tableMultiFit->setVerticalHeaderLabels(tables);
    
    if (tableMultiFit->columnCount()>10) return; // no Stretch
    int startCol=3; if (checkBoxSANSsupport->isChecked()) startCol++;

    for (int tt=startCol; tt<tableMultiFit->columnCount();tt++) tableMultiFit->resizeColumnToContents(tt);
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
    //+++ Table Name
    Table *t;
    int xColIndex,yColIndex;
    //+++
    QString curveName=comboBoxDatasetSim->currentText();
    QString tableName=curveName.left(curveName.lastIndexOf("_"));
    if ( !findFitDataTable(curveName, t, xColIndex,yColIndex ) ) return;
    int N=t->numRows();
    int ii=0;
    while(t->text(ii,xColIndex) == "" && ii<N) ii++;
    // +++
    int Ntot=0;
    QRegExp rx( "((\\-|\\+)?\\d*(\\.|\\,)\\d*((e|E)(\\-|\\+)\\d*)?)|((\\-|\\+)?\\d+)" );
    for(int j=0;j<N;j++) if (rx.exactMatch(t->text(j,xColIndex))) Ntot++;
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
    } else{
        textLabelRangeFirstLimit->setText(QString::number(GSL_MIN(min,max)));
        textLabelRangeLastLimit->setText(QString::number(max));
        textLabelRangeFirst->setText(QString::number(GSL_MIN(min,max)));
        textLabelRangeLast->setText(QString::number(max));
    }
}
