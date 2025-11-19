/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Explorer functions of compile interface
 ******************************************************************************/

#include "compile18.h"
#include "fit-function-explorer.h"
#include "repository-synchronization.h"

// +++  new Function Name
void compile18::newFunctionName()
{
    if (listViewFunctions->selectionModel()->selectedRows().count() == 0)
        return;
    if (lineEditFunctionName->text() != listViewFunctions->selectionModel()->selectedRows()[0].data().toString())
        listViewFunctions->clearSelection();
}
// +++  new Categoty Name
void compile18::newCategoryName()
{
    if (listViewGroup->selectionModel()->selectedRows().count() == 0)
        return;
    if (listViewGroup->selectionModel()->selectedRows()[0].data().toString() == "ALL")
        return;
    if (lineEditGroupName->text() != listViewGroup->selectionModel()->selectedRows()[0].data().toString())
    {
        listViewGroup->clearSelection();
        listViewFunctions->model()->removeColumn(0);
    }
}
// +++  stot 1D to 2D
void compile18::stot1Dto2D()
{
    if (radioButton1D->isChecked())
    {
        spinBoxXnumber->setMinimum(1);
        spinBoxXnumber->setValue(1);
        spinBoxXnumber->setEnabled(false);
        lineEditXXX->setText("x");
        spinBoxXnumber->hide();
    }
    else
    {
        spinBoxXnumber->setMinimum(2);
        spinBoxXnumber->setValue(2);
        spinBoxXnumber->setEnabled(true);
        lineEditXXX->setText("ix,iy");
        spinBoxXnumber->show();
    }
    scanGroups();

    textEditCode->clear();

    changedFXYinfo();
}
// +++ path Changed
void compile18::pathChanged()
{
    scanGroups();
    scanIncludedFunctions();
}
// +++ scan FitFunction dir on groups
void compile18::scanGroups()
{
    QStringList group;
    QString s, groupName;

    QDir d(pathFIF);

    QString fifExt = radioButton2D->isChecked() ? "*.2dfif" : "*.fif";

    QStringList lst;
    if (!pathFIF.isEmpty())
        lst = FunctionsExplorer::scanFiles(pathFIF, fifExt, false);

    for (int i = 0; i < lst.count(); i++)
    {
        if (d.exists(lst[i]))
        {
            QFile f(pathFIF + lst[i]);
            f.open(QIODevice::ReadOnly);
            QTextStream t(&f);

            //+++[group]
            s = t.readLine();
            if (s.contains("[group]"))
            {
                groupName = t.readLine().trimmed();
                if (!group.contains(groupName) && groupName != "")
                    group << groupName;
            }
            f.close();
        }
    }
    group.sort();

    if (!pathFIF.isEmpty())
        group << FunctionsExplorer::scanSubfolders(pathFIF, fifExt);
    group.prepend("ALL");

    lineEditGroupName->setText("");
    lineEditFunctionName->setText("");
    if (textLabelInfoVersion->isHidden())
        textLabelInfoSAS->setText("");

    listViewGroup->setModel(new QStringListModel(group));

    connect(listViewGroup->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &compile18::groupChanged);

    QStringList empty;
    listViewFunctions->setModel(new QStringListModel(empty));
}
// +++  scan Included Functions
void compile18::scanIncludedFunctions()
{
    QStringList lst;
    lst = FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.h", false);
    lst += FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.cpp", false);
    lst += FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.f", false);
    lst += FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.for", false);
    lst += FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.f90", false);

    listBoxIncludeFunctionsNew->setModel(new QStringListModel(QStringList()));
    listBoxIncludeFunctionsNew->setModel(new QStringListModel(lst));

    connect(listBoxIncludeFunctionsNew, SIGNAL(doubleClicked(const QModelIndex &)), this,
            SLOT(addIncludedFunction(const QModelIndex &)));
}
// +++  fing functions of single Group
void compile18::groupChanged(const QModelIndex &index, const QModelIndex &p)
{
    QString fifExt = radioButton2D->isChecked() ? "*.2dfif" : "*.fif";
    QString groupName = index.data().toString();

    QStringList functions;
    if (groupName.at(groupName.length() - 1) == '/')
        functions = FunctionsExplorer::listFilesInSubfolder(pathFIF, fifExt, groupName);
    else
        functions = groupFunctions(groupName);

    QStringList functionsRoot, functionsSubfolders;

    for (int i = 0; i < functions.count(); i++)
        if (functions[i].contains("/"))
            functionsSubfolders << functions[i];
        else
            functionsRoot << functions[i];

    functionsSubfolders.sort();
    functionsRoot.sort();
    functions.clear();

    functions << functionsRoot << functionsSubfolders;

    listViewFunctions->setModel(new QStringListModel(functions));
    
    connect(listViewFunctions->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(openFIFfileSimple(const QModelIndex &, const QModelIndex &)));

    if (functions.count() == 0)
        spinBoxPara->setValue(0);

    lineEditFunctionName->clear();
    spinBoxP->setValue(0);

    lineEditY->setText("f");
    textEditCode->clear();
    textEditDescription->clear();
    textEditDescription->setText("good place for function's comments");
    textEditHFiles->clear();
    textEditFunctions->clear();    
    tabWidgetCode->setCurrentIndex(0);
    tableCPP->setRowCount(0);
    radioButtonBAT->setChecked(false);
    radioButtonCPP->setChecked(false); radioButtonCPP->setText("*.cpp");
    radioButtonFIF->setChecked(false); radioButtonFIF->setText("*.fif");
}
// +++  find functions of single Group
QStringList compile18::groupFunctions(const QString &groupName)
{
    QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";
    QDir d(pathFIF);

    QStringList functions;

    QStringList lst = FunctionsExplorer::scanFiles(pathFIF, "*" + fifExt, false);
    
    for (int i = 0; i < lst.count(); i++)
    {
        if (d.exists(lst[i]))
        {
            QFile f(pathFIF + lst[i]);
            f.open( QIODevice::ReadOnly );
            QTextStream t(&f);

            if (t.readLine().contains("[group]") && (groupName == "ALL" || t.readLine().trimmed() == groupName))
                functions << lst[i].remove(fifExt);

            f.close();
        }
    }
    return functions;
}
// +++ open FIF: from listbox
void compile18::openFIFfileSimple(const QModelIndex &index, const QModelIndex &prev)
{
    QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";
    QString file = index.data().toString();

    textLabelInfoVersion->hide();
    textLabelInfoSASauthor->hide();

    QString ss = pathFIF + file + fifExt;
    openFIFfile(ss);
}
// +++  open FIF: from listbox
void compile18::openFIFfileSimple()
{
    QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";

    textLabelInfoVersion->hide();
    textLabelInfoSASauthor->hide();

    if (listViewFunctions->selectionModel()->selectedRows().count() > 0)
        openFIFfile(pathFIF + listViewFunctions->selectionModel()->selectedRows()[0].data().toString() + fifExt);
}
// +++ local: generate text from list
QString generateTextFromList(QStringList lst)
{
    QString txt;
    foreach (const QString &block, lst)
        txt += block + "\n";
    if (!txt.isEmpty())
        txt.chop(1);
    return txt;
}
// +++ local: select row by name
bool selectRowByName(QListView *listView, const QString &name)
{
    QAbstractItemModel *model = listView->model();
    if (!model)
        return false;

    for (int row = 0; row < model->rowCount(); ++row)
    {
        QModelIndex index = model->index(row, 0);
        QString itemText = model->data(index, Qt::DisplayRole).toString();
        if (itemText == name)
        {
            listView->blockSignals(true);
            listView->setCurrentIndex(index);
            listView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
            listView->blockSignals(false);
            return true;
        }
    }
    return false;
}
// +++ open FIF: code
void compile18::openFIFfile(QString fifName)
{
    if (!fifName.contains(pathFIF))
        fifName = pathFIF + fifName;

    QString content = FunctionsExplorer::readFifFileContent(fifName);
    QStringList listOfBlocks = FunctionsExplorer::getFifFileBlocks(content);

    textEditHFiles->clear();
    textEditCode->clear();
    textEditFunctions->clear();
    textEditForwardFortran->clear();

    if (fifName.contains(".2dfif"))
        radioButton2D->setChecked(true);

    spinBoxP->setValue(0);

    if (radioButtonFIF->isChecked())
    {
        tableCPP->setRowCount(0);
        QStringList lst = content.split("\n");
        tableCPP->setRowCount(static_cast<int>(lst.count()) + 1);
        for (int ii = 0; ii < lst.count(); ii++)
            tableCPP->setItem(ii, 0, new QTableWidgetItem(lst[ii]));
    }

    QStringList lst;
    QString s;

    // +++ [name]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[name]");
    if (lst.count() < 2)
        return;

    // +++ function Name
    QString functionName = lst[1].trimmed();
    if (functionName == "")
    {
        QMessageBox::warning(this, "QtiSAS", "Error: [name]");
        return;
    }

    lineEditFunctionName->setText(functionName);
    textLabelInfoSAS->setText(functionName);

    selectRowByName(listViewFunctions, functionName);

    // +++ [group]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[group]");
    if (lst.count() < 2)
        return;

    s = lst[0];
    s.remove("[group] ").remove("[group]");

    // +++ [group]: [eFit]
    checkBoxEfit->setChecked(s.contains("[eFit]"));
    s.remove("[eFit] ");
    if (s.contains("["))
    {
        lineEditEFIT->setText(s.left(s.indexOf("[")));
        s = s.right(s.length() - s.indexOf("["));
    }
    else
        lineEditEFIT->setText(s);

    // +++ [group]: [Weight]
    checkBoxWeight->setChecked(s.contains("[Weight]"));
    if (s.contains("[Weight]"))
    {
        s.remove("[Weight] ");
        comboBoxWeightingMethod->setCurrentIndex(s.left(1).toInt());
        s = s.right(s.length() - 2);
    }
    else
        comboBoxWeightingMethod->setCurrentIndex(0);

    // +++ [group]: [Superpositional]
    checkBoxSuperpositionalFit->setChecked(s.contains("[Superpositional]"));
    if (s.contains("[Superpositional]"))
    {
        s.remove("[Superpositional] ");
        QStringList sLst = s.split(" ", Qt::SkipEmptyParts);
        spinBoxSubFitNumber->setValue(sLst[0].toInt());
    }
    else
        spinBoxSubFitNumber->setValue(1);

    lineEditFitMethodPara->setText("");

    // +++ [group]: [Algorithm]
    checkBoxAlgorithm->setChecked(s.contains("[Algorithm]"));
    if (s.contains("[Algorithm]"))
    {
        s.remove("[Algorithm] ");
        comboBoxFitMethod->setCurrentIndex(s.left(1).toInt());
        s = s.right(s.length() - 2);
        if (s.contains("["))
            lineEditFitMethodPara->setText(s.left(s.indexOf("[")));
        else
            lineEditFitMethodPara->setText(s);
    }
    else
        comboBoxFitMethod->setCurrentIndex(0);

    // +++ group Name
    QString groupName = lst[1].trimmed();
    if (groupName == "")
        groupName = "ALL";

    QString groupOld = "";
    if (listViewGroup->selectionModel()->selectedRows().count() > 0)
        groupOld = listViewGroup->selectionModel()->selectedRows()[0].data().toString();

    if (groupOld.isEmpty() || groupOld.right(1) != '/' || functionName.left(groupOld.length()) != groupOld)
        lineEditGroupName->setText(groupName);
    else
    {
        lineEditGroupName->blockSignals(true);
        lineEditGroupName->setText(groupName);
        lineEditGroupName->blockSignals(false);
    }

    // +++ [number parameters]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[number parameters]");
    if (lst.count() < 2)
        return;

    int pNumber = lst[1].remove(",").trimmed().toInt();
    if (pNumber <= 0)
    {
        QMessageBox::warning(this, "QtiSAS", "Error: [number parameters]");
        return;
    }
    spinBoxP->setValue(pNumber);

    // +++ [description]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[description]");
    if (lst.count() < 2)
        return;
    textEditDescription->clear();
    textEditDescription->insertHtml(generateTextFromList(lst.mid(1)));

    // +++ [x]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[x]");
    if (lst.count() < 2)
        return;
    if (radioButton2D->isChecked())
    {
        QStringList sLst = lst[1].remove(" ").split(",", Qt::SkipEmptyParts);
        spinBoxXnumber->setValue(sLst.count());
    }
    lineEditXXX->setText(lst[1].remove(" "));

    // +++ [y]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[y]");
    if (lst.count() < 2)
        return;
    lineEditY->setText(lst[1].trimmed());

    // +++ [parameter names]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[parameter names]");
    if (lst.count() < 2)
        return;

    QStringList paraNames = lst[1].split(",", Qt::SkipEmptyParts);
    if (paraNames.size() != pNumber)
        QMessageBox::warning(this, "QtiSAS", "Error: [parameter names]");

    // +++ [initial values]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[initial values]");
    if (lst.count() < 2)
        return;

    QStringList initValues = lst[1].split(",", Qt::SkipEmptyParts);
    if (initValues.size() != pNumber)
        QMessageBox::warning(this, "QtiSAS", "Error: [initial values]");

    // +++ [adjustibility]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[adjustibility]");
    if (lst.count() < 2)
        return;
    QStringList adjustibilityList = lst[1].split(",", Qt::SkipEmptyParts);
    if (adjustibilityList.size() != pNumber)
        QMessageBox::warning(this, "QtiSAS", "Error: [adjustibility]");

    // +++ [parameter description]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[parameter description]");
    if (lst.count() < 2)
        return;

    QStringList paraDescription = lst[1].split(",", Qt::SkipEmptyParts);
    if (paraDescription.size() != pNumber)
        QMessageBox::warning(this, "QtiSAS", "Error: [parameter description]");

    for (int i = 0; i < pNumber; i++)
    {
        tableParaNames->item(i, 0)->setText(paraNames[i]);

        QString sCurrent = initValues[i];
        if (sCurrent.contains('['))
            tableParaNames->item(i, 1)->setText(QString::number(sCurrent.left(sCurrent.indexOf("[")).toDouble()));
        else
            tableParaNames->item(i, 1)->setText(QString::number(sCurrent.toDouble()));
        if (sCurrent.contains('[') && sCurrent.contains("..") && sCurrent.contains(']'))
            tableParaNames->item(i, 2)->setText(
                sCurrent.right(sCurrent.length() - sCurrent.indexOf("[") - 1).remove("[").remove("]"));
        else
            tableParaNames->item(i, 2)->setText("..");
        if (adjustibilityList[i].toInt() == 1)
            tableParaNames->item(i, 2)->setCheckState(Qt::Checked);
        else
            tableParaNames->item(i, 2)->setCheckState(Qt::Unchecked);

        tableParaNames->item(i, 3)->setText(paraDescription[i]);
    }

    // +++ [h-headers]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[h-headers]");
    textEditHFiles->clear();
    if (lst.count() > 1)
        textEditHFiles->append(generateTextFromList(lst.mid(1)));

    // +++ [included functions]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[included functions]");
    textEditFunctions->clear();
    if (lst.count() > 1)
    {
        textEditFunctions->append(generateTextFromList(lst.mid(1)));
        textEditFunctions->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
    }

    // +++[code]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[code]");
    textEditCode->clear();
    if (lst.count() > 1)
    {
        textEditCode->append(generateTextFromList(lst.mid(1)));
        textEditCode->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
    }

    // +++[fortran]
    checkBoxAddFortran->setChecked(false);
    fortranFunction->setText("");
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[fortran]");
    if (lst.count() > 3 && lst[1].contains("1"))
    {
        checkBoxAddFortran->setChecked(true);
        fortranFunction->setText(lst[2]);
        textEditForwardFortran->append(generateTextFromList(lst.mid(3)));
    }

    // +++ [python]
    checkBoxIncludePython->setChecked(false);
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[python]");
    if (lst.count() > 1)
        checkBoxIncludePython->setChecked(!lst[1].contains("0"));

    // +++ [after.fit: python]
    checkBoxAfterFitPython->setChecked(false);
    textEditAfterFitPython->clear();
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[after.fit: python]");
    if (lst.count() > 1)
        checkBoxAfterFitPython->setChecked(lst[1].contains("1"));

    // +++ [after.fit: python code]
    if (checkBoxAfterFitPython->isChecked())
    {
        lst = FunctionsExplorer::readBlock(listOfBlocks, "[after.fit: python code]");
        if (lst.count() > 1)
            textEditAfterFitPython->append(generateTextFromList(lst.mid(1)));
    }

    // +++ [x.range]
    checkBoxCustomXrange->setChecked(false);
    checkBoxUniformX->setChecked(false);
    checkBoxLogStep->setChecked(true);
    lineEditMin->setText("");
    lineEditMax->setText("");
    lineEditPoints->setText("");
    lineEditYmin->setText("");

    lst = FunctionsExplorer::readBlock(listOfBlocks, "[x.range]");
    if (lst.count() > 7)
    {
        checkBoxCustomXrange->setChecked(lst[1].contains("1"));
        checkBoxUniformX->setChecked(lst[2].contains("1"));
        lineEditMin->setText(lst[3]);
        lineEditMax->setText(lst[4]);
        lineEditPoints->setText(lst[5]);
        lineEditYmin->setText(lst[6]);
        checkBoxLogStep->setChecked(lst[7].contains("1"));
    }

    radioButtonCPP->setText(lineEditFunctionName->text() + ".cpp");
    radioButtonFIF->setText(lineEditFunctionName->text() + ".fif");

    if (radioButtonCPP->isChecked())
        makeCPP();

    if (radioButtonBAT->isChecked())
        makeCompileScript();

    textEditHFiles->textChanged();
}
// +++  make CPP file
void compile18::makeCPP()
{
    QString fn = pathFIF + lineEditFunctionName->text().trimmed() + ".cpp";
    
    if (lineEditFunctionName->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> Fill Function name!"));
        return;
    }
    saveAsCPP(fn);
}
#ifdef Q_OS_WIN
QString mingwCompilerPath()
{
    QString progFilesFolder = qEnvironmentVariable("PROGRAMFILES").replace('\\', '/') + '/';
    QStringList possibleFolders = {"C:/", "C:/Qt/Tools/", progFilesFolder, progFilesFolder + "Qt/Tools/", "C:/msys64/"};

    QStringList qtVersions = {"mingw1310_64", "mingw1120_64", "mingw810_64", "mingw",
                              "mingw64",      "mingw_64",     "llvm1706",    "llvm"};

    for (const QString &folder : possibleFolders)
    {
        for (const QString &version : qtVersions)
        {
            QString potentialPath = folder + version;
            QFileInfo gfortranFile(potentialPath + "/bin/gfortran.exe");

            if (gfortranFile.exists())
                return potentialPath;
        }
    }

    std::cout << "Install MinGW with gfortran !!!";
    return QStringLiteral("Install MinGW with gfortran !!!");
}
#endif
// +++  make compile.script file
void compile18::makeCompileScript()
{
    QString ext = FunctionsExplorer::filterShared().remove("*");
    if (radioButton2D->isChecked())
        ext += "2d";

    pathFIF = fitPath->text();
    batFileMSVC = lineEditBatFileMSVC->text();

    QString pathGSL = "";
#if defined(Q_OS_WIN)
    pathGSL = qApp->applicationDirPath();
#elif defined(Q_OS_MAC)
    QDir dd;
    dd.setPath(qApp->applicationDirPath());
    dd.cdUp();
    pathGSL = dd.absolutePath();
#endif

    QString fn = pathFIF + "compile.script";
    QString script = "";
    QString fortranText = ""; // fortran o-file
    QString compileFlags = lineEditCompileFlags->text();
    QString linkFlags = lineEditLinkFlags->text();
    QString functionName = lineEditFunctionName->text().trimmed();

#if defined(Q_OS_WIN)
    fn += ".ps1";
    script += "Set-Location " + QString("\"") + pathFIF + "\"" + "\n";
    script += "$GSL = " + QString("\"`\"") + pathGSL + QString("`\"\"") + "\n";
    script += "$vcvars = " + QString("\"`\"") + batFileMSVC + QString("`\"\"") + "\n";
#elif defined(Q_OS_MAC)
    script += "cd \"" + pathFIF + "\"\n";
    script += "GSL=" + QString("\"") + pathGSL + "\"" + "\n";
    script += "export LIBRARY_PATH=$GSL/Frameworks/:$LIBRARY_PATH\n";
    script += "export CPLUS_INCLUDE_PATH=$GSL/Resources/:$CPLUS_INCLUDE_PATH\n";
#else
    script += "cd \"" + pathFIF + "\"\n";
#endif

    //+++ Python
    if (checkBoxIncludePython->isChecked())
    {

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)

        QString pyInclude = app()->scriptCaller("import sysconfig; return sysconfig.get_path('include')").toString();

        QString pyVersion =
            app()->scriptCaller("import sys; return f'{sys.version_info[0]}.{sys.version_info[1]}'").toString();

#endif

#if defined(Q_OS_WIN)

        const QString pyLibDir =
            app()->scriptCaller("import sysconfig; return sysconfig.get_config_var('LIBDIR')").toString();

        script += "$PYTHON_INCLUDE_PATH = \"" + pyInclude + "\"\n";
        script += "$PYTHON_LIBS_PATH = \"" + pyLibDir + "\"\n";
        script += "$PYTHON_LIB = \"python" + pyVersion.remove('.') + ".lib\"\n";

        compileFlags += " /I`\"$PYTHON_INCLUDE_PATH`\"";
        linkFlags += " /LIBPATH:`\"$PYTHON_LIBS_PATH`\" $PYTHON_LIB";

#elif defined(Q_OS_MAC)

        const QString pyLibDir = QFileInfo(QFileInfo(pyInclude).dir().path()).dir().filePath("lib");

        script += "export PYTHON_INCLUDE=\"" + pyInclude + "\"\n";
        script += "export PYTHON_LIB_PATH=\"" + pyLibDir + "\"\n";
        script += "export PYTHON_LIB=\"python" + pyVersion + "\"\n";

        compileFlags += " -I\"$PYTHON_INCLUDE/\"";
        linkFlags += " -L\"$PYTHON_LIB_PATH/\" -l$PYTHON_LIB -ldl -framework CoreFoundation";

#else

        compileFlags += " $(python3-config --includes)";
        linkFlags += " $(python3-config --ldflags --embed)";

#endif
    }


    //+++ COMPILE FUNCTION
