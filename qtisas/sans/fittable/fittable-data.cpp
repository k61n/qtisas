/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s)'s data tab functions
 ******************************************************************************/

#include "fittable18.h"
#include <RangeSelectorTool.h>

//*******************************************
//  check new values in tableCurve
//*******************************************
void fittable18::tableCurvechanged( int raw, int col )
{
    tableCurves->blockSignals(true);

    if( (int(col/2)*2)!=col )
    {
        // +++
        if (raw==0)
        {

            // Selected data to Fit
            QString table=((QComboBoxInTable*)tableCurves->cellWidget(raw,col))->currentText();
            table=table.left(table.lastIndexOf("_"));
            // then set lists for dI and sigmaReso ; and Number of points
            colList(table,col);
        }
        
        if (raw==2)
        {
            QString NQ=((QComboBoxInTable*)tableCurves->cellWidget(raw-1,col-1))->currentText();
            
            
            if ((NQ=="N") && (((tableCurves->item(2,col)->text().toInt()) < 1) || ((tableCurves->item(2,col)->text().toInt()) > (tableCurves->item(3,col)->text().toInt())))) tableCurves->item(2,col)->setText("1");
        }
        if (raw==3)
        {
            QString NQ=((QComboBoxInTable*)tableCurves->cellWidget(raw-2,col-1))->currentText();
            if ((NQ=="N") && (((tableCurves->item(3,col)->text().toInt()) > (tableCurves->item(1,col)->text().toInt()) || (tableCurves->item(3,col)->text().toInt()) < (tableCurves->item(2,col)->text().toInt())) ))
                 tableCurves->item(raw,col)->setText(tableCurves->item(1,col)->text());
        }
        if (raw==0 || raw>3) tableCurves->setCurrentCell(raw, col);
        
    }
    else
    {
        
        if (raw==0)
        {
            QComboBoxInTable *comboList = (QComboBoxInTable*)tableCurves->cellWidget(0,col+1);
            QString pattern=tableCurves->item(0,col)->text();
            //+++ 2020.04
            QRegExp rx(pattern);
            rx.setPatternSyntax(QRegExp::Wildcard);
            //---
            
            int iNumber=comboList->count();
            int oldNumber=comboList->currentIndex();
            
            
            //if (!comboList->currentText().contains(pattern))
            if (!rx.exactMatch(comboList->currentText()))
            {
                int i=0;
                while ( i<iNumber) { if (rx.exactMatch(comboList->itemText(i))) break; i++;};
                comboList->blockSignals(true);
                if (i>=iNumber) comboList->setCurrentIndex(oldNumber); else comboList->setCurrentIndex(i);
                comboList->blockSignals(false);
                tableCurvechanged( 0, col+1 );
            }
        }
        if (raw==1)
        {
            
            
            QTableWidgetItem *iQmin = (QTableWidgetItem *)tableCurves->item (2,col);
            iQmin->setCheckState(Qt::Unchecked);
            tableCurves->item(2,col+1)->setFlags(tableCurves->item(2,col+1)->flags() & ~Qt::ItemIsEditable);
            tableCurves->item(2,col+1)->setBackground(QColor(236,236,236));
            
            
            QTableWidgetItem *iQmax  = (QTableWidgetItem *)tableCurves->item (3,col);
            iQmax ->setCheckState(Qt::Unchecked);
            tableCurves->item(3,col+1)->setFlags(tableCurves->item(3,col+1)->flags() & ~Qt::ItemIsEditable);
            tableCurves->item(3,col+1)->setBackground(QColor(236,236,236));
            
            
            QvsN();
        }
        
        if (raw==2)
        {
            if (((QTableWidgetItem *)tableCurves->item (2,col))->checkState()==Qt::Checked)
            {
                tableCurves->item(2,col+1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
                tableCurves->item(2,col+1)->setBackground(Qt::white);
            }
            else
            {
                tableCurves->item(2,col+1)->setFlags(tableCurves->item(2,col+1)->flags() & ~Qt::ItemIsEditable);
                tableCurves->item(2,col+1)->setBackground(QColor(236,236,236));
                QvsN();
            }

        }
        if (raw==3)
        {
            if (((QTableWidgetItem *)tableCurves->item (3,col))->checkState()==Qt::Checked)
            {
                tableCurves->item(3,col+1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
                tableCurves->item(3,col+1)->setBackground(Qt::white);
            }
            else
            {
                tableCurves->item(3,col+1)->setFlags(tableCurves->item(3,col+1)->flags() & ~Qt::ItemIsEditable);
                tableCurves->item(3,col+1)->setBackground(QColor(236,236,236));
                QvsN();
            }
        }
        if (raw==4)
        {
            if (((QTableWidgetItem *)tableCurves->item(4,col))->checkState()==Qt::Checked)
            {
                ((QTableWidgetItem *)tableCurves->item (4,col))->setText("on");

                tableCurves->item(4,col+1)->setText("");
                ((QComboBoxInTable*)tableCurves->cellWidget(4,col+1))->setMaximumWidth(((QComboBoxInTable*)tableCurves->cellWidget(0,col+1))->width());
                ((QComboBoxInTable*)tableCurves->cellWidget(4,col+1))->setMinimumWidth(((QComboBoxInTable*)tableCurves->cellWidget(0,col+1))->width());
                ((QComboBoxInTable*)tableCurves->cellWidget(4,col+1))->setMaximumWidth(10000);
                ((QComboBoxInTable*)tableCurves->cellWidget(4,col+1))->setMinimumWidth(0);
            }
            else
            {
                ((QTableWidgetItem *)tableCurves->item (4,col))->setText("off");

                ((QComboBoxInTable*)tableCurves->cellWidget(4,col+1))->setMinimumWidth(0);
                ((QComboBoxInTable*)tableCurves->cellWidget(4,col+1))->setMaximumWidth(0);
                tableCurves->item(4,col+1)->setText("... [w=1] ... ");
            }
        }
        if (raw==5)
        {
            if (((QTableWidgetItem *)tableCurves->item(5,col))->checkState()==Qt::Checked)
            {
                tableCurves->item(5,col)->setText("on");
                
                ((QComboBoxInTable*)tableCurves->cellWidget(5,col+1))->setMaximumWidth(((QComboBoxInTable*)tableCurves->cellWidget(0,col+1))->width());
                ((QComboBoxInTable*)tableCurves->cellWidget(5,col+1))->setMinimumWidth(((QComboBoxInTable*)tableCurves->cellWidget(0,col+1))->width());
                ((QComboBoxInTable*)tableCurves->cellWidget(5,col+1))->setMaximumWidth(10000);
                ((QComboBoxInTable*)tableCurves->cellWidget(5,col+1))->setMinimumWidth(0);
                tableCurves->item(5,col+1)->setText("");
            }
            else
            {
                tableCurves->item(5,col)->setText("off");

                ((QComboBoxInTable*)tableCurves->cellWidget(5,col+1))->setMinimumWidth(0);
                ((QComboBoxInTable*)tableCurves->cellWidget(5,col+1))->setMaximumWidth(0);
                tableCurves->item(5,col+1)->setText("... no resolution integration");
            }
        }
        if (raw==6)
        {
            if (((QTableWidgetItem *)tableCurves->item(6,col))->checkState()==Qt::Checked)
            {
                tableCurves->item(6,col)->setText("on");

                ((QComboBoxInTable*)tableCurves->cellWidget(6,col+1))->setMaximumWidth(((QComboBoxInTable*)tableCurves->cellWidget(0,col+1))->width());
                ((QComboBoxInTable*)tableCurves->cellWidget(6,col+1))->setMinimumWidth(((QComboBoxInTable*)tableCurves->cellWidget(0,col+1))->width());
                ((QComboBoxInTable*)tableCurves->cellWidget(6,col+1))->setMaximumWidth(10000);
                ((QComboBoxInTable*)tableCurves->cellWidget(6,col+1))->setMinimumWidth(0);
                tableCurves->item(6,col+1)->setText("");
            }
            else
            {
                tableCurves->item(6,col)->setText("off");

                ((QComboBoxInTable*)tableCurves->cellWidget(6,col+1))->setMinimumWidth(0);
                ((QComboBoxInTable*)tableCurves->cellWidget(6,col+1))->setMaximumWidth(0);
                tableCurves->item(6,col+1)->setText("... no polydispersity integration");
            }
        }
    }
    
    tableCurves->blockSignals(false);
}


//*******************************************
//  colList
//*******************************************
void fittable18::colList( QString tableName, int col)
{
    int i;
    QStringList lst,lstDI,lstSigma;
    QStringList lstDI0,lstSigma0;
    
    QList<MdiSubWindow *> windows = app()->windowsList();
    
    if (tableName!="")
    {
        lstDI0= app()->columnsList(Table::yErr);
        for(int i=0; i<lstDI0.count();i++)
        {
            if (lstDI0[i].left(lstDI0[i].lastIndexOf("_"))==tableName) lstDI<<lstDI0[i];
        }
        lstSigma0= app()->columnsList(Table::xErr);
        for(int i=0; i<lstSigma0.count();i++)
        {
            if (lstSigma0[i].left(lstSigma0[i].lastIndexOf("_"))==tableName) lstSigma<<lstSigma0[i];
        }
        /*
        //+++ 2020.04
        QRegExp rx(tableName);
        //rx.setWildcard( true );
        //---
    
        lstDI=lstDI.filter(rx);
        

        lstSigma=lstSigma.filter(rx);
         */
    }
    
    int Raws=0;
    foreach (MdiSubWindow *w, windows) {
        if (QString(w->metaObject()->className()) == "Table" && w->name()==tableName) {
            Table* table = (Table*)w;
            lst=table->columnsList();
            Raws=table->numRows();
        }
    }
    
    setSigmaAndWeightCols(lst,lstDI,lstSigma,col,Raws);
}

//*******************************************
//  QvsN
//*******************************************
void fittable18::QvsN()
{
    int Ntotal;
    double *Qtotal;
    double *Itotal;
    double *dItotal;
    double *Sigmatotal;
    
    if ( !SetQandI(Ntotal, Qtotal, Itotal, dItotal, Sigmatotal))
    {
        QMessageBox::warning(this,tr("QtiSAS"),
                             tr("Check data :: Problem to read data!"));
    }
}

//*******************************************
// +++  Set Q and I  +++
//*******************************************
bool fittable18::SetQandI(int &Ntotal, double*&Qtotal, double*&Itotal, double*&dItotal, double*&Sigmatotal)
{
    
    bool SANSsupport=checkBoxSANSsupport->isChecked();
    //
    size_t mm,j,ii, nReal, iistart;
    //
    double min, max;
    //
    bool weightYN, resoYN;
    //
    QString colReso;
    //
    int M=spinBoxNumberCurvesToFit->value();
    //
    size_t N=0;
    
    //
    Table *table;
    int xColIndex,yColIndex;
    
    for(mm=0;mm<M;mm++)
    {
        //
        //Table Name
        QComboBoxInTable *curve =(QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1);
        if ( curve->count()==0 ) return false;
        
        QString curveName=curve->currentText();
        QString tableName=curveName.left(curveName.lastIndexOf("_"));
        
        if ( findFitDataTable(curveName, table, xColIndex,yColIndex ) )
        {
            tableCurves->item(1,2*mm+1)->setText(QString::number(table->numRows()));
            N+=table->numRows();
            //
            nReal=0;
            for (ii=0; ii<table->numRows(); ii++) if ((table->text(ii,xColIndex))!="") nReal++;
            //
            if (nReal<=1)
            {
                return false;
            }
            
            if (((QComboBoxInTable*)tableCurves->cellWidget(1,2*mm))->currentText()=="N")
            {
                QTableWidgetItem *iQmin = (QTableWidgetItem*)tableCurves->item (2,2*mm);
                //
                if ( !iQmin->checkState()) tableCurves->item(2,2*mm+1)->setText("1");
                //
                QTableWidgetItem *iQmax = (QTableWidgetItem*)tableCurves->item (3,2*mm);
                if (!iQmax->checkState())	tableCurves->item(3,2*mm+1)->setText(QString::number(table->numRows()));
            }
            else
            {
                iistart=0;
                //
                while ((table->text(iistart,xColIndex))=="") iistart++;
                min=table->text(iistart,xColIndex).toDouble();
                max=min;
                //
                for (ii=iistart; ii<table->numRows(); ii++)
                {
                    if ((table->text(ii,xColIndex).toDouble())>max && table->text(ii,xColIndex)!="") max=table->text(ii,xColIndex).toDouble();
                    if ((table->text(ii, xColIndex).toDouble())<min && table->text(ii,xColIndex)!="") min=table->text(ii,xColIndex).toDouble();
                }
                //
                QTableWidgetItem *iQmin = (QTableWidgetItem*)tableCurves->item (2,2*mm);
                //
                if (!iQmin->checkState()) tableCurves->item(2,2*mm+1)->setText(QString::number(min));
                //
                QTableWidgetItem *iQmax = (QTableWidgetItem*)tableCurves->item (3,2*mm);
                //
                if (!iQmax->checkState())
                    tableCurves->item(3,2*mm+1)->setText(QString::number(max));
            }
        }
        else return false;
        
        if (!table)
        {
            return false;
        }
    }
    
    Ntotal=N;
    
    double *II=new double[N];
    double *QQ=new double[N];
    double *dII=new double[N];
    double *sigmaResoO=new double[N];
    
    
    
    size_t mnmn=0;
    
    for(mm=0; mm<M;mm++)
    {
        // I & Q
        QComboBoxInTable *curve =(QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1);
        QString curveName=curve->currentText();
        QString tableName=curveName.left(curveName.lastIndexOf("_"));
        // dI
        weightYN=false;
        QString colWeight="";
        QTableWidgetItem *it = (QTableWidgetItem*)tableCurves->item(4,2*mm);
        if (it->checkState())
        {
            weightYN=true;
            QComboBoxInTable *weight =(QComboBoxInTable*)tableCurves->cellWidget(4, 2*mm+1);
            colWeight=weight->currentText();
        }
        
        // Reso
        resoYN=false;
        
        if (SANSsupport)
        {
            colReso="";
            it = (QTableWidgetItem*)tableCurves->item(5,2*mm);
            if (it->checkState())
            {
                resoYN=true;
            }
            QComboBoxInTable *reso =(QComboBoxInTable*)tableCurves->cellWidget(5, 2*mm+1);
            colReso=reso->currentText();
        }
        //
        if ( findFitDataTable(curveName, table, xColIndex,yColIndex ) )
        {
            double wa=lineEditWA->text().toDouble(); wa=fabs(wa);
            double wb=lineEditWB->text().toDouble(); wb=fabs(wb); if (wb==0) wb=1.0;
            double wc=lineEditWC->text().toDouble(); wc=fabs(wc);
            double wxmax=lineEditWXMAX->text().toDouble();
            
            for(j=0;j<table->numRows();j++)
            {
                //Q
                if (table->text(j,xColIndex)=="")
                    QQ[mnmn]=-911119.119911;
                else
                    QQ[mnmn]=table->text(j,xColIndex).toDouble();
                //I
                if (table->text(j,yColIndex)=="")
                    II[mnmn]=-911119.119911;
                else
                    II[mnmn]=table->text(j,yColIndex).toDouble();		    //dI
                if (weightYN)
                {
                    if (comboBoxWeightingMethod->currentIndex()==0)//+++ [ w = 1/dY² ]
                    {
                        if (table->text(j,table->colIndex(colWeight))=="" || table->text(j,table->colIndex(colWeight)).toDouble()==0)
                            dII[mnmn]=-911119.119911;
                        else
                            dII[mnmn]=fabs(table->text(j,table->colIndex(colWeight)).toDouble());
                    }
                    else if (comboBoxWeightingMethod->currentIndex()==1) //+++ [ w = 1/Y  ]
                    {
                        if (II[mnmn]==-911119.119911)
                        {
                            QQ[mnmn]=-911119.119911;
                            dII[mnmn]=-911119.119911;
                        }
                        else
                            dII[mnmn]=sqrt(fabs(II[mnmn]));
                    }
                    else if (comboBoxWeightingMethod->currentIndex()==2)//+++ [ w = dY  ]
                    {
                        if (II[mnmn]==-911119.119911 || table->text(j,table->colIndex(colWeight)).toDouble()==0)
                            dII[mnmn]=-911119.119911;
                        else
                            dII[mnmn]=1/sqrt(fabs(table->text(j,table->colIndex(colWeight)).toDouble()));
                    }
                    else if (comboBoxWeightingMethod->currentIndex()==3) //+++ [ w = 1/Y² ]
                    {
                        if (II[mnmn]==-911119.119911)
                        {
                            QQ[mnmn]=-911119.119911;
                            dII[mnmn]=-911119.119911;
                        }
                        else
                            dII[mnmn]=fabs(II[mnmn]);
                    }
                    else if (comboBoxWeightingMethod->currentIndex()==4) //+++ [ w = 1/Y^a ]
                    {
                        if (II[mnmn]==-911119.119911)
                        {
                            QQ[mnmn]=-911119.119911;
                            dII[mnmn]=-911119.119911;
                        }
                        else
                            dII[mnmn]=pow( fabs(II[mnmn]),wa/2);
                    }
                    else if (comboBoxWeightingMethod->currentIndex()==5) //+++ [ w = 1/|c^a+b*Y^a| ]
                    {
                        if (II[mnmn]==-911119.119911)
                        {
                            QQ[mnmn]=-911119.119911;
                            dII[mnmn]=-911119.119911;
                        }
                        else
                            dII[mnmn]=sqrt(pow(wc,wa)+wb*pow( fabs(II[mnmn]),wa));
                    }
                    else if (comboBoxWeightingMethod->currentIndex()==6) // +++ [ w = 1/ Y^a / c^|Xmax-X| ) ]
                    {
                        if (II[mnmn]==-911119.119911)
                        {
                            QQ[mnmn]=-911119.119911;
                            dII[mnmn]=-911119.119911;
                        }
                        else
                            dII[mnmn]=sqrt(pow(wc,fabs(wxmax-QQ[mnmn]))*pow( fabs(II[mnmn]),wa));
                    }
                }
                else
                    dII[mnmn]=1;
                //Sigma
                if (resoYN)
                {
                    //-NEW
                    if ( colReso.contains("ASCII.1D.SANS")) sigmaResoO[mnmn]= 0.0 - fabs (app()->sigma(QQ[mnmn]) );
                    else if ( colReso.contains("20%")) sigmaResoO[mnmn]=QQ[mnmn]*0.20;
                    else if ( colReso.contains("10%")) sigmaResoO[mnmn]=QQ[mnmn]*0.10;
                    else if ( colReso.contains("05%")) sigmaResoO[mnmn]=QQ[mnmn]*0.05;
                    else if ( colReso.contains("02%")) sigmaResoO[mnmn]=QQ[mnmn]*0.02;
                    else if ( colReso.contains("01%")) sigmaResoO[mnmn]=QQ[mnmn]*0.01;
                    else if ( colReso=="from SPHERES") sigmaResoO[mnmn]=app()->sigma(QQ[mnmn]);	 // Change to SPHERES  function
                    else
                    {
                        sigmaResoO[mnmn]=table->text(j,table->colIndex(colReso)).toDouble();
                    }
                }
                else if (SANSsupport)
                {
                    //-NEW
                    if ( colReso.contains("ASCII.1D.SANS")) sigmaResoO[mnmn]=0.0 - fabs (app()->sigma(QQ[mnmn]) );
                    else if ( colReso.contains("20%")) sigmaResoO[mnmn]=QQ[mnmn]*0.20;
                    else if ( colReso.contains("10%")) sigmaResoO[mnmn]=QQ[mnmn]*0.10;
                    else if ( colReso.contains("05%")) sigmaResoO[mnmn]=QQ[mnmn]*0.05;
                    else if ( colReso.contains("02%")) sigmaResoO[mnmn]=QQ[mnmn]*0.02;
                    else if ( colReso.contains("01%")) sigmaResoO[mnmn]=QQ[mnmn]*0.01;
                    else if ( colReso=="from SPHERES") sigmaResoO[mnmn]=0.0 - fabs (app()->sigma(QQ[mnmn]) );	 // Change to SPHERES  function
                    else
                    {
                        sigmaResoO[mnmn]=0.0 - fabs ( table->text(j,table->colIndex(colReso)).toDouble());
                    }
                }
                else
                    sigmaResoO[mnmn]=0.0;
                
                mnmn++;
            }
        }
    }
    
    
    Qtotal=new double[N];
    Itotal=new double[N];
    dItotal=new double[N];
    Sigmatotal=new double[N];
    
    int prec=spinBoxSignDigits->value();
    
    for(int i=0; i<N; i++)
    {
        //if (QQ[i]==-911119.119911) Qtotal[i]=QQ[i]; else Qtotal[i] =  round2prec(QQ[i], prec);
        //if (II[i]==-911119.119911) Itotal[i]=II[i]; else Itotal[i] =  round2prec(II[i], prec);
        //if (dII[i]==-911119.119911) dItotal[i]=dII[i]; else dItotal[i] =  round2prec(dII[i], prec);
        //if (sigmaResoO[i]==-911119.119911) Sigmatotal[i]=sigmaResoO[i]; else Sigmatotal[i] =  round2prec(sigmaResoO[i], prec);
        Qtotal[i]=QQ[i];
        Itotal[i]=II[i];
        dItotal[i]=dII[i];
        Sigmatotal[i]=sigmaResoO[i];
    }
    
    delete[] QQ;
    delete[] II;
    delete[] dII;
    delete[] sigmaResoO;
    
    return true;
}


// +++
bool fittable18::datasetChangedSim( int num)
{
    //+++
    int p=spinBoxPara->value();
    int M=spinBoxNumberCurvesToFit->value();
    
    //+++ only real data
    if (num>=M) return false;
    
    int i,m;
    
    QStringList listTemp;
    bool refreshList=true;
    
    //+++ Parameters +++
    tableParaSimulate->blockSignals(true);
    for (i=0;i<p;i++) tableParaSimulate->setItem(i,0,new QTableWidgetItem(tablePara->item(i,2+3*num)->text()));
    tableParaSimulate->blockSignals(false);
    
    bool tablesExists=true;
    if ( ((QComboBoxInTable*)tableCurves->cellWidget(0,2*num+1))->currentText()=="" )  tablesExists=false;
    
    for (m=0;m<M;m++) if (tablesExists)
    {
        listTemp << ((QComboBoxInTable*)tableCurves->cellWidget(0,2*m+1))->currentText();
        if ( comboBoxDatasetSim->count()<M ) refreshList=false;
        else if (((QComboBoxInTable*)tableCurves->cellWidget(0,2*m+1))->currentText() != comboBoxDatasetSim->itemText(m) ) refreshList=false;
    }
    
    
    if ( !refreshList && tablesExists )
    {
        comboBoxDatasetSim->clear();
        if (listTemp.count()>0)
        {
            comboBoxDatasetSim->clear();
            comboBoxDatasetSim->addItems(listTemp);
            comboBoxDatasetSim->setCurrentIndex(num); // here  could be problem!!!
        }
        
    }
    
    //+++ weight combobox import
    QComboBoxInTable *weightItem = (QComboBoxInTable*)tableCurves->cellWidget(4,2*num+1);
    QStringList listW;
    listW.clear();
    comboBoxWeightSim->clear();
    
    if (weightItem->count()>0)
    {
        for (int j=0;j<weightItem->count();j++) listW<<weightItem->itemText(j);
        comboBoxWeightSim->addItems(listW);
        comboBoxWeightSim->setItemText(comboBoxWeightSim->currentIndex(),
                                       ((QComboBoxInTable*)tableCurves->cellWidget(4,2*num+1))->currentText());
    }
    
    //+++
    QTableWidgetItem* weightCh=(QTableWidgetItem* )tableCurves->item(4,2*num);
    if (weightCh->checkState()) checkBoxWeightSim->setChecked(true);
    else checkBoxWeightSim->setChecked(false);
    
    
    //+++
    QString tableName, curveName;
    
    QString currentInstrument=comboBoxInstrument->currentText();
    
    double min,max;
    int Ntot;
    
    if (tablesExists)
    {
        tableName=comboBoxDatasetSim->itemText(num).left(comboBoxDatasetSim->itemText(num).lastIndexOf("_"));
        curveName=comboBoxDatasetSim->itemText(num);
        
        //+++ source dataset +++
        Table *t;
        int xColIndex, yColIndex;
        
        if ( !findFitDataTable(curveName, t, xColIndex,yColIndex ) )
        {
            comboBoxResoSim->clear();
            
            if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("SANS") )
            {
                comboBoxResoSim->addItem("calculated in \"ASCII.1D.SANS\"");
                comboBoxResoSim->addItem("\"01%\":  sigma(Q)=0.01*Q");
                comboBoxResoSim->addItem("\"02%\":  sigma(Q)=0.02*Q");
                comboBoxResoSim->addItem("\"05%\":  sigma(Q)=0.05*Q");
                comboBoxResoSim->addItem("\"10%\":  sigma(Q)=0.10*Q");
                comboBoxResoSim->addItem("\"20%\":  sigma(Q)=0.20*Q");
                
                QComboBoxInTable *polyItem = (QComboBoxInTable *) tableCurves->cellWidget(6,2*num+1);
                QStringList list;
                list.clear();
                comboBoxPolySim->clear();
                if (polyItem->count()>0)
                {
                    for (int j=0;j<polyItem->count();j++) list<< polyItem->itemText(j);
                    comboBoxPolySim->addItems(list);
                    comboBoxPolySim->setItemText(comboBoxPolySim->currentIndex(),
                                                 polyItem->currentText());
                }
            }
            if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("Back") )
            {
                comboBoxResoSim->addItem("from SPHERES");
                
            }
            
            return false;
        }
        else
        {
            comboBoxResoSim->clear();

            if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("SANS") )
            {
                comboBoxResoSim->addItem("calculated in \"ASCII.1D.SANS\"");
                comboBoxResoSim->addItem("\"01%\":  sigma(Q)=0.01*Q");
                comboBoxResoSim->addItem("\"02%\":  sigma(Q)=0.02*Q");
                comboBoxResoSim->addItem("\"05%\":  sigma(Q)=0.05*Q");
                comboBoxResoSim->addItem("\"10%\":  sigma(Q)=0.10*Q");
                comboBoxResoSim->addItem("\"20%\":  sigma(Q)=0.20*Q");
                
                QComboBoxInTable *polyItem = (QComboBoxInTable *) tableCurves->cellWidget(6,2*num+1);
                QStringList list;
                list.clear();
                comboBoxPolySim->clear();
                if (polyItem->count()>0)
                {
                    for (int j=0;j<polyItem->count();j++) list<< polyItem->itemText(j);
                    comboBoxPolySim->addItems(list);
                    comboBoxPolySim->setItemText(comboBoxPolySim->currentIndex(),
                                                 polyItem->currentText());
                }
            }
            if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("Back") )
            {
                comboBoxResoSim->addItem("from SPHERES");
                
            }
        }
        
        int N=t->numRows();
        int ii=0;
        while(t->text(ii,xColIndex) == "" && ii<N) ii++;
        
        // +++
        Ntot=0;
        QRegExp rx( "((\\-|\\+)?\\d*(\\.|\\,)\\d*((e|E)(\\-|\\+)\\d*)?)|((\\-|\\+)?\\d+)" );
        for(int j=0;j<N;j++) if (rx.exactMatch(t->text(j,xColIndex))) Ntot++;
        if (Ntot<2) Ntot=1000;
        
        if (radioButtonSameQrange->isChecked()) lineEditNumPointsSim->setText(QString::number(Ntot));
        
        if (ii==N)
        {
            return false;
            // min=0.001; max=0.1;
        }
        else
        {
            min=t->text(ii,xColIndex).toDouble();
            max=t->text(ii,xColIndex).toDouble();
            
            for (int j=ii;j<N;j++)
            {
                if ((t->text(j,xColIndex).toDouble())>max && t->text(j,xColIndex)!="") max=t->text(j,xColIndex).toDouble();
                if (t->text(j,xColIndex).toDouble()<min && t->text(j,xColIndex)!="") min=t->text(j,xColIndex).toDouble();
            }
            
        }
        if (radioButtonSameQrange->isChecked()) lineEditFromQsim->setText(QString::number(min));
        if (radioButtonSameQrange->isChecked()) lineEditToQsim->setText(QString::number(max));
        
        QRegExp rxCol(tableName+"_*");
        rxCol.setPatternSyntax(QRegExp::Wildcard);
        QStringList cols;
        
        QStringList colTemp=app()->columnsList(Table::xErr);
        for (int j=0; j<colTemp.count();j++)
        {
            if (rxCol.exactMatch(colTemp[j])) cols<<colTemp[j];
        }
        
        if (checkBoxSANSsupport->isChecked())
        {
            // +++ Reso & Poly
            QComboBoxInTable *resoItem = (QComboBoxInTable*) tableCurves->cellWidget(5,2*num+1);
            QStringList list;
            
            
            for (int j=0;j<resoItem->count();j++) list<< resoItem->itemText(j);
            
            comboBoxResoSim->clear();
            comboBoxResoSim->addItems(list);
            comboBoxResoSim->setItemText(comboBoxResoSim->currentIndex(),
                                         resoItem->currentText());
            
            QTableWidgetItem* resoCh=(QTableWidgetItem* )tableCurves->item(5,2*num);
            if (resoCh->checkState()) checkBoxResoSim->setChecked(true);
            else checkBoxResoSim->setChecked(false);
            
            if (currentInstrument.contains("SANS") )
            {
                QComboBoxInTable *polyItem = (QComboBoxInTable*) tableCurves->cellWidget(6,2*num+1);
                list.clear();
                for (int j=0;j<polyItem->count();j++) list<< polyItem->itemText(j);
                comboBoxPolySim->clear();
                comboBoxPolySim->addItems(list);
                
                comboBoxPolySim->setItemText(comboBoxPolySim->currentIndex(),
                                             polyItem->currentText());
                
                QTableWidgetItem* polyCh=(QTableWidgetItem* )tableCurves->item(6,2*num);
                if (polyCh->checkState()) checkBoxPolySim->setChecked(true);
                else checkBoxPolySim->setChecked(false);
            }
        }
    }
    else
    {
        if (checkBoxSANSsupport->isChecked())
        {
            comboBoxResoSim->clear();
            
            if (currentInstrument.contains("SANS") )
            {
                comboBoxResoSim->addItem("calculated in \"ASCII.1D.SANS\"");
                comboBoxResoSim->addItem("\"01%\":  sigma(Q)=0.01*Q");
                comboBoxResoSim->addItem("\"02%\":  sigma(Q)=0.02*Q");
                comboBoxResoSim->addItem("\"05%\":  sigma(Q)=0.05*Q");
                comboBoxResoSim->addItem("\"10%\":  sigma(Q)=0.10*Q");
                comboBoxResoSim->addItem("\"20%\":  sigma(Q)=0.20*Q");
                
                QComboBoxInTable *polyItem = (QComboBoxInTable*) tableCurves->cellWidget(6,2*num+1);
                QStringList list;
                list.clear();
                for (int j=0;j<polyItem->count();j++) list<< polyItem->itemText(j);
                comboBoxPolySim->clear();comboBoxPolySim->clear();
                comboBoxPolySim->addItems(list);
                comboBoxPolySim->setItemText(comboBoxPolySim->currentIndex(),
                                             polyItem->currentText());
            }
            
            if (currentInstrument.contains("Back") )
            {
                
                comboBoxResoSim->addItem("from SPHRES");
            }
        }
    }
    
    if (tablesExists)
    {
        // Range transfer
        QComboBoxInTable *QN = (QComboBoxInTable*) tableCurves->cellWidget(1,2*num);
        comboBoxSimQN->setCurrentIndex(QN->currentIndex());
        if(comboBoxSimQN->currentText()=="N")
        {
            textLabelRangeFirstLimit->setText(QString::number(GSL_MIN(1,Ntot)));
            textLabelRangeLastLimit->setText(QString::number(Ntot));
        }
        else
        {
            textLabelRangeFirstLimit->setText(QString::number(GSL_MIN(min,max)));
            textLabelRangeLastLimit->setText(QString::number(max));
        }
        //	textLabelDofSim->setText(QString::number(Ntot)); // 2016
        
        
        textLabelRangeFirst->setText(tableCurves->item(2,2*num+1)->text());
        textLabelRangeLast->setText(tableCurves->item(3,2*num+1)->text());
        
        //    textLabelRangeFirstLimit->setText(QString::number(1));
        //    textLabelRangeLastLimit->setText(QString::number(Ntot));
    }
    return true;
}

