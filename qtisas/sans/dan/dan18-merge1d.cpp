/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS ascii merging tools
 ******************************************************************************/

#include "dan18.h"
#include "Folder.h"

void dan18::mergeSlots()
{
    connect( spinBoxMmergingCond, SIGNAL( valueChanged(int) ), this, SLOT( mergingTableChange() ) );
    connect( spinBoxNtoMerge, SIGNAL( valueChanged(int) ), this, SLOT( mergingTableChange() ) );
    connect( pushButtonMerge, SIGNAL( clicked() ), this, SLOT( mergeProject() ) );
    connect( pushButtonMergeAscii, SIGNAL( clicked() ), this, SLOT( mergeAscii() ) );
    connect( pushButtonDefineMergeFromTable, SIGNAL( clicked() ), this, SLOT( readMergeInfo() ) );
    connect( pushButtonSaveMergeDataToTable, SIGNAL( clicked() ), this, SLOT( saveMergeInfo() ) );
    //+++2022
    connect( pushButtonMergeTOF, SIGNAL( clicked() ), this, SLOT( mergeProjectTOF() ) );
    connect( pushButtonMergeTOFAscii, SIGNAL( clicked() ), this, SLOT( mergeAsciiTOF() ) );
    //+++ 2022
    connect( radioButtonQrangeTofManual, SIGNAL( toggled(bool) ), spinQminTof, SLOT( setVisible(bool) ) );
    connect( radioButtonQrangeTofManual, SIGNAL( toggled(bool) ), spinQmaxTof, SLOT( setVisible(bool) ) );
    
    connect( checkBoxMergeFromTable, SIGNAL( toggled(bool) ), this, SLOT( mergeFromIntegratedTable(bool) ) );
}

//++++ mergeFromIntegratedTable config +++++++++++++++
void dan18::mergeFromIntegratedTable(bool YN)
{
    if (YN)
    {
        checkBoxMergeFromTable->setMinimumHeight(35);
        checkBoxMergeFromTable->setMaximumHeight(16777215);
        verticalSpacerMerge->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Fixed);
        
    }
    else
    {
        checkBoxMergeFromTable->setMinimumHeight(25);
        checkBoxMergeFromTable->setMaximumHeight(25);
        verticalSpacerMerge->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Expanding);
    }
    layout()->invalidate();
}

// +++ mergingTableChange() +++
void dan18::mergingTableChange()
{
    //--- old numbers
    int oldN=tableMerge->columnCount()-1;
    int oldM=tableMerge->rowCount();
    //--- new numbers
    int N=spinBoxNtoMerge->value();
    int M=spinBoxMmergingCond->value();
    //---
    spinBoxRefForMerging->setMaximum(N);
    spinBoxRefForMerging->setMinimum(1);
    //--- not changed (no sence in principal)
    if (N==oldN && M==oldM) return;
    
    // +++ find tables +++
    QList<MdiSubWindow *> windows = app()->windowsList();
    //+++ filter for tables
    QRegExp rx(lineEditFilter->text());
    rx.setPatternSyntax(QRegExp::Wildcard);
    //+++ list of filtered tables
    QStringList tableNames;
    int tables=0;
    //+++
    foreach (MdiSubWindow *w, windows) {
        if (QString(w->metaObject()->className()) == "Table" && rx.exactMatch(w->name())) {
            tableNames<<w->name();
            tables++;
        }
    }
    tableNames.sort();
    //--- prepended select line
    tableNames.prepend("select"); tables++;
    
    // --- find tables ---
    tableMerge->setColumnCount(1);  tableMerge->setColumnCount(1+N);
    tableMerge->setRowCount(0); tableMerge->setRowCount(M);
    
    int cc;
    QStringList colsNames;
    for (cc=0;cc<N;cc++) colsNames<<"Q-Range-"+QString::number(cc+1);
    colsNames.prepend("New Table Name");
    
    //+++ Set Data-Sets List +++
    for(int mm=0; mm<M;mm++)
    {
        for(int nn=0; nn<N;nn++)
        {
            QComboBoxInTable *iTable = new QComboBoxInTable(mm,nn+1,tableMerge);
            iTable->addItems(tableNames);
            tableMerge->setCellWidget(mm,nn+1,iTable);
        }
    
        tableMerge->setItem(mm, 0, new QTableWidgetItem);
        tableMerge->item(mm, 0)->setText("");
    }
    
    tableMerge->setHorizontalHeaderLabels(colsNames);
    tableMerge->setColumnWidth(0,115);
    
}

//+++++++++++++++++++++++
bool dan18::checkTableExistance(QString tName, int &Rows, int &Cols, double &Qmin, double &Qmax )
{
    //+++ List
    QList<MdiSubWindow *> windows = app()->windowsList();
    //+++
    foreach (MdiSubWindow *w, windows)
    {
        if (w->name()==tName && QString(w->metaObject()->className()) == "Table")
        {
            Table *t=(Table*)w;
            Cols=t->numCols();
            Rows=t->numRows();
            Qmin=t->text(0,0).toDouble();
            Qmax=Qmin;
            for (int j=1; j<Rows;j++)
            {
                if ( t->text(j,0).toDouble() < Qmin ) Qmin=t->text(j,0).toDouble();
                if ( t->text(j,0).toDouble() > Qmax ) Qmax=t->text(j,0).toDouble();
            }
            if (Cols<2 || Rows==0 || Qmin==Qmax) return false;
            else return true;
        }
    }
    return false;
}

