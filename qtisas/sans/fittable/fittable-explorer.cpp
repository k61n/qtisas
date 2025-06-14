/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s)'s explorer functions
 ******************************************************************************/

#include "fittable18.h"
#include "fit-function-explorer.h"

//******************************************************
//+++  scan Group: eFit functions for Analysis Menu
//******************************************************
QStringList fittable18::scanGroupEfit() const
{
    return scanGroup(true);
}
//******************************************************
//+++  scan Group: update list of groups in fit.explorer
//******************************************************
void fittable18::scanGroup()
{
    bool checkEfit = checkBoxShowEFIT->isChecked();
    QStringList group = scanGroup(checkEfit);

    listViewGroup->setModel(new QStringListModel(group));

    connect(listViewGroup->selectionModel(), &QItemSelectionModel::currentChanged, this, &fittable18::groupChanged);

    listViewFunctions->setModel(new QStringListModel(QStringList()));
}
//******************************************************
//+++  scan Group: update list of groups in fit.explorer
//******************************************************
QStringList fittable18::scanGroup(bool checkEfit) const
{
    QString filter = FunctionsExplorer::filterShared();
    QStringList lstFIF = FunctionsExplorer::scanFiles(libPath, "*.fif", true);
    QStringList lstShared = FunctionsExplorer::scanFiles(libPath, filter, true);

    QDir d(libPath);
    QString firstline, secondline, subfolder;
    QStringList group, groupSubfolders;
    QString groupName;

    for (int i = 0; i < lstFIF.count(); i++)
    {
        if (!d.exists(lstFIF[i] + ".fif"))
            continue;

        if (!lstShared.contains(lstFIF[i]))
            continue;

        QFile f(libPath + lstFIF[i] + ".fif");
        f.open(QIODevice::ReadOnly);

        QTextStream t(&f);
        firstline = t.readLine();
        secondline = t.readLine();

        f.close();

        if (checkEfit && !firstline.contains("[eFit]"))
            continue;

        subfolder = FunctionsExplorer::subFolder(lstFIF[i]) + "/";
        if (subfolder != "/" && !groupSubfolders.contains(subfolder))
            groupSubfolders << subfolder;

        groupName = secondline.trimmed();
        if (!group.contains(groupName) && groupName != "")
            group << groupName;
    }

    group << groupSubfolders;
    group.prepend("ALL");

    return group;
}
//*******************************************
//+++  fing functions of single Group
//*******************************************
void fittable18::groupChanged(const QModelIndex &index, const QModelIndex &p)
{
    QString groupName = index.data().toString();
    if (groupName == "")
        return;

    QStringList functions = groupFunctions(groupName, checkBoxShowEFIT->isChecked());

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

    app()->jnseWidget->filterFitFunctions(functions,false);

    listViewFunctions->setModel(new QStringListModel(functions));

    connect(listViewFunctions->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(openDLL(const QModelIndex &, const QModelIndex &)));
    if (functions.count() == 0)
        spinBoxPara->setValue(0);
}
//*******************************************
//+++  fing functions of single Group
//*******************************************
QStringList fittable18::groupFunctions(const QString &groupName, bool onlyEFIT) const
{
    QString filter = FunctionsExplorer::filterShared();
    QStringList lst = FunctionsExplorer::scanFiles(libPath, filter, true);
    QStringList lstFIF = FunctionsExplorer::scanFiles(libPath, "*.fif", true);
    QStringList lstALL;

    QString firstLine, secondLine;

    for (int i = 0; i < lst.count(); i++)
    {
        if (!lstFIF.contains(lst[i]))
            continue;
        QFile f(libPath + lst[i] + ".fif");
        if (!f.open(QIODevice::ReadOnly))
            continue;

        QTextStream t(&f);
        firstLine = t.readLine();
        secondLine = t.readLine();

        f.close();

        if (onlyEFIT && !firstLine.contains("[eFit]"))
            continue;

        if (groupName.at(groupName.size() - 1) == '/')
        {
            if (lst[i].contains(groupName))
                lstALL << lst[i];
            continue;
        }

        if (groupName == "ALL" || secondLine.trimmed() == groupName)
            lstALL << lst[i];
    }
    return lstALL;
}
//*******************************************
//fit function:External DLL
//*******************************************
void fittable18::openDLL()
{
    QString filter = FunctionsExplorer::filterShared();
    QString pluginName = QFileDialog::getOpenFileName(this, "QtiSAS - Fit - Function", libPath, filter, 0);
    openDLLgeneral(pluginName);
    scanGroup();
}
//*******************************************
//*open DLL
//*******************************************
void fittable18::openDLL(const QModelIndex &index, const QModelIndex &prev)
{
    QString file = index.data().toString();
    openDLL(file);
}
//*******************************************
//*open DLL
//*******************************************
void fittable18::openDLL(const QString &file)
{
    textLabelInfoSAS->hide();
    textLabelInfoSASversion->hide();
    textLabelInfoSASauthor->hide();

    QString filter = FunctionsExplorer::filterShared().remove("*");

    QString pluginName = libPath + file + filter;
    QString fifName = libPath + file + ".fif";
    
#if defined( Q_OS_WIN)
    pluginName = pluginName.replace("/", "\\");
    pluginName = pluginName.replace("\\\\", "\\");
    fifName = fifName.replace("/", "\\");
    fifName = fifName.replace("\\\\", "\\");
#endif

    openDLLgeneral(pluginName);

    readFIFheader(fifName);

    SANSsupportYN();
}
//*******************************************
//* open DLL
//*******************************************
void fittable18::openDLLgeneral(QString file)
{
    // Lists
    F_paraList.clear();
    F_initValues.clear();
    F_adjustPara.clear();
    F_paraListComments.clear();
    //*F
    F_paraListF.clear();
    F_initValuesF.clear();
    F_adjustParaF.clear();
    F_paraListCommentsF.clear();
    //
    int p=0;
    pF=0;
    pSANS=0;
    
    QString pluginName = file;
    
    if (pluginName.isEmpty())
    {
        QMessageBox::warning(this,tr("QtiKWS"), tr("Error: <p> DLL or LIB"));
        return;
        p=0; pF=0;
        spinBoxPara->setValue(0);
    }
    
    if (!QFile::exists (pluginName))
    {
        QMessageBox::warning(this,tr("QtiKWS"), tr("Error: <p>*.dll OR *.so OR *.dylib"));
        return;
        p=0; pF=0;
        spinBoxPara->setValue(0);
    }
    
    
    if (libName!="")
    {
        lib->unload();
        p=0; pF=0;
        spinBoxPara->setValue(0);
    }
    
    //+++   new 2017
    pluginName=pluginName.replace("//","/");

    QLibrary *libN= new QLibrary(pluginName);
    
    lib=libN;
    
    
    lib->load();
    if (!lib->isLoaded()) {toResLog("error: "+lib->errorString()+pluginName);return;};
    
    
    libName=pluginName;
    
//+++ fitFunctionChar = function name

    QString F_name;
    typedef char* (*fitFuncChar)();
    fitFuncChar fitFunctionChar = (fitFuncChar) lib->resolve("name");
    if (fitFunctionChar)
    {
        F_name=QString(fitFunctionChar());
        //+++ Set Function Name
        textLabelFfunc->setText(F_name);
        
        //tablePara->setRowLabels(F_paraList);
    }
    else
    {
        p=0; pF=0;
        spinBoxPara->setValue(0);
        QMessageBox::warning(this,tr("QtiKWS"), tr("Error: <p>check function 1"));
        return;
    }

 
//+++ Parameter`s Number
    fitFunctionChar=(fitFuncChar) lib->resolve("paraNumber");
    pF=QString(fitFunctionChar()).toInt();
    if (pF<1)
    {
        p=0; pF=0;
        spinBoxPara->setValue(0);
        QMessageBox::warning(this,tr("QtiKWS"), tr("Error: <p>check function 3"));
    }
    
    
//+++ Parameter`s Names
    fitFunctionChar = (fitFuncChar)  lib->resolve("parameters");
    if (fitFunctionChar)
    {
        F_paraListF = QString(fitFunctionChar()).split(",", Qt::SkipEmptyParts);
        if (F_paraListF.count()==(pF+1) && F_paraListF[pF]!="")
        {
            XQ=F_paraListF[pF];
            F_paraListF.removeAll(F_paraListF.at(pF));
        }
        else XQ="x";
    }
    else
    {
        p=0; pF=0;
        spinBoxPara->setValue(0);
        QMessageBox::warning(this,tr("QtiKWS"), tr("Error: <p>check function 2"));
        return;
    }
    
//+++ Parameter`s Init Values
    fitFunctionChar = (fitFuncChar)  lib->resolve("init_parameters");
    if (fitFunctionChar)
    {
        F_initValuesF = QString(fitFunctionChar()).split(",", Qt::SkipEmptyParts);
    }
    
//+++ Parameter`s Adjust Parameters
    fitFunctionChar = (fitFuncChar)  lib->resolve("adjust_parameters");
    if (fitFunctionChar)
    {
        F_adjustParaF = QString(fitFunctionChar()).split(",", Qt::SkipEmptyParts);
    }
    
    
    
//+++ listComments
    fitFunctionChar=(fitFuncChar) lib->resolve("listComments");
    if (fitFunctionChar)
    {
        F_paraListCommentsF = QString(fitFunctionChar()).split(",,", Qt::SkipEmptyParts);
    }
    
//+++ Function
    typedef double (*fitFuncD)(double, void* );
    fitFuncD f_fit;
    f_fit = (fitFuncD) lib->resolve("functionSANS");
    
    if (!f_fit)
    {
        p=0; pF=0;
        spinBoxPara->setValue(0);
        QMessageBox::warning(this,tr("QtiKWS"), tr("Error: <p>check function 4"));
        return;
    }
    
    F.function=f_fit;
    
    F_paraList=F_paraListF;
    F_initValues=F_initValuesF;
    F_adjustPara=F_adjustParaF;
    F_paraListComments=F_paraListCommentsF;
    
    if (pF!=0)
    {
        QString currentInstrument=comboBoxInstrument->currentText();
        if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("SANS") )
        {
            p=pF+pSANS;//+SANS support
            F_paraList+=SANS_param_names;
            F_initValues+=SANS_initValues;
            F_adjustPara+=SANS_adjustPara;
            F_paraListComments+=SANS_paraListComments;
        }
        else
        {
            p=pF;
        }
    }
    if (p)
    {
        spinBoxPara->setValue(p);//
    }
    else
    {
        p=0;
        return;
    }
    
