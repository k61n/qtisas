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

//*******************************************
//+++  new Function Name
//*******************************************
void compile18::newFunctionName(){

    if (listBoxFunctionsNew->selectionModel()->selectedRows().count()==0) 
        return;
    
    if ( lineEditFunctionName->text()!=listBoxFunctionsNew->selectionModel()->selectedRows()[0].data().toString()) 
        listBoxFunctionsNew->clearSelection();
}
//*******************************************
//+++  new Categoty Name
//*******************************************
void compile18::newCategoryName(){

    if (listBoxGroupNew->selectionModel()->selectedRows().count()==0) 
        return;
    if (listBoxGroupNew->selectionModel()->selectedRows()[0].data().toString()=="ALL") 
        return;
    
    if ( lineEditGroupName->text() != listBoxGroupNew->selectionModel()->selectedRows()[0].data().toString()){
        listBoxGroupNew->clearSelection();
        listBoxFunctionsNew->model()->removeColumn(0);
    }
}
//*******************************************
//+++  stot 1D to 2D
//*******************************************
void compile18::stot1Dto2D(){

    if (radioButton1D->isChecked()){
        spinBoxXnumber->setMinimum(1);
        spinBoxXnumber->setValue(1);
        spinBoxXnumber->setEnabled(false);
        lineEditXXX->setText("x");
        //pushButtonMenuSANS->show(); // 2023.todo
        spinBoxXnumber->hide();
    } else{
        spinBoxXnumber->setMinimum(2);
        spinBoxXnumber->setValue(2);
        spinBoxXnumber->setEnabled(true);
        lineEditXXX->setText("ix,iy");
        pushButtonMenuSANS->hide();
        spinBoxXnumber->show();
    }

    scanGroups();
    
    textEditCode->clear();

    changedFXYinfo();
}
//*******************************************
//+++ path Changed
//*******************************************
void compile18::pathChanged(){
    
    scanGroups();
    scanIncludedFunctions();
}
//*******************************************
//+++ scan FitFunction dir on groups
//*******************************************
void compile18::scanGroups(){

    QStringList group;
    QString s, groupName;

    QDir d(pathFIF);

    QString fifExt = radioButton2D->isChecked() ? "*.2dfif" : "*.fif";

    QStringList lst = FunctionsExplorer::scanFiles(pathFIF, fifExt, false);

    for(int i=0; i<lst.count(); i++){

        if (d.exists(lst[i])){

            QFile f(pathFIF+"/"+lst[i]);
            f.open( QIODevice::ReadOnly );
            QTextStream t( &f );  

            //+++[group]
            s = t.readLine();
            if (s.contains("[group]")){

                groupName=t.readLine().trimmed();
                if ( !group.contains(groupName) && groupName!="" ) 
                    group << groupName;
            }
            
            f.close();
        }
    }
    group.sort();

    group << FunctionsExplorer::scanSubfolders(pathFIF, fifExt);
    group.prepend("ALL");
    
    lineEditGroupName->setText("");
    lineEditFunctionName->setText("");
    if (textLabelInfoVersion->isHidden())
        textLabelInfoSAS->setText("");
    
    listBoxGroupNew->setModel(new QStringListModel(group));

    connect(listBoxGroupNew->selectionModel(),SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),this, SLOT(groupFunctions(const QModelIndex &, const QModelIndex &)));
    
    QStringList empty;
    listBoxFunctionsNew->setModel(new QStringListModel(empty));
}
//*******************************************
//+++  scan Included Functions
//*******************************************
void compile18::scanIncludedFunctions(){

    QString s, fileName;    
    QStringList group;
    
    QDir d(pathFIF+"/IncludedFunctions");
    QStringList lst = d.entryList(QStringList() << "*.h");
    lst += d.entryList(QStringList() << "*.cpp");
    
    QStringList empty;
    listBoxIncludeFunctionsNew->setModel(new QStringListModel(empty));
    listBoxIncludeFunctionsNew->setModel(new QStringListModel(lst));
    
    connect(listBoxIncludeFunctionsNew,SIGNAL(doubleClicked(const QModelIndex &)),this, SLOT(addIncludedFunction(const QModelIndex &)));
}
//*******************************************
//+++  fing functions of single Group
//*******************************************
void compile18::groupFunctions(const QModelIndex &index, const QModelIndex &p)
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

    listBoxFunctionsNew->setModel(new QStringListModel(functions));
    
    connect(listBoxFunctionsNew->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),
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
//*******************************************
//+++  find functions of single Group
//*******************************************
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
            QFile f(pathFIF + "/" + lst[i]);
            f.open( QIODevice::ReadOnly );
            QTextStream t(&f);

            if (t.readLine().contains("[group]") && (groupName == "ALL" || t.readLine().trimmed() == groupName))
                functions << lst[i].remove(fifExt);

            f.close();
        }
    }
    return functions;
}
//*******************************************
//+++ open FIF: from listbox
//*******************************************
void compile18::openFIFfileSimple(const QModelIndex &index, const QModelIndex &prev)
{
    QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";
    QString file = index.data().toString();

    textLabelInfoVersion->hide();
    textLabelInfoSASauthor->hide();

    pathUpdate();

    QString ss = pathFIF + "/" + file + fifExt;
    openFIFfile(ss);
}
//*******************************************
//+++  open FIF: from listbox
//*******************************************
void compile18::openFIFfileSimple()
{
    QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";

    textLabelInfoVersion->hide();
    textLabelInfoSASauthor->hide();

    pathUpdate();

    if (listBoxFunctionsNew->selectionModel()->selectedRows().count() > 0)
        openFIFfile(pathFIF + "/" + listBoxFunctionsNew->selectionModel()->selectedRows()[0].data().toString() +
                    fifExt);
}