//+++++++++++++++++++++++
bool dan18::addTable(const QString table, gsl_matrix* &data, int &N, int Rows, int overlap, int &firstPosition)
{
    //---
    QList<MdiSubWindow *> windows = app()->windowsList();
    Table *t;

    //--- find table
    foreach (MdiSubWindow *w, windows)
    {
        if (w->name()==table && QString(w->metaObject()->className()) == "Table") t=(Table *)w;
    }
    //--- cross check
    if (t->numCols()<2 || t->numRows()==0) return false;
    
    int pos;
    double Q,I,dI,sigma;
    
    //--- number filter
    QRegExp rx("((\\-|\\+)?(\\d*)?(\\.|\\,)?\\d*((e|E)(\\-|\\+)\\d*)?)");
    
    int NN=N-1;
    bool first=true;
    if (NN>0) first=false;
    
    double Qmax,Qmin,middle,diff;
    
    if (!first && overlap<100)
    {
        Qmax=gsl_matrix_get(data,N-1,0);
        Qmin=t->text(0,0).toDouble();
        middle=0.5*(Qmax+Qmin);
        diff=fabs(Qmax-Qmin);
        
        while ( NN>0 && gsl_matrix_get(data,NN,0) > middle+diff/2*overlap/100 ) { gsl_matrix_set(data,NN,0,-99.99); NN--;};
    }
    
    //++++
    firstPosition=N;
    
    //+++
    for (int i=0;i<Rows;i++)
    {
        pos=0;
        if ( rx.indexIn(t->text(i,0),pos)>-1  && (first || overlap==100 || t->text(i,0).toDouble() > (middle-diff/2*overlap/100) ) ) Q=rx.cap( 1 ).toDouble(); else Q=-99.99;
        pos=0;
        if ( rx.indexIn(t->text(i,1),pos)>-1 ) I=rx.cap( 1 ).toDouble(); else I=-99.99;
        pos=0;
        if ( t->numCols() > 2 && rx.indexIn(t->text(i,2),pos)>-1 ) dI=rx.cap( 1 ).toDouble(); else dI=-99.99;
        pos=0;
        if ( t->numCols() > 3 && rx.indexIn(t->text(i,3),pos)>-1 ) sigma=rx.cap( 1 ).toDouble(); else sigma=-99.99;
        
        if (Q!=-99.99 && I!=-99.99)
        {
            gsl_matrix_set(data,N,0,Q);
            gsl_matrix_set(data,N,1,I);
            gsl_matrix_set(data,N,2,dI);
            gsl_matrix_set(data,N,3,sigma);
            N++;
        }
    }
    
    return true;
}

//+++
void dan18::saveMergedMatrix(QString name, QString labelList,gsl_matrix* data, int N, bool loadReso, bool loadQerr, bool tofYN, bool asciiYN)
{
    if (asciiYN)
        return saveMergedMatrixAscii(name, data, N, loadReso, loadQerr, tofYN);
    
    QString colLabel = name;
    
    name = name.remove("=").replace(".", "_").remove(" ");
    name = name.replace(QRegExp(QString::fromUtf8("[`~!@#$%^&*()_â€”+=|:;<>Â«Â»,.?/{}\'\"\\\[\\\]\\\\]")), "-");

    if (checkBoxMergeIndexing->isChecked())
    {
        name += "-v-";
        name = app()->generateUniqueName(name);
    }

    QString label = "Merged Tables >> " + labelList;
    
    bool newTableOutput = false;

    //+++ create table
    Table* t;

    bool tableExist = app()->checkTableExistence(name, t);

    if (checkBoxMergeIndexing->isChecked() || !tableExist)
    {
        if (loadReso && loadQerr)
        {
            t = app()->newHiddenTable(name, label, N, 5);
            t->setColName(3, "dQ");
            t->setColPlotDesignation(3, Table::xErr);
            t->setColNumericFormat(2, 8, 3, true);
            t->setColName(4, "Sigma");
            t->setColPlotDesignation(4, Table::xErr);
            t->setColNumericFormat(2, 8, 3, true);
        }
        else if (loadReso)
        {
            t = app()->newHiddenTable(name, label, N, 4);
            t->setColName(3, "Sigma");
            t->setColPlotDesignation(3, Table::xErr);
            t->setColNumericFormat(2, 8, 3, true);
        }
        else if (loadQerr)
        {
            t = app()->newHiddenTable(name, label, N, 4);
            t->setColName(3, "dQ");
            t->setColPlotDesignation(3, Table::xErr);
            t->setColNumericFormat(2, 8, 3, true);
        }
        else
        {
            t = app()->newHiddenTable(name, label, N, 3);
            t->setColNumericFormat(2, 8, 3, true);
        }
    }
    else
    {
        t->blockSignals(true);
        if (t->numRows() != N)
            t->setNumRows(N);
        if (loadReso && loadQerr && t->numCols() < 5)
            t->setNumCols(5);
        else if ((loadQerr || loadReso) && t->numCols() < 4)
            t->setNumCols(4);
        else if (t->numCols() < 3)
            t->setNumCols(3);
    }
    
    t->setColName(0, "Q");
    t->setColName(1, "I");
    t->setColComment(1, colLabel);
    t->setColName(2, "dI");
    
    t->setColNumericFormat(2, 8, 0, true);
    t->setColNumericFormat(2, 8, 1, true);
    t->setColNumericFormat(2, 8, 2, true);
    
    t->setColPlotDesignation(0,Table::X);
    t->setColPlotDesignation(1,Table::Y);
    t->setColPlotDesignation(2,Table::yErr);
    
    t->setHeaderColType();
    
    app()->updateWindowLists(t);
    
    int currentRow=0;
    
    for (int i=0; i<N;i++)  if (gsl_matrix_get(data, i,0) != -99.99)
    {
        t->setText(currentRow,0, QString::number(gsl_matrix_get(data,i,0),'E'));
        //
        t->setText(currentRow,1, QString::number(gsl_matrix_get(data,i,1),'E'));
        //
        if (gsl_matrix_get(data, i,2) == -99.99) t->setText(currentRow,2, "---");
        else t->setText(currentRow,2, QString::number(gsl_matrix_get(data,i,2),'E'));
        //
        
        if (!tofYN)
        {
            if (loadReso)
            {
                if (gsl_matrix_get(data, i,3) == -99.99) t->setText(currentRow,3, "---");
                else t->setText(currentRow,3, QString::number(gsl_matrix_get(data,i,3),'E'));
            }
        }
        else
        {
            if (loadReso && !loadQerr)
            {
                t->setText(currentRow,3, QString::number(gsl_matrix_get(data,i,4),'E'));
            }
            if (loadReso && loadQerr)
            {
                t->setText(currentRow,4, QString::number(gsl_matrix_get(data,i,4),'E'));
            }
            
            if (loadQerr)
            {
                t->setText(currentRow,3, QString::number(gsl_matrix_get(data,i,3),'E'));
            }
        }
        
        currentRow++;
    };
    
    t->setNumRows(currentRow);

    t->blockSignals(false);

    //+++ adjust cols
    if (!tableExist)
        t->adjustColumnsWidth(false);
    
    t->notifyChanges();
    app()->modifiedProject(t);
    app()->showFullRangeAllPlots(name);
}

