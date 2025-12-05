/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS data analysis interface
 ******************************************************************************/

#include <zlib.h>

#include "dan18.h"

#include "parser-hdf5.h"

//*******************************************
//+++  RT tools:: Sum [slot]
//*******************************************
void dan18::rtConnectSlots()
{
    connect( pushButtonAddManyDat, SIGNAL( clicked() ), this, SLOT(tofAddFiles()));
    connect(pushButtonAddRTtable, &QToolButton::clicked, this, &dan18::addFilesInActiveTableCols);
    connect( pushButtonRTsum, SIGNAL( clicked() ), this, SLOT(  rtSumRead() ) );
    connect( pushButtonRTMergeLinear,     SIGNAL( clicked() ), this, SLOT( rtMergeLinear() ) );
    connect( pushButtonRTMergeProgressive,     SIGNAL( clicked() ), this, SLOT( rtMergeProgressive() ) );
    connect( pushButtonSplitFrames, SIGNAL( clicked() ), this, SLOT( rtSplit() ) );
    connect( pushButtonRTAll,     SIGNAL( clicked() ), this, SLOT( rtAll() ) );
    connect( lineEditSplitFramesProgr, SIGNAL( valueChanged(int) ), this, SLOT( geometricalRatioPrefix(int) ) );
    connect( lineEditSplitFramesProgrMeasured, SIGNAL( valueChanged(int) ), this, SLOT( geometricalRatioPrefixMeasured(int) ) );
}

//*******************************************
//+++  RT tools:: Sum [slot]
//*******************************************
void dan18::rtSumRead()
{
    QString Dir = filesManager->pathInString();
    QString DirOut = filesManager->pathOutString();
    QString filter = textEditPattern->text(); // move to filesManager
    QString wildCard = filesManager->wildCardDetector();
    bool imageData = radioButtonDetectorFormatImage->isChecked();

    bool ok;
    static const QRegularExpression rxF("(\\d+)");

    //+++ select files
    auto *fd = new QFileDialog(this, "Getting File Information", Dir, "*");
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - RT :: Read Sums"));
    fd->setNameFilter(filter + ";;" + textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (fd->exec() == QDialog::Rejected)
        return;
    
    
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    QStringList numberList;

    // convert name to number
    for (int i = 0; i < filesNumber; i++)
    {
        QString s = selectedDat[i];
        s = s.remove(Dir);
        s = FilesManager::findFileNumberInFileName(wildCard, s);
        numberList << s;
    }

    QStringList header, lst;

    // list of Modes
    for (int i = 0; i < filesNumber; i++)
    {
        int slicesCount = tofrt->readSlicesCount(numberList[i]);
        if (slicesCount > 1)
            lst << "RT Mode :: " + QString::number(slicesCount) + " frames";
        else
            lst << "Normal Mode :: 1 frame";
    }

    // select RT-mode
    QString res = QInputDialog::getItem(this,
                                        "qtiSAS :: DAN", "Select Reference for RT - mode:", lst, 0, true, &ok);
    if ( !ok ) return;
    
    // +++ number of frames
    int numberFrames = rxF.match(res).captured().toInt();
    
    if ( numberFrames<=1) {
        QMessageBox::critical( 0, "qtiSAS", "... You selected non-RT file ...");
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

    QString tableName = "sum-rt-";

    if (imageData)
        return rtSumReadBinary(numberFrames, selectedDatFinal, tableName);
    else
        return rtSumRead(numberFrames, selectedDatFinal, tableName);
}
//*******************************************
//+++  RT tools:: Merge Linear [lsot]
//*******************************************
void dan18::rtMergeLinear()
{
    QString Dir = filesManager->pathInString();
    QString DirOut = filesManager->pathOutString();
    QString filter = textEditPattern->text(); // move to filesManager
    QString wildCard = filesManager->wildCardDetector();

    int merge = spinBoxMergeRT->value();

    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Merge RT Frames (LINEAR)"));
    fd->setNameFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (fd->exec() == QDialog::Rejected)
        return;
    
    //+++
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    //+++
    QStringList numberList;

    //+++ convert name to number
    for (int i = 0; i < filesNumber; i++)
    {
        QString s = selectedDat[i];
        s = FilesManager::findFileNumberInFileName(wildCard, s.remove(Dir));
        numberList << s;
    }

    QStringList header, lst;
    static const QRegularExpression rxF("(\\d+)");

    //+++ list of Modes
    for (int i = 0; i < filesNumber; i++)
    {
        //:::+ uni-rt
        int slicesCount = tofrt->readSlicesCount(numberList[i]);
        if (slicesCount > 1)
            lst << "RT or TOF Mode :: " + QString::number(slicesCount) + " frames";
        else
            lst << "Normal Mode :: 1 frame";
        //:::+- uni-tof
        
    }

    // select TOF-mode
    bool ok;
    QString res = QInputDialog::getItem(this, "qtiSAS :: DAN", "Select Reference for RT-mode:", lst, 0, true, &ok);
    if ( !ok ) return;
    
    // +++ number of frames
    int numberFrames = rxF.match(res).captured().toInt();
    
    //+++
    if (merge>numberFrames) return;
    if (!res.contains("RT")) return;
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
            newFilesNames<<DirOut+"rt"+QString::number(merge)+"_"+numberList[i]+QString::number(merge)+"_frames_"+QString::number(numberFrames)+"_linear_"+QString::number(merge)+".DAT";
            filesNumberFinal++;
        }
    }
    
    
    rtMerge(numberFrames, merge, 0, selectedDatFinal, newFilesNames);
    return ;
}

