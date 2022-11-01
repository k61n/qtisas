/***************************************************************************
 File                   : fittable-explorer.cpp
 Project                : QtiSAS
 --------------------------------------------------------------------
 Copyright              : (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)    : v.pipich*gmail.com
 Description            : table(s) fitting interface: explorer functions
 
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
#include "fittable18.h"

//*******************************************
//+++  scan Group
//*******************************************
void fittable18::scanGroup()
{
    // +++ WIN
#if defined( Q_OS_WIN)
    QString filter="*.dll";
    
    // +++  MAC
#elif defined(Q_OS_MAC)
    QString filter="*.dylib";
    
    // +++ UNIX
#else
    QString filter="*.so";
#endif
    
    QDir d(libPath);
    
    QStringList lstFIF = d.entryList("*.fif");
    
    QStringList group=scanGroup(lstFIF, checkBoxShowEFIT->isChecked());
    group.sort();
    group.prepend("ALL");
    
    //+++ 2020-06
    listBoxGroupNew->setModel(new QStringListModel(group));
    connect(listBoxGroupNew->selectionModel(),SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),this, SLOT(groupFunctions(const QModelIndex &, const QModelIndex &)));
    QStringList empty;
    listBoxFunctionsNew->setModel(new QStringListModel(empty));
    //+++
}

QStringList fittable18::scanGroupEfit()
{
    // +++ WIN
#if defined( Q_OS_WIN)
    QString filter="*.dll";
    
    // +++  MAC
#elif defined(Q_OS_MAC)
    QString filter="*.dylib";
    
    // +++ UNIX
#else
    QString filter="*.so";
#endif
    
    QDir d(libPath);
    
    QStringList lstFIF = d.entryList("*.fif");
    
    return scanGroup(lstFIF, true);
}

QStringList fittable18::scanGroup(QStringList lstFIF, bool checkEfit )
{
    QDir d(libPath);
    
    QString s;
    QStringList group;
    QString groupName;
    
    for(int i=0;i<lstFIF.count();i++)
    {
        if (!d.exists(lstFIF[i])) continue;
        
        QFile f(libPath+"/"+lstFIF[i]);
        f.open( IO_ReadOnly );
        QTextStream t( &f );
        
        //+++[group]
        s = t.readLine();
        if (s.contains("[group]")) if (!checkEfit || (checkEfit && s.contains("[eFit]")) )
        {
            groupName=t.readLine().stripWhiteSpace();
            if (!group.contains(groupName) && groupName!="") group<<groupName;
        }
        
        f.close();
    }
    return group;
}



//*******************************************
//+++  fing functions of single Group
//*******************************************
void fittable18::groupFunctions(const QModelIndex &index, const QModelIndex &p)
{
    QString groupName=index.data().toString();
    QStringList functions=groupFunctions(groupName, checkBoxShowEFIT->isChecked());
    functions.sort();

    app()->jnseWidget->filterFitFunctions(functions,false);
    
    listBoxFunctionsNew->setModel(new QStringListModel(functions));
    connect(listBoxFunctionsNew->selectionModel(),SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),this, SLOT(openDLL(const QModelIndex &, const QModelIndex &)));
    if (functions.count()==0) spinBoxPara->setValue(0);
}

QStringList fittable18::groupFunctions( const QString &groupName,  bool onlyEFITglobal )
{

    int i;
    QString s;
    QStringList functions;
    
    // +++ WIN
#if defined( Q_OS_WIN )
    QString filter="*.dll";
    
    // +++  MAC
#elif defined(Q_OS_MAC)
    QString filter="*.dylib";
    
    // +++ UNIX
#else
    QString filter="*.so";
#endif
    
    
    QDir d(libPath);
    
    QStringList lst = d.entryList(filter);
    QStringList lstFIF = d.entryList("*.fif");
    QStringList lstALL;
    
    bool onlyEFIT;
    for(i=0;i<lst.count();i++)
    {
        onlyEFIT=onlyEFITglobal;
        QFileInfo fi(libPath+"/"+lst[i]);
        QString base=fi.baseName();
        if (d.exists (lst[i]) && d.exists (base+".fif"))
        {
            lstALL<<base;
            QFile f(libPath+"/"+base+".fif");
            f.open( IO_ReadOnly );
            QTextStream t( &f );
            
            //+++[group]
            s = t.readLine();
            if (!onlyEFIT || (onlyEFIT && s.contains("[eFit]"))) onlyEFIT=true;
            else onlyEFIT=false;
            
            if (s.contains("[group]"))
            {
                if ((groupName=="ALL" || t.readLine().stripWhiteSpace()==groupName) && onlyEFIT) functions<<base;
            }
            
            f.close();
        }
    }

    return functions;
}


//*******************************************
//fit function:External DLL
//*******************************************
void fittable18::openDLL()
{
    // +++ WIN
#if defined( Q_OS_WIN ) //MSVC Compiler
    QString filter="DLL (*.dll)";
    
    // +++  MAC
#elif defined(Q_OS_MAC)
    QString filter="DLL (*.dylib)";
    
    // +++ Unix
#else
    QString filter="DLL (*.so)";
#endif
    
    QString pluginName = QFileDialog::getOpenFileName(libPath, filter, this, 0, "QtiSAS - Fit - Function", 0, TRUE);
    openDLLgeneral(pluginName);
    scanGroup();
}

//*******************************************
//*open DLL
//*******************************************
void fittable18::openDLL(const QModelIndex &index, const QModelIndex &prev)
{
    QString file=index.data().toString();
    openDLL(file);
}

void fittable18::openDLL( const QString &file )
{
    textLabelInfoSAS->hide();
    textLabelInfoSASversion->hide();
    textLabelInfoSASauthor->hide();
    
    // +++ WIN
#if defined( Q_OS_WIN)
    QString filter=".dll";
    
    // +++  MAC
#elif defined(Q_OS_MAC)
    QString filter=".dylib";
    
    // +++ UNIX
#else
    QString filter=".so";
    
#endif
    
    QString pluginName = libPath+"/"+file+filter;
    QString fifName = libPath+"/"+file+".fif";
    
#if defined( Q_OS_WIN)
    pluginName=pluginName.replace("/","\\");
    pluginName=pluginName.replace("\\\\","\\");
    fifName=fifName.replace("/","\\");
    fifName=fifName.replace("\\\\","\\");
#endif
    
    openDLLgeneral(pluginName);
    
    readFIFheader(fifName);
    
    SANSsupportYN();
}

/*
QLibrary myLib("mylib");
typedef void (*MyPrototype)();
MyPrototype myFunction = (MyPrototype) myLib.resolve("mysymbol");
if (myFunction)
myFunction();
*/

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
        F_paraListF = QStringList::split(",", QString(fitFunctionChar()), false);
        if (F_paraListF.count()==(pF+1) && F_paraListF[pF]!="")
        {
            XQ=F_paraListF[pF];
            F_paraListF.remove(F_paraListF.at(pF));
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
        F_initValuesF = QStringList::split(",", QString(fitFunctionChar()), false);
    }
    
