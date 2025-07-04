/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Code-menu functions of compile interface
 ******************************************************************************/

#include "compile18.h"
#include "fit-function-explorer.h"

// +++ standart menu
void compile18::stdMenu()
{
    auto menuSTD = new QMenu(app());
    connect(menuSTD, &QMenu::triggered, this, &compile18::includeAction);

    menuSTD->addAction(lineEditY->text());
    menuSTD->addSeparator();

    const QStringList lst = lineEditXXX->text().remove(' ').split(',', Qt::SkipEmptyParts);
    for (const QString &item : lst)
        menuSTD->addAction(item);
    menuSTD->addSeparator();

    for (int row = 0; row < spinBoxP->value(); ++row)
        menuSTD->addAction(tableParaNames->item(row, 0)->text());
    menuSTD->addSeparator();

    menuSTD->addAction("all parameters");    

    menuSTD->popup(pushButtonMenuFlags->mapToGlobal(QPoint(0, 0)));
}
// +++ flags menu
void compile18::flagsMenu()
{
    auto menuFlags = new QMenu(app());
    connect(menuFlags, &QMenu::triggered, this, &compile18::includeAction);

    menuFlags->addAction("if (beforeFit) {...} else {...}");
    menuFlags->addAction("if (afterFit) {...} else {...}");
    menuFlags->addAction("if (beforeIter) {...} else {...}");
    menuFlags->addAction("if (afterIter) {...} else {...}");
    menuFlags->addSeparator();

    menuFlags->addAction("[use] beforeFit");
    menuFlags->addAction("[use] afterFit");
    menuFlags->addAction("[use] beforeIter");
    menuFlags->addAction("[use] afterIter");

    menuFlags->popup(pushButtonMenuData->mapToGlobal (QPoint(0,0)));
}
// +++ data menu
void compile18::dataMenu()
{
    auto mainMenuData = new QMenu(app());

    auto submenu = new QMenu(app());
    connect(submenu, &QMenu::triggered, this, &compile18::includeAction);

    submenu->setTitle("Call Current Point of the Fitted Dataset:");
    submenu->addAction("XXX[currentPoint]  :: X-vector");
    submenu->addAction("YYY[currentPoint]  :: Y-vector");
    submenu->addAction("dYYY[currentPoint] :: dY-vector");
    submenu->addAction("RESO[currentPoint] :: Resolusion-vector");
    
    mainMenuData->addMenu(submenu);
    mainMenuData->addSeparator();

    auto submenu2 = new QMenu(app());
    connect(submenu2, &QMenu::triggered, this, &compile18::includeAction);

    submenu2->setTitle("Call Number of ... :");
    submenu2->addAction("current point");
    submenu2->addAction("first point");
    submenu2->addAction("last point");
    submenu2->addSeparator();

    mainMenuData->addMenu(submenu2);
    mainMenuData->addSeparator();

    mainMenuData->popup(pushButtonMenuMath->mapToGlobal(QPoint(0, 0)));
}
// +++ math Menu filling
void compile18::mathMenu()
{
    auto mathFunctions = new QMenu(app());
    connect(mathFunctions, &QMenu::triggered, this, &compile18::mathAction);

    mathFunctions->addAction( tr("abs()"));
    mathFunctions->addSeparator();

    mathFunctions->addAction( tr("log2()"));
    mathFunctions->addAction( tr("log10()"));
    mathFunctions->addAction( tr("ln()"));
    mathFunctions->addSeparator();

    mathFunctions->addAction( tr("sin()"));
    mathFunctions->addAction( tr("cos()"));
    mathFunctions->addAction( tr("tan()"));
    mathFunctions->addSeparator();

    mathFunctions->addAction( tr("asin()"));
    mathFunctions->addAction( tr("acos()"));
    mathFunctions->addAction( tr("atan()"));
    mathFunctions->addSeparator();

    mathFunctions->addAction( tr("sinh()"));
    mathFunctions->addAction( tr("cosh()"));
    mathFunctions->addAction( tr("tanh()"));
    mathFunctions->addSeparator();

    mathFunctions->addAction( tr("asinh()"));
    mathFunctions->addAction( tr("acosh()"));
    mathFunctions->addAction( tr("atanh()"));

    mathFunctions->popup(pushButtonMenuMULTI->mapToGlobal(QPoint(0, 0)));
}
// +++ multi Menu filling
void compile18::multiMenu()
{
    auto multiMenu = new QMenu(app());

    auto submenu = new QMenu(app());
    connect(submenu, &QMenu::triggered, this, &compile18::multimenuAction);

    submenu->setTitle("Superpositional Function:");
    submenu->addAction("add switcher of functions");
    multiMenu->addMenu(submenu);
    multiMenu->addSeparator();

    auto submenu2 = new QMenu(app());
    connect(submenu2, &QMenu::triggered, this, &compile18::multimenuAction);

    submenu2->setTitle("Multiplication of Parameters:");
    submenu2->addAction("parameter order: a,b,...z -> a1,b1,..z1,a2,b2,...,z2,...,aN,bN,..zN");
    submenu2->addAction("parameter order: a,b,...z -> a1,a2,..aN,b1,b2,...,bN,...,z1,z2,..zN");
    multiMenu->addMenu(submenu2);
    multiMenu->addSeparator();

    auto submenu3 = new QMenu(app());
    connect(submenu3, &QMenu::triggered, this, &compile18::multimenuAction);

    submenu3->setTitle("Multiplication of Parameters and Function-Template:");
    submenu3->addAction("parameter order: a,b,..z -> a1,b1,..z1,a2,b2,...,z2,...,aN,bN,..zN");
    submenu3->addAction("parameter order: a,b,..z -> a1,a2,..aN,b1,b2,...,bN,...,z1,z2,..zN");

    multiMenu->addMenu(submenu3);

    multiMenu->popup(pushButtonMenuSASVIEW->mapToGlobal(QPoint(0,0) ));
}

