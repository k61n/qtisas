/***************************************************************************
 File : dan18-rawdata-addfiles.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : SANS Data Analisys Interface: add raw data
 
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
//+++  RT tools:: Sum [slot]
//*******************************************
void dan18::addfilesConnectSlots()
{
    connect( pushButtonAddUni , SIGNAL( clicked() ), this, SLOT( addSeveralFilesUniSingleFrame() ) );
    connect( pushButtonAddUniInTable  , SIGNAL( clicked() ), this, SLOT( readTableToAddCols()) );
    connect( pushButtonGenerateAddingTable  , SIGNAL( clicked() ), this, SLOT( generateTableToAdd()) );
}

//*******************************************
//+++  Uni:: Add Several Files [slot]
//*******************************************
void dan18::addSeveralFilesUniSingleFrame()
{
    ImportantConstants();
    
    QString dir 	= Dir;
    QString dirOut   = lineEditPathRAD->text();
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"DAN-SANS :: Add Several (Single) Files",dir,textEditPattern->text());
    fd->setDirectory(dir);
    fd->setFileMode(QFileDialog::ExistingFiles );
    fd->setWindowTitle(tr("DAN-SANS :: Add Several (Single) Files"));
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    QStringList selectedDat=fd->selectedFiles();
    
    int filesNumber= selectedDat.count();
    
    
    if (!selectedDat[0].contains(dir))
    {
        QMessageBox::warning(this,tr("DAN-SANS"), tr("Select data ONLY in INPUT folder!"));
        return;
    }

    // convert name to number
    QStringList numberList;
    QString sss;
    for (int i = 0; i < filesNumber; i++)
    {
        sss = selectedDat[i];
        QString nameFile = FilesManager::findFileNumberInFileName(wildCard, sss.remove(dir));
        numberList << nameFile;
    }

    QString finalNameIndex = numberList[0];
    
    
    if (filesNumber>=999) finalNameIndex=finalNameIndex+"0"+QString::number(filesNumber);
    else if (filesNumber>=99) finalNameIndex=finalNameIndex+"00"+QString::number(filesNumber);
    else if (filesNumber>=9) finalNameIndex=finalNameIndex+"000"+QString::number(filesNumber);
    else finalNameIndex=finalNameIndex+"0000"+QString::number(filesNumber);

    sss = selectedDat[0];
    if (comboBoxHeaderFormat->currentIndex() == 2)
        sss = filesManager->fileNameFull(numberList[0], lineEditWildCard->text());
    sss = sss.replace(dir, dirOut);
    sss = sss.replace(numberList[0], finalNameIndex);

    QString file=sss;
    //    file=dirOut+"/"+file;
    //    file=file.replace("//","/");
    
    
    bool ok;
    file = QInputDialog::getText(this, "qtiSAS", "Enter name of Mergrd File [change suffix  and number]:",
                                 QLineEdit::Normal, file, &ok);
    if ( !ok ) return;
    
    
    addSeveralFilesUniSingleFrame(selectedDat, numberList, file);
}

//*******************************************
//+++  Uni:: Add Several Files [function]
//*******************************************
void dan18::addSeveralFilesUniSingleFrame(QStringList selectedFileList, QStringList selectedNumberList, QString fileName)
{
    if (comboBoxHeaderFormat->currentIndex()==0) addNfilesUniASCII(selectedFileList, selectedNumberList, fileName);

    if (comboBoxHeaderFormat->currentIndex()==2) addNfilesYaml(selectedFileList, selectedNumberList, fileName);
}

//*******************************************
//+++  Uni:: Add Several Files [function]
//*******************************************
void dan18::addSeveralFilesUniSingleFrame(QStringList selectedNumberList, QString fileNumber)
{
    QString wildCardLocal=lineEditWildCard->text();
    QStringList selectedFileList;

    for (int i = 0; i < selectedNumberList.count(); i++)
    {
        selectedFileList << filesManager->fileNameFull(selectedNumberList[i], wildCardLocal);
    }

    QString newFileName;

    if (wildCardLocal.count("#") == 1)
    {
        newFileName = filesManager->fileNameFull(selectedNumberList[0], wildCardLocal);
        newFileName = newFileName.replace(Dir, lineEditPathRAD->text());
        newFileName = newFileName.replace(selectedNumberList[0], fileNumber);
    }
    else
    {
        newFileName = filesManager->newFileNameFull(fileNumber, wildCardLocal);
    }
    addSeveralFilesUniSingleFrame(selectedFileList, selectedNumberList, newFileName);
}
//*******************************************
//+++  Uni:: readTableToAddCols [slot]
//*******************************************
void dan18::readTableToAddCols()
{
    ImportantConstants();
    
    QString wildCardLocal=lineEditWildCard->text();
    
    int mm,nn;
    if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "Table") return;
    
    Table* t = (Table*)app()->activeWindow();
    
    int N=t->numCols();
    int M=t->numRows();
    
    QString fileNumber, file2add;
    QStringList selectedNumberList;
    QStringList selectedFiles;
    
    //+++ Set Data-Sets List +++
    for(nn=0; nn<N;nn++)
    {
        selectedNumberList.clear();
        selectedFiles.clear();
        
        fileNumber=t->text(0,nn);
        fileNumber=fileNumber.simplified();
        fileNumber=fileNumber.replace(" ", "-").replace("/", "-").replace(",", "-").replace(".", "-").remove("%");
        
        
        if (fileNumber == "")
            continue;

        for (mm = 1; mm < M; mm++)
        {
            file2add = t->text(mm, nn);
            file2add = file2add.simplified();
            if (file2add == "")
                continue;
            if (!filesManager->checkFileNumber(file2add))
                continue;
            selectedNumberList << file2add;
            selectedFiles << filesManager->fileNameFull(file2add, wildCard);
        }

        if (toolBoxAdv->currentIndex()==0) addSeveralFilesUniSingleFrame(selectedNumberList,fileNumber);
        else if (toolBoxAdv->currentIndex()==1)
        {
            fileNumber=lineEditPathRAD->text()+"rt0_"+fileNumber+"_added-"+QString::number(selectedNumberList.count())+".DAT";
            tofrtAddFiles(selectedFiles,fileNumber);
        }
        else if (toolBoxAdv->currentIndex()==2)
        {
            fileNumber=lineEditPathRAD->text()+"tof0_"+fileNumber+"_added-"+QString::number(selectedFiles.count())+".DAT";
            tofrtAddFiles(selectedNumberList,fileNumber);
        }
    }
}

//*******************************************
//+++  Uni:: Add Several Files [slot]
//*******************************************
void dan18::generateTableToAdd()
{
    ImportantConstants();
    
    QString dir 	= Dir;
    QString dirOut   = lineEditPathRAD->text();
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"DAN-SANS :: Generate ADDING table template",dir,textEditPattern->text());
    fd->setDirectory(dir);
    fd->setFileMode(QFileDialog::ExistingFiles );
    fd->setWindowTitle(tr("DAN-SANS :: Generate ADDING table template"));
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    QStringList selectedDat=fd->selectedFiles();
    
    int filesNumber= selectedDat.count();
    
    if (!selectedDat[0].contains(dir))
    {
        QMessageBox::warning(this,tr("QtiSAS"), tr("Select data ONLY in INPUT folder!"));
        return;
    }

    // convert name to number
    QStringList numberList;
    QString sss;
    for (int i = 0; i < filesNumber; i++)
    {
        sss=selectedDat[i];
        QString nameFile = FilesManager::findFileNumberInFileName(wildCard, sss.remove(dir));
        numberList << nameFile;
    }
     generateTemplateToAddeFiles(numberList);
}

//*******************************************
//+++  Merging Table Generation [function]
//*******************************************
bool dan18::generateTemplateToAddeFiles(QStringList selectedNumbers )
{
    QString name="adding-template-";
    name=app()->generateUniqueName(name);
    Table *t=app()->newTable(name, 3, selectedNumbers.count());
   
    
    t->setWindowLabel("DAN::Adding::Template");
    app()->setListViewLabel(t->name(), "DAN::Adding::Template");
    app()->updateWindowLists(t);
    
    QStringList selectedNumbersNames;
    int  *skip=new int[selectedNumbers.count()];
    
    for (int i=0; i<selectedNumbers.count();i++ )
    {
        selectedNumbersNames<<configurationPlusSampleName(selectedNumbers[i]);
        skip[i]=0;
    }
    
    QString currentNumberName;
    QStringList currentSelectedNumbers;
    
    int maxNumberRaws=3;
    int maxNumberCols=0;
    QStringList colType;
    
    
    
    for (int i=0; i<selectedNumbers.count(); i++ )
    {
        currentSelectedNumbers.clear();
        
        currentNumberName=selectedNumbersNames[i];
        
        if (currentNumberName=="") continue;
        
        if (skip[i]==1) continue;
        
        for (int ii=i; ii<selectedNumbers.count(); ii++ )
        {
            if (skip[ii]==1) continue;
            
            if (selectedNumbersNames[ii]==currentNumberName)
            {
                currentSelectedNumbers<<selectedNumbers[ii];
                skip[ii]=1;
            }
            
        }
        
        if ( currentSelectedNumbers.count()<1 ) continue;
        
        if (maxNumberRaws<currentSelectedNumbers.count()+1) maxNumberRaws=currentSelectedNumbers.count()+1 ;
        t->setNumRows(maxNumberRaws);
        
        maxNumberCols++;
        
        QString currentLabel;
        if (currentSelectedNumbers.count()>999) currentLabel="0";
        else if (currentSelectedNumbers.count()>99) currentLabel="00";
        else if (currentSelectedNumbers.count()>9) currentLabel="000";
        else  currentLabel="0000";
        
        currentLabel=currentSelectedNumbers[0]+currentLabel+QString::number(currentSelectedNumbers.count());
        
        t->setText(0, maxNumberCols-1,currentLabel);
        
        for (int ii=0; ii<currentSelectedNumbers.count();ii++) t->setText(ii+1, maxNumberCols-1,currentSelectedNumbers[ii]);
        
        t->setColName(maxNumberCols- 1, currentNumberName );
        t->setColPlotDesignation(maxNumberCols-1,Table::None); colType<<"1";
    }
    
    t->setNumCols(maxNumberCols);  
    t->setColumnTypes(colType);
    
    //+++ adjust cols
    for (int tt=0; tt<t->numCols(); tt++)
    {
        t->table()->resizeColumnToContents(tt);
        t->table()->setColumnWidth(tt, t->table()->columnWidth(tt)+30); 
    }
    
    return true;
}

QString dan18::configurationPlusSampleName(QString Number)
{
    QStringList lst;
    int  C;
    double M2, M3;
    double D, lambda, cps,thickness;
    QString beamSize;
    //+++
    int iter;
    
    
    int index=-1;
    if (Number.contains("["))
    {
        index=Number.right(Number.length()-Number.indexOf("[")).remove("[").remove("]").toInt();
    }
    
    readHeaderNumberFull( Number, lst );
    
    D = detector->readDinM(Number, lst);
    C = int(collimation->readCinM(Number, lst));
    lambda = selector->readLambda(Number, readDuration(Number), lst);
    thickness = sample->readThickness(Number, lst);
    beamSize = collimation->readCA(Number, lst) + "|" + collimation->readSA(Number, lst);
    
    // check CD conditions
    int Ncond, iC;
    int iMax=tableEC->columnCount();
    
    Ncond=-1;
    
    for(iC=iMax-1; iC>=0;iC--)
    {
        bool condLambda=lambda>(0.95*tableEC->item(dptWL, iC)->text().toDouble() ) &&
        lambda<(1.05*tableEC->item(dptWL,iC)->text().toDouble() );
        
        bool condD=D>(0.95*tableEC->item(dptD,iC)->text().toDouble()) &&
        D<(1.05*tableEC->item(dptD,iC)->text().toDouble());
        
        bool condC=(C==tableEC->item(dptC,iC)->text().toInt());
        
        bool condBeamSize = (beamSize==tableEC->item(dptBSIZE,iC)->text());
        
        bool condSample=true;
        
        bool attenuatorCompare=true;
        
        bool beamPosCompare=true;
        
        bool polarizationCompare=true;
        
        bool detAngleCompare=true;
        
        QString NumberEC=tableEC->item(dptEC,iC)->text();
        
        if (NumberEC!="")
        {
            condSample = sample->compareSamplePositions(Number, NumberEC);
            attenuatorCompare = collimation->compareAttenuators(Number, NumberEC);
            beamPosCompare = detector->compareBeamPosition(Number, NumberEC);
            polarizationCompare = collimation->comparePolarization(Number, NumberEC);
            detAngleCompare = detector->compareDetRotationPosition(Number, NumberEC);
        }
        
        if (condC && condD && condLambda && condSample && condBeamSize && attenuatorCompare && beamPosCompare && polarizationCompare && detAngleCompare)
        {
            Ncond=iC;
            break;
        }
    }
    
    QString colName = "C" + QString::number(Ncond + 1) + "-" + sample->readName(Number, lst);
    colName=colName.simplified();
    colName=colName.replace(" ", "-").replace("/", "-").replace("_", "-").replace(",", "-").replace(".", "-").remove("%").remove("(").remove(")");
    
    
    if (Ncond>=0 || (iMax == 1 && tableEC->item(0,0)->text()=="")) return colName;
    
    return "";
}
