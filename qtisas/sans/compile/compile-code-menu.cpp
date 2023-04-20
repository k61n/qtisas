/***************************************************************************
File                   : compile-code-menu.cpp
Project                : QtiSAS
--------------------------------------------------------------------
Copyright              : (C) 2017-2021 by Vitaliy Pipich
Email (use @ for *)    : v.pipich*gmail.com
Description            : compile interface: code-menu functions

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

void compile18::includeData()
{
    QString s="\n";
    s+="//++++++++++++++++++++++++++++++++++++++++++\n";
    s+="//+++  Current Data Sets\n";
    s+="//+++     XXX  :  fitted data X-vector\n";
    s+="//+++     YYY   :  fitted data Y-vector\n";
    s+="//+++     RESO:  fitted data Reso-vector\n";
    s+="//++++++++++++++++++++++++++++++++++++++++++\n\n";
    s+="double *XXX  =((struct functionT *) ParaM)->Q;\n";
    s+="double *YYY   =((struct functionT *) ParaM)->I;\n";
    s+="double *RESO=((struct functionT *) ParaM)->SIGMA;\n\n";
    s+="//++++++++++++++++++++++++++++++++++++++++++\n";
    s+="//+++     Current Data Set Limits and Current Data Point : \n";
    s+="//+++     currentFirstPoint <=currentPoint <= currentLastPoint\n";
    s+="//++++++++++++++++++++++++++++++++++++++++++\n\n";
    s+="int currentPoint=((struct functionT *) ParaM)->currentPoint;\n";
    s+="int currentFirstPoint=((struct functionT *) ParaM)->currentFirstPoint;\n";
    s+="int currentLastPoint=((struct functionT *) ParaM)->currentLastPoint;\n";
    
    textEditCode->insertPlainText(s);
}


void compile18::includeMath(int id)
{
    QString s=textEditCode->textCursor().selectedText();
    if (s=="") s="x";
    
    switch (id)
    {
        case 100:  textEditCode->insertPlainText("fabs("+s+")"); break;
        case 101:  textEditCode->insertPlainText( "log("+s+")/log(2)"); break;// log2
        case 102:  textEditCode->insertPlainText("log10("+s+")"); break;
        case 103:  textEditCode->insertPlainText("ln("+s+")"); break;
            
            
        case 201:  textEditCode->insertPlainText("sin("+s+")"); break;
        case 202:  textEditCode->insertPlainText("cos("+s+")"); break;
        case 203:  textEditCode->insertPlainText("tan("+s+")"); break;
        case 204:  textEditCode->insertPlainText("asin("+s+")"); break;
        case 205:  textEditCode->insertPlainText("acos("+s+")"); break;
        case 206:  textEditCode->insertPlainText("atan("+s+")"); break;
        case 207:  textEditCode->insertPlainText("sinh("+s+")"); break;
        case 208:  textEditCode->insertPlainText("cosh("+s+")"); break;
        case 209:  textEditCode->insertPlainText("tanh("+s+")"); break;
        case 210:  textEditCode->insertPlainText( "log("+s+" + sqrt("+s+" * "+s+" + 1))"); break;// asinh
        case 211:  textEditCode->insertPlainText( "log("+s+" + sqrt("+s+" * "+s+" - 1))"); break;// acosh
        case 212:  textEditCode->insertPlainText( "(0.5 * log((1 + "+s+") / (1 - "+s+")))"); break;// atanh
            
    }
    /*
     // misc
     DefineFun(_T("exp"), Exp);
     DefineFun(_T("sqrt"), Sqrt);
     DefineFun(_T("sign"), Sign);
     DefineFun(_T("rint"), Rint);
     DefineFun(_T("abs"), Abs);
     DefineFun(_T("if"), Ite);
     // Functions with variable number of arguments
     DefineFun(_T("sum"), Sum);
     DefineFun(_T("avg"), Avg);
     DefineFun(_T("min"), Min);
     DefineFun(_T("max"), Max);
     //case 4:     pushButtonCenter->setOn(true);break;
     }
     */
}

void compile18::stdMenu()
{
    QMenu* menuSTD = new QMenu( app() );
    menuSTD->addSeparator();
    menuSTD->addSeparator();
    
    menuSTD->addAction(lineEditY->text());
    
    menuSTD->addSeparator();
    menuSTD->addSeparator();
    
    QStringList lst = lineEditXXX->text().remove(" ").split(",", QString::SkipEmptyParts);
    for (int i=0; i<lst.count();i++)
        menuSTD->addAction(lst[i]);
    
    menuSTD->addSeparator();
    menuSTD->addSeparator();


    menuSTD->addAction("all parameters");
    for (int i=0; i<spinBoxP->value();i++)
        menuSTD->addAction(tableParaNames->item(i,0)->text());
    
    if (spinBoxP->value()>0) menuSTD->addSeparator();
    menuSTD->addSeparator();
    menuSTD->addSeparator();
    
    menuSTD->popup(pushButtonMenuFlags->mapToGlobal (QPoint(0,0)));
    
    connect(menuSTD, SIGNAL( activated ( int ) ), this, SLOT( includeID(int) ) );
}

void compile18::flagsMenu()
{
    QMenu* menuFlags = new QMenu( app() );
    ;
    menuFlags->addSeparator();
    menuFlags->addSeparator();
    menuFlags->addAction( "if (beforeFit) {...} else {...}");
    menuFlags->addAction( "if (afterFit) {...} else {...}");
    menuFlags->addAction( "if (beforeIter) {...} else {...}");
    menuFlags->addAction( "if (afterIter) {...} else {...}");
    menuFlags->addSeparator();
    menuFlags->addSeparator();
    menuFlags->addAction("[use] beforeFit");
    menuFlags->addAction("[use] afterFit");
    menuFlags->addAction("[use] beforeIter");
    menuFlags->addAction("[use] afterIter");
    menuFlags->addSeparator();
    menuFlags->addSeparator();

    menuFlags->popup(pushButtonMenuData->mapToGlobal (QPoint(0,0)));
    
    connect(menuFlags, SIGNAL( activated ( int ) ), this, SLOT( includeID(int) ) );
}

