/***************************************************************************
 File : dan18-rawdata-extractor.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : SANS Data Analisys Interface
 
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
#include "dan18.h"

//*******************************************
//+++  2017: Extractor ...
//*******************************************
void dan18::extractorConnectSlots()
{
    connect( pushButtonNewInfoExtractor, SIGNAL( clicked() ), this, SLOT( newInfoExtractor() ) );
    connect( pushButtonMakeListExtractor, SIGNAL( clicked() ), this, SLOT( addToInfoExtractor() ) );
    connect( pushButtonExtractorAddCol, SIGNAL( clicked() ), this, SLOT( addColToInfoExtractor() ) );
}

QString dan18::getHeaderInfoString(QString number, QString name)
{
    if (name == "Sample")
        return sample->readName(number);
    if (name == "C")
        return QString::number(int(collimation->readCinM(number)));
    if (name == "D")
        return QString::number(detector->readDinM(number));
    if (name == "lambda")
        return QString::number(readLambda(number), 'f', 4);
    if (name == "Sum")
        return QString::number(readSum(number));
    if (name == "Duration")
        return QString::number(readDuration(number));
    if (name == "cps")
        return QString::number(readSum(number) / readDuration(number), 'f', 5);
    if (name == "Thickness")
        return QString::number(sample->readThickness(number), 'f', 5);
    if (name == "Sum-vs-Mask")
        return QString::number(integralVSmaskSimmetrical(number));
    if (name == "Sum-vs-Mask-Dead-Time-Corrected")
        return QString::number(integralVSmaskUniDeadTimeCorrected(number));
    if (name == "Q2-vs-Mask")
        return QString::number(Q2_VS_maskSimmetrical(number));
    if (name == "Runs")
        return number;
    if (name == "Monitor-1")
        return QString::number(readMonitor1(number));
    if (name == "Monitor-2")
        return QString::number(readMonitor2(number));
    if (name.contains("Monitor-3"))
        return QString::number(readMonitor3(number));

    if (name == "Name")
        name = "Experiment-Title";
    if (name == "Sample-Nr")
        name = "Sample-Position-Number";
    if (name == "Who")
        name = "User-Name";
    if (name == "Offset")
        name = "C,D-Offset";

    QString res = parserHeader->readNumberString(number, "[" + name + "]");

    if (name.contains("Motor-"))
        res = QString::number(res.toDouble(), 'f', 3);
    if (name.contains("Field-"))
        res = QString::number(res.toDouble(), 'f', 5);
    if (name.contains("Selector"))
        res = QString::number(res.toDouble(), 'f', 4);
    return res;
}
QString dan18::generateUniqueStringInList (QStringList lst, const QString str)
{
    int index = 0;

    
    for ( int i = 0; i < int ( lst.count() ); i++ )
    {
        if ( lst[i].startsWith ( str ) && (lst[i].toInt()>=1 || lst[i]==str) ) index++;
    }
    


    if (index==0) return str;
    
    QString newName = str;

    newName += QString::number ( index );
    
    while ( lst.contains ( newName ) )
    {
        newName = str + QString::number ( ++index );
    }
    return newName;
}

void dan18::addColToInfoExtractor()
{
    //+++
    QString activeTable=comboBoxInfoExtractor->currentText()
;
    
    //+++ very IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    QStringList infoTablesList;
    findTableListByLabel("Info::Extractor", infoTablesList);
    
    
    QString TableName;
    
    if ( infoTablesList.count()==0 || !infoTablesList.contains(activeTable))
    {

        infoTablesList<<TableName;
        infoTablesList.sort();
        comboBoxInfoExtractor->clear();
        
        comboBoxInfoExtractor->insertItems(0, infoTablesList);

        return;
    }
    else TableName=activeTable;
    
    
    int i;
    
    //+++ create table
    Table* tableDat;
    
    QString wildCardLocal=wildCard;
    wildCardLocal=wildCard;


    int startRaw=0;
    

    
    //+++
    if (!checkTableExistence(TableName)) return;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==TableName)  tableDat=(Table *)t;
    //+++
    
    QString what2add=comboBoxInfoExtractorCol->currentText();

    int initNumberCols=tableDat->numCols();
    QList<int> initColType=tableDat->columnTypes();
    
    if (initNumberCols<1) return;
    
    
    tableDat->addCol();
    tableDat->setColComment(initNumberCols, what2add);
    tableDat->setColName(initNumberCols,generateUniqueStringInList (tableDat->colNames(), comboBoxInfoExtractorCol->currentText()));

    
    if (what2add=="Sample"||what2add=="Beam"||what2add=="Date"||what2add=="Time"||what2add.contains("Comment")||what2add=="Name"||what2add=="Who")
    { tableDat->setColPlotDesignation(initNumberCols,Table::None); initColType<<1;}
    else if (what2add=="Files"||what2add=="RT-number"||what2add=="RT-Time-Factor"||what2add=="RT-Repetitions")
    { tableDat->setColPlotDesignation(initNumberCols,Table::None); initColType<<1;}
    else if (what2add=="RT-Frame-Duration"||what2add=="Attenuator"||what2add=="Polarization"||what2add=="Lenses")
    { tableDat->setColPlotDesignation(initNumberCols,Table::None); initColType<<1;}
    else if (what2add=="C"||what2add =="D"||what2add=="lambda"||what2add=="Offset"||what2add=="Sample-Nr"||what2add=="Thickness"||what2add=="Time-Factor")
    { tableDat->setColPlotDesignation(initNumberCols,Table::None); initColType<<0;}
    else if (what2add.contains("Field-")||what2add.contains("Beamwindow-")||what2add.contains("-Position")||what2add.contains("BeamWin-"))
    { tableDat->setColPlotDesignation(initNumberCols,Table::None); initColType<<0;}
    else if (what2add.contains("Motor-"))
    { tableDat->setColPlotDesignation(initNumberCols,Table::X); initColType<<0;}
    else    { tableDat->setColPlotDesignation(initNumberCols,Table::Y); initColType<<0;};
    
    tableDat->setColumnTypes(initColType);
    if (what2add=="Monitor-3") what2add="Monitor-3|Tr|ROI";
    for(int iter=0; iter<tableDat->numRows();iter++) tableDat->setText(iter,initNumberCols,getHeaderInfoString(tableDat->text(iter,0), what2add));


    
    tableDat->table()->resizeColumnToContents(initNumberCols);
    tableDat->table()->setColumnWidth(initNumberCols, tableDat->table()->columnWidth(initNumberCols)+10);
    maximizeWindow(TableName);
}

void dan18::addToInfoExtractor()
{
    //+++
    QString activeTable=comboBoxInfoExtractor->currentText();
    
    //+++ vry IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    QStringList infoTablesList;
    findTableListByLabel("Info::Extractor", infoTablesList);
    
    
    QString TableName;
    
    if (infoTablesList.count()==0 || !infoTablesList.contains(activeTable))
    {
        bool ok;
        TableName = QInputDialog::getText(this,
                                          "Table's Extractor", "Enter name of Table:",
                                          QLineEdit::Normal,
                                          "info-extrator", &ok);
        if ( !ok ||  TableName.isEmpty() )
        {
            return;
        }
        
        if (infoTablesList.contains(TableName)) TableName=app()->generateUniqueName(TableName+"-");
        infoTablesList<<TableName;
        infoTablesList.sort();
        comboBoxInfoExtractor->clear();
        
        comboBoxInfoExtractor->insertItems(0, infoTablesList);
        comboBoxInfoExtractor->setCurrentIndex(infoTablesList.indexOf(TableName));
    }
    else TableName=activeTable;
    
    
    int i;
    
    //+++ create table
    Table* tableDat;
    
    QString wildCardLocal=wildCard;
    
    if (wildCardLocal.count("#")<3)
        wildCardLocal=wildCardLocal.replace("#","*");
    else if (wildCard.count("#")>2)
    {
        QMessageBox::critical( 0, "qtiSAS", "Check >> File : Pattern (#>1)");
        return;
    }
    else if (wildCardLocal.count("*")!=1)
    {
        QMessageBox::critical( 0, "qtiSAS", "Check >> File : Pattern (* != 1)");
        return;
    }
    
    QString filter=textEditPattern->text();
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"DAN - Getting File Information",Dir,"*");

    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Getting File Information"));
    fd->setNameFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    if (filesNumber==0)
    {
        QMessageBox::critical( 0, "qtiSAS", "Nothing was selected");
        return;
    }
    
    QString test=selectedDat[0];
    if ( test.contains(Dir) )
    {
        test=test.remove(Dir);
        if (!dirsInDir && test.contains("/") )
        {
            QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
            return;
        }
        else if (dirsInDir && test.count("/")>1 )
        {
            QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder:: + sub-folders");
            return;
        }
    }
    else
    {
        QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
        return;
    }
    
    
    int startRaw=0;
    
    wildCardLocal=wildCard;
    
    //app()->ws->hide();
    //app()->ws->blockSignals ( true );
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    //+++
    if (checkTableExistence(TableName))
    {
        //+++ Find table
        QList<MdiSubWindow *> tableList=app()->tableList();
        foreach (MdiSubWindow *t, tableList) if (t->name()==TableName)  tableDat=(Table *)t;
        //+++

        if (tableDat->numCols()<1)
        {
            QMessageBox::critical( 0, "qtiSAS", "Create new table (# cols)");
            //app()->ws->show();
            //app()->ws->blockSignals ( false );
            return;
        }
        
        startRaw=tableDat->numRows();
        
    }
    else
    {
        
        tableDat=app()->newTable(TableName, filesNumber,1);
        
        //+++
        tableDat->setWindowLabel("Info::Extractor");
        app()->setListViewLabel(tableDat->name(), "Info::Extractor");
        app()->updateWindowLists(tableDat);
        
        QStringList colType;
        //+++ Cols Names
        tableDat->setColName(0,"Runs");	tableDat->setColPlotDesignation(0,Table::X);colType<<"1";
        tableDat->setColumnTypes(colType);
    }
    
    
    tableDat->setNumRows(startRaw+filesNumber);
    
    
    //+++
    tableDat->show();
    //+++
    QString fnameOnly;
    int skip;
    QString s,ss,nameMatrix;
    int iter;
    //
    QString pos,num;

    for (iter = startRaw; iter < (startRaw + filesNumber); iter++)
    {
        //+++ header
        nameMatrix = FilesManager::findFileNumberInFileName(wildCard, selectedDat[iter - startRaw].remove(Dir));
        //+++ Run Nuber [itRuns]
        tableDat->setText(iter, 0, nameMatrix);
        for (int i = 1; i < tableDat->numCols(); i++)
            tableDat->setText(iter, i, getHeaderInfoString(nameMatrix, tableDat->comment(i)));
    }

    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    //app()->ws->show();
    //app()->ws->blockSignals ( false );
    
    maximizeWindow(TableName);
    

    tableDat->table()->resizeColumnToContents(0);
    tableDat->table()->setColumnWidth(0, tableDat->table()->columnWidth(0)+10);
}

//++++++SLOT::newInfoExtractor++
void dan18::newInfoExtractor()
{
    bool ok;
    QString TableName = QInputDialog::getText(this,
                                              "Table's Extractor", "Constract yourself...",
                                              QLineEdit::Normal,
                                              "info-extractor", &ok);
    if ( !ok ||  TableName.isEmpty() )
    {
        return;
    }
    newInfoExtractor(TableName);
}

//++++++SLOT::newInfoExtractor++
void dan18::newInfoExtractor(QString TableName)
{
    
    //+++ vry IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    QStringList infoTablesList;
    findTableListByLabel("Info::Extractor", infoTablesList);

    if (comboBoxInfoExtractor->count()>0 && TableName=="") return; //
    
    comboBoxInfoExtractor->clear();
    if (TableName=="" ) {comboBoxInfoExtractor->insertItems(0, infoTablesList);return;}

    if (infoTablesList.contains(TableName)) TableName=app()->generateUniqueName(TableName+"-");
    infoTablesList<<TableName;
    infoTablesList.sort();

    
    comboBoxInfoExtractor->insertItems(0, infoTablesList);
    comboBoxInfoExtractor->setCurrentIndex(infoTablesList.indexOf(TableName));
    
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    //+++ create table
    Table* tableDat;
    
    tableDat=app()->newTable(TableName, 0,1);
    
    //+++
    tableDat->setWindowLabel("Info::Extractor");
    app()->setListViewLabel(tableDat->name(), "Info::Extractor");
    app()->updateWindowLists(tableDat);
    
    QStringList colType;
    //+++ Cols Names
    tableDat->setColName(0,"Runs");	tableDat->setColPlotDesignation(0,Table::X);colType<<"1";
    tableDat->setColumnTypes(colType);
    
    //+++
    tableDat->show();
    
    maximizeWindow(TableName);
    

}

void dan18::extractorInit()
{
QStringList lst;
    
    lst<<"Sample";
    lst<<"Comment1";
    lst<<"Comment2";
    lst<<"Name";
    lst<<"Who";
    lst<<"Sample-Nr";
    lst<<"Date";
    lst<<"Time";
    lst<<"Duration";
    lst<<"Sum";
    lst<<"cps";
    lst<<"C";
    lst<<"D";
    lst<<"Offset";
    lst<<"lambda";
    lst<<"Selector";
    lst<<"Thickness";
    lst<<"Monitor-1";
    lst<<"Monitor-2";
    lst<<"Monitor-3";
    lst<<"Sample-Motor-1";
    lst<<"Sample-Motor-2";
    lst<<"Sample-Motor-3";
    lst<<"Sample-Motor-4";
    lst<<"Sample-Motor-5";
    lst<<"Field-1";
    lst<<"Field-2";
    lst<<"Field-3";
    lst<<"Field-4";
    lst<<"Attenuator";
    lst<<"Polarization";
    lst<<"Lenses";
    lst<<"Sum-vs-Mask";
    lst<<"Sum-vs-Mask-Dead-Time-Corrected";
    lst<<"Q2-vs-Mask";
        lst<<"Runs";
/*
    lst<<"Beam";
    lst<<"Beamwindow-X";
    lst<<"Beamwindow-Y";

    lst<<"X-Position";
    lst<<"Y-Position";

    lst<<"BeamWin-Xs";
    lst<<"BeamWin-Ys";
    lst<<"Beamwin-X-Pos";
    lst<<"Beamwin-Y-Pos";
    lst<<"Time-Factor";

    lst<<"RT-number";
    lst<<"RT-Time-Factor";
    lst<<"RT-Repetitions";
    lst<<"RT-Frame-Duration";

    lst<<"Slices-Count";
    lst<<"Slices-Duration";
    lst<<"Slices-Current-Number";
    lst<<"Slices-Current-Duration";
    lst<<"Slices-Current-Monitor1";
    lst<<"Slices-Current-Monitor2";
    lst<<"Slices-Current-Monitor3";
    lst<<"Slices-Current-Sum";

    */
    comboBoxInfoExtractorCol->addItems(lst);
}