//+++  RT tools:: Merge Progressive
void dan18::rtMergeProgressive()
{
    QString Dir = filesManager->pathInString();
    QString DirOut = filesManager->pathOutString();
    QString filter = textEditPattern->text(); // move to filesManager
    QString wildCard = filesManager->wildCardDetector();

    int merge = lineEditSplitFramesProgr->value();

    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Merge RT Frames (PROGRESSIVE)"));
    fd->setNameFilter(filter+";;"+textEditPattern->text());
    foreach(QComboBox *obj, fd->findChildren<QComboBox *>())
        if (QString(obj->objectName()).contains("fileTypeCombo"))
            obj->setEditable(true);
    
    if (fd->exec() == QDialog::Rejected)
        return;
    
    //+++
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    //+++
    QStringList numberList;

    //+++ convert name to number
    for (int i = 0; i < filesNumber; i++)
    {
        QString s = selectedDat[i];
        s = FilesManager::findFileNumberInFileName(wildCard, s.remove(Dir));
        numberList << s;
    }

    QStringList header, lst;
    static const QRegularExpression rxF("(\\d+)");

    //+++ list of Modes
    for (int i = 0; i < filesNumber; i++)
    {
        //:::+ uni-rt
        int slicesCount = tofrt->readSlicesCount(numberList[i]);
        if (slicesCount > 1)
            lst << "RT Mode :: " + QString::number(slicesCount) + " frames";
        else
            lst << "Normal Mode :: 1 frame";
        //:::+- uni-tof
    }

    // select TOF-mode
    bool ok;
    QString res = QInputDialog::getItem(this, "qtiSAS :: DAN", "Select Reference for RT-mode:", lst, 0, true, &ok);
    if ( !ok ) return;
    
    // +++ number of frames
    int numberFrames = rxF.match(res).captured().toInt();
    
    //+++
    if (!res.contains("RT")) return;
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
            
            QString newName;
            
            if (selectedDat[i].contains("_frames_"))
            {
                QString s=selectedDat[i];
                s=s.remove(".DAT").remove(Dir);
                //+++
                newName= DirOut+s.left(s.indexOf("_"));
                newName=newName + QString::number(merge) + "_" + numberList[i] + QString::number(merge);
                newName=newName + "_" + s.right (s.length() - s.indexOf("frames")) +"_progressive_"+QString::number(merge)+".DAT";
            }
            else
            {
                newName = DirOut+"rt1"+QString::number(merge) +"_"+numberList[i]+ QString::number(merge);
                newName=newName +"_frames_"+QString::number(numberFrames)+"_linear_1";
                newName=newName +"_progressive_"+QString::number(merge)+".DAT";
            }
            
            newFilesNames<<newName;
            
            filesNumberFinal++;
        }
    }
    rtMerge(numberFrames, 1, merge, selectedDatFinal, newFilesNames);
    return;
}

//*******************************************
//+++  RT tools:: rtMerge [function]
//*******************************************
int dan18::rtMerge(int initialNumberFrames, int linearMerging, int geometricalMerging, QStringList inputFiles, QStringList outputFiles)
{
    QString Dir = filesManager->pathInString();
    QString wildCard = filesManager->wildCardDetector();

    int numberFrames=int( initialNumberFrames/linearMerging );
    
    QList<int> geometricalSplitting;
    
    int newNumberFrames=geomerticalProgressionCalculation(numberFrames, geometricalMerging, geometricalSplitting);
    
    //+++ Progress Dialog +++
    QProgressDialog *progress= new QProgressDialog("RT Merging ... ","Stop", 0,inputFiles.count());
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

    for (int i = 0; i < inputFiles.count(); i++)
    {
        number = inputFiles[i];
        number = number.remove(Dir);
        number = FilesManager::findFileNumberInFileName(wildCard, number);

        mainHeaderLength=lengthMainHeader(inputFiles[i]);
        
        //+++ Progress +++
        progress->setValue(i);
        progress->setLabelText("File # "+QString::number(i+1)+" of "+QString::number(inputFiles.count()));
        if ( progress->wasCanceled() ) break;
        
        sFinal.clear();
        
        
        //+++++++++++++++++
        // +++ Files ++++++
        //+++++++++++++++++
        QFile fileInput( inputFiles[i] );
        //+++
        if (!fileInput.open(QIODevice::ReadOnly))  break;
        QTextStream streamInput( &fileInput );
        
        
        //+++++++++++++++++
        // +++ Header +++++
        //+++++++++++++++++
        for (int l=0; l<mainHeaderLength; l++) sFinal<<streamInput.readLine();

        int indexInHeader = parserHeader->listOfHeaders.indexOf("[Slices-Count]");
        QString pos = tableHeaderPosNew->item(indexInHeader, 0)->text();
        QString num = tableHeaderPosNew->item(indexInHeader, 1)->text();

        QString oldCount = parserHeader->readNumberString(number, pos, num);

        sFinal[pos.toInt()-1]=sFinal[pos.toInt()-1].replace(num.toInt()-1,oldCount.length(), QString::number(newNumberFrames));
        
        
        for (int j=0;j<newNumberFrames;j++)
        {
            sMiddle.clear();
            for (int l=0; l<frameLength*linearMerging*geometricalSplitting[j]; l++) sMiddle<<streamInput.readLine();
            tofMergeFrames(linearMerging*geometricalSplitting[j], sMiddle, tofHeaderBeforeLength, matrixInFileLength, frameLength, numberTofPerLine);
            
            for (int i=0; i<sMiddle.count(); i++) sFinal<<sMiddle[i].remove("\n");
            
        }
        
        
        QFile fileOutput(outputFiles[i]);
        
        //+++
        if (!fileOutput.open(QIODevice::WriteOnly))  break;
        QTextStream streamOutput( &fileOutput );
        
        for (int i=0; i<sFinal.count(); i++) streamOutput<<sFinal[i] +"\n";
        
        fileInput.close();
        fileOutput.close();
        
    }
    progress->close();
    return newNumberFrames;
}


