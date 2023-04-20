/***************************************************************************
 File                   : compile-explorer.cpp
 Project                : QtiSAS
 --------------------------------------------------------------------
 Copyright              : (C) 2017-2021 by Vitaliy Pipich
 Email (use @ for *)    : v.pipich*gmail.com
 Description            : compile interface: explorer functions
 
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
#include "compile18.h"

void compile18::newFunctionName()
{
    if (listBoxFunctionsNew->selectionModel()->selectedRows().count()==0) return;
    
    if ( lineEditFunctionName->text()!=listBoxFunctionsNew->selectionModel()->selectedRows()[0].data().toString()
        ) listBoxFunctionsNew->clearSelection();
}


void compile18::newCategoryName()
{
    if (listBoxGroupNew->selectionModel()->selectedRows().count()==0) return;
    if (listBoxGroupNew->selectionModel()->selectedRows()[0].data().toString()=="ALL") return;
    
    if ( lineEditGroupName->text() != listBoxGroupNew->selectionModel()->selectedRows()[0].data().toString())
    {
        listBoxGroupNew->clearSelection();
        listBoxFunctionsNew->model()->removeColumn(0);
    }
}

//*******************************************
//+++  stot1Dto2D()
//*******************************************
void compile18::stot1Dto2D()
{
    if (radioButton1D->isChecked())
    {
        spinBoxXnumber->setMinimum(1);
        spinBoxXnumber->setValue(1);
        spinBoxXnumber->setEnabled(false);
        lineEditXXX->setText("x");
        //pushButtonMenuSANS->show();
        spinBoxXnumber->hide();
    }
    else
    {
        spinBoxXnumber->setMinimum(2);
        spinBoxXnumber->setValue(2);
        spinBoxXnumber->setEnabled(true);
        lineEditXXX->setText("ix,iy");
        pushButtonMenuSANS->hide();
        spinBoxXnumber->show();
    }
    scanGroups();
    
    textEditCode->clear();
    //textEditCode->setText("// --->  f = f ( "+lineEditXXX->text().remove(" ")+", {P1, ... } );");
    changedFXYinfo();
}

//*******************************************
//+++ pathChanged
//*******************************************
void compile18::pathChanged()
{
    scanGroups();
    scanIncludedFunctions();
}

//*******************************************
//+++ scan FitFunction dir on groups
//*******************************************
void compile18::scanGroups()
{
    int i;
    QString s;
    QStringList group;
    QString groupName;
    
    //group<<"ALL";
    
    QDir d(pathFIF);
    //+++
    QString fifExt="*.";
    if (radioButton2D->isChecked()) fifExt+="2d";
    fifExt+="fif";
    //+++
    QStringList lst = d.entryList(QStringList() << fifExt);
    
    
    for(i=0;i<lst.count();i++)
    {
        if (d.exists(lst[i]))
        {
            QFile f(pathFIF+"/"+lst[i]);
            f.open( QIODevice::ReadOnly );
            QTextStream t( &f );
            
            //+++[group]
            s = t.readLine();
            if (s.contains("[group]"))
            {
                groupName=t.readLine().trimmed();
                if (!group.contains(groupName) && groupName!="") group<<groupName;
            }
            
            f.close();
        }
    }
    group.sort();
    group.prepend("ALL");
    
    lineEditGroupName->setText("");
    lineEditFunctionName->setText("");
    if (textLabelInfoVersion->isHidden()) textLabelInfoSAS->setText("");
    
    
    //+++ 2020-06
    listBoxGroupNew->setModel(new QStringListModel(group));

    connect(listBoxGroupNew->selectionModel(),SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),this, SLOT(groupFunctions(const QModelIndex &, const QModelIndex &)));
    
    QStringList empty;
    listBoxFunctionsNew->setModel(new QStringListModel(empty));
    //+++
    
}

//*******************************************
//+++  scan Included Functions
//*******************************************
void compile18::scanIncludedFunctions()
{
    int i;
    QString s;
    QString fileName;
    
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
    QString groupName=index.data().toString();
    lineEditGroupName->setText(groupName);
    
    QStringList functions=groupFunctions(groupName);
    functions.sort();

    
    listBoxFunctionsNew->setModel(new QStringListModel(functions));
    
    connect(listBoxFunctionsNew->selectionModel(),SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),this, SLOT(openFIFfileSimple(const QModelIndex &, const QModelIndex &)));
    
    if (functions.count()==0) spinBoxPara->setValue(0);
    
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
QStringList compile18::groupFunctions( const QString &groupName )
{
    QString s;
    QStringList functions;
    
    QDir d(pathFIF);
    
    QString fifExt=".";
    if (radioButton2D->isChecked()) fifExt+="2d";
    fifExt+="fif";
    //+++
    QStringList lst = d.entryList(QStringList() << "*" + fifExt);
    
    
    for(int i=0;i<lst.count();i++)
    {
        if (d.exists (lst[i]))
        {
            QFile f(pathFIF+"/"+lst[i]);
            f.open( QIODevice::ReadOnly );
            QTextStream t( &f );
            
            //+++[group]
            s = t.readLine();
            if (s.contains("[group]"))
            {
                if (groupName=="ALL" || t.readLine().trimmed()==groupName) functions<<lst[i].remove(fifExt);
            }
            
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
    QString file=index.data().toString();

    textLabelInfoVersion->hide();
    textLabelInfoSASauthor->hide();
    /*
    QList<int> lst;
    lst<<135<<85<<135<<5;
    splitterMain->setSizes(lst);
    */
    
    pathUpdate();
    QString fifExt=".";
    if (radioButton2D->isChecked()) fifExt+="2d";
    fifExt+="fif";
    
    QString ss=pathFIF+"/"+file+fifExt;
    openFIFfile(ss);
    
}


//*******************************************
//+++  open FIF: from listbox
//*******************************************
void compile18::openFIFfileSimple()
{
    textLabelInfoVersion->hide();
    textLabelInfoSASauthor->hide();
    /*
    QList<int> lst;
    lst<<135<<85<<135<<5;
    splitterMain->setSizes(lst);
    */
    
    pathUpdate();
    QString fifExt=".";
    if (radioButton2D->isChecked()) fifExt+="2d";
    fifExt+="fif";
    
    QString ss=pathFIF+"/"+listBoxFunctionsNew->selectionModel()->selectedRows()[0].data().toString()+fifExt;
    openFIFfile(ss);
}

//*******************************************
//+++  open FIF: from file
//*******************************************
void compile18::openFIFfile()
{
    QString filter = tr("QtiSAS function") + " (*.fif);;";
    filter += tr("QtiSAS 2D-function") + " (*.2dfif);;";
    filter += tr("Origin function") + " (*.fdf *.FDF);;";
    
    //+++
    QString fn = QFileDialog::getOpenFileName(this, "QtiSAS - Fit - Function", pathFIF, filter, 0);
    
    
    newFIF();
    
    if (fn.contains("fif", Qt::CaseSensitive))
    {
        openFIFfile(fn);
        scanGroups();
    }
    else if (fn.contains(".fdf", Qt::CaseInsensitive))
    {
        openOrigin(fn);
    }
    
}

