/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS data reduction tools
 ******************************************************************************/

#include "dan18.h"
#include "Folder.h"

#include <QElapsedTimer>

#include <gsl/gsl_fit.h>
#include <gsl/gsl_spline.h>

//*******************************************
//+++  RT tools:: Sum [slot]
//*******************************************
void dan18::dandanConnectSlots()
{
    connect(pushButtonIQ, SIGNAL( clicked() ), this, SLOT(slotDANbuttonIQ() ) );
    connect(pushButtonPolar, SIGNAL( clicked() ), this, SLOT(slotDANbuttonPolar() ) );
    connect(pushButtonIxy, SIGNAL( clicked() ), this, SLOT(slotDANbuttonIxy() ) );
    connect(pushButtondIxy, SIGNAL( clicked() ), this, SLOT(slotDANbuttonDIxy() ) );
    connect(pushButtonIQx, SIGNAL( clicked() ), this, SLOT(slotDANbuttonIQx() ) );
    connect(pushButtonIQy, SIGNAL( clicked() ), this, SLOT(slotDANbuttonIQy()) );
    connect(pushButtonSigma, SIGNAL( clicked() ), this, SLOT(slotDANbuttonSigma()) );
    connect(pushButtonQxy, SIGNAL( clicked() ), this, SLOT(slotDANbuttonQxy() ) );
    connect(pushButtondQxy, SIGNAL( clicked() ), this, SLOT(slotDANbuttondQxy() ) );
}


//+++
void dan18::slotDANbuttonPolar()    {   danDanMultiButton("I-Polar");   };
void dan18::slotDANbuttonIQ()       {   danDanMultiButton("I-Q");       };
void dan18::slotDANbuttonIxy()      {   danDanMultiButton("I-x-y");     };
void dan18::slotDANbuttonDIxy()     {   danDanMultiButton("dI-x-y");    };
void dan18::slotDANbuttonIQx()      {   danDanMultiButton("I-Qx");      };
void dan18::slotDANbuttonIQy()      {   danDanMultiButton("I-Qy");      };
void dan18::slotDANbuttonSigma()    {   danDanMultiButton("Sigma-x-y"); };
void dan18::slotDANbuttonQxy()      {   danDanMultiButton("Q-x-y");     };
void dan18::slotDANbuttondQxy()     {   danDanMultiButton("dQ-x-y"); };