#if defined(Q_OS_WIN)
    script += "cmd /c \"$vcvars && " + compileFlags + " " + functionName + ".cpp ";
    script += "/Fo" + functionName + ".obj\"\n";
#else
    script += compileFlags + " " + functionName + ".cpp -o " + functionName + ".o\n";
#endif

    //+++ COMPILE FORTRAN FILE
    QString fortranOstring = "";
    if (checkBoxAddFortran->isChecked())
    {
        QString fortranCompileString = fortranFunction->text().replace(", ", " ").simplified();
        QStringList ofiles = fortranCompileString.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QStringList fileNames;

        for (const QString &ofile : ofiles)
            fileNames << QFileInfo(ofile).fileName();

        fortranOstring = fileNames.join(' ');

        fortranOstring.replace(".f90", ".o", Qt::CaseInsensitive);
        fortranOstring.replace(".for", ".o", Qt::CaseInsensitive);
        fortranOstring.replace(".f", ".o", Qt::CaseInsensitive);

#if defined(Q_OS_WIN)
        fortranCompileString.replace(" ", "\" \"").prepend("\"").append("\"");

        script += "$env:Path = \"" + mingwCompilerPath() + "/bin/;$env:Path\"\n";
        script += "gfortran -c " + fortranCompileString + " -static-libgfortran\n";
        fortranText = " " + fortranOstring;

#elif defined(Q_OS_MAC)

#if defined(Q_PROCESSOR_ARM_64)
        script += "export gfortranSTR=/opt/homebrew/bin/gfortran" + QString("\n");
#else
        script += "export gfortranSTR=/usr/local/bin/gfortran" + QString("\n");
#endif

        compileFlags =
            compileFlags.replace("clang ", "$gfortranSTR ").remove(" -I$GSL").remove("/Resources").remove("/include");
        script += compileFlags + "  " + fortranCompileString + "\n";
        script += "gfortranPath=\"$(dirname `$gfortranSTR --print-file-name libgfortran.a`)\";";
        script += "gfortranPath=${gfortranPath%x*4}\n";
        fortranText = " -L$gfortranPath -lgfortran fortranOstring ";
        fortranText = " " + fortranOstring;
#else
        script += "gfortranSTR=$(which $(compgen -c gfortran | tail -n 1))" + QString("\n");
        compileFlags = compileFlags.replace("g++ ", "$gfortranSTR ").remove(" -I$GSL").remove("/include");
        script += compileFlags + "  " + fortranCompileString + "\n";
        fortranText = " " + fortranOstring;
#endif
    }

    //+++ LINK