//*******************************************
//+++  RT tools:: rtSumRead [function]
//*******************************************
void dan18::rtSumRead(int numberFrames, QStringList inputFiles, QString tableName)
{
    QString Dir = filesManager->pathInString();
    QString wildCard = filesManager->wildCardDetector();

    //+++ Progress Dialog +++
    QProgressDialog *progress= new QProgressDialog("RT :: SUM","Stop", 0,inputFiles.count());
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
    
    Table *tableDat=app()->newTable(app()->generateUniqueName(tableName), numberFrames,2*inputFiles.count()+5);
    tableDat->setColName(4,"Number");
    tableDat->setColName(1,"from");
    tableDat->setColName(2,"to");
    tableDat->setColName(3,"NumberFrames");
    tableDat->setColName(0,"NormalizedNumber");
    
    
    QString sTOForRT="Real Time";

    QString s = inputFiles[0];
    sTOForRT = "Real Time :: Detector Sum(s) :: " + FilesManager::findFileNumberInFileName(wildCard, s.remove(Dir));

    for (int i = 1; i < inputFiles.count(); i++)
    {
        s = inputFiles[i];
        sTOForRT = sTOForRT + ", " + FilesManager::findFileNumberInFileName(wildCard, s.remove(Dir));
    }

    tableDat->setWindowLabel(sTOForRT);
    app()->setListViewLabel(tableDat->name(), sTOForRT);
    app()->updateWindowLists(tableDat);
    
    int initianNumberFrames=numberFrames;
    int mergedLinear=1;
    int mergedProgressive=0;
    
    QString firstNileName=inputFiles[0];
    firstNileName=firstNileName.remove(Dir).remove(".DAT");
    
    if (firstNileName.contains("_frames_"))
    {
        QStringList list = firstNileName.split("_", Qt::SkipEmptyParts);
        
        
        initianNumberFrames=list[list.indexOf("frames")+1].toInt();
        
        if (firstNileName.contains("linear")) mergedLinear=list[list.indexOf("linear")+1].toInt();
        
        if (firstNileName.contains("progressive")) mergedProgressive=list[list.indexOf("progressive")+1].toInt();
        
    }
    
    QList<int> geometricalSplitting;
    if (geomerticalProgressionCalculation(int(initianNumberFrames/mergedLinear), mergedProgressive, geometricalSplitting) !=numberFrames)
    {
        QMessageBox::critical( 0, "qtiSAS", "... check RT data numbers ...");
        return;
    }

    for (int i = 0; i < inputFiles.count(); i++)
    {
        s = inputFiles[i];
        s = FilesManager::findFileNumberInFileName(wildCard, s.remove(Dir));

        tableDat->setColName(5 + 2 * i, "Sum-" + s);
        tableDat->setColName(5 + 2 * i + 1, "NormalizedSum-" + s);
        tableDat->setColComment(5 + 2 * i, "File: " + s);
        tableDat->setColComment(5 + 2 * i + 1, "File: " + s);
    }

    int currentSumOfFrames=0;
    int from, to, numberOfFrames;
    
    
    for (int i=0;i<numberFrames;i++)
    {
        numberOfFrames=geometricalSplitting[i]*mergedLinear;
        from=currentSumOfFrames+1;
        to=currentSumOfFrames+numberOfFrames;
        currentSumOfFrames+=numberOfFrames;
        
        tableDat->setText(i,4,QString::number(i+1));
        tableDat->setText(i,1,QString::number(from));
        tableDat->setText(i,2,QString::number(to));
        tableDat->setText(i,3,QString::number(numberOfFrames));
        tableDat->setText(i,0,QString::number((from+to)/2.0));
    }
    
    
    QStringList sMiddle;
    QString number;
    //
    std::cout<<"\n";
    for (int i=0; i<inputFiles.count(); i++)
    {
        //+++ Progress +++
        progress->setValue(i);
        progress->setLabelText("File # "+QString::number(i+1)+" of "+QString::number(inputFiles.count()));
        if ( progress->wasCanceled() ) break;

        number = inputFiles[i];
        number = number.remove(Dir);
        number = FilesManager::findFileNumberInFileName(wildCard, number);

        mainHeaderLength=lengthMainHeader(inputFiles[i]);
        if (comboBoxHeaderFormat->currentText().contains("YAML")) mainHeaderLength=0;

        int numberRepetitions = monitors->readNumberRepetitions(number);

        //+++++++++++++++++
        // +++ Files ++++++
        //+++++++++++++++++
        QFile fileInput( inputFiles[i] );
        //+++
        if (!fileInput.open(QIODevice::ReadOnly))  break;
        QTextStream streamInput( &fileInput );
        
        QString s;
        
        for (int ii=0; ii<mainHeaderLength; ii++) streamInput.readLine();
        
        std::cout << "[" << number.toLocal8Bit().constData() << "]: reading of sum's\n";
        for (int j=0;j<numberFrames;j++)
        {
            // read header
            for (int ii=0; ii<tofHeaderBeforeLength; ii++) streamInput.readLine();
            
            sMiddle.clear();
            // read matrix
            for (int ii=0; ii<matrixInFileLength; ii++) sMiddle<<streamInput.readLine();
            
            int sum=tofSumCulculate(sMiddle, matrixInFileLength, numberTofPerLine);
            int currentMergedFrames=tableDat->text(j,3).toInt();
            tableDat->setText(j,5+2*i,QString::number(sum));
            tableDat->setText(j,5+2*i+1,QString::number(double(sum)/double(currentMergedFrames)/double(numberRepetitions),'f',3 ));
            
            // read post header
            for (int ii=0; ii<tofHeaderAfterLength; ii++) streamInput.readLine();
            
            if (int((j+1)/10)*10==j+1)
            {
                std::cout<<sum<<"["<<j+1<<"].";std::cout.flush();
                if (int((j+1)/100)*100==j+1) std::cout<<"\n";
            };
            
        }
        
        fileInput.close();
    }
    progress->close();
}