//*******************************************
//+++ dan
//*******************************************
void dan18::danDanMultiButton(QString button)
{
    int MD = lineEditMD->text().toInt();

    //+++ time steps
    QElapsedTimer dt;
    dt.start();
    qint64 pre_dt = 0;
    
    bool radioButtonOpenInProjectisChecked=radioButtonOpenInProject->isChecked();
    bool checkBoxSortOutputToFoldersisChecked=checkBoxSortOutputToFolders->isChecked();
    bool checkBoxAnisotropyisChecked=checkBoxAnisotropy->isChecked();
    bool radioButtonRadHFisChecked=radioButtonRadHF->isChecked();
    bool radioButtonRadStdisChecked=radioButtonRadStd->isChecked();
    bool radioButtonXYdimQisChecked=radioButtonXYdimQ->isChecked();
    bool checkBoxRewriteOutputisChecked=checkBoxRewriteOutput->isChecked();
    bool checkBoxNameAsTableNameisChecked=checkBoxNameAsTableName->isChecked();
    bool checkBoxMaskNegativeisChecked=checkBoxMaskNegative->isChecked();
    bool checkBoxParallaxTrisChecked=checkBoxParallaxTr->isChecked();
    bool checkBoxParallaxisChecked=checkBoxParallax->isChecked();
    bool checkBoxBCTimeNormalizationisChecked=checkBoxBCTimeNormalization->isChecked();
    bool checkBoxWaTrDetChecked=checkBoxWaTrDet->isChecked();
    
    QString comboBoxModecurrentText=comboBoxMode->currentText();
    

    
    //+++ Output data Suffix
    QString dataSuffix;
    switch (comboBoxMode->currentIndex())
    {
		case 0:
		    dataSuffix="SM";
		    break;
		case 1:
		    dataSuffix="BS";
		    break;
		case 2:
		    dataSuffix="BS-SENS";
		    break;
		case 3:
		    dataSuffix="SM";
		    break;
		case 4:
		    dataSuffix="SM";
		    break;
		default:
		    dataSuffix="SM";
		    break;
    }
    
    //+++ current folder
    Folder *cf;
    
    //+++ skip Tr processing
    bool skipTrProcessing=false;
    
    //+++ Generation of list of Transmission files
    QList<int> listTr;
    
    if (checkBoxSkiptransmisionConfigurations->isChecked())
    {
        skipTrProcessing=true;
        
        int numberTrConfigurations=0;
        
        for (int i=0; i<sliderConfigurations->value(); i++)
        {
            if (tableEC->item(dptECTR, i)->checkState() == Qt::Checked && ! (comboBoxTransmMethod->currentIndex()==4 && tableEC->item(dptWL, i)->text().toDouble()<9.5 ) )
            {
                numberTrConfigurations++;
                listTr<<(i+1);
            }
        }
        
        if (numberTrConfigurations == sliderConfigurations->value() ) skipTrProcessing=false;
    }

    //+++ Subtract Bufffer
    bool subtractBuffer= false;
    if (comboBoxModecurrentText.contains("(BS)")) subtractBuffer=true;
    
    //+++ Subtract Bufffer && Correct to own Sensitivity
    bool bufferAsSens= false;
    if (comboBoxModecurrentText.contains("(BS-SENS)")) bufferAsSens=true;
    
    //+++ update mask and sens lists +++
    updateMaskList();
    updateSensList();
    
    //+++ mask gsl matrix
    gsl_matrix *mask = gsl_matrix_alloc(MD,MD);
    
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    gsl_matrix *sensErr=gsl_matrix_alloc(MD,MD);
    gsl_matrix *sensTrDet;
    if (checkBoxWaTrDetChecked) sensTrDet=gsl_matrix_alloc(MD,MD);
    
    //+++
    gsl_matrix *Sample=gsl_matrix_alloc(MD,MD);
    gsl_matrix *SampleErr=gsl_matrix_alloc(MD,MD);
    
    //+++
    gsl_matrix *Buffer=gsl_matrix_alloc(MD,MD);
    gsl_matrix *BufferErr=gsl_matrix_alloc(MD,MD);
    
    //+++
    gsl_matrix *EC=gsl_matrix_alloc(MD,MD);
    gsl_matrix *ECErr=gsl_matrix_alloc(MD,MD);
    //+++
    gsl_matrix *BC=gsl_matrix_alloc(MD,MD);
    gsl_matrix *BCErr=gsl_matrix_alloc(MD,MD);
    
    //+++ error matrix
    gsl_matrix *ErrMatrix=gsl_matrix_alloc(MD,MD);
    
    
    //+++ script table
    Table *w;
    
    //+++ loops
    int i, xxx, yyy;
    
    //+++ current row in script table
    int iRow;
    
    //+++ current sample run-number and current condition
    int condition;
    QString Nsample;
    
    //+++ result string info for current row
    QString status, statusAll;
    QString maskName, sensName, label;
    maskName="";
    sensName="";
    
    bool theSameMask=false;
    bool theSameSens=false;

    
    //+++ optional parameter if column "Scale" exist
    double scale;
    
    //+++ optional parameter if column "Background" exist
    double BackgroundConst;
    
    //+++ optional parameter if column "VShift" exist
    double VShift;
    
    //+++ optional parameter if column "HShift" exist
    double HShift;
    
    //+++ optional parameter if column "Suffix" exist
    QString Suffix="";
    
    //+++ optional parameter if column "TrDet" exist
    double TrDet=1.0;
    
    //+++ Current sample transmission, tichness, factor & center of beam
    double trans, transBuffer, fractionBuffer, thickness, abs, absBuffer, Xcenter, Ycenter;
    
    //+++ table Name +++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++ check existence of script table w
    if (!app()->checkTableExistence(tableName))
    {
        QMessageBox::critical(0, "qtiSAS", "Table ~"+tableName+"~ does not exist!!! <br><h4>");
        
        //+++ Clean Memory +++
        gsl_matrix_free(Sample);
        gsl_matrix_free(SampleErr);
        gsl_matrix_free(Buffer);
        gsl_matrix_free(BufferErr);
        gsl_matrix_free(EC);
        gsl_matrix_free(ECErr);
        gsl_matrix_free(BC);
        gsl_matrix_free(BCErr);
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(sensErr);
        if (checkBoxWaTrDetChecked) gsl_matrix_free(sensTrDet);
        gsl_matrix_free(ErrMatrix);
        
        return;
    }
    
    //+++ searching active table script
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
    
    
    //+++ again check existence of table
    if (!w)
    {
        QMessageBox::critical(0, "qtiSAS", "Table ~"+tableName+"~ does not exist!!! <br><h4>");
        
        //+++ Clean Memory +++
        gsl_matrix_free(Sample);
        gsl_matrix_free(SampleErr);
        gsl_matrix_free(Buffer);
        gsl_matrix_free(BufferErr);
        gsl_matrix_free(EC);
        gsl_matrix_free(ECErr);
        gsl_matrix_free(BC);
        gsl_matrix_free(BCErr);
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(sensErr);
        if (checkBoxWaTrDetChecked) gsl_matrix_free(sensTrDet);
        gsl_matrix_free(ErrMatrix);
        
        return;
    }
    
    //+++ Indexing
    QStringList scriptColList=w->colNames();
    
    //+++ Check of script-table-structure
    bool scriptOK=true;
    QString scriptIsNotOK="";
    
    
    int indexInfo=scriptColList.indexOf("Run-info");if (indexInfo<0){ scriptOK=false; scriptIsNotOK="Run-info";};             //+++  Run-info  +++
    int indexSample=scriptColList.indexOf("#-Run"); if (indexSample<0){ scriptOK=false; scriptIsNotOK="#-Run";};              //+++  #-Run +++
    int indexCond=scriptColList.indexOf("#-Condition"); if (indexCond<0){ scriptOK=false; scriptIsNotOK="#-Condition";};      //+++  #-Condition +++
    int indexC=scriptColList.indexOf("C"); if (indexC<0){ scriptOK=false; scriptIsNotOK="C";};                                //+++  C +++
    int indexD=scriptColList.indexOf("D"); if (indexD<0){ scriptOK=false; scriptIsNotOK="D";};                                //+++  D +++
    int indexLam=scriptColList.indexOf("Lambda"); if (indexLam<0){ scriptOK=false; scriptIsNotOK="Lambda";};                  //+++  Lambda +++
    int indexCA=scriptColList.indexOf("Beam Size"); if (indexCA<0){ scriptOK=false; scriptIsNotOK="Beam Size";};              //+++  Beam Size +++
    int indexBC=scriptColList.indexOf("#-BC"); if (indexBC<0){ scriptOK=false; scriptIsNotOK="#-BC";};                        //+++  #-BC +++
    
    int indexEC=scriptColList.indexOf("#-EC [EB]");
    if (indexEC<0){ indexEC=scriptColList.indexOf("#-EC"); if (indexEC<0) {scriptOK=false; scriptIsNotOK="#-EC";}};                       //+++  #-EC [EB] +++
    int indexBuffer=scriptColList.indexOf("#-Buffer"); if (subtractBuffer && indexBuffer<0){ scriptOK=false; scriptIsNotOK="#-Buffer";};  //+++  #-Buffer +++
    int indexThickness=scriptColList.indexOf("Thickness"); if (indexThickness<0){ scriptOK=false; scriptIsNotOK="Thickness";};            //+++  Thickness+++
    
    int indexTr=scriptColList.indexOf("Transmission-Sample");
    if (indexTr<0){ indexTr=scriptColList.indexOf("Transmission"); if (indexTr<0) {scriptOK=false; scriptIsNotOK="Transmission-Sample";}};        //+++  Transmission-Sample +++
    int indexTrBuffer=scriptColList.indexOf("Transmission-Buffer");
    if (subtractBuffer && indexTrBuffer<0){ scriptOK=false; scriptIsNotOK="Transmission-Buffer";};                                                  //+++  Transmission-Buffer +++
    int indexBufferFraction=scriptColList.indexOf("Buffer-Fraction");
    if (subtractBuffer && indexBufferFraction<0){ scriptOK=false; scriptIsNotOK="Buffer-Fraction";};                                                //+++  Buffer-Fraction +++
    
    
    int indexFactor=scriptColList.indexOf("Factor"); if (indexFactor<0){ scriptOK=false; scriptIsNotOK="Factor";};            //+++  Factor +++
    int indexXC=scriptColList.indexOf("X-center"); if (indexXC<0){ scriptOK=false; scriptIsNotOK="X-center";};                //+++  X-center +++
    int indexYC=scriptColList.indexOf("Y-center"); if (indexYC<0){ scriptOK=false; scriptIsNotOK="Y-center";};                //+++  Y-center +++
    int indexMask=scriptColList.indexOf("Mask"); if (indexMask<0){ scriptOK=false; scriptIsNotOK="Mask";};                    //+++  Mask +++
    int indexSens=scriptColList.indexOf("Sens"); if (indexSens<0){ scriptOK=false; scriptIsNotOK="Sens";};                    //+++  Sens +++
    int indexStatus=scriptColList.indexOf("Status"); if (indexStatus<0){ scriptOK=false; scriptIsNotOK="Status";};              //+++  Status +++

    if (!scriptOK)
    {
        QMessageBox::critical(0, "qtiSAS", "Table ~"+tableName+" has wrong format [" + scriptIsNotOK + "]. <br> Check table or  generate new one.<h4>");
        
        //+++ Clean Memory +++
        gsl_matrix_free(Sample);
        gsl_matrix_free(SampleErr);
        gsl_matrix_free(Buffer);
        gsl_matrix_free(BufferErr);
        gsl_matrix_free(EC);
        gsl_matrix_free(ECErr);
        gsl_matrix_free(BC);
        gsl_matrix_free(BCErr);
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(sensErr);
        if (checkBoxWaTrDetChecked) gsl_matrix_free(sensTrDet);
        gsl_matrix_free(ErrMatrix);
        
        return;
    }
    
    //--- Check of script-table-structure
    
    //+++ Scale +++ Hand-made column
    int indexScale=scriptColList.indexOf("Scale");
    
    //+++ BackgroundConst +++ Hand-made column
    int indexBackgroundConst=scriptColList.indexOf("Background");
    
    //+++ VShift +++ Hand-made column
    int indexVShift=scriptColList.indexOf("VShift");
    
    //+++ HShift +++ Hand-made column
    int indexHShift=scriptColList.indexOf("HShift");

    //+++ Suffix +++ Hand-made column
    int indexSuffix=scriptColList.indexOf("Suffix");
    
    //+++ Use sensitivity Local +++
    int indexUseSensBufferLocal=scriptColList.indexOf("Use-Buffer-as-Sensitivity");

    //+++ TrDet +++ Hand-made column
    int indexTrDet=scriptColList.indexOf("TrDet");
    if (indexTrDet<0) checkBoxWaTrDetChecked=false;
    
    //+++ get number of rows
    int Nrows=w->numRows();
    
    //+++ last raw
    int last=Nrows;
    

    QElapsedTimer time;
    time.start();
    
    int progressUpdateSteps=1;
    double timeLeft;
    
    //+++ Mask & Sens +++
    QStringList listMask, listSens;
    QString winLabelMask="DAN::Mask::"+QString::number(MD);
    QString winLabelSens="DAN::Sensitivity::"+QString::number(MD);
    
    findMatrixListByLabel(winLabelMask, listMask);
    if (!listMask.contains("mask")) listMask.prepend("mask");
    findMatrixListByLabel(winLabelSens, listSens);
    if (!listSens.contains("sens")) listSens.prepend("sens");
    
    //+++ tamplate to merge datasets
    QStringList mergedTemplate;
    
    // +++ iRow corresponds to the number of row
    //+++ row by row data reduction
    //+++ main loop
    
    bool checkSelection=false;
    
    int firstLine=0;
    int lastLine=last;
    int selectedLines=0;
    for (iRow=0; iRow<last; iRow++)
    {
        if (w->isRowSelected(iRow,true))
        {
            selectedLines++;
            if(!checkSelection) firstLine=iRow;
            checkSelection=true;
            //break;
        }
    }
    if(selectedLines>0) lastLine=firstLine+selectedLines;

    //+++ Progress Dialog +++
    QProgressDialog progress;
    progress.setMinimumDuration(10);
    progress.setWindowModality(Qt::WindowModal);
    progress.setRange(0,lastLine-firstLine);
    progress.setCancelButtonText("Abort Data Processing");
    progress.setMinimumWidth(400);
    progress.setMaximumWidth(4000);
    progress.setLabelText("Data Processing Started...");
    progress.show();
    
    app()->workspace()->blockSignals ( true );//+++ test 2019
    
    //+++
    if (radioButtonOpenInProjectisChecked && checkBoxSortOutputToFoldersisChecked)
    {
        cf = ( Folder * ) app()->current_folder;
        
        if (button=="I-x-y")	app()->changeFolder("DAN :: I [x,y]");
        else if (button=="dI-x-y")	app()->changeFolder("DAN :: dI [x,y]");
        else if (button=="I-Q")	    app()->changeFolder("DAN :: I [Q]");
        else if (button=="I-Qx")		app()->changeFolder("DAN :: I [Qx]");
        else if (button=="I-Qy")		app()->changeFolder("DAN :: I [Qy]");
        else if (button=="I-Polar")	app()->changeFolder("DAN :: I [Q,phi]");
        else if (button=="Sigma-x-y")app()->changeFolder("DAN :: Sigma [x,y]");
        else if (button=="Q-x-y")	app()->changeFolder("DAN :: Q [x,y]");
        else if (button=="dQ-x-y")	app()->changeFolder("DAN :: dQ [x,y]");
    }

    QStringList lst;
    double sigmaTrans, sigmaTransBuffer;

    
    printf("\nDAN|START file-to-file data reduction:\n");
    pre_dt = dt.elapsed();
    
    //+++ START file-to-file data reduction
    for (iRow=firstLine; iRow<lastLine; iRow++)
    {
        //+++ check number  of condition
        status=">>>  no conditions  "; // status for each file
        
        //+++
        condition=w->text(iRow,indexCond).toInt();
        
        //+++ goto next row
        if(condition<0)
        {
            mergedTemplate<<"-0-";
            
            //+++ Set Status +++
            w->setText(iRow,indexStatus,status);
            
            //+++ Progress +++
            if ( progress.wasCanceled() ) break;
            continue;
        }
        
        if(skipTrProcessing && listTr.indexOf(condition)>=0) {mergedTemplate<<"-0-"; continue;}
        
        
        //+++ goto next row
        if (condition==0)
        {
            mergedTemplate<<"-0-";
            
            //+++ Set Status +++
            w->setText(iRow,indexStatus,status);
            
            
            //+++ Progress +++
            if ( progress.wasCanceled() ) break;
            
            continue;
        }
        
        scale=1.0;
        if (indexScale>0)
        {
            scale=w->text(iRow,indexScale).toDouble();
            if (scale<=0.0) scale=1.0;
        }
        
        BackgroundConst=0.0;
        if (indexBackgroundConst>0)
        {
            BackgroundConst=w->text(iRow,indexBackgroundConst).toDouble();
        }
        
        VShift=0.0;
        if (indexVShift>0)
        {
            VShift=w->text(iRow,indexVShift).toDouble();
        }
        
        HShift=0.0;
        if (indexHShift>0)
        {
            HShift=w->text(iRow,indexHShift).toDouble();
        }

        Suffix="";
        if (indexSuffix>0)
        {
            Suffix=w->text(iRow,indexSuffix).remove(" ");
        }
        
        TrDet=1.0;
        if (indexTrDet>0)
        {
            TrDet=w->text(iRow,indexTrDet).toDouble();
            if (TrDet>1.0) TrDet=1.0;
            else if (TrDet<0) TrDet=0.0;
        }
        
        //++++ MASK+  >>>>
        if (maskName!=w->text(iRow,indexMask) || sensName!=w->text(iRow,indexSens) )
        {
            maskName=w->text(iRow,indexMask);
            if ( !listMask.contains(maskName) )
            {
                maskName=comboBoxMaskFor->currentText();
                w->setText(iRow,indexMask,maskName);
            }
            gsl_matrix_set_zero (mask);
            make_GSL_Matrix_Symmetric(maskName, mask, MD);
            
            
            //+++ SENS+  >>>>
            sensName=w->text(iRow,indexSens);
            if ( !listSens.contains(sensName) )
            {
                sensName=comboBoxSensFor->currentText();
                w->setText(iRow,indexSens,sensName);
            }
            gsl_matrix_set_zero (sens);
            make_GSL_Matrix_Symmetric(sensName, sens, MD);
            
            //+++
            if (subtractBuffer && bufferAsSens)
            {
                if (indexUseSensBufferLocal<0 || w->text(iRow, indexUseSensBufferLocal)=="yes")
                {
                }
                else 	sensAndMaskSynchro(mask, sens, MD);
            }
            else sensAndMaskSynchro(mask, sens, MD );
            
            //+++ I-Qx +  >>>>
            if (button=="I-Qx")
            {
                bool slicesBS=checkBoxSlicesBS->isChecked();
                int from=spinBoxFrom->value();
                if(slicesBS && spinBoxLTyBS->value()>from) from=spinBoxLTyBS->value();
                int to=spinBoxTo->value();
                if(slicesBS && spinBoxRByBS->value()<to) to=spinBoxRByBS->value();
                if (from <=to && from>0 && to<=MD)
                {
                    
                    for (xxx=0; xxx<MD; xxx++ )
                        for (int yyy=0; yyy<(from-1); yyy++ ) gsl_matrix_set(mask,yyy,xxx, 0.0 );
                    for (int xxx=0; xxx<MD; xxx++ )
                        for (yyy=to; yyy<MD; yyy++ ) gsl_matrix_set(mask,yyy,xxx, 0.0 );
                }
            }
            //--- I-Qx - >>>>
            
            //+++ I-Qy + >>>>
            if (button=="I-Qy")
            {
                bool slicesBS=checkBoxSlicesBS->isChecked();
                int from=spinBoxFrom->value();
                if(slicesBS && spinBoxLTxBS->value()>from) from=spinBoxLTxBS->value();
                int to=spinBoxTo->value();
                if(slicesBS && spinBoxRBxBS->value()<to) to=spinBoxRBxBS->value();
                if (from <=to && from>0 && to<=MD)
                {
                    for (yyy=0; yyy<MD; yyy++ )
                        for (xxx=0; xxx<(from-1); xxx++ ) gsl_matrix_set(mask,yyy,xxx, 0.0 );
                    for (int yyy=0; yyy<MD; yyy++ )
                        for (xxx=to; xxx<MD; xxx++ ) gsl_matrix_set(mask,yyy,xxx, 0.0 );
                }
            }
            //---- I-Qy -  >>>>
            
            
            //+++ Sensetivty Error Matrix
            QString sensFile=getSensitivityNumber(sensName);
            
            gsl_matrix_set_zero (sensErr);
            if (sensFile != "" && filesManager->checkFileNumber(sensFile))
                readErrorMatrix(sensFile, sensErr);
        }
        label=w->text(iRow,indexInfo);
        if (Suffix!="") label=Suffix+"-"+label;
        Nsample=w->text(iRow,indexSample);
        
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //+++ All actions only in case Sample Exists                   !!!!!!!!!!!!!!!!
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (!filesManager->checkFileNumber(Nsample))
        {
            //+++ if file is skipted +++
            if (Nsample.toInt() !=0) w->setText(i,indexSample,w->text(i,indexSample)+"  >>>  not exist!!!");
            status=">>> file "+Nsample+" not found; output:: no ";
            
            //+++ Set Status +++
            w->setText(iRow,indexStatus,status);
            
            //+++ Progress +++
            if ( progress.wasCanceled() ) break;
            continue;
        }
        
        //+++
        gsl_matrix_set_zero (Sample);
        gsl_matrix_set_zero (SampleErr);
        
        gsl_matrix_set_zero (ErrMatrix);
        
        //+++
        gsl_matrix_set_zero (Buffer);
        gsl_matrix_set_zero (BufferErr);
        
        //+++
        gsl_matrix_set_zero (EC);
        gsl_matrix_set_zero (ECErr);
        //
        gsl_matrix_set_zero (BC);
        gsl_matrix_set_zero (BCErr);
        
        //+++
        readMatrixCor( Nsample, Sample);
        readErrorMatrix( Nsample, SampleErr);  // (dN/N)^2
        
        gslMatrixShift(Sample, MD, HShift, VShift );
        gslMatrixShift(SampleErr, MD, HShift, VShift );
        
        
        //+++
        status=">>>  sample#="+Nsample+": OK  ";
        
        
        QString Nbuffer;
        
        if (subtractBuffer)
        {
            
            Nbuffer=w->text( iRow, indexBuffer );
            
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //+++ All actions only in case Sample Exists  !!!!!!!!!!!!!!!!!
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            if (!filesManager->checkFileNumber(Nbuffer))
            {
                //+++ if file is skipted +++
                if ( Nbuffer.toInt() !=0 ) w->setText(i,indexBuffer, Nbuffer + "  >>>  not exist!!!");
                status=">>> file "+Nbuffer+"BUFFER not found; output:: no ";
                
                //+++ Set Status +++
                w->setText(iRow,indexStatus,status);
                
                //+++ Progress +++
                if ( progress.wasCanceled() ) break;
                continue;
            }
            //+++
            readMatrixCor( Nbuffer, Buffer);
            readErrorMatrix( Nbuffer, BufferErr);
            
            
        }
        
        //+++ If BC exist but EC not !!! +++

        //+++ EC +++
        QString NEC=w->text(iRow,indexEC);
        bool ECexist=false;
        if (filesManager->checkFileNumber(NEC))
        {
            readMatrixCor( NEC,EC);
            readErrorMatrix( NEC, ECErr);
            status+=">>>  EC#=" + NEC + ": OK  ";
            //+++
            ECexist=true;
        }
        else status+=">>>  EC: no correction  ";
        
        
        //+++ BC +++
        QString NBC=w->text(iRow,indexBC);

        if (filesManager->checkFileNumber(NBC))
        {
            // read BC matrix 2012
            if (checkBoxBCTimeNormalizationisChecked)
            {
                readMatrixCorTimeNormalizationOnly( NBC, BC );
                
                //Normalization constant
                double TimeSample=spinBoxNorm->value();
                double ttime = monitors->readDuration(Nsample);
                if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;

                double NormSample = monitors->normalizationFactor(Nsample);

                if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
                
                gsl_matrix_scale(BC,NormSample);      // EB=T*EB
            }
            else readMatrixCor( NBC, BC );
            
            readErrorMatrix( NBC, BCErr);
            status+=">>>  BC#=" + NBC + ": OK  ";
        }
        else status+=">>>  BC: no correction  ";
        
        //+++ transmission check
        lst.clear();
        lst = w->text(iRow, indexTr)
                  .remove(" ")
                  .remove(QChar(177))
                  .remove("\t")
                  .remove("]")
                  .split("[", Qt::SkipEmptyParts);
        
        trans=lst[0].toDouble();
        sigmaTrans=0;
        if (lst.count()>1) sigmaTrans=lst[1].toDouble();

        if(trans!=0) sigmaTrans=fabs(sigmaTrans/trans);
        
        if (trans<=0.0 || trans>2.0)
        {
            w->setText(iRow,indexTr,"Check!!!");
            QMessageBox::warning(this,tr("qtiSAS"), tr(QString("Line # "+QString::number(iRow+1)+": check transmission!").toLocal8Bit().constData()));
            
            //+++ Progress +++
            if ( progress.wasCanceled() ) break;
            continue;
            
        }
        else status=status+">>>  Transmission="+QString::number(trans,'f',4)+"  ";
        
        
        //+++ transmission-Buffer check
        transBuffer=1.0;
        if (indexTrBuffer>=0)
        {
            if (subtractBuffer)
            {
                //+++ transmission check
                lst.clear();
                lst = w->text(iRow, indexTrBuffer)
                          .remove(" ")
                          .remove(QChar(177))
                          .remove("\t")
                          .remove("]")
                          .split("[", Qt::SkipEmptyParts);
                
                transBuffer=lst[0].toDouble();
                sigmaTransBuffer=0;
                if (lst.count()>1) sigmaTransBuffer=lst[1].toDouble();

                if(transBuffer!=0) sigmaTransBuffer=fabs(sigmaTransBuffer/transBuffer);
                
                
                if (transBuffer<=0.0 || transBuffer>2.0)
                {
                    w->setText(iRow,indexTrBuffer,"Check!!!");
                    QMessageBox::warning(this,tr("qtiSAS"), tr(QString("Line # "+QString::number(iRow+1)+": check Buffer!").toLocal8Bit().constData()));
                    //+++ Progress +++
                    if ( progress.wasCanceled() ) break;
                    continue;
                }
            }
        }
        
        //+++ fraction buffer
        fractionBuffer=0.0;
        if (indexBufferFraction>=0)
        {
            if (subtractBuffer)
            {
                //+++ transmission check
                fractionBuffer=w->text(iRow,indexBufferFraction).toDouble();
                if (fractionBuffer<0.0)
                {
                    toResLog("DAN :: Fraction of Buffer is negative :: "+QString::number(iRow+1)+"\n");
                }
            }
        }
        
        //+++ thickness check
        thickness=w->text(iRow,indexThickness).toDouble();
        if (thickness<=0.0 || thickness>100.0)
        {
            thickness=0.1;
            w->setText(iRow,indexThickness,"Check!!!");
            QMessageBox::warning(this,tr("qtiSAS"), tr(QString("Line # "+QString::number(iRow+1)+": check Thickness!").toLocal8Bit().constData()));
            //+++ Progress +++
            if ( progress.wasCanceled() ) break;
            continue;
        }
        else status=status+">>>  Thickness="+QString::number(thickness,'f',3)+"  ";
        
        //+++ absolute factor check
        double abs0=scale*w->text(iRow,indexFactor).toDouble();
        if(thickness!=0) abs0/=thickness;
        
        if (abs0<=0)
        {
            w->setText(iRow,indexFactor,"check!!!");
            QMessageBox::warning(this,tr("qtiSAS"), tr(QString("Line # "+QString::number(iRow+1)+": check Abs.Factor!").toLocal8Bit().constData()));
            //+++ Progress +++
            if ( progress.wasCanceled() ) break;
            continue;
        }
        else status=status+">>>  Abs.Factor="+QString::number(abs0,'e',4)+"  ";
        
        //+++
        if (trans!=0.0) abs=abs0/trans;
        if (transBuffer!=0.0) absBuffer=abs0/transBuffer;
        
        //+++ X-center check
        Xcenter=w->text(iRow,indexXC).toDouble();
        status=status+">>>  X-center="+QString::number(Xcenter,'e',4)+"  "; // 2013-09-18
        
        Xcenter-=1;
        
        Ycenter=w->text(iRow,indexYC).toDouble();
        
        //+++ Y-center check
        status=status+">>>  Y-center="+QString::number(Ycenter,'e',4)+"  "; // 2013-09-18
        Ycenter-=1;
        
        //+++ Sensitivity Error
        gsl_matrix_add(ErrMatrix, sensErr);
        
        // +++  Reading of some parameters
        Nsample=w->text(iRow,indexSample);
        
        double detdist  = 100.0*w->text(iRow,indexD).toDouble();  //sample-to-detector distance
        double C = 100.0*w->text(iRow,indexC).toDouble();
        double lambda = w->text(iRow,indexLam).toDouble();
        double deltaLambda = selector->readDeltaLambda(Nsample);
        
        double pixel  = lineEditResoPixelSize->text().toDouble();
        double binning=comboBoxBinning->currentText().toDouble();
        
        double pixelAsymetry  = lineEditAsymetry->text().toDouble();
        if (pixelAsymetry<=0) pixelAsymetry=1.0;
        
        double r2 = collimation->readR2(Nsample);
        double r1 = collimation->readR1(Nsample);

        double detRotationX = detector->readDetRotationX(Nsample);
        double detRotationY = detector->readDetRotationY(Nsample);

        if (checkBoxWaTrDetChecked)
        {
            gslMatrixWaTrDet (MD, TrDet, sensTrDet, Xcenter, Ycenter, detdist, pixel*binning, pixelAsymetry );
        }
        
        int iii,jjj;
        double err2, Isample, Ibc, Iec, Ibuffer, IecBuffer, IbcBuffer, ItransSample, ItransBuffer ;
        
        double scaleBufferData=0;
        if (subtractBuffer) scaleBufferData=fractionBuffer*absBuffer/abs;
        
        for (iii=0;iii<MD;iii++) for (jjj=0;jjj<MD;jjj++)
        {
            //+++
            Isample=gsl_matrix_get(Sample,iii,jjj);
            err2=Isample;       //+++
            Isample*=Isample;
            Isample*=gsl_matrix_get(SampleErr,iii,jjj);
            //+++
            Iec=gsl_matrix_get(EC,iii,jjj);
            err2 -= (trans*Iec);        //+++
            Iec*=Iec;
            Iec=Iec*trans*trans;
            Iec*=gsl_matrix_get(ECErr,iii,jjj);
            //+++
            Ibc=gsl_matrix_get(BC,iii,jjj);
            err2 -= ( (1.0-trans) * Ibc );  //+++
            Ibc*=Ibc;
            Ibc*=(1.0-trans)*(1.0-trans);
            Ibc*=gsl_matrix_get(BCErr,iii,jjj);
            
            //+++  2019 : error of transmission
            ItransSample=gsl_matrix_get(Sample,iii,jjj)-gsl_matrix_get(BC,iii,jjj);
            ItransSample*=ItransSample;
            ItransSample=ItransSample*sigmaTrans*sigmaTrans;
            
            if (subtractBuffer)
            {
                //+++2019
                Ibuffer=scaleBufferData*gsl_matrix_get(Buffer,iii,jjj);
                err2-= Ibuffer;
                Ibuffer*=Ibuffer;
                Ibuffer*=gsl_matrix_get(BufferErr,iii,jjj);
                //+++2019
                IecBuffer=transBuffer*scaleBufferData*gsl_matrix_get(EC,iii,jjj);
                err2 += IecBuffer;
                IecBuffer*=IecBuffer;
                IecBuffer*=gsl_matrix_get(ECErr,iii,jjj);
                //+++2019
                IbcBuffer=scaleBufferData*(1.0-transBuffer)*gsl_matrix_get(BC,iii,jjj);
                err2 += IbcBuffer;
                IbcBuffer*=IbcBuffer;
                IbcBuffer*=gsl_matrix_get(BCErr,iii,jjj);
                //+++  2019 : error of transmission
                ItransBuffer=scaleBufferData*(gsl_matrix_get(Buffer,iii,jjj)-gsl_matrix_get(BC,iii,jjj));
                ItransBuffer*=ItransBuffer;
                ItransBuffer=ItransBuffer*sigmaTransBuffer*sigmaTransBuffer;
            }
            /*
            if (iii==50 && jjj==50)
            {
                std::cout<<"sigmaTrans="<<sigmaTrans<<"\n";
                std::cout<<"I="<<err2<<"\n";
                std::cout<<"dIsample="<<sqrt(Isample)/err2<<"\n";
                std::cout<<"dIec="<<sqrt(Iec)/err2<<"\n";
                std::cout<<"dIbc="<<sqrt(Ibc)/err2<<"\n";
                std::cout<<"dItransSample="<<sqrt(ItransSample)/err2<<"\n";
                std::cout<<"sigmaTransBuffer="<<sigmaTransBuffer<<"\n";
                std::cout<<"dIBuffer="<<sqrt(Ibuffer)/err2<<"\n";
                std::cout<<"dIecBuffer="<<sqrt(IecBuffer)/err2<<"\n";
                std::cout<<"dIbcBuffer="<<sqrt(IbcBuffer)/err2<<"\n";
                std::cout<<"dItransBuffer="<<sqrt(ItransBuffer)/err2<<"\n";
            }
            */
            
            if ( err2 != 0.0 ) err2=1.0/err2; else err2=0.0;
            err2=err2*err2;
            
            
            if (subtractBuffer) err2*=(Isample+Iec+Ibc+ItransSample+Ibuffer+IecBuffer+IbcBuffer+ItransBuffer);
            else err2*=(Isample+Iec+Ibc+ItransSample);

            
            gsl_matrix_set(ErrMatrix, iii, jjj, sqrt(
                                                     gsl_matrix_get(ErrMatrix,iii,jjj) + err2 ) ); // sigma(relative)
            gsl_matrix_set(SampleErr, iii, jjj,
                           gsl_matrix_get(ErrMatrix,iii,jjj) * gsl_matrix_get(ErrMatrix,iii,jjj)
                           );//sigma^2
        }
        
        
        gsl_matrix_mul_elements(ErrMatrix,mask);
        gsl_matrix_mul_elements(SampleErr,mask);
        
        
        //+++ TODO :: Error of BUFFER
        
        if (checkBoxWaTrDetChecked) gsl_matrix_mul_elements(Sample,sensTrDet);
        
        //+++ Matrix-to-Matrix actions +++
        gsl_matrix_sub(Sample,BC);                      // Sample=Sample - BC
        
        // 2012 Time normalization BC
        if (filesManager->checkFileNumber(NBC) && checkBoxBCTimeNormalizationisChecked && ECexist)
        {
            readMatrixCorTimeNormalizationOnly( NBC, BC );
            
            //Normalization constant
            double TimeSample=spinBoxNorm->value();
            double ttime = monitors->readDuration(NEC);
            if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;

            double NormSample = monitors->normalizationFactor(NEC);

            if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
            
            gsl_matrix_scale(BC,NormSample);      // EB=T*EB
        }
        
        
        if (checkBoxWaTrDetChecked) gsl_matrix_mul_elements(EC,sensTrDet);
        gsl_matrix_sub(EC,BC);                          // EC=EC - BC
        
        
        if (subtractBuffer)
        {
            if (filesManager->checkFileNumber(NBC) && checkBoxBCTimeNormalizationisChecked && ECexist)
            {
                readMatrixCorTimeNormalizationOnly( NBC, BC );
                
                //Normalization constant
                double TimeSample=spinBoxNorm->value();
                double ttime = monitors->readDuration(Nbuffer);
                if (ttime > 0.0)
                    TimeSample /= ttime;
                else
                    TimeSample = 0.0;

                double NormSample = monitors->normalizationFactor(Nbuffer);

                if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
                
                gsl_matrix_scale(BC,NormSample);      // EB=T*EB
            }
            if (checkBoxWaTrDetChecked) gsl_matrix_mul_elements(Buffer,sensTrDet);
            gsl_matrix_sub(Buffer,BC);    //Buffer=Buffer - BC
        }
        
        double Xc, Yc;
        readCenterfromMaskName( maskName, Xc, Yc, MD );
        
        
        if (trans<1.0 && trans>0.0 && checkBoxParallaxTrisChecked)
        {
            double Detector = detector->readD(w->text(iRow, indexSample)); // [cm]
            transmissionThetaDependenceTrEC(EC, Xc, Yc, Detector, trans);
        }
        
        gsl_matrix_scale(EC,trans);                             // EC=T*EC
        if (ECexist) gsl_matrix_sub(Sample,EC);         // Sample=Sample  - EC
        
        if (subtractBuffer)
        {
            gsl_matrix_scale(EC,transBuffer/trans);    // EC=Tbuffer*EC
            if (ECexist) gsl_matrix_sub(Buffer,EC);    // Buffer=Buffer  - EC
        }
        
        gsl_matrix_mul_elements(Sample,mask);
        if (subtractBuffer) gsl_matrix_mul_elements(Buffer,mask);
        
        //+++ Paralax Correction
        if (checkBoxParallaxisChecked || checkBoxParallaxTrisChecked)
        {
            double Detector = detector->readD(w->text(iRow, indexSample)); // [cm]

            parallaxCorrection(Sample, Xc, Yc, Detector, trans);
            
            if (subtractBuffer)
            {
                double Detector = detector->readD(w->text(iRow, indexBuffer)); // [cm]
                parallaxCorrection(Buffer, Xc, Yc, Detector, transBuffer);
                
            }
        }
        
        if (subtractBuffer)
        {
            gsl_matrix_scale(Buffer,fractionBuffer*absBuffer/abs);
            gsl_matrix_sub(Sample,Buffer);
        }

        if (comboBoxACmethod->currentIndex() == 3)
        {
            double normalization = monitors->normalizationFactor(w->text(iRow, indexSample));
            if (normalization > 0)
                gsl_matrix_scale(Sample, 1 / normalization);
        }
        else
        {
            gsl_matrix_scale(Sample, abs);
        }

        //+++ Sensitivity correction
        if (subtractBuffer && bufferAsSens)
        {
            if (indexUseSensBufferLocal<0 || w->text(iRow, indexUseSensBufferLocal)=="yes")
            {
                double Isample, Ibuffer;
                int numberPixels=0;
                double summBufferSens=0;
                
                
                for (iii=0;iii<MD;iii++) for (jjj=0;jjj<MD;jjj++)
                {
                    if (gsl_matrix_get(mask,iii,jjj)>0)
                    {
                        numberPixels++;
                        Isample= gsl_matrix_get(Sample,iii,jjj);
                        Ibuffer= gsl_matrix_get(Buffer,iii,jjj);
                        if (Ibuffer>0)
                        {
                            Isample/=Ibuffer;
                        }
                        summBufferSens+=Ibuffer*gsl_matrix_get(sens,iii,jjj);
                        gsl_matrix_set(Sample,iii,jjj,Isample);
                    }
                }
                gsl_matrix_scale(Sample,summBufferSens/numberPixels);
            }
            else gsl_matrix_mul_elements(Sample,sens);
        }
        else gsl_matrix_mul_elements(Sample,sens);
        
        //+++ subtract const bgd
        if (indexBackgroundConst>0)
        {
            for (iii=0;iii<MD;iii++) for (jjj=0;jjj<MD;jjj++)
            {
                if (gsl_matrix_get(mask,iii,jjj)>0) gsl_matrix_set(Sample,iii,jjj,gsl_matrix_get(Sample,iii,jjj)-BackgroundConst);
            }
        }
        
        //+++ Masking of  Negative Points
        if (checkBoxMaskNegativeisChecked)
        {
            for (iii=0;iii<MD;iii++) for (jjj=0;jjj<MD;jjj++)
            {
                if (gsl_matrix_get(Sample,iii,jjj)<=0)
                {
                    gsl_matrix_set(Sample,iii,jjj,0.0);
                    gsl_matrix_set(mask,iii,jjj,0.0);
                    gsl_matrix_set(sens,iii,jjj,0.0);
                    
                }
            }
        }

        
        //+++2019 ErrMatrix -> dI (before sigma)
        double errTmp, samTmp;
        for (iii=0;iii<MD;iii++) for (jjj=0;jjj<MD;jjj++)
        {
            errTmp=gsl_matrix_get(ErrMatrix,iii,jjj);
            samTmp=gsl_matrix_get(Sample,iii,jjj);
            gsl_matrix_set(ErrMatrix,iii,jjj,errTmp*fabs(samTmp));
        }
        
        // 2017 ...
        matrixConvolusion(Sample,mask,MD);
        
        QString nameQI = w->text(iRow,indexSample).simplified();  //  file number or name as name
        if(lineEditWildCard->text().contains("#")) nameQI=nameQI+"-"+w->text(iRow,indexInfo).simplified();  // plus info
        
        if (checkBoxNameAsTableNameisChecked)
        {
            nameQI=w->text(iRow,indexInfo);  // label as name
            if(lineEditWildCard->text().contains("#")) nameQI=nameQI+"-"+Nsample;  // label as name
            else nameQI+="-c"+w->text(iRow,indexCond);
            
        }
        
        //nameQI=nameQI.replace("_", "-");
        nameQI=nameQI.simplified();
        nameQI=nameQI.replace(" ", "-").replace("/", "-").replace("_", "-").replace(",", "-").replace(".", "-").remove("%");
        
        nameQI=dataSuffix+"-"+nameQI;
        
        //+++ Ext
        QString currentExt=lineEditFileExt->text().remove(" ");
        if(currentExt!="") currentExt+="-";
        
        
        //+++   Open Reduced Matrix in Project
        if (radioButtonOpenInProjectisChecked && button=="I-x-y")
        {
            QString matrixOutName="I-"+currentExt+nameQI;
            if (!checkBoxRewriteOutputisChecked)
            {
                matrixOutName+="-v-";
                matrixOutName=app()->generateUniqueName(matrixOutName);
            }
            
            if (radioButtonXYdimQisChecked)
            {
                if(detdist!=0)
                {
                    double xs=4.0*M_PI/lambda*sin(0.5*atan((1-(Xcenter+1))*pixel*binning/detdist));
                    double xe=4.0*M_PI/lambda*sin(0.5*atan((MD-(Xcenter+1))*pixel*binning/detdist));
                    double ys=4.0*M_PI/lambda*sin(0.5*atan((1-(Ycenter+1))*pixel*binning*pixelAsymetry/detdist));
                    double ye=4.0*M_PI/lambda*sin(0.5*atan((MD-(Ycenter+1))*pixelAsymetry*pixel*binning/detdist));

                    makeMatrixSymmetric(Sample,matrixOutName, label, MD, xs, xe, ys,ye, true);
                }
                else makeMatrixSymmetric(Sample,matrixOutName, label, MD,true);
                
            }
            else makeMatrixSymmetric(Sample,matrixOutName, label, MD, true);
        }
        
        //+++ Open Error Matrix in Project
        if (radioButtonOpenInProjectisChecked && button=="dI-x-y")makeMatrixSymmetric(ErrMatrix,"dI-"+currentExt+nameQI,label, MD, true);
            
        //+++   Save Reduced Matrix to File
        if (!radioButtonOpenInProjectisChecked && button=="I-x-y")
        {
            if (checkBoxSortOutputToFoldersisChecked)
            {
                QDir dd; if (!dd.cd(lineEditPathRAD->text()+"/ASCII-I")) dd.mkdir(lineEditPathRAD->text()+"/ASCII-I");
                if (comboBoxIxyFormat->currentText().contains("Matrix")) saveMatrixToFile(lineEditPathRAD->text()+"/ASCII-I/I-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT",Sample, MD);
                else
                {
                    gsl_matrix *sigmaMa=gsl_matrix_alloc(MD,MD);;//+++2019
                    sigmaGslMatrix(sigmaMa,MD,mask,Xcenter,Ycenter,lambda,deltaLambda,C,detdist,pixel*binning,r1,r2);
                    saveMatrixAsTableToFile(lineEditPathRAD->text()+"/ASCII-I/I-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT", Sample,ErrMatrix,sigmaMa,mask,MD,Xcenter,Ycenter,lambda,deltaLambda,detdist,pixel*binning,pixel*binning*pixelAsymetry );
                    
                    gsl_matrix_free(sigmaMa); //+++2019
                }
            }
            else 
            {
                if (comboBoxIxyFormat->currentText().contains("Matrix")) saveMatrixToFile(lineEditPathRAD->text()+"/I-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT",Sample, MD);
                else
                {
                    gsl_matrix *sigmaMa=gsl_matrix_alloc(MD,MD);;//+++2019
                    sigmaGslMatrix(sigmaMa,MD,mask,Xcenter,Ycenter,lambda,deltaLambda,C,detdist,pixel*binning,r1,r2);
                    saveMatrixAsTableToFile(lineEditPathRAD->text()+"/I-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT", Sample,ErrMatrix,sigmaMa,mask,MD,Xcenter,Ycenter,lambda,deltaLambda, detdist, pixel*binning, pixel*binning*pixelAsymetry );
                    gsl_matrix_free(sigmaMa); //+++2019
                }
            }
            
        }	
        
        //+++   Save Error Matrix to File				
        if (!radioButtonOpenInProjectisChecked && button=="dI-x-y")
        {
            if (checkBoxSortOutputToFoldersisChecked)
            {	QDir dd; if (!dd.cd(lineEditPathRAD->text()+"/ASCII-dI")) dd.mkdir(lineEditPathRAD->text()+"/ASCII-dI");
                saveMatrixToFile(lineEditPathRAD->text()+"/ASCII-dI/dI-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT",ErrMatrix, MD);
            }
            else saveMatrixToFile(lineEditPathRAD->text()+"/dI-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT",ErrMatrix, MD);
        }

        //+++ Standart radial averiging
        if (button == "I-Q")
        {
            if (comboBoxModecurrentText.contains("(MS)"))
            {
                double angleMS=double(spinBoxMCshiftAngle->value())/180.0*M_PI;

                radUniStandartMSmode(MD, Sample, SampleErr, mask, Xcenter, Ycenter, nameQI, C, lambda, deltaLambda,
                                     detdist, pixel * binning, r1, r2, label,
                                     selector->readRotations(Nsample, monitors->readDuration(Nsample)), pixelAsymetry,
                                     angleMS);
            }
            else
            {
                double angleAnisotropy = double(spinBoxAnisotropyOffset->value()) / 180.0 * M_PI;
                radUni(MD, Sample, SampleErr, mask, Xcenter, Ycenter, nameQI, C, lambda, deltaLambda, detdist,
                       pixel * binning, r1, r2, label,
                       selector->readRotations(Nsample, monitors->readDuration(Nsample)), pixelAsymetry, detRotationX,
                       detRotationY, angleAnisotropy);
            }
            mergedTemplate << nameQI;
        }

        //+++ Hosisontal Slice
        if (button=="I-Qx") horizontalSlice(MD,Sample,SampleErr,mask,Xcenter,Ycenter,nameQI,C,lambda,deltaLambda,detdist,pixel*binning*pixelAsymetry,r1,r2,label);
        
        //+++ Vertical Slice
        if (button=="I-Qy") verticalSlice(MD,Sample,SampleErr,mask,Xcenter,Ycenter,nameQI,C,lambda,deltaLambda,detdist,pixel*binning*pixelAsymetry,r1,r2,label);
        
        //+++ Polarv Coordinates
        if (button=="I-Polar") radUniPolar(MD, Sample, mask, Xcenter, Ycenter, nameQI, lambda, detdist, pixel*binning, pixelAsymetry );
        
        //+++ SIGMA [x,y]
        if (button=="Sigma-x-y") sigmaMatrix(MD, mask, Xcenter, Ycenter, "Sigma-"+currentExt+nameQI, lambda, deltaLambda, C, detdist, pixel*binning, r1,r2 );
        
        //+++ Q [x,y]
        if (button=="Q-x-y") MatrixQ(MD, mask, Xcenter, Ycenter, "Q-"+currentExt+nameQI, lambda, detdist, pixel*binning, pixelAsymetry, detRotationX, detRotationY );

        //+++ dQ [x,y]
        if (button=="dQ-x-y") dQmatrix(MD, mask, Xcenter, Ycenter, "dQ-"+currentExt+nameQI, lambda, detdist, pixel*binning, pixelAsymetry );
        
        //+++ Set Status
        w->setText(iRow,indexStatus,status);
        
        statusAll+="DAN :: "+status+"\n";
        
        //+++ Progress
        progressUpdateSteps--;
        
        if (progressUpdateSteps==0)
        {
            timeLeft=time.elapsed()/1000.0*(lastLine-iRow)/(iRow+1-firstLine);
            if (timeLeft>300)
                progress.setLabelText("[left: "+QString::number(timeLeft/60.0,'f',0)+" min] Current row: "+QString::number(iRow)+" [last: "+QString::number(lastLine)+"]; av. speed: "+QString::number(time.elapsed()/1000.0/(iRow-firstLine+1),'f',2)+" sec/run");
            else
                    progress.setLabelText("[left: "+QString::number(timeLeft,'f',0)+" sec] Current row: "+QString::number(iRow)+" [last: "+QString::number(lastLine)+"]; av. speed: "+QString::number(time.elapsed()/1000.0/(iRow-firstLine+1),'f',2)+" sec/run");
            progress.setValue( iRow-firstLine);
            
            if (iRow-firstLine<10) progressUpdateSteps=1;
            else if (iRow-firstLine<50) progressUpdateSteps=2;
            else progressUpdateSteps=5;
        }
        
        printf("DAN|Reduced Sample:\t\t%s\t[%6.5lgsec]\n", nameQI.toLocal8Bit().constData(),
               static_cast<double>(dt.elapsed() - pre_dt) / 1000.0);
        pre_dt = dt.elapsed();
        
        if ( progress.wasCanceled() ) break;
    }
    
    //+++ to log window: status
    toResLog(statusAll);
    toResLog("\n");
    //+++
    progress.cancel();
    app()->workspace()->blockSignals ( false );//+++ test 2019
    
    //+++ settings table
    saveSettings();

    //+++ merging table
    if ( button=="I-Q" && checkBoxMergingTable->isChecked() )
    {
        generateMergingTable(w, mergedTemplate);
        
        if (checkBoxAutoMerging->isChecked() )
        {
            if (radioButtonOpenInProjectisChecked && checkBoxSortOutputToFoldersisChecked) app()->changeFolder("DAN :: script, info, ...");
            if (app()->activateWindow(comboBoxMakeScriptTable->currentText()+"-mergingTemplate"))
            {
                readMergeInfo();
                mergeMethod();
            }
        }
    }

    //+++ back to init. folder
    if (radioButtonOpenInProject->isChecked() && checkBoxSortOutputToFoldersisChecked)
    {
        app()->folders->setCurrentItem ( cf->folderListItem() );
        app()->changeFolder(cf,true);
    }
    
    
    //+++ Clean Memory +++
    gsl_matrix_free(Sample);
    gsl_matrix_free(SampleErr);
    gsl_matrix_free(Buffer);
    gsl_matrix_free(BufferErr);
    gsl_matrix_free(EC);
    gsl_matrix_free(ECErr);
    gsl_matrix_free(BC);
    gsl_matrix_free(BCErr);
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    gsl_matrix_free(sensErr);
    if (checkBoxWaTrDetChecked) gsl_matrix_free(sensTrDet);
    gsl_matrix_free(ErrMatrix);
    
    scriptColList.clear();
    mergedTemplate.clear();
    listMask.clear();
    listSens.clear();
    
    //--- Clean Memory ---
}