//*******************************************
//*** findFitDataTable
//*******************************************
bool fittable18::findFitDataTable(QString curveName, Table* &table, int &xColIndex, int &yColIndex )
{
    int i, ixy;
    bool exist=false;
    
    QString tableName=curveName.left(curveName.lastIndexOf("_"));
    
    QString colName=curveName.remove(tableName+"_");
    
    
    QList<MdiSubWindow *> windows = app()->windowsList();
    
    foreach (MdiSubWindow *w, windows) {
        if (QString(w->metaObject()->className()) == "Table" && w->name()==tableName) {
            table=(Table*)w;
            yColIndex=table->colIndex(colName);
            xColIndex=0;
            
            bool xSearch=true;
            ixy=yColIndex-1;
            while(xSearch && ixy>0 )
            {
                if (table->colPlotDesignation(ixy)==1)
                {
                    xColIndex=ixy;
                    xSearch=false;
                }
                else ixy--;
            }
            exist=true;
        }
    }

    return exist;
}

//*******************************************
//*** findFitDataTable
//*******************************************
bool fittable18::findFitDataTableDirect(QString curveName, Table* &table, int &xColIndex, int &yColIndex )
{
    int i, ixy;
    
    QString tableName=curveName.left(curveName.lastIndexOf("_"));
    QString colName=curveName.remove(tableName+"_");
    
    
    
    yColIndex=table->colIndex(colName);
    xColIndex=0;
    
    bool xSearch=true;
    ixy=yColIndex-1;
    while(xSearch && ixy>0 )
    {
        if (table->colPlotDesignation(ixy)==1)
        {
            xColIndex=ixy;
            xSearch=false;
        }
        else ixy--;
    }
    
    
    return true;
}

