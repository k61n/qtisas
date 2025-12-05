/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS mask tools
 ******************************************************************************/

#include "dan18.h"
#include "parser-ascii.h"

//+++ connect Slots
void dan18::maskConnectSlot()
{
    //+++ 2020-05
    connect( lineEditDeadRows,        SIGNAL( returnPressed() ), this, SLOT( createMask()  ) );
    connect( lineEditDeadCols,        SIGNAL( returnPressed() ), this, SLOT( createMask()  ) );
    connect( lineEditMaskPolygons,        SIGNAL( returnPressed() ), this, SLOT( createMask()  ) );
    //---
    connect( pushButtonCreateMaskAs,    SIGNAL( clicked() ), this, SLOT( saveMaskAs()   ) );
    connect( pushButtonCreateMask,      SIGNAL( clicked() ), this, SLOT( createMask()   ) );
    connect( pushButtonSaveMaskTr,      SIGNAL( clicked() ), this, SLOT( createMaskTr() ) );
    connect( pushButtonSaveMask,        SIGNAL( clicked() ), this, SLOT( saveMaskFul()  ) );
    connect( pushButtonOpenMask,        SIGNAL( clicked() ), this, SLOT( loadMaskFul()  ) );
    
    connect( pushButtonGetCoord1,       SIGNAL( clicked() ), this, SLOT( readCoord1()   ) );
    connect( pushButtonGetCoord2,       SIGNAL( clicked() ), this, SLOT( readCoord2()   ) );
    connect( pushButtonGetCoord3,       SIGNAL( clicked() ), this, SLOT( readCoord3()   ) );
    connect( pushButtonGetCoord4,       SIGNAL( clicked() ), this, SLOT( readCoord4()   ) );
    
    connect( toolButtonPlusMaskBS,      SIGNAL( clicked() ), this, SLOT( maskPlusMaskBS() ) );

    connect( pushButtonGetCoordDRows,   SIGNAL( clicked() ), this, SLOT( readCoordDRows()   ) );
    connect( pushButtonGetCoordDCols,   SIGNAL( clicked() ), this, SLOT( readCoordDCols()   ) );
    connect( pushButtonGetCoordTrian,   SIGNAL( clicked() ), this, SLOT( readCoordTriangle()   ) );

    connect(comboBoxMaskFor, &QComboBox::textActivated, this, &dan18::readMaskParaFromMaskMatrix);

    //+++ mask Tools
    connect( pushButtonReadDisplay,       SIGNAL( clicked() ), this, SLOT( readCoorMaskTools()   ) );
    connect( toolBoxMask, SIGNAL( currentChanged(int) ), this, SLOT( matrixList() ) );

    connect(comboBoxMlistMask, &QComboBox::textActivated, this, &dan18::matrixList);

    connect(radioButtonToolActiveMask,SIGNAL( clicked() ), this, SLOT( matrixList()   ) );
    connect(radioButtonToolGraph,SIGNAL( clicked() ), this, SLOT( matrixList()   ) );
    connect(radioButtonSelectMatrix,SIGNAL( clicked() ), this, SLOT( matrixList()   ) );
    
    connect( pushButtonUpdateMatrixActive,SIGNAL( clicked() ), this, SLOT( updateMatixActive()) );
    connect( pushButtonSetValue,SIGNAL( clicked() ), this, SLOT( maskSetValue()) );
    connect( pushButtonSquared, SIGNAL( clicked() ), this, SLOT( maskSquared() ) );
    connect( pushButtonMaskSector, SIGNAL( clicked() ), this, SLOT( maskSector() ) );
    connect( pushButtonEllipse, SIGNAL( clicked() ), this, SLOT( maskElips() ) );
    connect( pushButtonEllShell, SIGNAL( clicked() ), this, SLOT( maskElipsShell() ) );
    
}

//+++++SLOT::Save mask as
void dan18::saveMaskAs()
{
    bool ok;
    QString oldName=comboBoxMaskFor->currentText();
    
    QString maskName = QInputDialog::getText(this,
                                             "qtiSAS", "Input matrix name for mask:", QLineEdit::Normal,
                                             oldName, &ok);
    
    if ( !ok ) return;

    saveMaskAs(maskName);
}

void dan18::saveMaskAs(QString maskName)
{
    int MD = lineEditMD->text().toInt();

    updateMaskList();

    bool ok = false;

    if (maskName.isEmpty())
        return;

    QList<MdiSubWindow *> windows = app()->windowsList();
    foreach (MdiSubWindow *w, windows)
        if (w->name() == maskName)
        {
            if (!w->windowLabel().contains("DAN::Mask::" + QString::number(MD)))
                ok = true;
            break;
        }

    if (ok || maskName.isEmpty())
        return;

    bool exist = app()->existWindow(maskName);

    createMaskFul(maskName);
    app()->update();

    if (!exist && checkBoxSortOutputToFolders->isChecked())
        app()->changeFolder("DAN :: mask, sens");
    app()->update();
    updateMaskList();
    app()->update();
    comboBoxMaskFor->setCurrentIndex(comboBoxMaskFor->findText(maskName));
}

//+++ SLOT::MASK create standart mask
void dan18::createMask()
{
    updateMaskList();
    
    QString maskName=comboBoxMaskFor->currentText();
    bool exist = app()->existWindow(maskName);

    createMaskFul(maskName);
    
    if (!exist)
    {
        //maximizeWindow(maskName);
        if (checkBoxSortOutputToFolders->isChecked()) app()->changeFolder("DAN :: mask, sens");
        /*
        Folder *cf = ( Folder * ) app()->current_folder;
        app()->folders->setCurrentItem ( cf->folderListItem() );
        app()->folders->setFocus();
         */
    }
}