//+++ SD:: RAD-UniStd-log
void dan18::radUniStandartMSmode
(
 int md,
 gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask,
 double Xcenter, double Ycenter,
 QString &sampleMatrix,
 double C, double lambda, double deltaLambda, double detdist, double detelem, double r1, double r2,
 QString label, double numberF, double pixelAsymetry, double angle
 )
{
    //+++
    int numRowsOut=0;
    int numAllQ=0;
    QList<int> negativeNumbersPosition;
    
    //+++ Presentation
    QString CurrentLabel=comboBoxSelectPresentation->currentText();
    
    //+++ Ext
    QString currentExt=lineEditFileExt->text().remove(" ");
    if(currentExt!="") currentExt+="-";
    
    //+++ Table Name
    QString tableOUT;
    tableOUT="QI-MS-NC-";
    if (CurrentLabel!="QI")tableOUT=tableOUT+CurrentLabel+"-";
    tableOUT=tableOUT+sampleMatrix;
    tableOUT=tableOUT.remove("-SM");
    tableOUT=tableOUT.replace("QI-","QI-"+currentExt);
    
    QString tableOUTms=tableOUT;
    tableOUTms=tableOUTms.replace("-MS-NC-","-MS-MC-");
    tableOUTms.remove("-SM");
    
    
    
    if (!checkBoxRewriteOutput->isChecked())
    {
        tableOUT+="-";
        tableOUT=app()->generateUniqueName(tableOUT);
        tableOUTms+="-";
        tableOUTms=app()->generateUniqueName(tableOUTms);
    }
    
    if (sansTab->currentIndex()==1 && comboBoxCheck->currentText().contains("raw-QI"))
    {
        tableOUT=  "raw-QI---MS-NC";
        tableOUTms="raw-QI---MS-MC";
    }
    else if (sansTab->currentIndex()==1 )
    {
        tableOUT= "raw-"+tableOUT;
        tableOUTms="raw-"+tableOUTms;
    }
    
    //+++ RAD-table
    Table *wOut;
    Table *wOutms;

    bool tableExist = app()->checkTableExistence(tableOUT);
    bool tableExistms = app()->checkTableExistence(tableOUTms);

    if (radioButtonOpenInProject->isChecked())
    {
        int colnumberInc=0;
        if (comboBox4thCol->currentIndex()<2)  colnumberInc=1;
        if (comboBox4thCol->currentIndex()==3)  colnumberInc=2;
        
        QList<MdiSubWindow *> tableList=app()->tableList();
        
        if (tableExist && checkBoxRewriteOutput->isChecked())
        {
            //+++ Find table
            foreach (MdiSubWindow *t, tableList) if (t->name()==tableOUT)  wOut=(Table *)t;
            //+++
            wOut->setNumRows(0);
            wOut->setNumCols(3+colnumberInc);
        }
        else wOut=app()->newHiddenTable(tableOUT,CurrentLabel, 0, 3+colnumberInc);

        
        if (tableExistms && checkBoxRewriteOutput->isChecked())
        {
            //+++ Find table
            foreach (MdiSubWindow *t, tableList) if (t->name()==tableOUTms)  wOutms=(Table *)t;
            for (int i=0;i<(int)tableList.count();i++)
            //+++
            wOutms->setNumRows(0);
            wOutms->setNumCols(3+colnumberInc);
        }
        else wOutms=app()->newHiddenTable(tableOUTms,CurrentLabel, 0, 3+colnumberInc);
        
        
        app()->setListViewLabel(wOut->name(), label);
        app()->updateWindowLists(wOut);
        
        app()->setListViewLabel(wOutms->name(), label);
        app()->updateWindowLists(wOutms);
        
        wOut->setColName(0,"Q");
        wOut->setColName(1,"I"); wOut->setColComment(1, label);
        wOut->setColName(2,"dI");
        wOut->setWindowLabel(label);
        
        wOut->setColPlotDesignation(2,Table::yErr);
        wOut->setHeaderColType();
        
        wOut->setColNumericFormat(2, 8, 0, true);
        wOut->setColNumericFormat(2, 8, 1, true);
        wOut->setColNumericFormat(2, 8, 2, true);
        
        wOutms->setColName(0,"Q");
        wOutms->setColName(1,"I"); wOutms->setColComment(1, label);
        wOutms->setColName(2,"dI");
        wOutms->setWindowLabel(label);
        
        wOutms->setColPlotDesignation(2,Table::yErr);
        wOutms->setHeaderColType();
        
        wOutms->setColNumericFormat(2, 8, 0, true);
        wOutms->setColNumericFormat(2, 8, 1, true);
        wOutms->setColNumericFormat(2, 8, 2, true);
        
        
        if (comboBox4thCol->currentIndex()==0)
        {
            wOut->setColName(3,"Sigma");
            wOut->setColPlotDesignation(3,Table::xErr);
            wOutms->setColName(3,"Sigma");
            wOutms->setColPlotDesignation(3,Table::xErr);
            wOut->setColNumericFormat(2, 8, 3, true);
            wOutms->setColNumericFormat(2, 8, 3, true);
        }
        else if (comboBox4thCol->currentIndex()==1)
        {
            wOut->setColName(3,"dQ");
            wOut->setColPlotDesignation(3,Table::xErr);
            wOutms->setColName(3,"dQ");
            wOutms->setColPlotDesignation(3,Table::xErr);
            wOut->setColNumericFormat(2, 8, 3, true);
            wOutms->setColNumericFormat(2, 8, 3, true);
        }
        else if (comboBox4thCol->currentIndex()==3)
        {
            wOut->setColName(3,"dQ");
            wOut->setColPlotDesignation(3,Table::xErr);
            wOut->setColName(4,"Sigma");
            wOut->setColPlotDesignation(4,Table::xErr);
            wOut->setColPlotDesignation(4,Table::xErr);
            wOut->setColNumericFormat(2, 8, 3, true);
            wOut->setColNumericFormat(2, 8, 4, true);
            
            wOutms->setColName(3,"dQ");
            wOutms->setColPlotDesignation(3,Table::xErr);
            wOutms->setColName(4,"Sigma");
            wOutms->setColPlotDesignation(4,Table::xErr);
            wOutms->setColPlotDesignation(4,Table::xErr);
            wOutms->setColNumericFormat(2, 8, 3, true);
            wOutms->setColNumericFormat(2, 8, 4, true);
        }
        
        wOut->setHeaderColType();
        wOutms->setHeaderColType();
        
    }
    
    //------------------------------------
    // calculation of radial averaging
    //------------------------------------
    
    
    //+++ MAXIMAL RADIUS
    double rmax =  sqrt( (Xcenter)*(Xcenter) + pixelAsymetry*pixelAsymetry*( Ycenter)*(Ycenter));
    rmax = GSL_MAX(rmax, (sqrt((md - 1 -
                                Xcenter)*(md - 1 - Xcenter) + pixelAsymetry*pixelAsymetry*(Ycenter)*(Ycenter))));
    rmax = GSL_MAX(rmax,sqrt((md - 1 -
                              Xcenter)*(md - 1 - Xcenter) + pixelAsymetry*pixelAsymetry*(md - 1 - Ycenter)*(md - 1 - Ycenter)));
    rmax = GSL_MAX(rmax,sqrt(( Xcenter)*(Xcenter) + pixelAsymetry*pixelAsymetry*(md - 1 - Ycenter)*(md - 1 - Ycenter)));
    
    int iii,jjj;
    
    int nTotal=(int)(rmax+0.5);
    
    double *II=new double[nTotal];
    double *dII=new double[nTotal];
    
    double *IIms=new double[nTotal];
    double *dIIms=new double[nTotal];
    
    int *nn=new int[nTotal];
    
    double offsetCh;
    
    //+++ cleanning
    for(int ir=0;ir<nTotal;ir++)
    {
        II[ir]= 0;
        dII[ir]= 0;
        IIms[ir]= 0;
        dIIms[ir]= 0;
        nn[ir]=0;
    }
    
    int nShift=0;
    
    
    for(int ir=0;ir<nTotal;ir++)
    {
        
        //    double length=2*M_PI*ir;
        
        double length=2*M_PI*ir;
        if (pixelAsymetry<1.0 && pixelAsymetry>0.0)
        {
            length= M_PI*ir*( 3* (1 + 1/pixelAsymetry) - sqrt((3+1/pixelAsymetry)*(1+3/pixelAsymetry))); // 2015-10 :: Ramanujan approach
        }
        int numPhiSteps=(int)(length+0.5);
        
        double phiSteps=1.0;
        if (numPhiSteps>1) phiSteps=2*M_PI/numPhiSteps;
        int xPhi, yPhi;
        
        int realNumPhiSteps=0;
        
        for(int iPhi=0;iPhi<numPhiSteps;iPhi++)
        {
            xPhi=(int)(ir*cos(-iPhi*phiSteps+M_PI/2.0+angle)+Xcenter+0.5);
            yPhi=(int)(ir/pixelAsymetry*sin(-iPhi*phiSteps+M_PI/2.0+angle)+Ycenter+0.5);
            
            if (xPhi>=0 && xPhi<md && yPhi>=0 && yPhi<md && gsl_matrix_get(mask,yPhi,xPhi)>0)  realNumPhiSteps++;
        }
        
        if (realNumPhiSteps<4) {nShift++;continue;}
        
        if (ir==nShift+spinBoxMCcheckQ->value())
        {
            std::cout<<"\n MS-mode cross-check :: file: "<<tableOUTms.toLocal8Bit().constData()<<"--- #: "<<ir-nShift<<"\n\n";
            std::cout<<"xPhi"<<"\t\t"<<"yPhi"<<"\t\t""sin2phi"<<"\t\t"<<"Intensity"<<"\n";
        }
        
        double *sin2phi=new double[realNumPhiSteps];
        double *intensityPhi=new double[realNumPhiSteps];
        double *dIntensityPhi=new double[realNumPhiSteps];
        
        realNumPhiSteps=0;
        
        for(int iPhi=0; iPhi<numPhiSteps; iPhi++)
        {
            xPhi=(int)(ir*cos(-iPhi*phiSteps+M_PI/2.0+angle)+Xcenter+0.5);
            yPhi=(int)(ir/pixelAsymetry*sin(-iPhi*phiSteps+M_PI/2.0+angle)+Ycenter+0.5);
            
            if (xPhi>=0 && xPhi<md && yPhi>=0 && yPhi<md && gsl_matrix_get(mask,yPhi,xPhi)>0)
            {
                sin2phi[realNumPhiSteps]=sin(-iPhi*phiSteps)*sin(-iPhi*phiSteps);
                intensityPhi[realNumPhiSteps]=gsl_matrix_get(Sample,yPhi,xPhi);
                
                dIntensityPhi[realNumPhiSteps]=1;//sqrt(gsl_matrix_get(SampleErr,yPhi,xPhi))*fabs(gsl_matrix_get(Sample,yPhi,xPhi));
                
                if (ir==nShift+spinBoxMCcheckQ->value()) std::cout<<"\n"<<QString::number(xPhi,'E',8).toLocal8Bit().constData()<<"\t"<<QString::number(yPhi,'E',8).toLocal8Bit().constData()<<"\t"<<QString::number(sin2phi[realNumPhiSteps],'E',8).toLocal8Bit().constData()<<"\t"<<QString::number(intensityPhi[realNumPhiSteps],'E',8).toLocal8Bit().constData();
                
                realNumPhiSteps++;
            }
        }
        if (ir==nShift+spinBoxMCcheckQ->value()) std::cout<<"\n";
        double c0, c1, cov00, cov01, cov11, chisq;
        gsl_fit_wlinear (sin2phi, 1, dIntensityPhi, 1, intensityPhi, 1, realNumPhiSteps,
                         &c0, &c1, &cov00, &cov01, &cov11,
                         &chisq);
        
        nn[ir]=1;
        II[ir]=c0;
        dII[ir]=chisq/(realNumPhiSteps-2)*cov00;
        IIms[ir]=c1;
        dIIms[ir]=chisq/(realNumPhiSteps-2)*cov11;
        
        delete[] sin2phi;
        delete[] intensityPhi;
        delete[] dIntensityPhi;
    }
    
    
    // new log scale
    double stepProg=doubleSpinBoxAvLog->value();
    
    double IIlocal, dIIlocal, IIlocalms, dIIlocalms;
    int nnLocal;
    
    int *mergedPoints=new int[nTotal];
    
    int i=0;
    while(i<nTotal)
    {
        double tm=pow(stepProg,i);
        int plusPoints=int(tm);
        if (tm-int(tm)>0.5) plusPoints++;
        
        if (i+plusPoints>=nTotal) plusPoints=nTotal-i;
        
        
        IIlocal=0.0;
        dIIlocal=0.0;
        IIlocalms=0.0;
        dIIlocalms=0.0;
        nnLocal=0;
        for (int ii=0;ii<plusPoints;ii++)
        {
            nnLocal+=nn[i+ii];
            IIlocal+=II[i+ii];
            dIIlocal+=dII[i+ii];
            IIlocalms+=IIms[i+ii];
            dIIlocalms+=dIIms[i+ii];
            
            mergedPoints[i+ii]=plusPoints;
            II[i+ii]=0.0;
            dII[i+ii]=0;
            IIms[i+ii]=0.0;
            dIIms[i+ii]=0;
        }
        
        mergedPoints[i]=plusPoints;
        nn[i]=nnLocal;
        II[i]=IIlocal;
        dII[i]=dIIlocal;
        IIms[i]=IIlocalms;
        dIIms[i]=dIIlocalms;
        
        i=i+plusPoints;
    }
    
    
    // +++ rad
    
    double qr, avg, avge,avgms, avgems, sig, dq;
    
    QString streamSTR="";
    QString streamSTRms="";
    
    //+++ new Skip Points
    int skipFirst=spinBoxRemoveFirst->value();
    int skipLast=spinBoxRemoveLast->value();
    
    for(int ir=0;ir<nTotal;ir++)
    {
        if (nn[ir]>=1)
        {
            qr  = 4.0*M_PI/lambda *
            sin(0.5*atan((2*ir+mergedPoints[ir]-1)/2.0*detelem/detdist));  // test
            
            
            avg = II[ir] / nn[ir];
            avge=dII[ir] / nn[ir];
            
            avgms = IIms[ir] / nn[ir];
            avgems=dIIms[ir] / nn[ir];
            
            sig= sigmaQmerged( qr, detdist, C, lambda, deltaLambda, r1, r2, mergedPoints[ir]);
            dq=dQ(qr, numberF, mergedPoints[ir]*ir*(pixelAsymetry+1)/2*detelem, detdist, detelem );
            
            //
            double Q,I,dI,Ims,dIms;
            double QQQ, III, dIII, IIIms, dIIIms;
            
            Q=qr;
            I=avg;
            dI=avge;
            
            Ims=avgms;
            dIms=avgems;
            
            
            QQQ=qr;
            III=avg;
            dIII=avge;
            IIIms=avgms;
            dIIIms=avgems;
            
            // * ****** Change of Presentation  *****************************
            if (CurrentLabel!="QI")
            {
                if (CurrentLabel=="Guinier")
                {
                    QQQ=Q*Q;
                    III=log(fabs(I));
                    if (I!=0) dIII=dI/fabs(I);
                    IIIms=log(fabs(Ims));
                    if (Ims!=0) dIIIms=dIms/fabs(Ims);
                }
                else if (CurrentLabel=="Zimm")
                {
                    QQQ=Q*Q;
                    if (I!=0) III=1/I; else III=0;
                    if (I!=0) dIII=dI/I/I; else dIII=0;
                    
                    if (Ims!=0) IIIms=1/Ims; else IIIms=0;
                    if (Ims!=0) dIIIms=dIms/Ims/Ims; else dIIIms=0;
                }
                else if (CurrentLabel=="Porod")
                {
                    QQQ=log10(Q);
                    III=log10(fabs(I));
                    if (I!=0) dIII=dI/fabs(I)/log(10.0);
                    IIIms=log10(fabs(Ims));
                    if (Ims!=0) dIII=dI/fabs(Ims)/log(10.0);
                }
                else if (CurrentLabel=="Porod2")
                {
                    QQQ=Q*Q;
                    III=I*pow(Q,4);
                    dIII=dI*pow(Q,4);
                    IIIms=Ims*pow(Q,4);
                    dIIIms=dIms*pow(Q,4);
                }
                else if (CurrentLabel=="logQ")
                {
                    QQQ=log10(Q);
                }
                else if (CurrentLabel=="logI")
                {
                    III=log10(fabs(I));
                    if (I!=0) dIII=dI/fabs(I)/log(10.0);
                    IIIms=log10(fabs(Ims));
                    if (Ims!=0) dIIIms=dIms/fabs(Ims)/log(10.0);
                }
                else if (CurrentLabel=="Debye")
                {
                    QQQ=Q*Q;
                    if (I!=0) III=1/sqrt(fabs(I)); else III=0;
                    if (I!=0) dIII=dI/pow(fabs(I),1.5); else dIII=0;
                    if (Ims!=0) IIIms=1/sqrt(fabs(Ims)); else IIIms=0;
                    if (Ims!=0) dIIIms=dIms/pow(fabs(Ims),1.5); else dIIIms=0;
                }
                else if (CurrentLabel=="1Moment")
                {
                    III=Q*I;
                    dIII=Q*dI;
                    IIIms=Q*Ims;
                    dIIIms=Q*dIms;
                }
                else if (CurrentLabel=="2Moment")
                {
                    III=Q*Q*I;
                    dIII=Q*Q*dI;
                    IIIms=Q*Q*Ims;
                    dIIIms=Q*Q*dIms;
                }
                else if (CurrentLabel=="Kratky")
                {
                    III=Q*Q*I;
                    dIII=Q*Q*dI;
                    IIIms=Q*Q*Ims;
                    dIIIms=Q*Q*dIms;
                }
                else if (CurrentLabel=="GuinierRod")
                {
                    QQQ=Q*Q;
                    III=log(fabs(I*Q));
                    if (I!=0) dIII=Q*fabs((dI/I)); else III=0;
                    IIIms=log(fabs(Ims*Q));
                    if (Ims!=0) dIIIms=Q*fabs((dIms/Ims)); else IIIms=0;
                }
                else if (CurrentLabel=="GuinierPlate")
                {
                    QQQ=Q*Q;
                    III=log(fabs(I*Q*Q));
                    if (I!=0) dIII=Q*Q*fabs(dI/I); else III=0;
                    IIIms=log(fabs(Ims*Q*Q));
                    if (Ims!=0) dIIIms=Q*Q*fabs(dIms/Ims); else IIIms=0;
                }
                
            }
            
            //+++  To File Q-I-dI-?
            if (radioButtonFile->isChecked())
            {
                
                if (numAllQ>=skipFirst)
                {
                    if ( !( (avg<=0 || avgms<=0) && checkBoxMaskNegativeQ->isChecked()) )
                    {
                        if (comboBox4thCol->currentIndex()==0)
                        {
                            streamSTR    +=      QString::number(QQQ,'E',8)
                            +"   "+QString::number(III,'E',8)
                            +"   "+QString::number(dIII,'E',8)
                            +"   "+QString::number(sig,'E',8)+"\n";
                            streamSTRms    +=      QString::number(QQQ,'E',8)
                            +"   "+QString::number(IIIms,'E',8)
                            +"   "+QString::number(dIIIms,'E',8)
                            +"   "+QString::number(sig,'E',8)+"\n";
                        }
                        else if (comboBox4thCol->currentIndex()==1)
                        {
                            streamSTR    +=      QString::number(QQQ,'E',8)
                            +"   "+QString::number(III,'E',8)
                            +"   "+QString::number(dIII,'E',8)
                            +"   "+QString::number(dq,'E',8)+"\n";
                            streamSTRms    +=      QString::number(QQQ,'E',8)
                            +"   "+QString::number(IIIms,'E',8)
                            +"   "+QString::number(dIIIms,'E',8)
                            +"   "+QString::number(dq,'E',8)+"\n";
                        }
                        else if (comboBox4thCol->currentIndex()==2)
                        {
                            streamSTR	+=      QString::number(QQQ,'E',8)
                            +"   "+QString::number(III,'E',8)
                            +"   "+QString::number(dIII,'E',8)+"\n" ;
                            streamSTRms	+=      QString::number(QQQ,'E',8)
                            +"   "+QString::number(IIIms,'E',8)
                            +"   "+QString::number(dIIIms,'E',8)+"\n" ;
                        }
                        else
                        {
                            streamSTR+=QString::number(QQQ,'E',8)
                            +"   "+QString::number(III,'E',8)
                            +"   "+QString::number(dIII,'E',8)
                            +"   "+QString::number(dq,'E',8)
                            +"   "+QString::number(sig,'E',8)+"\n";
                            streamSTRms	+=QString::number(QQQ,'E',8)
                            +"   "+QString::number(IIIms,'E',8)
                            +"   "+QString::number(dIIIms,'E',8)
                            +"   "+QString::number(dq,'E',8)
                            +"   "+QString::number(sig,'E',8)+"\n";
                        }
                        
                        numRowsOut++;
                    }
                    else negativeNumbersPosition<<numAllQ;
                }
                
                numAllQ++;
                
            }
            else
            {
                if (numAllQ>=skipFirst)
                {
                    if ( !((avg<=0 || avgms<=0) && checkBoxMaskNegativeQ->isChecked()) )
                    {
                        wOut->setNumRows(numRowsOut+1);
                        wOut->setText(numRowsOut,0,QString::number(QQQ,'E',8));
                        wOut->setText(numRowsOut,1,QString::number(III,'E',8));
                        wOut->setText(numRowsOut,2,QString::number(dIII,'E',8));
                        
                        wOutms->setNumRows(numRowsOut+1);
                        wOutms->setText(numRowsOut,0,QString::number(QQQ,'E',8));
                        wOutms->setText(numRowsOut,1,QString::number(IIIms,'E',8));
                        wOutms->setText(numRowsOut,2,QString::number(dIIIms,'E',8));
                        
                        
                        //+++ Sigma output +++
                        if (comboBox4thCol->currentIndex()==0) {
                            wOut->setText(numRowsOut,3, QString::number(sig,'E',8));
                            wOutms->setText(numRowsOut,3, QString::number(sig,'E',8));
                        }
                        else if (comboBox4thCol->currentIndex()==1){
                            wOut->setText(numRowsOut,3, QString::number(dq,'E',8));
                            wOutms->setText(numRowsOut,3, QString::number(dq,'E',8));
                        }
                        else if (comboBox4thCol->currentIndex()==3){
                            wOut->setText(numRowsOut,3, QString::number(dq,'E',8));
                            wOut->setText(numRowsOut,4, QString::number(sig,'E',8));
                            wOutms->setText(numRowsOut,3, QString::number(dq,'E',8));
                            wOutms->setText(numRowsOut,4, QString::number(sig,'E',8));
                            
                        }
                        
                        numRowsOut++;
                    }
                    else negativeNumbersPosition<<numAllQ;
                }
                numAllQ++;
                
            }
        }
    }
    
    //+++ Calculate number last points 
    int scipLastReal=skipLast;
    
    if (skipLast>0) for (int i=1; i<=skipLast; i++)
    {
        if ( negativeNumbersPosition.indexOf( numAllQ-i )>=0 ) scipLastReal--;
    }
    
    //+++ remove last Points
    if (radioButtonOpenInProject->isChecked())
    {
        if (wOut->numRows()>=scipLastReal) wOut->setNumRows( wOut->numRows() - scipLastReal ); else wOut->setNumRows(0);
        if (wOutms->numRows()>=scipLastReal) wOutms->setNumRows( wOutms->numRows() -scipLastReal ); else wOutms->setNumRows(0);
        
        
        if (tableExist) wOut->notifyChanges();
        if (tableExistms) wOutms->notifyChanges();
        
        //+++ adjust cols
        for (int tt=0; tt<wOut->numCols(); tt++)
        {
            wOut->table()->resizeColumnToContents(tt);
            wOut->table()->setColumnWidth(tt, wOut->table()->columnWidth(tt)+10); 
            wOutms->table()->resizeColumnToContents(tt);
            wOutms->table()->setColumnWidth(tt, wOutms->table()->columnWidth(tt)+10); 
            
        }
        
        //wOut->hide();//--- 2021-03
        //wOutms->hide();//--- 2021-03

        app()->hideWindow(wOut); //+++ 2021-03
        app()->hideWindow(wOutms); //+++ 2021-03
    }
    
    
    int findN, tempI;
    
    
    for (findN=0;findN<scipLastReal;findN++) {
        tempI=streamSTR.lastIndexOf("\n",-3);
        streamSTR.remove(tempI+1,streamSTR.length()-tempI+2);};  
    
    for (findN=0;findN<scipLastReal;findN++) {
        tempI=streamSTRms.lastIndexOf("\n",-3);
        streamSTRms.remove(tempI+1,streamSTRms.length()-tempI+2);};  
    
    if (!radioButtonOpenInProject->isChecked())
    {
        QFile f;
        
        QDir dd;
        
        QString asciiPath=lineEditPathRAD->text();
        
        if (checkBoxSortOutputToFolders->isChecked())	
        {	
            asciiPath+="/ASCII-QI-MS";
            asciiPath=asciiPath.replace("//","/");
            if (!dd.cd(asciiPath)) 
            {
                dd.mkdir(asciiPath); 	
                
            };
        }
        
        QString fname="QI-MS-NC-";
        if (CurrentLabel!="QI") fname=fname+CurrentLabel+"-";
        fname=fname+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT";
        fname=fname.remove("-SM");
        fname=fname.replace("QI-","QI-"+currentExt);
        
        fname=asciiPath+"/"+fname;
        
        f.setFileName( fname );
        if ( f.open( QIODevice::WriteOnly ) )
        {
            QTextStream stream( &f );
            
            if(comboBoxSelectPresentation->currentIndex()==0 && checkBoxASCIIheader->isChecked())
            {
                QString header;
                if (comboBox4thCol->currentIndex()==0)
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]       Sigma[1/A]\n";
                }
                else if (comboBox4thCol->currentIndex()==1)
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]          dQ[1/A]\n";
                }				
                else if (comboBox4thCol->currentIndex()==2)
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]\n";
                }
                else if (comboBox4thCol->currentIndex()==3)
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]          dQ[1/A]       Sigma[1/A]\n";
                }
                
                stream<<header;
            }
            stream<<streamSTR;
            
            f.close();
        }
    }
    
    if (!radioButtonOpenInProject->isChecked())
    {
        QFile f;
        
        QDir dd;
        
        QString asciiPath=lineEditPathRAD->text();
        
        if (checkBoxSortOutputToFolders->isChecked())	
        {	
            asciiPath+="/ASCII-QI-MS";
            asciiPath=asciiPath.replace("//","/");
            if (!dd.cd(asciiPath)) 
            {
                dd.mkdir(asciiPath); 	
                
            };
        }
        
        QString fname="QI-MS-MC-";
        if (CurrentLabel!="QI") fname=fname+CurrentLabel+"-";
        fname=fname+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT";
        fname=fname.remove("-SM");
        fname=fname.replace("QI-","QI-"+currentExt);
        fname=asciiPath+"/"+fname;
        
        f.setFileName( fname );
        if ( f.open( QIODevice::WriteOnly ) )
        {
            QTextStream stream( &f );
            
            if(comboBoxSelectPresentation->currentIndex()==0 && checkBoxASCIIheader->isChecked())
            {
                QString header;
                if (comboBox4thCol->currentIndex()==0)
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]       Sigma[1/A]\n";
                }
                else if (comboBox4thCol->currentIndex()==1)
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]          dQ[1/A]\n";
                }				
                else if (comboBox4thCol->currentIndex()==2)
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]\n";
                }
                else if (comboBox4thCol->currentIndex()==3)
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]          dQ[1/A]       Sigma[1/A]\n";
                }
                
                stream<<header;
            }
            stream<<streamSTRms;
            
            f.close();
        }
    }
    
    delete[] II;
    delete[] dII;
    delete[] IIms;
    delete[] dIIms;
    
    delete[] nn;
    delete[] mergedPoints;
    
    sampleMatrix=tableOUT;
}

