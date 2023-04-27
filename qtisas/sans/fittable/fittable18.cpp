/***************************************************************************
 File                   : fittable18.cpp
 Project                : QtiSAS
 --------------------------------------------------------------------
 Copyright              : (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)    : v.pipich*gmail.com
 Description            : table(s) fitting interface: cpp file
 
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
        if (w->metaObject()->className() == "Table" && rx.exactMatch(w->name()))
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
    if (app()->windowsList().count()==0 || !app()->activeWindow() || app()->activeWindow()->metaObject()->className() != "MultiLayer")  return false;
    
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
    if (app()->windowsList().count()==0 || !app()->activeWindow() || app()->activeWindow()->metaObject()->className() != "MultiLayer")  return false;
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


//+++++FUNCTION::check Table Existence ++++++++++++++++++++++++
bool fittable18::checkTableExistence(QString tableName, Table* &w)
{
    QList<MdiSubWindow *> tableList=app()->tableList();
    //+++
    foreach (MdiSubWindow *t, tableList)
    {
        if (t->name()==tableName)
        {
            w=(Table *)t;
            return true;
        }
    }
    return false;
}

//+++++FUNCTION::check Table Existence ++++++++++++++++++++++++
bool fittable18::checkTableExistence(QString tableName)
{
    QList<MdiSubWindow *> tableList=app()->tableList();
    
    //+++
    foreach (MdiSubWindow *t, tableList)
    {
        if (t->name()==tableName)  return true;
    }
    
    return false;
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
        if (w->metaObject()->className() == "Note" && w->name()==name)
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

//*******************************************
//+++  callFromTerminal
//*******************************************
bool fittable18::callFromTerminal(QString commandLine)
{
    commandLine=commandLine.simplified();
    commandLine=commandLine.trimmed();
    
    QStringList lst;
    lst.clear();
    lst = commandLine.split(" ", QString::SkipEmptyParts);
    
    if (lst.count()==0) return false;
    
    QString command=lst[0];
    
    int id=widgetStackFit->currentIndex();
    
    //+++ id ==0
    
    if(command=="setFunction")
    {
        if (id!=0 || lst.count()!=2) return false;
        QString function=lst[1];
        scanGroup();
        
        //+++ 2020-06 QLISTVIEW
        const QModelIndexList indexes = listBoxGroupNew->model()->match(listBoxGroupNew->model()->index(0,0),Qt::DisplayRole,"ALL",1,Qt::MatchExactly);
        groupFunctions(indexes[0],indexes[0]);
        if (indexes.size()<1) return false;
        listBoxGroupNew->setCurrentIndex(indexes.at(0));
        //---
        
        
        openDLL(function);
        return true;
    }
    
    if(command=="setMode")
    {
        if (id!=0 || lst.count()!=2) return false;
        int mode=lst[1].toInt();
        if(mode>2) return false;
        if (mode==0)
        {
            comboBoxInstrument->setCurrentIndex(0);
            checkBoxSANSsupport->setChecked(false);
        }
        else
        {
            checkBoxSANSsupport->setChecked(true);
            comboBoxInstrument->setCurrentIndex(mode-1);
        }
        SANSsupportYN();
        return true;
    }
    
    if(command=="setGlobalMode")
    {
        if (id!=0 || lst.count()!=2) return false;
        int curves=lst[1].toInt();
        if(curves>20) return false;
        if (curves<=1)
        {
            spinBoxNumberCurvesToFit->setValue(1);
            checkBoxMultiData->setChecked(false);
        }
        else
        {
            checkBoxMultiData->setChecked(true);
            spinBoxNumberCurvesToFit->setValue(curves);
        }
        initMultiParaTable();
        return true;
    }
    
    if(command=="setSuperpositionalMode")
    {
        if (id!=0 || (lst.count()!=2 && lst.count()!=4) ) return false;
        
        if (lst[1].toInt()==0) checkBoxSuperpositionalFit->setChecked(false);
        else checkBoxSuperpositionalFit->setChecked(true);
        
        
        if (lst.count()==2) return true;
        
        spinBoxSubFitNumber->setValue(int(fabs(double(lst[2].toInt()))));
        return true;
    }
    
    if(command=="e-Fit")
    {
        if (id!=0) return false;
        return iFit();
    }
    
    if(command=="e-Fit+++")
    {
        return iFitAdv();
    }
    
    if(command=="next")
    {
        slotStackFitNext();
        return true;
    }
    
    if(command=="prev")
    {
        slotStackFitPrev();
        return true;
    }
    
    if(command=="setActiveCurve")
    {
        if (id!=1 || lst.count()!=2) return false;
        
        int M=spinBoxFnumber->value();
        int m=lst[1].toInt();
        if (m<1 || m>M) return false;
        return selectActiveCurve(m);
    }
    
    if(command=="setData")
    {
        if (id!=1 || lst.count()!=3) return false;
        
        int M=spinBoxFnumber->value();
        int m=lst[1].toInt();
        if (m<1 || m>M) return false;
        
        QString name=lst[2];
        return selectData(m, name);
    }
    
    if(command=="setRange")
    {
        if (id!=1 || lst.count()!=5) return false;
        
        int M=spinBoxFnumber->value();
        int m=lst[1].toInt();
        if (m<1 || m>M) return false;
        
        QString QN=lst[2];
        
        double min  = lst[3].toDouble();
        double max = lst[4].toDouble();
        
        return selectRange(m, QN, min, max);
    }
    
    if(command=="fit")
    {
        if (id!=1) return false;
        int repeat=1;
        
        if (lst.count()>=2 && lst[1].toInt()>1) repeat=lst[1].toInt();
        
        for (int ff=0; ff<repeat; ff++) fitSwitcher();
    }
    
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
