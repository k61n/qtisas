/***************************************************************************
 File : dan18-sens.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : SANS Data Analisys Interface
 
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
void dan18::sensConnectSlot()
{
    connect( pushButtonCreateSens,      SIGNAL( clicked() ), this, SLOT( createSens()       ));
    connect( pushButtonCreateSensAs,    SIGNAL( clicked() ), this, SLOT( saveSensAs()       ));
    connect( pushButtonSaveSens,        SIGNAL( clicked() ), this, SLOT( saveSensFul()      ));
    connect( pushButtonOpenSens,        SIGNAL( clicked() ), this, SLOT( loadSensFul()      ));
    
    connect( pushButtonAnyPlexi,        SIGNAL( clicked() ), this, SLOT( selectAnyPlexy()   ));
    connect( pushButtonAnyEB,           SIGNAL( clicked() ), this, SLOT( selectAnyEB()      ));
    connect( pushButtonAnyBC,           SIGNAL( clicked() ), this, SLOT( selectAnyBC()      ));
    connect( pushButtonAnyTr,           SIGNAL( clicked() ), this, SLOT( calculateAnyTr()   ));
    connect( pushButtonTrHiden,         SIGNAL( clicked() ), this, SLOT( calculateTrHidden()));
    

    
    connect( lineEditTransAnyD,         SIGNAL( editingFinished() ), this, SLOT(SensitivityLineEditCheck()    ));
    connect( lineEditPlexiAnyD,         SIGNAL( editingFinished() ), this, SLOT(SensitivityLineEditCheck()    ));
    connect( lineEditEBAnyD,            SIGNAL( editingFinished() ), this, SLOT(SensitivityLineEditCheck()    ));
    connect( lineEditBcAnyD,            SIGNAL( editingFinished() ), this, SLOT(SensitivityLineEditCheck()    ));
    
    connect( spinBoxErrLeftLimit,       SIGNAL( valueChanged(double) ), this, SLOT( checkErrorLimits()     ));
    connect( spinBoxErrRightLimit,      SIGNAL( valueChanged(double) ), this, SLOT( checkErrorLimits()     ));
    
    connect( comboBoxSensFor, SIGNAL( activated(const QString&) ), this, SLOT( readFileNumbersFromSensitivityMatrix(const QString&) ) );
}

//+++++SLOT::MASK create standart mask
void dan18::createSens()
{
    ImportantConstants();
    
    if (lineEditPlexiAnyD->paletteBackgroundColor()!=QColor(128, 255, 7) && buttonGroupSensanyD->isChecked()) { QMessageBox::critical(0, "DAN::SANS", "<b>check sensitivity fields!</b>");return;};
    
    QString maskName=comboBoxMaskFor->currentText();
    if (!checkExistenceOfMask(lineEditMD->text(), maskName)) return;
    
    updateSensList();
    
    QString sensmatrixName=comboBoxSensFor->currentText();
    bool exist=existWindow(sensmatrixName);
    
    createSensFul( sensmatrixName );
    if (spinBoxErrLeftLimit->value()>0 || spinBoxErrRightLimit->value()<100) createSensFul(sensmatrixName);
    
    if (!exist )
    {
        maximizeWindow(sensmatrixName);
        if (checkBoxSortOutputToFolders->isChecked()) app()->changeFolder("DAN :: mask, sens");
        /*
        Folder *cf = ( Folder * ) app()->current_folder;
        app()->folders->setCurrentItem ( cf->folderListItem() );
        app()->folders->setFocus();
         */
    }
}

//+++++SLOT::Save mask as
void dan18::saveSensAs()
{
    QString oldName=comboBoxSensFor->currentText();
    
    bool ok;
    QString sensName = QInputDialog::getText(
                                             "qtiSAS", "Input matrix name for sensitivity:", QLineEdit::Normal,
                                             oldName, &ok, this );
    
    if ( !ok ||  sensName.isEmpty() )
    {
        return;
    }
    
    saveSensAs(sensName);
}

