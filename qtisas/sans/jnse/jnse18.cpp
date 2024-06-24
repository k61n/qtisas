/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: JNSE data reading interface
 ******************************************************************************/

#include <QDebug>
#include <QDockWidget>
#include <QUrl>

#include "Folder.h"
#include "jnse18.h"

jnse18::jnse18(QWidget *parent)
: QWidget(parent)
{
    setupUi(this);
    
    //+++ Read Settings
    readSettings();
    pushButtonCOHINCOH->hide();
    jnseTab->setCurrentIndex(0);
    lineEditFunctionsFilter->hide();
    
    //+++ connections
    connectSlot();
}

// *******************************************
// *** connectSlot()
// *******************************************
void jnse18::connectSlot()
{
    //+++ data
    connect( pushButtonDATpath, SIGNAL( clicked() ), this, SLOT( buttomRADpath() ) );
    connect( pushButtonMakeList, SIGNAL( clicked() ), this, SLOT( slotMakeList() ) );
    connect( pushButtonCOHINCOH, SIGNAL( clicked() ), this, SLOT( slotMakeListCohIncoh() ) );
    connect( lineEditPathDAT, SIGNAL( textChanged(const QString&) ), this, SLOT( forceReadSettings() ) );
    //+++ fit
    connect( comboBoxHeaderTables, SIGNAL( activated(int) ), this, SLOT( headerTableSeleted() ) );
    connect( pushButtonStartNSEfit, SIGNAL( clicked() ), this, SLOT( nseFit() ) );
    
    //connect( lineEditFunctionsFilter, SIGNAL( returnPressed() ), this, SLOT( filterFitFunctions() ) );
    
    connect(jnseTab, SIGNAL(currentChanged(int)), this, SLOT( findHeaderTables() ) );
    
}

jnse18::~jnse18()
{
    writeSettings();
}

// *******************************************
// +++ app
// *******************************************
ApplicationWindow* jnse18::app()
{
    if (this->parentWidget()->parent())
    {
        QDockWidget *docWin=(QDockWidget*)this->parentWidget();
        return (ApplicationWindow *)docWin->parent();
    }
    return (ApplicationWindow *)this->parent();
}

// *******************************************
// *** Log-output
// *******************************************
void jnse18::toResLog(QString text)
{
    QString info =text;
    if (app()->dockWidgetArea(app()->logWindow) == Qt::RightDockWidgetArea )
    {
        app()->current_folder->appendLogInfo(text);
        app()->results->setText(app()->current_folder->logInfo());
    }
    else app()->showResults(text, true);
}


// *******************************************
// *** readSettings()
// *******************************************
void jnse18::readSettings()
{
#ifdef Q_OS_MACOS
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "qtisas", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "qtisas", "QtiSAS");
#endif

    //+++
    bool ok;
    QString ss;
    //+++
    settings.beginGroup("/JNSE");
    //+++
    ok = settings.contains("/lineEditPathDAT");
    ss = settings.value("/lineEditPathDAT",0).toString();
    if (ok && ss!="home") lineEditPathDAT->setText(ss);
    //+++
    ok = settings.contains("/lineEditFileName");
    ss = settings.value("/lineEditFileName",0).toString();
    if (ok && ss!="info") lineEditFileName->setText(ss);
    //+++
    settings.endGroup();
}
// *******************************************
// *** readSettings()
// *******************************************
void jnse18::writeSettings()
{
#ifdef Q_OS_MACOS
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "qtisas", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "qtisas", "QtiSAS");
#endif

    //+++
    settings.beginGroup("/JNSE");
    //+++
    if (lineEditPathDAT->text()!="home") settings.setValue("/lineEditPathDAT",  lineEditPathDAT->text());
    if (lineEditFileName->text()!="info")settings.setValue("/lineEditFileName",  lineEditFileName->text());
    //+++
    settings.endGroup();
}