//+++++SLOT::MASK create mask for transmission calculations
void dan18::createMaskTr()
{
    updateMaskList();
    
    QString maskName=comboBoxMaskFor->currentText();
    bool exist = app()->existWindow(maskName);

    createMaskFullTr(maskName);

    if (!exist)
        app()->maximizeWindow(maskName);
}

//+++++SLOT::save MASK+++++++++++
void dan18::saveMaskFul( )
{
    QString Dir = filesManager->pathInString();
    int MD = lineEditMD->text().toInt();

    QString maskName = comboBoxMaskFor->currentText();

    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);  // allocate sens matrix
    gsl_matrix_set_zero(mask);
    
    if ( make_GSL_Matrix_Symmetric(maskName, mask, MD) ) saveMatrixToFile(Dir+maskName+".mask",mask, MD);
    
    gsl_matrix_free(mask);//2013-11-12
}

//+++++SLOT::load MASK+++++++++++
void dan18::loadMaskFul()
{
    QString Dir = filesManager->pathInString();

    QString maskName = comboBoxMaskFor->currentText();
    
    QString maskFileName = QFileDialog::getOpenFileName(this,
                                                        "open file dialog - Choose a Mask file",
                                                        Dir,
                                                        "*.mask");
    if (maskFileName!="") loadMaskFul(maskName, maskFileName);
}
void dan18::readCoord1()
{
    double x,y;
    if (!readDisplay(x, y)) return;
    
    spinBoxLTx->setValue(Qwt3D::round(x));
    spinBoxLTy->setValue(Qwt3D::round(y));
}

void dan18::readCoord2()
{
    double x,y;
    readDisplay(x, y) ;
    if (!readDisplay(x, y)) return;
    
    spinBoxRBx->setValue(Qwt3D::round(x));
    spinBoxRBy->setValue(Qwt3D::round(y));
}

void dan18::readCoord3()
{
    double x,y;
    readDisplay(x, y) ;
    if (!readDisplay(x, y)) return;
    
    spinBoxLTxBS->setValue(Qwt3D::round(x));
    spinBoxLTyBS->setValue(Qwt3D::round(y));
}

void dan18::readCoord4()
{
    double x,y;
    readDisplay(x, y) ;
    if (!readDisplay(x, y)) return;
    
    spinBoxRBxBS->setValue(Qwt3D::round(x));
    spinBoxRByBS->setValue(Qwt3D::round(y));
}

//+++
void dan18::maskPlusMaskBS()
{
    updateMaskList();

    QString maskName=comboBoxMaskFor->currentText();
    if (!app()->existWindow(maskName))
        return;

    addBS2CurrentMask(maskName);
    
}