//+++ Parameter`s Adjust Parameters
    fitFunctionChar = (fitFuncChar)  lib->resolve("adjust_parameters");
    if (fitFunctionChar)
    {
        F_adjustParaF = QStringList::split(",", QString(fitFunctionChar()), false);
    }
    
    
    
//+++ listComments
    fitFunctionChar=(fitFuncChar) lib->resolve("listComments");
    if (fitFunctionChar)
    {
        F_paraListCommentsF = QStringList::split(",,", QString(fitFunctionChar()), false);
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
    if ( !f.open( IO_ReadOnly ) ) return;
    
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
        comboBoxWeightingMethod->setCurrentItem(s.left(1).toInt());
        weightChanged();
        s=s.right(s.length()-2);
    }

    if (s.contains("[Superpositional]"))
    {
        checkBoxSuperpositionalFit->setChecked(true);
        s.remove("[Superpositional] ");
        QStringList sLst = QStringList::split(" ", s, false);
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
        if (pos>-1 && pos<3) comboBoxFitMethod->setCurrentItem(pos);
        algorithmSelected();

        s=s.right(s.length()-2);
        
        comboBoxSimplex->setCurrentItem(0);
        comboBoxLevenberg->setCurrentItem(0);
        

        if ( s.contains("SD=") )
        {
            int start=s.find("SD=")+3;
            int finish=s.find(" ",s.find("SD=")+3);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            spinBoxSignDigits->setValue(ss.toInt());
        }
        
        if ( s.contains("ITERS=") )
        {
            int start=s.find("ITERS=")+6;
            int finish=s.find(" ",s.find("ITERS=")+6);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);

            if (pos==0 || pos==1) spinBoxMaxIter->setValue(ss.toInt());
            else spinBoxMaxNumberGenerations->setValue(ss.toInt());
        }
        
        if ( (pos==0 || pos==1) && s.contains("REL=") )
        {
            int start=s.find("REL=")+4;
            int finish=s.find(" ",s.find("REL=")+4);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            lineEditTolerance->setText(ss);
        }
        
        if ( pos==1 && s.contains("ABS=") )
        {
            int start=s.find("ABS=")+4;
            int finish=s.find(" ",s.find("ABS=")+4);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            lineEditToleranceAbs->setText(ss);
        }
        
        if  (s.contains("MODE=") )
        {
            int start=s.find("MODE=")+5;
            int finish=s.find(" ",s.find("MODE=")+5);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            if (pos==0 && ss.toInt()<4 && ss.toInt()>0) comboBoxSimplex->setCurrentIndex(ss.toInt()-1);
            else if (pos==1 && ss.toInt()<5 && ss.toInt()>0) comboBoxLevenberg->setCurrentIndex(ss.toInt()-1);
            else if (pos==2 && ss.toInt()<4 && ss.toInt()>0) comboBoxLocalSearch->setCurrentIndex(ss.toInt()-1);
        }
        
        if  (pos==0 && s.contains("CONVRATE=") )
        {
            int start=s.find("CONVRATE=")+9;
            int finish=s.find(" ",s.find("CONVRATE=")+9);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            if (ss.toInt()<4 && ss.toInt()>0) comboBoxConvRate->setCurrentIndex(ss.toInt()-1);
        }
        
        if  (pos==1 && s.contains("DER=") )
        {
            int start=s.find("DER=")+4;
            int finish=s.find(" ",s.find("DER=")+4);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            lineEditSTEP->setText(ss);
        }

        if  (pos==1 && s.contains("DSSV=") )
        {
            int start=s.find("DSSV=")+5;
            int finish=s.find(" ",s.find("DSSV=")+5);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            checkBoxConstChi2->setChecked(ss.contains("yes"));
        }
        
        if  (pos==2 && s.contains("GENCOUNT=") )
        {
            int start=s.find("GENCOUNT=")+9;
            int finish=s.find(" ",s.find("GENCOUNT=")+9);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            spinBoxGenomeCount->setValue(ss.toInt());
        }
        
        if  (pos==2 && s.contains("GENSIZE=") )
        {
            int start=s.find("GENSIZE=")+8;
            int finish=s.find(" ",s.find("GENSIZE=")+8);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            spinBoxGenomeSize->setValue(ss.toInt());
        }
        
        if  (pos==2 && s.contains("SELRATE=") )
        {
            int start=s.find("SELRATE=")+8;
            int finish=s.find(" ",s.find("SELRATE=")+8);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            lineEditSelectionRate->setText(ss);
        }

        if  (pos==2 && s.contains("MUTRATE=") )
        {
            int start=s.find("MUTRATE=")+8;
            int finish=s.find(" ",s.find("MUTRATE=")+8);
            int length=s.length();
            if (finish<start) finish=length;
            QString ss=s.mid( start, finish-start);
            
            lineEditMutationRate->setText(ss);
        }
        
        if  (pos==2 && s.contains("SEED=") )
        {
            int start=s.find("SEED=")+5;
            int finish=s.find(" ",s.find("SEED=")+5);
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
        pushButtonUndo->setEnabled(FALSE);
        pushButtonRedo->setEnabled(FALSE);
        undoRedo.clear();
        undoRedoActive=0;
        pushButtonLoadFittingSession->show();
        //textLabelInfoSAS->show();
        //textLabelInfoSASversion->show();
        //textLabelInfoSASauthor->show();
        
        //+++	listBoxGroup->setCurrentItem(0);
        
        pushButtonSaveSession->hide();
        
        //if(comboBoxFunction->currentIndex()!=listBoxFunctions->currentItem()) listBoxFunctions->setCurrentItem(comboBoxFunction->currentIndex()); //+++ to remove later
        
        //+++ 2020-06
        int currentIndex = listBoxFunctionsNew->currentIndex().row();  // may be an invalid index
        if (comboBoxFunction->currentIndex()!=currentIndex)
        {
            const QAbstractItemModel *model = listBoxFunctionsNew->model();
            const QModelIndexList indexes = model->match(
                                                         model->index(0,0),
                                                         Qt::DisplayRole,
                                                         comboBoxFunction->currentText(),
                                                         1, // first hit only
                                                         Qt::MatchExactly
                                                         );
            
            if (indexes.size() > 0) listBoxFunctionsNew->setCurrentIndex(indexes.at(0));
            
        }
        //---
        
    }
    else if (id==2)
    {
        textLabelLeft->setText("Select Function");
        textLabelCenter->setText("Fitting Session");
        textLabelRight->setText("Generate Results");
        textLabelLeft->setEnabled(TRUE);
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
        //for(int i=0;i<listBoxFunctions->count();i++) lst<<listBoxFunctions->text(i); //+++ to remove later
        
        //+++ 2020-06
        for ( int i = 0 ; i < listBoxFunctionsNew->model()->rowCount() ; ++i ) lst<<listBoxFunctionsNew->model()->index( i, 0 ).data( Qt::DisplayRole ).toString() ;
        //---
        
        comboBoxFunction->insertStringList(lst);
        
        
        //comboBoxFunction->setCurrentItem(listBoxFunctions->currentItem()); //+++ to remove later
        //+++ 2020-06
        comboBoxFunction->setCurrentItem(listBoxFunctionsNew->currentIndex().row());
        //---
        
        comboBoxFunction->show();
        
        if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("SANS") )
        {
            comboBoxPolyFunction_2->show();
        }
        
        textLabelLeft->setEnabled(TRUE);
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
        textLabelLeft->setEnabled(TRUE);
        pushButtonFitPrev->setEnabled(true);
        textLabelRight->setEnabled(FALSE);
        pushButtonFitNext->setEnabled(FALSE);
        
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
    //+++ 2020-06
    groupFunctions(listBoxGroupNew->currentIndex(),listBoxGroupNew->currentIndex());
    //---
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
    QString table = QInputDialog::getItem("QtiSAS", "Enter table name to save fitting session:", list, 0, TRUE, &ok, this );
    if ( !ok ) return;
    