// *******************************************
// *** buttomRADpath()
// *******************************************
void jnse18::buttomRADpath()
{
    QString pppath=lineEditPathDAT->text();
    
    if (pppath=="home") pppath = QDir::home().path();
    
    QString s = QFileDialog::getExistingDirectory(this,
                                                  "get data directory - Choose a directory",
                                                  pppath);
    if (s!="") lineEditPathDAT->setText(s);
}



// *******************************************
// *** slotMakeListDtr()
// *******************************************
void jnse18::slotMakeList()
{
    
    app()->changeFolder("JNSE :: *.dtr files");
    app()->folders->setFocus();
    
    QString TableName  = lineEditFileName->text();
    QString DatDir     = lineEditPathDAT->text();
    
    bool ok;
    TableName = QInputDialog::getText(this,
                                      "Table's Generation: all header info", "Enter name of Table:", QLineEdit::Normal,
                                      TableName, &ok);
    if ( !ok ||  TableName.isEmpty() ) return;
    
    int i;
    
    //+++ create table
    Table* tableDat;
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"JNSE - File Import",DatDir,"Make Yours Filter (*.YOURS)");
    fd->setDirectory(DatDir);
    fd->setFileMode(QFileDialog::ExistingFiles );
    fd->setWindowTitle(tr("JNSE - File Import"));
    fd->setNameFilter("JNSE - dtr files (*)");
    //+++
    if (fd->exec() == QDialog::Rejected)
    {
        QMessageBox::critical( 0, "QtiSAS", "Select firts!");
        return;
    }
    
    
    lineEditFileName->setText(TableName);
    
    
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    int startRaw=0;
    
    bool exist=false;
    QList<MdiSubWindow *> windows = app()->windowsList();
    foreach (MdiSubWindow *w, windows) if (QString(w->metaObject()->className()) == "Table" && w->name()==TableName) { exist=true; break;};
    
    //+++
    if (exist)
    {
        foreach (MdiSubWindow *w, windows) if (QString(w->metaObject()->className()) == "Table" && w->name()==TableName) {tableDat=(Table*)w; break;};
        
        if (!tableDat || tableDat->numCols()<19)
        {
            QMessageBox::critical( 0, "QtiSAS", "Create new table (# cols)");
            return;
        }
        
        startRaw=tableDat->numRows();
    }
    else tableDat=app()->newTable(TableName, 0, 20);

    
    QStringList typeCols;
    
    //+++ Cols Names
    tableDat->setColName(0,"sample-code"); tableDat->setColPlotDesignation(0,Table::None);  typeCols<<"1";
    tableDat->setColName(1,"sample-info"); tableDat->setColPlotDesignation(1,Table::None); typeCols<<"1";
    tableDat->setColName(2,"mode");tableDat->setColPlotDesignation(2,Table::None); typeCols<<"1";
    tableDat->setColName(3,"run-number");tableDat->setColPlotDesignation(3,Table::None);typeCols<<"0";
    tableDat->setColName(4,"q"); tableDat->setColPlotDesignation(4,Table::X); typeCols<<"0";
    tableDat->setColName(5,"dq"); tableDat->setColPlotDesignation(5,Table::xErr); typeCols<<"0";
    tableDat->setColName(6,"q-unit"); tableDat->setColPlotDesignation(6,Table::None); typeCols<<"0";
    tableDat->setColName(7,"t-unit"); tableDat->setColPlotDesignation(7,Table::None); typeCols<<"0";
    tableDat->setColName(8,"temp");tableDat->setColPlotDesignation(8,Table::None);typeCols<<"0";
    tableDat->setColName(9,"numor");tableDat->setColPlotDesignation(9,Table::None);typeCols<<"0";
    tableDat->setColName(10,"ref");tableDat->setColPlotDesignation(10,Table::None);typeCols<<"0";
    tableDat->setColName(11,"bgr");tableDat->setColPlotDesignation(11,Table::None);typeCols<<"0";
    tableDat->setColName(12,"with-bgr");tableDat->setColPlotDesignation(12,Table::None);typeCols<<"0";
    tableDat->setColName(13,"tfac-bgr");tableDat->setColPlotDesignation(13,Table::None);typeCols<<"0";
    tableDat->setColName(14,"volfrac");tableDat->setColPlotDesignation(14,Table::None);typeCols<<"0";
    tableDat->setColName(15,"qbins");tableDat->setColPlotDesignation(15,Table::None);typeCols<<"0";
    tableDat->setColName(16,"tbins");tableDat->setColPlotDesignation(16,Table::None);typeCols<<"0";
    tableDat->setColName(17,"oor-cnt");tableDat->setColPlotDesignation(17,Table::None);typeCols<<"0";
    tableDat->setColName(18,"upd-cnt");tableDat->setColPlotDesignation(18,Table::None);typeCols<<"0";
    tableDat->setColName(19,"table-name"); tableDat->setColPlotDesignation(19,Table::None);  typeCols<<"1";
    
    tableDat->setColumnTypes(typeCols);
    //+++
    tableDat->show();
    //+++
    QString fnameOnly;
    
    QString s,ss,nameMatrix;
    int iter;
    QString currentQ;
    //
    for(iter=0; iter<filesNumber;iter++)
    {
        // Files
        
        QFile f(selectedDat[iter]);
        QTextStream t( &f );
        f.open(QIODevice::ReadOnly);
        
        QStringList lst = selectedDat[iter].split("/", Qt::SkipEmptyParts);
        
        QString fileCode=lst[lst.count()-1];
        
        //fileCode=fileCode.replace("_","-");
        
        //fileCode=lst.split(".",fileCode)[0];
        fileCode=fileCode.remove(".dtr").remove(".DTR");
        int lineNum=0;
        
        bool stop=false;
        
        do
        {
            tableDat->setNumRows(startRaw+1);
            
            //+++ First line;;  Table Name & Date
            s = t.readLine().trimmed(); lineNum++;
            
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            //+++ Code 1
            tableDat->setText(startRaw,0,lst[0] + "  " + fileCode);
            //+++ Date
            tableDat->setText(startRaw,1,s.right(s.length()-s.indexOf(" ",0)).trimmed().remove("\""));
            //+++
            s = t.readLine().trimmed().simplified();lineNum++;
            while (s=="") {s = t.readLine().trimmed().simplified();lineNum++;}
            
            lst.clear();
            lst = s.split(" ", Qt::SkipEmptyParts);
            //+++
            if (!s.contains("vs t/ns") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            
            //+++ Code 2
            if (lst.count()>0) tableDat->setText(startRaw,2,lst[1]);
            
            //+++ Run Number
            QString matrixName=lst[lst.count()-1];
            tableDat->setText(startRaw,3,matrixName);
            matrixName.prepend(fileCode+"-");
            matrixName.append("-v-");

            matrixName=matrixName.replace(".", "-");
            matrixName=matrixName.replace(",", "-");
            matrixName=matrixName.replace(";", "-");
            matrixName=matrixName.replace(":", "-");

            matrixName=app()->generateUniqueName(matrixName);
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            if (!s.contains("parameter") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++ q
            if (!s.contains("q"))
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            
            currentQ=s.remove("q").trimmed();
            tableDat->setText(startRaw,4,currentQ);
            
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  q_var dq
            if (!s.contains("q_var") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,5,s.remove("q_var").trimmed());
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  q_unit_SI
            if (!s.contains("q_unit_SI") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,6,s.remove("q_unit_SI").trimmed());
            
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  t_unit_SI
            if (!s.contains("t_unit_SI") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,7,s.remove("t_unit_SI").trimmed());
            
            
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  temp
            if (!s.contains("temp") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,8,s.remove("temp").trimmed());
            
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  numor1
            if (!s.contains("numor1") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,9,s.remove("numor1").trimmed());
            
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  ref
            if (!s.contains("ref1") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,10,s.remove("ref1").trimmed());
            
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  bgr1
            if (!s.contains("bgr1") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,11,s.remove("bgr1").trimmed());

            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  withbgr
            if (!s.contains("withbgr") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,12,s.remove("withbgr").trimmed());

            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  tfac_bgr
            if (!s.contains("tfac_bgr") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,13,s.remove("tfac_bgr").trimmed());

            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  volfrac
            if (!s.contains("volfrac") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,14,s.remove("volfrac").trimmed());

            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  qbins
            if (!s.contains("qbins") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,15,s.remove("qbins").trimmed());
            
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  tbins
            if (!s.contains("tbins") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,16,s.remove("tbins").trimmed());
            
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  oor_cnt
            if (!s.contains("oor_cnt") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,17,s.remove("oor_cnt").trimmed());
            
            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++  upd_cnt
            if (!s.contains("upd_cnt") && !stop)
            {
                QMessageBox::critical( 0, "QtiSAS",
                                      "Check Format of File: "+selectedDat[iter-startRaw]+"; line # "+QString::number(lineNum)
                                      );
                break; stop=true;
            }
            tableDat->setText(startRaw,18,s.remove("upd_cnt").trimmed());
            
            tableDat->setText(startRaw,19,matrixName);

            //+++
            s = t.readLine().trimmed();lineNum++;
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            if (s.contains("values")) s = t.readLine().trimmed();lineNum++;
            
            //+++  Read Table +++
            Table* tableResults=app()->newTable(matrixName, 0,7);
            
            //+++ Cols Names
            tableResults->setColName(0,"tau-ns"); tableResults->setColPlotDesignation(0,Table::X);
            tableResults->setColName(1,"Sqt-To-Sq"); tableResults->setColPlotDesignation(1,Table::Y); tableResults->setColComment(1, "Q="+currentQ+"/A");
            tableResults->setColName(2,"d-Sqt-To-Sq"); tableResults->setColPlotDesignation(2,Table::yErr);
            tableResults->setColName(3,"var-Sqt-To-Sq"); tableResults->setColPlotDesignation(3,Table::yErr);
            tableResults->setColName(4,"var-tau-ns"); tableResults->setColPlotDesignation(4,Table::xErr);
            tableResults->setColName(5,"nupdates"); tableResults->setColPlotDesignation(5,Table::Y);
            tableResults->setColName(6,"Qeff"); tableResults->setColPlotDesignation(6,Table::Y);

           
            //+++
            int Ncur=0;
            QRegExp rx("((\\-|\\+)?\\d*(\\.|\\,)\\d*((e|E)(\\-|\\+)\\d*)?)");
            
            s = t.readLine().trimmed();lineNum++;
            s.replace(',','.');
            
            while(s!="" && !s.contains("#eod") && !s.contains("#nxt") )
            {
                
                s=s.trimmed().simplified();
                
                QStringList lst;
                lst = s.split(" ", Qt::SkipEmptyParts);
                
                if(lst.count()<7) continue;
                
                tableResults->setNumRows(Ncur+1);
                
                for (i=0;i<7;i++) tableResults->setText(Ncur,i,QString::number(lst[i].toDouble(),'E'));
                //if (lst.count()==4) tableResults->setText(Ncur,3,QString::number(lst[3].toDouble(),'E'));
                
                Ncur++;
                
                
                if (t.atEnd()) break;
                
                s = t.readLine().trimmed();lineNum++;
                s.replace(',','.');
                
                
            }
            //
            tableResults->adjustColumnsWidth(false);

            // +++ Hide all files
            tableResults->setWindowLabel("NSE Dataset");
            app()->setListViewLabel(tableResults->name(), "NSE Dataset");
            //app()->hiddenWindows->append(tableResults);
            tableResults->setHidden();
            
            
            // ---
            while (s=="") {s = t.readLine().trimmed();lineNum++;}
            
            //+++
            if (t.atEnd())
            {
                stop=true;
            }
            startRaw++;
            
        } while(!stop);
        
        f.close();
        
    }

    //+++
    tableDat->adjustColumnsWidth(false);

    tableDat->setWindowLabel("NSE Headers");
    app()->setListViewLabel(tableDat->name(), "NSE Headers");
    app()->updateWindowLists(tableDat);
    tableDat->showMaximized();
    
    findHeaderTables();
}

//+++ Search of  Header tables
void jnse18::findHeaderTables()
{
    QString activeTable=comboBoxHeaderTables->currentText();
    //+++
    QStringList list;
    QList<MdiSubWindow *> windows = app()->windowsList();
    foreach (MdiSubWindow *w, windows) if (QString(w->metaObject()->className()) == "Table")
    {
        Table *t = (Table *)w;
        if (t->table()->horizontalHeaderItem(3)->text() == "run-number")
            list<<t->name();
    }
    
    //
    comboBoxHeaderTables->clear();
    if (list.count()==0) return;
    comboBoxHeaderTables->addItems(list);
    if (activeTable!="" && list.contains(activeTable))
        comboBoxHeaderTables->setItemText(comboBoxHeaderTables->currentIndex(),
                                          activeTable);
    headerTableSeleted();
}

//+++
void jnse18::headerTableSeleted()
{
    if (comboBoxHeaderTables->count()>0)
    {
        QList<MdiSubWindow *> windows = app()->windowsList();
        foreach (MdiSubWindow *w, windows) if (QString(w->metaObject()->className()) == "Table" && w->name()==comboBoxHeaderTables->currentText())
        {
            Table *t = (Table *)w;
            spinBoxFrom->setMaximum(t->numRows());
            spinBoxTo->setValue(1);
            spinBoxTo->setMaximum(t->numRows());
            spinBoxTo->setValue(t->numRows());
            break;
        }
    }
    else
    {
        spinBoxFrom->setMaximum(1);
        spinBoxTo->setMaximum(1);
        findHeaderTables();
    }
    
}

// *******************************************
// *** slotMakeListCohIncoh()
// *******************************************
void jnse18::slotMakeListCohIncoh()
{
    QString DatDir    =lineEditPathDAT->text();
    
    
    app()->changeFolder("JNSE :: coh/inc files");
    app()->folders->setFocus();
    
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"J-NSE - File Import",DatDir,"Make Yours Filter (*.YOURS)");
    fd->setDirectory(DatDir);
    fd->setFileMode(QFileDialog::ExistingFiles );
    fd->setWindowTitle(tr("J-NSE - File Import"));
    fd->setNameFilter("J-NSE coh/incoh files (*)");
    //+++
    if (fd->exec() == QDialog::Rejected)
        return;
    
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    int startRaw=0;
    
    //+++
    QString fnameOnly, fileLabel, s;
    //
    for(int iter=0; iter<filesNumber;iter++)
    {
        // Files
        QFile f(selectedDat[iter]);
        QTextStream t( &f );
        f.open(QIODevice::ReadOnly);
        
        
        QStringList lst = selectedDat[iter].split("/", Qt::SkipEmptyParts);
        fnameOnly=selectedDat[iter];
        fnameOnly=fnameOnly.remove(DatDir);
        
        if (lst.count()>1)     fnameOnly=lst[lst.count()-2]+"-"+lst[lst.count()-1];
        if (lst.count()==1) fnameOnly=lst[0];
        
        
        fnameOnly=fnameOnly.remove("/").remove("(").remove(")");
        fnameOnly=fnameOnly.replace(".","-");
        fnameOnly=fnameOnly.replace("_","-");
        fnameOnly=app()->generateUniqueName(fnameOnly+"-");
        
        fileLabel="";
        
        //+++ First line;;  Table Name & Date
        s = t.readLine().trimmed();
        
        if (!s.contains("coh"))
        {
            f.close();
            continue;
        }
        
        
        fileLabel+="JNSE coh/inc file:  "+selectedDat[iter];
        //    fileLabel+=s+"\n";
        
        while(!s.contains("values"))
        {
            s = t.readLine().trimmed();
            //        fileLabel+=s+" --- ";
        }
        //    fileLabel+="\n";
        
        //+++  Read Table +++
        Table* tableResults=app()->newTable(fnameOnly, 0,5);
        
        //+++ Cols Names
        tableResults->setColName(0,"Q");         tableResults->setColPlotDesignation(0,Table::X);
        tableResults->setColName(1,"Icoh");     tableResults->setColPlotDesignation(1,Table::Y);
        tableResults->setColName(2,"dIcoh");    tableResults->setColPlotDesignation(2,Table::yErr);
        tableResults->setColName(3,"Iinc");     tableResults->setColPlotDesignation(3,Table::Y);
        tableResults->setColName(4,"dIinc");    tableResults->setColPlotDesignation(4,Table::yErr);
        
        int rows=0;
        
        do {
            s = t.readLine().trimmed();
            if (s.contains("#nxt")) break;
            
            s=s.simplified();
            
            QStringList lst;
            lst = s.split(" ", Qt::SkipEmptyParts);
            
            if(lst.count()!=3) continue;
            if(lst[0]=="NaN" || lst[1]=="NaN" || lst[2]=="NaN") continue;
            
            rows++;
            tableResults->setNumRows(rows);
            
            tableResults->setText(rows-1,0,QString::number(lst[0].toDouble(),'E'));
            tableResults->setText(rows-1,1,QString::number(lst[1].toDouble(),'E'));
            tableResults->setText(rows-1,2,QString::number(lst[2].toDouble(),'E'));
        } while (true);
        
        
        //+++ First line;;  Table Name & Date
        s = t.readLine().trimmed();
        
        if (!s.contains("inc")) {
            f.close();
            continue;
        }
        
        
        //    fileLabel+="\n\n"+s+"\n";
        
        while(!s.contains("values")) {
            s = t.readLine().trimmed();
            //        fileLabel+=s+" --- ";
        }
        //    fileLabel+="\n";
        
        int irows=0;
        
        do {
            s = t.readLine().trimmed();
            if (s.contains("#eod")) break;
            
            s=s.simplified();
            
            QStringList lst;
            lst = s.split(" ", Qt::SkipEmptyParts);
            
            if(lst.count()!=3) continue;
            if(lst[0]=="NaN" || lst[1]=="NaN" || lst[2]=="NaN") continue;
            
            irows++;
            if (irows>rows) {
                tableResults->setNumRows(rows);
                tableResults->setText(irows-1,0,QString::number(lst[0].toDouble(),'E'));
            }
            tableResults->setText(irows-1,3,QString::number(lst[1].toDouble(),'E'));
            tableResults->setText(irows-1,4,QString::number(lst[2].toDouble(),'E'));
        } while (true);

        tableResults->adjustColumnsWidth(false);

        // +++ Hide all files
        tableResults->setWindowLabel(fileLabel);
        app()->setListViewLabel(tableResults->name(), fileLabel);
        tableResults->setHidden();
        f.close();
    }
    
    //+++

}