void compile18::dataMenu()
{
    QMenu* menuData = new QMenu( app() );
    
    if(radioButton1D->isChecked()) {
        menuData->addAction("[Call Current Point of the Fitted Dataset]:");
        menuData->addAction("- XXX[currentpoint]  :: X-vector");
        menuData->addAction("- YYY[currentpoint]  :: Y-vector");
        menuData->addAction("- dYYY[currentpoint] :: dY-vector");
        menuData->addAction("- RESO[currentpoint] :: Resolusion-vector");
        menuData->addSeparator();
        menuData->addAction("[Call]:");
        menuData->addAction("- number of current point");
        menuData->addAction("- number of current point (first)");
        menuData->addAction("- number of current point (last)");
        menuData->addSeparator();
    }
    else {
        menuData->addAction("get all data");
        menuData->addSeparator();
        menuData->addAction("YYYY        :  fitted data :: Y-matrix");
        menuData->addAction("MASK       :  masking matrix");
        menuData->addAction("WEIGHT  :  weighting matrix");
        menuData->addAction("XXX         :  fitted data :: X-matrix");
        menuData->addSeparator();
        menuData->addAction("vector of matrix dimensions : rows");
        menuData->addAction("vector of matrix dimensions : columnss");
        menuData->addSeparator();
        menuData->addAction("current matrix");
        menuData->addAction("number matrixes to fit");
        menuData->addAction("number x-matrixes");
        menuData->addSeparator();
        
    }
    
    menuData->popup(pushButtonMenuSANS->mapToGlobal (QPoint(0,0)));
    
    connect(menuData, SIGNAL( activated ( int ) ), this, SLOT( includeID(int) ) );
}

void compile18::mathMenu()
{
    QMenu* mathFunctions = new QMenu( app() );
    
    mathFunctions->addAction( tr("abs()"));
    mathFunctions->addSeparator();
    mathFunctions->addAction( tr("log2()"));
    mathFunctions->addAction( tr("log10()"));
    mathFunctions->addAction( tr("ln()"));
    mathFunctions->addSeparator();
    mathFunctions->addAction( tr("sin()"));
    mathFunctions->addAction( tr("cos()"));
    mathFunctions->addAction( tr("tan()"));
    mathFunctions->addAction( tr("asin()"));
    mathFunctions->addAction( tr("acos()"));
    mathFunctions->addAction( tr("atan()"));
    mathFunctions->addAction( tr("sinh()"));
    mathFunctions->addAction( tr("cosh()"));
    mathFunctions->addAction( tr("tanh()"));
    mathFunctions->addAction( tr("asinh()"));
    mathFunctions->addAction( tr("acosh()"));
    mathFunctions->addAction( tr("atanh()"));
    
    mathFunctions->addSeparator();
    
    mathFunctions->popup(pushButtonMenuMULTI->mapToGlobal (QPoint(0,0)));
    
    connect(mathFunctions, SIGNAL( activated ( int ) ), this, SLOT( includeID(int) ) );
}

void compile18::sansMenu()
{
    QMenu* menuSANS = new QMenu( app() );
    menuSANS->addAction("[Polydispersity]:");
    menuSANS->addSeparator();
    menuSANS->addSeparator();
    menuSANS->addAction("[get] all polydispersity parameters");
    menuSANS->addSeparator();
    menuSANS->addAction( "[get] (bool) polyYN");
    menuSANS->addAction( "[use] (bool) polyYN");
    menuSANS->addAction( "if (polyYN) {...} else {...}");
    menuSANS->addSeparator();
    menuSANS->addAction( " [get] (int) polyFunctionNumber");
    menuSANS->addAction( " [use] (int) polyFunctionNumber");
    
    menuSANS->popup(pushButtonMenuMath->mapToGlobal (QPoint(0,0)));
    
    connect(menuSANS, SIGNAL( activated ( int ) ), this, SLOT( includeID(int) ) );
}

void compile18::multiMenu()
{
    QMenu* menuSANS = new QMenu( app() );
    //    menuSANS->insertItem("[Function Multiplication Tools]:");
    //    menuSANS->addSeparator();
    menuSANS->addAction("[Superpositional Function]:");
    menuSANS->addAction("add switcher of functions");
    menuSANS->addSeparator();
    menuSANS->addAction("[Multiplication of Parameters]:");
    menuSANS->addAction("parameter order: a,b,...z -> a1,b1,..z1,a2,b2,...,z2,...,aN,bN,..zN");
    menuSANS->addAction("parameter order: a,b,...z -> a1,a2,..aN,b1,b2,...,bN,...,z1,z2,..zN");
    menuSANS->addSeparator();
    menuSANS->addAction("[Multiplication of Parameters and Function-Template]:");
    menuSANS->addAction("parameter order: a,b,...z -> a1,b1,..z1,a2,b2,...,z2,...,aN,bN,..zN");
    menuSANS->addAction("parameter order: a,b,...z -> a1,a2,..aN,b1,b2,...,bN,...,z1,z2,..zNe");
    
    menuSANS->popup(pushButtonMenuSASVIEW->mapToGlobal(QPoint(0,0) ));
    
    connect(menuSANS, SIGNAL( activated ( int ) ), this, SLOT( includeIDmulti(int) ) );
}