//+++++SLOT::MASK slot Mask to table+++++++++++
void dan18::createMaskFul( QString maskName )
{
    int MD = lineEditMD->text().toInt();

    //if (!app()->hiddenApp) app()->hide();
    //app()->blockSignals ( true );
    
    if (checkBoxSortOutputToFolders->isChecked() && !checkExistenceOfMaskNoMessage(QString::number(MD), maskName))
    {
        app()->changeFolder("DAN :: mask, sens");
    }
    
    //+++
    int i,j,mm;
    Matrix *m;
    QString ss;
    bool YN=true;

    QString maskRange;
    maskRange ="|"+QString::number(spinBoxLTx->value());
    maskRange+="|"+QString::number(spinBoxRBx->value());
    maskRange+="|"+QString::number(spinBoxLTy->value());
    maskRange+="|"+QString::number(spinBoxRBy->value());
    maskRange+="|"+QString::number(spinBoxLTxBS->value());
    maskRange+="|"+QString::number(spinBoxRBxBS->value());
    maskRange+="|"+QString::number(spinBoxLTyBS->value());
    maskRange+="|"+QString::number(spinBoxRByBS->value());
    
    //+++2014-11
    maskRange+="| "+lineEditDeadRows->text();
    maskRange+="| "+lineEditDeadCols->text();
    maskRange+="| "+lineEditMaskPolygons->text();
    maskRange+="|";
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if (w->name()==maskName && QString(w->metaObject()->className()) == "Matrix")
    {
        m=(Matrix *)w;
        YN=false;
        break;
    }
    
    //+++
    if (YN)
    {
        
        m=app()->newMatrix(maskName,MD, MD);
        m->setNumericFormat('f',0);
        //m->setCaptionPolicy((MdiSubWindow::CaptionPolicy)0);
        m->setCoordinates(1,MD,1,MD);
        //+++
    }
    else
    {
        m->setDimensions(MD,MD);
        m->setCoordinates(1,MD,1,MD);
    }

    m->setWindowLabel("DAN::Mask::"+QString::number(MD)+maskRange);
    app()->setListViewLabel(maskName, "DAN::Mask::"+QString::number(MD)+maskRange);
    //app()->updateWindowLists(m);
    
    
    int LTy, RBy, LTx, RBx;
    int LTyBS, RByBS, LTxBS, RBxBS;
    
    double xCenter, yCenter, xSize, ySize;
    
    for(i=0;i<MD;i++) for(j=0;j<MD;j++)  m->setText(i,j,"1");
    

    
    if (groupBoxMask->isChecked())
    {
        
        if (comboBoxMaskEdgeShape->currentIndex()==0)
        {
            
            LTy=spinBoxLTy->value()-1;
            RBy=spinBoxRBy->value()-1;
            LTx=spinBoxLTx->value()-1;
            RBx=spinBoxRBx->value()-1;
            
            for(i=0;i<MD;i++) for(j=0;j<MD;j++)
            {
                if (i<(LTy) || i>(RBy) ||
                    j<(LTx) || j>(RBx))
                    m->setText(i,j,"0");
            }
            
        }
        else
        {
            
            LTy=spinBoxLTy->value()-1;
            RBy=spinBoxRBy->value()-1;
            LTx=spinBoxLTx->value()-1;
            RBx=spinBoxRBx->value()-1;
            
            xCenter=(LTx+RBx)/2.0;
            yCenter=(LTy+RBy)/2.0;
            xSize=(RBx-LTx)+2;
            ySize=(RBy-LTy)+2;
            
            for (int xx=0;xx<MD;xx++) for (int yy=0; yy<MD;yy++)
            {
                
                if ( (xCenter-xx)*(xCenter-xx)*4/xSize/xSize+(yCenter-yy)*(yCenter-yy)*4/ySize/ySize>1 )
                    m->setText(yy,xx,"0");
            }
            
            
            
            
        }
    }
    
    
    if (groupBoxMaskBS->isChecked())
    {
        
        if (comboBoxMaskBeamstopShape->currentIndex()==0)
        {
            
            LTyBS=spinBoxLTyBS->value()-1;
            RByBS=spinBoxRByBS->value()-1;
            LTxBS=spinBoxLTxBS->value()-1;
            RBxBS=spinBoxRBxBS->value()-1;
            
            for(i=0;i<MD;i++) for(j=0;j<MD;j++)
            {
                if (i>=(LTyBS) && i<=(RByBS) &&
                    j>=(LTxBS) && j<=(RBxBS))
                    m->setText(i,j,"0");
            }
        }
        else
        {
            
            LTy=spinBoxLTyBS->value()-1;
            RBy=spinBoxRByBS->value()-1;
            LTx=spinBoxLTxBS->value()-1;
            RBx=spinBoxRBxBS->value()-1;
            
            xCenter=(LTx+RBx)/2.0;
            yCenter=(LTy+RBy)/2.0;
            xSize=(RBx-LTx)+0.5;
            ySize=(RBy-LTy)+0.5;
            
            for (int xx=0;xx<MD;xx++) for (int yy=0; yy<MD;yy++)
            {
                
                if ( (xCenter-xx)*(xCenter-xx)*4/xSize/xSize+(yCenter-yy)*(yCenter-yy)*4/ySize/ySize<=1 )
                    m->setText(yy,xx,"0");
            }
            
            
        }
    }
    
    //+++ 2014-11-05
    QString sDeadRows=lineEditDeadRows->text();
    
    if (sDeadRows!="")
    {
        QStringList lst;
        lst = sDeadRows.split(";", Qt::SkipEmptyParts);
        for (int li=0; li<lst.count();li++)
        {
            QStringList lstMin;
            lstMin = lst[li].split("-", Qt::SkipEmptyParts);

            if (lstMin.count()==2)
            {
                if (lstMin[0].toInt()>0 && lstMin[1].toInt()>0 && lstMin[0].toInt()<=MD && lstMin[1].toInt()<=MD && lstMin[0].toInt()<=lstMin[1].toInt())
                {
                 for (int raw=lstMin[0].toInt()-1;raw<=lstMin[1].toInt()-1;raw++) for (int yy=0; yy<MD;yy++)
                 {
                     m->setText(raw,yy,"0");
                 }
                }
                
            }
            else if (lstMin.count()==4 && lstMin[0].toInt()>0 && lstMin[1].toInt()>0 && lstMin[0].toInt()<=MD && lstMin[1].toInt()<=MD && lstMin[0].toInt()<=lstMin[1].toInt())
            {
                int start=0;
                int finish=MD-1;
                
                if (lstMin[2].toInt()<=MD && lstMin[2].toInt()>0 ) start=lstMin[2].toInt()-1;
                if (lstMin[3].toInt()<=MD && lstMin[3].toInt()>0 ) finish=lstMin[3].toInt()-1;
                if (start>finish){int tmp=finish; finish=start; start=tmp;};
                
                for (int raw=lstMin[0].toInt()-1;raw<=lstMin[1].toInt()-1;raw++) for (int yy=start; yy<=finish;yy++)
                {
                    m->setText(raw,yy,"0");
                }
                
            }
        }
    }
    
    //+++ 2014-11-05
    QString sDeadCols=lineEditDeadCols->text();
    
    if (sDeadCols!="")
    {
        QStringList lst;
        lst = sDeadCols.split(";", Qt::SkipEmptyParts);
        for (int li=0; li<lst.count();li++)
        {
            QStringList lstMin;
            lstMin = lst[li].split("-", Qt::SkipEmptyParts);
      
            if (lstMin.count()==2)
            {
                if( lstMin[0].toInt()>0 && lstMin[1].toInt()>0 && lstMin[0].toInt()<=MD && lstMin[1].toInt()<=MD && lstMin[0].toInt()<=lstMin[1].toInt())
                {
                    for (int yy=0; yy<MD;yy++) for (int col=lstMin[0].toInt()-1;col<=lstMin[1].toInt()-1;col++)
                    {
                        m->setText(yy,col,"0");
                    }
                
                }
            }
            else if (lstMin.count()==4)
            {
                if( lstMin[0].toInt()>0 && lstMin[1].toInt()>0 && lstMin[0].toInt()<=MD && lstMin[1].toInt()<=MD && lstMin[0].toInt()<=lstMin[1].toInt())
                {
                    int start=0;
                    int finish=MD-1;
                    
                    if (lstMin[2].toInt()<=MD && lstMin[2].toInt()>0 ) start=lstMin[2].toInt()-1;
                    if (lstMin[3].toInt()<=MD && lstMin[3].toInt()>0 ) finish=lstMin[3].toInt()-1;
                    if (start>finish){int tmp=finish; finish=start; start=tmp;};
                    
                    for (int yy=start; yy<=finish;yy++) for (int col=lstMin[0].toInt()-1;col<=lstMin[1].toInt()-1;col++)
                    {
                        m->setText(yy,col,"0");
                    }
                    
                }
            }
        }
    }
    
    maskTriangle(m, MD);
    
    
    m->setColumnsWidth(25);
    m->resetView();
    
    
    LTy=spinBoxLTy->value();
    RBy=spinBoxRBy->value();
    LTx=spinBoxLTx->value();
    RBx=spinBoxRBx->value();
    
    LTyBS=spinBoxLTyBS->value();
    RByBS=spinBoxRByBS->value();
    LTxBS=spinBoxLTxBS->value();
    RBxBS=spinBoxRBxBS->value();
    
    //*************************************Log Window Output
    toResLog("DAN :: \nMask template is created: \""+maskName+"\". \nEdge: " +
                QString::number(LTx) +" | "+
                QString::number(RBx) +" | "+
                QString::number(LTy) +" | "+
                QString::number(RBy) +" and Beam-Stop: " +
                QString::number(LTxBS) +" | "+
                QString::number(RBxBS) +" | "+
                QString::number(LTyBS) +" | "+
                QString::number(RByBS)+".\n");
    //*************************************Log Window Output
    
    app()->updateWindowLists(m);
    //QThread::msleep(100);

    if (YN) {m->hide(); m->showMaximized();}

    //m->resetView();
    //m->notifyChanges();
    m->notifyModifiedData();
}