//*******************************************
//+++  RT tools Binary:: rtSumRead [function]
//*******************************************
void dan18::rtSumReadBinary(int numberFrames, QStringList inputFiles, QString tableName)
{
    QString Dir = filesManager->pathInString();
    QString wildCard = filesManager->wildCardDetector();

    //+++ Progress Dialog +++
    QProgressDialog *progress= new QProgressDialog("RT :: SUM :: Binary","Stop", 0,inputFiles.count());
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setLabelText("File # "+QString::number(0)+" of "+QString::number(inputFiles.count()));
    

    int DIM=comboBoxMDdata->currentText().toInt();
    
    Table *tableDat=app()->newTable(app()->generateUniqueName(tableName), numberFrames,2*inputFiles.count()+5);
    tableDat->setColName(4,"Number");
    tableDat->setColName(1,"from");
    tableDat->setColName(2,"to");
    tableDat->setColName(3,"NumberFrames");
    tableDat->setColName(0,"NormalizedNumber");

    QString sTOForRT="Real Time";

    QString s = inputFiles[0];
    sTOForRT = "Real Time :: Detector Sum(s) :: " + FilesManager::findFileNumberInFileName(wildCard, s.remove(Dir));

    for (int i = 1; i < inputFiles.count(); i++)
    {
        s = inputFiles[i];
        sTOForRT = sTOForRT + ", " + FilesManager::findFileNumberInFileName(wildCard, s.remove(Dir));
    }

    tableDat->setWindowLabel(sTOForRT);
    app()->setListViewLabel(tableDat->name(), sTOForRT);
    app()->updateWindowLists(tableDat);
    
    int initianNumberFrames=numberFrames;
    int mergedLinear=1;
    int mergedProgressive=0;
    
    QString firstNileName=inputFiles[0];
    firstNileName=firstNileName.left(firstNileName.indexOf(".KWS")).remove(Dir);
    
    if (firstNileName.contains("_frames_"))
    {
        QStringList list = firstNileName.split("_", Qt::SkipEmptyParts);
        
        
        initianNumberFrames=list[list.indexOf("frames")+1].toInt();
        
        if (firstNileName.contains("linear")) mergedLinear=list[list.indexOf("linear")+1].toInt();
        
        if (firstNileName.contains("progressive")) mergedProgressive=list[list.indexOf("progressive")+1].toInt();
        
    }
    
    QList<int> geometricalSplitting;
    if (geomerticalProgressionCalculation(int(initianNumberFrames/mergedLinear), mergedProgressive, geometricalSplitting) !=numberFrames)
    {
        QMessageBox::critical( 0, "qtiSAS", "... check RT data numbers ...");
        return;
    }

    for (int i = 0; i < inputFiles.count(); i++)
    {
        s = inputFiles[i];
        s = FilesManager::findFileNumberInFileName(wildCard, s.remove(Dir));

        tableDat->setColName(5 + 2 * i, "Sum-" + s);
        tableDat->setColName(5 + 2 * i + 1, "NormalizedSum-" + s);
        tableDat->setColComment(5 + 2 * i, "File: " + s);
        tableDat->setColComment(5 + 2 * i + 1, "File: " + s);
    }

    int currentSumOfFrames=0;
    int from, to, numberOfFrames;
    
    
    for (int i=0;i<numberFrames;i++)
    {
        numberOfFrames=geometricalSplitting[i]*mergedLinear;
        from=currentSumOfFrames+1;
        to=currentSumOfFrames+numberOfFrames;
        currentSumOfFrames+=numberOfFrames;
        
        tableDat->setText(i,4,QString::number(i+1));
        tableDat->setText(i,1,QString::number(from));
        tableDat->setText(i,2,QString::number(to));
        tableDat->setText(i,3,QString::number(numberOfFrames));
        tableDat->setText(i,0,QString::number((from+to)/2.0));
    }
    
    
    QStringList sMiddle;
    QString number;
    //
    std::cout<<"\n";
    for (int i=0; i<inputFiles.count(); i++)
    {
        //+++ Progress +++
        progress->setValue(i);
        progress->setLabelText("File # "+QString::number(i+1)+" of "+QString::number(inputFiles.count()));
        if ( progress->wasCanceled() ) break;

        number = inputFiles[i];
        number = number.remove(Dir);
        number = FilesManager::findFileNumberInFileName(wildCard, number);
        int numberRepetitions = monitors->readNumberRepetitions(number);

        //+++++++++++++++++
        // +++ Files ++++++
        //+++++++++++++++++
        std::cout << "[" << number.toLocal8Bit().constData() << "]: reading of sum's\n";
        std::cout << inputFiles[i].toLocal8Bit().constData() << std::flush;
        QFile file(inputFiles[i]);
        if (!file.open(QIODevice::ReadOnly)) return;

        
        
        
        for (int j=0;j<numberFrames;j++)
        {
            QByteArray data = file.read(DIM*DIM*sizeof(int));
            auto array = new int[DIM * DIM];
            memcpy(&array, data.constData(), data.size());

            std::cout << "   DIM:" << DIM << " numberFrames:" << numberFrames << " data.size:" << data.size() << "\n"
                      << std::flush;

            int sum = 0;

            for (int iii = 0; iii < DIM; iii++)
                for (int jjj = 0; jjj < DIM; jjj++)
                    sum += array[iii * DIM + jjj];
            delete[] array;

            int currentMergedFrames = tableDat->text(j, 3).toInt();
            tableDat->setText(j, 5 + 2 * i, QString::number(sum));
            tableDat->setText(
                j, 5 + 2 * i + 1,
                QString::number(double(sum) / double(currentMergedFrames) / double(numberRepetitions), 'f', 3));

            if (int((j + 1) / 10) * 10 == j + 1)
            {
                std::cout<<sum<<"["<<j+1<<"].";std::cout.flush();
                if (int((j+1)/100)*100==j+1) std::cout<<"\n";
            };
        }
        file.close();
    }
    progress->close();
}
//+++  TOF tools:: Split [slot]
void dan18::rtSplit()
{
    QString Dir = filesManager->pathInString();
    QString DirOut = filesManager->pathOutString();
    QString filter = textEditPattern->text(); // move to filesManager
    QString wildCard = filesManager->wildCardDetector();

    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Split  RT Frames"));
    fd->setNameFilter(filter+";;"+textEditPattern->text());
    foreach(QComboBox *obj, fd->findChildren<QComboBox *>())
        if (QString(obj->objectName()).contains("fileTypeCombo"))
            obj->setEditable(true);
    
    if (fd->exec() == QDialog::Rejected)
        return;
    
    //+++
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    //+++
    QStringList numberList;

    //+++ convert name to number
    for (int i = 0; i < filesNumber; i++)
    {
        QString s = selectedDat[i];
        s = FilesManager::findFileNumberInFileName(wildCard, s.remove(Dir));
        numberList << s;
    }

    QStringList header, lst;
    static const QRegularExpression rxF("(\\d+)");

    //+++ list of Modes
    for (int i = 0; i < filesNumber; i++)
    {
        //:::+ uni-tof
        int slicesCount = tofrt->readSlicesCount(numberList[i]);
        if (slicesCount > 1)
            lst << "RT Mode :: " + QString::number(slicesCount) + " frames";
        else
            lst << "Normal Mode :: 1 frame";
        //:::+- uni-tof
    }

    // select TOF-mode
    bool ok;
    QString res = QInputDialog::getItem(this, "qtiSAS :: DAN", "Select Reference for RT-mode:", lst, 0, true, &ok);
    if ( !ok ) return;
    
    // +++ number of frames
    int numberFrames = rxF.match(res).captured().toInt();
    
    //+++
    //    if (!res.contains("RT")) return;
    //    if (numberFrames<=1) return;
    
    //+++
    int filesNumberFinal=0;
    QStringList selectedDatFinal, newFilesNames;
    
    
    //+++
    for (int i=0; i<filesNumber; i++)
    {
        if (lst[i]==res)
        {
            selectedDatFinal<<selectedDat[i];
            newFilesNames<<DirOut+"rt_"+numberList[i];
            filesNumberFinal++;
        }
    }
    
    
    return rtSplit(numberFrames, selectedDatFinal, newFilesNames);
}



