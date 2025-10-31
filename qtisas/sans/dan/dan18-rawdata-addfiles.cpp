/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS add raw data
 ******************************************************************************/

#include "dan18.h"

//*******************************************
//+++  RT tools:: Sum [slot]
//*******************************************
void dan18::addfilesConnectSlots()
{
    connect( pushButtonAddUni , SIGNAL( clicked() ), this, SLOT( addSeveralFilesUniSingleFrame() ) );
    connect( pushButtonGenerateAddingTable  , SIGNAL( clicked() ), this, SLOT( generateTableToAdd()) );
    connect(pushButtonAddUniInTable, &QToolButton::clicked, this, &dan18::addFilesInActiveTableCols);
    connect(pushButtonAddingCols, &QToolButton::clicked, this, &dan18::addFilesInActiveTableRows);
}

//*******************************************
//+++  Uni:: Add Several Files [slot]
//*******************************************
void dan18::addSeveralFilesUniSingleFrame()
{
    QString Dir = filesManager->pathInString();
    QString DirOut = filesManager->pathOutString();
    QString filter = textEditPattern->text(); // move to filesManager
    QString wildCard = filesManager->wildCardDetector();
    
    //+++ select files
    auto *fd = new QFileDialog(this, "DAN-SANS :: Add Several (Single) Files", Dir, filter);
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles );
    fd->setWindowTitle(tr("DAN-SANS :: Add Several (Single) Files"));
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (fd->exec() == QDialog::Rejected)
        return;
    
    QStringList selectedDat=fd->selectedFiles();
    
    int filesNumber= selectedDat.count();
    
    
    if (!selectedDat[0].contains(Dir))
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
        QString nameFile = FilesManager::findFileNumberInFileName(wildCard, sss.remove(Dir));
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
    sss = sss.replace(Dir, DirOut);
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
    if (comboBoxHeaderFormat->currentIndex() == 0)
        return addNfilesUniASCII(selectedFileList, selectedNumberList, fileName);
    if (comboBoxHeaderFormat->currentIndex() == 2)
        return addNfilesYaml(selectedFileList, selectedNumberList, fileName);
    if (comboBoxHeaderFormat->currentIndex() == 3)
        return accumulateHDF5files(selectedFileList, selectedNumberList, fileName);
}

//*******************************************
//+++  Uni:: Add Several Files [function]
//*******************************************
void dan18::addSeveralFilesUniSingleFrame(QStringList selectedNumberList, QString fileNumber)
{
    QString Dir = filesManager->pathInString();

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
//+++  readTableToAddCols
//*******************************************
void dan18::addFilesInActiveTableCols()
{
    if (!app()->activeWindow())
        return;

    auto *t = (Table *)app()->activeWindow();
    if (!t)
        return;

    QString wildCard = filesManager->wildCardDetector();

    int N = t->numCols();
    int M = t->numRows();

    QString fileNumber, file2add;
    QStringList selectedNumberList;
    QStringList selectedFiles;

    for (int nn = 0; nn < N; nn++)
    {
        if (!t->text(0, nn).isEmpty())
        {
            fileNumber = t->text(0, nn);
            fileNumber = fileNumber.simplified();
            fileNumber = fileNumber.replace(" ", "-").replace("/", "-").replace(",", "-").replace(".", "-").remove("%");

            selectedNumberList.clear();
            selectedFiles.clear();

            if (fileNumber.isEmpty())
                continue;
        }
        else if (fileNumber.isEmpty())
            continue;

        for (int mm = 1; mm < M; mm++)
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

        if (toolBoxAdv->currentIndex() == 0)
            addSeveralFilesUniSingleFrame(selectedNumberList, fileNumber);
        else if (toolBoxAdv->currentIndex() == 1)
        {
            fileNumber = lineEditPathRAD->text();
            fileNumber += "rt0_" + fileNumber + "_added-" + QString::number(selectedNumberList.count()) + ".DAT";
            tofrtAddFiles(selectedFiles,fileNumber);
        }
        else if (toolBoxAdv->currentIndex() == 2)
        {
            fileNumber = lineEditPathRAD->text();
            fileNumber += "tof0_" + fileNumber + "_added-" + QString::number(selectedFiles.count()) + ".DAT";
            tofrtAddFiles(selectedNumberList,fileNumber);
        }
    }
}
//*******************************************
//+++  Uni:: addFilesInActiveTableRow
//*******************************************
void dan18::addFilesInActiveTableRows()
{
    if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "Table")
        return;

    auto t = (Table *)app()->activeWindow();

    QString wildCard = filesManager->wildCardDetector();

    int R = t->numRows();
    int C = t->numCols();

    QString fileNumber, file2add;
    QStringList selectedNumberList;
    QStringList selectedFiles;

    //+++ Set Data-Sets List +++
    for (int rr = 0; rr < R; rr++)
    {
        if (!t->text(rr, 0).isEmpty())
        {
            fileNumber = t->text(rr, 0);
            fileNumber = fileNumber.simplified();
            fileNumber = filesManager->newFileNameFull(fileNumber, wildCard);

            selectedNumberList.clear();
            selectedFiles.clear();

            if (fileNumber.isEmpty())
                continue;
        }
        else if (fileNumber.isEmpty())
            continue;

        for (int cc = 1; cc < C; cc++)
        {
            file2add = t->text(rr, cc);
            file2add = file2add.simplified();

            if (file2add.isEmpty())
                continue;

            if (!filesManager->checkFileNumber(file2add))
                continue;
            selectedNumberList << file2add;
            selectedFiles << filesManager->fileNameFull(file2add, wildCard);
        }

        if (rr == R - 1 || !t->text(rr + 1, 0).isEmpty())
            addSeveralFilesUniSingleFrame(selectedFiles, selectedNumberList, fileNumber);
    }
}
//*******************************************
//+++  Uni:: Add Several Files [slot]
//*******************************************
void dan18::generateTableToAdd()
{
    QString Dir = filesManager->pathInString();
    QString DirOut = filesManager->pathOutString();
    QString filter = textEditPattern->text(); // move to filesManager
    QString wildCard = filesManager->wildCardDetector();

    //+++ select files
    auto *fd = new QFileDialog(this, "DAN-SANS :: Generate ADDING table template", Dir, filter);
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles );
    fd->setWindowTitle(tr("DAN-SANS :: Generate ADDING table template"));
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (fd->exec() == QDialog::Rejected)
        return;
    
    QStringList selectedDat=fd->selectedFiles();
    
    int filesNumber= selectedDat.count();
    
    if (!selectedDat[0].contains(Dir))
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
        QString nameFile = FilesManager::findFileNumberInFileName(wildCard, sss.remove(Dir));
        numberList << nameFile;
    }
     generateTemplateToAddeFiles(numberList);
}