//+++++SLOT::MASK slot Mask to table+++++++++++
void dan18::createMaskFullTr( QString maskName )
{
    int MD = lineEditMD->text().toInt();

    //app()->ws->hide();
    //app()->ws->blockSignals ( true );
    
    bool cfbool=checkBoxSortOutputToFolders->isChecked() && !checkExistenceOfMaskNoMessage(QString::number(MD), maskName);
    
    if (cfbool)
    {
        app()->changeFolder("DAN :: mask, sens");
    }
    
    QString maskRange;
    maskRange ="|"+QString::number(spinBoxLTx->value());
    maskRange+="|"+QString::number(spinBoxRBx->value());
    maskRange+="|"+QString::number(spinBoxLTy->value());
    maskRange+="|"+QString::number(spinBoxRBy->value());
    maskRange+="|"+QString::number(spinBoxLTxBS->value());
    maskRange+="|"+QString::number(spinBoxRBxBS->value());
    maskRange+="|"+QString::number(spinBoxLTyBS->value());
    maskRange+="|"+QString::number(spinBoxRByBS->value());
    
    //+++2014-11
    maskRange+="| "+lineEditDeadRows->text();
    maskRange+="| "+lineEditDeadCols->text();
    maskRange+="| "+lineEditMaskPolygons->text();
    maskRange+="|";
    //+++
    int i,j,mm;
    Matrix *m;
    bool YN=true;
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if (w->name()==maskName && QString(w->metaObject()->className()) == "Matrix")
    {
        m=(Matrix *)w;
        YN=false;
        break;
    }
    
    //+++
    if (YN)
    {
        
        m=app()->newMatrix(maskName,MD, MD);
        app()->updateRecentProjectsList();
        m->setNumericFormat('f',0);
        m->setCoordinates(1,MD,1,MD);
        //+++
    }
    
    m->setWindowLabel("DAN::Mask::"+QString::number(MD)+maskRange);
    app()->setListViewLabel(m->name(), "DAN::Mask::"+QString::number(MD)+maskRange);
    app()->updateWindowLists(m);
    
    
    int LTy, RBy, LTx, RBx;
    int LTyBS, RByBS, LTxBS, RBxBS;
    
    double xCenter, yCenter, xSize, ySize;
    
    for(i=0;i<MD;i++) for(j=0;j<MD;j++)  m->setText(i,j,"0");
    
    
    if (groupBoxMaskBS->isChecked())
    {
        
        if (comboBoxMaskBeamstopShape->currentIndex()==0)
        {
            
            LTyBS=spinBoxLTyBS->value()-1;
            RByBS=spinBoxRByBS->value()-1;
            LTxBS=spinBoxLTxBS->value()-1;
            RBxBS=spinBoxRBxBS->value()-1;
            
            for(i=0;i<MD;i++) for(j=0;j<MD;j++)
            {
                if (i>=(LTyBS) && i<=(RByBS) &&
                    j>=(LTxBS) && j<=(RBxBS))
                    m->setText(i,j,"1");
            }
        }
        else
        {
            
            LTy=spinBoxLTyBS->value()-1;
            RBy=spinBoxRByBS->value()-1;
            LTx=spinBoxLTxBS->value()-1;
            RBx=spinBoxRBxBS->value()-1;
            
            xCenter=(LTx+RBx)/2.0;
            yCenter=(LTy+RBy)/2.0;
            xSize=(RBx-LTx)+0.5;
            ySize=(RBy-LTy)+0.5;
            
            for (int xx=0;xx<MD;xx++) for (int yy=0; yy<MD;yy++)
            {
                
                if ( (xCenter-xx)*(xCenter-xx)*4/xSize/xSize+(yCenter-yy)*(yCenter-yy)*4/ySize/ySize<=1 )
                    m->setText(yy,xx,"1");
            }
            
            
        }
    }
    
    m->setColumnsWidth(25);
    m->resetView();
    
    
    LTy=spinBoxLTy->value();
    RBy=spinBoxRBy->value();
    LTx=spinBoxLTx->value();
    RBx=spinBoxRBx->value();
    
    LTyBS=spinBoxLTyBS->value();
    RByBS=spinBoxRByBS->value();
    LTxBS=spinBoxLTxBS->value();
    RBxBS=spinBoxRBxBS->value();
    
    
    //*************************************Log Window Output
    toResLog("DAN :: \nMask template is created: \""+maskName+"\". \nEdge: " +
                QString::number(LTx) +" | "+
                QString::number(RBx) +" | "+
                QString::number(LTy) +" | "+
                QString::number(RBy) +" and Beam-Stop: " +
                QString::number(LTxBS) +" | "+
                QString::number(RBxBS) +" | "+
                QString::number(LTyBS) +" | "+
                QString::number(RByBS)+".\n");
    //*************************************Log Window Output
    
    if (cfbool)
    {
        app()->changeFolder(app()->projectFolder(),true);
    }
    
    //app()->ws->show();
    //app()->ws->blockSignals ( false );
    
    if (cfbool)
    {
        app()->changeFolder("DAN :: mask, sens");
    }
    
    m->notifyModifiedData();
}