//*******************************************
//+++ open FIF: code
//*******************************************
void compile18::openFIFfile(const QString& fifName)
{
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
    
    
    cursor = textEditForwardFortran->textCursor();
    cursor.movePosition( QTextCursor::End );
    textEditForwardFortran->setTextCursor( cursor );
    
    textEditForwardFortran->clear();
 
    fortranFunction->clear();
    
    if ( fifName.contains(".2dfif") ) radioButton2D->setChecked(true);
    
    int i;
    spinBoxP->setValue(0);
    
    //+++
    if (fifName.isEmpty())
    {
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> no FIF file, 1"));
        return;
    }
    
    //+++
    if (!QFile::exists (fifName))
    {
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> no FIF file, 2"));
        return;
    }
    
    //+++
    QFile f(fifName);
    
    //+++
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        QMessageBox::critical(0, tr("QtiSAS"),
                              tr(QString("Could not write to file: <br><h4>" + fifName+
                                 "</h4><p>Please verify that you have the right to read from this location!").toLocal8Bit().constData()));
        return;
    }
    else
    {
        QTextStream t( &f );
        QString s;
        
        //+++[group]
        s = t.readLine();
        if (s.contains("[group]")==0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [group]"));
            return;
        }
        else
        {
            s.remove("[group] ");
            if (s.contains("[eFit]")) checkBoxEfit->setChecked(true);	else checkBoxEfit->setChecked(false);
            s.remove("[eFit] ");
            if (s.contains("["))
            {
                lineEditEFIT->setText(s.left(s.indexOf("[")));
                s=s.right(s.length()-s.indexOf("["));
            }
            else lineEditEFIT->setText(s);
            if (s.contains("[Weight]"))
            {
                checkBoxWeight->setChecked(true);
                s.remove("[Weight] ");
                comboBoxWeightingMethod->setCurrentIndex(s.left(1).toInt());
                s=s.right(s.length()-2);
            }
            else
            {
                checkBoxWeight->setChecked(false);
                comboBoxWeightingMethod->setCurrentIndex(0);
            }
            if (s.contains("[Superpositional]"))
            {
                checkBoxSuperpositionalFit->setChecked(true);
                s.remove("[Superpositional] ");
                QStringList sLst = s.split(" ", QString::SkipEmptyParts);
                spinBoxSubFitNumber->setValue(sLst[0].toInt());
            }
            else
            {
                checkBoxSuperpositionalFit->setChecked(false);
                spinBoxSubFitNumber->setValue(1);
            }
            lineEditFitMethodPara->setText("");
            if (s.contains("[Algorithm]"))
            {
                checkBoxAlgorithm->setChecked(true);
                s.remove("[Algorithm] ");
                comboBoxFitMethod->setCurrentIndex(s.left(1).toInt());
                s=s.right(s.length()-2);
                if ( s.contains("[") ) lineEditFitMethodPara->setText(s.left(s.indexOf("[")));
                else lineEditFitMethodPara->setText(s);
            }
            else
            {
                checkBoxAlgorithm->setChecked(false);
                comboBoxFitMethod->setCurrentIndex(0);
            }
        }
        
        /*
         if (s.find("[Function from Standard Library]")>0)
         {
         pushButtonSave->setEnabled(false);
         pushButtonDelete->setEnabled(false);
         }
         else
         {
         pushButtonSave->setEnabled(true);
         pushButtonDelete->setEnabled(true);
         }
         */
        //+++group Name
        QString groupName=t.readLine().trimmed();
        if (groupName=="") groupName="ALL";
        lineEditGroupName->setText(groupName);
        //+++ skip
        s = t.readLine();
        
        //+++[name]
        s = t.readLine();
        if (s.contains("[name]")==0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [name]"));
            return;
        }
        
        //+++ function Name
        QString functionName=t.readLine().trimmed();
        if (functionName=="")
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [name]"));
            return;
        }
        lineEditFunctionName->setText(functionName);
        textLabelInfoSAS->setText(functionName);
        
        //+++ skip
        s = t.readLine();
        
        //+++[number parameters]
        s = t.readLine();
        if (s.contains("[number parameters]")==0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [number parameters]"));
            return;
        }
        //+++[number parameters]
        int pNumber=t.readLine().remove(",").trimmed().toInt();
        if (pNumber<=0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [number parameters]"));
            return;
        }
        spinBoxP->setValue(pNumber);
        
        //+++ skip
        s = t.readLine();
        //+++[description]
        s = t.readLine();
        if (s.contains("[description]")==0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [description]"));
            return;
        }
        //+++[description]
        s = t.readLine();
        textEditDescription->clear();
        QString html;
        while(s.contains("[x]")==0)
        {
            html+=s;
            //textEditDescription->append(s);
            s = t.readLine();
        }
        html=html.remove("\n\n");
        textEditDescription->insertHtml(html);
        
        //+++[x]
        s=t.readLine().trimmed();
        if (radioButton2D->isChecked())
        {
            QStringList lst = s.remove(" ").split(",", QString::SkipEmptyParts);
            
            spinBoxXnumber->setValue(lst.count());
            lineEditXXX->setText(s.remove(" "));
        }
        else
        {
            lineEditXXX->setText(s.remove(" "));
        }
        
        //+++ skip
        s = t.readLine();
        
        //+++[y]
        s = t.readLine();
        if (s.contains("[y]")==0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [y]"));
            return;
        }
        lineEditY->setText(t.readLine().trimmed());
        
        //+++ skip
        s = t.readLine();
        //+++[parameter name]
        s = t.readLine();
        if (s.contains("[parameter names]")==0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [parameter names]"));
            return;
        }
        //+++[parameter names]
        s = t.readLine().trimmed();
        QStringList paraNames = s.split(",", QString::SkipEmptyParts);
        if (paraNames.size()!=pNumber)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [parameter names]"));
        }
        
        //+++ skip
        s = t.readLine();
        //+++[initial values]
        s = t.readLine();
        if (s.contains("[initial values]")==0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [initial values]"));
            return;
        }
        //+++[initial values]
        s 	= t.readLine().trimmed();
        QStringList initValues = s.split(",", QString::SkipEmptyParts);
        if (initValues.size()!=pNumber)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [initial values]"));
        }
        
        //+++ skip
        s = t.readLine();
        //+++[adjustibility]
        s = t.readLine();
        if (s.contains("[adjustibility]")==0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [adjustibility]"));
            return;
        }
        //+++[adjustibility]
        s = t.readLine().trimmed();
        QStringList adjustibilityList = s.split(",", QString::SkipEmptyParts);
        if (adjustibilityList.size()!=pNumber)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [adjustibility]"));
        }
        
        //+++ skip
        s = t.readLine();
        //+++[parameter description]
        s = t.readLine();
        if (s.contains("[parameter description]")==0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [parameter description]"));
            return;
        }
        //+++[parameter description]
        s = t.readLine().trimmed();
        QStringList paraDescription = s.split(",", QString::SkipEmptyParts);
        if (paraDescription.size()!=pNumber)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [parameter description]"));
        }
        
        for(i=0; i<pNumber; i++)
        {
            //+++
            tableParaNames->item(i,0)->setText(paraNames[i]);
            
            QString sCurrent=initValues[i];
            if (sCurrent.contains('[')) tableParaNames->item(i,1)->setText( QString::number(sCurrent.left(sCurrent.indexOf("[")).toDouble()));
            else
                tableParaNames->item(i,1)->setText(QString::number(sCurrent.toDouble()));
            
            //+++
            if (sCurrent.contains('[') && sCurrent.contains("..") && sCurrent.contains(']'))
                tableParaNames->item(i,2)->setText(sCurrent.right(sCurrent.length()-sCurrent.indexOf("[")-1).remove("[").remove("]"));
            else tableParaNames->item(i,2)->setText(".."); //qt4
            
            if (adjustibilityList[i].toInt()==1)
                tableParaNames->item(i,2)->setCheckState(Qt::Checked); //qt4
            else
                tableParaNames->item(i,2)->setCheckState(Qt::Unchecked); //qt4

            tableParaNames->item(i,3)->setText(paraDescription[i]);
        }
        
        //+++ skip
        s = t.readLine();
        //+++ [h-headers]
        s = t.readLine();
        if (s.contains("[h-headers]")==0)
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [h-headers]"));
            return;
        }
        //+++[h-headers]
        s = t.readLine();
        textEditHFiles->clear();
        while(s.contains("[included functions]")==0)
        {
            textEditHFiles->append(s);
            s = t.readLine();
        }
        
        //+++ [included functions]
        QString ssss;
        s = t.readLine();
        textEditFunctions->clear();
        while(s.contains("[code]")==0)
        {
            ssss+=s+"\n";
            s = t.readLine();
        }
        
        textEditFunctions->append(ssss.left(ssss.length()-2));
        textEditFunctions->moveCursor(QTextCursor::Start,QTextCursor::MoveAnchor);
        
        //+++[code]
        ssss="";
        s = t.readLine();
        textEditCode->clear();
        while(s.contains("[fortran]" )==0 && s.contains("[end]")==0)
        {
            ssss+=s+"\n";
            s = t.readLine();
        }
        
        textEditCode->append(ssss.left(ssss.length()-2));
        textEditCode->moveCursor(QTextCursor::Start,QTextCursor::MoveAnchor);
        
        if (s.contains("[fortran]"))
        {
            //+++[fortran]
            s = t.readLine().remove(",");
            
            if (!s.contains("1")) checkBoxAddFortran->setChecked(false);
            else
            {
                checkBoxAddFortran->setChecked(true);
                s=t.readLine();
                fortranFunction->setText(s);
                s=t.readLine();
                textEditForwardFortran->clear();
                while(s.contains("[end]")==0)
                {
                    textEditForwardFortran->append(s);
                    s = t.readLine();
                }
            }
        }
        else
        {
            checkBoxAddFortran->setChecked(false);
            fortranFunction->setText("");
            textEditForwardFortran->clear();
        }
        
    }
    f.close();
    
    //radioButtonCPP->setText(lineEditFunctionName->text()+".cpp");
    //radioButtonFIF->setText(lineEditFunctionName->text()+".fif");
    updateFiles2();
    
    makeCPP();
}