void dan18::saveMergedMatrixAscii(QString name, gsl_matrix* data, int N, bool loadReso, bool loadQerr, bool tofYN)
{
    name="QI-MERGED-"+name;

    QString path=lineEditPathRAD->text()+"/ASCII-QI-MERGED/";
    if (tofYN) path=lineEditPathRAD->text()+"/ASCII-QI-MERGED-TOF/";
    
    if (checkBoxMergeIndexing->isChecked())
    {
        name+="-v-";
        
        int fff=1;
        bool fileExist=true;


        while (fileExist)
        {
            if ( QFile::exists(path+name+QString::number(fff)+".DAT") ) fff++;
            else fileExist=false;
        }
        name+=QString::number(fff);
    }
    
    name=name+".DAT";
    
    QFile f( path+name);
    
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        QMessageBox::warning(this,tr("QtiSAS"),
                             tr("<h4>... could not write file ...</h4>"));
        return;
    }
    QTextStream stream( &f );
        
    for (int i=0; i<N;i++)  if (gsl_matrix_get(data, i,0) != -99.99)
    {
            stream<<QString::number(gsl_matrix_get(data,i,0),'E');
            //
            stream<<"   "+QString::number(gsl_matrix_get(data,i,1),'E');
            //
            if (gsl_matrix_get(data, i,2) == -99.99) stream<<"   ---";
            else stream<<"   "+QString::number(gsl_matrix_get(data,i,2),'E');

        
        
        
        if (!tofYN)
        {
            if (loadReso)
            {
                if (gsl_matrix_get(data, i,3) == -99.99) stream<<"   ---";
                else stream<<"   "+QString::number(gsl_matrix_get(data,i,3),'E');
            }
        }
        else
        {
            if (loadQerr)
            {
                stream<<"   "+QString::number(gsl_matrix_get(data,i,3),'E');
            }
            
            if (loadReso)
            {
                stream<<"   "+QString::number(gsl_matrix_get(data,i,4),'E');
            }
        }

        
            stream<<"\n";
        }
        f.close();

}