//*******************************************
//+++  TOF tools:: Split [function]
//*******************************************
void dan18::rtSplit(int numberFrames, QStringList inputFiles, QStringList outputFiles)
{
    QString Dir = filesManager->pathInString();
    QString wildCard = filesManager->wildCardDetector();

    //+++ Progress Dialog +++
    //QProgressDialog progress( "RT Spit", "Stop", inputFiles.count(), this,"File # 0 of "+QString::number(inputFiles.count()), true );
    QProgressDialog *progress= new QProgressDialog("RT :: Split","Stop", 0,inputFiles.count());
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
    
    
    QStringList sHeader1, sHeader2, sHeader3, sFinal, sMiddle;
    QString number;
    
    
    int initianNumberFrames=numberFrames;
    int mergedLinear=1;
    int mergedProgressive=0;
    
    QString firstNileName=inputFiles[0];
    firstNileName=firstNileName.remove(Dir).remove(".DAT");
    
    if (firstNileName.contains("_frames_"))
    {
        QStringList list = firstNileName.split("_", Qt::SkipEmptyParts);
        
        initianNumberFrames=list[list.indexOf("frames")+1].toInt();
        
        if (firstNileName.contains("linear")) mergedLinear=list[list.indexOf("linear")+1].toInt();
        
        if (firstNileName.contains("progressive")) mergedProgressive=list[list.indexOf("progressive")+1].toInt();
    }
    
    QList<int> geometricalSplitting;
    if (geomerticalProgressionCalculation(int(initianNumberFrames/mergedLinear), mergedProgressive, geometricalSplitting) !=numberFrames)
    {
        QMessageBox::critical( 0, "qtiSAS", "... check RT data numbers ...");
        return;
    }
	
    
    //
    for (int i=0; i<inputFiles.count(); i++)
    {
        
        //+++ Progress +++
        
        progress->setValue(i+1);
        progress->setLabelText("File # "+QString::number(i+1)+" of "+QString::number(inputFiles.count()));
        if ( progress->wasCanceled() ) break;

        number = inputFiles[i];
        number = number.remove(Dir);
        number = FilesManager::findFileNumberInFileName(wildCard, number);
        mainHeaderLength = lengthMainHeader(inputFiles[i]);

        //+++++++++++++++++
        // +++ Files ++++++
        //+++++++++++++++++
        QFile fileInput( inputFiles[i] );
        //+++
        if (!fileInput.open(QIODevice::ReadOnly))  break;
        QTextStream streamInput( &fileInput );
        
        QString s, tmp;
        //+++++++++++++++++
        // +++ Header +++++
        //+++++++++++++++++
        int indexInHeaderSum = parserHeader->listOfHeaders.indexOf("[Sum]");
        int beforeSum=tableHeaderPosNew->item(indexInHeaderSum,0)->text().toInt()-1;
        
        
        sHeader1.clear();
        // fefore: 2014-10-23;	for (int l=0; l<(mainHeaderLength-8); l++) sHeader1<<streamInput.readLine()+"\n";
        for (int ii=0; ii<beforeSum; ii++) sHeader1<<streamInput.readLine()+"\n";
        streamInput.readLine();
        sHeader2.clear();
        // fefore: 2014-10-23;	for (int l=0; l<6; l++) sHeader2<<streamInput.readLine()+"\n";
        // 2015 for (int l=0; l<(mainHeaderLength- beforeSum-2); l++) sHeader2<<streamInput.readLine()+"\n";
        for (int ii=0; ii<(mainHeaderLength - beforeSum-2); ii++) sHeader2<<streamInput.readLine()+"\n";
        streamInput.readLine();

        double RTfactor0 = monitors->readDuration(number) / tofrt->readSlicesDuration(number).toDouble();
        double RTfactor;

        int from, to;
        from=spinBoxRtSplitFrom->value();
        to=spinBoxRtSplitTo->value();
        
        for (int j=0;j<from-1;j++)
        {
            for (int ii=0; ii<tofHeaderBeforeLength; ii++) streamInput.readLine();
            for (int ii=0; ii<matrixInFileLength; ii++) streamInput.readLine();
            for (int ii=0; ii< tofHeaderAfterLength; ii++) streamInput.readLine();
        }
        
        //+++ 2017 new :: measured data in progressive mode
        int geometricalRatioInt=lineEditSplitFramesProgrMeasured->value();
        double ratioMeasured=1.0+geometricalRatioInt/1000.0;
        double progressiveFactorMeasured;
        //--- 2017 new :: measured data in progressive mode

        for (int j = from - 1; j < fmin(to, geometricalSplitting.count()); j++)
        {
            progressiveFactorMeasured=pow(ratioMeasured, j);
            
            if (!checkBoxRTsynchro->isChecked())  RTfactor=RTfactor0/geometricalSplitting[j]/mergedLinear/progressiveFactorMeasured;
            else RTfactor=numberFrames/geometricalSplitting[j]/mergedLinear/progressiveFactorMeasured;
            
            sHeader3.clear();
            sHeader3<<"\n(* Initial Slice Header *)\n\n";
            
            for (int ii=0; ii<tofHeaderBeforeLength; ii++)
            {
                sHeader3<<streamInput.readLine(); // sHeader3<<"\n";
            };
            
            sHeader3<<"\n";
            
            sMiddle.clear();
            for (int ii=0; ii<matrixInFileLength; ii++) sMiddle<<streamInput.readLine();
            int sum=tofSplitConvert128to8(sMiddle, DIM, matrixInFileLength, numberTofPerLine, numberTofPerLineNormal);
            
            for (int ii=0; ii< tofHeaderAfterLength; ii++) streamInput.readLine();
            
            
            sFinal.clear();
            sFinal=sHeader1;
            
            sFinal<<QString::number(sum, 'E', 6)+" 0.000000E+00 0.000000E+00 0.000000E+00 0.000000E+00 0.000000E+00\n";
            sFinal+=sHeader2;
            sFinal+=sHeader3;
            
            // 2015 RT
            sFinal<<"\n";
            sFinal<<"(* RT block *)\n\n";
            sFinal<<"RT-Merged-Frames= "+QString::number(geometricalSplitting[j]*mergedLinear)+"\n";
            sFinal<<"RT-Current-Number= "<<QString::number(j+1) <<"\n";
            sFinal<<"RT-Time-Factor= "<<QString::number(RTfactor) <<"\n\n";
            
            sFinal<<"$\n";
            
            sFinal+=sMiddle;
            
            if (j>=999) s="0";
            else if (j>=99) s="00";
            else if (j>=9) s="000";
            else s="0000";
            
            QString sss=outputFiles[i]+s+QString::number(j+1)+"_";
            
            if(pushButtonRTAll->isDefault() && checkBoxRtSuffix->isChecked() && lineEditRtSuffix->text()!="")  sss+=lineEditRtSuffix->text();
            else sss+="Split";
            sss+=".DAT";
            
            if (j>=from-1 && j<=to-1)
            {
                QFile fileOutput(sss);
                
                //+++
                if (!fileOutput.open(QIODevice::WriteOnly))  break;
                QTextStream streamOutput( &fileOutput );
                
                for (int i=0; i<sFinal.count(); i++) streamOutput<<sFinal[i];
                
                fileOutput.close();
            }
            //streamInput.readLine();
        }
        
        
        fileInput.close();
    }
    progress->close();
}

