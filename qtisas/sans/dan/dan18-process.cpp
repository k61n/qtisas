/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS process tab tools
 ******************************************************************************/

#include <QScrollBar>

#include "dan18.h"

//*******************************************
//+++  RT tools:: Sum [slot]
//*******************************************
void dan18::processdataConnectSlots()
{
    connect( pushButtonNewScript, SIGNAL( clicked() ), this, SLOT( newScriptTable()));    
    connect( pushButtonMakeTable, SIGNAL( clicked() ), this, SLOT(makeScriptTable()) );
    connect(pushButtonSaveSettings, SIGNAL(clicked()), this, SLOT(saveScriptSettings()));
    connect(sliderConfigurations, SIGNAL( valueChanged(int) ), this, SLOT(SetColNumberNew (int) ) );  
    connect( comboBoxMakeScriptTable, SIGNAL( activated(int) ), this, SLOT( activeScriptTableSelected(int) ) );
    connect(pushButtonAddCopy, SIGNAL( clicked() ), this, SLOT(addCopyOfLastConfiguration() ) );
    connect(pushButtonDeleteFirst, SIGNAL( clicked() ), this, SLOT(deleteFirstConfiguration() ) );
    connect(tableEC->verticalHeader(), SIGNAL(sectionClicked(int)), this,SLOT(vertHeaderTableECPressed(int)));
    connect(tableEC->horizontalHeader(), SIGNAL(sectionClicked(int)), this,SLOT(horHeaderTableECPressed(int)));
    connect( tableEC, SIGNAL( cellActivated(int,int) ),  this, SLOT( tableECclick(int,int) ) );
    
    connect( pushButtonCalcTr, SIGNAL( clicked() ), this, SLOT( calculateTransmissionAll() ) );
    
    connect(pushButtonCenter, SIGNAL( clicked() ), this, SLOT(calculateCentersInScript() ) );
    connect(pushButtonAbsFactor, SIGNAL( clicked() ), this, SLOT(calculateAbsFactorInScript() ) );//+++2014-04
    connect(pushButtonTrMaskDB, SIGNAL( clicked() ), this, SLOT(calculateTrMaskDB() ) );//+++2014-04
    connect( radioButtonDpSelector2D, SIGNAL( toggled(bool) ), this, SLOT( dataProcessingOptionSelected()) );
    connect( radioButtonDpSelector1D, SIGNAL( toggled(bool) ), this, SLOT( dataProcessingOptionSelected()) );
    connect( radioButtonDpSelectorScript, SIGNAL( toggled(bool) ), this, SLOT( dataProcessingOptionSelected()) );
}
//+++ selection of unuqe configurations
bool dan18::allUniqueConfigurations(QList<int> &uniqueConfigurations, double range, bool checkPolarization,
                                    bool excludeTrConfiguration)
{
    uniqueConfigurations.clear();
    QString currentECrun, acceptedECrun;

    for (int col = 0; col < tableEC->columnCount(); col++)
    {
        currentECrun = tableEC->item(dptEC, col)->text();

        if (!filesManager->checkFileNumber(currentECrun))
            continue;

        if (excludeTrConfiguration && tableEC->item(dptECTR, col)->checkState() == Qt::Checked)
            continue;

        if (uniqueConfigurations.isEmpty())
        {
            uniqueConfigurations << col + 1;
            continue;
        }

        for (int c = 0; c < uniqueConfigurations.count(); c++)
        {
            acceptedECrun = tableEC->item(dptEC, uniqueConfigurations[c] - 1)->text();

            if (!compareConfigurations(acceptedECrun, currentECrun, range, checkPolarization))
            {
                if (c == uniqueConfigurations.count() - 1)
                    uniqueConfigurations << col + 1;
            }
            else
                break;
        }
    }

    if (uniqueConfigurations.count() != tableEC->columnCount())
        return false;

    return true;
}
//+++ compare two configurations
bool dan18::compareConfigurations(const QString &RunNumber1, const QString &RunNumber2, double range,
                                  bool checkPolarization) const
{
    if (range < 0.0 || range > 1.0)
        range = 0.05;

    double lambda1 = selector->readLambda(RunNumber1, monitors->readDuration(RunNumber1));
    double lambda2 = selector->readLambda(RunNumber2, monitors->readDuration(RunNumber2));
    if (fabs(lambda1 - lambda2) > range * fabs(lambda1 + lambda2) / 2.0)
        return false;

    double D1 = detector->readD(RunNumber1);
    double D2 = detector->readD(RunNumber2);
    if (fabs(D1 - D2) > range * fabs(D1 + D2) / 2.0)
        return false;

    double C1 = collimation->readC(RunNumber1);
    double C2 = collimation->readC(RunNumber2);
    if (fabs(C1 - C2) > range * fabs(C1 + C2) / 2.0)
        return false;

    QString beamSize1 = collimation->readCA(RunNumber1) + "|" + collimation->readSA(RunNumber1);
    QString beamSize2 = collimation->readCA(RunNumber2) + "|" + collimation->readSA(RunNumber2);
    if (beamSize1 != beamSize2)
        return false;

    if (checkBoxBeamcenterAsPara->isChecked())
    {
        double beamPositionX1 = detector->readDetectorX(RunNumber1);
        double beamPositionX2 = detector->readDetectorX(RunNumber2);
        if (fabs(beamPositionX1 - beamPositionX2) > range / 5.0 * fabs(beamPositionX1 + beamPositionX2) / 2.0)
            return false;

        double beamPositionY1 = detector->readDetectorY(RunNumber1);
        double beamPositionY2 = detector->readDetectorY(RunNumber2);
        if (fabs(beamPositionY1 - beamPositionY2) > range / 5.0 * fabs(beamPositionY1 + beamPositionY2) / 2.0)
            return false;
    }

    if (checkBoxRecalculateUseNumber->isChecked())
        if (sample->readPositionNumber(RunNumber1).toInt() != sample->readPositionNumber(RunNumber2).toInt())
            return false;

    if (checkPolarization)
    {
        QString polarization1 = collimation->readPolarization(RunNumber1);
        QString polarization2 = collimation->readPolarization(RunNumber2);
        if (polarization1 != polarization2)
            return false;
    }

    if (checkBoxAttenuatorAsPara->isChecked())
        if (collimation->readAttenuator(RunNumber1) != collimation->readAttenuator(RunNumber2))
            return false;

    if (checkBoxDetRotAsPara->isChecked())
    {
        double rotationX1 = detector->readDetRotationX(RunNumber1);
        double rotationX2 = detector->readDetRotationX(RunNumber2);
        if (fabs(rotationX1 - rotationX2) > range / 5.0 * fabs(rotationX1 + rotationX2) / 2.0)
            return false;
        double rotationY1 = detector->readDetRotationY(RunNumber1);
        double rotationY2 = detector->readDetRotationY(RunNumber2);
        if (fabs(rotationY1 - rotationY2) > range / 5.0 * fabs(rotationY1 + rotationY2) / 2.0)
            return false;
    }

    return true;
}
//+++ Search of  Script tables
void dan18::findSettingTables()
{
    QString activeTableScript=comboBoxMakeScriptTable->currentText();
    //+++
    QStringList list;
    QStringList listOpen;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList)
    {
        Table *tt=(Table *)t;
        if (tt->text(0,0)=="DAT::Path") list<<tt->name();
    }
    //+++
    
    //
    comboBoxMakeScriptTable->clear();
    comboBoxMakeScriptTable->addItems(list);
    if (list.contains(activeTableScript))
        comboBoxMakeScriptTable->setCurrentIndex(comboBoxMakeScriptTable->findText(activeTableScript));
}
//*******************************************
//+++  newScriptTable() [slot]
//*******************************************
void dan18::newScriptTable(QString tableName)
{
    if (tableName == "")
    {
        tableName = comboBoxMakeScriptTable->currentText();
        if (tableName == "")
            tableName = "script";

        bool ok;
        tableName = QInputDialog::getText(this, "Creation of Script-Table",
                                          "Enter name of a new Script-Table:", QLineEdit::Normal, tableName, &ok);
        if (!ok)
            return;
    }

    tableName = tableName.replace(" ", "-")
                    .replace("/", "-")
                    .replace("_", "-")
                    .replace(",", "-")
                    .replace(".", "-")
                    .remove("%")
                    .remove("(")
                    .remove(")");

    if (tableName.isEmpty())
        return;

    QString activeTable = comboBoxMakeScriptTable->currentText();

    if (checkBoxSortOutputToFolders->isChecked())
        app()->changeFolder("DAN :: script, info, ...");

    if (app()->checkTableExistence(tableName))
    {
        QMessageBox::critical(nullptr, "qtiSAS", "Table " + tableName + " is already exists");
        return;
    }
    
    //+++
    Table *w = app()->newTable(tableName, 0, 24);
    scriptTableManager->emptyScriptTable(w);

    //+++ update script combobox list
    QStringList  list;
    findTableListByLabel("DAN::Script::Table",list);
    comboBoxMakeScriptTable->clear();
    comboBoxMakeScriptTable->addItems(list);
    comboBoxMakeScriptTable->setCurrentIndex(comboBoxMakeScriptTable->findText(tableName));
    
    //+++ setting table
    saveScriptSettings(tableName + "-Settings");

    //+++
    app()->maximizeWindow(tableName);
}
//*******************************************
//+++  makeScriptTable [slot]
//*******************************************
void dan18::makeScriptTable(QStringList selectedDat)
{
    int MD = lineEditMD->text().toInt();
    QString wildCard = filesManager->wildCardDetector();
    QString Dir = filesManager->pathInString();
    bool dirsInDir = filesManager->subFoldersYN();

    if (selectedDat.isEmpty())
    {
        //+++ select files
        auto *fd = new QFileDialog(this, "DAN - Getting File Information", Dir, textEditPattern->text());
        fd->setDirectory(Dir);
        fd->setFileMode(QFileDialog::ExistingFiles);
        fd->setWindowTitle(tr("DAN - Getting File Information"));
        foreach (QComboBox *obj, fd->findChildren<QComboBox *>())
            if (QString(obj->objectName()).contains("fileTypeCombo"))
                obj->setEditable(true);
        //+++
        if (fd->exec() == QDialog::Rejected)
            return;
        //+++
        selectedDat = fd->selectedFiles();
    }

    int filesNumber = selectedDat.count();

    if (comboBoxMakeScriptTable->count() == 0)
    {
        QMessageBox::critical(nullptr, "qtiSAS", "Create first Script-Table");
        return;
    }

    //+++
    QString tableName = comboBoxMakeScriptTable->currentText();

    int startRaw = 0;
    
    QString s = "";
    int i, iMax;

    if (filesNumber == 0)
    {
        QMessageBox::critical( 0, "qtiSAS", "Nothing was selected");
        return;
    }

    QString test = selectedDat[0];
    if ( test.contains(Dir) )
    {
        test = test.remove(Dir);
        if (!dirsInDir && test.contains("/"))
        {
            QMessageBox::critical(nullptr, "qtiSAS", "Selected data not in ::Input Folder::");
            return;
        }
        else if (dirsInDir && test.count("/") > 1)
        {
            QMessageBox::critical(nullptr, "qtiSAS", "Selected data not in ::Input Folder:: + sub-folders");
            return;
        }
    }
    else
    {
        QMessageBox::critical(nullptr, "qtiSAS", "Selected data not in ::Input Folder::");
        return;
    }

    //+++
    Table *w;
    if (app()->checkTableExistence(tableName))
    {
        //+++ Find table
        QList<MdiSubWindow *> tableList = app()->tableList();
        foreach (MdiSubWindow *t, tableList)
            if (t->name() == tableName)
                w = (Table *)t;

        if (w->windowLabel() != "DAN::Script::Table")
        {
            QMessageBox::critical(nullptr, "qtiSAS", "Table ~" + tableName + " is not script");
            return;
        }

        if (w->numCols() < 24)
        {
            QMessageBox::critical( 0, "qtiSAS", "Better, create new table (# col)");
            return;
        }

        startRaw = w->numRows();

        if (!scriptTableManager->update(w))
        {
            QMessageBox::critical(nullptr, "qtiSAS",
                                  "Table ~" + tableName +
                                      " has wrong format. <br> Check table or  generate new one.<h4>");
            return;
        }
    }
    else
    {
        w = app()->newTable(tableName, 0, 24);
        //+++
        w->setWindowLabel("DAN::Script::Table");
        app()->setListViewLabel(w->name(), "DAN::Script::Table");
        app()->updateWindowLists(w);
        scriptTableManager->emptyScriptTable(w);
    }

    w->setNumRows(startRaw + filesNumber);

    //+++ Subtract Bufffer
    bool subtractBuffer = comboBoxMode->currentText().contains("(BS)");

    //+++
    int  C;
    QString Number;
    double M2, M3;
    double D, lambda, cps,thickness;
    QString beamSize;
    QStringList lst;
    QString polarization;
    int polarizationValue;
    //
    for (int iter = startRaw; iter < (startRaw + filesNumber); iter++)
    {
        lst.clear();
        //+++ header
        Number = FilesManager::findFileNumberInFileName(wildCard, selectedDat[iter - startRaw].remove(Dir));
        int index = -1;
        if (Number.contains("["))
        {
            index = Number.right(Number.length() - Number.indexOf("[")).remove("[").remove("]").toInt();
        }
        readHeaderNumberFull(Number, lst);

        polarization = collimation->readPolarization(Number, lst);
        polarizedAlias(polarization);

        D = detector->readDinM(Number, lst);
        C = int(collimation->readCinM(Number, lst));
        lambda = selector->readLambda(Number, monitors->readDuration(Number), lst);
        thickness = sample->readThickness(Number, lst);
        double analyzerTransmission = analyzerTransmissionSelector->getValue(lambda, Number);
        double analyzerEfficiency = analyzerEfficiencySelector->getValue(lambda, Number);
        //+++
        scriptTableManager->infoWrite(iter, sample->readName(Number, lst));
        scriptTableManager->runSampleWrite(iter, Number);
        scriptTableManager->polarizationWrite(iter, polarization);
        scriptTableManager->collimationWrite(iter, QString::number(C, 'f', 0));
        scriptTableManager->distanceWrite(iter, QString::number(D, 'f', 3));
        scriptTableManager->lambdaWrite(iter, QString::number(lambda, 'f', 3));
        scriptTableManager->beamSizeWrite(iter,
                                          collimation->readCA(Number, lst) + "|" + collimation->readSA(Number, lst));
        scriptTableManager->thicknessWrite(iter, QString::number(thickness, 'f', 3));
        scriptTableManager->analyzerTransmissionWrite(iter, QString::number(analyzerTransmission, 'f', 4));
        scriptTableManager->analyzerEfficiencyWrite(iter, QString::number(analyzerEfficiency, 'f', 4));
    }
    // check CD conditions
    int Ncond, iC;
    iMax = tableEC->columnCount();
    //+++ recalculate old files
    int startCalc=startRaw;
    if (checkBoxRecalculate->isChecked()) startCalc=0;

    for (int iter = startCalc; iter < (startRaw + filesNumber); iter++)
    {
        Number = scriptTableManager->runSample(iter);
        if (Number != "")
        {
            Ncond = -1;
            C = scriptTableManager->collimation(iter).toInt();
            D = scriptTableManager->distance(iter).toDouble();
            lambda = scriptTableManager->lambda(iter).toDouble();
            beamSize = scriptTableManager->beamSize(iter);

            for (iC = iMax - 1; iC >= 0; iC--)
            {
                bool condLambda = lambda > (0.95 * tableEC->item(dptWL, iC)->text().toDouble()) &&
                                  lambda < (1.05 * tableEC->item(dptWL, iC)->text().toDouble());
                bool condD = D > (0.95 * tableEC->item(dptD, iC)->text().toDouble()) &&
                             D < (1.05 * tableEC->item(dptD, iC)->text().toDouble());
                bool condSample = sample->compareSamplePositions(Number, tableEC->item(dptEC, iC)->text());
                bool attenuatorCompare = collimation->compareAttenuators(Number, tableEC->item(dptEC, iC)->text());
                bool beamPosCompare = detector->compareBeamPosition(Number, tableEC->item(dptEC, iC)->text());
                bool polarizationCompare = collimation->comparePolarization(
                    Number, ((QComboBoxInTable *)tableEC->cellWidget(dptPOL, iC))->currentText());
                bool detAngleCompare = detector->compareDetRotationPosition(Number, tableEC->item(dptEC, iC)->text());

                if (C == tableEC->item(dptC, iC)->text().toInt() && condD && condLambda && condSample &&
                    polarizationCompare && beamSize == tableEC->item(dptBSIZE, iC)->text() && attenuatorCompare &&
                    beamPosCompare && detAngleCompare)
                    Ncond = iC;
            }

            scriptTableManager->conditionWrite(iter, QString::number(Ncond + 1));

            if (Ncond >= 0)
            {
                scriptTableManager->runECWrite(iter, tableEC->item(dptEC, Ncond)->text());
                scriptTableManager->runBCWrite(iter, tableEC->item(dptBC, Ncond)->text());
                scriptTableManager->absoluteFactorWrite(
                    iter, QString::number(tableEC->item(dptACFAC, Ncond)->text().toDouble()));
                scriptTableManager->fractionBufferWrite(iter, "0.000");
                scriptTableManager->transmissionBufferWrite(iter, "1.000");
                scriptTableManager->sensFromBufferWrite(iter, "no");
                s = tableEC->item(dptCENTERX, Ncond)->text();
                scriptTableManager->centerXWrite(iter, QString::number(s.left(6).toDouble()));
                s = tableEC->item(dptCENTERY, Ncond)->text();
                scriptTableManager->centerYWrite(iter, QString::number(s.left(6).toDouble()));
                scriptTableManager->maskWrite(iter,
                                              ((QComboBoxInTable *)tableEC->cellWidget(dptMASK, Ncond))->currentText());
                scriptTableManager->sensWrite(iter,
                                              ((QComboBoxInTable *)tableEC->cellWidget(dptSENS, Ncond))->currentText());
            }
            else
            {
                scriptTableManager->runECWrite(iter, "");
                scriptTableManager->runBCWrite(iter, "");
                scriptTableManager->transmissionWrite(iter, "1.000");
                scriptTableManager->transmissionBufferWrite(iter, "1.000");
                scriptTableManager->absoluteFactorWrite(iter, "1.000");
                scriptTableManager->fractionBufferWrite(iter, "0.000");
                scriptTableManager->centerXWrite(iter, QString::number((MD + 1.0) / 2.0, 'f', 3));
                scriptTableManager->centerYWrite(iter, QString::number((MD + 1.0) / 2.0, 'f', 3));
                scriptTableManager->maskWrite(iter, "mask");
                scriptTableManager->sensWrite(iter, "sens");
                scriptTableManager->sensFromBufferWrite(iter, "no");
            }
        }
    }

    // +++ Calculate Center for every File
    if (checkBoxFindCenter->isChecked())
    {
        QString sampleFile, D;
        double Xc, Yc, XcErr, YcErr;

        for (int iter = startCalc; iter < (startRaw + filesNumber); iter++)
        {
            Xc = scriptTableManager->centerX(iter).toDouble();
            Yc = scriptTableManager->centerY(iter).toDouble();

            sampleFile = scriptTableManager->runSample(iter);
            D = scriptTableManager->distance(iter);

            QString maskName = comboBoxMaskFor->currentText();
            QString sensName = comboBoxSensFor->currentText();

            maskName = scriptTableManager->mask(iter);
            sensName = scriptTableManager->sens(iter);
            calcCenterNew(sampleFile, -1, Xc, Yc, XcErr, YcErr, maskName, sensName);
            scriptTableManager->centerXWrite(iter, QString::number(Xc, 'f', 3));
            scriptTableManager->centerYWrite(iter, QString::number(Yc, 'f', 3));
        }
    }

    //findSettingTables();
    QStringList  list;
    findTableListByLabel("DAN::Script::Table",list);
    comboBoxMakeScriptTable->clear();
    comboBoxMakeScriptTable->addItems(list);
    comboBoxMakeScriptTable->setCurrentIndex(comboBoxMakeScriptTable->findText(tableName));
    
    // adjust columns
    w->adjustColumnsWidth(false);
    
    saveScriptSettings(tableName + "-Settings");

    app()->maximizeWindow(tableName);

    //+++ Calculation of transmissions for Active Configurations
    calculateTransmission(startCalc);

    //+++ Copy/Paste of transmissions for non-Active Configurations
    if (checkBoxForceCopyPaste->isChecked())
        copyCorrespondentTransmissions(startCalc);
}
//+++
void dan18::saveScriptSettings(QString tableName)
{
    if (tableName == "")
    {
        tableName = comboBoxMakeScriptTable->currentText();
        if (tableName == "")
            return;
        tableName += "-Settings";
    }

    QString Dir = filesManager->pathInString();
    int MD = lineEditMD->text().toInt();

    Table *w;
    int i; 
    if (app()->checkTableExistence(tableName))
    {
        //+++ Find table
        QList<MdiSubWindow *> tableList=app()->tableList();
        foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
        //+++
        
        if (w->windowLabel()!="DAN::Settings::Table")
        {
            QMessageBox::critical( 0, "qtiSAS", "Table "+tableName+" is not settings table");
            return;
        }
        
        
        if (w->numCols()<2)
        {
            QMessageBox::critical( 0, "qtiSAS", "Better, create new settings table");
            return;
        }
    }
    else
    {
        if (checkBoxSortOutputToFolders->isChecked())
            app()->changeFolder("DAN :: script, info, ...");

        w = app()->newHiddenTable(tableName, "DAN::Settings::Table", 0, 2);
        w->setWindowLabel("DAN::Settings::Table");
        app()->setListViewLabel(w->name(), "DAN::Settings::Table");
        app()->updateWindowLists(w);

        // Col-Names
        w->setColName(0, "Parameter");
        w->setColPlotDesignation(0, Table::None);
        w->setColumnType(0, Table::Text);

        w->setColName(1, "Parameter-Value");
        w->setColPlotDesignation(1, Table::None);
        w->setColumnType(1, Table::Text);
        app()->hideWindow(w);
    }
    
    int currentRow=0;
    QString s;
    
    //----- input path
    w->setNumRows(currentRow+1);
    s=Dir+" <";
    w->setText(currentRow, 0, "Settings::Path::Input");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Input Filter
    w->setNumRows(currentRow+1);
    s=textEditPattern->text()+" <";
    w->setText(currentRow, 0, "Settings::Input::Filter");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Settings::Path::DirsInDir
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Settings::Path::DirsInDir");    
    if (checkBoxDirsIndir->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");
    currentRow++;
    
    //----- output path
    w->setNumRows(currentRow+1);
    s=lineEditPathRAD->text()+" <";
    w->setText(currentRow, 0, "Settings::Path::Output");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //--- Mask::Edges
    w->setNumRows(currentRow+1);
    s=QString::number(spinBoxLTx->value());
    s=s+" "+QString::number(spinBoxLTy->value());
    s=s+" "+QString::number(spinBoxRBx->value());
    s=s+" "+QString::number(spinBoxRBy->value());
    w->setText(currentRow,0,"Mask::Edges");
    w->setText(currentRow,1,s+" <");	
    currentRow++;
    
    //--- Mask::BeamStop
    w->setNumRows(currentRow+1);    
    s=QString::number(spinBoxLTxBS->value());
    s=s+" "+QString::number(spinBoxLTyBS->value());
    s=s+" "+QString::number(spinBoxRBxBS->value());
    s=s+" "+QString::number(spinBoxRByBS->value());
    w->setText(currentRow,0,"Mask::BeamStop");
    w->setText(currentRow,1,s+" <");	
    currentRow++;
    
    //----- Mask::Edges::Shape 
    w->setNumRows(currentRow+1);
    s=comboBoxMaskEdgeShape->currentText()+" <";
    w->setText(currentRow, 0, "Mask::Edges::Shape");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Mask::BeamStop::Shape 
    w->setNumRows(currentRow+1);
    s=comboBoxMaskBeamstopShape->currentText()+" <";
    w->setText(currentRow, 0, "Mask::BeamStop::Shape");
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //----- Mask::Dead::Rows 
    w->setNumRows(currentRow+1);
    s=lineEditDeadRows->text()+" <";
    w->setText(currentRow, 0, "Mask::Dead::Rows");
    w->setText(currentRow, 1, s);
    currentRow++;    
    
    //----- Mask::Dead::Cols 
    w->setNumRows(currentRow+1);
    s=lineEditDeadCols->text()+" <";
    w->setText(currentRow, 0, "Mask::Dead::Cols");
    w->setText(currentRow, 1, s);
    currentRow++;    
    
    //----- Mask::Triangular 
    w->setNumRows(currentRow+1);
    s=lineEditMaskPolygons->text()+" <";
    w->setText(currentRow, 0, "Mask::Triangular");
    w->setText(currentRow, 1, s);
    currentRow++;      
    
    //---- Sensitivity:.numbers
    w->setNumRows(currentRow+1);    
    if (lineEditPlexiAnyD->text()=="") s="0"; else s=lineEditPlexiAnyD->text();
    if (lineEditEBAnyD->text()=="") s=s+" 0"; else s=s+" "+lineEditEBAnyD->text();
    if (lineEditBcAnyD->text()=="") s=s+" 0"; else s=s+" "+lineEditBcAnyD->text();
    if (lineEditTransAnyD->text()=="") s=s+" 0"; else s=s+" "+lineEditTransAnyD->text();
    
    w->setText(currentRow,0,"Sensitivity::Numbers");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    
    //---- Sensitivity:.Corection
    w->setNumRows(currentRow+1);    
    w->setText(currentRow,0,"Sensitivity::Error::Range");
    
    s=QString::number(spinBoxErrLeftLimit->value());
    s=s+" "+QString::number(spinBoxErrRightLimit->value());
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Sensitivity::Error::Matrix
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Sensitivity::Error::Matrix");
    if (checkBoxSensError->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Sensitivity::Tr::Option
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Sensitivity::Tr::Option");
    if (checkBoxSensTr->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;	
    
    //+++ Processing::Conditions::Number
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Processing::Conditions::Number");
    w->setText(currentRow,1,QString::number(sliderConfigurations->value())+" <");
    int iMax=sliderConfigurations->value();
    currentRow++;
    
    //+++ Processing::EC
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptEC,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptEC,i)->text();
    }
    
    w->setText(currentRow,0,"Processing::EC");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::BC
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptBC,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptBC,i)->text();
    }
    
    w->setText(currentRow,0,"Processing::BC");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::EB
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptEB,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptEB,i)->text();
    }
    
    w->setText(currentRow,0,"Processing::EB");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::C[m]
    w->setNumRows(currentRow+1);
    s=QString::number(tableEC->item(dptC,0)->text().toInt());
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptC,i)->text().toInt());
    w->setText(currentRow,0,"Processing::C[m]");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::D[m]
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptD,0)->text().toDouble(),'f',3);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptD,i)->text().toDouble(),'f',3);
    
    w->setText(currentRow,0,"Processing::D[m]");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::Lambda[m]
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptWL,0)->text().toDouble(),'f',3);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptWL,i)->text().toDouble(),'f',3);
    
    w->setText(currentRow,0,"Processing::Lambda[A]");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::Beam::Size
    w->setNumRows(currentRow+1);            
    s=tableEC->item(dptBSIZE,0)->text();
    if (s=="") s="00x00|00x00";
    for (i=1; i<iMax;i++) 
    {
	if (tableEC->item(dptBSIZE,i)->text()!="") s=s+" "+tableEC->item(dptBSIZE,i)->text();
	else s=s+" 00x00|00x00";
    }
    w->setText(currentRow,0,"Processing::Beam::Size");
    w->setText(currentRow,1,s+" <");
    currentRow++;

    //+++ Processing::Polarization
    w->setNumRows(currentRow + 1);
    s = QString::number(((QComboBoxInTable *)tableEC->cellWidget(dptPOL, 0))->currentIndex());
    for (i = 1; i < iMax; i++)
        s = s + " " + QString::number(((QComboBoxInTable *)tableEC->cellWidget(dptPOL, i))->currentIndex());
    w->setText(currentRow, 0, "Processing::Polarization");
    w->setText(currentRow, 1, s + " <");
    currentRow++;

    //+++ Processing::Transm::EC
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptECTR,0)->text().toDouble(),'f',4);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptECTR,i)->text().toDouble(),'f',4);
    
    w->setText(currentRow,0,"Processing::Transm::EC");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::Transm::EC::Activity
    w->setNumRows(currentRow+1);            
    s="";
    for (i=0; i<iMax;i++) if (tableEC->item(dptECTR, i)->checkState() == Qt::Checked) s+="1 "; else s+="0 ";
    
    w->setText(currentRow,0,"Processing::Transm::EC::Activity");
    w->setText(currentRow,1,s+"<");
    currentRow++;    
    
    //+++ Processing::Plexi::Plexi
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptACFS,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptACFS,i)->text();
    }
    
    w->setText(currentRow,0,"Processing::Plexi::Plexi");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::Plexi::EB
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptACEB,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptACEB,i)->text();
    }
    
    w->setText(currentRow,0,"Processing::Plexi::EB");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::Plexi::BC
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptACBC,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptACBC,i)->text();
    }
    w->setText(currentRow,0,"Processing::Plexi::BC");
    w->setText(currentRow,1,s+" <");
    currentRow++;        
    
    //+++ Processing::AC::DAC
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptDAC,0)->text().toDouble(),'f',3);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptDAC,i)->text().toDouble(),'f',3);
    
    w->setText(currentRow,0,"Processing::AC::DAC");
    w->setText(currentRow,1,s+" <");
    currentRow++; 
    
    //+++ Processing::AC::MU
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptACMU,0)->text().toDouble(),'E',4);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptACMU,i)->text().toDouble(),'E',4);
    
    w->setText(currentRow,0,"Processing::AC::MU");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::AC::TR
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptACTR,0)->text().toDouble(),'f',4);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptACTR,i)->text().toDouble(),'f',4);
    
    w->setText(currentRow,0,"Processing::AC::TR");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::AC::Factor
    w->setNumRows(currentRow+1);            
    s=QString::number(tableEC->item(dptACFAC,0)->text().toDouble(),'E',4);
    for (i=1; i<iMax;i++) s=s+" "+QString::number(tableEC->item(dptACFAC,i)->text().toDouble(),'E',4);
    
    w->setText(currentRow,0,"Processing::AC::Factor");
    w->setText(currentRow,1,s+" <");
    currentRow++;        
    
    //+++ Processing::Center::File
    w->setNumRows(currentRow+1);        
    s="";
    for (i=0; i<iMax;i++)
    {
	if (tableEC->item(dptCENTER,i)->text()=="")
	{
	    s=s+" 0";
	}
	else s=s+" "+tableEC->item(dptCENTER,i)->text();
    } 
    w->setText(currentRow,0,"Processing::Center::File");
    w->setText(currentRow,1,s+" <");
    currentRow++;        
    
    //+++ Processing::X-center
    w->setNumRows(currentRow+1);            
    s=tableEC->item(dptCENTERX,0)->text();
    if (s=="") s=QString::number((MD+1.0)/2.0,'f',3)+QChar(177)+"10.0";
    for (i=1; i<iMax;i++) 
    {
	if(tableEC->item(dptCENTERX,i)->text()!="") s=s+" "+tableEC->item(dptCENTERX,i)->text();
	else s=s+" "+QString::number((MD+1.0)/2.0,'f',3)+QChar(177)+"10.0";
    }
    w->setText(currentRow,0,"Processing::X-center");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::Y-center
    w->setNumRows(currentRow+1);            
    s=tableEC->item(dptCENTERY,0)->text();
    if (s=="") s=QString::number((MD+1.0)/2.0,'f',3)+QChar(177)+"10.0";
    for (i=1; i<iMax;i++) 
    {
	if(tableEC->item(dptCENTERY,i)->text()!="") s=s+" "+tableEC->item(dptCENTERY,i)->text();
	else s=s+" "+QString::number((MD+1.0)/2.0,'f',3)+QChar(177)+"10.0";
    }
    w->setText(currentRow,0,"Processing::Y-center");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::Mask
    w->setNumRows(currentRow+1);            
    s=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,0))->currentText();
    for (i=1; i<iMax;i++) s=s+" "+((QComboBoxInTable*)tableEC->cellWidget(dptMASK,i))->currentText();
    
    w->setText(currentRow,0,"Processing::Mask");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::Sens
    w->setNumRows(currentRow+1);            
    s=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,0))->currentText();
    for (i=1; i<iMax;i++) s=s+" "+((QComboBoxInTable*)tableEC->cellWidget(dptSENS,i))->currentText();
    
    w->setText(currentRow,0,"Processing::Sensitivity");
    w->setText(currentRow,1,s+" <");
    currentRow++;    
    
    //+++ Processing::Tr::Mask
    w->setNumRows(currentRow+1);            
    s=((QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,0))->currentText();
    for (i=1; i<iMax;i++) s=s+" "+((QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,i))->currentText();
    
    w->setText(currentRow,0,"Processing::Tr::Mask");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Processing::File::Ext
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Processing::File::Ext");
    s=lineEditFileExt->text()+" <";
    w->setText(currentRow, 1, s);
    currentRow++;  

    //+++ Processing::scriptPolarized
    w->setNumRows(currentRow + 1);
    s = comboBoxPolarizationScriptTable->currentText();
    w->setText(currentRow, 0, "Processing::scriptPolarized");
    w->setText(currentRow, 1, s + " <");
    currentRow++;

    //+++ Options::Instrument
    w->setNumRows(currentRow+1);            
    s = comboBoxInstrument->currentText();
    w->setText(currentRow,0,"Options::Instrument");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::Mode
    w->setNumRows(currentRow+1);            
    s=comboBoxMode->currentText();
    w->setText(currentRow,0,"Options::Mode");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::Instrument::DeadTime::Homogenity
    w->setNumRows(currentRow+1);            
    s=comboBoxDTtype->currentText();
    w->setText(currentRow,0,"Options::Instrument::DeadTime::Homogenity");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    
    //+++ Options::Calibrant::Type
    w->setNumRows(currentRow+1);            
    w->setText(currentRow,0,"Options::Calibrant::Type");    
    s=comboBoxACmethod->currentText();
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::Calibrant::Active::Mask::Sens
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Calibrant::Active::Mask::Sens");
    if (checkBoxACDBuseActive->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::Calibrant
    w->setNumRows(currentRow+1);            
    s=comboBoxCalibrant->currentText();
    w->setText(currentRow,0,"Options::Calibrant");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::Calibrant::CalculateTr
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Calibrant::CalculateTr");
    if (checkBoxTransmissionPlexi->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::Calibrant::MU
    w->setNumRows(currentRow+1);            
    s=lineEditMuY0->text();
    s+=" "+lineEditMuA->text();
    s+=" "+lineEditMut->text();
    w->setText(currentRow,0,"Options::Calibrant::MU");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::Calibrant::TR
    w->setNumRows(currentRow+1);            
    s=lineEditTo->text();
    s+=" "+lineEditTA->text();
    s+=" "+lineEditLate->text();
    w->setText(currentRow,0,"Options::Calibrant::TR");		    
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::2D::Normalization
    w->setNumRows(currentRow+1);            
    s=comboBoxNorm->currentText();
    w->setText(currentRow,0,"Options::2D::Normalization");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::2D::Normalization::Constant
    w->setNumRows(currentRow+1);            
    s=QString::number(spinBoxNorm->value());
    w->setText(currentRow,0,"Options::2D::Normalization::Constant");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::2D::Normalization::BC::Normalization
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::Normalization::BC::Normalization");
    if (checkBoxBCTimeNormalization->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::2D::xyDimension::Pixel
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::xyDimension::Pixel");
    if (radioButtonXYdimPixel->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;    
    
    //--- Options::2D::RemoveNegativePoints::2D
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::RemoveNegativePoints::2D");
    if (checkBoxMaskNegative->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::2D::RemoveNegativePoints::1D
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::RemoveNegativePoints::1D");
    if (checkBoxMaskNegativeQ->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::2D::Polar::Resolusion
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::Polar::Resolusion");
    w->setText(currentRow,1,QString::number(spinBoxPolar->value())+" <");
    currentRow++;
    
    //--- Options::2D::HighQcorrection
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::HighQcorrection");
    if (checkBoxParallax->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::2D::HighQtype
    w->setNumRows(currentRow+1);
    s=comboBoxParallax->currentText();
    w->setText(currentRow,0,"Options::2D::HighQtype");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::2D::HighQtransmission
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::2D::HighQtransmission");
    if (checkBoxParallaxTr->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;

    //--- Options::2D::HighQwindow
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::2D::HighQwindow");
    if (checkBoxWaTrDet->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //--- Options::2D::DeadTimeModel
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::DeadTimeModel");
    if (radioButtonDeadTimeCh->isChecked())
	w->setText(currentRow,1,"Non-Paralysable <");
    else
	w->setText(currentRow,1,"Paralysable <");       
    currentRow++;
    
    //--- Options::2D::FindCenterMethod
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::FindCenterMethod");
    if (radioButtonRadStdSymm->isChecked())
	w->setText(currentRow,1,"X and Y symmetrization <");
    else if (radioButtonCenterHF->isChecked())
	w->setText(currentRow,1,"Moment-minimalization (H.F.) <");       
    else w->setText(currentRow,1,"Header <");       
    currentRow++;
    
    //--- Options::2D::OutputFormat
    w->setNumRows(currentRow+1);            
    s=QString::number(comboBoxIxyFormat->currentIndex());
    w->setText(currentRow,0,"Options::2D::OutputFormat");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::2D::HeaderOutputFormat
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::2D::HeaderOutputFormat");
    if (checkBoxASCIIheaderIxy->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;

    //--- Options::2D::HeaderSASVIEW
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::2D::HeaderSASVIEW");
    if (checkBoxASCIIheaderSASVIEW->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Options::1D::SASpresentation
    w->setNumRows(currentRow+1);            
    s=comboBoxSelectPresentation->currentText();
    w->setText(currentRow,0,"Options::1D::SASpresentation");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::1D::I[Q]::Format
    w->setNumRows(currentRow+1);            
    s=comboBox4thCol->currentText();
    w->setText(currentRow,0,"Options::1D::I[Q]::Format");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //+++ Options::1D::I[Q]::PlusHeader
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::I[Q]::PlusHeader");
    if (checkBoxASCIIheader->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Options::1D::I[Q]::Anisotropy
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::I[Q]::Anisotropy");
    if (checkBoxAnisotropy->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Options::1D::I[Q]::AnisotropyAngle
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::I[Q]::AnisotropyAngle");
    s=QString::number(spinBoxAnisotropyOffset->value())+" <";
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //+++Options::1D::TransmissionMethod
    w->setNumRows(currentRow+1);            
    s=comboBoxTransmMethod->currentText();
    w->setText(currentRow,0,"Options::1D::TransmissionMethod");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    
    //+++ Options::1D::Slices
    w->setNumRows(currentRow+1);            
    s=QString::number(spinBoxFrom->value())+" ";
    s+=QString::number(spinBoxTo->value());
    w->setText(currentRow,0,"Options::1D::Slices");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::1D::SlicesBS
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::SlicesBS");
    if (checkBoxSlicesBS->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Options::1D::SkipPoins
    w->setNumRows(currentRow+1);            
    s=QString::number(spinBoxRemoveFirst->value())+" ";
    s+=QString::number(spinBoxRemoveLast->value());
    w->setText(currentRow,0,"Options::1D::SkipPoins");
    w->setText(currentRow,1,s+" <");
    currentRow++;
    
    //--- Options::1D::I[Q]::Method
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::1D::I[Q]::Method");
    if (radioButtonRadHF->isChecked())
	w->setText(currentRow,1,"4-Pixel Interpolation (H.F.) <");
    else
	w->setText(currentRow,1,"Standard (Shells) <");       
    currentRow++;
    
    //+++ Options::1D::I[Q]::LinearFactor
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::I[Q]::LinearFactor");
    s=QString::number(spinBoxAvlinear->value())+" <";
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //+++ Options::1D::I[Q]::ProgressiveFactor
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::1D::I[Q]::ProgressiveFactor");
    s=QString::number(doubleSpinBoxAvLog->value())+" <";
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //--- Options::ScriptTable::RecalculateOldFiles
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::RecalculateOldFiles");
    if (checkBoxRecalculate->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::UseSamplePositionAsParameter
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::UseSamplePositionAsParameter");
    if (checkBoxRecalculateUseNumber->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::UseAttenuatorAsParameter
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::UseAttenuatorAsParameter");
    if (checkBoxAttenuatorAsPara->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::BeamCenterAsCondition
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::BeamCenterAsCondition");
    if (checkBoxBeamcenterAsPara->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::PolarizationAsCondition
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::PolarizationAsCondition");
    if (checkBoxPolarizationAsPara->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::DetectorAngleAsCondition
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::ScriptTable::DetectorAngleAsCondition");
    if (checkBoxDetRotAsPara->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;

    //--- Options::ScriptTable::FindCenterEveryFile
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::FindCenterEveryFile");
    if (checkBoxFindCenter->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::MergingTemplate
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::MergingTemplate");
    if (checkBoxMergingTable->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //--- Options::ScriptTable::AutoMerging
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::ScriptTable::AutoMerging");
    if (checkBoxAutoMerging->isChecked())
        w->setText(currentRow,1,"yes <");
    else
        w->setText(currentRow,1,"no <");
    currentRow++;
    
    //+++ Options::Overlap::Merging
    w->setNumRows(currentRow+1);
    w->setText(currentRow,0,"Options::Overlap::Merging");
    s=QString::number(spinBoxOverlap->value())+" <";
    w->setText(currentRow, 1, s);
    currentRow++;
    
    //+++ Options::Rewrite Output (No index)
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::RewriteOutput");
    if (checkBoxRewriteOutput->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::ScriptTable::Transmission::ForceCopyPaste
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::Transmission::ForceCopyPaste");
    if (checkBoxForceCopyPaste->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::ScriptTable::LabelAsName
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::LabelAsName");
    if (checkBoxNameAsTableName->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    
    //+++ Options::ScriptTable::Transmission::SkiptTrConfigurations
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::Transmission::SkiptTrConfigurations");
    if (checkBoxSkiptransmisionConfigurations->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::ScriptTable::Output::Folders
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::ScriptTable::Output::Folders");
    if (checkBoxSortOutputToFolders->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::Resolusion::Focusing
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Resolusion::Focusing");
    if (checkBoxResoFocus->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;    
    
    //+++ Options::Resolusion::CA::Round
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Resolusion::CA::Round");
    if (checkBoxResoCAround->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    //+++ Options::Resolusion::SA::Round
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Resolusion::SA::Round");
    if (checkBoxResoSAround->isChecked())
	w->setText(currentRow,1,"yes <");
    else
	w->setText(currentRow,1,"no <");       
    currentRow++;
    
    
    //+++ Options::Resolusion::Detector
    w->setNumRows(currentRow+1);        
    w->setText(currentRow,0,"Options::Resolusion::Detector");
    s=lineEditDetReso->text()+" <";
    w->setText(currentRow, 1, s);
    currentRow++;  
    
    //+++ Options::Polarization::Alias::Up
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Alias::Up");
    s = lineEditUp->text() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;
    //+++ Options::Polarization::Alias::Down
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Alias::Down");
    s = lineEditDown->text() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;
    //+++ Options::Polarization::Alias::UpUp
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Alias::UpUp");
    s = lineEditUpUp->text() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;
    //+++ Options::Polarization::Alias::Up-Down
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Alias::UpDown");
    s = lineEditUpDown->text() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;
    //+++ Options::Polarization::Alias::DownDown
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Alias::DownDown");
    s = lineEditDownDown->text() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;
    //+++ Options::Polarization::Alias::DownUp
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Alias::DownUp");
    s = lineEditDownUp->text() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;

    //+++ Options::Polarization::Pol::Polarization
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Pol::Polarization");
    s = polarizationSelector->writeSettingsString() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;
    //+++ Options::Polarization::Pol::Transmission
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Pol::Transmission");
    s = polTransmissionSelector->writeSettingsString() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;
    //+++ Options::Polarization::Pol::Flipper::Efficiency
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Pol::Flipper::Efficiency");
    s = polFlipperEfficiencySelector->writeSettingsString() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;

    //+++ Options::Polarization::Analyzer::Transmission
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Analyzer::Transmission");
    s = analyzerTransmissionSelector->writeSettingsString() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;
    //+++ Options::Polarization::Analyzer::Efficiency
    w->setNumRows(currentRow + 1);
    w->setText(currentRow, 0, "Options::Polarization::Analyzer::Efficiency");
    s = analyzerEfficiencySelector->writeSettingsString() + " <";
    w->setText(currentRow, 1, s);
    currentRow++;
    
    w->adjustColumnsWidth(false);
}

void dan18::SetColNumberNew( int cols )
{

    int oldCols=tableEC->columnCount();
    if (cols==oldCols) return;

    tableEC->setColumnCount(cols);
    for (int i=oldCols; i<cols; i++) tableEC->setHorizontalHeaderItem(i,new QTableWidgetItem(QString::number(i+1)));
    for(int i=oldCols;i<cols;i++) for (int ii=0; ii<tableEC->rowCount();ii++) tableEC->setItem(ii, i, new QTableWidgetItem);
    
    int spaceNew = buttonGroupMode0->width()-30 - tableEC->verticalHeader()->width() - tableEC->verticalScrollBar()->width()-1;
    
    tableEC->horizontalHeader()->setDefaultSectionSize(int(spaceNew/cols));
    tableEC->horizontalHeader()->setStretchLastSection(true);
    
    addMaskAndSens(cols, oldCols);
}

void dan18::activeScriptTableSelected(int newLine)
{
    QString newActiveScript=comboBoxMakeScriptTable->currentText();
    
    readSettingNew(newActiveScript+"-Settings");
    SensitivityLineEditCheck();
}

//*******************************************
//+++  Read Settings from Table
//*******************************************
bool dan18::readSettingNew(QString tableName)
{
    Table *w;
    foreach (MdiSubWindow *t, app()->tableList())
        if (t->name() == tableName)
        {
            w = (Table *)t;
            break;
        }
    if (!w)
    {
        QMessageBox::critical(nullptr, "qtiSAS", "Table ~" + tableName + "~ does not exist!!! <br><h4>");
        return false;
    }

    static const QRegularExpression rx(R"([-+]?\b\d+(\.\d+)?([eE][-+]?\d+)?\b)");
    QRegularExpressionMatchIterator irx;

    QStringList parameters;
    QString s;
    int index;

    for (int i = 0; i < w->numRows(); i++)
        parameters << w->text(i, 0);

    //+++ Sensitivity::Number
    index = static_cast<int>(parameters.indexOf("Sensitivity::Numbers"));
    if (index >= 0)
    {
        s = w->text(index, 1).remove(" <").simplified();
        QStringList lst = s.split(" ", Qt::SkipEmptyParts);

        if (lst[0] == "0")
            lineEditPlexiAnyD->setText("");
        else
            lineEditPlexiAnyD->setText(lst[0]);
        if (lst[1] == "0")
            lineEditEBAnyD->setText("");
        else
            lineEditEBAnyD->setText(lst[1]);
        if (lst[2] == "0")
            lineEditBcAnyD->setText("");
        else
            lineEditBcAnyD->setText(lst[2]);

        lineEditTransAnyD->setText(QString::number(lst[3].toDouble(), 'f', 4));
        SensitivityLineEditCheck();
    }

    //+++ Options::Instrument
    index = static_cast<int>(parameters.indexOf("Options::Instrument"));
    if (index >= 0)
    {
        QString newInstr = w->text(index, 1).remove(" <").trimmed();
        bool exist = false;

        if (comboBoxInstrument->findText(newInstr) >= 0)
        {
            comboBoxInstrument->setCurrentIndex(comboBoxInstrument->findText(newInstr));
            instrumentSelected();
            exist = true;
        }

        if (!exist)
            QMessageBox::critical(nullptr, "qtiSAS",
                                  "SA(N)S instrument " + newInstr + " does not exist. Create/Copy first!!!");
    }

    //+++ Options::Mode
    index = static_cast<int>(parameters.indexOf("Options::Mode"));
    if (index >= 0)
    {
        QString newMode = w->text(index, 1).remove(" <");

        for (int i = 0; i < comboBoxMode->count(); i++)
            if (comboBoxMode->itemText(i) == newMode)
            {
                comboBoxMode->setCurrentIndex(i);
                break;
            }
    }
    
    //+++ Settings::Path::Input
    index = static_cast<int>(parameters.indexOf("Settings::Path::Input"));
    if (index >= 0)
        lineEditPathDAT->setText(w->text(index, 1).remove(" <"));

    //+++ Settings::Input::Filter
    index = static_cast<int>(parameters.indexOf("Settings::Input::Filter"));
    if (index >= 0)
        textEditPattern->setText(w->text(index, 1).remove(" <"));

    //+++ Settings::Path::DirsInDir
    index = static_cast<int>(parameters.indexOf("Settings::Path::DirsInDir"));
    if (index >= 0)
        checkBoxDirsIndir->setChecked(w->text(index, 1).contains("yes"));

    //+++ Settings::Path::Output
    index = static_cast<int>(parameters.indexOf("Settings::Path::Output"));
    if (index >= 0)
        lineEditPathRAD->setText(w->text(index, 1).remove(" <"));

    //+++ Mask::Edges
    index = static_cast<int>(parameters.indexOf("Mask::Edges"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        if (irx.hasNext())
            spinBoxLTx->setValue(irx.next().captured(0).toInt());
        if (irx.hasNext())
            spinBoxLTy->setValue(irx.next().captured(0).toInt());
        if (irx.hasNext())
            spinBoxRBx->setValue(irx.next().captured(0).toInt());
        if (irx.hasNext())
            spinBoxRBy->setValue(irx.next().captured(0).toInt());
    }
    
    //+++ Mask::Edges::Shape
    index = static_cast<int>(parameters.indexOf("Mask::Edges::Shape"));
    if (index >= 0)
    {
        if (w->text(index, 1).contains("Rectangle"))
            comboBoxMaskEdgeShape->setCurrentIndex(0);
        else
            comboBoxMaskEdgeShape->setCurrentIndex(1);
    }

    //+++ Mmask::BeamStop
    index = static_cast<int>(parameters.indexOf("Mask::BeamStop"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        if (irx.hasNext())
            spinBoxLTxBS->setValue(irx.next().captured(0).toInt());
        if (irx.hasNext())
            spinBoxLTyBS->setValue(irx.next().captured(0).toInt());
        if (irx.hasNext())
            spinBoxRBxBS->setValue(irx.next().captured(0).toInt());
        if (irx.hasNext())
            spinBoxRByBS->setValue(irx.next().captured(0).toInt());
    }

    //+++ Mask::BeamStop::Shape
    index = static_cast<int>(parameters.indexOf("Mask::BeamStop::Shape"));
    if (index >= 0)
    {
        if (w->text(index, 1).contains("Rectangle"))
            comboBoxMaskBeamstopShape->setCurrentIndex(0);
        else
            comboBoxMaskBeamstopShape->setCurrentIndex(1);
    }

    //+++ Mask::Dead::Rows
    index = static_cast<int>(parameters.indexOf("Mask::Dead::Rows"));
    if (index >= 0)
        lineEditDeadRows->setText(w->text(index, 1).remove(" <"));

    //+++ Mask::Dead::Cols
    index = static_cast<int>(parameters.indexOf("Mask::Dead::Cols"));
    if (index >= 0)
        lineEditDeadCols->setText(w->text(index, 1).remove(" <"));

    //+++ Mask::Triangular
    index = static_cast<int>(parameters.indexOf("Mask::Triangular"));
    if (index >= 0)
        lineEditMaskPolygons->setText(w->text(index, 1).remove(" <"));

    //+++ Sensitivity::Error::Range
    index = static_cast<int>(parameters.indexOf("Sensitivity::Error::Range"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        if (irx.hasNext())
            spinBoxErrLeftLimit->setValue(irx.next().captured(0).toDouble());
        if (irx.hasNext())
            spinBoxErrRightLimit->setValue(irx.next().captured(0).toDouble());
    }

    //+++ Sensitivity::Error::Matrix
    index = static_cast<int>(parameters.indexOf("Sensitivity::Error::Matrix"));
    if (index >= 0)
        checkBoxSensError->setChecked(w->text(index, 1).contains("yes"));

    //+++ Sensitivity::Tr::Option
    index = static_cast<int>(parameters.indexOf("Sensitivity::Tr::Option"));
    if (index >= 0)
        checkBoxSensTr->setChecked(w->text(index, 1).contains("yes"));
    
    //+++ Processing::Conditions::Number
    int imax = 0;
    index = static_cast<int>(parameters.indexOf("Processing::Conditions::Number"));
    if (index >= 0)
    {
        imax = w->text(index, 1).remove(" <").trimmed().toInt();
        sliderConfigurations->setValue(imax);
    }

    //+++ Processing::EC
    index = static_cast<int>(parameters.indexOf("Processing::EC"));
    if (index >= 0)
    {
        QStringList lst = w->text(index, 1).remove(" <").simplified().split(" ", Qt::SkipEmptyParts);
        for (int i = 0; i < imax; i++)
            if (lst[i] == "0")
                tableEC->item(dptEC, i)->setText("");
            else
                tableEC->item(dptEC, i)->setText(lst[i]);
    }

    //+++ Processing::BC
    index = static_cast<int>(parameters.indexOf("Processing::BC"));
    if (index >= 0)
    {
        QStringList lst = w->text(index, 1).remove(" <").simplified().split(" ", Qt::SkipEmptyParts);
        for (int i = 0; i < imax; i++)
            if (lst[i] == "0")
                tableEC->item(dptBC, i)->setText("");
            else
                tableEC->item(dptBC, i)->setText(lst[i]);
    }

    //+++ Processing::EB
    index = static_cast<int>(parameters.indexOf("Processing::EB"));
    if (index >= 0)
    {
        QStringList lst = w->text(index, 1).remove(" <").simplified().split(" ", Qt::SkipEmptyParts);
        for (int i = 0; i < imax; i++)
            if (lst[i] == "0")
                tableEC->item(dptEB, i)->setText("");
            else
                tableEC->item(dptEB, i)->setText(lst[i]);
    }

    //+++ Processing::C[m]
    index = static_cast<int>(parameters.indexOf("Processing::C[m]"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        for (int i = 0; i < imax; i++)
            if (irx.hasNext())
                tableEC->item(dptC, i)->setText(irx.next().captured(0));
    }

    //+++ Processing::D[m]
    index = static_cast<int>(parameters.indexOf("Processing::D[m]"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        for (int i = 0; i < imax; i++)
            if (irx.hasNext())
                tableEC->item(dptD, i)->setText(irx.next().captured(0));
    }

    //+++ Processing::Lambda[A]
    index = static_cast<int>(parameters.indexOf("Processing::Lambda[A]"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        for (int i = 0; i < imax; i++)
            if (irx.hasNext())
                tableEC->item(dptWL, i)->setText(irx.next().captured(0));
    }

    //+++ Processing::Beam::Size
    index = static_cast<int>(parameters.indexOf("Processing::Beam::Size"));
    if (index >= 0)
    {
        s = w->text(index, 1);
        for (int i = 0; i < imax; i++)
        {
            s = s.trimmed();
            tableEC->item(dptBSIZE, i)->setText(s.left(s.indexOf(" ")));
            s = s.remove(0, s.indexOf(" "));
            s = s.trimmed();
        }
    }

    //+++ Processing::Polarization
    index = static_cast<int>(parameters.indexOf("Processing::Polarization"));
    if (index >= 0)
    {
        QStringList lst = w->text(index, 1).remove(" <").simplified().split(" ", Qt::SkipEmptyParts);
        if (lst.count() == imax)
            for (int i = 0; i < imax; i++)
                if (lst[i].toInt() <= 6 && lst[i].toInt() >= 0)
                    ((QComboBoxInTable *)tableEC->cellWidget(dptPOL, i))->setCurrentIndex(lst[i].toInt());
    }

    //+++ Processing::Transm::EC
    index = static_cast<int>(parameters.indexOf("Processing::Transm::EC"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        for (int i = 0; i < imax; i++)
            if (irx.hasNext())
                tableEC->item(dptECTR, i)->setText(irx.next().captured(0));
    }	

    //+++ Processing::Transm::EC::Activity
    index = static_cast<int>(parameters.indexOf("Processing::Transm::EC::Activity"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        for (int i = 0; i < imax; i++)
            if (irx.hasNext())
            {
                if (irx.next().captured(0).toInt() == 1)
                    tableEC->item(dptECTR, i)->setCheckState(Qt::Checked);
                else
                    tableEC->item(dptECTR, i)->setCheckState(Qt::Unchecked);
            }
    }

    //+++ Processing::Plexi::Plexi
    index = static_cast<int>(parameters.indexOf("Processing::Plexi::Plexi"));
    if (index >= 0)
    {
        QStringList lst = w->text(index, 1).remove(" <").simplified().split(" ", Qt::SkipEmptyParts);
        for (int i = 0; i < imax; i++)
            if (lst[i] == "0")
                tableEC->item(dptACFS, i)->setText("");
            else
                tableEC->item(dptACFS, i)->setText(lst[i]);
    }

    //+++ Processing::Plexi::EB
    index = static_cast<int>(parameters.indexOf("Processing::Plexi::EB"));
    if (index >= 0)
    {
        QStringList lst = w->text(index, 1).remove(" <").simplified().split(" ", Qt::SkipEmptyParts);
        for (int i = 0; i < imax; i++)
            if (lst[i] == "0")
                tableEC->item(dptACEB, i)->setText("");
            else
                tableEC->item(dptACEB, i)->setText(lst[i]);
    }

    //+++ Processing::Plexi::BC
    index = static_cast<int>(parameters.indexOf("Processing::Plexi::BC"));
    if (index >= 0)
    {
        QStringList lst = w->text(index, 1).remove(" <").simplified().split(" ", Qt::SkipEmptyParts);
        for (int i = 0; i < imax; i++)
            if (lst[i] == "0")
                tableEC->item(dptACBC, i)->setText("");
            else
                tableEC->item(dptACBC, i)->setText(lst[i]);
    }

    //+++ Processing::AC::DAC
    index = static_cast<int>(parameters.indexOf("Processing::AC::DAC"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        for (int i = 0; i < imax; i++)
            if (irx.hasNext())
                tableEC->item(dptDAC, i)->setText(irx.next().captured(0));
    }

    //+++ Processing::AC::MU
    index = static_cast<int>(parameters.indexOf("Processing::AC::MU"));
    if (index >= 0)
    {
        s = w->text(index, 1);
        for (int i = 0; i < imax; i++)
        {
            s = s.trimmed();
            tableEC->item(dptACMU, i)->setText(s.left(s.indexOf(" ")));
            s = s.remove(0, s.indexOf(" "));
            s = s.trimmed();
        }
    }

    //+++ Processing::AC::TR
    index = static_cast<int>(parameters.indexOf("Processing::AC::TR"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        for (int i = 0; i < imax; i++)
            if (irx.hasNext())
                tableEC->item(dptACTR, i)->setText(irx.next().captured(0));
    }

    //+++ Processing::AC::Factor
    index = static_cast<int>(parameters.indexOf("Processing::AC::Factor"));
    if (index >= 0)
    {
        s = w->text(index, 1);
        for (int i = 0; i < imax; i++)
        {
            s = s.trimmed();
            tableEC->item(dptACFAC, i)->setText(s.left(s.indexOf(" ")));
            s = s.remove(0, s.indexOf(" "));
            s = s.trimmed();
        }
    }

    //+++ Processing::Center::File
    index = static_cast<int>(parameters.indexOf("Processing::Center::File"));
    if (index >= 0)
    {
        QStringList lst = w->text(index, 1).remove(" <").simplified().split(" ", Qt::SkipEmptyParts);
        for (int i = 0; i < imax; i++)
            if (lst[i] == "0")
                tableEC->item(dptCENTER, i)->setText("");
            else
                tableEC->item(dptCENTER, i)->setText(lst[i]);
    }

    //+++ Processing::X-center
    index = static_cast<int>(parameters.indexOf("Processing::X-center"));
    if (index >= 0)
    {
        s = w->text(index, 1);
        for (int i = 0; i < imax; i++)
        {
            s = s.trimmed();
            tableEC->item(dptCENTERX, i)->setText(s.left(s.indexOf(" ")));
            s = s.remove(0, s.indexOf(" "));
            s = s.trimmed();
        }
    }

    //+++ Processing::Y-center
    index = static_cast<int>(parameters.indexOf("Processing::Y-center"));
    if (index >= 0)
    {
        s = w->text(index, 1);
        for (int i = 0; i < imax; i++)
        {
            s = s.trimmed();
            tableEC->item(dptCENTERY, i)->setText(s.left(s.indexOf(" ")));
            s = s.remove(0, s.indexOf(" "));
            s = s.trimmed();
        }
    }
    vertHeaderTableECPressed(dptMASK, false);
    vertHeaderTableECPressed(dptSENS, false);
    vertHeaderTableECPressed(dptMASKTR, false);

    //+++ Processing::Mask
    index = static_cast<int>(parameters.indexOf("Processing::Mask"));
    if (index >= 0)
    {
        s = w->text(index, 1);
        for (int i = 0; i < imax; i++)
        {
            s = s.trimmed();
            auto mask = (QComboBoxInTable *)tableEC->cellWidget(dptMASK, i);
            mask->setCurrentIndex(mask->findText("mask"));
            mask->setCurrentIndex(mask->findText(s.left(s.indexOf(" "))));
            s = s.remove(0, s.indexOf(" "));
            s = s.trimmed();
        }
    }

    //+++ Processing::MaskTr
    index = static_cast<int>(parameters.indexOf("Processing::Tr::Mask"));
    if (index >= 0)
    {
        s = w->text(index, 1);
        for (int i = 0; i < imax; i++)
        {
            s = s.trimmed();
            auto mask = (QComboBoxInTable *)tableEC->cellWidget(dptMASKTR, i);
            mask->setCurrentIndex(mask->findText("mask"));
            mask->setCurrentIndex(mask->findText(s.left(s.indexOf(" "))));
            s = s.remove(0, s.indexOf(" "));
            s = s.trimmed();
        }
    }

    //+++ Processing::Sensitivity
    index = static_cast<int>(parameters.indexOf("Processing::Sensitivity"));
    if (index >= 0)
    {
        s = w->text(index, 1);
        for (int i = 0; i < imax; i++)
        {
            s = s.trimmed();
            auto sens = (QComboBoxInTable *)tableEC->cellWidget(dptSENS, i);
            sens->setCurrentIndex(sens->findText("sens"));
            sens->setCurrentIndex(sens->findText(s.left(s.indexOf(" "))));
            s = s.remove(0, s.indexOf(" "));
            s = s.trimmed();
        }
    }

    //+++ Processing::File::Ext
    index = static_cast<int>(parameters.indexOf("Processing::File::Ext"));
    if (index >= 0)
        lineEditFileExt->setText(w->text(index, 1).remove(" <"));

    //+++ Processing::scriptPolarized
    index = static_cast<int>(parameters.indexOf("Processing::scriptPolarized"));
    if (index >= 0)
    {
        QString polScript = w->text(index, 1).remove(" <");
        if (comboBoxPolarizationScriptTable->findText(polScript) >= 0)
            comboBoxPolarizationScriptTable->setCurrentIndex(comboBoxPolarizationScriptTable->findText(polScript));
    }

    //+++ Options::Instrument::DeadTime::Homogenity
    index = static_cast<int>(parameters.indexOf("Options::Instrument::DeadTime::Homogenity"));
    if (index >= 0)
    {
        s = w->text(index, 1).remove(" <");
        if (comboBoxDTtype->findText(s) >= 0)
            comboBoxDTtype->setCurrentIndex(comboBoxDTtype->findText(s));
    }

    //+++ Options::Calibrant::Type
    index = static_cast<int>(parameters.indexOf("Options::Calibrant::Type"));
    if (index >= 0)
    {
        s = w->text(index, 1).remove(" <");
        if (s.contains("Direct Beam"))
            comboBoxACmethod->setCurrentIndex(1);
        else if (s.contains("Flat Scatter + Transmission"))
            comboBoxACmethod->setCurrentIndex(2);
        else if (s.contains("Counts per Channel"))
            comboBoxACmethod->setCurrentIndex(3);
        else
            comboBoxACmethod->setCurrentIndex(0);
    }

    //+++ Options::Calibrant::Active::Mask::Sens
    index = static_cast<int>(parameters.indexOf("Options::Calibrant::Active::Mask::Sens"));
    if (index >= 0)
        checkBoxACDBuseActive->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::Calibrant
    index = static_cast<int>(parameters.indexOf("Options::Calibrant"));
    if (index >= 0)
    {
        s = w->text(index, 1).remove(" <");
        if (comboBoxCalibrant->findText(s) >= 0)
        {
            comboBoxCalibrant->setCurrentIndex(comboBoxCalibrant->findText(s));
            calibrantselected();
        }
    }

    //+++ Options::Calibrant::CalculateTr
    index = static_cast<int>(parameters.indexOf("Options::Calibrant::CalculateTr"));
    if (index >= 0)
        checkBoxTransmissionPlexi->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::2D::Normalization
    index = static_cast<int>(parameters.indexOf("Options::2D::Normalization"));
    if (index >= 0)
    {
        s = w->text(index, 1).remove(" <");
        if (comboBoxNorm->findText(s) >= 0)
            comboBoxNorm->setCurrentIndex(comboBoxNorm->findText(s));
    }

    //+++ Options::2D::Normalization::Constant
    index = static_cast<int>(parameters.indexOf("Options::2D::Normalization::Constant"));
    if (index >= 0)
        spinBoxNorm->setValue(w->text(index, 1).remove(" <").toInt());

    //+++ Options::2D::Normalization::BC::Normalization
    index = static_cast<int>(parameters.indexOf("Options::2D::Normalization::BC::Normalization"));
    if (index >= 0)
        checkBoxBCTimeNormalization->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::2D::xyDimension::Pixel
    index = static_cast<int>(parameters.indexOf("Options::2D::xyDimension::Pixel"));
    if (index >= 0)
    {
        s = w->text(index, 1);
        if (s.contains("yes"))
        {
            radioButtonXYdimPixel->setChecked(true);
            radioButtonXYdimQ->setChecked(false);
        }
        else
        {
            radioButtonXYdimQ->setChecked(true);
            radioButtonXYdimPixel->setChecked(false);
        }
    }

    //+++ Options::2D::RemoveNegativePoints::2D
    index = static_cast<int>(parameters.indexOf("Options::2D::RemoveNegativePoints::2D"));
    if (index >= 0)
        checkBoxMaskNegative->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::2D::RemoveNegativePoints::1D
    index = static_cast<int>(parameters.indexOf("Options::2D::RemoveNegativePoints::1D"));
    if (index >= 0)
        checkBoxMaskNegativeQ->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::2D::Polar::Resolusion
    index = static_cast<int>(parameters.indexOf("Options::2D::Polar::Resolusion"));
    if (index >= 0)
        spinBoxPolar->setValue(w->text(index, 1).remove(" <").toInt());

    //+++ Options::2D::HighQcorrection
    index = static_cast<int>(parameters.indexOf("Options::2D::HighQcorrection"));
    if (index >= 0)
        checkBoxParallax->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::2D::HighQtype
    index = static_cast<int>(parameters.indexOf("Options::2D::HighQtype"));
    if (index >= 0)
    {
        s = w->text(index, 1).remove(" <");
        if (comboBoxParallax->findText(s) >= 0)
            comboBoxParallax->setCurrentIndex(comboBoxParallax->findText(s));
    }

    //+++ Options::2D::HighQtransmission
    index = static_cast<int>(parameters.indexOf("Options::2D::HighQtransmission"));
    if (index >= 0)
        checkBoxParallaxTr->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::2D::HighQwindow
    index = static_cast<int>(parameters.indexOf("Options::2D::HighQwindow"));
    if (index >= 0)
        checkBoxWaTrDet->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::2D::DeadTimeModel
    index = static_cast<int>(parameters.indexOf("Options::2D::DeadTimeModel"));
    if (index >= 0)
        radioButtonDeadTimeCh->setChecked(w->text(index, 1).contains("Non-Paralysable"));

    //+++ Options::2D::FindCenterMethod
    index = static_cast<int>(parameters.indexOf("Options::2D::FindCenterMethod"));
    if (index >= 0)
    {
        s = w->text(index, 1);
        if (s.contains("X and Y symmetrization"))
        {
            radioButtonCenterHF->setChecked(false);
            radioButtonRadStdSymm->setChecked(true);
            radioButtonCenterReadFromHeader->setChecked(false);
        }
        else if (s.contains("Moment-minimalization (H.F.)"))
        {
            radioButtonCenterHF->setChecked(true);
            radioButtonRadStdSymm->setChecked(false);
            radioButtonCenterReadFromHeader->setChecked(false);
        }
        else
        {
            radioButtonCenterHF->setChecked(false);
            radioButtonRadStdSymm->setChecked(false);
            radioButtonCenterReadFromHeader->setChecked(true);
        }
    }

    //+++ Options::2D::OutputFormat
    index = static_cast<int>(parameters.indexOf("Options::2D::OutputFormat"));
    if (index >= 0)
        comboBoxIxyFormat->setCurrentIndex(w->text(index, 1).remove(" <").toInt());

    //+++ Options::2D::HeaderOutputFormat
    index = static_cast<int>(parameters.indexOf("Options::2D::HeaderOutputFormat"));
    if (index >= 0)
        checkBoxASCIIheaderIxy->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::2D::HeaderSASVIEW
    index = static_cast<int>(parameters.indexOf("Options::2D::HeaderSASVIEW"));
    if (index >= 0)
        checkBoxASCIIheaderSASVIEW->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::1D::SASpresentation
    index = static_cast<int>(parameters.indexOf("Options::1D::SASpresentation"));
    if (index >= 0)
    {
        s = w->text(index, 1).remove(" <");
        if (comboBoxSelectPresentation->findText(s) >= 0)
        {
            comboBoxSelectPresentation->setCurrentIndex(comboBoxSelectPresentation->findText(s));
            sasPresentation();
        }
    }

    //+++ Options::1D::I[Q]::Format
    index = static_cast<int>(parameters.indexOf("Options::1D::I[Q]::Format"));
    if (index >= 0)
    {
        s = w->text(index, 1).remove(" <");
        if (comboBox4thCol->findText(s) >= 0)
            comboBox4thCol->setCurrentIndex(comboBox4thCol->findText(s));
    }

    //+++ Options::1D::I[Q]::PlusHeader
    index = static_cast<int>(parameters.indexOf("Options::1D::I[Q]::PlusHeader"));
    if (index >= 0)
        checkBoxASCIIheader->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::1D::I[Q]::Anisotropy
    index = static_cast<int>(parameters.indexOf("Options::1D::I[Q]::Anisotropy"));
    if (index >= 0)
        checkBoxAnisotropy->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::1D::I[Q]::AnisotropyAngle
    index = static_cast<int>(parameters.indexOf("Options::1D::I[Q]::AnisotropyAngle"));
    if (index >= 0)
        spinBoxAnisotropyOffset->setValue(w->text(index, 1).remove(" <").toInt());

    //+++Options::1D::TransmissionMethod
    index = static_cast<int>(parameters.indexOf("Options::1D::TransmissionMethod"));
    if (index >= 0)
    {
        s = w->text(index, 1).remove(" <");
        if (comboBoxTransmMethod->findText(s) >= 0)
            comboBoxTransmMethod->setCurrentIndex(comboBoxTransmMethod->findText(s));
    }

    //+++ Options::1D::Slices
    index = static_cast<int>(parameters.indexOf("Options::1D::Slices"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        if (irx.hasNext())
            spinBoxFrom->setValue(irx.next().captured(0).toInt());
        if (irx.hasNext())
            spinBoxTo->setValue(irx.next().captured(0).toInt());
    }

    //+++ Options::1D::SlicesBS
    index = static_cast<int>(parameters.indexOf("Options::1D::SlicesBS"));
    if (index >= 0)
        checkBoxSlicesBS->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::1D::SkipPoins
    index = static_cast<int>(parameters.indexOf("Options::1D::SkipPoins"));
    if (index >= 0)
    {
        irx = rx.globalMatch(w->text(index, 1));
        if (irx.hasNext())
            spinBoxRemoveFirst->setValue(irx.next().captured(0).toInt());
        if (irx.hasNext())
            spinBoxRemoveLast->setValue(irx.next().captured(0).toInt());
    }	

    //+++ Options::1D::I[Q]::Method
    index = static_cast<int>(parameters.indexOf("Options::1D::I[Q]::Method"));
    if (index >= 0)
        radioButtonRadHF->setChecked(w->text(index, 1).contains("4-Pixel Interpolation (H.F.)"));

    //+++ Options::1D::I[Q]::LinearFactor
    index = static_cast<int>(parameters.indexOf("Options::1D::I[Q]::LinearFactor"));
    if (index >= 0)
        spinBoxAvlinear->setValue(w->text(index, 1).remove(" <").toInt());

    //+++ Options::1D::I[Q]::ProgressiveFactor
    index = static_cast<int>(parameters.indexOf("Options::1D::I[Q]::ProgressiveFactor"));
    if (index >= 0)
        doubleSpinBoxAvLog->setValue(w->text(index, 1).remove(" <").toDouble());

    //+++ Options::ScriptTable::RecalculateOldFiles
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::RecalculateOldFiles"));
    if (index >= 0)
        checkBoxRecalculate->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::UseSamplePositionAsParameter
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::UseSamplePositionAsParameter"));
    if (index >= 0)
        checkBoxRecalculateUseNumber->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::UseAttenuatorAsParameter
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::UseAttenuatorAsParameter"));
    if (index >= 0)
        checkBoxAttenuatorAsPara->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::BeamCenterAsCondition
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::BeamCenterAsCondition"));
    if (index >= 0)
        checkBoxBeamcenterAsPara->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::PolarizationCondition
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::PolarizationAsCondition"));
    if (index >= 0)
        checkBoxPolarizationAsPara->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::DetectorAngleAsCondition
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::DetectorAngleAsCondition"));
    if (index >= 0)
        checkBoxDetRotAsPara->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::FindCenterEveryFile
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::FindCenterEveryFile"));
    if (index >= 0)
        checkBoxFindCenter->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::Automatical generation of merging template (I[Q] case)
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::MergingTemplate"));
    if (index >= 0)
        checkBoxMergingTable->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::AutoMerging
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::AutoMerging"));
    if (index >= 0)
        checkBoxAutoMerging->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::Overlap::Merging
    index = static_cast<int>(parameters.indexOf("Options::Overlap::Merging"));
    if (index >= 0)
        spinBoxOverlap->setValue(w->text(index, 1).remove(" <").toInt());

    //+++ Options::Rewrite Output (No index)
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::RewriteOutput"));
    if (index >= 0)
        checkBoxRewriteOutput->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::Transmission::ForceCopyPaste"
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::Transmission::ForceCopyPaste"));
    if (index >= 0)
        checkBoxForceCopyPaste->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::LabelAsName"
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::LabelAsName"));
    if (index >= 0)
        checkBoxNameAsTableName->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::Transmission::SkiptTrConfigurations
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::Transmission::SkiptTrConfigurations"));
    if (index >= 0)
        checkBoxSkiptransmisionConfigurations->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::ScriptTable::Output::Folders
    index = static_cast<int>(parameters.indexOf("Options::ScriptTable::Output::Folders"));
    if (index >= 0)
        checkBoxSortOutputToFolders->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::Resolusion::Focusing
    index = static_cast<int>(parameters.indexOf("Options::Resolusion::Focusing"));
    if (index >= 0)
        checkBoxResoFocus->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::Resolusion::Detector
    index = static_cast<int>(parameters.indexOf("Options::Resolusion::Detector"));
    if (index >= 0)
        lineEditDetReso->setText(w->text(index, 1).remove(" <"));

    //+++ Options::Resolusion::CA::Round
    index = static_cast<int>(parameters.indexOf("Options::Resolusion::CA::Round"));
    if (index >= 0)
        checkBoxResoCAround->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::Resolusion::SA::Round
    index = static_cast<int>(parameters.indexOf("Options::Resolusion::SA::Round"));
    if (index >= 0)
        checkBoxResoSAround->setChecked(w->text(index, 1).contains("yes"));

    //+++ Options::Polarization::Alias::Up
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Alias::Up"));
    if (index >= 0)
        lineEditUp->setText(w->text(index, 1).remove(" <"));

    //+++ Options::Polarization::Alias::Down
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Alias::Down"));
    if (index >= 0)
        lineEditDown->setText(w->text(index, 1).remove(" <"));

    //+++ Options::Polarization::Alias::UpUp
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Alias::UpUp"));
    if (index >= 0)
        lineEditUpUp->setText(w->text(index, 1).remove(" <"));

    //+++ Options::Polarization::Alias::UpDown
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Alias::UpDown"));
    if (index >= 0)
        lineEditUpDown->setText(w->text(index, 1).remove(" <"));

    //+++ Options::Polarization::Alias::DownDown
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Alias::DownDown"));
    if (index >= 0)
        lineEditDownDown->setText(w->text(index, 1).remove(" <"));

    //+++ Options::Polarization::Alias::DownUp
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Alias::DownUp"));
    if (index >= 0)
        lineEditDownUp->setText(w->text(index, 1).remove(" <"));

    //+++ Options::Polarization::Pol::Polarization
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Pol::Polarization"));
    if (index >= 0)
        polarizationSelector->readSettingsString(w->text(index, 1).remove(" <"));

    //+++ Options::Polarization::Pol::Transmission
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Pol::Transmission"));
    if (index >= 0)
        polTransmissionSelector->readSettingsString(w->text(index, 1).remove(" <"));

    //+++ Options::Polarization::Pol::Flipper::Efficiency
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Pol::Flipper::Efficiency"));
    if (index >= 0)
        polFlipperEfficiencySelector->readSettingsString(w->text(index, 1).remove(" <"));

    //+++ Options::Polarization::Analyzer::Transmission
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Analyzer::Transmission"));
    if (index >= 0)
        analyzerTransmissionSelector->readSettingsString(w->text(index, 1).remove(" <"));

    //+++ Options::Polarization::Analyzer::Efficiency
    index = static_cast<int>(parameters.indexOf("Options::Polarization::Analyzer::Efficiency"));
    if (index >= 0)
        analyzerEfficiencySelector->readSettingsString(w->text(index, 1).remove(" <"));

    //+++ header name generation
    for (int i = 0; i < imax; i++)
        horHeaderTableECPressed(i, false);

    //+++ script table activation
    if (tableName.right(9) == "-Settings")
    {
        QString activeScript = tableName.remove("-Settings");
        for (int i = 1; i < comboBoxMakeScriptTable->count(); i++)
            if (comboBoxMakeScriptTable->itemText(i) == activeScript)
                comboBoxMakeScriptTable->setCurrentIndex(i);
    }
    return true;
}	

void dan18::addCopyOfLastConfiguration()
{
    int oldNumber=sliderConfigurations->value();
    
    if (sliderConfigurations->maximum()==oldNumber) return;
    
    sliderConfigurations->setValue(oldNumber+1);
    
    tableEC->item(dptEC, oldNumber)->setText(tableEC->item(dptEC, oldNumber - 1)->text());
    tableEC->item(dptBC, oldNumber)->setText(tableEC->item(dptBC, oldNumber - 1)->text());
    tableEC->item(dptC, oldNumber)->setText(tableEC->item(dptC, oldNumber - 1)->text());
    tableEC->item(dptD, oldNumber)->setText(tableEC->item(dptD, oldNumber - 1)->text());
    tableEC->item(dptWL, oldNumber)->setText(tableEC->item(dptWL, oldNumber - 1)->text());
    tableEC->item(dptBSIZE, oldNumber)->setText(tableEC->item(dptBSIZE, oldNumber - 1)->text());
    tableEC->item(dptACFS, oldNumber)->setText(tableEC->item(dptACFS, oldNumber - 1)->text());
    tableEC->item(dptACEB, oldNumber)->setText(tableEC->item(dptACEB, oldNumber - 1)->text());
    tableEC->item(dptACBC, oldNumber)->setText(tableEC->item(dptACBC, oldNumber - 1)->text());
    tableEC->item(dptDAC, oldNumber)->setText(tableEC->item(dptDAC, oldNumber - 1)->text());
    tableEC->item(dptACMU, oldNumber)->setText(tableEC->item(dptACMU, oldNumber - 1)->text());
    tableEC->item(dptACTR, oldNumber)->setText(tableEC->item(dptACTR, oldNumber - 1)->text());
    tableEC->item(dptACFAC, oldNumber)->setText(tableEC->item(dptACFAC, oldNumber - 1)->text());
    tableEC->item(dptCENTER, oldNumber)->setText(tableEC->item(dptCENTER, oldNumber - 1)->text());
    tableEC->item(dptCENTERX, oldNumber)->setText(tableEC->item(dptCENTERX, oldNumber - 1)->text());
    tableEC->item(dptCENTERY, oldNumber)->setText(tableEC->item(dptCENTERY, oldNumber - 1)->text());
    tableEC->item(dptEB, oldNumber)->setText(tableEC->item(dptEB, oldNumber - 1)->text());

    QString oldPol = ((QComboBoxInTable *)tableEC->cellWidget(dptPOL, oldNumber - 1))->currentText();
    auto *pol = (QComboBoxInTable *)tableEC->cellWidget(dptPOL, oldNumber);
    pol->setCurrentIndex(pol->findText(oldPol));

    QString oldMask=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,oldNumber-1))->currentText();
    QComboBoxInTable *mask =(QComboBoxInTable*)tableEC->cellWidget(dptMASK,oldNumber);
    mask->setCurrentIndex(mask->findText(oldMask));
    
    QString oldSens=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,oldNumber-1))->currentText();
    QComboBoxInTable *sens =(QComboBoxInTable*)tableEC->cellWidget(dptSENS,oldNumber);
    sens->setCurrentIndex(sens->findText(oldSens));
    
    if (tableEC->item(dptECTR, oldNumber - 1)->checkState() == Qt::Checked)
    {
        tableEC->item(dptECTR, oldNumber)->setCheckState(Qt::Checked);
        tableEC->item(dptECTR, oldNumber)->setText(tableEC->item(dptECTR, oldNumber - 1)->text());
    }

    oldMask=((QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,oldNumber-1))->currentText();
    QComboBoxInTable *maskTR =(QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,oldNumber);
    maskTR->setCurrentIndex(maskTR->findText(oldMask));
}

//+++
void dan18::deleteFirstConfiguration()
{
    int oldNumber=sliderConfigurations->value();
    if (oldNumber==1) return;
    
    tableEC->removeColumn(0);
    
    sliderConfigurations->setValue(oldNumber-1);
}

void dan18::addMaskAndSens(int condNumber)
{
    addMaskAndSens(condNumber, 0);
}

void dan18::addMaskAndSens(int condNumber, int oldNumber)
{
    int MD = lineEditMD->text().toInt();

    for(int i=oldNumber;i<condNumber;i++)
    {
        QTableWidgetItem *checkTrNew = new QTableWidgetItem();
        checkTrNew->setCheckState(Qt::Unchecked);
        checkTrNew->setText("1.000");
        tableEC->setItem(dptECTR,i, checkTrNew);
    }

    
    QStringList lst;
    
    for(int i=oldNumber;i<condNumber;i++)
    {
        //+++ polarization
        lst.clear();
        lst << "out" << "up" << "down" << "up-up" << "up-down" << "down-down" << "down-up";
        auto *pol = new QComboBoxInTable(dptPOL, i, tableEC);
        pol->addItems(lst);
        tableEC->setCellWidget(dptPOL, i, pol);
        pol->setCurrentIndex(0);

        //+++ mask
        findMatrixListByLabel("DAN::Mask::"+QString::number(MD),lst);
        if (!lst.contains("mask")) lst.prepend("mask");
        QComboBoxInTable *mask = new QComboBoxInTable(dptMASK,i,tableEC);
        mask->addItems(lst);
        tableEC->setCellWidget(dptMASK,i,mask);
        mask->setCurrentIndex(lst.indexOf("mask"));
        
        //+++ sens
        findMatrixListByLabel("DAN::Sensitivity::"+QString::number(MD),lst);
        if (!lst.contains("sens")) lst.prepend("sens");
        QComboBoxInTable *sens = new QComboBoxInTable(dptSENS,i,tableEC);
        sens->addItems(lst);
        tableEC->setCellWidget(dptSENS,i,sens);
        sens->setCurrentIndex(lst.indexOf("sens"));
        
        //+++ mask
        findMatrixListByLabel("DAN::Mask::"+QString::number(MD),lst);
        if (!lst.contains("mask")) lst.prepend("mask");
        QComboBoxInTable *maskTR = new QComboBoxInTable(dptMASKTR,i,tableEC);
        maskTR->addItems(lst);
        tableEC->setCellWidget(dptMASKTR,i,maskTR);
        maskTR->setCurrentIndex(lst.indexOf("mask"));
    }
}

void dan18::vertHeaderTableECPressed(int raw,  bool headerReallyPressed )
{
    int MD = lineEditMD->text().toInt();

    updateMaskList();
    updateSensList();
    
    int CDL=tableEC->columnCount();
    
    
    if (raw==dptEC)
    {
        updateColInScript("#-EC", dptEC);
        updateColInScript("#-EC [EB]", dptEC);
    }
    
    if (raw==dptBC)
    {
        updateColInScript("#-BC", dptBC);
    }
    
    if (raw==dptC)  //+++ C +++
    {
        for (int i = 0; i < CDL; i++)
        {
            QString ECnumber=tableEC->item(dptEC,i)->text();

            if (filesManager->checkFileNumber(ECnumber))
            {
                // C
                int C = int(collimation->readCinM(ECnumber));
                tableEC->item(dptC, i)->setText(QString::number(C, 'f', 0));
            }
        }
        updateColInScript("C", dptC);
    }
    
    if (raw==dptD) //+++ D +++
    {
        for (int i = 0; i < CDL; i++)
        {
            QString ECnumber=tableEC->item(dptEC,i)->text();

            if (filesManager->checkFileNumber(ECnumber))
            {
                double D = detector->readDinM(ECnumber); //[m]
                tableEC->item(dptD,i)->setText(QString::number(D,'f',3));
            }
        }
        updateColInScript("D", dptD);
    }
    if (raw==dptWL) //+++ Lambda +++
    {
        for (int i = 0; i < CDL; i++)
        {
            QString ECnumber=tableEC->item(dptEC, i)->text();

            if (filesManager->checkFileNumber(ECnumber))
            {
                double lambda = selector->readLambda(ECnumber, monitors->readDuration(ECnumber));
                tableEC->item(dptWL,i)->setText(QString::number(lambda,'f',3));
            }
        }
        updateColInScript("Lambda", dptWL);
    }
    if (raw==dptBSIZE)        //+++ Beam Size +++
    {
        for (int i = 0; i < CDL; i++)
        {
            QString ECnumber=tableEC->item(dptEC,i)->text();

            if (filesManager->checkFileNumber(ECnumber))
            {
                QString CA = collimation->readCA(ECnumber);
                QString SA = collimation->readSA(ECnumber);
                tableEC->item(dptBSIZE, i)->setText(CA + "|" + SA);
            }
        }
        updateColInScript("Beam Size", dptBSIZE);
    }

    if (raw == dptPOL) //+++ Polarization +++
    {
        for (int i = 0; i < CDL; i++)
        {
            QString ECnumber = tableEC->item(dptEC, i)->text();

            if (filesManager->checkFileNumber(ECnumber))
            {
                QString polarization = collimation->readPolarization(ECnumber);
                int currentValue = polarizedAlias(polarization);
                ((QComboBoxInTable *)tableEC->cellWidget(dptPOL, i))->setCurrentIndex(currentValue);
            }
        }
        if (headerReallyPressed)
            updateMaskNamesInScript(0, "Polarization");
    }

    if (raw==dptECTR)
    {
        for (int i = 0; i < CDL; i++)
        {
            if (tableEC->item(dptECTR, i)->checkState() == Qt::Checked)
            {
                QString ECtStr=tableEC->item(dptEC,i)->text();
                QString EBtStr=tableEC->item(dptEB,i)->text();
                
                QString mask=comboBoxMaskFor->currentText();
                
                mask=((QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,i))->currentText();

                QString trString;
                double tr;
                double sigmaTr = 0;

                tr = readTransmission(ECtStr, EBtStr, mask, 0, 0, sigmaTr);

                if (tr > 1.5 || tr <= 0)
                    trString = "1.0000";
                else
                    trString = QString::number(tr, 'f', 4);

                tableEC->item(dptECTR, i)->setText(trString);
            }
            else
                tableEC->item(dptECTR, i)->setText(QString::number(1.0, 'f', 4));
        }
        
        for (int i = 0; i < CDL; i++)
        {
            if (tableEC->item(dptECTR, i)->checkState() != Qt::Checked)
            {
                double currentLambda=tableEC->item(dptWL,i)->text().toDouble();
                currentLambda=QString::number(currentLambda,'f',1).toDouble();
                
                for (int ii=0; ii<CDL; ii++)
                {
                    if (tableEC->item(dptECTR, ii)->checkState() == Qt::Checked)
                    {
                        double currentLambda2=tableEC->item(dptWL,ii)->text().toDouble();
                        currentLambda2=QString::number(currentLambda2,'f',1).toDouble();
                        if (currentLambda2==currentLambda)     tableEC->item(dptECTR,i)->setText(tableEC->item(dptECTR,ii)->text());
                    }
                }
            }
        }
        updateColInScript("Transmission-EC", dptECTR);
        //+++ 2021-03-04
        calcAbsCalNew();
        updateColInScript("Factor", dptACFAC);
        //--- 2021-03-04
    }
    if (raw==dptDAC) //+++ D-AC +++
    {
        for (int i = 0; i < CDL; i++)
        {
            QString PlexyNumber;
            
            if(comboBoxACmethod->currentIndex()==1) PlexyNumber=tableEC->item(dptACEB,i)->text();
            else PlexyNumber=tableEC->item(dptACFS,i)->text();

            if (filesManager->checkFileNumber(PlexyNumber))
            {
                double D = detector->readDinM(PlexyNumber); // [m]
                tableEC->item(dptDAC,i)->setText(QString::number(D,'f',3));
            }
        }
    }
    if (raw==dptACMU)
    {
        for (int i = 0; i < CDL; i++)
        {
            double lambda=tableEC->item(dptWL,i)->text().toDouble();
            tableEC->item(dptACMU,i)->setText(QString::number(muCalc(lambda),'E',4));
        }
    }
    
    if (raw==dptACTR)
    {
        for (int i = 0; i < CDL; i++)
        {
            if (checkBoxTransmissionPlexi->isChecked())
            {
                double lambda=tableEC->item(dptWL,i)->text().toDouble();
                tableEC->item(dptACTR,i)->setText(QString::number(tCalc(lambda),'f',4));
            }
            else
            {
                QString mask=comboBoxMaskFor->currentText();
                
                mask=tableEC->item(dptMASKTR,i)->text();
                
                double sigmaTr=0;
                double Tr=readTransmission( tableEC->item(dptACFS,i)->text(), tableEC->item(dptACEB,i)->text(), mask,0,0,sigmaTr);
                if (Tr<=0 || Tr>1)
                {
                    double lambda=tableEC->item(dptWL,i)->text().toDouble();
                    tableEC->item(dptACTR,i)->setText(QString::number(tCalc(lambda),'f',4));
                    
                }
                else tableEC->item(dptACTR,i)->setText(QString::number(Tr,'f',4));
            }
        }
    }
    if (raw==dptACFAC)
    {
        calcAbsCalNew();
        updateColInScript("Factor", dptACFAC);
    }
    
    if (raw==dptCENTERX || raw==dptCENTERY)
    {
        calcCenterNew();
        updateCenterAll();
    }
    if (raw==dptMASK)
    {
        //mask
        QStringList lst0, lst;
        findMatrixListByLabel("DAN::Mask::"+QString::number(MD),lst0);
        lst=lst0;
        if (lst.indexOf("mask")<0) lst.prepend("mask");
        QString currentMask;
        for (int i = 0; i < tableEC->columnCount(); i++)
        {
            QComboBoxInTable *mask =(QComboBoxInTable*)tableEC->cellWidget(dptMASK,i);
            currentMask=mask->currentText();
            mask->clear();
            mask->addItems(lst);
            mask->setCurrentIndex(lst.indexOf(currentMask));
        }
        if (headerReallyPressed) updateMaskNamesInScript(0, "Mask");
    }
    if (raw==dptSENS)
    {
        //sens
        QStringList lst;
        findMatrixListByLabel("DAN::Sensitivity::"+QString::number(MD),lst);
        if (lst.indexOf("sens")<0) lst.prepend("sens");
        QString currentSens;
        for (int i = 0; i < tableEC->columnCount(); i++)
        {
            QComboBoxInTable *sens =(QComboBoxInTable*)tableEC->cellWidget(dptSENS,i);
            currentSens=sens->currentText();
            sens->clear();
            sens->addItems(lst);
            sens->setCurrentIndex(lst.indexOf(currentSens));
        }
        if (headerReallyPressed) updateMaskNamesInScript(0, "Sens");
    }
    if (raw==dptMASKTR)
    {
        //mask
        QStringList lst0, lst;
        findMatrixListByLabel("DAN::Mask::"+QString::number(MD),lst0);
        lst=lst0;
        if (lst.indexOf("mask")<0) lst.prepend("mask");
        QString currentMask;
        for (int i = 0; i < tableEC->columnCount(); i++)
        {
            QComboBoxInTable *mask =(QComboBoxInTable*)tableEC->cellWidget(dptMASKTR,i);
            currentMask=mask->currentText();
            mask->clear();
            mask->addItems(lst);
            mask->setCurrentIndex(lst.indexOf(currentMask));
        }
    }
}

void dan18::horHeaderTableECPressed(int col,  bool headerReallyPressed)
{
//old    QString currentLabel=tableEC->horizontalHeader()->label(col);

    QString currentLabel=tableEC->horizontalHeaderItem(col)->text();

    QString colLabel;
    if ((currentLabel.contains("|D") && headerReallyPressed) || (!currentLabel.contains("|D") && !headerReallyPressed))
    {
	colLabel="cond.-#"+QString::number(col+1);
    }
    else {
	
	QString C=tableEC->item(dptC,col)->text();
	double D=tableEC->item(dptD,col)->text().toDouble();
	double lambda=tableEC->item(dptWL,col)->text().toDouble();
	colLabel="C"+C+"|D";
	colLabel+=QString::number(D,'f',0)+"|";
	colLabel+=QString::number(lambda,'f',0)+QChar(197);
    }
    //tableEC->horizontalHeader()->setLabel(col, colLabel);
    
    tableEC->horizontalHeaderItem(col)->setText(colLabel);
} 

void dan18::tableECclick(  int row, int col )
{
    if (row==dptEC || row==dptBC || row==dptEB || row==dptACFS ||
	row==dptACEB || row==dptACBC || row==dptCENTER)
    {
        QString s;
        if (!selectFile(s))
            return;
        QString oldValue = tableEC->item(row, col)->text();
        tableEC->item(row, col)->setText(s);

        // C
        int C = int(collimation->readCinM(s));
        // D
        double D = detector->readDinM(s);
        // lambda
        double lambda = selector->readLambda(s, monitors->readDuration(s));
        // CA
        QString CA = collimation->readCA(s);
        // CA
        QString SA = collimation->readSA(s);
        QString beam = CA + "|" + SA;

        QString polarization = collimation->readPolarization(s);
        int polarizationNumber = polarizedAlias(polarization);

        bool changeNumber = true;

        if (row == dptEC)
        {
            if (changeNumber)
            {
                tableEC->item(dptC, col)->setText(QString::number(C, 'f', 0));
                tableEC->item(dptD, col)->setText(QString::number(D, 'f', 3));
                tableEC->item(dptWL, col)->setText(QString::number(lambda, 'f', 3));
                tableEC->item(dptBSIZE, col)->setText(beam);
                ((QComboBoxInTable *)tableEC->cellWidget(dptPOL, col))->setCurrentIndex(polarizationNumber);
                horHeaderTableECPressed(col, false);
            }
	    }
    }
    else if (row == dptC) //+++ C +++
    {
        QString ECnumber=tableEC->item(dptEC,col)->text();
        if (filesManager->checkFileNumber(ECnumber))
        {
            // C
            int C = int(collimation->readCinM(ECnumber));
            tableEC->item(dptC, col)->setText(QString::number(C, 'f', 0));
        }
    }
    else if (row == dptD) //+++ D +++
    {
        QString ECnumber = tableEC->item(dptEC, col)->text();
        if (filesManager->checkFileNumber(ECnumber))
        {
            double D = detector->readDinM(ECnumber);
            tableEC->item(dptD, col)->setText(QString::number(D, 'f', 3));
        }
    }
    else if (row==dptWL) //+++ Lambda +++
    {
        QString ECnumber = tableEC->item(dptEC, col)->text();
        if (filesManager->checkFileNumber(ECnumber))
        {
            double lambda = selector->readLambda(ECnumber, monitors->readDuration(ECnumber));
            tableEC->item(dptWL, col)->setText(QString::number(lambda, 'f', 3));
        }
    }
    else if (row==dptBSIZE)	    //+++ Beam Size +++
    {
        QString ECnumber = tableEC->item(dptEC, col)->text();
        if (filesManager->checkFileNumber(ECnumber))
        {
            QString CA = collimation->readCA(ECnumber);
            QString SA = collimation->readSA(ECnumber);
            tableEC->item(dptBSIZE, col)->setText(CA + "|" + SA);
        }
    }    
    else if (row == dptECTR)
    {
        bool ok;
        double res = QInputDialog::getDouble(this, "qtiSAS", "Enter transmission of the current condition:",
                                             tableEC->item(row, col)->text().toDouble(), 0, 1.500, 3, &ok);
        if (ok)
        {
            tableEC->item(row, col)->setText(QString::number(res, 'f', 4));
            if (!calcAbsCalNew(col))
                return;
        }
    }
    else if (row == dptACMU)
    {
        double lambda = tableEC->item(dptWL, col)->text().toDouble();
        tableEC->item(dptACMU, col)->setText(QString::number(muCalc(lambda), 'E', 4));
    }
    else if (row == dptACTR)
    {
        if (checkBoxTransmissionPlexi->isChecked())
        {
            double lambda = tableEC->item(dptWL, col)->text().toDouble();
            tableEC->item(dptACTR, col)->setText(QString::number(tCalc(lambda), 'f', 4));
        }
        else
        {
            QString mask = comboBoxMaskFor->currentText();

            mask = ((QComboBoxInTable *)tableEC->cellWidget(dptMASKTR, col))->currentText();
            double sigmaTr = 0.0;
            double Tr = readTransmission(tableEC->item(dptACFS, col)->text(), tableEC->item(dptACEB, col)->text(), mask,
                                         0, 0, sigmaTr);
            tableEC->item(dptACTR, col)->setText(QString::number(Tr, 'f', 4));
        }
    }
    else if (row == dptACFAC)
    {
        if (!calcAbsCalNew(col))
            return;
    }
    else if (row == dptCENTERX || row == dptCENTERY)
    {
        calcCenterNew(col);
    }
}

void dan18::tableEChelp(int raw)
{ 
    QString sHelp;
    switch(raw)
    {
    case 0:
            sHelp = "run number of Empty Cell [ Empty Beam]:\n\n I[sample]- Tr I[EC]";
            break;
    case 1:
            sHelp = "run number of Black Current [BC, cadmium],\n electronic background  of detector and background from other instruments :\n\n I[sample]-I[BC], I[EC]-I[BC]";
            break;
            
    }
    QMessageBox::information( this, "DAN :: Smart Help", sHelp );
}

void dan18::calculateTransmissionAll()
{
    calculateTransmission(0);
    if (checkBoxForceCopyPaste->isChecked()) copycorrespondentTransmissions();
}

void dan18::calculateTransmission(int startRow)
{
    //+++
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++  Subtract Bufffer
    bool subtractBuffer = comboBoxMode->currentText().contains("(BS)");
    
    //+++  Subtract Bufffer && Correct to own Sensitivity
    bool bufferAsSens= false;
    if (comboBoxMode->currentText().contains("(BS-SENS)")) bufferAsSens=true;
    
    
    //+++ optional parameter if column "VShift" exist
    double VShift;
    
    //+++ optional parameter if column "HShift" exist
    double HShift;
    
    //+++
    Table* w;

    if (!app()->checkTableExistence(tableName))
        return;

    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
    //+++

    //+++ calculate only selected lines
    bool checkSelection=false;
    
    if(startRow==0)
    {
        
        for (int iRow=0; iRow<(int)tableList.count(); iRow++)
        {
            if (w->isRowSelected(iRow,true))
            {
                checkSelection=true;
                break;
            }
        }
        
        
    }

    if (w->windowLabel() != "DAN::Script::Table")
        return;

    if (!scriptTableManager->update(w))
    {
        QMessageBox::critical(nullptr, "qtiSAS",
                              "Table ~" + tableName + " has wrong format. <br> Check table or  generate new one.<h4>");
        return;
    }

    int rowNumber=w->numRows();
    
    //+++ Calculate transmission
    QString ECnumber;
    double trans=-1;
    double sigmaTr=0;
    int iMax=tableEC->columnCount();
    int iter;
    
    //+++ new active conditions
    QList<int> listTr;
    
    for (int i=0; i<iMax; i++)
    {
        if (tableEC->item(dptECTR, i)->checkState() == Qt::Checked) listTr<<(i+1);
    }
    
    for(iter=startRow; iter<rowNumber;iter++)
    {
        sigmaTr = 0;

        if (scriptTableManager->condition(iter) == "")
            continue;

        if (checkSelection && !w->isRowSelected(iter, true))
            continue;

        if (listTr.indexOf(scriptTableManager->condition(iter).toInt()) >= 0)
        {
            VShift = scriptTableManager->VSchift(iter).toDouble();
            HShift = scriptTableManager->HSchift(iter).toDouble();

            ECnumber = scriptTableManager->runEC(iter);

            if (ECnumber=="" && comboBoxTransmMethod->currentIndex()!=2)
            {
                scriptTableManager->transmissionWrite(iter, "no EC-file");
                if (subtractBuffer)
                    scriptTableManager->transmissionBufferWrite(iter, "no EC-file");
            }
            else
            {
                if (filesManager->checkFileNumber(ECnumber) || comboBoxTransmMethod->currentIndex() == 2)
                {
                    QString mask = scriptTableManager->mask(iter);
                    mask = ((QComboBoxInTable *)tableEC->cellWidget(dptMASKTR,
                                                                    scriptTableManager->condition(iter).toInt() - 1))
                               ->currentText();
                    sigmaTr = 0;
                    trans =
                        readTransmission(scriptTableManager->runSample(iter), ECnumber, mask, VShift, HShift, sigmaTr);

                    if (trans > 2.00 || trans <= 0)
                        scriptTableManager->transmissionWrite(iter, "check!!!");
                    else
                        scriptTableManager->transmissionWrite(
                            iter, "   " + QString::number(trans, 'f', 4) + " [ " + QChar(177) +
                                      QString::number(trans * sigmaTr, 'f', 4) + " ]");

                    if (subtractBuffer)
                    {
                        sigmaTr = 0;
                        trans = readTransmission(scriptTableManager->transmissionBuffer(iter), ECnumber, mask, VShift,
                                                 HShift, sigmaTr);

                        if (trans > 2.00 || trans <= 0)
                            scriptTableManager->transmissionBufferWrite(iter, "check!!!");
                        else
                            scriptTableManager->transmissionBufferWrite(
                                iter, "   " + QString::number(trans, 'f', 4) + " [ " + QChar(177) +
                                          QString::number(trans * sigmaTr, 'f', 4) + " ]");
                    }
                }
                else 
                {
                    scriptTableManager->transmissionWrite(iter, "check file!!!");
                    if (subtractBuffer)
                        scriptTableManager->transmissionBufferWrite(iter, "check file!!!");
                }
            }
        }
        else
        {
            scriptTableManager->transmissionWrite(iter, "Not active configuration");
            if (subtractBuffer)
                scriptTableManager->transmissionBufferWrite(iter, "Not active configuration");
        }
    }
}

//+-1-+SLOT::calculate absolute constants ++++++++++++++++++
bool dan18::calcAbsCalNew( )
{
    int i;
    
    // Number of C-D conditionws
    int Nconditions=tableEC->columnCount();
    
    // find center for each C-D condition
    
    int caseMethod=comboBoxACmethod->currentIndex();
    
    for(i=0;i<Nconditions;i++)
    {
        switch (caseMethod)
        {
            case 0: calcAbsCalNew( i ); break;
            case 1: calcAbsCalDB( i ); break;
            case 2: calcAbsCalTrFs(i ); break;
            case 3: calcAbsCalNew( i ); break;
        }
    }
    return true;
}

//+-2-+SLOT::calculate absolute constants +++++++++++++++++++++++++++++++++++++
bool dan18::calcAbsCalTrFs( int col )
{
    int MD = lineEditMD->text().toInt();

    QString maskName=comboBoxMaskFor->currentText();
    QString sensName=comboBoxSensFor->currentText();
    
    maskName=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText();
    if (!checkExistenceOfMask(QString::number(MD), maskName)) return false;
    sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
    if (!checkExistenceOfSens(QString::number(MD), sensName)) return false;
    
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
    
    //+++ sens reading +++
    if (listSens.contains(((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText()))
    {
        sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
    }
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(mask);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(sens);
    make_GSL_Matrix_Symmetric( sensName, sens, MD);
    
    sensAndMaskSynchro(mask, sens, MD );
    
    //+++ Variables
    int   ii, jj;
    
    double D=tableEC->item(dptDAC,col)->text().toDouble();
    
    double transmision=0.0;
    
    bool existPlexi, existEB,  existBC;
    
    double sum=0.0;
    
    double Nmask=0.0;
    
    double  cal;
    
    //+++ Matrix allocation
    gsl_matrix *plexi=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexi);
    gsl_matrix *plexiEB=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexiEB);
    gsl_matrix *plexiBC=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexiBC);
    
    //+++Check existence of plexi File
    QString PlexiNumber=tableEC->item(dptACFS,col)->text();

    existPlexi = filesManager->checkFileNumber(PlexiNumber);

    if (!existPlexi)
    {
        QMessageBox::warning(this,tr("qtiSAS"), tr(QString("C-D-Lambda condition # " + QString::number(col+1) + ":: plexi-file does not exist!").toLocal8Bit().constData()));
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: plexi-file does not exist!"+"\n");
        
        tableEC->item(dptACFS,col)->setText("0");
        tableEC->item(dptACFAC,col)->setText(QString::number(1.0,'E',1));
        
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(plexi);
        gsl_matrix_free(plexiEB);
        gsl_matrix_free(plexiBC);
        return false;
    }
    
    // read Plexi matrix
    readMatrixCor( PlexiNumber, plexi );
    
    //check existence of EB File
    QString EBNumber=tableEC->item(dptACEB,col)->text();

    existEB = filesManager->checkFileNumber(EBNumber);
    if (!existEB)
    {
        //*************************************Log Window Output
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: EBplexi-file does not exist! Absolute calibration without EB subtraction !"+"\n");
        //*************************************Log Window Output
        tableEC->item(dptACEB,col)->setText("0");
    }
    else
    {
        // read EB matrix
        readMatrixCor( EBNumber, plexiEB );
        
        transmision=tableEC->item(dptACTR,col)->text().toDouble();
        if (transmision>1.0) transmision=1.0;
        if (transmision<0.01) transmision=0.0;
        tableEC->item(dptACTR,col)->setText(QString::number(transmision, 'f',4));
    }
    
    //check existence of BC File
    QString BCNumber=tableEC->item(dptACBC,col)->text();
    existBC = filesManager->checkFileNumber(BCNumber);
    if (!existBC)
    {
        //*************************************Log Window Output
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: BCplexi-file does not exist! Absolute calibration without BCplexi-subtraction !"+"\n");
        //*************************************Log Window Output
        tableEC->item(dptACBC,col)->setText("0");
    }
    else
    {
        // read BC matrix 2012
        if (checkBoxBCTimeNormalization->isChecked())
        {
            readMatrixCorTimeNormalizationOnly( BCNumber, plexiBC );
            
            //Normalization constant
            double TimeSample=spinBoxNorm->value();
            double ttime = monitors->readDuration(EBNumber);
            if (ttime > 0.0)
                TimeSample /= ttime;
            else
                TimeSample = 0.0;

            double NormSample = monitors->normalizationFactor(EBNumber);

            if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
            
            gsl_matrix_scale(plexiBC,NormSample);      // EB=T*EB
        }
        else readMatrixCor( BCNumber, plexiBC );
    }
    
    gsl_matrix_sub(plexiEB,plexiBC);            // EB=EB - BC
    
    
    if (existBC && checkBoxBCTimeNormalization->isChecked())
    {
        // read BC matrix 2012
        readMatrixCorTimeNormalizationOnly( BCNumber, plexiBC );
        
        //Normalization constant
        double TimeSample=spinBoxNorm->value();
        double ttime = monitors->readDuration(PlexiNumber);
        if (ttime > 0.0)
            TimeSample /= ttime;
        else
            TimeSample = 0.0;

        double NormSample = monitors->normalizationFactor(PlexiNumber);

        if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
        
        gsl_matrix_scale(plexiBC,NormSample);      // EB=T*EB
    }
    
    gsl_matrix_sub(plexi,plexiBC);              // plexi=plexi - BC
    
    
    
    double Xc, Yc;
    readCenterfromMaskName( maskName, Xc, Yc, MD );
    
    
    if (transmision<1.0 && transmision>0.0 && checkBoxParallax->isChecked())
    {
        transmissionThetaDependenceTrEC(plexiEB, Xc, Yc, D*100.0, transmision);
    }
    
    gsl_matrix_scale(plexiEB,transmision);      // EB=T*EB
    gsl_matrix_sub(plexi,plexiEB);                      // plexi=plexi  - EB
    
    gsl_matrix_mul_elements(plexi,mask);
    gsl_matrix_mul_elements(plexi,sens);
    
    //+++
    if (checkBoxParallax->isChecked())
    {
        parallaxCorrection(plexi, Xc, Yc, D*100.0,transmision);
    }
    
    
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        if (gsl_matrix_get(mask,ii,jj)>0)
        {
            if (gsl_matrix_get(plexi,ii,jj)<=0.0    && checkBoxMaskNegative->isChecked() )
            {
                gsl_matrix_set(plexi,ii,jj,0.0);
                gsl_matrix_set(mask,ii,jj,0.0);
                gsl_matrix_set(sens,ii,jj,0.0);
            }
            else
            {
                Nmask+=gsl_matrix_get(mask,ii,jj);
                sum+=gsl_matrix_get(plexi,ii,jj);
            }
        }
    }
    
    //Error
    double sigma2=0.0;
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        if (gsl_matrix_get(mask,ii,jj)>0.0)
            sigma2+=(sum/Nmask-gsl_matrix_get(plexi,ii,jj))*(sum/Nmask-gsl_matrix_get(plexi,ii,jj));
    }
    
    sigma2=sigma2/Nmask/ (Nmask-1.0);
    sigma2=sqrt(sigma2);
   
    //+++
    double mu=tableEC->item(dptACMU,col)->text().toDouble();
    double Ds=tableEC->item(dptD,col)->text().toDouble();
    double transEC=tableEC->item(dptECTR,col)->text().toDouble();
    
    if (transEC > 1.5 || transEC <= 0.0)
    {
        transEC = 1.0;
        tableEC->item(dptECTR, col)->setText("1.000");
    }

    double transPolarizer = 1.0;
    if (comboBoxMode->currentText().contains("(PN)"))
    {

        QString polarizationEC = collimation->readPolarization(tableEC->item(dptEC, col)->text());
        int polarizationNumberEC = polarizedAlias(polarizationEC);
        QString polarizationPLEXY = collimation->readPolarization(PlexiNumber);
        int polarizationNumberPLEXY = polarizedAlias(polarizationPLEXY);

        double trConfigValue = polTransmissionSelector->getValue(tableEC->item(dptWL, col)->text().toDouble(),
                                                                 tableEC->item(dptEC, col)->text());
        if (trConfigValue <= 0)
            trConfigValue = 1.0;

        double trConfigAnalazer = analyzerTransmissionSelector->getValue(tableEC->item(dptWL, col)->text().toDouble(),
                                                                         tableEC->item(dptEC, col)->text());

        if (trConfigAnalazer <= 0)
            trConfigAnalazer = 1.0;

        if (polarizationNumberPLEXY == 0 && polarizationNumberEC > 0)
            transPolarizer = trConfigValue;
        else if ((polarizationNumberPLEXY == 1 || polarizationNumberPLEXY == 2) && polarizationNumberEC == 0)
            transPolarizer = 1.0 / trConfigValue;
        else if ((polarizationNumberPLEXY == 1 || polarizationNumberPLEXY == 2) && polarizationNumberEC > 0)
            transPolarizer = 1.0;
        else if ((polarizationNumberPLEXY == 3 || polarizationNumberPLEXY == 5) && polarizationNumberEC == 0)
            transPolarizer = 1.0 / trConfigValue / trConfigAnalazer;
        else if ((polarizationNumberPLEXY == 3 || polarizationNumberPLEXY == 5) && polarizationNumberEC > 0)
            transPolarizer = trConfigAnalazer;
    }

    //+++
    cal = mu * (1 - transmision) / sum * Nmask / D / D * Ds * Ds / transEC / transPolarizer;
    sigma2=mu*(1-transmision)/sum*Nmask/sum/D/D*Ds*Ds*sigma2;
    //+++
    tableEC->item(dptACFAC,col)->setText(QString::number(cal,'E',4));
    //tableEC->item(14,col)->setText(QString::number(sigma2,'E',1));
    toResLog("DAN :: Abs.Factor | Condition #"+QString::number(col+1)+" | "+ QString::number(cal,'E',4)+QChar(177)+QString::number(sigma2,'E',4)+"\n");
    
    //+++
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    gsl_matrix_free(plexi);
    gsl_matrix_free(plexiEB);
    gsl_matrix_free(plexiBC);
    return true;
}

//+-2-+SLOT::calculate absolute constants +++++++++++++++++++++++++++++++++++++
bool dan18::calcAbsCalDB( int col )
{
    int MD = lineEditMD->text().toInt();

    QString maskName=comboBoxMaskFor->currentText();
    QString sensName=comboBoxSensFor->currentText();
    
    if ( !checkBoxACDBuseActive->isChecked() ) maskName=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText();
    if ( !checkExistenceOfMask(QString::number(MD), maskName)) return false;
    
    if ( !checkBoxACDBuseActive->isChecked() )   sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
    if ( !checkExistenceOfSens(QString::number(MD), sensName)) return false;
    
    //+++ Mask & Sens +++
    QStringList listMask, listSens;
    QString winLabelMask="DAN::Mask::"+QString::number(MD);
    QString winLabelSens="DAN::Sensitivity::"+QString::number(MD);
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(mask);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(sens);
    make_GSL_Matrix_Symmetric( sensName, sens, MD);
    
    sensAndMaskSynchro(mask, sens, MD );
    
    
    //+++ Variables
    int   ii, jj;
    QString fileNumber;
    
    bool existEB;
    
    double sum=0.0;
    
    double Nmask=0.0;
    
    double  cal;
    
    //+++ Matrix allocation
    gsl_matrix *EB=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(EB);
    gsl_matrix *BC=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(BC);
    
    //+++Check existence of plexi File
    QString EBNumber=tableEC->item(dptACEB,col)->text();

    existEB = filesManager->checkFileNumber(EBNumber);

    if (!existEB)
    {
        QMessageBox::warning(this,tr("qtiSAS"), tr(QString("C-D-Lambda condition # " + QString::number(col+1) + " :: EB-file does not exist!").toLocal8Bit().constData()));
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+" :: EB-file does not exist!"+"\n");
        
        tableEC->item(dptACEB,col)->setText("0");
        tableEC->item(dptACFAC,col)->setText(QString::number(1.0,'E',1));
        
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(EB);
        gsl_matrix_free(BC);
        return false;
    }
    readMatrixCor( EBNumber, EB );
    
    //check existence of BC File
    QString BCNumber=tableEC->item(dptACBC,col)->text();
    bool existBC = filesManager->checkFileNumber(BCNumber);
    if (!existBC)
    {
        //*************************************Log Window Output
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: BC-file does not exist! Absolute calibration without BC-subtraction !"+"\n");
        //*************************************Log Window Output
        tableEC->item(dptACBC,col)->setText("0");
    }
    else
    {
        // read BC matrix 2012
        if (checkBoxBCTimeNormalization->isChecked())
        {
            readMatrixCorTimeNormalizationOnly( BCNumber, BC );
            
            //Normalization constant
            double TimeSample=spinBoxNorm->value();
            double ttime = monitors->readDuration(EBNumber);
            if (ttime > 0.0)
                TimeSample /= ttime;
            else
                TimeSample = 0.0;

            double NormSample = monitors->normalizationFactor(EBNumber);

            if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
            
            gsl_matrix_scale(BC,NormSample);      // EB=T*EB
        }
        else readMatrixCor( BCNumber, BC );
        
        gsl_matrix_sub(EB, BC);            // EB=EB - BC
    }
    
    
    
    gsl_matrix_mul_elements(EB,mask);
    gsl_matrix_mul_elements(EB,sens);
    
    double transEC=tableEC->item(dptECTR,col)->text().toDouble();
    
    if (transEC > 1.5 || transEC <= 0.0)
    {
        transEC = 1.0;
        tableEC->item(dptECTR, col)->setText("1.000");
    }
    
    // atten
    double attenuation = tableEC->item(dptACTR,col)->text().toDouble();
    if (attenuation<=0.0 || attenuation>1.0) attenuation=1.0;
    
    // mu
    double mu = tableEC->item(dptACMU,col)->text().toDouble();
    if (mu<=0) attenuation=1.0;
    
    double D=tableEC->item(dptDAC,col)->text().toDouble();
    D*=100.0; // cm
    
    double area=lineEditPS->text().toDouble();
    area=area*area;
    area*=lineEditAsymetry->text().toDouble(); // cm^2
    
    
    double I0=0;
    
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        I0+=gsl_matrix_get(EB,ii,jj);
    }
    
    double transPolarizer = 1.0;
    if (comboBoxMode->currentText().contains("(PN)"))
    {

        QString polarizationEC = collimation->readPolarization(tableEC->item(dptEC, col)->text());
        int polarizationNumberEC = polarizedAlias(polarizationEC);
        QString polarizationEB = collimation->readPolarization(EBNumber);
        int polarizationNumberEB = polarizedAlias(polarizationEB);

        double trConfigValue = polTransmissionSelector->getValue(tableEC->item(dptWL, col)->text().toDouble(),
                                                                 tableEC->item(dptEC, col)->text());
        if (trConfigValue <= 0)
            trConfigValue = 1.0;

        double trConfigAnalazer = analyzerTransmissionSelector->getValue(tableEC->item(dptWL, col)->text().toDouble(),
                                                                         tableEC->item(dptEC, col)->text());
        if (trConfigAnalazer <= 0)
            trConfigAnalazer = 1.0;

        if (polarizationNumberEB == 0 && polarizationNumberEC > 0)
            transPolarizer = trConfigValue;
        else if ((polarizationNumberEB == 1 || polarizationNumberEB == 2) && polarizationNumberEC == 0)
            transPolarizer = 1.0 / trConfigValue;
        else if ((polarizationNumberEB == 1 || polarizationNumberEB == 2) && polarizationNumberEC > 0)
            transPolarizer = 1.0;
        else if ((polarizationNumberEB == 3 || polarizationNumberEB == 5) && polarizationNumberEC == 0)
            transPolarizer = 1.0 / trConfigValue / trConfigAnalazer;
        else if ((polarizationNumberEB == 3 || polarizationNumberEB == 5) && polarizationNumberEC > 0)
            transPolarizer = trConfigAnalazer;
    }

    I0 /= attenuation;
    I0 *= mu;
    I0 *= area / D / D;
    I0 *= transEC;
    I0 *= transPolarizer;

    //+++
    tableEC->item(dptACFAC,col)->setText(QString::number(1/I0,'E',4));
    toResLog("DAN :: Abs.Factor (DB) | Condition #"+QString::number(col+1)+" | "+ QString::number(1/I0,'E',4)+"\n");
    
    //+++
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    gsl_matrix_free(EB);
    gsl_matrix_free(BC);
    return true;
}

//+-2-+SLOT::calculate absolute constants ++++++++++++++++++++++++++++
bool dan18::calcAbsCalNew( int col )
{
    int MD = lineEditMD->text().toInt();

    QString maskName=comboBoxMaskFor->currentText();
    QString sensName=comboBoxSensFor->currentText();
    
    maskName=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText();
    if (!checkExistenceOfMask(QString::number(MD), maskName)) return false;
    sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
    if (!checkExistenceOfSens(QString::number(MD), sensName)) return false;
    
    //+++ Mask & Sens +++
    QStringList listMask, listSens;
    QString winLabelMask="DAN::Mask::"+QString::number(MD);
    QString winLabelSens="DAN::Sensitivity::"+QString::number(MD);
    
    findMatrixListByLabel(winLabelMask, listMask);
    if (!listMask.contains("mask")) listMask.prepend("mask");
    findMatrixListByLabel(winLabelSens, listSens);
    if (!listSens.contains("sens")) listSens.prepend("sens");
    
    //+++ mask reading +++
    if (listMask.contains(((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText()) )
    {
        maskName=((QComboBoxInTable*)tableEC->cellWidget(dptMASK,col))->currentText();
    }
    
    //+++ mask reading +++
    if (listSens.contains(((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText()) )
    {
        sensName=((QComboBoxInTable*)tableEC->cellWidget(dptSENS,col))->currentText();
    }
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(mask);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(sens);
    make_GSL_Matrix_Symmetric( sensName, sens, MD);
    
    sensAndMaskSynchro(mask, sens, MD );
    
    //+++ Variables
    int   ii, jj;
    
    double D=tableEC->item(dptDAC,col)->text().toDouble();
    
    double transmision=0.0;
    
    bool existPlexi, existEB,  existBC;
    
    double sum=0.0;
    
    double Nmask=0.0;
    
    double  cal;
    
    //+++ Matrix allocation
    gsl_matrix *plexi=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexi);
    gsl_matrix *plexiEB=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexiEB);
    gsl_matrix *plexiBC=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_zero(plexiBC);
    
    //+++Check existence of plexi File
    QString PlexiNumber=tableEC->item(dptACFS,col)->text();

    existPlexi = filesManager->checkFileNumber(PlexiNumber);

    if (!existPlexi)
    {
        if (checkBoxSkiptransmisionConfigurations->isChecked())
            QMessageBox::warning(this,tr("qtiSAS"), tr(QString("C-D-Lambda condition # "+QString::number(col+1)+":: plexi-file does not exist!").toLocal8Bit().constData()));

        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: plexi-file does not exist!"+"\n");

        tableEC->item(dptACFS,col)->setText("0");
        tableEC->item(dptACFAC,col)->setText(QString::number(1.0,'E',1));
        
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(plexi);
        gsl_matrix_free(plexiEB);
        gsl_matrix_free(plexiBC);
        return false;
    }
    
    // read Plexi matrix
    readMatrixCor(PlexiNumber, plexi);
    // D = detector->readDinM(PlexiNumber);

    // check existence of EB File
    QString EBNumber = tableEC->item(dptACEB, col)->text();

    existEB = filesManager->checkFileNumber(EBNumber);
    if (!existEB)
    {
        //*************************************Log Window Output
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: EBplexi-file does not exist! Absolute calibration without EB subtraction !"+"\n");
        //*************************************Log Window Output
        tableEC->item(dptACEB,col)->setText("0");
    }
    else
    {
        // read EB matrix
        readMatrixCor( EBNumber, plexiEB );
        
        transmision=tableEC->item(dptACTR,col)->text().toDouble();
        if (transmision>1.0) transmision=1.0;
        if (transmision<0.01) transmision=0.0;
        tableEC->item(dptACTR,col)->setText(QString::number(transmision, 'f',4));
    }
    
    //check existence of BC File
    QString BCNumber=tableEC->item(dptACBC,col)->text();
    existBC = filesManager->checkFileNumber(BCNumber);
    if (!existBC)
    {
        //*************************************Log Window Output
        toResLog("DAN :: C-D-Lambda condition # "+QString::number(col+1)+":: BCplexi-file does not exist! Absolute calibration without BCplexi-subtraction !"+"\n");
        //*************************************Log Window Output
        tableEC->item(dptACBC,col)->setText("0");
    }
    else
    {
        // read BC matrix 2012
        if (checkBoxBCTimeNormalization->isChecked())
        {
            readMatrixCorTimeNormalizationOnly( BCNumber, plexiBC );
            
            //Normalization constant
            double TimeSample=spinBoxNorm->value();
            double ttime = monitors->readDuration(EBNumber);
            if (ttime > 0.0)
                TimeSample /= ttime;
            else
                TimeSample = 0.0;

            double NormSample = monitors->normalizationFactor(EBNumber);

            if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
            
            gsl_matrix_scale(plexiBC,NormSample);      // EB=T*EB
        }
        else readMatrixCor( BCNumber, plexiBC );
    }
    
    gsl_matrix_sub(plexiEB,plexiBC);            // EB=EB - BC
    
    
    if (existBC && checkBoxBCTimeNormalization->isChecked())
    {
        // read BC matrix 2012
        readMatrixCorTimeNormalizationOnly( BCNumber, plexiBC );
        
        //Normalization constant
        double TimeSample=spinBoxNorm->value();
        double ttime = monitors->readDuration(PlexiNumber);
        if (ttime > 0.0)
            TimeSample /= ttime;
        else
            TimeSample = 0.0;

        double NormSample = monitors->normalizationFactor(PlexiNumber);

        if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
        
        gsl_matrix_scale(plexiBC,NormSample);      // EB=T*EB
    }
    
    gsl_matrix_sub(plexi,plexiBC);              // plexi=plexi - BC
    
    
    
    double Xc, Yc;
    readCenterfromMaskName( maskName, Xc, Yc, MD );
    
    
    if (transmision<1.0 && transmision>0.0 && checkBoxParallax->isChecked())
    {
        transmissionThetaDependenceTrEC(plexiEB, Xc, Yc, D*100.0, transmision);
    }
    
    gsl_matrix_scale(plexiEB,transmision);      // EB=T*EB
    gsl_matrix_sub(plexi,plexiEB);                      // plexi=plexi  - EB
    
    gsl_matrix_mul_elements(plexi,mask);
    gsl_matrix_mul_elements(plexi,sens);
    
    //+++
    if (checkBoxParallax->isChecked())
    {
        parallaxCorrection(plexi, Xc, Yc, D*100.0,transmision);
    }
    
    
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        if (gsl_matrix_get(mask,ii,jj)>0)
        {
            if (gsl_matrix_get(plexi,ii,jj)<=0.0    && checkBoxMaskNegative->isChecked() )
            {
                gsl_matrix_set(plexi,ii,jj,0.0);
                gsl_matrix_set(mask,ii,jj,0.0);
                gsl_matrix_set(sens,ii,jj,0.0);
            }
            else
            {
                Nmask+=gsl_matrix_get(mask,ii,jj);
                sum+=gsl_matrix_get(plexi,ii,jj);
            }
        }
    }
    
    //Error
    double sigma2=0.0;
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        if (gsl_matrix_get(mask,ii,jj)>0.0)
            sigma2+=(sum/Nmask-gsl_matrix_get(plexi,ii,jj))*(sum/Nmask-gsl_matrix_get(plexi,ii,jj));
    }
    
    sigma2=sigma2/Nmask/ (Nmask-1.0);
    sigma2=sqrt(sigma2);
    //+++
    double mu=tableEC->item(dptACMU,col)->text().toDouble();
    double Ds=tableEC->item(dptD,col)->text().toDouble();

    double transEC=tableEC->item(dptECTR,col)->text().toDouble();
    
    if (transEC > 1.5 || transEC <= 0.0)
    {
        transEC = 1.0;
        tableEC->item(dptECTR, col)->setText("1.000");
    }

    double transPolarizer = 1.0;
    if (comboBoxMode->currentText().contains("(PN)"))
    {

        QString polarizationEC = collimation->readPolarization(tableEC->item(dptEC, col)->text());
        int polarizationNumberEC = polarizedAlias(polarizationEC);
        QString polarizationPLEXY = collimation->readPolarization(PlexiNumber);
        int polarizationNumberPLEXY = polarizedAlias(polarizationPLEXY);

        double trConfigValue = polTransmissionSelector->getValue(tableEC->item(dptWL, col)->text().toDouble(),
                                                                 tableEC->item(dptEC, col)->text());
        if (trConfigValue <= 0)
            trConfigValue = 1.0;

        double trConfigAnalazer = analyzerTransmissionSelector->getValue(tableEC->item(dptWL, col)->text().toDouble(),
                                                                         tableEC->item(dptEC, col)->text());
        if (trConfigAnalazer <= 0)
            trConfigAnalazer = 1.0;

        if (polarizationNumberPLEXY == 0 && polarizationNumberEC > 0)
            transPolarizer = trConfigValue;
        else if ((polarizationNumberPLEXY == 1 || polarizationNumberPLEXY == 2) && polarizationNumberEC == 0)
            transPolarizer = 1.0 / trConfigValue;
        else if ((polarizationNumberPLEXY == 1 || polarizationNumberPLEXY == 2) && polarizationNumberEC > 0)
            transPolarizer = 1.0;
        else if ((polarizationNumberPLEXY == 3 || polarizationNumberPLEXY == 5) && polarizationNumberEC == 0)
            transPolarizer = 1.0 / trConfigValue / trConfigAnalazer;
        else if ((polarizationNumberPLEXY == 3 || polarizationNumberPLEXY == 5) && polarizationNumberEC > 0)
            transPolarizer = trConfigAnalazer;
    }
    //+++
    cal = mu / sum * Nmask / D / D * Ds * Ds / transEC / transPolarizer;
    sigma2 = mu / sum * Nmask / sum / D / D * Ds * Ds * sigma2;

    //+++
    tableEC->item(dptACFAC,col)->setText(QString::number(cal,'E',4));
    toResLog("DAN :: Abs.Factor | Condition #"+QString::number(col+1)+" | "+ QString::number(cal,'E',4)+QChar(177)+QString::number(sigma2,'E',4)+"\n");
    
    //+++
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);    
    gsl_matrix_free(plexi);
    gsl_matrix_free(plexiEB);
    gsl_matrix_free(plexiBC);   
    return true;
}
void dan18::updateScriptTables()
{
    QStringList list;
    findTableListByLabel("DAN::Script::Table", list);
    comboBoxMakeScriptTable->clear();
    comboBoxMakeScriptTable->addItems(list);
}
void dan18::updatePolScriptTables()
{
    QStringList list;
    findTableListByLabel("DAN::Polarized::Table", list);
    comboBoxPolarizationScriptTable->clear();
    comboBoxPolarizationScriptTable->addItems(list);
}
//+++ calculation of mu
double dan18::muCalc(double lambda)
{
    double mu0=lineEditMuY0->text().toDouble();
    double muA=lineEditMuA->text().toDouble();
    double lambdaMu=lineEditMut->text().toDouble();
    
    return mu0+muA*exp(lambdaMu/lambda);
}

//+++ calculation of Transmission
double dan18::tCalc(double lambda)
{
    if (lambda<=0.0) return 1.0;
    double To=lineEditTo->text().toDouble();
    double TA=lineEditTA->text().toDouble();
    double lambdaT=lineEditLate->text().toDouble();
    
    return To-TA*exp(-lambdaT/lambda);
}

void dan18::readCenterfromMaskName( QString maskName, double &Xc, double &Yc, int MD )
{
    MdiSubWindow* mmm=(MdiSubWindow*)app()->window(maskName);
    
    if (!mmm)
    {
        Xc=double( spinBoxRBxBS->value() + spinBoxLTxBS->value() ) / 2.0 - 1.0;
        Yc=double( spinBoxRByBS->value() + spinBoxLTyBS->value() ) / 2.0 - 1.0;
        return;
    }
    
    QString  label=mmm->windowLabel();
    
    label=label.remove("DAN::Mask::"+QString::number(MD));
    label=label.replace("|"," ").simplified();
    
    QStringList lst;
    lst = label.split(" ", Qt::SkipEmptyParts);
    
    if (lst.count() > 7)
    {
        Xc=(lst[4].toDouble()+lst[5].toDouble())/2.0-1;
        Yc=(lst[6].toDouble()+lst[7].toDouble())/2.0-1;
    }
    else
    {
        Xc=double( spinBoxRBxBS->value() + spinBoxLTxBS->value() ) / 2.0 - 1.0;
        Yc=double( spinBoxRByBS->value() + spinBoxLTyBS->value() ) / 2.0 - 1.0;
    }
    
}

//+++
void dan18::updateCenterAll()
{
    updateCenter(0);
}

//+++
void dan18::updateCenter(int startRow)
{
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++
    Table* w;

    if (!app()->checkTableExistence(tableName))
        return;

    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;

    
    if (w->windowLabel()!="DAN::Script::Table")    	return;

    if (!scriptTableManager->update(w))
    {
        QMessageBox::critical(nullptr, "qtiSAS",
                              "Table ~" + tableName + " has wrong format. <br> Check table or  generate new one.<h4>");
        return;
    }

    int rowNumber = w->numRows();
    int cond;
    
    for(int iter=startRow; iter<rowNumber;iter++)
    {
        cond = scriptTableManager->condition(iter).toInt();
        if (cond > 0)
        {
            scriptTableManager->centerXWrite(iter,
                                             tableEC->item(dptCENTERX, int(fabs(double(cond))) - 1)->text().left(6));
            scriptTableManager->centerYWrite(iter,
                                             tableEC->item(dptCENTERY, int(fabs(double(cond))) - 1)->text().left(6));
        }
    }
}

void dan18::sensAndMaskSynchro( gsl_matrix* &mask, gsl_matrix* &sens, int MaDe )
{
    for(int i=0;i<MaDe;i++)for(int j=0;j<MaDe;j++)
    {
        if (gsl_matrix_get(mask, i,j)==0 || gsl_matrix_get(sens, i,j)==0)
        {
            gsl_matrix_set(mask, i,j,0);
            gsl_matrix_set(sens, i,j,0);
        }
    }
}

void dan18::copycorrespondentTransmissions()
{
    copyCorrespondentTransmissions(0);
}

void dan18::copyCorrespondentTransmissions(int startRow)
{
    if (comboBoxMakeScriptTable->count()==0) return;
    
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++  Subtract Bufffer
    bool subtractBuffer= false;
    if (comboBoxMode->currentText().contains("(BS)")) subtractBuffer=true;

    //+++  Polarized Neutrons
    bool polMode = false;
    if (comboBoxMode->currentText().contains("(PN)"))
        polMode = true;

    //+++
    Table* w;

    if (!app()->checkTableExistence(tableName))
        return;

    //+++ Find table
    QList<MdiSubWindow *> tableList = app()->tableList();
    foreach (MdiSubWindow *t, tableList)
        if (t->name() == tableName)
        {
            w = (Table *)t;
            break;
        }

    //+++ scriptTableManager
    if (!scriptTableManager->update(w))
    {
        QMessageBox::critical(nullptr, "qtiSAS",
                              "Table ~" + tableName + " has wrong format. <br> Check table or  generate new one.<h4>");
        return;
    }

    //+++ calculate only selected lines
    bool checkSelection=false;
    
    if(startRow==0)
    {
        
        for (int iRow=0; iRow<(int)tableList.count(); iRow++)
        {
            if (w->isRowSelected(iRow,true))
            {
                checkSelection=true;
                break;
            }
        }
        
        
    }

    if (w->windowLabel() != "DAN::Script::Table")
        return;

    int rowNumber = w->numRows();

    int pos;
    QString name, polarization, polarizationCurr;
    double wl, wlCurr;
    QString trCurr, tr;
    QStringList lst;
    
    for (int i=startRow; i<rowNumber; i++)
    {
        if (scriptTableManager->condition(i) == "")
            continue;

        if (checkSelection && !w->isRowSelected(i,true)) continue;
        
        name = scriptTableManager->info(i);
        polarization = "";
        if (polMode)
            polarization = scriptTableManager->polarization(i).split("-", Qt::SkipEmptyParts)[0];
        
        wl = scriptTableManager->lambda(i).toDouble();

        lst.clear();
        lst = scriptTableManager->transmission(i).remove(" ").split("[", Qt::SkipEmptyParts);

        if (lst[0].toDouble()<=0)
        {
            tr="";
            
            for (int j=0;j<rowNumber; j++)
            {
                polarizationCurr = "";
                if (polMode)
                    polarizationCurr = scriptTableManager->polarization(j).split("-", Qt::SkipEmptyParts)[0];
                wlCurr = scriptTableManager->lambda(j).toDouble();
                trCurr = scriptTableManager->transmission(j);
                lst.clear();
                lst = scriptTableManager->transmission(j).remove(" ").split("[", Qt::SkipEmptyParts);

                if (i != j && name == scriptTableManager->info(j) && polarization == polarizationCurr &&
                    lst[0].toDouble() > 0 && wlCurr < 1.05 * wl && wlCurr > 0.95 * wl)
                    tr = trCurr;
            }
            
            //+++
            if (tr != "")
                scriptTableManager->transmissionWrite(i, tr);
            else
                scriptTableManager->transmissionWrite(i, "check transmission");
        }

        if (subtractBuffer)
        {
            lst.clear();
            lst = scriptTableManager->transmissionBuffer(i).remove(" ").split("[", Qt::SkipEmptyParts);

            if (lst[0].toDouble()<=0)
            {
                tr="";
                
                for (int j=0;j<rowNumber; j++)
                {
                    wlCurr = scriptTableManager->lambda(j).toDouble();
                    trCurr = scriptTableManager->transmissionBuffer(j);
                    lst.clear();
                    lst = scriptTableManager->transmissionBuffer(j).remove(" ").split("[", Qt::SkipEmptyParts);

                    if (i != j && name == scriptTableManager->info(j) && lst[0].toDouble() > 0 && wlCurr < 1.05 * wl &&
                        wlCurr > 0.95 * wl)
                        tr = trCurr;
                }

                if (tr != "")
                    scriptTableManager->transmissionBufferWrite(i, tr);
                else
                    scriptTableManager->transmissionBufferWrite(i, "check transmission");
            }
        }
    }
}

void dan18::updateMaskNamesInScript(int startRow, QString headerName)
{
    
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    
    //+++
    Table* w;

    if (!app()->checkTableExistence(tableName))
        return;

    //+++ Find table
    QList<MdiSubWindow *> tableList = app()->tableList();
    foreach (MdiSubWindow *t, tableList)
        if (t->name() == tableName)
        {
            w = (Table *)t;
            break;
        }

    if (w->windowLabel() != "DAN::Script::Table")
        return;

    //+++
    if (!scriptTableManager->update(w))
    {
        QMessageBox::critical(nullptr, "qtiSAS",
                              "Table ~" + tableName + " has wrong format. <br> Check table or  generate new one.<h4>");
        return;
    }

    //+++ Indexing
    QStringList scriptColList = w->colNames();

    //+++
    int index = scriptColList.indexOf(headerName);

    if (index<0) return;
    
    int rowNumber=w->numRows();
    int cond;
    
    for(int iter=startRow; iter<rowNumber;iter++)
    {
        cond = scriptTableManager->condition(iter).toInt();
        
        if (headerName.contains("Mask"))
        {
            if (cond==0) w->setText(iter,index,comboBoxMaskFor->currentText());
            else w->setText(iter,index,((QComboBoxInTable*)tableEC->cellWidget(dptMASK,fabs(cond)-1))->currentText());
        }
        else if (headerName.contains("Sens"))
        {
            if (cond==0) w->setText(iter,index,comboBoxSensFor->currentText());
            else w->setText(iter,index,((QComboBoxInTable*)tableEC->cellWidget(dptSENS,fabs(cond)-1))->currentText());
        }
        else
        {
            if (cond == 0)
                w->setText(iter, index, "out");
            else
                w->setText(iter, index,
                           ((QComboBoxInTable *)tableEC->cellWidget(dptPOL, abs(cond) - 1))->currentText());
        }
    }
}

void dan18::updateColInScript(const QString &colName, int rowIndex, int startRow)
{
    
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++
    Table* w;

    if (!app()->checkTableExistence(tableName))
        return;

    //+++ Find table
    QList<MdiSubWindow *> tableList = app()->tableList();
    foreach (MdiSubWindow *t, tableList)
        if (t->name() == tableName)
        {
            w = (Table *)t;
            break;
        }

    if (w->windowLabel() != "DAN::Script::Table")
        return;

    if (!scriptTableManager->update(w))
    {
        QMessageBox::critical(nullptr, "qtiSAS",
                              "Table ~" + tableName + " has wrong format. <br> Check table or  generate new one.<h4>");
        return;
    }

    //+++ Indexing
    QStringList scriptColList=w->colNames();
    
    //+++  ColName +++
    int index=scriptColList.indexOf(colName);
    if (index<0) return;
    
    int rowNumber=w->numRows();
    int cond;
    
    for(int iter=startRow; iter<rowNumber;iter++)
    {
        cond = scriptTableManager->condition(iter).toInt();
        if (cond>0)
        {
            w->setText(iter,index, tableEC->item(rowIndex,int(fabs(double(cond)))-1)->text());
        }
    }
}
//*******************************************
//+++  Merging Table Generation
//*******************************************
bool dan18::generateMergingTable(Table *scriptTable, const QStringList &generatedTables, const int &startingRaw)
{
    if (checkBoxSortOutputToFolders->isChecked())
        app()->changeFolder("DAN :: script, info, ...");

    QString name = scriptTable->name() + "-mergingTemplate";

    removeWindows(name);

    Table *t = app()->newHiddenTable(name, "DAN::Merging::Template", 0, sliderConfigurations->value() + 1);

    // Col-Types: Text
    for (int tt = 0; tt < t->numCols(); tt++)
        t->setColumnType(tt, Table::Text);

    QStringList usedNames;
    QString currentSample;

    int numRows = 0;

    for (int i = 0; i < (int)generatedTables.count(); i++)
    {
        if (scriptTable->text(i + startingRaw, 2).toInt() > 0 && generatedTables[i] != "-0-")
        {
            currentSample = scriptTable->text(i + startingRaw, 0);
            if (usedNames.count(currentSample) > 0)
                t->setText(static_cast<int>(usedNames.indexOf(currentSample)),
                           scriptTable->text(i + startingRaw, 2).toInt(), generatedTables[i]);
            else
            {
                numRows++;
                t->setNumRows(numRows);
                t->setText(numRows - 1, 0, currentSample);
                t->setText(numRows - 1, scriptTable->text(i + startingRaw, 2).toInt(), generatedTables[i]);
                usedNames << currentSample;
            }
        }
    }

    if (comboBoxMode->currentText().contains("(MS)"))
    {
        numRows = t->numRows();
        t->setNumRows(2 * numRows);
        QString sTemp;

        for (int i = 0; i < numRows; i++)
        {
            t->setText(i + numRows, 0, t->text(i, 0) + "-MC");
            t->setText(i, 0, t->text(i, 0) + "-NC");

            for (int j = 0; j < sliderConfigurations->value(); j++)
            {
                sTemp = t->text(i, j + 1);
                t->setText(i + numRows, j + 1, sTemp.replace("NC", "MC"));
            }
        }
    }

    for (int i = 0; i < t->numRows(); i++)
    {
        currentSample = t->text(i, 0);
        currentSample = currentSample.replace("]", " ");
        currentSample = currentSample.replace("[", "s");
        currentSample = currentSample.simplified();
        currentSample = currentSample.replace(" ", "-");
        t->setText(i, 0, currentSample);
    }

    t->adjustColumnsWidth(false);

    app()->setListViewLabel(t->name(), "DAN::Merging::Template");
    app()->updateWindowLists(t);
    app()->hideWindow(t);

    return true;
}

//+++ 2020
void dan18::calculateCentersInScript()
{
    calculateCentersInScript(0);
}

void dan18::calculateCentersInScript(int startRow)
{
    int MD = lineEditMD->text().toInt();

    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++ optional parameter if column "VShift" exist
    double VShift;
    
    //+++ optional parameter if column "HShift" exist
    double HShift;

    //+++
    Table *w;
    if (!app()->checkTableExistence(tableName))
        return;

    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName) { w=(Table *)t; break;}
    
    //+++ calculate only selected lines
    bool checkSelection=false;
    
    if(startRow==0)
    {
        for (int iRow=0; iRow<(int)tableList.count(); iRow++)
        {
            if (w->isRowSelected(iRow,true))
            {
                checkSelection=true;
                break;
            }
        }
    }

    if (w->windowLabel() != "DAN::Script::Table")
        return;

    if (!scriptTableManager->update(w))
    {
        QMessageBox::critical(nullptr, "qtiSAS",
                              "Table ~" + tableName + " has wrong format. <br> Check table or  generate new one.<h4>");
        return;
    }

    int rowNumber = w->numRows();

    //+++ Calculate transmission
    QString SAMPLEnumber="";
    QString ECnumber="";
    QString BCnumber="";

    int iMax=tableEC->columnCount();
    int iter;
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    //+++ Sample allocation
    gsl_matrix *sample=gsl_matrix_alloc(MD,MD);
    //+++ EC/EB allocation
    gsl_matrix *ec=gsl_matrix_alloc(MD,MD);
    //+++ BC allocation
    gsl_matrix *bc=gsl_matrix_alloc(MD,MD);
    
    double trans=1.0;
    
    double Xcenter, Ycenter, XcErr, YcErr;
    
    QStringList lst;
    
    std::cout<<"\n"<<std::flush;
    
    for(iter=startRow; iter<rowNumber;iter++)
    {
        if (scriptTableManager->condition(iter) == "")
            continue;
        if (checkSelection && !w->isRowSelected(iter, true))
            continue;

        VShift = scriptTableManager->VSchift(iter).toDouble();
        HShift = scriptTableManager->HSchift(iter).toDouble();

        SAMPLEnumber = scriptTableManager->runSample(iter);
        if (!filesManager->checkFileNumber(SAMPLEnumber))
            continue;

        ECnumber = scriptTableManager->runEC(iter);
        if (!filesManager->checkFileNumber(ECnumber))
            ECnumber = "";

        BCnumber = scriptTableManager->runBC(iter);
        if (!filesManager->checkFileNumber(BCnumber))
            BCnumber = "";

        QString maskName = scriptTableManager->mask(iter);

        QString sensName = scriptTableManager->sens(iter);

        double trans=1.0;
        
        
        //+++ mask gsl matrix
        gsl_matrix_set_all(mask, 1.0);
        make_GSL_Matrix_Symmetric( maskName, mask, MD);
        
        //+++ sens gsl matrix
        gsl_matrix_set_all(sens, 1.0);
        make_GSL_Matrix_Symmetric( sensName, sens, MD);
        
        //+++ sample
        gsl_matrix_set_zero(sample);
        readMatrixCor( SAMPLEnumber, sample );
        gslMatrixShift(sample, MD, HShift, VShift );
        
        //+++ ec
        gsl_matrix_set_zero(ec);
        if (ECnumber!="") readMatrixCor( ECnumber,ec);
        
        //+++ bc
        gsl_matrix_set_zero(bc);
        
        if (BCnumber!="")
        {
            if (checkBoxBCTimeNormalization->isChecked())
            {
                readMatrixCorTimeNormalizationOnly( BCnumber, bc );
                //Normalization constant
                double TimeSample=spinBoxNorm->value();
                double ttime = monitors->readDuration(SAMPLEnumber);
                if (ttime > 0.0)
                    TimeSample /= ttime;
                else
                    TimeSample = 0.0;

                double NormSample = monitors->normalizationFactor(SAMPLEnumber);

                if (TimeSample>0) NormSample/=TimeSample; else NormSample=0;
                
                gsl_matrix_scale(bc,NormSample);
            }
            else readMatrixCor( BCnumber, bc);
        }
        
        //+++ transmission check
        lst.clear();

        QString s = scriptTableManager->transmission(iter);

        s=s.remove(" ").remove(QChar(177)).remove("\t").remove("]");
        lst = s.split("[", Qt::SkipEmptyParts);

        trans=lst[0].toDouble();

        //+++ X-center check
        Xcenter = scriptTableManager->centerX(iter).toDouble();

        //+++ Y-center check
        Ycenter = scriptTableManager->centerY(iter).toDouble();

        gsl_matrix_sub(sample,bc);                       // SAMPLE=SAMPLE-BC
        gsl_matrix_sub(ec,bc);                          // EC=EC-BC
        gsl_matrix_scale(ec,trans);
        gsl_matrix_sub(sample,ec);
        
        //fileNumber=BCFile.toInt();
        gsl_matrix_mul_elements(sample,mask);
        gsl_matrix_mul_elements(sample,sens);
        
        if (radioButtonRadStdSymm->isChecked())
        {
            
            calcCenterUniSymmetryX(MD, sample, mask, Xcenter, XcErr);
            calcCenterUniSymmetryY(MD, sample, mask, Ycenter, YcErr);
        }
        else
        {
            calcCenterUniHF(MD, sample, mask, Xcenter, Ycenter, XcErr, YcErr );
        }

        scriptTableManager->centerXWrite(iter, QString::number(Xcenter));
        scriptTableManager->centerYWrite(iter, QString::number(Ycenter));

        std::cout << "Sample: " << SAMPLEnumber.toLocal8Bit().constData() << "\tXc = " << QString::number(Xcenter,'f',3).toLocal8Bit().constData() << " [+/- " << QString::number(XcErr,'f',3).toLocal8Bit().constData() << "]" << "\tYc = ";
        std::cout << QString::number(Ycenter,'f',3).toLocal8Bit().constData() << " [+/- " << QString::number(YcErr,'f',3).toLocal8Bit().constData() << "]";
        std::cout << "\t" << "... EC: " << ECnumber.toLocal8Bit().constData() << " BC: " << BCnumber.toLocal8Bit().constData() << " Tr: " << trans;
        std::cout << " mask: " << maskName.toLocal8Bit().constData() << " sens: " << sensName.toLocal8Bit().constData();
        std::cout << "\n" << std::flush;
    }
    std::cout<<"\n"<<std::flush;
        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(sample);
        gsl_matrix_free(ec);
        gsl_matrix_free(bc);
}

//+++ 2020
void dan18:: calculateAbsFactorInScript()
{
    calculateAbsFactorInScript(0);
}

void dan18::calculateAbsFactorInScript(int startRow)
{
    int MD = lineEditMD->text().toInt();

    if (comboBoxACmethod->currentIndex()!=1) return;

    //+++
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++
    Table* w;
    
    //+++
    if (!app()->checkTableExistence(tableName))
        return;

    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName) { w=(Table *)t; break;}
    
    //+++ calculate only selected lines
    bool checkSelection=false;
    
    if(startRow==0)
    {
        for (int iRow=0; iRow<(int)tableList.count(); iRow++)
        {
            if (w->isRowSelected(iRow,true))
            {
                checkSelection=true;
                break;
            }
        }
    }

    if (w->windowLabel() != "DAN::Script::Table")
        return;

    if (!scriptTableManager->update(w))
    {
        QMessageBox::critical(nullptr, "qtiSAS",
                              "Table ~" + tableName + " has wrong format. <br> Check table or  generate new one.<h4>");
        return;
    }

    int rowNumber=w->numRows();
    
    //+++ Calculate transmission
    QString ECnumber="";
    QString BCnumber="";
    
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    //+++ sens gsl matrix
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    //+++ EC/EB allocation
    gsl_matrix *ec=gsl_matrix_alloc(MD,MD);
    //+++ BC allocation
    gsl_matrix *bc=gsl_matrix_alloc(MD,MD);
    
    std::cout<<"\n"<<std::flush;

    for(int iter=startRow; iter<rowNumber;iter++)
    {
        if (scriptTableManager->condition(iter) == "")
            continue;
        if (checkSelection && !w->isRowSelected(iter, true))
            continue;
        int col = scriptTableManager->condition(iter).toInt() - 1;
        if (col < 0)
            continue;

        ECnumber = scriptTableManager->runEC(iter);
        if (!filesManager->checkFileNumber(ECnumber))
            continue;

        BCnumber = scriptTableManager->runBC(iter);
        if (!filesManager->checkFileNumber(BCnumber))
            BCnumber = "";

        QString maskName = scriptTableManager->mask(iter);
        QString sensName = scriptTableManager->sens(iter);

        //+++ mask gsl matrix
        gsl_matrix_set_all(mask, 1.0);
        make_GSL_Matrix_Symmetric( maskName, mask, MD);
        
        //+++ sens gsl matrix
        gsl_matrix_set_all(sens, 1.0);
        make_GSL_Matrix_Symmetric( sensName, sens, MD);
        
        
        //+++ ec
        gsl_matrix_set_zero(ec);
        readMatrixCor( ECnumber,ec);
        
        //+++ bc
        gsl_matrix_set_zero(bc);
        
        if (BCnumber!="")
        {
            if (checkBoxBCTimeNormalization->isChecked())
            {
                readMatrixCorTimeNormalizationOnly( BCnumber, bc );
                //Normalization constant
                double TimeEC=spinBoxNorm->value();
                double ttime = monitors->readDuration(ECnumber);
                if (ttime > 0.0)
                    TimeEC /= ttime;
                else
                    TimeEC = 0.0;

                double NormEC = monitors->normalizationFactor(ECnumber);

                if (TimeEC>0) NormEC/=TimeEC; else NormEC=0;
                
                gsl_matrix_scale(bc,NormEC);
            }
            else readMatrixCor( BCnumber, bc);
        }

        gsl_matrix_sub(ec,bc);                          // EC=EC-BC

        gsl_matrix_mul_elements(ec,mask);
        gsl_matrix_mul_elements(ec,sens);

        double transEC=tableEC->item(dptECTR,col)->text().toDouble();
        
        // atten
        double attenuation = tableEC->item(dptACTR,col)->text().toDouble();
        if (attenuation<=0.0 || attenuation>1.0) attenuation=1.0;
        
        // mu
        double mu = tableEC->item(dptACMU,col)->text().toDouble();
        if (mu<=0) attenuation=1.0;
        
        double D=tableEC->item(dptDAC,col)->text().toDouble();
        
        //already in m
        D*=100.0; // cm
        
        double area=lineEditPS->text().toDouble(); //cm^2
        area=area*area;
        area*=lineEditAsymetry->text().toDouble();
        
        double I0=0;
        
        for(int ii=0;ii<MD;ii++) for(int jj=0;jj<MD;jj++)
        {
            I0+=gsl_matrix_get(ec,ii,jj);
        }
        double II=I0;
        I0/=attenuation;
        I0*=mu;
        I0*=area/D/D;

        scriptTableManager->absoluteFactorWrite(iter, QString::number(1 / I0, 'E', 4));

        std::cout<<"EC/EB: "<<ECnumber.toLocal8Bit().constData()<<"\tFactor = "<<QString::number(1/I0,'E',4).toLocal8Bit().constData();
        std::cout<<"\t"<<" BC: "<<BCnumber.toLocal8Bit().constData()<<" mask: "<<maskName.toLocal8Bit().constData()<<" sens: "<<sensName.toLocal8Bit().constData()<<" D: "<<D<<" area: "<<area<<" I0: "<<II;
        std::cout<<"\n"<<std::flush;
    }
    std::cout<<"\n"<<std::flush;
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    gsl_matrix_free(ec);
    gsl_matrix_free(bc);
}

void dan18::calculateTrMaskDB()
{
    calculateTrMaskDB(0);
    if (checkBoxForceCopyPaste->isChecked()) copycorrespondentTransmissions();
}

void dan18::calculateTrMaskDB(int startRow)
{
    //+++
    if (comboBoxMakeScriptTable->count()==0) return;
    
    //+++
    QString tableName=comboBoxMakeScriptTable->currentText();
    
    //+++  Subtract Bufffer
    bool subtractBuffer= false;
    if (comboBoxMode->currentText().contains("(BS)")) subtractBuffer=true;
    
    
    //+++  Subtract Bufffer && Correct to own Sensitivity
    bool bufferAsSens= false;
    if (comboBoxMode->currentText().contains("(BS-SENS)")) bufferAsSens=true;
    
    
    //+++ optional parameter if column "VShift" exist
    double VShift;
    
    //+++ optional parameter if column "HShift" exist
    double HShift;
    
    //+++
    Table* w;

    if (!app()->checkTableExistence(tableName))
        return;

    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==tableName)  w=(Table *)t;
    //+++
    
    //+++ calculate only selected lines
    bool checkSelection=false;
    
    if(startRow==0)
    {
        
        for (int iRow=0; iRow<(int)tableList.count(); iRow++)
        {
            if (w->isRowSelected(iRow,true))
            {
                checkSelection=true;
                break;
            }
        }
        
        
    }

    if (w->windowLabel() != "DAN::Script::Table")
        return;

    if (!scriptTableManager->update(w))
    {
        QMessageBox::critical(nullptr, "qtiSAS",
                              "Table ~" + tableName + " has wrong format. <br> Check table or  generate new one.<h4>");
        return;
    }

    int rowNumber = w->numRows();
    
    //+++ Calculate transmission
    QString ECnumber;
    double XCenter, YCenter;
    double trans=-1;
    double sigmaTr=0;
    int iMax=tableEC->columnCount();
    int iter;
    
    //+++ new active conditions
    QList<int> listTr;
    
    for (int i=0; i<iMax; i++)
    {
        if (tableEC->item(dptECTR, i)->checkState() == Qt::Checked) listTr<<(i+1);
    }
    
    for(iter=startRow; iter<rowNumber;iter++)
    {
        sigmaTr=0;

        if (scriptTableManager->condition(iter) == "")
            continue;

        if (checkSelection && !w->isRowSelected(iter, true))
            continue;

        if (listTr.indexOf(scriptTableManager->condition(iter).toInt()) >= 0)
        {
            VShift = scriptTableManager->VSchift(iter).toDouble();
            HShift = scriptTableManager->HSchift(iter).toDouble();

            ECnumber = scriptTableManager->runEC(iter);

            //+++ X-center check
            XCenter = scriptTableManager->centerX(iter).toDouble();

            //+++ Y-center check
            YCenter = scriptTableManager->centerY(iter).toDouble();

            if (ECnumber == "" && comboBoxTransmMethod->currentIndex() != 2)
            {
                scriptTableManager->transmissionWrite(iter, "no EC-file");
                if (subtractBuffer)
                    scriptTableManager->transmissionBufferWrite(iter, "no EC-file");
                continue;
            }

            if (filesManager->checkFileNumber(ECnumber) || comboBoxTransmMethod->currentIndex() == 2)
            {
                double Radius = scriptTableManager->maskDB(iter).toDouble();
                sigmaTr = 0;
                trans = readTransmissionMaskDB(scriptTableManager->runSample(iter), ECnumber, VShift, HShift, XCenter,
                                               YCenter, Radius, sigmaTr);

                if (trans > 2.00 || trans <= 0)
                    scriptTableManager->transmissionWrite(iter, "check!!!");
                else
                    scriptTableManager->transmissionWrite(iter, "   " + QString::number(trans, 'f', 4) + " [ " +
                                                                    QChar(177) +
                                                                    QString::number(trans * sigmaTr, 'f', 4) + " ]");
                if (subtractBuffer)
                {
                    sigmaTr = 0;
                    trans = readTransmissionMaskDB(scriptTableManager->transmissionBuffer(iter), ECnumber, VShift,
                                                   HShift, XCenter, YCenter, Radius, sigmaTr);
                    if (trans > 2.00 || trans <= 0)
                        scriptTableManager->transmissionBufferWrite(iter, "check!!!");
                    else
                        scriptTableManager->transmissionBufferWrite(
                            iter, "   " + QString::number(trans, 'f', 4) + " [ " + QChar(177) +
                                      QString::number(trans * sigmaTr, 'f', 4) + " ]");
                }
            }
            else
            {
                scriptTableManager->transmissionWrite(iter, "check file!!!");
                if (subtractBuffer)
                    scriptTableManager->transmissionBufferWrite(iter, "check file!!!");
            }
        }
        else
        {
            scriptTableManager->transmissionWrite(iter, "Not active configuration");
            if (subtractBuffer)
                scriptTableManager->transmissionBufferWrite(iter, "Not active configuration");
        }
    }
}

void dan18::dataProcessingOptionSelected()
{
    if (radioButtonDpSelector2D->isChecked()) stackedWidgetDpOptions->setCurrentIndex(0);
    else if (radioButtonDpSelector1D->isChecked()) stackedWidgetDpOptions->setCurrentIndex(1);
    else stackedWidgetDpOptions->setCurrentIndex(2);
}