//+++++SLOT::load MASK+++++++++++
void dan18::loadMaskFul( QString maskName, QString maskFileName)
{
    int MD = lineEditMD->text().toInt();

    //app()->ws->hide();
    //app()->ws->blockSignals ( true );
    
    if (checkBoxSortOutputToFolders->isChecked() && !checkExistenceOfMaskNoMessage(QString::number(MD), maskName))
    {
        app()->changeFolder("DAN :: mask, sens");
    }
    
    //+++ 
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);  // allocate sens matrix
    gsl_matrix_set_zero(mask);

    ParserASCII::readMatrixByNameGSL(maskFileName, mask);

    for (int i=0;i<MD;i++) for (int j=0; j<MD; j++) 
    {
        if (gsl_matrix_get(mask,i,j) >0.0 ) gsl_matrix_set(mask,i,j,1.0); else gsl_matrix_set(mask,i,j,0.0);    
    }
    
    //    readMatrixByName (maskFileName, MD, MD, false, false, 0, mask);
    
    QString label="DAN::Mask::"+QString::number(MD)+" Mask file: "+maskFileName;
    
    makeMatrixSymmetric(mask, maskName, label, MD);
    
    //*************************************Log Window Output
    toResLog("DAN :: \nMask template is created: \""+maskName+"\"." +" Mask file: "+maskFileName +".\n");
    //*************************************Log Window Output
    
    gsl_matrix_free(mask);//2013-11-12
    
    //app()->ws->show();
    //app()->ws->blockSignals (false );
}


void dan18::addBS2CurrentMask( QString maskName )
{
    
    //+++
    int i,j,mm;
    Matrix *m;
    QString ss;
    bool YN=true;
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if (w->name()==maskName && QString(w->metaObject()->className()) == "Matrix")
    {
        m=(Matrix *)w;
        YN=false;
        break;
    }
    
    //+++
    if (YN) return;
    
    
    
    int LTy, RBy, LTx, RBx;
    int LTyBS, RByBS, LTxBS, RBxBS;
    
    double xCenter, yCenter, xSize, ySize;
    
    
    if (comboBoxMaskBeamstopShape->currentIndex()==0)
    {
        
        LTyBS=spinBoxLTyBS->value()-1;
        RByBS=spinBoxRByBS->value()-1;
        LTxBS=spinBoxLTxBS->value()-1;
        RBxBS=spinBoxRBxBS->value()-1;
        
        for(i=LTyBS;i<=RByBS;i++) for(j=LTxBS;j<=RBxBS;j++)
        {
            m->setText(i,j,"0");
        }
    }
    else
    {
        
        LTy=spinBoxLTyBS->value()-1;
        RBy=spinBoxRByBS->value()-1;
        LTx=spinBoxLTxBS->value()-1;
        RBx=spinBoxRBxBS->value()-1;
        
        xCenter=(LTx+RBx)/2.0;
        yCenter=(LTy+RBy)/2.0;
        xSize=(RBx-LTx)+0.5;
        ySize=(RBy-LTy)+0.5;
        
        for(int yy=LTy;yy<=RBy;yy++) for(int xx=LTx;xx<=RBx;xx++)
        {
            
            if ( (xCenter-xx)*(xCenter-xx)*4/xSize/xSize+(yCenter-yy)*(yCenter-yy)*4/ySize/ySize<=1 ) 
                m->setText(yy,xx,"0");
        }
        
    }
    
    //m->resetView();
    m->notifyModifiedData();
}


void dan18::readCoordDRows()
{
    double x,y;
    if (!readDisplay(x, y)) return;
    
    QString currentStr=lineEditDeadRows->text();
    currentStr=currentStr.remove(" ");
    if (currentStr.right(1)=="-") currentStr+=QString::number(Qwt3D::round(y))+";";
    else currentStr+=QString::number(Qwt3D::round(y))+"-";
        
    lineEditDeadRows->setText(currentStr);
}

