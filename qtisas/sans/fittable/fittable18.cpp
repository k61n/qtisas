/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s) fitting interface
 ******************************************************************************/

#include "fittable18.h"
#include <QDockWidget>
#include <QUrl>
#include "Folder.h"


fittable18::fittable18(QWidget *parent)
: QWidget(parent)
{
    setupUi(this);
    connectSlot();
    initFITTABLE();
}


fittable18::~fittable18()
{
}

//*******************************************
//+++ setPathExtern
//*******************************************
void fittable18::setPathExtern(QString path)
{
    libPath=path;
    //fitPath->setText(path);
}
//*******************************************
//+++ app
//*******************************************
ApplicationWindow* fittable18::app()
{
    QDockWidget *docWin=(QDockWidget*)this->parentWidget();
    return (ApplicationWindow *)docWin->parent();
}

//*******************************************
//*** Log-output
//*******************************************
void fittable18::toResLog(QString text)
{
    QString info =text;
    if (app()->dockWidgetArea(app()->logWindow) == Qt::RightDockWidgetArea )
    {
        app()->current_folder->appendLogInfo(text);
        app()->results->setText(app()->current_folder->logInfo());
    }
    else app()->showResults(text, true);
}

//*******************************************
//*** removeTables by pattern
//*******************************************
void fittable18::removeTables(QString pattern)
{
    QList<MdiSubWindow *> windows = app()->windowsList();
    
    QRegExp rx(pattern);
    rx.setPatternSyntax(QRegExp::Wildcard);
    

    foreach (MdiSubWindow *w, windows)
    {
        if (QString(w->metaObject()->className()) == "Table" && rx.exactMatch(w->name()))
        {
            Table *close =(Table*)w;
            close->askOnCloseEvent(false);
            app()->closeWindow(close);
        }
    }
}

//*********************************************************
//*** findActiveGraph
//*********************************************************
bool fittable18::findActiveGraph( Graph * & g)
{
    if (app()->windowsList().count()==0 || !app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "MultiLayer")  return false;
    
    MultiLayer* plot = (MultiLayer*)app()->activeWindow();
    
    if (plot->isEmpty()) return false;
    
    g = (Graph*)plot->activeLayer();
    
    return true;
}

//*********************************************************
//*** findActivePlot
//*********************************************************
bool fittable18::findActivePlot( MultiLayer* & plot)
{
    if (app()->windowsList().count()==0 || !app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "MultiLayer")  return false;
    plot = (MultiLayer*)app()->activeWindow();
    if (plot->isEmpty()) return false;
    return true;
}
//*********************************************************
// +++  Add Curve  +++
//*********************************************************
bool fittable18::AddCurve(Graph* g,QString curveName)
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
        QMessageBox::warning(this,tr("QtiSAS"),
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

//*******************************************
//+++  find Table List By Label
//*******************************************
void fittable18::findTableListByLabel(QString tableLabel,QStringList  &list)
{
    list.clear();
    //+++
    QList<MdiSubWindow *> tableList=app()->tableList();
    //+++
    foreach (MdiSubWindow *t, tableList)
    {
        if (((Table*)t)->windowLabel().contains(tableLabel) ) list<<((Table*)t)->name();
    }
}


//*******************************************
//+++  makeNote
//*******************************************
void fittable18::makeNote(QString info, QString name, QString label)
{
    QList<MdiSubWindow *> windows = app()->windowsList();

    foreach (MdiSubWindow *w, windows)
    {
        if (QString(w->metaObject()->className()) == "Note" && w->name()==name)
        {
            ((Note *)w)->currentEditor()->setText(info);
            return;
        }
    }
    
    Note *nn=app()->newNote(name);
    nn->setWindowLabel(label);
    app()->setListViewLabel(nn->name(), label);
    app()->hideWindow(nn);
    app()->hideWindow(nn);

    //+++
    nn->currentEditor()->setText(info);
}


void fittable18::multiNSEfit(QString tableName, QString fitFunction, int from,int to)
{
#ifdef JNSE
#ifdef FITTABLE
    if (tableName=="") return;
    if (fitFunction=="") return;
    if (from>to) return;
    int M=to-from+1;
    
    if ( widgetStackFit->currentIndex()>0 ) slotStackFitPrev();
    if ( widgetStackFit->currentIndex()>0 ) slotStackFitPrev();
    
    const QModelIndexList indexes = listBoxFunctionsNew->model()->match(listBoxFunctionsNew->model()->index(0,0),Qt::DisplayRole,fitFunction,1,Qt::MatchExactly);
    if (indexes.size() <1) return;
    listBoxFunctionsNew->setCurrentIndex(indexes.at(0));
    
    checkBoxSANSsupport->setChecked ( false );
    checkBoxMultiData->setChecked ( true );
    spinBoxNumberCurvesToFit->setValue ( M );
    
    slotStackFitNext();
    
    int rawQ=F_paraListF.indexOf("Q");
    if (rawQ<0) rawQ=F_paraListF.indexOf("q");
    qDebug()<<rawQ<<" "<<F_paraListF[0];
    QList<MdiSubWindow *> tableList0=app()->tableList();
    //+++
    foreach (MdiSubWindow *w, tableList0)
    {
        if (w->name()==tableName)
        {
            Table *t=(Table *)w;
            for ( int j=0; j<M; j++ )
            {
                //tableCurves->setText ( 0,2*j,t->text ( from+j-1,3 ) );
                tableCurves->item (0,2*j)->setText(t->text ( from+j-1,19)+"**" );
                tableCurvechanged ( 0,2*j );
                tableCurves->item (0,2*j)->setText("" );
                if (rawQ>=0) tablePara->item( rawQ,3*j+2)->setText(t->text ( from+j-1,4 ) );
                tableCurvechanged ( 0,2*j+1 );
            }
            plotSwitcher();
        }
    }
   
#endif
#endif
}