//+++++++++++++++++++++++
void dan18::mergeMethod(bool asciiYN)
{
    //+++ current folder
    Folder *cf = ( Folder * ) app()->current_folder;
    
    if (!asciiYN) app()->changeFolder("DAN :: Merge.1D");
    
    if (asciiYN)
    {
        QString path=lineEditPathRAD->text();
        QDir dd(path);
        if (!dd.cd("./ASCII-QI-MERGED/")) dd.mkdir("./ASCII-QI-MERGED/");
    }
    
    gsl_set_error_handler_off();
    int M=spinBoxNtoMerge->value();
    int N=spinBoxMmergingCond->value();
    
    if (M<2 || N<1)
    {
        QMessageBox::warning(this,tr("QtiSAS"),
                             tr("<h4>There are nothing to do...</h4>"));
        return;
    }
    
    QString nameMerged;
    
    int NN;
    int Cols, Rows;
    double  Qmin, Qmax;
    QString tName;
    
    QList<double> QminList;
    QList<double> QmaxList;
    QList<int> rowsList;
    QList<int> colsList;
    QList<int> positions;
    QList<int> order;
    
    //+++
    bool checkSelection=false;
    QItemSelectionModel *select = tableMerge->selectionModel();
    if (select->hasSelection()) checkSelection=true;

    const QModelIndex mIndex;
    
    //+++
    bool scaleErrors=checkBoxScaleErrors->isChecked();
    int plusLeft=spinBoxSmartMergePlusLeft->value();
    int plusRight=spinBoxSmartMergePlusRight->value();
    
    
    //+++ Progress Dialog +++
    QProgressDialog progress;
    progress.setMinimumDuration(10);
    progress.setWindowModality(Qt::WindowModal);
    progress.setRange(0,N);
    progress.setCancelButtonText("Abort Merging Processing");
    progress.setMinimumWidth(400);
    progress.setMaximumWidth(4000);
    progress.setLabelText("Data Merging Processing Started...");
    progress.show();
    
    QString textProgress;
    //+++ time steps
    QTime dt = QTime::currentTime ();
    
    for(int nn=0; nn<N;nn++)
    {
        if (checkSelection && !select->isRowSelected(nn, mIndex)) continue;
        
        if (tableMerge->item(nn,0)->text()=="") nameMerged="merged-"+QString::number(nn+1);
        else nameMerged=tableMerge->item(nn,0)->text();
        

        //++++++++++++++++++++++++++++++++++++
        QminList.clear();
        QmaxList.clear();
        rowsList.clear();
        colsList.clear();
        positions.clear();
        order.clear();
        
        int Nall=0;
        int usedTablesNumber=0;
        
        //+++
        for(int mm=0; mm<M;mm++)
        {
            tName=((QComboBoxInTable*)tableMerge->cellWidget(nn, mm+1))->currentText();

            //+++
            Rows=0;
            Cols=0;
            Qmin=0;
            Qmax=0;
            //+++
            if (checkTableExistance(tName, Rows, Cols, Qmin, Qmax ) )
            {
                Nall+=Rows;
                int pos=usedTablesNumber;
                
                for (int k=0; k < usedTablesNumber; k++ ) if  (Qmin < QminList[k] ) {pos--; order[k]=order[k]+1;};
                //+++
                order<<pos;
                positions<<mm;
                QminList<<Qmin;
                QmaxList<<Qmax;
                rowsList<<Rows;
                colsList<<Cols;
                //
                usedTablesNumber++;
            }
        }

        //+++
        gsl_matrix* data=gsl_matrix_alloc(Nall,4);
        
        //+++
        NN=0;
        bool loadReso=true;
        QString labelList;
        int overlap=spinBoxOverlap->value();
        int firstPoint, firstPointFromList=0;
        firstPoint=1;
        //+++
        QList<int> firstPointS;
        //+++
        for (int k=0; k < usedTablesNumber; k++ )
        {
            int currentPos=order.indexOf(k);
            addTable(((QComboBoxInTable*)tableMerge->cellWidget(nn, positions[currentPos]+1))->currentText(), data, NN, rowsList[currentPos],overlap, firstPoint);
            
            firstPointS<<firstPoint;
            //+++ Reference Set Check
            if (positions[currentPos]+1==spinBoxRefForMerging->value()) firstPointFromList=k;
            //+++
            labelList+=((QComboBoxInTable*)tableMerge->cellWidget(nn, positions[currentPos]+1))->currentText()+", ";
            if (colsList[currentPos]<4) loadReso=false;
        }
        
        
          //      continue;
        
        //___________ Smart merging ________________________
        if (checkBoxSmart->isChecked())
        {
            if (nn==0)
            {
                std::cout<<"\n\nMegring Scaling Factors :: Smart Mode \n";
                for (int k=firstPointFromList-1; k>=0;k--)std::cout<<"Q-Range-"<<QString::number(positions[order.indexOf(k)]+1).toLocal8Bit().constData()<<"\t";
                std::cout<<"Q-Range-"<<QString::number(positions[order.indexOf(firstPointFromList)]+1).toLocal8Bit().constData()<<"\t";
                for (int k=firstPointFromList+1; k < usedTablesNumber;k++)std::cout<<"Q-Range-"<<QString::number(positions[order.indexOf(k)]+1).toLocal8Bit().constData()<<"\t";
                 std::cout<<"\t\n";
            }
                
            double qExponent=comboBoxSmartSelect->currentIndex();
            

            //+++ left sets from reference adjasting...

            for (int k=firstPointFromList-1; k>=0;k--)
            {
                //--- finding of minimum of dataset [k+1] "reference"
                int QminInt=firstPointS[k+1];
                double QminRef=gsl_matrix_get(data,QminInt,0);
                
                //--- finding of maximum of dataset [k] left from reference
                int QmaxInt=firstPointS[k+1]-1;
                double QmaxRef=gsl_matrix_get(data,QmaxInt,0);
                while (QmaxRef==-99.99) { QmaxInt--; QmaxRef=gsl_matrix_get(data,QmaxInt,0);};
                
                //--- overlap range
                double currentOverlap=QmaxRef-QminRef;
                
                if (currentOverlap>0 || (plusLeft >0 && plusRight>0))
                {
                    //--- mean value of left dataset in overlap range
                    double leftLevel=0;
                    int numLeftLevel=0;
                    double Q;
                    while (gsl_matrix_get(data,QmaxInt,0)>=QminRef || gsl_matrix_get(data,QmaxInt,0)==-99.99)
                    {
                        if (gsl_matrix_get(data,QmaxInt,0)!=-99.99 && gsl_matrix_get(data,QmaxInt,1)!=-99.99 )
                        {
                            Q=gsl_matrix_get(data,QmaxInt,0);
                            leftLevel+=gsl_matrix_get(data,QmaxInt,1)*pow(Q,qExponent);
                            numLeftLevel++;
                        }
                        QmaxInt--;
                    }
                    
                    //2018 new:
                    for(int lp=0; lp<plusLeft;lp++)
                    {
                        if (gsl_matrix_get(data,QmaxInt,0)!=-99.99 && gsl_matrix_get(data,QmaxInt,1)!=-99.99 )
                        {
                            Q=gsl_matrix_get(data,QmaxInt,0);
                            leftLevel+=gsl_matrix_get(data,QmaxInt,1)*pow(Q,qExponent);
                            numLeftLevel++;
                        }
                        QmaxInt--;
                    }
                    
                    //---
                    double rightLevel=0;
                    int numRightLevel=0;
                    while (gsl_matrix_get(data,QminInt,0)<=QmaxRef || gsl_matrix_get(data,QminInt,0)==-99.99)
                    {
                        if (gsl_matrix_get(data,QminInt,0)!=-99.99 && gsl_matrix_get(data,QminInt,1)!=-99.99 )
                        {
                            Q=gsl_matrix_get(data,QminInt,0);
                            rightLevel+=gsl_matrix_get(data,QminInt,1)*pow(Q,qExponent);
                            numRightLevel++;
                        }
                        QminInt++;
                    }
                    //2018 new:
                    for(int rp=0; rp<plusRight;rp++)
                    {
                        if (gsl_matrix_get(data,QminInt,0)!=-99.99 && gsl_matrix_get(data,QminInt,1)!=-99.99 )
                        {
                            Q=gsl_matrix_get(data,QminInt,0);
                            rightLevel+=gsl_matrix_get(data,QminInt,1)*pow(Q,qExponent);
                            numRightLevel++;
                        }
                        QminInt++;
                    }
                    
                    //+++
                    if (numRightLevel>0 && rightLevel>0 && numLeftLevel>0 && leftLevel>0)
                    {
                        double faktor=rightLevel/numRightLevel/leftLevel*numLeftLevel;
                        std::cout<<QString::number(faktor,'E',5).toLocal8Bit().constData()<<"\t";
                        for (int iii=firstPointS[k]; iii<firstPointS[k+1]; iii++)
                        {
                            if (gsl_matrix_get(data,iii,0)!=-99.99 && gsl_matrix_get(data,iii,1)!=-99.99 )
                            {
                             gsl_matrix_set(data,iii,1, gsl_matrix_get(data,iii,1)*faktor);
                             if (scaleErrors) gsl_matrix_set(data,iii,2, gsl_matrix_get(data,iii,2)*faktor);
                            }
                        }
                    }
                    else break;
                }
                else break;
            }
            
            std::cout<<QString::number(1.0,'E',5).toLocal8Bit().constData()<<"\t";
            
            //+++ right sets from reference adjasting...
            for (int k=firstPointFromList+1; k < usedTablesNumber;k++)
            {

                //--- finding of minimum of dataset [k] right from reference
                int QminInt=firstPointS[k];
                double QminRef=gsl_matrix_get(data,QminInt,0);
                
                //--- finding of maximum of dataset [k-1] of "reference"
                int QmaxInt=firstPointS[k]-1;
                double QmaxRef=gsl_matrix_get(data,QmaxInt,0);
                while (QmaxRef==-99.99) { QmaxInt--; QmaxRef=gsl_matrix_get(data,QmaxInt,0);};
                
                //--- overlap range
                double currentOverlap=QmaxRef-QminRef;
                
                
                if (currentOverlap>0  || (plusLeft>0 && plusRight>0) )
                {
                    //--- mean value of left dataset in overlap range
                    double leftLevel=0;
                    int numLeftLevel=0;
                    double Q;
                    while (gsl_matrix_get(data,QmaxInt,0)>=QminRef || gsl_matrix_get(data,QmaxInt,0)==-99.99)
                    {
                        if (gsl_matrix_get(data,QmaxInt,0)!=-99.99 && gsl_matrix_get(data,QmaxInt,1)!=-99.99 )
                        {
                            Q=gsl_matrix_get(data,QmaxInt,0);
                            leftLevel+=gsl_matrix_get(data,QmaxInt,1)*pow(Q,qExponent);
                            numLeftLevel++;
                        }
                        QmaxInt--;
                    }
                    
                    //2018 new:
                    for(int lp=0; lp<plusLeft;lp++)
                    {
                        if (gsl_matrix_get(data,QmaxInt,0)!=-99.99 && gsl_matrix_get(data,QmaxInt,1)!=-99.99 )
                        {
                            Q=gsl_matrix_get(data,QmaxInt,0);
                            leftLevel+=gsl_matrix_get(data,QmaxInt,1)*pow(Q,qExponent);
                            numLeftLevel++;
                        }
                        QmaxInt--;
                    }
                    
                    //---
                    double rightLevel=0;
                    int numRightLevel=0;
                    while (gsl_matrix_get(data,QminInt,0)<=QmaxRef || gsl_matrix_get(data,QminInt,0)==-99.99)
                    {
                        if (gsl_matrix_get(data,QminInt,0)!=-99.99 && gsl_matrix_get(data,QminInt,1)!=-99.99 )
                        {
                            Q=gsl_matrix_get(data,QmaxInt,0);
                            rightLevel+=gsl_matrix_get(data,QminInt,1)*pow(Q,qExponent);
                            numRightLevel++;
                        }
                        QminInt++;
                    }
                    //2018 new:
                    for(int rp=0; rp<plusRight;rp++)
                    {
                        if (gsl_matrix_get(data,QminInt,0)!=-99.99 && gsl_matrix_get(data,QminInt,1)!=-99.99 )
                        {
                            Q=gsl_matrix_get(data,QmaxInt,0);
                            rightLevel+=gsl_matrix_get(data,QminInt,1)*pow(Q,qExponent);
                            numRightLevel++;
                        }
                        QminInt++;
                    }
                    
                    //+++
                    if (numRightLevel>0 && rightLevel>0 && numLeftLevel>0 && leftLevel>0)
                    {
                        double faktor=leftLevel/numLeftLevel/rightLevel*numRightLevel;
                        std::cout<<QString::number(faktor,'E',5).toLocal8Bit().constData()<<"\t";
                        int lastPoint;
                        if (k+1==usedTablesNumber) lastPoint=NN; else lastPoint=firstPointS[k+1];
                        for (int iii=firstPointS[k]; iii<lastPoint; iii++)
                        {
                            if (gsl_matrix_get(data,iii,0)!=-99.99 && gsl_matrix_get(data,iii,1)!=-99.99 )
                            {
                                gsl_matrix_set(data,iii,1, gsl_matrix_get(data,iii,1)*faktor);
                                if (scaleErrors) gsl_matrix_set(data,iii,2, gsl_matrix_get(data,iii,2)*faktor);
                            }
                        }
                    }
                    else break;
                }	
                else  break;
            }
            
        }
        //+++
        
        int removeFirst=spinBoxRemoveFirstMerging->value();
        int removeLast=spinBoxRemoveLastMerging->value();
        if(removeFirst+removeLast>=NN || removeFirst+removeLast==0)saveMergedMatrix(nameMerged, labelList, data, NN, loadReso,false,false, asciiYN);
        else
        {
            NN=NN-removeFirst-removeLast;
            gsl_matrix* dataFinal=gsl_matrix_alloc(NN,4);
            for (int ii=0; ii<NN; ii++) for (int jj=0; jj<4; jj++) gsl_matrix_set(dataFinal, ii, jj, gsl_matrix_get(data,ii+removeFirst,jj) );
            saveMergedMatrix(nameMerged, labelList, dataFinal, NN, loadReso,false,false,asciiYN);
            gsl_matrix_free(dataFinal);
        }
        gsl_matrix_free(data);
        if (checkBoxSmart->isChecked()) std::cout<<nameMerged.toLocal8Bit().constData()<<"\n";
        
        
        
        
        textProgress="Merged: "+QString::number(nn+1)+" / "+QString::number(N)+"\n";
        textProgress+="Time left: "+QString::number(int(dt.msecsTo(QTime::currentTime())/1000.0*N/(nn+1)-dt.msecsTo(QTime::currentTime())/1000.0))  + " / " + QString::number(int(dt.msecsTo(QTime::currentTime())/1000.0*N/(nn+1)))+"\n";
        if (asciiYN) textProgress+="\nASCII file id saved in the folder: \n"+lineEditPathRAD->text()+"ASCII-QI-MERGED/";;
        progress.setLabelText(textProgress);
        
        progress.setValue(nn+1);
        if ( progress.wasCanceled() ) break;
    }
    
    if (!asciiYN) app()->changeFolder(cf,true);
}