//+++  Merging Table Generation [function]
bool dan18::generateTemplateToAddeFiles(QStringList selectedNumbers)
{
    QString name = "adding-template-";
    name = app()->generateUniqueName(name);
    Table *t = app()->newTable(name, 3, selectedNumbers.count());

    t->setWindowLabel("DAN::Adding::Template");
    app()->setListViewLabel(t->name(), "DAN::Adding::Template");
    app()->updateWindowLists(t);

    QStringList selectedNumbersNames;
    int *skip = new int[selectedNumbers.count()];

    for (int i=0; i<selectedNumbers.count();i++ )
    {
        selectedNumbersNames<<configurationPlusSampleName(selectedNumbers[i]);
        skip[i]=0;
    }
    
    QString currentNumberName;
    QStringList currentSelectedNumbers;

    int maxNumberRaws = 3;
    int maxNumberCols = 0;

    for (int i = 0; i < selectedNumbers.count(); i++)
    {
        currentSelectedNumbers.clear();
        
        currentNumberName=selectedNumbersNames[i];
        
        if (currentNumberName == "")
            continue;
        if (skip[i] == 1)
            continue;

        for (int ii = i; ii < selectedNumbers.count(); ii++)
        {
            if (skip[ii] == 1)
                continue;

            if (selectedNumbersNames[ii] == currentNumberName)
            {
                currentSelectedNumbers << selectedNumbers[ii];
                skip[ii]=1;
            }
        }

        if (currentSelectedNumbers.count() < 1)
            continue;
        if (maxNumberRaws < currentSelectedNumbers.count() + 1)
            maxNumberRaws = currentSelectedNumbers.count() + 1;

        t->setNumRows(maxNumberRaws);
        
        maxNumberCols++;
        
        QString currentLabel;

        if (currentSelectedNumbers.count() > 999)
            currentLabel = "0";
        else if (currentSelectedNumbers.count() > 99)
            currentLabel = "00";
        else if (currentSelectedNumbers.count() > 9)
            currentLabel = "000";
        else
            currentLabel = "0000";

        currentLabel = currentSelectedNumbers[0] + currentLabel + QString::number(currentSelectedNumbers.count());

        t->setColName(maxNumberCols - 1, currentNumberName);
        t->setColPlotDesignation(maxNumberCols - 1, Table::None);
        t->setColumnType(maxNumberCols - 1, Table::ColType::Text);
        t->setText(0, maxNumberCols - 1, currentLabel);

        for (int ii = 0; ii < currentSelectedNumbers.count(); ii++)
            t->setText(ii + 1, maxNumberCols - 1, currentSelectedNumbers[ii]);
    }
    
    //+++ to minimize number of columns in the table
    t->setNumCols(maxNumberCols);  
    
    //+++ adjust cols
    t->adjustColumnsWidth(false);
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
    lambda = selector->readLambda(Number, monitors->readDuration(Number, lst), lst);
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