//+++ Function Parameters Allocation
    F_para= gsl_vector_calloc(pF+2);
    F_para_limit_left= gsl_vector_calloc(pF+2);
    F_para_limit_right= gsl_vector_calloc(pF+2);
    F_para_fit_yn= gsl_vector_int_calloc(pF+2);
    
    //+++
    tableParaComments00->setRowCount(0);
    tableParaComments00->setRowCount(pF);
    tableParaComments00->setVerticalHeaderLabels(F_paraList);
    

    
    int i;
    for(i=0;i<p;i++) tableParaComments00->setItem(i,0,new QTableWidgetItem(F_paraListComments[i+1]));
    
    textBrowserFunctionDescription00->clear();
    textBrowserFunctionDescription00->insertHtml(F_paraListComments[0]);//setText(F_paraListComments[0]);
}

void fittable18::readFIFheader(QString fifName)
{
    eFitWeight=false;
    
    //+++
    if (fifName.isEmpty()) return;
    
    //+++
    if (!QFile::exists (fifName))	return;
    
    //+++
    QFile f(fifName);
    
    //+++
    if ( !f.open( QIODevice::ReadOnly ) ) return;
    
    QTextStream t( &f );
    QString s = t.readLine();
    
    if (s.contains("[group]")==0) return;
    
    s.remove("[group] ");
    if (s.contains("[eFit]")) s.remove("[eFit] ");
    if (s.contains("["))
    {
        lineEditEFIT->setText(s.left(s.indexOf("[")).simplified());
        s=s.right(s.length()-s.indexOf("["));
    }
    else lineEditEFIT->setText(s.simplified());
    
    if (s.contains("[Weight]"))
    {
        eFitWeight=true;
        s.remove("[Weight] ");
        comboBoxWeightingMethod->setCurrentIndex(s.left(1).toInt());
        weightChanged();
        s=s.right(s.length()-2);
    }

    if (s.contains("[Superpositional]"))
    {
        checkBoxSuperpositionalFit->setChecked(true);
        s.remove("[Superpositional] ");
        QStringList sLst = s.split(" ", Qt::SkipEmptyParts);
        spinBoxSubFitNumber->setValue(sLst[0].toInt());
    }
    else
    {
        checkBoxSuperpositionalFit->setChecked(false);
        spinBoxSubFitNumber->setValue(1);
    }
    
    if (s.contains("[Algorithm]"))
    {
        s.remove("[Algorithm] ");
        int pos=s.left(1).toInt();
        if (pos>-1 && pos<3) comboBoxFitMethod->setCurrentIndex(pos);
        algorithmSelected();

        s=s.right(s.length()-2);
        
        comboBoxSimplex->setCurrentIndex(0);
        comboBoxLevenberg->setCurrentIndex(0);
        

        if ( s.contains("SD=") )
        {
            int start=s.indexOf("SD=")+3;
            int finish=s.indexOf(" ",s.indexOf("SD=")+3);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            spinBoxSignDigits->setValue(ss.toInt());
        }
        
        if ( s.contains("ITERS=") )
        {
            int start=s.indexOf("ITERS=")+6;
            int finish=s.indexOf(" ",s.indexOf("ITERS=")+6);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);

            if (pos==0 || pos==1) spinBoxMaxIter->setValue(ss.toInt());
            else spinBoxMaxNumberGenerations->setValue(ss.toInt());
        }
        
        if ( (pos==0 || pos==1) && s.contains("REL=") )
        {
            int start=s.indexOf("REL=")+4;
            int finish=s.indexOf(" ",s.indexOf("REL=")+4);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            lineEditTolerance->setText(ss);
        }
        
        if ( pos==1 && s.contains("ABS=") )
        {
            int start=s.indexOf("ABS=")+4;
            int finish=s.indexOf(" ",s.indexOf("ABS=")+4);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            lineEditToleranceAbs->setText(ss);
        }
        
        if  (s.contains("MODE=") )
        {
            int start=s.indexOf("MODE=")+5;
            int finish=s.indexOf(" ",s.indexOf("MODE=")+5);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            if (pos==0 && ss.toInt()<4 && ss.toInt()>0) comboBoxSimplex->setCurrentIndex(ss.toInt()-1);
            else if (pos==1 && ss.toInt()<5 && ss.toInt()>0) comboBoxLevenberg->setCurrentIndex(ss.toInt()-1);
            else if (pos==2 && ss.toInt()<4 && ss.toInt()>0) comboBoxLocalSearch->setCurrentIndex(ss.toInt()-1);
        }
        
        if  (pos==0 && s.contains("CONVRATE=") )
        {
            int start=s.indexOf("CONVRATE=")+9;
            int finish=s.indexOf(" ",s.indexOf("CONVRATE=")+9);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            if (ss.toInt()<4 && ss.toInt()>0) comboBoxConvRate->setCurrentIndex(ss.toInt()-1);
        }
        
        if  (pos==1 && s.contains("DER=") )
        {
            int start=s.indexOf("DER=")+4;
            int finish=s.indexOf(" ",s.indexOf("DER=")+4);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            lineEditSTEP->setText(ss);
        }

        if  (pos==1 && s.contains("DSSV=") )
        {
            int start=s.indexOf("DSSV=")+5;
            int finish=s.indexOf(" ",s.indexOf("DSSV=")+5);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            checkBoxConstChi2->setChecked(ss.contains("yes"));
        }
        
        if  (pos==2 && s.contains("GENCOUNT=") )
        {
            int start=s.indexOf("GENCOUNT=")+9;
            int finish=s.indexOf(" ",s.indexOf("GENCOUNT=")+9);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            spinBoxGenomeCount->setValue(ss.toInt());
        }
        
        if  (pos==2 && s.contains("GENSIZE=") )
        {
            int start=s.indexOf("GENSIZE=")+8;
            int finish=s.indexOf(" ",s.indexOf("GENSIZE=")+8);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            spinBoxGenomeSize->setValue(ss.toInt());
        }
        
        if  (pos==2 && s.contains("SELRATE=") )
        {
            int start=s.indexOf("SELRATE=")+8;
            int finish=s.indexOf(" ",s.indexOf("SELRATE=")+8);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            lineEditSelectionRate->setText(ss);
        }

        if  (pos==2 && s.contains("MUTRATE=") )
        {
            int start=s.indexOf("MUTRATE=")+8;
            int finish=s.indexOf(" ",s.indexOf("MUTRATE=")+8);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            lineEditMutationRate->setText(ss);
        }
        
        if  (pos==2 && s.contains("SEED=") )
        {
            int start=s.indexOf("SEED=")+5;
            int finish=s.indexOf(" ",s.indexOf("SEED=")+5);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
           spinBoxRandomSeed->setValue(ss.toInt());
        }
        /*
         std::cout<<s.latin1()<<"\n"<<flush;
         cout<<"start ->"<<start<<"\n"<<flush;
         cout<<"finish ->"<<finish<<"\n"<<flush;
         std::cout<<ss.latin1()<<"---\n"<<flush;
         */
    }
    

}

