/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Vitaliy Pipich <v.pipich@gmail.com>
Description: Polarized Neutron Mode Functions
 ******************************************************************************/
#include "dan18.h"

//+++
// new gsl_matrix function for MD x MD matrix:
// a = a / b (only for non-masked pixels defined in mask)
//+++
void gsl_matrix_div_elements_with_mask(gsl_matrix *a, gsl_matrix *b, gsl_matrix *mask, int MD)
{
    for (int i = 0; i < MD; i++)
        for (int j = 0; j < MD; j++)
            if (gsl_matrix_get(mask, i, j) != 0.0)
            {
                if (gsl_matrix_get(b, i, j) != 0.0)
                    gsl_matrix_set(a, i, j, gsl_matrix_get(a, i, j) / gsl_matrix_get(b, i, j));
                else
                    gsl_matrix_set(a, i, j, 0.0);
            }
}
//+++
// new gsl_matrix function for MD x MD matrix:
// a = scale * b
//+++
void gsl_matrix_addscaled(gsl_matrix *a, gsl_matrix *b, double scale, int MD)
{
    gsl_matrix *m = gsl_matrix_calloc(MD, MD);
    gsl_matrix_memcpy(m, b);
    gsl_matrix_scale(m, scale);
    gsl_matrix_add(a, m);
    gsl_matrix_free(m);
}
//+++  polarizedConnectSlots()
void dan18::polarizedConnectSlots()
{
    connect(pushButtonNewPolScript, SIGNAL(clicked()), this, SLOT(newPolarizedScriptTable()));
    connect(pushButtonPolarizationScriptUpdate, SIGNAL(clicked()), this, SLOT(updatePolarizedScriptTable()));
}
//+++  polarizedAlias(...)
int dan18::polarizedAlias(QString &valueFromHeader) const
{
    if (!comboBoxMode->currentText().contains("(PN)"))
    {
        valueFromHeader = "out";
        return 0;
    }
    if (valueFromHeader.contains(lineEditDownUp->text()))
    {
        valueFromHeader = "down-up";
        return 6;
    }
    if (valueFromHeader.contains(lineEditDownDown->text()))
    {
        valueFromHeader = "down-down";
        return 5;
    }
    if (valueFromHeader.contains(lineEditUpDown->text()))
    {
        valueFromHeader = "up-down";
        return 4;
    }
    if (valueFromHeader.contains(lineEditUpUp->text()))
    {
        valueFromHeader = "up-up";
        return 3;
    }
    if (valueFromHeader.contains(lineEditDown->text()))
    {
        valueFromHeader = "down";
        return 2;
    }
    if (valueFromHeader.contains(lineEditUp->text()))
    {
        valueFromHeader = "up";
        return 1;
    }
    valueFromHeader = "out";
    return 0;
}
//+++ danDanMultiButtonPN
void dan18::danDanMultiButtonPN(const QString &button)
{
    //+++
    // step #0: extruction of needed parameters from DAN interface
    //+++
    int MD = lineEditMD->text().toInt();
    bool radioButtonOpenInProjectisChecked = radioButtonOpenInProject->isChecked();
    bool checkBoxSortOutputToFoldersisChecked = checkBoxSortOutputToFolders->isChecked();

    //+++
    // step #1: polarized-script-table "selection": __wPol__
    //+++
    QString tableName = comboBoxPolarizationScriptTable->currentText();
    if (tableName == "")
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Create first Polarized-Script-Table");
        return;
    }

    Table *wPol;
    bool tableExists = false;
    QList<MdiSubWindow *> tableList = app()->tableList();
    foreach (MdiSubWindow *t, tableList)
        if (t->name() == tableName)
        {
            wPol = (Table *)t;
            tableExists = true;
            break;
        }

    if (!tableExists)
    {
        QMessageBox::critical(nullptr, "QtiSAS",
                              "Polarized-Script-Table ~" + tableName + "~ does not exist !!! <br><h4>");

        QStringList list;
        findTableListByLabel("DAN::Polarized::Table", list);
        comboBoxPolarizationScriptTable->clear();
        comboBoxPolarizationScriptTable->addItems(list);

        return;
    }

    //+++
    // step #2: corresponding main script-table "selection": __wScript__
    //+++
    QString scriptTableName = comboBoxMakeScriptTable->currentText();
    if (scriptTableName == "")
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Create first Script-Table");
        return;
    }

    Table *wScript;
    tableExists = false;
    foreach (MdiSubWindow *t, tableList)
        if (t->name() == scriptTableName)
        {
            wScript = (Table *)t;
            tableExists = true;
            break;
        }

    if (!tableExists)
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Script-Table ~" + scriptTableName + "~ does not exist !!! <br><h4>");

        QStringList list;
        findTableListByLabel("DAN::Script::Table", list);
        comboBoxMakeScriptTable->clear();
        comboBoxMakeScriptTable->addItems(list);

        return;
    }

    //+++
    // step #2.1: Indexing of columns in  script-table  __wScript__
    //+++
    QStringList scriptColList = wScript->colNames();

    //+++ Run-info
    int indexInfo = scriptColList.indexOf("Run-info");

    //+++  #-Condition : condition of the Sample
    int indexCond = scriptColList.indexOf("#-Condition");
    if (indexCond < 0)
    {
        std::cout << "#-Condition column is missing ...\n";
        return;
    }
    //+++ #-Run : Sample Run Number
    int indexSample = scriptColList.indexOf("#-Run");
    if (indexSample < 0)
    {
        std::cout << "#-Run column is missing ...\n";
        return;
    }
    //+++  #-EC [EB]
    int indexEC = scriptColList.indexOf("#-EC [EB]");
    if (indexEC < 0)
    {
        std::cout << "#-EC [EB] column is missing ...\n";
        return;
    }
    //+++ D: sample-to-detector distance
    int indexD = scriptColList.indexOf("D");
    if (indexD < 0)
    {
        std::cout << "D-column does not exists ...\n";
        return;
    }
    //+++ Lambda
    int indexLam = scriptColList.indexOf("Lambda");
    if (indexLam < 0)
    {
        std::cout << "Lambda-column does not exists ...\n";
        return;
    }
    //+++  X-center
    int indexXC = scriptColList.indexOf("X-center");
    if (indexXC < 0)
    {
        std::cout << "X-center column does not exists ...\n";
        return;
    }
    //+++  Y-center
    int indexYC = scriptColList.indexOf("Y-center");
    if (indexYC < 0)
    {
        std::cout << "Y-center column does not exists ...\n";
        return;
    }
    //+++ optional parameter if column "Suffix" exist
    int indexSuffix = scriptColList.indexOf("Suffix");

    //+++
    // step #3.1: list of all unique samples __allUniqueSamples__ in polarized-script-table
    //+++
    QStringList allUniqueSamples;
    for (int rowPol = 0; rowPol < wPol->numRows(); rowPol++)
    {
        QString currentSample = wPol->text(rowPol, 0).simplified();
        if (currentSample == "")
            continue;

        if (!allUniqueSamples.contains(currentSample))
            allUniqueSamples << currentSample;
    }

    //+++
    // step #3.2: list of all unique confogurations __allUniqueConfigurations__ in polarized-script-table
    //+++
    QList<int> allUniqueConfigurations;
    for (int rowPol = 0; rowPol < wPol->numRows(); rowPol++)
    {
        int currentConfiguration = wPol->text(rowPol, 1).toInt();
        if (currentConfiguration <= 0)
            continue;

        if (!allUniqueConfigurations.contains(currentConfiguration))
            allUniqueConfigurations << currentConfiguration;
    }

    //+++
    // step #3.3: 2/3-dimensional lists of lines in wPol / wScript tables
    //+++

    //+++  allUniqueSamples[iSample]: linesInPolScript[iSample][iConf]
    QList<QList<int>> linesInPolScript;
    QList<int> linesCurrentSampleInPolScript;

    //+++ allUniqueSamples[iSample]: linesInScript[iSample][iConf][iPol]
    QList<QList<QList<int>>> linesInScript;
    QList<QList<int>> listCurrentSampleCurrentConfig;
    QList<int> linesCurrentSampleScript;

    for (int iSample = 0; iSample < allUniqueSamples.count(); iSample++)
    {
        linesCurrentSampleInPolScript.clear();
        for (int iConf = 0; iConf < allUniqueConfigurations.count(); iConf++)
            linesCurrentSampleInPolScript << -1;

        for (int rowPol = 0; rowPol < wPol->numRows(); rowPol++)
            if (wPol->text(rowPol, 0).simplified() == allUniqueSamples[iSample])
                linesCurrentSampleInPolScript[allUniqueConfigurations.indexOf(wPol->text(rowPol, 1).toInt())] = rowPol;

        linesInPolScript << linesCurrentSampleInPolScript;

        listCurrentSampleCurrentConfig.clear();
        for (int iConf = 0; iConf < linesCurrentSampleInPolScript.count(); iConf++)
        {
            linesCurrentSampleScript.clear();
            for (int iPol = 0; iPol < 7; iPol++)
                linesCurrentSampleScript << -2;

            if (linesCurrentSampleInPolScript[iConf] >= 0)
                for (int iPol = 0; iPol < 7; iPol++)
                {
                    QString currentRunNumber = wPol->text(linesCurrentSampleInPolScript[iConf], 2 + iPol);
                    if (currentRunNumber != "")
                        for (int rowScript = 0; rowScript < wScript->numRows(); rowScript++)
                            if (wScript->text(rowScript, indexSample) == currentRunNumber)
                            {
                                linesCurrentSampleScript[iPol] = rowScript;
                                break;
                            }
                }
            listCurrentSampleCurrentConfig << linesCurrentSampleScript;
        }

        linesInScript << listCurrentSampleCurrentConfig;
    }

    //+++
    // step #4: mode of the PN: PN / PNx2 / PNx4
    //+++
    QString dataSuffix = "PN";
    int numberCommonFiles = 1;
    if (comboBoxPolReductionMode->currentIndex() == 1)
    {
        dataSuffix += "x2";
        numberCommonFiles = 2;
    }
    else if (comboBoxPolReductionMode->currentIndex() == 2)
    {
        dataSuffix += "x4";
        numberCommonFiles = 4;
    }

    QElapsedTimer time;
    time.start();

    //+++ time steps
    QElapsedTimer dt;
    dt.start();
    qint64 pre_dt = 0;

    printf("\nDAN|START data reduction:\n\n");
    pre_dt = dt.elapsed();

    //+++ Progress Dialog
    QProgressDialog progress;
    progress.setMinimumDuration(10);
    progress.setWindowModality(Qt::WindowModal);
    progress.setRange(0, allUniqueSamples.count() * allUniqueConfigurations.count());
    progress.setCancelButtonText("Abort (PN) Data Processing");
    progress.setMinimumWidth(400);
    progress.setMaximumWidth(4000);
    progress.setLabelText("(PN) Data Processing Started...");
    progress.show();

    app()->workspace()->blockSignals(true);

    //+++ current folder
    Folder *cf;
    MdiSubWindow *activeWindow;
    bool maximizedExists = false;
    if (radioButtonOpenInProjectisChecked && checkBoxSortOutputToFoldersisChecked)
    {
        cf = (Folder *)app()->current_folder;

        if (button == "I-x-y")
            app()->changeFolder("DAN :: I [x,y]");
        else if (button == "dI-x-y")
            app()->changeFolder("DAN :: dI [x,y]");
        else if (button == "I-Q")
            app()->changeFolder("DAN :: I [Q]");
        else if (button == "I-Qx")
            app()->changeFolder("DAN :: I [Qx]");
        else if (button == "I-Qy")
            app()->changeFolder("DAN :: I [Qy]");
        else if (button == "I-Polar")
            app()->changeFolder("DAN :: I [Q,phi]");
        else if (button == "Sigma-x-y")
            app()->changeFolder("DAN :: Sigma [x,y]");
        else if (button == "Q-x-y")
            app()->changeFolder("DAN :: Q [x,y]");
        else if (button == "dQ-x-y")
            app()->changeFolder("DAN :: dQ [x,y]");

        if (cf && cf->activeWindow() && cf->activeWindow()->status() == MdiSubWindow::Maximized)
        {
            activeWindow = cf->activeWindow();
            maximizedExists = true;
        }
    }

    //+++ Final data reduction steps
    for (int iSample = 0; iSample < allUniqueSamples.count(); iSample++)
        for (int iConf = 0; iConf < allUniqueConfigurations.count(); iConf++)
            if (linesInPolScript[iSample][iConf] >= 0)
            {
                //+++ Buffer subtraction variables
                int rowInPol = linesInPolScript[iSample][iConf];
                int rowInPolBuffer = -1;

                if (wPol->numCols() > 10)
                    rowInPolBuffer = wPol->text(rowInPol, 10).toInt() - 1;

                if (rowInPolBuffer < 0 || rowInPolBuffer >= wPol->numRows() || rowInPolBuffer == rowInPol)
                    rowInPolBuffer = -1;

                double bufferVolumeFraction = 0.0;
                QString bufferName = "";
                if (rowInPolBuffer >= 0)
                {
                    bufferVolumeFraction = wPol->text(rowInPol, 9).toDouble();
                    bufferName = wPol->text(rowInPolBuffer, 0).simplified();

                    if (bufferVolumeFraction <= 0 || bufferVolumeFraction > 1.0 || bufferName == "")
                    {
                        bufferVolumeFraction = 0.0;
                        rowInPolBuffer = -1;
                        bufferName = "";
                    }
                }
                //--- Buffer subtraction variables


                if (numberCommonFiles == 1)
                {
                    //+++
                    // "PN" mode: no corrections
                    //+++

                    for (int iPol = 0; iPol < 7; iPol++)
                    {
                        if (linesInScript[iSample][iConf][iPol] < 0)
                            continue;

                        int rowScript = linesInScript[iSample][iConf][iPol];

                        int rowInScriptBuffer = rowInPolBuffer;

                        if (rowInScriptBuffer >= 0)
                            for (int jSample = 0; jSample < allUniqueSamples.count(); jSample++)
                                if (allUniqueSamples[jSample] == bufferName)
                                {
                                    rowInScriptBuffer = linesInScript[jSample][iConf][iPol];
                                    if (rowInScriptBuffer < 0 || rowInScriptBuffer >= wScript->numRows())
                                        rowInScriptBuffer = -1;
                                    break;
                                }

                        gsl_matrix *Sample, *SampleErr;
                        gsl_matrix *EC, *ECErr;
                        gsl_matrix *Buffer, *BufferErr;
                        gsl_matrix *maskSample, *maskBuffer, *maskEC;

                        double absSample, absBuffer;
                        double TrSample, TrBuffer;
                        double TrSampleSigma, TrBufferSigma;
                        double thicknessSample, thicknessBuffer;

                        double analyzerTr = 1.0;
                        double analyzerEff = 1.0;
                        double analyzerTrBuffer = 1.0;
                        double analyzerEffBuffer = 1.0;

                        //+++
                        // Sample Actions
                        //+++

                        //+++ read absSample, thicknessSample, TrSample, TrSampleSigma
                        if (!singleDanFactorTransThickness(wScript, scriptColList, rowScript, absSample, TrSample,
                                                           TrSampleSigma, thicknessSample))
                            continue;

                        //+++ Analyzer transmission
                        singleDanAnalyzerStatus(wScript, scriptColList, rowScript, analyzerTr, analyzerEff);

                        //+++ read Sample/SampleErr Matrixes
                        if (!singleDanSimplified(wScript, scriptColList, rowScript, Sample, SampleErr, maskSample,
                                                 TrSample, false))
                            continue;

                        //+++ Sample(i,j) = Sample(i,j) / TrSample / analyzerTr
                        gsl_matrix_scale(Sample, 1.0 / TrSample / analyzerTr);

                        //+++ SampleErr(i,j) = SampleErr(i,j) + TrSampleSigma * TrSampleSigma
                        gsl_matrix_add_constant(SampleErr, TrSampleSigma * TrSampleSigma);

                        //+++  SampleErr(i,j) = SampleErr(i,j) * Sample(i,j) * Sample(i,j)
                        gsl_matrix_mul_elements(SampleErr, Sample);
                        gsl_matrix_mul_elements(SampleErr, Sample);

                        //+++
                        // Buffer Actions
                        //+++

                        //+++ read absBuffer, thicknessBuffer, TrBuffer, TrBufferSigma
                        if (rowInScriptBuffer >= 0)
                            if (!singleDanFactorTransThickness(wScript, scriptColList, rowInScriptBuffer, absBuffer,
                                                               TrBuffer, TrBufferSigma, thicknessBuffer))
                                rowInScriptBuffer = -1;

                        //+++ Analyzer transmission
                        if (rowInScriptBuffer >= 0)
                            singleDanAnalyzerStatus(wScript, scriptColList, rowInScriptBuffer, analyzerTrBuffer,
                                                    analyzerEffBuffer);

                        //+++ read Buffer/BufferErr matrixes
                        if (rowInScriptBuffer >= 0)
                            if (!singleDanSimplified(wScript, scriptColList, rowInScriptBuffer, Buffer, BufferErr,
                                                     maskBuffer, TrBuffer, false))
                                rowInScriptBuffer = -1;

                        //+++ buffer subtruction if buffer exists
                        if (rowInScriptBuffer >= 0)
                        {
                            if (absBuffer != absSample)
                                std::cout << "!!! : abs.Buffer != abs.Sample: we use abs.Buffer = abs.Sample \n";
                            if (thicknessBuffer != thicknessSample)
                                std::cout << "!!! : thickness.Buffer != thickness.Sample: we use thickness.Buffer = "
                                             "thickness.Sample \n";

                            //+++ Buffer(i,j) = Buffer(i,j) * bufferVolumeFraction / TrBuffer / analyzerTrBuffer
                            gsl_matrix_scale(Buffer, bufferVolumeFraction / TrBuffer / analyzerTrBuffer);

                            //+++ BufferErr(i,j) = BufferErr(i,j) + TrBufferSigma * TrBufferSigma
                            gsl_matrix_add_constant(BufferErr, TrBufferSigma * TrBufferSigma);

                            //+++  BufferErr(i,j) = BufferErr(i,j) * Buffer(i,j) * Buffer(i,j)
                            gsl_matrix_mul_elements(BufferErr, Buffer);
                            gsl_matrix_mul_elements(BufferErr, Buffer);

                            //+++ SampleErr(i,j) = SampleErr(i,j) + BufferErr(i,j)
                            gsl_matrix_add(SampleErr, BufferErr);

                            //+++ Sample(i,j) = Sample(i,j) - Buffer(i,j)
                            gsl_matrix_sub(Sample, Buffer);

                            //+++ maskSample and maskBuffer sinchronization
                            gsl_matrix_mul_elements(maskSample, maskBuffer);

                            // free memory: Buffer / BufferErr /MaskBuffer
                            gsl_matrix_free(Buffer);
                            gsl_matrix_free(BufferErr);
                            gsl_matrix_free(maskBuffer);
                        }

                        //+++
                        // Empty Cell Actions
                        //+++

                        if (rowInScriptBuffer < 0)
                            if (singleDanSimplified(wScript, scriptColList, rowScript, EC, ECErr, maskEC, 1.0, true))
                            {
                                //+++  ECErr(i,j) = ECErr(i,j) * EC(i,j) * EC(i,j)
                                gsl_matrix_mul_elements(ECErr, EC);
                                gsl_matrix_mul_elements(ECErr, EC);

                                //+++ scale to analyzer transmission
                                gsl_matrix_scale(EC, 1.0 / analyzerTr);

                                //+++ Sample(i,j) = Sample(i,j) - EC(i,j)
                                gsl_matrix_sub(Sample, EC);

                                //+++ SampleErr(i,j) = SampleErr(i,j) + ECErr(i,j)
                                gsl_matrix_add(SampleErr, ECErr);

                                //+++ maskSample and maskBuffer sinchronization
                                gsl_matrix_mul_elements(maskSample, maskEC);

                                // free memory: EC / ECErr / MaskEC
                                gsl_matrix_free(EC);
                                gsl_matrix_free(ECErr);
                                gsl_matrix_free(maskEC);
                            }

                        //+++  SampleErr(i,j) = SampleErr(i,j) / Sample(i,j) / Sample(i,j) : back to relative
                        gsl_matrix_div_elements_with_mask(SampleErr, Sample, maskSample, MD);
                        gsl_matrix_div_elements_with_mask(SampleErr, Sample, maskSample, MD);

                        //+++  Sample(i,j) = Sample(i,j) * absSample / thicknessSample : absolute units
                        gsl_matrix_scale(Sample, absSample / thicknessSample);

                        QString label = dataSuffix;
                        switch (iPol)
                        {
                        case 0:
                            label += "-Out";
                            break;
                        case 1:
                            label += "-Up";
                            break;
                        case 2:
                            label += "-Down";
                            break;
                        case 3:
                            label += "-UpUp";
                            break;
                        case 4:
                            label += "-UpDown";
                            break;
                        case 5:
                            label += "-DownDown";
                            break;
                        case 6:
                            label += "-DownUp";
                            break;
                        default:
                            label += "-Out";
                            break;
                        }

                        double time = static_cast<double>(dt.elapsed() - pre_dt) / 1000.0;
                        pre_dt = dt.elapsed();

                        label = singleDanMultiButton(wScript, scriptColList, rowScript, button, label, Sample,
                                                     SampleErr, maskSample, time);

                        gsl_matrix_free(Sample);
                        gsl_matrix_free(SampleErr);
                        gsl_matrix_free(maskSample);
                    }
                }
                else if (numberCommonFiles == 2)
                {
                    //+++
                    // PNx2 mode Down/Up or Min/Plus analysis
                    //+++

                    //+++ sample rows in script table
                    int rowInScriptUp = linesInScript[iSample][iConf][1];
                    int rowInScriptDown = linesInScript[iSample][iConf][2];
                    if (rowInScriptUp < 0 || rowInScriptDown < 0 || rowInScriptUp >= wScript->numRows() ||
                        rowInScriptDown >= wScript->numRows())
                        continue;

                    //+++ buffer rows in script table
                    int rowInScriptBufferUp = rowInPolBuffer;
                    int rowInScriptBufferDown = rowInPolBuffer;

                    if (rowInScriptBufferUp >= 0 && rowInScriptBufferDown >= 0)
                        for (int jSample = 0; jSample < allUniqueSamples.count(); jSample++)
                            if (allUniqueSamples[jSample] == bufferName)
                            {
                                rowInScriptBufferUp = linesInScript[jSample][iConf][1];
                                if (rowInScriptBufferUp < 0 || rowInScriptBufferUp >= wScript->numRows())
                                    rowInScriptBufferUp = -1;
                                rowInScriptBufferDown = linesInScript[jSample][iConf][2];
                                if (rowInScriptBufferDown < 0 || rowInScriptBufferDown >= wScript->numRows())
                                    rowInScriptBufferDown = -1;
                                break;
                            }

                    if (rowInScriptBufferUp < 0 || rowInScriptBufferDown < 0)
                    {
                        rowInScriptBufferUp = -1;
                        rowInScriptBufferDown = -1;
                    }
                    //--- buffer rows

                    gsl_matrix *SampleUp, *SampleDown;
                    gsl_matrix *SampleUpErr, *SampleDownErr;
                    gsl_matrix *maskUp, *maskDown;

                    double absUp, absDown;
                    double TrUp, TrDown;
                    double SigmaTrUp, SigmaTrDown;
                    double thicknessUp, thicknessDown;

                    //+++ SampleDown:: reading absDown, thicknessDown, TrDown and SigmaTrDown
                    if (!singleDanFactorTransThickness(wScript, scriptColList, rowInScriptDown, absDown, TrDown,
                                                       SigmaTrDown, thicknessDown))
                        continue;

                    //+++ SampleDown:: reading SampleDown, SampleDownErr and maskDown matrixes
                    if (!singleDanSimplified(wScript, scriptColList, rowInScriptDown, SampleDown, SampleDownErr,
                                             maskDown, TrDown, false))
                        continue;

                    //+++ SampleUp:: reading absUp, thicknessUp, TrUp and SigmaTrUp
                    if (!singleDanFactorTransThickness(wScript, scriptColList, rowInScriptUp, absUp, TrUp, SigmaTrUp,
                                                       thicknessUp))
                    {
                        gsl_matrix_free(SampleDown);
                        gsl_matrix_free(SampleDownErr);
                        continue;
                    }

                    //+++ SampleUp:: reading  SampleUp, SampleUpErr and maskUp matrixes
                    if (!singleDanSimplified(wScript, scriptColList, rowInScriptUp, SampleUp, SampleUpErr, maskUp, TrUp,
                                             false))
                    {
                        gsl_matrix_free(SampleDown);
                        gsl_matrix_free(SampleDownErr);
                        continue;
                    }

                    //+++ mask sinchro: maskDown(i,j) = maskDown(i,j) * maskUp(i,j)
                    gsl_matrix_mul_elements(maskDown, maskUp);

                    //+++ current wave length
                    double lambda = wScript->text(rowInScriptDown, indexLam).toDouble();

                    //+++ polarizer: current polarization and flipper efficiency
                    double P = polarizationSelector->getValue(lambda, wScript->text(rowInScriptDown, indexSample));
                    double f =
                        polFlipperEfficiencySelector->getValue(lambda, wScript->text(rowInScriptDown, indexSample));
                    double Pf = 2 * f - 1.0;

                    //+++ Transmission normalization: TrDown -> Tr-; TrUp -> Tr+
                    double TRmin = (P * Pf + 1) / (P * (Pf + 1)) * TrDown + (P - 1) / (P * (Pf + 1)) * TrUp;

                    double TRminSigma2 = 0;
                    TRminSigma2 += (P * Pf + 1) * (P * Pf + 1) * SigmaTrDown * SigmaTrDown;
                    TRminSigma2 += (P - 1) * (P - 1) * SigmaTrUp * SigmaTrUp;
                    TRminSigma2 /= (P * (Pf + 1) * P * (Pf + 1));

                    double TRplus = (P * Pf - 1) / (P * (Pf + 1)) * TrDown + (P + 1) / (P * (Pf + 1)) * TrUp;

                    double TRplusSigma2 = 0;
                    TRplusSigma2 += (P * Pf - 1) * (P * Pf - 1) * SigmaTrDown * SigmaTrDown;
                    TRplusSigma2 += (P + 1) * (P + 1) * SigmaTrUp * SigmaTrUp;
                    TRplusSigma2 /= (P * (Pf + 1) * P * (Pf + 1));

                    //+++ SampleDown -> I-; SampleUp -> I+

                    gsl_matrix *Imin = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_memcpy(Imin, SampleDown);
                    gsl_matrix_scale(Imin, (P * Pf + 1.0) / P / (Pf + 1) / TRmin);

                    gsl_matrix *IminErr = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_memcpy(IminErr, SampleDownErr);
                    gsl_matrix_mul_elements(IminErr, Imin);
                    gsl_matrix_mul_elements(IminErr, Imin);

                    gsl_matrix *mTemp = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_memcpy(mTemp, SampleUp);
                    gsl_matrix_scale(mTemp, (P - 1) / P / (Pf + 1) / TRmin);

                    gsl_matrix *mTempErr = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_memcpy(mTempErr, SampleUpErr);
                    gsl_matrix_mul_elements(mTempErr, mTemp);
                    gsl_matrix_mul_elements(mTempErr, mTemp);

                    gsl_matrix_add(Imin, mTemp);
                    gsl_matrix_add(IminErr, mTempErr);
                    gsl_matrix_div_elements_with_mask(IminErr, Imin, maskDown, MD);
                    gsl_matrix_div_elements_with_mask(IminErr, Imin, maskDown, MD);

                    gsl_matrix_add_constant(IminErr, TRminSigma2);

                    gsl_matrix *Iplus = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_memcpy(Iplus, SampleUp);
                    gsl_matrix_scale(Iplus, (P + 1.0) / P / (Pf + 1) / TRplus);

                    gsl_matrix *IplusErr = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_memcpy(IplusErr, SampleUpErr);
                    gsl_matrix_mul_elements(IplusErr, Iplus);
                    gsl_matrix_mul_elements(IplusErr, Iplus);

                    gsl_matrix_memcpy(mTemp, SampleDown);
                    gsl_matrix_scale(mTemp, (P * Pf - 1.0) / P / (Pf + 1) / TRplus);

                    gsl_matrix_memcpy(mTempErr, SampleDownErr);
                    gsl_matrix_mul_elements(mTempErr, mTemp);
                    gsl_matrix_mul_elements(mTempErr, mTemp);

                    gsl_matrix_add(Iplus, mTemp);

                    gsl_matrix_add(IplusErr, mTempErr);
                    gsl_matrix_div_elements_with_mask(IplusErr, Iplus, maskDown, MD);
                    gsl_matrix_div_elements_with_mask(IplusErr, Iplus, maskDown, MD);

                    gsl_matrix_add_constant(IplusErr, TRplusSigma2);

                    // free Sample* matrixes
                    gsl_matrix_free(SampleUp);
                    gsl_matrix_free(SampleUpErr);
                    gsl_matrix_free(SampleDown);
                    gsl_matrix_free(SampleDownErr);

                    //+++
                    // Buffer Actions
                    //+++

                    double absBufferUp, TrBufferUp, TrBufferSigmaUp, thicknessBufferUp;
                    double absBufferDown, TrBufferDown, TrBufferSigmaDown, thicknessBufferDown;
                    gsl_matrix *BufferUp, *BufferErrUp, *maskBufferUp;
                    gsl_matrix *BufferDown, *BufferErrDown, *maskBufferDown;

                    //+++ BufferUp:: reading absBuffer, thicknessBuffer, TrBuffer, TrBufferSigma : Up
                    if (rowInScriptBufferUp >= 0)
                        if (!singleDanFactorTransThickness(wScript, scriptColList, rowInScriptBufferUp, absBufferUp,
                                                           TrBufferUp, TrBufferSigmaUp, thicknessBufferUp))
                            rowInScriptBufferUp = -1;

                    //+++ BufferUp:: read Buffer/BufferErr matrixes : Up
                    if (rowInScriptBufferUp >= 0)
                        if (!singleDanSimplified(wScript, scriptColList, rowInScriptBufferUp, BufferUp, BufferErrUp,
                                                 maskBufferUp, TrBufferUp, false))
                            rowInScriptBufferUp = -1;

                    if (rowInScriptBufferUp >= 0)
                    {
                        //+++ BufferDown:: read absBuffer, thicknessBuffer, TrBuffer, TrBufferSigma : Down
                        if (rowInScriptBufferDown >= 0)
                            if (!singleDanFactorTransThickness(wScript, scriptColList, rowInScriptBufferDown,
                                                               absBufferDown, TrBufferDown, TrBufferSigmaDown,
                                                               thicknessBufferDown))
                                rowInScriptBufferDown = -1;

                        //+++ BufferDown:: read Buffer/BufferErr matrixes : Down
                        if (rowInScriptBufferDown >= 0)
                            if (!singleDanSimplified(wScript, scriptColList, rowInScriptBufferDown, BufferDown,
                                                     BufferErrDown, maskBufferDown, TrBufferDown, false))
                                rowInScriptBufferDown = -1;

                        if (rowInScriptBufferDown < 0)
                        {
                            gsl_matrix_free(BufferUp);
                            gsl_matrix_free(BufferErrUp);
                            gsl_matrix_free(maskBufferUp);
                            rowInScriptBufferUp = -1;
                        }
                    }

                    if (rowInScriptBufferUp >= 0)
                    {
                        if (absBufferUp != absUp)
                            std::cout
                                << "!!! : abs.Buffer.Up != abs.Sample.Up: we use abs.Buffer.Up = abs.Sample.Up \n";
                        if (absBufferDown != absDown)
                            std::cout << "!!! : abs.Buffer.Down != abs.Sample.Down: we use abs.Buffer.Down ="
                                         "abs.Sample.Down \n";
                        if (thicknessBufferUp != thicknessUp)
                            std::cout << "!!! : thickness.Buffer.Up != thickness.Sample.Up: we use thickness.Buffer.Up "
                                         "= thickness.Sample.Up \n";
                        if (thicknessBufferDown != thicknessDown)
                            std::cout << "!!! : thickness.Buffer.Up != thickness.Sample.Up: we use thickness.Buffer.Up "
                                         "= thickness.Sample.Up \n";

                        gsl_matrix_mul_elements(maskBufferDown, maskBufferUp);

                        // TrDown -> Tr-; TrUp -> Tr+
                        double TRminBuffer =
                            (P * Pf + 1) / (P * (Pf + 1)) * TrBufferDown + (P - 1) / (P * (Pf + 1)) * TrBufferUp;

                        double TRminBufferSigma2 = 0;
                        TRminBufferSigma2 += (P * Pf + 1) * (P * Pf + 1) * TrBufferSigmaDown * TrBufferSigmaDown;
                        TRminBufferSigma2 += (P - 1) * (P - 1) * TrBufferSigmaUp * TrBufferSigmaUp;
                        TRminBufferSigma2 /= (P * (Pf + 1) * P * (Pf + 1));

                        double TRplusBuffer =
                            (P * Pf - 1) / (P * (Pf + 1)) * TrBufferDown + (P + 1) / (P * (Pf + 1)) * TrBufferUp;

                        double TRplusBufferSigma2 = 0;
                        TRplusBufferSigma2 += (P * Pf - 1) * (P * Pf - 1) * TrBufferSigmaDown * TrBufferSigmaDown;
                        TRplusBufferSigma2 += (P + 1) * (P + 1) * TrBufferSigmaUp * TrBufferSigmaUp;
                        TRplusBufferSigma2 /= (P * (Pf + 1) * P * (Pf + 1));

                        // BufferDown -> I-; BufferUp -> I+

                        gsl_matrix *IminBuffer = gsl_matrix_calloc(MD, MD);
                        gsl_matrix_memcpy(IminBuffer, BufferDown);
                        gsl_matrix_scale(IminBuffer, (P * Pf + 1.0) / P / (Pf + 1) / TRminBuffer);

                        gsl_matrix *IminErrBuffer = gsl_matrix_calloc(MD, MD);
                        gsl_matrix_memcpy(IminErrBuffer, BufferErrDown);
                        gsl_matrix_mul_elements(IminErrBuffer, IminBuffer);
                        gsl_matrix_mul_elements(IminErrBuffer, IminBuffer);

                        gsl_matrix_memcpy(mTemp, BufferUp);
                        gsl_matrix_scale(mTemp, (P - 1) / P / (Pf + 1) / TRminBuffer);

                        gsl_matrix_memcpy(mTempErr, BufferErrUp);
                        gsl_matrix_mul_elements(mTempErr, mTemp);
                        gsl_matrix_mul_elements(mTempErr, mTemp);

                        gsl_matrix_add(IminBuffer, mTemp);
                        gsl_matrix_add(IminErrBuffer, mTempErr);
                        gsl_matrix_div_elements_with_mask(IminErrBuffer, IminBuffer, maskBufferDown, MD);
                        gsl_matrix_div_elements_with_mask(IminErrBuffer, IminBuffer, maskBufferDown, MD);

                        gsl_matrix_add_constant(IminErrBuffer, TRminBufferSigma2);

                        gsl_matrix *IplusBuffer = gsl_matrix_calloc(MD, MD);
                        gsl_matrix_memcpy(IplusBuffer, BufferUp);
                        gsl_matrix_scale(IplusBuffer, (P + 1.0) / P / (Pf + 1) / TRplusBuffer);

                        gsl_matrix *IplusErrBuffer = gsl_matrix_calloc(MD, MD);
                        gsl_matrix_memcpy(IplusErrBuffer, BufferErrUp);
                        gsl_matrix_mul_elements(IplusErrBuffer, IplusBuffer);
                        gsl_matrix_mul_elements(IplusErrBuffer, IplusBuffer);

                        gsl_matrix_memcpy(mTemp, BufferDown);
                        gsl_matrix_scale(mTemp, (P * Pf - 1.0) / P / (Pf + 1) / TRplusBuffer);

                        gsl_matrix_memcpy(mTempErr, BufferErrDown);
                        gsl_matrix_mul_elements(mTempErr, mTemp);
                        gsl_matrix_mul_elements(mTempErr, mTemp);

                        gsl_matrix_add(IplusBuffer, mTemp);

                        gsl_matrix_add(IplusErrBuffer, mTempErr);
                        gsl_matrix_div_elements_with_mask(IplusErrBuffer, IplusBuffer, maskBufferDown, MD);
                        gsl_matrix_div_elements_with_mask(IplusErrBuffer, IplusBuffer, maskBufferDown, MD);

                        //+++ scale buffer with volume faction
                        gsl_matrix_scale(IminBuffer, bufferVolumeFraction);
                        gsl_matrix_scale(IplusBuffer, bufferVolumeFraction);

                        gsl_matrix_add_constant(IplusErrBuffer, TRplusBufferSigma2);

                        //+++ SampleErr(i,j) = SampleErr(i,j) + BufferErr(i,j)
                        gsl_matrix_add(IminErr, IminErrBuffer);
                        gsl_matrix_add(IplusErr, IplusErrBuffer);

                        //+++ Sample(i,j) = Sample(i,j) - Buffer(i,j)
                        gsl_matrix_sub(Imin, IminBuffer);
                        gsl_matrix_sub(Iplus, IplusBuffer);

                        //+++ maskSample and maskBuffer sinchronization
                        gsl_matrix_mul_elements(maskDown, maskBufferDown);

                        // free memory: Buffer / BufferErr /MaskBuffer: Down
                        gsl_matrix_free(BufferDown);
                        gsl_matrix_free(BufferErrDown);
                        gsl_matrix_free(maskBufferDown);
                        // free memory: Buffer / BufferErr /MaskBuffer: Up
                        gsl_matrix_free(BufferUp);
                        gsl_matrix_free(BufferErrUp);
                        gsl_matrix_free(maskBufferUp);
                        // free memory: IBuffer / IBufferErr : min / plus
                        gsl_matrix_free(IminBuffer);
                        gsl_matrix_free(IplusBuffer);
                        gsl_matrix_free(IminErrBuffer);
                        gsl_matrix_free(IplusErrBuffer);
                    }

                    //+++
                    // Empty Cell Actions
                    //+++

                    if (rowInScriptBufferUp < 0)
                    {
                        gsl_matrix *ECUp, *ECErrUp, *maskECUp;
                        gsl_matrix *ECDown, *ECErrDown, *maskECDown;

                        //+++ read EC/ECErr matrixes : Up
                        if (singleDanSimplified(wScript, scriptColList, rowInScriptUp, ECUp, ECErrUp, maskECUp, 1.0,
                                                true))
                        {
                            if (!singleDanSimplified(wScript, scriptColList, rowInScriptDown, ECDown, ECErrDown,
                                                     maskECDown, 1.0, true))
                            {
                                // free memory: EC / ECErr /MaskEC: Up
                                gsl_matrix_free(ECUp);
                                gsl_matrix_free(ECErrUp);
                                gsl_matrix_free(maskECUp);
                            }
                            else
                            {
                                // ECDown -> I-; ECUp -> I+
                                gsl_matrix *IminEC = gsl_matrix_calloc(MD, MD);
                                gsl_matrix_memcpy(IminEC, ECDown);
                                gsl_matrix_scale(IminEC, (P * Pf + 1.0) / P / (Pf + 1));

                                gsl_matrix *IminErrEC = gsl_matrix_calloc(MD, MD);
                                gsl_matrix_memcpy(IminErrEC, ECErrDown);
                                gsl_matrix_mul_elements(IminErrEC, IminEC);
                                gsl_matrix_mul_elements(IminErrEC, IminEC);

                                gsl_matrix_memcpy(mTemp, ECUp);
                                gsl_matrix_scale(mTemp, (P - 1) / P / (Pf + 1));

                                gsl_matrix_memcpy(mTempErr, ECErrUp);
                                gsl_matrix_mul_elements(mTempErr, mTemp);
                                gsl_matrix_mul_elements(mTempErr, mTemp);

                                gsl_matrix_add(IminEC, mTemp);
                                gsl_matrix_add(IminErrEC, mTempErr);
                                gsl_matrix_div_elements_with_mask(IminErrEC, IminEC, maskDown, MD);
                                gsl_matrix_div_elements_with_mask(IminErrEC, IminEC, maskDown, MD);

                                gsl_matrix *IplusEC = gsl_matrix_calloc(MD, MD);
                                gsl_matrix_memcpy(IplusEC, ECUp);
                                gsl_matrix_scale(IplusEC, (P + 1.0) / P / (Pf + 1));

                                gsl_matrix *IplusErrEC = gsl_matrix_calloc(MD, MD);
                                gsl_matrix_memcpy(IplusErrEC, ECErrUp);
                                gsl_matrix_mul_elements(IplusErrEC, IplusEC);
                                gsl_matrix_mul_elements(IplusErrEC, IplusEC);

                                gsl_matrix_memcpy(mTemp, ECDown);
                                gsl_matrix_scale(mTemp, (P * Pf - 1.0) / P / (Pf + 1));

                                gsl_matrix_memcpy(mTempErr, ECErrDown);
                                gsl_matrix_mul_elements(mTempErr, mTemp);
                                gsl_matrix_mul_elements(mTempErr, mTemp);

                                gsl_matrix_add(IplusEC, mTemp);

                                gsl_matrix_add(IplusErrEC, mTempErr);
                                gsl_matrix_div_elements_with_mask(IplusErrEC, IplusEC, maskDown, MD);
                                gsl_matrix_div_elements_with_mask(IplusErrEC, IplusEC, maskDown, MD);

                                //+++ SampleErr(i,j) = SampleErr(i,j) + ECErr(i,j)
                                gsl_matrix_add(IminErr, IminErrEC);
                                gsl_matrix_add(IplusErr, IplusErrEC);

                                //+++ Sample(i,j) = Sample(i,j) - EC(i,j)
                                gsl_matrix_sub(Imin, IminEC);
                                gsl_matrix_sub(Iplus, IplusEC);

                                // free memory: EC / ECErr /MaskEC: Down
                                gsl_matrix_free(ECDown);
                                gsl_matrix_free(ECErrDown);
                                gsl_matrix_free(maskECDown);
                                // free memory: EC / ECErr /MaskEC: Up
                                gsl_matrix_free(ECUp);
                                gsl_matrix_free(ECErrUp);
                                gsl_matrix_free(maskECUp);
                                // free memory: IEC / IECErr : min / plus
                                gsl_matrix_free(IminEC);
                                gsl_matrix_free(IplusEC);
                                gsl_matrix_free(IminErrEC);
                                gsl_matrix_free(IplusErrEC);
                            }
                        }
                    }

                    gsl_matrix_scale(Imin, absDown / thicknessDown);
                    gsl_matrix_scale(Iplus, absUp / thicknessUp);

                    double time = static_cast<double>(dt.elapsed() - pre_dt) / 1000.0;
                    pre_dt = dt.elapsed();

                    QString labelUp = singleDanMultiButton(wScript, scriptColList, rowInScriptUp, button,
                                                           dataSuffix + "-Up", Iplus, IplusErr, maskDown, -1.0);
                    QString labelDown = singleDanMultiButton(wScript, scriptColList, rowInScriptDown, button,
                                                             dataSuffix + "-Down", Imin, IminErr, maskDown, time);

                    gsl_matrix_free(mTemp);
                    gsl_matrix_free(mTempErr);
                    gsl_matrix_free(maskUp);
                    gsl_matrix_free(maskDown);
                }
                else if (numberCommonFiles == 4)
                {
                    //+++
                    // PNx4 mode
                    //+++

                    //+++ sample rows in script table
                    int rowInScriptOut = linesInScript[iSample][iConf][0];
                    int rowInScriptUp = linesInScript[iSample][iConf][1];
                    int rowInScriptDown = linesInScript[iSample][iConf][2];
                    int rowInScriptUpUp = linesInScript[iSample][iConf][3];
                    int rowInScriptUpDown = linesInScript[iSample][iConf][4];
                    int rowInScriptDownDown = linesInScript[iSample][iConf][5];
                    int rowInScriptDownUp = linesInScript[iSample][iConf][6];

                    if (rowInScriptUpUp < 0 || rowInScriptUpDown < 0 || rowInScriptDownDown < 0 ||
                        rowInScriptDownUp < 0)
                        continue;

                    //+++ buffer rows in script table
                    int rowInScriptBufferOut = rowInPolBuffer;
                    int rowInScriptBufferUp = rowInPolBuffer;
                    int rowInScriptBufferDown = rowInPolBuffer;
                    int rowInScriptBufferUpUp = rowInPolBuffer;
                    int rowInScriptBufferUpDown = rowInPolBuffer;
                    int rowInScriptBufferDownDown = rowInPolBuffer;
                    int rowInScriptBufferDownUp = rowInPolBuffer;

                    if (rowInScriptBufferUpUp >= 0 && rowInScriptBufferUpDown >= 0 && rowInScriptBufferDownDown >= 0 &&
                        rowInScriptBufferDownUp)
                        for (int jSample = 0; jSample < allUniqueSamples.count(); jSample++)
                            if (allUniqueSamples[jSample] == bufferName)
                            {
                                rowInScriptBufferUpUp = linesInScript[jSample][iConf][3];
                                if (rowInScriptBufferUpUp < 0 || rowInScriptBufferUpUp >= wScript->numRows())
                                    rowInScriptBufferUpUp = -1;

                                rowInScriptBufferUpDown = linesInScript[jSample][iConf][4];
                                if (rowInScriptBufferUpDown < 0 || rowInScriptBufferUpDown >= wScript->numRows())
                                    rowInScriptBufferUpDown = -1;

                                rowInScriptBufferDownDown = linesInScript[jSample][iConf][5];
                                if (rowInScriptBufferDownDown < 0 || rowInScriptBufferDownDown >= wScript->numRows())
                                    rowInScriptBufferDownDown = -1;

                                rowInScriptBufferDownUp = linesInScript[jSample][iConf][6];
                                if (rowInScriptBufferDownUp < 0 || rowInScriptBufferDownUp >= wScript->numRows())
                                    rowInScriptBufferDownUp = -1;
                                break;
                            }

                    if (rowInScriptBufferUpUp < 0 || rowInScriptBufferUpDown < 0 || rowInScriptBufferDownDown < 0 ||
                        rowInScriptBufferDownUp < 0)
                    {
                        rowInScriptBufferUpUp = -1;
                        rowInScriptBufferUpDown = -1;
                        rowInScriptBufferDownDown = -1;
                        rowInScriptBufferDownUp = -1;
                    }
                    //--- buffer rows

                    gsl_matrix *SampleUpUp, *SampleUpDown, *SampleDownDown, *SampleDownUp;
                    gsl_matrix *SampleUpUpErr, *SampleUpDownErr, *SampleDownDownErr, *SampleDownUpErr;
                    gsl_matrix *maskUpUp, *maskUpDown, *maskDownDown, *maskDownUp;
                    gsl_matrix *EC, *ECErr, *maskEC;

                    double absUpUp, absUpDown, absDownDown, absDownUp;
                    double TrUpUp, TrUpDown, TrDownDown, TrDownUp;
                    double SigmaTrUpUp, SigmaTrUpDown, SigmaTrDownDown, SigmaTrDownUp;
                    double thicknessUpUp, thicknessUpDown, thicknessDownDown, thicknessDownUp;
                    double analyzerTrUpUp, analyzerTrUpDown, analyzerTrDownDown, analyzerTrDownUp;
                    double analyzerEffUpUp, analyzerEffUpDown, analyzerEffDownDown, analyzerEffDownUp;
                    bool status = true;

                    //+++ SampleUpUp:: read absUpUp, thicknessUpUp, TrUpUp, SigmaTrUpUp
                    if (!singleDanFactorTransThickness(wScript, scriptColList, rowInScriptUpUp, absUpUp, TrUpUp,
                                                       SigmaTrUpUp, thicknessUpUp))
                        continue;
                    //+++ SampleUpUp:: read analyzerTrUpUp, analyzerEffUpUp
                    if (!singleDanAnalyzerStatus(wScript, scriptColList, rowInScriptUpUp, analyzerTrUpUp,
                                                 analyzerEffUpUp))
                        continue;
                    //+++ SampleUpUp:: read SampleUpUp, SampleUpUpErr, maskUpUp
                    if (!singleDanSimplified(wScript, scriptColList, rowInScriptUpUp, SampleUpUp, SampleUpUpErr,
                                             maskUpUp, TrUpUp, false))
                        continue;

                    //+++ SampleUpDown:: read absUpDown, thicknessUpDown, TrUpDown, SigmaTrUpDown
                    status &= singleDanFactorTransThickness(wScript, scriptColList, rowInScriptUpDown, absUpDown,
                                                            TrUpDown, SigmaTrUpDown, thicknessUpDown);
                    //+++ SampleUpDown:: read analyzerTrUpDown, analyzerEffUpDown
                    status &= singleDanAnalyzerStatus(wScript, scriptColList, rowInScriptUpDown, analyzerTrUpDown,
                                                      analyzerEffUpDown);
                    //+++ SampleUpDown:: read SampleUpDown, SampleUpDownErr, maskUpDown
                    if (!status || !singleDanSimplified(wScript, scriptColList, rowInScriptUpDown, SampleUpDown,
                                                        SampleUpDownErr, maskUpDown, TrUpDown, false))
                    {
                        gsl_matrix_free(SampleUpUp);
                        gsl_matrix_free(SampleUpUpErr);
                        gsl_matrix_free(maskUpUp);
                        continue;
                    }

                    //+++ SampleDownDown:: read absDownDown, thicknessDownDown, TrDownDown, SigmaTrDownDown
                    status &= singleDanFactorTransThickness(wScript, scriptColList, rowInScriptDownDown, absDownDown,
                                                            TrDownDown, SigmaTrDownDown, thicknessDownDown);
                    //+++ SampleDownDown:: read analyzerTrDownDown, analyzerEffDownDown
                    status &= singleDanAnalyzerStatus(wScript, scriptColList, rowInScriptDownDown, analyzerTrDownDown,
                                                      analyzerEffDownDown);
                    //+++ SampleDownDown:: read SampleDownDown, SampleDownDownErr, maskDownDown
                    if (!status || !singleDanSimplified(wScript, scriptColList, rowInScriptDownDown, SampleDownDown,
                                                        SampleDownDownErr, maskDownDown, TrDownDown, false))
                    {
                        gsl_matrix_free(SampleUpUp);
                        gsl_matrix_free(SampleUpUpErr);
                        gsl_matrix_free(maskUpUp);
                        gsl_matrix_free(SampleUpDown);
                        gsl_matrix_free(SampleUpDownErr);
                        gsl_matrix_free(maskUpDown);
                        continue;
                    }

                    //+++ SampleDownUp:: read absDownUp, thicknessDownUp, TrDownUp, SigmaTrDownUp
                    status &= singleDanFactorTransThickness(wScript, scriptColList, rowInScriptDownUp, absDownUp,
                                                            TrDownUp, SigmaTrDownUp, thicknessDownUp);
                    //+++ SampleDownUp:: read analyzerTrDownUp, analyzerEffDownUp
                    status &= singleDanAnalyzerStatus(wScript, scriptColList, rowInScriptDownUp, analyzerTrDownUp,
                                                      analyzerEffDownUp);
                    //+++ SampleDownUp:: read SampleDownUp, SampleDownUpErr, maskDownUp
                    if (!status || !singleDanSimplified(wScript, scriptColList, rowInScriptDownUp, SampleDownUp,
                                                        SampleDownUpErr, maskDownUp, TrDownUp, false))
                    {
                        gsl_matrix_free(SampleUpUp);
                        gsl_matrix_free(SampleUpUpErr);
                        gsl_matrix_free(maskUpUp);
                        gsl_matrix_free(SampleUpDown);
                        gsl_matrix_free(SampleUpDownErr);
                        gsl_matrix_free(maskUpDown);
                        gsl_matrix_free(SampleDownDown);
                        gsl_matrix_free(SampleDownDownErr);
                        gsl_matrix_free(maskDownDown);
                        continue;
                    }

                    //+++ current wave length
                    double lambda = wScript->text(rowInScriptDownDown, indexLam).toDouble();

                    //+++ polarizer: current polarization and flipper efficiency
                    double P = polarizationSelector->getValue(lambda, wScript->text(rowInScriptDownDown, indexSample));
                    double f =
                        polFlipperEfficiencySelector->getValue(lambda, wScript->text(rowInScriptDownDown, indexSample));
                    double Pf = 2 * f - 1.0;
                    double P0 = (1 + P) / 2;
                    double fxP = f * P - (1 - f) * (1 - P);

                    // TrDown -> Tr-; TrUp -> Tr+
                    double TRmin = (P * Pf + 1) / (P * (Pf + 1)) * TrDownDown + (P - 1) / (P * (Pf + 1)) * TrUpUp;

                    double TRminSigma2 = 0;
                    TRminSigma2 += (P * Pf + 1) * (P * Pf + 1) * SigmaTrDownDown * SigmaTrDownDown;
                    TRminSigma2 += (P - 1) * (P - 1) * SigmaTrUpUp * SigmaTrUpUp;
                    TRminSigma2 /= (P * (Pf + 1) * P * (Pf + 1));

                    double TRplus = (P * Pf - 1) / (P * (Pf + 1)) * TrDownDown + (P + 1) / (P * (Pf + 1)) * TrUpUp;

                    double TRplusSigma2 = 0;
                    TRplusSigma2 += (P * Pf - 1) * (P * Pf - 1) * SigmaTrDownDown * SigmaTrDownDown;
                    TRplusSigma2 += (P + 1) * (P + 1) * SigmaTrUpUp * SigmaTrUpUp;
                    TRplusSigma2 /= (P * (Pf + 1) * P * (Pf + 1));

                    double A1 = analyzerEffDownDown;
                    double A2 = analyzerEffUpDown;
                    double A3 = analyzerEffDownUp;
                    double A4 = analyzerEffUpUp;

                    double thickness = (thicknessDownDown + thicknessDownUp + thicknessUpUp + thicknessUpDown) / 4.0;

                    gsl_matrix_scale(SampleDownDown, absDownDown / thickness / analyzerTrDownDown);
                    gsl_matrix_scale(SampleDownUp, absDownUp / thickness / analyzerTrDownUp);
                    gsl_matrix_scale(SampleUpUp, absUpUp / thickness / analyzerTrUpUp);
                    gsl_matrix_scale(SampleUpDown, absUpDown / thickness / analyzerTrUpDown);

                    gsl_matrix *Smm = gsl_matrix_calloc(MD, MD);

                    gsl_matrix_addscaled(Smm, SampleDownDown, (fxP - (P0 - 1.0)) * (A2 + A4) * (A3 + 1), MD);
                    gsl_matrix_addscaled(Smm, SampleDownUp, (P0 - 1 - fxP) * (A2 + A4) * (1 - A1), MD);
                    gsl_matrix_addscaled(Smm, SampleUpDown, (P0 - 1) * (A1 + A3) * (A4 + 1), MD);
                    gsl_matrix_addscaled(Smm, SampleUpUp, (P0 - 1) * (A1 + A3) * (A2 - 1), MD);

                    gsl_matrix_scale(Smm, 1.0 / (2 * fxP * (A1 + A3) * (A2 + A4)) / TRmin);

                    gsl_matrix *Smp = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_addscaled(Smp, SampleDownDown, (fxP - P0 + 1) * (A2 + A4) * (A3 - 1), MD);
                    gsl_matrix_addscaled(Smp, SampleDownUp, (fxP - P0 + 1) * (A2 + A4) * (1 + A1), MD);
                    gsl_matrix_addscaled(Smp, SampleUpDown, (P0 - 1) * (A1 + A3) * (A4 - 1), MD);
                    gsl_matrix_addscaled(Smp, SampleUpUp, (P0 - 1) * (A1 + A3) * (A2 + 1), MD);

                    gsl_matrix_scale(Smp, 1.0 / (2 * fxP * (A1 + A3) * (A2 + A4)) / TRmin);

                    gsl_matrix *Spp = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_addscaled(Spp, SampleDownDown, (fxP - P0) * (A2 + A4) * (A3 - 1), MD);
                    gsl_matrix_addscaled(Spp, SampleDownUp, (fxP - P0) * (A2 + A4) * (1 + A1), MD);
                    gsl_matrix_addscaled(Spp, SampleUpDown, P0 * (A1 + A3) * (A4 - 1), MD);
                    gsl_matrix_addscaled(Spp, SampleUpUp, P0 * (A1 + A3) * (A2 + 1), MD);

                    gsl_matrix_scale(Spp, 1.0 / (2 * fxP * (A1 + A3) * (A2 + A4)) / TRplus);

                    gsl_matrix *Spm = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_addscaled(Spm, SampleDownDown, (fxP - P0) * (A2 + A4) * (A3 + 1), MD);
                    gsl_matrix_addscaled(Spm, SampleDownUp, (P0 - fxP) * (A2 + A4) * (1 - A1), MD);
                    gsl_matrix_addscaled(Spm, SampleUpDown, P0 * (A1 + A3) * (A4 + 1), MD);
                    gsl_matrix_addscaled(Spm, SampleUpUp, P0 * (A1 + A3) * (A2 - 1), MD);

                    gsl_matrix_scale(Spm, 1.0 / (2 * fxP * (A1 + A3) * (A2 + A4)) / TRplus);

                    gsl_matrix_free(SampleUpUp);
                    gsl_matrix_free(SampleUpDown);
                    gsl_matrix_free(SampleDownDown);
                    gsl_matrix_free(SampleDownUp);

                    //+++
                    // Buffer Actions
                    //+++

                    double absBufferUpUp, TrBufferUpUp, TrBufferSigmaUpUp, thicknessBufferUpUp;
                    double absBufferUpDown, TrBufferUpDown, TrBufferSigmaUpDown, thicknessBufferUpDown;
                    double absBufferDownUp, TrBufferDownUp, TrBufferSigmaDownUp, thicknessBufferDownUp;
                    double absBufferDownDown, TrBufferDownDown, TrBufferSigmaDownDown, thicknessBufferDownDown;
                    double analyzerTrBufferUpUp, analyzerTrBufferUpDown;
                    double analyzerTrBufferDownDown, analyzerTrBufferDownUp;
                    double analyzerEffBufferUpUp, analyzerEffBufferUpDown;
                    double analyzerEffBufferDownDown, analyzerEffBufferDownUp;

                    gsl_matrix *BufferUpUp, *BufferErrUpUp, *maskBufferUpUp;
                    gsl_matrix *BufferUpDown, *BufferErrUpDown, *maskBufferUpDown;
                    gsl_matrix *BufferDownUp, *BufferErrDownUp, *maskBufferDownUp;
                    gsl_matrix *BufferDownDown, *BufferErrDownDown, *maskBufferDownDown;

                    if (rowInScriptBufferUpUp >= 0)
                    {
                        //+++ BufferUpUp:: read absBuffer, thicknessBuffer, TrBuffer, TrBufferSigma : UpUp
                        if (!singleDanFactorTransThickness(wScript, scriptColList, rowInScriptBufferUpUp, absBufferUpUp,
                                                           TrBufferUpUp, TrBufferSigmaUpUp, thicknessBufferUpUp))
                            rowInScriptBufferUpUp = -1;

                        //+++ BufferUpUp:: read analyzerTrBufferUpUp, analyzerEffBufferUpUp
                        if (rowInScriptBufferUpUp >= 0)
                            if (!singleDanAnalyzerStatus(wScript, scriptColList, rowInScriptBufferUpUp,
                                                         analyzerTrBufferUpUp, analyzerEffBufferUpUp))
                                rowInScriptBufferUpUp = -1;

                        //+++ BufferUpUp:: read Buffer/BufferErr matrixes : UpUp
                        if (rowInScriptBufferUpUp >= 0)
                            if (!singleDanSimplified(wScript, scriptColList, rowInScriptBufferUpUp, BufferUpUp,
                                                     BufferErrUpUp, maskBufferUpUp, TrBufferUpUp, false))
                                rowInScriptBufferUpUp = -1;

                        if (rowInScriptBufferUpUp >= 0)
                        {
                            //+++ BufferUpDown:: read absBuffer, thicknessBuffer, TrBuffer, TrBufferSigma : UpDown
                            if (!singleDanFactorTransThickness(wScript, scriptColList, rowInScriptBufferUpDown,
                                                               absBufferUpDown, TrBufferUpDown, TrBufferSigmaUpDown,
                                                               thicknessBufferUpDown))
                                rowInScriptBufferUpUp = -1;

                            //+++ BufferUpDown:: read analyzerTrBufferUpDown, analyzerEffBufferUpDown
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanAnalyzerStatus(wScript, scriptColList, rowInScriptBufferUpDown,
                                                             analyzerTrBufferUpDown, analyzerEffBufferUpDown))
                                    rowInScriptBufferUpUp = -1;

                            //+++ read Buffer/BufferErr matrixes : UpDown
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanSimplified(wScript, scriptColList, rowInScriptBufferUpDown, BufferUpDown,
                                                         BufferErrUpDown, maskBufferUpDown, TrBufferUpDown, false))
                                    rowInScriptBufferUpUp = -1;

                            if (rowInScriptBufferUpUp < 0)
                            {
                                gsl_matrix_free(BufferUpUp);
                                gsl_matrix_free(BufferErrUpUp);
                                gsl_matrix_free(maskBufferUpUp);
                                rowInScriptBufferUpDown = -1;
                            }
                        }

                        if (rowInScriptBufferUpUp >= 0)
                        {
                            //+++ BufferDownDown:: read absBuffer, thicknessBuffer, TrBuffer, TrBufferSigma : DownDown
                            if (!singleDanFactorTransThickness(wScript, scriptColList, rowInScriptBufferDownDown,
                                                               absBufferDownDown, TrBufferDownDown,
                                                               TrBufferSigmaDownDown, thicknessBufferDownDown))
                                rowInScriptBufferUpUp = -1;

                            //+++ BufferDownDown:: read analyzerTrBufferDownDown, analyzerEffBufferDownDown
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanAnalyzerStatus(wScript, scriptColList, rowInScriptBufferDownDown,
                                                             analyzerTrBufferDownDown, analyzerEffBufferDownDown))
                                    rowInScriptBufferUpUp = -1;

                            //+++ BufferDownDown:: read Buffer/BufferErr matrixes : DownDown
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanSimplified(wScript, scriptColList, rowInScriptBufferDownDown,
                                                         BufferDownDown, BufferErrDownDown, maskBufferDownDown,
                                                         TrBufferDownDown, false))
                                    rowInScriptBufferUpUp = -1;

                            if (rowInScriptBufferUpUp < 0)
                            {
                                gsl_matrix_free(BufferUpUp);
                                gsl_matrix_free(BufferErrUpUp);
                                gsl_matrix_free(maskBufferUpUp);
                                rowInScriptBufferUpDown = -1;

                                gsl_matrix_free(BufferUpDown);
                                gsl_matrix_free(BufferErrUpDown);
                                gsl_matrix_free(maskBufferUpDown);
                                rowInScriptBufferDownDown = -1;
                            }
                        }

                        if (rowInScriptBufferUpUp >= 0)
                        {
                            //+++ BufferDownUp:: read absBuffer, thicknessBuffer, TrBuffer, TrBufferSigma : DownUp
                            if (!singleDanFactorTransThickness(wScript, scriptColList, rowInScriptBufferDownUp,
                                                               absBufferDownUp, TrBufferDownUp, TrBufferSigmaDownUp,
                                                               thicknessBufferDownUp))
                                rowInScriptBufferUpUp = -1;

                            //+++ BufferDownUp:: read analyzerTrBufferDownUp, analyzerEffBufferDownUp
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanAnalyzerStatus(wScript, scriptColList, rowInScriptBufferDownUp,
                                                             analyzerTrBufferDownUp, analyzerEffBufferDownUp))
                                    rowInScriptBufferUpUp = -1;

                            //+++ BufferDownUp:: read Buffer/BufferErr matrixes : DownUp
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanSimplified(wScript, scriptColList, rowInScriptBufferDownUp, BufferDownUp,
                                                         BufferErrDownUp, maskBufferDownUp, TrBufferDownUp, false))
                                    rowInScriptBufferUpUp = -1;

                            if (rowInScriptBufferUpUp < 0)
                            {
                                gsl_matrix_free(BufferUpUp);
                                gsl_matrix_free(BufferErrUpUp);
                                gsl_matrix_free(maskBufferUpUp);
                                rowInScriptBufferUpDown = -1;

                                gsl_matrix_free(BufferUpDown);
                                gsl_matrix_free(BufferErrUpDown);
                                gsl_matrix_free(maskBufferUpDown);
                                rowInScriptBufferDownDown = -1;

                                gsl_matrix_free(BufferDownDown);
                                gsl_matrix_free(BufferErrDownDown);
                                gsl_matrix_free(maskBufferDownDown);
                                rowInScriptBufferDownUp = -1;
                            }
                        }

                        if (rowInScriptBufferUpUp >= 0)
                        {
                            // TrDown -> Tr-; TrUp -> Tr+
                            double TRminBuffer = (P * Pf + 1) / (P * (Pf + 1)) * TrBufferDownDown +
                                                 (P - 1) / (P * (Pf + 1)) * TrBufferUpUp;

                            double TRminSigma2Buffer = 0;
                            TRminSigma2Buffer +=
                                (P * Pf + 1) * (P * Pf + 1) * TrBufferSigmaDownDown * TrBufferSigmaDownDown;
                            TRminSigma2Buffer += (P - 1) * (P - 1) * TrBufferSigmaUpUp * TrBufferSigmaUpUp;
                            TRminSigma2Buffer /= (P * (Pf + 1) * P * (Pf + 1));

                            double TRplusBuffer = (P * Pf - 1) / (P * (Pf + 1)) * TrBufferDownDown +
                                                  (P + 1) / (P * (Pf + 1)) * TrBufferUpUp;

                            double TRplusSigma2Buffer = 0;
                            TRplusSigma2Buffer +=
                                (P * Pf - 1) * (P * Pf - 1) * TrBufferSigmaDownDown * TrBufferSigmaDownDown;
                            TRplusSigma2Buffer += (P + 1) * (P + 1) * TrBufferSigmaUpUp * TrBufferSigmaUpUp;
                            TRplusSigma2Buffer /= (P * (Pf + 1) * P * (Pf + 1));

                            double A1B = analyzerEffBufferDownDown;
                            double A2B = analyzerEffBufferUpDown;
                            double A3B = analyzerEffBufferDownUp;
                            double A4B = analyzerEffBufferUpUp;

                            gsl_matrix_scale(BufferDownDown, absDownDown / thickness / analyzerTrBufferDownDown);
                            gsl_matrix_scale(BufferDownUp, absDownUp / thickness / analyzerTrBufferDownUp);
                            gsl_matrix_scale(BufferUpUp, absUpUp / thickness / analyzerTrBufferUpUp);
                            gsl_matrix_scale(BufferUpDown, absUpDown / thickness / analyzerTrBufferUpDown);

                            gsl_matrix *Buffermm = gsl_matrix_calloc(MD, MD);

                            gsl_matrix_addscaled(Buffermm, BufferDownDown, (fxP - (P0 - 1.0)) * (A2B + A4B) * (A3B + 1),
                                                 MD);
                            gsl_matrix_addscaled(Buffermm, BufferDownUp, (P0 - 1 - fxP) * (A2B + A4B) * (1 - A1B), MD);
                            gsl_matrix_addscaled(Buffermm, BufferUpDown, (P0 - 1) * (A1B + A3B) * (A4B + 1), MD);
                            gsl_matrix_addscaled(Buffermm, BufferUpUp, (P0 - 1) * (A1B + A3B) * (A2B - 1), MD);

                            gsl_matrix_scale(Buffermm, bufferVolumeFraction / (2 * fxP * (A1B + A3B) * (A2B + A4B)) /
                                                           TRminBuffer);

                            gsl_matrix *Buffermp = gsl_matrix_calloc(MD, MD);
                            gsl_matrix_addscaled(Buffermp, BufferDownDown, (fxP - P0 + 1) * (A2B + A4B) * (A3B - 1),
                                                 MD);
                            gsl_matrix_addscaled(Buffermp, BufferDownUp, (fxP - P0 + 1) * (A2B + A4B) * (1 + A1B), MD);
                            gsl_matrix_addscaled(Buffermp, BufferUpDown, (P0 - 1) * (A1B + A3B) * (A4B - 1), MD);
                            gsl_matrix_addscaled(Buffermp, BufferUpUp, (P0 - 1) * (A1B + A3B) * (A2B + 1), MD);

                            gsl_matrix_scale(Buffermp, bufferVolumeFraction / (2 * fxP * (A1B + A3B) * (A2B + A4B)) /
                                                           TRminBuffer);

                            gsl_matrix *Bufferpp = gsl_matrix_calloc(MD, MD);
                            gsl_matrix_addscaled(Bufferpp, BufferDownDown, (fxP - P0) * (A2B + A4B) * (A3B - 1), MD);
                            gsl_matrix_addscaled(Bufferpp, BufferDownUp, (fxP - P0) * (A2B + A4B) * (1 + A1B), MD);
                            gsl_matrix_addscaled(Bufferpp, BufferUpDown, P0 * (A1B + A3B) * (A4B - 1), MD);
                            gsl_matrix_addscaled(Bufferpp, BufferUpUp, P0 * (A1B + A3B) * (A2B + 1), MD);

                            gsl_matrix_scale(Bufferpp, bufferVolumeFraction / (2 * fxP * (A1B + A3B) * (A2B + A4B)) /
                                                           TRplusBuffer);

                            gsl_matrix *Bufferpm = gsl_matrix_calloc(MD, MD);
                            gsl_matrix_addscaled(Bufferpm, BufferDownDown, (fxP - P0) * (A2B + A4B) * (A3B + 1), MD);
                            gsl_matrix_addscaled(Bufferpm, BufferDownUp, (P0 - fxP) * (A2B + A4B) * (1 - A1B), MD);
                            gsl_matrix_addscaled(Bufferpm, BufferUpDown, P0 * (A1B + A3B) * (A4B + 1), MD);
                            gsl_matrix_addscaled(Bufferpm, BufferUpUp, P0 * (A1B + A3B) * (A2B - 1), MD);

                            gsl_matrix_scale(Bufferpm, bufferVolumeFraction / (2 * fxP * (A1B + A3B) * (A2B + A4B)) /
                                                           TRplusBuffer);

                            //+++ Sample(i,j) = Sample(i,j) - Buffer(i,j)
                            gsl_matrix_sub(Smm, Buffermm);
                            gsl_matrix_sub(Smp, Buffermp);
                            gsl_matrix_sub(Spm, Bufferpm);
                            gsl_matrix_sub(Spp, Bufferpp);

                            gsl_matrix_free(BufferUpUp);
                            gsl_matrix_free(BufferErrUpUp);
                            gsl_matrix_free(maskBufferUpUp);

                            gsl_matrix_free(BufferUpDown);
                            gsl_matrix_free(BufferErrUpDown);
                            gsl_matrix_free(maskBufferUpDown);

                            gsl_matrix_free(BufferDownDown);
                            gsl_matrix_free(BufferErrDownDown);
                            gsl_matrix_free(maskBufferDownDown);

                            gsl_matrix_free(BufferDownUp);
                            gsl_matrix_free(BufferErrDownUp);
                            gsl_matrix_free(maskBufferDownUp);
                        }
                    }

                    //+++
                    // EC Actions
                    //+++

                    if (rowInScriptBufferUpUp < 0)
                    {
                        status = true;

                        // Empty Cell: from OUT configuration
                        double absEC, TrEC, SigmaTrEC, thicknessEC;

                        if (rowInScriptOut >= 0 && singleDanFactorTransThickness(wScript, scriptColList, rowInScriptOut,
                                                                                 absEC, TrEC, SigmaTrEC, thicknessEC))
                            status &= singleDanSimplified(wScript, scriptColList, rowInScriptOut, EC, ECErr, maskEC,
                                                          1.0, true);
                        else
                            status = false;

                        if (status)
                            std::cout << "Empty Cell: from OUT configuration\n";

                        // Empty Cell: from DOWN configuration
                        if (!status)
                        {
                            status = true;

                            if (rowInScriptDown >= 0 &&
                                singleDanFactorTransThickness(wScript, scriptColList, rowInScriptDown, absEC, TrEC,
                                                              SigmaTrEC, thicknessEC))
                                status &= singleDanSimplified(wScript, scriptColList, rowInScriptDown, EC, ECErr,
                                                              maskEC, 1.0, true);
                            else
                                status = false;

                            if (status)
                                std::cout << "Empty Cell: from DOWN configuration\n";
                        }

                        // Empty Cell: from UP configuration
                        if (!status)
                        {
                            status = true;

                            if (rowInScriptUp >= 0 &&
                                singleDanFactorTransThickness(wScript, scriptColList, rowInScriptUp, absEC, TrEC,
                                                              SigmaTrEC, thicknessEC))
                                status &= singleDanSimplified(wScript, scriptColList, rowInScriptUp, EC, ECErr, maskEC,
                                                              1.0, true);
                            else
                                status = false;

                            if (status)
                                std::cout << "Empty Cell: from UP configuration\n";
                        }

                        if (status)
                        {
                            gsl_matrix_scale(EC, absEC / thickness);

                            gsl_matrix_addscaled(Smm, EC, -1.0 * (P * A1 + (1 - P) * (1 - A1)), MD);
                            gsl_matrix_addscaled(Spm, EC, -1.0 * ((1 - fxP) * A2 - fxP * (1 - A2)), MD);
                            gsl_matrix_addscaled(Smp, EC, -1.0 * ((1 - P) * A3 - P * (1 - A3)), MD);
                            gsl_matrix_addscaled(Spp, EC, -1.0 * (fxP * A4 + (1 - fxP) * (1 - A4)), MD);

                            gsl_matrix_free(EC);
                            gsl_matrix_free(ECErr);
                            gsl_matrix_free(maskEC);
                        }
                    }

                    double time = static_cast<double>(dt.elapsed() - pre_dt) / 1000.0;
                    pre_dt = dt.elapsed();

                    QString labelUpUp = singleDanMultiButton(wScript, scriptColList, rowInScriptUpUp, button,
                                                             dataSuffix + "-UpUp", Spp, SampleUpUpErr, maskUpUp, -1.0);
                    QString labelUpDown =
                        singleDanMultiButton(wScript, scriptColList, rowInScriptUpDown, button, dataSuffix + "-UpDown",
                                             Spm, SampleUpDownErr, maskUpDown, -1.0);
                    QString labelDownDown =
                        singleDanMultiButton(wScript, scriptColList, rowInScriptDownDown, button,
                                             dataSuffix + "-DownDown", Smm, SampleDownDownErr, maskDownDown, -1.0);
                    QString labelDownUp =
                        singleDanMultiButton(wScript, scriptColList, rowInScriptDownUp, button, dataSuffix + "-DownUp",
                                             Smp, SampleDownUpErr, maskDownUp, time);

                    gsl_matrix_free(SampleUpUpErr);
                    gsl_matrix_free(maskUpUp);
                    gsl_matrix_free(SampleUpDownErr);
                    gsl_matrix_free(maskUpDown);
                    gsl_matrix_free(SampleDownDownErr);
                    gsl_matrix_free(maskDownDown);
                    gsl_matrix_free(SampleDownUpErr);
                    gsl_matrix_free(maskDownUp);
                }

                //+++ progress
                progress.setLabelText(
                    "Current: " + QString::number(allUniqueConfigurations.count() * iSample + iConf) + " [last: " +
                    QString::number(allUniqueSamples.count() * allUniqueConfigurations.count()) + "]; av. speed: " +
                    QString::number(static_cast<double>(time.elapsed()) / 1000.0 /
                                        (allUniqueConfigurations.count() * iSample + iConf),
                                    'f', 2) +
                    " sec/run");

                progress.setValue(allUniqueConfigurations.count() * iSample + iConf);
                if (progress.wasCanceled())
                    goto theEnd;
            }