void compile18::includeIDmulti(int id)
{
    if (id==603)
    {
        bool ok;
        int number = QInputDialog::getInteger(this, "Supertositional function",
                                              "Enter number of sub-functions you want to see additionaly to default one:",
                                              2, 1, 100, 1, &ok);

        if (!ok) return;
        
        number++;
        checkBoxSuperpositionalFit->setChecked(true);
        spinBoxSubFitNumber->setValue(number);
        
        
        QString s="\n";
        QString yvar=lineEditY->text().remove(" ");
        s+="//++++++++++++++++++++++++++++++++++++++++++\n";
        s+="//+++ Superpositional Function Switcher\n";
        s+="//++++++++++++++++++++++++++++++++++++++++++\n";
        s+="//+++ Define below all your sub-functions (default one is first):\n";
        for (int iPara=0;iPara<number;iPara++) s+="double "+yvar+"_"+QString::number(iPara)+"= ...(code)...;\n";
        s+="//+++\n";
        s+="switch(currentFunction)\n";
        s+="{\n";
        s+="case 0: "+yvar+"="+yvar+"_"+QString::number(1);
        for (int iPara=2;iPara<number;iPara++) s+="+"+yvar+"_"+QString::number(iPara);
        s+="; break;\n";
        for (int iPara=1;iPara<number;iPara++) s+="case "+QString::number(iPara)+": "+yvar+"="+yvar+"_"+QString::number(iPara)+"; break;\n";
        s+="}\n";
        
        textEditCode->insertPlainText(s);
    }
    if (id==601 || id==602 || id==604 || id==605)
    {
        int numberPara=tableParaNames->rowCount();
        if (numberPara==0) return;
        // get numbet copies
        bool ok;
        int numberCopies = QInputDialog::getInteger(this,
                                                    "Input number of Multi-Functions ", "Enter a number:",
                                                    2, 2, 100, 1, &ok);
        if ( ok )
        {
            spinBoxP->setValue(numberPara*numberCopies );
            
            setNumberparameters(numberPara*numberCopies);
            
            if (id==602 || id==601)
            {
                QString s="\n";
                s+="//++++++++++++++++++++++++++++++++++++++++++\n";
                s+="//+++  Multiplication-Fuction-Template\n";
                s+="//++++++++++++++++++++++++++++++++++++++++++\n\n";
                s+="double ";
                
                for (int iPara=0;iPara<numberPara-1;iPara++) s+=tableParaNames->item(iPara,0)->text()+", ";
                s+=tableParaNames->item(numberPara-1,0)->text()+";\n\n ";
                
                s+=lineEditY->text()+"=0.0;\n\n";
                
                s+="for (int ii=0; ii<"+QString::number(numberCopies )+";ii++)\n{\n";
                
                for (int iPara=0;iPara<numberPara;iPara++) s+=tableParaNames->item(iPara,0)->text()
                    +"=gsl_vector_get(Para,ii*"+QString::number(numberPara)+"+"+QString::number(iPara)+");\n";
                
                s+="\n"+lineEditY->text()+"+=function("+lineEditXXX->text();
                for (int iPara=0;iPara<numberPara;iPara++) s+=", "+tableParaNames->item(iPara,0)->text();
                s+=");\n}\n";
                
                textEditCode->insertPlainText(s);
            }
            if (id==604 || id==605)
            {
                QString s="\n";
                s+="//++++++++++++++++++++++++++++++++++++++++++\n";
                s+="//+++  Multiplication-Fuction-Template\n";
                s+="//++++++++++++++++++++++++++++++++++++++++++\n\n";
                s+="double ";
                
                for (int iPara=0;iPara<numberPara-1;iPara++) s+=tableParaNames->item(iPara,0)->text()+", ";
                s+=tableParaNames->item(numberPara-1,0)->text()+";\n\n ";
                
                s+=lineEditY->text()+"=0.0;\n\n";
                
                s+="for (int ii=0; ii<"+QString::number(numberCopies )+";ii++)\n{\n";
                
                for (int iPara=0;iPara<numberPara;iPara++) s+=tableParaNames->item(iPara,0)->text()
                    +"=gsl_vector_get(Para,ii"+"+"+QString::number(iPara)+"*"+QString::number(numberPara)+");\n";
                
                s+="\n"+lineEditY->text()+"+=function("+lineEditXXX->text();
                for (int iPara=0;iPara<numberPara;iPara++) s+=", "+tableParaNames->item(iPara,0)->text();
                s+=");\n}\n";
                
                textEditCode->insertPlainText(s);
            }
 
            if (id==602)
            {
                for (int iPara=0;iPara<numberPara;iPara++)
                {
                    
                    QString curLabel=tableParaNames->item(iPara,0)->text();
                    QString curValue=tableParaNames->item(iPara,1)->text();
                    
                    Qt::CheckState curItem=tableParaNames->item(iPara,2)->checkState();
                    
                    QString curInfo=tableParaNames->item(iPara,3)->text();
                    
                    for (int iFun=0;iFun<numberCopies ;iFun++)
                    {
                        tableParaNames->setItem(iFun*numberPara+iPara,0,new QTableWidgetItem(curLabel+QString::number(iFun+1)));
                        tableParaNames->setItem(iFun*numberPara+iPara,1,new QTableWidgetItem(curValue));
                        
                        QTableWidgetItem *item = new QTableWidgetItem();
                        item->setCheckState(curItem);
                        tableParaNames->setItem(iFun*numberPara+iPara,2,item);
                        
                        tableParaNames->setItem(iFun*numberPara+iPara,3,new QTableWidgetItem(curInfo +" ["+QString::number(iFun+1)+"]"));
                    }
                }
            }
            
            if (id==605)
            {
                QStringList lstCurLabel, lstCurValue, lstCurInfo;
                QList<Qt::CheckState> lstCurItem;
                for (int iPara=0;iPara<numberPara;iPara++)
                {
                    lstCurLabel<<tableParaNames->item(iPara,0)->text();
                    lstCurValue<<tableParaNames->item(iPara,1)->text();
                    
                    lstCurItem<<tableParaNames->item(iPara,2)->checkState();
                    
                    lstCurInfo<<tableParaNames->item(iPara,3)->text();
                }
                
                for (int iPara=0;iPara<numberPara;iPara++)
                {
                    
                    QString curLabel=lstCurLabel[iPara];
                    QString curValue=lstCurValue[iPara];
                    
                    Qt::CheckState curItem=lstCurItem[iPara];
                    
                    QString curInfo=lstCurInfo[iPara];
                    
                    for (int iFun=0;iFun<numberCopies ;iFun++)
                    {
                        tableParaNames->setItem(iFun+iPara*numberPara,0,new QTableWidgetItem(curLabel+QString::number(iFun+1)));
                        tableParaNames->setItem(iFun+iPara*numberPara,1,new QTableWidgetItem(curValue));
                        
                        QTableWidgetItem *item = new QTableWidgetItem();
                        item->setCheckState(curItem);
                        tableParaNames->setItem(iFun+iPara*numberPara,2,item);
                        
                        tableParaNames->setItem(iFun*numberPara+iPara,3,new QTableWidgetItem(curInfo +" ["+QString::number(iFun+1)+"]"));
                    }
                }
            }
            
            
        }
        
        
        
        

        
    }

    if (id==700 || id==7000 || id==70000)
    {
        int oldParaNumber=spinBoxP->value();
        
        spinBoxP->setValue(oldParaNumber+2);
        tableParaNames->item(oldParaNumber,0)->setText("scale");
        tableParaNames->item(oldParaNumber,1)->setText("1.0");
        tableParaNames->item(oldParaNumber,3)->setText("[1] scale factor | volume fraction | ...model dependent ...");
        
        tableParaNames->item(oldParaNumber+1,0)->setText("bgd");
        tableParaNames->item(oldParaNumber+1,1)->setText("0.001[0.0..20]");
        tableParaNames->item(oldParaNumber+1,3)->setText("[1/cm] background");
        
        textEditCode->insertPlainText(lineEditY->text().remove(" ")+"=bgd+scale*   ;");
        return;
        
    }
    
    if (id>700 && id <70000)
    {
        QDir d(pathFIF+"/IncludedFunctions");
        QStringList lst;
        
        QString file;
        if (id<7000)
        {
            lst = d.entryList(QStringList() << "sasview*.h");
            file=pathFIF+"/IncludedFunctions/"+lst[id-701];
        }
        else
        {
            lst = d.entryList(QStringList() << "qtikws*.h");
            lst += d.entryList(QStringList() << "qtisas*.h");
            file=pathFIF+"/IncludedFunctions/"+lst[id-7001];
        }
        //       textEditCode->insert(file);

        //+++
        if (!QFile::exists (file))
        {
            QMessageBox::warning(this,tr("QtiSAS"), tr("Error: <p> no FIF file, 2"));
            return;
        }
        
        //+++
        QFile f(file);
        
        //+++
        if ( !f.open( QIODevice::ReadOnly ) )
        {
            QMessageBox::critical(0, tr("QtiSAS"),
                                  tr(QString("Could not write to file: <br><h4>" + file +
                                     "</h4><p>Please verify that you have the right to read from this location!").toLocal8Bit().constData()));
            return;
        }
        
        QTextStream t( &f );
        QString s;
        
        QString functionCallName, callFunction;
        QStringList paraNames, paraInitValues, paraRanges, paraUnits, paraInfos, info;
        
        for(int line=0; line<16; line++)
        {
            s = t.readLine();
            if(s.contains("//[add_finish]")) break;
            if(!s.contains("//[")) continue;
            
            if (s.contains("//[name]"))
            {
                functionCallName=s.remove("//[name] ").remove(" "); continue;
            }
            if (s.contains("//[parameter_names] "))
            {
                s=s.remove("//[parameter_names] ").remove(" ");
                paraNames = s.split(",", QString::SkipEmptyParts);
                callFunction=functionCallName+"("+lineEditXXX->text().remove(" ")+","+s+")";
                continue;
            }
            if (s.contains("//[parameter_init_values] "))
            {
                s=s.remove("//[parameter_init_values] ").remove(" ");
                paraInitValues = s.split(",", QString::SkipEmptyParts);
                continue;
            }
            if (s.contains("//[parameter_init_range] "))
            {
                s=s.remove("//[parameter_init_range] ").remove(" ");
                paraRanges = s.split(",", QString::KeepEmptyParts);
                continue;
            }
            if (s.contains("//[parameter_units] "))
            {
                s=s.remove("//[parameter_units] ").remove(" ");
                paraUnits = s.split(",", QString::SkipEmptyParts);
                continue;
            }
            if (s.contains("//[parameter_info] "))
            {
                s=s.remove("//[parameter_info] ").remove("[").remove("]");
                paraInfos = s.split(",", QString::SkipEmptyParts);
                continue;
            }
            if (s.contains("//[info]"))
            {
                s=s.remove("//[info] ");
                
                info = s.split(",, ", QString::SkipEmptyParts);
                continue;
            }
        }

        int numPara=paraNames.count();
        if (numPara==0) return;
        
        int oldParaNumber=spinBoxP->value();
        
        spinBoxP->setValue(oldParaNumber+numPara);
        
        if (paraNames.count()==numPara) for (int i=0;i<numPara;i++) tableParaNames->item(oldParaNumber+i,0)->setText(paraNames[i]);
        
        if (paraInitValues.count()==numPara) for (int i=0;i<numPara;i++) tableParaNames->item(oldParaNumber+i,1)->setText(paraInitValues[i]);
        
        if (paraRanges.count()==numPara )
        {
            for (int i=0;i<numPara;i++) if (paraRanges[i].contains("..")) tableParaNames->item(oldParaNumber+i,2)->setText(paraRanges[i].remove("[").remove("]"));
        else
        tableParaNames->item(oldParaNumber+i,2)->setText("..");
        }
        else for (int i=0;i<numPara;i++) tableParaNames->item(oldParaNumber+i,2)->setText("..");
        
        if (paraInfos.count()==numPara && paraUnits.count()==numPara) for (int i=0;i<numPara;i++) tableParaNames->item(oldParaNumber+i,3)->setText(paraUnits[i]+" "+paraInfos[i]);
        else if (paraInfos.count()==numPara) for (int i=0;i<numPara;i++) tableParaNames->item(oldParaNumber+i,3)->setText(paraInfos[i]);

        textEditCode->insertPlainText(callFunction);
        
        textEditDescription->insertPlainText("\n"+callFunction+":::\n");

        for (int i=0;i<info.count();i++) textEditDescription->insertPlainText(info[i].remove("\"").simplified()+"\n");

        if (id<7000)
        {
            textEditHFiles->insertPlainText("#include \"IncludedFunctions/"+lst[id-701]+"\"");
        }
        else
        {
           textEditHFiles->insertPlainText("#include \"IncludedFunctions/"+lst[id-7001]+"\"");
        }
        
        textEditHFiles->insertPlainText("\n");
    }
    
    if (id>70000)
    {
        
        
        QDir d(pathFIF+"/IncludedFunctions");
        QStringList lst;
        
        QString file;
        lst = d.entryList(QStringList() << "fortran*.f");
        lst += d.entryList(QStringList() << "fortran*.f90");
        file=pathFIF+"/IncludedFunctions/"+lst[id-70001];
        
        //       textEditCode->insert(file);
        
        
        
        
        
        //+++
        if (!QFile::exists (file))
        {
            QMessageBox::warning(this,tr("QtKws"), tr("Error: <p> no FIF file, 2"));
            return;
        }
        
        //+++
        QFile f(file);
        
        //+++
        if ( !f.open( QIODevice::ReadOnly ) )
        {
            QMessageBox::critical(0, tr("QtKws"),
                                  tr(QString("Could not write to file: <br><h4>" + file +
                                     "</h4><p>Please verify that you have the right to read from this location!").toLocal8Bit().constData()));
            return;
        }
        
        QTextStream t( &f );
        QString s;
        
        QString functionCallName, callFunction;
        QStringList paraNames, paraInitValues, paraRanges, paraUnits, paraInfos, info;
        
        for(int line=0; line<16; line++)
        {
            s = t.readLine();
            if(s.contains("C[add_finish]") || s.contains("![add_finish]") ) break;
            if(!s.contains("C[") && !s.contains("![")) continue;
            
            if (s.contains("C[name]") ) {functionCallName=s.remove("C[name] ").remove(" "); continue;};
            if (s.contains("![name]") ) {functionCallName=s.remove("![name] ").remove(" "); continue;};
            
            if (s.contains("C[parameter_names] ") || s.contains("![parameter_names] "))
            {
                s=s.remove("C[parameter_names] ").remove("![parameter_names] ").remove(" ");
                paraNames = s.split(",", QString::SkipEmptyParts);
                callFunction=functionCallName+"_(&"+lineEditXXX->text().remove(" ");//+s+")";
                for(int sss=0; sss<paraNames.count(); sss++) callFunction+=", &"+paraNames[sss];
                callFunction+=")";
                continue;
            }
            if (s.contains("C[parameter_init_values] ") || s.contains("![parameter_init_values] "))
            {
                s=s.remove("C[parameter_init_values] ").remove("![parameter_init_values] ").remove(" ");
                paraInitValues = s.split(",", QString::SkipEmptyParts);
                continue;
            }
            if (s.contains("C[parameter_init_range] ") || s.contains("![parameter_init_range] "))
            {
                s=s.remove("C[parameter_init_range] ").remove("![parameter_init_range] ").remove(" ");
                paraRanges = s.split(",", QString::SkipEmptyParts);
                continue;
            }
            if (s.contains("C[parameter_units] ") || s.contains("![parameter_units] "))
            {
                s=s.remove("C[parameter_units] ").remove("![parameter_units] ").remove(" ");
                paraUnits = s.split(",", QString::SkipEmptyParts);
                continue;
            }
            if (s.contains("C[parameter_info] ") || s.contains("![parameter_info] "))
            {
                s=s.remove("C[parameter_info] ").remove("![parameter_info] ").simplified().replace("] ,","],").replace("],",",,,").remove("[").remove("]");
                paraInfos = s.split(",,,", QString::SkipEmptyParts);
                continue;
            }
            if (s.contains("C[info]") || s.contains("![info]"))
            {
                s=s.remove("C[info] ").remove("![info] ");
                
                info = s.split(",, ", QString::SkipEmptyParts);
                continue;
            }
            
            
            
        }
        
        int numPara=paraNames.count();
        if (numPara==0) return;
        
        int oldParaNumber=spinBoxP->value();
        
        spinBoxP->setValue(oldParaNumber+numPara);
        if (paraNames.count()==numPara) for (int i=0;i<numPara;i++) tableParaNames->item(oldParaNumber+i,0)->setText(paraNames[i]);
        
        if (paraInitValues.count()==numPara) for (int i=0;i<numPara;i++) tableParaNames->item(oldParaNumber+i,1)->setText(paraInitValues[i]);
        
        if (paraRanges.count()==numPara )
        {
            for (int i=0;i<numPara;i++) if (paraRanges[i].contains("..")) tableParaNames->item(oldParaNumber+i,2)->setText(paraRanges[i].remove("[").remove("]"));
            else
                tableParaNames->item(oldParaNumber+i,2)->setText("..");
        }
        else for (int i=0;i<numPara;i++) tableParaNames->item(oldParaNumber+i,2)->setText("..");
        

        if (paraInfos.count()==numPara && paraUnits.count()==numPara) for (int i=0;i<numPara;i++) tableParaNames->item(oldParaNumber+i,3)->setText(paraUnits[i]+" "+paraInfos[i]);
        else if (paraInfos.count()==numPara) for (int i=0;i<numPara;i++) tableParaNames->item(oldParaNumber+i,3)->setText(paraInfos[i]);
        
        ;
        
        textEditCode->insertPlainText(callFunction);
        
        textEditDescription->insertPlainText("\n"+callFunction+":::\n");
        for (int i=0;i<info.count();i++) textEditDescription->insertPlainText(info[i].remove("\"").simplified()+"\n");
        
        
        
        //+++
        QFileInfo fi(file);
        QString filename=fi.fileName();
        filename="./IncludedFunctions/"+fi.fileName();
        fortranFunction->setText(filename);
        extructFortranFunctions(file);
        checkBoxAddFortran->setChecked(true);
        
    }

}