//+++++SLOT::Save mask as
void dan18::saveSensAs(QString sensName)
{
    ImportantConstants();
    
    QString maskName=comboBoxMaskFor->currentText();
    
    if (!checkExistenceOfMask(lineEditMD->text(), maskName)) return;
    
    QString oldName=comboBoxSensFor->currentText();
    
    if (sensName.isEmpty() )return;
    
    bool ok=false;
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if ( w->isA("Matrix") && w->name()==sensName)
    {
        if (!w->windowLabel().contains( "DAN::Sensitivity::"+QString::number(MD))) ok=true;
        break;
    }
    
    if ( ok ||  sensName.isEmpty() ) return;

    bool exist=existWindow(sensName);
    
    createSensFul( sensName );
    if (spinBoxErrLeftLimit->value()>0 || spinBoxErrRightLimit->value()<100) createSensFul(sensName);
    
    if (!exist )
    {
        maximizeWindow(sensName);
        if (checkBoxSortOutputToFolders->isChecked()) app()->changeFolder("DAN :: mask, sens");
        /*
        Folder *cf = ( Folder * ) app()->current_folder;
        app()->folders->setCurrentItem ( cf->folderListItem() );
        app()->folders->setFocus();
         */
    }
    //    statusShow();
    updateSensList();
    
    comboBoxSensFor->setCurrentText(sensName);
}

//+++++SLOT::save Sens+++++++++++
void dan18::saveSensFul()
{
    ImportantConstants();
    
    QString sensName=comboBoxSensFor->currentText();
    
    //+++
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);  // allocate sens matrix
    gsl_matrix_set_zero(sens);
    
    if ( make_GSL_Matrix_Symmetric(sensName, sens, MD) ) saveMatrixToFile(Dir+sensName+".sens",sens, MD);
    
    gsl_matrix_free(sens);
}


//+++++SLOT::load Sens+++++++++++
void dan18::loadSensFul()
{
    ImportantConstants();
    QString sensName = comboBoxSensFor->currentText();
    
    QString sensFileName = QFileDialog::getOpenFileName(
                                                        Dir,
                                                        "*.sens",
                                                        this,
                                                        "open file dialog",
                                                        "Choose a Sensitivity file");
    
    if (sensFileName!="") loadSensFul(sensName, sensFileName);
}

void dan18::selectAnyPlexy()
{
    QString fileNumber="";
    if (selectFile(fileNumber))
    {
        lineEditPlexiAnyD->setText(fileNumber);
        lineEditPlexiAnyD->setStyleSheet("background-color: rgb(128, 255, 7);");;
    }
}

void dan18::selectAnyEB()
{
    QString fileNumber="";
    if (selectFile(fileNumber))
    {
        lineEditEBAnyD->setText(fileNumber);
        lineEditEBAnyD->setStyleSheet("background-color: rgb(128, 255, 7);");;
    }
}


void dan18::selectAnyBC()
{
    QString fileNumber="";
    if (selectFile(fileNumber))
    {
        lineEditBcAnyD->setText(fileNumber);
        lineEditBcAnyD->setStyleSheet("background-color: rgb(128, 255, 7);");;
    }
}


void dan18::calculateTrHidden()
{
    ImportantConstants();
    
    if (lineEditPlexiAnyD->paletteBackgroundColor()!=QColor(128, 255, 7)  ||
        lineEditEBAnyD->paletteBackgroundColor()!=QColor(128, 255, 7))
    {
        lineEditTransAnyD->setText("0.0000");
        return;
    }
    double sigmaTr=0;
    double Tr=readTransmission( lineEditPlexiAnyD->text(), lineEditEBAnyD->text(), comboBoxMaskFor->currentText(), 0, 0, sigmaTr);
    
    lineEditTransAnyD->setText(QString::number(Tr,'f',4));
}

void dan18::calculateAnyTr()
{
    ImportantConstants();
    
    if (lineEditPlexiAnyD->paletteBackgroundColor()==QColor(128, 255, 7) && checkBoxSensTr->isChecked())
    {
        double lambda=readLambda(lineEditPlexiAnyD->text());
        lineEditTransAnyD->setText(QString::number(tCalc(lambda),'f',4));
        return;
    }
    
    
    if (lineEditPlexiAnyD->paletteBackgroundColor()!=QColor(128, 255, 7)  ||
        lineEditEBAnyD->paletteBackgroundColor()!=QColor(128, 255, 7))
    {
        lineEditTransAnyD->setText("0.0000");
        return;
    }
    
    double sigmaTr=0;
    double Tr=readTransmission( lineEditPlexiAnyD->text(), lineEditEBAnyD->text(), comboBoxMaskFor->currentText(),0,0,sigmaTr);
    
    lineEditTransAnyD->setText(QString::number(Tr,'f',4));
}