#if defined(Q_OS_WIN)
    linkFlags = "cmd /c \"$vcvars && " + linkFlags + " ";
    linkFlags += functionName + ".obj" + fortranText;
    linkFlags += " /OUT:" + functionName + ext + "\"";
#else
    linkFlags = linkFlags.replace(" -o", "  -o " + functionName + ext + "  " + functionName + ".o" + fortranText);
#endif

    script += linkFlags + "  \n";

    if (checkBoxAddFortran->isChecked())
    {

#if defined(Q_OS_WIN)
        script += "Remove-Item " + fortranOstring + "\n";
#else
        script += "rm " + fortranOstring + "\n";
#endif
    }

#if defined(Q_OS_WIN)
    script += "Remove-Item \"" + functionName + ".obj\"";
    script += ", \"" + functionName + ".lib\"";
    script += ", \"" + functionName + ".exp\"";
    script += " -ErrorAction SilentlyContinue";
#else
    script += "rm " + functionName + ".o\n";
#endif

    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                              tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
        return;
    }

    QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    t << script;
    f.close();
    f.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser);
    
    if (radioButtonBAT->isChecked())
    {
        tableCPP->setRowCount(0);
        QStringList lst = script.split("\n");
        tableCPP->setRowCount(lst.count());
        for (int ii = 0; ii < lst.count(); ii++)
            tableCPP->setItem(ii, 0, new QTableWidgetItem(lst[ii]));
    }
}
// +++  save FIF file /slot
void compile18::makeFIF()
{
    QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";
    QString group = lineEditGroupName->text().trimmed();
    QString name = lineEditFunctionName->text().trimmed() + fifExt;

    QString nameOld = "";
    if (listViewFunctions->selectionModel()->selectedRows().count() > 0)
        nameOld = listViewFunctions->selectionModel()->selectedRows()[0].data().toString() + fifExt;

    if (name != nameOld && name.contains("/"))
    {
        QString folder = FunctionsExplorer::subFolder(name);
        if (folder == "")
            name = name.remove("/");
        else
        {
            QDir dir(pathFIF);
            QString subfolderPath = dir.filePath(folder);
            if (!QDir(subfolderPath).exists())
                dir.mkpath(subfolderPath);
        }
    }

    QString fn = pathFIF + "/" + name;
    if (!save(fn, false))
        return;

    if (nameOld != name)
    {
        scanGroups();
        const QModelIndexList indexes = listViewGroup->model()->match(listViewGroup->model()->index(0, 0),
                                                                      Qt::DisplayRole, group, 1, Qt::MatchExactly);
        if (!indexes.isEmpty())
            listViewGroup->selectionModel()->select(indexes[0], QItemSelectionModel::Select);

        groupChanged(indexes[0], indexes[0]);

        const QModelIndexList indexesFunctions = listViewFunctions->model()->match(
            listViewFunctions->model()->index(0, 0), Qt::DisplayRole, name.remove(fifExt), 1, Qt::MatchExactly);

        if (!indexesFunctions.isEmpty())
            listViewFunctions->selectionModel()->select(indexesFunctions[0], QItemSelectionModel::Select);

        openFIFfileSimple(indexesFunctions[0],indexesFunctions[0]);
    }
}
// +++  delete FIF file
void compile18::deleteFIF()
{
    QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";
    QString fn = listViewFunctions->selectionModel()->selectedRows()[0].data().toString().trimmed() + fifExt;
    if (fn == "")
        return;

    int functionCount = listViewFunctions->model()->rowCount();

    if (QMessageBox::question(this, tr("QtiSAS::Delete Function?"), tr("Do you want to delete Function %1?").arg(fn),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        return;

    QDir d(pathFIF);
    d.remove(fn);
    fn = fn.remove(fifExt, Qt::CaseInsensitive);
    for (const QString &ext : {".cpp", ".dll", ".so", ".dylib"})
        d.remove(fn + ext);

    QString subFolder = FunctionsExplorer::subFolder(fn);
    QString group = lineEditGroupName->text().trimmed();
    if (listViewGroup->selectionModel()->selectedIndexes().count() > 0)
        group = listViewGroup->selectionModel()->selectedRows()[0].data().toString().trimmed();

    if (subFolder != "")
    {
        QDir dir(pathFIF + subFolder);
        if (dir.exists() && dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty() &&
            !subFolder.contains("IncludedFunctions"))
        {
            group = lineEditGroupName->text().trimmed();
            QDir(pathFIF).rmdir(subFolder);
        }
    }

    scanGroups();

    const QModelIndexList indexes =
        listViewGroup->model()->match(listViewGroup->model()->index(0, 0), Qt::DisplayRole, group, 1, Qt::MatchExactly);

    newFIF();

    if (functionCount > 1)
    {
        if (!indexes.isEmpty())
            listViewGroup->selectionModel()->select(indexes[0], QItemSelectionModel::Select);
        groupChanged(indexes[0], indexes[0]);
    }
}
// +++  save FIF file /function
bool compile18::save(const QString &fn, bool askYN)
{
    QString text;

    if (spinBoxP->value() == 0)
    {
        QMessageBox::warning(this, tr("QtiSAS"), tr("Error: <p> Check function!"));
        return false;
    }

    if (lineEditFunctionName->text().trimmed() == "")
    {
        QMessageBox::warning(this, tr("QtiSAS"), tr("Error: <p> Fill Function name!"));
        return false;
    }

    if (askYN && QFile::exists(fn) &&
        QMessageBox::question(this, tr("QtiSAS -- Overwrite File? "),
                              tr("A file called: <p><b>%1</b><p>already exists.\n"
                                 "Do you want to overwrite it?")
                                  .arg(fn),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        return false;
    else
    {
        text += "[group]";

        if (checkBoxEfit->isChecked())
            text += " [eFit] " + lineEditEFIT->text();
        if (checkBoxWeight->isChecked())
            text += " [Weight] " + QString::number(comboBoxWeightingMethod->currentIndex());
        if (checkBoxSuperpositionalFit->isChecked())
            text += " [Superpositional] " + QString::number(spinBoxSubFitNumber->value());
        if (checkBoxAlgorithm->isChecked())
            text += " [Algorithm] " + QString::number(comboBoxFitMethod->currentIndex()) + " " +
                    lineEditFitMethodPara->text();

        text += "\n";
        text += lineEditGroupName->text().trimmed();
        text += "\n\n";

        text += "[name]\n";
        text += lineEditFunctionName->text().trimmed();
        text += "\n\n";

        text += "[number parameters]\n";
        text += QString::number(spinBoxP->value());
        text += "\n\n";
        text += "[description]\n";

        QString htmlText = textEditDescription->toHtml();
        htmlText = R"(<html><head><meta name= "qrichtext " content= "1 " /></head>)" +
                   htmlText.right(htmlText.length() - htmlText.indexOf("<body"));

        text += htmlText;
        text += "\n\n";

        text += "[x]\n";
        text += lineEditXXX->text().remove(" ");
        text += "\n\n";

        text += "[y]\n";
        if (lineEditY->text().trimmed() != "")
            text += lineEditY->text().trimmed();
        else
            text += "I";
        text += "\n\n";

        text += "[parameter names]\n";
        int p = spinBoxP->value();
        for (int i = 0; i < p; ++i)
        {
            auto *item = tableParaNames->item(i, 0);
            if (item->text().isEmpty())
                item->setText("P" + QString::number(i + 1));
            text += item->text().trimmed();
            if (i < p - 1)
                text += ',';
            else
                text += "\n\n";
        }

        text += "[initial values]\n";

        for (int i = 0; i < spinBoxP->value(); i++)
        {
            QString valueCurrent = tableParaNames->item(i, 1)->text().remove(" ");
            QString limitCurrent = tableParaNames->item(i, 2)->text().remove(" ");

            if (limitCurrent == "..")
                limitCurrent = "";
            else if (limitCurrent.contains(".."))
            {
                if (limitCurrent.endsWith(".."))
                    limitCurrent += "inf";
                else if (limitCurrent.startsWith(".."))
                    limitCurrent = "-inf" + limitCurrent;
                limitCurrent = "[" + limitCurrent + "]";
            }
            text += QString::number(valueCurrent.toDouble()) + limitCurrent;
            if (i < (spinBoxP->value() - 1))
                text += ',';
            else
                text += "\n\n";
        }

        text += "[adjustibility]\n";

        QStringList lst;
        lst.reserve(p);
        for (int i = 0; i < p; ++i)
        {
            bool checked = tableParaNames->item(i, 2)->checkState() == Qt::Checked;
            lst << (checked ? "1" : "0");
        }
        text += lst.join(',') + "\n\n";

        
        text += "[parameter description]\n";

        lst.clear();
        lst.reserve(p);
        for (int i = 0; i < p; ++i)
        {
            auto *item = tableParaNames->item(i, 3);
            QString cellText = item->text().trimmed();

            if (cellText.isEmpty())
            {
                cellText = "---";
                item->setText(cellText);
            }
            lst << cellText;
        }
        text += lst.join(',') + "\n\n";
        
        text += "[h-headers]\n";
        text += textEditHFiles->toPlainText();
        text += "\n\n";

        text += "[included functions]\n";
        text += textEditFunctions->toPlainText();
        text += "\n\n";

        text += "[code]\n";
        text += textEditCode->toPlainText();
        text += "\n\n";

        text += "[fortran]\n";
        text += checkBoxAddFortran->isChecked() ? "1" : "0";
        text += "\n";
        text += fortranFunction->text();
        text += "\n";
        text += textEditForwardFortran->toPlainText();
        text += "\n\n";

        text += "[python]\n";
        text += checkBoxIncludePython->isChecked() ? "1" : "0";
        text += "\n\n";

        text += "[after.fit: python]\n";
        text += checkBoxAfterFitPython->isChecked() ? "1" : "0";
        text += "\n\n";

        text += "[after.fit: python code]\n";
        text += textEditAfterFitPython->toPlainText();
        text += "\n\n";

        text += "[x.range]\n";
        text += checkBoxCustomXrange->isChecked() ? "1" : "0";
        text += "\n";
        text += checkBoxUniformX->isChecked() ? "1" : "0";
        text += "\n";
        text += lineEditMin->text();
        text += "\n";
        text += lineEditMax->text();
        text += "\n";
        text += lineEditPoints->text();
        text += "\n";
        text += lineEditYmin->text();
        text += "\n";
        text += checkBoxLogStep->isChecked() ? "1" : "0";
        text += "\n\n";

        text += "[end]";

        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(nullptr, tr("QtiSAS - File Save Error"),
                                  tr("Could not writ<e to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
            return false;
        }
        QTextStream t(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        t.setCodec("UTF-8");
#endif
        t << text;
        f.close();
        
        if (radioButtonFIF->isChecked())
        {
            tableCPP->setRowCount(0);
            QStringList lst = text.split("\n");
            tableCPP->setRowCount(lst.count());
            for (int ii = 0; ii < lst.count(); ii++)
                tableCPP->setItem(ii, 0, new QTableWidgetItem(lst[ii]));
        }
    }

    return true;
}

QString ampersand_encode(const QString &str)
{
    QString chr;
    QStringList list = QStringList();
    for (int i = 0; i < str.size(); ++i)
    {
        chr = QString(str[i]);
        if (QString(chr.toLocal8Bit()) != chr)
            list << "&#" + QString::number(chr[0].unicode()) + ";";
        else list << chr;
    }
    return list.join("");
}
//+++ saveAsCPP
void compile18::saveAsCPP(const QString &fn)
{
    if (radioButton2D->isChecked())
        return saveAsCPP2d(fn);
    return saveAsCPP1d(fn);
}
// +++ saveAsCPP1d
void compile18::saveAsCPP1d(const QString &fn)
{
    QString fifExt = ".fif";
    QString text;

    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ do not change\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "#if defined(_WIN64) || defined(_WIN32) //MSVC Compiler\n"
            "#define MY_EXPORT __declspec(dllexport)\n"
            "#else\n"
            "#define MY_EXPORT\n"
            "#endif\n\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ h-files\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "#include <math.h>\n"
            "#include <iostream>\n"
            "#include <gsl/gsl_vector.h>\n"
            "#include <gsl/gsl_matrix.h>\n"
            "#include <gsl/gsl_math.h>\n";

    if (checkBoxIncludePython->isChecked())
        text += "#include <Python.h>\n";

    QStringList lstTmp = text.split("\n");

    lnTextEditHFiles->firstLineIncrement = static_cast<int>(lstTmp.count()) - 1;
    lnTextEditHFiles->updateLineNumbers(true);
    lstTmp.clear();

    QString textHF = textEditHFiles->toPlainText();
    QStringList lstHF = textHF.split("\n");

    for (int i = 0; i < lstHF.count(); i++)
        if (lstHF[i].contains("#include"))
        {
            if (lstHF[i].startsWith("namespace "))
                lstHF[i].replace("{", "{\n").replace("}", "\n}");
            text += lstHF[i] + "\n";
        }


    text += "std::string fitFunctionPath=\"";
    text += fitPath->text().toLocal8Bit().constData();
    text += "\";\n";
    text += "std::string OS=\"";

#if defined(_WIN64) || defined(_WIN32)
    text += "WINDOWS";
#elif defined(Q_OS_MAC)
    text += "MAC";
#else
    text += "LINUX";
#endif
    text += "\";\n";

    text += "\n/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Parameters Structure\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "#ifndef  TFUNCTION_H\n#define TFUNCTION_H\n";
    text += "struct functionT\n{\n";
    text += "  gsl_vector *para;\n";
    text += "  gsl_vector *para_limit_left;\n";
    text += "  gsl_vector *para_limit_right;\n";
    text += "  gsl_vector_int *para_fit_yn;\n";
    text += "  double *Q;\n";
    text += "  double *I;\n";
    text += "  double *dI;\n";
    text += "  double *SIGMA;\n";
    text += "  int *listLastPoints;\n";
    text += "  int currentM;\n";
    text += "  int currentFirstPoint;\n";
    text += "  int currentLastPoint;\n";
    text += "  int currentPoint;\n";
    text += "  bool polyYN;\n";
    text += "  int polyFunction;\n";
    text += "  bool beforeFit;\n";
    text += "  bool afterFit;\n";
    text += "  bool beforeIter;\n";
    text += "  bool afterIter;\n";
    text += "  double Int1;\n";
    text += "  double Int2;\n";
    text += "  double Int3;\n";
    text += "  int currentInt;\n";
    text += "  int prec;\n";
    text += "  char *tableName;\n";
    text += "  char **tableColNames;\n";
    text += "  int *tableColDestinations;\n";
    text += "  gsl_matrix *mTable;\n";
    text += "  int currentFunction;\n";
    text += "};\n";
    text += "#endif\n";
    text += "\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Global definition of parameters\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "double " + lineEditXXX->text().remove(" ").trimmed() + ";\n";
    text += "static double " + tableParaNames->item(0, 0)->text().trimmed();

    for (int i = 1; i < spinBoxP->value(); i++)
        text += ", " + tableParaNames->item(i, 0)->text().trimmed();

    text += ";\n";
    text += "static bool initNow;\n";
    text += "\n";
    text += "#define Para ((struct functionT *) ParaM)->para\n";
    text += "\n";
    text += "#define XXX   ((struct functionT *) ParaM)->Q\n";
    text += "#define YYY   ((struct functionT *) ParaM)->I\n";
    text += "#define dYYY  ((struct functionT *) ParaM)->dI\n";
    text += "#define RESO  ((struct functionT *) ParaM)->SIGMA\n";
    text += "#define prec  ((struct functionT *) ParaM)->prec\n";
    text += "\n";
    text += "#define listLastPoints    ((struct functionT *) ParaM)->listLastPoints\n";
    text += "#define currentM          ((struct functionT *) ParaM)->currentM\n";
    text += "#define currentFirstPoint ((struct functionT *) ParaM)->currentFirstPoint\n";
    text += "#define currentLastPoint  ((struct functionT *) ParaM)->currentLastPoint\n";
    text += "#define currentPoint      ((struct functionT *) ParaM)->currentPoint\n";
    text += "#define currentInt        ((struct functionT *) ParaM)->currentInt\n";
    text += "#define currentFunction   ((struct functionT *) ParaM)->currentFunction\n";
    text += "\n";
    text += "#define beforeFit ((struct functionT *) ParaM)->beforeFit\n";
    text += "#define afterFit  ((struct functionT *) ParaM)->afterFit\n";
    text += "#define beforeIter ((struct functionT *) ParaM)->beforeIter\n";
    text += "#define afterIter ((struct functionT *) ParaM)->afterIter\n";
    text += "#define polyYN    ((struct functionT *) ParaM)->polyYN\n";
    text += "#define Int1    ((struct functionT *) ParaM)->Int1\n";
    text += "#define Int2    ((struct functionT *) ParaM)->Int2\n";
    text += "#define Int3    ((struct functionT *) ParaM)->Int3\n";
    text += "\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Function: read Parameters\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "void readParameters(void * ParaM)\n";
    text += "{\n";
    text += "  if (beforeFit || afterFit || beforeIter) initNow=true; else initNow=false;\n";

    text += "  \n";
    for (int i = 0; i < spinBoxP->value(); i++)
    {
        QString paraName = tableParaNames->item(i, 0)->text().trimmed();
        text += "  " + paraName + "=gsl_vector_get(Para, " + QString::number(i) + ");\n";
    }
    text += "}\n";
    text += "\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Function: save Parameters\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "void saveParameters(void * ParaM)\n";
    text += "{\n";
    text += "  if (!beforeFit && !afterFit && !beforeIter && !afterIter) return;\n";

    for (int i = 0; i < spinBoxP->value(); i++)
        text +=
            "  gsl_vector_set(Para," + QString::number(i) + "," + tableParaNames->item(i, 0)->text().trimmed() + ");\n";

    text += "}\n";
    text += "\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Function returns Name of Function\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "extern \"C\" MY_EXPORT char *name()\n";
    text += "{\n";
    text += "  return \"" + lineEditFunctionName->text() + "\";\n";
    text += "}\n";
    text += "\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Function return List of names of Parameters \n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "extern \"C\" MY_EXPORT char *parameters()\n";
    text += "{\n";
    text += "  return \"" + tableParaNames->item(0, 0)->text().trimmed();

    for (int i = 1; i < spinBoxP->value(); i++)
        text += ',' + tableParaNames->item(i, 0)->text().trimmed();

    text += ',' + lineEditXXX->text().trimmed() + "\";\n";
    text += "}\n";
    text += "\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Initial value of Parameters  \n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "extern \"C\" MY_EXPORT char *init_parameters()\n";
    text += "{\n";
    text += "  return \"";

    for (int i = 0; i < spinBoxP->value(); i++)
    {
        QString valueCurrent = tableParaNames->item(i, 1)->text();
        QString limitCurrent = tableParaNames->item(i, 2)->text().remove(" ");

        if (limitCurrent == "..")
            limitCurrent = "";
        else if (limitCurrent.contains(".."))
        {
            if (limitCurrent.endsWith(".."))
                limitCurrent += "inf";
            else if (limitCurrent.startsWith(".."))
                limitCurrent = "-inf" + limitCurrent;
            limitCurrent = "[" + limitCurrent + "]";
        }
        text += QString::number(valueCurrent.toDouble()) + limitCurrent;
        text += (i < spinBoxP->value() - 1) ? "," : "\";\n";
    }

    text += "}\n";
    text += "\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Initial 'Adjustibility' of Parameters  \n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "extern \"C\" MY_EXPORT char *adjust_parameters()\n";
    text += "{\n";
    text += "  return \"";

    int p = spinBoxP->value();
    for (int i = 0; i < p; ++i)
    {
        bool checked = tableParaNames->item(i, 2)->checkState() == Qt::Checked;
        text += (checked ? "1" : "0");
        text += (i < p - 1 ? "," : "");
    }

    text += "\";\n";
    text += "}\n";
    text += "\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Number of parameters\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "extern \"C\" MY_EXPORT char *paraNumber()\n";
    text += "{\n";
    text += "  return \"" + QString::number(spinBoxP->value()) + "\";\n";
    text += "}\n";
    text += "\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Description of Function and parameters\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "extern \"C\" MY_EXPORT char *listComments()\n";
    text += "{\n";
    text += "  char  *list;\n";
    text += "  list=";

    QString htmlText = textEditDescription->toHtml().remove("\n").replace("\\;", R"(\\ \\;)");

    htmlText = R"(<html><head><meta name= "qrichtext " content= "1 " /></head>)" +
               htmlText.right(htmlText.length() - htmlText.indexOf("<body"));
    htmlText = ampersand_encode(htmlText);

    QString richText = htmlText.replace('"', "\\\"");

    if (richText == "")
        richText = "---";

    text += "\"" + richText + "\"\n";
    text += "\t\t\t\"";

    for (int i = 0; i < spinBoxP->value(); i++)
        text += ",," + tableParaNames->item(i, 3)->text();

    text += "\";\n return list;\n}\n\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Included Functions\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n\n";

    lstTmp = text.split("\n");
    lnTextEditFunctions->firstLineIncrement = static_cast<int>(lstTmp.count()) - 1;
    lnTextEditFunctions->updateLineNumbers(true);
    lstTmp.clear();

    text += textEditFunctions->toPlainText();

    if (checkBoxAddFortran->isChecked())
    {
        text += "\n/////////////////////////////////////////////////////////////////////////////////\n";
        text += "//+++ Forward Declaration Fortran Functions\n";
        text += "/////////////////////////////////////////////////////////////////////////////////\n";
        text += "extern \"C\" \n{\n";
        text += textEditForwardFortran->toPlainText() + "\n}\n\n";
    }

    text += "\n\n/////////////////////////////////////////////////////////////////////////////////\n";
    text += "//+++ Fit Function\n";
    text += "/////////////////////////////////////////////////////////////////////////////////\n";
    text += "extern \"C\" MY_EXPORT double functionSANS(double key";
    text += ", void * ParaM)\n";
    text += "  {\n";
    text += "//+++++++++++++++++++++++++++++++++++++++++++++\n";
    text += "  " + lineEditXXX->text().remove(" ") + "=key;\n";
    text += "  double " + lineEditY->text().trimmed() + ";\n";
    text += "  readParameters(ParaM);\n";
    text += "//+++++++++++++++++++++++++++++++++++++++++++++\n";
    text += "  gsl_set_error_handler_off();\n";
    text += "//+++++++++++++++++++++++++++++++++++++++++++++\n\n";

    lstTmp = text.split("\n");
    lnTextEditCode->firstLineIncrement = static_cast<int>(lstTmp.count()) - 1;
    lnTextEditCode->updateLineNumbers(true);
    lstTmp.clear();

    text += textEditCode->toPlainText();
    text += "\n\n//+++++++++++++++++++++++++++++++++++++++++++++\n";
    text += "\t saveParameters(ParaM);\n";
    text += "\t return " + lineEditY->text().trimmed() + ";\n";
    text += "//+++++++++++++++++++++++++++++++++++++++++++++\n";
    text += "}\n";

    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(nullptr, tr("QtiSAS - File Save Error"),
                              tr("Write error:<br><h4>%1</h4><p>Check permissions for this location.").arg(fn));
        return;
    }
    QTextStream t(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    t << text;
    f.close();
    
    if (radioButtonCPP->isChecked())
    {
        tableCPP->setRowCount(0);
        QStringList lst = text.split("\n");
        tableCPP->setRowCount(lst.count()+1);
        for (int ii = 0; ii < lst.count(); ii++)
            tableCPP->setItem(ii, 0, new QTableWidgetItem(lst[ii]));
    }
}
// +++ saveAsCPP2d : todo
void compile18::saveAsCPP2d(const QString &fn)
{
}
// +++ downloadFIF()
void compile18::downloadFIF(const QString &repoUrl)
{
    const QStringList allowedExtensions = {"fif", "h", "c", "f", "f90", "py"};
    RepositorySynchronization::updateGit(pathFIF, repoUrl, allowedExtensions);
    newFIF();
}
// +++ newFIF()
void compile18::newFIF()
{
    textEditHFiles->clear();
    textEditCode->clear();
    textEditFunctions->clear();
    lineEditGroupName->clear();
    lineEditFunctionName->clear();
    textEditDescription->clear();
    textEditDescription->setText("good place for function's comments");

    spinBoxP->setValue(0);
    lineEditY->setText("f");

    stot1Dto2D();
    changedFXYinfo();

    scanGroups();
    scanIncludedFunctions();

    tabWidgetCode->setCurrentIndex(0);
    tableCPP->setRowCount(0);
    radioButtonBAT->setChecked(false);

    radioButtonCPP->setChecked(false);
    radioButtonCPP->setText("*.cpp");

    radioButtonFIF->setChecked(false);
    radioButtonFIF->setText("*.fif");

    checkBoxAddFortran->setChecked(false);
    fortranFunction->setText("");
    textEditForwardFortran->setText("");

    checkBoxEfit->setChecked(false);
    lineEditEFIT->setText("");
    checkBoxWeight->setChecked(false);
    comboBoxWeightingMethod->setCurrentIndex(1);
    checkBoxSuperpositionalFit->setChecked(false);
    spinBoxSubFitNumber->setValue(1);
    checkBoxAlgorithm->setChecked(false);
    comboBoxFitMethod->setCurrentIndex(1);
    lineEditFitMethodPara->setText("");

    listViewGroup->selectionModel()->setCurrentIndex(listViewGroup->model()->index(0, 0),
                                                       QItemSelectionModel::ClearAndSelect);
}
// +++  build Shared Library
void compile18::buildSharedLibrary(bool compileAllYN)
{
    bool activateFunction = !pushButtonSave->isEnabled() || compileAllYN;
    if (activateFunction)
        openFIFfileSimple();
    else
        makeFIF();

    makeCPP();
    makeCompileScript();

    if (activateFunction)
    {
        listViewFunctions->selectionModel()->blockSignals(true);
        listViewFunctions->selectionModel()->select(listViewFunctions->currentIndex(), QItemSelectionModel::Select);
        listViewFunctions->scrollTo(listViewFunctions->currentIndex(), QAbstractItemView::PositionAtCenter);
        listViewFunctions->selectionModel()->blockSignals(false);
    }

    QString ext = radioButton2D->isChecked() ? "2d" : "";
    QString funcName = lineEditFunctionName->text().trimmed();

    QDir dir(pathFIF);
    dir.remove(funcName + ".o");

#ifdef Q_OS_WIN
    dir.remove(funcName + ".dll" + ext);
#elif defined(Q_OS_MAC)
    dir.remove(funcName + ".dylib" + ext);
#else
    dir.remove(funcName + ".so" + ext);
#endif

    QString script = QDir::cleanPath(pathFIF + "/compile.script");
#ifdef Q_OS_WIN
    script += ".ps1";
#endif

    procc = new QProcess(qApp);

    if (!compileAllYN)
        toResLog("\n<< compile >>\n");

    connect(procc, &QProcess::readyReadStandardError, this, &compile18::readFromStdout);

    // Start platform-specific process
#ifdef Q_OS_WIN
    procc->start("powershell.exe",
                 QStringList() << "-ExecutionPolicy" << "Bypass" << "-File" << QDir::toNativeSeparators(script));
#elif defined(Q_OS_MAC)
    procc->start("/bin/zsh", QStringList() << "-c" << script);
#else
    procc->start("/bin/bash", QStringList() << "-c" << script);
#endif

    procc->waitForFinished();

    QString soName = fitPath->text() + "/" + funcName + ".";
#ifdef Q_OS_MAC
    soName += "dylib";
#elif defined(Q_OS_WIN)
    soName += "dll";
#else
    soName += "so";
#endif

    if (!ext.isEmpty())
        soName += ext;

    soName = QDir::cleanPath(soName);

    if (QFile::exists(soName))
    {
        QString okMsg = "<< compile status >> OK: function '" + funcName + "' is ready\n";
        toResLog(okMsg);
        app()->d_status_info->setText(okMsg.trimmed());
    }
    else
    {
        QString errMsg = QString("<< compile status >> ERROR: check code/options/file not created: %1 / script: %2\n")
                             .arg(soName, script);

        toResLog(errMsg);
        app()->d_status_info->setText("<< compile status >>  ERROR: check function code / compiler options");
    }

    QString objName = funcName + ".o" + ext;
    if (dir.exists(objName))
        dir.remove(objName);
}
// +++  compile via test interface
void compile18::compileTest()
{
    QString ext = radioButton2D->isChecked() ? "2d" : "";
    QString funcName = lineEditFunctionName->text().trimmed();
    QString script = pathFIF + "compile.script";
#if defined(Q_OS_WIN)
    script += ".ps1";
#endif

    QDir dir(pathFIF);
    dir.remove(funcName + ".o");
#ifdef Q_OS_WIN
    dir.remove(funcName + ".dll" + ext);
#elif defined(Q_OS_MAC)
    dir.remove(funcName + ".dylib" + ext);
#else
    dir.remove(funcName + ".so" + ext);
#endif

    procc = new QProcess(qApp);
    toResLog("\n<< compile >>\n");

    connect(procc, &QProcess::readyReadStandardError, this, &compile18::readFromStdout);

#ifdef Q_OS_WIN
    procc->start("powershell.exe",
                 QStringList() << "-ExecutionPolicy" << "Bypass" << "-File" << QDir::toNativeSeparators(script));
#elif defined(Q_OS_MAC)
    procc->start("/bin/zsh", QStringList() << "-c" << script);
#else
    procc->start("/bin/bash", QStringList() << "-c" << script);
#endif

    procc->waitForFinished();

    QString stdOutput = procc->readAllStandardOutput();
    toResLog(stdOutput);

    // Construct output filename
    QString soName = fitPath->text() + funcName + ".";

#ifdef Q_OS_MAC
    soName += "dylib";
#elif defined(Q_OS_WIN)
    soName += "dll";
#else
    soName += "so";
#endif

    if (radioButton2D->isChecked())
        soName += "2d";

    if (QFile::exists(soName))
        toResLog("<< compile status >> OK: function '" + funcName + "' is ready\n");
    else
        toResLog("<< compile status >>  ERROR: check function code / compiler options\n");

    QString objectName = funcName + ".o" + ext;
    if (dir.exists(objectName))
        dir.remove(objectName);
}

// +++  check Function Name
void compile18::checkFunctionName()
{
    QString name = lineEditFunctionName->text();

    QString cleanName;
    for (QChar c : name)
    {
        if (c.isLetterOrNumber() || c == '-' || c == '/')
            cleanName.append(c);
        else if (c == '_')
            cleanName.append('-');
    }

    cleanName = cleanName.trimmed();
    lineEditFunctionName->setText(cleanName);
    textLabelInfoSAS->setText(cleanName);
}
// +++  read From Stdout
void compile18::readFromStdout()
{
    QString s = procc->readAllStandardError();
    QStringList lst = s.split("\n", Qt::SkipEmptyParts);

    QString errors;
    for (const QString &line : lst)
        if (line.contains("error:"))
            errors += line + "\n";
    errors.replace("‘", "`").replace("’", "'");

    toResLog(errors);
}
// +++  compile all functions
void compile18::compileAll()
{
    int numberFunctions = listViewFunctions->model()->rowCount();
    if (numberFunctions == 0)
        return;

    QProgressDialog progress("Compile All Functions in Functions Window ...", "Abort Compilation", 0,
                             numberFunctions + 1, app()->isHidden() ? nullptr : this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setValue(0);

    listViewFunctions->selectionModel()->blockSignals(true);
    for (int i = 0; i < numberFunctions; i++)
    {
        QModelIndex idx = listViewFunctions->model()->index(i, 0);
        listViewFunctions->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);

        buildSharedLibrary(true);

        progress.setValue(i + 1);
        progress.setLabelText("Function :: " + lineEditFunctionName->text() + " is ready");
        if (progress.wasCanceled())
            break;
    }
    listViewFunctions->selectionModel()->blockSignals(false);
}