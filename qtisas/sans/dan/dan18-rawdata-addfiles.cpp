/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS add raw data
 ******************************************************************************/

#include "dan18.h"
#include "parser-edf.h"

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
//+++ remove common wild-cards from two strings
bool removeCommonWildCards(QString &wc1, QString &wc2)
{
    // --- Common prefix ---
    int prefix = 0;
    int len = static_cast<int>(std::min(wc1.size(), wc2.size()));
    while (prefix < len && wc1[prefix] == wc2[prefix])
        ++prefix;

    // --- Common suffix ---
    int suffix = 0;
    int aLen = static_cast<int>(wc1.size()) - prefix;
    int bLen = static_cast<int>(wc2.size()) - prefix;

    while (suffix < aLen && suffix < bLen && wc1[wc1.size() - 1 - suffix] == wc2[wc2.size() - 1 - suffix])
    {
        ++suffix;
    }

    // Nothing to remove?
    if (prefix == 0 && suffix == 0)
        return false;

    // --- Extract difference ---
    wc1 = wc1.mid(prefix, wc1.size() - prefix - suffix);
    wc2 = wc2.mid(prefix, wc2.size() - prefix - suffix);

    return true;
}
//+++  Uni:: Add Several Files
void dan18::addSeveralFilesUniSingleFrame(QStringList selectedFileList, QStringList selectedNumberList, QString fileName)
{
    if (checkBoxYes2ndHeader->isChecked() && comboBoxHeaderFormat->currentIndex() == 2)
    {
        if (radioButtonDetectorFormatAscii->isChecked())
            addNfilesUniASCII(selectedFileList, selectedNumberList, fileName);

        // the header file: the first run's, its number replaced by the new one, next to the new detector file
        // (the patterns' difference appended "_HRD_standard" twice: "#_*.det" vs "#_*_HRD_standard.yaml")
        const QString number0 = selectedNumberList[0];
        const QString header0 = QFileInfo(filesManager->fileNameFullHeader(selectedNumberList[0])).fileName();
        const QString detector0 = QFileInfo(selectedFileList[0]).fileName();
        const QString detectorNew = QFileInfo(fileName).fileName();
        if (header0.startsWith(number0) && detector0.startsWith(number0) &&
            detectorNew.endsWith(detector0.mid(number0.length())))
        {
            const QString numberNew = detectorNew.left(detectorNew.length() - detector0.length() + number0.length());
            addNheadersYaml(selectedNumberList,
                            QFileInfo(fileName).dir().filePath(numberNew + header0.mid(number0.length())));
            return;
        }

        QString wcDetector = filesManager->wildCardDetector();
        QString wcHeader = filesManager->wildCardHeader();

        if (removeCommonWildCards(wcDetector, wcHeader))
        {
            fileName.replace(QRegularExpression(QRegularExpression::escape(wcDetector) + "$"), wcHeader);
            addNheadersYaml(selectedNumberList, fileName);
        }
        return;
    }

    if (fileName.contains(".edf", Qt::CaseInsensitive))
    {
        if (!ParserEDF::accumulateFiles(fileName, selectedFileList))
            return;

        if (comboBoxHeaderFormat->currentIndex() == 0)
        {
            QStringList header;
            if (!addHeadersAscii(selectedFileList, selectedNumberList, header))
                return;
            ParserEDF::replaceHeader(fileName, header);
        }
        else
            QMessageBox::critical(this, "QtiSAS :: DAN-SANS",
                                  "... adding EDF files with a non-ASCII header is not yet supported ...");

        return;
    }
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
    if (selectedNumberList.isEmpty())
        return;

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
    if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "Table")
        return;
    addFilesInTableCols((Table *)app()->activeWindow());
}
//+++ per column: the first row the new run's number, the rows below the runs added into it
void dan18::addFilesInTableCols(Table *t)
{
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
            fileNumber = tableTextC(t, 0, nn);
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
            file2add = tableTextC(t, mm, nn);
            file2add = file2add.simplified();
            if (file2add == "")
                continue;

            if (!filesManager->checkFileNumber(file2add))
                continue;

            selectedNumberList << file2add;
            selectedFiles << filesManager->fileNameFull(file2add, wildCard);
        }

        // "Skip single runs": a column of one run has nothing to add (unchecked: copied under the new number)
        if (selectedNumberList.count() < 2 && checkBoxAddSkipSingle->isChecked())
            continue;
        if (toolBoxAdv->currentIndex() == 0)
            addSeveralFilesUniSingleFrame(selectedNumberList, fileNumber);
        else if (toolBoxAdv->currentIndex() == 1)
        {
            // the output folder + "rt0_<new number>_added-N.DAT" (the number was overwritten by the folder before)
            QString file = lineEditPathRAD->text() + "rt0_" + fileNumber + "_added-" +
                           QString::number(selectedNumberList.count()) + ".DAT";
            tofrtAddFiles(selectedFiles, file);
        }
        else if (toolBoxAdv->currentIndex() == 2)
        {
            // tofrtAddFiles() takes the files (it was given the run numbers)
            QString file = lineEditPathRAD->text() + "tof0_" + fileNumber + "_added-" +
                           QString::number(selectedFiles.count()) + ".DAT";
            tofrtAddFiles(selectedFiles, file);
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
    addFilesInTableRows((Table *)app()->activeWindow());
}
//+++ per row: the first column the new run's number, the columns right of it the runs added into it (a row with an
//+++ empty first column continues the row above)
void dan18::addFilesInTableRows(Table *t)
{
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
            fileNumber = tableTextC(t, rr, 0);
            fileNumber = fileNumber.simplified();

            selectedNumberList.clear();
            selectedFiles.clear();

            if (fileNumber.isEmpty())
                continue;
        }
        else if (fileNumber.isEmpty())
            continue;

        for (int cc = 1; cc < C; cc++)
        {
            file2add = tableTextC(t, rr, cc);
            file2add = file2add.simplified();

            if (file2add.isEmpty())
                continue;

            if (!filesManager->checkFileNumber(file2add))
                continue;
            selectedNumberList << file2add;
            selectedFiles << filesManager->fileNameFull(file2add, wildCard);
        }

        if ((rr == R - 1 || !t->text(rr + 1, 0).isEmpty()) &&
            !(selectedNumberList.count() < 2 && checkBoxAddSkipSingle->isChecked()))
            // named as by columns: the first run's file name, its number replaced (a pattern with "*" has no new
            // file name of its own)
            addSeveralFilesUniSingleFrame(selectedNumberList, fileNumber);
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

//+++  Merging Table Generation [function]: the selected runs of the same sample and configuration, a column each, a
//+++  single run too (as Dan.Rawdata.newAddingTemplate(): DAN's configuration rule on the runs' headers, so no table
//+++  of configurations is needed; before, an empty one put all runs of a sample, of any configuration, into one
//+++  column)
QString dan18::generateTemplateToAddeFiles(const QStringList &selectedNumbers)
{
    const QString name = danRawdata->newAddingTemplate(selectedNumbers, 1);
    if (name.isEmpty() && !m_scriptRun)
        QMessageBox::warning(this, "QtiSAS", danRawdata->lastError());
    return name;
}