//*******************************************
//*Set-Sigma-and-Weight-Columns
//*******************************************
void fittable18::setSigmaAndWeightCols(QStringList lst,QStringList lstDI, QStringList lstSigma, int col,int Raws)
{
    //sen number of Raws
    tableCurves->item(1,col)->setText(QString::number(Raws));
    //
    QTableWidgetItem *cbImin = (QTableWidgetItem*)tableCurves->item (2,col-1);
    QTableWidgetItem *cbImax = (QTableWidgetItem*)tableCurves->item (3,col-1);
    
    int temp=tableCurves->item(2,col)->text().toInt();
    if ( (!cbImin->checkState()) || ( ( ((QComboBoxInTable*)tableCurves->cellWidget(1,col-1)))->currentText()=="N") && ( (temp<=0) || (temp >= Raws)))  tableCurves->item(2,col)->setText(QString::number(1));
    
    temp=tableCurves->item(3,col)->text().toInt();
    if ( (!cbImax->checkState()) || ( (((QComboBoxInTable*)tableCurves->cellWidget(1,col-1))->currentText()=="N") &&((temp<=2) || (temp > Raws) || (temp<tableCurves->item(2,col)->text().toInt()))))  tableCurves->item(3,col)->setText(QString::number(Raws));
    
    //weight
    QComboBoxInTable *weight =(QComboBoxInTable*)tableCurves->cellWidget(4,col);

    weight->blockSignals(true);
    weight->clear();
    weight->addItems(lstDI);
    weight->blockSignals(false);

    //reso
    QString currentInstrument=comboBoxInstrument->currentText();
    if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("SANS") )
    {
        QComboBoxInTable *reso =(QComboBoxInTable*)tableCurves->cellWidget(5,col);

        reso->blockSignals(true);
        
        //-NEW-
        lstSigma<<"calculated in \"ASCII.1D.SANS\"";
        lstSigma<<"\"01%\":  sigma(Q)=0.01*Q";
        lstSigma<<"\"02%\":  sigma(Q)=0.02*Q";
        lstSigma<<"\"05%\":  sigma(Q)=0.05*Q";
        lstSigma<<"\"10%\":  sigma(Q)=0.10*Q";
        lstSigma<<"\"20%\":  sigma(Q)=0.20*Q";

        reso->clear();
        reso->addItems(lstSigma);
        reso->blockSignals(false);
    }
    else if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("Back") )
    {
        QComboBoxInTable *reso =(QComboBoxInTable*)tableCurves->cellWidget(5,col);
        reso->blockSignals(true);
        lstSigma<<"from SPHERES";
        reso->clear();
        reso->addItems(lstSigma);
        reso->blockSignals(false);
    }
}