//+++ SD:: RAD-UniStd-log  v. 2022-10-05
void dan18::radUni
(
 int md,
 gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask,
 double Xcenter, double Ycenter,
 QString &sampleMatrix,
 double C, double lambda, double deltaLambda, double detdist, double detelem, double r1, double r2,
 QString label, double numberF, double pixelAsymetry, double DetRotationX, double DetRotationY, double angleAnisotropy
 )

{
    //------------------------------------
    // All Data
    //------------------------------------
    int nTotal;
    double *QQ;
    double *II;
    double *dII;
    double *IIcos2phi;
    double *nn;

    //------------------------------------
    // calculation of radial averaging
    //------------------------------------
    if ( radioButtonRadStd->isChecked())
    radAvStd(
             md, Sample, SampleErr, mask, Xcenter, Ycenter, lambda, detdist, detelem,
             pixelAsymetry, DetRotationX, DetRotationY, angleAnisotropy, nTotal, QQ, II, dII, IIcos2phi, nn
             );
    else
        radAvHF(
                 md, Sample, SampleErr, mask, Xcenter, Ycenter, lambda, detdist, detelem,
                 pixelAsymetry, DetRotationX, DetRotationY, angleAnisotropy, nTotal, QQ, II, dII, IIcos2phi, nn
                 );
        
    //------------------------------------
    // merged points: vector of control of merging; nCurrent: current active q-s
    //------------------------------------
    int *mergedPoints=new int[nTotal];
    int nCurrent;
    
    //------------------------------------
    // Options: Log/Line binning, remove first/last/negative points
    //------------------------------------
    radAvDataCorrectionOptions (nTotal, QQ, II, dII, IIcos2phi, mergedPoints,  nn, nCurrent);

    //------------------------------------
    // Final Radial Data Vectors
    //------------------------------------
    double *qCurrent=new double[nCurrent];
    double *dqCurrent=new double[nCurrent];
    double *iCurrent=new double[nCurrent];
    double *diCurrent=new double[nCurrent];
    double *anisotropyCurrent=new double[nCurrent];
    double *sigmaCurrent=new double[nCurrent];

    //------------------------------------
    // Data Transfer From QQ/II/dII to qCurrent/dqCurrent/iCurrentdiCurrent/sigmaCurrent
    //------------------------------------
    double minPixel=1.0;
    if (pixelAsymetry<1.0) minPixel=pixelAsymetry;

    int i=0;
    for(int ir=0;ir<nTotal;ir++)
    {
        if (nn[ir]>=0)
        {
            if (i>=nCurrent) break;
            //+++
            qCurrent[i]=QQ[ir];
            
            if (II[ir]>0 ) anisotropyCurrent[i]= IIcos2phi[ir] / II[ir]; else anisotropyCurrent[i]=0.0;
            if (nn[ir]>0 ) iCurrent[i]= II[ir] / nn[ir]; else iCurrent[i]=0.0;
            if (nn[ir]>0 ) diCurrent[i]=sqrt(fabs(dII[ir])) / nn[ir]; else diCurrent[i]=0.0;
            
            if (mergedPoints[ir]>0)
            {
                sigmaCurrent[i]= sigmaQmerged( qCurrent[i], detdist, C, lambda, deltaLambda, r1, r2, mergedPoints[ir]);
                dqCurrent[i]=dQ(qCurrent[i], numberF, mergedPoints[ir]*ir*minPixel*(pixelAsymetry+1)/2*detelem, detdist, detelem );
            }
            else
            {
                sigmaCurrent[i]= sigmaQmerged( qCurrent[i], detdist, C, lambda, deltaLambda, r1, r2, 1);
                dqCurrent[i]=dQ(qCurrent[i], numberF, 1*ir*minPixel*(pixelAsymetry+1)/2*detelem, detdist, detelem );
            }
            //+++
            i++;
        }
    }

    //------------------------------------
    // Cleanning of Memory from QQ/II/dII/nn/mergedPoints arrays
    //------------------------------------
    delete[] QQ;
    delete[] II;
    delete[] dII;
    delete[] IIcos2phi;
    delete[] nn;
    delete[] mergedPoints;
    
    //------------------------------------
    // Change of SAS presentations: from Q-I to Guinier/Zimm/...
    //------------------------------------
    radAvDataChangeSASpresentation(nCurrent,qCurrent,iCurrent,diCurrent,dqCurrent);
    
    //------------------------------------
    // QI table/ascii-file generation
    //------------------------------------
    if ( radioButtonOpenInProject->isChecked() )
        radAvTableGeneration(sampleMatrix,label,nCurrent, qCurrent,iCurrent,diCurrent,dqCurrent,sigmaCurrent,anisotropyCurrent);
    else
        radAvASCIIGeneration(sampleMatrix,label,nCurrent, qCurrent,iCurrent,diCurrent,dqCurrent,sigmaCurrent,anisotropyCurrent);
    
    //------------------------------------
    // Cleanning of Memory from qCurrent/dqCurrent/iCurrentdiCurrent/sigmaCurrent arrays
    //------------------------------------
    delete[] qCurrent;
    delete[] iCurrent;
    delete[] diCurrent;
    delete[] dqCurrent;
    delete[] anisotropyCurrent;
    delete[] sigmaCurrent;
}

//+++ Change of SAS presentations: from Q-I to Guinier/Zimm/...
void dan18::radAvDataChangeSASpresentation( int nCurrent, double *&QQ, double *&II, double *&dII, double *&dQQ )
{
    //+++ Presentation
    QString CurrentLabel=comboBoxSelectPresentation->currentText();
    
    //+++
    if (CurrentLabel=="QI") return;
    

    double Q,I,dI, dQ;
    double QQQ, III, dIII, dQQQ;
    
    for(int ir=0;ir<nCurrent;ir++)
    {
        Q=QQ[ir];
        I=II[ir];
        dI=dII[ir];
        dQ=dQQ[ir];
        
        if (CurrentLabel=="Guinier")
        {
            QQQ=Q*Q;
            dQQQ=2*Q*dQ;
            III=log(fabs(I));
            if (I!=0) dIII=dI/fabs(I);
        }
        else if (CurrentLabel=="Zimm")
        {
            QQQ=Q*Q;
            dQQQ=2*Q*dQ;
            if (I!=0) III=1/I; else III=0;
            if (I!=0) dIII=dI/I/I; else dIII=0;
        }
        else if (CurrentLabel=="Porod")
        {
            QQQ=log10(Q);
            if (Q==0) dQQQ=0; else dQQQ=dQ/log(10.0)/fabs(Q);
            III=log10(fabs(I));
            if (I!=0) dIII=dI/fabs(I)/log(10.0);
        }
        else if (CurrentLabel=="Porod2")
        {
            QQQ=Q*Q;
            dQQQ=2*Q*dQ;
            III=I*pow(Q,4);
            dIII=dI*pow(Q,4);
        }
        else if (CurrentLabel=="logQ")
        {
            QQQ=log10(Q);
            if (Q==0) dQQQ=0; else dQQQ=dQ/log(10.0)/fabs(Q);
        }
        else if (CurrentLabel=="logI")
        {
            
            III=log10(fabs(I));
            if (I!=0) dIII=dI/fabs(I)/log(10.0);
        }
        else if (CurrentLabel=="Debye")
        {
            QQQ=Q*Q;
            dQQQ=2*Q*dQ;
            if (I!=0) III=1/sqrt(fabs(I)); else III=0;
            if (I!=0) dIII=dI/pow(fabs(I),1.5); else dIII=0;
        }
        else if (CurrentLabel=="1Moment")
        {
            III=Q*I;
            dIII=Q*dI;
        }
        else if (CurrentLabel=="2Moment")
        {
            III=Q*Q*I;
            dIII=Q*Q*dI;
        }
        else if (CurrentLabel=="Kratky")
        {
            III=Q*Q*I;
            dIII=Q*Q*dI;
        }
        else if (CurrentLabel=="GuinierRod")
        {
            QQQ=Q*Q;
            dQQQ=2*Q*dQ;
            III=log(fabs(I*Q));
            if (I!=0) dIII=Q*fabs((dI/I)); else III=0;
        }
        else if (CurrentLabel=="GuinierPlate")
        {
            QQQ=Q*Q;
            dQQQ=2*Q*dQ;
            III=log(fabs(I*Q*Q));
            if (I!=0) dIII=Q*Q*fabs(dI/I); else III=0;
        }
        
        QQ[ir]= QQQ;
        II[ir]= III;
        dII[ir]=dIII;
        dQQ[ir]=dQQQ;
    }
}

//+++ SD:: QI rad Av std log, det. ritation, ...
void dan18::radAvStd
(
 int md,
 gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask,
 double Xcenter, double Ycenter,
 double lambda, double detdist, double detelem,
 double pixelAsymetry, double DetRotationX, double DetRotationY, double angleAnisotropy,
 int &nTotal, double *&QQ, double *&II, double *&dII, double *&IIcos2phi, double *&nn
 )
{
    //+++ DetRotationX/Y in radians
    DetRotationX=DetRotationX/180.0*M_PI;
    DetRotationY=DetRotationY/180.0*M_PI;
    
    double minPixel=1.0;
    if (pixelAsymetry<1.0) minPixel=pixelAsymetry;
    
    double xcRotated=detdist*sin(DetRotationX)*cos(DetRotationY);
    double ycRotated=detdist*sin(DetRotationY)*cos(DetRotationX);
    double zcRotated=detdist*cos(DetRotationX)*cos(DetRotationY);
    
    double R0=sqrt(xcRotated*xcRotated + ycRotated*ycRotated + zcRotated*zcRotated);
    
    //+++ (0,0)
    double Rcorner= sqrt( detelem * (0-Xcenter)* detelem * (0-Xcenter)+pixelAsymetry*pixelAsymetry*detelem * detelem * (0-Ycenter)*(0-Ycenter) + detdist*detdist );
    
    double rRcorner=detelem*(0-Xcenter)*xcRotated;
    rRcorner+=pixelAsymetry*detelem*(0-Ycenter)*ycRotated;
    rRcorner+=detdist*zcRotated;
    
    double angleMax=acos(rRcorner/Rcorner/R0);
    double angleMin=angleMax;
    
    double angleCurrent;
    
    //+++ (0, 1..md-1)
    for (int i=1;i<md;i++)
    {
        Rcorner= sqrt( detelem * (0-Xcenter)* detelem * (0-Xcenter)+pixelAsymetry*pixelAsymetry*detelem * detelem * (i-Ycenter)*(i-Ycenter) + detdist*detdist );
        
        rRcorner=detelem*(0-Xcenter)*xcRotated;
        rRcorner+=pixelAsymetry*detelem*(i-Ycenter)*ycRotated;
        rRcorner+=detdist*zcRotated;
        
        angleCurrent=acos(rRcorner/Rcorner/R0);
        
        if (angleCurrent > angleMax ) angleMax=angleCurrent;
        if (angleCurrent < angleMin ) angleMin=angleCurrent;
    }
    
    //+++ (1..md-1, 0)
    for (int i=1;i<md;i++)
    {
        Rcorner= sqrt( detelem * (i-Xcenter)* detelem * (i-Xcenter)+pixelAsymetry*pixelAsymetry*detelem * detelem * (0-Ycenter)*(0-Ycenter) + detdist*detdist );
        
        rRcorner=detelem*(i-Xcenter)*xcRotated;
        rRcorner+=pixelAsymetry*detelem*(0-Ycenter)*ycRotated;
        rRcorner+=detdist*zcRotated;
        
        angleCurrent=acos(rRcorner/Rcorner/R0);
        
        if (angleCurrent > angleMax ) angleMax=angleCurrent;
        if (angleCurrent < angleMin ) angleMin=angleCurrent;
    }
    
    //+++ (md-1, 1..md-1)
    for (int i=1;i<md;i++)
    {
        Rcorner= sqrt( detelem * (md-1-Xcenter)* detelem * (md-1-Xcenter)+pixelAsymetry*pixelAsymetry*detelem * detelem * (i-Ycenter)*(i-Ycenter) + detdist*detdist );
        
        rRcorner=detelem*(md-1-Xcenter)*xcRotated;
        rRcorner+=pixelAsymetry*detelem*(i-Ycenter)*ycRotated;
        rRcorner+=detdist*zcRotated;
        
        angleCurrent=acos(rRcorner/Rcorner/R0);
        
        if (angleCurrent > angleMax ) angleMax=angleCurrent;
        if (angleCurrent < angleMin ) angleMin=angleCurrent;
    }
    
    //+++ (1..md-1, md-1)
    for (int i=1;i<md;i++)
    {
        Rcorner= sqrt( detelem * (i-Xcenter)* detelem * (i-Xcenter)+pixelAsymetry*pixelAsymetry*detelem * detelem * (md-1-Ycenter)*(md-1-Ycenter) + detdist*detdist );
        
        rRcorner=detelem*(i-Xcenter)*xcRotated;
        rRcorner+=pixelAsymetry*detelem*(md-1-Ycenter)*ycRotated;
        rRcorner+=detdist*zcRotated;
        
        angleCurrent=acos(rRcorner/Rcorner/R0);
        
        if (angleCurrent > angleMax ) angleMax=angleCurrent;
        if (angleCurrent < angleMin ) angleMin=angleCurrent;
    }
    
    //+++ (Xcenter, Ycenter)
    Rcorner= fabs( detdist );
    
    rRcorner=0;
    rRcorner+=0;
    rRcorner+=detdist*zcRotated;
    
    angleCurrent=acos(rRcorner/Rcorner/R0);
    
    
    if (angleCurrent < angleMin ) angleMin=0;
    
    double angleStep = 2.0*asin(minPixel*detelem/2.0/detdist);
    
    
    nTotal=int((angleMax-angleMin)/angleStep+0.5)+1;
    
    QQ=new double[nTotal];
    II=new double[nTotal];
    dII=new double[nTotal];
    IIcos2phi=new double[nTotal];
    nn=new double[nTotal];
    
    
    //+++ Step "0": all 0
    for(int ir=0;ir<nTotal;ir++)
    {
        QQ[ir]  = 4.0*M_PI/lambda*sin (0.5 * ( angleMin+angleStep*ir ) );
        II[ir] = 0;
        dII[ir]= 0;
        IIcos2phi[ir]= 0;
        nn[ir] =0;
    }
    
    //+++ Step "1": all points inside
    double R, rR, acosTh, phi;
    int currentPos;
    
    for (int iii=0;iii<md;iii++) for (int jjj=0;jjj<md;jjj++)
    {
        if (gsl_matrix_get(mask,iii,jjj)>0)
        {
            R= sqrt( detelem * (jjj-Xcenter)* detelem * (jjj-Xcenter)+pixelAsymetry*pixelAsymetry*detelem * detelem * (iii-Ycenter)*(iii-Ycenter) + detdist*detdist );
            
            rR=detelem*(jjj-Xcenter)*xcRotated;
            rR+=pixelAsymetry*detelem*(iii-Ycenter)*ycRotated;
            rR+=detdist*zcRotated;
            
            acosTh=acos(rR/R/R0);
            
            currentPos=int((acosTh-angleMin)/angleStep +0.5);
            
            if (currentPos>=nTotal) continue;
            
            II[currentPos]+=gsl_matrix_get(Sample,iii,jjj);
        dII[currentPos]+=gsl_matrix_get(SampleErr,iii,jjj)*gsl_matrix_get(Sample,iii,jjj)*gsl_matrix_get(Sample,iii,jjj);
            nn[currentPos]+=1;
            
            //+++ assymetry
            if( (iii-Ycenter)!=0) phi=atan( (jjj-Xcenter) / pixelAsymetry/ (iii-Ycenter));
            else
            {
                if ((jjj-Xcenter)>=0) phi=M_PI/2; else phi=1.5*M_PI;
            }
            phi+=angleAnisotropy;
            IIcos2phi[currentPos]+=gsl_matrix_get(Sample,iii,jjj)*cos(2*phi);
            //---
        }
    }
}

//+++ SD:: QI rad Av std log, det. ritation, ...
void dan18::radAvHF
(
 int md,
 gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask,
 double Xcenter, double Ycenter,
 double lambda, double detdist, double detelem,
 double pixelAsymetry, double DetRotationX, double DetRotationY, double angleAnisotropy,
 int &nTotal, double *&QQ, double *&II, double *&dII, double *&IIcos2phi, double *&nn
 )
{
    
    //+++
    int numRowsOut=0;
    int numAllQ=0;
    
    
    //------------------------------------
    // calculation of radial averaging
    //------------------------------------
    
    //+++ Xc & Yc center of detector
    double Xc=(double(md)-1.0)/2.0;
    double Yc=(double(md)-1.0)/2.0;
    
    //+++
    double R;
    double rmaxLocal;
    
    //+++  maximal distance from center to corner
    double rmax =  sqrt( (Xcenter)*(Xcenter) + pixelAsymetry*pixelAsymetry*( Ycenter)*(Ycenter));
    //+
    rmaxLocal = sqrt( ( (md-1) -Xcenter ) * ( (md-1) - Xcenter) + pixelAsymetry*pixelAsymetry*Ycenter * Ycenter );
    if (rmaxLocal>rmax) rmax = rmaxLocal;
    //+
    rmaxLocal = sqrt( ( (md-1) - Xcenter )*( (md-1) - Xcenter ) + pixelAsymetry*pixelAsymetry*( (md-1) - Ycenter )*( (md-1) - Ycenter) );
    if (rmaxLocal>rmax) rmax = rmaxLocal;
    //+
    rmaxLocal = sqrt( Xcenter*Xcenter + pixelAsymetry*pixelAsymetry*( (md-1) - Ycenter)*( (md-1) - Ycenter) );
    if (rmaxLocal>rmax) rmax = rmaxLocal;
    
    
    int ir,ii;                  //iR iPhi...
    int ix1,iy1,ix1old,iy1old,ix2,iy2;                //...
    int msk, phisteps;          //mask &
    
    double qr,phi,xs,ys,rex,rey;
    double wt,rad;
    
    double pi=M_PI;
    double  twt ;                       // totales Gewicht    >>>RESET<<<
    double avg;                 // Mittel
    double avge;                        //Fehler
    double anisotropy, phiAnisotropy;
    
    double sigmaErr;
    

    
    nTotal=int(rmax+0.5)+1;
    
    QQ=new double[nTotal];
    II=new double[nTotal];
    dII=new double[nTotal];
    IIcos2phi=new double[nTotal];
    nn=new double[nTotal];
    
    
    //+++ Step "0": all 0
    for(int ir=0;ir<nTotal;ir++)
    {
        QQ[ir]  = 0;
        II[ir]  = 0;
        dII[ir] = 0;
        IIcos2phi[ir]= 0;
        nn[ir]  =0;
    }
    
    
    for(int ir=0;ir<=int(rmax+0.5);ir++)
    {
        //      qr  = 2.*pi/lambda *  (ir*detelem/detdist);
        qr  = 4.0*M_PI/lambda *
        sin(0.5*atan(ir*detelem/detdist));  // test
        
        QQ[ir]=qr; //+++ 2022
        
        //+++ 09.09.10+++ rad =  detdist / detelem * tan (2.0*asin(0.5*ir*detelem/detdist) );  // echter rad
        rad=ir; //+++ 09.09.10+++
        
        
        
        //+++ 09.09.10+++ phisteps =int( GSL_MAX( int(4*pi*rad+0.5), 4) );
        phisteps =int( GSL_MAX( int(4*pi*rad+0.5), 1) );
        
        twt = 0.0;      //      ! totales Gewicht    >>>RESET<<<
        avg = 0.0;      //      ! Mittel
        avge= 0.0;      //      ! Fehler
        anisotropy=0.0; //      ! anisotropy
        
        ix1old=-1000;
        iy1old=-1000;
        
        for(ii=0; ii< phisteps; ii++)
        {
            //+++ Phi
            phi = 2.*pi*ii/phisteps;
            
            //+++
            xs  = Xcenter + rad * cos(phi);
            ys  = Ycenter + rad/pixelAsymetry * sin(phi);
            
            //+++
            R=detelem*sqrt((xs-Xc)*(xs-Xc)+(ys-Yc)*(ys-Yc));
            
            //+++
            ix1 = int(xs);      //         ! Eckpunkte
            iy1 = int(ys);
            if (ix1<0) ix1=ix1-1;
            if (iy1<0) iy1=iy1-1;
            
            
            ix1old=ix1;
            iy1old=iy1;
            
            ix2 = ix1 + 1;
            iy2 = iy1 + 1;
            
            
            rex = xs - ix1;     //  ! Reste f"ur Gewichtung sp"ater
            rey = ys - iy1;
            
            msk = 0;            //  ! linker unterer Punkt
            
            
            if (ix1>=0 && ix1<md && iy1>=0 && iy1<md)
            {
                //+++
                msk = int( gsl_matrix_get(mask,iy1,ix1) );
                wt  = msk*(1.0-rex)*(1.0-rey);
                twt += wt;
                avg += wt *gsl_matrix_get(Sample,iy1,ix1);
                
                //+++ err
                sigmaErr=gsl_matrix_get(SampleErr,iy1,ix1);
                avge += wt*wt*sigmaErr*gsl_matrix_get(Sample,iy1,ix1)*gsl_matrix_get(Sample,iy1,ix1);
                
                //+++ anisotropy 2022
                if ((iy1-Ycenter)!=0) phiAnisotropy=atan( (ix1-Xcenter) / pixelAsymetry / (iy1-Ycenter));
                else
                {
                    if ( (ix1-Xcenter)>=0 ) phiAnisotropy=M_PI/2; else phiAnisotropy=1.5*M_PI;
                }
                phiAnisotropy+=angleAnisotropy;
                anisotropy += wt * gsl_matrix_get(Sample,iy1,ix1)*cos(2*phiAnisotropy);
                //---
            }
            
            msk = 0;    //              ! rechter unterer Punkt
            
            if (ix2>=0 && ix2<md && iy1>=0 && iy1<md)
            {
                msk =int( gsl_matrix_get(mask,iy1,ix2)  );
                wt  = msk*(rex)*(1.0-rey);
                twt +=  wt;
                avg += wt*gsl_matrix_get(Sample,iy1,ix2);
                
                //+++err
                sigmaErr=gsl_matrix_get(SampleErr,iy1, ix2);
                avge += wt*wt*sigmaErr*gsl_matrix_get(Sample,iy1,
                                                      ix2)*gsl_matrix_get(Sample,iy1, ix2);
                
                //+++ anisotropy 2022
                if ((iy1-Ycenter)!=0) phiAnisotropy=atan( (ix2-Xcenter) / pixelAsymetry / (iy1-Ycenter));
                else
                {
                    if ( (ix2-Xcenter)>=0 ) phiAnisotropy=M_PI/2; else phiAnisotropy=1.5*M_PI;
                }
                phiAnisotropy+=angleAnisotropy;
                anisotropy += wt * gsl_matrix_get(Sample,iy1,ix2)*cos(2*phiAnisotropy);
                //---
            }
            
            msk = 0;//               ! linker oberer Punkt
            
            if (ix1>=0 && ix1<md && iy2>=0 && iy2<md)
            {
                msk =int( gsl_matrix_get(mask,iy2,ix1) );
                wt  = msk*(1.0-rex)*(rey);
                twt += wt;
                avg += wt*gsl_matrix_get(Sample,iy2,ix1);
                
                //+++err
                sigmaErr = gsl_matrix_get(SampleErr,iy2,ix1);
                avge += wt*wt*sigmaErr*gsl_matrix_get(Sample,iy2,ix1)*gsl_matrix_get(Sample,iy2,ix1);
                
                //+++ anisotropy 2022
                if ((iy2-Ycenter)!=0) phiAnisotropy=atan( (ix1-Xcenter) / pixelAsymetry / (iy2-Ycenter));
                else
                {
                    if ( (ix1-Xcenter)>=0 ) phiAnisotropy=M_PI/2; else phiAnisotropy=1.5*M_PI;
                }
                phiAnisotropy+=angleAnisotropy;
                anisotropy += wt * gsl_matrix_get(Sample,iy2,ix1)*cos(2*phiAnisotropy);
                //---
            }
            
            msk = 0;//               ! rechter oberer Punkt
            
            if (ix2>=0 && ix2<md && iy2>=0 && iy2<md)
            {
                msk =int( gsl_matrix_get(mask,iy2,ix2));
                wt  = msk*(rex)*(rey);
                twt += wt;
                avg += wt * gsl_matrix_get(Sample,iy2,ix2);
                
                //+++erri
                sigmaErr=gsl_matrix_get(SampleErr,iy2,ix2);
                
                avge += wt*wt*sigmaErr*gsl_matrix_get(Sample,iy2,ix2)*gsl_matrix_get(Sample,iy2,ix2);
                
                //+++ anisotropy 2022
                if ((iy2-Ycenter)!=0) phiAnisotropy=atan( (ix2-Xcenter) / pixelAsymetry / (iy2-Ycenter));
                else
                {
                    if ( (ix2-Xcenter)>=0 ) phiAnisotropy=M_PI/2; else phiAnisotropy=1.5*M_PI;
                }
                phiAnisotropy+=angleAnisotropy;
                anisotropy += wt * gsl_matrix_get(Sample,iy2,ix2)*cos(2*phiAnisotropy);
                //---
            }

            
        }
        
        
        II[ir]  = avg;
        dII[ir] = avge;
        nn[ir] = twt;
        IIcos2phi[ir] = anisotropy;
        

        
    }
    
}