//+++++++++++++++++++++++ 2022
void dan18::mergeMethodTOF(bool asciiYN)
{
    Table* t;
    QString oldMergingTable=lineEditCheckTofTable->text();
    if (app()->activeWindow() && QString(app()->activeWindow()->metaObject()->className()) == "Table")
        t = (Table *)app()->activeWindow();
    else if (oldMergingTable == "" || !app()->checkTableExistence(oldMergingTable, t))
    {
        lineEditCheckTofTable->setText("");
        return;
    }
    
    bool mergeAlongRow=checkBoxMergeRowsTof->isChecked();
    
    lineEditCheckTofTable->setText(t->name());
    
    int nCols=t->numCols();
    int nRows=t->numRows();
    
    
    if (!mergeAlongRow && (nRows<2 || nCols==0)) return;
    if (mergeAlongRow && (nRows==0 || nCols<2)) return;
    
    //+++ current folder
    Folder *cf = ( Folder * ) app()->current_folder;
    
    if (!asciiYN) app()->changeFolder("DAN :: Merge.Binning");
    else
    {
        QString path=lineEditPathRAD->text();
        QDir dd(path);
        if (!dd.cd("./ASCII-QI-MERGED-TOF/")) dd.mkdir("./ASCII-QI-MERGED-TOF/");
    }
    
    int numberPoints=spinBoxMergeTof->value();
    
    
    QStringList list;
    QString tableName, currentTableName;
    int nTable;
    
    if (!mergeAlongRow)
    {
        //+++ : selection check
        bool checkSelection=false;
        
        int firstCol=0;
        int lastCol=nCols;
        int selectedCols=0;
        for (int iCol=0; iCol<lastCol; iCol++)
        {
            if (t->isColumnSelected(iCol,true))
            {
                selectedCols++;
                if(!checkSelection) firstCol=iCol;
                checkSelection=true;
            }
        }
        if(selectedCols>0) lastCol=firstCol+selectedCols;
        
        
        //+++ Set Data-Sets List +++
        // withing columns
        for(int col=firstCol; col<lastCol;col++)
        {
            tableName=t->text(0,col).simplified();
            if (tableName=="") continue;
            
            nTable=0;
            list.clear();
            
            for(int raw=1; raw<nRows;raw++)
            {
                currentTableName=t->text(raw,col).simplified();
                if (currentTableName=="") continue;

                if (!app()->checkTableExistence(currentTableName))
                    continue;
                nTable++;
                list<<currentTableName;
            }
            
            if (nTable<1) continue;
            
            mergeTOF(tableName,list,nTable,numberPoints,asciiYN);
        }
    }
    else
    {
        //+++ : selection check
        bool checkSelection=false;
        
        int firstLine=0;
        int lastLine=nRows;
        int selectedLines=0;
        for (int iRow=0; iRow<lastLine; iRow++)
        {
            if (t->isRowSelected(iRow,true))
            {
                selectedLines++;
                if(!checkSelection) firstLine=iRow;
                checkSelection=true;
            }
        }
        if(selectedLines>0) lastLine=firstLine+selectedLines;
        
        
        //+++ Set Data-Sets List +++
        // withing rows
        for(int row=firstLine; row<lastLine;row++)
        {
            tableName=t->text(row,0).simplified();
            if (tableName=="") continue;
            
            nTable=0;
            list.clear();
            
            for(int col=1; col<nCols;col++)
            {
                currentTableName=t->text(row,col).simplified();
                if (currentTableName=="") continue;

                if (!app()->checkTableExistence(currentTableName))
                    continue;
                nTable++;
                list<<currentTableName;
            }
            
            if (nTable<1) continue;
            
            mergeTOF(tableName,list,nTable,numberPoints,asciiYN);
        }
    }
    
    
    if (!asciiYN)app()->changeFolder(cf,true);
}