//*******************************************
//* vertHeaderTableCurves
//*******************************************
void fittable18::vertHeaderTableCurves(int raw)
{
    int M=spinBoxNumberCurvesToFit->value();
    
    if (raw==0)
    {
        updateDatasets();
    }
    if (raw>1)
    {
        
        QTableWidgetItem *itS0 = (QTableWidgetItem *)tableCurves->item(raw,0); //
        if (itS0->checkState() ) itS0->setCheckState(Qt::Unchecked);
        else itS0->setCheckState(Qt::Checked);
        
        for(int mm=1;mm<M;mm++)
        {
            QTableWidgetItem *itS = (QTableWidgetItem *) tableCurves->item(raw,2*mm);
            if (itS0->checkState()) itS->setCheckState(Qt::Checked);
            else itS->setCheckState(Qt::Unchecked);
        }
    }
}

//*******************************************
//* horizHeaderCurves
//*******************************************
void fittable18::horizHeaderCurves( int col )
{
    tableCurves->blockSignals(true);
    
    tableCurves->resizeColumnToContents(col);
    
    if (int( col/2)*2!=col)
    {
        Graph *g;
        
        if(!findActiveGraph(g)){QMessageBox::critical(this,tr("QtiSAS"), tr("Activate first GRAPH with data to fit !!!")); return;};
        
        if (g->curveCount()==0) {QMessageBox::critical(this,tr("QtiSAS"), tr("Graph is EMPTY !!!")); return;};
        

        QString curveName;
        double startID;
        double endID;
        bool selectedRange=true;

        if (g->rangeSelectorsEnabled())
        {
            // returns the curve range information as a string: "curve_name [start:end]"
            QString rangeInfo=g->curveRange(g->rangeSelectorTool()->selectedCurve());
            QStringList lst = rangeInfo.split(" ", Qt::SkipEmptyParts);
            curveName=lst[0];
            
            startID=g->selectedXStartValue();
            endID=g->selectedXEndValue();


        }
        else
        {
            QwtPlotItem *sp = (QwtPlotItem *)g->plotItem(0);
            if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
                {QMessageBox::critical(this,tr("QtiSAS"), tr("First curve is Spectrogram !!!")); return;};
                
            selectedRange=false;
            
            // returns the curve range information as a string: "curve_name [start:end]"
            QString rangeInfo=g->curveRange((QwtPlotCurve *)g->curve(0));
            QStringList lst = rangeInfo.split(" ", Qt::SkipEmptyParts);
            curveName=lst[0];
            rangeInfo=lst[1].remove("[").remove("]");
            lst.clear();
            lst = rangeInfo.split(":", Qt::SkipEmptyParts);
            startID=lst[0].toInt();
            endID=lst[1].toInt();
        }

        
        if (startID>endID)
        {
            double tmp=endID;
            endID=startID;
            startID=tmp;
        }
        
        if (startID==endID) {QMessageBox::critical(this,tr("QtiSAS"), tr("min=max !!!")); return; };
        
        
        tableCurves->item(0,col-1)->setText(curveName);
        tableCurvechanged(0,col-1); tableCurvechanged(0,col);
        tableCurves->item(0,col-1)->setText("");
        
        
        if (selectedRange)
        {
            QComboBox *iQ =(QComboBox*)tableCurves->cellWidget(1,col-1);
            iQ->setCurrentIndex(1); tableCurvechanged(1,col-1);
            
            //+++
            QTableWidgetItem *cbImin = (QTableWidgetItem*)tableCurves->item(2,col-1);
            cbImin->setCheckState(Qt::Checked);
            tableCurves->item(2,col)->setText(QString::number(startID,'g',10));
            
            //+++
            QTableWidgetItem *cbImax = (QTableWidgetItem*)tableCurves->item(3,col-1);
            cbImax->setCheckState(Qt::Checked);
            tableCurves->item(3,col)->setText(QString::number(endID,'g',10));
        }
        else
        {
            
            QComboBox *iQ =(QComboBox*)tableCurves->cellWidget(1,col-1);
            iQ->setCurrentIndex(0); tableCurvechanged(1,col-1);
            
            //+++
            QTableWidgetItem *cbImin = (QTableWidgetItem*)tableCurves->item(2,col-1);
            cbImin->setCheckState(Qt::Checked);
            tableCurves->item(2,col)->setText(QString::number(startID));
            
            //+++
            QTableWidgetItem *cbImax = (QTableWidgetItem*)tableCurves->item(3,col-1);
            cbImax->setCheckState(Qt::Checked);
            tableCurves->item(3,col)->setText(QString::number(endID));
        }
        
    }
    
         tableCurves->blockSignals(false);
}