//+++ SD:: QI data corrections
void dan18::radAvDataCorrectionOptions( int nTotal, double *&QQ, double *&II, double *&dII, double *&IIcos2phi, int *&mergedPoints,  double *&nn, int &nCurrent )
{
    nCurrent=nTotal;
    

    //+++ log/linear merging factors: stepProg/linearMergingFactor
    double stepProg=doubleSpinBoxAvLog->value();
    int linearMergingFactor=spinBoxAvlinear->value();;

    
    for(int ir=0;ir<nTotal;ir++) mergedPoints[ir]=1;
    
    if (stepProg>1.0)
    {
        int i=0;
        nCurrent=0;
        
        while(i<nTotal)
        {
            //+++ pluspoints
            double tm=pow(stepProg,i);
            int plusPoints=int(tm);
            if (tm-int(tm)>0.5) plusPoints++;
            
            if (i+plusPoints>=nTotal) plusPoints=nTotal-i;
            
            
            double IIlocal=0.0;
            double dIIlocal=0.0;
            double IIcos2philocal=0;
            double nnLocal=0;
            double qLocal=0;
            
            for (int ii=0;ii<plusPoints;ii++)
            {
                qLocal+=QQ[i+ii];
                nnLocal+=nn[i+ii];
                IIlocal+=II[i+ii];
                IIcos2philocal+=IIcos2phi[i+ii];
                dIIlocal+=dII[i+ii];
                
                nn[i+ii]=-1;
                mergedPoints[i+ii]=plusPoints;
                II[i+ii]=0.0;
                dII[i+ii]=0;
            }
            if (nnLocal==0 && linearMergingFactor==1) nn[i]=-1;
            else nn[i]=nnLocal;
            
            mergedPoints[i]=plusPoints;
            QQ[i]=qLocal/plusPoints;
            II[i]=IIlocal;
            dII[i]=dIIlocal;
            IIcos2phi[i]=IIcos2philocal;
            if (nn[i]>=0) nCurrent++;
            
            i=i+plusPoints;
        }
    }
    
    //+++ Step "2": linear merging
    if (linearMergingFactor>1)
    {
        int numberLinear=int( nTotal/linearMergingFactor+0.5 );
        

        nCurrent=0;
        
        for (int i=0; i<numberLinear;i++)
        {
            double IIlocal=0.0;
            double dIIlocal=0.0;
            double IIcos2philocal=0;
            int nnLocal=0.0;
            double qLocal=0.0;
            
            bool allNegative=true;
            int newNumberMergedPoints=0;
            
            for(int j=0;j<linearMergingFactor;j++)
            {
                if (nn[linearMergingFactor*i+j]<0) continue;
                if (linearMergingFactor*i+j>=nTotal) continue;
                allNegative=false;
                
                newNumberMergedPoints+=mergedPoints[linearMergingFactor*i+j];
                qLocal+=QQ[linearMergingFactor*i+j]*mergedPoints[linearMergingFactor*i+j];
                nnLocal+=nn[linearMergingFactor*i+j];
                IIlocal+=II[linearMergingFactor*i+j];
                dIIlocal+=dII[linearMergingFactor*i+j];
                IIcos2philocal+=IIcos2phi[linearMergingFactor*i+j];
                
                nn[linearMergingFactor*i+j]=-1;
                
                II[linearMergingFactor*i+j]=0.0;
                dII[linearMergingFactor*i+j]=0.0;
                IIcos2phi[linearMergingFactor*i+j]=0.0;
            }
            
            if(allNegative) continue;
            if(newNumberMergedPoints<=0) continue;
            if (nnLocal==0) nn[linearMergingFactor*i]=-1;
            else nn[linearMergingFactor*i]=nnLocal;
            if (nn[linearMergingFactor*i]>=0) nCurrent++;
            
            QQ[linearMergingFactor*i]=qLocal/newNumberMergedPoints;
            II[linearMergingFactor*i]=IIlocal;
            dII[linearMergingFactor*i]=dIIlocal;
            IIcos2phi[linearMergingFactor*i]=IIcos2philocal;
            
            for(int j=0;j<linearMergingFactor;j++ )
            {
                if (linearMergingFactor*i+j>=nTotal) continue;;
                mergedPoints[linearMergingFactor*i+j]=newNumberMergedPoints;
            }
            
        }
    }
    
    //if (linearMergingFactor==1.0 && stepProg==1.0 )
    for(int ir=0;ir<nTotal;ir++) if (nn[ir]==0) {nn[ir]=-1;nCurrent--;};
    
    //+++ Step "2": remove first points
    int skipFirst=spinBoxRemoveFirst->value();
    
    if (skipFirst>0)
    {
        if (skipFirst>nCurrent) skipFirst=nCurrent;
        int skippedFirst=0;
        
        for(int ir=0;ir<nTotal;ir++)
        {
            if (skippedFirst>=skipFirst)  break;
            if (nn[ir]<0) continue;
            
            nn[ir]=-1;
            skippedFirst++;
            nCurrent--;
        }
    }
    
    //+++ Step "4": remove last points
    int skipLast=spinBoxRemoveLast->value();
    
    if (skipLast>0)
    {
        if (skipLast>nCurrent) skipLast=nCurrent;
        int skippedLast=0;
        
        for(int ir=nTotal-1;ir>=0;ir--)
        {
            if (skippedLast>=skipLast)  break;
            if (nn[ir]<0) continue;
            
            nn[ir]=-1;
            skippedLast++;
            nCurrent--;
        }
    }
    
    //+++ step "5": negative points remove
    if ( checkBoxMaskNegativeQ->isChecked() )
    {
        for(int ir=0;ir<nTotal;ir++)
        {
            if (nn[ir]<0) continue;
            if (II[ir]>0) continue;
            
            nn[ir]=-1;
            nCurrent--;
        }
    }
}

//+++ SD:: QI table generation
void dan18::radAvTableGeneration( QString &sampleMatrix, QString label, int N, double *Q,double *I, double *dI, double *dQ, double *sigma, double *anisotropy)
{
    //+++ checkBoxAnisotropy
    bool checkBoxAnisotropyisChecked=checkBoxAnisotropy->isChecked();
    
    //+++ Presentation
    QString CurrentLabel=comboBoxSelectPresentation->currentText();
    
    //+++ Ext
    QString currentExt=lineEditFileExt->text().remove(" ");
    if(currentExt!="") currentExt+="-";
    
    //+++ Table Name
    QString tableOUT;
    tableOUT="QI-";
    if (CurrentLabel!="QI")tableOUT=tableOUT+CurrentLabel+"-";
    tableOUT=tableOUT+sampleMatrix;
    tableOUT=tableOUT.replace("QI-", "QI-"+currentExt);
    
    //+++ Rewrite or unique name
    if (!checkBoxRewriteOutput->isChecked())
    {
        tableOUT+="-";
        tableOUT=app()->generateUniqueName(tableOUT);
    }
    
    //+++ oneline data reduction
    if (sansTab->currentIndex()==1 && comboBoxCheck->currentText().contains("raw-QI"))
    {
        tableOUT=  "raw-QI";
    }
    else if (sansTab->currentIndex()==1 )
    {
        tableOUT= "raw-"+tableOUT;
    }
    
    //+++ Table Name
    sampleMatrix=tableOUT;
    
    //+++ exist table already?
    bool tableExist = app()->checkTableExistence(tableOUT);

    //+++ RAD-table
    Table *wOut;
    bool tableIsHidden = true;

    //+++ Init Table
    int colnumberInc=0;
    if (comboBox4thCol->currentIndex()<2)  colnumberInc=1;
    else if (comboBox4thCol->currentIndex()==3)  colnumberInc=2;
    
    if (checkBoxAnisotropyisChecked) colnumberInc++;
    
    //+++ table exist
    if (tableExist && checkBoxRewriteOutput->isChecked())
    {
        //+++ Find table
        QList<MdiSubWindow *> tableList = app()->tableList();
        foreach (MdiSubWindow *t, tableList)
            if (t->name() == tableOUT)
                wOut = (Table *)t;
        if (!wOut->isHidden())
            tableIsHidden = false;
        wOut->blockSignals(true);
        if (wOut->numRows() != N)
            wOut->setNumRows(N);
        if (wOut->numCols() < 3 + colnumberInc)
            wOut->setNumCols(3 + colnumberInc);
    }
    else
        wOut = app()->newHiddenTable(tableOUT, CurrentLabel, N, 3 + colnumberInc);
    
    app()->setListViewLabel(wOut->name(), label);
    app()->updateWindowLists(wOut);
    
    wOut->setColName(0,"Q");
    wOut->setColName(1,"I"); wOut->setColComment(1, label);
    wOut->setColName(2,"dI");
    wOut->setWindowLabel(label);
    
    wOut->setColPlotDesignation(2,Table::yErr);
    wOut->setHeaderColType();
    
    
    wOut->setColNumericFormat(2, 8, 0, true);
    wOut->setColNumericFormat(2, 8, 1, true);
    wOut->setColNumericFormat(2, 8, 2, true);
    
    if (comboBox4thCol->currentIndex()==0)
    {
        wOut->setColName(3,"Sigma");
        wOut->setColPlotDesignation(3,Table::xErr);
        wOut->setColNumericFormat(2, 8, 3, true);
    }
    else if (comboBox4thCol->currentIndex()==1)
    {
        wOut->setColName(3,"dQ");
        wOut->setColPlotDesignation(3,Table::xErr);
        wOut->setColNumericFormat(2, 8, 3, true);
    }
    else if (comboBox4thCol->currentIndex()==3)
    {
        wOut->setColName(3,"dQ");
        wOut->setColPlotDesignation(3,Table::xErr);
        wOut->setColNumericFormat(2, 8, 3, true);
        wOut->setColName(4,"Sigma");
        wOut->setColPlotDesignation(4,Table::xErr);
        wOut->setColPlotDesignation(4,Table::xErr);
        wOut->setColNumericFormat(2, 8, 4, true);
    }
    
    if (checkBoxAnisotropyisChecked)
    {
        wOut->setColName(3+colnumberInc-1,"Anisotropy"); wOut->setColComment(3+colnumberInc-1, label);
    }
    
    
    wOut->setHeaderColType();
    
    

    for(int i=0;i<N;i++)
    {

        wOut->setText(i,0,QString::number(Q[i],'E',8));
        wOut->setText(i,1,QString::number(I[i],'E',8));
        wOut->setText(i,2,QString::number(dI[i],'E',8));
    }
    
    //+++ Sigma output +++
    if (comboBox4thCol->currentIndex()==0)
    {
        for(int i=0;i<N;i++) wOut->setText(i,3, QString::number(sigma[i],'E',8));
    }
    else if (comboBox4thCol->currentIndex()==1)
    {
        for(int i=0;i<N;i++) wOut->setText(i,3, QString::number(dQ[i],'E',8));
    }
    else if (comboBox4thCol->currentIndex()==3)
    {
        for(int i=0;i<N;i++)
        {
        wOut->setText(i,3, QString::number(dQ[i],'E',8));
        wOut->setText(i,4, QString::number(sigma[i],'E',8));
        }
    }
    
    if (checkBoxAnisotropyisChecked) for(int i=0;i<N;i++) wOut->setText(i,3+colnumberInc-1, QString::number(anisotropy[i],'E',8));
    

    //+++ adjust cols width
    if (!tableExist)
        wOut->adjustColumnsWidth(false);

    //+++ on sygnals, and update existing curves on graphs
    if (tableExist && checkBoxRewriteOutput->isChecked())
    {
        wOut->blockSignals(false);
        wOut->notifyChanges();
        app()->showFullRangeAllPlots(wOut->name());
    }

    //+++  hide table
    if (tableIsHidden)
        app()->hideWindow(wOut);
}

//+++ SD:: QI ASCII-file generation
void dan18::radAvASCIIGeneration( QString &sampleMatrix, QString label, int N, double *Q,double *I, double *dI, double *dQ, double *sigma, double *anisotropy )
{
    //+++ checkBoxAnisotropy
    bool checkBoxAnisotropyisChecked=checkBoxAnisotropy->isChecked();
    
    //+++ Presentation
    QString CurrentLabel=comboBoxSelectPresentation->currentText();
    
    //+++ Ext
    QString currentExt=lineEditFileExt->text().remove(" ");
    if(currentExt!="") currentExt+="-";
    
    QFile f;
    QDir dd;
    
    QString asciiPath=lineEditPathRAD->text();
    if (checkBoxSortOutputToFolders->isChecked())
    {
        asciiPath+="/ASCII-QI";
        asciiPath=asciiPath.replace("//","/");
        if (!dd.cd(asciiPath))
        {
            dd.mkdir(asciiPath);
            
        };
    }
    
    QString fname="QI-";
    if (CurrentLabel!="QI") fname=fname+CurrentLabel+"-";
    fname=fname+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT";
    fname=fname.replace("QI-", "QI-"+currentExt);
    fname=asciiPath+"/"+fname;
    
    f.setFileName( fname );
    
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        f.close();
        return;
        
    }
    
    QTextStream stream( &f );
    
    //+++ header if
    if(comboBoxSelectPresentation->currentIndex()==0  && checkBoxASCIIheader->isChecked())
    {
        QString header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]";
        if (comboBox4thCol->currentIndex()==0)
        {
            header    = header +   "       Sigma[1/A]";
        }
        else if (comboBox4thCol->currentIndex()==1)
        {
            header    = header +   "          dQ[1/A]";
        }
        else if (comboBox4thCol->currentIndex()==2)
        {
            header    =  header;
        }
        else if (comboBox4thCol->currentIndex()==3)
        {
            header    =  header +   "          dQ[1/A]       Sigma[1/A]";
        }
        
        if (checkBoxAnisotropyisChecked) header    =  header +   "    Anisotropy[1]";
        
        header=header+"\n";
        
        stream<<header;
    }
    
    //+++ data export
    QString str;
    for(int i=0;i<N;i++)
    {
        str =      QString::number(Q[i],'E',8) + "   "+QString::number(I[i],'E',8) + "   "+QString::number(dI[i],'E',8);
        if (comboBox4thCol->currentIndex()==0)
        {

            str = str + "   " + QString::number(sigma[i],'E',8);
        }
        else if (comboBox4thCol->currentIndex()==1)
        {
            str = str + "   "+QString::number(dQ[i],'E',8);
        }
        else if (comboBox4thCol->currentIndex()==2)
        {
            str=str;
        }
        else if (comboBox4thCol->currentIndex()==3)
        {
            str = str + "   "+QString::number(dQ[i],'E',8) + "   "+QString::number(sigma[i],'E',8);
        }
        
        if (checkBoxAnisotropyisChecked) str = str + "   " + QString::number(anisotropy[i],'E',8);
        
        str = str+"\n";
        stream<<str;
    }
    
    
    f.close();
}

//+++ Horizontal Slice
void dan18::horizontalSlice
(
 int md,
 gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask,
 double Xcenter, double Ycenter,
 QString sampleMatrix,
 double C, double lambda, double deltaLambda, double detdist, double detelem, double r1, double r2,
 QString label
 )
{
    //+++
    int numRowsOut=0;
    double qr;
    //+++ Presentation
    QString CurrentLabel=comboBoxSelectPresentation->currentText();
    //+++ Ext
    QString currentExt=lineEditFileExt->text().remove(" ");
    if(currentExt!="") currentExt+="-";
    
    //+++ Table Name
    QString tableOUT;
    tableOUT="QXI-";
    
    if(CurrentLabel!="QI") tableOUT=tableOUT+CurrentLabel+"-";
    tableOUT=tableOUT+sampleMatrix;
    tableOUT=tableOUT.replace("QXI-","QXI-"+currentExt);
    
    
    if (!checkBoxRewriteOutput->isChecked())
    {
        tableOUT=tableOUT+"-";
        tableOUT=app()->generateUniqueName(tableOUT);
    }
    
    
    
    //+++ RAD-table
    Table *wOut;
    
    
    if (radioButtonOpenInProject->isChecked())
    {
        bool tableExist = app()->checkTableExistence(tableOUT);

        if (tableExist)
        {
            //+++ Find table
            QList<MdiSubWindow *> tableList=app()->tableList();
            foreach (MdiSubWindow *t, tableList) if (t->name()==tableOUT)  wOut=(Table *)t;
            //+++
            wOut->setNumRows(0);
            
            if (comboBox4thCol->currentIndex()==0) wOut->setNumCols(4);
            else wOut->setNumCols(3);
            
        }
        else
        {
            if (comboBox4thCol->currentIndex()==0)
            {
                wOut=app()->newHiddenTable(tableOUT,CurrentLabel, 0, 4);
                
                wOut->setColName(3,"Sigma");
                wOut->setColPlotDesignation(3,Table::xErr);
                wOut->setColNumericFormat(2, 8, 3, true);
            }
            else
            {
                wOut=app()->newHiddenTable(tableOUT,CurrentLabel, 0, 3);
                wOut->setColNumericFormat(2, 8, 3, true);
            }
            wOut->setColName(0,"Q");
            wOut->setColName(1,"I");
            wOut->table()->horizontalHeaderItem(1)->setText(label);
            wOut->setColName(2,"dI");
            
            wOut->setColNumericFormat(2, 8, 0, true);
            wOut->setColNumericFormat(2, 8, 1, true);
            wOut->setColNumericFormat(2, 8, 2, true);
            
            wOut->setWindowLabel(label);
            app()->setListViewLabel(wOut->name(), label);
            app()->updateWindowLists(wOut);
            
            wOut->setColPlotDesignation(2,Table::yErr);
            wOut->setHeaderColType();
        }
    }
    
    int nTotal=md;
    
    int xxx,yyy;
    
    double *II=new double[nTotal];
    double *dII=new double[nTotal];
    double *sigmaM=new double[nTotal];
    int *nn=new int[nTotal];
    
    for(xxx=0;xxx<nTotal;xxx++)
    {
        II[xxx]= 0;
        dII[xxx]= 0;
        nn[xxx]=0;
    }
    
    for (xxx=0;xxx<md;xxx++) for (yyy=0;yyy<md;yyy++)
    {
        if (gsl_matrix_get(mask,yyy,xxx)>0)
        {
            II  [xxx] += gsl_matrix_get(Sample,yyy,xxx);
            dII[xxx] += gsl_matrix_get(SampleErr,yyy,xxx)*gsl_matrix_get(Sample,yyy,xxx)*gsl_matrix_get(Sample,yyy,xxx);
            nn[xxx]+=1;
            
            qr  = 4.0*M_PI/lambda *
            sin(0.5*atan(sqrt( (Xcenter-xxx)*(Xcenter-xxx)+(Ycenter-yyy)*(Ycenter-yyy) )*detelem/detdist));
            sigmaM[xxx]+=sigma( qr, detdist, C, lambda, deltaLambda, r1, r2 );
        }
    }
    
    double avg, avge, sig;
    
    QString streamSTR="";
    
    //+++ new Skip Points
    int skipFirst=spinBoxRemoveFirst->value();
    int skipLast=spinBoxRemoveLast->value();
    
    for(xxx=0;xxx<nTotal;xxx++)
    {
        if (nn[xxx]>=1)
        {
            qr  = 4.0*M_PI/lambda * sin(0.5*atan((xxx-Xcenter)*detelem/detdist));
            avg = II[xxx] / nn[xxx];
            avge=sqrt(dII[xxx]) / nn[xxx];
            sig= sigmaM[xxx] / nn[xxx];
            
            
            //
            double Q,I,dI;
            double QQQ, III, dIII;
            Q=qr;
            I=avg;
            dI=avge;
            
            QQQ=qr;
            III=avg;
            dIII=avge;
            
            
            // * ****** Change of Presentation  *****************************
            if (CurrentLabel!="QI")
            {
                if (CurrentLabel=="Guinier")
                {
                    QQQ=Q*Q;
                    III=log(fabs(I));
                    if (I!=0) dIII=dI/fabs(I);
                }
                else if (CurrentLabel=="Zimm")
                {
                    QQQ=Q*Q;
                    if (I!=0) III=1/I; else III=0;
                    if (I!=0) dIII=dI/I/I; else dIII=0;
                }
                else if (CurrentLabel=="Porod")
                {
                    QQQ=log10(Q);
                    III=log10(fabs(I));
                    if (I!=0) dIII=dI/fabs(I)/log(10.0);
                }
                else if (CurrentLabel=="Porod2")
                {
                    QQQ=Q*Q;
                    III=I*pow(Q,4);
                    dIII=dI*pow(Q,4);
                }
                else if (CurrentLabel=="logQ")
                {
                    QQQ=log10(Q);
                }
                else if (CurrentLabel=="logI")
                {
                    
                    III=log10(fabs(I));
                    if (I!=0) dIII=dI/fabs(I)/log(10.0);
                }
                else if (CurrentLabel=="Debye")
                {
                    QQQ=Q*Q;
                    if (I!=0) III=1/sqrt(fabs(I)); else III=0;
                    if (I!=0) dIII=dI/pow(fabs(I),1.5); else dIII=0;
                }
                else if (CurrentLabel=="1Moment")
                {
                    III=Q*I;
                    dIII=Q*dI;
                }
                else if (CurrentLabel=="2Moment")
                {
                    III=Q*Q*I;
                    dIII=Q*Q*dI;
                }
                else if (CurrentLabel=="Kratky")
                {
                    III=Q*Q*I;
                    dIII=Q*Q*dI;
                }
                else if (CurrentLabel=="GuinierRod")
                {
                    QQQ=Q*Q;
                    III=log(fabs(I*Q));
                    if (I!=0) dIII=Q*fabs((dI/I)); else III=0;
                }
                else if (CurrentLabel=="GuinierPlate")
                {
                    QQQ=Q*Q;
                    III=log(fabs(I*Q*Q));
                    if (I!=0) dIII=Q*Q*fabs(dI/I); else III=0;
                }
            }
            
            if ( !(avg<=0 && checkBoxMaskNegativeQ->isChecked()) )
            {
                if (comboBox4thCol->currentIndex()==0)
                {
                    streamSTR+=QString::number(QQQ,'E',8)
                    +"   "+QString::number(III,'E',8)
                    +"   "+QString::number(dIII,'E',8)
                    +"   "+QString::number(sig,'E',8)+"\n";
                }
                else
                {
                    streamSTR +=QString::number(QQQ,'E',8)
                    +"   "+QString::number(III,'E',8)
                    +"   "+QString::number(dIII,'E',8)+"\n" ;
                }
            }
            
            //+++  to Table  +++
            if ( !(avg<=0 && checkBoxMaskNegativeQ->isChecked()) )
                if (radioButtonOpenInProject->isChecked())
                {
                    if (numRowsOut>=skipFirst)
                    {
                        wOut->setNumRows(numRowsOut-skipFirst+1);
                        wOut->setText(numRowsOut-skipFirst,0,QString::number(QQQ,'E',8));
                        wOut->setText(numRowsOut-skipFirst,1,QString::number(III,'E',8));
                        wOut->setText(numRowsOut-skipFirst,2,QString::number(dIII,'E',8));
                        if (comboBox4thCol->currentIndex()==0)
                        {
                            wOut->setText(numRowsOut-skipFirst,3,QString::number(sig,'E',8));
                        }
                    }
                    numRowsOut++;
                }
        }
    }
    
    //+++ remove last Points
    if (radioButtonOpenInProject->isChecked())
    {
        if (wOut->numRows()>=skipLast) wOut->setNumRows( wOut->numRows() - skipLast ); else wOut->setNumRows(0);
        if (wOut) wOut->notifyChanges();
    }
    
    int findN, tempI;
    for (findN=0;findN<skipFirst;findN++) {
        tempI=streamSTR.indexOf("\n"); streamSTR.remove(0,tempI+1);};
    for (findN=0;findN<skipLast;findN++) {
        tempI=streamSTR.lastIndexOf("\n",-3);
        streamSTR.remove(tempI+1,streamSTR.length()-tempI+2);};
    
    if (!radioButtonOpenInProject->isChecked())
    {
        QFile f;
        QString fname;
        
        if (checkBoxSortOutputToFolders->isChecked())	
        {	
            QDir dd;
            if (!dd.cd(lineEditPathRAD->text()+"/ASCII-QXI")) 
            {	
                dd.mkdir(lineEditPathRAD->text()+"/ASCII-QXI"); 			
            }
            fname=lineEditPathRAD->text()+"/ASCII-QXI/QXI-";
        }
        else fname=lineEditPathRAD->text()+"/QXI-";
        
        
        if (CurrentLabel!="QI") fname=fname+CurrentLabel+"-";
        
        fname=fname+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT";		fname=fname.replace("QXI-","QXI-"+currentExt);
        
        
        f.setFileName( fname );
        if ( f.open( QIODevice::WriteOnly ) )
        {
            QTextStream stream( &f );
            
            if(comboBoxSelectPresentation->currentIndex()==0 && checkBoxASCIIheader->isChecked())
            {
                QString header;
                if (comboBox4thCol->currentIndex()==0)
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]       Sigma[1/A]\n";
                }
                else 
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]\n";
                }
                
                stream<<header;
            }
            
            
            stream<<streamSTR;
            
            f.close();
        }
    }
    
    delete[] II;
    delete[] dII;
    delete[] nn;
    delete[] sigmaM;
}



