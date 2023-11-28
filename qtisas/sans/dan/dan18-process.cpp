/***************************************************************************
 File : dan18-process.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : SANS Data Analisys Interface: process tab tools
 
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

#include <QScrollBar>

#include "dan18.h"

//*******************************************
//+++  RT tools:: Sum [slot]
//*******************************************
void dan18::processdataConnectSlots()
{
    connect( pushButtonNewScript, SIGNAL( clicked() ), this, SLOT( newScriptTable()));    
    connect( pushButtonMakeTable, SIGNAL( clicked() ), this, SLOT(makeScriptTable()) );
    connect( pushButtonSaveSettings, SIGNAL( clicked() ), this, SLOT( saveSettingsSlot() ) );   
    connect(sliderConfigurations, SIGNAL( valueChanged(int) ), this, SLOT(SetColNumberNew (int) ) );  
    connect( comboBoxMakeScriptTable, SIGNAL( activated(int) ), this, SLOT( activeScriptTableSelected(int) ) );
    connect(pushButtonAddCopy, SIGNAL( clicked() ), this, SLOT(addCopyOfLastConfiguration() ) );
    connect(pushButtonDeleteFirst, SIGNAL( clicked() ), this, SLOT(deleteFirstConfiguration() ) );
    connect(tableEC->verticalHeader(), SIGNAL(sectionClicked(int)), this,SLOT(vertHeaderTableECPressed(int)));
    connect(tableEC->horizontalHeader(), SIGNAL(sectionClicked(int)), this,SLOT(horHeaderTableECPressed(int)));
    connect( tableEC, SIGNAL( cellActivated(int,int) ),  this, SLOT( tableECclick(int,int) ) );
    
    connect( pushButtonCalcTr, SIGNAL( clicked() ), this, SLOT( calculateTransmissionAll() ) );
    
    connect(pushButtonCenter, SIGNAL( clicked() ), this, SLOT(calculateCentersInScript() ) );
    connect(pushButtonAbsFactor, SIGNAL( clicked() ), this, SLOT(calculateAbsFactorInScript() ) );//+++2014-04
    connect(pushButtonTrMaskDB, SIGNAL( clicked() ), this, SLOT(calculateTrMaskDB() ) );//+++2014-04
    connect( radioButtonDpSelector2D, SIGNAL( toggled(bool) ), this, SLOT( dataProcessingOptionSelected()) );
    connect( radioButtonDpSelector1D, SIGNAL( toggled(bool) ), this, SLOT( dataProcessingOptionSelected()) );
    connect( radioButtonDpSelectorScript, SIGNAL( toggled(bool) ), this, SLOT( dataProcessingOptionSelected()) );
}

//+++ Search of  Script tables
void dan18::findSettingTables()
{
    QString activeTableScript=comboBoxMakeScriptTable->currentText();
    //+++
    QStringList list;
    QStringList listOpen;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList)
    {
        Table *tt=(Table *)t;
        if (tt->text(0,0)=="DAT::Path") list<<tt->name();
    }
    //+++
    
    //
    list.prepend("New-Table");
    comboBoxMakeScriptTable->clear();
    comboBoxMakeScriptTable->addItems(list);
    if (list.contains(activeTableScript))
        comboBoxMakeScriptTable->setItemText(comboBoxMakeScriptTable->currentIndex(),
                                             activeTableScript);
}


//*******************************************
//+++  newScriptTable() [slot]
//*******************************************

void dan18::newScriptTable()
{
    ImportantConstants();
    //+++
    QString activeTable=comboBoxMakeScriptTable->currentText();
    if (activeTable=="") activeTable="script";
    //+++

    QString tableName;
    
    bool ok;
    tableName = QInputDialog::getText(this,
                                      "Creation of Script-Table", "Enter name of a new Script-Table:",
                                      QLineEdit::Normal,
                                      activeTable, &ok);
 
    tableName=tableName.replace(" ", "-").replace("/", "-").replace("_", "-").replace(",", "-").replace(".", "-").remove("%").remove("(").remove(")");
    
    if ( !ok ||  tableName.isEmpty() )
    {
        return;
    }
    newScriptTable(tableName);
}

//*******************************************
//+++  newScriptTable() [slot]
//*******************************************
void dan18::newScriptTable(QString tableName)
{
    ImportantConstants();
    QString activeTable=comboBoxMakeScriptTable->currentText();
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
  
    tableName=tableName.replace(" ", "-").replace("/", "-").replace("_", "-").replace(",", "-").replace(".", "-").remove("%").remove("(").remove(")");
    
    if ( tableName.isEmpty() )
    {
	return;
    }
    
    if (checkTableExistence(tableName))
    {
        QMessageBox::critical( 0, "qtiSAS", "Table "+tableName+" is already exists");
        //if (!app()->hiddenApp) QMessageBox::critical( 0, "qtiSAS", "Table "+tableName+" is already exists");
        //comboBoxMakeScriptTable->setCurrentText(tableName);
        return;
    }
    
    //+++
    Table* w = app()->newTable(tableName, 0,21);
    //+++ new
    w->setWindowLabel("DAN::Script::Table");
    app()->setListViewLabel(w->name(), "DAN::Script::Table");
    app()->updateWindowLists(w);

    
    QStringList colType;
    
    //+++
    //Col-Names
    int colNumber=0;
    
    w->setColName(colNumber,"Run-info"); 	
    w->setColPlotDesignation(colNumber,Table::None); colType<<"1";
    int indexInfo=colNumber;
    colNumber++;
    
    
    w->setColName(colNumber,"#-Run");  	
    w->setColPlotDesignation(colNumber,Table::X); 	 colType<<"1";
    int indexSA=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"#-Condition");	
    w->setColPlotDesignation(colNumber,Table::None); 	colType<<"0";
    int indexCond=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"C");   	
    w->setColPlotDesignation(colNumber,Table::None);
    w->setColComment(colNumber,"[m]"); 			colType<<"0";
    int indexC=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"D");  
    w->setColPlotDesignation(colNumber,Table::None);
    w->setColComment(colNumber,"[m]"); 			colType<<"0";
    int indexD=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Lambda"); 	
    w->setColPlotDesignation(colNumber,Table::None);
    QString sss="[";sss+=QChar(197);sss+="]";
    w->setColComment(colNumber,sss); 			colType<<"1";
    int indexLam=colNumber;
    colNumber++;

    w->setColName(colNumber,"Beam Size");	
    w->setColPlotDesignation(colNumber,Table::None);
    w->setColComment(colNumber,"[mm x mm]");		colType<<"1";
    int indexCA=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"#-BC");
    w->setColPlotDesignation(colNumber,Table::None);		colType<<"1";
    int indexBC=colNumber;
    colNumber++;    
    
    w->setColName(colNumber,"#-EC [EB]"); 
    w->setColPlotDesignation(colNumber,Table::None);		colType<<"1";
    int indexEC=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"#-Buffer"); 
    w->setColPlotDesignation(colNumber,Table::None);		colType<<"1";
    int indexBuffer=colNumber;
    colNumber++;
    
    
    w->setColName(colNumber,"Thickness"); 
    w->setColPlotDesignation(colNumber,Table::None);
    w->setColComment(colNumber,"[cm]"); colType<<"0";
    int indexThickness=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Transmission-Sample"); 
    w->setColPlotDesignation(colNumber,Table::None);		colType<<"1"; //+++ 2019 colType<<"0";
    int indexTr=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Transmission-Buffer"); 
    w->setColPlotDesignation(colNumber,Table::None);		colType<<"1"; //+++ 2019 colType<<"0";
    int indexTrBuffer=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Buffer-Fraction"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"0";
    int indexBufferFraction=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Factor"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"0";
    int indexFactor=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"X-center"); 
    w->setColPlotDesignation(colNumber,Table::Y);		colType<<"0";
    int indexXC=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Y-center"); 
    w->setColPlotDesignation(colNumber,Table::Y);		colType<<"0";
    int indexYC=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Mask"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"1";
    int indexMask=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Sens"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"1";
    int indexSens=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Use-Buffer-as-Sensitivity"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"1";
    int indexUseBubberAsSensLocal=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Status"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"1";
    int indexStatus=colNumber;
    colNumber++;
    
    //+++ Col Types
    w->setColumnTypes(colType);
    
    QStringList  list;
    findTableListByLabel("DAN::Script::Table",list);
    //    list.prepend("New-Script-Table");
    comboBoxMakeScriptTable->clear();
    comboBoxMakeScriptTable->addItems(list);
    comboBoxMakeScriptTable->setItemText(comboBoxMakeScriptTable->currentIndex(),
                                         tableName);
    
    // adjust columns
    for (int tt=0; tt<w->numCols(); tt++) 
    {
	w->table()->resizeColumnToContents(tt);
	w->table()->setColumnWidth(tt, w->table()->columnWidth(tt)+10); 
	
    }
    
    saveSettings(tableName+"-Settings");
    maximizeWindow(tableName);
    
    
    //+++  Subtract Bufffer
    bool subtractBuffer= false;
    if (comboBoxMode->currentText().contains("(BS)")) subtractBuffer=true;
    
    if (subtractBuffer)
    {
	w->table()->showColumn(indexBuffer);
	w->table()->resizeColumnToContents(indexBuffer);
	w->table()->setColumnWidth(indexBuffer, w->table()->columnWidth(indexBuffer)+10); 
	
	w->table()->showColumn(indexTrBuffer);	
	w->table()->resizeColumnToContents(indexTrBuffer);
	w->table()->setColumnWidth(indexTrBuffer, w->table()->columnWidth(indexTrBuffer)+10); 
	
	w->table()->showColumn(indexBufferFraction);	
	w->table()->resizeColumnToContents(indexBufferFraction);
	w->table()->setColumnWidth(indexBufferFraction, w->table()->columnWidth(indexBufferFraction)+10); 
	
	w->table()->showColumn(indexUseBubberAsSensLocal);	
	w->table()->resizeColumnToContents(indexUseBubberAsSensLocal);
	w->table()->setColumnWidth(indexUseBubberAsSensLocal,w->table()->columnWidth(indexUseBubberAsSensLocal)+10); 
	
	
    }
    else 
    {
	w->table()->hideColumn(indexBuffer);
	w->table()->hideColumn(indexTrBuffer);	
	w->table()->hideColumn(indexBufferFraction);
	w->table()->hideColumn(indexUseBubberAsSensLocal);	
    }
    
    w->table()->showColumn(indexMask);
    w->table()->resizeColumnToContents(indexMask);
    w->table()->setColumnWidth(indexMask, w->table()->columnWidth(indexMask)+10); 
    
    w->table()->showColumn(indexSens);
    w->table()->resizeColumnToContents(indexSens);
    w->table()->setColumnWidth(indexSens, w->table()->columnWidth(indexSens)+10); 
    
}
//*******************************************
//+++  makeScriptTable [slot]
//*******************************************
void dan18::makeScriptTable()
{
    ImportantConstants();

    //+++ select files
    
    QFileDialog *fd = new QFileDialog(this,"DAN - Getting File Information",Dir,textEditPattern->text());
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles );
    fd->setWindowTitle(tr("DAN - Getting File Information"));
    //    fd->addFilter(textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    //+++
    if (!fd->exec() == QDialog::Accepted ) return;
    

    
    
    //+++
    QStringList selectedDat=fd->selectedFiles();

    
    makeScriptTable(selectedDat);
}

//*******************************************
//+++  makeScriptTable [slot]
//*******************************************
void dan18::makeScriptTable(QStringList selectedDat)
{    
    if(comboBoxMakeScriptTable->count()==0) 
    {
        QMessageBox::critical( 0, "qtiSAS", "Create first Script-Table");
	//if (!app()->hiddenApp) QMessageBox::critical( 0, "qtiSAS", "Create first Script-Table");
    //else std::cout<<"ERROR :: "<<"Create first Script-Table\n";
	return;
    }
    
    ImportantConstants();
    
    //+++
    QString activeTable=comboBoxMakeScriptTable->currentText();

    //+++
    QString tableName;
    
    tableName=activeTable;
    
    int startRaw=0;
    
    QString s="";
    int i, iMax;
    
    //+++
    Table* w;
    
    int filesNumber= selectedDat.count();
    
    if (filesNumber==0)
    {
        QMessageBox::critical( 0, "qtiSAS", "Nothing was selected");
        //if (!app()->hiddenApp) QMessageBox::critical( 0, "qtiSAS", "Nothing was selected");
        //else std::cout<<"ERROR :: "<<"Nothing was selected\n";
        return;
    }
    
    QString test=selectedDat[0];
    if ( test.contains(Dir) )
    {
	test=test.remove(Dir);
	if (!dirsInDir && test.contains("/") )
	{
        QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
        //if (!app()->hiddenApp) QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
        //else std::cout<<"ERROR :: "<<"Selected data not in ::Input Folder::\n";

        return;
	}
	else if (dirsInDir && test.count("/") > 1 ) 
	{
        QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder:: + sub-folders");
        //if (!app()->hiddenApp) QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder:: + sub-folders");
        //else std::cout<<"ERROR :: "<<"Selected data not in ::Input Folder:: + sub-folders\n";

	    return;
	}
    }
    else
    {
        QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
        //if (!app()->hiddenApp) QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
        //else std::cout<<"ERROR :: "<<"Selected data not in ::Input Folder::\n";

        return;
    }
    
    
    if (checkTableExistence(tableName))
    {
        //+++ Find table
        QList<MdiSubWindow *> tableList=app()->tableList();
        foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
        //+++
        
        if (w->windowLabel()!="DAN::Script::Table")
        {
            QMessageBox::critical( 0, "qtiSAS", "Table ~"+tableName+" is not script");
            //if (!app()->hiddenApp) QMessageBox::critical( 0, "qtiSAS", "Table ~"+tableName+" is not script");
            //else std::cout<<"ERROR :: "<<"Table ~"+tableName+" is not script\n";
            
            return;
        }
        
        
        if (w->numCols()<21)
        {
            QMessageBox::critical( 0, "qtiSAS", "Better, create new table (# col)");
            //if (!app()->hiddenApp) QMessageBox::critical( 0, "qtiSAS", "Better, create new table (# col)");
            //else std::cout<<"ERROR :: "<<"Better, create new table (# col)\n";
            
            return;
        }
        startRaw=w->numRows();
    }
    else
    {
	w=app()->newTable(tableName, 0,21);
	//+++ new
	w->setWindowLabel("DAN::Script::Table");
	app()->setListViewLabel(w->name(), "DAN::Script::Table");
	app()->updateWindowLists(w);
    }
    
    w->setNumRows(startRaw+filesNumber);
    
    QStringList colType;
    
    //+++
    //Col-Names
    int colNumber=0;
    
    w->setColName(colNumber,"Run-info"); 	
    w->setColPlotDesignation(colNumber,Table::None); colType<<"1";
    int indexInfo=colNumber;
    colNumber++;
    
    
    w->setColName(colNumber,"#-Run");  	
    w->setColPlotDesignation(colNumber,Table::X); 	 colType<<"1";
    int indexSA=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"#-Condition");	
    w->setColPlotDesignation(colNumber,Table::None); 	colType<<"0";
    int indexCond=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"C");   	
    w->setColPlotDesignation(colNumber,Table::None);
    w->setColComment(colNumber,"[m]"); 			colType<<"0";
    int indexC=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"D");  
    w->setColPlotDesignation(colNumber,Table::None);
    w->setColComment(colNumber,"[m]"); 			colType<<"0";
    int indexD=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Lambda"); 	
    w->setColPlotDesignation(colNumber,Table::None);
    QString sss="[";sss+=QChar(197);sss+="]";
    w->setColComment(colNumber,sss); 			colType<<"1";
    int indexLam=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Beam Size");	
    w->setColPlotDesignation(colNumber,Table::None);
    w->setColComment(colNumber,"[mm x mm]");		colType<<"1";
    int indexCA=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"#-BC");
    w->setColPlotDesignation(colNumber,Table::None);		colType<<"1";
    int indexBC=colNumber;
    colNumber++;    
    
    w->setColName(colNumber,"#-EC [EB]"); 
    w->setColPlotDesignation(colNumber,Table::None);		colType<<"1";
    int indexEC=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"#-Buffer"); 
    w->setColPlotDesignation(colNumber,Table::None);		colType<<"1";
    int indexBuffer=colNumber;
    colNumber++;
    
    
    w->setColName(colNumber,"Thickness"); 
    w->setColPlotDesignation(colNumber,Table::None);
    w->setColComment(colNumber,"[cm]"); colType<<"0";
    int indexThickness=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Transmission-Sample");
    w->setColPlotDesignation(colNumber,Table::None);        colType<<"1"; //+++ 2019 colType<<"0";
    int indexTr=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Transmission-Buffer");
    w->setColPlotDesignation(colNumber,Table::None);        colType<<"1"; //+++ 2019 colType<<"0";
    int indexTrBuffer=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Buffer-Fraction"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"0";
    int indexBufferFraction=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Factor"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"0";
    int indexFactor=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"X-center"); 
    w->setColPlotDesignation(colNumber,Table::Y);		colType<<"0";
    int indexXC=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Y-center"); 
    w->setColPlotDesignation(colNumber,Table::Y);		colType<<"0";
    int indexYC=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Mask"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"1";
    int indexMask=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Sens"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"1";
    int indexSens=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Use-Buffer-as-Sensitivity"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"1";
    int indexUseBubberAsSensLocal=colNumber;
    colNumber++;
    
    w->setColName(colNumber,"Status"); 
    w->setColPlotDesignation(colNumber,Table::None);	colType<<"1";
    int indexStatus=colNumber;
    colNumber++;
    
    //+++ Col Types
    w->setColumnTypes(colType);
    
    //+++
    int iter;
    int  C;
    QString Number;
    double M2, M3;
    double D, lambda, cps,thickness;
    QString beamSize;
    QStringList lst;
    //
    for (iter = startRaw; iter < (startRaw + filesNumber); iter++)
    {
        lst.clear();
        //+++ header
        // readHeaderFile(selectedDat[iter-startRaw], linesInHeader+linesInDataHeader, lst);
        Number = findFileNumberInFileName(wildCard, selectedDat[iter - startRaw].remove(Dir));

        int index = -1;
        if (Number.contains("["))
        {
            index = Number.right(Number.length() - Number.indexOf("[")).remove("[").remove("]").toInt();
        }
        readHeaderNumberFull(Number, lst);

        D = detector->readDinM(Number, lst);
        C = int(collimation->readCinM(Number, lst));
        lambda = readLambda(lst, index, Number);
        thickness = readThickness(lst, index, Number);
        //+++
        w->setText(iter, indexInfo, readInfo(lst, index, Number));
        //+++
        w->setText(iter, indexSA, Number);
        //+++
        w->setText(iter, indexC, QString::number(C, 'f', 0));
        //+++
        w->setText(iter, indexD, QString::number(D, 'f', 3));
        //+++
        w->setText(iter, indexLam, QString::number(lambda, 'f', 3));
        //+++
        w->setText(iter, indexCA, collimation->readCA(Number, lst) + "|" + collimation->readSA(Number, lst));
        //+++
        w->setText(iter, indexThickness, QString::number(thickness, 'f', 3));
    }

    // check CD conditions
    int Ncond, iC;
    iMax=tableEC->columnCount();
    //+++ recalculate old files
    int startCalc=startRaw;
    if (checkBoxRecalculate->isChecked()) startCalc=0;
    
    for(iter=startCalc; iter<(startRaw+filesNumber);iter++)
    {
	Number=w->text(iter,indexSA);
	if (Number!="")
	{
	    Ncond=-1;
	    C=w->text( iter, indexC ).toInt();
	    D=w->text( iter, indexD ).toDouble();
	    lambda=w->text( iter, indexLam ).toDouble();
	    beamSize=w->text( iter, indexCA);
	    
	    for(iC=iMax-1; iC>=0;iC--)
	    {
		bool condLambda=lambda>(0.95*tableEC->item(dptWL, iC)->text().toDouble() ) &&
				lambda<(1.05*tableEC->item(dptWL,iC)->text().toDouble() );
		bool condD=D>(0.95*tableEC->item(dptD,iC)->text().toDouble()) &&
			   D<(1.05*tableEC->item(dptD,iC)->text().toDouble());
		
		bool condSample=compareSamplePositions( Number, tableEC->item(dptEC,iC)->text() );
		
		bool attenuatorCompare=compareAttenuators( Number, tableEC->item(dptEC,iC)->text() );
		
		bool beamPosCompare=compareBeamPosition( Number, tableEC->item(dptEC,iC)->text() );
		
		bool polarizationCompare=comparePolarization( Number, tableEC->item(dptEC,iC)->text() );
		
        bool detAngleCompare=compareDetAnglePosition( Number, tableEC->item(dptEC,iC)->text() );
            
		if (C==tableEC->item(dptC,iC)->text().toInt() && condD && condLambda && condSample && polarizationCompare &&
		    beamSize==tableEC->item(dptBSIZE,iC)->text() && attenuatorCompare && beamPosCompare && detAngleCompare) Ncond=iC;
	    }
	    
	    w->setText(iter,indexCond,QString::number(Ncond+1));
	    
	    if (Ncond>=0)
	    {
		w->setText(iter,indexEC,tableEC->item(dptEC,Ncond)->text());
		w->setText(iter,indexBC,tableEC->item(dptBC,Ncond)->text());
		
		w->setText(iter,indexFactor,QString::number(tableEC->item(dptACFAC,Ncond)->text().toDouble()));
		w->setText(iter,indexBufferFraction,"0.000");
		w->setText(iter,indexTrBuffer,"1.000");	
		w->setText(iter,indexUseBubberAsSensLocal,"no");	
		
		s=tableEC->item(dptCENTERX,Ncond)->text();
		w->setText(iter,indexXC,QString::number(s.left(6).toDouble()));
		s=tableEC->item(dptCENTERY,Ncond)->text();
		w->setText(iter,indexYC,QString::number(s.left(6).toDouble()));
		
		w->setText(iter,indexMask,((QComboBoxInTable*)tableEC->cellWidget(dptMASK,Ncond))->currentText());
		w->setText(iter,indexSens,((QComboBoxInTable*)tableEC->cellWidget(dptSENS,Ncond))->currentText());
	    }
	    else
	    {
		w->setText(iter,indexEC,"");
		w->setText(iter,indexBC,"");
		w->setText(iter,indexTr,"1.000");
		w->setText(iter,indexTrBuffer,"1.000");		
		w->setText(iter,indexFactor,"1.000");
		w->setText(iter,indexBufferFraction,"0.000");
		w->setText(iter,indexXC,QString::number((MD+1.0)/2.0, 'f',3));
		w->setText(iter,indexYC,QString::number((MD+1.0)/2.0, 'f',3));
		w->setText(iter,indexMask,"mask");
		w->setText(iter,indexSens,"sens");	
		w->setText(iter,indexUseBubberAsSensLocal,"no");	
	    }
	}
	
    }
    
    
    // +++ Calculate Center for every File
    if (checkBoxFindCenter->isChecked())
    {
	QString sampleFile, D;
	double Xc, Yc,XcErr, YcErr;
	
	for(iter=startCalc; iter<(startRaw+filesNumber);iter++)
	{
	    Xc=w->text(iter,indexXC).toDouble();
	    Yc=w->text(iter,indexYC).toDouble();
	    
	    sampleFile=w->text(iter,indexSA);
	    D=w->text(iter,indexD);
	    
	    QString maskName=comboBoxMaskFor->currentText();
	    QString sensName=comboBoxSensFor->currentText();
	    
	    maskName=w->text(iter,indexMask);
	    sensName=w->text(iter,indexSens);
	    calcCenterNew(sampleFile, -1, Xc, Yc,XcErr, YcErr,maskName,sensName);
	    w->setText(iter,indexXC, QString::number(Xc,'f',3));
	    w->setText(iter,indexYC, QString::number(Yc,'f',3));
	}
    }
    
    //findSettingTables();
    QStringList  list;
    findTableListByLabel("DAN::Script::Table",list);
    //    list.prepend("New-Script-Table");
    comboBoxMakeScriptTable->clear();
    comboBoxMakeScriptTable->addItems(list);
    comboBoxMakeScriptTable->setItemText(comboBoxMakeScriptTable->currentIndex(), tableName);
    
    // adjust columns
    for (int tt=0; tt<w->numCols(); tt++) 
    {
	w->table()->resizeColumnToContents(tt);
	w->table()->setColumnWidth(tt, w->table()->columnWidth(tt)+10); 
	
    }
    
    saveSettings(tableName+"-Settings");
    maximizeWindow(tableName);
    
    
    //+++  Subtract Bufffer
    bool subtractBuffer= false;
    if (comboBoxMode->currentText().contains("(BS)")) subtractBuffer=true;
    
    if (subtractBuffer)
    {
	w->table()->showColumn(indexBuffer);
	w->table()->resizeColumnToContents(indexBuffer);
	w->table()->setColumnWidth(indexBuffer, w->table()->columnWidth(indexBuffer)+10); 
	
	w->table()->showColumn(indexTrBuffer);	
	w->table()->resizeColumnToContents(indexTrBuffer);
	w->table()->setColumnWidth(indexTrBuffer, w->table()->columnWidth(indexTrBuffer)+10); 
	
	w->table()->showColumn(indexBufferFraction);	
	w->table()->resizeColumnToContents(indexBufferFraction);
	w->table()->setColumnWidth(indexBufferFraction, w->table()->columnWidth(indexBufferFraction)+10); 
	
	w->table()->showColumn(indexUseBubberAsSensLocal);	
	w->table()->resizeColumnToContents(indexUseBubberAsSensLocal);
	w->table()->setColumnWidth(indexUseBubberAsSensLocal,w->table()->columnWidth(indexUseBubberAsSensLocal)+10); 
	
	
    }
    else 
    {
	w->table()->hideColumn(indexBuffer);
	w->table()->hideColumn(indexTrBuffer);	
	w->table()->hideColumn(indexBufferFraction);
	w->table()->hideColumn(indexUseBubberAsSensLocal);	
    }
    
    w->table()->showColumn(indexMask);
    w->table()->resizeColumnToContents(indexMask);
    w->table()->setColumnWidth(indexMask, w->table()->columnWidth(indexMask)+10); 
    
    w->table()->showColumn(indexSens);
    w->table()->resizeColumnToContents(indexSens);
    w->table()->setColumnWidth(indexSens, w->table()->columnWidth(indexSens)+10); 
    
    //+++ Calculation of transmissions for Active Configurations
    calculateTransmission(startCalc);
    //+++ Copy/Paste of transmissions for non-Active Configurations
    if (checkBoxForceCopyPaste->isChecked()) copyCorrespondentTransmissions(startCalc);
    
}

void dan18::saveSettingsSlot()
{
    //+++
    QString activeTable=comboBoxMakeScriptTable->currentText();
    if (activeTable=="") return;
    
    activeTable+="-Settings";
    
    //+++
    saveSettings( activeTable);
}

//+++
void dan18::saveSettings(QString tableName)
{
    ImportantConstants();

    Table *w;
    int i; 
    if (checkTableExistence(tableName))
    {
        //+++ Find table
        QList<MdiSubWindow *> tableList=app()->tableList();
        foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
        //+++
        
        if (w->windowLabel()!="DAN::Settings::Table")
        {
            QMessageBox::critical( 0, "qtiSAS", "Table "+tableName+" is not settings table");
            return;
        }
        
        
        if (w->numCols()<2)
        {
            QMessageBox::critical( 0, "qtiSAS", "Better, create new settings table");
            return;
        }
    }
    else
    {
	if (checkBoxSortOutputToFolders->isChecked())
	{
	    app()->changeFolder("DAN :: script, info, ...");
	}
	
	w=app()->newHiddenTable(tableName,"DAN::Settings::Table", 0, 2);
	//->newHiddenTable(tableName, 0,2);
	//+++ new
	w->setWindowLabel("DAN::Settings::Table");
	app()->setListViewLabel(w->name(), "DAN::Settings::Table");
	app()->updateWindowLists(w);

	
	//Col-Names
	QStringList colType;
	w->setColName(0,"Parameter"); w->setColPlotDesignation(0,Table::None);colType<<"1";
	w->setColName(1,"Parameter-Value"); w->setColPlotDesignation(1,Table::None);colType<<"1";
	w->setColumnTypes(colType);
    }
    
    int currentRow=0;
    QString s;
    
    //----- input path
    w->setNumRows(currentRow+1);
    s=Dir+" <";
    w->setText(currentRow, 0, "Settings::Path::Input");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Input Filter
    w->setNumRows(currentRow+1);
    s=textEditPattern->text()+" <";
    w->setText(currentRow, 0, "Settings::Input::Filter");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Settings::Path::DirsInDir
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Settings::Path::DirsInDir");    
    if (checkBoxDirsIndir->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");
    currentRow++;
    
    //----- output path
    w->setNumRows(currentRow+1);
    s=lineEditPathRAD->text()+" <";
    w->setText(currentRow, 0, "Settings::Path::Output");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //--- Mask::Edges
    w->setNumRows(currentRow+1);
    s=QString::number(spinBoxLTx->value());
    s=s+" "+QString::number(spinBoxLTy->value());
    s=s+" "+QString::number(spinBoxRBx->value());
    s=s+" "+QString::number(spinBoxRBy->value());
    w->setText(currentRow,0,"Mask::Edges");
    w->setText(currentRow,1,s+" <");	
    currentRow++;
    
    //--- Mask::BeamStop
    w->setNumRows(currentRow+1);    
    s=QString::number(spinBoxLTxBS->value());
    s=s+" "+QString::number(spinBoxLTyBS->value());
    s=s+" "+QString::number(spinBoxRBxBS->value());
    s=s+" "+QString::number(spinBoxRByBS->value());
    w->setText(currentRow,0,"Mask::BeamStop");
    w->setText(currentRow,1,s+" <");	
    currentRow++;
    
    //----- Mask::Edges::Shape 
    w->setNumRows(currentRow+1);
    s=comboBoxMaskEdgeShape->currentText()+" <";
    w->setText(currentRow, 0, "Mask::Edges::Shape");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Mask::BeamStop::Shape 
    w->setNumRows(currentRow+1);
    s=comboBoxMaskBeamstopShape->currentText()+" <";
    w->setText(currentRow, 0, "Mask::BeamStop::Shape");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Mask::Dead::Rows 
    w->setNumRows(currentRow+1);
    s=lineEditDeadRows->text()+" <";
    w->setText(currentRow, 0, "Mask::Dead::Rows");
    w->setText(currentRow, 1, s);
    currentRow++;    
    
    //----- Mask::Dead::Cols 
    w->setNumRows(currentRow+1);
    s=lineEditDeadCols->text()+" <";
    w->setText(currentRow, 0, "Mask::Dead::Cols");
    w->setText(currentRow, 1, s);
    currentRow++;    
    
    //----- Mask::Triangular 
    w->setNumRows(currentRow+1);
    s=lineEditMaskPolygons->text()+" <";
    w->setText(currentRow, 0, "Mask::Triangular");
    w->setText(currentRow, 1, s);
    currentRow++;      
    
    //---- Sensitivity:.numbers
    w->setNumRows(currentRow+1);    
    if (lineEditPlexiAnyD->text()=="") s="0"; else s=lineEditPlexiAnyD->text();
    if (lineEditEBAnyD->text()=="") s=s+" 0"; else s=s+" "+lineEditEBAnyD->text();
    if (lineEditBcAnyD->text()=="") s=s+" 0"; else s=s+" "+lineEditBcAnyD->text();
    if (lineEditTransAnyD->text()=="") s=s+" 0"; else s=s+" "+lineEditTransAnyD->text();
    
    w->setText(currentRow,0,"Sensitivity::Numbers");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    
    //---- Sensitivity:.Corection
    w->setNumRows(currentRow+1);    
    w->setText(currentRow,0,"Sensitivity::Error::Range");
    
    s=QString::number(spinBoxErrLeftLimit->value());
    s=s+" "+QString::number(spinBoxErrRightLimit->value());
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Sensitivity::Error::Matrix
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Sensitivity::Error::Matrix");
    if (checkBoxSensError->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Sensitivity::Tr::Option
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Sensitivity::Tr::Option");
    if (checkBoxSensTr->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;	
    
    //+++ Processing::Conditions::Number
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Processing::Conditions::Number");
    w->setText(currentRow,1,QString::number(sliderConfigurations->value())+" <");
    int iMax=sliderConfigurations->value();
    currentRow++;
    
    //+++ Processing::EC
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptEC,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptEC,i)->text();
    }
    
    w->setText(currentRow,0,"Processing::EC");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::BC
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptBC,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptBC,i)->text();
    }
    
    w->setText(currentRow,0,"Processing::BC");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::EB
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptEB,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptEB,i)->text();
    }
    
    w->setText(currentRow,0,"Processing::EB");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::C[m]
    w->setNumRows(currentRow+1);
    s=QString::number(tableEC->item(dptC,0)->text().toInt());
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptC,i)->text().toInt());
    w->setText(currentRow,0,"Processing::C[m]");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::D[m]
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptD,0)->text().toDouble(),'f',3);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptD,i)->text().toDouble(),'f',3);
    
    w->setText(currentRow,0,"Processing::D[m]");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::Lambda[m]
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptWL,0)->text().toDouble(),'f',3);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptWL,i)->text().toDouble(),'f',3);
    
    w->setText(currentRow,0,"Processing::Lambda[A]");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::Beam::Size
    w->setNumRows(currentRow+1);            
    s=tableEC->item(dptBSIZE,0)->text();
    if (s=="") s="00x00|00x00";
    for (i=1; i<iMax;i++) 
    {
	if (tableEC->item(dptBSIZE,i)->text()!="") s=s+" "+tableEC->item(dptBSIZE,i)->text();
	else s=s+" 00x00|00x00";
    }
    w->setText(currentRow,0,"Processing::Beam::Size");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::Transm::EC
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptECTR,0)->text().toDouble(),'f',4);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptECTR,i)->text().toDouble(),'f',4);
    
    w->setText(currentRow,0,"Processing::Transm::EC");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::Transm::EC::Activity
    w->setNumRows(currentRow+1);            
    s="";
    for (i=0; i<iMax;i++) if (tableEC->item(dptECTR, i)->checkState() == Qt::Checked) s+="1 "; else s+="0 ";
    
    w->setText(currentRow,0,"Processing::Transm::EC::Activity");
    w->setText(currentRow,1,s+"<");
    currentRow++;    
    
    //+++ Processing::Plexi::Plexi
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptACFS,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptACFS,i)->text();
    }
    
    w->setText(currentRow,0,"Processing::Plexi::Plexi");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::Plexi::EB
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptACEB,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptACEB,i)->text();
    }
    
    w->setText(currentRow,0,"Processing::Plexi::EB");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::Plexi::BC
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptACBC,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptACBC,i)->text();
    }
    w->setText(currentRow,0,"Processing::Plexi::BC");
    w->setText(currentRow,1,s+" <");
    currentRow++;        
    
    //+++ Processing::AC::DAC
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptDAC,0)->text().toDouble(),'f',3);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptDAC,i)->text().toDouble(),'f',3);
    
    w->setText(currentRow,0,"Processing::AC::DAC");
    w->setText(currentRow,1,s+" <");
    currentRow++; 
    
    //+++ Processing::AC::MU
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptACMU,0)->text().toDouble(),'E',4);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptACMU,i)->text().toDouble(),'E',4);
    
    w->setText(currentRow,0,"Processing::AC::MU");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::AC::TR
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptACTR,0)->text().toDouble(),'f',4);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptACTR,i)->text().toDouble(),'f',4);
    
    w->setText(currentRow,0,"Processing::AC::TR");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::AC::Factor
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptACFAC,0)->text().toDouble(),'E',4);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptACFAC,i)->text().toDouble(),'E',4);
    
    w->setText(currentRow,0,"Processing::AC::Factor");
    w->setText(currentRow,1,s+" <");
    currentRow++;        
    
    //+++ Processing::Center::File
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptCENTER,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptCENTER,i)->text();
    } 
    w->setText(currentRow,0,"Processing::Center::File");
    w->setText(currentRow,1,s+" <");
    currentRow++;        
    
    //+++ Processing::X-center
    w->setNumRows(currentRow+1);            
    s=tableEC->item(dptCENTERX,0)->text();
    if (s=="") s=QString::number((MD+1.0)/2.0,'f',3)+QChar(177)+"10.0";
    for (i=1; i<iMax;i++) 
    {
	if(tableEC->item(dptCENTERX,i)->text()!="") s=s+" "+tableEC->item(dptCENTERX,i)->text();
	else s=s+" "+QString::number((MD+1.0)/2.0,'f',3)+QChar(177)+"10.0";
    }
    w->setText(currentRow,0,"Processing::X-center");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::Y-center
    w->setNumRows(currentRow+1);            
    s=tableEC->item(dptCENTERY,0)->text();
    if (s=="") s=QString::number((MD+1.0)/2.0,'f',3)+QChar(177)+"10.0";
    for (i=1; i<iMax;i++) 
    {
	if(tableEC->item(dptCENTERY,i)->text()!="") s=s+" "+tableEC->item(dptCENTERY,i)->text();
	else s=s+" "+QString::number((MD+1.0)/2.0,'f',3)+QChar(177)+"10.0";
    }
    w->setText(currentRow,0,"Processing::Y-center");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::Mask
    w->setNumRows(currentRow+1);            
    s=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,0))->currentText();
    for (i=1; i<iMax;i++) s=s+" "+((QComboBoxInTable*)tableEC->cellWidget(dptMASK,i))->currentText();
    
    w->setText(currentRow,0,"Processing::Mask");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::Sens
    w->setNumRows(currentRow+1);            
    s=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,0))->currentText();
    for (i=1; i<iMax;i++) s=s+" "+((QComboBoxInTable*)tableEC->cellWidget(dptSENS,i))->currentText();
    
    w->setText(currentRow,0,"Processing::Sensitivity");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::Tr::Mask
    w->setNumRows(currentRow+1);            
    s=((QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,0))->currentText();
    for (i=1; i<iMax;i++) s=s+" "+((QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,i))->currentText();
    
    w->setText(currentRow,0,"Processing::Tr::Mask");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::File::Ext
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Processing::File::Ext");
    s=lineEditFileExt->text()+" <";
    w->setText(currentRow, 1, s);
    currentRow++;  
    
    //+++ Options::Instrument
    w->setNumRows(currentRow+1);            
    s=comboBoxSel->currentText();
    w->setText(currentRow,0,"Options::Instrument");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::Mode
    w->setNumRows(currentRow+1);            
    s=comboBoxMode->currentText();
    w->setText(currentRow,0,"Options::Mode");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::Instrument::DeadTime::Homogenity
    w->setNumRows(currentRow+1);            
    s=comboBoxDTtype->currentText();
    w->setText(currentRow,0,"Options::Instrument::DeadTime::Homogenity");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    
    //+++ Options::Calibrant::Type
    w->setNumRows(currentRow+1);            
    w->setText(currentRow,0,"Options::Calibrant::Type");    
    s=comboBoxACmethod->currentText();
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::Calibrant::Active::Mask::Sens
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Calibrant::Active::Mask::Sens");
    if (checkBoxACDBuseActive->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::Calibrant
    w->setNumRows(currentRow+1);            
    s=comboBoxCalibrant->currentText();
    w->setText(currentRow,0,"Options::Calibrant");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::Calibrant::CalculateTr
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Calibrant::CalculateTr");
    if (checkBoxTransmissionPlexi->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::Calibrant::MU
    w->setNumRows(currentRow+1);            
    s=lineEditMuY0->text();
    s+=" "+lineEditMuA->text();
    s+=" "+lineEditMut->text();
    w->setText(currentRow,0,"Options::Calibrant::MU");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::Calibrant::TR
    w->setNumRows(currentRow+1);            
    s=lineEditTo->text();
    s+=" "+lineEditTA->text();
    s+=" "+lineEditLate->text();
    w->setText(currentRow,0,"Options::Calibrant::TR");		    
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::2D::Normalization
    w->setNumRows(currentRow+1);            
    s=comboBoxNorm->currentText();
    w->setText(currentRow,0,"Options::2D::Normalization");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::2D::Normalization::Constant
    w->setNumRows(currentRow+1);            
    s=QString::number(spinBoxNorm->value());
    w->setText(currentRow,0,"Options::2D::Normalization::Constant");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::2D::Normalization::BC::Normalization
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::Normalization::BC::Normalization");
    if (checkBoxBCTimeNormalization->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::2D::xyDimension::Pixel
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::xyDimension::Pixel");
    if (radioButtonXYdimPixel->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;    
    
    //--- Options::2D::RemoveNegativePoints::2D
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::RemoveNegativePoints::2D");
    if (checkBoxMaskNegative->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::2D::RemoveNegativePoints::1D
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::RemoveNegativePoints::1D");
    if (checkBoxMaskNegativeQ->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::2D::Polar::Resolusion
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::Polar::Resolusion");
    w->setText(currentRow,1,QString::number(spinBoxPolar->value())+" <");
    currentRow++;
    
    //--- Options::2D::HighQcorrection
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::HighQcorrection");
    if (checkBoxParallax->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::2D::HighQtype
    w->setNumRows(currentRow+1);
    s=comboBoxParallax->currentText();
    w->setText(currentRow,0,"Options::2D::HighQtype");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::2D::HighQtransmission
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::2D::HighQtransmission");
    if (checkBoxParallaxTr->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;

    //--- Options::2D::HighQwindow
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::2D::HighQwindow");
    if (checkBoxWaTrDet->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //--- Options::2D::DeadTimeModel
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::DeadTimeModel");
    if (radioButtonDeadTimeCh->isChecked())
	w->setText(currentRow,1,"Non-Paralysable <");
    else
	w->setText(currentRow,1,"Paralysable <");       
    currentRow++;
    
    //--- Options::2D::FindCenterMethod
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::FindCenterMethod");
    if (radioButtonRadStdSymm->isChecked())
	w->setText(currentRow,1,"X and Y symmetrization <");
    else if (radioButtonCenterHF->isChecked())
	w->setText(currentRow,1,"Moment-minimalization (H.F.) <");       
    else w->setText(currentRow,1,"Header <");       
    currentRow++;
    
    //--- Options::2D::OutputFormat
    w->setNumRows(currentRow+1);            
    s=QString::number(comboBoxIxyFormat->currentIndex());
    w->setText(currentRow,0,"Options::2D::OutputFormat");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::2D::HeaderOutputFormat
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::HeaderOutputFormat");
    if (checkBoxASCIIheaderIxy->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;

    //--- Options::2D::HeaderSASVIEW
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::2D::HeaderSASVIEW");
    if (checkBoxASCIIheaderSASVIEW->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Options::1D::SASpresentation
    w->setNumRows(currentRow+1);            
    s=comboBoxSelectPresentation->currentText();
    w->setText(currentRow,0,"Options::1D::SASpresentation");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::1D::I[Q]::Format
    w->setNumRows(currentRow+1);            
    s=comboBox4thCol->currentText();
    w->setText(currentRow,0,"Options::1D::I[Q]::Format");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::1D::I[Q]::PlusHeader
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::I[Q]::PlusHeader");
    if (checkBoxASCIIheader->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Options::1D::I[Q]::Anisotropy
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::I[Q]::Anisotropy");
    if (checkBoxAnisotropy->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Options::1D::I[Q]::AnisotropyAngle
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::I[Q]::AnisotropyAngle");
    s=QString::number(spinBoxAnisotropyOffset->value())+" <";
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //+++Options::1D::TransmissionMethod
    w->setNumRows(currentRow+1);            
    s=comboBoxTransmMethod->currentText();
    w->setText(currentRow,0,"Options::1D::TransmissionMethod");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    
    //+++ Options::1D::Slices
    w->setNumRows(currentRow+1);            
    s=QString::number(spinBoxFrom->value())+" ";
    s+=QString::number(spinBoxTo->value());
    w->setText(currentRow,0,"Options::1D::Slices");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::1D::SlicesBS
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::SlicesBS");
    if (checkBoxSlicesBS->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Options::1D::SkipPoins
    w->setNumRows(currentRow+1);            
    s=QString::number(spinBoxRemoveFirst->value())+" ";
    s+=QString::number(spinBoxRemoveLast->value());
    w->setText(currentRow,0,"Options::1D::SkipPoins");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::1D::I[Q]::Method
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::1D::I[Q]::Method");
    if (radioButtonRadHF->isChecked())
	w->setText(currentRow,1,"4-Pixel Interpolation (H.F.) <");
    else
	w->setText(currentRow,1,"Standard (Shells) <");       
    currentRow++;
    
    //+++ Options::1D::I[Q]::LinearFactor
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::I[Q]::LinearFactor");
    s=QString::number(spinBoxAvlinear->value())+" <";
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //+++ Options::1D::I[Q]::ProgressiveFactor
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::I[Q]::ProgressiveFactor");
    s=QString::number(doubleSpinBoxAvLog->value())+" <";
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //--- Options::ScriptTable::RecalculateOldFiles
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::RecalculateOldFiles");
    if (checkBoxRecalculate->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::UseSamplePositionAsParameter
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::UseSamplePositionAsParameter");
    if (checkBoxRecalculateUseNumber->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::UseAttenuatorAsParameter
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::UseAttenuatorAsParameter");
    if (checkBoxAttenuatorAsPara->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::BeamCenterAsCondition
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::BeamCenterAsCondition");
    if (checkBoxBeamcenterAsPara->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::PolarizationAsCondition
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::PolarizationAsCondition");
    if (checkBoxPolarizationAsPara->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::DetectorAngleAsCondition
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::ScriptTable::DetectorAngleAsCondition");
    if (checkBoxDetRotAsPara->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;

    //--- Options::ScriptTable::FindCenterEveryFile
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::FindCenterEveryFile");
    if (checkBoxFindCenter->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::MergingTemplate
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::MergingTemplate");
    if (checkBoxMergingTable->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::AutoMerging
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::ScriptTable::AutoMerging");
    if (checkBoxAutoMerging->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Options::Overlap::Merging
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::Overlap::Merging");
    s=QString::number(spinBoxOverlap->value())+" <";
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //+++ Options::Rewrite Output (No index)
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::RewriteOutput");
    if (checkBoxRewriteOutput->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::ScriptTable::Transmission::ForceCopyPaste
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::Transmission::ForceCopyPaste");
    if (checkBoxForceCopyPaste->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::ScriptTable::LabelAsName
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::LabelAsName");
    if (checkBoxNameAsTableName->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    
    //+++ Options::ScriptTable::Transmission::SkiptTrConfigurations
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::Transmission::SkiptTrConfigurations");
    if (checkBoxSkiptransmisionConfigurations->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::ScriptTable::Output::Folders
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::Output::Folders");
    if (checkBoxSortOutputToFolders->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::Resolusion::Focusing
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Resolusion::Focusing");
    if (checkBoxResoFocus->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;    
    
    //+++ Options::Resolusion::CA::Round
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Resolusion::CA::Round");
    if (checkBoxResoCAround->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::Resolusion::SA::Round
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Resolusion::SA::Round");
    if (checkBoxResoSAround->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    
    //+++ Options::Resolusion::Detector
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Resolusion::Detector");
    s=lineEditDetReso->text()+" <";
    w->setText(currentRow, 1, s);
    currentRow++;  
    
    
    for (int tt=0; tt<w->numCols(); tt++) 
    {
	w->table()->resizeColumnToContents(tt);
	w->table()->setColumnWidth(tt, w->table()->columnWidth(tt)+10); 
    }
}

void dan18::SetColNumberNew( int cols )
{

    int oldCols=tableEC->columnCount();
    if (cols==oldCols) return;

    tableEC->setColumnCount(cols);
    for (int i=oldCols; i<cols; i++) tableEC->setHorizontalHeaderItem(i,new QTableWidgetItem(QString::number(i+1)));
    for(int i=oldCols;i<cols;i++) for (int ii=0; ii<tableEC->rowCount();ii++) tableEC->setItem(ii, i, new QTableWidgetItem);
    
    int spaceNew = buttonGroupMode0->width()-30 - tableEC->verticalHeader()->width() - tableEC->verticalScrollBar()->width()-1;
    
    tableEC->horizontalHeader()->setDefaultSectionSize(int(spaceNew/cols));
    tableEC->horizontalHeader()->setStretchLastSection(true);
    
    addMaskAndSens(cols, oldCols);
}


void dan18::activeScriptTableSelected(int newLine)
{
    QString newActiveScript=comboBoxMakeScriptTable->currentText();
    
    readSettingNew(newActiveScript+"-Settings");
    SensitivityLineEditCheck();
}

//*******************************************
//+++  Read Settings from Table
//*******************************************
bool dan18::readSettingNew(QString tableName )
{
    if (!checkTableExistence(tableName))
    {
	QMessageBox::critical(0, "qtiSAS", "Table ~"+tableName+"~ does not exist!!! <br><h4>");
	return false;
    }
    //+++
    int i;
    Table* w;
    QStringList parameters;
    QString s;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
    //+++
    
    for (i=0;i<w->numRows();i++) parameters<<w->text(i,0);
    
    //+++ Sensitivity::Number
    if (parameters.indexOf("Sensitivity::Numbers") >= 0)
    {
        s = w->text(parameters.indexOf("Sensitivity::Numbers"), 1);
        s = s.remove(" <").simplified();
        QStringList lst;
        lst = s.split(" ", QString::SkipEmptyParts);

        if (lst[0] == "0")
            lineEditPlexiAnyD->setText("");
        else
            lineEditPlexiAnyD->setText(lst[0]);
        if (lst[1] == "0")
            lineEditEBAnyD->setText("");
        else
            lineEditEBAnyD->setText(lst[1]);
        if (lst[2] == "0")
            lineEditBcAnyD->setText("");
        else
            lineEditBcAnyD->setText(lst[2]);

        lineEditTransAnyD->setText(QString::number(lst[3].toDouble(), 'f', 4));
        SensitivityLineEditCheck();
    }

    //+++ Options::Instrument
    if (parameters.indexOf("Options::Instrument") >= 0)
    {
        QString newInstr = w->text(parameters.indexOf("Options::Instrument"), 1).remove(" <").trimmed();
        bool exist = false;

        if (comboBoxSel->findText(newInstr) >= 0)
        {
            comboBoxSel->setCurrentIndex(comboBoxSel->findText(newInstr));
            instrumentSelected();
            exist = true;
        }

        if (!exist)
            QMessageBox::critical(nullptr, "qtiSAS",
                                  "SA(N)S instrument " + newInstr + " does not exist. Create/Copy first!!!");
    }

    //+++ Options::Mode
    if (parameters.indexOf("Options::Mode")>=0) 
    {
	QString newMode=w->text(parameters.indexOf("Options::Mode"),1).remove(" <");
	
	
	for (int i=0; i<comboBoxMode->count();i++) 
	{
	    if (comboBoxMode->itemText(i)==newMode)
	    {
		comboBoxMode->setCurrentIndex(i);
		break;
	    }
	}
    }
    
    //+++ Settings::Path::Input
    if (parameters.indexOf("Settings::Path::Input")>=0) 
    {
	lineEditPathDAT->setText(w->text(parameters.indexOf("Settings::Path::Input"),1).remove(" <"));
    } 
    
    //+++ Settings::Input::Filter
    if (parameters.indexOf("Settings::Input::Filter")>=0) 
    {
	textEditPattern->setText(w->text(parameters.indexOf("Settings::Input::Filter"),1).remove(" <"));
    }
    
    //+++ Settings::Path::DirsInDir
    if (parameters.indexOf("Settings::Path::DirsInDir")>=0) 
    {
	s=w->text(parameters.indexOf("Settings::Path::DirsInDir"),1);
	if (s.contains("yes")) checkBoxDirsIndir->setChecked(true); else checkBoxDirsIndir->setChecked(false);
    }
    
    //+++ Settings::Path::Output
    if (parameters.indexOf("Settings::Path::Output")>=0) 
    {
	lineEditPathRAD->setText(w->text(parameters.indexOf("Settings::Path::Output"),1).remove(" <"));
    }
    
    //+++ Mask::Edges
    if (parameters.indexOf("Mask::Edges")>=0) 
    {
	s=w->text(parameters.indexOf("Mask::Edges"),1);
	
	QRegExp rx( "(\\d+)" );
	
	int pos=0;
	pos = rx.indexIn( s, pos ); pos+=rx.matchedLength();  spinBoxLTx->setValue(rx.cap( 1 ).toInt());
	pos = rx.indexIn( s, pos ); pos+=rx.matchedLength(); spinBoxLTy->setValue(rx.cap( 1 ).toInt());
	pos = rx.indexIn( s, pos ); pos+=rx.matchedLength(); spinBoxRBx->setValue(rx.cap( 1 ).toInt());
	pos = rx.indexIn( s, pos ); pos+=rx.matchedLength(); spinBoxRBy->setValue(rx.cap( 1 ).toInt());
    }
    
    //+++ Mask::Edges::Shape    
    if (parameters.indexOf("Mask::Edges::Shape")>=0) 
    {
	s=w->text(parameters.indexOf("Mask::Edges::Shape"),1);
	
	if (s.contains("Rectangle")) comboBoxMaskEdgeShape->setCurrentIndex(0);
	else comboBoxMaskEdgeShape->setCurrentIndex(1);
    }
    
    //+++ Mmask::BeamStop
    if (parameters.indexOf("Mask::BeamStop")>=0) 
    {
	s=w->text(parameters.indexOf("Mask::BeamStop"),1);
	QRegExp rx( "(\\d+)" );
	
	int pos=0;
	pos = rx.indexIn( s, pos ); pos+=rx.matchedLength();  spinBoxLTxBS->setValue(rx.cap( 1 ).toInt());
	pos = rx.indexIn( s, pos ); pos+=rx.matchedLength(); spinBoxLTyBS->setValue(rx.cap( 1 ).toInt());
	pos = rx.indexIn( s, pos ); pos+=rx.matchedLength(); spinBoxRBxBS->setValue(rx.cap( 1 ).toInt());
	pos = rx.indexIn( s, pos ); pos+=rx.matchedLength(); spinBoxRByBS->setValue(rx.cap( 1 ).toInt());
    }
    
    //+++ Mask::BeamStop::Shape    
    if (parameters.indexOf("Mask::BeamStop::Shape")>=0) 
    {
	s=w->text(parameters.indexOf("Mask::BeamStop::Shape"),1);
	
	if (s.contains("Rectangle")) comboBoxMaskBeamstopShape->setCurrentIndex(0);
	else comboBoxMaskBeamstopShape->setCurrentIndex(1);
    }
    
    //+++ Mask::Dead::Rows 
    if (parameters.indexOf("Mask::Dead::Rows")>=0) 
    {
	s=w->text(parameters.indexOf("Mask::Dead::Rows"),1).remove(" <");
	lineEditDeadRows->setText(s);
    }    
    
    //+++ Mask::Dead::Cols 
    if (parameters.indexOf("Mask::Dead::Cols")>=0) 
    {
	s=w->text(parameters.indexOf("Mask::Dead::Cols"),1).remove(" <");
	lineEditDeadCols->setText(s);
    }
    
    //+++ Mask::Triangular
    if (parameters.indexOf("Mask::Triangular")>=0) 
    {
	s=w->text(parameters.indexOf("Mask::Triangular"),1).remove(" <");
	lineEditMaskPolygons->setText(s);
    }    
    
    //+++ Sensitivity::Error::Range
    if (parameters.indexOf("Sensitivity::Error::Range")>=0) 
    {
	s=w->text(parameters.indexOf("Sensitivity::Error::Range"),1);
	QRegExp rx( "(\\d*\\.\\d+)" ); 
	
	int pos=0;
	pos = rx.indexIn( s, pos ); pos+=rx.matchedLength();  spinBoxErrLeftLimit->setValue(rx.cap( 1 ).toDouble());
	pos = rx.indexIn( s, pos ); pos+=rx.matchedLength();  spinBoxErrRightLimit->setValue(rx.cap( 1 ).toDouble());
    }
    //+++ Sensitivity::Error::Matrix
    if (parameters.indexOf("Sensitivity::Error::Matrix")>=0) 
    {
	s=w->text(parameters.indexOf("Sensitivity::Error::Matrix"),1);
	if (s.contains("yes")) checkBoxSensError->setChecked(true); else checkBoxSensError->setChecked(false);
    }
    //+++ Sensitivity::Tr::Option
    if (parameters.indexOf("Sensitivity::Tr::Option")>=0) 
    {
	s=w->text(parameters.indexOf("Sensitivity::Tr::Option"),1);
	if (s.contains("yes")) checkBoxSensTr->setChecked(true); else checkBoxSensTr->setChecked(false);
    }    
    
    int imax;
    
    //+++ Processing::Conditions::Number
    if (parameters.indexOf("Processing::Conditions::Number")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Conditions::Number"),1);
	
	imax=s.remove(" <").trimmed().toInt();

	sliderConfigurations->setValue(imax);
    }
    
    //+++ Processing::EC
    if (parameters.indexOf("Processing::EC")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::EC"),1);  
	s=s.remove(" <").simplified();
	QStringList lst;
	lst = s.split(" ", QString::SkipEmptyParts );
	
	for (i=0; i<imax;i++) 
	{
	    if (lst[i]=="0") tableEC->item(dptEC,i)->setText(""); else tableEC->item(dptEC,i)->setText(lst[i]);
	}
    }	
    
    //+++ Processing::BC
    if (parameters.indexOf("Processing::BC")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::BC"),1);  
	s=s.remove(" <").simplified();
	QStringList lst;
	lst = s.split(" ", QString::SkipEmptyParts);
	
	for (i=0; i<imax;i++) 
	{
	    if (lst[i]=="0") tableEC->item(dptBC,i)->setText(""); else tableEC->item(dptBC,i)->setText(lst[i]);
	}
    }
    
    //+++ Processing::EB
    if (parameters.indexOf("Processing::EB")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::EB"),1);  
	s=s.remove(" <").simplified();
	QStringList lst;
	lst = s.split(" ", QString::SkipEmptyParts);
	
	for (i=0; i<imax;i++) 
	{
	    if (lst[i]=="0") tableEC->item(dptEB,i)->setText(""); else tableEC->item(dptEB,i)->setText(lst[i]);
	}	
    }
    
    //+++ Processing::C[m]
    if (parameters.indexOf("Processing::C[m]")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::C[m]"),1);  
	int pos=0;
	QRegExp rxF( "(\\d+)" );	
	
	for (i=0; i<imax;i++) 
	{
	    pos = rxF.indexIn( s, pos ); pos+=rxF.matchedLength();
	    tableEC->item(dptC,i)->setText(rxF.cap(1));
	}
    }
    
    //+++ Processing::D[m]
    if (parameters.indexOf("Processing::D[m]")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::D[m]"),1);
	int pos=0;
	
	QRegExp rx( "(\\d*\\.\\d+)" );	
	
	for (i=0; i<imax;i++) 
	{
	    pos = rx.indexIn( s, pos ); pos+=rx.matchedLength();
	    tableEC->item(dptD,i)->setText(rx.cap(1));
	}
    }	
    
    //+++ Processing::Lambda[A]
    if (parameters.indexOf("Processing::Lambda[A]")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Lambda[A]"),1);
	int pos=0;
	
	QRegExp rx( "(\\d*\\.\\d+)" );	
	
	for (i=0; i<imax;i++) 
	{
	    pos = rx.indexIn( s, pos ); pos+=rx.matchedLength();
	    tableEC->item(dptWL,i)->setText(rx.cap(1));
	}
    }
    
    //+++ Processing::Beam::Size
    if (parameters.indexOf("Processing::Beam::Size")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Beam::Size"),1);
	
	for (i=0; i<imax;i++) 
	{
	    s=s.trimmed();
	    tableEC->item(dptBSIZE,i)->setText(s.left(s.indexOf(" ")));
	    s=s.remove(0,s.indexOf(" "));
	    s=s.trimmed();
	}
    }
    
    //+++ Processing::Transm::EC
    if (parameters.indexOf("Processing::Transm::EC")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Transm::EC"),1);
	int pos=0;
	
	QRegExp rx( "(\\d*\\.\\d+)" );	
	
	for (i=0; i<imax;i++) 
	{
	    pos = rx.indexIn( s, pos ); pos+=rx.matchedLength();
	    tableEC->item(dptECTR,i)->setText(rx.cap(1));
	}
    }	
    
    
    //+++ Processing::Transm::EC::Activity
    if (parameters.indexOf("Processing::Transm::EC::Activity")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Transm::EC::Activity"),1);  
	int pos=0;
	QRegExp rxF( "(\\d+)" );	
	
	for (i=0; i<imax;i++) 
	{
	    pos = rxF.indexIn( s, pos ); pos+=rxF.matchedLength();
	    if (rxF.cap(1).toInt()==1) tableEC->item(dptECTR, i)->setCheckState(Qt::Checked);
        else tableEC->item(dptECTR, i)->setCheckState(Qt::Unchecked);
	}
    }
    
    //+++ Processing::Plexi::Plexi
    if (parameters.indexOf("Processing::Plexi::Plexi")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Plexi::Plexi"),1);  
	s=s.remove(" <").simplified();
	QStringList lst;
	lst = s.split(" ", QString::SkipEmptyParts);
	
	for (i=0; i<imax;i++) 
	{
	    if (lst[i]=="0") tableEC->item(dptACFS,i)->setText(""); else tableEC->item(dptACFS,i)->setText(lst[i]);
	}
    }	
    
    //+++ Processing::Plexi::EB
    if (parameters.indexOf("Processing::Plexi::EB")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Plexi::EB"),1);  
	s=s.remove(" <").simplified();
	QStringList lst;
	lst = s.split(" ", QString::SkipEmptyParts);
	
	for (i=0; i<imax;i++) 
	{
	    if (lst[i]=="0") tableEC->item(dptACEB,i)->setText(""); else tableEC->item(dptACEB,i)->setText(lst[i]);
	}
    }	
    
    //+++ Processing::Plexi::BC
    if (parameters.indexOf("Processing::Plexi::BC")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Plexi::BC"),1);  
	s=s.remove(" <").simplified();
	QStringList lst;
	lst = s.split(" ", QString::SkipEmptyParts);
	
	for (i=0; i<imax;i++) 
	{
	    if (lst[i]=="0") tableEC->item(dptACBC,i)->setText(""); else tableEC->item(dptACBC,i)->setText(lst[i]);
	}
    }
    
    //+++ Processing::AC::DAC
    if (parameters.indexOf("Processing::AC::DAC")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::AC::DAC"),1);
	int pos=0;
	
	QRegExp rx( "(\\d*\\.\\d+)" );	
	
	for (i=0; i<imax;i++) 
	{
	    pos = rx.indexIn( s, pos ); pos+=rx.matchedLength();
	    tableEC->item(dptDAC,i)->setText(rx.cap(1));
	}
    }
    
    //+++ Processing::AC::MU
    if (parameters.indexOf("Processing::AC::MU")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::AC::MU"),1);
	
	for (i=0; i<imax;i++) 
	{
	    s=s.trimmed();
	    tableEC->item(dptACMU,i)->setText(s.left(s.indexOf(" ")));
	    s=s.remove(0,s.indexOf(" "));
	    s=s.trimmed();
	}
    }
    
    
    //+++ Processing::AC::TR
    if (parameters.indexOf("Processing::AC::TR")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::AC::TR"),1);
	int pos=0;
	
	QRegExp rx( "(\\d*\\.\\d+)" );	
	
	for (i=0; i<imax;i++) 
	{
	    pos = rx.indexIn( s, pos ); pos+=rx.matchedLength();
	    tableEC->item(dptACTR,i)->setText(rx.cap(1));
	}
    }	
    
    //+++ Processing::AC::Factor
    if (parameters.indexOf("Processing::AC::Factor")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::AC::Factor"),1);
	
	for (i=0; i<imax;i++) 
	{
	    s=s.trimmed();
	    tableEC->item(dptACFAC,i)->setText(s.left(s.indexOf(" ")));
	    s=s.remove(0,s.indexOf(" "));
	    s=s.trimmed();
	}
    }
    
    //+++ Processing::Center::File
    if (parameters.indexOf("Processing::Center::File")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Center::File"),1);  
	s=s.remove(" <").simplified();
	QStringList lst;
	lst = s.split(" ", QString::SkipEmptyParts);
	
	for (i=0; i<imax;i++) 
	{
	    if (lst[i]=="0") tableEC->item(dptCENTER,i)->setText(""); else tableEC->item(dptCENTER,i)->setText(lst[i]);
	}
    }
    
    //+++ Processing::X-center
    if (parameters.indexOf("Processing::X-center")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::X-center"),1);
	
	for (i=0; i<imax;i++) 
	{
	    s=s.trimmed();
	    tableEC->item(dptCENTERX,i)->setText(s.left(s.indexOf(" ")));
	    s=s.remove(0,s.indexOf(" "));
	    s=s.trimmed();
	}
    }
    
    //+++ Processing::Y-center
    if (parameters.indexOf("Processing::Y-center")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Y-center"),1);
	
	for (i=0; i<imax;i++) 
	{
	    s=s.trimmed();
	    tableEC->item(dptCENTERY,i)->setText(s.left(s.indexOf(" ")));
	    s=s.remove(0,s.indexOf(" "));
	    s=s.trimmed();
	}
    }
    
    vertHeaderTableECPressed(dptMASK, false);
    vertHeaderTableECPressed(dptSENS, false);
    vertHeaderTableECPressed(dptMASKTR, false);
    
    //+++ Processing::Mask
    if (parameters.indexOf("Processing::Mask")>=0)
    {
        
        s=w->text(parameters.indexOf("Processing::Mask"),1);
        
        for (i=0; i<imax;i++)
        {
            s=s.trimmed();
            QComboBoxInTable *mask =(QComboBoxInTable*)tableEC->cellWidget(dptMASK,i);
            mask->setCurrentIndex(mask->findText("mask"));
            
            mask->setCurrentIndex(mask->findText(s.left(s.indexOf(" "))));
            s=s.remove(0,s.indexOf(" "));
            s=s.trimmed();
        }
    }
    
    //+++ Processing::MaskTr
    if (parameters.indexOf("Processing::Tr::Mask")>=0) 
    {
	
	s=w->text(parameters.indexOf("Processing::Tr::Mask"),1);
	
	for (i=0; i<imax;i++) 
	{
	    s=s.trimmed();
        QComboBoxInTable *mask =(QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,i);
        mask->setCurrentIndex(mask->findText("mask"));
        mask->setCurrentIndex(mask->findText(s.left(s.indexOf(" "))));
	    s=s.remove(0,s.indexOf(" "));
	    s=s.trimmed();
	}
    }
    //+++ Processing::Sensitivity
    if (parameters.indexOf("Processing::Sensitivity")>=0) 
    {
	s=w->text(parameters.indexOf("Processing::Sensitivity"),1);
	
	for (i=0; i<imax;i++) 
	{
	    s=s.trimmed();
        QComboBoxInTable *sens =(QComboBoxInTable*)tableEC->cellWidget(dptSENS,i);
        sens->setCurrentIndex(sens->findText("sens"));
        sens->setCurrentIndex(sens->findText(s.left(s.indexOf(" "))));
	    s=s.remove(0,s.indexOf(" "));
	    s=s.trimmed();
	}
    }
    
    //+++ Processing::File::Ext
    if (parameters.indexOf("Processing::File::Ext")>=0) 
    {
	lineEditFileExt->setText(w->text(parameters.indexOf("Processing::File::Ext"),1).remove(" <"));
    }  
    
    //+++ Options::Instrument::DeadTime::Homogenity
    if (parameters.indexOf("Options::Instrument::DeadTime::Homogenity")>=0) 
    {
	comboBoxDTtype->setItemText(comboBoxDTtype->currentIndex(), w->text(parameters.indexOf("Options::Instrument::DeadTime::Homogenity"),1).remove(" <"));
    }
    
    
    //+++ Options::Calibrant::Type
    if (parameters.indexOf("Options::Calibrant::Type")>=0) 
    {
	s=w->text(parameters.indexOf("Options::Calibrant::Type"),1).remove(" <");
	
	if (s.contains("Direct Beam")) comboBoxACmethod->setCurrentIndex(1);
	else if (s.contains("Flat Scatter + Transmission")) comboBoxACmethod->setCurrentIndex(2);	    
	else if (s.contains("Counts per Channel")) comboBoxACmethod->setCurrentIndex(3);
	else
	{
	    comboBoxACmethod->setCurrentIndex(0);
	}
	
    }
    
    //+++ Options::Calibrant::Active::Mask::Sens
    if (parameters.indexOf("Options::Calibrant::Active::Mask::Sens")>=0) 
    {
	s=w->text(parameters.indexOf("Options::Calibrant::Active::Mask::Sens"),1);
	if (s.contains("yes")) checkBoxACDBuseActive->setChecked(true); else checkBoxACDBuseActive->setChecked(false);
    }
    
    //+++ Options::Calibrant
    if (parameters.indexOf("Options::Calibrant")>=0) 
    {
	comboBoxCalibrant->setItemText(comboBoxCalibrant->currentIndex(),
                                   w->text(parameters.indexOf("Options::Calibrant"),1).remove(" <"));
	calibrantselected();
    }
    
    //+++ Options::Calibrant::CalculateTr
    if (parameters.indexOf("Options::Calibrant::CalculateTr")>=0) 
    {
	s=w->text(parameters.indexOf("Options::Calibrant::CalculateTr"),1);
	if (s.contains("yes")) checkBoxTransmissionPlexi->setChecked(true); else checkBoxTransmissionPlexi->setChecked(false);
    }
    
    //+++ Options::2D::Normalization
    if (parameters.indexOf("Options::2D::Normalization")>=0) 
    {
	comboBoxNorm->setItemText(comboBoxNorm->currentIndex(),
                              w->text(parameters.indexOf("Options::2D::Normalization"),1).remove(" <"));
    }
    //+++ Options::2D::Normalization::Constant
    if (parameters.indexOf("Options::2D::Normalization::Constant")>=0) 
    {
	spinBoxNorm->setValue(w->text(parameters.indexOf("Options::2D::Normalization::Constant"),1).remove(" <").toInt());
    }
    
    //+++ Options::2D::Normalization::BC::Normalization
    if (parameters.indexOf("Options::2D::Normalization::BC::Normalization")>=0) 
    {
	s=w->text(parameters.indexOf("Options::2D::Normalization::BC::Normalization"),1);
	if (s.contains("yes")) checkBoxBCTimeNormalization->setChecked(true); else checkBoxBCTimeNormalization->setChecked(false);
    }
    
    //+++ Options::2D::xyDimension::Pixel
    if (parameters.indexOf("Options::2D::xyDimension::Pixel")>=0) 
    {
	s=w->text(parameters.indexOf("Options::2D::xyDimension::Pixel"),1);
	
	if (s.contains("yes"))
	{
	    radioButtonXYdimPixel->setChecked(true);
	    radioButtonXYdimQ->setChecked(false);		
	}
	else 
	{
	    radioButtonXYdimQ->setChecked(true);		
	    radioButtonXYdimPixel->setChecked(false);
	}
    }
    
    //+++ Options::2D::RemoveNegativePoints::2D
    if (parameters.indexOf("Options::2D::RemoveNegativePoints::2D")>=0) 
    {
	s=w->text(parameters.indexOf("Options::2D::RemoveNegativePoints::2D"),1);
	if (s.contains("yes")) checkBoxMaskNegative->setChecked(true); else checkBoxMaskNegative->setChecked(false);
    }
    
    //+++ Options::2D::RemoveNegativePoints::1D
    if (parameters.indexOf("Options::2D::RemoveNegativePoints::1D")>=0) 
    {
	s=w->text(parameters.indexOf("Options::2D::RemoveNegativePoints::1D"),1);
	if (s.contains("yes")) checkBoxMaskNegativeQ->setChecked(true); else checkBoxMaskNegativeQ->setChecked(false);
    }
    
    //+++ Options::2D::Polar::Resolusion
    if (parameters.indexOf("Options::2D::Polar::Resolusion")>=0) 
    {
	s=w->text(parameters.indexOf("Options::2D::Polar::Resolusion"),1);
	spinBoxPolar->setValue(w->text(parameters.indexOf("Options::2D::Polar::Resolusion"),1).remove(" <").toInt());
    }
    
    
    //+++ Options::2D::HighQcorrection
    if (parameters.indexOf("Options::2D::HighQcorrection")>=0) 
    {
	s=w->text(parameters.indexOf("Options::2D::HighQcorrection"),1);
	if (s.contains("yes")) checkBoxParallax->setChecked(true); else checkBoxParallax->setChecked(false);
    }
    
    //+++ Options::2D::HighQtype
    if (parameters.indexOf("Options::2D::HighQtype")>=0)
    {
        comboBoxParallax->setItemText(comboBoxParallax->currentIndex(),
                                      w->text(parameters.indexOf("Options::2D::HighQtype"),1).remove(" <"));
    }
    
    //+++ Options::2D::HighQtransmission
    if (parameters.indexOf("Options::2D::HighQtransmission")>=0)
    {
        s=w->text(parameters.indexOf("Options::2D::HighQtransmission"),1);
        if (s.contains("yes")) checkBoxParallaxTr->setChecked(true); else checkBoxParallaxTr->setChecked(false);
    }
    
    //+++ Options::2D::HighQwindow
    if (parameters.indexOf("Options::2D::HighQwindow")>=0)
    {
        s=w->text(parameters.indexOf("Options::2D::HighQwindow"),1);
        if (s.contains("yes")) checkBoxWaTrDet->setChecked(true); else checkBoxWaTrDet->setChecked(false);
    }
    
    //+++ Options::2D::DeadTimeModel
    if (parameters.indexOf("Options::2D::DeadTimeModel")>=0) 
    {
	s=w->text(parameters.indexOf("Options::2D::DeadTimeModel"),1);
	if (s.contains("Non-Paralysable")) radioButtonDeadTimeCh->setChecked(true); else radioButtonDeadTimeDet->setChecked(true);
    }
    
    //+++ Options::2D::FindCenterMethod
    if (parameters.indexOf("Options::2D::FindCenterMethod")>=0) 
    {
	s=w->text(parameters.indexOf("Options::2D::FindCenterMethod"),1);
	if (s.contains("X and Y symmetrization")) 
	{
	    radioButtonCenterHF->setChecked(false);
	    radioButtonRadStdSymm->setChecked(true);
	    radioButtonCenterReadFromHeader->setChecked(false);
	}
	else if (s.contains("Moment-minimalization (H.F.)")) 
	{
	    radioButtonCenterHF->setChecked(true);
	    radioButtonRadStdSymm->setChecked(false);
	    radioButtonCenterReadFromHeader->setChecked(false);
	}
	else
	{
	    radioButtonCenterHF->setChecked(false);
	    radioButtonRadStdSymm->setChecked(false);
	    radioButtonCenterReadFromHeader->setChecked(true);
	}
    }  
    //+++ Options::2D::OutputFormat
    if (parameters.indexOf("Options::2D::OutputFormat")>=0) 
    {
	comboBoxIxyFormat->setCurrentIndex(w->text(parameters.indexOf("Options::2D::OutputFormat"),1).remove(" <").toInt());
    }
    //+++ Options::2D::HeaderOutputFormat
    if (parameters.indexOf("Options::2D::HeaderOutputFormat")>=0) 
    {
	s=w->text(parameters.indexOf("Options::2D::HeaderOutputFormat"),1);
	if (s.contains("yes")) checkBoxASCIIheaderIxy->setChecked(true); 
	else checkBoxASCIIheaderIxy->setChecked(false);
    }
    //+++ Options::2D::HeaderSASVIEW
    if (parameters.indexOf("Options::2D::HeaderSASVIEW")>=0)
    {
        s=w->text(parameters.indexOf("Options::2D::HeaderSASVIEW"),1);
        if (s.contains("yes")) checkBoxASCIIheaderSASVIEW->setChecked(true);
        else checkBoxASCIIheaderSASVIEW->setChecked(false);
    }
    //+++ Options::1D::SASpresentation
    if (parameters.indexOf("Options::1D::SASpresentation")>=0) 
    {
	comboBoxSelectPresentation->setItemText(comboBoxSelectPresentation->currentIndex(),
                                            w->text(parameters.indexOf("Options::1D::SASpresentation"),1).remove(" <"));
	sasPresentation( );
    }
    
    //+++ Options::1D::I[Q]::Format
    if (parameters.indexOf("Options::1D::I[Q]::Format")>=0) 
    {
	comboBox4thCol->setItemText(comboBox4thCol->currentIndex(), w->text(parameters.indexOf("Options::1D::I[Q]::Format"),1).remove(" <"));
    }
    
    //+++ Options::1D::I[Q]::PlusHeader
    if (parameters.indexOf("Options::1D::I[Q]::PlusHeader")>=0)
    {
        s=w->text(parameters.indexOf("Options::1D::I[Q]::PlusHeader"),1);
        if (s.contains("yes")) checkBoxASCIIheader->setChecked(true); else checkBoxASCIIheader->setChecked(false);
    }
    
    //+++ Options::1D::I[Q]::Anisotropy
    if (parameters.indexOf("Options::1D::I[Q]::Anisotropy")>=0)
    {
        s=w->text(parameters.indexOf("Options::1D::I[Q]::Anisotropy"),1);
        if (s.contains("yes")) checkBoxAnisotropy->setChecked(true); else checkBoxAnisotropy->setChecked(false);
    }
    
    //+++ Options::1D::I[Q]::AnisotropyAngle
    if (parameters.indexOf("Options::1D::I[Q]::AnisotropyAngle")>=0)
    {
        spinBoxAnisotropyOffset->setValue(w->text(parameters.indexOf("Options::1D::I[Q]::AnisotropyAngle"),1).remove(" <").toInt());
    }
    
    //+++Options::1D::TransmissionMethod
    if (parameters.indexOf("Options::1D::TransmissionMethod")>=0) 
    {
	comboBoxTransmMethod->setItemText(comboBoxTransmMethod->currentIndex(),
                                      w->text(parameters.indexOf("Options::1D::TransmissionMethod"),1).remove(" <"));
    }    
    
    //+++ Options::1D::Slices
    if (parameters.indexOf("Options::1D::Slices")>=0) 
    {
	s=w->text(parameters.indexOf("Options::1D::Slices"),1);  
	int pos=0;
	QRegExp rxF( "(\\d+)" );	
	
	pos = rxF.indexIn( s, pos ); pos+=rxF.matchedLength();
	spinBoxFrom->setValue(rxF.cap(1).toInt());
	
	pos = rxF.indexIn( s, pos ); pos+=rxF.matchedLength();
	spinBoxTo->setValue(rxF.cap(1).toInt());
    }
    //+++ Options::1D::SlicesBS
    if (parameters.indexOf("Options::1D::SlicesBS")>=0)
    {
        s=w->text(parameters.indexOf("Options::1D::SlicesBS"),1);
        if (s.contains("yes")) checkBoxSlicesBS->setChecked(true); else checkBoxSlicesBS->setChecked(false);
    }
    //+++ Options::1D::SkipPoins
    if (parameters.indexOf("Options::1D::SkipPoins")>=0) 
    {
	s=w->text(parameters.indexOf("Options::1D::SkipPoins"),1);  
	int pos=0;
	QRegExp rxF( "(\\d+)" );	
	
	pos = rxF.indexIn( s, pos ); pos+=rxF.matchedLength();
	spinBoxRemoveFirst->setValue(rxF.cap(1).toInt());
	
	pos = rxF.indexIn( s, pos ); pos+=rxF.matchedLength();
	spinBoxRemoveLast->setValue(rxF.cap(1).toInt());
    }	
    
    //+++ Options::1D::I[Q]::Method
    if (parameters.indexOf("Options::1D::I[Q]::Method")>=0) 
    {
	s=w->text(parameters.indexOf("Options::1D::I[Q]::Method"),1);
	if (s.contains("4-Pixel Interpolation (H.F.)")) radioButtonRadHF->setChecked(true); else radioButtonRadStd->setChecked(true);
    }
    
    //+++ Options::1D::I[Q]::LinearFactor
    if (parameters.indexOf("Options::1D::I[Q]::LinearFactor")>=0)
    {
        spinBoxAvlinear->setValue(w->text(parameters.indexOf("Options::1D::I[Q]::LinearFactor"),1).remove(" <").toInt());
    }
    
    //+++ Options::1D::I[Q]::ProgressiveFactor
    if (parameters.indexOf("Options::1D::I[Q]::ProgressiveFactor")>=0)
    {
        doubleSpinBoxAvLog->setValue(w->text(parameters.indexOf("Options::1D::I[Q]::ProgressiveFactor"),1).remove(" <").toDouble());
    }
    
    //+++ Options::ScriptTable::RecalculateOldFiles
    if (parameters.indexOf("Options::ScriptTable::RecalculateOldFiles")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::RecalculateOldFiles"),1);
	if (s.contains("yes")) checkBoxRecalculate->setChecked(true); else checkBoxRecalculate->setChecked(false);
    }
    
    //+++ Options::ScriptTable::UseSamplePositionAsParameter
    if (parameters.indexOf("Options::ScriptTable::UseSamplePositionAsParameter")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::UseSamplePositionAsParameter"),1);
	if (s.contains("yes")) checkBoxRecalculateUseNumber->setChecked(true); else checkBoxRecalculateUseNumber->setChecked(false);
    }
    
    //+++ Options::ScriptTable::UseAttenuatorAsParameter
    if (parameters.indexOf("Options::ScriptTable::UseAttenuatorAsParameter")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::UseAttenuatorAsParameter"),1);
	if (s.contains("yes")) checkBoxAttenuatorAsPara->setChecked(true); else checkBoxAttenuatorAsPara->setChecked(false);
    }
    
    //+++ Options::ScriptTable::BeamCenterAsCondition
    if (parameters.indexOf("Options::ScriptTable::BeamCenterAsCondition")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::BeamCenterAsCondition"),1);
	if (s.contains("yes")) checkBoxBeamcenterAsPara->setChecked(true); else checkBoxBeamcenterAsPara->setChecked(false);
    }
    //+++ Options::ScriptTable::PolarizationCondition
    if (parameters.indexOf("Options::ScriptTable::PolarizationAsCondition")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::PolarizationAsCondition"),1);
	if (s.contains("yes")) checkBoxPolarizationAsPara->setChecked(true); else checkBoxPolarizationAsPara->setChecked(false);
    }
    //+++ Options::ScriptTable::DetectorAngleAsCondition
    if (parameters.indexOf("Options::ScriptTable::DetectorAngleAsCondition")>=0)
    {
        s=w->text(parameters.indexOf("Options::ScriptTable::DetectorAngleAsCondition"),1);
        if (s.contains("yes")) checkBoxDetRotAsPara->setChecked(true); else checkBoxDetRotAsPara->setChecked(false);
    }
    //+++ Options::ScriptTable::FindCenterEveryFile
    if (parameters.indexOf("Options::ScriptTable::FindCenterEveryFile")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::FindCenterEveryFile"),1);
	if (s.contains("yes")) checkBoxFindCenter->setChecked(true); else checkBoxFindCenter->setChecked(false);
    }
    
    //+++ Options::Automatical generation of merging template (I[Q] case)
    if (parameters.indexOf("Options::ScriptTable::MergingTemplate")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::MergingTemplate"),1);
	if (s.contains("yes")) checkBoxMergingTable->setChecked(true); else checkBoxMergingTable->setChecked(false);
    }
    
    //+++ Options::ScriptTable::AutoMerging
    if (parameters.indexOf("Options::ScriptTable::AutoMerging")>=0)
    {
        s=w->text(parameters.indexOf("Options::ScriptTable::AutoMerging"),1);
        if (s.contains("yes")) checkBoxAutoMerging->setChecked(true); else checkBoxAutoMerging->setChecked(false);
    }
    
    //+++ Options::Overlap::Merging
    if (parameters.indexOf("Options::Overlap::Merging")>=0)
    {
        spinBoxOverlap->setValue(w->text(parameters.indexOf("Options::Overlap::Merging"),1).remove(" <").toInt());
    }
    
    //+++ Options::Rewrite Output (No index)
    if (parameters.indexOf("Options::ScriptTable::RewriteOutput")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::RewriteOutput"),1);
	if (s.contains("yes")) checkBoxRewriteOutput->setChecked(true); else checkBoxRewriteOutput->setChecked(false);
    }
    
    //+++ Options::ScriptTable::Transmission::ForceCopyPaste"
    if (parameters.indexOf("Options::ScriptTable::Transmission::ForceCopyPaste")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::Transmission::ForceCopyPaste"),1);
	if (s.contains("yes")) checkBoxForceCopyPaste->setChecked(true); else checkBoxForceCopyPaste->setChecked(false);
    }
    
    //+++ Options::ScriptTable::LabelAsName"
    if (parameters.indexOf("Options::ScriptTable::LabelAsName")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::LabelAsName"),1);
	if (s.contains("yes")) checkBoxNameAsTableName->setChecked(true); else checkBoxNameAsTableName->setChecked(false);
    }
    
    //+++ Options::ScriptTable::Transmission::SkiptTrConfigurations
    if (parameters.indexOf("Options::ScriptTable::Transmission::SkiptTrConfigurations")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::Transmission::SkiptTrConfigurations"),1);
	if (s.contains("yes")) checkBoxSkiptransmisionConfigurations->setChecked(true); 
	else checkBoxSkiptransmisionConfigurations->setChecked(false);
    }
    
    //+++ Options::ScriptTable::Output::Folders
    if (parameters.indexOf("Options::ScriptTable::Output::Folders")>=0) 
    {
	s=w->text(parameters.indexOf("Options::ScriptTable::Output::Folders"),1);
	if (s.contains("yes")) checkBoxSortOutputToFolders->setChecked(true); 
	else checkBoxSortOutputToFolders->setChecked(false);
    }    
    
    //+++ Options::Resolusion::Focusing
    if (parameters.indexOf("Options::Resolusion::Focusing")>=0) 
    {
	s=w->text(parameters.indexOf("Options::Resolusion::Focusing"),1);
	if (s.contains("yes")) checkBoxResoFocus->setChecked(true); 
	else checkBoxResoFocus->setChecked(false);
    }    
    
    //+++ Options::Resolusion::Detector
    if (parameters.indexOf("Options::Resolusion::Detector")>=0) 
    {
	lineEditDetReso->setText(w->text(parameters.indexOf("Options::Resolusion::Detector"),1).remove(" <"));
    } 	
    
    
    //+++ Options::Resolusion::CA::Round
    if (parameters.indexOf("Options::Resolusion::CA::Round")>=0) 
    {
	s=w->text(parameters.indexOf("Options::Resolusion::CA::Round"),1);
	if (s.contains("yes")) checkBoxResoCAround->setChecked(true); 
	else checkBoxResoCAround->setChecked(false);
    }    
    
    //+++ Options::Resolusion::SA::Round
    if (parameters.indexOf("Options::Resolusion::SA::Round")>=0) 
    {
	s=w->text(parameters.indexOf("Options::Resolusion::SA::Round"),1);
	if (s.contains("yes")) checkBoxResoSAround->setChecked(true); 
	else checkBoxResoSAround->setChecked(false);
    }
    
    
    //+++ header name generation
    for (int i=0; i<imax; i++) horHeaderTableECPressed(i, false);
    
    //+++ script table activation
    if (tableName.right(9)=="-Settings")
    {
	QString activeScript=tableName.remove("-Settings");
	for (int i=1; i<comboBoxMakeScriptTable->count(); i++)
	{
	    if (comboBoxMakeScriptTable->itemText(i)==activeScript) comboBoxMakeScriptTable->setCurrentIndex(i);
	}
    }
    
    return true;
    
}	

void dan18::addCopyOfLastConfiguration()
{
    int oldNumber=sliderConfigurations->value();
    
    if (sliderConfigurations->maximum()==oldNumber) return;
    
    sliderConfigurations->setValue(oldNumber+1);
    
    for (int i=0;i<16;i++)     tableEC->item(i,oldNumber)->setText(tableEC->item(i,oldNumber-1)->text());
    for (int i=18;i<20;i++)    tableEC->item(i,oldNumber)->setText(tableEC->item(i,oldNumber-1)->text());
    
    QString oldMask=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,oldNumber-1))->currentText();
    QComboBoxInTable *mask =(QComboBoxInTable*)tableEC->cellWidget(dptMASK,oldNumber);
    mask->setCurrentIndex(mask->findText(oldMask));
    
    QString oldSens=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,oldNumber-1))->currentText();
    QComboBoxInTable *sens =(QComboBoxInTable*)tableEC->cellWidget(dptSENS,oldNumber);
    sens->setCurrentIndex(sens->findText(oldSens));
    
    oldMask=((QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,oldNumber-1))->currentText();
    QComboBoxInTable *maskTR =(QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,oldNumber);
    maskTR->setCurrentIndex(maskTR->findText(oldMask));
}

//+++
void dan18::deleteFirstConfiguration()
{
    int oldNumber=sliderConfigurations->value();
    if (oldNumber==1) return;
    
    tableEC->removeColumn(0);
    
    sliderConfigurations->setValue(oldNumber-1);
}

void dan18::addMaskAndSens(int condNumber)
{
    addMaskAndSens(condNumber, 0);
}

void dan18::addMaskAndSens(int condNumber, int oldNumber)
{
    ImportantConstants();

    for(int i=oldNumber;i<condNumber;i++)
    {
        QTableWidgetItem *checkTrNew = new QTableWidgetItem();
        checkTrNew->setCheckState(Qt::Unchecked);
        checkTrNew->setText("1.000");
        tableEC->setItem(dptECTR,i, checkTrNew);
    }

    
    QStringList lst;
    
    for(int i=oldNumber;i<condNumber;i++)
    {
        
        //+++ mask
        findMatrixListByLabel("DAN::Mask::"+QString::number(MD),lst);
        if (!lst.contains("mask")) lst.prepend("mask");
        QComboBoxInTable *mask = new QComboBoxInTable(dptMASK,i,tableEC);
        mask->addItems(lst);
        tableEC->setCellWidget(dptMASK,i,mask);
        mask->setCurrentIndex(lst.indexOf("mask"));
        
        //+++ sens
        findMatrixListByLabel("DAN::Sensitivity::"+QString::number(MD),lst);
        if (!lst.contains("sens")) lst.prepend("sens");
        QComboBoxInTable *sens = new QComboBoxInTable(dptSENS,i,tableEC);
        sens->addItems(lst);
        tableEC->setCellWidget(dptSENS,i,sens);
        sens->setCurrentIndex(lst.indexOf("sens"));
        
        //+++ mask
        findMatrixListByLabel("DAN::Mask::"+QString::number(MD),lst);
        if (!lst.contains("mask")) lst.prepend("mask");
        QComboBoxInTable *maskTR = new QComboBoxInTable(dptMASKTR,i,tableEC);
        maskTR->addItems(lst);
        tableEC->setCellWidget(dptMASKTR,i,maskTR);
        maskTR->setCurrentIndex(lst.indexOf("mask"));
    }
}


// tableEC:: if vertical header is pressed...
void dan18::vertHeaderTableECPressed(int raw)
{
    vertHeaderTableECPressed(raw, true);
}

void dan18::vertHeaderTableECPressed(int raw,  bool headerReallyPressed )
{
    ImportantConstants();
    
    int i;
    
    updateMaskList();
    updateSensList();
    
    int CDL=tableEC->columnCount();
    
    
    if (raw==dptEC)
    {
        updateColInScriptAll("#-EC", dptEC);
        updateColInScriptAll("#-EC [EB]", dptEC);
    }
    
    if (raw==dptBC)
    {
        updateColInScriptAll("#-BC", dptBC);
    }
    
    if (raw==dptC)  //+++ C +++
    {
        for (i=0; i<CDL; i++)
        {
            QString ECnumber=tableEC->item(dptEC,i)->text();
            
            if (checkFileNumber(ECnumber))
            {
                // C
                int C = int(collimation->readCinM(ECnumber));
                tableEC->item(dptC, i)->setText(QString::number(C, 'f', 0));
            }
        }
        updateColInScriptAll("C", dptC);
    }
    
    if (raw==dptD) //+++ D +++
    {
        for (i=0; i<CDL; i++)
        {
            QString ECnumber=tableEC->item(dptEC,i)->text();
            
            if ( checkFileNumber( ECnumber ) )
            {
                double D = detector->readDinM(ECnumber); //[m]
                tableEC->item(dptD,i)->setText(QString::number(D,'f',3));
            }
        }
        updateColInScriptAll("D", dptD);
    }
    if (raw==dptWL) //+++ Lambda +++
    {
        for (i=0; i<CDL; i++)
        {
            QString ECnumber=tableEC->item(dptEC, i)->text();
            
            if ( checkFileNumber( ECnumber ) )
            {
                double lambda=readLambda( ECnumber );
                tableEC->item(dptWL,i)->setText(QString::number(lambda,'f',3));
            }
        }
        updateColInScriptAll("Lambda", dptWL);
    }
    if (raw==dptBSIZE)        //+++ Beam Size +++
    {
        for (i=0; i<CDL; i++)
        {
            QString ECnumber=tableEC->item(dptEC,i)->text();
            
            if (checkFileNumber(ECnumber))
            {
                QString CA = collimation->readCA(ECnumber);
                QString SA = collimation->readSA(ECnumber);
                tableEC->item(dptBSIZE, i)->setText(CA + "|" + SA);
            }
        }
        updateColInScriptAll("Beam Size", dptBSIZE);
    }
    
    if (raw==dptECTR)
    {
        for (i=0; i<CDL; i++)
        {
            if (tableEC->item(dptECTR, i)->checkState() == Qt::Checked)
            {
                QString ECtStr=tableEC->item(dptEC,i)->text();
                QString EBtStr=tableEC->item(dptEB,i)->text();
                
                QString mask=comboBoxMaskFor->currentText();
                
                mask=((QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,i))->currentText();

                QString trString;
                double tr;
                double sigmaTr = 0;

                tr = readTransmission(ECtStr, EBtStr, mask, 0, 0, sigmaTr);

                if (tr > 1.5 || tr <= 0)
                    trString = "1.0000";
                else
                    trString = QString::number(tr, 'f', 4);

                tableEC->item(dptECTR, i)->setText(trString);
            }
            else
                tableEC->item(dptECTR, i)->setText(QString::number(1.0, 'f', 4));
        }
        
        for (i=0; i<CDL; i++)
        {
            if (tableEC->item(dptECTR, i)->checkState() != Qt::Checked)
            {
                double currentLambda=tableEC->item(dptWL,i)->text().toDouble();
                currentLambda=QString::number(currentLambda,'f',1).toDouble();
                
                for (int ii=0; ii<CDL; ii++)
                {
                    if (tableEC->item(dptECTR, ii)->checkState() == Qt::Checked)
                    {
                        double currentLambda2=tableEC->item(dptWL,ii)->text().toDouble();
                        currentLambda2=QString::number(currentLambda2,'f',1).toDouble();
                        if (currentLambda2==currentLambda)     tableEC->item(dptECTR,i)->setText(tableEC->item(dptECTR,ii)->text());
                    }
                }
            }
        }
        updateColInScriptAll("Transmission-EC", dptECTR);
        //+++ 2021-03-04
        calcAbsCalNew();
        updateColInScriptAll("Factor",dptACFAC);
        //--- 2021-03-04
    }
    if (raw==dptDAC) //+++ D-AC +++
    {
        for (i=0; i<CDL; i++)
        {
            QString PlexyNumber;
            
            if(comboBoxACmethod->currentIndex()==1) PlexyNumber=tableEC->item(dptACEB,i)->text();
            else PlexyNumber=tableEC->item(dptACFS,i)->text();
            
            if ( checkFileNumber( PlexyNumber ) )
            {
                double D = detector->readDinM(PlexyNumber); // [m]
                tableEC->item(dptDAC,i)->setText(QString::number(D,'f',3));
            }
        }
    }
    if (raw==dptACMU)
    {
        for (i=0; i<CDL; i++)
        {
            double lambda=tableEC->item(dptWL,i)->text().toDouble();
            tableEC->item(dptACMU,i)->setText(QString::number(muCalc(lambda),'E',4));
        }
    }
    
    if (raw==dptACTR)
    {
        for (i=0; i<CDL; i++)
        {
            if (checkBoxTransmissionPlexi->isChecked())
            {
                double lambda=tableEC->item(dptWL,i)->text().toDouble();
                tableEC->item(dptACTR,i)->setText(QString::number(tCalc(lambda),'f',4));
            }
            else
            {
                QString mask=comboBoxMaskFor->currentText();
                
                mask=tableEC->item(dptMASKTR,i)->text();
                
                double sigmaTr=0;
                double Tr=readTransmission( tableEC->item(dptACFS,i)->text(), tableEC->item(dptACEB,i)->text(), mask,0,0,sigmaTr);
                if (Tr<=0 || Tr>1)
                {
                    double lambda=tableEC->item(dptWL,i)->text().toDouble();
                    tableEC->item(dptACTR,i)->setText(QString::number(tCalc(lambda),'f',4));
                    
                }
                else tableEC->item(dptACTR,i)->setText(QString::number(Tr,'f',4));
            }
        }
    }
    if (raw==dptACFAC)
    {
        calcAbsCalNew();
        updateColInScriptAll("Factor",dptACFAC);
    }
    
    if (raw==dptCENTERX || raw==dptCENTERY)
    {
        calcCenterNew();
        updateCenterAll();
    }
    if (raw==dptMASK)
    {
        //mask
        QStringList lst0, lst;
        findMatrixListByLabel("DAN::Mask::"+QString::number(MD),lst0);
        lst=lst0;
        if (lst.indexOf("mask")<0) lst.prepend("mask");
        QString currentMask;
        for(i=0;i<tableEC->columnCount();i++)
        {
            QComboBoxInTable *mask =(QComboBoxInTable*)tableEC->cellWidget(dptMASK,i);
            currentMask=mask->currentText();
            mask->clear();
            mask->addItems(lst);
            mask->setCurrentIndex(lst.indexOf(currentMask));
        }
        if (headerReallyPressed) updateMaskNamesInScript(0, "Mask");
    }
    if (raw==dptSENS)
    {
        //sens
        QStringList lst;
        findMatrixListByLabel("DAN::Sensitivity::"+QString::number(MD),lst);
        if (lst.indexOf("sens")<0) lst.prepend("sens");
        QString currentSens;
        for(i=0;i<tableEC->columnCount();i++)
        {
            QComboBoxInTable *sens =(QComboBoxInTable*)tableEC->cellWidget(dptSENS,i);
            currentSens=sens->currentText();
            sens->clear();
            sens->addItems(lst);
            sens->setCurrentIndex(lst.indexOf(currentSens));
        }
        if (headerReallyPressed) updateMaskNamesInScript(0, "Sens");
    }
    if (raw==dptMASKTR)
    {
        //mask
        QStringList lst0, lst;
        findMatrixListByLabel("DAN::Mask::"+QString::number(MD),lst0);
        lst=lst0;
        if (lst.indexOf("mask")<0) lst.prepend("mask");
        QString currentMask;
        for(i=0;i<tableEC->columnCount();i++)
        {
            QComboBoxInTable *mask =(QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,i);
            currentMask=mask->currentText();
            mask->clear();
            mask->addItems(lst);
            mask->setCurrentIndex(lst.indexOf(currentMask));
        }
    }
}


// tableEC:: if hor header is pressed...
void dan18::horHeaderTableECPressed(int col)
{
    horHeaderTableECPressed(col,true);
}

void dan18::horHeaderTableECPressed(int col,  bool headerReallyPressed)
{
//old    QString currentLabel=tableEC->horizontalHeader()->label(col);

    QString currentLabel=tableEC->horizontalHeaderItem(col)->text();

    QString colLabel;
    if ((currentLabel.contains("|D") && headerReallyPressed) || (!currentLabel.contains("|D") && !headerReallyPressed))
    {
	colLabel="cond.-#"+QString::number(col+1);
    }
    else {
	
	QString C=tableEC->item(dptC,col)->text();
	double D=tableEC->item(dptD,col)->text().toDouble();
	double lambda=tableEC->item(dptWL,col)->text().toDouble();
	colLabel="C"+C+"|D";
	colLabel+=QString::number(D,'f',0)+"|";
	colLabel+=QString::number(lambda,'f',0)+QChar(197);
    }
    //tableEC->horizontalHeader()->setLabel(col, colLabel);
    
    tableEC->horizontalHeaderItem(col)->setText(colLabel);
} 

void dan18::tableECclick(  int row, int col )
{
    ImportantConstants();
    
    if (row==dptEC || row==dptBC || row==dptEB || row==dptACFS ||
	row==dptACEB || row==dptACBC || row==dptCENTER)
    {
        QString s;
        if (!selectFile(s))
            return;
        QString oldValue = tableEC->item(row, col)->text();
        tableEC->item(row, col)->setText(s);

        // C
        int C = int(collimation->readCinM(s));
        // D
        double D = detector->readDinM(s);
        // lambda
        double lambda = readLambda(s);
        // CA
        QString CA = collimation->readCA(s);
        // CA
        QString SA = collimation->readSA(s);
        QString beam = CA + "|" + SA;

        bool changeNumber = true;

        if (row == dptEC)
        {
            if (changeNumber)
            {
                tableEC->item(dptC, col)->setText(QString::number(C, 'f', 0));
                tableEC->item(dptD, col)->setText(QString::number(D, 'f', 3));
                tableEC->item(dptWL, col)->setText(QString::number(lambda, 'f', 3));
                tableEC->item(dptBSIZE, col)->setText(beam);
                horHeaderTableECPressed(col, false);
            }
	    }
    }
    else if (row == dptC) //+++ C +++
    {
        QString ECnumber=tableEC->item(dptEC,col)->text();
        if (checkFileNumber(ECnumber))
        {
            // C
            int C = int(collimation->readCinM(ECnumber));
            tableEC->item(dptC, col)->setText(QString::number(C, 'f', 0));
        }
    }
    else if (row == dptD) //+++ D +++
    {
        QString ECnumber = tableEC->item(dptEC, col)->text();
        if (checkFileNumber(ECnumber))
        {
            double D = detector->readDinM(ECnumber);
            tableEC->item(dptD, col)->setText(QString::number(D, 'f', 3));
        }
    }
    else if (row==dptWL) //+++ Lambda +++
    {
	QString ECnumber=tableEC->item(dptEC, col)->text();
	if ( checkFileNumber(  ECnumber ) )
	{
	    double lambda=readLambda( ECnumber );	    
	    tableEC->item(dptWL,col)->setText(QString::number(lambda,'f',3));
	}
    }
    else if (row==dptBSIZE)	    //+++ Beam Size +++
    {
        QString ECnumber = tableEC->item(dptEC, col)->text();
        if (checkFileNumber(ECnumber))
        {
            QString CA = collimation->readCA(ECnumber);
            QString SA = collimation->readSA(ECnumber);
            tableEC->item(dptBSIZE, col)->setText(CA + "|" + SA);
        }
    }    
    else if (row == dptECTR)
    {
        bool ok;
        double res = QInputDialog::getDouble(this, "qtiSAS", "Enter transmission of the current condition:",
                                             tableEC->item(row, col)->text().toDouble(), 0, 1.500, 3, &ok);
        if (ok)
        {
            tableEC->item(row, col)->setText(QString::number(res, 'f', 4));
            if (!calcAbsCalNew(col))
                return;
        }
    }
    else if (row == dptACMU)
    {
        double lambda = tableEC->item(dptWL, col)->text().toDouble();
        tableEC->item(dptACMU, col)->setText(QString::number(muCalc(lambda), 'E', 4));
    }
    else if (row == dptACTR)
    {
        if (checkBoxTransmissionPlexi->isChecked())
        {
            double lambda = tableEC->item(dptWL, col)->text().toDouble();
            tableEC->item(dptACTR, col)->setText(QString::number(tCalc(lambda), 'f', 4));
        }
        else
        {
            QString mask = comboBoxMaskFor->currentText();

            mask = ((QComboBoxInTable *)tableEC->cellWidget(dptMASKTR, col))->currentText();
            double sigmaTr = 0.0;
            double Tr = readTransmission(tableEC->item(dptACFS, col)->text(), tableEC->item(dptACEB, col)->text(), mask,
                                         0, 0, sigmaTr);
            tableEC->item(dptACTR, col)->setText(QString::number(Tr, 'f', 4));
        }
    }
    else if (row == dptACFAC)
    {
        if (!calcAbsCalNew(col))
            return;
    }
    else if (row == dptCENTERX || row == dptCENTERY)
    {
        calcCenterNew(col);
    }
}

void dan18::tableEChelp(int raw)
{ 
    QString sHelp;
    switch(raw)
    {
    case 0:
            sHelp = "run number of Empty Cell [ Empty Beam]:\n\n I[sample]- Tr I[EC]";
            break;
    case 1:
            sHelp = "run number of Black Current [BC, cadmium],\n electronic background  of detector and background from other instruments :\n\n I[sample]-I[BC], I[EC]-I[BC]";
            break;
            
    }
    QMessageBox::information( this, "DAN :: Smart Help", sHelp );
}

void dan18::calculateTransmissionAll()
{
    calculateTransmission(0);
    if (checkBoxForceCopyPaste->isChecked()) copycorrespondentTransmissions();
}

void dan18::calculateTransmission(int startRow)
{
    ImportantConstants();
    //+++
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++  Subtract Bufffer
    bool subtractBuffer= false;
    if (comboBoxMode->currentText().contains("(BS)")) subtractBuffer=true;
    
    
    //+++  Subtract Bufffer && Correct to own Sensitivity
    bool bufferAsSens= false;
    if (comboBoxMode->currentText().contains("(BS-SENS)")) bufferAsSens=true;
    
    
    //+++ optional parameter if column "VShift" exist
    double VShift;
    
    //+++ optional parameter if column "HShift" exist
    double HShift;
    
    //+++
    Table* w;
    
    
    if (!checkTableExistence(tableName)) return;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
    //+++

    //+++ calculate only selected lines
    bool checkSelection=false;
    
    if(startRow==0)
    {
        
        for (int iRow=0; iRow<(int)tableList.count(); iRow++)
        {
            if (w->isRowSelected(iRow,true))
            {
                checkSelection=true;
                break;
            }
        }
        
        
    }
    
    if (w->windowLabel()!="DAN::Script::Table")    	return;
    
    //+++ Indexing
    QStringList scriptColList=w->colNames();
    
    
    
    //+++ #-Condition +++
    int indexCond=scriptColList.indexOf("#-Condition");
    if (indexCond<0) return;
    
    //+++  #-Run +++
    int indexSample=scriptColList.indexOf("#-Run");
    if (indexSample<0) return;
    
    //+++ #-Buffer +++
    int indexBuffer=scriptColList.indexOf("#-Buffer");
    if (subtractBuffer && indexBuffer<0) return;
    
    //+++ #-EC [EB] +++
    int indexEC=scriptColList.indexOf("#-EC [EB]");
    if (indexEC<0)
    {
        indexEC=scriptColList.indexOf("#-EC");
        if (indexEC<0) return;
    }
    //+++ Transmission-Buffer +++
    int indexTrBuffer=scriptColList.indexOf("Transmission-Buffer");
    if (subtractBuffer && indexTrBuffer<0) return;
    
    
    //+++ Transmission-Sample +++
    int indexTr=scriptColList.indexOf("Transmission-Sample");
    if (indexTr<0) return;
    
    //+++ Mask +++
    int indexMask=scriptColList.indexOf("Mask");
    if (indexMask<0) return;
    
    //+++ VShift +++ Hand-made column
    int indexVShift=scriptColList.indexOf("VShift");
    
    //+++ HShift +++ Hand-made column
    int indexHShift=scriptColList.indexOf("HShift");
    
    
    int rowNumber=w->numRows();
    
    //+++ Calculate transmission
    QString ECnumber;
    double trans=-1;
    double sigmaTr=0;
    int iMax=tableEC->columnCount();
    int iter;
    
    //+++ new active conditions
    QList<int> listTr;
    
    for (int i=0; i<iMax; i++)
    {
        if (tableEC->item(dptECTR, i)->checkState() == Qt::Checked) listTr<<(i+1);
    }
    
    for(iter=startRow; iter<rowNumber;iter++)
    {
        sigmaTr=0;
        
        if (w->text(iter,indexCond)=="") continue;
        
        if (checkSelection && !w->isRowSelected(iter,true)) continue;
        
        if (listTr.indexOf(w->text(iter,indexCond).toInt())>=0)
        {
            VShift=0.0;
            if (indexVShift>0) VShift=w->text(iter,indexVShift).toDouble();
            
            HShift=0.0;
            if (indexHShift>0) HShift=w->text(iter,indexHShift).toDouble();
            
            ECnumber=w->text(iter,indexEC);
            
            if (ECnumber=="" && comboBoxTransmMethod->currentIndex()!=2)
            {
                w->setText(iter,indexTr,"no EC-file");
                if (subtractBuffer)  w->setText(iter,indexTrBuffer,"no EC-file");
            }
            else
            {
                if ( checkFileNumber( ECnumber ) || comboBoxTransmMethod->currentIndex()==2 )
                {
                    QString mask=w->text( iter, indexMask );
                    mask=((QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,w->text(iter,indexCond).toInt()-1))->currentText();
                    sigmaTr=0;
                    trans=readTransmission( w->text( iter,indexSample ), ECnumber, mask,VShift,HShift,sigmaTr);
                    
                    if (trans>2.00 || trans<=0)  w->setText(iter,indexTr, "check!!!");
                    else w->setText(iter,indexTr, "   "+QString::number(trans,'f',4)+" [ "+QChar(177)+QString::number(trans*sigmaTr,'f',4)+" ]");
                    if (subtractBuffer)
                    {
                        sigmaTr=0;
                        trans=readTransmission( w->text(iter,indexBuffer), ECnumber,mask,VShift,HShift,sigmaTr);
                        
                        if (trans>2.00 || trans<=0)  w->setText(iter,indexTrBuffer, "check!!!");
                        else w->setText(iter,indexTrBuffer, "   "+QString::number(trans,'f',4)+" [ "+QChar(177)+QString::number(trans*sigmaTr,'f',4)+" ]");
                    }
                }
                else 
                {
                    w->setText(iter,indexTr, "check file!!!");
                    if (subtractBuffer)  w->setText(iter,indexTrBuffer,"check file!!!");
                }
                
            }	    
        }
        else 
        {
            w->setText(iter,indexTr, "Not active configuration"); 
            if (subtractBuffer) w->setText(iter,indexTrBuffer, "Not active configuration"); 
        }
    }
}

//+-1-+SLOT::calculate absolute constants ++++++++++++++++++
bool dan18::calcAbsCalNew( )
{
    int i;
    
    // Number of C-D conditionws
    int Nconditions=tableEC->columnCount();
    
    // find center for each C-D condition
    
    int caseMethod=comboBoxACmethod->currentIndex();
    
    for(i=0;i<Nconditions;i++)
    {
        switch (caseMethod)
        {
            case 0: calcAbsCalNew( i ); break;
            case 1: calcAbsCalDB( i ); break;
            case 2: calcAbsCalTrFs(i ); break;
            case 3: calcAbsCalNew( i ); break;
        }
    }
    return true;
}

//+-2-+SLOT::calculate absolute constants +++++++++++++++++++++++++++++++++++++
bool dan18::calcAbsCalTrFs( int col )
{
    ImportantConstants();
    
    QString maskName=comboBoxMaskFor->currentText();
    QString sensName=comboBoxSensFor->currentText();
    
    maskName=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText();
    if (!checkExistenceOfMask(QString::number(MD), maskName)) return false;
    sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
    if (!checkExistenceOfSens(QString::number(MD), sensName)) return false;
    
    //+++ Mask & Sens +++
    QStringList listMask, listSens;
    QString winLabelMask="DAN::Mask::"+QString::number(MD);
    QString winLabelSens="DAN::Sensitivity::"+QString::number(MD);
    
    findMatrixListByLabel(winLabelMask, listMask);
    if (!listMask.contains("mask")) listMask.prepend("mask");
    findMatrixListByLabel(winLabelSens, listSens);
    if (!listSens.contains("sens")) listSens.prepend("sens");
    
    //+++ mask reading +++
    if (listMask.contains(((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText()))
    {
        maskName=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText();
    }
    
    //+++ sens reading +++
    if (listSens.contains(((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText()))
    {
        sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
    }
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(mask);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(sens);
    make_GSL_Matrix_Symmetric( sensName, sens, MD);
    
    sensAndMaskSynchro(mask, sens, MD );
    
    //+++ Variables
    int   ii, jj;
    
    double D=tableEC->item(dptDAC,col)->text().toDouble();
    
    double transmision=0.0;
    
    bool existPlexi, existEB,  existBC;
    
    double sum=0.0;
    
    double Nmask=0.0;
    
    double  cal;
    
    //+++ Matrix allocation
    gsl_matrix *plexi=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexi);
    gsl_matrix *plexiEB=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexiEB);
    gsl_matrix *plexiBC=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexiBC);
    
    //+++Check existence of plexi File
    QString PlexiNumber=tableEC->item(dptACFS,col)->text();
    
    existPlexi=checkFileNumber( PlexiNumber );
    
    if (!existPlexi)
    {
        QMessageBox::warning(this,tr("qtiSAS"), tr(QString("C-D-Lambda condition # " + QString::number(col+1) + ":: plexi-file does not exist!").toLocal8Bit().constData()));
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: plexi-file does not exist!"+"\n");
        
        tableEC->item(dptACFS,col)->setText("0");
        tableEC->item(dptACFAC,col)->setText(QString::number(1.0,'E',1));
        
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(plexi);
        gsl_matrix_free(plexiEB);
        gsl_matrix_free(plexiBC);
        return false;
    }
    
    // read Plexi matrix
    readMatrixCor( PlexiNumber, plexi );
    
    //check existence of EB File
    QString EBNumber=tableEC->item(dptACEB,col)->text();
    
    existEB=checkFileNumber( EBNumber );
    if (!existEB)
    {
        //*************************************Log Window Output
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: EBplexi-file does not exist! Absolute calibration without EB subtraction !"+"\n");
        //*************************************Log Window Output
        tableEC->item(dptACEB,col)->setText("0");
    }
    else
    {
        // read EB matrix
        readMatrixCor( EBNumber, plexiEB );
        
        transmision=tableEC->item(dptACTR,col)->text().toDouble();
        if (transmision>1.0) transmision=1.0;
        if (transmision<0.01) transmision=0.0;
        tableEC->item(dptACTR,col)->setText(QString::number(transmision, 'f',4));
    }
    
    //check existence of BC File
    QString BCNumber=tableEC->item(dptACBC,col)->text();
    existBC=checkFileNumber( BCNumber );
    if (!existBC)
    {
        //*************************************Log Window Output
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: BCplexi-file does not exist! Absolute calibration without BCplexi-subtraction !"+"\n");
        //*************************************Log Window Output
        tableEC->item(dptACBC,col)->setText("0");
    }
    else
    {
        // read BC matrix 2012
        if (checkBoxBCTimeNormalization->isChecked())
        {
            readMatrixCorTimeNormalizationOnly( BCNumber, plexiBC );
            
            //Normalization constant
            double TimeSample=spinBoxNorm->value();
            double ttime=readDuration( EBNumber );
            if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;
            
            double NormSample=readDataNormalization(EBNumber);
            
            if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
            
            gsl_matrix_scale(plexiBC,NormSample);      // EB=T*EB
        }
        else readMatrixCor( BCNumber, plexiBC );
    }
    
    gsl_matrix_sub(plexiEB,plexiBC);            // EB=EB - BC
    
    
    if (existBC && checkBoxBCTimeNormalization->isChecked())
    {
        // read BC matrix 2012
        readMatrixCorTimeNormalizationOnly( BCNumber, plexiBC );
        
        //Normalization constant
        double TimeSample=spinBoxNorm->value();
        double ttime=readDuration( PlexiNumber );
        if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;
        
        //
        double NormSample=readDataNormalization(PlexiNumber);
        
        if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
        
        gsl_matrix_scale(plexiBC,NormSample);      // EB=T*EB
    }
    
    gsl_matrix_sub(plexi,plexiBC);              // plexi=plexi - BC
    
    
    
    double Xc, Yc;
    readCenterfromMaskName( maskName, Xc, Yc, MD );
    
    
    if (transmision<1.0 && transmision>0.0 && checkBoxParallax->isChecked())
    {
        transmissionThetaDependenceTrEC(plexiEB, Xc, Yc, D*100.0, transmision);
    }
    
    gsl_matrix_scale(plexiEB,transmision);      // EB=T*EB
    gsl_matrix_sub(plexi,plexiEB);                      // plexi=plexi  - EB
    
    gsl_matrix_mul_elements(plexi,mask);
    gsl_matrix_mul_elements(plexi,sens);
    
    //+++
    if (checkBoxParallax->isChecked())
    {
        parallaxCorrection(plexi, Xc, Yc, D*100.0,transmision);
    }
    
    
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        if (gsl_matrix_get(mask,ii,jj)>0)
        {
            if (gsl_matrix_get(plexi,ii,jj)<=0.0    && checkBoxMaskNegative->isChecked() )
            {
                gsl_matrix_set(plexi,ii,jj,0.0);
                gsl_matrix_set(mask,ii,jj,0.0);
                gsl_matrix_set(sens,ii,jj,0.0);
            }
            else
            {
                Nmask+=gsl_matrix_get(mask,ii,jj);
                sum+=gsl_matrix_get(plexi,ii,jj);
            }
        }
    }
    
    //Error
    double sigma2=0.0;
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        if (gsl_matrix_get(mask,ii,jj)>0.0)
            sigma2+=(sum/Nmask-gsl_matrix_get(plexi,ii,jj))*(sum/Nmask-gsl_matrix_get(plexi,ii,jj));
    }
    
    sigma2=sigma2/Nmask/ (Nmask-1.0);
    sigma2=sqrt(sigma2);
   
    //+++
    double mu=tableEC->item(dptACMU,col)->text().toDouble();
    double Ds=tableEC->item(dptD,col)->text().toDouble();
    double transEC=tableEC->item(dptECTR,col)->text().toDouble();
    
    if (transEC > 1.5 || transEC <= 0.0)
    {
        transEC = 1.0;
        tableEC->item(dptECTR, col)->setText("1.000");
    }

    
    //+++
    cal=mu*(1-transmision)/sum*Nmask/D/D*Ds*Ds/transEC;
    sigma2=mu*(1-transmision)/sum*Nmask/sum/D/D*Ds*Ds*sigma2;
    //+++
    tableEC->item(dptACFAC,col)->setText(QString::number(cal,'E',4));
    //tableEC->item(14,col)->setText(QString::number(sigma2,'E',1));
    toResLog("DAN :: Abs.Factor | Condition #"+QString::number(col+1)+" | "+ QString::number(cal,'E',4)+QChar(177)+QString::number(sigma2,'E',4)+"\n");
    
    //+++
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    gsl_matrix_free(plexi);
    gsl_matrix_free(plexiEB);
    gsl_matrix_free(plexiBC);
    return true;
}

//+-2-+SLOT::calculate absolute constants +++++++++++++++++++++++++++++++++++++
bool dan18::calcAbsCalDB( int col )
{
    //+++
    ImportantConstants();
    //+++
    QString maskName=comboBoxMaskFor->currentText();
    QString sensName=comboBoxSensFor->currentText();
    
    if ( !checkBoxACDBuseActive->isChecked() ) maskName=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText();
    if ( !checkExistenceOfMask(QString::number(MD), maskName)) return false;
    
    if ( !checkBoxACDBuseActive->isChecked() )   sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
    if ( !checkExistenceOfSens(QString::number(MD), sensName)) return false;
    
    //+++ Mask & Sens +++
    QStringList listMask, listSens;
    QString winLabelMask="DAN::Mask::"+QString::number(MD);
    QString winLabelSens="DAN::Sensitivity::"+QString::number(MD);
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(mask);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(sens);
    make_GSL_Matrix_Symmetric( sensName, sens, MD);
    
    sensAndMaskSynchro(mask, sens, MD );
    
    
    //+++ Variables
    int   ii, jj;
    QString fileNumber;
    
    bool existEB;
    
    double sum=0.0;
    
    double Nmask=0.0;
    
    double  cal;
    
    //+++ Matrix allocation
    gsl_matrix *EB=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(EB);
    gsl_matrix *BC=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(BC);
    
    //+++Check existence of plexi File
    QString EBNumber=tableEC->item(dptACEB,col)->text();
    
    existEB=checkFileNumber( EBNumber );
    
    if (!existEB)
    {
        QMessageBox::warning(this,tr("qtiSAS"), tr(QString("C-D-Lambda condition # " + QString::number(col+1) + " :: EB-file does not exist!").toLocal8Bit().constData()));
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+" :: EB-file does not exist!"+"\n");
        
        tableEC->item(dptACEB,col)->setText("0");
        tableEC->item(dptACFAC,col)->setText(QString::number(1.0,'E',1));
        
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(EB);
        gsl_matrix_free(BC);
        return false;
    }
    readMatrixCor( EBNumber, EB );
    
    //check existence of BC File
    QString BCNumber=tableEC->item(dptACBC,col)->text();
    bool existBC=checkFileNumber( BCNumber );
    if (!existBC)
    {
        //*************************************Log Window Output
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: BC-file does not exist! Absolute calibration without BC-subtraction !"+"\n");
        //*************************************Log Window Output
        tableEC->item(dptACBC,col)->setText("0");
    }
    else
    {
        // read BC matrix 2012
        if (checkBoxBCTimeNormalization->isChecked())
        {
            readMatrixCorTimeNormalizationOnly( BCNumber, BC );
            
            //Normalization constant
            double TimeSample=spinBoxNorm->value();
            double ttime=readDuration( EBNumber );
            if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;
            
            
            double NormSample=readDataNormalization(EBNumber);
            
            if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
            
            gsl_matrix_scale(BC,NormSample);      // EB=T*EB
        }
        else readMatrixCor( BCNumber, BC );
        
        gsl_matrix_sub(EB, BC);            // EB=EB - BC
    }
    
    
    
    gsl_matrix_mul_elements(EB,mask);
    gsl_matrix_mul_elements(EB,sens);
    
    double transEC=tableEC->item(dptECTR,col)->text().toDouble();
    
    if (transEC > 1.5 || transEC <= 0.0)
    {
        transEC = 1.0;
        tableEC->item(dptECTR, col)->setText("1.000");
    }
    
    // atten
    double attenuation = tableEC->item(dptACTR,col)->text().toDouble();
    if (attenuation<=0.0 || attenuation>1.0) attenuation=1.0;
    
    // mu
    double mu = tableEC->item(dptACMU,col)->text().toDouble();
    if (mu<=0) attenuation=1.0;
    
    double D=tableEC->item(dptDAC,col)->text().toDouble();
    D*=100.0; // cm
    
    double area=lineEditPS->text().toDouble();
    area=area*area;
    area*=lineEditAsymetry->text().toDouble(); // cm^2
    
    
    double I0=0;
    
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        I0+=gsl_matrix_get(EB,ii,jj);
    }
    
    I0/=attenuation;
    I0*=mu;
    I0*=area/D/D;
    I0*=transEC; // 2019 new
    
    //+++
    tableEC->item(dptACFAC,col)->setText(QString::number(1/I0,'E',4));
    toResLog("DAN :: Abs.Factor (DB) | Condition #"+QString::number(col+1)+" | "+ QString::number(1/I0,'E',4)+"\n");
    
    //+++
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    gsl_matrix_free(EB);
    gsl_matrix_free(BC);
    return true;
}

//+-2-+SLOT::calculate absolute constants ++++++++++++++++++++++++++++
bool dan18::calcAbsCalNew( int col )
{
    ImportantConstants();
    
    QString maskName=comboBoxMaskFor->currentText();
    QString sensName=comboBoxSensFor->currentText();
    
    maskName=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText();
    if (!checkExistenceOfMask(QString::number(MD), maskName)) return false;
    sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
    if (!checkExistenceOfSens(QString::number(MD), sensName)) return false;
    
    //+++ Mask & Sens +++
    QStringList listMask, listSens;
    QString winLabelMask="DAN::Mask::"+QString::number(MD);
    QString winLabelSens="DAN::Sensitivity::"+QString::number(MD);
    
    findMatrixListByLabel(winLabelMask, listMask);
    if (!listMask.contains("mask")) listMask.prepend("mask");
    findMatrixListByLabel(winLabelSens, listSens);
    if (!listSens.contains("sens")) listSens.prepend("sens");
    
    //+++ mask reading +++
    if (listMask.contains(((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText()) )
    {
        maskName=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText();
    }
    
    //+++ mask reading +++
    if (listSens.contains(((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText()) )
    {
        sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
    }
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(mask);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(sens);
    make_GSL_Matrix_Symmetric( sensName, sens, MD);
    
    sensAndMaskSynchro(mask, sens, MD );
    
    //+++ Variables
    int   ii, jj;
    
    double D=tableEC->item(dptDAC,col)->text().toDouble();
    
    double transmision=0.0;
    
    bool existPlexi, existEB,  existBC;
    
    double sum=0.0;
    
    double Nmask=0.0;
    
    double  cal;
    
    //+++ Matrix allocation
    gsl_matrix *plexi=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexi);
    gsl_matrix *plexiEB=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexiEB);
    gsl_matrix *plexiBC=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexiBC);
    
    //+++Check existence of plexi File
    QString PlexiNumber=tableEC->item(dptACFS,col)->text();
    
    existPlexi=checkFileNumber( PlexiNumber );
    
    if (!existPlexi)
    {
        if (checkBoxSkiptransmisionConfigurations->isChecked())
            QMessageBox::warning(this,tr("qtiSAS"), tr(QString("C-D-Lambda condition # "+QString::number(col+1)+":: plexi-file does not exist!").toLocal8Bit().constData()));

        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: plexi-file does not exist!"+"\n");

        tableEC->item(dptACFS,col)->setText("0");
        tableEC->item(dptACFAC,col)->setText(QString::number(1.0,'E',1));
        
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(plexi);
        gsl_matrix_free(plexiEB);
        gsl_matrix_free(plexiBC);
        return false;
    }
    
    // read Plexi matrix
    readMatrixCor(PlexiNumber, plexi);
    // D = detector->readDinM(PlexiNumber);

    // check existence of EB File
    QString EBNumber = tableEC->item(dptACEB, col)->text();

    existEB = checkFileNumber(EBNumber);
    if (!existEB)
    {
        //*************************************Log Window Output
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: EBplexi-file does not exist! Absolute calibration without EB subtraction !"+"\n");
        //*************************************Log Window Output
        tableEC->item(dptACEB,col)->setText("0");
    }
    else
    {
        // read EB matrix
        readMatrixCor( EBNumber, plexiEB );
        
        transmision=tableEC->item(dptACTR,col)->text().toDouble();
        if (transmision>1.0) transmision=1.0;
        if (transmision<0.01) transmision=0.0;
        tableEC->item(dptACTR,col)->setText(QString::number(transmision, 'f',4));
    }
    
    //check existence of BC File
    QString BCNumber=tableEC->item(dptACBC,col)->text();
    existBC=checkFileNumber( BCNumber );
    if (!existBC)
    {
        //*************************************Log Window Output
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: BCplexi-file does not exist! Absolute calibration without BCplexi-subtraction !"+"\n");
        //*************************************Log Window Output
        tableEC->item(dptACBC,col)->setText("0");
    }
    else
    {
        // read BC matrix 2012
        if (checkBoxBCTimeNormalization->isChecked())
        {
            readMatrixCorTimeNormalizationOnly( BCNumber, plexiBC );
            
            //Normalization constant
            double TimeSample=spinBoxNorm->value();
            double ttime=readDuration( EBNumber );
            if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;
            
            double NormSample=readDataNormalization(EBNumber);
            
            if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
            
            gsl_matrix_scale(plexiBC,NormSample);      // EB=T*EB
        }
        else readMatrixCor( BCNumber, plexiBC );
    }
    
    gsl_matrix_sub(plexiEB,plexiBC);            // EB=EB - BC
    
    
    if (existBC && checkBoxBCTimeNormalization->isChecked())
    {
        // read BC matrix 2012
        readMatrixCorTimeNormalizationOnly( BCNumber, plexiBC );
        
        //Normalization constant
        double TimeSample=spinBoxNorm->value();
        double ttime=readDuration( PlexiNumber );
        if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;
        
        //
        double NormSample=readDataNormalization(PlexiNumber);
        
        if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
        
        gsl_matrix_scale(plexiBC,NormSample);      // EB=T*EB
    }
    
    gsl_matrix_sub(plexi,plexiBC);              // plexi=plexi - BC
    
    
    
    double Xc, Yc;
    readCenterfromMaskName( maskName, Xc, Yc, MD );
    
    
    if (transmision<1.0 && transmision>0.0 && checkBoxParallax->isChecked())
    {
        transmissionThetaDependenceTrEC(plexiEB, Xc, Yc, D*100.0, transmision);
    }
    
    gsl_matrix_scale(plexiEB,transmision);      // EB=T*EB
    gsl_matrix_sub(plexi,plexiEB);                      // plexi=plexi  - EB
    
    gsl_matrix_mul_elements(plexi,mask);
    gsl_matrix_mul_elements(plexi,sens);
    
    //+++
    if (checkBoxParallax->isChecked())
    {
        parallaxCorrection(plexi, Xc, Yc, D*100.0,transmision);
    }
    
    
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        if (gsl_matrix_get(mask,ii,jj)>0)
        {
            if (gsl_matrix_get(plexi,ii,jj)<=0.0    && checkBoxMaskNegative->isChecked() )
            {
                gsl_matrix_set(plexi,ii,jj,0.0);
                gsl_matrix_set(mask,ii,jj,0.0);
                gsl_matrix_set(sens,ii,jj,0.0);
            }
            else
            {
                Nmask+=gsl_matrix_get(mask,ii,jj);
                sum+=gsl_matrix_get(plexi,ii,jj);
            }
        }
    }
    
    //Error
    double sigma2=0.0;
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        if (gsl_matrix_get(mask,ii,jj)>0.0)
            sigma2+=(sum/Nmask-gsl_matrix_get(plexi,ii,jj))*(sum/Nmask-gsl_matrix_get(plexi,ii,jj));
    }
    
    sigma2=sigma2/Nmask/ (Nmask-1.0);
    sigma2=sqrt(sigma2);
    //+++
    double mu=tableEC->item(dptACMU,col)->text().toDouble();
    double Ds=tableEC->item(dptD,col)->text().toDouble();

    double transEC=tableEC->item(dptECTR,col)->text().toDouble();
    
    if (transEC > 1.5 || transEC <= 0.0)
    {
        transEC = 1.0;
        tableEC->item(dptECTR, col)->setText("1.000");
    }

    
    //+++
    cal=mu/sum*Nmask/D/D*Ds*Ds/transEC;
    sigma2=mu/sum*Nmask/sum/D/D*Ds*Ds*sigma2;
    //+++
    tableEC->item(dptACFAC,col)->setText(QString::number(cal,'E',4));
    toResLog("DAN :: Abs.Factor | Condition #"+QString::number(col+1)+" | "+ QString::number(cal,'E',4)+QChar(177)+QString::number(sigma2,'E',4)+"\n");
    
    //+++
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);    
    gsl_matrix_free(plexi);
    gsl_matrix_free(plexiEB);
    gsl_matrix_free(plexiBC);   
    return true;
}

//+++
bool dan18::compareSamplePositions( QString n1, QString n2 )
{
    if ( !checkBoxRecalculateUseNumber->isChecked()) return true;
    
    if ( readSampleNumber( n1 )==readSampleNumber( n2 )) return true;
    else return false;
}

//+++
bool dan18::compareAttenuators( QString n1, QString n2 )
{
    if ( !checkBoxAttenuatorAsPara->isChecked()) return true;
    
    if (readAttenuator(n1)!=readAttenuator(n2)) return false;
    
    return true;
}

//+++
bool dan18::compareBeamPosition( QString n1, QString n2 )
{
    if ( !checkBoxBeamcenterAsPara->isChecked()) return true;
    
    if ( fabs( double ( readDetectorX( n1 ) - readDetectorX( n2 )) )> 5.0 ) return false;
    if ( fabs( double ( readDetectorY( n1 ) - readDetectorY( n2 )) )> 5.0 ) return false;
    
    return true;
}

//+++
bool dan18::comparePolarization( QString n1, QString n2 )
{
    if ( !checkBoxPolarizationAsPara->isChecked()) return true;
    
    if (readPolarization(n1)!=readPolarization(n2)) return false;
    
    return true;
}

//+++
bool dan18::compareDetAnglePosition( QString n1, QString n2 )
{
    if ( !checkBoxDetRotAsPara->isChecked()) return true;
    
    if ( fabs( double ( readDetRotationX( n1 ) - readDetRotationX( n2 )) )> 0.015 ) return false;
    if ( fabs( double ( readDetRotationY( n1 ) - readDetRotationY( n2 )) )> 0.015 ) return false;
    
    return true;
}

void dan18::updateScriptTables()
{
    QStringList  list;
    findTableListByLabel("DAN::Script::Table",list);
    //    list.prepend("New-Script-Table");
    comboBoxMakeScriptTable->clear();
    comboBoxMakeScriptTable->addItems(list);
}

//+++ calculation of mu
double dan18::muCalc(double lambda)
{
    double mu0=lineEditMuY0->text().toDouble();
    double muA=lineEditMuA->text().toDouble();
    double lambdaMu=lineEditMut->text().toDouble();
    
    return mu0+muA*exp(lambdaMu/lambda);
}


//+++ calculation of Transmission
double dan18::tCalc(double lambda)
{
    if (lambda<=0.0) return 1.0;
    double To=lineEditTo->text().toDouble();
    double TA=lineEditTA->text().toDouble();
    double lambdaT=lineEditLate->text().toDouble();
    
    return To-TA*exp(-lambdaT/lambda);
}

void dan18::readCenterfromMaskName( QString maskName, double &Xc, double &Yc, int MD )
{
    MdiSubWindow* mmm=(MdiSubWindow*)app()->window(maskName);
    
    if (!mmm)
    {
        Xc=double( spinBoxRBxBS->value() + spinBoxLTxBS->value() ) / 2.0 - 1.0;
        Yc=double( spinBoxRByBS->value() + spinBoxLTyBS->value() ) / 2.0 - 1.0;
        return;
    }
    
    QString  label=mmm->windowLabel();
    
    label=label.remove("DAN::Mask::"+QString::number(MD));
    label=label.replace("|"," ").simplified();
    
    QStringList lst;
    lst = label.split(" ", QString::SkipEmptyParts);
    
    if (lst.count() > 7)
    {
        Xc=(lst[4].toDouble()+lst[5].toDouble())/2.0-1;
        Yc=(lst[6].toDouble()+lst[7].toDouble())/2.0-1;
    }
    else
    {
        Xc=double( spinBoxRBxBS->value() + spinBoxLTxBS->value() ) / 2.0 - 1.0;
        Yc=double( spinBoxRByBS->value() + spinBoxLTyBS->value() ) / 2.0 - 1.0;
    }
    
}

//+++
void dan18::updateCenterAll()
{
    updateCenter(0);
}

//+++
void dan18::updateCenter(int startRow)
{
    
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++
    Table* w;
    
    
    if (!checkTableExistence(tableName)) return;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
    //+++
    
    if (w->windowLabel()!="DAN::Script::Table")    	return;
    
    //+++ Indexing
    QStringList scriptColList=w->colNames();
    
    
    
    //+++ #-Condition +++
    int indexCond=scriptColList.indexOf("#-Condition");
    if (indexCond<0) return;
    
    //+++ X-center +++
    int indexXC=scriptColList.indexOf("X-center");
    if (indexXC<0) return;
    
    //+++ Ycenter +++
    int indexYC=scriptColList.indexOf("Y-center");
    if (indexYC<0) return;
    
    
    int rowNumber=w->numRows();
    int cond;
    
    for(int iter=startRow; iter<rowNumber;iter++)
    {
        cond=w->text(iter,indexCond).toInt();
        if (cond>0)
        {
            w->setText(iter,indexXC, tableEC->item(dptCENTERX,int(fabs(double(cond)))-1)->text().left(6));
            w->setText(iter,indexYC, tableEC->item(dptCENTERY,int(fabs(double(cond)))-1)->text().left(6));
        }
    }
}


void dan18::sensAndMaskSynchro( gsl_matrix* &mask, gsl_matrix* &sens, int MaDe )
{
    for(int i=0;i<MaDe;i++)for(int j=0;j<MaDe;j++)
    {
        if (gsl_matrix_get(mask, i,j)==0 || gsl_matrix_get(sens, i,j)==0)
        {
            gsl_matrix_set(mask, i,j,0);
            gsl_matrix_set(sens, i,j,0);
        }
    }
}


void dan18::copycorrespondentTransmissions()
{
    copyCorrespondentTransmissions(0);
}

void dan18::copyCorrespondentTransmissions(int startRow)
{
    if (comboBoxMakeScriptTable->count()==0) return;
    
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++  Subtract Bufffer
    bool subtractBuffer= false;
    if (comboBoxMode->currentText().contains("(BS)")) subtractBuffer=true;
    
    //+++
    Table* w;
    
    
    if (!checkTableExistence(tableName)) return;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
    //+++
    
    //+++ calculate only selected lines
    bool checkSelection=false;
    
    if(startRow==0)
    {
        
        for (int iRow=0; iRow<(int)tableList.count(); iRow++)
        {
            if (w->isRowSelected(iRow,true))
            {
                checkSelection=true;
                break;
            }
        }
        
        
    }
    
    if (w->windowLabel()!="DAN::Script::Table")    	return;
    
    //+++ Indexing
    QStringList scriptColList=w->colNames();
    
    //+++  Run-info  +++
    int indexInfo=scriptColList.indexOf("Run-info");
    if (indexInfo<0) return;
    
    //+++ Lambda +++
    int indexLam=scriptColList.indexOf("Lambda");
    if (indexLam<0) return;
    
    //+++ Transmission-Sample +++
    int indexTr=scriptColList.indexOf("Transmission-Sample");
    if (indexTr<0) return;
    
    //+++ Transmission-Buffer +++
    int indexTrBuffer=scriptColList.indexOf("Transmission-Buffer");
    if (subtractBuffer && indexTr<0) return;
    
    //+++ #-Condition +++
    int indexCond=scriptColList.indexOf("#-Condition");
    if (indexCond<0) return;
    
    int rowNumber=w->numRows();
    
    int pos;
    QString name;
    double wl, wlCurr;
    QString trCurr, tr;
    QStringList lst;
    
    for (int i=startRow; i<rowNumber; i++)
    {
        if (w->text(i,indexCond)=="") continue;
        
        if (checkSelection && !w->isRowSelected(i,true)) continue;
        
        
        name=w->text(i,indexInfo);
        wl=w->text(i,indexLam).toDouble();
        
        lst.clear();
        lst = w->text(i, indexTr).remove(" ").split("[", QString::SkipEmptyParts);

        if (lst[0].toDouble()<=0)
        {
            tr="";
            
            for (int j=0;j<rowNumber; j++)
            {
                wlCurr=w->text(j,indexLam).toDouble();
                trCurr=w->text(j, indexTr);//+++2019 .toDouble();
                lst.clear();
                lst = w->text(j, indexTr).remove(" ").split("[", QString::SkipEmptyParts);

                //+++2019 if (i!=j && name==w->text(j,indexInfo) && trCurr>0 && wlCurr<1.05*wl && wlCurr>0.95*wl) tr=trCurr;
                
                if (i!=j && name==w->text(j,indexInfo) && lst[0].toDouble()>0 && wlCurr<1.05*wl && wlCurr>0.95*wl) tr=trCurr;
            }
            
            //+++if (tr>0) w->setText(i,indexTr,QString::number(tr,'f',3)); else w->setText(i,indexTr, "check transmission");
            if (tr!="") w->setText(i,indexTr,tr); else w->setText(i,indexTr, "check transmission");
        }
        
        if (subtractBuffer)
        {
            lst.clear();
            lst = w->text(i, indexTrBuffer).remove(" ").split("[", QString::SkipEmptyParts);
            
            if (lst[0].toDouble()<=0)
            {
                tr="";
                
                for (int j=0;j<rowNumber; j++)
                {
                    wlCurr=w->text(j,indexLam).toDouble();
                    trCurr=w->text(j, indexTrBuffer);//+++2019.toDouble();
                    lst.clear();
                    lst = w->text(j,indexTrBuffer).remove(" ").split("[", QString::SkipEmptyParts);
                    
                    if (i!=j && name==w->text(j,indexInfo) && lst[0].toDouble()>0 && wlCurr<1.05*wl && wlCurr>0.95*wl) tr=trCurr;
                }
                
                //+++2019 if (tr>0) w->setText(i,indexTrBuffer,QString::number(tr,'f',3)); else w->setText(i,indexTrBuffer, "check transmission");
                if (tr!="") w->setText(i,indexTrBuffer,tr); else w->setText(i,indexTrBuffer, "check transmission");
            }
        }
    }
}

void dan18::updateMaskNamesInScript(int startRow, QString headerName)
{
    
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    
    //+++
    Table* w;
    
    
    if (!checkTableExistence(tableName)) return;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
    //+++
    
    if (w->windowLabel()!="DAN::Script::Table")    	return;
    
    //+++ Indexing
    QStringList scriptColList=w->colNames();
    
    
    
    //+++ #-Condition +++
    int indexCond=scriptColList.indexOf("#-Condition");
    if (indexCond<0) return;
    
    //+++  Mask +++
    int index=-1;

    if (headerName.contains("Mask")) index=scriptColList.indexOf("Mask");
    else index=scriptColList.indexOf("Sens");
    
    if (index<0) return;
    
    int rowNumber=w->numRows();
    int cond;
    
    for(int iter=startRow; iter<rowNumber;iter++)
    {
        cond=w->text(iter,indexCond).toInt();
        
        if (headerName.contains("Mask"))
        {
            if (cond==0) w->setText(iter,index,comboBoxMaskFor->currentText());
            else w->setText(iter,index,((QComboBoxInTable*)tableEC->cellWidget(dptMASK,fabs(cond)-1))->currentText());
        }
        else
        {
            if (cond==0) w->setText(iter,index,comboBoxSensFor->currentText());
            else w->setText(iter,index,((QComboBoxInTable*)tableEC->cellWidget(dptSENS,fabs(cond)-1))->currentText());
        }
    }
}



void dan18::updateColInScriptAll(QString colName, int rowIndex)
{
    updateColInScript(0, colName, rowIndex);
}


void dan18::updateColInScript(int startRow, QString colName, int rowIndex)
{
    
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++
    Table* w;
    
    
    if (!checkTableExistence(tableName)) return;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
    //+++
    
    if (w->windowLabel()!="DAN::Script::Table")    	return;
    
    //+++ Indexing
    QStringList scriptColList=w->colNames();
    
    
    
    //+++ #-Condition +++
    int indexCond=scriptColList.indexOf("#-Condition");
    if (indexCond<0) return;
    
    //+++  ColName +++
    int index=scriptColList.indexOf(colName);
    if (index<0) return;
    
    int rowNumber=w->numRows();
    int cond;
    
    for(int iter=startRow; iter<rowNumber;iter++)
    {
        cond=w->text(iter,indexCond).toInt();
        if (cond>0)
        {
            w->setText(iter,index, tableEC->item(rowIndex,int(fabs(double(cond)))-1)->text());
        }
    }
}


//*******************************************
//+++  Merging Table Generation
//*******************************************
bool dan18::generateMergingTable(Table *scriptTable, QStringList generatedTables )
{
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    QString name=scriptTable->name();
    name+="-mergingTemplate";
    
    removeWindows(name);
    
    
    Table *t=app()->newHiddenTable(name,"DAN::Merging::Template", 0, sliderConfigurations->value()+1);
    //Col-Types: Text inisially
    QStringList colType;
    for (int tt=0; tt<t->numCols(); tt++) colType<<"1";
    t->setColumnTypes(colType);

    int i,j;
    
    QStringList usedNames;
    QString currentSample;
    
    int numRows=0;
    
    
    for (i=0;i<(int)generatedTables.count();i++)
    {
        if (scriptTable->text(i,2).toInt()>0 && generatedTables[i]!="-0-")
        {
            currentSample=scriptTable->text(i,0);
            //	    if ( usedNames.grep(currentSample).count()>0 )
            if ( usedNames.count(currentSample)>0 )
            {
                t->setText(usedNames.indexOf(currentSample),scriptTable->text(i,2).toInt(),generatedTables[i]);
            }
            else
            {
                numRows++;
                t->setNumRows(numRows);
                t->setText(numRows-1,0,currentSample);
                t->setText(numRows-1,scriptTable->text(i,2).toInt(),generatedTables[i]);
                usedNames<<currentSample;
            }
        }
    }
    
    
    if (comboBoxMode->currentText().contains("(MS)"))
    {
        
        numRows=t->numRows();
        t->setNumRows(2*numRows);
        QString sTemp;
        
        for (i=0;i<numRows;i++)
        {
            t->setText(i+numRows,0,t->text(i,0)+"-MC");
            t->setText(i,0,t->text(i,0)+"-NC");
            
            for (j=0;j<sliderConfigurations->value();j++)
            {
                sTemp=t->text(i,j+1);
                t->setText(i+numRows,j+1,sTemp.replace("NC","MC"));
            }
        }
    }
    
    
    //   return true;
    for (i=0;i<t->numRows();i++)
    {
        currentSample=t->text(i,0);
        currentSample=currentSample.replace("]"," ");
        currentSample=currentSample.replace("[","s");
        currentSample=currentSample.simplified();
        currentSample=currentSample.replace(" ","-");
        t->setText(i,0,currentSample);
    }
    
    //+++ adjust cols
    t->adjustColumnsWidth(false);

    app()->setListViewLabel(t->name(), "DAN::Merging::Template");
    app()->updateWindowLists(t);
    
    return true;
}


//+++ 2020
void dan18::calculateCentersInScript()
{
    calculateCentersInScript(0);
}

void dan18::calculateCentersInScript(int startRow)
{
    ImportantConstants();
    //+++
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++ optional parameter if column "VShift" exist
    double VShift;
    
    //+++ optional parameter if column "HShift" exist
    double HShift;
    
    //+++
    Table* w;
    
    //+++
    if (!checkTableExistence(tableName)) return;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName) { w=(Table *)t; break;}
    
    //+++ calculate only selected lines
    bool checkSelection=false;
    
    if(startRow==0)
    {
        for (int iRow=0; iRow<(int)tableList.count(); iRow++)
        {
            if (w->isRowSelected(iRow,true))
            {
                checkSelection=true;
                break;
            }
        }
    }
    
    if (w->windowLabel()!="DAN::Script::Table")        return;
    
    //+++ Indexing
    QStringList scriptColList=w->colNames();
    
    //+++ #-Condition +++
    int indexCond=scriptColList.indexOf("#-Condition"); if (indexCond<0) return;
    
    //+++  #-Run +++
    int indexSample=scriptColList.indexOf("#-Run"); if (indexSample<0) return;
    
    //+++  #-BC +++
    int indexBC=scriptColList.indexOf("#-BC"); if (indexBC<0)return;
    
    //+++ #-EC [EB] +++
    int indexEC=scriptColList.indexOf("#-EC [EB]"); if (indexEC<0) return;

    //+++ Transmission-Sample +++
    int indexTr=scriptColList.indexOf("Transmission-Sample"); if (indexTr<0) return;
    
    //+++ Mask +++
    int indexMask=scriptColList.indexOf("Mask"); if (indexMask<0) return;

    //+++ Sens +++
    int indexSens=scriptColList.indexOf("Sens"); if (indexSens<0) return;
    
    //+++ VShift +++ Hand-made column
    int indexVShift=scriptColList.indexOf("VShift");
    
    //+++ HShift +++ Hand-made column
    int indexHShift=scriptColList.indexOf("HShift");
    
    //+++  X-center +++
    int indexXC=scriptColList.indexOf("X-center"); if (indexXC<0)return;
    
     //+++  Y-center +++
    int indexYC=scriptColList.indexOf("Y-center"); if (indexYC<0)return;
    
    int rowNumber=w->numRows();
    
    //+++ Calculate transmission
    QString SAMPLEnumber="";
    QString ECnumber="";
    QString BCnumber="";
    

    int iMax=tableEC->columnCount();
    int iter;
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    //+++ Sample allocation
    gsl_matrix *sample=gsl_matrix_alloc(MD,MD);
    //+++ EC/EB allocation
    gsl_matrix *ec=gsl_matrix_alloc(MD,MD);
    //+++ BC allocation
    gsl_matrix *bc=gsl_matrix_alloc(MD,MD);
    
    double trans=1.0;
    
    double Xcenter, Ycenter, XcErr, YcErr;
    
    QStringList lst;
    
    std::cout<<"\n"<<std::flush;
    
    for(iter=startRow; iter<rowNumber;iter++)
    {
        if (w->text(iter,indexCond)=="") continue;
        if (checkSelection && !w->isRowSelected(iter,true)) continue;
        

        VShift=0.0;
        if (indexVShift>0) VShift=w->text(iter,indexVShift).toDouble();
            
        HShift=0.0;
        if (indexHShift>0) HShift=w->text(iter,indexHShift).toDouble();
        
        SAMPLEnumber=w->text(iter,indexSample);
        if ( !checkFileNumber( SAMPLEnumber )) continue;
        
        ECnumber=w->text(iter,indexEC);
        if ( !checkFileNumber( ECnumber )) ECnumber="";

        BCnumber=w->text(iter,indexBC);
        if ( !checkFileNumber( BCnumber )) BCnumber="";
        
        QString maskName=w->text( iter, indexMask );

        QString sensName=w->text( iter, indexSens );
        
        double trans=1.0;
        
        
        //+++ mask gsl matrix
        gsl_matrix_set_all(mask, 1.0);
        make_GSL_Matrix_Symmetric( maskName, mask, MD);
        
        //+++ sens gsl matrix
        gsl_matrix_set_all(sens, 1.0);
        make_GSL_Matrix_Symmetric( sensName, sens, MD);
        
        //+++ sample
        gsl_matrix_set_zero(sample);
        readMatrixCor( SAMPLEnumber, sample );
        gslMatrixShift(sample, MD, HShift, VShift );
        
        //+++ ec
        gsl_matrix_set_zero(ec);
        if (ECnumber!="") readMatrixCor( ECnumber,ec);
        
        //+++ bc
        gsl_matrix_set_zero(bc);
        
        if (BCnumber!="")
        {
            if (checkBoxBCTimeNormalization->isChecked())
            {
                readMatrixCorTimeNormalizationOnly( BCnumber, bc );
                //Normalization constant
                double TimeSample=spinBoxNorm->value();
                double ttime=readDuration( SAMPLEnumber );
                if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;
                
                double NormSample=readDataNormalization(SAMPLEnumber);
                
                if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
                
                gsl_matrix_scale(bc,NormSample);
            }
            else readMatrixCor( BCnumber, bc);
        }
        
        //+++ transmission check
        lst.clear();

        QString s=w->text(iter,indexTr);

        s=s.remove(" ").remove(QChar(177)).remove("\t").remove("]");
        lst = s.split("[", QString::SkipEmptyParts);

        trans=lst[0].toDouble();

        //+++ X-center check
        Xcenter=w->text(iter,indexXC).toDouble();
        
        //+++ Y-center check
        Ycenter=w->text(iter,indexYC).toDouble();
        
        gsl_matrix_sub(sample,bc);                       // SAMPLE=SAMPLE-BC
        gsl_matrix_sub(ec,bc);                          // EC=EC-BC
        gsl_matrix_scale(ec,trans);
        gsl_matrix_sub(sample,ec);
        
        //fileNumber=BCFile.toInt();
        gsl_matrix_mul_elements(sample,mask);
        gsl_matrix_mul_elements(sample,sens);
        
        if (radioButtonRadStdSymm->isChecked())
        {
            
            calcCenterUniSymmetryX(MD, sample, mask, Xcenter, XcErr);
            calcCenterUniSymmetryY(MD, sample, mask, Ycenter, YcErr);
        }
        else
        {
            calcCenterUniHF(MD, sample, mask, Xcenter, Ycenter, XcErr, YcErr );
        }
        
        w->setText(iter,indexXC,QString::number(Xcenter));
        w->setText(iter,indexYC,QString::number(Ycenter));
        
        std::cout << "Sample: " << SAMPLEnumber.toLocal8Bit().constData() << "\tXc = " << QString::number(Xcenter,'f',3).toLocal8Bit().constData() << " [+/- " << QString::number(XcErr,'f',3).toLocal8Bit().constData() << "]" << "\tYc = ";
        std::cout << QString::number(Ycenter,'f',3).toLocal8Bit().constData() << " [+/- " << QString::number(YcErr,'f',3).toLocal8Bit().constData() << "]";
        std::cout << "\t" << "... EC: " << ECnumber.toLocal8Bit().constData() << " BC: " << BCnumber.toLocal8Bit().constData() << " Tr: " << trans;
        std::cout << " mask: " << maskName.toLocal8Bit().constData() << " sens: " << sensName.toLocal8Bit().constData();
        std::cout << "\n" << std::flush;
    }
    std::cout<<"\n"<<std::flush;
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(sample);
        gsl_matrix_free(ec);
        gsl_matrix_free(bc);
}

//+++ 2020
void dan18:: calculateAbsFactorInScript()
{
    calculateAbsFactorInScript(0);
}

void dan18::calculateAbsFactorInScript(int startRow)
{
    if (comboBoxACmethod->currentIndex()!=1) return;
    
    ImportantConstants();
    //+++
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++
    Table* w;
    
    //+++
    if (!checkTableExistence(tableName)) return;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName) { w=(Table *)t; break;}
    
    //+++ calculate only selected lines
    bool checkSelection=false;
    
    if(startRow==0)
    {
        for (int iRow=0; iRow<(int)tableList.count(); iRow++)
        {
            if (w->isRowSelected(iRow,true))
            {
                checkSelection=true;
                break;
            }
        }
    }
    
    if (w->windowLabel()!="DAN::Script::Table")        return;
    
    //+++ Indexing
    QStringList scriptColList=w->colNames();
    //+++ #-Condition +++
    int indexCond=scriptColList.indexOf("#-Condition"); if (indexCond<0) return;
    //+++  #-Run +++
    int indexSample=scriptColList.indexOf("#-Run"); if (indexSample<0) return;
    //+++  #-BC +++
    int indexBC=scriptColList.indexOf("#-BC"); if (indexBC<0)return;
    //+++ #-EC [EB] +++
    int indexEC=scriptColList.indexOf("#-EC [EB]"); if (indexEC<0) return;
    //+++ Mask +++
    int indexMask=scriptColList.indexOf("Mask"); if (indexMask<0) return;
    //+++ Sens +++
    int indexSens=scriptColList.indexOf("Sens"); if (indexSens<0) return;
    //+++  Factor +++
    int indexFactor=scriptColList.indexOf("Factor"); if (indexFactor<0) return;
    
    int rowNumber=w->numRows();
    
    //+++ Calculate transmission
    QString ECnumber="";
    QString BCnumber="";
    
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    //+++ EC/EB allocation
    gsl_matrix *ec=gsl_matrix_alloc(MD,MD);
    //+++ BC allocation
    gsl_matrix *bc=gsl_matrix_alloc(MD,MD);
    
    std::cout<<"\n"<<std::flush;

    for(int iter=startRow; iter<rowNumber;iter++)
    {
        if (w->text(iter,indexCond)=="") continue;
        if (checkSelection && !w->isRowSelected(iter,true)) continue;
        

        int col=w->text(iter,indexCond).toInt()-1;
        if (col<0) continue;
        
        ECnumber=w->text(iter,indexEC);
        if ( !checkFileNumber( ECnumber )) continue;
        
        BCnumber=w->text(iter,indexBC);
        if ( !checkFileNumber( BCnumber )) BCnumber="";
        
        QString maskName=w->text( iter, indexMask );
        QString sensName=w->text( iter, indexSens );
        
        //+++ mask gsl matrix
        gsl_matrix_set_all(mask, 1.0);
        make_GSL_Matrix_Symmetric( maskName, mask, MD);
        
        //+++ sens gsl matrix
        gsl_matrix_set_all(sens, 1.0);
        make_GSL_Matrix_Symmetric( sensName, sens, MD);
        
        
        //+++ ec
        gsl_matrix_set_zero(ec);
        readMatrixCor( ECnumber,ec);
        
        //+++ bc
        gsl_matrix_set_zero(bc);
        
        if (BCnumber!="")
        {
            if (checkBoxBCTimeNormalization->isChecked())
            {
                readMatrixCorTimeNormalizationOnly( BCnumber, bc );
                //Normalization constant
                double TimeEC=spinBoxNorm->value();
                double ttime=readDuration( ECnumber );
                if (ttime>0.0) TimeEC/=ttime; else TimeEC=0.0;
                
                double NormEC=readDataNormalization(ECnumber);
                
                if (TimeEC>0) NormEC/=TimeEC; else NormEC=0;
                
                gsl_matrix_scale(bc,NormEC);
            }
            else readMatrixCor( BCnumber, bc);
        }

        gsl_matrix_sub(ec,bc);                          // EC=EC-BC

        gsl_matrix_mul_elements(ec,mask);
        gsl_matrix_mul_elements(ec,sens);

        double transEC=tableEC->item(dptECTR,col)->text().toDouble();
        
        // atten
        double attenuation = tableEC->item(dptACTR,col)->text().toDouble();
        if (attenuation<=0.0 || attenuation>1.0) attenuation=1.0;
        
        // mu
        double mu = tableEC->item(dptACMU,col)->text().toDouble();
        if (mu<=0) attenuation=1.0;
        
        double D=tableEC->item(dptDAC,col)->text().toDouble();
        
        //already in m
        D*=100.0; // cm
        
        double area=lineEditPS->text().toDouble(); //cm^2
        area=area*area;
        area*=lineEditAsymetry->text().toDouble();
        
        double I0=0;
        
        for(int ii=0;ii<MD;ii++) for(int jj=0;jj<MD;jj++)
        {
            I0+=gsl_matrix_get(ec,ii,jj);
        }
        double II=I0;
        I0/=attenuation;
        I0*=mu;
        I0*=area/D/D;
        
        w->setText(iter,indexFactor,QString::number(1/I0,'E',4));
        
        std::cout<<"EC/EB: "<<ECnumber.toLocal8Bit().constData()<<"\tFactor = "<<QString::number(1/I0,'E',4).toLocal8Bit().constData();
        std::cout<<"\t"<<" BC: "<<BCnumber.toLocal8Bit().constData()<<" mask: "<<maskName.toLocal8Bit().constData()<<" sens: "<<sensName.toLocal8Bit().constData()<<" D: "<<D<<" area: "<<area<<" I0: "<<II;
        std::cout<<"\n"<<std::flush;
    }
    std::cout<<"\n"<<std::flush;
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    gsl_matrix_free(ec);
    gsl_matrix_free(bc);
}

void dan18::calculateTrMaskDB()
{
    calculateTrMaskDB(0);
    if (checkBoxForceCopyPaste->isChecked()) copycorrespondentTransmissions();
}

void dan18::calculateTrMaskDB(int startRow)
{
    ImportantConstants();
    //+++
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++  Subtract Bufffer
    bool subtractBuffer= false;
    if (comboBoxMode->currentText().contains("(BS)")) subtractBuffer=true;
    
    
    //+++  Subtract Bufffer && Correct to own Sensitivity
    bool bufferAsSens= false;
    if (comboBoxMode->currentText().contains("(BS-SENS)")) bufferAsSens=true;
    
    
    //+++ optional parameter if column "VShift" exist
    double VShift;
    
    //+++ optional parameter if column "HShift" exist
    double HShift;
    
    //+++
    Table* w;
    
    
    if (!checkTableExistence(tableName)) return;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
    //+++
    
    //+++ calculate only selected lines
    bool checkSelection=false;
    
    if(startRow==0)
    {
        
        for (int iRow=0; iRow<(int)tableList.count(); iRow++)
        {
            if (w->isRowSelected(iRow,true))
            {
                checkSelection=true;
                break;
            }
        }
        
        
    }
    
    if (w->windowLabel()!="DAN::Script::Table")        return;
    
    //+++ Indexing
    QStringList scriptColList=w->colNames();
    
 
    //+++ Mask +++
    int indexMaskDB=scriptColList.indexOf("MaskDB");
    if (indexMaskDB<0)
    {
        QMessageBox::critical( 0, "qtiSAS", "column \"MaskFB\" does not exist!");
        return;
    }
    
    //+++ #-Condition +++
    int indexCond=scriptColList.indexOf("#-Condition");
    if (indexCond<0) return;
    
    //+++  #-Run +++
    int indexSample=scriptColList.indexOf("#-Run");
    if (indexSample<0) return;
    
    //+++ #-Buffer +++
    int indexBuffer=scriptColList.indexOf("#-Buffer");
    if (subtractBuffer && indexBuffer<0) return;
    
    //+++ #-EC [EB] +++
    int indexEC=scriptColList.indexOf("#-EC [EB]");
    if (indexEC<0)
    {
        indexEC=scriptColList.indexOf("#-EC");
        if (indexEC<0) return;
    }
    //+++ Transmission-Buffer +++
    int indexTrBuffer=scriptColList.indexOf("Transmission-Buffer");
    if (subtractBuffer && indexTrBuffer<0) return;
    
    //+++ Transmission-Sample +++
    int indexTr=scriptColList.indexOf("Transmission-Sample");
    if (indexTr<0) return;
    
    //+++ VShift +++ Hand-made column
    int indexVShift=scriptColList.indexOf("VShift");
    
    //+++ HShift +++ Hand-made column
    int indexHShift=scriptColList.indexOf("HShift");
    
    //+++ X-center +++
    int indexXC=scriptColList.indexOf("X-center");
    if (indexXC<0) return;
    
    //+++ Ycenter +++
    int indexYC=scriptColList.indexOf("Y-center");
    if (indexYC<0) return;
    
    int rowNumber=w->numRows();
    
    //+++ Calculate transmission
    QString ECnumber;
    double XCenter, YCenter;
    double trans=-1;
    double sigmaTr=0;
    int iMax=tableEC->columnCount();
    int iter;
    
    //+++ new active conditions
    QList<int> listTr;
    
    for (int i=0; i<iMax; i++)
    {
        if (tableEC->item(dptECTR, i)->checkState() == Qt::Checked) listTr<<(i+1);
    }
    
    for(iter=startRow; iter<rowNumber;iter++)
    {
        sigmaTr=0;
        
        if (w->text(iter,indexCond)=="") continue;
        
        if (checkSelection && !w->isRowSelected(iter,true)) continue;
        
        if (listTr.indexOf(w->text(iter,indexCond).toInt())>=0)
        {
            VShift=0.0;
            if (indexVShift>0) VShift=w->text(iter,indexVShift).toDouble();
            
            HShift=0.0;
            if (indexHShift>0) HShift=w->text(iter,indexHShift).toDouble();
            
            ECnumber=w->text(iter,indexEC);
            
            //+++ X-center check
            XCenter=w->text(iter,indexXC).toDouble();
            
            //+++ Y-center check
            YCenter=w->text(iter,indexYC).toDouble();
            
            if (ECnumber=="" && comboBoxTransmMethod->currentIndex()!=2)
            {
                w->setText(iter,indexTr,"no EC-file");
                if (subtractBuffer)  w->setText(iter,indexTrBuffer,"no EC-file");
                continue;
            }

            
            if ( checkFileNumber( ECnumber ) || comboBoxTransmMethod->currentIndex()==2 )
            {
                double Radius=w->text( iter, indexMaskDB ).toDouble();
                sigmaTr=0;
                trans=readTransmissionMaskDB( w->text( iter,indexSample ),ECnumber,VShift,HShift, XCenter, YCenter, Radius, sigmaTr);

                if (trans>2.00 || trans<=0)  w->setText(iter,indexTr, "check!!!");
                else w->setText(iter,indexTr, "   "+QString::number(trans,'f',4)+" [ "+QChar(177)+QString::number(trans*sigmaTr,'f',4)+" ]");
                if (subtractBuffer)
                {
                    sigmaTr=0;
                    trans=readTransmissionMaskDB( w->text(iter,indexBuffer),ECnumber,VShift,HShift, XCenter, YCenter, Radius, sigmaTr);
                    
                    if (trans>2.00 || trans<=0)  w->setText(iter,indexTrBuffer, "check!!!");
                    else w->setText(iter,indexTrBuffer, "   "+QString::number(trans,'f',4)+" [ "+QChar(177)+QString::number(trans*sigmaTr,'f',4)+" ]");
                }
            }
            else
            {
                w->setText(iter,indexTr, "check file!!!");
                if (subtractBuffer)  w->setText(iter,indexTrBuffer,"check file!!!");
            }
            
        }
        else
        {
            w->setText(iter,indexTr, "Not active configuration");
            if (subtractBuffer) w->setText(iter,indexTrBuffer, "Not active configuration");
        }
    }
}


void dan18::dataProcessingOptionSelected()
{
    if (radioButtonDpSelector2D->isChecked()) stackedWidgetDpOptions->setCurrentIndex(0);
    else if (radioButtonDpSelector1D->isChecked()) stackedWidgetDpOptions->setCurrentIndex(1);
    else stackedWidgetDpOptions->setCurrentIndex(2);
}