void compile18::includeID(int id)
{
    // parameters
    if (id>2000 && id<3000)
    {
        textEditCode->insertPlainText(tableParaNames->item(id-2000-1,0)->text());
        return;
    }
    // x-variables
    if (id>1000 && id<2000)
    {
        QStringList lst = lineEditXXX->text().remove(" ").split(",", QString::SkipEmptyParts);
        textEditCode->insertPlainText(lst[id-1000-1]);
        return;
    }
    
    QString s;
    if (id>=200 && id<500) // || id>=600 && id<700)
    {
        s=textEditCode->textCursor().selectedText();
        if (s=="") s="x";
    }
    
    
    switch (id)
    {
        case 111:
            s=  "beforeFit";
            break;
        case 112:
            s="afterFit";
            break;
        case 113:
            s="beforeIter";
            break;
        case 114:
            s="afterIter";
            break;
        case 105:
            s="if (beforeFit)\n {\n\n}\nelse\n{\n\n}\n";
            break;
        case 106:
            s="if (afterFit)\n {\n\n}\nelse\n{\n\n}\n";
            break;
        case 107:
            s="if (beforeIter)\n {\n\n}\nelse\n{\n\n}\n";
            break;
        case 108:
            s="if (afterIter)\n {\n\n}\nelse\n{\n\n}\n";
            break;
        case 201:
            s="XXX[currentPoint]";
            break;
        case 202:
            s="YYY[currentPoint]";
            break;
        case 203:
            s="dYYY[currentPoint]";
            break;
        case 204:
            s="RESO[currentPoint]";
            break;
        case 205:
            s="currentPoint";
            break;
        case 206:
            s="currentFirstPoint";
            break;
        case 207:
            s="currentLastPoint";
            break;
        case 211:
            s="\n";
            s+="//++++++++++++++++++++++++++++++++++++++++++\n";
            s+="//+++  Current Matrix(s)\n";
            s+="//+++     YYYY        :  fitted Matrix(es)\n";
            s+="//+++     MASK       :  mask Matrix(es)\n";
            s+="//+++     WEIGHT   :  weight Matrix(es)\n";
            s+="//+++     XXXX        :  x-Matrix(es)  [if defined]\n";
            s+="//++++++++++++++++++++++++++++++++++++++++++\n";
            s+="gsl_matrix *YYYY          =((struct functionND *) ParaM)->I;\n";
            s+="gsl_matrix *MASK         =((struct functionND *) ParaM)->mask;\n";
            s+="gsl_matrix *WEIGHT    =((struct functionND *) ParaM)->dI;\n";
            s+="gsl_matrix *XXXX         =((struct functionND *) ParaM)->xMatrix;\n\n";
            s+="//++++++++++++++++++++++++++++++++++++++++++\n";
            s+="//+++     Matrix(es) dimensions \n";
            s+="//++++++++++++++++++++++++++++++++++++++++++\n";
            s+="size_t  *Rows=((struct functionND *) ParaM)->Rows;\n";
            s+="size_t  *Columns=((struct functionND *) ParaM)->Columns;\n\n";
            s+="//++++++++++++++++++++++++++++++++++++++++++\n";
            s+="//+++     Simultanious Fit \n";
            s+="//+++     - yNumber               - number of y-Matrixes to Fit  \n";
            s+="//+++     - xNumber               - number of x-Matrixes  \n";
            s+="//+++     - yNumberCurrent  - number of current y-Matrixes [1...yNumber]  \n";
            s+="//++++++++++++++++++++++++++++++++++++++++++\n";
            s+="size_t  yNumber=((struct functionND *) ParaM)->yNumber;\n";
            s+="size_t  xNumber=((struct functionND *) ParaM)->xNumber;\n";
            s+="size_t  yNumberCurrent =key;\n\n";
            break;
        case 212:
            s="gsl_matrix *YYYY    =((struct functionND *) ParaM)->I;\n";
            break;
        case 213:
            s="gsl_matrix *MASK         =((struct functionND *) ParaM)->mask;\n";
            break;
        case 214:
            s="gsl_matrix *WEIGHT    =((struct functionND *) ParaM)->dI;\n";
            break;
        case 215:
            s="gsl_matrix *XXXX         =((struct functionND *) ParaM)->xMatrix;\n";
            break;
        case 216:
            s="size_t  *Rows=((struct functionND *) ParaM)->Rows;\n";
            break;
        case 217:
            s="size_t  *Columns=((struct functionND *) ParaM)->Columns;\n";
            break;
        case 218:
            s="size_t  yNumberCurrent =key;\n";
            break;
        case 219:
            s="size_t  yNumber=((struct functionND *) ParaM)->yNumber;\n";
            break;
        case 220:
            s="size_t  xNumber=((struct functionND *) ParaM)->xNumber;\n";
            break;
            
        case 300:
            s="fabs("+s+")";
            break;
        case 301:
            s="log("+s+")/log(2)";
            break;// log2
        case 302:
            s="log10("+s+")";
            break;
        case 303:
            s="ln("+s+")";
            break;
            
        case 401:
            s="sin("+s+")";
            break;
        case 402:
            s="cos("+s+")";
            break;
        case 403:
            s="tan("+s+")";
            break;
        case 404:
            s="asin("+s+")";
            break;
        case 405:
            s="acos("+s+")";
            break;
        case 406:
            s="atan("+s+")";
            break;
        case 407:
            s="sinh("+s+")";
            break;
        case 408:
            s="cosh("+s+")";
            break;
        case 409:
            s="tanh("+s+")";
            break;
        case 410:
            s="log("+s+" + sqrt("+s+" * "+s+" + 1))";
            break;// asinh
        case 411:
            s="log("+s+" + sqrt("+s+" * "+s+" - 1))";
            break;// acosh
        case 412:
            s="(0.5 * log((1 + "+s+") / (1 - "+s+")))";
            break;// atanh
        case 501:
            s="\n";
            s+="//++++++++++++++++++++++++++++++++++++++++++\n";
            s+="//+++  Polydispersity parameters\n";
            s+="//++++++++++++++++++++++++++++++++++++++++++\n\n";
            s+="// Polydispersity check box is checked?:  (bool) polyYN\n";
            s+="bool polyYN=((struct functionT *) ParaM)->polyYN;\n\n";
            s+="// Number of polydispersity function:\n";
            s+="// (1) Gauss,(2) Schultz, (3) Gamma, (4) Log-Normal, (5) Uniform, (6) Triangular \n";
            s+="int polyFunctionNumber=((struct functionT *) ParaM)->polyFunction;\n\n";
            
            s+="// Parameters of polydispersity function:\n";
            s+="// (1)-(5) sigmaPoly1, (6) sigmaPoly1 and sigmaPoly2 \n";
            s+="double sigmaPoly1=0.0, sigmaPoly2=0.0;\n";
            s+="if (polyYN)\n{\n\tsigmaPoly1= gsl_vector_get(Para, Para->size-2);\n\tsigmaPoly2=gsl_vector_get(Para, Para->size-1);\n}\n\n";
            s+="//---------------------------------------------------------------------------------------------------------\n";
            s+="//---  Polydispersity parameters\n";
            s+="//----------------------------------------------------------------------------------------------------------\n";
            
            break;
        case 502:
            s="bool polyYN=((struct functionT *) ParaM)->polyYN;\n";
            break;
        case 503:
            s="if (polyYN)\n{\n\n}\nelse\n{\n\n}\n";
            break;
        case 504:
            s="int polyFunctionNumber=((struct functionT *) ParaM)->polyFunction;\n";
            break;
        case 505:
            s="polyFunctionNumber";
            break;
        case 506:
            s="polyYN";
            break;
        case 1000:
            s=lineEditY->text();
            break;
        case 2000:
            for (int i=0; i<spinBoxP->value();i++)
            {
                s+=tableParaNames->item(i,0)->text()+",";
            }
            s=s.left(s.length()-1);
            break;
            
    }
    textEditCode->insertPlainText(s);
}