//*******************************************
//+++  updateFiles2
//*******************************************
void compile18::updateFiles2()
{
    if ( radioButtonFIF->isChecked() && pushButtonSave->isEnabled())
    {
        makeFIF();
    }
    
    if ( radioButtonCPP->isChecked() ) makeCPP();
    if ( radioButtonBAT->isChecked() ) makeBATnew();
}

//*******************************************
//+++  make CPP file
//*******************************************
void compile18::makeCPP()
{
    QString fn=pathFIF+"/"+lineEditFunctionName->text().trimmed()+".cpp";
    
    if (lineEditFunctionName->text().trimmed()=="")
    {
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> Fill Function name!"));
        return;
    }
    saveAsCPP(fn);
    
}

//*******************************************
//+++  make BAT file
//*******************************************
void compile18::makeBATnew()
{
    //+++ extension
    QString ext="";
#if defined(Q_OS_WIN)
    ext=".dll";
#elif defined(Q_OS_MAC)
    ext=".dylib";
#else
    ext=".so";
#endif
    
    //+++ 2D or 1D
    if (radioButton2D->isChecked()) ext+="2d";
    
    pathFIF=fitPath->text();
    pathMinGW=mingwPathline->text();
    pathGSL=gslPathline->text();
    
    QString fn=pathFIF+"/BAT.BAT";
    
    QString text; // body
    QString fortranText=""; // fortran o-file
    
    QString compileFlags=lineEditCompileFlags->text();
    QString linkFlags=lineEditLinkFlags->text();
    
    text ="";
    
    
    
#if defined(Q_OS_WIN)
    fn=fn.replace("\/","\\");
    pathFIF=pathFIF.replace("\/","\\");
    pathMinGW=pathMinGW.replace("\/","\\");
    pathGSL=pathGSL.replace("\/","\\");
    
    if (pathFIF.contains(":"))
    {
        text=text+pathFIF.left(pathFIF.indexOf(":")+1)+"\n";
    }
    
    
    text=text+"cd "+"\""+pathFIF+"\""+"\n";
    
    //+++ %COMPILER%
    
    text=text+"set MINGW_IN_SHELL=1\n";
    if (checkBoxCompilerLocal->isChecked())
    {
        // 2012-09-20  text=text+"set COMPILER="+"\""+pathMinGW+"\""+"\n";
        text=text+"set COMPILER="+pathMinGW+"\n";
        text=text+"set PATH=%COMPILER%/bin;%PATH%\n";
    }
    
    //+++ %GSL%
    if (checkBoxGSLlocal->isChecked())
    {
        text=text+"set GSL="+"\""+pathGSL+"\""+"\n";
    }
    
    
    
    compileFlags=compileFlags.replace("$GSL","%GSL%");
    linkFlags=linkFlags.replace("$GSL","%GSL%");
    linkFlags=linkFlags.replace("$COMPILER","%COMPILER%");
    
#else
    text=text+"cd "+pathFIF+"\n";
    //+++ $COMPILER
    if (checkBoxCompilerLocal->isChecked())
    {
        text=text+"export COMPILER="+"\""+pathMinGW+"\""+"\n";
        text=text+"export PATH=$COMPILER/bin:$PATH\n";
    }
    
    //+++ $GSL
    if (checkBoxGSLlocal->isChecked())
    {
        text=text+"export GSL="+"\""+pathGSL+"\""+"\n";
    }
    
#endif
    
    text =text+ compileFlags+" "+lineEditFunctionName->text().trimmed()+".cpp\n";
    
    if (checkBoxAddFortran->isChecked())
    {
        QString gfortranlib="";
        
#if defined(Q_OS_WIN)
        text =text+ compileFlags+" "+"\""+fortranFunction->text()+"\""+" -o "+"\""+"fortran.o"+"\""+" \n";
#elif defined(Q_OS_MAC)
        if (!compileFlags.contains("-arch x86_64")) compileFlags=compileFlags.replace("g++ ","/usr/local/bin/gfortran -arch x86_64 ").remove(" -I$GSL");
        else compileFlags=compileFlags.replace("g++ ","/usr/local/bin/gfortran ").remove(" -I$GSL");
        text =text+ compileFlags+"  "+fortranFunction->text()+" -o "+"fortran.o"+"\n";
        text=text+"gfortranPath=\"$(dirname `/usr/local/bin/gfortran --print-file-name libgfortran.a`)\"; gfortranPath=${gfortranPath%x*4}\n";
        gfortranlib=" -L$gfortranPath -lgfortran ";
#else
        text =text+ compileFlags+"  "+fortranFunction->text()+" -o "+"fortran.o"+"\n";
#endif
        fortranText=gfortranlib+" fortran.o";
    }
    
    linkFlags=linkFlags.replace(" -o","  -o " +lineEditFunctionName->text().trimmed()+ext+"  " +lineEditFunctionName->text().trimmed()+".o" +fortranText);
    text =text+ linkFlags+"  ";
#if defined(Q_OS_WIN)
    if (checkBoxAddFortran->isChecked()) text =text+"%COMPILER%/lib/libgfortran.a"+"  ";
#endif
    text =text+"\n";
    
    if (checkBoxAddFortran->isChecked())
    {
#if defined(Q_OS_WIN)
        text =text+ "del "+"\""+"fortran.o"+"\""+" \n ";
#else
        text =text+ "rm "+"fortran.o\n";
#endif
    }
    
#if defined(Q_OS_WIN)
    text =text+"del "+lineEditFunctionName->text().trimmed()+".o\n" ;
    text.replace("/","\\");
#else
    text =text+ "rm "+lineEditFunctionName->text().trimmed()+".o\n" ;
#endif
    
    QFile f(fn);
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                              tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
        return;
    }
    QTextStream t( &f );
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << text;
    f.close();
    
    if ( radioButtonBAT->isChecked() )
    {
        tableCPP->setRowCount(0);
        QStringList lst = text.split("\n", QString::KeepEmptyParts);
        tableCPP->setRowCount(lst.count());
        for (int ii=0; ii<lst.count();ii++) tableCPP->setItem(ii,0,new QTableWidgetItem(lst[ii]));
    }
    
}