//*******************************************
//+++  RT tools:: All [slot]
//*******************************************
void dan18::rtAll(){
    if(!checkBoxAddRTtable->isChecked())    return rtAllselection();
}

void dan18::rtAllselection()
{
    QString Dir = filesManager->pathInString();
    QString DirOut = filesManager->pathOutString();
    QString filter = textEditPattern->text(); // move to filesManager
    QString wildCard = filesManager->wildCardDetector();

    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"*");
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Select RT files"));
    fd->setNameFilter(filter+";;"+textEditPattern->text());
    foreach(QComboBox *obj, fd->findChildren<QComboBox *>())
        if (QString(obj->objectName()).contains("fileTypeCombo"))
            obj->setEditable(true);
    
    if (fd->exec() == QDialog::Rejected)
        return;

    
    //+++
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    //+++
    QStringList numberList;

    //+++ convert name to number
    for (int i = 0; i < filesNumber; i++)
    {
        QString s = selectedDat[i];
        s = s.remove(Dir);
        s = FilesManager::findFileNumberInFileName(wildCard, s);
        numberList << s;
    }

    QStringList header, lst;
    static const QRegularExpression rxF("(\\d+)");

    //+++ list of Modes
    for (int i = 0; i < filesNumber; i++)
    {
        //:::+ uni-tof
        int slicesCount = tofrt->readSlicesCount(numberList[i]);
        lst << "RT Mode :: " + QString::number(slicesCount) + " frames";
        //:::+- uni-tof
    }

    // select TOF-mode
    bool ok;
    QString res = QInputDialog::getItem(this, "QtiSAS :: DAN-SANS", "Select RT mode:", lst, 0, true, &ok);
    if ( !ok ) return;
    
    // +++ number of frames
    int numberFrames = rxF.match(res).captured().toInt();
    
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

    bool rawData = true;
    QDir dd(Dir);

    //+++ merging
    int linearMerging=spinBoxMergeRT->value();
    int progresiveMerging=lineEditSplitFramesProgr->value();
    
    QString sTemp;
    int newNumberFrames=numberFrames;
    
    if (linearMerging>1 || progresiveMerging>0)
    {	
        //+++
	    for (int i=0; i<filesNumberFinal; i++) 
	    {
            sTemp = Dir;
            if (progresiveMerging==0)
            {
                sTemp+= "rt"+QString::number(linearMerging)+"_"+filesNumbers[i];
                sTemp+= QString::number(linearMerging)+"_frames_"+QString::number(numberFrames);
                sTemp+="_linear_"+QString::number(linearMerging);
            }
            else if (linearMerging==1)
            {
                sTemp+= "rt1"+QString::number(progresiveMerging) +"_"+filesNumbers[i];
                sTemp+=QString::number(progresiveMerging)+"_frames_"+QString::number(numberFrames);
                sTemp+="_linear_1_progressive_"+QString::number(progresiveMerging);		    
            }
            else
            {  
                sTemp+= "rt"+QString::number(linearMerging)+QString::number(progresiveMerging);
                sTemp+="_"+filesNumbers[i]+QString::number(linearMerging)+QString::number(progresiveMerging);
                sTemp+="_frames_"+QString::number(numberFrames);
                sTemp+="_linear_"+QString::number(linearMerging);
                sTemp+="_progressive_"+QString::number(progresiveMerging);		    		    
            }
            sTemp+=".DAT";
            
            newFilesNames<<sTemp;
        }
        //+++
        
        newNumberFrames=rtMerge(numberFrames, linearMerging, progresiveMerging, selectedDatFinal, newFilesNames);
        
        selectedDatFinal.clear();
        selectedDatFinal=newFilesNames;
        newFilesNames.clear();
        rawData=false;
    }    
    
    
    //+++
    QString s;
    
    for (int i=0; i<filesNumberFinal; i++)
    {
        s=DirOut;
        if (checkBoxRtPrefix->isChecked()) 
        {
            QString ss=lineEditRtPrefix->text();
            if(ss!="") s+=ss+"_";
            else s+="rt_";
        }
        else s+="rt_";
        
        s+=filesNumbers[i];
        
        //	if (!checkBoxTof12345->isChecked()) s+="12345";
        
        newFilesNames<<s;
    }
    
    //+++
    lineEditPathRAD->setText(DirOut);
    pushButtonRTAll->setDefault(true);
    
    rtSplit(newNumberFrames, selectedDatFinal, newFilesNames);
    
    if (checkBoxRtDelete->isChecked() && !rawData) for (int i=0; i<filesNumberFinal; i++) dd.remove ( selectedDatFinal[i]);
    pushButtonRTAll->setDefault(false);
}

void dan18::geometricalRatioPrefix(int value)
{
    if  (value<10) lineEditSplitFramesProgr->setPrefix("1.00");
    else if  (value<100) lineEditSplitFramesProgr->setPrefix("1.0");
    else lineEditSplitFramesProgr->setPrefix("1.");
}

void dan18::geometricalRatioPrefixMeasured(int value)
{
    if  (value<10) lineEditSplitFramesProgrMeasured->setPrefix("1.00");
    else if  (value<100) lineEditSplitFramesProgrMeasured->setPrefix("1.0");
    else lineEditSplitFramesProgrMeasured->setPrefix("1.");
}