void compile18::sasviewMenu()
{
    QMenu* menuSANS = new QMenu( app() );

    //    menuSANS->insertItem("[Function Multiplication Tools]:");
    //    menuSANS->addSeparator();
    menuSANS->addAction("...=bgd+scale*...");
    connect(menuSANS, SIGNAL( activated ( int ) ), this, SLOT( includeIDmulti(int) ) );
    
    QString s;
    QString fileName;
    
    QStringList group;
    
    QDir d(pathFIF+"/IncludedFunctions");
    QStringList lst = d.entryList(QStringList() << "sasview*.h");
    
    QStringList lst0, lstFolders;
    
    for (int i=0;i<lst.count();i++)
    {
        lst0 = lst[i].split("-", QString::SkipEmptyParts);
        
        if(lst0.count()>2 && !lstFolders.contains(lst0[1])) lstFolders<<lst0[1];
    }
    
    
    //for (int i=0;i<lst.count();i++) menuSANS->insertItem(lst[i], 701+i);
    int counter=0;
    for (int i=0;i<lstFolders.count();i++)
    {
        QMenu* submenu = new QMenu( app() );
        submenu->setTitle(lstFolders[i]);
        menuSANS->addMenu(submenu);
        
        
        
        for (int j=0;j<lst.count();j++)
        {
            lst0 = lst[j].split("-", QString::SkipEmptyParts);
            
            if(lst0.count()>2 && lst0[1]==lstFolders[i])
            {
                submenu->addAction(lst[j]);//.remove("sasview-"+lstFolders[i]+"-").remove(".h"), 701+j);
            }
        }
        connect(submenu, SIGNAL( activated ( int ) ), this, SLOT( includeIDmulti(int) ) );
        
    }
    menuSANS->popup( pushButtonMenuQTIKWS->mapToGlobal ( QPoint(0,0) ) );
    
    
    
    /*
     connect(menuSANS, SIGNAL( activated ( int ) ), this, SLOT( includeIDmulti(int) ) );
     */
    
    
    
}