//*******************************************
//+++  save FIF file /slot
//*******************************************
void compile18::makeFIF()
{
    
    //+++
    QString fifExt=".";
    if (radioButton2D->isChecked()) fifExt+="2d";
    fifExt+="fif";
    //+++
    QString nameOld="";
    if (listBoxFunctionsNew->selectionModel()->selectedRows().count()>0)nameOld=listBoxFunctionsNew->selectionModel()->selectedRows()[0].data().toString()+fifExt;
    //+++
    QString fn=pathFIF+"/"+lineEditFunctionName->text().trimmed()+fifExt;
    if (!save(fn,false)) return;
    //+++
    QString group=lineEditGroupName->text().trimmed();
    QString name=lineEditFunctionName->text().trimmed()+fifExt;
    if (nameOld!=name)
    {
        scanGroups();
        const QModelIndexList indexes = listBoxGroupNew->model()->match(listBoxGroupNew->model()->index(0,0),Qt::DisplayRole,group,1,Qt::MatchExactly);
        if (indexes.count()>0) listBoxGroupNew->selectionModel()->select(indexes[0], QItemSelectionModel::Select);
        groupFunctions(indexes[0],indexes[0]);
        const QModelIndexList indexesFunctions = listBoxFunctionsNew->model()->match(listBoxFunctionsNew->model()->index(0,0),Qt::DisplayRole,name.remove(fifExt),1,Qt::MatchExactly);
         if (indexesFunctions.count()>0) listBoxFunctionsNew->selectionModel()->select(indexesFunctions[0], QItemSelectionModel::Select);
        openFIFfileSimple(indexesFunctions[0],indexesFunctions[0]);
    }
}

//*******************************************
//+++  delete FIF file
//*******************************************
void compile18::deleteFIF()
{
    //+++
    QString fifExt=".";
    if (radioButton2D->isChecked()) fifExt+="2d";
    fifExt+="fif";
    
    QString fn=listBoxFunctionsNew->selectionModel()->selectedRows()[0].data().toString().trimmed()+fifExt;
    if (fn=="") return;
    
    int functionCount=listBoxFunctionsNew->model()->rowCount();
    
    if ( QMessageBox::question(this, tr("QtiSAS::Delete Function? "), tr( "Do you want to delete Function %1?").arg(fn), tr("&Yes"), tr("&No"),QString::null, 0, 1 ) ) return;
    
    QDir d(pathFIF );
    d.remove(fn);
    d.remove(fn.remove(fifExt, Qt::CaseInsensitive)+".cpp");
    
    QString group=lineEditGroupName->text().trimmed();
    
    scanGroups();

    const QModelIndexList indexes = listBoxGroupNew->model()->match(listBoxGroupNew->model()->index(0,0),Qt::DisplayRole,group,1,Qt::MatchExactly);
    
    //remove dll
    d.remove(fn.remove(fifExt, Qt::CaseInsensitive)+".dll");
    d.remove(fn.remove(fifExt, Qt::CaseInsensitive)+".so");
    d.remove(fn.remove(fifExt, Qt::CaseInsensitive)+".dylib");
    
    newFIF();
    
    if (functionCount>1)
    {
        if (indexes.count()>0) listBoxGroupNew->selectionModel()->select(indexes[0], QItemSelectionModel::Select);
        groupFunctions(indexes[0],indexes[0]);
    }
}

//*******************************************
//+++ save as FIF file /slot
//*******************************************
void compile18::saveasFIF()
{
    //+++
    QString fifExt=".";
    QString filter;
    
    if (radioButton1D->isChecked())
    {
        filter = tr("QtiSAS function") + " (*.fif);;";
    }
    else
    {
        filter = tr("QtiSAS 2D-function") + " (*.2dfif);;";
        fifExt+="2d";
    }
    
    fifExt+="fif";
    
    filter += tr("C++ function") + " (*.cpp);;";
    //+++
    QString selectedFilter;
    QString fn = QFileDialog::getSaveFileName(this,tr("Save Window As"),  pathFIF + lineEditGroupName->text().trimmed(), filter,
                                              &selectedFilter, QFileDialog::DontResolveSymlinks);
    
    QFileInfo fi(fn);
    QString baseName = fi.fileName();
    if (!baseName.contains("."))
    {
        if (selectedFilter.contains(fifExt)) save(fn+fifExt, true);
        else if (selectedFilter.contains("C++ function")) saveAsCPP(fn+".cpp");
        
    }
    
    
}