//*******************************************
//+++  SENS:: Sensitivities limits
//*******************************************
void dan18::checkErrorLimits()
{
    if ( spinBoxErrLeftLimit->value() >= spinBoxErrRightLimit->value() )
    {
        spinBoxErrLeftLimit->setValue(0.0);
        spinBoxErrRightLimit->setValue(100.0);
    }
}


//+++++SLOT::MASK slot Mask to table ++++++++++++++++++++++++++++++++++++++++
void dan18::createSensFul(QString sensName)
{
    //app()->ws->hide();
    //app()->ws->blockSignals ( true );
    
    if (checkBoxSortOutputToFolders->isChecked()  && !checkExistenceOfSensNoMessage(QString::number(MD), sensName))
    {
        app()->changeFolder("DAN :: mask, sens");
    }
    
    ImportantConstants();
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);  // allocate mask matrix
    gsl_matrix_set_zero(mask);
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);  // allocate sens matrix
    gsl_matrix_set_zero(sens);
    // error matrix
    gsl_matrix *sensErr=gsl_matrix_alloc(MD,MD);  // allocate sens matrix
    gsl_matrix_set_zero(sensErr);
    
    QString maskName=comboBoxMaskFor->currentText();
    
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    //+++
    gsl_matrix_memcpy(sens,mask);
    
    
    if (!buttonGroupSensanyD->isChecked() || lineEditPlexiAnyD->paletteBackgroundColor()!=QColor(128, 255, 7))
    {
        
        QString label="DAN::Sensitivity::"+QString::number(MD)+"::Plexi::No";
        label+="::EB::No";
        label+="::BC::No";
        label+="::Tr::No";
        
        double maskedSensValue=lineEditSensMaskedPixels->text().toDouble();
        if (maskedSensValue>0.0)
        {
            gsl_matrix_add_constant(mask,-1.0);
            gsl_matrix_scale(mask,0.0 - maskedSensValue);
            gsl_matrix_add(sens, mask);
        }
        
        makeMatrixSymmetric(sens, sensName,label, MD);
        
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(sensErr);
        
        
        //app()->ws->show();
        //app()->ws->blockSignals ( false );
        
        return;
    }
    
    
    if (lineEditPlexiAnyD->paletteBackgroundColor()==QColor(128, 255, 7))
    {
        // numberof  "active" pixels  Nmask
        double Nmask=0;
        double sum=0.0;
        double sum2=0.0;
        int ii,jj;
        QString info="\n\nSensitivity matrix: \""+sensName+"\".\n";
        info+="Mask matrix: \""+maskName+"\".\n\n";
        // +++
        gsl_matrix *PLEXI=gsl_matrix_alloc(MD,MD);
        gsl_matrix_set_zero(PLEXI);
        gsl_matrix *EB=gsl_matrix_alloc(MD,MD);
        gsl_matrix_set_zero(EB);
        gsl_matrix *BC=gsl_matrix_alloc(MD,MD);
        gsl_matrix_set_zero(BC);
        
        double transmision=0.0;
        
        //+++
        QString PlexiFileNumber=lineEditPlexiAnyD->text();
        
        double D=readDataDinM( PlexiFileNumber );
        
        readErrorMatrixRel( PlexiFileNumber, sensErr );
        gsl_matrix_mul_elements(sensErr,mask);
        
        double counts;
        
        //***spinBoxErrLeftLimit
        if ( spinBoxErrLeftLimit->value() > 0 ||  spinBoxErrRightLimit->value() < 100.0 )
            for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
            {
                if (gsl_matrix_get(mask,ii,jj)>0)
                {
                    counts=gsl_matrix_get(sensErr,ii,jj)*100;
                    if ( counts < spinBoxErrLeftLimit->value() || counts > spinBoxErrRightLimit->value() )
                    {
                        gsl_matrix_set(mask,ii,jj, 0);
                        gsl_matrix_set(sensErr,ii,jj, 0);
                    }
                }
            }
        
        
        
        //+++
        if ( checkFileNumber( PlexiFileNumber ) )
        {
            readMatrixCor( PlexiFileNumber, PLEXI);
            info+="| Plexi-run-#:: "+  PlexiFileNumber;
        }
        //
        QString EBfileNumber=lineEditEBAnyD->text();
        
        bool EBexistYN=false;
        if ( checkFileNumber( EBfileNumber ) )
        {
            readMatrixCor( EBfileNumber, EB);
            info+=" | EB-run-#:: "+ EBfileNumber;
            EBexistYN=true;
        }
        else
        {
            info+=" | EB-run:: ...not used...";
        }
        
        //
        QString BCfileNumber=lineEditBcAnyD->text();
        
        if ( checkFileNumber( BCfileNumber ) )
        {
            // read BC matrix 2012
            if (checkBoxBCTimeNormalization->isChecked())
            {
                readMatrixCorTimeNormalizationOnly( BCfileNumber, BC );
                
                //Normalization constant
                double TimeSample=spinBoxNorm->value();
                double ttime=readDuration( PlexiFileNumber );
                if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;
                
                double NormSample=readDataNormalization(PlexiFileNumber);
                
                if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
                
                gsl_matrix_scale(BC,NormSample);      // EB=T*EB
            }
            else readMatrixCor( BCfileNumber, BC );
            
            
            info+=" | BC-run-#:: "+ BCfileNumber;
        }
        else
        {
            info+=" | BC-run:: ...not used...";
        }
        
        
        //+++
        gsl_matrix_sub(PLEXI,BC);       // Plexi-BC
        
        
        if ( checkFileNumber( BCfileNumber ) && checkBoxBCTimeNormalization->isChecked() && EBexistYN)
        {
            readMatrixCorTimeNormalizationOnly( BCfileNumber, BC );
            
            //Normalization constant
            double TimeSample=spinBoxNorm->value();
            double ttime=readDuration( EBfileNumber );
            if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;
            
            double NormSample=readDataNormalization(EBfileNumber);
            
            if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
            
            gsl_matrix_scale(BC,NormSample);      // EB=T*EB
        }
        
        
        gsl_matrix_sub(EB,BC);  // EB-BC
        
        //+++ Transmision selection
        transmision=lineEditTransAnyD->text().toDouble() ;
        info+=" | Tr : "+QString::number(transmision, 'f', 4);
        
        double Xc, Yc;
        readCenterfromMaskName( maskName, Xc, Yc, MD );
        
        
        if (transmision<1.0 && transmision>0.0 && checkBoxParallaxTr->isChecked())
        {
            
            transmissionThetaDependenceTrEC(EB, Xc, Yc, D*100, transmision);
        }
        
        //+++
        gsl_matrix_scale(EB,transmision);
        if(EBexistYN) gsl_matrix_sub(PLEXI,EB);
        gsl_matrix_mul_elements(PLEXI,mask);
        

        
        //+++
        if (checkBoxParallax->isChecked() || checkBoxParallaxTr->isChecked() )
        {
            parallaxCorrection(PLEXI, Xc, Yc, D*100, transmision);
        }
        

        
        double sum0=0;
        bool maskPixel=false;
        //+++
        for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
        {
            //+++
            counts=gsl_matrix_get(PLEXI,ii,jj);
            if (gsl_matrix_get(mask,ii,jj)>0 && counts<=0 && checkBoxMaskNegative->isChecked())
            {
                gsl_matrix_set(PLEXI,ii,jj,0.0);
                gsl_matrix_set(mask,ii,jj,0.0);
                gsl_matrix_set(sens,ii,jj,0.0);
                counts=0;
                maskPixel=true;
            }
            else maskPixel=false;
            //+++
            if ( !maskPixel )
            {
                sum0+=counts;
                sum2+=counts*counts;
                
                sum+=1/counts;
                Nmask+=gsl_matrix_get(mask,ii,jj);
            }
            
        }
        
        //+++
        for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
        {
            if (gsl_matrix_get(mask,ii,jj)>0)
            {
                counts=gsl_matrix_get(PLEXI,ii,jj);
                
                if (Nmask!=0 && counts>0)
                    gsl_matrix_set(sens,ii,jj, sum0/Nmask/counts);
                else
                {
                    gsl_matrix_set(mask,ii,jj,0.0);
                    gsl_matrix_set(sens,ii,jj,0);
                }
            }
        }
        
        // matrixConvolusion(PLEXI,mask,MD); //*** 2019: removed (two times convoluted)
        
        
        // +++ to res log
        
        info+=    " | Number of used pixels : N : "+ QString::number(Nmask) +
        "  |\n| Normalized Mean Intensity : "+
        QString::number(sum0/Nmask)+" "+QChar(177)+" " +
        QString::number(sqrt(fabs(sum2-sum0*sum0)/Nmask)/Nmask);
        
        info+=" | Normalization : "+comboBoxNorm->currentText()+" : "+QString::number( 1.0/readDataNormalization( lineEditPlexiAnyD->text() ) )+" |\n";
        
        
        info+="| Dead-time correction : "+ QString::number(
                                                           readDataDeadTime( lineEditPlexiAnyD->text()) )+" | ";
        info+="High Q Corrections : ";
        if (checkBoxParallax->isChecked())
        {
            info+="Yes : Center : "+QString::number(Xc+1,'f',2)+"x"+ QString::number(Yc+1,'f',2) +" |\n";
        }
        else info+="No |\n";
        
        
        
        
        toResLog("DAN :: "+info+"\n");
        if (checkBoxSensError->isChecked())
            makeMatrixSymmetric(sensErr, sensName+"Error", "DAN::Sensitivity::Error::"+QString::number(MD), MD);
        
        
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
        
        makeMatrixSymmetric(mask, maskName, "DAN::Mask::"+QString::number(MD)+maskRange, MD);
        
        
        //+++ 2018 :: set sens for masked pixels
        
        QString label="DAN::Sensitivity::"+QString::number(MD)+"::Plexi::"+lineEditPlexiAnyD->text();
        label+="::EB::"+lineEditEBAnyD->text();
        label+="::BC::"+lineEditBcAnyD->text();
        label+="::Tr::"+lineEditTransAnyD->text();
        
        
        double maskedSensValue=lineEditSensMaskedPixels->text().toDouble();
        if (maskedSensValue>0.0)
        {
            gsl_matrix_add_constant(mask,-1.0);
            gsl_matrix_scale(mask,0.0 - maskedSensValue);
            gsl_matrix_add(sens, mask);
        }
        
        makeMatrixSymmetric(sens, sensName,label, MD);
        
        
        gsl_matrix_free(PLEXI);
        gsl_matrix_free(EB);
        gsl_matrix_free(BC);	
    }   
    
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    gsl_matrix_free(sensErr);
    
    //app()->ws->show();
    //app()->ws->blockSignals ( false );
}