QRegularExpression prefixFilter(const QString &prefix)
{
    return QRegularExpression("^" + QRegularExpression::escape(prefix));
}

QString stringFolder(QString s, const QString &folder)
{
    return s.replace(folder + "/" + folder, folder).replace(folder + "/", folder + "-");
}
// +++ sasview menu
void compile18::sasviewMenu()
{
    auto menuSASVIEW = new QMenu(app());
    connect(menuSASVIEW, &QMenu::triggered, this, &compile18::bgdMenuSelected);

    menuSASVIEW->addAction("...=bgd+scale*...");

    QStringList lst;
    lst = FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.h", false).filter(prefixFilter("sasview"));

    QStringList lst0, lstFolders;

    for (int i = 0; i < lst.count(); i++)
    {
        lst0 = stringFolder(lst[i], "sasview").split("-", Qt::SkipEmptyParts);
        if (lst0.count() > 2 && !lstFolders.contains(lst0[1]))
            lstFolders << lst0[1];
    }

    for (int i = 0; i < lstFolders.count(); i++)
    {
        auto submenu = new QMenu(app());
        submenu->setTitle(lstFolders[i]);
        menuSASVIEW->addMenu(submenu);

        for (int j = 0; j < lst.count(); j++)
        {
            lst0 = stringFolder(lst[j], "sasview").split("-", Qt::SkipEmptyParts);
            if (lst0.count() > 2 && lst0[1] == lstFolders[i])
                submenu->addAction(lst[j]);
        }
        connect(submenu, &QMenu::triggered, this, &compile18::functionMenuSelected);
    }
    menuSASVIEW->popup(pushButtonMenuQTIKWS->mapToGlobal(QPoint(0, 0)));
}
// +++ qtisas/qtikws menu
void compile18::qtisasMenu()
{
    auto menuQTISAS = new QMenu(app());
    connect(menuQTISAS, &QMenu::triggered, this, &compile18::bgdMenuSelected);

    menuQTISAS->addAction("...=bgd+scale*...");

    QStringList lst;
    lst = FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.h", false).filter(prefixFilter("qtisas"));
    lst += FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.h", false).filter(prefixFilter("qtikws"));

    QStringList lst0, lstFolders;
    QString s;

    for (int i = 0; i < lst.count(); i++)
    {
        s = lst[i];
        s = stringFolder(s, "qtisas");
        s = stringFolder(s, "qtikws");

        lst0 = s.split("-", Qt::SkipEmptyParts);
        if (lst0.count() > 2 && !lstFolders.contains(lst0[1]))
            lstFolders << lst0[1];
    }

    for (int i = 0; i < lstFolders.count(); i++)
    {
        auto submenu = new QMenu(app());
        submenu->setTitle(lstFolders[i]);
        menuQTISAS->addMenu(submenu);

        for (int j = 0; j < lst.count(); j++)
        {
            s = lst[j];
            s = stringFolder(s, "qtisas");
            s = stringFolder(s, "qtikws");

            lst0 = s.split("-", Qt::SkipEmptyParts);
            if (lst0.count() > 2 && lst0[1] == lstFolders[i])
                submenu->addAction(lst[j]);
        }
        connect(submenu, &QMenu::triggered, this, &compile18::functionMenuSelected);
    }

    menuQTISAS->popup(pushButtonMenuFORTRAN->mapToGlobal(QPoint(0, 0)));
}
// +++ fortran menu
void compile18::fortranMenu()
{
    auto menuFORTRAN = new QMenu(app());
    connect(menuFORTRAN, &QMenu::triggered, this, &compile18::bgdMenuSelected);

    menuFORTRAN->addAction("...=bgd+scale*...");

    QStringList lst;
    lst = FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.f", false).filter(prefixFilter("fortran"));
    lst += FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.f90", false).filter(prefixFilter("fortran"));
    lst += FunctionsExplorer::scanFiles(pathFIF + "IncludedFunctions", "*.for", false).filter(prefixFilter("fortran"));

    QStringList lst0, lstFolders;
    
    for (int i = 0; i < lst.count(); i++)
    {
        lst0 = stringFolder(lst[i], "fortran").split("-", Qt::SkipEmptyParts);
        if (lst0.count() > 2 && !lstFolders.contains(lst0[1]))
            lstFolders << lst0[1];
    }

    for (int i = 0; i < lstFolders.count(); i++)
    {
        auto submenu = new QMenu(app());
        submenu->setTitle(lstFolders[i]);
        menuFORTRAN->addMenu(submenu);

        for (int j = 0; j < lst.count(); j++)
        {
            lst0 = stringFolder(lst[j], "fortran").split("-", Qt::SkipEmptyParts);
            if (lst0.count() > 2 && lst0[1] == lstFolders[i])
                submenu->addAction(lst[j]);
        }
        connect(submenu, &QMenu::triggered, this, &compile18::fortranMenuSelected);
    }
    menuFORTRAN->popup(pushButtonMenuFORTRAN->mapToGlobal(QPoint(pushButtonMenuFORTRAN->width(), 0)));
}
// +++ action math-Menu
void compile18::mathAction(QAction *action)
{
    QString func = action->text();
    if (func == "ln()")
        func = "log()";

    QString arg = textEditCode->textCursor().selectedText();
    if (arg.isEmpty())
        arg = "x";

    QString wrapped = func;
    if (!func.endsWith(")"))
        wrapped += "(" + arg + ")";
    else
        wrapped = func.replace("()", "(" + arg + ")");

    textEditCode->insertPlainText(wrapped);
}
// +++  action Multi-Menu
void compile18::multimenuAction(QAction *action)
{
    QString actionText = action->text();

    if (actionText == "add switcher of functions")
    {
        bool ok;
        int number = QInputDialog::getInt(this, "Supertositional function",
            "Enter number of sub-functions you want to see additionaly to default one:", 2, 1, 100, 1, &ok);
        if (!ok)
            return;

        number++;
        checkBoxSuperpositionalFit->setChecked(true);
        spinBoxSubFitNumber->setValue(number);

        QString s = "\n";
        QString yvar=lineEditY->text().remove(" ");
        s += "//++++++++++++++++++++++++++++++++++++++++++\n";
        s += "//+++ Superpositional Function Switcher\n";
        s += "//++++++++++++++++++++++++++++++++++++++++++\n";
        s += "//+++ Define below all your sub-functions (default one is first):\n";

        for (int iPara = 0; iPara < number; iPara++)
            s += "double " + yvar + "_" + QString::number(iPara) + "= ...(code)...;\n";

        s += "//+++\n";
        s += "switch(currentFunction)\n";
        s += "{\n";
        s += "case 0: " + yvar + "=" + yvar + "_" + QString::number(1);

        for (int iPara = 2; iPara < number; iPara++)
            s += "+" + yvar + "_" + QString::number(iPara);

        s += "; break;\n";

        for (int iPara = 1; iPara < number; iPara++)
            s += "case " + QString::number(iPara) + ": " + yvar + "=" + yvar + "_" + QString::number(iPara) +
                 "; break;\n";

        s+="}\n";

        textEditCode->insertPlainText(s);
        return;
    }

    int numberPara = tableParaNames->rowCount();

    bool templateIncluded = false;
    bool firstIndexing = false;

    if (actionText.contains("a,b,...z -> a1,a2,..aN,b1,b2,"))
        firstIndexing = true; // templateIncluded = false, firstIndexing = true
    if (actionText.contains("a,b,..z -> a1,b1,..z1,a2,b2"))
        templateIncluded = true; // templateIncluded = true, firstIndexing = false
    if (actionText.contains("a,b,..z -> a1,a2,..aN,b1,b2"))
        firstIndexing = templateIncluded = true; // templateIncluded = true firstIndexing = true

    if (numberPara == 0)
        return;

    // get numbet copies
    bool ok = false;
    int numberCopies =
        QInputDialog::getInt(this, "Input number of Multi-Functions ", "Enter a number:", 2, 2, 100, 1, &ok);
    if (!ok)
        return;

    ok = false;
    int numberSkip = QInputDialog::getInt(this, "Skip First Parameters", "Enter number of parameters to skip", 0, 0,
                                          numberPara - 1, 1, &ok);

    if (!ok)
        return;

    spinBoxP->setValue(numberSkip + (numberPara - numberSkip) * numberCopies);
    setNumberparameters(numberSkip + (numberPara - numberSkip) * numberCopies);

    if (templateIncluded)
    {
        QString s = "\n";
        s += "//++++++++++++++++++++++++++++++++++++++++++\n";
        s += "//+++  Multiplication-Fuction-Template\n";
        s += "//++++++++++++++++++++++++++++++++++++++++++\n\n";
        s += "double ";

        for (int iPara = numberSkip; iPara < numberPara - 1; iPara++)
            s += tableParaNames->item(iPara, 0)->text() + ", ";

        s += tableParaNames->item(numberPara - 1, 0)->text() + ";\n\n ";
        s += lineEditY->text() + "=0.0;\n\n";
        s += "for (int ii = 0; ii < " + QString::number(numberCopies) + "; ii++)\n{\n";

        if (firstIndexing)
        {
            for (int iPara = numberSkip; iPara < numberPara; iPara++)
            {
                s += tableParaNames->item(iPara, 0)->text() + " = gsl_vector_get(Para, ii + ";
                s += QString::number(numberSkip + (iPara - numberSkip) * numberCopies) + ");\n";
            }
        }
        else
        {
            for (int iPara = numberSkip; iPara < numberPara; iPara++)
            {
                s += tableParaNames->item(iPara, 0)->text() + " = gsl_vector_get(Para, ii * ";
                s += QString::number(numberPara - numberSkip) + " + " + QString::number(iPara) + ");\n";
            }
        }

        s += "\n//" + lineEditY->text() + " += function(" + lineEditXXX->text();

        for (int iPara = numberSkip; iPara < numberPara; iPara++)
            s += ", " + tableParaNames->item(iPara, 0)->text();

        s += ");\n}\n";

        textEditCode->insertPlainText(s);
    }

    if (firstIndexing)
    {
        for (int iPara = numberPara - 1; iPara >= numberSkip; iPara--)
        {
            QString curLabel = tableParaNames->item(iPara, 0)->text();
            QString curValue = tableParaNames->item(iPara, 1)->text();
            Qt::CheckState curItem = tableParaNames->item(iPara, 2)->checkState();
            QString range = tableParaNames->item(iPara, 2)->text();
            QString curInfo = tableParaNames->item(iPara, 3)->text();

            for (int iFun = 0; iFun < numberCopies; iFun++)
            {
                int row = numberSkip + iFun + numberCopies * (iPara - numberSkip);
                tableParaNames->setItem(row, 0, new QTableWidgetItem(curLabel + QString::number(iFun + 1)));
                tableParaNames->setItem(row, 1, new QTableWidgetItem(curValue));
                auto *item = new QTableWidgetItem(range);
                item->setCheckState(curItem);
                tableParaNames->setItem(row, 2, item);
                tableParaNames->setItem(row, 3, new QTableWidgetItem(curInfo + " [" + QString::number(iFun + 1) + "]"));
            }
        }
    }
    else
    {
        for (int iPara = numberSkip; iPara < numberPara; iPara++)
        {
            QString curLabel = tableParaNames->item(iPara, 0)->text();
            QString curValue = tableParaNames->item(iPara, 1)->text();
            Qt::CheckState curItem = tableParaNames->item(iPara, 2)->checkState();
            QString range = tableParaNames->item(iPara, 2)->text();
            QString curInfo = tableParaNames->item(iPara, 3)->text();

            for (int iFun = 0; iFun < numberCopies; iFun++)
            {
                int row = iFun * (numberPara - numberSkip) + iPara;
                tableParaNames->setItem(row, 0, new QTableWidgetItem(curLabel + QString::number(iFun + 1)));
                tableParaNames->setItem(row, 1, new QTableWidgetItem(curValue));
                auto *item = new QTableWidgetItem(range);
                item->setCheckState(curItem);
                tableParaNames->setItem(row, 2, item);
                tableParaNames->setItem(row, 3, new QTableWidgetItem(curInfo + " [" + QString::number(iFun + 1) + "]"));
            }
        }
    }
}
// +++ action dependents on action Text
void compile18::includeAction(QAction *action)
{
    QString actionText = action->text();

    actionText.remove("[use] ");
    actionText.replace(" {...}", "{\n}\n");
    actionText.replace(" else", "else");

    const QStringList vectorHints = {"  :: X-vector", "  :: Y-vector", " :: dY-vector", " :: Resolusion-vector"};

    for (const QString &hint : vectorHints)
        actionText.remove(hint);

    actionText.replace("current point", "currentPoint");
    actionText.replace("first point", "currentFirstPoint");
    actionText.replace("last point", "currentLastPoint");

    if (actionText == "all parameters")
    {
        QStringList params;
        for (int i = 0; i < spinBoxP->value(); ++i)
        {
            if (auto item = tableParaNames->item(i, 0))
                params << item->text();
        }
        actionText = params.join(", ");
    }

    textEditCode->insertPlainText(actionText.trimmed());
}
// +++ CompileMenu Slot: selected to insert bgd + scale *  fuction
void compile18::bgdMenuSelected(QAction *bgdAction)
{
    if (bgdAction->text() != "...=bgd+scale*...")
        return;

    int oldParaNumber = spinBoxP->value();

    spinBoxP->setValue(oldParaNumber + 2);
    tableParaNames->item(oldParaNumber, 0)->setText("scale");
    tableParaNames->item(oldParaNumber, 1)->setText("1.0");
    tableParaNames->item(oldParaNumber, 3)->setText("[1] scale factor | volume fraction | ...model dependent ...");

    tableParaNames->item(oldParaNumber + 1, 0)->setText("bgd");
    tableParaNames->item(oldParaNumber + 1, 1)->setText("0.001[0.0..20]");
    tableParaNames->item(oldParaNumber + 1, 3)->setText("[1/cm] background");

    textEditCode->insertPlainText(lineEditY->text().remove(" ") + "= bgd + scale * ;");
    textEditCode->moveCursor(QTextCursor::Left);
}
// +++ CompileMenu Slot: selected to insert sasview/qtisas/qtikws fuction
void compile18::functionMenuSelected(QAction *functionAction)
{
    QDir d(pathFIF + "/IncludedFunctions");
    QString file = pathFIF+"/IncludedFunctions/" + functionAction->text();

    if (!QFile::exists(file))
    {
        QMessageBox::warning(this, "QtiSAS", "Error: <p> no FIF file, 2");
        return;
    }

    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(nullptr, "QtiSAS",
                              QString("Could not write to file: <br><h4>" + file +
                                      "</h4><p>Please verify that you have the right to read from this location!")
                                  .toLocal8Bit()
                                  .constData());
        return;
    }

    QTextStream t(&f);
    QString s;

    QString functionCallName, callFunction;
    QStringList paraNames, paraInitValues, paraRanges, paraUnits, paraInfos, info;

    for (int line = 0; line < 16; line++)
    {
        s = t.readLine();
        if (s.contains("//[add_finish]"))
            break;
        if (!s.contains("//["))
            continue;

        if (s.contains("//[name]"))
        {
            functionCallName = s.remove("//[name] ").remove(" ");
            continue;
        }

        if (s.contains("//[parameter_names] "))
        {
            s = s.remove("//[parameter_names] ").remove(" ");
            paraNames = s.split(",", Qt::SkipEmptyParts);
            callFunction = functionCallName + "(" + lineEditXXX->text().remove(" ") + "," + s + ")";
            continue;
        }

        if (s.contains("//[parameter_init_values] "))
        {
            s = s.remove("//[parameter_init_values] ").remove(" ");
            paraInitValues = s.split(",", Qt::SkipEmptyParts);
            continue;
        }

        if (s.contains("//[parameter_init_range] "))
        {
            s = s.remove("//[parameter_init_range] ").remove(" ");
            paraRanges = s.split(",");
            continue;
        }

        if (s.contains("//[parameter_units] "))
        {
            s = s.remove("//[parameter_units] ").remove(" ");
            paraUnits = s.split(",", Qt::SkipEmptyParts);
            continue;
        }

        if (s.contains("//[parameter_info] "))
        {
            s = s.remove("//[parameter_info] ").remove("[").remove("]");
            paraInfos = s.split(",", Qt::SkipEmptyParts);
            continue;
        }

        if (s.contains("//[info]"))
        {
            s = s.remove("//[info] ");
            info = s.split(",, ", Qt::SkipEmptyParts);
            continue;
        }
    }

    int numPara = static_cast<int>(paraNames.count());
    if (numPara == 0)
        return;

    int oldParaNumber = spinBoxP->value();
    spinBoxP->setValue(oldParaNumber + numPara);

    if (paraNames.count() == numPara)
        for (int i = 0; i < numPara; i++)
            tableParaNames->item(oldParaNumber + i, 0)->setText(paraNames[i]);

    if (paraInitValues.count() == numPara)
        for (int i = 0; i < numPara; i++)
            tableParaNames->item(oldParaNumber + i, 1)->setText(paraInitValues[i]);

    if (paraRanges.count() == numPara)
    {
        for (int i = 0; i < numPara; i++)
            if (paraRanges[i].contains("..")) 
                tableParaNames->item(oldParaNumber + i, 2)->setText(paraRanges[i].remove("[").remove("]"));
            else
                tableParaNames->item(oldParaNumber + i, 2)->setText("..");
    }
    else 
        for (int i = 0; i < numPara; i++)
            tableParaNames->item(oldParaNumber + i, 2)->setText("..");

    if (paraInfos.count() == numPara && paraUnits.count() == numPara)
        for (int i = 0; i < numPara; i++)
            tableParaNames->item(oldParaNumber + i, 3)->setText(paraUnits[i] + " " + paraInfos[i]);
    else if (paraInfos.count() == numPara)
        for (int i = 0; i < numPara; i++)
            tableParaNames->item(oldParaNumber + i, 3)->setText(paraInfos[i]);

    textEditCode->insertPlainText(callFunction);

    if (textEditDescription->toPlainText().contains("good place for function's comments"))
        textEditDescription->clear();
    textEditDescription->insertPlainText("\n" + callFunction + ":::\n");

    for (int i = 0; i < info.count(); i++)
        textEditDescription->insertPlainText(info[i].remove("\"").simplified().replace(",,", "\n") + "\n");

    textEditHFiles->insertPlainText("#include \"IncludedFunctions/" + functionAction->text() + "\"\n");
}
// +++ compileMenu Slot: selected to insert sasview/qtisas/qtikws fuction
void compile18::fortranMenuSelected(QAction *fortranAction)
{
    QStringList lst;
    QString file;

    QDir d(pathFIF + "/IncludedFunctions");
    lst = d.entryList(QStringList() << "fortran*.f");
    lst += d.entryList(QStringList() << "fortran*.f90");
    file = pathFIF + "/IncludedFunctions/" + fortranAction->text();

    if (!QFile::exists(file))
    {
        QMessageBox::warning(this, "QtiSAS", "Error: <p> no FIF file, 2");
        return;
    }

    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(nullptr, "QtiSAS",
                              QString("Could not write to file: <br><h4>" + file +
                                      "</h4><p>Please verify that you have the right to read from this location!")
                                  .toLocal8Bit()
                                  .constData());
        return;
    }

    QTextStream t(&f);

    QString s, functionCallName, callFunction;
    QStringList paraNames, paraInitValues, paraRanges, paraUnits, paraInfos, info;
        
    for (int line = 0; line < 16; line++)
    {
        s = t.readLine();
        if (s.contains("C[add_finish]") || s.contains("![add_finish]"))
            break;
        if (!s.contains("C[") && !s.contains("!["))
            continue;
        if (s.contains("C[name]"))
        {
            functionCallName = s.remove("C[name] ").remove(" ");
            continue;
        };
        if (s.contains("![name]"))
        {
            functionCallName = s.remove("![name] ").remove(" ");
            continue;
        };
        if (s.contains("C[parameter_names] ") || s.contains("![parameter_names] "))
        {
            s = s.remove("C[parameter_names] ").remove("![parameter_names] ").remove(" ");
            paraNames = s.split(",", Qt::SkipEmptyParts);
            callFunction = functionCallName + "_(&" + lineEditXXX->text().remove(" ");
            for (int sss = 0; sss < paraNames.count(); sss++)
                callFunction += ", &" + paraNames[sss];
            callFunction += ")";
            continue;
        }
        if (s.contains("C[parameter_init_values] ") || s.contains("![parameter_init_values] "))
        {
            s = s.remove("C[parameter_init_values] ").remove("![parameter_init_values] ").remove(" ");
            paraInitValues = s.split(",", Qt::SkipEmptyParts);
            continue;
        }
        if (s.contains("C[parameter_init_range] ") || s.contains("![parameter_init_range] "))
        {
            s = s.remove("C[parameter_init_range] ").remove("![parameter_init_range] ").remove(" ");
            paraRanges = s.split(",", Qt::SkipEmptyParts);
            continue;
        }
        if (s.contains("C[parameter_units] ") || s.contains("![parameter_units] "))
        {
            s = s.remove("C[parameter_units] ").remove("![parameter_units] ").remove(" ");
            paraUnits = s.split(",", Qt::SkipEmptyParts);
            continue;
        }
        if (s.contains("C[parameter_info] ") || s.contains("![parameter_info] "))
        {
            s = s.remove("C[parameter_info] ")
                    .remove("![parameter_info] ")
                    .simplified()
                    .replace("] ,", "],")
                    .replace("],", ",,,")
                    .remove("[")
                    .remove("]");
            paraInfos = s.split(",,,", Qt::SkipEmptyParts);
            continue;
        }
        if (s.contains("C[info]") || s.contains("![info]"))
        {
            s = s.remove("C[info] ").remove("![info] ");
            info = s.split(",, ", Qt::SkipEmptyParts);
            continue;
        }
    }

    int numPara = static_cast<int>(paraNames.count());
    if (numPara == 0)
        return;

    int oldParaNumber = spinBoxP->value();
    spinBoxP->setValue(oldParaNumber + numPara);

    if (paraNames.count() == numPara)
        for (int i = 0; i < numPara; i++)
            tableParaNames->item(oldParaNumber + i, 0)->setText(paraNames[i]);

    if (paraInitValues.count() == numPara)
        for (int i = 0; i < numPara; i++)
            tableParaNames->item(oldParaNumber + i, 1)->setText(paraInitValues[i]);

    if (paraRanges.count() == numPara)
    {
        for (int i = 0; i < numPara; i++)
            if (paraRanges[i].contains(".."))
                tableParaNames->item(oldParaNumber + i, 2)->setText(paraRanges[i].remove("[").remove("]"));
            else
                tableParaNames->item(oldParaNumber + i, 2)->setText("..");
    }
    else 
        for (int i = 0; i < numPara; i++)
            tableParaNames->item(oldParaNumber + i, 2)->setText("..");

    if (paraInfos.count() == numPara && paraUnits.count() == numPara)
        for (int i = 0; i < numPara; i++)
            tableParaNames->item(oldParaNumber + i, 3)->setText(paraUnits[i] + " " + paraInfos[i]);
    else if (paraInfos.count() == numPara)
        for (int i = 0; i < numPara; i++)
            tableParaNames->item(oldParaNumber + i, 3)->setText(paraInfos[i]);

    textEditCode->insertPlainText(callFunction);

    textEditDescription->insertPlainText("\n" + callFunction + ":::\n");
    for (int i = 0; i < info.count(); i++)
        textEditDescription->insertPlainText(info[i].remove("\"").simplified().replace(",,", "\n") + "\n");

    QFileInfo fi(file);
    QString filename = fi.fileName();
    filename = "./IncludedFunctions/" + fi.fileName();
    fortranFunction->setText(filename);
    extructFortranFunctions(file);
    checkBoxAddFortran->setChecked(true);    
}
// +++ Find text in the main code window
void compile18::findInCode()
{
    if (!find(textEditCode, lineEditFind))
    {
        textEditCode->moveCursor(QTextCursor::Start);
        find(textEditCode, lineEditFind);
    }
}
// +++ Find text in included window
void compile18::findInIncluded()
{
    if (!find(textEditFunctions, lineEditFindIncluded))
    {
        textEditFunctions->moveCursor(QTextCursor::Start);
        find(textEditFunctions, lineEditFindIncluded);
    }
}
// +++ Find Function: find in qte-editor text typed in qle
bool compile18::find(QTextEdit *qte, QLineEdit *qle)
{
    QString searchString = qle->text();
    QTextCursor currentCursor = qte->textCursor();

    QList<QTextEdit::ExtraSelection> extraSelections;

    QColor color = QColor(Qt::gray).lighter(130);

    QTextCursor newCursor;
    bool newCurPosFound = false;
    while (qte->find(searchString))
    {
        QTextEdit::ExtraSelection extra;
        extra.format.setBackground(color);

        extra.cursor = qte->textCursor();
        extraSelections.append(extra);

        if (!newCurPosFound && extra.cursor.position() >= currentCursor.position())
        {
            newCurPosFound = true;
            currentCursor = extra.cursor;
        }
    }

    qte->setExtraSelections(extraSelections);
    qte->setTextCursor(currentCursor);

    return newCurPosFound;
}