void compile18::qtikwsMenu()
{
    QMenu* menuSANS = new QMenu( app() );
    //    menuSANS->insertItem("[Function Multiplication Tools]:");
    //    menuSANS->addSeparator();
    menuSANS->addAction("...=bgd+scale*...");
    connect(menuSANS, SIGNAL( activated ( int ) ), this, SLOT( includeIDmulti(int) ) );
    
    QString s;
    QString fileName;
    
    QStringList group;
    
    QDir d(pathFIF+"/IncludedFunctions");
    QStringList lst = d.entryList(QStringList() << "qtikws*.h");
    lst += d.entryList(QStringList() << "qtisas*.h");
    
    QStringList lst0, lstFolders;
    
    for (int i=0;i<lst.count();i++)
    {
        lst0 = lst[i].split("-", QString::SkipEmptyParts);
        
        if(lst0.count()>2 && !lstFolders.contains(lst0[1])) lstFolders<<lst0[1];
    }
    
    
    //for (int i=0;i<lst.count();i++) menuSANS->insertItem(lst[i], 701+i);
    int counter=0;
    for (int i=0;i<lstFolders.count();i++)
    {
        QMenu* submenu = new QMenu( app() );
        submenu->setTitle(lstFolders[i]);
        menuSANS->addMenu(submenu);
        
        
        
        for (int j=0;j<lst.count();j++)
        {
            lst0 = lst[j].split("-", QString::SkipEmptyParts);
            
            if(lst0.count()>2 && lst0[1]==lstFolders[i])
            {
                submenu->addAction(lst[j]);//.remove("qtikws-"+lstFolders[i]+"-").remove(".h"), 7001+j);
            }
        }
        connect(submenu, SIGNAL( activated ( int ) ), this, SLOT( includeIDmulti(int) ) );
        
    }
    menuSANS->popup( pushButtonMenuFORTRAN->mapToGlobal ( QPoint(0,0) ) );
    
    
    
    /*
     connect(menuSANS, SIGNAL( activated ( int ) ), this, SLOT( includeIDmulti(int) ) );
     */
    
    
}
void compile18::fortranMenu()
{
    QMenu* menuSANS = new QMenu( app() );
    //    menuSANS->insertItem("[Function Multiplication Tools]:");
    //    menuSANS->addSeparator();
    menuSANS->addAction("...=bgd+scale*...");
    connect(menuSANS, SIGNAL( activated ( int ) ), this, SLOT( includeIDmulti(int) ) );
    
    QString s;
    QString fileName;
    
    QStringList group;
    
    QDir d(pathFIF+"/IncludedFunctions");
    QStringList lst = d.entryList(QStringList() << "fortran*.f");
    lst += d.entryList(QStringList() << "fortran*.f90");
    
    QStringList lst0, lstFolders;
    
    for (int i=0;i<lst.count();i++)
    {
        lst0 = lst[i].split("-", QString::SkipEmptyParts);
        
        if(lst0.count()>2 && !lstFolders.contains(lst0[1])) lstFolders<<lst0[1];
    }
    
    
    //for (int i=0;i<lst.count();i++) menuSANS->insertItem(lst[i], 701+i);
    int counter=0;
    for (int i=0;i<lstFolders.count();i++)
    {
        QMenu* submenu = new QMenu( app() );
        submenu->setTitle(lstFolders[i]);
        menuSANS->addMenu(submenu);
        
        
        
        for (int j=0;j<lst.count();j++)
        {
            lst0 = lst[j].split("-", QString::SkipEmptyParts);
            
            if(lst0.count()>2 && lst0[1]==lstFolders[i])
            {
                submenu->addAction(lst[j].remove("qtikws-"+lstFolders[i]+"-").remove(".h"));
            }
        }
        connect(submenu, SIGNAL( activated ( int ) ), this, SLOT( includeIDmulti(int) ) );
        
    }
    menuSANS->popup( pushButtonMenuFORTRAN->mapToGlobal ( QPoint(pushButtonMenuFORTRAN->width(),0) ) );
    
    
    
    /*
     connect(menuSANS, SIGNAL( activated ( int ) ), this, SLOT( includeIDmulti(int) ) );
     */
    
    
}