void dan18::readCoordDCols()
{
    double x,y;
    if (!readDisplay(x, y)) return;
    
    QString currentStr=lineEditDeadCols->text();
    currentStr=currentStr.remove(" ");
    if (currentStr.right(1)=="-") currentStr+=QString::number(Qwt3D::round(x))+";";
    else currentStr+=QString::number(Qwt3D::round(x))+"-";
    
    lineEditDeadCols->setText(currentStr);
}

void dan18::readCoordTriangle()
{
    double x,y;
    if (!readDisplay(x, y)) return;
    
    QString currentStr=lineEditMaskPolygons->text();
    currentStr=currentStr.remove(" ");
    
    if (int(currentStr.count(",")/2)*2!=currentStr.count(",") || currentStr.count(",")==0 || currentStr.right(1)==";" ) currentStr+=QString::number(Qwt3D::round(x))+"-"+QString::number(Qwt3D::round(y))+",";
    else currentStr+=QString::number(Qwt3D::round(x))+"-"+QString::number(Qwt3D::round(y))+";";
    
    lineEditMaskPolygons->setText(currentStr);
}


bool dan18::maskTriangle(Matrix *m, int md)
{
    QString sMask=lineEditMaskPolygons->text();
    
    if (sMask!="")
    {
        QStringList lst;
        lst = sMask.split(";", Qt::SkipEmptyParts);
        
        int *pointListX=new int[3];
        int *pointListY=new int[3];
        
        for (int li=0; li<lst.count();li++)
        {
            QStringList lstPoint;
            lstPoint = lst[li].split(",", Qt::SkipEmptyParts);
            if (lstPoint.count()!=3) continue;
            
            for (int pi=0; pi<lstPoint.count();pi++)
            {
                QStringList lstXY;
                lstXY = lstPoint[pi].split("-", Qt::SkipEmptyParts);
                pointListX[pi]=lstXY[0].toInt();
                pointListY[pi]=lstXY[1].toInt();
            }
            
            maskTriangle(m, md, pointListX[0], pointListY[0], pointListX[1], pointListY[1], pointListX[2], pointListY[2]);
            
        }
        delete[] pointListX;
        delete[] pointListY;
    }
    return true;
}

bool dan18::maskTriangle(Matrix *m, int md, int x1, int y1, int x2, int y2, int x3, int y3)
{
    if (md<=0) return false;
    
    if (x1>md || x2>md || x3>md || y1>md || y2>md || y3>md) return false;
    if (x1<1 || x2<1 || x3<1 || y1<1 || y2<1 || y3<1) return false;
    if ( ( x1==x2 && y1==y2) || ( x1==x3 && y1==y3) || ( x3==x2 && y3==y2)) return false;
    
    x1--; y1--;
    x2--; y2--;
    x3--; y3--;
    
    for (int ix=0; ix<md; ix++) for (int iy=0; iy<md; iy++)
    {
        double a = (x1 - ix) * (y2 - y1) - (x2 - x1) * (y1 - iy);
        double b = (x2 - ix) * (y3 - y2) - (x3 - x2) * (y2 - iy);
        double c = (x3 - ix) * (y1 - y3) - (x1 - x3) * (y3 - iy);
        
        if ((a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0))
        {
            m->setText(iy,ix,"0");
        }
        
    }
    return true;    
}

//+++
void dan18::readMaskParaFromMaskMatrix( const QString &name )
{
    int MD = lineEditMD->text().toInt();
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if (w->name()==name && QString(w->metaObject()->className()) == "Matrix")
    {
        if (w->windowLabel().contains("DAN::Mask::"+QString::number(MD)))
        {
            QString s=w->windowLabel().remove("DAN::Mask::"+QString::number(MD));
            if ( s.contains("|") )
            {
                //s=s.replace("|"," ");
                s=s.simplified();
                QStringList lst;
                
                    lst = s.split("|", Qt::SkipEmptyParts);
                
                if (lst.count()==8)
                {
                    spinBoxLTx->setValue(lst[0].toInt());
                    spinBoxRBx->setValue(lst[1].toInt());
                    spinBoxLTy->setValue(lst[2].toInt());
                    spinBoxRBy->setValue(lst[3].toInt());
                    spinBoxLTxBS->setValue(lst[4].toInt());
                    spinBoxRBxBS->setValue(lst[5].toInt());
                    spinBoxLTyBS->setValue(lst[6].toInt());
                    spinBoxRByBS->setValue(lst[7].toInt());
                }
                if (lst.count()==11)
                {
                    spinBoxLTx->setValue(lst[0].toInt());
                    spinBoxRBx->setValue(lst[1].toInt());
                    spinBoxLTy->setValue(lst[2].toInt());
                    spinBoxRBy->setValue(lst[3].toInt());
                    spinBoxLTxBS->setValue(lst[4].toInt());
                    spinBoxRBxBS->setValue(lst[5].toInt());
                    spinBoxLTyBS->setValue(lst[6].toInt());
                    spinBoxRByBS->setValue(lst[7].toInt());
                    lineEditDeadRows->setText(lst[8]);
                    lineEditDeadCols->setText(lst[9]);
                    lineEditMaskPolygons->setText(lst[10]);
                }
            }
        }
    }
}

bool dan18::readDisplay(double &x, double &y)
{
    QString info=app()->info->text();
    
    QStringList lst = info.split(";", Qt::SkipEmptyParts);
    
    if(lst.count()<2) return false;
    
    QString xs=lst[0].remove("x=").remove(" ");
    xs=QString::number(xs.toDouble(),'f',3);
    x=xs.toDouble();
    
    
    QString ys=lst[1].remove("y=").remove(" ");
    ys=QString::number(ys.toDouble(),'f',3);
    y=ys.toDouble();
    
    return true;
}