int  dan18::geomerticalProgressionCalculation(int numberFrames, int geometricalRatio, QList<int> &geometricalSplitting)
{
    double ratio=1.0+geometricalRatio/1000.0;
    
    int currentnumberFrames;
    int n=0;
    int N=0;
    
    while ( n<numberFrames)
    {
        currentnumberFrames = int( pow (ratio, N) );
        n += currentnumberFrames;
        if (n>numberFrames)  currentnumberFrames -= ( n-numberFrames);
        geometricalSplitting <<  currentnumberFrames;
        N++;
    }
    
    return N;
}
//	+++  RT tools:: Add two Files
void dan18::addNfilesUniASCII(const QStringList &files, const QStringList &fileNumers, const QString &file)
{
    QStringList header;

    if (!addHeadersAscii(files, fileNumers, header))
        return;
    if (!addNmatrixesUni(fileNumers, header))
        return;

    QFile f(file);

    if (!f.open(QIODevice::WriteOnly))
        return;

    QTextStream stream(&f);

    for (int i = 0; i < header.count(); i++)
        stream << header[i] + "\n";

    f.close();
}
//*******************************************
//+++  RT tools:: Add Files Yaml
//*******************************************
void dan18::addNfilesYaml(QStringList files, QStringList fileNumers, QString file)
{
    if (!checkBoxYes2ndHeader->isChecked())
        return;
    int linesInHeader = spinBoxHeaderNumberLines->value();
    int linesInDataHeader = spinBoxDataHeaderNumberLines->value();
    QStringList header;

    readHeaderFile(files[0], linesInHeader + linesInDataHeader, header);

    if (radioButtonDetectorFormatImage->isChecked())
    {//+++ binary matrix

        bool readbleImage = file.contains(".jpg", Qt::CaseInsensitive) || file.contains(".bmp", Qt::CaseInsensitive) || file.contains(".pbm", Qt::CaseInsensitive) || file.contains(".pgm", Qt::CaseInsensitive);
        readbleImage = readbleImage || file.contains(".png", Qt::CaseInsensitive) || file.contains(".ppm", Qt::CaseInsensitive) || file.contains(".xbm", Qt::CaseInsensitive) || file.contains(".xpm", Qt::CaseInsensitive);
        readbleImage = readbleImage || file.contains(".tif", Qt::CaseInsensitive);
        if (readbleImage)
        {
            QMessageBox::critical( 0, "QtiSAS :: DAN-SANS", "... this binary format is not yet supported  ...");
            return;
        }
        else if (file.right(3)==".gz")
        {
            addGZippedMatrixes(fileNumers,file);
        }
        else
        {
            return;
        }
    }
    else
    {//+++ ascii matrix
        if (!addNmatrixesUni(fileNumers, header))
            return;

        QFile f(file);
        
        //+++
        if (!f.open(QIODevice::WriteOnly))  return;
        
        QTextStream stream( &f );
        
        for (int i=0; i<header.count(); i++)  stream << header[i]+"\n";
        
        f.close();
    }

    addNheadersYaml(fileNumers, file);
}
//*******************************************
//+++  RT tools:: Add Files HDF5
//*******************************************
void dan18::accumulateHDF5files(const QStringList &files, const QStringList &fileNumers, const QString &file)
{
    if (file.isEmpty() || files.isEmpty() || files[0].isEmpty())
    {
        std::cerr << "! Check file-names" << std::endl;
        return;
    }

    if (QFile::exists(file))
        QFile::remove(file);

    if (!QFile::copy(files[0], file))
    {
        std::cerr << "Failed to copy " << files[0].toStdString() << " to " << file.toStdString() << std::endl;
        return;
    }
    QString code;
    code = tableHeaderPosNew->item(int(parserHeader->listOfHeaders.indexOf("[Duration]")), 0)->text();
    if (code != "const" && !ParserHDF5::accumulateDatasets(file, files, code))
        return;
    code = tableHeaderPosNew->item(int(parserHeader->listOfHeaders.indexOf("[Sum]")), 0)->text();
    if (code != "const" && !ParserHDF5::accumulateDatasets(file, files, code))
        return;
    code = tableHeaderPosNew->item(int(parserHeader->listOfHeaders.indexOf("[Selector]")), 0)->text();
    if (code != "const" && !ParserHDF5::accumulateDatasets(file, files, code))
        return;
    code = tableHeaderPosNew->item(int(parserHeader->listOfHeaders.indexOf("[Monitor-1]")), 0)->text();
    if (code != "const" && !ParserHDF5::accumulateDatasets(file, files, code))
        return;
    code = tableHeaderPosNew->item(int(parserHeader->listOfHeaders.indexOf("[Monitor-2]")), 0)->text();
    if (code != "const" && !ParserHDF5::accumulateDatasets(file, files, code))
        return;
    code = tableHeaderPosNew->item(int(parserHeader->listOfHeaders.indexOf("[Monitor-3|Tr|ROI]")), 0)->text();
    if (code != "const" && !ParserHDF5::accumulateDatasets(file, files, code))
        return;
    code = lineEditHdfDetectorEntry->text();
    if (!ParserHDF5::accumulateDatasets(file, files, code))
        return;
}
//*******************************************
//+++  RT:: N Matrixes
//*******************************************
bool dan18::addNmatrixesUni(const QStringList &fileNumers, QStringList &header)
{
    int linesInHeader = spinBoxHeaderNumberLines->value();
    int linesInDataHeader = spinBoxDataHeaderNumberLines->value();

    int N = fileNumers.count();
    if (N < 1)
        return false;

    int MDinFile = comboBoxMDdata->currentText().toInt();
    int linesPerRaw = spinBoxReadMatrixNumberPerLine->value();
    
    gsl_matrix *data = gsl_matrix_calloc(MDinFile, MDinFile);
    gsl_matrix *dataTemp = gsl_matrix_calloc(MDinFile, MDinFile);
    
    for (int i = 0; i < N; i++)
    {
        readMatrixByNameFull(filesManager->fileNameFullDetector(fileNumers[i]), MDinFile, linesPerRaw, false,
                             linesInHeader + linesInDataHeader, false, false, dataTemp);
        gsl_matrix_add(data, dataTemp);
    }

    QString s = "";
    int curr = 0;
    
    for (int i = 0; i < MDinFile; i++)
        for (int j = 0; j < MDinFile; j++)
        {
            s += QString::number(gsl_matrix_get(data, i, j), 'f', 0);
            s += "   ";
            curr++;
            if (curr == linesPerRaw)
            {
                curr = 0;
                header << s;
                s = "";
            }
    }
    gsl_matrix_free(data);
    gsl_matrix_free(dataTemp);
    return true;
}
//*******************************************
//+++  addGZipped Matrixes :: 2021-05
//*******************************************
void dan18::addGZippedMatrixes(const QStringList &fileNumers, const QString &file)
{
    
    QString wildCaldLocal=lineEditWildCard->text();
    int DD=comboBoxMDdata->currentText().toInt();
    int INITIAL=DD*DD;
    int dimSmallest=INITIAL;

    gzFile *fd = new gzFile [fileNumers.count()];
    char* *buf = new char* [fileNumers.count()];
    uint32_t* *intbuf = new uint32_t* [fileNumers.count()];
    
    
    for (int i=0; i<fileNumers.count();i++)
    {
        //+++ open i-th file
        fd[i] = gzopen(filesManager->fileNameFull(fileNumers[i], wildCaldLocal).toLocal8Bit().constData(), "rb");

        //+++ allocation of i-th buffer
        buf[i]  =(char*)malloc(4*INITIAL);
        
        //+++ read i-th buffer
        int read = 0;
        int rest = 4*INITIAL;
        
        do {
            if (!rest)
            {
                buf[i] = (char*)realloc(buf[i], 2*read);
                rest = read;
            }
            int neww = gzread(fd[i], &buf[i][read], rest);

            rest -= neww;
            read += neww;
        } while (!gzeof(fd[i]));
     
        //+++ char/int buffer conversion
        intbuf[i] = (uint32_t *)buf[i];

        if (read/4.0<dimSmallest && read>0) dimSmallest=read/4.0;

        //+++ close i-th file
        gzclose(fd[i]);
    }
    
    int sum=0;
    //+++ adding all data to the first buffer
    for (int iii=0; iii<dimSmallest;iii++)for (int i=1; i<fileNumers.count();i++) intbuf[0][iii]=intbuf[0][iii]+intbuf[i][iii];

    //+++ converting int buffer to char
    char *gz_buffer = (char *)intbuf[0];
    
    //+++ saving
    gzFile fdw = gzopen(file.toLocal8Bit().constData(), "wb");
    gzwrite(fdw,gz_buffer,dimSmallest*4);
    gzclose(fdw);

    free(fd);
    free(buf);
    free(intbuf);
    free(gz_buffer);
}
//+++  Add  Headers: ASCII
bool dan18::addHeadersAscii(const QStringList &files, const QStringList &fileNumers, QStringList &newHeader)
{
    int linesInHeader = spinBoxHeaderNumberLines->value();
    int linesInDataHeader = spinBoxDataHeaderNumberLines->value();

    QString filesNumberString = "added files: " + fileNumers[0];
    for (int i = 1; i < fileNumers.count(); i++)
        filesNumberString += ", " + fileNumers[i];

    int N = files.count();
    if (N < 1)
        return false;

    if (!parserHeader->readHeader(files[0], linesInHeader + linesInDataHeader, newHeader))
        return false;

    QString newComment = sample->readComment1(fileNumers[0], newHeader) + "; " + filesNumberString;
    if (newComment.left(3).contains("; "))
        newComment = newComment.remove("; ");

    parserHeader->replaceHeaderValue(fileNumers[0], "[Comment1]", newHeader, newComment);

    // +++  added 6 parameters
    double duration = 0.0;
    double sum = 0.0;
    double selectorFrequency = 0.0;
    double monitor1 = 0.0;
    double monitor2 = 0;
    double monitor3 = 0;
    for (int i = 0; i < N; i++)
    {
        duration += monitors->readDuration(fileNumers[i]);
        sum += monitors->readSum(fileNumers[i]);
        selectorFrequency += selector->readFrequencylikeInHeader(fileNumers[i]);
        monitor1 += monitors->readMonitor1(fileNumers[i], 0.0);
        monitor2 += monitors->readMonitor2(fileNumers[i], 0.0);
        monitor3 += monitors->readMonitor3(fileNumers[i], 0.0);
    }
    // +++ duration
    duration /= monitors->unitsConverterDurationToSec(1.0);
    parserHeader->replaceHeaderValue(fileNumers[0], "[Duration]", newHeader, QString::number(duration));
    // +++ sum
    parserHeader->replaceHeaderValue(fileNumers[0], "[Sum]", newHeader, QString::number(sum));
    // +++ selector
    parserHeader->replaceHeaderValue(fileNumers[0], "[Selector]", newHeader, QString::number(selectorFrequency));
    // +++ Monitor1
    parserHeader->replaceHeaderValue(fileNumers[0], "[Monitor-1]", newHeader, QString::number(monitor1));
    // +++ Monitor2
    parserHeader->replaceHeaderValue(fileNumers[0], "[Monitor-2]", newHeader, QString::number(monitor2));
    // +++ Monitor3
    parserHeader->replaceHeaderValue(fileNumers[0], "[Monitor-3|Tr|ROI]", newHeader, QString::number(monitor3));

    return true;
}
//+++ Add  Headers: Yaml
bool dan18::addNheadersYaml(const QStringList &fileNumers, const QString &fileName)
{
    QString filesNumberString = "added files: " + fileNumers[0];
    for (int i = 1; i < fileNumers.count(); i++)
        filesNumberString += ", " + fileNumers[i];

    int N = fileNumers.count();
    if (N < 1)
        return false;

    QString firstFile = fileNumers[0];
    firstFile = filesManager->fileNameFullHeader(firstFile);

    if (fileName == "")
        return false;

    QString newComment = sample->readComment1(fileNumers[0]) + "; " + filesNumberString;
    if (newComment.left(3).contains("; "))
        newComment = newComment.remove("; ");

    QFile originalFile(firstFile), newFile(fileName);

    originalFile.open(QIODevice::ReadOnly);
    newFile.open(QIODevice::WriteOnly);

    QTextStream instream(&originalFile);
    QTextStream outstream(&newFile);
    while (!instream.atEnd())
    {
        QString line = instream.readLine();
        outstream << line << "\n";
    }
    originalFile.close();
    newFile.close();

    parserHeader->replaceEntryYaml(fileName, "[Comment1]", false, newComment);

    bool selectorNumber = true;
    if (comboBoxUnitsSelector->currentIndex() > 0)
        selectorNumber = false;

    // +++  added 6 parameters
    double duration = 0.0;
    double sum = 0.0;
    double selectorFrequency = 0.0;
    double monitor1 = 0.0;
    double monitor2 = 0;
    double monitor3 = 0;
    for (int i = 0; i < N; i++)
    {
        duration += monitors->readDuration(fileNumers[i]);
        sum += monitors->readSum(fileNumers[i]);
        if (selectorNumber)
            selectorFrequency += selector->readFrequencylikeInHeader(fileNumers[i]);
        else
            selectorFrequency +=
                selector->readFrequencylikeInHeader(fileNumers[i]) * monitors->readDuration(fileNumers[i]);
        monitor1 += monitors->readMonitor1(fileNumers[i], 0.0);
        monitor2 += monitors->readMonitor2(fileNumers[i], 0.0);
        monitor3 += monitors->readMonitor3(fileNumers[i], 0.0);
    }

    if (!selectorNumber)
        selectorFrequency /= duration;

    parserHeader->replaceEntryYaml(fileName, "[Sum]", true, QString::number(sum));
    parserHeader->replaceEntryYaml(fileName, "[Duration]", true, QString::number(duration));
    parserHeader->replaceEntryYaml(fileName, "[Monitor-1]", true, QString::number(monitor1));
    parserHeader->replaceEntryYaml(fileName, "[Monitor-2]", true, QString::number(monitor2));
    parserHeader->replaceEntryYaml(fileName, "[Monitor-3|Tr|ROI]", true, QString::number(monitor3));
    parserHeader->replaceEntryYaml(fileName, "[Selector]", true, QString::number(selectorFrequency));

    return true;
}