//*******************************************
//*Page-to-Page !OB
//*******************************************
void fittable18::slotStackFitPrev()
{
    int id=widgetStackFit->currentIndex();
    
    widgetStackFit->setCurrentIndex(id-1);
    
    if (id==1)
    {
        
        widgetStackFit->setMinimumHeight(300);
        
        textLabelLeft->setText("...");
        textLabelCenter->setText("Select Function");
        textLabelRight->setText("Fitting Session");
        textLabelFfunc->hide();
        comboBoxPolyFunction_2->hide();
        comboBoxFunction->hide();
        spinBoxCurrentFunction->hide();
        spinBoxCurrentFunction->hide();
        
        textLabelLeft->setEnabled(false);
        pushButtonFitPrev->setEnabled(false);
        spinBoxPara->hide();
        //+++
        pushButtonUndo->setEnabled(false);
        pushButtonRedo->setEnabled(false);
        undoRedo.clear();
        undoRedoErrorbars.clear();
        undoRedoFittable.clear();
        undoRedoShareble.clear();
        undoRedoRange.clear();
        undoRedoActive = 0;
        pushButtonLoadFittingSession->show();

        pushButtonSaveSession->hide();

        int currentIndex = listViewFunctions->currentIndex().row();
        {
            const QAbstractItemModel *model = listViewFunctions->model();
            const QModelIndexList indexes =
                model->match(model->index(0, 0), Qt::DisplayRole, comboBoxFunction->currentText(), 1, Qt::MatchExactly);
            if (!indexes.empty())
                listViewFunctions->setCurrentIndex(indexes.at(0));
        }
    }
    else if (id==2)
    {
        textLabelLeft->setText("Select Function");
        textLabelCenter->setText("Fitting Session");
        textLabelRight->setText("Generate Results");
        textLabelLeft->setEnabled(true);
        pushButtonFitPrev->setEnabled(true);
        textLabelRight->setEnabled(true);
        pushButtonFitNext->setEnabled(true);
        pushButtonLoadFittingSession->hide();
        textLabelInfoSAS->hide();
        textLabelInfoSASversion->hide();
        textLabelInfoSASauthor->hide();
        comboBoxDatasetSim->clear();
        
        updateDatasets();
    }
}

//*******************************************
//*Page-to-Page
//*******************************************
bool fittable18::slotStackFitNext()
{
    int id=widgetStackFit->currentIndex();

    int p=spinBoxPara->value();
    
    if (id==0)
    {
        bool autoCheck=checkBoxAutoRecalculate->isChecked();    //+++2019.05.28
        checkBoxAutoRecalculate->setChecked(false);             //+++2019.05.28
        
        if (checkBoxSuperpositionalFit->isChecked())
        {
            pushButtonSimulateSuperpositional->show();
            pushButtonSimulateSuperpositionalRes->show();
            spinBoxCurrentFunction->show();
            spinBoxCurrentFunction->setMaximum(spinBoxSubFitNumber->value()-1);
            spinBoxCurrentFunction->setValue(0);
        }
        else
        {
            pushButtonSimulateSuperpositional->hide();
            pushButtonSimulateSuperpositionalRes->hide();
            spinBoxCurrentFunction->hide();
            spinBoxCurrentFunction->setMaximum(0);
            spinBoxCurrentFunction->setValue(0);
        }
        
        if (checkBoxSANSsupport->isChecked()) widgetStackFit->setMinimumHeight(400);
        else widgetStackFit->setMinimumHeight(300);
        //+++
        initLimits();
        
        
        QString currentInstrument=comboBoxInstrument->currentText();

        if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("SANS") )
        {
            //toolBoxResoPoly->show();
            comboBoxPolyFunction_2->show();
        }
        else if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("Back") )
        {
            //toolBoxResoPoly->show();
            comboBoxPolyFunction_2->hide();
        }
        else
        {
            //toolBoxResoPoly->hide();
            comboBoxPolyFunction_2->hide();
        }
        
        if (checkBoxSuperpositionalFit->isChecked()) spinBoxCurrentFunction->show();
        
        if (p==0)
        {
            QMessageBox::warning(this ,tr("QtiSAS "),tr("Select Function!"));
            return false;
        }
        
        initFitPage();
        textLabelLeft->setText("Select Function");
        textLabelCenter->setText("Fitting Session");
        textLabelRight->setText("Generate Results");
        
        textLabelFfunc->show();
        comboBoxFunction->clear();

        QStringList lst;

        for (int i = 0; i < listViewFunctions->model()->rowCount(); ++i)
            lst << listViewFunctions->model()->index(i, 0).data(Qt::DisplayRole).toString();

        comboBoxFunction->addItems(lst);

        comboBoxFunction->setCurrentIndex(listViewFunctions->currentIndex().row());
        
        comboBoxFunction->show();
        
        if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("SANS") )
        {
            comboBoxPolyFunction_2->show();
        }
        
        textLabelLeft->setEnabled(true);
        pushButtonFitPrev->setEnabled(true);
        
        textLabelRight->setEnabled(true);
        pushButtonFitNext->setEnabled(true);
        spinBoxPara->show();
        // +++
        initMultiTable();
        // +++
        QList<int> lstSplitterSANS;
        if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("SANS") )
        {
            lstSplitterSANS<<1<<1;
            //splitterOptions->setSizes(lstSplitterSANS);
            checkBoxResoSim->show();
            checkBoxPolySim->show();
            
        }
        else if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("Back") )
        {
            lstSplitterSANS<<1<<1;
            //splitterOptions->setSizes(lstSplitterSANS);
            checkBoxResoSim->show();
            checkBoxPolySim->hide();
        }
        else
        {
            lstSplitterSANS<<1<<0;
            //splitterOptions->setSizes(lstSplitterSANS);
            checkBoxResoSim->show();
            checkBoxPolySim->hide();
            checkBoxResoSim->hide();
            checkBoxPolySim->hide();
        }
        
        //+++
        checkBoxResoSim->setChecked(false);
        checkBoxPolySim->setChecked(false);
        radioButtonUniform_Q->setChecked(false);
        radioButtonSameQrange->setChecked(true);
        //~~~
        saveUndo();
        //+++
        pushButtonLoadFittingSession->hide();
        textLabelInfoSAS->hide();
        textLabelInfoSASversion->hide();
        textLabelInfoSASauthor->hide();
        
        pushButtonSaveSession->show();
        spinBoxFnumber->setValue(spinBoxNumberCurvesToFit->value());
        
        buttonGroupNavigator->show();
        algorithmSelected();
        
        checkBoxAutoRecalculate->setChecked(autoCheck); //+++2019.05.28

        optionSelected();//2020-10
        lineEditImin->setText("0");//2020-10
    }
    else if (id==1)
    {
        bool autoCalculation=checkBoxAutoRecalculateSim->isChecked();
        checkBoxAutoRecalculateSim->setChecked(false);
        //+++  maneger
        textLabelLeft->setText("Fitting Session");
        textLabelCenter->setText("Generate Results");
        textLabelRight->setText("...");
        textLabelLeft->setEnabled(true);
        pushButtonFitPrev->setEnabled(true);
        textLabelRight->setEnabled(false);
        pushButtonFitNext->setEnabled(false);
        
        //+++ Simulate
        datasetChangedSim(0);
        
        // +++ multifit
        int cols=spinBoxPara->value()+3;
        
        QStringList colNames;
        colNames<<"fit?"<<"[Y::Set]"<<"[Weight]";
        if (checkBoxSANSsupport->isChecked())
        {
            cols+=1;
            colNames<<"[Reso]";
        }
        colNames+=F_paraList;
        
        tableMultiFit->setHorizontalHeaderLabels(colNames);
        pushButtonLoadFittingSession->hide();
        textLabelInfoSAS->hide();
        textLabelInfoSASversion->hide();
        textLabelInfoSASauthor->hide();

        checkBoxAutoRecalculateSim->setChecked(autoCalculation);
    }
    widgetStackFit->setCurrentIndex(id+1);
    return true;
}
//*******************************************
//*updateEFunctions
//*******************************************
void fittable18::updateEFunctions()
{
    scanGroup();
    groupChanged(listViewGroup->currentIndex(), listViewGroup->currentIndex());
}
//*******************************************
// slot: make NEW table with fit results
//*******************************************
void fittable18::saveFittingSession()
{
    QStringList list;
    findTableListByLabel("FIT1D::Settings::Table",list);
    
    list.prepend("");
    
    if (list.count()==0)
    {
        QMessageBox::warning(this,tr("QtiSAS"),tr("No table with lablel FIT1D::Settings::Table was found!"));
        return;
    }
    
    bool ok;
    QString table = QInputDialog::getItem(this, "QtiSAS", "Enter table name to save fitting session:", list, 0, true, &ok);
    if ( !ok ) return;
    if ( ok && !table.isEmpty() )
    {
        // user entered something and pressed OK
    }
    else
    {
        return;
    }
    
    saveFittingSession(table);
    
}

