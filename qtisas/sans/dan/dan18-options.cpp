/***************************************************************************
 File : dan18-options.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : SANS Data Analisys Interface: select instrument functions
 
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
#include "dan18.h"

//+++ connect Slots
void dan18::optionsConnectSlot()
{
    //+++ Options
    connect( comboBoxSelectPresentation,        SIGNAL( activated(const  QString&) ), this,     SLOT( sasPresentation() ) );
    connect( comboBoxCalibrant,                 SIGNAL( activated(int) ), this,                 SLOT(calibrantselected() ) );
    connect( pushButtonDeleteCurrentCalibrator, SIGNAL( clicked() ), this,                      SLOT( deleteCurrentCalibrant() ) );
    connect( pushButtonsaveCurrentCalibrant,    SIGNAL( clicked() ), this,                      SLOT( saveCalibrantAs() ) );
    connect( lineEditMD,                        SIGNAL( textChanged(const QString&) ), this,    SLOT( MDchanged() ) );
    connect( spinBoxRegionOfInteres,            SIGNAL( valueChanged(int) ), this,              SLOT( dataRangeOfInteresChanged(int) ) );
    connect( comboBoxMDdata,                    SIGNAL( activated(const QString&) ), this,      SLOT( dataDimensionChanged(const QString&) ) );
    connect( comboBoxBinning,                   SIGNAL( activated(const QString&) ), this,      SLOT( binningChanged(const QString&) ) );
    connect( checkBoxYes2ndHeader,              SIGNAL( toggled(bool) ), this,                  SLOT( secondHeaderExist(bool) ) );
    connect( radioButtonLambdaHeader,           SIGNAL( toggled(bool) ), this,                  SLOT( readLambdaFromHeader(bool) ) );
}



QString dan18::fileNameUni( QString wildCardLocal, QString Number)
{
    QString index="";
    if (Number.contains("["))
    {
        index=Number.right(Number.length()-Number.find("[")).remove("[").remove("]");
        Number=Number.left(Number.find("["));
        if (wildCardLocal.contains("["))
            wildCardLocal=wildCardLocal.left(wildCardLocal.find("["))+index;
    }
    
    QString subDir="";
    
    if (Number.contains("/")==1)
    {
        subDir=Number.left(Number.find("/")+1);
        Number=Number.remove(subDir);
    }
    
    
    if (wildCardLocal.count("#")==1)	wildCardLocal=wildCardLocal.replace("#",Number);
    else if (wildCardLocal.count("#")==2)
    {
        QStringList lst;
        lst=lst.split("-",Number);
        if (lst.count()==2)
        {
            wildCardLocal=wildCardLocal.replace(wildCardLocal.find("#"),1,lst[0]);
            wildCardLocal=wildCardLocal.replace(wildCardLocal.find("#"),1,lst[1]);
        }
        else
        {
            wildCardLocal=wildCardLocal.replace(wildCardLocal.find("#"),1,lst[0]);
            wildCardLocal=wildCardLocal.replace("#","*");
        }
        
    }
    else if (wildCardLocal.count("#")>2) return "";
    else if (wildCardLocal.count("*")==1)
        wildCardLocal=wildCardLocal.replace("*",Number);
    else return "";
    
    //+++ real Wild Card
    //wildCardLocal=wildCardLocal.replace("#",Number);
    
    //+++ local file-name
    QDir d(Dir+subDir);
    QStringList lst = d.entryList(wildCardLocal);
    QString file=lst.last();
    
    //+++ file full-name
    return Dir+subDir+file;
}

QString dan18::newFileNameUni( QString wildCardLocal, QString Number)
{
    QString index="";
    if (Number.contains("["))
    {
        index=Number.right(Number.length()-Number.find("[")).remove("[").remove("]");
        Number=Number.left(Number.find("["));
        if (wildCardLocal.contains("["))
            wildCardLocal=wildCardLocal.left(wildCardLocal.find("["))+index;
    }
    
    
    if (wildCardLocal.count("#")==1)
        wildCardLocal=wildCardLocal.replace("#",Number);
    else if (wildCardLocal.count("#")==2)
    {
        QStringList lst;
        lst=lst.split("-",Number);
        if (lst.count()==2)
        {
            wildCardLocal=wildCardLocal.replace(wildCardLocal.find("#"),1,lst[0]);
            wildCardLocal=wildCardLocal.replace(wildCardLocal.find("#"),1,lst[1]);
        }
        else
        {
            wildCardLocal=wildCardLocal.replace(wildCardLocal.find("#"),1,lst[0]);
            wildCardLocal=wildCardLocal.replace("#","*");
        }
        
    }
    else if (wildCardLocal.count("#")>2) return "";
    else if (wildCardLocal.count("*")==1)
        wildCardLocal=wildCardLocal.replace("*",Number);
    else return "";
    
    //+++ real Wild Card
    //wildCardLocal=wildCardLocal.replace("#",Number);
    
    
    //+++ file full-name
    return lineEditPathRAD->text()+wildCardLocal;
}


//*******************************************
//+++  new-daDan:: find Table List By Label
//*******************************************
void dan18::deleteObjectByLabel(QString winLabel)
{
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if (w->windowLabel().contains(winLabel)) removeWindows(w->name());
}

//+++ Delete Windows Buttom +++
void dan18::removeWindows(QString pattern)
{
    QRegExp rx(pattern);
    rx.setWildcard( TRUE );
    QList<MdiSubWindow*> windows = app()->windowsList();
    //foreach(MdiSubWindow *w, windows) if (rx.exactMatch(w->name())) emit app()->closeWindow(w);
    foreach(MdiSubWindow *w, windows) if (rx.exactMatch(w->name())) {w->askOnCloseEvent(false);w->close();};
}

//+++
void dan18::sasPresentation( )
{
    QString presentation=comboBoxSelectPresentation->currentText();
    if (presentation=="QI") textLabelPres->setText("I vs. Q");
    if (presentation=="Guinier") textLabelPres->setText("ln[I] vs. Q<sup>2</sup>");
    if (presentation=="Zimm") textLabelPres->setText("I<sup>-1</sup> vs. Q<sup>2</sup>");
    if (presentation=="Porod") textLabelPres->setText("log[I] vs. log[Q]");
    if (presentation=="Porod2") textLabelPres->setText("IQ<sup>4</sup> vs. Q<sup>2</sup>");
    if (presentation=="logQ") textLabelPres->setText(" I vs. log[Q]");
    if (presentation=="logI") textLabelPres->setText(" log[I] vs. Q ");
    if (presentation=="Debye")
        textLabelPres->setText("I<sup>-1/2</sup> vs. Q<sup>2</sup>");
    if (presentation=="1Moment") textLabelPres->setText("IQ vs. Q");
    if (presentation=="2Moment")
        textLabelPres->setText("IQ<sup>2</sup> vs. Q");
    if (presentation=="Kratky")
        textLabelPres->setText("IQ<sup>2</sup> vs. Q");
    if (presentation=="GuinierRod") textLabelPres->setText("ln[IQ] vs. Q<sup>2</sup>");
    if (presentation=="GuinierPlate")
        textLabelPres->setText("ln[IQ<sup>2</sup>] vs. Q<sup>2</sup>");
}

void dan18::saveCalibrantAs()
{
    if (app()->sasPath=="") return;
    //+++
    QDir dd;
    QString calPath=app()->sasPath+"/calibrationStandards";
    calPath=calPath.replace("//","/");
    if (!dd.cd(calPath))
    {
        calPath=QDir::homeDirPath()+"/calibrationStandards";
        calPath=calPath.replace("//","/");
        if (!dd.cd(calPath))
        {
            dd.cd(QDir::homeDirPath());
            dd.mkdir("./qtiSAS/calibrationStandards");
            dd.cd("./qtiSAS/calibrationStandards");
        }
    };
    calPath=dd.absPath();
    
    bool ok=false;
    
    QString fileName=comboBoxCalibrant->currentText();
    
    if (comboBoxCalibrant->currentItem()<2) fileName="Create Your Calibrant: Input Calibrant Name";
    
    
    while (ok==false)
    {
        fileName = QInputDialog::getText(
                                         "QtiSAS", "Create Your Calibrant", QLineEdit::Normal,
                                         fileName, &ok, this );
        if ( !ok ||  fileName.isEmpty() )
        {
            return;
        }
        
        if (fileName=="Plexi-1.5mm-[Current]" || fileName=="Plexi-1.5mm[KWS-1&2]" || fileName=="DirectBeam-No-Attenuator" || fileName=="DirectBeam[KWS-3]" || fileName=="H2O[SANS-1]")
        {
            ok=false;
            fileName=="Please do not use STANDARD calibrant names";
        }
    }
    
    QString s="";
    
    s+="[muO] "+lineEditMuY0->text()+"\n";
    s+="[muA] "+lineEditMuA->text()+"\n";
    s+="[lambdaM] "+lineEditMut->text()+"\n";
    s+="[TO] "+lineEditTo->text()+"\n";
    s+="[TA] "+lineEditTA->text()+"\n";
    s+="[lambdaT] "+lineEditLate->text()+"\n";
    
    QFile f(calPath+"/"+fileName+".ACS");
    
    
    if ( !f.open( IO_WriteOnly ) )
    {
        //*************************************Log Window Output
        QMessageBox::warning(this,"Could not write to file:: "+fileName+".SANS", tr("QtiSAS::DAN"));
        //*************************************Log Window Output
        return;
    }
    
    QTextStream stream( &f );
    stream<<s;
    f.close();
    
    findCalibrators();
    
    comboBoxCalibrant->setCurrentText(fileName);
    
    calibrantselected();
    
    return;
    
}


void dan18::deleteCurrentCalibrant()
{
    if (app()->sasPath=="") return;
    
    if (comboBoxCalibrant->currentItem()<2)
    {
        return;
    }
    
    QString fileName=comboBoxCalibrant->currentText();
    
    //+++
    QDir dd;
    QString calPath=app()->sasPath+"/calibrationStandards";
    calPath=calPath.replace("//","/");
    
    if (!dd.cd(calPath))
    {
        calPath=QDir::homeDirPath()+"/calibrationStandards";
        calPath=calPath.replace("//","/");
        if (!dd.cd(calPath))
        {
            dd.cd(QDir::homeDirPath());
            dd.mkdir("./qtiSAS/calibrationStandards");
            dd.cd("./qtiSAS/calibrationStandards");
        }
    };
    calPath=dd.absPath();
    
    dd.remove(fileName+".ACS");
    
    findCalibrators();
    calibrantselected();
}

//+++ when calibrant is changed
void dan18::calibrantselected()
{
    QString calibrant=comboBoxCalibrant->currentText();
    
    if (comboBoxCalibrant->currentItem()<5) pushButtonDeleteCurrentCalibrator->setEnabled(false);
    else pushButtonDeleteCurrentCalibrator->setEnabled(true);
    
    //+++
    lineEditMuY0->setEnabled(false);
    lineEditMuA->setEnabled(false);
    lineEditMut->setEnabled(false);
    lineEditTo->setEnabled(false);
    lineEditTA->setEnabled(false);
    lineEditLate->setEnabled(false);
    
    if (calibrant=="Plexi-1.5mm[KWS-1&2]")
    {
        lineEditMuY0->setText("5.5998E-02");
        lineEditMuA->setText("1.211E-02");
        lineEditMut->setText("-6.7721E+00");
        lineEditTo->setText("4.993E-01");
        lineEditTA->setText("4.283E-01");
        lineEditLate->setText("8.246E+00");
    }
    else     if (calibrant=="Plexi-1.5mm-[current]")
    {
        lineEditMuY0->setText("5.5998E-02");
        lineEditMuA->setText("1.211E-02");
        lineEditMut->setText("-6.7721E+00");
        lineEditTo->setText("4.993E-01");
        lineEditTA->setText("4.283E-01");
        lineEditLate->setText("8.246E+00");
    }
    else if (calibrant=="DirectBeam[KWS-3]")
    {
        lineEditMuY0->setText("1.0");
        lineEditMuA->setText("0.0");
        lineEditMut->setText("1.0");
        lineEditTo->setText("1.0");
        lineEditTA->setText("0.0");
        lineEditLate->setText("1.0");
    }
    else if (calibrant=="DirectBeam-No-Attenuator")
    {
        lineEditMuY0->setText("1.0");
        lineEditMuA->setText("0.0");
        lineEditMut->setText("1.0");
        lineEditTo->setText("1.0");
        lineEditTA->setText("0.0");
        lineEditLate->setText("1.0");
    }
    else if (calibrant=="H2O[SANS-1]")
    {
        lineEditMuY0->setText("0.0522");
        lineEditMuA->setText("0.0");
        lineEditMut->setText("1.0");	
        lineEditTo->setText("0.522");
        lineEditTA->setText("0.0");
        lineEditLate->setText("1.0"); 
    }
    else
    { 	
        calibratorChanged();
    }
}


//+++2010-07-24-netak-1408aa
void dan18::MDchanged()
{
    MD=lineEditMD->text().toInt();

    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows)
    {
        QString label=w->windowLabel();
        if (w->name()=="mask" && !label.contains("DAN::Mask::"+QString::number(MD)) && !pushButtonInstrLabel->isHidden()) removeWindows("mask");
        if (w->name()=="sens" && !label.contains("DAN::Sensitivity::"+QString::number(MD)) && !pushButtonInstrLabel->isHidden())removeWindows("sens");
    }
    
    
    updateMaskList();
    updateSensList();
    
    spinBoxLTx->setMaxValue(MD); spinBoxLTx->setValue(int(MD/16));
    spinBoxLTy->setMaxValue(MD); spinBoxLTy->setValue(int(MD/16));
    spinBoxRBx->setMaxValue(MD); spinBoxRBx->setValue(MD-int(MD/16)+1);
    spinBoxRBy->setMaxValue(MD); spinBoxRBy->setValue(MD-int(MD/16)+1);
    
    spinBoxLTxBS->setMaxValue(MD); spinBoxLTxBS->setValue(int(MD/2-MD/16+MD/64));
    spinBoxLTyBS->setMaxValue(MD); spinBoxLTyBS->setValue(int(MD/2-MD/16+MD/64));
    spinBoxRBxBS->setMaxValue(MD); spinBoxRBxBS->setValue(int(MD/2+MD/16-MD/64)+1);
    spinBoxRByBS->setMaxValue(MD); spinBoxRByBS->setValue(int(MD/2+MD/16-MD/64)+1);
    
    spinBoxFrom->setMaxValue(MD);
    spinBoxFrom->setValue(1);
    spinBoxTo->setMaxValue(MD);
    spinBoxTo->setValue(MD);    
}



//+++
void dan18::dataDimensionChanged(const QString& newDimension)
{
    spinBoxRegionOfInteres->setMaxValue(newDimension.toInt());
    spinBoxRegionOfInteres->setValue(newDimension.toInt());
    
    dataRangeOfInteresChanged(newDimension.toInt());
}

//+++
void dan18::dataRangeOfInteresChanged(int newValue)
{
    if ( int(newValue/2)*2!=newValue)
    {
        spinBoxRegionOfInteres->setValue(newValue-1);
        newValue=newValue-1;
    }
    QStringList lst;
    lst<<"1"<<"2";
    for (int i=3; i<=int(newValue/2);i++)
    {
        if ( int(newValue/i)*i==newValue) lst<<QString::number(i);
    }
    lst<<QString::number(newValue);
    comboBoxBinning->clear();
    comboBoxBinning->insertStringList(lst);
    comboBoxBinning->setCurrentItem(0);
    
    binningChanged("1");
}

//+++
void dan18::binningChanged(const QString& newValue)
{
    
    lineEditMD->setText(QString::number(int(spinBoxRegionOfInteres->value() / newValue.toInt())));
    lineEditPS->setText(QString::number(lineEditResoPixelSize->text().toDouble()*comboBoxBinning->currentText().toDouble()));
}

void dan18::secondHeaderExist( bool exist )
{
    if (exist)
    {
        lineEditWildCard2ndHeader->setEnabled(true);
        spinBoxHeaderNumberLines2ndHeader->setEnabled(true);
    }
    else
    {
        lineEditWildCard2ndHeader->setEnabled(false);
        lineEditWildCard2ndHeader->setText("");
        spinBoxHeaderNumberLines2ndHeader->setEnabled(false);
        spinBoxHeaderNumberLines2ndHeader->setValue(0);
    }
}

void dan18::readLambdaFromHeader( bool YN )
{
    if (!YN)
    {
        lineEditSel1->setEnabled(true);
        lineEditSel2->setEnabled(true);
    }
    else
    {
        lineEditSel1->setEnabled(false);
        lineEditSel2->setEnabled(false);
    }
}

void dan18::findCalibrators()
{
    if (!app() || app()->sasPath=="") return;
    //+++
    QDir dd;
    
    QString calPath=app()->sasPath+"/calibrationStandards";
    calPath=calPath.replace("//","/");
    if (!dd.cd(calPath))
    {
        calPath=QDir::homeDirPath()+"/calibrationStandards";
        calPath=calPath.replace("//","/");
        
        if (!dd.cd(calPath))
        {
            dd.cd(QDir::homeDirPath());
            dd.mkdir("./qtiSAS/calibrationStandards");
            dd.cd("./qtiSAS/calibrationStandards");
        }
    };
    calPath=dd.absPath();
    
    QStringList lst = dd.entryList("*.ACS");
    lst.gres(".ACS", "");
    lst.prepend("DirectBeam-No-Attenuator");
    lst.prepend("Plexi-1.5mm-[Current]");
    lst.prepend("H2O[SANS-1]");
    lst.prepend("DirectBeam[KWS-3]");
    lst.prepend("Plexi-1.5mm[KWS-1&2]");
    
    QString ct=comboBoxCalibrant->currentText();
    
    comboBoxCalibrant->clear();
    comboBoxCalibrant->insertStringList(lst);
    if (lst.contains(ct))     comboBoxCalibrant->setCurrentText(ct);
}



void dan18::calibratorChanged()
{
    if (app()->sasPath=="") return;
    //+++
    QDir dd;
    QString calPath=app()->sasPath+"/calibrationStandards";
    calPath=calPath.replace("//","/");
    
    if (!dd.cd(calPath))
    {
        calPath=QDir::homeDirPath()+"/calibrationStandards";
        calPath=calPath.replace("//","/");
        
        if (!dd.cd(calPath))
        {
            dd.cd(QDir::homeDirPath());
            dd.mkdir("./qtiSAS/calibrationStandards");
            dd.cd("./qtiSAS/calibrationStandards");
        }
    };
    calPath=dd.absPath();
    
    if (comboBoxCalibrant->currentItem() > 1 )
    {
        QFile f(calPath+"/"+comboBoxCalibrant->currentText()+".ACS") ;
        if ( !f.open( IO_ReadOnly ) ) return;
        
        //+++
        lineEditMuY0->setEnabled(true);
        lineEditMuA->setEnabled(true);
        lineEditMut->setEnabled(true);
        lineEditTo->setEnabled(true);
        lineEditTA->setEnabled(true);
        lineEditLate->setEnabled(true);
        
        
        QTextStream t( &f );
        
        
        QString s;
        QStringList lst;
        
        while(!t.atEnd())
        {
            s=t.readLine().stripWhiteSpace();
            
            lst.clear();
            lst=lst.split(" ",s);
            if (lst.count()>1)
            {
                if (s.contains("muO")) lineEditMuY0->setText(lst[1]);
                if (s.contains("muA")) lineEditMuA->setText(lst[1]);
                if (s.contains("lambdaM")) lineEditMut->setText(lst[1]);
                if (s.contains("TO")) lineEditTo->setText(lst[1]);
                if (s.contains("TA")) lineEditTA->setText(lst[1]);
                if (s.contains("lambdaT")) lineEditLate->setText(lst[1]);
            }
        }
        f.close();
    }
}