//+++++SLOT::load Sens+++++++++++
void dan18::loadSensFul( QString sensName, QString sensFileName)
{
    //app()->ws->hide();
    //app()->ws->blockSignals ( true );
    
    if (checkBoxSortOutputToFolders->isChecked() && !checkExistenceOfSensNoMessage(QString::number(MD), sensName))
    {
        app()->changeFolder("DAN :: mask, sens");
    }
    
    //+++ 
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);  // allocate sens matrix
    gsl_matrix_set_zero(sens);
    
    
    readMatrixByNameGSL (sensFileName, sens);
    
    
    for (int i=0;i<MD;i++) for (int j=0; j<MD; j++) 
    {
        if (gsl_matrix_get(sens,i,j) <0.0 ) gsl_matrix_set(sens,i,j,0.0);
    }
    
    QString label="DAN::Sensitivity::"+QString::number(MD)+" Sens file: "+sensFileName;
    
    makeMatrixSymmetric(sens, sensName, label, MD);
    
    gsl_matrix_free(sens);
    
    lineEditPlexiAnyD->setText(" Sens file: "+sensFileName);
    lineEditPlexiAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
    lineEditEBAnyD->setText("");
    lineEditEBAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
    lineEditBcAnyD->setText("");
    lineEditBcAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
    lineEditTransAnyD->setText( QString::number(1.0, 'f', 4 ) );
    //app()->ws->show();
    //app()->ws->blockSignals ( false );
}

