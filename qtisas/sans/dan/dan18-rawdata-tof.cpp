/***************************************************************************
 File : dan18-rawdata-tof.cpp
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
//+++  RT tools:: Sum [slot]
//*******************************************
void dan18::tofConnectSlots()
{
    //+++ tof mode
    connect( pushButtonTofAddFiles,   SIGNAL( clicked() ), this, SLOT( tofAddFiles() ) );
    connect( pushButtonTofSumRead,   SIGNAL( clicked() ), this, SLOT( tofSumRead() ) );
    connect( pushButtonTofShift,   SIGNAL( clicked() ), this, SLOT( tofShift() ) );
    connect( pushButtonTofCollapse,SIGNAL( clicked() ), this, SLOT( tofCollapse() ) );
    connect( pushButtonTofRemove,  SIGNAL( clicked() ), this, SLOT( tofRemove() ) );
    connect( pushButtonTofMerge,   SIGNAL( clicked() ), this, SLOT( tofMerge() ) );
    connect( pushButtonTofSplit,   SIGNAL( clicked() ), this, SLOT( tofSplit() ) );
    connect( pushButtonTofCheck,   SIGNAL( clicked() ), this, SLOT( tofCheck() ) );
    connect( pushButtonTofAll,     SIGNAL( clicked() ), this, SLOT( tofAll() ) );
}


//*******************************************
//+++  TOF tools:: Add Tof/RT files [slot]
//*******************************************
void dan18::tofAddFiles()
{
    //+++
    ImportantConstants();
    //+++
    QString DirIn               = Dir;
    QString DirOut=lineEditPathRAD->text();
    //+++
    QString filter=textEditPattern->text();
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDir(DirIn);
    fd->setMode(QFileDialog::ExistingFiles);
    fd->setCaption(tr("DAN - Add TOF/RT-files /frame-by-frame/ "));
    fd->setFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->name()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    //+++
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    
    QStringList lst;
    QRegExp rxF( "(\\d+)" );
    
    //+++
    QStringList numberList;
    
    //+++ convert name to number
    for (int i=0; i<filesNumber; i++)
    {
        QString s=selectedDat[i];
        s=findFileNumberInFileName(wildCard, s.remove(Dir));
        numberList<< s;
    }
    
    //+++ Check files: RT or not
    int numberFrames=readSlicesCount(numberList[0]);
    
    for (int i=1; i<filesNumber; i++)
    {
        //:::+ uni-rt
        int slicesCount=readSlicesCount(numberList[i]);
        
        if (numberFrames <=1 || numberFrames!=slicesCount)
        {
            QMessageBox::critical( 0, "qtiSAS", "File: "+numberList[i]   + " has number slices (" + QString::number(slicesCount)
                                  +") different from other (" + QString::number(numberFrames) +").");
            return;
        }
        //:::+- uni-rt
    }
    
    if ( QMessageBox::question ( this, "TOF|RT:: adding of files" , "You selected "+QString::number(filesNumber) + " files , every file contains  " +QString::number(numberFrames ) + " frames. Continue?",  tr ( "&Yes" ), tr ( "&No" ), QString::null, 0, 1 ) )
        return;
    
    
    QString finalNameIndex=numberList[0];
    
    if (filesNumber>=99) finalNameIndex=finalNameIndex+"0"+QString::number(filesNumber);
    else if (filesNumber>=9) finalNameIndex=finalNameIndex+"00"+QString::number(filesNumber);
    else finalNameIndex=finalNameIndex+"000"+QString::number(filesNumber);
    
    QString sss=DirOut;
    if (toolBoxAdv->currentIndex()==1) sss+="rt0_"; else sss+="tof0_";
    sss+=finalNameIndex;
    sss+="_added-"+QString::number(filesNumber)+".DAT";
    
    
    QString file=sss;
    
    bool ok;
    file = QInputDialog::getText("qtiSAS", "Enter name of Mergrd File [change suffix  and number]:", QLineEdit::Normal, file, &ok, this );
    if ( !ok ) return;
    
    tofrtAddFiles(selectedDat, file );
}

    
void dan18::tofrtAddFiles(QStringList selectedDat, QString file )
{
    int filesNumber= selectedDat.count();
    
    //+++
    QStringList numberList;
    
    //+++ convert name to number
    for (int i=0; i<filesNumber; i++)
    {
        QString s=selectedDat[i];
        s=findFileNumberInFileName(wildCard, s.remove(Dir));
        numberList<< s;
    }
    
    int numberFrames=readSlicesCount(numberList[0]);
        
    QFile f(file);
    
    //+++
    if ( !f.open( IO_WriteOnly ) )  return;
    
    QTextStream stream( &f );

    
    //+++ time of additioon
    QTime t; t.start();
    
    //+++ header
    QStringList header;
    addNheadersUni(selectedDat, numberList, header) ;
    std::cout<<selectedDat[0].latin1()<<"  "<<numberList[0].latin1()<<"  "<<header[0].latin1()<<"\n";std::cout.flush();

    //+++ add numbber repetitions
    for (int i=0; i<header.count()-1; i++)  stream << header[i]+"\n";

    stream << "\n(*  RT|TOF adding of files  *)\n";
    stream << "RT-Number-Repetitions="<<QString::number(filesNumber)<<"\n\n";
    stream << header[header.count()-1]+"\n";
    
    
    //+++ adding matrixes
    std::cout<<"\n["<<file.latin1()<<"]: adding\n";
    addNmatrixes2016(selectedDat, numberList, stream, header.count(), numberFrames);
    
    //+++ close file
    f.close();

    std::cout<<"\nadding ["<<filesNumber<<"] :: "<<t.elapsed()/1000<<"sec\n";
}

//*******************************************
//+++  TOF:: Add Matrixes [function]
//*******************************************
bool dan18::addNmatrixesTof(QStringList files, QStringList fileNumers, QStringList &matrixesText, int mainHeaderLength, int framesCount)
{
    matrixesText.clear();
    
    int N=files.count();
    if (N<1) return false;
    
    int MDinFile=comboBoxMDdata->currentText().toInt();
    int numbersPerRow=spinBoxReadMatrixTofNumberPerLine->value();
    
    int localHeaderLength = spinBoxDataHeaderNumberLines->value();
    int spaceBetweenFrames = spinBoxDataLinesBetweenFrames->value();
    
    gsl_matrix *data=gsl_matrix_calloc(MDinFile*framesCount,MDinFile);
    
    int currentHeaderLength;
    
    //+++++++++++++++++
    // +++ File ++++++
    //+++++++++++++++++
    QFile fileInput( files[0] );
    //+++
    if ( !fileInput.open(IO_ReadOnly ) )  return false;
    QTextStream streamInput( &fileInput );
    for (int ii=0; ii<mainHeaderLength; ii++) streamInput.readLine();
    
    QStringList headersLines, lst;
    
    for(int ff=0;ff<framesCount;ff++)
    {
        for (int ii=0; ii<localHeaderLength; ii++) headersLines << streamInput.readLine();
        for (int ii=0; ii<MDinFile; ii++) streamInput.readLine();
        for (int ii=0; ii<spaceBetweenFrames; ii++) streamInput.readLine();
    }
    fileInput.close();
    
    int value;
    QString s;
    
    for(int f=0;f<N;f++)
    {
        // open file to read
        QFile file( files[f] );
        //+++
        if ( !file.open(IO_ReadOnly ) )  return false;
        QTextStream t( &file );
        
        // read main header
        for (int ii=0; ii<mainHeaderLength; ii++) t.readLine();
        
        // read frames
        for(int ff=0;ff<framesCount;ff++)
        {
            for (int ii=0; ii<localHeaderLength; ii++) t.readLine();
            
            for (int i=0;i<MDinFile;i++)
            {
                lst.clear();
                s=t.readLine();
                s=s.replace(",", " "); // new :: bersans
                s=s.simplifyWhiteSpace();
                lst=lst.split(" ", s );
                
                if (lst.count()!=numbersPerRow) toResLog("File :: "+files[f]+" has Error at "+ QString::number(i)+" line. Numbers :: "+QString::number(lst.count())+"\n");
                
                for (int j=0; j<numbersPerRow;j++)
                {
                    value=gsl_matrix_get(data,MDinFile*ff+i,j)+lst[j].toDouble();
                    gsl_matrix_set(data,MDinFile*ff+i,j, value);
                    
                }
            }
            
            for (int ii=0; ii<spaceBetweenFrames; ii++) t.readLine();
        }
        file.close();
    }
    
    
    for(int ff=0;ff<framesCount;ff++)
    {
        for (int ii=0; ii<localHeaderLength; ii++) matrixesText<<headersLines[ff*localHeaderLength+ii];
        
        for (int i=0;i<MDinFile;i++)
        {
            s=" ";
            for (int j=0; j<MDinFile;j++)
            {
                s+=QString::number(gsl_matrix_get(data,MDinFile*ff+i,j),'f',0);
                s+="   ";
            }
            matrixesText<<s;
        }
        for (int ii=0; ii<spaceBetweenFrames; ii++) matrixesText<<"";
        
        for (int ii=0; ii<MDinFile; ii++) streamInput.readLine();
    }
    
    gsl_matrix_free(data);
    
    return true;
}

//*******************************************
//+++  TOF tools:: Shift [slot]
//*******************************************
void dan18::tofSumRead()
{
    
    ImportantConstants();
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: TOF, RT, ...");
    }
    
    bool ok;
    
    QRegExp rxF( "(\\d+)" );
    
    QString dir = Dir;
    
    QString DirOut=lineEditPathRAD->text();
    QString filter=textEditPattern->text();
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDir(Dir);
    fd->setMode(QFileDialog::ExistingFiles);
    fd->setCaption(tr("DAN - TOF :: Read Sums"));
    fd->setFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->name()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    QStringList numberList;
    // convert name to number
    for (int i=0; i<filesNumber; i++)
    {
        QString s=selectedDat[i];
        s=findFileNumberInFileName(wildCard, s.remove(dir));
        numberList<< s;
    }
    
    QStringList header, lst;
    
    // list of Modes
    for (int i=0; i<filesNumber; i++)
    {
        //:::+ uni-tof
        int slicesCount=readSlicesCount(numberList[i]);
        if (slicesCount>1) lst<< "RT or TOF Mode :: " + QString::number(slicesCount) + " frames";
        else lst<< "Normal Mode :: 1 frame";
        //:::+- uni-tof
    }
    
    // select RT-mode
    QString res = QInputDialog::getItem(
                                        "qtiSAS :: DAN", "Select Reference for TOF | RT - mode:", lst, 0, TRUE, &ok,
                                        this );
    if ( !ok ) return;
    
    // +++ number of frames
    rxF.search( res, 0);
    
    int numberFrames = rxF.cap(1).toInt();
    
    
    if (!res.contains("TOF") || numberFrames<=1) {
        QMessageBox::critical( 0, "qtiSAS", "... You selected non-TOF file ...");
        return;
    }
    
    int filesNumberFinal=0;
    QStringList selectedDatFinal;
    
    for (int i=0; i<filesNumber; i++)
    {
        if (lst[i]==res)
        {
            selectedDatFinal<<selectedDat[i];
            filesNumberFinal++;
        }
    }
    
    QString tableName="sum-tof";
    if (selectedDatFinal[0].contains("tof1_")) tableName+="-shift-";
    else if (selectedDatFinal[0].contains("tof2_")) tableName+="-collapse-";
    else if (selectedDatFinal[0].contains("tof3_")) tableName+="-remove-";
    else if (selectedDatFinal[0].contains("tof4_")) tableName+="-merge-";
    else tableName+="-";
    
    
    return tofSumRead(numberFrames,selectedDatFinal, tableName);
    
}

//*******************************************
//+++  TOF tools:: SumCulculate [function]
//*******************************************
int dan18::tofSumCulculate(QStringList lst, int matrixInFileLength, int numberTofPerLine)
{
    
    double sum=0;
    QStringList lstSum;
    QString s;
    
    
    for(int l=0;l<matrixInFileLength;l++)
    {
        s=lst[l].remove("\n").replace("\t"," ").simplifyWhiteSpace();
        lstSum.clear();
        lstSum=lstSum.split(" ",s);
        for(int i=0;i<lstSum.count();i++) sum+=lstSum[i].toDouble();
    }
    return int(sum);
}

//*******************************************
//+++  TOF tools:: tofSumRead [function]
//*******************************************
void dan18::tofSumRead(int numberFrames, QStringList inputFiles, QString tableName)
{
    
    
    //+++ Progress Dialog +++
    QProgressDialog *progress= new QProgressDialog("TOF | RT :: SUM","Stop", 0,inputFiles.count());
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setLabelText("File # "+QString::number(0)+" of "+QString::number(inputFiles.count()));
    
    int numberTofPerLineNormal=spinBoxReadMatrixNumberPerLine->value();
    int mainHeaderLength=spinBoxHeaderNumberLines->value();
    int tofHeaderBeforeLength=spinBoxDataHeaderNumberLines->value();
    int tofHeaderAfterLength=spinBoxDataLinesBetweenFrames->value();
    int numberTofPerLine=spinBoxReadMatrixTofNumberPerLine->value();
    int DIM=comboBoxMDdata->currentText().toInt();
    int matrixInFileLength=DIM;
    matrixInFileLength*=matrixInFileLength;
    matrixInFileLength/=numberTofPerLine;
    
    int frameLength=tofHeaderBeforeLength+tofHeaderAfterLength+matrixInFileLength;
    
    Table *tableDat=app()->newTable(app()->generateUniqueName(tableName), numberFrames,inputFiles.count()+2);
    tableDat->setColName(1,"Number");
    tableDat->setColName(0,"NormalizedNumber");
    
    
    QString sTOForRT="Real Time";
    if (tableName.contains("TOF") || tableName.contains("tof")) sTOForRT="TOF";
    
    QString  s=inputFiles[0];
    sTOForRT = sTOForRT + " :: Detector Sum(s) :: " +findFileNumberInFileName(wildCard, s.remove(Dir));
    for (int i=1; i<inputFiles.count(); i++)
    {
        s=inputFiles[i];
        sTOForRT = sTOForRT + ", " + findFileNumberInFileName(wildCard, s.remove(Dir));
    }
    
    tableDat->setWindowLabel(sTOForRT);
    app()->setListViewLabel(tableDat->name(), sTOForRT);
    app()->updateWindowLists(tableDat);
    
    for (int i=0; i<inputFiles.count(); i++)
    {
        s=inputFiles[i];
        s=findFileNumberInFileName(wildCard, s.remove(Dir));
        
        tableDat->setColName(i+2,"Sum-"+s);
        
        tableDat->setColComment(i+2, "File: " + s);
    }
    
    for (int i=0;i<numberFrames;i++)
    {
        double normMumber=i+1;
        
        if (inputFiles[0].contains("tof1_") || inputFiles[0].contains("tof2_") ) { normMumber+=spinBoxTofShift->value();}
        else if (inputFiles[0].contains("tof3_")) {normMumber+=spinBoxTofShift->value(); normMumber+=spinBoxRemove->value();}
        else if (inputFiles[0].contains("tof4_")) {normMumber=1+spinBoxTofShift->value()+spinBoxRemove->value();normMumber+=(i+0.5)*spinBoxMerge->value();}
        
        tableDat->setText(i,0,QString::number(normMumber));
        tableDat->setText(i,1,QString::number(i+1));
        
        
    }
    
    
    QStringList sMiddle;
    QString number;
    //
    for (int i=0; i<inputFiles.count(); i++)
    {
        //+++ Progress +++
        progress->setValue(i);
        progress->setLabelText("File # "+QString::number(i+1)+" of "+QString::number(inputFiles.count()));
        if ( progress->wasCanceled() ) break;
        
        
        number=inputFiles[i];
        number=number.remove(Dir);
        number=findFileNumberInFileName(wildCard, number);
        
        mainHeaderLength=lengthMainHeader(inputFiles[i]);
        if (comboBoxHeaderFormat->currentText().contains("YAML")) mainHeaderLength=0;
        
        //+++++++++++++++++
        // +++ Files ++++++
        //+++++++++++++++++
        QFile fileInput( inputFiles[i] );
        //+++
        if ( !fileInput.open(IO_ReadOnly ) )  break;
        QTextStream streamInput( &fileInput );
        
        QString s;
        
        for (int ii=0; ii<mainHeaderLength; ii++) streamInput.readLine();
        
        for (int j=0;j<numberFrames;j++)
        {
            // read header
            for (int ii=0; ii<tofHeaderBeforeLength; ii++) streamInput.readLine();
            
            sMiddle.clear();
            // read matrix
            for (int ii=0; ii<matrixInFileLength; ii++) sMiddle<<streamInput.readLine();
            
            tableDat->setText(j,i+2,QString::number(tofSumCulculate(sMiddle, matrixInFileLength, numberTofPerLine)));
            
            // read post header
            for (int ii=0; ii<tofHeaderAfterLength; ii++) streamInput.readLine();
        }
        
        fileInput.close();
    }
    
}



//*******************************************
//+++  TOF tools:: Shift [slot]
//*******************************************
void dan18::tofShift()
{
    
    int shift=spinBoxTofShift->value();
    
    
    ImportantConstants();
    
    bool ok;
    
    QRegExp rxF( "(\\d+)" );
    
    QString dir              = Dir;
    QString DirOut=lineEditPathRAD->text();
    
    QString filter=textEditPattern->text();
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDir(Dir);
    fd->setMode(QFileDialog::ExistingFiles);
    fd->setCaption(tr("DAN - Shifting of the start of TOF files"));
    fd->setFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->name()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    QStringList numberList;
    // convert name to number
    for (int i=0; i<filesNumber; i++)
    {
        QString s=selectedDat[i];
        s=findFileNumberInFileName(wildCard, s.remove(dir));
        numberList<< s;
    }
    
    QStringList header, lst;
    
    // list of Modes
    for (int i=0; i<filesNumber; i++)
    {
        //:::+ uni-tof
        int slicesCount=readSlicesCount(numberList[i]);
        if (slicesCount>1) lst<< "RT or TOF Mode :: " + QString::number(slicesCount) + " frames";
        else lst<< "Normal Mode :: 1 frame";
        //:::+- uni-tof
        
    }
    
    // select RT-mode
    QString res = QInputDialog::getItem(
                                        "qtiSAS :: DAN", "Select Reference for TOF-mode:", lst, 0, TRUE, &ok,
                                        this );
    if ( !ok ) return;
    
    // +++ number of frames
    rxF.search( res, 0);
    
    int numberFrames = rxF.cap(1).toInt();
    
    if (shift>=numberFrames) {
        QMessageBox::critical( 0, "qtiSAS", "... Shift is large than number of frames ...");
        return;
    }
    
    if (!res.contains("TOF")) {
        QMessageBox::critical( 0, "qtiSAS", "... You selected non-TOF file ...");
        return;
    }
    
    if (numberFrames<=1) return;
    
    
    
    int filesNumberFinal=0;
    QStringList selectedDatFinal, newFilesNames;
    
    for (int i=0; i<filesNumber; i++)
    {
        if (lst[i]==res)
        {
            selectedDatFinal<<selectedDat[i];
            newFilesNames<<DirOut+"tof1_"+numberList[i]+"1_Shift.DAT";
            filesNumberFinal++;
        }
    }
    
    
    
    return tofShift(shift, numberFrames,selectedDatFinal, newFilesNames);
}
//*******************************************
//+++  TOF tools:: Shift [function]
//*******************************************
void dan18::tofShift(int shift, int numberFrames, QStringList inputFiles, QStringList outputFiles)
{
    
    if (shift<1) return;
    if (shift>=numberFrames) return;
    
    //+++ Progress Dialog +++
    QProgressDialog *progress= new QProgressDialog("TOF Shift","Stop", 0,inputFiles.count());
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setLabelText("File # "+QString::number(0)+" of "+QString::number(inputFiles.count()));
    
    int mainHeaderLength=spinBoxHeaderNumberLines->value();
    int tofHeaderBeforeLength=spinBoxDataHeaderNumberLines->value();
    int tofHeaderAfterLength=spinBoxDataLinesBetweenFrames->value();
    int numberTofPerLine=spinBoxReadMatrixTofNumberPerLine->value();
    int matrixInFileLength=comboBoxMDdata->currentText().toInt();
    matrixInFileLength*=matrixInFileLength;
    matrixInFileLength/=numberTofPerLine;
    
    int frameLength=tofHeaderBeforeLength+tofHeaderAfterLength+matrixInFileLength;
    
    QString sFinal, sMiddle;
    QString number;
    //
    for (int i=0; i<inputFiles.count(); i++)
    {
        number=inputFiles[i];
        number=number.remove(Dir);
        number=findFileNumberInFileName(wildCard, number);
        
        mainHeaderLength=lengthMainHeader(inputFiles[i]);
        
        //+++ Progress +++
        
        progress->setValue(i+1);
        progress->setLabelText("File # "+QString::number(i+1)+" of "+QString::number(outputFiles.count()));
        if ( progress->wasCanceled() ) break;
        
        sFinal="";
        sMiddle="";
        
        
        //+++++++++++++++++
        // +++ Files ++++++
        //+++++++++++++++++
        QFile fileInput( inputFiles[i] );
        //+++
        if ( !fileInput.open(IO_ReadOnly ) )  break;
        QTextStream streamInput( &fileInput );
        
        
        //+++++++++++++++++
        // +++ Header +++++
        //+++++++++++++++++
        for (int l=0; l<mainHeaderLength; l++) sFinal+=streamInput.readLine()+"\n";
        for (int l=0; l<frameLength*shift; l++) sMiddle+=streamInput.readLine()+"\n";
        for (int l=0; l<frameLength*(numberFrames-shift); l++) sFinal+=streamInput.readLine()+"\n";
        sFinal+=sMiddle;
        
        QFile fileOutput(outputFiles[i]);
        
        //+++
        if ( !fileOutput.open( IO_WriteOnly ) )  break;
        QTextStream streamOutput( &fileOutput );
        streamOutput<<sFinal;
        
        
        
        fileInput.close();
        fileOutput.close();
    }
}

//*******************************************
//+++  TOF tools:: Collapse [slot]
//*******************************************
void dan18::tofCollapse()
{
    
    int collapse=spinBoxCollapse->value();
    
    
    ImportantConstants();
    
    bool ok;
    
    QRegExp rxF( "(\\d+)" );
    
    QString dir              = Dir;
    QString DirOut=lineEditPathRAD->text();
    
    QString filter=textEditPattern->text();
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDir(Dir);
    fd->setMode(QFileDialog::ExistingFiles);
    fd->setCaption(tr("DAN - Collapse of TOF files to Single-Phase-Case"));
    fd->setFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->name()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    QStringList numberList;
    // convert name to number
    for (int i=0; i<filesNumber; i++)
    {
        QString s=selectedDat[i];
        s=findFileNumberInFileName(wildCard, s.remove(dir));
        numberList<< s;
    }
    
    QStringList header, lst;
    
    // list of Modes
    for (int i=0; i<filesNumber; i++)
    {
        //:::+ uni-tof
        int slicesCount=readSlicesCount(numberList[i]);
        if (slicesCount>1) lst<< "RT or TOF Mode :: " + QString::number(slicesCount) + " frames";
        else lst<< "Normal Mode :: 1 frame";
        //:::+- uni-tof
    }
    
    // select RT-mode
    QString res = QInputDialog::getItem(
                                        "qtiSAS :: DAN", "Select Reference for TOF-mode:", lst, 0, TRUE, &ok,
                                        this );
    if ( !ok ) return;
    
    // +++ number of frames
    rxF.search( res, 0);
    int numberFrames = rxF.cap(1).toInt();
    
    
    if (collapse>numberFrames) {
        QMessageBox::critical( 0, "qtiSAS", "... collapse > numberFrames ...");
        return;
    }
    
    if (!res.contains("TOF")) {
        QMessageBox::critical( 0, "qtiSAS", "... You selected non-TOF file ...");
        return;
    }
    
    
    int filesNumberFinal=0;
    QStringList selectedDatFinal, newFilesNames;
    
    for (int i=0; i<filesNumber; i++)
    {
        if (lst[i]==res)
        {
            selectedDatFinal<<selectedDat[i];
            newFilesNames<<DirOut+"tof2_"+numberList[i]+"2_Collapse.DAT";
            filesNumberFinal++;
        }
    }
    
    
    return tofCollapse(collapse, numberFrames,selectedDatFinal, newFilesNames);
    
}

//*******************************************
//+++  TOF tools:: collapse2Phases [function]
//*******************************************
void collapse2Phases(QStringList &sFrames,QStringList sFramesNext, int numberFramesFinal, int frameLength, int numberTofPerLine){
    
    QStringList lst;
    
    int pos1, pos2;
    QString s1, s2, ss1, ss2, s, ss, sss;
    
    QRegExp rxF( "(\\d+)" );
    
    
    for (int i=0; i<numberFramesFinal; i++ ){
        lst<<sFrames[i*frameLength];
        
        for(int ii=0;ii<numberTofPerLine;ii++)
        {
            pos1=0;
            pos2=0;
            s1= sFrames[i*frameLength+ii+1];
            s2= sFramesNext[i*frameLength+ii+1];
            sss="";
            for(int jj=0;jj<numberTofPerLine;jj++)
            {
                pos1 = rxF.search( s1, pos1 ); pos1+=rxF.matchedLength();
                ss1=rxF.cap(1);
                
                pos2 = rxF.search( s2, pos2 ); pos2+=rxF.matchedLength();
                ss2=rxF.cap(1);
                
                s= " "+ QString::number ( ss1.toInt() + ss2.toInt());
                ss.fill(' ', 9-s.length());
                s.prepend(ss);
                sss+=s;
                
            }
            sss.remove(0,1);
            lst <<  sss;
        }
        lst<<"";
    }
    sFrames.clear();
    sFrames=lst;
    
    return;
}

//*******************************************
//+++  TOF tools:: Collapse [function]
//*******************************************
void dan18::tofCollapse(int collapse, int numberFrames, QStringList inputFiles, QStringList outputFiles)
{
    
    if (collapse==1)
    {
        //
        for (int i=0; i<inputFiles.count(); i++){
            
            std::ifstream ifs(inputFiles[i].ascii());
            std::ofstream ofs(outputFiles[i].ascii());
            
            ofs << ifs.rdbuf();
        }
        return;
    }
    
    int numberFramesFinal=int(numberFrames/collapse);
    if (numberFramesFinal*collapse < numberFrames ) return;
    
    
    //+++ Progress Dialog +++
    QProgressDialog *progress= new QProgressDialog("TOF Collapse","Stop", 0,inputFiles.count());
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setLabelText("File # "+QString::number(0)+" of "+QString::number(inputFiles.count()));
    
    int mainHeaderLength=spinBoxHeaderNumberLines->value();
    int tofHeaderBeforeLength=spinBoxDataHeaderNumberLines->value();
    int tofHeaderAfterLength=spinBoxDataLinesBetweenFrames->value();
    int numberTofPerLine=spinBoxReadMatrixTofNumberPerLine->value();
    int matrixInFileLength=comboBoxMDdata->currentText().toInt();
    matrixInFileLength*=matrixInFileLength;
    matrixInFileLength/=numberTofPerLine;
    
    int frameLength=tofHeaderBeforeLength+tofHeaderAfterLength+matrixInFileLength;
    
    QStringList sFinal, sFrames, sFramesNext;
    QString number;
    //
    for (int i=0; i<inputFiles.count(); i++)
    {
        number=inputFiles[i];
        number=number.remove(Dir);
        number=findFileNumberInFileName(wildCard, number);
        
        mainHeaderLength=lengthMainHeader(inputFiles[i]);
        
        
        //+++ Progress +++
        progress->setValue(i+1);
        progress->setLabelText("File # "+QString::number(i+1)+" of "+QString::number(inputFiles.count()));
        if ( progress->wasCanceled() ) break;
        
        
        sFinal.clear();
        sFrames.clear();
        
        
        //+++++++++++++++++
        // +++ Files ++++++
        //+++++++++++++++++
        QFile fileInput( inputFiles[i] );
        //+++
        if ( !fileInput.open(IO_ReadOnly ) )  break;
        QTextStream streamInput( &fileInput );
        
        
        //+++++++++++++++++
        // +++ Header +++++
        //+++++++++++++++++
        for (int l=0; l<mainHeaderLength; l++) sFinal<<streamInput.readLine();
        
        int indexInHeader=listOfHeaders.findIndex("[Slices-Count]");
        QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
        QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
        
        QString oldCount =readNumberString(number, pos, num);
        
        sFinal[pos.toInt()-1]=sFinal[pos.toInt()-1].replace(num.toInt()-1,oldCount.length(), QString::number(numberFramesFinal));
        
        for (int l=0; l<frameLength*numberFramesFinal; l++) sFrames<<streamInput.readLine();
        
        for (int j=0; j<(collapse-1); j++) {
            sFramesNext.clear();
            for (int l=0; l<frameLength*numberFramesFinal; l++) sFramesNext<<streamInput.readLine();
            collapse2Phases(sFrames,sFramesNext, numberFramesFinal, frameLength, numberTofPerLine);
        }
        
        sFinal+=sFrames;
        
        QFile fileOutput(outputFiles[i]);
        
        //+++
        if ( !fileOutput.open( IO_WriteOnly ) )  break;
        QTextStream streamOutput( &fileOutput );
        
        for (int i=0; i<(mainHeaderLength+frameLength*numberFramesFinal); i++) streamOutput<<sFinal[i]+"\n";
        
        
        
        fileInput.close();
        fileOutput.close();
    }
}

//*******************************************
//+++  TOF tools:: Remove [slot]
//*******************************************
void dan18::tofRemove()
{
    
    int remove=spinBoxRemove->value();
    
    ImportantConstants();
    
    bool ok;
    
    QRegExp rxF( "(\\d+)" );
    
    QString dir              = Dir;
    QString DirOut=lineEditPathRAD->text();
    
    QString filter=textEditPattern->text();
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDir(Dir);
    fd->setMode(QFileDialog::ExistingFiles);
    fd->setCaption(tr("DAN - Remove First and Last Frame(s)"));
    fd->setFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->name()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    QStringList numberList;
    // convert name to number
    for (int i=0; i<filesNumber; i++)
    {
        QString s=selectedDat[i];
        s=findFileNumberInFileName(wildCard, s.remove(dir));
        numberList<< s;
    }
    
    QStringList header, lst;
    
    // list of Modes
    for (int i=0; i<filesNumber; i++)
    {
        //:::+ uni-tof
        int slicesCount=readSlicesCount(numberList[i]);
        if (slicesCount>1) lst<< "RT or TOF Mode :: " + QString::number(slicesCount) + " frames";
        else lst<< "Normal Mode :: 1 frame";
        //:::+- uni-tof
    }
    
    // select RT-mode
    QString res = QInputDialog::getItem(
                                        "qtiSAS :: DAN", "Select Reference for RT-mode:", lst, 0, TRUE, &ok,
                                        this );
    if ( !ok ) return;
    
    // +++ number of frames
    rxF.search( res, 0);
    int numberFrames = rxF.cap(1).toInt();
    
    if (remove>(int(numberFrames/2))) return;
    
    if (!res.contains("TOF")) return;
    
    if (numberFrames<=1) return;
    
    if (remove>(int(numberFrames/2))) {
        QMessageBox::critical( 0, "qtiSAS", "... remove >numberFrames/2 ...");
        return;
    }
    
    if (!res.contains("TOF")) {
        QMessageBox::critical( 0, "qtiSAS", "... You selected non-TOF file ...");
        return;
    }
    
    int filesNumberFinal=0;
    QStringList selectedDatFinal, newFilesNames;
    
    
    
    
    for (int i=0; i<filesNumber; i++)
    {
        if (lst[i]==res)
        {
            selectedDatFinal<<selectedDat[i];
            newFilesNames<<DirOut+"tof3_"+numberList[i]+"3_Remove.DAT";
            filesNumberFinal++;
        }
    }
    
    return tofRemove(remove, numberFrames,selectedDatFinal, newFilesNames);
}

//*******************************************
//+++  TOF tools:: Remove [function]
//*******************************************
void dan18::tofRemove(int remove, int numberFrames, QStringList inputFiles, QStringList outputFiles)
{
    
    if (remove<0) return;
    if (remove>(int(numberFrames/2))) return;
    if (numberFrames<=1) return;
    
    
    //+++ Progress Dialog +++
    QProgressDialog *progress= new QProgressDialog("TOF Shift","Stop", 0,inputFiles.count());
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setLabelText("File # "+QString::number(0)+" of "+QString::number(inputFiles.count()));
    
    int mainHeaderLength=spinBoxHeaderNumberLines->value();
    int tofHeaderBeforeLength=spinBoxDataHeaderNumberLines->value();
    int tofHeaderAfterLength=spinBoxDataLinesBetweenFrames->value();
    int numberTofPerLine=spinBoxReadMatrixTofNumberPerLine->value();
    int matrixInFileLength=comboBoxMDdata->currentText().toInt();
    matrixInFileLength*=matrixInFileLength;
    matrixInFileLength/=numberTofPerLine;
    
    int frameLength=tofHeaderBeforeLength+tofHeaderAfterLength+matrixInFileLength;
    
    QStringList sFinal, sMiddle;
    QString number;
    //
    for (int i=0; i<inputFiles.count(); i++)
    {
        number=inputFiles[i];
        number=number.remove(Dir);
        number=findFileNumberInFileName(wildCard, number);
        
        mainHeaderLength=lengthMainHeader(inputFiles[i]);
        
        //+++ Progress +++
        
        progress->setValue(i+1);
        progress->setLabelText("File # "+QString::number(i+1)+" of "+QString::number(inputFiles.count()));
        if ( progress->wasCanceled() ) break;
        
        sFinal.clear();
        sMiddle.clear();
        
        
        //+++++++++++++++++
        // +++ Files ++++++
        //+++++++++++++++++
        QFile fileInput( inputFiles[i] );
        //+++
        if ( !fileInput.open(IO_ReadOnly ) )  break;
        QTextStream streamInput( &fileInput );
        
        
        //+++++++++++++++++
        // +++ Header +++++
        //+++++++++++++++++
        for (int l=0; l<mainHeaderLength; l++) sFinal<<streamInput.readLine();
        
        int indexInHeader=listOfHeaders.findIndex("[Slices-Count]");
        QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
        QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
        
        QString oldCount =readNumberString(number, pos, num);
        
        sFinal[pos.toInt()-1]=sFinal[pos.toInt()-1].replace(num.toInt()-1,oldCount.length(), QString::number(numberFrames-2*remove));
        
        
        for (int l=0; l<frameLength*remove; l++) sMiddle<<streamInput.readLine();
        for (int l=0; l<frameLength*(numberFrames-2*remove); l++) sFinal<<streamInput.readLine();
        //Final+=sMiddle;
        
        QFile fileOutput(outputFiles[i]);
        
        //+++
        if ( !fileOutput.open( IO_WriteOnly ) )  break;
        QTextStream streamOutput( &fileOutput );
        //streamOutput<<sFinal;
        
        for (int i=0; i<sFinal.count(); i++) streamOutput<<sFinal[i]+"\n";
        
        fileInput.close();
        fileOutput.close();
    }
}


//*******************************************
//+++  TOF tools:: Merge [slot]
//*******************************************
void dan18::tofMerge()
{
    //+++
    int merge=spinBoxMerge->value();
    
    //+++
    ImportantConstants();
    //+++
    QString DirIn               = Dir;
    QString DirOut=lineEditPathRAD->text();
    //+++
    QString filter=textEditPattern->text();
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDir(DirIn);
    fd->setMode(QFileDialog::ExistingFiles);
    fd->setCaption(tr("DAN - Merge TOF Frames"));
    fd->setFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->name()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    //+++
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    //+++
    QStringList numberList;
    
    //+++ convert name to number
    for (int i=0; i<filesNumber; i++)
    {
        QString s=selectedDat[i];
        s=findFileNumberInFileName(wildCard, s.remove(Dir));
        numberList<< s;
    }
    
    QStringList header, lst;
    QRegExp rxF( "(\\d+)" );
    
    //+++ list of Modes
    for (int i=0; i<filesNumber; i++)
    {
        //:::+ uni-tof
        int slicesCount=readSlicesCount(numberList[i]);
        if (slicesCount>1) lst<< "RT or TOF Mode :: " + QString::number(slicesCount) + " frames";
        else lst<< "Normal Mode :: 1 frame";
        //:::+- uni-tof
        
    }
    
    // select TOF-mode
    bool ok;
    QString res = QInputDialog::getItem("qtiSAS :: DAN", "Select Reference for RT-mode:", lst, 0, TRUE, &ok,this );
    if ( !ok ) return;
    
    // +++ number of frames
    rxF.search( res, 0);
    int numberFrames = rxF.cap(1).toInt();
    
    //+++
    if (merge>numberFrames) return;
    if (!res.contains("TOF")) return;
    if (numberFrames<=1) return;
    
    //+++
    int filesNumberFinal=0;
    QStringList selectedDatFinal, newFilesNames;
    
    
    //+++
    for (int i=0; i<filesNumber; i++)
    {
        if (lst[i]==res)
        {
            selectedDatFinal<<selectedDat[i];
            newFilesNames<<DirOut+"tof4_"+numberList[i]+"4_Merge.DAT";
            filesNumberFinal++;
        }
    }
    
    
    return tofMerge(merge, numberFrames, selectedDatFinal, newFilesNames);
}

//*******************************************
//+++  TOF tools:: Merge Frames [function]
//*******************************************
void dan18::tofMergeFrames(int merge, QStringList &lst, int tofHeaderBeforeLength, int matrixInFileLength, int frameLength, int numberTofPerLine)
{
    
    QStringList sLst, sLstFinal;
    
    for (int l=tofHeaderBeforeLength; l<(matrixInFileLength+tofHeaderBeforeLength); l++) sLst<<lst[l];
    
    QString s, ss, s1, s2, ss1, ss2, sss;
    int pos1, pos2;
    QRegExp rxF( "(\\d+)" );
    
    for (int i=1; i<merge; i++){
        
        sLstFinal.clear();
        for (int l=0; l<matrixInFileLength; l++) {
            
            s1=sLst[l];
            s2=lst[i*frameLength+1+l];
            
            pos1=0;
            pos2=0;
            
            sss="";
            for(int jj=0;jj<numberTofPerLine;jj++)
            {
                pos1 = rxF.search( s1, pos1 ); pos1+=rxF.matchedLength();
                ss1=rxF.cap(1);
                
                pos2 = rxF.search( s2, pos2 ); pos2+=rxF.matchedLength();
                ss2=rxF.cap(1);
                
                s= " "+ QString::number ( ss1.toInt() + ss2.toInt());
                ss.fill(' ', 9-s.length());
                s.prepend(ss);
                sss+=s;
                
            }
            sss.remove(0,1);
            
            sLstFinal <<  sss;
        }
        sLst.clear();
        for (int l=0; l<matrixInFileLength; l++) sLst<<sLstFinal[l];
    }
    sss=""; s1=""; s2="";
    pos1=0;	pos2=0;
    
    pos1 = rxF.search( lst[0], pos1 );
    s1=rxF.cap(1);
    
    pos2 = rxF.search( lst[(merge-1)*frameLength], pos2 );
    s2=rxF.cap(1);
    
    sss=lst[0].replace(s1, s1+"-"+s2);
    
    lst.clear();
    lst<<sss+"\n";
    for(int jj=0;jj<matrixInFileLength;jj++) lst<<sLst[jj]+"\n";
    lst<<"\n";
    return;
}

//*******************************************
//+++  TOF tools:: Merge [function]
//*******************************************
void dan18::tofMerge(int merge, int numberFrames, QStringList inputFiles, QStringList outputFiles)
{
    
    // open multi-files test
    //    QFile *test=new QFile[3];
    //    test[0].setName(inputFiles[0]);
    //	if ( !test[0].open(IO_ReadOnly ) )  return;
    
    
    //+++ Progress Dialog +++
    QProgressDialog *progress= new QProgressDialog("TOF Merge","Stop", 0,inputFiles.count());
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setLabelText("File # "+QString::number(0)+" of "+QString::number(inputFiles.count()));
    
    int mainHeaderLength=spinBoxHeaderNumberLines->value();
    int tofHeaderBeforeLength=spinBoxDataHeaderNumberLines->value();
    int tofHeaderAfterLength=spinBoxDataLinesBetweenFrames->value();
    int numberTofPerLine=spinBoxReadMatrixTofNumberPerLine->value();
    int matrixInFileLength=comboBoxMDdata->currentText().toInt();
    matrixInFileLength*=matrixInFileLength;
    matrixInFileLength/=numberTofPerLine;
    
    int frameLength=tofHeaderBeforeLength+tofHeaderAfterLength+matrixInFileLength;
    
    QStringList sFinal, sMiddle;
    QString number;
    //
    for (int i=0; i<inputFiles.count(); i++)
    {
        number=inputFiles[i];
        number=number.remove(Dir);
        number=findFileNumberInFileName(wildCard, number);
        
        mainHeaderLength=lengthMainHeader(inputFiles[i]);
        
        //+++ Progress +++
        progress->setValue(i+1);
        progress->setLabelText("File # "+QString::number(i+1)+" of "+QString::number(inputFiles.count()));
        if ( progress->wasCanceled() ) break;
        
        sFinal.clear();
        
        
        //+++++++++++++++++
        // +++ Files ++++++
        //+++++++++++++++++
        QFile fileInput( inputFiles[i] );
        //+++
        if ( !fileInput.open(IO_ReadOnly ) )  break;
        QTextStream streamInput( &fileInput );
        
        
        //+++++++++++++++++
        // +++ Header +++++
        //+++++++++++++++++
        for (int l=0; l<mainHeaderLength; l++) sFinal<<streamInput.readLine();
        
        int indexInHeader=listOfHeaders.findIndex("[Slices-Count]");
        QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
        QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
        
        QString oldCount =readNumberString(number, pos, num);
        
        sFinal[pos.toInt()-1]=sFinal[pos.toInt()-1].replace(num.toInt()-1,oldCount.length(), QString::number(int(numberFrames/merge)));
        
        
        for (int j=0;j<int(numberFrames/merge);j++){
            sMiddle.clear();
            for (int l=0; l<frameLength*merge; l++) sMiddle<<streamInput.readLine();
            tofMergeFrames(merge, sMiddle, tofHeaderBeforeLength, matrixInFileLength, frameLength, numberTofPerLine);
            
            for (int i=0; i<sMiddle.count(); i++) sFinal<<sMiddle[i].remove("\n");
            
        }
        
        
        QFile fileOutput(outputFiles[i]);
        
        //+++
        if ( !fileOutput.open( IO_WriteOnly ) )  break;
        QTextStream streamOutput( &fileOutput );
        
        for (int i=0; i<sFinal.count(); i++) streamOutput<<sFinal[i] +"\n";
        
        fileInput.close();
        fileOutput.close();
        
    }
}

//*******************************************
//+++  TOF tools:: Split [slot]
//*******************************************
void dan18::tofSplit()
{
    
    //+++
    ImportantConstants();
    //+++
    QString DirIn               = Dir;
    QString DirOut=lineEditPathRAD->text();
    //+++
    QString filter=textEditPattern->text();
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDir(DirIn);
    fd->setMode(QFileDialog::ExistingFiles);
    fd->setCaption(tr("DAN - Split TOF | RT Frames"));
    fd->setFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->name()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    //+++
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    //+++
    QStringList numberList;
    
    //+++ convert name to number
    for (int i=0; i<filesNumber; i++)
    {
        QString s=selectedDat[i];
        s=findFileNumberInFileName(wildCard, s.remove(Dir));
        numberList<< s;
    }
    
    QStringList header, lst;
    QRegExp rxF( "(\\d+)" );
    
    //+++ list of Modes
    for (int i=0; i<filesNumber; i++)
    {
        //:::+ uni-tof
        int slicesCount=readSlicesCount(numberList[i]);
        if (slicesCount>1) lst<< "RT or TOF Mode :: " + QString::number(slicesCount) + " frames";
        else lst<< "Normal Mode :: 1 frame";
        //:::+- uni-tof
    }
    
    // select TOF-mode
    bool ok;
    QString res = QInputDialog::getItem("qtiSAS :: DAN", "Select Reference for RT-mode:", lst, 0, TRUE, &ok,this );
    if ( !ok ) return;
    
    // +++ number of frames
    rxF.search( res, 0);
    int numberFrames = rxF.cap(1).toInt();
    
    //+++
    //    if (!res.contains("TOF")) return;
    //    if (numberFrames<=1) return;
    
    //+++
    int filesNumberFinal=0;
    QStringList selectedDatFinal, newFilesNames;
    
    
    bool yamlYN = false;
    if (comboBoxHeaderFormat->currentText().contains("YAML")) yamlYN=true;
    //+++
    for (int i=0; i<filesNumber; i++)
    {
        if (lst[i]==res)
        {
            selectedDatFinal<<selectedDat[i];
            if (yamlYN) newFilesNames<<DirOut+"tof5_"+numberList[i];
            else newFilesNames<<DirOut+"tof5_"+numberList[i]+"5";
            filesNumberFinal++;
        }
    }
    
    
    return tofSplit(numberFrames, selectedDatFinal, newFilesNames);
}


QString equalSpacedString(QString inputNumber, int newLength )
{
    int length=inputNumber.length();
    for(int i=length; i<newLength; i++)
    {
        inputNumber.prepend(" ");
    }
    return inputNumber;
}
//*******************************************
//+++  TOF tools:: tofSplitConvert128to8 [function]
//*******************************************
int dan18::tofSplitConvert128to8(QStringList &lst, int DIM, int matrixInFileLength, int numberTofPerLine, int numberTofPerLineNormal)
{
    
    int sum=0;
    QStringList lstSum;
    QString s, ss;
    QStringList sLst;

    for(int i=0;i<matrixInFileLength;i++)
    {
        s=lst[i].remove("\n").simplifyWhiteSpace();
        lstSum.clear();
        lstSum=lstSum.split(" ",s);

        for(int ii=0;ii<lstSum.count();ii++) sum+=lstSum[ii].toInt();
        
        int counter=0;
        
        for(int ii=0;ii<lstSum.count();ii++)
        {
            sLst<< equalSpacedString(lstSum[ii],9);
            counter++;
            if (counter>=numberTofPerLineNormal)
            {
                counter=0;
                sLst<<"\n";
            }
        }
        if (counter>0) sLst<<"\n";
    }
    lst.clear();
    lst=sLst;
    return sum;
}

//*******************************************
//+++  TOF tools:: Split [function]
//*******************************************
void dan18::tofSplit(int numberFrames, QStringList inputFiles, QStringList outputFiles)
{
    
    
    double WL=lineEditTofLambda->text().toDouble(); if (WL<=0) return;
    double DWL=lineEditTofDeltaLambda->text().toDouble(); if (DWL<=0) return;
    double DWLWL=DWL/WL;
    
    //+++ Progress Dialog +++
    QProgressDialog *progress= new QProgressDialog("TOF Spit","Stop", 0,inputFiles.count());
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setLabelText("File # "+QString::number(0)+" of "+QString::number(inputFiles.count()));
    
    int numberTofPerLineNormal=spinBoxReadMatrixNumberPerLine->value();
    int mainHeaderLength=spinBoxHeaderNumberLines->value();
    int tofHeaderBeforeLength=spinBoxDataHeaderNumberLines->value();
    int tofHeaderAfterLength=spinBoxDataLinesBetweenFrames->value();
    int numberTofPerLine=spinBoxReadMatrixTofNumberPerLine->value();
    int DIM=comboBoxMDdata->currentText().toInt();
    int matrixInFileLength=DIM;
    matrixInFileLength=matrixInFileLength*matrixInFileLength;
    matrixInFileLength=matrixInFileLength/numberTofPerLine;
    
    
    QStringList sHeader1a, sHeader1b, sHeader2, sHeader3, sFinal, sMiddle;
    QString number;
    
    //
    for (int i=0; i<inputFiles.count(); i++)
    {
        
        //+++ Progress +++
        
        progress->setValue(i+1);
        progress->setLabelText("File # "+QString::number(i+1)+" of "+QString::number(inputFiles.count()));
        if ( progress->wasCanceled() ) break;
        
        number=inputFiles[i];
        number=number.remove(Dir);
        number=findFileNumberInFileName(wildCard, number);
        
        mainHeaderLength=lengthMainHeader(inputFiles[i]);
        //+++++++++++++++++
        // +++ Files ++++++
        //+++++++++++++++++
        QFile fileInput( inputFiles[i] );
        //+++
        if ( !fileInput.open(IO_ReadOnly ) )  break;
        QTextStream streamInput( &fileInput );
        
        QString s, tmp;
        //+++++++++++++++++
        // +++ Header +++++
        //+++++++++++++++++
        int indexInHeaderSum=listOfHeaders.findIndex("[Sum]");
        int beforeSum=tableHeaderPosNew->item(indexInHeaderSum,0)->text().toInt()-1;
        
        int indexInHeaderLambda=listOfHeaders.findIndex("[Lambda]");
        
        
        sHeader1a.clear();
        for (int ii=0; ii<14; ii++) sHeader1a<<streamInput.readLine()+"\n";
        
        QString sLambda=streamInput.readLine();
        
        sHeader1b.clear();
        for (int ii=15; ii<beforeSum; ii++) sHeader1b<<streamInput.readLine()+"\n";
        
        
        streamInput.readLine();
        sHeader2.clear();
        // fefore: 2014-10-23;	for (int l=0; l<6; l++) sHeader2<<streamInput.readLine()+"\n";
        // 2015 for (int l=0; l<(mainHeaderLength- beforeSum-2); l++) sHeader2<<streamInput.readLine()+"\n";
        for (int ii=0; ii<(mainHeaderLength - beforeSum-2); ii++) sHeader2<<streamInput.readLine()+"\n";
        streamInput.readLine();
        
        double RTfactor=readDuration(number)/readSlicesDuration( number);
        
        int from, to;
        from=spinBoxTofSplitFrom->value();
        to=spinBoxTofSplitTo->value();
        
        for (int j=0;j<from-1;j++)
        {
            for (int ii=0; ii<tofHeaderBeforeLength; ii++) streamInput.readLine();
            for (int ii=0; ii<matrixInFileLength; ii++) streamInput.readLine();
            for (int ii=0; ii< tofHeaderAfterLength; ii++) streamInput.readLine();
        }
        
        
        for (int j=from-1;j<to;j++){

            
            sHeader3.clear();
            sHeader3<<"(* Initial Slice Header *)\n\n";
            
            for (int ii=0; ii<tofHeaderBeforeLength; ii++)
            {
                sHeader3<<streamInput.readLine(); sHeader3<<"\n";
            };
            
            sHeader3<<"\n";
            
            sMiddle.clear();
            for (int ii=0; ii<matrixInFileLength; ii++) sMiddle<<streamInput.readLine();
            int sum=tofSplitConvert128to8(sMiddle, DIM, matrixInFileLength, numberTofPerLine, numberTofPerLineNormal);
            
            for (int ii=0; ii< tofHeaderAfterLength; ii++) streamInput.readLine();
            
            
            sFinal.clear();
            sFinal=sHeader1a;
            
            if (radioButtonLambdaHeader->isChecked() &&tableHeaderPosNew->item(indexInHeaderLambda,0)->text().contains("{lambda=}") && sLambda.find("lambda=")>0)
            {
                sLambda=sLambda.left(sLambda.find("lambda="));
                sLambda+="lambda="+ QString::number(WL+ (j-int(numberFrames/2))*DWL, 'F',2)+"A";
            }
            
            sFinal+=sLambda+"\n";
            
            sFinal+=sHeader1b;
            
            
            sFinal<<QString::number(sum, 'E', 6)+" 0.000000E+00 0.000000E+00 0.000000E+00 0.000000E+00 0.000000E+00\n";
            sFinal+=sHeader2;
            sFinal+=sHeader3;
            
            
            
            sFinal<<"(* TOF block *)\n\n";
            
            
            sFinal<<"Lambda= " <<QString::number(WL+ (j-int(numberFrames/2))*DWL, 'F',3)<<"\n";
            sFinal<<"Delta-Lambda= " <<QString::number(DWLWL,'F',3)<<"\n\n";
            
            sFinal<<"Slices-Count= " <<QString::number(numberFrames)<<"\n";
            sFinal<<"Slices-Duration= " <<QString::number(readSlicesDuration( number))<<"\n\n";
            
            sFinal<<"Slices-Current-Number= " <<QString::number(j+1)<<"\n";
            sFinal<<"Slices-Current-Duration= " <<QString::number(readSlicesDuration( number))<<"\n";
            sFinal<<"Slices-Current-Monitor1= " <<QString::number(readMonitor1( number )/RTfactor, 'E', 6)<<"\n";
            sFinal<<"Slices-Current-Monitor2= " <<QString::number(readMonitor2( number )/RTfactor, 'E', 6)<<"\n";
            sFinal<<"Slices-Current-Monitor3= " <<QString::number(readMonitor3( number )/RTfactor, 'E', 6)<<"\n";
            sFinal<<"Slices-Current-Sum= " <<QString::number(sum, 'E', 6)<<"\n\n";
            
            sFinal<<"$\n";
            
            sFinal+=sMiddle;
            
            if (j>=999) s="0";
            else if (j>=99) s="00";
            else if (j>=9) s="000";
            else s="0000";
            
            QString sss=outputFiles[i]+s+QString::number(j+1)+"_";
            
            if(pushButtonTofAll->isDefault() && checkBoxTofSuffix->isChecked() && lineEditTofSuffix->text()!="")  sss+=lineEditTofSuffix->text();
            else sss+="Split";
            sss+=".DAT";
            
            if (j>=from-1 && j<=to-1)
            {
                QFile fileOutput(sss);
                
                //+++
                if ( !fileOutput.open( IO_WriteOnly ) )  break;
                QTextStream streamOutput( &fileOutput );
                
                for (int i=0; i<sFinal.count(); i++) streamOutput<<sFinal[i];
                
                fileOutput.close();
            }
            //streamInput.readLine();
        }
		
        fileInput.close();
    }
    
}

//*******************************************
//+++  TOF tools:: tofCheckShift [slot]
//*******************************************
void dan18::tofCheckShift()
{
    
    spinBoxCollapse->setValue(2);
    
    ImportantConstants();
    
    QString filter=textEditPattern->text();
    
    QString file =
    QFileDialog::getOpenFileName(
                                 Dir,
                                 filter,
                                 this,
                                 "open file dialog",
                                 "Choose a TOF-reference file" );
    
    
    //+++++++++++++++++
    // +++ Files ++++++
    //+++++++++++++++++
    QFile fileInput( file );
    //+++
    if ( !fileInput.open(IO_ReadOnly ) )  return;
    QTextStream streamInput( &fileInput );
    
    int numberTofPerLineNormal=spinBoxReadMatrixNumberPerLine->value();
    int mainHeaderLength=spinBoxHeaderNumberLines->value();
    int tofHeaderBeforeLength=spinBoxDataHeaderNumberLines->value();
    int tofHeaderAfterLength=spinBoxDataLinesBetweenFrames->value();
    int numberTofPerLine=spinBoxReadMatrixTofNumberPerLine->value();
    int DIM=comboBoxMDdata->currentText().toInt();
    int matrixInFileLength=DIM;
    matrixInFileLength*=matrixInFileLength;
    matrixInFileLength/=numberTofPerLine;
    
    int frameLength=tofHeaderBeforeLength+tofHeaderAfterLength+matrixInFileLength;
    //+++++++++++++++++
    // +++ Header +++++
    //+++++++++++++++++
    for (int l=0; l<(mainHeaderLength-1); l++) streamInput.readLine();
    
    int inputFrames=comboBoxTofInputAllFrames->currentText().toInt();
    int outputFrames=comboBoxTofOutputFrames->currentText().toInt();
    
    QStringList sMiddle, lst;
    for (int j=0;j<inputFrames;j++){
        streamInput.readLine();
        sMiddle.clear();
        for (int l=0; l<matrixInFileLength; l++) sMiddle<<streamInput.readLine();
        int sum=tofSplitConvert128to8(sMiddle, DIM, matrixInFileLength, numberTofPerLine, numberTofPerLineNormal);
        lst<<QString::number(sum);
        streamInput.readLine();
    }
    
    int iMax=0;
    int max=0;
    
    for (int j=0;j<int(lst.count()/2);j++) if (lst[j].toInt()>max) {max=lst[j].toInt(); iMax=j;};
    
    int shift=iMax-inputFrames/4+1;
    
    if (shift>0){
        
        spinBoxTofShift->setValue(shift);
    }
    else
    {
        iMax=inputFrames/2;
        max=0;
        for (int j=int(lst.count()/2);j<lst.count();j++) if (lst[j].toInt()>max) {max=lst[j].toInt(); iMax=j;};
        
        shift=iMax-inputFrames/4;
        
        if (shift>0){
            
            spinBoxTofShift->setValue(shift);
        }
    }
    
    double mergedFrames=spinBoxMerge->value();
    QString ss=file;
    ss=ss.remove(Dir);
    double WL=readLambda( findFileNumberInFileName(wildCard, ss));
    double spread=spinBoxWLS->value();
    double teoSpread=2*spread/inputFrames*2*mergedFrames;
    
    lineEditTheoWLS->setText(QString::number(teoSpread, 'F',1));
    lineEditTofLambda->setText(QString::number(WL,'F',2));
    lineEditTofDeltaLambda->setText(QString::number(WL*teoSpread/100,'F',3));
}

//*******************************************
//+++  TOF tools:: Check [slot]
//*******************************************
void dan18::tofCheck()
{
    
    int inputFrames=comboBoxTofInputAllFrames->currentText().toInt();
    int outputFrames=comboBoxTofOutputFrames->currentText().toInt();
    
    inputFrames=int(inputFrames/2);
    
    if (int(outputFrames/2)*2==outputFrames){
        QMessageBox::critical( 0, "qtiSAS :: TOF", "Final number of Frames should be ODD");
        return;
    } 
    
    int remove=int((inputFrames - int(inputFrames/outputFrames)*outputFrames)/2);
    spinBoxRemove->setValue(remove);
    
    int merge= int(inputFrames/outputFrames);
    
    spinBoxMerge->setValue(merge);
    
    tofCheckShift();
    
}

//*******************************************
//+++  TOF tools:: All [slot]
//*******************************************
void dan18::tofAll()
{
    
    //+++
    ImportantConstants();
    
    //+++	
    QString DirIn               = Dir;
    QString DirOut=lineEditPathRAD->text();    
    //+++
    QString filter=textEditPattern->text();
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDir(DirIn);
    fd->setMode(QFileDialog::ExistingFiles);
    fd->setCaption(tr("DAN - Select TOF files"));
    fd->setFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->name()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    //+++
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    //+++
    QStringList numberList;
    
    //+++ convert name to number
    for (int i=0; i<filesNumber; i++)
    {
        QString s=selectedDat[i];
        s=s.remove(DirIn);
        s=findFileNumberInFileName(wildCard, s);
        numberList<< s;
    }
    
    QStringList header, lst;
    QRegExp rxF( "(\\d+)" );
    
    //+++ list of Modes 
    for (int i=0; i<filesNumber; i++)
    {
        //:::+ uni-tof
        int slicesCount=readSlicesCount(numberList[i]);
        if (slicesCount>1) lst<< "RT or TOF Mode :: " + QString::number(slicesCount) + " frames";
        else lst<< "Normal Mode :: 1 frame";	
        //:::+- uni-tof		
    }
    
    // select TOF-mode
    bool ok;
    QString res = QInputDialog::getItem("qtiSAS :: DAN", "Select TOF|RT mode:", lst, 0, TRUE, &ok,this );
    if ( !ok ) return;
    
    // +++ number of frames
    rxF.search( res, 0);
    int numberFrames = rxF.cap(1).toInt();
    
    //+++
    if (!res.contains("TOF")) return;
    if (numberFrames<=1) return;
    
    //+++
    int filesNumberFinal=0;
    QStringList selectedDatFinal, filesNumbers, newFilesNames;
    
    
    //+++
    for (int i=0; i<filesNumber; i++)
    {
        if (lst[i]==res)
        {
            selectedDatFinal<<selectedDat[i];
            filesNumbers<<numberList[i];
            filesNumberFinal++;
        }
    }
    
    
    bool rawData=true;
    
    lineEditPathRAD->setText(DirIn);
    
    QDir dd(DirIn);
    
    //+++ shift
    int shift=spinBoxTofShift->value();
    
    if (shift>0)
    {	
        //+++
        for (int i=0; i<filesNumberFinal; i++)
        {
            
            newFilesNames<<DirIn+"tof1_"+filesNumbers[i]+"1_Shift.DAT";
        }
        //+++
        tofShift(shift, numberFrames, selectedDatFinal, newFilesNames);
        selectedDatFinal.clear();
        selectedDatFinal=newFilesNames;
        newFilesNames.clear();
        rawData=false;
    }    
    
    
    
    
    
    //+++
    int collapse=spinBoxCollapse->value();
    
    if (collapse>1)
    {
        //+++
        for (int i=0; i<filesNumberFinal; i++)
        {    
            newFilesNames<<DirIn+"tof2_"+filesNumbers[i]+"12_Collapse.DAT";
        }
        //+++
        tofCollapse(collapse, numberFrames, selectedDatFinal, newFilesNames );	
        if (checkBoxTofDelete->isChecked()) for (int i=0; i<filesNumberFinal; i++) dd.remove ( selectedDatFinal[i]);
        selectedDatFinal.clear();
        selectedDatFinal=newFilesNames;
        newFilesNames.clear();
        numberFrames=int(numberFrames/collapse);
        
        rawData=false;
    }
    
    //+++
    int remove=spinBoxRemove->value();
    
    if (remove>0)
    {
        //+++
        for (int i=0; i<filesNumberFinal; i++)
        {	    
            newFilesNames<<DirIn+"tof3_"+filesNumbers[i]+"123_Remove.DAT";
        }
        //+++
        tofRemove(remove,  numberFrames, selectedDatFinal, newFilesNames );
        if (checkBoxTofDelete->isChecked()) for (int i=0; i<filesNumberFinal; i++) dd.remove ( selectedDatFinal[i]);
        selectedDatFinal.clear();
        selectedDatFinal=newFilesNames;
        newFilesNames.clear();	
        numberFrames=numberFrames-2*remove;
        
        rawData=false;
    }
    
    
    
    
    //+++
    int merge=spinBoxMerge->value();
    
    if (merge>1)
    {
        //+++
        for (int i=0; i<filesNumberFinal; i++)
        {
            newFilesNames<<DirIn+"tof4_"+filesNumbers[i]+"1234_Merge.DAT";
        }
        //+++
        tofMerge(merge, numberFrames, selectedDatFinal, newFilesNames);
        if (checkBoxTofDelete->isChecked()) for (int i=0; i<filesNumberFinal; i++) dd.remove ( selectedDatFinal[i]);
        selectedDatFinal.clear();
        selectedDatFinal=newFilesNames;
        newFilesNames.clear();
        numberFrames=int(numberFrames/merge);
        
        rawData=false;
    }
    
    //+++
    QString s;
    
    for (int i=0; i<filesNumberFinal; i++)
    {
        s=DirOut;
        if (checkBoxTofPrefix->isChecked()) 
        {
            QString ss=lineEditTofPrefix->text();
            if(ss!="") s+=ss+"_";
            else s+="tof5_";
        }
        else s+="tof5_";
        
        s+=filesNumbers[i];
        
        if (!checkBoxTof12345->isChecked()) s+="12345";
        
        newFilesNames<<s;
    }
    
    //+++
    lineEditPathRAD->setText(DirOut);
    pushButtonTofAll->setDefault(true);
    tofSplit(numberFrames, selectedDatFinal, newFilesNames);
    
    if (checkBoxTofDelete->isChecked() && !rawData) for (int i=0; i<filesNumberFinal; i++) dd.remove ( selectedDatFinal[i]);
    pushButtonTofAll->setDefault(false);
}

inline void readLineToVector(QString line, gsl_vector_int *v, int length)
{
    QStringList lst;
    line=line.replace(",", " "); // new :: bersans
    line=line.simplifyWhiteSpace();
    lst=lst.split(" ", line );
    for (int i=0;i<length;i++) gsl_vector_int_set(v, i, lst[i].toInt());
    
}

//*******************************************
//+++  TOF:: Add Matrixes New Test [2016]
//*******************************************
bool dan18::addNmatrixes2016(QStringList files, QStringList fileNumers, QTextStream &matrixesText, int mainHeaderLength, int framesCount)
{
    std::cout<<"\n";
    
    int N=files.count();
    if (N<1) return false;
    
//    matrixesText.clear();
    
    int MDinFile=comboBoxMDdata->currentText().toInt();
    int numbersPerRow=spinBoxReadMatrixTofNumberPerLine->value();
    
    int localHeaderLength = spinBoxDataHeaderNumberLines->value();
    int spaceBetweenFrames = spinBoxDataLinesBetweenFrames->value();
    
    // open multi-files test
    QFile *file=new QFile[N];
    QTextStream *streamInput=new QTextStream[N];
    
    //+++
    for(int f=0;f<N;f++) {file[f].setName(files[f]);};
    //+++
    for(int f=0;f<N;f++)
    {
        if ( !file[f].open(IO_ReadOnly ) )  return false;
        streamInput[f].setDevice( &file[f] );
    };
    //+++
    for(int f=0;f<N;f++) { for (int ii=0; ii<mainHeaderLength; ii++) streamInput[f].readLine();};
    
    //+++
    gsl_vector_int *v0 = gsl_vector_int_calloc (MDinFile);
    gsl_vector_int *vi = gsl_vector_int_calloc (MDinFile);
    QString s;
    
    for(int ff=0;ff<framesCount;ff++)
    {
        //+++ reading of local header
        for (int ii=0; ii<localHeaderLength; ii++) {matrixesText << streamInput[0].readLine()+"\n";};
        for(int f=1;f<N;f++) { for (int ii=0; ii<localHeaderLength; ii++) streamInput[f].readLine();};
        //+++ matrixes
        for (int i=0;i<MDinFile;i++)
        {
            readLineToVector( streamInput[0].readLine(), v0, MDinFile);
            for(int f=1;f<N;f++)
            {
                readLineToVector( streamInput[f].readLine(), vi, MDinFile);
                gsl_vector_int_add (v0, vi);
            }
            s=" ";
            for (int j=0; j<MDinFile;j++)
            {
                s+=QString::number(gsl_vector_int_get(v0,j),'f',0);
                s+="   ";
            }
            matrixesText << s+"\n";
        }
        //+++ reading of post header
        for (int ii=0; ii<spaceBetweenFrames; ii++) matrixesText << streamInput[0].readLine()+"\n";
        for(int f=1;f<N;f++) for (int ii=0; ii<spaceBetweenFrames; ii++) streamInput[f].readLine();
        if (int((ff+1)/10)*10==ff+1)
        {
            std::cout<<ff+1<<".";std::cout.flush();
            if (int((ff+1)/100)*100==ff+1) std::cout<<"\n";
        };
    }
    


    for(int f=0;f<N;f++) file[f].close();
    

    
    gsl_vector_int_free(v0);
    gsl_vector_int_free(vi);
    
    return true;
}

inline void readFrameToMatrix(QTextStream &streamInput, gsl_matrix_int *m, int length)
{
    QString line;
    QStringList lst;
    for (int i=0;i<length;i++)
    {
        line=streamInput.readLine();
        line=line.replace(",", " "); // new :: bersans
        line=line.simplifyWhiteSpace();
        lst=lst.split(" ", line );
        for (int j=0;j<length;j++) gsl_matrix_int_set(m, i, j, lst[j].toInt());
    }
    
}

//*******************************************
//+++  TOF:: Add Matrixes New Test [2016] matrix
//*******************************************
bool dan18::addNmatrixes2016matrix(QStringList files, QStringList fileNumers, QTextStream &matrixesText, int mainHeaderLength, int framesCount)
{
    std::cout<<"\n";
    int N=files.count();
    if (N<1) return false;
    
//    matrixesText.clear();
    
    int MDinFile=comboBoxMDdata->currentText().toInt();
    int numbersPerRow=spinBoxReadMatrixTofNumberPerLine->value();
    
    int localHeaderLength = spinBoxDataHeaderNumberLines->value();
    int spaceBetweenFrames = spinBoxDataLinesBetweenFrames->value();
    
    // open multi-files test
    QFile *file=new QFile[N];
    QTextStream *streamInput=new QTextStream[N];
    
    //+++
    for(int f=0;f<N;f++) {file[f].setName(files[f]);};
    //+++
    for(int f=0;f<N;f++)
    {
        if ( !file[f].open(IO_ReadOnly ) )  return false;
        streamInput[f].setDevice( &file[f] );
    };
    //+++
    for(int f=0;f<N;f++) { for (int ii=0; ii<mainHeaderLength; ii++) streamInput[f].readLine();};
    
    //+++
    gsl_matrix_int *m0 = gsl_matrix_int_calloc (MDinFile,MDinFile);
    gsl_matrix_int *mi = gsl_matrix_int_calloc (MDinFile,MDinFile);

    QString s;
    
    for(int ff=0;ff<framesCount;ff++)
    {
        //+++ reading of local header
        for (int ii=0; ii<localHeaderLength; ii++) {matrixesText << streamInput[0].readLine()+"\n";};
        for(int f=1;f<N;f++) { for (int ii=0; ii<localHeaderLength; ii++) streamInput[f].readLine();};

        //+++ matrixes
        readFrameToMatrix(streamInput[0], m0, MDinFile);

        for(int f=1;f<N;f++)
        {
            readFrameToMatrix(streamInput[f], mi, MDinFile);
            gsl_matrix_int_add (m0, mi);
        }
        
        for (int i=0;i<MDinFile;i++)
        {
            s=" ";
            for (int j=0; j<MDinFile;j++)
            {
                s+=QString::number(gsl_matrix_int_get(m0,i,j),'f',0);
                s+="   ";
            }
            matrixesText<<s+"\n";
        }

        //+++ reading of post header
        for (int ii=0; ii<spaceBetweenFrames; ii++) matrixesText << streamInput[0].readLine()+"\n";
        for(int f=1;f<N;f++) for (int ii=0; ii<spaceBetweenFrames; ii++) streamInput[f].readLine();
        if (int((ff+1)/10)*10==ff+1)
        {
            std::cout<<ff+1<<".";std::cout.flush();
            if (int((ff+1)/100)*100==ff+1) std::cout<<"\n";
        };
    }
    
    
    
    for(int f=0;f<N;f++) file[f].close();
    
    
    
    gsl_matrix_int_free(m0);
    gsl_matrix_int_free(mi);
    
    return true;
}