QString generateTextFromList(QStringList lst)
{
    QString txt;
    foreach (const QString &block, lst)
        txt += block + "\n";
    if (!txt.isEmpty())
        txt.chop(1);
    return txt;
}
bool checkBlock(QStringList lst, QString codeName, bool hideMessageBox = false)
{
    if (lst.count() < 1 || !lst[0].contains(codeName))
    {
        if (!hideMessageBox)
            QMessageBox::warning(0, "QtiSAS", QString("Error: %1").arg(codeName));
        return false;
    }
    return true;
}
//*******************************************
//+++ open FIF: code
//*******************************************
void compile18::openFIFfile(const QString &fifName)
{
    QTextCursor cursor = textEditHFiles->textCursor();
    cursor.movePosition(QTextCursor::End);
    textEditHFiles->setTextCursor(cursor);

    textEditHFiles->clear();

    cursor = textEditCode->textCursor();
    cursor.movePosition(QTextCursor::End);

    textEditCode->setTextCursor(cursor);
    textEditCode->clear();

    cursor = textEditFunctions->textCursor();
    cursor.movePosition(QTextCursor::End);

    textEditFunctions->setTextCursor(cursor);
    textEditFunctions->clear();

    cursor = textEditForwardFortran->textCursor();
    cursor.movePosition(QTextCursor::End);

    textEditForwardFortran->setTextCursor(cursor);
    textEditForwardFortran->clear();

    if (fifName.contains(".2dfif"))
        radioButton2D->setChecked(true);

    spinBoxP->setValue(0);

    if (fifName.isEmpty())
    {
        QMessageBox::warning(this, "QtiSAS", "Error: <p> no FIF file, 1");
        return;
    }

    if (!QFile::exists(fifName))
    {
        QMessageBox::warning(this, "QtiSAS", "Error: <p> no FIF file, 2");
        return;
    }

    QFile f(fifName);

    if (!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(0, "QtiSAS",
                              QString("Could not write to file: <br><h4>" + fifName +
                                      "</h4><p>Please verify that you have the right to read from this location!")
                                  .toLocal8Bit()
                                  .constData());
        return;
    }

    QTextStream in(&f);
    QString content = in.readAll();
    if (!content.contains("\n\n[included functions]"))
        content.replace("\n[included functions]", "\n\n[included functions]"); // to support reading of old fif-files
    content.replace("\n\n[", "..:Splitterr:..[");
    f.close();

    QStringList listOfBlocks = content.split("..:Splitterr:..");

    QStringList lst;
    QString s;

    //+++[group]
    lst = listOfBlocks[0].split("\n");
    if (!checkBlock(lst, "[group]"))
        return;

    s = lst[0];
    s.remove("[group] ").remove("[group]");

    //+++[group]: [eFit]
    checkBoxEfit->setChecked(s.contains("[eFit]"));
    s.remove("[eFit] ");
    if (s.contains("["))
    {
        lineEditEFIT->setText(s.left(s.indexOf("[")));
        s = s.right(s.length() - s.indexOf("["));
    }
    else
        lineEditEFIT->setText(s);

    //+++[group]: [Weight]
    checkBoxWeight->setChecked(s.contains("[Weight]"));
    if (s.contains("[Weight]"))
    {
        s.remove("[Weight] ");
        comboBoxWeightingMethod->setCurrentIndex(s.left(1).toInt());
        s = s.right(s.length() - 2);
    }
    else
        comboBoxWeightingMethod->setCurrentIndex(0);

    //+++[group]: [Superpositional]
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

    //+++[group]: [Algorithm]
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

    //+++group Name
    QString groupName = lst[1].trimmed();
    if (groupName == "")
        groupName = "ALL";
    lineEditGroupName->blockSignals(true);
    lineEditGroupName->setText(groupName);
    lineEditGroupName->blockSignals(false);

    //+++[name]
    lst = listOfBlocks[1].split("\n");
    if (!checkBlock(lst, "[name]"))
        return;

    //+++ function Name
    QString functionName = lst[1].trimmed();
    if (functionName == "")
    {
        QMessageBox::warning(this, "QtiSAS", "Error: [name]");
        return;
    }
    lineEditFunctionName->setText(functionName);
    textLabelInfoSAS->setText(functionName);

    //+++[number parameters]
    lst = listOfBlocks[2].split("\n");
    if (!checkBlock(lst, "[number parameters]"))
        return;

    //+++[number parameters]: pNumber
    int pNumber = lst[1].remove(",").trimmed().toInt();
    if (pNumber <= 0)
    {
        QMessageBox::warning(this, "QtiSAS", "Error: [number parameters]");
        return;
    }
    spinBoxP->setValue(pNumber);

    //+++[description]
    lst = listOfBlocks[3].split("\n");
    if (!checkBlock(lst, "[description]"))
        return;
    textEditDescription->clear();
    textEditDescription->insertHtml(generateTextFromList(lst.mid(1)));

    //+++[x]
    lst = listOfBlocks[4].split("\n");
    if (!checkBlock(lst, "[x]"))
        return;
    if (radioButton2D->isChecked())
    {
        QStringList sLst = lst[1].remove(" ").split(",", Qt::SkipEmptyParts);
        spinBoxXnumber->setValue(sLst.count());
    }
    lineEditXXX->setText(lst[1].remove(" "));

    //+++[y]
    lst = listOfBlocks[5].split("\n");
    if (!checkBlock(lst, "[y]"))
        return;
    lineEditY->setText(lst[1].trimmed());

    //+++[parameter names]
    lst = listOfBlocks[6].split("\n");
    if (!checkBlock(lst, "[parameter names]"))
        return;

    //+++[parameter names]: paraNames
    QStringList paraNames = lst[1].split(",", Qt::SkipEmptyParts);
    if (paraNames.size() != pNumber)
        QMessageBox::warning(this, "QtiSAS", "Error: [parameter names]");

    //+++[initial values]
    lst = listOfBlocks[7].split("\n");
    if (!checkBlock(lst, "[initial values]"))
        return;

    //+++[initial values]: initValues
    QStringList initValues = lst[1].split(",", Qt::SkipEmptyParts);
    if (initValues.size() != pNumber)
        QMessageBox::warning(this, "QtiSAS", "Error: [initial values]");

    //+++[adjustibility]
    lst = listOfBlocks[8].split("\n");
    if (!checkBlock(lst, "[adjustibility]"))
        return;

    //+++[adjustibility]: adjustibilityList
    QStringList adjustibilityList = lst[1].split(",", Qt::SkipEmptyParts);
    if (adjustibilityList.size() != pNumber)
        QMessageBox::warning(this, "QtiSAS", "Error: [adjustibility]");

    //+++[parameter description]
    lst = listOfBlocks[9].split("\n");
    if (!checkBlock(lst, "[parameter description]"))
        return;

    //+++[parameter description]: paraDescription
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

    //+++ [h-headers]
    lst = listOfBlocks[10].split("\n");
    if (!checkBlock(lst, "[h-headers]"))
        return;
    textEditHFiles->append(generateTextFromList(lst.mid(1)));

    //+++ [included functions]
    lst = listOfBlocks[11].split("\n");
    if (!checkBlock(lst, "[included functions]"))
        return;
    textEditFunctions->append(generateTextFromList(lst.mid(1)));
    textEditFunctions->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);

    //+++[code]
    lst = listOfBlocks[12].split("\n");
    if (!checkBlock(lst, "[code]"))
        return;
    textEditCode->clear();
    textEditCode->append(generateTextFromList(lst.mid(1)));
    textEditCode->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);

    //+++[fortran]
    checkBoxAddFortran->setChecked(false);
    fortranFunction->setText("");
    
    if (listOfBlocks.count() > 13)
    {
        lst = listOfBlocks[13].split("\n");

        if (checkBlock(lst, "[fortran]") && lst.count() > 2 && lst[1].contains("1"))
        {
            checkBoxAddFortran->setChecked(true);
            fortranFunction->setText(lst[2]);
            textEditForwardFortran->append(generateTextFromList(lst.mid(3)));
        }
    }

    //+++[python]
    checkBoxIncludePython->setChecked(false);
    if (listOfBlocks.count() > 14)
    {
        lst = listOfBlocks[14].split("\n");
        if (checkBlock(lst, "[python]", true))
            checkBoxIncludePython->setChecked(lst[1].contains("1"));
    }

    //+++[after.fit: python]
    checkBoxAfterFitPython->setChecked(false);
    textEditAfterFitPython->clear();

    if (listOfBlocks.count() > 15)
    {
        lst = listOfBlocks[15].split("\n");
        if (checkBlock(lst, "[after.fit: python]", true))
            checkBoxAfterFitPython->setChecked(lst[1].contains("1"));
    }

    //+++[after.fit: python code]
    if (checkBoxAfterFitPython->isChecked() && listOfBlocks.count() > 16)
    {
        lst = listOfBlocks[16].split("\n");
        if (checkBlock(lst, "[after.fit: python code]", true))
            textEditAfterFitPython->append(generateTextFromList(lst.mid(1)));
    }

    radioButtonCPP->setText(lineEditFunctionName->text() + ".cpp");
    radioButtonFIF->setText(lineEditFunctionName->text() + ".fif");

    updateFiles(true);

    makeCPP();
}
//*******************************************
//+++  make CPP file
//*******************************************
void compile18::makeCPP()
{
    QString fn=pathFIF+"/"+lineEditFunctionName->text().trimmed()+".cpp";
    
    if (lineEditFunctionName->text().trimmed()==""){
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> Fill Function name!"));
        return;
    }
    saveAsCPP(fn);
}
//*******************************************
//+++  make compile.script.bat file
//*******************************************
void compile18::makeCompileScript()
{
    QString ext = FunctionsExplorer::filterShared().remove("*");
    if (radioButton2D->isChecked())
        ext += "2d";

    pathFIF = fitPath->text();
    pathMinGW = mingwPathline->text();

    QString pathGSL = "";
#if defined(Q_OS_WIN)
    pathGSL = qApp->applicationDirPath();
#elif defined(Q_OS_MAC)
    QDir dd;
    dd.setPath(qApp->applicationDirPath());
    dd.cdUp();
    pathGSL = dd.absolutePath();
#endif

    QString fn = pathFIF + "/compile.script.bat";
    QString script = "";
    QString fortranText = ""; // fortran o-file
    QString compileFlags = lineEditCompileFlags->text();
    QString linkFlags = lineEditLinkFlags->text();
    QString functionName = lineEditFunctionName->text().trimmed();

#if defined(Q_OS_WIN)
    fn = fn.replace("\/", "\\");
    pathFIF = pathFIF.replace("\/", "\\");
    pathMinGW = pathMinGW.replace("\/", "\\");
    pathGSL = pathGSL.replace("\/", "\\");

    if (pathFIF.contains(":"))
        script += pathFIF.left(pathFIF.indexOf(":") + 1) + "\n";

    script += "cd " + QString("\"") + pathFIF + "\"" + "\n";

    //+++ %COMPILER%
    script += "set MINGW_IN_SHELL=1\n";
    script += "set COMPILER=" + pathMinGW + "\n";
    script += "set PATH=%COMPILER%/bin;%PATH%\n";

    //+++ %GSL%
    script += "set GSL= \"" + pathGSL + "\"\n";

    compileFlags = compileFlags.replace("$GSL", "%GSL%");
    linkFlags = linkFlags.replace("$GSL", "%GSL%");
    linkFlags = linkFlags.replace("$COMPILER", "%COMPILER%");
#else
    script += "cd \"" + pathFIF + "\"\n";
#endif

#if defined(Q_OS_MAC)
    script += "GSL=" + QString("\"") + pathGSL + "\"" + "\n";
    script += "export LIBRARY_PATH=$GSL/Frameworks/:$LIBRARY_PATH\n";
    script += "export CPLUS_INCLUDE_PATH=$GSL/Resources/:$CPLUS_INCLUDE_PATH\n";
#endif

    //+++ PYTHON
    if (checkBoxIncludePython->isChecked())
    {
#if defined(Q_OS_WIN)

        script += "for /f \"delims=\" %%I in ('python -c \"import sysconfig; print(sysconfig.get_path('include'))\"')"
                  " do set PYTHON_INCLUDE_PATH=%%I\n";
        script += "for /f \"delims=\" %%V in ('python -c \"import sys; print(sys.version_info.major * 100 +"
                  " sys.version_info.minor)\"') do set PYTHON_VERSION=%%V\n";
        script += "set PYTHON_LIB=python%PYTHON_VERSION%\n";
        script += "set PYTHON_LIBS_PATH=%PYTHON_INCLUDE_PATH:include=libs%\n";

        compileFlags += " -I\"%PYTHON_INCLUDE_PATH%\"";
        linkFlags += " -L\"%PYTHON_LIBS_PATH%\" -l%PYTHON_LIB%";
#elif defined(Q_OS_MAC)

        script += "export PATH=\"/usr/bin:$PATH\"\n";
        script += "export PYTHON_VERSION=$(python3 -c \"import sys;"
                  " print(f'{sys.version_info.major}.{sys.version_info.minor}')\")\n";
        script +=
            "export PYTHON_PATH=\""
            "/Library/Developer/CommandLineTools/Library/Frameworks/Python3.framework/Versions/$PYTHON_VERSION\"\n";

        compileFlags += " -I\"$PYTHON_PATH/Headers/\"";

        linkFlags += " -L\"$PYTHON_PATH/lib/python$PYTHON_VERSION/config-$PYTHON_VERSION-darwin\""
                     " -lpython$PYTHON_VERSION -ldl -framework CoreFoundation";
#else
        compileFlags += " $(python3-config --includes)";
        linkFlags += " $(python3-config --ldflags --embed)";
#endif
    }

    script += compileFlags + " " + functionName + ".cpp -o " + functionName + ".o\n";

    if (checkBoxAddFortran->isChecked())
    {
        QString gfortranlib = "";

#if defined(Q_OS_WIN)

        script +=
            compileFlags + " " + "\"" + fortranFunction->text() + "\"" + " -o " + "\"" + "fortran.o" + "\"" + " \n";
#elif defined(Q_OS_MAC)

        script += "gfortranSTR=$(which gfortran)" + QString("\n");
        compileFlags =
            compileFlags.replace("clang ", "$gfortranSTR ").remove(" -I$GSL").remove("/Resources").remove("/include");
        script += compileFlags + "  " + fortranFunction->text() + " -o " + "fortran.o" + "\n";
        script += "gfortranPath=\"$(dirname `$gfortranSTR --print-file-name libgfortran.a`)\";";
        script += "gfortranPath=${gfortranPath%x*4}\n";
        gfortranlib = " -L$gfortranPath -lgfortran ";
#else

        script += "gfortranSTR=$(which $(compgen -c gfortran | tail -n 1))" + QString("\n");
        compileFlags = compileFlags.replace("g++ ", "$gfortranSTR ").remove(" -I$GSL").remove("/include");
        script += compileFlags + "  " + fortranFunction->text() + " -o " + "fortran.o" + "\n";
#endif

        fortranText = gfortranlib + QString(" fortran.o");
    }

    linkFlags = linkFlags.replace(" -o", "  -o " + functionName + ext + "  " + functionName + ".o" + fortranText);
    script += linkFlags + "  ";

#if defined(Q_OS_WIN)

    if (checkBoxAddFortran->isChecked())
        script += "-static-libgfortran  ";
#endif

    script += "\n";

    if (checkBoxAddFortran->isChecked())
    {
#if defined(Q_OS_WIN)

        script += "del \" fortran.o \" \n";
#else

        script += "rm fortran.o\n";
#endif
    }

#if defined(Q_OS_WIN)

    script += "del " + functionName + ".o\n";
    script.replace("/", "\\").replace("\\f", "/f");
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

//*******************************************
//+++  save FIF file /slot
//*******************************************
void compile18::makeFIF()
{
    QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";
    QString group = lineEditGroupName->text().trimmed();
    QString name = lineEditFunctionName->text().trimmed() + fifExt;

    QString nameOld = "";
    if (listBoxFunctionsNew->selectionModel()->selectedRows().count() > 0)
        nameOld = listBoxFunctionsNew->selectionModel()->selectedRows()[0].data().toString() + fifExt;

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
 
    if (nameOld!=name){
        scanGroups();
        const QModelIndexList indexes = listBoxGroupNew->model()->match(listBoxGroupNew->model()->index(0,0),Qt::DisplayRole,group,1,Qt::MatchExactly);
        if (indexes.count()>0) 
            listBoxGroupNew->selectionModel()->select(indexes[0], QItemSelectionModel::Select);
        
        groupFunctions(indexes[0],indexes[0]);
        
        const QModelIndexList indexesFunctions = listBoxFunctionsNew->model()->match(listBoxFunctionsNew->model()->index(0,0),Qt::DisplayRole,name.remove(fifExt),1,Qt::MatchExactly);
        if (indexesFunctions.count()>0) 
            listBoxFunctionsNew->selectionModel()->select(indexesFunctions[0], QItemSelectionModel::Select);
        
        openFIFfileSimple(indexesFunctions[0],indexesFunctions[0]);
    }
}
//*******************************************
//+++  delete FIF file
//*******************************************
void compile18::deleteFIF()
{
    QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";
    QString fn = listBoxFunctionsNew->selectionModel()->selectedRows()[0].data().toString().trimmed() + fifExt;
    if (fn == "")
        return;

    int functionCount = listBoxFunctionsNew->model()->rowCount();

    if (QMessageBox::question(this, tr("QtiSAS::Delete Function?"), tr("Do you want to delete Function %1?").arg(fn),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        return;

    QDir d(pathFIF);
    d.remove(fn);
    fn = fn.remove(fifExt, Qt::CaseInsensitive);
    d.remove(fn + ".cpp");
    // remove shared objects
    d.remove(fn + ".dll");
    d.remove(fn + ".so");
    d.remove(fn + ".dylib");

    QString subFolder = FunctionsExplorer::subFolder(fn);
    QString group = lineEditGroupName->text().trimmed();
    if (listBoxGroupNew->selectionModel()->selectedIndexes().count() > 0)
        group = listBoxGroupNew->selectionModel()->selectedRows()[0].data().toString().trimmed();

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

    const QModelIndexList indexes = listBoxGroupNew->model()->match(listBoxGroupNew->model()->index(0,0),Qt::DisplayRole,group,1,Qt::MatchExactly);

    newFIF();

    if (functionCount>1){
        if (indexes.count()>0) listBoxGroupNew->selectionModel()->select(indexes[0], QItemSelectionModel::Select);
        groupFunctions(indexes[0],indexes[0]);
    }
}
//*******************************************
//+++  save FIF file /function
//*******************************************
bool compile18::save( QString fn, bool askYN ){
    int i;
    QString text;
    
    if (spinBoxP->value()==0){
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> Check function!"));
        return false;
    }
    
    if (lineEditFunctionName->text().trimmed()==""){
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> Fill Function name!"));
        return false;
    }

    if (askYN && QFile::exists(fn) &&
        QMessageBox::question(this, tr("QtiSAS -- Overwrite File? "),
                              tr("A file called: <p><b>%1</b><p>already exists.\n"
                                 "Do you want to overwrite it?")
                                  .arg(fn),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        return false;
    else {
        text+="[group]";
        
        if (checkBoxEfit->isChecked()) text+=" [eFit] "+ lineEditEFIT->text();
        if (checkBoxWeight->isChecked()) text+=" [Weight] "+QString::number(comboBoxWeightingMethod->currentIndex());
        if (checkBoxSuperpositionalFit->isChecked()) text+=" [Superpositional] "+QString::number(spinBoxSubFitNumber->value());
        if (checkBoxAlgorithm->isChecked()) text+=" [Algorithm] "+QString::number(comboBoxFitMethod->currentIndex()) +" "+ lineEditFitMethodPara->text();
        
        text+="\n";
        
        text+=lineEditGroupName->text().trimmed();
        text+="\n\n";
        
        text+="[name]\n";
        text+=lineEditFunctionName->text().trimmed();
        text+="\n\n";
        
        text+="[number parameters]\n";
        text+=QString::number(spinBoxP->value())+",";
        text+="\n\n";
        
        text+="[description]\n";
        QString htmlText=textEditDescription->toHtml();
        htmlText="<html><head><meta name= \"qrichtext \" content= \"1 \" /></head>"+htmlText.right(htmlText.length()-htmlText.indexOf("<body"));
        text+=htmlText;//text();
        text+="\n\n";
        
        text+="[x]\n";
        text+=lineEditXXX->text().remove(" ");
        text+="\n\n";
        
        text+="[y]\n";
        if (lineEditY->text().trimmed()!="") text+=lineEditY->text().trimmed(); else text+="I";
        text+="\n\n";
        
        text+="[parameter names]\n";
        i=0;
        if (spinBoxP->value()>1){
            for(i=0;i<(spinBoxP->value()-1);i++){
                if (tableParaNames->item(i,0)->text()=="") tableParaNames->item(i,0)->setText("P"+QString::number(i+1));
                text=text+tableParaNames->item(i,0)->text().trimmed()+',';
            }
        }
        
        if (tableParaNames->item(i,0)->text()=="") tableParaNames->item(i,0)->setText("P"+QString::number(i+1));
        text=text+tableParaNames->item(i,0)->text().trimmed()+',';
        text+="\n\n";
        
        text+="[initial values]\n";

        for(i=0;i<spinBoxP->value();i++){
            QString valueCurrent=tableParaNames->item(i,1)->text().remove(" ");
            QString limitCurrent=tableParaNames->item(i,2)->text().remove(" ");
            
            if (limitCurrent=="..") limitCurrent="";
            else if (limitCurrent.contains("..")){
                if (limitCurrent.endsWith("..")) limitCurrent+="inf";
                else if (limitCurrent.startsWith("..")) limitCurrent="-inf"+limitCurrent;
                limitCurrent="["+limitCurrent+"]";
                
            }
            text=text+QString::number(valueCurrent.toDouble())+limitCurrent;
            if (i<(spinBoxP->value()-1)) text=text+','; else text+="\n\n";
        }

        text+="[adjustibility]\n";
        for(i=0;i<(spinBoxP->value()-1);i++){
            if (tableParaNames->item(i,2)->checkState()==Qt::Checked) text=text+'1'+','; else text=text+'0'+',';
        }
        if (tableParaNames->item(i,2)->checkState()==Qt::Checked) text=text+'1'; else text=text+'0';
        text+="\n\n";
        
        text+="[parameter description]\n";
        i=0;
        if (spinBoxP->value()>1){
            for(i=0;i<(spinBoxP->value()-1);i++){
                if (tableParaNames->item(i,3)->text().trimmed()=="") tableParaNames->item(i,3)->setText("---");
                text=text+tableParaNames->item(i,3)->text()+",";
            }
        }
        if (tableParaNames->item(i,3)->text().trimmed()=="") tableParaNames->item(i,3)->setText("---");
        text=text+tableParaNames->item(i,3)->text()+",";
        text+="\n\n";
        
        text+="[h-headers]\n";
        text+=textEditHFiles->toPlainText();
        text += "\n\n";
        
        text+="[included functions]\n";
        text+=textEditFunctions->toPlainText();
        text+="\n\n";
        
        text+="[code]\n";
        text+=textEditCode->toPlainText();
        text+="\n\n";
        
        text+="[fortran]\n";
        if (checkBoxAddFortran->isChecked()) text+="1,"; else text+="0,";
        text+="\n";
        text+=fortranFunction->text();
        text+="\n";
        text+=textEditForwardFortran->toPlainText();
        text+="\n\n";

        text += "[python]\n";
        if (checkBoxIncludePython->isChecked())
            text += "1,";
        else
            text += "0,";
        text += "\n\n";

        text += "[after.fit: python]\n";
        if (checkBoxAfterFitPython->isChecked())
            text += "1,";
        else
            text += "0,";
        text += "\n\n";

        text += "[after.fit: python code]\n";
        text += textEditAfterFitPython->toPlainText();
        text += "\n\n";

        text+="[end]";
        
        QFile f(fn);
        if ( !f.open( QIODevice::WriteOnly ) ){
            QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                                  tr("Could not writ<e to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
            return false;
        }
        QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        t.setCodec("UTF-8");
#endif
        t << text;
        f.close();
        
        if ( radioButtonFIF->isChecked() ){
            tableCPP->setRowCount(0);
            QStringList lst = text.split("\n");
            tableCPP->setRowCount(lst.count());
            for (int ii=0; ii<lst.count();ii++) tableCPP->setItem(ii,0,new QTableWidgetItem(lst[ii]));
        }
    }
    
    return true;
}

QString ampersand_encode(const QString& str){    
    QString chr;
    QStringList list = QStringList();
    for (int i = 0; i < str.size(); ++i) {
        chr = QString(str[i]);
        if (QString(chr.toLocal8Bit()) != chr)
            list << "&#" + QString::number(chr[0].unicode()) + ";";
        else list << chr;
    }
    return list.join("");
}
//*******************************************
//+++ saveAsCPP : new v. >2019-06
//*******************************************
void compile18::saveAsCPP( QString fn ){

    if (radioButton2D->isChecked()) return saveAsCPP2d(fn);
    return saveAsCPP1d(fn);
}
//*******************************************
//+++ saveAsCPP1d : new v. >2019-06
//*******************************************
void compile18::saveAsCPP1d( QString fn ){

    QString fifExt=".fif";
    int i;
    QString text;
    
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ do not change\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="#if defined(_WIN64) || defined(_WIN32) //MSVC Compiler\n#define MY_EXPORT __declspec(dllexport)\n#else\n#define MY_EXPORT\n#endif\n\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ h-files\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="#include <math.h>\n#include <iostream>\n#include <gsl/gsl_vector.h>\n#include <gsl/gsl_matrix.h>\n#include <gsl/gsl_math.h>\n";
    if (checkBoxIncludePython->isChecked())
        text += "#include <Python.h>\n";

    QStringList lstTmp=text.split("\n");
    
    lnTextEditHFiles->firstLineIncrement=lstTmp.count()-1; lnTextEditHFiles->updateLineNumbers(true);
    lstTmp.clear();
    
    QString textHF=textEditHFiles->toPlainText();
    QStringList lstHF=textHF.split("\n");
    
    for(i=0;i<lstHF.count();i++) if (lstHF[i].contains("#include")) text=text+lstHF[i]+"\n";
    
    text=text+"std::string fitFunctionPath="+'"';
    text += fitPath->text().toLocal8Bit().constData();
    text=text+'"' +";\n";
    text=text+"std::string OS="+'"';

#if defined(_WIN64) || defined(_WIN32)
    text+="WINDOWS";
#elif defined(Q_OS_MAC)
    text+="MAC";
#else
    text+="LINUX";
#endif
    text=text+'"' +";\n";

    text+="\n/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Parameters Structure\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    
    text+="#ifndef  TFUNCTION_H\n#define TFUNCTION_H\n";
    text+="struct functionT\n{\n";
    text+="  gsl_vector *para;\n";
    text+="  gsl_vector *para_limit_left;\n";
    text+="  gsl_vector *para_limit_right;\n";
    text+="  gsl_vector_int *para_fit_yn;\n";
    text+="  double *Q;\n";
    text+="  double *I;\n";
    text+="  double *dI;\n";
    text+="  double *SIGMA;\n";
    text+="  int *listLastPoints;\n";
    text+="  int currentM;\n";
    text+="  int currentFirstPoint;\n";
    text+="  int currentLastPoint;\n";
    text+="  int currentPoint;\n";
    text+="  bool polyYN;\n";
    text+="  int polyFunction;\n";
    text+="  bool beforeFit;\n";
    text+="  bool afterFit;\n";
    text+="  bool beforeIter;\n";
    text+="  bool afterIter;\n";
    text+="  double Int1;\n";
    text+="  double Int2;\n";
    text+="  double Int3;\n";
    text+="  int currentInt;\n";
    text+="  int prec;\n";
    text+="  char *tableName;\n";
    text+="  char **tableColNames;\n";
    text+="  int *tableColDestinations;\n";
    text+="  gsl_matrix *mTable;\n";
    text+="  int currentFunction;\n";
    text+="};\n";
    text+="#endif\n";
    text+="\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Global definition of parameters : new > 2019.06\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text += "double " + lineEditXXX->text().remove(" ").trimmed() + ";\n";
    text+="static double " + tableParaNames->item(0,0)->text().trimmed();
    for(i=1;i<spinBoxP->value();i++)
        text+=", "+tableParaNames->item(i,0)->text().trimmed();

    text+=";\n";
    text+="static bool initNow;\n";
    text+="\n";
    text+="#define Para ((struct functionT *) ParaM)->para\n";
    text+="\n";
    text+="#define XXX   ((struct functionT *) ParaM)->Q\n";
    text+="#define YYY   ((struct functionT *) ParaM)->I\n";
    text+="#define dYYY  ((struct functionT *) ParaM)->dI\n";
    text+="#define RESO  ((struct functionT *) ParaM)->SIGMA\n";
    text+="#define prec  ((struct functionT *) ParaM)->prec\n";
    text+="\n";
    text+="#define listLastPoints    ((struct functionT *) ParaM)->listLastPoints\n";
    text+="#define currentM          ((struct functionT *) ParaM)->currentM\n";
    text+="#define currentFirstPoint ((struct functionT *) ParaM)->currentFirstPoint\n";
    text+="#define currentLastPoint  ((struct functionT *) ParaM)->currentLastPoint\n";
    text+="#define currentPoint      ((struct functionT *) ParaM)->currentPoint\n";
    text+="#define currentInt        ((struct functionT *) ParaM)->currentInt\n";
    text+="#define currentFunction   ((struct functionT *) ParaM)->currentFunction\n";
    text+="\n";
    text+="#define beforeFit ((struct functionT *) ParaM)->beforeFit\n";
    text+="#define afterFit  ((struct functionT *) ParaM)->afterFit\n";
    text+="#define beforeIter ((struct functionT *) ParaM)->beforeIter\n";
    text+="#define afterIter ((struct functionT *) ParaM)->afterIter\n";
    text+="#define polyYN    ((struct functionT *) ParaM)->polyYN\n";
    text+="#define Int1    ((struct functionT *) ParaM)->Int1\n";
    text+="#define Int2    ((struct functionT *) ParaM)->Int2\n";
    text+="#define Int3    ((struct functionT *) ParaM)->Int3\n";
    text+="\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Function: read Parameters : new > 2019.06\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="void readParameters(void * ParaM)\n";
    text+="{\n";
    text+="  if (beforeFit || afterFit || beforeIter) initNow=true; else initNow=false;\n";

    text+="  \n";
    for(i=0;i<spinBoxP->value();i++){
        QString paraName=tableParaNames->item(i,0)->text().trimmed();
        text+="  "+paraName+"=gsl_vector_get(Para, "+QString::number(i)+");\n";
    }
    text+="}\n";
    text+="\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Function: save Parameters : new > 2019.06\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="void saveParameters(void * ParaM)\n";
    text+="{\n";
    text+="  if (!beforeFit && !afterFit && !beforeIter && !afterIter) return;\n";
    for(i=0;i<spinBoxP->value();i++)
        text=text+"  gsl_vector_set(Para," +  QString::number(i) + "," + tableParaNames->item(i,0)->text().trimmed()+");\n";
    text+="}\n";
    text+="\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Function returns Name of Function\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " + '"' + "C" + '"' + " MY_EXPORT char *name()\n";
    text+="{\n";
    text=text+"  return "+'"'+lineEditFunctionName->text()+'"'+";\n";
    text+="}\n";
    text+="\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Function return List of names of Parameters \n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *parameters()\n";
    text+="{\n";
    text=text+"  return "+'"'+tableParaNames->item(0,0)->text().trimmed();
    for(i=1;i<spinBoxP->value();i++) text=text+','+tableParaNames->item(i,0)->text().trimmed();
    text=text+','+lineEditXXX->text().trimmed()+'"'+";\n";
    text+="}\n";
    text+="\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Initial value of Parameters  \n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *init_parameters()\n";
    text+="{\n";
    text=text+"  return "+'"';
    for(i=0;i<spinBoxP->value();i++){
        QString valueCurrent=tableParaNames->item(i,1)->text();
        QString limitCurrent=tableParaNames->item(i,2)->text().remove(" ");
        
        if (limitCurrent=="..") 
            limitCurrent="";
        else 
            if (limitCurrent.contains("..")){
                
                if (limitCurrent.endsWith("..")) 
                    limitCurrent+="inf";
                else 
                    if (limitCurrent.startsWith("..")) 
                        limitCurrent="-inf"+limitCurrent;
                
                limitCurrent="["+limitCurrent+"]";
        }
        text=text+QString::number(valueCurrent.toDouble())+limitCurrent;
        if (i<(spinBoxP->value()-1)) text=text+','; else text=text+'"'+";\n";
    }

    text+="}\n";
    text+="\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Initial 'Adjustibility' of Parameters  \n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *adjust_parameters()\n";
    text+="{\n";
    text=text+"  return "+'"';
    for(i=0;i<(spinBoxP->value()-1);i++)
        if (tableParaNames->item(i,2)->checkState()==Qt::Checked) text=text+'1'+','; else text=text+'0'+',';
    
    if (tableParaNames->item(i,2)->checkState()) text=text+'1'; else text=text+'0';
    text=text+'"'+";\n";
    text+="}\n";
    text+="\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Number of parameters\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *paraNumber()\n";
    text+="{\n";
    text=text+"  return "+'"'+QString::number(spinBoxP->value()) + '"'+";\n";
    text+="}\n";
    text+="\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Description of Function and parameters\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *listComments()\n";
    text+="{\n";
    text+="  char  *list;\n";
    text+="  list=";
    
    QString htmlText=textEditDescription->toHtml().remove("\n").replace("\\;", "\\\\ \\\\;");;//text().remove("\n").replace("\\;", "\\\\ \\\\;");
    
    htmlText="<html><head><meta name= \"qrichtext \" content= \"1 \" /></head>"+htmlText.right(htmlText.length()-htmlText.indexOf("<body"));
    htmlText=ampersand_encode(htmlText);
    
    QString sss="";
    sss+="\\";
    sss+='"';
    QString richText=htmlText.replace('"',sss);

    if (richText=="") richText="---";
        
    text=text+'"'+richText+'"'+"\n";
    
    text=text+"\t\t\t"+'"';
    for(i=0;i<spinBoxP->value();i++) text=text+",,"+tableParaNames->item(i,3)->text();
    text=text+'"'+";\n return list;\n}\n\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Included Functions\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n\n";
    
    lstTmp=text.split("\n");
    
    lnTextEditFunctions->firstLineIncrement=lstTmp.count()-1; lnTextEditFunctions->updateLineNumbers(true);
    lstTmp.clear();
    
    text+=textEditFunctions->toPlainText();
    
    if (checkBoxAddFortran->isChecked()){
        text+="\n/////////////////////////////////////////////////////////////////////////////////\n";
        text+="//+++ Forward Declaration Fortran Functions\n";
        text+="/////////////////////////////////////////////////////////////////////////////////\n";
        text=text+"extern "+'"'+"C"+'"'+" \n{\n";
        text=text+textEditForwardFortran->toPlainText()+"\n}\n\n";
    }
    
    text+="\n\n/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Fit Function\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern "+'"'+"C"+'"'+" MY_EXPORT double functionSANS(double key";
    
    text=text+", void * ParaM)\n";
    text=text+"{\n";
    text=text+"//+++++++++++++++++++++++++++++++++++++++++++++\n";
    text = text + "\t " + lineEditXXX->text().remove(" ") + "=key;\n";
    text=text+"\t double " +lineEditY->text().trimmed()+";\n";
    text=text+"\t readParameters(ParaM);\n";
    text=text+"//+++++++++++++++++++++++++++++++++++++++++++++\n";
    text=text+"\t gsl_set_error_handler_off();\n";
    text=text+"//+++++++++++++++++++++++++++++++++++++++++++++\n\n";
    
    lstTmp=text.split("\n");
    
    lnTextEditCode->firstLineIncrement=lstTmp.count()-1; lnTextEditCode->updateLineNumbers(true);
    lstTmp.clear();
    
    text=text + textEditCode->toPlainText();
    text=text+"\n\n//+++++++++++++++++++++++++++++++++++++++++++++\n";
    text=text+"\t saveParameters(ParaM);\n";
    text=text+"\t return "+lineEditY->text().trimmed()+";\n";
    text=text+"//+++++++++++++++++++++++++++++++++++++++++++++\n";
    text=text+"}\n";
    
    QFile f(fn);
    if ( !f.open( QIODevice::WriteOnly ) ){
        QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                              tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
        return;
    }
    QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    t << text;
    f.close();
    
    if ( radioButtonCPP->isChecked() ){
        tableCPP->setRowCount(0);
        QStringList lst = text.split("\n");
        tableCPP->setRowCount(lst.count()+1);
        for (int ii=0; ii<lst.count();ii++) tableCPP->setItem(ii,0,new QTableWidgetItem(lst[ii]));
    }
}
//*******************************************
//+++ saveAsCPP1d : new v. >2019-06
//*******************************************
void compile18::saveAsCPP2d( QString fn ){

    int i;
    QString text;

    QString fifExt=".";
    if (radioButton2D->isChecked()) fifExt+="2d";
    fifExt+="fif";
    
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ do not change\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="#if defined(_WIN64) || defined(_WIN32) //MSVC Compiler\n#define MY_EXPORT __declspec(dllexport)\n#else\n#define MY_EXPORT\n#endif\n\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ h-files\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="#include <math.h>\n#include <iostream>\n#include <string>\n#include <gsl/gsl_vector.h>\n#include <gsl/gsl_matrix.h>\n#include <gsl/gsl_math.h>\n";
    //+++
    
    QStringList lstTmp=text.split("\n");
    
    lnTextEditHFiles->firstLineIncrement=lstTmp.count()-1; lnTextEditHFiles->updateLineNumbers(true);
    lstTmp.clear();
    
    QString textHF=textEditHFiles->toPlainText();
    QStringList lstHF=textHF.split("\n");
    
    for(i=0;i<lstHF.count();i++) if (lstHF[i].contains("#include")) text=text+lstHF[i]+"\n";
    text+="using namespace std;\n";
    
    text=text+"string fitFunctionPath="+'"';
    text += fitPath->text().toLocal8Bit().constData();
    text=text+'"' +";\n";
    text=text+"string OS="+'"';

#if defined(_WIN64) || defined(_WIN32)
    text+="WINDOWS";
#elif defined(Q_OS_MAC)
    text+="MAC";
#else
    text+="LINUX";
#endif
    text=text+'"' +";\n";
    
    //+++ NEW matrix v10
    if (radioButton2D->isChecked()){

        text+="\n/////////////////////////////////////////////////////////////////////////////////\n";
        text+="//+++ N-Dimensional Function Structure\n";
        text+="/////////////////////////////////////////////////////////////////////////////////\n";
        
        text+="#ifndef  NDFUNCTION_H\n#define NDFUNCTION_H\n\n";
        
        text+="struct functionND\n{\n";
        
        text+="double *Q;\n";
        text+="gsl_vector *para;\n";
        text+="size_t xNumber;\n";
        text+="size_t yNumber;\n";
        
        text+="size_t *Rows;\n";
        text+="size_t *Columns;\n";
        
        
        text+="gsl_matrix *I;\n";
        text+="gsl_matrix *dI;\n";
        text+="gsl_matrix *mask;\n";
        text+="gsl_matrix *xMatrix;\n";
        
        text+="bool beforeFit;\n";
        text+="bool afterFit;\n";
        text+="bool beforeIter;\n";
        text+="bool afterIter;\n";
        text+="int prec;\n";
        
        text+="char *tableName;\n";
        text+="char **tableColNames;\n";
        text+="int *tableColDestinations;\n";
        text+="gsl_matrix *mTable;\n";
        
        text+="};\n\n#endif\n ";
        
        text+="\n/////////////////////////////////////////////////////////////////////////////////\n";
        text+="//+++ Function returns list of x-Names\n";
        text+="/////////////////////////////////////////////////////////////////////////////////\n";
        text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *xName()\n{\n\treturn "+'"';
        text+=lineEditXXX->text();
        text=text+'"'+";\n}\n\n";
    
    } else{

        text+="\n/////////////////////////////////////////////////////////////////////////////////\n";
        text+="//+++ Parameters Structure\n";
        text+="/////////////////////////////////////////////////////////////////////////////////\n";
        
        text+="#ifndef  TFUNCTION_H\n#define TFUNCTION_H\n\n";
        
        text+="struct functionT\n{\n";
        
        text+="gsl_vector *para;\n";
        
        text+="double *Q;\n";
        text+="double *I;\n";
        text+="double *dI;\n";
        text+="double *SIGMA;\n";
        text+="double *SIGMAapplied;\n";
        
        text+="int currentFirstPoint;\n";
        text+="int currentLastPoint;\n";
        text+="int currentPoint;\n";
        
        text+="bool polyYN;\n";
        text+="int polyFunction;\n";
        
        text+="bool beforeFit;\n";
        text+="bool afterFit;\n";
        text+="bool beforeIter;\n";
        text+="bool afterIter;\n";
        text+="double Int1;\n";
        text+="double Int2;\n";
        text+="double Int3;\n";
        text+="int currentInt;\n";
        text+="int prec;\n";
        
        text+="char *tableName;\n";
        text+="char **tableColNames;\n";
        text+="int *tableColDestinations;\n";
        text+="gsl_matrix *mTable;\n";
        
        text+="};\n\n#endif\n ";    
    }
    
    text+="\n/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Function returns Name of Function\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *name()\n{\n\treturn "+'"';
    text+=lineEditFunctionName->text();
    text=text+'"'+";\n}\n\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Function return List of names of Parameters \n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *parameters()\n{\n\treturn "+'"';
    
    text=text+tableParaNames->item(0,0)->text().trimmed();
    
    for(i=1;i<spinBoxP->value();i++) text=text+','+tableParaNames->item(i,0)->text().trimmed();
    
    if (!radioButton2D->isChecked() )
        text=text+','+lineEditXXX->text().trimmed();
    
    text=text+'"'+";\n}\n\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Initial value of Parameters  \n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *init_parameters()\n{\n\treturn "+'"';
    
    for(i=0;i<(spinBoxP->value()-1);i++){
        
        QString sCurrent=tableParaNames->item(i,1)->text();
        if (sCurrent.contains('[') && sCurrent.contains("..") && sCurrent.contains(']'))
            text=text+sCurrent.remove(" ")+',';
        else
            text=text+QString::number(tableParaNames->item(i,1)->text().toDouble())+',';
    }
    
    QString sCurrent=tableParaNames->item(i,1)->text();
    if (sCurrent.contains('[') && sCurrent.contains("..") && sCurrent.contains(']'))
        text+=sCurrent.remove(" ");
    else
        text+=QString::number(tableParaNames->item(i,1)->text().toDouble());
    
    text=text+'"'+";\n}\n\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Initial 'Adjustibility' of Parameters  \n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *adjust_parameters()\n{\n\treturn "+'"';
    for(i=0;i<(spinBoxP->value()-1);i++)
        if (tableParaNames->item(i,2)->checkState()==Qt::Checked) 
            text=text+'1'+','; 
        else 
            text=text+'0'+',';
    
    if (tableParaNames->item(i,2)->checkState()==Qt::Checked) 
        text=text+'1'; 
    else 
        text=text+'0';
    
    text=text+'"'+";\n}\n\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Number of parameters\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *paraNumber()\n{\n\treturn "+'"';
    text+=QString::number(spinBoxP->value());
    text=text+'"'+";\n}\n\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Description of Function and parameters\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *listComments()\n{\n ";
    text+="\tchar  *list;\n\tlist=";
    //+++
    QString htmlText=textEditDescription->toPlainText().remove("\n").replace("\\;", "\\\\ \\\\;");
    
    QString richText;
    int nChars=htmlText.length();
    for (i=0;i<nChars;i++){
        if (htmlText[i]=='"') richText+=" \\";
        richText+=htmlText[i];
    }

    richText.replace("\\ ", "\\\\ ");
    richText.replace("\\\\\\", "\\\\");
    
    if (richText=="") richText="---";
    
    text=text+'"'+richText+'"'+"\n";
    
    text=text+"\t\t\t"+'"';
    for(i=0;i<spinBoxP->value();i++) text=text+",,"+tableParaNames->item(i,3)->text();
    text=text+'"'+";\n\treturn list;\n}\n\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Included Functions\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n\n";
    
    lstTmp=text.split("\n");
    
    lnTextEditFunctions->firstLineIncrement=lstTmp.count()-1; lnTextEditFunctions->updateLineNumbers(true);
    lstTmp.clear();
    
    text+=textEditFunctions->toPlainText();
    
    if (checkBoxAddFortran->isChecked()){

        text+="\n/////////////////////////////////////////////////////////////////////////////////\n";
        text+="//+++ Forward Declaration Fortran Functions\n";
        text+="/////////////////////////////////////////////////////////////////////////////////\n";
        text=text+"extern "+'"'+"C"+'"'+" \n{\n";
        text=text+textEditForwardFortran->toPlainText()+"\n}\n\n";
    }
    
    text+="\n\n/////////////////////////////////////////////////////////////////////////////////\n";
    text+="//+++ Fit Function\n";
    text+="/////////////////////////////////////////////////////////////////////////////////\n";
    text=text+"extern "+'"'+"C"+'"'+" MY_EXPORT double functionSANS(double key";
    
    
    if (radioButton2D->isChecked() ){

        text=text+", void * ParaM)\n{\n//+++\ngsl_vector *Para\t=((struct functionND *) ParaM)->para;\n";
        text=text+"double *xxxx\t=((struct functionND *) ParaM)->Q;\n";
        
        QStringList lst = lineEditXXX->text().remove(" ").split(",", Qt::SkipEmptyParts);
        
        text=text+"double "+lst[0]+"\t\t=1.0 + xxxx[0];\n";
        text=text+"double "+lst[1]+"\t\t=1.0 + xxxx[1];\n";
        
        for (int i=2; i<spinBoxXnumber->value(); i++)
            text=text+"double "+lst[i]+"\t\t= xxxx["+QString::number(i)+"];\n";
        
        for(i=0;i<spinBoxP->value();i++)
            text=text+"double "+tableParaNames->item(i,0)->text().trimmed()+"\t\t=gsl_vector_get(Para,"+QString::number(i)+");\n";
        
        text=text+"int prec\t=((struct functionND *) ParaM)->prec;\n//+++\n";
        
    } else{
        text=text+", void * ParaM)\n{\n//+++\ngsl_vector *Para\t=((struct functionT *) ParaM)->para;\n";
        for(i=0;i<spinBoxP->value();i++)
            text=text+"double "+tableParaNames->item(i,0)->text().trimmed()+"\t\t=gsl_vector_get(Para, "+QString::number(i)+");\n";
        
        text=text+"//+++\ndouble\t" +lineEditXXX->text().remove(" ")+"=key;\n";
        text=text+"int prec\t=((struct functionT *) ParaM)->prec;\n//+++\n";        
    }
    
    text=text+"double\t" +lineEditY->text().trimmed()+";\n\n//+++++++++++++++++++++++++++++++++++++++++++++\n";
    
    text=text+"gsl_set_error_handler_off();";
    text=text+"\n//+++++++++++++++++++++++++++++++++++++++++++++\n\n";
    
    lstTmp=text.split("\n");
    
    lnTextEditCode->firstLineIncrement=lstTmp.count()-1; lnTextEditCode->updateLineNumbers(true);
    lstTmp.clear();
    
    text+=textEditCode->toPlainText();
    text=text+"\n//+++++++++++++++++++++++++++++++++++++++++++++\n\n";
    if (radioButton2D->isChecked() ){
        text=text+"stop:\n";
        
        for(i=0;i<spinBoxP->value();i++)
            text=text+"gsl_vector_set(Para," +QString::number(i)+","+tableParaNames->item(i,0)->text().trimmed()+");\n";
    } else{
        for(i=0;i<spinBoxP->value();i++)
            text=text+"gsl_vector_set(Para," +QString::number(i)+","+tableParaNames->item(i,0)->text().trimmed()+");\n";
    }

    text=text+"\n"+"return "+lineEditY->text().trimmed()+";\n}\n";
        
    
    QFile f(fn);
    if ( !f.open( QIODevice::WriteOnly ) ){
        QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                              tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
        return;
    }

    QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    t << text;
    f.close();
    
    if ( radioButtonCPP->isChecked() ){
        tableCPP->setRowCount(0);
        QStringList lst = text.split("\n");
        tableCPP->setRowCount(lst.count()+1);
        for (int ii=0; ii<lst.count();ii++) tableCPP->setItem(ii,0,new QTableWidgetItem(lst[ii]));
    }
}
//*******************************************
//+++ newFIF()
//*******************************************
void compile18::newFIF(){
    QTextCursor cursor = textEditHFiles->textCursor();
    cursor.movePosition( QTextCursor::End );
    textEditHFiles->setTextCursor( cursor );
    
    textEditHFiles->clear();
    
    cursor = textEditCode->textCursor();
    cursor.movePosition( QTextCursor::End );
    textEditCode->setTextCursor( cursor );
    
    textEditCode->clear();
    
    
    cursor = textEditFunctions->textCursor();
    cursor.movePosition( QTextCursor::End );
    textEditFunctions->setTextCursor( cursor );
    
    textEditFunctions->clear();
    
    lineEditGroupName->clear();
    lineEditFunctionName->clear();
    
    spinBoxP->setValue(0);
    lineEditY->setText("f");
    
    stot1Dto2D();
    
    changedFXYinfo();
    
    textEditDescription->clear();
    textEditDescription->setText("good place for function's comments");
    textEditHFiles->clear();

    scanGroups();
    scanIncludedFunctions();
    
    tabWidgetCode->setCurrentIndex(0);
    tableCPP->setRowCount(0);
    radioButtonBAT->setChecked(false);
    radioButtonCPP->setChecked(false); radioButtonCPP->setText("*.cpp");
    radioButtonFIF->setChecked(false); radioButtonFIF->setText("*.fif");
    
    checkBoxAddFortran->setChecked(false);
    fortranFunction->setText("");
    textEditForwardFortran->setText("");
    
    //+++ Fit.Control
    checkBoxEfit->setChecked(false);
    lineEditEFIT->setText("");
    checkBoxWeight->setChecked(false);
    comboBoxWeightingMethod->setCurrentIndex(1);
    checkBoxSuperpositionalFit->setChecked(false);
    spinBoxSubFitNumber->setValue(1);
    checkBoxAlgorithm->setChecked(false);
    comboBoxFitMethod->setCurrentIndex(1);
    lineEditFitMethodPara->setText("");
}
//*******************************************
//+++  "build Single Function"
//*******************************************
void compile18::buildSingleFunction()
{
    boolCompileAll = false;
    buildSharedLibrary();
}
//*******************************************
//+++  build Shared Library
//*******************************************
void compile18::buildSharedLibrary()
{
    QString ext = "";
    if (radioButton2D->isChecked()) 
        ext = "2d";

    if (pushButtonSave->isEnabled())
        makeFIF();
    else
       openFIFfileSimple();

    makeCPP();
    makeCompileScript();

    QDir d(pathFIF);
    
    QString file = "\"" + pathFIF + "/compile.script.bat\"";
    d.remove(lineEditFunctionName->text().trimmed() + ".o");
    
#ifdef Q_OS_WIN
    d.remove(lineEditFunctionName->text().trimmed() + ".dll" + ext);
#elif defined(Q_OS_MAC)
    d.remove(lineEditFunctionName->text().trimmed() + ".dylib" + ext);
#else
    d.remove(lineEditFunctionName->text().trimmed() + ".so" + ext);
#endif

    procc = new QProcess(qApp);

    if (!boolCompileAll)
        toResLog("\n<< compile >>\n");

    connect(procc, SIGNAL(readyReadStandardError()), this, SLOT(readFromStdout()));

#ifdef Q_OS_WIN
    procc->start("cmd.exe", QStringList() << "/c" << file.replace("\/", "\\").replace("\\\\", "\\").remove("\""));
#else
    procc->start("/bin/bash", QStringList() << "-c" << file.replace("//", "/"));
#endif

    procc->waitForFinished();

    QString soName = fitPath->text() + "/" + lineEditFunctionName->text() + ".";

#ifdef Q_OS_MACOS
    soName += "dylib";
#elif defined(Q_OS_WIN)
    soName += "dll";
#else
    soName += "so";
#endif

    if (radioButton2D->isChecked())
        soName += "2d";
    soName = soName.replace("//", "/");

    if (QFile::exists(soName))
    {
        toResLog("<< compile status >> OK: function '" + lineEditFunctionName->text() + "' is ready\n");
        app()->d_status_info->setText("<< compile status >> OK: function '" + lineEditFunctionName->text() +
                                      "' is ready");
    }
    else
    {
        toResLog(QString("<< compile status >>  ERROR: check function code / compiler options / file was not created: "
                         "%1 / script file: %2\n")
                     .arg(soName)
                     .arg(file));
        app()->d_status_info->setText("<< compile status >>  ERROR: check function code / compiler options");
    }

    if (d.exists(lineEditFunctionName->text().trimmed() + ".o" + ext))
        d.remove(lineEditFunctionName->text().trimmed() + ".o" + ext);
}
//*******************************************
//+++  make dll file
//*******************************************
void compile18::compileTest(){
    QString ext="";
    if (radioButton2D->isChecked()) 
        ext="2d";
    
    QDir d(pathFIF);
    QString file = pathFIF + "/compile.script.bat";
    d.remove(lineEditFunctionName->text().trimmed()+".o");
    
#ifdef Q_OS_WIN
    d.remove(lineEditFunctionName->text().trimmed()+".dll"+ext);
#elif defined(Q_OS_MAC)
    d.remove(lineEditFunctionName->text().trimmed()+".dylib"+ext);
#else
    d.remove(lineEditFunctionName->text().trimmed()+".so"+ext);
#endif

    procc = new QProcess(qApp);
    if (!boolCompileAll) 
        toResLog("\n<< compile >>\n");
    connect( procc, SIGNAL(readyReadStandardError()), this, SLOT(readFromStdout()) );
    procc->start("/bin/bash", QStringList() << "-c" << file);
    procc->waitForFinished();
    QString strOut = procc->readAllStandardOutput();
    toResLog(strOut);
    QString soName=fitPath->text()+"/"+lineEditFunctionName->text()+".";
    
#ifdef Q_OS_MACOS // Mac
    soName+="dylib";
#elif defined(Q_OS_WIN)
    soName+="dll";
#else
    soName+="so";
#endif
    
    if (radioButton2D->isChecked()) 
        soName+="2d";
    
    if (QFile::exists (soName))
        toResLog("<< compile status >> OK: function '"+ lineEditFunctionName->text()+"' is ready\n");
    else
        toResLog("<< compile status >>  ERROR: check function code / compiler options\n");
    
    
    if (d.exists(lineEditFunctionName->text().trimmed()+".o"+ext))
        d.remove(lineEditFunctionName->text().trimmed()+".o"+ext);
}
//*******************************************
//+++  check Function Name
//*******************************************
void compile18::checkFunctionName(){
    QString name=lineEditFunctionName->text();
    
    name=name.replace("_","-");
    name=name.remove(" ");
    name=name.remove(".");
    name=name.remove(":").remove(";").remove("|").remove("!").remove("%").remove("@");
    name=name.remove("[").remove("]").remove("=").remove("?");
    name = name.remove(",").remove("~").remove("*").remove("<").remove(">");
    name = name.trimmed();

    lineEditFunctionName->setText(name);
    textLabelInfoSAS->setText(name);
}
//*******************************************
//+++  read From Stdout
//*******************************************
void compile18::readFromStdout(){
    QString s=procc->readAllStandardError();

    QStringList lst;
    lst = s.split("\n", Qt::SkipEmptyParts);
    
    s="";
    for(int i=0;i<lst.count();i++) if (lst[i].contains("error:")) s+= lst[i]+"\n";
    for(int i=0;i<lst.count();i++) if (lst[i].contains("error:")) s+= lst[i]+"\n";
    
    s=s.replace("‘","`").replace("’","'");
    
    toResLog( s);
}
//*******************************************
//+++  compile all functions
//*******************************************
void compile18::compileAll(){
    boolCompileAll=true;
    int numberFunctions=listBoxFunctionsNew->model()->rowCount();
    if (numberFunctions==0) return;
    
    //+++ Progress Dialog +++
    QProgressDialog progress("Compile All Functions in Functions Window ...", "Abort Compilation", 0, numberFunctions+1, this);
    progress.setWindowModality(Qt::WindowModal);
    
    
    progress.setValue(0);
    
    
    QModelIndexList indexes = listBoxGroupNew->selectionModel()->selectedIndexes();
    if (indexes.count()==0) return;
    
    for (int i=0;i<numberFunctions;i++)
    {
        listBoxGroupNew->selectionModel()->select(indexes[0], QItemSelectionModel::Select);
        listBoxFunctionsNew->selectionModel()->clearSelection();
        listBoxFunctionsNew->selectionModel()->select(listBoxFunctionsNew->model()->index(i,0), QItemSelectionModel::Select);

        openFIFfileSimple();

        if (lineEditFunctionName->text()!=listBoxFunctionsNew->model()->index(i,0).data().toString()) continue;
        
        buildSharedLibrary();

        //+++ Progress +++
        progress.setValue(i+1);
        progress.setLabelText("Function :: " + lineEditFunctionName->text() +" is ready");
        if ( progress.wasCanceled() ) break;    
    }
    
   listBoxGroupNew->selectionModel()->select(indexes[0], QItemSelectionModel::Select);
}