//+++ check existence of mask and sensitivity matrixes
bool dan18::checkExistenceOfMask(QString MaDe, QString maskToCheck)
{
    QStringList lst = app()->findMatrixListByLabel("DAN::Mask::" + MaDe);
    if (!lst.contains(maskToCheck))
    {
        QMessageBox::critical(0, "qtiSAS", "<b>"+maskToCheck+"</b> does not exist!");
        return false;
    }
    return true;
}

bool dan18::checkExistenceOfMaskNoMessage(QString MaDe, QString maskToCheck)
{
    QStringList lst = app()->findMatrixListByLabel("DAN::Mask::" + MaDe);
    if (!lst.contains(maskToCheck))
        return false;

    return true;
}

void dan18::updateMaskList()
{
    int MD = lineEditMD->text().toInt();

    QStringList lst = app()->findMatrixListByLabel("DAN::Mask::" + QString::number(MD));
    if (!lst.contains("mask"))
        lst.prepend("mask");

    QString currentMask;
    
    currentMask=comboBoxMaskFor->currentText();
    comboBoxMaskFor->clear();
    comboBoxMaskFor->addItems(lst);
    if (lst.contains(currentMask))
        comboBoxMaskFor->setCurrentIndex(lst.indexOf(currentMask));
}

//+++ Mask Tools

void dan18::readCoorMaskTools()
{
    double x,y;
    if (!readDisplay(x, y)) return;
    
    doubleSpinBoxXcenter->setValue(x);
    doubleSpinBoxYcenter->setValue(y);
}