//+++ Vertical Slice
void dan18::verticalSlice
(
 int md,
 gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask,
 double Xcenter, double Ycenter,
 QString sampleMatrix,
 double C, double lambda, double deltaLambda, double detdist, double detelem, double r1, double r2,
 QString label
 )
{
    //+++
    int numRowsOut=0;
    double qr;
    //+++ Presentation
    QString CurrentLabel=comboBoxSelectPresentation->currentText();
    
    //+++ Ext
    QString currentExt=lineEditFileExt->text().remove(" ");
    if(currentExt!="") currentExt+="-";
    
    //+++ Table Name
    QString tableOUT;
    tableOUT="QYI-";
    
    if(CurrentLabel!="QI")tableOUT=tableOUT+CurrentLabel+"-";
    tableOUT=tableOUT+sampleMatrix;
    tableOUT=tableOUT.replace("QYI-","QYI-"+currentExt);
    
    
    if (!checkBoxRewriteOutput->isChecked())
    {
        tableOUT=tableOUT+"-";
        tableOUT=app()->generateUniqueName(tableOUT);
    }
    
    
    //+++ RAD-table
    Table *wOut;
    
    
    if (radioButtonOpenInProject->isChecked())
    {
        bool tableExist = app()->checkTableExistence(tableOUT);

        if (tableExist)
        {
            //+++ Find table
            QList<MdiSubWindow *> tableList=app()->tableList();
            foreach (MdiSubWindow *t, tableList) if (t->name()==tableOUT)  wOut=(Table *)t;
            //+++
            wOut->setNumRows(0);
            
            if (comboBox4thCol->currentIndex()==0) wOut->setNumCols(4);
            else wOut->setNumCols(3);
            
        }
        else
        {
            if (comboBox4thCol->currentIndex()==0)
            {
                wOut=app()->newHiddenTable(tableOUT,CurrentLabel, 0, 4);
                
                wOut->setColName(3,"Sigma");
                wOut->setColPlotDesignation(3,Table::xErr);
                wOut->setColNumericFormat(2, 8, 3, true);
            }
            else
            {
                wOut=app()->newHiddenTable(tableOUT,CurrentLabel, 0, 3);
                wOut->setColNumericFormat(2, 8, 3, true);
            }
            wOut->setColName(0,"Q");
            wOut->setColName(1,"I");
            wOut->table()->horizontalHeaderItem(1)->setText(label);
            wOut->setColName(2,"dI");
            
            wOut->setColNumericFormat(2, 8, 0, true);
            wOut->setColNumericFormat(2, 8, 1, true);
            wOut->setColNumericFormat(2, 8, 2, true);
            
            wOut->setWindowLabel(label);
            app()->setListViewLabel(wOut->name(), label);
            app()->updateWindowLists(wOut);
            
            wOut->setColPlotDesignation(2,Table::yErr);
            wOut->setHeaderColType();
        }
    }
    
    int nTotal=md;
    
    int xxx,yyy;
    
    double *II=new double[nTotal];
    double *dII=new double[nTotal];
    double *sigmaM=new double[nTotal];
    int *nn=new int[nTotal];
    
    int ir;
    
    for(ir=0;ir<nTotal;ir++)
    {
        II[ir]= 0;
        dII[ir]= 0;
        nn[ir]=0;
    }
    
    for (yyy=0;yyy<md;yyy++) for (xxx=0;xxx<md;xxx++)
    {
        if (gsl_matrix_get(mask,yyy,xxx)>0)
        {
            II  [yyy] += gsl_matrix_get(Sample,yyy,xxx);
            dII[yyy] += gsl_matrix_get(SampleErr,yyy,xxx)*gsl_matrix_get(Sample,yyy,xxx)*gsl_matrix_get(Sample,yyy,xxx);
            nn[yyy]+=1;
            
            
            
            qr  = 4.0*M_PI/lambda *
            sin(0.5*atan(sqrt( (Xcenter-xxx)*(Xcenter-xxx)+(Ycenter-yyy)*(Ycenter-yyy) )*detelem/detdist));
            sigmaM[yyy]+=sigma( qr, detdist, C, lambda, deltaLambda, r1, r2 );
        }
    }
    
    double avg, avge, sig;
    
    QString streamSTR="";
    
    //+++ new Skip Points
    int skipFirst=spinBoxRemoveFirst->value();
    int skipLast=spinBoxRemoveLast->value();
    
    for(yyy=0;yyy<nTotal;yyy++)
    {
        if (nn[yyy]>=1)
        {
            qr  = 4.0*M_PI/lambda * sin(0.5*atan((yyy-Ycenter)*detelem/detdist));
            avg = II[yyy] / nn[yyy];
            avge=sqrt(dII[yyy]) / nn[yyy];
            sig= sigmaM[yyy] / nn[yyy];
            
            
            //
            double Q,I,dI;
            double QQQ, III, dIII;
            Q=qr;
            I=avg;
            dI=avge;
            
            QQQ=qr;
            III=avg;
            dIII=avge;
            
            
            // * ****** Change of Presentation  *****************************
            if (CurrentLabel!="QI")
            {
                if (CurrentLabel=="Guinier")
                {
                    QQQ=Q*Q;
                    III=log(fabs(I));
                    if (I!=0) dIII=dI/fabs(I);
                }
                else if (CurrentLabel=="Zimm")
                {
                    QQQ=Q*Q;
                    if (I!=0) III=1/I; else III=0;
                    if (I!=0) dIII=dI/I/I; else dIII=0;
                }
                else if (CurrentLabel=="Porod")
                {
                    QQQ=log10(Q);
                    III=log10(fabs(I));
                    if (I!=0) dIII=dI/fabs(I)/log(10.0);
                }
                else if (CurrentLabel=="Porod2")
                {
                    QQQ=Q*Q;
                    III=I*pow(Q,4);
                    dIII=dI*pow(Q,4);
                }
                else if (CurrentLabel=="logQ")
                {
                    QQQ=log10(Q);
                }
                else if (CurrentLabel=="logI")
                {
                    
                    III=log10(fabs(I));
                    if (I!=0) dIII=dI/fabs(I)/log(10.0);
                }
                else if (CurrentLabel=="Debye")
                {
                    QQQ=Q*Q;
                    if (I!=0) III=1/sqrt(fabs(I)); else III=0;
                    if (I!=0) dIII=dI/pow(fabs(I),1.5); else dIII=0;
                }
                else if (CurrentLabel=="1Moment")
                {
                    III=Q*I;
                    dIII=Q*dI;
                }
                else if (CurrentLabel=="2Moment")
                {
                    III=Q*Q*I;
                    dIII=Q*Q*dI;
                }
                else if (CurrentLabel=="Kratky")
                {
                    III=Q*Q*I;
                    dIII=Q*Q*dI;
                }
                else if (CurrentLabel=="GuinierRod")
                {
                    QQQ=Q*Q;
                    III=log(fabs(I*Q));
                    if (I!=0) dIII=Q*fabs((dI/I)); else III=0;
                }
                else if (CurrentLabel=="GuinierPlate")
                {
                    QQQ=Q*Q;
                    III=log(fabs(I*Q*Q));
                    if (I!=0) dIII=Q*Q*fabs(dI/I); else III=0;
                }
            }
            
            if ( !(avg<=0 && checkBoxMaskNegativeQ->isChecked()) )
            {
                if (comboBox4thCol->currentIndex()==0)
                {
                    streamSTR+=QString::number(QQQ,'E',8)
                    +"   "+QString::number(III,'E',8)
                    +"   "+QString::number(dIII,'E',8)
                    +"   "+QString::number(sig,'E',8)+"\n";
                }
                else
                {
                    streamSTR +=QString::number(QQQ,'E',8)
                    +"   "+QString::number(III,'E',8)
                    +"   "+QString::number(dIII,'E',8)+"\n" ;
                }
            }
            //+++  to Table  +++
            if ( !(avg<=0 && checkBoxMaskNegativeQ->isChecked()) )
                if (radioButtonOpenInProject->isChecked())
                {
                    if (numRowsOut>=skipFirst)
                    {
                        wOut->setNumRows(numRowsOut-skipFirst+1);
                        wOut->setText(numRowsOut-skipFirst,0,QString::number(QQQ,'E',8));
                        wOut->setText(numRowsOut-skipFirst,1,QString::number(III,'E',8));
                        wOut->setText(numRowsOut-skipFirst,2,QString::number(dIII,'E',8));
                        if (comboBox4thCol->currentIndex()==0)
                        {
                            wOut->setText(numRowsOut-skipFirst,3,QString::number(sig,'E',8));
                        }
                    }
                    numRowsOut++;
                }
        }
    }
    
    //+++ remove last Points
    if (radioButtonOpenInProject->isChecked())
    {
        if (wOut->numRows()>=skipLast) wOut->setNumRows( wOut->numRows() -
                                                        skipLast ); else wOut->setNumRows(0);
        if (wOut) wOut->notifyChanges();
    }
    
    int findN, tempI;
    for (findN=0;findN<skipFirst;findN++) {
        tempI=streamSTR.indexOf("\n"); streamSTR.remove(0,tempI+1);};
    for (findN=0;findN<skipLast;findN++) {
        tempI=streamSTR.lastIndexOf("\n",-3);
        streamSTR.remove(tempI+1,streamSTR.length()-tempI+2);};
    
    if (!radioButtonOpenInProject->isChecked())
    {
        QFile f;
        
        QString fname;
        
        if (checkBoxSortOutputToFolders->isChecked())	
        {	
            QDir dd;
            if (!dd.cd(lineEditPathRAD->text()+"/ASCII-QYI")) 
            {	
                dd.mkdir(lineEditPathRAD->text()+"/ASCII-QYI"); 			
            }
            fname=lineEditPathRAD->text()+"/ASCII-QYI/QYI-";
        }
        else fname=lineEditPathRAD->text()+"/QYI-";
        
        
        if (CurrentLabel!="QI") fname=fname+CurrentLabel+"-";
        
        fname=fname+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT";
        fname=fname.replace("QYI-","QYI-"+currentExt);
        
        f.setFileName( fname );
        if ( f.open( QIODevice::WriteOnly ) )
        {
            QTextStream stream( &f );
            
            
            
            if(comboBoxSelectPresentation->currentIndex()==0 && checkBoxASCIIheader->isChecked())
            {
                QString header;
                if (comboBox4thCol->currentIndex()==0)
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]       Sigma[1/A]\n";
                }
                else 
                {
                    header    =  "###     Q[1/A]          I[1/cm]         dI[1/cm]\n";
                }
                
                stream<<header;
            }
            
            
            stream<<streamSTR;
            
            f.close();
        }
    }
    
    delete[] II;
    delete[] dII;
    delete[] nn;
    delete[] sigmaM;
}