void dan18::mergeTOF(QString tableName, QStringList list, int nTable, int numberPoints, bool asciiYN)
{
    Table* t;
    if (!app()->checkTableExistence(list[0], t))
        return;

    int nCols = t->numCols();
    if (nCols < 3)
        return;

    QStringList listCleanned;
    listCleanned<<list[0];
    int nTableCleanned=1;
    
    for(int i=1; i<nTable;i++)
    {
        if (!app()->checkTableExistence(list[i], t))
            continue;

        if (t->numCols()<nCols) continue;
        nTableCleanned++;
        listCleanned<<list[i];
    }
    
    double qMin=1000, qMax=-1000;
    double qCurrent;
    bool logScale=false;
    if (checkBoxLogStepTof->isChecked()) logScale=true;
    
    
    if (radioButton->isChecked())
    {
        //+++ Q-range from data
        for(int i=0; i<nTableCleanned;i++)
        {
            if (!app()->checkTableExistence(listCleanned[i], t))
                continue;

            for(int rr=0; rr<t->numRows();rr++)
            {
                qCurrent=t->text(rr,0).toDouble();
                if(!logScale && qCurrent<qMin) qMin=qCurrent;
                if(logScale && qCurrent>0 && qCurrent<qMin) qMin=qCurrent;
                
                if(qCurrent>qMax) qMax=qCurrent;
            }
        }
    }
    else
    {
        qMin=spinQminTof->value();
        qMax=spinQmaxTof->value();
        if (qMax<qMin)
        {
            double tmp=qMin; qMin=qMax; qMax=tmp;
        }
    }
    
    if (qMin==qMax)  return;
    
    

    if (logScale && qMin<=0 && qMax<=0) logScale=false;
    if (logScale && qMin<=0 && qMax>0) qMin=0.00001;
    
    //double qStep=fabs(qMax-qMin)/(numberPoints-1);
    
    
    int *n=new int[numberPoints];
    double *Q=new double[numberPoints];
    double *Qmin=new double[numberPoints];
    double *Qmax=new double[numberPoints];
    double *I=new double[numberPoints];
    double *dI=new double[numberPoints];
    double *Sigma=new double[numberPoints];
    double *dQ=new double[numberPoints];
    
    for(int i=0; i<numberPoints;i++){  Qmin[i]=qMax;Qmax[i]=qMin;n[i]=0;Q[i]=0;I[i]=0;dI[i]=0;dQ[i]=0;Sigma[i]=0; }
    
    int dataFormat=comboBox4thCol->currentIndex();
    int currentPosition;
    double iCurrent, diCurrent, sigmaCurrent;
    
    double errLowLimit=spinBoxErrLeftLimitTof->value()/100.00;
    double errUpperLimit=spinBoxErrRightLimitTof->value()/100.00;
    
    for(int i=0; i<nTableCleanned;i++)
    {
        if (!app()->checkTableExistence(listCleanned[i], t))
            continue;

        for(int rr=0; rr<t->numRows();rr++)
        {
            qCurrent=t->text(rr,0).toDouble();
            if (qCurrent<qMin || qCurrent>qMax ) continue;
            
            iCurrent=t->text(rr,1).toDouble();
            diCurrent=t->text(rr,2).toDouble();
            
            if (diCurrent==0.0) continue;
            if (diCurrent/iCurrent<errLowLimit || diCurrent/iCurrent>errUpperLimit ) continue;
            
            if (logScale) currentPosition=int((log10(qCurrent)-log10(qMin))/(log10(qMax)-log10(qMin))*(numberPoints-1)+0.5);
            else currentPosition=int((qCurrent-qMin)/(qMax-qMin)*(numberPoints-1)+0.5);
            
            
            n[currentPosition]++;
            
            if (qCurrent<Qmin[currentPosition])Qmin[currentPosition]=qCurrent;
            if (qCurrent>Qmax[currentPosition])Qmax[currentPosition]=qCurrent;
            
            Q[currentPosition]+=qCurrent;
            I[currentPosition]+=iCurrent;
            dI[currentPosition]+=diCurrent*diCurrent;
            
            if (nCols>3 && dataFormat==0 ) Sigma[currentPosition]+=t->text(rr,3).toDouble()*t->text(rr,3).toDouble();
            if (nCols>4 && dataFormat==3 ) Sigma[currentPosition]+=t->text(rr,4).toDouble()*t->text(rr,4).toDouble();
            
            if (nCols>3 &&  (dataFormat==1||dataFormat==2) ) dQ[currentPosition]+=t->text(3,rr).toDouble()*t->text(3,rr).toDouble();
        }
    }
    
    for(int i=0; i<numberPoints;i++)
    {
        if (n[i]==0) {Qmin[i]=qMin;Qmax[i]=qMax;n[i]=0;Q[i]=-99.99;I[i]=0;dI[i]=0;dQ[i]=0;Sigma[i]=0; continue; };
        Q[i]=Q[i]/n[i];
        I[i]=I[i]/n[i];
        dI[i]=sqrt(dI[i])/n[i];
        Sigma[i]=sqrt(Sigma[i])/n[i]+(Qmax[i]-Qmin[i])/2;
        dQ[i]=sqrt(dQ[i])/n[i];
    }
    
    
    
    int removeFirst=spinBoxRemoveFirstMergingTOF->value();
    int removeLast=spinBoxRemoveLastMergingTOF->value();
    
    if(removeFirst+removeLast>=numberPoints ) {removeFirst=0; removeLast=0;};
    
    
    int finalNumberPoints=numberPoints-removeFirst-removeLast;
    gsl_matrix* dataFinal=gsl_matrix_alloc(finalNumberPoints,5);
    
    for (int ii=0; ii<finalNumberPoints; ii++)
    {
        gsl_matrix_set(dataFinal,ii, 0, Q[ii+removeFirst]);
        gsl_matrix_set(dataFinal,ii, 1, I[ii+removeFirst]);
        gsl_matrix_set(dataFinal,ii, 2, dI[ii+removeFirst]);
        gsl_matrix_set(dataFinal,ii, 3, dQ[ii+removeFirst]);
        gsl_matrix_set(dataFinal,ii, 4, Sigma[ii+removeFirst]);
    }


    bool loadReso=false;
    if (nCols>3 && dataFormat==0 ) loadReso=true;
    if (nCols>4 && dataFormat==3 ) loadReso=true;
    
    bool loadQerr=false;
    if (nCols>3 &&  (dataFormat==1||dataFormat==2) ) loadQerr=true;
    
    QString label=list[0];
    if (nTable>1) label+=","+list[1];
    if (nTable==3 || nTable==4 ) label+=","+list[2];
    if (nTable==4 ) label+=","+list[3];
    if (nTable>4) label+=", ... , "+ list[nTable-2]+","+ list[nTable-1];
    
    saveMergedMatrix(tableName, label, dataFinal,finalNumberPoints,loadReso,loadQerr, true, asciiYN);
    
    gsl_matrix_free(dataFinal);
    
    
    delete[] n;
    delete[] Q;
    delete[] Qmin;
    delete[] Qmax;
    delete[] I;
    delete[] dI;
    delete[] Sigma;
    delete[] dQ;
}