void dan18::matrixList( QString selectedName)
{
    if (toolBoxMask->currentIndex()==0) return;
 
    if (selectedName!="")
    {
        if (selectedName=="Matrix-Active") pushButtonUpdateMatrixActive->show();
        else pushButtonUpdateMatrixActive->hide();
    }

    //mask
    QStringList lst;
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if (QString(w->metaObject()->className()) == "Matrix") lst<<w->name();

    QString currentMask;
    currentMask=comboBoxMlistMask->currentText();
    if (selectedName!="" && lst.contains(selectedName)) currentMask=selectedName;
    if (selectedName=="" && radioButtonToolActiveMask->isChecked()&& lst.contains(comboBoxMaskFor->currentText())) currentMask=comboBoxMaskFor->currentText();
    if (selectedName=="" && radioButtonToolGraph->isChecked())
    {
        MultiLayer *plot = (MultiLayer *)app()->activeWindow(app()->MultiLayerWindow);
        if (plot)
        {
            Graph* g = (Graph*)plot->activeLayer();
            if (g && g->curvesList().count()>0 && g->curvesList()[0]->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
            {
                Spectrogram *sp = (Spectrogram *)g->curvesList()[0];
                QString sTmp=sp->matrix()->name();
                if ( lst.contains(sTmp)) currentMask=sTmp;
            }
            
        }
    }
    
    comboBoxMlistMask->clear();
    comboBoxMlistMask->addItems(lst);
    if (lst.contains(currentMask))
        comboBoxMlistMask->setItemText(comboBoxMlistMask->currentIndex(), currentMask);
    
    if (comboBoxMlistMask->currentText()=="Matrix-Active") pushButtonUpdateMatrixActive->show();
    else pushButtonUpdateMatrixActive->hide();
}


void dan18::maskSetValue()
{
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if (QString(w->metaObject()->className()) == "Matrix" && w->name()==comboBoxMlistMask->currentText())
    {
        Matrix *m=(Matrix *)w;
        double newValue=comboBoxValue->currentText().toDouble();
        
        for (int xx=0;xx<m->numCols();xx++) for (int yy=0; yy<m->numRows();yy++)
        {
            m->setText(yy,xx,QString::number(newValue));
        }
        m->notifyModifiedData();
        return;
    }
}

void dan18::updateMatixActive()
{
    QString Number=lineEditCheck->text().simplified(); //+++ uni-sas
    
    if(Number.contains(";"))
    {
        QStringList lstNumberIn = Number.split(";", Qt::SkipEmptyParts);
        Number=lstNumberIn[0];
    }
    
    check(Number, false,"View Matrix [Matrix-Active]");
}

void dan18::maskSquared()
{
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if ( QString(w->metaObject()->className()) == "Matrix" && w->name()==comboBoxMlistMask->currentText())
    {
        Matrix *m=(Matrix *)w;
        
        double xCenter=doubleSpinBoxXcenter->value()-1;
        double yCenter=doubleSpinBoxYcenter->value()-1;
        double xSize=doubleSpinBoxRectXsize->value();
        double ySize=doubleSpinBoxRectYsize->value();
        bool maskInside=true;
        if (comboBoxMaskInside->currentIndex()==1) maskInside=false;
        double newValue=comboBoxValue->currentText().toDouble();
        
        for (int xx=0;xx<m->numCols();xx++) for (int yy=0; yy<m->numRows();yy++)
        {
            if ( maskInside && xx <= (xCenter + xSize/2) && xx>= (xCenter - xSize/2) && yy<= (yCenter + ySize/2) && yy>= (yCenter - ySize/2) ) m->setText(yy,xx,QString::number(newValue));
            
            if ( !maskInside && (xx> (xCenter + xSize/2) || xx< (xCenter - xSize/2) || yy> (yCenter + ySize/2) || yy< (yCenter - ySize/2) )) m->setText(yy,xx,QString::number(newValue));
        }
        m->notifyModifiedData();
        return;
    }
}


void dan18::maskSector()
{
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if ( QString(w->metaObject()->className()) == "Matrix" && w->name()==comboBoxMlistMask->currentText())
    {
        Matrix *m=(Matrix *)w;
        
        double xCenter=doubleSpinBoxXcenter->value()-1;
        double yCenter=doubleSpinBoxYcenter->value()-1;
        double phiMin=doubleSpinBoxAngleFrom->value();
        double phiMax=doubleSpinBoxAngleTo->value();
        double currentPhi;
        bool maskInside=true;
        if (comboBoxMaskInside->currentIndex()==1) maskInside=false;
        double newValue=comboBoxValue->currentText().toDouble();
        
        for (int xx=0;xx<m->numCols();xx++) for (int yy=0; yy<m->numRows();yy++)
        {
            double rr=sqrt ( (xx-xCenter)*(xx-xCenter) + (yy-yCenter)*(yy-yCenter));
            if (xx>=xCenter && yy>=yCenter && rr>0) currentPhi=asin((xx-xCenter)/rr)/M_PI*180;
            else if (xx>=xCenter && yy<yCenter && rr>0) currentPhi=90+asin((yCenter-yy)/rr)/M_PI*180;
            else if (xx<xCenter && yy<=yCenter && rr>0) currentPhi=180+asin((xCenter-xx)/rr)/M_PI*180;
            else if (xx<xCenter && yy>yCenter && rr>0) currentPhi=270+asin((yy-yCenter)/rr)/M_PI*180;
            else currentPhi=phiMin;
            
            if ( maskInside)
            {
                if (phiMin<0 && currentPhi >= 360+phiMin )
                {
                    m->setText(yy,xx,QString::number(newValue));
                    continue;
                }
                if (currentPhi >= phiMin && currentPhi <= phiMax) m->setText(yy,xx,QString::number(newValue));
            }
            else
            {
                if (phiMin<0)
                {
                   if ( currentPhi < 360+phiMin && currentPhi > phiMax ) m->setText(yy,xx,QString::number(newValue));
                }
                else
                {
                    if ( currentPhi < phiMin || currentPhi > phiMax ) m->setText(yy,xx,QString::number(newValue));
                }
            }
        }
        m->notifyModifiedData();
        return;
    }
}


void dan18::maskElips()
{
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if ( QString(w->metaObject()->className()) == "Matrix" && w->name()==comboBoxMlistMask->currentText())
    {
        Matrix *m=(Matrix *)w;
        
        double xCenter=doubleSpinBoxXcenter->value()-1;
        double yCenter=doubleSpinBoxYcenter->value()-1;
        double xSize=doubleSpinBoxEllipseXsize->value();
        double ySize=doubleSpinBoxEllipseYsize->value();
        bool maskInside=true;
        if (comboBoxMaskInside->currentIndex()==1) maskInside=false;
        double newValue=comboBoxValue->currentText().toDouble();
        if ( xSize==0 || ySize==0) return;
        
        for (int xx=0;xx<m->numCols();xx++) for (int yy=0; yy<m->numRows();yy++)
        {
            if ( maskInside && (xCenter-xx)*(xCenter-xx)*4/xSize/xSize+(yCenter-yy)*(yCenter-yy)*4/ySize/ySize<=1 ) m->setText(yy,xx,QString::number(newValue));
            if ( !maskInside && (xCenter-xx)*(xCenter-xx)*4/xSize/xSize+(yCenter-yy)*(yCenter-yy)*4/ySize/ySize>1 ) m->setText(yy,xx,QString::number(newValue));
        }
        m->notifyModifiedData();
        return;
    }
}


void dan18::maskElipsShell()
{
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if ( QString(w->metaObject()->className()) == "Matrix" && w->name()==comboBoxMlistMask->currentText())
    {
        Matrix *m=(Matrix *)w;
        
        double xCenter=doubleSpinBoxXcenter->value()-1;
        double yCenter=doubleSpinBoxYcenter->value()-1;
        double xSize=doubleSpinBoxEllipseXsize->value();
        double ySize=doubleSpinBoxEllipseYsize->value();
        double shell=doubleSpinBoxEllShell->value();
        
        bool maskInside=true;
        if (comboBoxMaskInside->currentIndex()==1) maskInside=false;
        double newValue=comboBoxValue->currentText().toDouble();
        if ( xSize==0 ||  ySize==0 || shell==0 ) return;
        
        for (int xx=0;xx<m->numCols();xx++) for (int yy=0; yy<m->numRows();yy++)
        {
            if ( maskInside && (xCenter-xx)*(xCenter-xx)*4/(xSize+shell)/(xSize+shell)+(yCenter-yy)*(yCenter-yy)*4/(ySize+shell)/(ySize+shell)<=1 && (xCenter-xx)*(xCenter-xx)*4/(xSize-shell)/(xSize-shell)+(yCenter-yy)*(yCenter-yy)*4/(ySize-shell)/(ySize-shell)>=1) m->setText(yy,xx,QString::number(newValue));
            
            if ( !maskInside && ( (xCenter-xx)*(xCenter-xx)*4/(xSize+shell)/(xSize+shell)+(yCenter-yy)*(yCenter-yy)*4/(ySize+shell)/(ySize+shell)>1 || (xCenter-xx)*(xCenter-xx)*4/(xSize-shell)/(xSize-shell)+(yCenter-yy)*(yCenter-yy)*4/(ySize-shell)/(ySize-shell)< 1) ) m->setText(yy,xx,QString::number(newValue));
        }
        m->notifyModifiedData();
        return;
    }
}