//*******************************************
//+++  save FIF file /function
//*******************************************
bool compile18::save( QString fn, bool askYN )
{
    int i;
    QString text;
    
    if (spinBoxP->value()==0)
    {
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> Check function!"));
        return false;
    }
    
    if (lineEditFunctionName->text().trimmed()=="")
    {
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> Fill Function name!"));
        return false;
    }
    
    
    if (askYN && QFile::exists(fn) &&  QMessageBox::question(this, tr("QtiSAS -- Overwrite File? "), tr("A file called: <p><b>%1</b><p>already exists.\n"   "Do you want to overwrite it?").arg(fn), tr("&Yes"), tr("&No"),QString::null, 0, 1 ) ) return false;
    
    else
    {
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
        if (spinBoxP->value()>1)
        {
            for(i=0;i<(spinBoxP->value()-1);i++)
            {
                if (tableParaNames->item(i,0)->text()=="") tableParaNames->item(i,0)->setText("P"+QString::number(i+1));
                text=text+tableParaNames->item(i,0)->text().trimmed()+',';
            }
        }
        
        if (tableParaNames->item(i,0)->text()=="") tableParaNames->item(i,0)->setText("P"+QString::number(i+1));
        text=text+tableParaNames->item(i,0)->text().trimmed()+',';
        text+="\n\n";
        
        text+="[initial values]\n";

        for(i=0;i<spinBoxP->value();i++)
        {
            QString valueCurrent=tableParaNames->item(i,1)->text().remove(" ");
            QString limitCurrent=tableParaNames->item(i,2)->text().remove(" ");
            
            if (limitCurrent=="..") limitCurrent="";
            else if (limitCurrent.contains(".."))
            {
                if (limitCurrent.endsWith("..")) limitCurrent+="inf";
                else if (limitCurrent.startsWith("..")) limitCurrent="-inf"+limitCurrent;
                limitCurrent="["+limitCurrent+"]";
                
            }
            text=text+QString::number(valueCurrent.toDouble())+limitCurrent;
            if (i<(spinBoxP->value()-1)) text=text+','; else text+="\n\n";
        }

        
        text+="[adjustibility]\n";
        for(i=0;i<(spinBoxP->value()-1);i++)
        {
            if (tableParaNames->item(i,2)->checkState()==Qt::Checked) text=text+'1'+','; else text=text+'0'+',';
        }
        if (tableParaNames->item(i,2)->checkState()==Qt::Checked) text=text+'1'; else text=text+'0';
        text+="\n\n";
        
        text+="[parameter description]\n";
        i=0;
        if (spinBoxP->value()>1)
        {
            for(i=0;i<(spinBoxP->value()-1);i++)
            {
                if (tableParaNames->item(i,3)->text().trimmed()=="") tableParaNames->item(i,3)->setText("---");
                text=text+tableParaNames->item(i,3)->text()+",";

            }
        }
        if (tableParaNames->item(i,3)->text().trimmed()=="") tableParaNames->item(i,3)->setText("---");
        text=text+tableParaNames->item(i,3)->text()+",";
        text+="\n\n";
        
        text+="[h-headers]\n";
        text+=textEditHFiles->toPlainText();
        text+="\n";
        
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
        
        text+="[end]";
        
        QFile f(fn);
        if ( !f.open( QIODevice::WriteOnly ) )
        {
            QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                                  tr("Could not writ<e to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
            return false;
        }
        QTextStream t( &f );
        t.setEncoding(QTextStream::UnicodeUTF8);
        t << text;
        f.close();
        
        if ( radioButtonFIF->isChecked() )
        {
            tableCPP->setRowCount(0);
            QStringList lst = text.split("\n", QString::KeepEmptyParts);
            tableCPP->setRowCount(lst.count());
            for (int ii=0; ii<lst.count();ii++) tableCPP->setItem(ii,0,new QTableWidgetItem(lst[ii]));
        }
    }
    
    return true;
}

QString ampersand_encode(const QString& str)
{    
    QString chr;
    QStringList list = QStringList();
    for (int i = 0; i < str.size(); ++i) {
        chr = QString(str[i]);
        if (QString(chr.toLatin1())!=chr) list << "&#" + QString::number(chr[0].unicode()) + ";";
        else list << chr;
    }
    return list.join("");
}


//*******************************************
//+++ saveAsCPP : new v. >2019-06
//*******************************************
void compile18::saveAsCPP( QString fn )
{
    if (radioButton2D->isChecked()) return saveAsCPP2d(fn);
    return saveAsCPP1d(fn);
}

//*******************************************
//+++ saveAsCPP1d : new v. >2019-06
//*******************************************
void compile18::saveAsCPP1d( QString fn )
{
        
    //+++
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
    //+++
    
    QStringList lstTmp=text.split("\n");
    
    lnTextEditHFiles->firstLineIncrement=lstTmp.count()-1; lnTextEditHFiles->updateLineNumbers(true);
    lstTmp.clear();
    
    QString textHF=textEditHFiles->toPlainText();
    QStringList lstHF=textHF.split("\n");
    
    for(i=0;i<lstHF.count();i++) if (lstHF[i].contains("#include")) text=text+lstHF[i]+"\n";
    
    text=text+"std::string fitFunctionPath="+'"';
    text+=fitPath->text().toAscii().constData();
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
    text+="static double " + tableParaNames->item(0,0)->text().trimmed();
    for(i=1;i<spinBoxP->value();i++)
    {
        text+=", "+tableParaNames->item(i,0)->text().trimmed();
    }
    text+=";\n";
    text+="static bool initNow;\n";
    text+="\n";
    text+="#define Para ((struct functionT *) ParaM)->para\n";
    text+="\n";
//    text=text+";\ndouble *XXX, *YYY, *dYYY, *RESO";
    text+="#define XXX   ((struct functionT *) ParaM)->Q\n";
    text+="#define YYY   ((struct functionT *) ParaM)->I\n";
    text+="#define dYYY  ((struct functionT *) ParaM)->dI\n";
    text+="#define RESO  ((struct functionT *) ParaM)->SIGMA\n";
    text+="#define prec  ((struct functionT *) ParaM)->prec\n";
    text+="\n";
    //    text=text+";\nint currentFirstPoint, currentLastPoint, currentPoint, currentInt";
    text+="#define listLastPoints    ((struct functionT *) ParaM)->listLastPoints\n";
    text+="#define currentM          ((struct functionT *) ParaM)->currentM\n";
    text+="#define currentFirstPoint ((struct functionT *) ParaM)->currentFirstPoint\n";
    text+="#define currentLastPoint  ((struct functionT *) ParaM)->currentLastPoint\n";
    text+="#define currentPoint      ((struct functionT *) ParaM)->currentPoint\n";
    text+="#define currentInt        ((struct functionT *) ParaM)->currentInt\n";
    text+="#define currentFunction   ((struct functionT *) ParaM)->currentFunction\n";
    text+="\n";
//    text=text+";\nbool beforeFit, afterFit, beforeIter, afterIter, initNow";
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
    for(i=0;i<spinBoxP->value();i++)
    {
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
    {
        text=text+"  gsl_vector_set(Para," +  QString::number(i) + "," + tableParaNames->item(i,0)->text().trimmed()+");\n";
    }
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
    for(i=0;i<spinBoxP->value();i++)
    {
        QString valueCurrent=tableParaNames->item(i,1)->text();
        QString limitCurrent=tableParaNames->item(i,2)->text().remove(" ");
        
        if (limitCurrent=="..") limitCurrent="";
        else if (limitCurrent.contains(".."))
        {
            if (limitCurrent.endsWith("..")) limitCurrent+="inf";
            else if (limitCurrent.startsWith("..")) limitCurrent="-inf"+limitCurrent;
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
    {
        if (tableParaNames->item(i,2)->checkState()==Qt::Checked) text=text+'1'+','; else text=text+'0'+',';
    }
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
    //+++
    QString htmlText=textEditDescription->toHtml().remove("\n").replace("\\;", "\\\\ \\\\;");;//text().remove("\n").replace("\\;", "\\\\ \\\\;");
    
    htmlText="<html><head><meta name= \"qrichtext \" content= \"1 \" /></head>"+htmlText.right(htmlText.length()-htmlText.indexOf("<body"));
    htmlText=ampersand_encode(htmlText);
    
    //htmlText.replace("\\"+'\\"', "\\"+"\\ \\"+'"');
    QString sss="";
    sss+="\\";
    sss+='"';
    QString richText=htmlText.replace('"',sss);
    
    /*
    int nChars=htmlText.length();
    for (i=0;i<nChars;i++)
    {
        if (htmlText[i]=='"') richText+=" \\";
        richText+=htmlText[i];
    }
    
    richText.replace("\\ ", "\\\\ ");
    richText.replace("\\\\\\", "\\\\");
    */
    
    
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
    
    if (checkBoxAddFortran->isChecked())
    {
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
    text=text+"\t double " +lineEditXXX->text().remove(" ")+"=key;\n";
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
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                              tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
        return;
    }
    QTextStream t( &f );
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << text;
    f.close();
    
    if ( radioButtonCPP->isChecked() )
    {
        tableCPP->setRowCount(0);
        QStringList lst = text.split("\n", QString::KeepEmptyParts);
        tableCPP->setRowCount(lst.count()+1);
        for (int ii=0; ii<lst.count();ii++) tableCPP->setItem(ii,0,new QTableWidgetItem(lst[ii]));
    }
}

//*******************************************
//+++ saveAsCPP1d : new v. >2019-06
//*******************************************
void compile18::saveAsCPP2d( QString fn )
{
    
    //+++
    QString fifExt=".";
    if (radioButton2D->isChecked()) fifExt+="2d";
    fifExt+="fif";
    int i;
    QString text;
    
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
    text+=fitPath->text().toAscii().constData();
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
    if (radioButton2D->isChecked())
    {
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
        
        //+++
        text+="\n/////////////////////////////////////////////////////////////////////////////////\n";
        text+="//+++ Function returns list of x-Names\n";
        text+="/////////////////////////////////////////////////////////////////////////////////\n";
        text=text+"extern " +'"'+"C"+'"'+" MY_EXPORT char *xName()\n{\n\treturn "+'"';
        text+=lineEditXXX->text();
        text=text+'"'+";\n}\n\n";
    }
    else
    {
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
    
    //+++
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
    
    for(i=0;i<(spinBoxP->value()-1);i++)
    {
        
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
    {
        if (tableParaNames->item(i,2)->checkState()==Qt::Checked) text=text+'1'+','; else text=text+'0'+',';
    }
    if (tableParaNames->item(i,2)->checkState()==Qt::Checked) text=text+'1'; else text=text+'0';
    
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
    
    //    htmlText.replace("\\"+'\\"', "\\"+"\\ \\"+'"');
    QString richText;
    int nChars=htmlText.length();
    for (i=0;i<nChars;i++)
    {
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
    
    if (checkBoxAddFortran->isChecked())
    {
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
    
    
    if (radioButton2D->isChecked() )
    {
        text=text+", void * ParaM)\n{\n//+++\ngsl_vector *Para\t=((struct functionND *) ParaM)->para;\n";
        text=text+"double *xxxx\t=((struct functionND *) ParaM)->Q;\n";
        
        
        QStringList lst = lineEditXXX->text().remove(" ").split(",", QString::SkipEmptyParts);
        
        text=text+"double "+lst[0]+"\t\t=1.0 + xxxx[0];\n";
        text=text+"double "+lst[1]+"\t\t=1.0 + xxxx[1];\n";
        
        for (int i=2; i<spinBoxXnumber->value(); i++)
            text=text+"double "+lst[i]+"\t\t= xxxx["+QString::number(i)+"];\n";
        
        for(i=0;i<spinBoxP->value();i++)
        {
            text=text+"double "+tableParaNames->item(i,0)->text().trimmed()+"\t\t=gsl_vector_get(Para,"+QString::number(i)+");\n";
        }
        
        text=text+"int prec\t=((struct functionND *) ParaM)->prec;\n//+++\n";
        
    }
    else
    {
        text=text+", void * ParaM)\n{\n//+++\ngsl_vector *Para\t=((struct functionT *) ParaM)->para;\n";
        for(i=0;i<spinBoxP->value();i++)
        {
            text=text+"double "+tableParaNames->item(i,0)->text().trimmed()+"\t\t=gsl_vector_get(Para, "+QString::number(i)+");\n";
        }
        
        text=text+"//+++\ndouble\t" +lineEditXXX->text().remove(" ")+"=key;\n";
        text=text+"int prec\t=((struct functionT *) ParaM)->prec;\n//+++\n";
        
        
    }
    
    
    text=text+"double\t" +lineEditY->text().trimmed()+";\n\n//+++++++++++++++++++++++++++++++++++++++++++++\n";
    
    //+++
    text=text+"gsl_set_error_handler_off();";
    text=text+"\n//+++++++++++++++++++++++++++++++++++++++++++++\n\n";
    
    
    lstTmp=text.split("\n");
    
    lnTextEditCode->firstLineIncrement=lstTmp.count()-1; lnTextEditCode->updateLineNumbers(true);
    lstTmp.clear();
    
    
    text+=textEditCode->toPlainText();
    text=text+"\n//+++++++++++++++++++++++++++++++++++++++++++++\n\n";
    if (radioButton2D->isChecked() )
    {
        text=text+"stop:\n";
        
        for(i=0;i<spinBoxP->value();i++)
        {
            text=text+"gsl_vector_set(Para," +QString::number(i)+","+tableParaNames->item(i,0)->text().trimmed()+");\n";
        }
    }
    else
    {
        for(i=0;i<spinBoxP->value();i++)
        {
            text=text+"gsl_vector_set(Para," +QString::number(i)+","+tableParaNames->item(i,0)->text().trimmed()+");\n";
        }
    }
    text=text+"\n"+"return "+lineEditY->text().trimmed()+";\n}\n";
    
    
    
    QFile f(fn);
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                              tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
        return;
    }
    QTextStream t( &f );
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << text;
    f.close();
    
    if ( radioButtonCPP->isChecked() )
    {
        tableCPP->setRowCount(0);
        QStringList lst = text.split("\n", QString::KeepEmptyParts);
        tableCPP->setRowCount(lst.count()+1);
        for (int ii=0; ii<lst.count();ii++) tableCPP->setItem(ii,0,new QTableWidgetItem(lst[ii]));
    }
}


//*******************************************
//+++ openOrigin
//*******************************************
void compile18::openOrigin(QString fdfName)
{
    //+++
    radioButton1D->setChecked(true);
    int i;
    
    
    //+++
    if (fdfName.isEmpty())
    {
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> File not selected"));
        return;
    }
    
    //+++
    if (!QFile::exists (fdfName))
    {
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> File does not exist"));
        return;
    }
    
    spinBoxP->setValue(0);
    
    //+++
    QFile f(fdfName);
    
    //+++
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        QMessageBox::critical(0, tr("QtiSAS"),
                              tr(QString("Could not read file: <br><h4>" + fdfName +
                                 "</h4><p>Please verify that you have the right to read from this location!").toLocal8Bit().constData()));
        return;
    }
    
    QTextStream t( &f );
    
    QStringList lst;
    while ( !t.atEnd() ) lst << t.readLine();
    
    
    parseOrigin(lst);
    
    return;
    
    QString s;
    bool userYN=true;
    
    //+++ Category
    lineEditGroupName->setText("Origin::Built-In");
    
    
    //+++[General Information]
    s = t.readLine();
    
    if (s.contains("[General Information]") || s.contains("[GENERAL INFORMATION]")  )
    {
        lineEditGroupName->setText("Origin::User-Defined");
        userYN=true;
    }
    else
    {
        QMessageBox::warning(this,tr("QtiSAS"), tr("Error: [General Information]"));
        return;
    }
    
    
    
    int iterNumber=0;
    QString ss;
    int pNumber=0;
    do
    {
        s = t.readLine();
        ss=s;
        
        if (s.contains("Function Name="))
        {
            //+++Function Name
            ss.remove("Function Name=");
            if (ss.contains(";"))  ss=ss.left(ss.indexOf(';')-1);
            lineEditFunctionName->setText(ss.trimmed());
            textLabelInfoSAS->setText(ss.trimmed());
        }
        else   if (s.contains("Brief Description="))
        {
            //+++Brief Description
            ss.remove("Brief Description=");
            
            textEditDescription->setText(ss+"\n");
        }
        else   if (s.contains("Function Type="))
        {
            //+++Function Type
            if (ss.indexOf(";")) ss=ss.left(ss.indexOf(";"));
            textEditDescription->append(ss+"\n");
        }
        else   if (s.contains("Function Form="))
        {
            //+++ Function Form
            if (ss.indexOf(";")) ss=ss.left(ss.indexOf(";"));
            textEditDescription->append(ss+"\n");
        }
        else   if (s.contains("Function Source="))
        {
            //+++ Function Source
            textEditDescription->append(ss+"\n");
        }
        else   if (s.contains("Number Of Parameters="))
        {
            //+++ Number Of Parameters
            ss.remove("Number Of Parameters=");
            spinBoxP->setValue(ss.toInt());
            
            if (spinBoxP->value()==0)
            {
                QMessageBox::warning(this,tr("QtiSAS"), tr("Error: Number Of Parameters: 0"));
                return;
            }
            pNumber=spinBoxP->value();
        }
        else   if (s.contains("Number Of Independent Variables="))
        {
            //+++Number Of Independent Variables
            ss.trimmed();
            ss.remove("Number Of Independent Variables=");
            
            if (ss.toInt()!=1)
            {
                QMessageBox::warning(this,tr("QtiSAS"), tr("Error: Number Of Independent Variables more than 1 not supported "));
                return;
            }
        }
        else   if (s.contains("Number Of Dependent Variables="))
        {
            //+++ Number Of Dependent Variables
            ss.trimmed();
            ss.remove("Number Of Dependent Variables=");
            
            if (ss.toInt()!=1)
            {
                QMessageBox::warning(this,tr("QtiSAS"), tr("Error: Number Of Independent Variables more than 1 not supported "));
                return;
            }
        }
        else   if (s.contains("Analytical Derivatives for User-Defined="))
        {
            //+++ Analytical Derivatives for User-Defined
            textEditDescription->append(ss+"\n");
        }
        
        //+++
        iterNumber++;
    }
    while (!s.contains("[Fitting Parameters]") && !s.contains("[FITTING PARAMETERS]") && iterNumber<33);
    
    
    //+++[Fitting Parameters]
    iterNumber=0;
    QStringList paraNames, paraDescription, initPara;
    
    do
    {
        s = t.readLine();
        ss=s;
        
        if (s.contains("Naming Method="))
        {
            //+++Naming Method
            textEditDescription->append(ss+"\n");
        }
        else   if (s.contains("Names="))
        {
            ss=ss.remove("Names=");
            paraNames = ss.split(",", QString::SkipEmptyParts);
        }
        else   if (s.contains("Meanings="))
        {
            ss=ss.remove("Meanings=");
            paraDescription = ss.split(",", QString::SkipEmptyParts);
        }
        else   if (s.contains("Initial Values="))
        {
            ss=ss.remove("Initial Values=");
            ss=ss.remove("(V)");
            ss=ss.remove("(F)");
            initPara = ss.split(",", QString::SkipEmptyParts);
        }
        
        
        iterNumber++;
    }
    while (!s.contains("[") && iterNumber<33);
    
    for(i=0; i<pNumber; i++)
    {
        //+++
        tableParaNames->item(i,0)->setText(paraNames[i].trimmed());
        //+++
        tableParaNames->item(i,3)->setText(paraDescription[i].trimmed());
        //+++
        tableParaNames->item(i,1)->setText(initPara[i].trimmed());
    }
    
    
    
    //+++ [FORMULA] user-defined-format
    if (s.contains("[FORMULA]"))
    {
        //+++ [FORMULA] user-defined-format
        s = t.readLine();
        
        while  (!s.contains("[CONSTRAINTS]") )
        {
            ss=s;
            ss=ss.trimmed();
            if (ss!="")
            {
                ss+=";\n";
                ss.replace(";;",";");
                ss.replace("; ;",";");
                ss.replace(";  ;",";");
                
                //+++ ln() to log()
                ss.replace("ln(", "log(");
                //+++ log() to log10()
                ss.replace("log(", "log10(");
                //+++ ^ to pow(,)
                if (ss.contains("^"))
                {
                    ss.replace("^", " pow(,) ");
                    
                    ss+="// edit this line to adjust pow(,) function:  a^b=pow(a,b)";
                }
                
                
                
                textEditCode->append(ss);
                // textEditDescription->append(ss+"\n");
            }
            
            s = t.readLine();
        }
    }
    
    
    while (!s.contains("[INDEPENDENT VARIABLES]") && !s.contains("[Independent Variables]") ) s = t.readLine();
    if (s.contains("[Independent Variables]")) s = t.readLine();
    s = t.readLine();
    ss=s;
    ss=ss.trimmed();
    ss=ss.remove("=");
    //+++ x
    lineEditXXX->setText(ss);
    
    
    while (!s.contains("[DEPENDENT VARIABLES]") && !s.contains("[Dependent Variables]") ) s = t.readLine();
    if (s.contains("[Dependent Variables]")) {s = t.readLine(); userYN=false;};
    s = t.readLine();
    ss=s;
    ss=ss.trimmed();
    ss=ss.remove("=");
    //+++ x
    lineEditY->setText(ss);
    
    
    if (!userYN)
    {
        while (!s.contains("[Formula]") ) s = t.readLine();
        //+++ [Formula] standart
        iterNumber=0;
        s = t.readLine();
        
        while  (!s.contains("[") && iterNumber<33)
        {
            ss=s;
            ss=ss.trimmed();
            if (ss!="")
            {
                ss+=";";
                ss.replace(";;",";");
                ss.replace("; ;",";");
                ss.replace(";  ;",";");
                
                //+++ ln() to log()
                ss.replace("ln(", "log(");
                //+++ log() to log10()
                ss.replace("log(", "log10(");
                //+++ ^ to pow(,)
                if (ss.contains("^"))
                {
                    ss.replace("^", " pow(,) ");
                    
                    ss+="// edit this line to adjust pow(,) function:  a^b=pow(a,b)";
                }
                
                textEditCode->append(ss);
                
                ss.remove(";");
                if (ss.indexOf("//")) ss=ss.left(ss.indexOf("//"));
                ss.replace( " pow(,) ","^");
                textEditDescription->insertPlainText(ss+"\n");
            }
            iterNumber++;
            s = t.readLine();
        }
    }
}

//*******************************************
//+++ parseOrigin
//*******************************************
void compile18::parseOrigin(QStringList lst)
{
    QStringList lstBlockName, lstBlock;
    
    QString currentBody;
    int numberBlocks=0;
    for (int i=0; i<lst.count();i++)
    {
        //toResLog(lst[i]);
        if (lst[i].left(1)=="[" && lst[i].contains("]"))
        {
            lstBlockName<<lst[i].left(lst[i].indexOf("]")+1);
            numberBlocks++;
            if (numberBlocks>1) lstBlock<<currentBody;
            currentBody="";
        }
        if (i==(lst.count()-1))
        {
            if (numberBlocks>1) lstBlock<<currentBody;
            currentBody="";
        }
        currentBody+=lst[i]+"\n";
    }
    
    int blockLocated;
    QString s;
    QStringList lstBody;
    // [General Information]
    blockLocated=-1;
    lstBody.clear();
    s="";
    for (int i=0; i<lstBlockName.count();i++)
    {
        if (lstBlockName[i].contains("[General Information]", Qt::CaseInsensitive)) blockLocated=i;
    }
    if (blockLocated<0) return;
    lstBody = lstBlock[blockLocated].split("\n", QString::SkipEmptyParts);
    
    QString fName, fDescription, fSource, fType, fForm, fNumberOfParameters, fNumberIndVars, fNumberDepVars;
    for (int j=0; j<lstBody.count();j++)
    {
        if (lstBody[j].contains("Function Name=", Qt::CaseInsensitive))
        {
            s=lstBody[j];
            if (s.contains(";")) s=s.left(s.indexOf(";"));
            s=s.right(s.length()-s.indexOf("=")-1);
            fName=s;
        }
        if (lstBody[j].contains("Brief Description=", Qt::CaseInsensitive))
        {
            s=lstBody[j];
            if (s.contains(";")) s=s.left(s.indexOf(";"));
            s=s.right(s.length()-s.indexOf("=")-1);
            fDescription=s;
        }
        if (lstBody[j].contains("Function Source=", Qt::CaseInsensitive))
        {
            s=lstBody[j];
            if (s.contains(";")) s=s.left(s.indexOf(";"));
            s=s.right(s.length()-s.indexOf("=")-1);
            fSource=s;
        }
        if (lstBody[j].contains("Function Type=", Qt::CaseInsensitive))
        {
            s=lstBody[j];
            if (s.contains(";")) s=s.left(s.indexOf(";"));
            s=s.right(s.length()-s.indexOf("=")-1);
            fType=s;
        }
        if (lstBody[j].contains("Function Form=", Qt::CaseInsensitive))
        {
            s=lstBody[j];
            if (s.contains(";")) s=s.left(s.indexOf(";"));
            s=s.right(s.length()-s.indexOf("=")-1);
            fForm=s;
        }
        if (lstBody[j].contains("Number Of Parameters=", Qt::CaseInsensitive))
        {
            s=lstBody[j];
            if (s.contains(";")) s=s.left(s.indexOf(";"));
            s=s.right(s.length()-s.indexOf("=")-1);
            fNumberOfParameters=s;
        }
        if (lstBody[j].contains("Number Of Independent Variables=", Qt::CaseInsensitive))
        {
            s=lstBody[j];
            if (s.contains(";")) s=s.left(s.indexOf(";"));
            s=s.right(s.length()-s.indexOf("=")-1);
            fNumberIndVars=s;
        }
        if (lstBody[j].contains("Number Of Dependent Variables=", Qt::CaseInsensitive))
        {
            s=lstBody[j];
            if (s.contains(";")) s=s.left(s.indexOf(";"));
            s=s.right(s.length()-s.indexOf("=")-1);
            fNumberDepVars=s;
        }
    }
    
    
//    toResLog(fName +"\n"+fDescription +"\n"+fSource +"\n"+fType +"\n"+fForm +"\n"+fNumberOfParameters +"\n"+fNumberIndVars +"\n"+fNumberDepVars);
    
    
    /*
     
     
     
     [FITTING PARAMETERS]
     Naming Method=User-Defined
     Names=y0, a, Wmax
     Meanings=y initial value, amplitude, Maximun growth rate
     Lower Bounds=0.0(X,ON),0.0(X,ON),0.0(X,ON)
     Upper Bounds=--,--,--
     Number Of Significant Digits=
     
     
     [FORMULA]
     y = a/(1 + ((a-y0)/y0)*exp(-4*Wmax*x/a))
     
     
     [CONSTRAINTS]
     a > y0
     
     
     [Parameters Initialization]
     sort( x_y_curve );
     //smooth( x_y_curve, 2 );
     y0 = min( y_data );
     a = max( y_data );
     Wmax = a / fwhm( x_y_curve );
     
     
     [CONSTANTS]
     
     
     [INITIALIZATIONS]
     
     
     
     [AFTER FITTING]
     
     
     
     [INDEPENDENT VARIABLES]
     x=
     
     
     [DEPENDENT VARIABLES]
     y=
     
     
     [CONTROLS]
     General Linear Constraints=Off
     Initialization Scripts=Off
     Scripts After Fitting=Off
     Number Of Duplicates=N/A
     Duplicate Offset=N/A
     Duplicate Unit=N/A
     
     */
}


//*******************************************
//+++ newFIF()
//*******************************************
void compile18::newFIF()
{
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
    //    textEditCode->setText("// --->  f = f ( "+lineEditXXX->text().remove(" ")+", {P1, ...} );");
    
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
//+++  "compile"
//*******************************************
void compile18::compileSingleFunction()
{
    boolCompileAll=false;
    makeDLL();
}

//*******************************************
//+++  make dll file 
//*******************************************
void compile18::makeDLL()
{
    //+++
    QString ext="";
    if (radioButton2D->isChecked()) ext="2d";
    
    if (pushButtonSave->isEnabled())
    {
        makeFIF();
    }
    else
    {
        //toResLog("Function from Standard Library");
        openFIFfileSimple();
    }
    makeCPP();
    makeBATnew();
    
    
    QDir d(pathFIF);
    
    QString file=pathFIF+"/BAT.BAT";
    d.remove(lineEditFunctionName->text().trimmed()+".o");
    
#ifdef Q_OS_WIN
    d.remove(lineEditFunctionName->text().trimmed()+".dll"+ext);
#elif defined(Q_OS_MAC)
    d.remove(lineEditFunctionName->text().trimmed()+".dylib"+ext);
#else
    d.remove(lineEditFunctionName->text().trimmed()+".so"+ext);
#endif
    
#ifndef Q_OS_WIN
    //+++ +++ chmod 777 file +++ +++++++++++++++++++++++++++
    QProcess *proc = new QProcess( qApp);
    QStringList cmd;
    cmd.append("chmod" );
    cmd.append("777");
    cmd.append(file);
    
    const QString command = cmd.join(" ");
    proc->start(command);
#endif
    
    procc = new QProcess(qApp);
    
    
    if (!boolCompileAll) toResLog("\n<< compile >>\n");
    
    connect( procc, SIGNAL(readyReadStandardError()), this, SLOT(readFromStdout()) );
    
    procc->start(file);
    
    
    procc->waitForFinished();
    //QString strOut = procc->readAllStandardOutput().data();
    //strOut += procc->readAllStandardError().data();
    //toResLog(strOut);
    
    QString soName=fitPath->text()+"/"+lineEditFunctionName->text()+".";
    
#ifdef Q_OS_MAC // Mac
    soName+="dylib";
#elif defined(Q_OS_WIN)
    soName+="dll";
#else
    soName+="so";
#endif
    
    if (radioButton2D->isChecked()) soName+="2d";
    
    if (QFile::exists (soName))
    {
        toResLog("<< compile status >> OK: function '"+ lineEditFunctionName->text()+"' is ready\n");
        app()->d_status_info->setText("<< compile status >> OK: function '"+ lineEditFunctionName->text()+"' is ready");
    }
    else
    {
        toResLog("<< compile status >>  ERROR: check function code / compiler options\n");
        app()->d_status_info->setText("<< compile status >>  ERROR: check function code / compiler options");
    }
    if (d.exists(lineEditFunctionName->text().trimmed()+".o"+ext))
    {
        d.remove(lineEditFunctionName->text().trimmed()+".o"+ext);
    }
}


//*******************************************
//+++  make dll file
//*******************************************
void compile18::compileTest()
{
    //+++
    QString ext="";
    if (radioButton2D->isChecked()) ext="2d";
    
    QDir d(pathFIF);
    
    QString file=pathFIF+"/BAT.BAT";
    d.remove(lineEditFunctionName->text().trimmed()+".o");
    
#ifdef Q_OS_WIN
    d.remove(lineEditFunctionName->text().trimmed()+".dll"+ext);
#elif defined(Q_OS_MAC)
    d.remove(lineEditFunctionName->text().trimmed()+".dylib"+ext);
#else
    d.remove(lineEditFunctionName->text().trimmed()+".so"+ext);
#endif
    
#ifndef Q_OS_WIN
    //+++ +++ chmod 777 file +++ +++++++++++++++++++++++++++
    QProcess *proc = new QProcess( qApp);
    QStringList cmd;
    cmd.append("chmod" );
    cmd.append("777");
    cmd.append(file);
    
    const QString command = cmd.join(" ");
    proc->start(command);
#endif
    
    procc = new QProcess(qApp);
    
    
    if (!boolCompileAll) toResLog("\n<< compile >>\n");
    
    connect( procc, SIGNAL(readyReadStandardError()), this, SLOT(readFromStdout()) );
    
    procc->start(file);
    
    procc->waitForFinished();
    QString strOut = procc->readAllStandardOutput();
    toResLog(strOut);
    
    QString soName=fitPath->text()+"/"+lineEditFunctionName->text()+".";
    
#ifdef Q_OS_MAC // Mac
    soName+="dylib";
#elif defined(Q_OS_WIN)
    soName+="dll";
#else
    soName+="so";
#endif
    
    if (radioButton2D->isChecked()) soName+="2d";
    
    if (QFile::exists (soName))
        toResLog("<< compile status >> OK: function '"+ lineEditFunctionName->text()+"' is ready\n");
    else
        toResLog("<< compile status >>  ERROR: check function code / compiler options\n");
    
    
    if (d.exists(lineEditFunctionName->text().trimmed()+".o"+ext))
    {
        d.remove(lineEditFunctionName->text().trimmed()+".o"+ext);
    }
}



void compile18::checkFunctionName()
{
    QString name=lineEditFunctionName->text();
    
    name=name.replace("_","-");
    name=name.remove(" ");
    name=name.remove(".");
    name=name.remove(":").remove(";").remove("|").remove("!").remove("%").remove("@");
    name=name.remove("[").remove("]").remove("=").remove("?");
    name=name.remove("/").remove(",").remove("~").remove("*").remove("<").remove(">");
    
    lineEditFunctionName->setText(name);
    textLabelInfoSAS->setText(name);
}


void compile18::readFromStdout()
{
    QString s=procc->readAllStandardError();

    QStringList lst;
    lst = s.split("\n", QString::SkipEmptyParts);
    
    s="";
    for(int i=0;i<lst.count();i++) if (lst[i].contains("error:")) s+= lst[i]+"\n";
    for(int i=0;i<lst.count();i++) if (lst[i].contains("error:")) s+= lst[i]+"\n";
    
    s=s.replace("‘","`").replace("’","'");
    
    toResLog( s);
}

void compile18::compileAll()
{
    boolCompileAll=true;
    int numberFunctions=listBoxFunctionsNew->model()->rowCount();//listBoxFunctions->count();
    if (numberFunctions==0) return;
    //if (listBoxFunctions->currentItem()>=0) makeFIF();
    
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

        //listBoxFunctions->setCurrentItem(i);
        openFIFfileSimple();

        if (lineEditFunctionName->text()!=listBoxFunctionsNew->model()->index(i,0).data().toString()) continue;
        
        
        //	sleep(500);
        /*
         time_t start_time, cur_time;
         
         time(&start_time);
         do
         {
         time(&cur_time);
         }
         while((cur_time - start_time) < 10);
         */
        //openFIFfileSimple();
        
        makeDLL();
        
        //	sleep(1);
        
        //+++ Progress +++
        progress.setValue(i+1);
        progress.setLabelText("Function :: " + lineEditFunctionName->text() +" is ready");
        if ( progress.wasCanceled() ) break;
        
        
        
        
    }
    
   listBoxGroupNew->selectionModel()->select(indexes[0], QItemSelectionModel::Select);
}
