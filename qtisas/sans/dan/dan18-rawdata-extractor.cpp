/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS data analysis interface
 ******************************************************************************/

#include <QDirIterator>
#include "dan18.h"
#include "qnaturalsortlist.h"

//*******************************************
//+++  2017: Extractor ...
//*******************************************
void dan18::extractorConnectSlots()
{
    connect( pushButtonNewInfoExtractor, SIGNAL( clicked() ), this, SLOT( newInfoExtractor() ) );
    connect( pushButtonMakeListExtractor, SIGNAL( clicked() ), this, SLOT( addToInfoExtractor() ) );
    connect( pushButtonExtractorAddCol, SIGNAL( clicked() ), this, SLOT( addColToInfoExtractor() ) );
    connect(lineEditCheckExtractorTableName, &QLineEdit::returnPressed, this, &dan18::infoExtractorSlot);
}

QString dan18::getHeaderInfoString(QString number, QString name)
{
    if (!filesManager->checkFileNumber(number))
        return "";

    if (name == "Sample")
        return sample->readName(number);
    if (name == "C")
        return QString::number(int(collimation->readCinM(number)));
    if (name == "D")
        return QString::number(detector->readDinM(number));
    if (name == "lambda")
        return QString::number(selector->readLambda(number, monitors->readDuration(number)), 'f', 4);
    if (name == "Sum")
        return QString::number(monitors->readSum(number));
    if (name == "Duration")
        return QString::number(monitors->readDuration(number));
    if (name == "cps")
        return QString::number(monitors->readSum(number) / monitors->readDuration(number), 'f', 5);
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
    if (name == "File-Names")
    {
        QString sub = "";
        return filesManager->fileName(number, filesManager->wildCardDetector(), sub);
    }
    if (name == "Monitor-1")
        return QString::number(monitors->readMonitor1(number));
    if (name == "Monitor-2")
        return QString::number(monitors->readMonitor2(number));
    if (name.contains("Monitor-3"))
        return QString::number(monitors->readMonitor3(number));
    if (name == "Polarization")
        return collimation->readPolarization(number);
    if (name == "Polarizer-Polarization")
        return QString::number(
            polarizationSelector->getValue(selector->readLambda(number, monitors->readDuration(number)), number));
    if (name == "Polarizer-Transmission")
        return QString::number(
            polTransmissionSelector->getValue(selector->readLambda(number, monitors->readDuration(number)), number));
    if (name == "Polarizer-Flipper-Efficiency")
        return QString::number(polFlipperEfficiencySelector->getValue(
            selector->readLambda(number, monitors->readDuration(number)), number));
    if (name == "Analyzer-Transmission")
        return QString::number(analyzerTransmissionSelector->getValue(
            selector->readLambda(number, monitors->readDuration(number)), number));
    if (name == "Analyzer-Efficiency")
        return QString::number(
            analyzerEfficiencySelector->getValue(selector->readLambda(number, monitors->readDuration(number)), number));

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

void dan18::infoExtractorSlot()
{
    QString tableName, columnNumberList, runsRange;
    QStringList lstWithName = lineEditCheckExtractorTableName->text().split(":");

    if (lstWithName.count() < 2)
        columnNumberList = lstWithName.value(0).trimmed();
    else
    {
        tableName = lstWithName.value(0).trimmed();
        columnNumberList = lstWithName.value(1).trimmed();
    }

    if (lstWithName.count() > 2)
        runsRange = lstWithName.value(2).trimmed();

    if (!infoExtractorScript(tableName, columnNumberList, runsRange))
        QMessageBox::warning(this, "QtiSAS",
                             tr("Invalid input string: \"%1\"").arg(lineEditCheckExtractorTableName->text()));
}

bool dan18::infoExtractorScript(QString tableName, const QString &columnNumberList, const QString &runsRange)
{
    if (tableName.isEmpty() && runsRange.isEmpty() && columnNumberList.isEmpty())
        return false;

    tableName = tableName.remove(QRegularExpression("[^A-Za-z0-9_-]"));

    if (!tableName.isEmpty())
    {
        int tableIndex = comboBoxInfoExtractor->findText(tableName);
        if (tableIndex >= 0)
            comboBoxInfoExtractor->setCurrentIndex(tableIndex);
        else
            newInfoExtractor(tableName);
    }
    else if (comboBoxInfoExtractor->count() == 0)
        return false;

    QStringList lst = columnNumberList.split(",", Qt::SkipEmptyParts);
    for (QString &s : lst)
    {
        int index = s.trimmed().toInt();
        if (index < 1 || index > comboBoxInfoExtractorCol->count())
            continue;
        comboBoxInfoExtractorCol->setCurrentIndex(index - 1);
        addColToInfoExtractor();
    }

    if (!runsRange.isEmpty())
    {
        QStringList range = runsRange.split("..", Qt::SkipEmptyParts);
        if (range.count() != 2)
            return false;

        QString start = range[0].trimmed();
        QString end = range[1].trimmed();

        QNaturalSortList fileList;
        QDirIterator it(filesManager->pathInString(), QStringList() << filesManager->wildCardInString(), QDir::Files);
        while (it.hasNext())
            fileList << it.next();
        fileList.sortNatural();

        qsizetype from = fileList.indexOf(filesManager->fileNameFullDetector(start));
        qsizetype to = fileList.indexOf(filesManager->fileNameFullDetector(end));

        if (from < 0 || to < 0 || from >= fileList.count() || to >= fileList.count())
            return false;

        if (from > to)
            std::swap(from, to);

        QStringList subList = fileList.mid(from, to - from + 1);
        addToInfoExtractor(subList, false);
    }

    return true;
}

void dan18::addColToInfoExtractor()
{
    QString activeTable = comboBoxInfoExtractor->currentText();

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

    int startRaw=0;

    if (!app()->checkTableExistence(TableName))
        return;
    
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

    QStringList lstTextType{
        "Sample",       "Beam",   "Date",      "Time",           "Comment1",       "Comment2",          "Name",
        "Who",          "Files",  "RT-number", "RT-Time-Factor", "RT-Repetitions", "RT-Frame-Duration", "Attenuator",
        "Polarization", "Lenses", "Runs",      "File-Names"};

    QStringList lstNumericType{"C",
                               "D",
                               "lambda",
                               "Offset",
                               "Sample-Nr",
                               "Thickness",
                               "Time-Factor",
                               "Field-1",
                               "Field-2",
                               "Field-3",
                               "Field-4",
                               "X-Position",
                               "Y-Position",
                               "Beamwindow-X",
                               "Beamwindow-Y",
                               "BeamWin-Xs",
                               "BeamWin-Ys",
                               "BeamWin-X-Pos",
                               "BeamWin-Y-Pos",
                               "Polarizer-Polarization",
                               "Polarizer-Transmission",
                               "Polarizer-Flipper-Efficiency",
                               "Analyzer-Transmission",
                               "Analyzer-Efficiency"};

    if (lstTextType.contains(what2add))
    {
        tableDat->setColPlotDesignation(initNumberCols, Table::None);
        tableDat->setColumnType(initNumberCols, Table::Text);
    }
    else if (lstNumericType.contains(what2add))
    {
        ;
        tableDat->setColumnType(initNumberCols, Table::Numeric);
    }
    else if (what2add.contains("Motor-"))
    {
        tableDat->setColPlotDesignation(initNumberCols, Table::X);
        tableDat->setColumnType(initNumberCols, Table::Numeric);
    }
    else
    {
        tableDat->setColPlotDesignation(initNumberCols, Table::Y);
        tableDat->setColumnType(initNumberCols, Table::Numeric);
    }

    if (what2add == "Monitor-3")
        what2add = "Monitor-3|Tr|ROI";

    for(int iter=0; iter<tableDat->numRows();iter++) tableDat->setText(iter,initNumberCols,getHeaderInfoString(tableDat->text(iter,0), what2add));

    tableDat->table()->resizeColumnToContents(initNumberCols);

    app()->maximizeWindow(TableName);
}

void dan18::addToInfoExtractor()
{
    QString Dir = filesManager->pathInString();
    QString wildCard = filesManager->wildCardDetector();
    bool dirsInDir = filesManager->subFoldersYN();

    QString activeTable = comboBoxInfoExtractor->currentText();

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
    
    QString wildCardLocal = wildCard;
    
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
    
    if (fd->exec() == QDialog::Rejected)
        return;
    
    addToInfoExtractor(fd->selectedFiles());
}

void errorMsg(const QString &info, bool viaButton)
{
    if (viaButton)
        QMessageBox::critical(nullptr, "QtiSAS", info);
    else
        qDebug() << "InfoExtractor: " << info;
}

void dan18::addToInfoExtractor(const QStringList &selectedDat, bool viaButton)
{
    QString Dir = filesManager->pathInString();
    QString wildCard = filesManager->wildCardDetector();
    bool dirsInDir = filesManager->subFoldersYN();
    QString TableName = comboBoxInfoExtractor->currentText();

    Table *tableDat;

    int filesNumber = int(selectedDat.count());

    if (filesNumber == 0)
        return errorMsg("InfoExtractor: Nothing was selected", viaButton);

    QString test = selectedDat[0];
    if (test.contains(Dir))
    {
        test = test.remove(Dir);
        if (!dirsInDir && test.contains("/"))
            return errorMsg("Selected data not in ::Input Folder::", viaButton);
        else if (dirsInDir && test.count("/") > 1)
            return errorMsg("Selected data not in ::Input Folder:: + sub-folders", viaButton);
    }
    else
        return errorMsg("Selected data not in ::Input Folder:: + sub-folders", viaButton);

    int startRaw = 0;

    if (checkBoxSortOutputToFolders->isChecked())
        app()->changeFolder("DAN :: script, info, ...");

    if (app()->checkTableExistence(TableName))
    {
        foreach (MdiSubWindow *t, app()->tableList())
            if (t->name() == TableName)
                tableDat = (Table *)t;

        if (tableDat->numCols() < 1)
            return errorMsg("Create new table (# cols)", viaButton);

        startRaw = tableDat->numRows();
    }
    else
    {
        tableDat = app()->newTable(TableName, filesNumber, 1);

        tableDat->setWindowLabel("Info::Extractor");
        app()->setListViewLabel(tableDat->name(), "Info::Extractor");
        app()->updateWindowLists(tableDat);

        tableDat->setColName(0, "Runs");
        tableDat->setColPlotDesignation(0, Table::X);
        tableDat->setColumnType(0, Table::Text);
    }
    
    tableDat->setNumRows(startRaw + filesNumber);
    
    tableDat->show();

    for (int iter = startRaw; iter < (startRaw + filesNumber); iter++)
    {
        QString nameMatrix = selectedDat[iter - startRaw];
        nameMatrix = FilesManager::findFileNumberInFileName(wildCard, nameMatrix.remove(Dir));
        tableDat->setText(iter, 0, nameMatrix);
        for (int i = 1; i < tableDat->numCols(); i++)
            tableDat->setText(iter, i, getHeaderInfoString(nameMatrix, tableDat->comment(i)));
    }

    if (checkBoxSortOutputToFolders->isChecked())
        app()->changeFolder("DAN :: script, info, ...");
    
    app()->maximizeWindow(TableName);

    tableDat->table()->resizeColumnToContents(0);
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

    //+++ Cols Names
    tableDat->setColName(0, "Runs");
    tableDat->setColPlotDesignation(0, Table::X);
    tableDat->setColumnType(0, Table::Text);

    //+++
    tableDat->show();

    app()->maximizeWindow(TableName);
}
// +++ extractor: Init
void dan18::extractorInit()
{
    QStringList lst;

    lst << "Sample";
    lst << "Comment1";
    lst << "Comment2";
    lst << "Name";
    lst << "Who";
    lst << "Sample-Nr";
    lst << "Date";
    lst << "Time";
    lst << "Duration";
    lst << "Sum";
    lst << "cps";
    lst << "C";
    lst << "D";
    lst << "Offset";
    lst << "lambda";
    lst << "Selector";
    lst << "Thickness";
    lst << "Monitor-1";
    lst << "Monitor-2";
    lst << "Monitor-3";
    lst << "Sample-Motor-1";
    lst << "Sample-Motor-2";
    lst << "Sample-Motor-3";
    lst << "Sample-Motor-4";
    lst << "Sample-Motor-5";
    lst << "Field-1";
    lst << "Field-2";
    lst << "Field-3";
    lst << "Field-4";
    lst << "Attenuator";
    lst << "Polarization";
    lst << "Polarizer-Polarization";
    lst << "Polarizer-Transmission";
    lst << "Polarizer-Flipper-Efficiency";
    lst << "Analyzer-Transmission";
    lst << "Analyzer-Efficiency";
    lst << "Lenses";
    lst << "Sum-vs-Mask";
    lst << "Sum-vs-Mask-Dead-Time-Corrected";
    lst << "Q2-vs-Mask";
    lst << "Runs";
    lst << "File-Names";

    comboBoxInfoExtractorCol->addItems(lst);

    for (int i = 0; i < lst.size(); ++i)
        lst[i] = QString("[%1]: %2").arg(i + 1).arg(lst[i]);

    lineEditCheckExtractorTableName->setToolTip(
        "- Add several columns by typing a list of indexes (e.g. \"2, 5, 8\")\n"
        "  and pressing Return.\n\n"
        "- Example: \"2, 5, 8\" " +
        QString::fromUtf8("↵") +
        "  → adds columns to the active table (selected in the combobox).\n\n"
        "- Example: \"TableName: 2, 5, 8\" " +
        QString::fromUtf8("↵") +
        "  → adds columns to the specified table \"TableName\".\n\n"
        "- Example: \"TableName: 2, 5, 8 : 02466 .. 02488\" " +
        QString::fromUtf8("↵") +
        "  → adds columns to the specified table \"TableName\"\n"
        "    (creates it if it does not exist) and fills them with runs from 02466 to 02488.\n\n"
        "Available columns:\n\n" +
        lst.join("\n"));
}