theEnd:

    //+++ settings table
    saveScriptSettings();

    //+++ progress
    progress.cancel();


    //+++ back to init. folder
    if (radioButtonOpenInProject->isChecked() && checkBoxSortOutputToFoldersisChecked)
    {
        if (maximizedExists)
        {
            activeWindow->hide();
            activeWindow->showMaximized();
        }
        app()->folders->setCurrentItem(cf->folderListItem());
        app()->changeFolder(cf, true);
    }

    app()->workspace()->blockSignals(false);
}
//+++  newPolarizedScriptTable()
void dan18::newPolarizedScriptTable(QString tableName)
{
    if (tableName == "")
    {
        tableName = comboBoxPolarizationScriptTable->currentText();
        if (tableName == "")
            tableName = "scriptPOLARIZED";
        bool ok;
        tableName = QInputDialog::getText(this, "Creation of Script-Table",
                                          "Enter name of a new Polarized-Table:", QLineEdit::Normal, tableName, &ok);
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

    if (checkBoxSortOutputToFolders->isChecked())
        app()->changeFolder("DAN :: script, info, ...");

    if (app()->checkTableExistence(tableName))
    {
        QMessageBox::critical(nullptr, "qtiSAS", "Table " + tableName + " is already exists");
        return;
    }

    //+++ new pol-script table
    Table *w = app()->newTable(tableName, 0, 11);
    w->setWindowLabel("DAN::Polarized::Table");
    app()->setListViewLabel(w->name(), "DAN::Polarized::Table");
    app()->updateWindowLists(w);

    // Col-Names
    int colNumber = 0;

    w->setColName(colNumber, "Sample-info");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Text);
    colNumber++;

    w->setColName(colNumber, "#-Condition");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Numeric);
    colNumber++;

    w->setColName(colNumber, "#-out");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Text);
    colNumber++;

    w->setColName(colNumber, "#-up");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Text);
    colNumber++;

    w->setColName(colNumber, "#-down");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Text);
    colNumber++;

    w->setColName(colNumber, "#-up-up");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Text);
    colNumber++;

    w->setColName(colNumber, "#-up-down");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Text);
    colNumber++;

    w->setColName(colNumber, "#-down-down");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Text);
    colNumber++;

    w->setColName(colNumber, "#-down-up");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Text);
    colNumber++;

    w->setColName(colNumber, "Buffer-Volume-Fraction");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Text);
    colNumber++;

    w->setColName(colNumber, "Buffer-Row-in-This-Script");
    w->setColPlotDesignation(colNumber, Table::None);
    w->setColumnType(colNumber, Table::Text);
    colNumber++;

    QStringList list;
    findTableListByLabel("DAN::Polarized::Table", list);
    comboBoxPolarizationScriptTable->clear();
    comboBoxPolarizationScriptTable->addItems(list);
    comboBoxPolarizationScriptTable->setCurrentIndex(comboBoxPolarizationScriptTable->findText(tableName));

    // adjust columns
    w->adjustColumnsWidth(false);

    maximizeWindow(tableName);
}
//+++  newPolarizedScriptTable
void dan18::updatePolarizedScriptTable(QString tableName)
{
    if (tableName == "")
        tableName = comboBoxPolarizationScriptTable->currentText();

    if (tableName == "")
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Create first Polarized-Script-Table");
        return;
    }

    //+++ searching active polarized script
    Table *wPol;
    bool tableExists = false;
    QList<MdiSubWindow *> tableList = app()->tableList();
    foreach (MdiSubWindow *t, tableList)
        if (t->name() == tableName)
        {
            wPol = (Table *)t;
            tableExists = true;
        }

    if (!tableExists)
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Table ~" + tableName + "~ does not exist !!! <br><h4>");

        QStringList list;
        findTableListByLabel("DAN::Polarized::Table", list);
        comboBoxPolarizationScriptTable->clear();
        comboBoxPolarizationScriptTable->addItems(list);

        return;
    }

    QString scriptTableName = comboBoxMakeScriptTable->currentText();
    if (scriptTableName == "")
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Create first Script-Table");
        return;
    }

    //+++ searching active script table
    Table *wScript;
    tableExists = false;
    foreach (MdiSubWindow *t, tableList)
        if (t->name() == scriptTableName)
        {
            wScript = (Table *)t;
            tableExists = true;
        }
    if (!tableExists)
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Table ~" + scriptTableName + "~ does not exist !!! <br><h4>");

        QStringList list;
        findTableListByLabel("DAN::Script::Table", list);
        comboBoxMakeScriptTable->clear();
        comboBoxMakeScriptTable->addItems(list);

        return;
    }

    int configuratinsNumber = tableEC->columnCount();

    //+++ Indexing
    QStringList scriptColList = wScript->colNames();
    int indexInfo = scriptColList.indexOf("Run-info");
    if (indexInfo < 0)
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Run-info column does not exists");
        return;
    }
    int indexCond = scriptColList.indexOf("#-Condition");
    if (indexCond < 0)
    {
        QMessageBox::critical(nullptr, "QtiSAS", "#-Condition column does not exists");
        return;
    }
    int indexRun = scriptColList.indexOf("#-Run");
    if (indexRun < 0)
    {
        QMessageBox::critical(nullptr, "QtiSAS", "#-Run column does not exists");
        return;
    }

    // lint of all non-pol configurations: with/without transmition configurations
    QList<int> existingNonPolConfigurations;
    allUniqueConfigurations(existingNonPolConfigurations, 0.05, false,
                            checkBoxSkiptransmisionConfigurations->isChecked());

    // lint of all pol configurations: with/without transmition configurations
    QList<int> existingPolConfigurations;
    allUniqueConfigurations(existingPolConfigurations, 0.05, true, checkBoxSkiptransmisionConfigurations->isChecked());

    // corresponding pol-configurations
    QList<QList<int>> correspondingPolConfigurations;
    QList<int> currentPolConfigurations;
    for (int c = 0; c < existingNonPolConfigurations.count(); c++)
    {
        currentPolConfigurations.clear();
        for (int cp = 0; cp < existingPolConfigurations.count(); cp++)
        {
            if (existingPolConfigurations[cp] == existingNonPolConfigurations[c])
            {
                currentPolConfigurations << existingPolConfigurations[cp];
                continue;
            }

            const QString runNumC = tableEC->item(dptEC, existingNonPolConfigurations[c] - 1)->text();
            const QString runNumCol = tableEC->item(dptEC, existingPolConfigurations[cp] - 1)->text();

            if (compareConfigurations(runNumC, runNumCol, 0.05, false) &&
                !compareConfigurations(runNumC, runNumCol, 0.05, true) &&
                !currentPolConfigurations.contains(existingPolConfigurations[cp]))
                currentPolConfigurations << existingPolConfigurations[cp];
        }
        correspondingPolConfigurations << currentPolConfigurations;
    }

    // list of all unique samples
    QStringList allUniqueSamples;
    for (int row = 0; row < wScript->numRows(); row++)
    {
        QString currentSample = wScript->text(row, indexInfo);
        if (currentSample.simplified() == "")
            continue;

        if (!allUniqueSamples.contains(currentSample))
            allUniqueSamples << currentSample;
    }

    // list of lists of configurations
    QList<QList<int>> configurationsForEverySampe;
    QList<int> currentSampleConfigurations;
    for (int iSample = 0; iSample < allUniqueSamples.count(); iSample++)
    {
        currentSampleConfigurations.clear();
        for (int row = 0; row < wScript->numRows(); row++)
        {
            QString currentSample = wScript->text(row, indexInfo);
            if (currentSample != allUniqueSamples[iSample])
                continue;

            int currentSampleConfigurationNumber = wScript->text(row, indexCond).toInt();
            if (currentSampleConfigurationNumber < 1)
                continue;

            if (existingNonPolConfigurations.contains(currentSampleConfigurationNumber) &&
                !currentSampleConfigurations.contains(currentSampleConfigurationNumber))
                currentSampleConfigurations << currentSampleConfigurationNumber;
        }
        std::sort(currentSampleConfigurations.begin(), currentSampleConfigurations.end());
        configurationsForEverySampe << currentSampleConfigurations;
    }

    // transfer of samples runs to Pol-Script table
    wPol->setNumRows(0);
    wPol->setNumRows(allUniqueSamples.count() * existingNonPolConfigurations.count());

    for (int iSample = 0; iSample < allUniqueSamples.count(); iSample++)
        for (int row = 0; row < wScript->numRows(); row++)
            if (allUniqueSamples[iSample] == wScript->text(row, indexInfo))
                if (existingPolConfigurations.contains(wScript->text(row, indexCond).toInt()))
                    for (int cp = 0; cp < correspondingPolConfigurations.count(); cp++)
                        if (correspondingPolConfigurations[cp].contains(wScript->text(row, indexCond).toInt()))
                        {
                            wPol->setText(iSample * existingNonPolConfigurations.count() + cp, 0,
                                          allUniqueSamples[iSample]);
                            wPol->setText(iSample * existingNonPolConfigurations.count() + cp, 1,
                                          QString::number(existingNonPolConfigurations[cp]));
                            QString polValue = collimation->readPolarization(wScript->text(row, indexRun));
                            wPol->setText(iSample * existingNonPolConfigurations.count() + cp,
                                          2 + polarizedAlias(polValue), wScript->text(row, indexRun));
                            wPol->setText(iSample * existingNonPolConfigurations.count() + cp, 9, "0.0");
                            wPol->setText(iSample * existingNonPolConfigurations.count() + cp, 10, "0");
                            break;
                        }

    // adjust columns
    wPol->adjustColumnsWidth(false);
}