//*******************************************
//* changedSetToSet
//*******************************************
void fittable18::changedSetToSet( int raw, int col )
{
    if (raw==0 && col==1)
    {
        bool use;
        QTableWidgetItem *uYN = (QTableWidgetItem*)tableMultiFit->item (0,1);
        if (uYN->checkState()) use=true; else use=false;

        for (int i=1;i<tableMultiFit->rowCount();i++)
        {
            QTableWidgetItem *setYN = (QTableWidgetItem*)tableMultiFit->item (i,0);
            

            if (use) setYN->setCheckState(Qt::Checked); else setYN->setCheckState(Qt::Unchecked);
        }
    }
}

//*******************************************
//+++ check of Para table to global parameters
//*******************************************
void fittable18::checkGlobalParameters(int raw, int col)
{
    if (tablePara->focusWidget()==0 && col>=0) return;
    if(raw!=tablePara->currentRow() && col!=tablePara->currentColumn() && col>=0) return;
    
    col=fabs(col);
    
    tablePara->blockSignals(true);
    
    int M=spinBoxNumberCurvesToFit->value();
    int p=spinBoxPara->value();

    if (M>1)
    {
        
        //+++ Parameters && Sharing && Varying
        
        for (int pp=0; pp<p;pp++)
        {
            QTableWidgetItem *itS = (QTableWidgetItem*)tablePara->item(pp,0); // Share?
            
            for (int mm=1; mm<M;mm++)
            {
                QTableWidgetItem *itA = (QTableWidgetItem*)tablePara->item(pp,3*mm+1); // Vary?
                
                if (itS->checkState())
                {
                    itA->setCheckState(Qt::Unchecked);
                    tablePara->item(pp,3*mm+2)->setText(tablePara->item(pp,2)->text());
                    tablePara->item(pp,3*mm+3)->setText("---");
                }
            }
            
        }
    }
    QString sss;
    if  (col>0 && ((  (int) ( (col-1)/3) )*3)!=(col-1) )
    {
        sss=tablePara->item(raw,col)->text();
        sss=sss.replace(",", ".");
        //+++ 2016 test	tablePara->setText(raw, col, QString::number(sss.toDouble(),'E',spinBoxSignDigits->value()-1));
        tablePara->item(raw, col)->setText(QString::number(sss.toDouble(),'G',spinBoxSignDigits->value()));
    }
    if  (    (int)((col+1)/3)*3 == col+1 && checkBoxAutoRecalculate->isChecked()) headerPressedTablePara(col);
    
    if  (    (int)((col+2)/3)*3 == col+2 )
    {
        QString str=tablePara->item(raw, col)->text();
        QStringList lst;
        
        lst = str.split("..");
        if (lst.count()!=2)tablePara->item(raw, col)->setText("..");

    }
    
    tablePara->blockSignals(false);
    
    if  ( (int)((col+1)/3)*3 == col+1 && checkBoxAutoRecalculate->isChecked())  app()->fittableWidget->tablePara->setFocus();
}

//*******************************************
//+++ Weight/Reso/Poly selector in interface
//*******************************************
void fittable18::optionSelected()
{
    if (!checkBoxSANSsupport->isChecked())
    {
        stackedWidgetOptions->setCurrentIndex(0);
        groupBoxDpSlelection->hide();
        frameResoOptions->hide();
        framePolyOptions->hide();
        return;
    }
    else
    {
        groupBoxDpSlelection->show();
        frameResoOptions->show();
        framePolyOptions->show();
    }
    if (radioButtonSelectWeight->isChecked()) stackedWidgetOptions->setCurrentIndex(0);
    else if (radioButtonSelectReso->isChecked()) stackedWidgetOptions->setCurrentIndex(1);
    else stackedWidgetOptions->setCurrentIndex(2);
}