//+++ SD:: RAD-UniPolar
void dan18::radUniPolar
(
 int md,
 gsl_matrix *Sample, gsl_matrix *mask,
 double Xcenter, double Ycenter,
 QString sampleMatrix,
 double lambda, double detdist, double detelem, double pixelAsymetry
 ) //+++ TODOAsymetry
{
    
    //------------------------------------
    // calculation of radial averaging
    //------------------------------------
    
    double Xc=(double(md)-1.0)/2.0;
    double Yc=(double(md)-1.0)/2.0;
    
    double R;
    
    double rmax =  sqrt( (Xcenter)*(Xcenter) + ( Ycenter)*(Ycenter));
    rmax = GSL_MAX(
                   rmax,
                   sqrt(   ( (md-1) - Xcenter ) * ( (md-1) -
                                                   Xcenter ) + (Ycenter)*(Ycenter)   )
                   );
    rmax = GSL_MAX(
                   rmax,
                   sqrt( ( (md-1) - Xcenter ) * ( (md-1) - Xcenter ) + ( (md-1) - Ycenter)*((md-1) - Ycenter)  ) );
    rmax = GSL_MAX(
                   rmax,
                   sqrt( ( Xcenter ) * ( Xcenter ) + ( ( md-1 ) - Ycenter ) * ( (md - 1 ) - Ycenter )  ) );
    

    int ix1,iy1,ix2,iy2;                //...
    int msk, phisteps;          //mask &
    
    double qr,phi,xs,ys,rex,rey;
    double wt,rad;
    
    double pi=M_PI;
    
    double Iphi, twtPhi;
    
    //+++ Ext
    QString currentExt=lineEditFileExt->text().remove(" ");
    if(currentExt!="") currentExt+="-";
    
    //+++ Table Name (Phi)
    QString tablePhi;
    tablePhi="Polar-"+currentExt+sampleMatrix+"-Phi";
    
    if (!checkBoxRewriteOutput->isChecked())
    {
        tablePhi+="-";
        tablePhi=app()->generateUniqueName(tablePhi);
    }
    
    
    //+++ Phi steps
    phisteps = spinBoxPolar->value();
    
    //+++ skip 1st row and column
    bool skipFirst = checkBoxSkipPolar->isChecked();

    int addColRow = 0;
    if (!skipFirst)
        addColRow = 1;

    //+++ matrix Definition
    gsl_matrix *matrixPolar = gsl_matrix_calloc(phisteps + addColRow, lround(rmax) + addColRow);

    double qs, qe, phis=0, phie;

    //+++ R-scan
    for (int ir = 1; ir <= lround(rmax); ir++)
    {
        qr = 2.0 * pi / lambda * (ir * detelem / detdist);
        rad = detdist / detelem * tan(2.0 * asin(0.5 * ir * detelem / detdist)); // rad

        for (int ii = 0; ii < phisteps; ii++)
        {
            //+++ Phi
            phi = 2.*pi*ii/phisteps;
            
            //+++
            xs  = Xcenter + rad * cos(phi-pi/2);
            ys  = Ycenter + rad * sin(phi-pi/2);
            
            //+++
            R=detelem*sqrt((xs-Xc)*(xs-Xc)+(ys-Yc)*(ys-Yc));
            
            //+++
            ix1 = int(xs);      //         ! Eckpunkte
            iy1 = int(ys);
            ix2 = ix1 + 1;
            iy2 = iy1 + 1;
            
            rex = xs - ix1;     //        ! Reste f"ur Gewichtung sp"ater
            rey = ys - iy1;
            
            msk = 0;    //        ! linker unterer Punkt
            Iphi=0;
            twtPhi=0;
            
            if (ix1>=0 && ix1<md && iy1>=0 && iy1<md)
            {
                //+++
                msk = int( gsl_matrix_get(mask,iy1,ix1) );
                wt  = msk*(1.0-rex)*(1.0-rey);
                //+++
                Iphi+=wt *gsl_matrix_get(Sample,iy1,ix1);
                twtPhi+=wt;
            }
            
            msk = 0;    //              ! rechter unterer Punkt
            
            if (ix2>=0 && ix2<md && iy1>=0 && iy1<md)
            {
                msk =int( gsl_matrix_get(mask,iy1,ix2)  );
                wt  = msk*(rex)*(1.0-rey);
                //+++ I vs Phi
                Iphi+=wt *gsl_matrix_get(Sample,iy1,ix2);
                twtPhi += wt;
            }
            
            msk = 0;//               ! linker oberer Punkt
            
            if (ix1>=0 && ix1<md && iy2>=0 && iy2<md)
            {
                msk =int( gsl_matrix_get(mask,iy2,ix1) );
                wt  = msk*(1.0-rex)*(rey);
                //+++ I vs Phi
                Iphi += wt *gsl_matrix_get(Sample,iy2,ix1);
                twtPhi += wt;
            }
            
            msk = 0;//               ! rechter oberer Punkt
            
            if (ix2>=0 && ix2<md && iy2>=0 && iy2<md)
            {
                msk =int( gsl_matrix_get(mask,iy2,ix2));
                wt  = msk*(rex)*(rey);
                //+++ I vs Phi
                Iphi += wt *gsl_matrix_get(Sample,iy2,ix2);
                twtPhi += wt;
            }
            
            
            if (twtPhi>0)
            {
                int shiftAngle=ii+int( double(0.5 * double(phisteps)));
                if (shiftAngle>=phisteps) shiftAngle-=phisteps;
                
                shiftAngle+=int( double(spinBoxPolarShift->value()) / 360.0 * double(phisteps));
                if (shiftAngle>=phisteps) shiftAngle-=phisteps;
                shiftAngle=phisteps-1-shiftAngle;

                gsl_matrix_set(matrixPolar, shiftAngle + addColRow, ir - 1 + addColRow, Iphi / twtPhi);
            }
            else
                gsl_matrix_set(matrixPolar, ii + addColRow, ir - 1 + addColRow, 0);
            if (ii == 0 && !skipFirst)
                gsl_matrix_set(matrixPolar, 0, ir - 1 + addColRow, qr);
            if (ir == 1 && !skipFirst)
                gsl_matrix_set(matrixPolar, ii + 1, 0, phi);
            if (ii == 0 && ir == 1)
                qs = qr;
            if (ii == phisteps - 1 && ir == lround(rmax) - 1)
            {
                qe = qr;
                phie = phi;
            }
        }
    }
    
    if (radioButtonOpenInProject->isChecked())     
    {
        if (!skipFirst)
            makeMatrixUni(matrixPolar, tablePhi, int(lround(rmax)) + addColRow, phisteps + addColRow, 0.5,
                          double(lround(rmax)) + addColRow + 0.5, 0.5, phisteps + 0.5 + addColRow, true);
        else
            makeMatrixUni(matrixPolar, tablePhi, int(lround(rmax)), phisteps, qs, qe,
                          (phie - 2 * M_PI) / 2.0 / M_PI * 180, (phie + 2 * M_PI) / 2.0 / M_PI * 180, true);
    }
    else
    {
        if (checkBoxSortOutputToFolders->isChecked())	
        {	
            QDir dd;
            if (!dd.cd(lineEditPathRAD->text()+"/ASCII-POLAR")) 
            {	
                dd.mkdir(lineEditPathRAD->text()+"/ASCII-POLAR"); 			
            }
            saveMatrixToFile(lineEditPathRAD->text()+"/ASCII-POLAR/Polar-"+currentExt+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT",matrixPolar, phisteps,1+int(rmax+0.5));
        }
        else saveMatrixToFile(lineEditPathRAD->text()+"/Polar-"+currentExt+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT",matrixPolar, phisteps,1+int(rmax+0.5));
        
    }
    
    
    gsl_matrix_free(matrixPolar);
}


void dan18::sigmaGslMatrix
(
 gsl_matrix *sigmaMa, int md, gsl_matrix *mask, double Xcenter, double Ycenter,
 double lambda, double deltaLambda, double C, double detdist, double detelem, double r1, double r2
 )
{
    //+++
    
    //------------------------------------
    // calculation of radial averaging
    //------------------------------------
    double R, Q;
    
    for (int xxx=0;xxx<md;xxx++)    for (int yyy=0;yyy<md;yyy++)
    {
        //+++
        R=detelem*sqrt((xxx-Xcenter)*(xxx-Xcenter)+(yyy-Ycenter)*(yyy-Ycenter));
        Q  = 4.0*M_PI/lambda * sin(0.5*atan(R/detdist));
        gsl_matrix_set(sigmaMa,yyy,xxx, sigma( Q, detdist, C, lambda, deltaLambda,  r1, r2 ));
    }
    
    gsl_matrix_mul_elements(sigmaMa,mask);
}

//+++
void dan18::sigmaMatrix
(
 int md, gsl_matrix *mask,
 double Xcenter, double Ycenter,
 QString sampleMatrix,
 double lambda, double deltaLambda, double C, double detdist, double detelem, double r1, double r2
 )
{
    //+++
    
    //------------------------------------
    // calculation of radial averaging
    //------------------------------------
    double R, Q;
    
    gsl_matrix* sigmaMa=gsl_matrix_alloc(md,md);
    
    for (int xxx=0;xxx<md;xxx++)    for (int yyy=0;yyy<md;yyy++)
    {
        //+++
        R=detelem*sqrt((xxx-Xcenter)*(xxx-Xcenter)+(yyy-Ycenter)*(yyy-Ycenter));
        Q  = 4.0*M_PI/lambda * sin(0.5*atan(R/detdist));
        gsl_matrix_set(sigmaMa,yyy,xxx, sigma( Q, detdist, C, lambda, deltaLambda,  r1, r2 ));
    }
    
    gsl_matrix_mul_elements(sigmaMa,mask);
    
    
    if (radioButtonOpenInProject->isChecked()) makeMatrixSymmetric(sigmaMa,sampleMatrix,"SIGMA::Matrix",md, true);
    else
    {
        if (checkBoxSortOutputToFolders->isChecked())
        {
            QDir dd;
            if (!dd.cd(lineEditPathRAD->text()+"/ASCII-Sigma"))
            {
                dd.mkdir(lineEditPathRAD->text()+"/ASCII-Sigma");
            }
            saveMatrixToFile(lineEditPathRAD->text()+"/ASCII-Sigma/"+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT",sigmaMa, md);
        }
        else saveMatrixToFile(lineEditPathRAD->text()+"/"+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT",sigmaMa, md);
        
    }
    
    gsl_matrix_free(sigmaMa);
}


//+++
void dan18::MatrixQ
(
 int md, gsl_matrix *mask,
 double Xcenter, double Ycenter,
 QString sampleMatrix,
 double lambda, double detdist, double detelem, double pixelAsymetry, double DetectorHShiftAngle, double DetectorVShiftAngle
 )
{
    //------------------------------------
    // calculation of radial averaging
    //------------------------------------
    double R, rR, cosTh, Q;
    
    
    
    gsl_matrix* matrixQ=gsl_matrix_alloc(md, md);
    
    
    DetectorHShiftAngle=DetectorHShiftAngle/180.0*M_PI; // phi
    DetectorVShiftAngle=DetectorVShiftAngle/180.0*M_PI;  //thetha
    
    double xcRotated=detdist*sin(DetectorHShiftAngle)*cos(DetectorVShiftAngle);
    double ycRotated=detdist*sin(DetectorVShiftAngle)*cos(DetectorHShiftAngle);
    double zcRotated=detdist*cos(DetectorHShiftAngle)*cos(DetectorVShiftAngle);
    
    double R0=sqrt(xcRotated*xcRotated + ycRotated*ycRotated + zcRotated*zcRotated);
    
    for (int xxx=0;xxx<md;xxx++)    for (int yyy=0;yyy<md;yyy++)
    {
        //+++
        //R=detelem*sqrt((xxx-Xcenter)*(xxx-Xcenter)+pixelAsymetry*pixelAsymetry*(yyy-Ycenter)*(yyy-Ycenter));
        
        R= sqrt( detelem * (xxx-Xcenter)* detelem * (xxx-Xcenter)+pixelAsymetry*pixelAsymetry*detelem * detelem * (yyy-Ycenter)*(yyy-Ycenter) + detdist*detdist );
        
        rR=detelem*(xxx-Xcenter)*xcRotated;
        rR+=pixelAsymetry*detelem*(yyy-Ycenter)*ycRotated;
        rR+=detdist*zcRotated;
        
        cosTh=acos(rR/R/R0);

        Q  = 4.0*M_PI/lambda * sin(0.5*cosTh);
        gsl_matrix_set(matrixQ,yyy,xxx, Q);
    }
    
    gsl_matrix_mul_elements(matrixQ,mask);
    
    if (radioButtonOpenInProject->isChecked()) makeMatrixSymmetric(matrixQ,sampleMatrix,"Q::Matrix", md, true);
    else
    {
        if (checkBoxSortOutputToFolders->isChecked())
        {	QDir dd;
            if (!dd.cd(lineEditPathRAD->text()+"/ASCII-Q"))
            {
                dd.mkdir(lineEditPathRAD->text()+"/ASCII-Q");
            }
            saveMatrixToFile(lineEditPathRAD->text()+"/ASCII-Q/"+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT",matrixQ, md);
        }
        else saveMatrixToFile(lineEditPathRAD->text()+"/"+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT",matrixQ, md);
        
    }
    
    gsl_matrix_free(matrixQ);
}


//+++
void dan18::gslMatrixWaTrDet ( int md, double Tr,  gsl_matrix *sensTrDet, double Xcenter, double Ycenter, double detdist, double detelem, double pixelAsymetry)
{
    //------------------------------------
    // Det Sens Correction
    //------------------------------------
    double R, cosTheta;
    
    
    for (int xxx=0;xxx<md;xxx++)    for (int yyy=0;yyy<md;yyy++)
    {
        //+++
        R=detelem*sqrt((xxx-Xcenter)*(xxx-Xcenter)+pixelAsymetry*pixelAsymetry*(yyy-Ycenter)*(yyy-Ycenter));
        cosTheta  = cos( atan(R/detdist) );
        if (cosTheta==0 ) gsl_matrix_set(sensTrDet,yyy,xxx, 0);
        else gsl_matrix_set(sensTrDet,yyy,xxx, pow(Tr, 1.0-1.0/cosTheta));
    }
    
}

//+++
void dan18::dQmatrix
(
 int md, gsl_matrix *mask,
 double Xcenter, double Ycenter,
 QString sampleMatrix,
 double lambda, double detdist, double detelem, double pixelAsymetry )
{
    //+++
    
    //------------------------------------
    // calculation of radial averaging
    //------------------------------------
    double R, Q;
    double epsilonR;
    double epsilonLambda=0.01;
    double epsilonD=1.0/detdist;
    gsl_matrix* matrixQ=gsl_matrix_alloc(md, md);
    
    for (int xxx=0;xxx<md;xxx++)    for (int yyy=0;yyy<md;yyy++)
    {
        //+++
        R=detelem*sqrt((xxx-Xcenter)*(xxx-Xcenter)+pixelAsymetry*pixelAsymetry*(yyy-Ycenter)*(yyy-Ycenter));
        Q  = 4.0*M_PI/lambda * sin(0.5*atan(R/detdist));
        epsilonR=2.0*M_PI*detelem/Q/lambda/detdist;
        gsl_matrix_set(matrixQ,yyy,xxx, Q*sqrt(epsilonR*epsilonR+epsilonLambda*epsilonLambda+epsilonD*epsilonD));
    }
    
    gsl_matrix_mul_elements(matrixQ,mask);
    
    if (radioButtonOpenInProject->isChecked()) makeMatrixSymmetric(matrixQ,sampleMatrix,"dQ::Matrix", md, true);
    else
    {
        if (checkBoxSortOutputToFolders->isChecked())
        {	QDir dd;
            if (!dd.cd(lineEditPathRAD->text()+"/ASCII-dQ"))
            {
                dd.mkdir(lineEditPathRAD->text()+"/ASCII-dQ");
            }
            saveMatrixToFile(lineEditPathRAD->text()+"/ASCII-dQ/"+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT",matrixQ, md);
        }
        else saveMatrixToFile(lineEditPathRAD->text()+"/"+sampleMatrix+"-"+comboBoxSel->currentText()+".DAT",matrixQ, md);
        
    }    
    gsl_matrix_free(matrixQ);
}

//*******************************************
//+++ Structure to Find Center
//*******************************************
struct structCenter
{
    gsl_matrix * corund;
    gsl_matrix * mask;
    int dim;
};


//+++ Xc function Int
double functionToFindXcenterInt (int Xc, void * params)
{
    gsl_matrix *corund  = ((struct structCenter *)params)->corund;
    gsl_matrix *mask    = ((struct structCenter *)params)->mask;
    int dim             = ((struct structCenter *)params)->dim;
    
    double result=0;
    int pixels=0;
    double temp;
    double left,right;
    
    int maxPoints=GSL_MIN( (dim-1-Xc) , Xc) - 10; //+++
    
    for (int yy=10; yy<(dim-10);yy++)
    {
        for (int xx=0; xx<maxPoints;xx++)
        {
            left=gsl_matrix_get(corund, yy, Xc-1.0-xx);
            right=gsl_matrix_get(corund, yy, Xc+1.0+xx);
            
            temp=gsl_matrix_get(mask, yy, Xc-xx-1)*gsl_matrix_get(mask, yy, Xc+xx+1);
            
            pixels+=int(temp);
            
            temp*=(right-left)*(right-left);
            
            result+=temp;
        }
    }
    return result/pixels;
}


//+++ Yc function Int
double functionToFindYcenterInt (int Yc, void * params)
{
    gsl_matrix *corund  = ((struct structCenter *)params)->corund;
    gsl_matrix *mask    = ((struct structCenter *)params)->mask;
    int dim             = ((struct structCenter *)params)->dim;
    
    double result=0;
    int pixels=0;
    double temp;
    double left,right;
    
    int maxPoints=GSL_MIN( (dim-1-Yc), Yc) - 10; //+++
    
    for (int xx=10; xx<(dim-10);xx++)
    {
        for (int yy=0; yy<maxPoints;yy++)
        {
            left=gsl_matrix_get(corund, Yc-1-yy, xx);
            right=gsl_matrix_get(corund, Yc+1+yy, xx);
            
            temp=gsl_matrix_get(mask, Yc-yy-1,xx)*gsl_matrix_get(mask, Yc+yy+1,xx);
            
            pixels+=temp;
            
            temp*=(right-left)*(right-left);
            
            result+=temp;
        }
    }
    return result/pixels;
}


//+++ Xc function
double functionToFindXcenter (double Xc, void * params)
{
    gsl_matrix *corund  = ((struct structCenter *)params)->corund;
    gsl_matrix *mask    = ((struct structCenter *)params)->mask;
    int dim             = ((struct structCenter *)params)->dim;
    
    double result=0;
    int pixels=0;
    double temp;
    double left,right;
    double delta=Xc-(int)Xc;
    int maxPoints=30;//GSL_MIN((dim-1-(int)Xc), (int)Xc) - 15; //+++
    
    for (int yy=15; yy<(dim-15);yy++)
    {
        for (int xx=0; xx<maxPoints;xx++)
        {
            left=(1-delta)*gsl_matrix_get(corund, yy, (int)(Xc)-xx-1)+
            (delta)*gsl_matrix_get(corund, yy, (int)(Xc)-xx);
            right=(1-delta)*gsl_matrix_get(corund, yy, (int)(Xc)+xx+1)+
            (delta)*gsl_matrix_get(corund, yy, (int)(Xc)+xx+2);
            
            temp=gsl_matrix_get(mask, yy, (int)(Xc)-xx-1);
            temp*=gsl_matrix_get(mask, yy, (int)(Xc)-xx);
            temp*=gsl_matrix_get(mask, yy, (int)(Xc)+xx+1);
            temp*=gsl_matrix_get(mask, yy, (int)(Xc)+xx+2);
            
            pixels+=temp;
            
            temp*=fabs(right);
            
            result+=temp;
        }
    }
    return result/pixels;
}

double functionToFindYcenter (double Yc, void * params)
{
    gsl_matrix *corund  = ((struct structCenter *)params)->corund;
    gsl_matrix *mask    = ((struct structCenter *)params)->mask;
    int dim             = ((struct structCenter *)params)->dim;
    
    double result=0.0;
    int pixels=0;
    double temp;
    double left,right;
    double delta=Yc-(int)(Yc);
    int maxPoints=GSL_MIN((dim-1-(int)Yc), (int)Yc )-15; //+++
    
    for (int xx=15; xx<(dim-15);xx++)
    {
        for (int yy=0; yy<maxPoints;yy++)
        {
            left=(1-delta)*gsl_matrix_get(corund, (int)(Yc)-yy-1, xx)+
            (delta)*gsl_matrix_get(corund, (int)(Yc)-yy,xx);
            right=(1-delta)*gsl_matrix_get(corund, (int)(Yc)+yy+1,xx)+
            (delta)*gsl_matrix_get(corund, (int)(Yc)+yy+2,xx);
            
            
            temp=gsl_matrix_get(mask, (int)(Yc)-yy-1, xx);
            temp*=gsl_matrix_get(mask, (int)(Yc)-yy,xx);
            temp*=gsl_matrix_get(mask, (int)(Yc)+yy+1, xx);
            temp*=gsl_matrix_get(mask, (int)(Yc)+yy+2,xx);
            pixels+=temp;
            temp*=(right-left)*(right-left);
            result+=temp;
        }
    }
    return result/pixels;
}


//+++++SLOT::calculate centere+++++++++++++++++++++++++++++++++++++++++++++++++++++
void dan18::calcCenterNew()
{
    int i;
    
    // Number of C-D conditionws
    int Nconditions=tableEC->columnCount();
    
    // find center for each C-D condition
    for(i=0;i<Nconditions;i++)
    {
        calcCenterNew(i);
    }
}


//+++++SLOT::calculate centere+++++++++++++++++++++++++++++++++++++++++++++++++++++
void dan18::calcCenterNew(int col)
{
    int MD = lineEditMD->text().toInt();

    //Variables definitin
    QString D;
    double Xc, Yc;
    double XcErr=0.0, YcErr=0.0;
    QString sampleFile=tableEC->item(dptCENTER,col)->text();

    if (radioButtonCenterReadFromHeader->isChecked())
    {
        Xc = detector->readDetectorX(sampleFile);
        Yc = detector->readDetectorY(sampleFile);
    }
    else
    {
        // find center for each C-D condition
        Xc=tableEC->item(dptCENTERX,col)->text().left(tableEC->item(dptCENTERX,col)->text().indexOf(QChar(177))).toDouble();
        
        if (Xc<(MD/20.0) || Xc>(19.0*MD/20.0)) Xc=(MD+1.0)/2.0;
        Yc=tableEC->item(dptCENTERY,col)->text().left(tableEC->item(dptCENTERY,col)->text().indexOf(QChar(177))).toDouble();
        if (Yc<(MD/20.0) || Yc>(19.0*MD/20.0)) Yc=(MD+1.0)/2.0;
        
        D=tableEC->item(dptD,col)->text();
        
        QString maskName=comboBoxMaskFor->currentText();
        QString sensName=comboBoxSensFor->currentText();
        
        //+++ Mask & Sens +++
        QStringList listMask, listSens;
        QString winLabelMask="DAN::Mask::"+QString::number(MD);
        QString winLabelSens="DAN::Sensitivity::"+QString::number(MD);
        
        findMatrixListByLabel(winLabelMask, listMask);
        if (!listMask.contains("mask")) listMask.prepend("mask");
        findMatrixListByLabel(winLabelSens, listSens);
        if (!listSens.contains("sens")) listSens.prepend("sens");
        
        //+++ mask reading +++
        if (listMask.contains(((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText()))
        {
            maskName=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText();
        }
        
        //+++ mask reading +++
        if (listSens.contains(((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText()))
        {
            sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
        }
        
        calcCenterNew(sampleFile, col, Xc, Yc, XcErr, YcErr, maskName, sensName);
    }
    
    tableEC->item(dptCENTERX,col)->setText(QString::number(Xc,'f',3) +QChar(177)+QString::number(XcErr,'f',3));
    tableEC->item(dptCENTERY,col)->setText(QString::number(Yc,'f',3)+QChar(177)+QString::number(YcErr,'f',3));
}


//+++++SLOT::calculate centere+++++++++++++++++++++++++++++++++++++++++++++++++++++
void dan18::calcCenterNew(QString sampleFile, int col, double &Xc, double &Yc, double &XcErr,
                          double &YcErr, QString maskName, QString sensName)
{
    int MD = lineEditMD->text().toInt();

    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_all(mask, 1.0);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_all(sens, 1.0);
    make_GSL_Matrix_Symmetric( sensName, sens, MD);
    
    //Variables definitin
    bool existCorund;
    
    //Matrix allocation
    gsl_matrix *corund=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(corund);
    
    //Check existence of Corund File
    existCorund = filesManager->checkFileNumber(sampleFile);

    if (!existCorund)
    {
        QMessageBox::warning(this,tr("qtiSAS"), tr(QString("C-D-Lambda condition # "+QString::number(col+1)+":: corund-file does not exist!").toLocal8Bit().constData()));
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: corund-file does not exist!"+"\n");
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(corund);
        
        return;
    }
    
    // read corund matrix
    readMatrixCor( sampleFile, corund );
    
    
    //fileNumber=BCFile.toInt();
    gsl_matrix_mul_elements(corund,mask);
    gsl_matrix_mul_elements(corund,sens);
    
    if (radioButtonRadStdSymm->isChecked())
    {
        
        calcCenterUniSymmetryX(MD, corund, mask, Xc, XcErr);
        calcCenterUniSymmetryY(MD, corund, mask, Yc, YcErr);
    }
    else
    {
        calcCenterUniHF(MD, corund, mask, Xc, Yc, XcErr, YcErr );
    }
    
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    
    gsl_matrix_free(corund);
}


//+++++SLOT::calculate X centere Uni Symmetry++++++++++++
void dan18::calcCenterUniSymmetryX
(
 int md, gsl_matrix *corund, gsl_matrix *mask, double &Xc,
 double &XcErr
 )
{
    
    const int numberPoints=2;
    
    int i;
    const int nnnnnnnn=2*numberPoints+1;
    double xi, x[nnnnnnnn], y[nnnnnnnn];
    
    structCenter stct={corund,mask,md};
    
    double min=0.0;
    int imin=0;
    double fun;
    
    int old35=int(md/4.0)+3;
    
    for (i = old35; i <= (md-old35); i++)
    {
        
        fun= functionToFindXcenterInt(i, &stct);
        
        if (i==old35 ||  fun < min ) {imin=i; min=fun;};
    }
    
    if (imin<=(old35+numberPoints) || imin >md-old35-numberPoints)
    {
        Xc=(md+1.0)/2.0;
        XcErr=50.000;
    }
    else
    {
        for (i = 0; i <= (2*numberPoints); i++)
        {
            x[i]=imin-numberPoints+i;
            y[i]=functionToFindXcenterInt(imin-numberPoints+i, &stct);
        }
        
        
        gsl_interp_accel    *acc    = gsl_interp_accel_alloc();
        gsl_spline          *spline = gsl_spline_alloc(gsl_interp_cspline, 2*numberPoints+1);
        
        gsl_spline_init (spline, x, y, 2*numberPoints+1);
        
        double xMin = x[numberPoints-1];
        min=y[numberPoints-1];
        
        for (xi = x[numberPoints-1]; xi <= x[numberPoints+1]; xi += 0.01)
        {
            if ( gsl_spline_eval (spline, xi, acc) < min ) { xMin=xi; min =
                gsl_spline_eval (spline, xi, acc);};
        }
        
        Xc=xMin+1.0;
        
        if ( imin==(int)Xc )
            XcErr= fabs( fabs( (y[numberPoints+1]-min)/
                              (y[numberPoints+2]-y[numberPoints+1]) ) + fabs (
                                                                              (y[numberPoints]-min)/ (y[numberPoints-1]-y[numberPoints]) ) )/10.0;
        else
            XcErr= fabs ( fabs( (y[numberPoints]-min)/
                               (y[numberPoints+1]-y[numberPoints]) ) + fabs (
                                                                             (y[numberPoints-1]-min)/ (y[numberPoints-2]-y[numberPoints-1]) ) )/10.0;
        
        gsl_spline_free(spline);
        gsl_interp_accel_free(acc);
    }
}


//+++++SLOT::calculate X centere Uni Symmetry++++++++++++++++
void dan18::calcCenterUniSymmetryY
(
 int md, gsl_matrix *corund, gsl_matrix *mask, double &Yc,
 double &YcErr
 )
{
    
    
    const int numberPoints=2;
    
    int i;
    const int nnnnnnnn=2*numberPoints+1;
    double xi, x[nnnnnnnn], y[nnnnnnnn];
    
    structCenter stct={corund,mask,md};
    
    double min=0.0;
    int imin=0;
    double fun;
    
    int old35=int(md/4.0)+3;
    
    for (i = old35; i <= (md-old35); i++)
    {
        fun= functionToFindYcenterInt(i, &stct);
        if (i==old35 || fun < min ) {imin=i; min=fun;};
    }
    
    if (imin<=(old35+numberPoints) || imin >md-old35-numberPoints)
    {
        Yc=(md+1.0)/2.0;
        YcErr=50.000;
    }
    else
    {
        for (i = 0; i <= (2*numberPoints); i++)
        {
            x[i]=imin-numberPoints+i;
            y[i]=functionToFindYcenterInt(imin-numberPoints+i, &stct);
        }
        
        gsl_interp_accel    *acc    = gsl_interp_accel_alloc();
        gsl_spline          *spline = gsl_spline_alloc(gsl_interp_cspline, 2*numberPoints+1);
        
        gsl_spline_init (spline, x, y, 2*numberPoints+1);
        
        double xMin = x[numberPoints-1];
        
        min=y[numberPoints-1];
        
        for (xi = x[numberPoints-1]; xi <= x[numberPoints+1]; xi += 0.01)
        {
            // printf ("%4.2f %E\n", xi, gsl_spline_eval (spline, xi, acc));
            if ( gsl_spline_eval (spline, xi, acc) < min ) { xMin=xi; min =
                gsl_spline_eval (spline, xi, acc);};
        }
        
        Yc=xMin+1.0;
        
        if ( imin==(int)xMin )
            YcErr= fabs( fabs( (y[numberPoints+1]-min)/
                              (y[numberPoints+2]-y[numberPoints+1]))  + fabs (
                                                                              (y[numberPoints]-min)/ (y[numberPoints-1]-y[numberPoints]) ) )/10.0;
        else
            YcErr= fabs ( fabs ((y[numberPoints]-min)/
                                (y[numberPoints+1]-y[numberPoints]) ) + fabs( (y[numberPoints-1]-min)/
                                                                             (y[numberPoints-2]-y[numberPoints-1]) ) )/10.0;
        
        gsl_spline_free(spline);
        gsl_interp_accel_free(acc);
    }
    
    
}



//+++ Resolution Function: Sigma +++
double dan18::sigma( double Q, double D, double C, double Lambda, double deltaLambda,
                    double r1, double r2)
{
    return sigmaQmerged(Q, D, C, Lambda, deltaLambda, r1, r2, 1);
}

double dan18::sigmaQmerged( double Q, double D, double C, double Lambda, double deltaLambda,
                           double r1, double r2, int numberMergedQ)
{
    double PI =M_PI;
    double pixel=lineEditResoPixelSize->text().toDouble();
    double pixelAsymetry  = lineEditAsymetry->text().toDouble();
    
    double binning=comboBoxBinning->currentText().toDouble();
    double Rdet    =numberMergedQ*binning*(1.0+pixelAsymetry)/2.0*pixel/M_SQRTPI;      //[cm]
    
    //+++ new
    double RdetReso=lineEditDetReso->text().toDouble()/M_SQRTPI;
    if (RdetReso>Rdet) Rdet=RdetReso;
    //---
    
    double SigQ2,SigQ;
    
    if (checkBoxResoFocus->isChecked())
    {
        C=D;
        double f=D/2.0;
        SigQ2=PI*PI/Lambda/Lambda/3.0*(3.0*r1*r1/C/C+2.0*r2*r2/f/f*deltaLambda*deltaLambda+Rdet*Rdet/D/D+Lambda*Lambda/2.0/2.0/PI/PI*Q*Q*deltaLambda*deltaLambda);
        
        SigQ=2*0.4246609*sqrt(SigQ2);
        return SigQ;
    }
    
    
    
    double Theta0,cos2Theta02, a1, a2, beta1;
    double K0                 = 2.0*PI/Lambda;   //[A-1]
    
    //
    Theta0          = asin(Q/2.0/K0);
    cos2Theta02     = cos(2.0*Theta0)*cos(2.0*Theta0);
    
    //+++ Sigma-D1  +++
    double SigmaDetector      = K0*cos(Theta0)*cos2Theta02*Rdet/D;       //[A-1]
    
    //
    if (cos2Theta02!=0.0) a1 = r1/(C+D/cos2Theta02); else a1 =0.0;
    a2 = r2*cos2Theta02/D;
    
    //
    if (a2>a1)
    {
        beta1   =2.0*r2*(1.0/C+cos2Theta02/D)-r1*r1*D/(2.0*r2*cos2Theta02*C*(C+D/cos2Theta02));
    }
    else
    {
        beta1   =2.0*r1/C-0.5*r2*r2/r1*cos2Theta02*cos2Theta02*(C+D/cos2Theta02)*(C+D/cos2Theta02)/C/D/D;
    }
    
    //+++ Sigma-W  +++
    SigQ2        =Q*Q*deltaLambda*deltaLambda;
    //+++ Sigma-D1  +++
    SigQ2       +=SigmaDetector*SigmaDetector;
    //+++ Sigma-C1  +++
    SigQ2       +=K0*K0*cos(Theta0)*cos(Theta0)*beta1*beta1;
    //+++  Sigma-AV  +++
    SigQ2       +=K0*K0*cos(Theta0)*cos(Theta0)*cos2Theta02*cos2Theta02*Rdet/D*Rdet/D;
    //
    SigQ         =0.4246609*sqrt(SigQ2); //[A-1];
    
    return SigQ;
}


//+++ Q-error +++
double dan18::dQ( double Q, int numberF , double d, double D, double pixel )
{
    double deltaLambda=0;
    if (numberF>100) deltaLambda=1/sqrt(double(numberF));
    else deltaLambda=0.01;
    
    double deltaSmalD=0;
    if (d>0) deltaSmalD=0.5*pixel/d;
    
    double deltaD=0;
    if (D<50.0) deltaD=1.0/D;
    else if (D<400.0) deltaD=2.0/D;
    else deltaD=5.0/D;
    
    
    return Q * sqrt ( deltaLambda*deltaLambda + deltaD*deltaD + deltaSmalD*deltaSmalD );
}

//+++++SLOT::calculate centere/Uni-HF+++++++++++++++++++++++++++++++++++++++++++++++++++++
void dan18::calcCenterUniHF(int md, gsl_matrix *corund,
                            gsl_matrix *mask, double &Xc, double &Yc, double &XcErr, double &YcErr
                            )
{
    // Henrich code
    double Xcenter=Xc-1.0;         //Initial Value
    double Ycenter=Yc-1.0;
    int MinX=0;                         // maximum range for center value (hard to define)
    int MinY=0;
    int MaxX=md-1;
    int MaxY=md-1;
    
    double Xstep=100.0;
    double Ystep=100.0;
    int totStep=0;
    
    double MinStep=0.001;       // Constant, smallest step when loop ends
    int ix,iy;
    
    while(totStep<=50 && (fabs(Xstep)>=MinStep || fabs(Ystep)>=MinStep))
    {
        double MomX=0.0;
        double MomY=0.0;
        double Norm=0.0;
        
        int mask1, mask2, imx, imy;
        
        
        for(ix=0;ix<md;ix++) for(iy=0;iy<md;iy++)
        {
            
            mask1=int( gsl_matrix_get(mask, iy, ix) );
            
            mask2=0;
            imx=int(2.0*Xcenter-ix+0.5);
            imy=int(2.0*Ycenter-iy+0.5);
            
            if ( imx>=0 && imx<md && imy>=0 && imy<md)
                mask2 = int( gsl_matrix_get(mask,imy,imx) );
            
            if ((mask1*mask2) != 0)
            {
                MomX += (ix-Xcenter)*gsl_matrix_get(corund,iy,ix);
                MomY += (iy-Ycenter)*gsl_matrix_get(corund,iy,ix);
                Norm +=gsl_matrix_get(corund,iy,ix);
            }
        }
        
        
        
        if (Norm>0.0)
        {
            Xstep = MomX / Norm;
            Ystep = MomY / Norm;
        }
        else
        {
            Xstep = 0.0;
            Ystep = 0.0;
        }
        
        if ((Xcenter+Xstep)>MaxX)
        {
            Xstep  = MaxX - Xcenter;
            Xcenter = MaxX;
        }
        else if ((Xcenter+Xstep)< MinX)
        {
            Xstep   = MinX - Xcenter;
            Xcenter = MinX;
        }
        else
        {
            Xcenter = Xcenter + Xstep;
        }
        
        if ((Ycenter+Ystep) > MaxY)
        {
            Ystep   = MaxY - Ycenter;
            Ycenter = MaxY;
        }
        else  if ((Ycenter+Ystep)<MinY)
        {
            Ystep   = MinY - Ycenter;
            Ycenter = MinY;
        }
        else
        {
            Ycenter = Ycenter + Ystep;
        }
        
        totStep = totStep + 1;
    }
    Xc=Xcenter+1.0;
    Yc=Ycenter+1.0;
    
    // !!!!!! Error-bar calculation for Xc
    int loop_test=15;
    int loop_PM=int(loop_test/2);
    int ii;
    
    gsl_vector *XcNear=gsl_vector_alloc(loop_test);
    gsl_vector *YcNear=gsl_vector_alloc(loop_test);
    
    gsl_vector *M1Xc=gsl_vector_alloc(loop_test);
    gsl_vector *M1Yc=gsl_vector_alloc(loop_test);
    
    for (ii=0; ii<loop_test;ii++)
    {
        Xcenter=Xc-1+(ii-loop_PM)*0.5;
        gsl_vector_set(XcNear,ii,Xcenter);
        Ycenter=Yc-1+(ii-loop_PM)*0.5;
        gsl_vector_set(YcNear,ii,Ycenter);
    }
    
    double ys=0.0, xs=0.0,yys=0.0, xxs=0.0,xys=0.0;
    
    for (ii=0; ii<loop_test;ii++)
    {
        Xcenter=Xc-1+(ii-loop_PM)*0.5;
        Ycenter=Yc-1;
        
        double MomX=0.0;
        double MomY=0.0;
        double Norm=0.0;
        
        int mask1, mask2, imx, imy;
        for(ix=0;ix<md;ix++) for(iy=0;iy<md;iy++)
        {
            mask1=int ( gsl_matrix_get(mask,iy,ix) );
            mask2=0;
            imx=int(2.0*Xcenter-ix+0.5);
            imy=int(2.0*Ycenter-iy+0.5);
            
            if ( imx>=0 && imx<md && imy>=0 && imy<md)
                mask2 = int( gsl_matrix_get(mask,imy,imx) );
            
            if ((mask1*mask2) != 0)
            {
                MomX += (ix-Xcenter)*gsl_matrix_get(corund,iy,ix);
                MomY += (iy-Ycenter)*gsl_matrix_get(corund,iy,ix);
                Norm +=gsl_matrix_get(corund,iy,ix);
            }
        }
        if (Norm>0.0)
        {
            MomX = MomX / Norm;
            MomY = MomY / Norm;
        }
        
        gsl_vector_set(M1Xc,ii, MomX);
        
        ys+=  Xcenter;
        xs+=  MomX;
        yys+= Xcenter*Xcenter;
        xxs+= MomX*MomX;
        xys+= MomX*Xcenter;
    }
    
    
    double Delta = loop_test*xxs - xs*xs;
    double Aordi = (xxs*ys-xs*xys)/Delta;
    double Bslop = (loop_test*xys-xs*ys )/Delta;
    
    double  ssig=0.0;
    for (ii=0; ii<loop_test;ii++)
    {
        ssig += pow( (gsl_vector_get(XcNear,ii) - Aordi -
                      Bslop*gsl_vector_get(M1Xc,ii)), 2);
    }
    
    //+++
    XcErr=sqrt(ssig*xxs/Delta);
    
    ys=0.0; xs=0.0; yys=0.0; xxs=0.0; xys=0.0;
    
    for (ii=0; ii<loop_test;ii++)
    {
        Ycenter=Yc-1+(ii-loop_PM)*0.5;
        Xcenter=Xc-1;
        
        double MomX=0.0;
        double MomY=0.0;
        double Norm=0.0;
        
        int mask1, mask2, imx, imy;
        
        for(ix=0;ix<md;ix++) for(iy=0;iy<md;iy++)
        {
            mask1=int( gsl_matrix_get(mask,iy,ix) );
            mask2=0;
            imx=int(2.0*Xcenter-ix+0.5);
            imy=int(2.0*Ycenter-iy+0.5);
            
            
            if ( imx>=0 && imx<md && imy>=0 && imy<md)
                mask2 = int( gsl_matrix_get(mask,imy,imx) );
            
            if ((mask1*mask2) != 0)
            {
                MomX += (ix-Xcenter)*gsl_matrix_get(corund,iy,ix);
                MomY += (iy-Ycenter)*gsl_matrix_get(corund,iy,ix);
                Norm +=gsl_matrix_get(corund,iy,ix);
            }
        }
        if (Norm>0.0)
        {
            MomX = MomX / Norm;
            MomY = MomY / Norm;
        }
        
        gsl_vector_set(M1Yc,ii, MomY);
        
        ys+=  Ycenter;
        xs+=  MomY;
        yys+= Ycenter*Ycenter;
        xxs+= MomY*MomY;
        xys+= MomY*Ycenter;
    }
    
    Delta = loop_test*xxs - xs*xs;
    Aordi = (xxs*ys-xs*xys)/Delta;
    Bslop = (loop_test*xys-xs*ys )/Delta;
    
    ssig=0.0;
    for (ii=0; ii<loop_test;ii++)
    {
        ssig += pow( (gsl_vector_get(YcNear,ii) - Aordi -
                      Bslop*gsl_vector_get(M1Yc,ii)), 2);
    }
    
    //+++
    YcErr=sqrt(ssig*xxs/Delta);
    
    
    gsl_vector_free(XcNear);
    gsl_vector_free(YcNear);
    gsl_vector_free(M1Xc);
    gsl_vector_free(M1Yc);
    
}

//*******************************************
//+++ dan
//*******************************************
bool dan18::danDanMultiButtonSingleLine(    QString button,
                                            QString label, QString Nsample,QString NEC, QString NBC, QString Nbuffer, QString maskName, QString sensName,
                                            double Detector, double C, double Lambda,
                                            double trans, double transBuffer, double fractionBuffer, double thickness,
                                            double abs0, double Xcenter, double Ycenter,
                                            double scale, double BackgroundConst,double VShift,double HShift
                                        )
{
    int MD = lineEditMD->text().toInt();

    //+++ 2022
    double detRotationX = detector->readDetRotationX(Nsample);
    double detRotationY = detector->readDetRotationY(Nsample);

    //+++ Output data Suffix
    QString dataSuffix;
    switch (comboBoxMode->currentIndex())
    {
        case 0:
            dataSuffix="SM";
            break;
        case 1:
            dataSuffix="BS";
            break;
        case 2:
            dataSuffix="BS-SENS";
            break;
        case 3:
            dataSuffix="SM";
            break;
        case 4:
            dataSuffix="SM";
            break;
        default:
            dataSuffix="SM";
            break;
    }
    
    //+++ Subtract Bufffer
    bool subtractBuffer= false;
    if (comboBoxMode->currentText().contains("(BS)")) subtractBuffer=true;
    
    if (fractionBuffer==0.0) subtractBuffer= false;
    
    //+++ result string info for current row
    QString status="... "+Nsample+" ";
    
    //+++ mask gsl matrix
    gsl_matrix *mask = gsl_matrix_calloc(MD,MD);
    if (!make_GSL_Matrix_Symmetric(maskName, mask, MD))
    {
//        gsl_matrix_free(mask);
//        status+=">>> no mask [e01]";
                gsl_matrix_set_all(mask,1.0);
//        toResLog("DAN :: "+status);
//        return false;
    }
    
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_calloc(MD,MD);
    gsl_matrix *sensErr=gsl_matrix_calloc(MD,MD);
    
    if (!make_GSL_Matrix_Symmetric(sensName, sens, MD))
    {
//        gsl_matrix_free(mask);
//        gsl_matrix_free(sens);
//        gsl_matrix_free(sensErr);
//        status+=">>> no sens [e02]";
        gsl_matrix_set_all(sens,1.0);
        gsl_matrix_set_all(sensErr,1.0);
//        toResLog("DAN :: "+status);
//        return false;
    }
    
    //+++
    sensAndMaskSynchro(mask, sens, MD );
    
    //+++ I-Qx +  >>>>
    if (button=="I-Qx")
    {
        bool slicesBS=checkBoxSlicesBS->isChecked();
        int from=spinBoxFrom->value();
        if(slicesBS && spinBoxLTyBS->value()>from) from=spinBoxLTyBS->value();
        int to=spinBoxTo->value();
        if(slicesBS && spinBoxRByBS->value()<to) to=spinBoxRByBS->value();
        if (from <=to && from>0 && to<=MD)
        {
            for (int xxx=0; xxx<MD; xxx++ ) for (int yyy=0; yyy<(from-1); yyy++ ) gsl_matrix_set(mask,yyy,xxx, 0.0 );
            for (int xxx=0; xxx<MD; xxx++ ) for (int yyy=to; yyy<MD; yyy++ ) gsl_matrix_set(mask,yyy,xxx, 0.0 );
        }
    }
    //--- I-Qx - >>>>
    
    //+++ I-Qy + >>>>
    if (button=="I-Qy")
    {
        bool slicesBS=checkBoxSlicesBS->isChecked();
        int from=spinBoxFrom->value();
        if(slicesBS && spinBoxLTxBS->value()>from) from=spinBoxLTxBS->value();
        int to=spinBoxTo->value();
        if(slicesBS && spinBoxRBxBS->value()<to) to=spinBoxRBxBS->value();
        if (from <=to && from>0 && to<=MD)
        {
            for (int yyy=0; yyy<MD; yyy++ ) for (int xxx=0; xxx<(from-1); xxx++ ) gsl_matrix_set(mask,yyy,xxx, 0.0 );
            for (int yyy=0; yyy<MD; yyy++ ) for (int xxx=to; xxx<MD; xxx++ ) gsl_matrix_set(mask,yyy,xxx, 0.0 );
        }
    }
    //---- I-Qy -  >>>>
    
    
    //+++ Sensetivty Error Matrix
    QString sensFile=getSensitivityNumber(sensName);
    gsl_matrix_set_zero (sensErr);
    if (sensFile != "" && filesManager->checkFileNumber(sensFile))
        readErrorMatrix(sensFile, sensErr);

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //+++ All actions only in case Sample Exists                   !!!!!!!!!!!!!!!!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    if (!filesManager->checkFileNumber(Nsample))
    {
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(sensErr);
        status+=">>> file "+Nsample+" not found; output:: no [e03]";
        toResLog("DAN :: "+status+"\n");
        return false;
    }
    
    //+++ sample matrixes
    gsl_matrix *Sample=gsl_matrix_calloc(MD,MD);
    gsl_matrix *SampleErr=gsl_matrix_calloc(MD,MD);
    
    //+++
    readMatrixCor( Nsample, Sample);
    readErrorMatrix( Nsample, SampleErr);
    
    gslMatrixShift(Sample, MD, HShift, VShift );
    gslMatrixShift(SampleErr, MD, HShift, VShift );
    
    //+++
    status=">>>  sample#="+Nsample+": OK  ";
    
    
    
    //+++ buffer matrixes
    gsl_matrix *Buffer=gsl_matrix_calloc(MD,MD);
    gsl_matrix *BufferErr=gsl_matrix_calloc(MD,MD);
    
    if (subtractBuffer)
    {
        if (!filesManager->checkFileNumber(Nbuffer))
        {
            gsl_matrix_set_all(Buffer,0.0);
            gsl_matrix_set_all(BufferErr,0.0);
            subtractBuffer=false;
            status+=">>> no buffer [e04]";
        }
        //+++
        readMatrixCor( Nbuffer, Buffer);
        readErrorMatrix( Nbuffer, BufferErr);
    }
    
    
    //+++ EC +++
    bool ECexist=false;
    
    //+++ ec matrixes
    gsl_matrix *EC=gsl_matrix_calloc(MD,MD);
    gsl_matrix *ECErr=gsl_matrix_calloc(MD,MD);

    if (filesManager->checkFileNumber(NEC))
    {
        readMatrixCor( NEC,EC);
        readErrorMatrix( NEC, ECErr);
        status+=">>>  EC#=" + NEC + ": OK  ";
        //+++
        ECexist=true;
    }
    else status+=">>>  EC: no correction  ";
    
    //+++ BC +++
    
    //+++ bc matrixes
    gsl_matrix *BC=gsl_matrix_alloc(MD,MD);
    gsl_matrix *BCErr=gsl_matrix_alloc(MD,MD);

    if (filesManager->checkFileNumber(NBC))
    {
        // read BC matrix 2012
        if (checkBoxBCTimeNormalization->isChecked())
        {
            readMatrixCorTimeNormalizationOnly( NBC, BC );
            
            //Normalization constant
            double TimeSample=spinBoxNorm->value();
            double ttime = monitors->readDuration(Nsample);
            if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;

            double NormSample = monitors->normalizationFactor(Nsample);

            if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
            
            gsl_matrix_scale(BC,NormSample);      // EB=T*EB
        }
        else readMatrixCor( NBC, BC );
        
        readErrorMatrix( NBC, BCErr);
        status+=">>>  BC#=" + NBC + ": OK  ";
    }
    else status+=">>>  BC: no correction  ";
    
    
    //+++ transmission check
    if (trans<=0.0 || trans>2.0)
    {
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(sensErr);
        gsl_matrix_free(Sample);
        gsl_matrix_free(SampleErr);
        gsl_matrix_free(Buffer);
        gsl_matrix_free(BufferErr);
        status+=">>> transmission is out 0..2 range check it [e05]: "+QString::number(trans,'f',3);
        toResLog("DAN :: "+status+"\n");
        return false;
        
    }
    else status=status+">>>  Transmission="+QString::number(trans,'f',3)+"  ";
    
    
    //+++ transmission-Buffer check
    if (subtractBuffer)
    {
        //+++ transmission check
        if (transBuffer<=0.0 || transBuffer>2.0)
        {
            gsl_matrix_free(mask);
            gsl_matrix_free(sens);
            gsl_matrix_free(sensErr);
            gsl_matrix_free(Sample);
            gsl_matrix_free(SampleErr);
            gsl_matrix_free(Buffer);
            gsl_matrix_free(BufferErr);
            status+=">>> transmission of buffer is out 0..2 range check it [e06] : "+QString::number(trans,'f',3);
            toResLog("DAN :: "+status+"\n");
            return false;
        }
    } else  transBuffer=1.0;
    
    
    
    //+++ absolute factor check
    status=status+">>>  Abs.Factor="+QString::number(abs0,'e',4)+"  ";
    
    if(thickness!=0) abs0/=thickness;

    double abs=abs0;
    double absBuffer=abs0;

    
    if (trans!=0.0) abs=abs0/trans;
    if (transBuffer!=0.0) absBuffer=abs0/transBuffer;
    
    //+++ X-center check
    status=status+">>>  X-center="+QString::number(Xcenter,'e',4)+"  "; // 2013-09-18
    Xcenter-=1;
    
    //+++ Y-center check
    status=status+">>>  Y-center="+QString::number(Ycenter,'e',4)+"  "; // 2013-09-18
    Ycenter-=1;
    
    
    //+++ error matrix
    gsl_matrix *ErrMatrix=gsl_matrix_calloc(MD,MD);
    
    //+++ Sensitivity Error
    gsl_matrix_add(ErrMatrix, sensErr);
    
    int iii,jjj;
    double err2, Isample, Ibc, Iec;
    
    for (iii=0;iii<MD;iii++) for (jjj=0;jjj<MD;jjj++)
    {
        //+++
        Isample=gsl_matrix_get(Sample,iii,jjj);
        err2=Isample;       //+++
        Isample*=Isample;
        Isample*=gsl_matrix_get(SampleErr,iii,jjj);
        //+++
        Iec=gsl_matrix_get(EC,iii,jjj);
        err2 -= (trans*Iec);        //+++
        Iec*=Iec;
        Iec*=trans*trans;
        Iec*=gsl_matrix_get(ECErr,iii,jjj);
        //+++
        Ibc=gsl_matrix_get(BC,iii,jjj);
        if (ECexist) err2 -= ( (1.0-trans) * Ibc ); else err2 -= Ibc;   //+++
        Ibc*=Ibc;
        if (ECexist) Ibc*=(1.0-trans)*(1.0-trans);
        Ibc*=gsl_matrix_get(BCErr,iii,jjj);
        
        if ( err2 != 0.0 ) err2=1.0/err2; else err2=0.0;
        err2=err2*err2;
        err2*=(Isample+Iec+Ibc);
        
        gsl_matrix_set(ErrMatrix, iii, jjj, sqrt(gsl_matrix_get(ErrMatrix,iii,jjj) + err2 ) );
        gsl_matrix_set(SampleErr, iii, jjj, gsl_matrix_get(ErrMatrix,iii,jjj) * gsl_matrix_get(ErrMatrix,iii,jjj));
    }
    
    
    
    gsl_matrix_mul_elements(ErrMatrix,mask);
    gsl_matrix_mul_elements(SampleErr,mask);
    
    //+++ TODO :: Error of BUFFER
    
    
    //+++ Matrix-to-Matrix actions +++
    gsl_matrix_sub(Sample,BC);                      // Sample=Sample - BC
    
    // 2012 Time normalization BC
    if (filesManager->checkFileNumber(NBC) && checkBoxBCTimeNormalization->isChecked() && ECexist)
    {
        readMatrixCorTimeNormalizationOnly( NBC, BC );
        
        //Normalization constant
        double TimeSample=spinBoxNorm->value();
        double ttime = monitors->readDuration(NEC);
        if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;

        double NormSample = monitors->normalizationFactor(NEC);

        if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
        
        gsl_matrix_scale(BC,NormSample);      // EB=T*EB
    }
    
    gsl_matrix_sub(EC,BC);                          // EC=EC - BC
    
    
    if (subtractBuffer)
    {
        if (filesManager->checkFileNumber(NBC) && checkBoxBCTimeNormalization->isChecked() && ECexist)
        {
            readMatrixCorTimeNormalizationOnly( NBC, BC );
            
            //Normalization constant
            double TimeSample=spinBoxNorm->value();
            double ttime = monitors->readDuration(Nbuffer);
            if (ttime>0.0) TimeSample/=ttime; else TimeSample=0.0;

            double NormSample = monitors->normalizationFactor(Nbuffer);

            if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
            
            gsl_matrix_scale(BC,NormSample);      // EB=T*EB
        }
        gsl_matrix_sub(Buffer,BC);    //Buffer=Buffer - BC
    }
    
    
    if (trans<1.0 && trans>0.0 && checkBoxParallaxTr->isChecked()) transmissionThetaDependenceTrEC(EC, Xcenter, Ycenter, Detector, trans);
    gsl_matrix_scale(EC,trans);                             // EC=T*EC
    
    if (ECexist) gsl_matrix_sub(Sample,EC);         // Sample=Sample  - EC
    
    if (subtractBuffer)
    {
        gsl_matrix_scale(EC,transBuffer/trans);    // EC=Tbuffer*EC
        if (ECexist) gsl_matrix_sub(Buffer,EC);    // Buffer=Buffer  - EC
    }
    
    
    gsl_matrix_mul_elements(Sample,mask);
    if (subtractBuffer) gsl_matrix_mul_elements(Buffer,mask);
    
    
    
    //+++ Paralax Correction
    if (checkBoxParallax->isChecked() || checkBoxParallaxTr->isChecked())
    {
        parallaxCorrection(Sample, Xcenter, Ycenter, Detector, trans);
        
        if (subtractBuffer) parallaxCorrection(Buffer, Xcenter, Ycenter, Detector, transBuffer);
    }
    
    if (subtractBuffer)
    {
        gsl_matrix_scale(Buffer,fractionBuffer*absBuffer/abs);
        gsl_matrix_sub(Sample,Buffer);
    }
    
    
    if (comboBoxACmethod->currentIndex()==3)
    {
        double normalization = monitors->normalizationFactor(Nsample);
        if (normalization>0) gsl_matrix_scale(Sample,1/normalization);
    }
    else
    {
        gsl_matrix_scale(Sample,abs);
    }
    
    
    
    //+++ Sensitivity correction
    gsl_matrix_mul_elements(Sample,sens);
    
    
    if (BackgroundConst!=0)
    {
        for (iii=0;iii<MD;iii++) for (jjj=0;jjj<MD;jjj++)
        {
            if (gsl_matrix_get(mask,iii,jjj)>0)
            {
                gsl_matrix_set(Sample,iii,jjj,gsl_matrix_get(Sample,iii,jjj)-BackgroundConst);
            }
        }
    }
    
    //+++ Masking of  Negative Points +++
    if (checkBoxMaskNegative->isChecked())
    {
        for (iii=0;iii<MD;iii++) for (jjj=0;jjj<MD;jjj++)
        {
            if (gsl_matrix_get(Sample,iii,jjj)<=0)
            {
                gsl_matrix_set(Sample,iii,jjj,0.0);
                gsl_matrix_set(mask,iii,jjj,0.0);
                gsl_matrix_set(sens,iii,jjj,0.0);
            }
        }
    }
    
    
    // 2017 ...
    matrixConvolusion(Sample,mask,MD);
    gsl_matrix_scale(Sample,scale);
    
    
    QString nameQI = Nsample;  //  file number or name as name
    
    
    if (checkBoxNameAsTableName->isChecked())
    {
        nameQI=label;
        if(lineEditWildCard->text().contains("#")) nameQI=nameQI+"-"+Nsample;  // label as name
    }
    
    //nameQI=nameQI.replace("_", "-");
    nameQI=nameQI.simplified();
    nameQI=nameQI.replace(" ", "-").replace("/", "-").replace("_", "-").replace(",", "-").replace(".", "-").remove("%");
    
    nameQI=dataSuffix+"-"+nameQI;
    
    
    //+++ Ext
    QString currentExt=lineEditFileExt->text().remove(" ");
    if(currentExt!="") currentExt+="-";
    
    
    //+++   Open Reduced Matrix in Project
    if (radioButtonOpenInProject->isChecked() && button=="I-x-y")
    {
        QString matrixOutName="I-"+currentExt+nameQI;
        matrixOutName=label;//+++2019
        if (!checkBoxRewriteOutput->isChecked())
        {
            matrixOutName+="-v-";
            matrixOutName=app()->generateUniqueName(matrixOutName);
        }
        
        if (radioButtonXYdimQ->isChecked())
        {
            double pixel  = lineEditPS->text().toDouble();
            double pixelAsymetry  = lineEditAsymetry->text().toDouble();
            if (pixelAsymetry<=0) pixelAsymetry=1.0;
            
            if(Detector!=0)
            {                
                double xs=4.0*M_PI/Lambda*sin(0.5*atan((1-(Xcenter+1))*pixel/Detector));
                double xe=4.0*M_PI/Lambda*sin(0.5*atan((MD-(Xcenter+1))*pixel/Detector));
                double ys=4.0*M_PI/Lambda*sin(0.5*atan((1-(Ycenter+1))*pixel*pixelAsymetry/Detector));
                double ye=4.0*M_PI/Lambda*sin(0.5*atan((MD-(Ycenter+1))*pixelAsymetry*pixel/Detector));
                
                //double xs=4.0*M_PI/Lambda*sin(0.5*atan((0.5-Xcenter)*pixel/Detector));
                //double xe=4.0*M_PI/Lambda*sin(0.5*atan(((MD+0.5)-Xcenter)*pixel/Detector));
                //double ys=4.0*M_PI/Lambda*sin(0.5*atan((0.5-Ycenter)*pixel*pixelAsymetry/Detector));
                //double ye=4.0*M_PI/Lambda*sin(0.5*atan(((MD+0.5)-Ycenter)*pixelAsymetry*pixel/Detector));
                
                makeMatrixSymmetric(Sample,matrixOutName, label, MD, xs, xe, ys,ye, true);
            }
            else makeMatrixSymmetric(Sample,matrixOutName, label, MD, true);
            
        }
        else makeMatrixSymmetric(Sample,matrixOutName, label, MD, true);
    }
    
    //+++ Open Error Matrix in Project
    if (radioButtonOpenInProject->isChecked() && button=="dI-x-y")
    {
        makeMatrixSymmetric(ErrMatrix,"dI-"+currentExt+nameQI,label, MD, true);
    }
    
    //+++   Save Reduced Matrix to File
    if (!radioButtonOpenInProject->isChecked() && button=="I-x-y")
    {
        if (checkBoxSortOutputToFolders->isChecked())
        {
            QDir dd;
            if (!dd.cd(lineEditPathRAD->text()+"/ASCII-I"))
            {
                dd.mkdir(lineEditPathRAD->text()+"/ASCII-I");
            }
            if (comboBoxIxyFormat->currentText().contains("Matrix")) saveMatrixToFile(lineEditPathRAD->text()+"/ASCII-I/I-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT",Sample, MD);
            else
            {
                double pixel  = lineEditPS->text().toDouble();
                double pixelAsymetry  = lineEditAsymetry->text().toDouble();
                
                //saveMatrixAsTableToFile(lineEditPathRAD->text()+"/ASCII-I/I-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT", Sample,ErrMatrix, mask, MD, Xcenter, Ycenter, Lambda, Detector, pixel, pixel*pixelAsymetry );
            }
        }
        else
        {
            if (comboBoxIxyFormat->currentText().contains("Matrix")) saveMatrixToFile(lineEditPathRAD->text()+"/I-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT",Sample, MD);
            else
            {
                double pixel  = lineEditPS->text().toDouble();
                double pixelAsymetry  = lineEditAsymetry->text().toDouble();
                
                //saveMatrixAsTableToFile(lineEditPathRAD->text()+"/I-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT", Sample,ErrMatrix, mask, MD, Xcenter, Ycenter, Lambda, Detector, pixel, pixel*pixelAsymetry );
            }
        }
        
    }
    
    //+++   Save Error Matrix to File
    if (!radioButtonOpenInProject->isChecked() && button=="dI-x-y")
    {
        if (checkBoxSortOutputToFolders->isChecked())
        {	QDir dd;
            if (!dd.cd(lineEditPathRAD->text()+"/ASCII-dI"))
            {
                dd.mkdir(lineEditPathRAD->text()+"/ASCII-dI");
            }
            saveMatrixToFile(lineEditPathRAD->text()+"/ASCII-dI/dI-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT",ErrMatrix, MD);
        }
        else saveMatrixToFile(lineEditPathRAD->text()+"/dI-"+currentExt+nameQI+"-"+comboBoxSel->currentText()+".DAT",ErrMatrix, MD);
    }
    
    //+++++++++++++++RAD +++++++++++++++++
    // +++  Reading of some parameters +++
    double deltaLambda = selector->readDeltaLambda(Nsample);
    
    double pixel  = lineEditResoPixelSize->text().toDouble();
    double binning=comboBoxBinning->currentText().toDouble();
    
    double pixelAsymetry  = lineEditAsymetry->text().toDouble();
    if (pixelAsymetry<=0) pixelAsymetry=1.0;

    double r2 = collimation->readR2(Nsample);
    double r1 = collimation->readR1(Nsample);

    //+++ Standart radial averiging +++
    if ( button=="I-Q")
    {
        if (comboBoxMode->currentText().contains("(MS)"))
        {
            double angleMS = double(spinBoxMCshiftAngle->value()) / 180.0 * M_PI;
            radUniStandartMSmode(MD, Sample, SampleErr, mask, Xcenter, Ycenter, nameQI, C, Lambda, deltaLambda,
                                 Detector, pixel * binning, r1, r2, label,
                                 selector->readRotations(Nsample, monitors->readDuration(Nsample)), pixelAsymetry,
                                 angleMS);
        }
        else
        {
            double angleAnisotropy = double(spinBoxAnisotropyOffset->value()) / 180.0 * M_PI;
            radUni(MD, Sample, SampleErr, mask, Xcenter, Ycenter, nameQI, C, Lambda, deltaLambda, Detector,
                   pixel * binning, r1, r2, label, selector->readRotations(Nsample, monitors->readDuration(Nsample)),
                   pixelAsymetry, detRotationX, detRotationY, angleAnisotropy);
        }
    }

    //+++ Hosisontal Slice +++
    if (button=="I-Qx")
    {
        horizontalSlice( MD, Sample, SampleErr, mask, Xcenter, Ycenter,
                        nameQI, C,  Lambda, deltaLambda, Detector, pixel*binning, r1, r2, label);
    }
    
    //+++ Vertical Slice +++
    if (button=="I-Qy")
    {
        verticalSlice( MD, Sample, SampleErr, mask, Xcenter, Ycenter,
                      nameQI, C,  Lambda, deltaLambda, Detector, pixel*binning*pixelAsymetry, r1, r2, label);
    }
    
    //+++ Polarv Coordinates
    if (button=="I-Polar")
    {
        radUniPolar(MD, Sample, mask, Xcenter, Ycenter, nameQI, Lambda, Detector, pixel*binning, pixelAsymetry );
    }
    
    //+++ SIGMA [x,y]
    if (button=="Sigma-x-y")
    {
        sigmaMatrix(MD, mask, Xcenter, Ycenter, "Sigma-"+currentExt+nameQI, Lambda, deltaLambda, C, Detector, pixel*binning, r1,r2 );
    }
    
    //+++ Q [x,y]
    if (button=="Q-x-y")
    {
        MatrixQ(MD, mask, Xcenter, Ycenter, "Q-"+currentExt+nameQI, Lambda, Detector, pixel*binning, pixelAsymetry, detRotationX, detRotationY );
    }
    //+++ dQ [x,y]
    if (button=="dQ-x-y")
    {
        dQmatrix(MD, mask, Xcenter, Ycenter, "dQ-"+currentExt+nameQI, Lambda, Detector, pixel*binning, pixelAsymetry );
    }
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    toResLog("DAN :: "+status+"\n");
    
    
    //+++ Clean Memory +++
    gsl_matrix_free(Sample);
    gsl_matrix_free(SampleErr);
    gsl_matrix_free(Buffer);
    gsl_matrix_free(BufferErr);
    gsl_matrix_free(EC);
    gsl_matrix_free(ECErr);
    gsl_matrix_free(BC);
    gsl_matrix_free(BCErr);
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    gsl_matrix_free(sensErr);
    gsl_matrix_free(ErrMatrix);
    
    return true;
}