/*
    bool ok;
    QString table = QInputDialog::getText("Input ", "Enter table name to save fitting session:", QLineEdit::Normal,
                                          QString::null, &ok, this );
*/
    
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
    
    if (checkTableExistence(table, w) )
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
    
    
    
    //Col-Names
    QStringList colType;
    w->setColName(0,"Parameter"); w->setColPlotDesignation(0,Table::None);colType<<"1";
    w->setColName(1,"Parameter-Value"); w->setColPlotDesignation(1,Table::None);colType<<"1";
    w->setColumnTypes(colType);
    
    
    
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
    
    s+=lineEditWA->text()+" ";
    s+=lineEditWB->text()+" ";
    s+=lineEditWC->text()+" ";
    s+=lineEditWXMAX->text()+" ";
    
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
    
    
    
    for (int tt=0; tt<w->numCols(); tt++)
    {
        w->table()->adjustColumn (tt);
        w->table()->setColumnWidth(tt, w->table()->columnWidth(tt)+10);
    }
    
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
    QString table = QInputDialog::getItem("QtiSAS", "Select Settings-Table:", list, 0, FALSE, &ok, this );
    if ( !ok ) return;
    
    Table *w;
    if (!checkTableExistence(table,w) ) return;
    
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
    if (parameters.findIndex("Function::Folder")>=0)
    {
        QDir d(libPath);
        if (d.exists())
        {
            scanGroup();
            //+++ 2020-06 QLISTVIEW
            const QModelIndexList indexes = listBoxGroupNew->model()->match(listBoxGroupNew->model()->index(0,0),Qt::DisplayRole,"ALL",1,Qt::MatchExactly);
            groupFunctions(indexes[0],indexes[0]);
            if (indexes.size()>0) listBoxGroupNew->setCurrentIndex(indexes.at(0));
            //---
            //groupFunctions("ALL");
        }
        else return;
        //else groupFunctions("ALL");
    }
    else return;
    
    //+++ Function::Name
    if (parameters.findIndex("Function::Name")>=0)
    {
        s=w->text(parameters.findIndex("Function::Name"),1).remove(" <").stripWhiteSpace();
        
        //+++ 2020-06 QLISTVIEW
        const QModelIndexList indexes = listBoxFunctionsNew->model()->match(listBoxFunctionsNew->model()->index(0,0),Qt::DisplayRole,s,1,Qt::MatchExactly);
        if (indexes.size() <1) return;
        listBoxFunctionsNew->setCurrentIndex(indexes.at(0));
        //---
        
        openDLL(s);
    }
    else return;
    
    //+++ Function::SANS::Support
    if (parameters.findIndex("Function::SANS::Support")>=0)
    {
        s=w->text(parameters.findIndex("Function::SANS::Support"),1).remove(" <").stripWhiteSpace();
        
        if (s.contains("yes")) checkBoxSANSsupport->setChecked(true);
        else checkBoxSANSsupport->setChecked(false);
        SANSsupportYN();
        SANSsupportYN(); // TEST
    }
    
    //+++ Function::Parameters::Number
    if (parameters.findIndex("Function::Parameters::Number")>=0)
    {
        s=w->text(parameters.findIndex("Function::Parameters::Number"),1).remove(" <").stripWhiteSpace();
        
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
    if (parameters.findIndex("Function::Global::Fit")>=0)
    {
        s=w->text(parameters.findIndex("Function::Global::Fit"),1).remove(" <").stripWhiteSpace();
        
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
    if (parameters.findIndex("Function::Global::Fit::Number")>=0)
    {
        s=w->text(parameters.findIndex("Function::Global::Fit::Number"),1).remove(" <").stripWhiteSpace();
        
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
    if (parameters.findIndex("Session::Data::Datasets")>=0)
    {
        s=w->text(parameters.findIndex("Session::Data::Datasets"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        allCurves=app()->columnsList(Table::Y);
        
        for (int mm=0;mm<M;mm++)
        {
            QComboBoxInTable* curves= (QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1);
            
            if (allCurves.findIndex(lst[mm])>=0)
            {
                curves->setCurrentItem(allCurves.findIndex(lst[mm]));
                tableCurvechanged(0, 2*mm+1);
            }
        }
    }
    
    
    //+++ Session::Data::N
    if (parameters.findIndex("Session::Data::N")>=0)
    {
        s=w->text(parameters.findIndex("Session::Data::N"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        for (int mm=0;mm<M;mm++)
        {
            QComboBoxInTable* curves= (QComboBoxInTable*)tableCurves->cellWidget(1, 2*mm);
            
            if (lst[mm]=="N") curves->setCurrentItem(0);
            else curves->setCurrentItem(1);
            tableCurvechanged(1, 2*mm);
        }
        
    }
    
    //+++ Session::Data::NN
    if (parameters.findIndex("Session::Data::NN")>=0)
    {
        s=w->text(parameters.findIndex("Session::Data::NN"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        for (int mm=0;mm<M;mm++)
        {
            tableCurves->item(1, 2*mm+1)->setText(lst[mm]);
        }
    }
    
    //+++ Session::Data::From::Use
    if (parameters.findIndex("Session::Data::From::Use")>=0)
    {
        s=w->text(parameters.findIndex("Session::Data::From::Use"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        for (int mm=0;mm<M;mm++)
        {
            QTableWidgetItem *item = (QTableWidgetItem *)tableCurves->item (2,2*mm);
            if (lst[mm]=="1") item->setCheckState(Qt::Checked);
            else item->setCheckState(Qt::Unchecked);
            tableCurvechanged(2, 2*mm);
        }
        
    }
    
    //+++ Session::Data::From::Number
    if (parameters.findIndex("Session::Data::From::Number")>=0)
    {
        s=w->text(parameters.findIndex("Session::Data::From::Number"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        for (int mm=0;mm<M;mm++)
        {
            tableCurves->item(2, 2*mm+1)->setText(lst[mm]);
            tableCurvechanged(2, 2*mm+1);
        }
    }
    
    //+++ Session::Data::To::Use
    if (parameters.findIndex("Session::Data::To::Use")>=0)
    {
        s=w->text(parameters.findIndex("Session::Data::To::Use"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        for (int mm=0;mm<M;mm++)
        {
            QTableWidgetItem *item = (QTableWidgetItem *)tableCurves->item (3,2*mm);
            if (lst[mm]=="1") item->setCheckState(Qt::Checked);
            else item->setCheckState(Qt::Unchecked);
            tableCurvechanged(3, 2*mm);
        }
        
    }
    
    //+++ Session::Data::To::Number
    if (parameters.findIndex("Session::Data::To::Number")>=0)
    {
        s=w->text(parameters.findIndex("Session::Data::To::Number"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        for (int mm=0;mm<M;mm++)
        {
            tableCurves->item(3, 2*mm+1)->setText(lst[mm]);
            tableCurvechanged(3, 2*mm+1);
        }
    }
    
    //+++ Session::Weighting::Use
    if (parameters.findIndex("Session::Weighting::Use")>=0)
    {
        s=w->text(parameters.findIndex("Session::Weighting::Use"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        for (int mm=0;mm<M;mm++)
        {
            QTableWidgetItem *item = (QTableWidgetItem *)tableCurves->item (4,2*mm);
            if (lst[mm]=="1") item->setCheckState(Qt::Checked);
            else item->setCheckState(Qt::Unchecked);
        }
    }
    
    //+++ Session::Weighting::Dataset
    if (parameters.findIndex("Session::Weighting::Dataset")>=0)
    {
        s=w->text(parameters.findIndex("Session::Weighting::Dataset"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        for (int mm=0;mm<M;mm++)
        {
            allCurves.clear();
            allCurves=app()->columnsList(Table::yErr);
            QString tableName=((QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1))->currentText();
            tableName=tableName.left(tableName.lastIndexOf("_"));
            allCurves=allCurves.grep(tableName);
            
            QComboBoxInTable *curves =(QComboBoxInTable*)tableCurves->cellWidget(4, 2*mm+1);
            
            if (allCurves.findIndex(lst[mm])>=0)
            {
                curves->setCurrentItem(allCurves.findIndex(lst[mm]));
            }
        }
    }
    
    //+++ Session::Limits::Left
    if (parameters.findIndex("Session::Limits::Left")>=0)
    {
        s=w->text(parameters.findIndex("Session::Limits::Left"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        for (int pp=0; pp<gsl_min(p,lst.count());pp++)
        {
            tableControl->item(pp,0)->setText(lst[pp]);
        }
    }
    
    //+++ Session::Limits::Right
    if (parameters.findIndex("Session::Limits::Right")>=0)
    {
        s=w->text(parameters.findIndex("Session::Limits::Right"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        for (int pp=0; pp<gsl_min(p,lst.count());pp++)
        {
            tableControl->item(pp,4)->setText(lst[pp]);
        }
    }
    
    if (checkBoxSANSsupport->isChecked())
    {
        //+++ Session::Resolution::Use
        if (parameters.findIndex("Session::Resolution::Use")>=0)
        {
            s=w->text(parameters.findIndex("Session::Resolution::Use"),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            
            for (int mm=0;mm<M;mm++)
            {
                QTableWidgetItem *item = (QTableWidgetItem *)tableCurves->item (5,2*mm);
                if (lst[mm]=="1") item->setCheckState(Qt::Checked);
                else item->setCheckState(Qt::Unchecked);
            }
        }
        
        //+++ Session::Resolution::Datasets
        if (parameters.findIndex("Session::Resolution::Datasets")>=0)
        {
            s=w->text(parameters.findIndex("Session::Resolution::Datasets"),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            
            for (int mm=0;mm<M;mm++)
            {
                allCurves.clear();
                allCurves=app()->columnsList(Table::xErr);
                QString tableName=((QComboBoxInTable*)tableCurves->cellWidget(0, 2*mm+1))->currentText();
                tableName=tableName.left(tableName.lastIndexOf("_"));
                allCurves=allCurves.grep(tableName);
                
                QComboBoxInTable* curves= (QComboBoxInTable*)tableCurves->cellWidget(5, 2*mm+1);
                
                if (allCurves.findIndex(lst[mm])>=0)
                {
                    curves->setCurrentItem(allCurves.findIndex(lst[mm]));
                }
            }
        }
        
        //+++ Session::Polydispersity::Use
        if (parameters.findIndex("Session::Polydispersity::Use")>=0)
        {
            s=w->text(parameters.findIndex("Session::Polydispersity::Use"),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            
            for (int mm=0;mm<M;mm++)
            {
                QTableWidgetItem *item = (QTableWidgetItem *)tableCurves->item (6,2*mm);
                if (lst[mm]=="1") item->setCheckState(Qt::Checked);
                else item->setCheckState(Qt::Unchecked);
            }
        }
        
        //+++ Session::Polydispersity::Datasets
        if (parameters.findIndex("Session::Polydispersity::Datasets")>=0)
        {
            s=w->text(parameters.findIndex("Session::Polydispersity::Datasets"),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            allCurves.clear();
            allCurves=F_paraList;
            
            for (int mm=0;mm<M;mm++)
            {
                QComboBoxInTable* curves= (QComboBoxInTable*)tableCurves->cellWidget(6, 2*mm+1);
                
                if (allCurves.findIndex(lst[mm])>=0)
                {
                    curves->setCurrentItem(allCurves.findIndex(lst[mm]));
                }
            }
        }
        
        //+++ Session::Options::Reso
        if (parameters.findIndex("Session::Options::Reso")>=0)
        {
            s=w->text(parameters.findIndex("Session::Options::Reso"),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            //+++
            comboBoxResoFunction->setCurrentItem(lst[0].toInt());
            //+++
            comboBoxSpeedControlReso->setCurrentItem(lst[1].toInt());
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
        if (parameters.findIndex("Session::Options::Poly")>=0)
        {
            s=w->text(parameters.findIndex("Session::Options::Poly"),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            //+++
            comboBoxPolyFunction->setCurrentItem(lst[0].toInt());
            //+++
            comboBoxSpeedControlPoly->setCurrentItem(lst[1].toInt());
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
    if (parameters.findIndex("Session::Options::Fit::Control")>=0)
    {
        s=w->text(parameters.findIndex("Session::Options::Fit::Control"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        int currentItem=0;
        comboBoxFitMethod->setCurrentItem(lst[currentItem].toInt()); currentItem++;
        comboBoxSimplex->setCurrentItem(lst[currentItem].toInt()); currentItem++;
        comboBoxLevenberg->setCurrentItem(lst[currentItem].toInt()); currentItem++;
        //+++
        spinBoxMaxIter->setValue(lst[currentItem].toInt());currentItem++;
        //+++
        lineEditTolerance->setText(lst[currentItem]);currentItem++;
        //+++
        lineEditToleranceAbs->setText(lst[currentItem]);currentItem++;
        //+++
        spinBoxSignDigits->setValue(lst[currentItem].toInt());currentItem++;
        //+++
        comboBoxWeightingMethod->setCurrentItem(lst[currentItem].toInt());currentItem++;
        //+++
        if (lst[currentItem]=="1") checkBoxCovar->setChecked(true); else checkBoxCovar->setChecked(false);
        currentItem++;
        //+++
        lineEditWA->setText(lst[currentItem]);currentItem++;
        //+++
        lineEditWB->setText(lst[currentItem]);currentItem++;
        //+++
        lineEditWC->setText(lst[currentItem]);currentItem++;
        //+++
        lineEditWXMAX->setText(lst[currentItem]);currentItem++;
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
    if (parameters.findIndex("Session::Options::Instrument::Reso")>=0)
    {
        s=w->text(parameters.findIndex("Session::Options::Instrument::Reso"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        if (lst.count()==6)
        {
            int value=lst[0].toInt();
            if (value >-1 && value<=4) comboBoxResoFunction->setCurrentItem(value);
            //+++
            value=lst[1].toInt();
            if (value >-1 && value<=8) comboBoxSpeedControlReso->setCurrentItem(value);
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
    if (parameters.findIndex("Session::Options::Instrument::Poly")>=0)
    {
        s=w->text(parameters.findIndex("Session::Options::Instrument::Poly"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        if (lst.count()==6)
        {
            int value=lst[0].toInt();
            if (value >-1 && value<=4) comboBoxPolyFunction->setCurrentItem(value);
            //+++
            value=lst[1].toInt();
            if (value >-1 && value<=8) comboBoxSpeedControlPoly->setCurrentItem(value);
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
        if (parameters.findIndex("Session::Parameters::Shared")>=0)
        {
            s=w->text(parameters.findIndex("Session::Parameters::Shared"),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            
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
        if (parameters.findIndex(uselName)>=0)
        {
            s=w->text(parameters.findIndex(uselName),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            
            for (int mm=0; mm<M;mm++)
            {
                QTableWidgetItem *active = (QTableWidgetItem *)tablePara->item(pp,3*mm+1);
                if (lst[mm]=="1") active->setCheckState(Qt::Checked);
                else active->setCheckState(Qt::Unchecked);		;
            }
        }
        
        QString cellName="Session::Parameters::Values::"+QString::number(pp+1);
        
        //+++ useName
        if (parameters.findIndex(cellName)>=0)
        {
            s=w->text(parameters.findIndex(cellName),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            
            for (int mm=0; mm<M;mm++)
            {
                tablePara->item(pp,3*mm+2)->setText(QString::number(lst[mm].toDouble(),'G',spinBoxSignDigits->value()));
            }
        }

        QString rangeName="Session::Parameters::Ranges::"+QString::number(pp+1);
        
        //+++ useName
        if (parameters.findIndex(rangeName)>=0)
        {
            s=w->text(parameters.findIndex(rangeName),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            
            for (int mm=0; mm<M;mm++)
            {
                //std::cout<<lst[mm].latin1()<<"\n"<<flush;
                QTableWidgetItem *range = (QTableWidgetItem *)tablePara->item(pp,3*mm+1);
                if (lst[mm].contains("..") || lst[mm].contains("-+")) range->setText(lst[mm].remove(" "));
            }
        }
    }
    
    //+++ Session::Parameters::Errors
    if (parameters.findIndex("Session::Parameters::Errors")>=0)
    {
        s=w->text(parameters.findIndex("Session::Parameters::Errors"),1).remove(" <").stripWhiteSpace();
        lst.clear();
        lst=lst.split(" ",s,false);
        
        int iii=0;
        for (int i=0; i<M;i++) for (int pp=0;pp<p;pp++)
        {
            tablePara->item(pp,3*i+3)->setText(lst[iii]);
            iii++;
        }
    }
    
    //+++ Simulate::Statistics
    if (parameters.findIndex("Session::Fit::SaveSession")>=0)
    {
        s=w->text(parameters.findIndex("Session::Fit::SaveSession"),1).remove(" <").stripWhiteSpace();
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
    if (parameters.findIndex("Session::Chi2")>=0)
    {
        s=w->text(parameters.findIndex("Session::Chi2"),1).remove(" <").stripWhiteSpace();
        textLabelChi->setText(s);
    }
    //+++ Session::R2
    if (parameters.findIndex("Session::R2")>=0)
    {
        s=w->text(parameters.findIndex("Session::R2"),1).remove(" <").stripWhiteSpace();
        textLabelR2->setText(s);
    }
    //+++ Session::Time
    if (parameters.findIndex("Session::Time")>=0)
    {
        s=w->text(parameters.findIndex("Session::Time"),1).remove(" <").stripWhiteSpace();
        textLabelTime->setText(s);
    }
    
    //+++ Simulate::Color
    if (parameters.findIndex("Simulate::Color")>=0)
    {
        s=w->text(parameters.findIndex("Simulate::Color"),1).remove(" <").stripWhiteSpace();
        comboBoxColor->setCurrentItem(s.toInt());
    }
    
    //+++ Simulate::Color::Indexing
    if (parameters.findIndex("Simulate::Color::Indexing")>=0)
    {
        s=w->text(parameters.findIndex("Simulate::Color::Indexing"),1).remove(" <").stripWhiteSpace();
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
    if (parameters.findIndex("Simulate::Statistics")>=0)
    {
        s=w->text(parameters.findIndex("Simulate::Statistics"),1).remove(" <").stripWhiteSpace();
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
    if (parameters.findIndex("Simulate::SaveSession")>=0)
    {
        s=w->text(parameters.findIndex("Simulate::SaveSession"),1).remove(" <").stripWhiteSpace();
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
    if (parameters.findIndex("Simulate::Indexing")>=0)
    {
        s=w->text(parameters.findIndex("Simulate::Indexing"),1).remove(" <").stripWhiteSpace();
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
    if (parameters.findIndex("Simulate::Uniform")>=0)
    {
        s=w->text(parameters.findIndex("Simulate::Uniform"),1).remove(" <").stripWhiteSpace();
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
        if (parameters.findIndex("Simulate::Uniform::Parameters")>=0)
        {
            s=w->text(parameters.findIndex("Simulate::Uniform::Parameters"),1).remove(" <").stripWhiteSpace();
            lst.clear();
            lst=lst.split(" ",s,false);
            
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
    
    if(!findActiveGraph(g)){QMessageBox::critical(this,tr("QtiKWS"), tr("Activate first GRAPH with data to fit !!!")); return false;};
    
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
            sss=sss.right(sss.length()-sss.find("color=")-6);
            if (sss.contains(" ")) sss=sss.left(sss.find(" "));
            comboBoxColor->setCurrentIndex(comboBoxColor->findText(sss.replace("_"," ")));
        }

        if (!lineEditEFIT->text().contains("no-fit") )fitOrCalculate(false);
        else if (lineEditEFIT->text().contains("yes-simulate") )fitOrCalculate(true);
        
        comboBoxColor->setCurrentIndex(currentColor);
        
        if (lineEditEFIT->text().contains("name="))
        {
            QString sss=lineEditEFIT->text();
            sss=sss.right(sss.length()-sss.find("name=")-5);
            if (sss.contains(" ")) sss=sss.left(sss.find(" "));

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