// *******************************************
// *** forceReadSettings()
// *******************************************
void jnse18::forceReadSettings()
{
    if (lineEditPathDAT->text()=="home") readSettings();
}

// *******************************************
// *** nseFit()
// *******************************************
void jnse18::nseFit()
{
    if (comboBoxHeaderTables->count()>0)
    {
        emit signalJnseFit(comboBoxHeaderTables->currentText(),comboBoxFitFunctions->currentText(),spinBoxFrom->value(),spinBoxTo->value());
    }
    else QMessageBox::warning(this,tr("QtiSAS"), tr("Select Header's Table first!"));
}

// *******************************************
// *** filterFitFunctions
// *******************************************
void jnse18::filterFitFunctions(QStringList lst, bool local)
{
    if (local && comboBoxFitFunctions->count()==0) return;
    if (!local && lst.count()==0) return;
    
    if (local)
    {
        for ( int i = 0 ; i < comboBoxFitFunctions->count() ; ++i ) lst<<comboBoxFitFunctions->model()->index( i, 0 ).data( Qt::DisplayRole ).toString();
    }
    QRegExp rx(lineEditFunctionsFilter->text());
    rx.setPatternSyntax(QRegExp::Wildcard);
    lst=lst.filter(rx);
    
    comboBoxFitFunctions->clear();
    comboBoxFitFunctions->addItems(lst);
}