void compile18::findInCode()
{
    if (!find(textEditCode,lineEditFind))
    {
        textEditCode->moveCursor(QTextCursor::Start);
        find(textEditCode,lineEditFind);
    }
}
void compile18::findInIncluded()
{
    if (!find(textEditFunctions,lineEditFindIncluded))
    {
        textEditFunctions->moveCursor(QTextCursor::Start);
        find(textEditFunctions,lineEditFindIncluded);
    }
    
}

bool compile18::find(QTextEdit *qte, QLineEdit *qle)
{
    QString searchString=qle->text();
    QTextCursor currentCursor=qte->textCursor();

    QList<QTextEdit::ExtraSelection> extraSelections;
    
    //qte->moveCursor(QTextCursor::Start);
    QColor color = QColor(Qt::gray).lighter(130);
    
    QTextCursor newCursor;
    bool newCurPosFound=false;
    while(qte->find(searchString))
    {
            QTextEdit::ExtraSelection extra;
            extra.format.setBackground(color);
            
            extra.cursor = qte->textCursor();
            extraSelections.append(extra);
            if (!newCurPosFound && extra.cursor.position()>=currentCursor.position())
            {
                newCurPosFound=true;
                currentCursor=extra.cursor;
            }
    }
    
    qte->setExtraSelections(extraSelections);
    
    
    qte->setTextCursor(currentCursor);

    return newCurPosFound;
    //qte->find(searchString,qte->textCursor());
    
    /*
    QTextCursor qte->d_highlight_cursor = textCursor();
    bool stop = previous ? qte->d_highlight_cursor.atStart() : qte->d_highlight_cursor.atEnd();
    bool found = false;
    while (!qte->d_highlight_cursor.isNull() && !stop){
        qte->d_highlight_cursor = document()->find(searchString, d_highlight_cursor, flags);
        if (!d_highlight_cursor.isNull()){
            found = true;
            setTextCursor(d_highlight_cursor);
            return true;
        }
        stop = previous ? d_highlight_cursor.atStart() : d_highlight_cursor.atEnd();
    }
    
    if (!found)
        QMessageBox::information(this, tr("QtiSAS"), tr("QtiSAS has finished searching the document."));
    return found;
     */
    return true;
}