//*******************************************
// slot: make NEW table with fit results
//*******************************************
void fittable18::saveFittingSession(QString table)
{
    
    int p=spinBoxPara->value();
    int M=spinBoxNumberCurvesToFit->value();
    QString F_name=textLabelFfunc->text();

    Table* w;
    if (app()->checkTableExistence(table, w))
    {
        if (w->windowLabel()!="FIT1D::Settings::Table")
        {
            QMessageBox::critical( 0, "QtiKWS", "Table "+table+" is not fitting-settings table");
            return;
        }
        w->setNumRows(0);
        w->setNumCols(2);
    }
    else
    {
        //app(this)->changeFolder("FIT :: 1D");
        w=app()->newHiddenTable(table,"FIT1D::Settings::Table", 0, 2);
        //+++ new
        w->setWindowLabel("FIT1D::Settings::Table");
        app()->setListViewLabel(w->name(), "FIT1D::Settings::Table");
        app()->updateWindowLists(w);
    }

    // Col-Names
    w->setColName(0, "Parameter");
    w->setColPlotDesignation(0, Table::None);
    w->setColumnType(0, Table::Text);

    w->setColName(1, "Parameter-Value");
    w->setColPlotDesignation(1, Table::None);
    w->setColumnType(1, Table::Text);

    int currentRow=0;
    QString s;
    
    //----- Function::Folder
    w->setNumRows(currentRow+1);
    s=libPath+" <";
    w->setText(currentRow, 0, "Function::Folder");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Function::Name
    w->setNumRows(currentRow+1);
    s=textLabelFfunc->text()+" <";
    w->setText(currentRow, 0, "Function::Name");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //+++ Function::Parameters::Number
    w->setNumRows(currentRow+1);
    s=QString::number(p)+" ";
    w->setText(currentRow,0,"Function::Parameters::Number");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    
    //+++ Function::SANS::Support
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Function::SANS::Support");
    if (checkBoxSANSsupport->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Function::Global::Fit
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Function::Global::Fit");
    if (checkBoxMultiData->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Function::Global::Fit::Number
    w->setNumRows(currentRow+1);
    s=QString::number(M)+" ";
    w->setText(currentRow,0,"Function::Global::Fit::Number");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++ Session::Data::Datasets   0
    w->setNumRows(currentRow+1);
    s="";
    for (int i=0; i<M;i++) s+=((QComboBoxInTable*)tableCurves->cellWidget(0,2*i+1))->currentText()+" ";
    w->setText(currentRow,0,"Session::Data::Datasets");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++ Session::Data::N 1a
    w->setNumRows(currentRow+1);
    s="";
    for (int i=0; i<M;i++) s+=((QComboBoxInTable*)tableCurves->cellWidget(1,2*i))->currentText()+" ";
    w->setText(currentRow,0,"Session::Data::N");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++ Session::Data::NN 1b
    w->setNumRows(currentRow+1);
    s="";
    for (int i=0; i<M;i++)
        s+=tableCurves->item(1,2*i+1)->text()+" ";
    w->setText(currentRow,0,"Session::Data::NN");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++ Session::Data::From::Use
    w->setNumRows(currentRow+1);
    s="";
    for (int i=0; i<M;i++)
    {
        QTableWidgetItem *active = (QTableWidgetItem *)tableCurves->item(2,2*i);
        if (active->checkState()) s+="1 "; else s+="0 ";
    }
    w->setText(currentRow,0,"Session::Data::From::Use");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    
    //+++ Session::Data::From::Number
    w->setNumRows(currentRow+1);
    s="";
    for (int i=0; i<M;i++)
        s+=tableCurves->item(2,2*i+1)->text()+" ";
    w->setText(currentRow,0,"Session::Data::From::Number");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++ Session::Data::To::Use
    w->setNumRows(currentRow+1);
    s="";
    for (int i=0; i<M;i++)
    {
        QTableWidgetItem *active = (QTableWidgetItem *)tableCurves->item(3,2*i);
        if (active->checkState()) s+="1 "; else s+="0 ";
    }
    
    w->setText(currentRow,0,"Session::Data::To::Use");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    
    //+++ Session::Data::To::Number
    w->setNumRows(currentRow+1);
    s="";
    for (int i=0; i<M;i++)
        s+=tableCurves->item(3,2*i+1)->text()+" ";
    w->setText(currentRow,0,"Session::Data::To::Number");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++ Session::Weighting::Use
    w->setNumRows(currentRow+1);
    s="";
    for (int i=0; i<M;i++)
    {
        QTableWidgetItem *active = (QTableWidgetItem *)tableCurves->item(4,2*i);
        if (active->checkState()) s+="1 "; else s+="0 ";
    }
    
    w->setText(currentRow,0,"Session::Weighting::Use");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    
    //+++ Session::Weighting::Dataset
    w->setNumRows(currentRow+1);
    s="";
    for (int i=0; i<M;i++) s+=((QComboBoxInTable*)tableCurves->cellWidget(4,2*i+1))->currentText()+" ";

    w->setText(currentRow,0,"Session::Weighting::Dataset");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++ Session::Limits::Left
    w->setNumRows(currentRow+1);
    s="";
    for (int pp=0; pp<p;pp++)
        s+=tableControl->item(pp,0)->text()+" ";
    w->setText(currentRow,0,"Session::Limits::Left");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++ Session::Limits::Right
    w->setNumRows(currentRow+1);
    s="";
    for (int pp=0; pp<p;pp++)
        s+=tableControl->item(pp,4)->text()+" ";
    w->setText(currentRow,0,"Session::Limits::Right");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    if (checkBoxSANSsupport->isChecked())
    {
        //+++ Session::Resolution::Use
        w->setNumRows(currentRow+1);
        s="";
        for (int i=0; i<M;i++)
        {
            QTableWidgetItem *active = (QTableWidgetItem *)tableCurves->item(5,2*i);
            if (active->checkState()) s+="1 "; else s+="0 ";
        }
        
        w->setText(currentRow,0,"Session::Resolution::Use");
        w->setText(currentRow,1,s+"<");
        currentRow++;
        
        //+++ Session::Resolution::Datasets
        w->setNumRows(currentRow+1);
        s="";
        for (int i=0; i<M;i++) s+=((QComboBoxInTable*)tableCurves->cellWidget(5,2*i+1))->currentText()+" ";
        w->setText(currentRow,0,"Session::Resolution::Datasets");
        w->setText(currentRow,1,s+"<");
        currentRow++;
        
        //+++ Session::Polydispersity::Use
        w->setNumRows(currentRow+1);
        s="";
        for (int i=0; i<M;i++)
        {
            QTableWidgetItem *active = (QTableWidgetItem *)tableCurves->item(6,2*i);
            if (active->checkState()) s+="1 "; else s+="0 ";
        }
        
        w->setText(currentRow,0,"Session::Polydispersity::Use");
        w->setText(currentRow,1,s+"<");
        currentRow++;
        
        //+++ Session::Polydispersity::Datasets
        w->setNumRows(currentRow+1);
        s="";
        for (int i=0; i<M;i++) s+=((QComboBoxInTable*)tableCurves->cellWidget(6,2*i+1))->currentText()+" ";
        w->setText(currentRow,0,"Session::Polydispersity::Datasets");
        w->setText(currentRow,1,s+"<");
        currentRow++;
        
        //+++ Session::Options::Reso
        w->setNumRows(currentRow+1);
        s=QString::number(comboBoxResoFunction->currentIndex())+" ";
        s+=QString::number(comboBoxSpeedControlReso->currentIndex())+" ";
        s+=lineEditAbsErr->text()+" ";
        s+=lineEditRelErr->text()+" ";
        s+=QString::number(spinBoxIntWorkspase->value())+" ";
        s+=QString::number(spinBoxIntLimits->value());
        w->setText(currentRow,0,"Session::Options::Reso");
        w->setText(currentRow,1,s+" <");
        currentRow++;
        
        //+++ Session::Options::Poly
        w->setNumRows(currentRow+1);
        s=QString::number(comboBoxPolyFunction->currentIndex())+" ";
        s+=QString::number(comboBoxSpeedControlPoly->currentIndex())+" ";
        s+=lineEditAbsErrPoly->text()+" ";
        s+=lineEditRelErrPoly->text()+" ";
        s+=QString::number(spinBoxIntWorkspasePoly->value())+" ";
        s+=QString::number(spinBoxIntLimitsPoly->value());
        w->setText(currentRow,0,"Session::Options::Poly");
        w->setText(currentRow,1,s+" <");
        currentRow++;
    }
    
    //+++ Session::Options::Fit::Control
    w->setNumRows(currentRow+1);
    s=QString::number(comboBoxFitMethod->currentIndex())+" ";
    s+=QString::number(comboBoxSimplex->currentIndex())+" ";
    s+=QString::number(comboBoxLevenberg->currentIndex())+" ";
    s+=QString::number(spinBoxMaxIter->value())+" ";
    s+=lineEditTolerance->text()+" ";
    s+=lineEditToleranceAbs->text()+" ";
    s+=QString::number(spinBoxSignDigits->value())+" ";
    s+=QString::number(comboBoxWeightingMethod->currentIndex())+" ";
    if (checkBoxCovar->isChecked()) s+="1 "; else s+="0 ";
    
    s += QString::number(spinBoxWA->value()) + " ";
    s += QString::number(spinBoxWB->value()) + " ";
    s += QString::number(spinBoxWC->value()) + " ";
    s += QString::number(spinBoxWXMAX->value()) + " ";
    
    s+=QString::number(spinBoxGenomeCount->value())+" ";
    s+=QString::number(spinBoxGenomeSize->value())+" ";
    s+=QString::number(spinBoxMaxNumberGenerations->value())+" ";
    s+=lineEditSelectionRate->text()+" ";
    s+=lineEditMutationRate->text()+" ";
    s+=QString::number(spinBoxRandomSeed->value())+" ";
    
    w->setText(currentRow,0,"Session::Options::Fit::Control");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++ Session::Options::Instrument::Reso
    w->setNumRows(currentRow+1);
    s=QString::number(comboBoxResoFunction->currentIndex())+" ";
    s+=QString::number(comboBoxSpeedControlReso->currentIndex())+" ";
    s+=lineEditAbsErr->text()+" ";
    s+=lineEditRelErr->text()+" ";
    s+=QString::number(spinBoxIntWorkspase->value())+" ";
    s+=QString::number(spinBoxIntLimits->value())+" ";
    w->setText(currentRow,0,"Session::Options::Instrument::Reso");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++ Session::Options::Instrument::Poly
    w->setNumRows(currentRow+1);
    s=QString::number(comboBoxPolyFunction->currentIndex())+" ";
    s+=QString::number(comboBoxSpeedControlPoly->currentIndex())+" ";
    s+=lineEditAbsErrPoly->text()+" ";
    s+=lineEditRelErrPoly->text()+" ";
    s+=QString::number(spinBoxIntWorkspasePoly->value())+" ";
    s+=QString::number(spinBoxIntLimitsPoly->value())+" ";
    w->setText(currentRow,0,"Session::Options::Instrument::Poly");
    w->setText(currentRow,1,s+"<");
    currentRow++;
    
    //+++
    if (checkBoxMultiData->isChecked())
    {
        //+++ Session::Parameters::Shared
        w->setNumRows(currentRow+1);
        s="";
        for (int pp=0; pp<p;pp++)
        {
            QTableWidgetItem *active = (QTableWidgetItem *)tablePara->item(pp,0);
            if (active->checkState()) s+="1 "; else s+="0 ";
        }
        
        w->setText(currentRow,0,"Session::Parameters::Shared");
        w->setText(currentRow,1,s+" <");
        currentRow++;
        
        
    }
    
    //+++
    for (int pp=0;pp<p;pp++)
    {
        QString uselName="Session::Parameters::Use::"+QString::number(pp+1);
        
        //+++ Session::Parameters::Use::pp+1
        w->setNumRows(currentRow+1);
        s="";
        for (int i=0; i<M;i++)
        {
            QTableWidgetItem *active = (QTableWidgetItem *)tablePara->item(pp,3*i+1);
            if (active->checkState()) s+="1 "; else s+="0 ";
        }
        
        w->setText(currentRow,0,uselName);
        w->setText(currentRow,1,s+" <");
        currentRow++;
        
        QString cellName="Session::Parameters::Values::"+QString::number(pp+1);
        
        //+++Session::Parameters::Values::pp+1
        w->setNumRows(currentRow+1);
        s="";
        for (int i=0; i<M;i++) s+=tablePara->item(pp,3*i+2)->text()+" ";
        w->setText(currentRow,0,cellName);
        w->setText(currentRow,1,s+"<");
        currentRow++;
        
        QString rangeName="Session::Parameters::Ranges::"+QString::number(pp+1);
        
        //+++Session::Parameters::Ranges::pp+1
        w->setNumRows(currentRow+1);
        s="";
        for (int i=0; i<M;i++)
        {
            QTableWidgetItem *range = (QTableWidgetItem *)tablePara->item(pp,3*i+1);
            s+=range->text()+" ";
        }
        w->setText(currentRow,0,rangeName);
        w->setText(currentRow,1,s+"<");
        currentRow++;
    }
    
    //+++Session::Parameters::Errors
    w->setNumRows(currentRow+1);
    w->setText(currentRow, 0, "Session::Parameters::Errors");
    s="";
    for (int i=0; i<M;i++) for (int pp=0;pp<p;pp++) s+=tablePara->item(pp,3*i+3)->text()+" ";
    w->setText(currentRow,1,s+" <");
    currentRow++;

    //+++ Session::Fit::SaveSession
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Session::Fit::SaveSession");
    if (checkBoxSaveSessionFit->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    
    //----- Session::Chi2
    w->setNumRows(currentRow+1);
    s=textLabelChi->text()+" <";
    w->setText(currentRow, 0, "Session::Chi2");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Session::R2
    w->setNumRows(currentRow+1);
    s=textLabelR2->text()+" <";
    w->setText(currentRow, 0, "Session::R2");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Session::Time
    w->setNumRows(currentRow+1);
    s=textLabelTime->text()+" <";
    w->setText(currentRow, 0, "Session::Time");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Simulate::Color
    w->setNumRows(currentRow+1);
    s=QString::number(comboBoxColor->currentIndex())+" <";
    w->setText(currentRow, 0, "Simulate::Color");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //+++ Simulate::Color::Indexing
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::Color::Indexing");
    if (checkBoxColorIndexing->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Simulate::Statistics
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::Statistics");
    if (checkBoxCovar->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Simulate::SaveSession
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::SaveSession");
    if (checkBoxSaveSession->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Simulate::Indexing
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::Indexing");
    if (checkBoxSimIndexing->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Simulate::Uniform
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::Uniform");
    if (radioButtonUniform_Q->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Simulate::Uniform::Parameters
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Simulate::Uniform::Parameters");
    s="";
    s+=lineEditFromQsim->text()+" ";
    s+=lineEditToQsim->text()+" ";
    s+=lineEditNumPointsSim->text()+" ";
    
    if (checkBoxLogStep->isChecked())
    {
        s+="1 ";
        s+=lineEditImin->text()+" ";
    }
    else s+="0 0 ";
    
    w->setText(currentRow,1,s+"<");
    currentRow++;

    w->adjustColumnsWidth(false);
}

//*******************************************
//* readSettingsTable
//*******************************************
void fittable18::readSettingsTable()
{
    
    QStringList list;
    findTableListByLabel("FIT1D::Settings::Table",list);
    
    if (list.count()==0)
    {
        QMessageBox::warning(this,tr("QtiSAS"),tr("No table with lablel FIT1D::Settings::Table was found!"));
        return;
    }
    
    bool ok;
    QString table = QInputDialog::getItem(this, "QtiSAS", "Select Settings-Table:", list, 0, false, &ok);
    if ( !ok ) return;

    Table *w;
    if (!app()->checkTableExistence(table, w))
        return;

    QStringList parameters;
    parameters.clear();
    
    for (int i=0; i<w->numRows(); i++) parameters<<w->text(i,0);
    
    if (parameters.count()==0)
    {
        QMessageBox::warning(this,tr("QtiSAS"),tr("Check Settings-Table!"));
        return;
    }
    
    QString s;

    //+++ Function::Folder
    if (parameters.indexOf("Function::Folder")>=0)
    {
        QDir d(libPath);
        if (d.exists())
        {
            scanGroup();
            const QModelIndexList indexes = listViewGroup->model()->match(listViewGroup->model()->index(0, 0),
                                                                          Qt::DisplayRole, "ALL", 1, Qt::MatchExactly);
            groupChanged(indexes[0], indexes[0]);
            if (!indexes.empty())
                listViewGroup->setCurrentIndex(indexes.at(0));
        }
        else
            return;
    }
    else
        return;

    //+++ Function::Name
    if (parameters.indexOf("Function::Name") >= 0)
    {
        int rowIndex = static_cast<int>(parameters.indexOf("Function::Name"));
        s = w->text(rowIndex, 1).remove(" <").trimmed();

        const QModelIndexList indexes = listViewFunctions->model()->match(listViewFunctions->model()->index(0, 0),
                                                                          Qt::DisplayRole, s, 1, Qt::MatchExactly);
        if (indexes.empty())
            return;
        listViewFunctions->setCurrentIndex(indexes.at(0));

        openDLL(s);
    }
    else return;
    
    //+++ Function::SANS::Support
    if (parameters.indexOf("Function::SANS::Support")>=0)
    {
        s=w->text(parameters.indexOf("Function::SANS::Support"),1).remove(" <").trimmed();
        
        if (s.contains("yes")) checkBoxSANSsupport->setChecked(true);
        else checkBoxSANSsupport->setChecked(false);
        SANSsupportYN();
        SANSsupportYN(); // TEST
    }
    
    //+++ Function::Parameters::Number
    if (parameters.indexOf("Function::Parameters::Number")>=0)
    {
        s=w->text(parameters.indexOf("Function::Parameters::Number"),1).remove(" <").trimmed();
        
        if (spinBoxPara->value()==s.toInt()-1)
        {
            //QMessageBox::warning(this,tr("QtiSAS"),
            //                    tr("Check Function :: Number of parameters!"));
            //return;
            
        }
        else if (spinBoxPara->value()!=s.toInt())
        {
            QMessageBox::warning(this,tr("QtiSAS"),
                                 tr("Check Function :: Number of parameters!"));
            return;
        }
    }
    else return;
    
    //+++ Function::Global::Fit
    if (parameters.indexOf("Function::Global::Fit")>=0)
    {
        s=w->text(parameters.indexOf("Function::Global::Fit"),1).remove(" <").trimmed();
        
        if (s.contains("yes"))
        {
            checkBoxMultiData->setChecked(true);
            spinBoxNumberCurvesToFit->setEnabled(true);
        }
        else
        {
            checkBoxMultiData->setChecked(false);
            spinBoxNumberCurvesToFit->setEnabled(false);
            spinBoxNumberCurvesToFit->setValue(1);
        }
    }
    else return;
    
    //+++ Function::Global::Fit::Number
    if (parameters.indexOf("Function::Global::Fit::Number")>=0)
    {
        s=w->text(parameters.indexOf("Function::Global::Fit::Number"),1).remove(" <").trimmed();
        
        if (s.toInt()>0 && checkBoxMultiData->isChecked())
        {
            spinBoxNumberCurvesToFit->setValue(s.toInt());
        }
    }
    
    slotStackFitNext();
    
    QStringList lst, allCurves;
    int M=spinBoxNumberCurvesToFit->value();
    int p=spinBoxPara->value();
    //+++ Session::Data::Datasets
    if (parameters.indexOf("Session::Data::Datasets")>=0)
    {
        s=w->text(parameters.indexOf("Session::Data::Datasets"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        allCurves=app()->columnsList(Table::Y);
        
        for (int mm=0;mm<M;mm++)
        {
            QComboBoxInTable* curves= (QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1);
            
            if (allCurves.indexOf(lst[mm])>=0)
            {
                curves->setCurrentIndex(allCurves.indexOf(lst[mm]));
                tableCurvechanged(0, 2*mm+1);
            }
        }
    }
    
    
    //+++ Session::Data::N
    if (parameters.indexOf("Session::Data::N")>=0)
    {
        s=w->text(parameters.indexOf("Session::Data::N"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        for (int mm=0;mm<M;mm++)
        {
            QComboBoxInTable* curves= (QComboBoxInTable*)tableCurves->cellWidget(1, 2*mm);
            
            if (lst[mm]=="N") curves->setCurrentIndex(0);
            else curves->setCurrentIndex(1);
            tableCurvechanged(1, 2*mm);
        }
        
    }
    
    //+++ Session::Data::NN
    if (parameters.indexOf("Session::Data::NN")>=0)
    {
        s=w->text(parameters.indexOf("Session::Data::NN"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        for (int mm=0;mm<M;mm++)
        {
            tableCurves->item(1, 2*mm+1)->setText(lst[mm]);
        }
    }
    
    //+++ Session::Data::From::Use
    if (parameters.indexOf("Session::Data::From::Use")>=0)
    {
        s=w->text(parameters.indexOf("Session::Data::From::Use"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        for (int mm=0;mm<M;mm++)
        {
            QTableWidgetItem *item = (QTableWidgetItem *)tableCurves->item (2,2*mm);
            if (lst[mm]=="1") item->setCheckState(Qt::Checked);
            else item->setCheckState(Qt::Unchecked);
            tableCurvechanged(2, 2*mm);
        }
        
    }
    
    //+++ Session::Data::From::Number
    if (parameters.indexOf("Session::Data::From::Number")>=0)
    {
        s=w->text(parameters.indexOf("Session::Data::From::Number"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        for (int mm=0;mm<M;mm++)
        {
            tableCurves->item(2, 2*mm+1)->setText(lst[mm]);
            tableCurvechanged(2, 2*mm+1);
        }
    }
    
    //+++ Session::Data::To::Use
    if (parameters.indexOf("Session::Data::To::Use")>=0)
    {
        s=w->text(parameters.indexOf("Session::Data::To::Use"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        for (int mm=0;mm<M;mm++)
        {
            QTableWidgetItem *item = (QTableWidgetItem *)tableCurves->item (3,2*mm);
            if (lst[mm]=="1") item->setCheckState(Qt::Checked);
            else item->setCheckState(Qt::Unchecked);
            tableCurvechanged(3, 2*mm);
        }
        
    }
    
    //+++ Session::Data::To::Number
    if (parameters.indexOf("Session::Data::To::Number")>=0)
    {
        s=w->text(parameters.indexOf("Session::Data::To::Number"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        for (int mm=0;mm<M;mm++)
        {
            tableCurves->item(3, 2*mm+1)->setText(lst[mm]);
            tableCurvechanged(3, 2*mm+1);
        }
    }
    
    //+++ Session::Weighting::Use
    if (parameters.indexOf("Session::Weighting::Use")>=0)
    {
        s=w->text(parameters.indexOf("Session::Weighting::Use"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        for (int mm=0;mm<M;mm++)
        {
            QTableWidgetItem *item = (QTableWidgetItem *)tableCurves->item (4,2*mm);
            if (lst[mm]=="1") item->setCheckState(Qt::Checked);
            else item->setCheckState(Qt::Unchecked);
        }
    }
    
    //+++ Session::Weighting::Dataset
    if (parameters.indexOf("Session::Weighting::Dataset")>=0)
    {
        s=w->text(parameters.indexOf("Session::Weighting::Dataset"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        for (int mm=0;mm<M;mm++)
        {
            allCurves.clear();
            allCurves=app()->columnsList(Table::yErr);
            QString tableName=((QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1))->currentText();
            tableName=tableName.left(tableName.lastIndexOf("_"));
            allCurves = allCurves.filter(tableName);

            auto curves = (QComboBoxInTable *)tableCurves->cellWidget(4, 2 * mm + 1);

            if (lst.count() == M && allCurves.indexOf(lst[mm]) >= 0)
                curves->setCurrentIndex(allCurves.indexOf(lst[mm]));
        }
    }
    
    //+++ Session::Limits::Left
    if (parameters.indexOf("Session::Limits::Left")>=0)
    {
        s=w->text(parameters.indexOf("Session::Limits::Left"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        for (int pp=0; pp<gsl_min(p,lst.count());pp++)
        {
            tableControl->item(pp,0)->setText(lst[pp]);
        }
    }
    
    //+++ Session::Limits::Right
    if (parameters.indexOf("Session::Limits::Right")>=0)
    {
        s=w->text(parameters.indexOf("Session::Limits::Right"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        for (int pp=0; pp<gsl_min(p,lst.count());pp++)
        {
            tableControl->item(pp,4)->setText(lst[pp]);
        }
    }
    
    if (checkBoxSANSsupport->isChecked())
    {
        //+++ Session::Resolution::Use
        if (parameters.indexOf("Session::Resolution::Use")>=0)
        {
            s=w->text(parameters.indexOf("Session::Resolution::Use"),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            
            for (int mm=0;mm<M;mm++)
            {
                QTableWidgetItem *item = (QTableWidgetItem *)tableCurves->item (5,2*mm);
                if (lst[mm]=="1") item->setCheckState(Qt::Checked);
                else item->setCheckState(Qt::Unchecked);
            }
        }
        
        //+++ Session::Resolution::Datasets
        if (parameters.indexOf("Session::Resolution::Datasets")>=0)
        {
            s=w->text(parameters.indexOf("Session::Resolution::Datasets"),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            
            for (int mm=0;mm<M;mm++)
            {
                allCurves.clear();
                allCurves=app()->columnsList(Table::xErr);
                QString tableName=((QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1))->currentText();
                tableName=tableName.left(tableName.lastIndexOf("_"));
                allCurves=allCurves.filter(tableName);
                
                QComboBoxInTable* curves= (QComboBoxInTable*)tableCurves->cellWidget(5, 2*mm+1);
                
                if (allCurves.indexOf(lst[mm])>=0)
                {
                    curves->setCurrentIndex(allCurves.indexOf(lst[mm]));
                }
            }
        }
        
        //+++ Session::Polydispersity::Use
        if (parameters.indexOf("Session::Polydispersity::Use")>=0)
        {
            s=w->text(parameters.indexOf("Session::Polydispersity::Use"),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            
            for (int mm=0;mm<M;mm++)
            {
                QTableWidgetItem *item = (QTableWidgetItem *)tableCurves->item (6,2*mm);
                if (lst[mm]=="1") item->setCheckState(Qt::Checked);
                else item->setCheckState(Qt::Unchecked);
            }
        }
        
        //+++ Session::Polydispersity::Datasets
        if (parameters.indexOf("Session::Polydispersity::Datasets")>=0)
        {
            s=w->text(parameters.indexOf("Session::Polydispersity::Datasets"),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            allCurves.clear();
            allCurves=F_paraList;
            
            for (int mm=0;mm<M;mm++)
            {
                QComboBoxInTable* curves= (QComboBoxInTable*)tableCurves->cellWidget(6, 2*mm+1);
                
                if (allCurves.indexOf(lst[mm])>=0)
                {
                    curves->setCurrentIndex(allCurves.indexOf(lst[mm]));
                }
            }
        }
        
        //+++ Session::Options::Reso
        if (parameters.indexOf("Session::Options::Reso")>=0)
        {
            s=w->text(parameters.indexOf("Session::Options::Reso"),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            //+++
            comboBoxResoFunction->setCurrentIndex(lst[0].toInt());
            //+++
            comboBoxSpeedControlReso->setCurrentIndex(lst[1].toInt());
            //+++
            lineEditAbsErr->setText(lst[2]);
            //+++
            lineEditRelErr->setText(lst[3]);
            //+++
            spinBoxIntWorkspase->setValue(lst[4].toInt());
            //+++
            spinBoxIntLimits->setValue(lst[5].toInt());
            
        }
        //+++ Session::Options::Poly
        if (parameters.indexOf("Session::Options::Poly")>=0)
        {
            s=w->text(parameters.indexOf("Session::Options::Poly"),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            //+++
            comboBoxPolyFunction->setCurrentIndex(lst[0].toInt());
            comboBoxPolyFunction_2->setCurrentIndex(lst[0].toInt());
            //+++
            comboBoxSpeedControlPoly->setCurrentIndex(lst[1].toInt());
            //+++
            lineEditAbsErrPoly->setText(lst[2]);
            //+++
            lineEditRelErrPoly->setText(lst[3]);
            //+++
            spinBoxIntWorkspasePoly->setValue(lst[4].toInt());
            //+++
            spinBoxIntLimitsPoly->setValue(lst[5].toInt());
            
        }
    }
    
    //+++ Session::Options::Fit::Control
    if (parameters.indexOf("Session::Options::Fit::Control")>=0)
    {
        s=w->text(parameters.indexOf("Session::Options::Fit::Control"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        int currentItem=0;
        comboBoxFitMethod->setCurrentIndex(lst[currentItem].toInt()); currentItem++;
        comboBoxSimplex->setCurrentIndex(lst[currentItem].toInt()); currentItem++;
        comboBoxLevenberg->setCurrentIndex(lst[currentItem].toInt()); currentItem++;
        //+++
        spinBoxMaxIter->setValue(lst[currentItem].toInt());currentItem++;
        //+++
        lineEditTolerance->setText(lst[currentItem]);currentItem++;
        //+++
        lineEditToleranceAbs->setText(lst[currentItem]);currentItem++;
        //+++
        spinBoxSignDigits->setValue(lst[currentItem].toInt());currentItem++;
        //+++
        comboBoxWeightingMethod->setCurrentIndex(lst[currentItem].toInt());currentItem++;
        //+++
        if (lst[currentItem]=="1") checkBoxCovar->setChecked(true); else checkBoxCovar->setChecked(false);
        currentItem++;
        //+++
        spinBoxWA->setValue(lst[currentItem].toDouble());
        currentItem++;
        //+++
        spinBoxWB->setValue(lst[currentItem].toDouble());
        currentItem++;
        //+++
        spinBoxWC->setValue(lst[currentItem].toDouble());
        currentItem++;
        //+++
        spinBoxWXMAX->setValue(lst[currentItem].toDouble());
        currentItem++;
        //
        spinBoxGenomeCount->setValue(lst[currentItem].toInt());currentItem++;
        //
        spinBoxGenomeSize->setValue(lst[currentItem].toInt());currentItem++;
        //
        spinBoxMaxNumberGenerations->setValue(lst[currentItem].toInt());currentItem++;
        //
        lineEditSelectionRate->setText(lst[currentItem]);currentItem++;
        //
        lineEditMutationRate->setText(lst[currentItem]);currentItem++;
        //
        spinBoxRandomSeed->setValue(lst[currentItem].toInt());currentItem++;
    }
    algorithmSelected();
    weightChanged();
    
    //+++ Session::Options::Instrument::Reso
    if (parameters.indexOf("Session::Options::Instrument::Reso")>=0)
    {
        s=w->text(parameters.indexOf("Session::Options::Instrument::Reso"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        if (lst.count()==6)
        {
            int value=lst[0].toInt();
            if (value >-1 && value<=4) comboBoxResoFunction->setCurrentIndex(value);
            //+++
            value=lst[1].toInt();
            if (value >-1 && value<=8) comboBoxSpeedControlReso->setCurrentIndex(value);
            //+++
            lineEditAbsErr->setText(lst[2]);
            //+++
            lineEditRelErr->setText(lst[3]);
            //+++
            spinBoxIntWorkspase->setValue(lst[4].toInt());
            //+++
            spinBoxIntLimits->setValue(lst[5].toInt());
        }
    }
    speedControlReso();
    
    //+++ Session::Options::Instrument::Poly
    if (parameters.indexOf("Session::Options::Instrument::Poly")>=0)
    {
        s=w->text(parameters.indexOf("Session::Options::Instrument::Poly"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        if (lst.count()==6)
        {
            int value = lst[0].toInt();
            if (value > -1 && value <= 4)
            {
                comboBoxPolyFunction->setCurrentIndex(value);
                comboBoxPolyFunction_2->setCurrentIndex(value);
            }
            //+++
            value=lst[1].toInt();
            if (value >-1 && value<=8) comboBoxSpeedControlPoly->setCurrentIndex(value);
            //+++
            lineEditAbsErrPoly->setText(lst[2]);
            //+++
            lineEditRelErrPoly->setText(lst[3]);
            //+++
            spinBoxIntWorkspasePoly->setValue(lst[4].toInt());
            //+++
            spinBoxIntLimitsPoly->setValue(lst[5].toInt());
        }
    }
    speedControlPoly();
    
    
    
    if (checkBoxMultiData->isChecked())
    {
        //+++ Session::Parameters::Shared
        if (parameters.indexOf("Session::Parameters::Shared")>=0)
        {
            s=w->text(parameters.indexOf("Session::Parameters::Shared"),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            
            for (int pp=0;pp<p;pp++)
            {
                QTableWidgetItem *active = (QTableWidgetItem *)tablePara->item(pp,0);
                if (lst[pp]=="1") active->setCheckState(Qt::Checked);
                else active->setCheckState(Qt::Unchecked);
            }
        }
    }
    
    for (int pp=0;pp<p;pp++)
    {
        QString uselName="Session::Parameters::Use::"+QString::number(pp+1);
        
        //+++ useName
        if (parameters.indexOf(uselName)>=0)
        {
            s=w->text(parameters.indexOf(uselName),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            
            for (int mm=0; mm<M;mm++)
            {
                QTableWidgetItem *active = (QTableWidgetItem *)tablePara->item(pp,3*mm+1);
                if (lst[mm]=="1") active->setCheckState(Qt::Checked);
                else active->setCheckState(Qt::Unchecked);		;
            }
        }
        
        QString cellName="Session::Parameters::Values::"+QString::number(pp+1);
        
        //+++ useName
        if (parameters.indexOf(cellName)>=0)
        {
            s=w->text(parameters.indexOf(cellName),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            
            for (int mm=0; mm<M;mm++)
            {
                tablePara->item(pp,3*mm+2)->setText(QString::number(lst[mm].toDouble(),'G',spinBoxSignDigits->value()));
            }
        }

        QString rangeName="Session::Parameters::Ranges::"+QString::number(pp+1);
        
        //+++ useName
        if (parameters.indexOf(rangeName)>=0)
        {
            s=w->text(parameters.indexOf(rangeName),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            
            for (int mm=0; mm<M;mm++)
            {
                //std::cout<<lst[mm].latin1()<<"\n"<<flush;
                QTableWidgetItem *range = (QTableWidgetItem *)tablePara->item(pp,3*mm+1);
                if (lst[mm].contains("..") || lst[mm].contains("-+")) range->setText(lst[mm].remove(" "));
            }
        }
    }
    
    //+++ Session::Parameters::Errors
    if (parameters.indexOf("Session::Parameters::Errors")>=0)
    {
        s=w->text(parameters.indexOf("Session::Parameters::Errors"),1).remove(" <").trimmed();
        lst.clear();
        lst = s.split(" ", Qt::SkipEmptyParts);
        
        int iii=0;
        for (int i=0; i<M;i++) for (int pp=0;pp<p;pp++)
        {
            tablePara->item(pp,3*i+3)->setText(lst[iii]);
            iii++;
        }
    }
    
    //+++ Simulate::Statistics
    if (parameters.indexOf("Session::Fit::SaveSession")>=0)
    {
        s=w->text(parameters.indexOf("Session::Fit::SaveSession"),1).remove(" <").trimmed();
        if (s.contains("yes"))
        {
            checkBoxSaveSessionFit->setChecked(true);
        }
        else
        {
            checkBoxSaveSessionFit->setChecked(false);
        }
    }
    
    //+++ Session::Chi2
    if (parameters.indexOf("Session::Chi2")>=0)
    {
        s=w->text(parameters.indexOf("Session::Chi2"),1).remove(" <").trimmed();
        textLabelChi->setText(s);
    }
    //+++ Session::R2
    if (parameters.indexOf("Session::R2")>=0)
    {
        s=w->text(parameters.indexOf("Session::R2"),1).remove(" <").trimmed();
        textLabelR2->setText(s);
    }
    //+++ Session::Time
    if (parameters.indexOf("Session::Time")>=0)
    {
        s=w->text(parameters.indexOf("Session::Time"),1).remove(" <").trimmed();
        textLabelTime->setText(s);
    }
    
    //+++ Simulate::Color
    if (parameters.indexOf("Simulate::Color")>=0)
    {
        s=w->text(parameters.indexOf("Simulate::Color"),1).remove(" <").trimmed();
        comboBoxColor->setCurrentIndex(s.toInt());
    }
    
    //+++ Simulate::Color::Indexing
    if (parameters.indexOf("Simulate::Color::Indexing")>=0)
    {
        s=w->text(parameters.indexOf("Simulate::Color::Indexing"),1).remove(" <").trimmed();
        if (s.contains("yes"))
        {
            checkBoxColorIndexing->setChecked(true);
        }
        else
        {
            checkBoxColorIndexing->setChecked(false);
        }
    }
    
    //+++ Simulate::Statistics
    if (parameters.indexOf("Simulate::Statistics")>=0)
    {
        s=w->text(parameters.indexOf("Simulate::Statistics"),1).remove(" <").trimmed();
        if (s.contains("yes"))
        {
            checkBoxCovar->setChecked(true);
        }
        else
        {
            checkBoxCovar->setChecked(false);
        }
    }
    
    //+++ Simulate::SaveSession
    if (parameters.indexOf("Simulate::SaveSession")>=0)
    {
        s=w->text(parameters.indexOf("Simulate::SaveSession"),1).remove(" <").trimmed();
        if (s.contains("yes"))
        {
            checkBoxSaveSession->setChecked(true);
        }
        else
        {
            checkBoxSaveSession->setChecked(false);
        }
    }
    
    //+++ Simulate::Indexing
    if (parameters.indexOf("Simulate::Indexing")>=0)
    {
        s=w->text(parameters.indexOf("Simulate::Indexing"),1).remove(" <").trimmed();
        if (s.contains("yes"))
        {
            checkBoxSimIndexing->setChecked(true);
        }
        else
        {
            checkBoxSimIndexing->setChecked(false);
        }
    }
    
    //+++ Simulate::Uniform
    if (parameters.indexOf("Simulate::Uniform")>=0)
    {
        s=w->text(parameters.indexOf("Simulate::Uniform"),1).remove(" <").trimmed();
        if (s.contains("yes"))
        {
            radioButtonUniform_Q->setChecked(true);
        }
        else
        {
            radioButtonUniform_Q->setChecked(false);
        }
    }
    
    if (radioButtonUniform_Q->isChecked())
    {
        //+++ Simulate::Uniform::Parameters
        if (parameters.indexOf("Simulate::Uniform::Parameters")>=0)
        {
            s=w->text(parameters.indexOf("Simulate::Uniform::Parameters"),1).remove(" <").trimmed();
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            
            lineEditFromQsim->setText(lst[0]);
            //+++
            lineEditToQsim->setText(lst[1]);
            //+++
            lineEditNumPointsSim->setText(lst[2]);
            //+++
            if (lst[3]=="1")
            {
                checkBoxLogStep->setChecked(true);
                if (lst.count()>4) lineEditImin->setText(lst[4]);
            }
            else
            {
                checkBoxLogStep->setChecked(false);
                lineEditImin->setText("0");
            }
        }
    }
}

//*******************************************
//* iFitAdv
//*******************************************
bool fittable18::iFitAdv(){
    return iFit(true);
}

//*******************************************
//* iFit
//*******************************************
bool fittable18::iFit(){
    return iFit(false);
}

//*******************************************
//* iFit
//*******************************************
bool fittable18::iFit(bool modeAdv){
    
    int idStart=widgetStackFit->currentIndex();
    
    if (idStart>1) return false;
    
    Graph *g;

    if (!app()->findActiveGraph(g))
    {
        QMessageBox::critical(this, tr("QtiKWS"), tr("Activate first GRAPH with data to fit !!!"));
        return false;
    }

    if (g->curveCount()==0) {QMessageBox::critical(this,tr("QtiKWS"), tr("Graph is EMPTY !!!")); return false;};
    
    if (idStart==0)
    {
        if (!slotStackFitNext()) return false;
    }
    
    //widgetStackFit->hide();
    
    horizHeaderCurves(1);
    
    if (!lineEditEFIT->text().contains("no-before")) initParametersBeforeFit();
    if (!lineEditEFIT->text().contains("no-fit") || lineEditEFIT->text().contains("yes-simulate"))
    {
        int currentColor=comboBoxColor->currentIndex();
        
        if (lineEditEFIT->text().contains("color="))
        {
            QString sss=lineEditEFIT->text();
            sss=sss.right(sss.length()-sss.indexOf("color=")-6);
            if (sss.contains(" ")) sss=sss.left(sss.indexOf(" "));
            comboBoxColor->setCurrentIndex(comboBoxColor->findText(sss.replace("_"," ")));
        }

        if (!lineEditEFIT->text().contains("no-fit") )fitOrCalculate(false);
        else if (lineEditEFIT->text().contains("yes-simulate") )fitOrCalculate(true);
        
        comboBoxColor->setCurrentIndex(currentColor);
        
        if (lineEditEFIT->text().contains("name="))
        {
            QString sss=lineEditEFIT->text();
            sss=sss.right(sss.length()-sss.indexOf("name=")-5);
            if (sss.contains(" ")) sss=sss.left(sss.indexOf(" "));

            QList<MdiSubWindow*> windows = app()->tableList();
            foreach(MdiSubWindow *w, windows) if ( w->name()==sss) sss=app()->generateUniqueName(sss);
            foreach(MdiSubWindow *w, windows) if ( w->name()=="fitCurve-"+comboBoxFunction->currentText())
            {
                sss.replace("\n", " ").replace("\t", " ");
                app()->setWindowName(w, sss);
                app()->modifiedProject(w);
            }
        }
    }
    if (!lineEditEFIT->text().contains("no-after")) initParametersAfterFit();
    if (!lineEditEFIT->text().contains("no-reslog")) resToLogWindow();
    if (lineEditEFIT->text().contains("yes-res-in-plot")) addFitResultToActiveGraph(); 
    if (checkBoxSuperpositionalFit->isChecked() && !lineEditEFIT->text().contains("no-superpositionalfit")) simulateSuperpositional();
    
    if (!modeAdv) slotStackFitPrev();
    
    //widgetStackFit->show();
    
    return true;
}