//+++++++++++++++++++++++
void dan18::saveMergeInfo()
{
    int nn,mm;
    bool ok;
    QString tableName = QInputDialog::getText(this,
                                              "Save information from merging interface as a new Table ", "Enter name of a new Table",
                                              QLineEdit::Normal,
                                              QString::null, &ok);
  
    if ( !ok ||  tableName.isEmpty() )return;
    
    int N = spinBoxNtoMerge->value();
    int M = spinBoxMmergingCond->value();

    tableName = app()->generateUniqueName(tableName);
    
    Table *t = app()->newTable(tableName, M, N + 1);

    //+++ Cols Names
    t->setColName(0, "NewName");
    t->setColPlotDesignation(0, Table::None);
    t->setColumnType(0, Table::Text);

    for (nn = 0; nn < N; nn++)
    {
        t->setColName(nn + 1, "Q-Range-" + QString::number(nn + 1));
        t->setColPlotDesignation(nn + 1, Table::None);
        t->setColumnType(0, Table::Numeric);
    }

    for(mm=0; mm<M;mm++)
    {
        for(nn=1; nn<(N+1);nn++)
        {
            QComboBoxInTable *iTable = (QComboBoxInTable*)tableMerge->cellWidget(mm,nn);
            if (iTable->currentIndex()>0) t->setText(mm,nn,iTable->currentText());
        }
        t->setText(mm,0,tableMerge->item(mm,0)->text());
    }
            return;
    //+++
    t->setWindowLabel("Merge::Table");
    app()->setListViewLabel(t->name(), "Merge::Table");
    app()->updateWindowLists(t);
    
    for (int tt=0; tt<t->numCols(); tt++)
    {
        t->table()->resizeColumnToContents(tt);
        t->table()->setColumnWidth(tt, t->table()->columnWidth(tt)+10); 
    }
}

//+++++++++++++++++++++++
void dan18::readMergeInfo()
{
    if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "Table") return;
    
    Table* t = (Table*)app()->activeWindow();

    int N=t->numCols()-1;
    spinBoxNtoMerge->setValue(0);
    spinBoxNtoMerge->setValue(N);
    int M=t->numRows();
    spinBoxMmergingCond->setValue(0);
    spinBoxMmergingCond->setValue(M);

    //+++ Set Data-Sets List +++
    for(int nn=0; nn<N;nn++) for(int mm=0; mm<M;mm++)
    {
        QComboBoxInTable *iTable = (QComboBoxInTable*)tableMerge->cellWidget(mm,nn+1);
        if (iTable->findText(t->text(mm,nn+1),Qt::MatchExactly)>0)
            iTable->setItemText(iTable->currentIndex(), t->text(mm,nn+1));
    }
    
    QString nameQI;
    for(int mm=0; mm<M;mm++)
    {
        nameQI=t->text(mm,0);
        nameQI=nameQI.simplified();
        nameQI=nameQI.replace(" ", "-").replace("/", "-").replace("_", "-").replace(",", "-").replace(".", "-").remove("%");
        tableMerge->item(mm, 0)->setText(nameQI);
    }
}

