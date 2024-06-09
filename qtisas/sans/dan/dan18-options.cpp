/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS select instrument functions
 ******************************************************************************/

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
    rx.setPatternSyntax(QRegExp::Wildcard);
    QList<MdiSubWindow *> windows = app()->windowsList();
    foreach (MdiSubWindow *w, windows)
        if (rx.exactMatch(w->name()))
        {
            w->askOnCloseEvent(false);
            app()->closeWindow(w);
        }
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

    QString calPath;

    if (!QDir(app()->sasPath + "/calibrationStandards/").exists())
        QDir().mkdir(app()->sasPath + "/calibrationStandards/");

    calPath = app()->sasPath + "/calibrationStandards/";
    calPath = calPath.replace("//", "/");

    bool ok = false;
    QString fileName = comboBoxCalibrant->currentText();

    if (comboBoxCalibrant->currentIndex() < 5)
        fileName = "Create Your Calibrant: Input Calibrant Name";

    while (ok == false)
    {
        fileName = QInputDialog::getText(this,
                                         "QtiSAS", "Create Your Calibrant", QLineEdit::Normal,
                                         fileName, &ok);
        if ( !ok ||  fileName.isEmpty() )
        {
            return;
        }
        
        if (fileName=="Plexi-1.5mm-[Current]" || fileName=="Plexi-1.5mm[KWS-1&2]" || fileName=="DirectBeam-No-Attenuator" || fileName=="DirectBeam[KWS-3]" || fileName=="H2O[SANS-1]")
        {
            ok=false;
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
    
    
    if ( !f.open( QIODevice::WriteOnly ) )
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

    QStringList lst;
    for (int i = 0; i < comboBoxCalibrant->count(); i++)
        lst << comboBoxCalibrant->itemText(i);

    if (lst.contains(fileName))
        comboBoxCalibrant->setCurrentIndex(lst.indexOf(fileName));

    calibrantselected();
    
    return;
}

void dan18::deleteCurrentCalibrant()
{
    if (app()->sasPath == "")
        return;

    if (comboBoxCalibrant->currentIndex() < 5)
        return;

    QString fileName = comboBoxCalibrant->currentText();

    //+++
    QDir dd;
    QString calPath=app()->sasPath+"/calibrationStandards";
    calPath=calPath.replace("//","/");
    
    if (!dd.cd(calPath))
    {
        calPath=QDir::homePath()+"/calibrationStandards";
        calPath=calPath.replace("//","/");
        if (!dd.cd(calPath))
        {
            dd.cd(QDir::homePath());
            dd.mkdir("./qtiSAS/calibrationStandards");
            dd.cd("./qtiSAS/calibrationStandards");
        }
    };
    calPath=dd.absolutePath();
    
    dd.remove(fileName+".ACS");
    
    findCalibrators();
    calibrantselected();
}

//+++ when calibrant is changed
void dan18::calibrantselected()
{
    QString calibrant=comboBoxCalibrant->currentText();
    
    if (comboBoxCalibrant->currentIndex()<5) pushButtonDeleteCurrentCalibrator->setEnabled(false);
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
    int MD = lineEditMD->text().toInt();

    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows)
    {
        QString label=w->windowLabel();
        if (w->name()=="mask" && !label.contains("DAN::Mask::"+QString::number(MD)) && !pushButtonInstrLabel->isHidden()) removeWindows("mask");
        if (w->name()=="sens" && !label.contains("DAN::Sensitivity::"+QString::number(MD)) && !pushButtonInstrLabel->isHidden())removeWindows("sens");
    }
    
    
    updateMaskList();
    updateSensList();
    
    spinBoxLTx->setMaximum(MD); spinBoxLTx->setValue(int(MD/16));
    spinBoxLTy->setMaximum(MD); spinBoxLTy->setValue(int(MD/16));
    spinBoxRBx->setMaximum(MD); spinBoxRBx->setValue(MD-int(MD/16)+1);
    spinBoxRBy->setMaximum(MD); spinBoxRBy->setValue(MD-int(MD/16)+1);
    
    spinBoxLTxBS->setMaximum(MD); spinBoxLTxBS->setValue(int(MD/2-MD/16+MD/64));
    spinBoxLTyBS->setMaximum(MD); spinBoxLTyBS->setValue(int(MD/2-MD/16+MD/64));
    spinBoxRBxBS->setMaximum(MD); spinBoxRBxBS->setValue(int(MD/2+MD/16-MD/64)+1);
    spinBoxRByBS->setMaximum(MD); spinBoxRByBS->setValue(int(MD/2+MD/16-MD/64)+1);
    
    spinBoxFrom->setMaximum(MD);
    spinBoxFrom->setValue(1);
    spinBoxTo->setMaximum(MD);
    spinBoxTo->setValue(MD);    
}



//+++
void dan18::dataDimensionChanged(const QString& newDimension)
{
    spinBoxRegionOfInteres->setMaximum(newDimension.toInt());
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
    comboBoxBinning->addItems(lst);
    comboBoxBinning->setCurrentIndex(0);
    
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
void dan18::findCalibrators()
{
    if (!app() || app()->sasPath=="") return;

    QString calPath;

    if (!QDir(app()->sasPath + "/calibrationStandards/").exists())
        QDir().mkdir(app()->sasPath + "/calibrationStandards/");

    calPath = app()->sasPath + "/calibrationStandards/";
    calPath = calPath.replace("//", "/");
    
    QStringList lst = QDir(calPath).entryList(QStringList() << "*.ACS");
    lst.replaceInStrings(".ACS", "");
    lst.prepend("DirectBeam-No-Attenuator");
    lst.prepend("Plexi-1.5mm-[Current]");
    lst.prepend("H2O[SANS-1]");
    lst.prepend("DirectBeam[KWS-3]");
    lst.prepend("Plexi-1.5mm[KWS-1&2]");
    
    QString ct=comboBoxCalibrant->currentText();
    
    comboBoxCalibrant->clear();
    comboBoxCalibrant->addItems(lst);
    if (lst.contains(ct))
        comboBoxCalibrant->setCurrentIndex(lst.indexOf(ct));
}



void dan18::calibratorChanged()
{
    if (app()->sasPath=="") return;

    QString calPath;

    if (!QDir(app()->sasPath + "/calibrationStandards/").exists())
        QDir().mkdir(app()->sasPath + "/calibrationStandards/");

    calPath = app()->sasPath + "/calibrationStandards/";
    calPath = calPath.replace("//", "/");

    if (comboBoxCalibrant->currentIndex() > 1 )
    {
        QFile f(calPath+"/"+comboBoxCalibrant->currentText()+".ACS") ;
        if ( !f.open( QIODevice::ReadOnly ) ) return;
        
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
            s=t.readLine().trimmed();
            
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
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