//+++
void dan18::readFileNumbersFromSensitivityMatrix( const QString &name )
{
    MD=lineEditMD->text().toInt();
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if ( w->isA("Matrix") && w->name()==name)
    {
        if (w->windowLabel().contains("DAN::Sensitivity::"+QString::number(MD)))
        {
            if (w->windowLabel().contains(" Sens file:"))
            {
                QString s=w->windowLabel();
                lineEditPlexiAnyD->setText(s.right(s.length()-s.find("Sens file:")));
                lineEditPlexiAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
                lineEditEBAnyD->setText("");
                lineEditEBAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
                lineEditBcAnyD->setText("");
                lineEditBcAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
                lineEditTransAnyD->setText( QString::number(1.0, 'f', 4 ) );
                break;
            }
            
            QString s=w->windowLabel().remove("DAN::Sensitivity::"+QString::number(MD)+"::Plexi::");
            if ( s.contains("::EB::") )
            {
                
                lineEditPlexiAnyD->setText(s.left(s.find("::EB::")));
                lineEditEBAnyD->setText(s.mid(s.find("::EB::")+6,s.find("::BC::")-s.find("::EB::")-6 ));
                lineEditBcAnyD->setText(s.mid(s.find("::BC::")+6,s.find("::Tr::")-s.find("::BC::")-6 ));
                lineEditTransAnyD->setText(s.mid(s.find("::Tr::")+6,s.length()-s.find("::Tr::")-6 ));
                SensitivityLineEditCheck();
            }
            else
            {
                lineEditPlexiAnyD->setText(s);
            }
            
        }
        break;
    }
}



//+++++SENSITIVITY-enable-check++++++++++++
void dan18::SensitivityLineEditCheck()
{
    ImportantConstants();
    
    double change;
    
    change = lineEditTransAnyD->text().toDouble();
    if (change<0.|| change>1.0)
    {
        lineEditTransAnyD->setText( QString::number( 0.0, 'f', 4 ) );
    }
    else lineEditTransAnyD->setText( QString::number(change, 'f', 4 ) );
    
    //Plexi
    if (checkFileNumber( lineEditPlexiAnyD->text() ))
        lineEditPlexiAnyD->setStyleSheet("background-color: rgb(128, 255, 7);");
    else
        lineEditPlexiAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
    
    //EB
    if (checkFileNumber( lineEditEBAnyD->text() ) )
        lineEditEBAnyD->setStyleSheet("background-color: rgb(128, 255, 7);");
    else
        lineEditEBAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
    //BC
    if (checkFileNumber( lineEditBcAnyD->text() ) )
        lineEditBcAnyD->setStyleSheet("background-color: rgb(128, 255, 7);");
    else
        lineEditBcAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
}

//+++ check existence of mask and sensitivity matrixes
bool dan18::checkExistenceOfSens(QString MaDe, QString sensToCheck)
{
    QStringList lst;
    findMatrixListByLabel("DAN::Sensitivity::"+MaDe,lst);
    if (!lst.contains(sensToCheck))
    {
        QMessageBox::critical(0, "qtiSAS", "<b>"+sensToCheck+"</b> does not exist!");
        return false;
    }
    return true;
}

bool dan18::checkExistenceOfSensNoMessage(QString MaDe, QString sensToCheck)
{
    QStringList lst;
    findMatrixListByLabel("DAN::Sensitivity::"+MaDe,lst);
    if (!lst.contains(sensToCheck))
    {
        return false;
    }
    return true;
}

void dan18::updateSensList()
{
    ImportantConstants();
    
    //sens
    QStringList lst;
    findMatrixListByLabel("DAN::Sensitivity::"+QString::number(MD),lst);
    if (!lst.contains("sens")) lst.prepend("sens");
    QString currentSens;
    
    currentSens=comboBoxSensFor->currentText();
    comboBoxSensFor->clear();
    comboBoxSensFor->insertStringList(lst);
    if (lst.contains(currentSens)) comboBoxSensFor->setCurrentText(currentSens);
}

//*******************************************
//+++  new-daDan:: get-Sensitivity-Number
//*******************************************
QString dan18::getSensitivityNumber(QString sensName)
{
    QString res="";
    MD=lineEditMD->text().toInt();
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if ( w->isA("Matrix") && w->name()==sensName)
    {
        if (w->windowLabel().contains("DAN::Sensitivity::"+QString::number(MD)))
        {
            if (w->windowLabel().contains("Sens file:")) return "";
            QString s=w->windowLabel().remove("DAN::Sensitivity::"+QString::number(MD)+"::Plexi::");
            if (s.contains("::EB::")) s=s.left(s.find("::EB::"));
                res=s;
        }
        break;
    }

return res;
}
