/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2024 Vitaliy Pipich <v.pipich@gmail.com>
Description: Polarized Neutron Mode Functions
 ******************************************************************************/

#include "dan18.h"

double SQUARE(double x)
{
    return pow(abs(x), 2);
}
// TrDown -> Tr-; TrUp -> Tr+
bool transmissionCorrectionPNx2(double &TrDown, double &TrDownSigma, double &TrUp, double &TrUpSigma, double P,
                                double Pf, bool infoTerminalYN = false)
{
    if (P == 0 || Pf == -1)
        return false;

    double TrMinus = (P * Pf + 1) / (P * (Pf + 1)) * TrDown + (P - 1) / (P * (Pf + 1)) * TrUp;
    double TrPlus = (P * Pf - 1) / (P * (Pf + 1)) * TrDown + (P + 1) / (P * (Pf + 1)) * TrUp;

    double TrMinusSigma = sqrt(SQUARE((P * Pf + 1) * TrDownSigma) + SQUARE((P - 1) * TrUpSigma)) / P / (Pf + 1);
    double TrPlusSigma = sqrt(SQUARE((P * Pf - 1) * TrDownSigma) + SQUARE((P + 1) * TrUpSigma)) / P / (Pf + 1);

    if (infoTerminalYN)
    {
        std::cout << " TrDown: " << TrDown << " TrMinus: " << TrMinus << "\n";
        std::cout << " TrDownSigma: " << TrDownSigma << " TrMinusSigma: " << TrMinusSigma << "\n";
        std::cout << " TrUp: " << TrUp << " TrPlus: " << TrPlus << "\n";
        std::cout << " TrUpSigma: " << TrUpSigma << " TrPlusSigma: " << TrPlusSigma << "\n";
    }

    TrDown = TrMinus;
    TrUp = TrPlus;
    TrDownSigma = TrMinusSigma;
    TrUpSigma = TrPlusSigma;

    return true;
}
//+++ SampleDown -> I-; SampleUp -> I+
bool matrixCorrectionPNx2(gsl_matrix *Sdown, gsl_matrix *SdownSigma, gsl_matrix *Sup, gsl_matrix *SupSigma,
                          gsl_matrix *mask, double TrMinus, double TrMinusSigma, double TrPlus, double TrPlusSigma,
                          double P, double Pf, int MD)
{
    if (P == 0 || Pf == -1 || TrPlus == 0 || TrMinus == 0)
        return false;

    double minus, plus, down, up;
    double minusSigma2, plusSigma2, downSigma2, upSigma2;

    for (int ix = 0; ix < MD; ix++)
        for (int iy = 0; iy < MD; iy++)
            if (gsl_matrix_get(mask, iy, ix) != 0)
            {
                down = gsl_matrix_get(Sdown, iy, ix);
                up = gsl_matrix_get(Sup, iy, ix);

                minus = (P * Pf + 1) / (P * (Pf + 1)) * down / TrMinus + (P - 1) / (P * (Pf + 1)) * up / TrPlus;
                plus = (P * Pf - 1) / (P * (Pf + 1)) * down / TrMinus + (P + 1) / (P * (Pf + 1)) * up / TrPlus;

                gsl_matrix_set(Sdown, iy, ix, minus);
                gsl_matrix_set(Sup, iy, ix, plus);

                downSigma2 = gsl_matrix_get(SdownSigma, iy, ix);
                upSigma2 = gsl_matrix_get(SupSigma, iy, ix);

                minusSigma2 = SQUARE((P * Pf + 1) * down / TrMinus) * (TrMinusSigma * TrMinusSigma + downSigma2);
                minusSigma2 += SQUARE((P - 1) * up / TrPlus) * (TrPlusSigma * TrPlusSigma + upSigma2);
                minusSigma2 /= SQUARE(P * (Pf + 1));

                plusSigma2 = SQUARE((P * Pf - 1) * down / TrMinus) * (TrMinusSigma * TrMinusSigma + downSigma2);
                plusSigma2 += SQUARE((P + 1) * up / TrPlus) * (TrPlusSigma * TrPlusSigma + upSigma2);
                plusSigma2 /= SQUARE(P * (Pf + 1));

                gsl_matrix_set(SdownSigma, iy, ix, minusSigma2);
                gsl_matrix_set(SupSigma, iy, ix, plusSigma2);
            }
    return true;
}
bool matrixCorrectionPNx4(gsl_matrix *Sdd, gsl_matrix *SddSigma, gsl_matrix *Sdu, gsl_matrix *SduSigma, gsl_matrix *Suu,
                          gsl_matrix *SuuSigma, gsl_matrix *Sud, gsl_matrix *SudSigma, gsl_matrix *mask, double TrMinus,
                          double TrMinusSigma, double TrPlus, double TrPlusSigma, double A1, double A2, double A3,
                          double A4, double P, double f, double MD)
{
    double Pf = 2 * f - 1.0;
    double P0 = (1 + P) / 2;
    double fxP = f * P;

    if (fxP == 0 || TrPlus == 0 || TrMinus == 0 || (A1 + A3) == 0 || (A2 + A4) == 0)
        return false;

    double dd, du, uu, ud;
    double mm, mp, pp, pm;
    double ddSigma, duSigma, uuSigma, udSigma;
    double mmSigma, mpSigma, ppSigma, pmSigma;

    for (int ix = 0; ix < MD; ix++)
        for (int iy = 0; iy < MD; iy++)
            if (gsl_matrix_get(mask, iy, ix) != 0)
            {
                //+++ corrected matrixes
                dd = gsl_matrix_get(Sdd, iy, ix);
                du = gsl_matrix_get(Sdu, iy, ix);
                ud = gsl_matrix_get(Sud, iy, ix);
                uu = gsl_matrix_get(Suu, iy, ix);

                mm = dd * (fxP - (P0 - 1.0)) * (A2 + A4) * (A3 + 1);
                mm += du * (P0 - 1 - fxP) * (A2 + A4) * (1 - A1);
                mm += ud * (P0 - 1) * (A1 + A3) * (A4 + 1);
                mm += uu * (P0 - 1) * (A1 + A3) * (A2 - 1);
                mm /= (2 * fxP * (A1 + A3) * (A2 + A4) * TrMinus);

                mp = dd * (fxP - P0 + 1) * (A2 + A4) * (A3 - 1);
                mp += du * (fxP - P0 + 1) * (A2 + A4) * (1 + A1);
                mp += ud * (P0 - 1) * (A1 + A3) * (A4 - 1);
                mp += uu * (P0 - 1) * (A1 + A3) * (A2 + 1);
                mp /= (2 * fxP * (A1 + A3) * (A2 + A4) * TrMinus);

                pp = dd * (fxP - P0) * (A2 + A4) * (A3 - 1);
                pp += du * (fxP - P0) * (A2 + A4) * (1 + A1);
                pp += ud * P0 * (A1 + A3) * (A4 - 1);
                pp += uu * P0 * (A1 + A3) * (A2 + 1);
                pp /= (2 * fxP * (A1 + A3) * (A2 + A4) * TrPlus);

                pm = dd * (fxP - P0) * (A2 + A4) * (A3 + 1);
                pm += du * (P0 - fxP) * (A2 + A4) * (1 - A1);
                pm += ud * P0 * (A1 + A3) * (A4 + 1);
                pm += uu * P0 * (A1 + A3) * (A2 - 1);
                pm /= (2 * fxP * (A1 + A3) * (A2 + A4) * TrPlus);

                gsl_matrix_set(Sdd, iy, ix, mm);
                gsl_matrix_set(Sdu, iy, ix, mp);
                gsl_matrix_set(Sud, iy, ix, pm);
                gsl_matrix_set(Suu, iy, ix, pp);

                //+++ sigma matrixes

                ddSigma = gsl_matrix_get(SddSigma, iy, ix);
                duSigma = gsl_matrix_get(SduSigma, iy, ix);
                udSigma = gsl_matrix_get(SudSigma, iy, ix);
                uuSigma = gsl_matrix_get(SuuSigma, iy, ix);

                mmSigma = SQUARE(dd * (fxP - (P0 - 1.0)) * (A2 + A4) * (A3 + 1)) * ddSigma;
                mmSigma += SQUARE(du * (P0 - 1 - fxP) * (A2 + A4) * (1 - A1)) * duSigma;
                mmSigma += SQUARE(ud * (P0 - 1) * (A1 + A3) * (A4 + 1)) * udSigma;
                mmSigma += SQUARE(uu * (P0 - 1) * (A1 + A3) * (A2 - 1)) * uuSigma;
                mmSigma /= SQUARE(2 * fxP * (A1 + A3) * (A2 + A4) * TrMinus);
                mmSigma += SQUARE(mm * TrMinusSigma);

                mpSigma = SQUARE(dd * (fxP - P0 + 1) * (A2 + A4) * (A3 - 1)) * ddSigma;
                mpSigma += SQUARE(du * (fxP - P0 + 1) * (A2 + A4) * (1 + A1)) * duSigma;
                mpSigma += SQUARE(ud * (P0 - 1) * (A1 + A3) * (A4 - 1)) * udSigma;
                mpSigma += SQUARE(uu * (P0 - 1) * (A1 + A3) * (A2 + 1)) * uuSigma;
                mpSigma /= SQUARE(2 * fxP * (A1 + A3) * (A2 + A4) * TrMinus);
                mpSigma += SQUARE(mp * TrMinusSigma);

                ppSigma = SQUARE(dd * (fxP - P0) * (A2 + A4) * (A3 - 1)) * ddSigma;
                ppSigma += SQUARE(du * (fxP - P0) * (A2 + A4) * (1 + A1)) * duSigma;
                ppSigma += SQUARE(ud * P0 * (A1 + A3) * (A4 - 1)) * udSigma;
                ppSigma += SQUARE(uu * P0 * (A1 + A3) * (A2 + 1)) * uuSigma;
                ppSigma /= SQUARE(2 * fxP * (A1 + A3) * (A2 + A4) * TrPlus);
                ppSigma += SQUARE(pp * TrPlusSigma);

                pmSigma = SQUARE(dd * (fxP - P0) * (A2 + A4) * (A3 + 1)) * ddSigma;
                pmSigma += SQUARE(du * (P0 - fxP) * (A2 + A4) * (1 - A1)) * duSigma;
                pmSigma += SQUARE(ud * P0 * (A1 + A3) * (A4 + 1)) * udSigma;
                pmSigma += SQUARE(uu * P0 * (A1 + A3) * (A2 - 1)) * uuSigma;
                pmSigma /= SQUARE(2 * fxP * (A1 + A3) * (A2 + A4) * TrPlus);
                pmSigma += SQUARE(pm * TrPlusSigma);

                gsl_matrix_set(SddSigma, iy, ix, mmSigma);
                gsl_matrix_set(SduSigma, iy, ix, mpSigma);
                gsl_matrix_set(SudSigma, iy, ix, pmSigma);
                gsl_matrix_set(SuuSigma, iy, ix, ppSigma);
            }
    return true;
}
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
    printf("\nDAN|START data reduction:\n\n");

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
    //
    if (!scriptTableManager->update(wScript))
        return;

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

    QList<QList<QStringList>> linesInScriptMerged;
    QList<QStringList> listCurrentSampleCurrentConfigMerged;
    QStringList linesCurrentSampleScriptMerged;

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

            linesCurrentSampleScriptMerged.clear();
            for (int iPol = 0; iPol < 7; iPol++)
                linesCurrentSampleScriptMerged << "";

            if (linesCurrentSampleInPolScript[iConf] >= 0)
                for (int iPol = 0; iPol < 7; iPol++)
                {
                    QString currentRunNumber = wPol->text(linesCurrentSampleInPolScript[iConf], 2 + iPol);
                    if (currentRunNumber != "")
                        for (int rowScript = 0; rowScript < wScript->numRows(); rowScript++)
                            if (scriptTableManager->runSample(rowScript) == currentRunNumber)
                            {
                                linesCurrentSampleScript[iPol] = rowScript;
                                break;
                            }
                }
            listCurrentSampleCurrentConfig << linesCurrentSampleScript;
            listCurrentSampleCurrentConfigMerged << linesCurrentSampleScriptMerged;
        }

        linesInScript << listCurrentSampleCurrentConfig;
        linesInScriptMerged << listCurrentSampleCurrentConfigMerged;
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
                        if (!singleDanFactorTransThickness(scriptTableManager, rowScript, absSample, TrSample,
                                                           TrSampleSigma, thicknessSample))
                            continue;

                        //+++ Analyzer transmission
                        singleDanAnalyzerStatus(scriptTableManager, rowScript, analyzerTr, analyzerEff);

                        //+++ read Sample/SampleErr Matrixes
                        if (!singleDanSimplified(scriptTableManager, rowScript, Sample, SampleErr, maskSample, TrSample,
                                                 false))
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
                            if (!singleDanFactorTransThickness(scriptTableManager, rowInScriptBuffer, absBuffer,
                                                               TrBuffer, TrBufferSigma, thicknessBuffer))
                                rowInScriptBuffer = -1;

                        //+++ Analyzer transmission
                        if (rowInScriptBuffer >= 0)
                            singleDanAnalyzerStatus(scriptTableManager, rowInScriptBuffer, analyzerTrBuffer,
                                                    analyzerEffBuffer);

                        //+++ read Buffer/BufferErr matrixes
                        if (rowInScriptBuffer >= 0)
                            if (!singleDanSimplified(scriptTableManager, rowInScriptBuffer, Buffer, BufferErr,
                                                     maskBuffer, TrBuffer, false))
                                rowInScriptBuffer = -1;

                        //+++ buffer subtruction if buffer exists
                        if (rowInScriptBuffer >= 0 && bufferVolumeFraction != 0)
                        {
                            gsl_matrix_mul_elements(maskSample, maskBuffer);

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

                            // free memory: Buffer / BufferErr /MaskBuffer
                            gsl_matrix_free(Buffer);
                            gsl_matrix_free(BufferErr);
                            gsl_matrix_free(maskBuffer);
                        }

                        //+++
                        // Empty Cell Actions
                        //+++

                        if (rowInScriptBuffer < 0 || (bufferVolumeFraction < 1 && bufferVolumeFraction > 0))
                            if (singleDanSimplified(scriptTableManager, rowScript, EC, ECErr, maskEC, 1.0, true))
                            {
                                double bufferScale = (rowInScriptBuffer < 0) ? 1.0 : 1.0 - bufferVolumeFraction;

                                //+++ scale to analyzer transmission
                                gsl_matrix_scale(EC, bufferScale / analyzerTr);

                                //+++ Sample(i,j) = Sample(i,j) - EC(i,j)
                                gsl_matrix_sub(Sample, EC);

                                //+++  ECErr(i,j) = ECErr(i,j) * EC(i,j) * EC(i,j)
                                gsl_matrix_mul_elements(ECErr, EC);
                                gsl_matrix_mul_elements(ECErr, EC);

                                //+++ SampleErr(i,j) = SampleErr(i,j) + ECErr(i,j)
                                gsl_matrix_add(SampleErr, ECErr);

                                //+++ maskSample and maskBuffer sinchronization
                                gsl_matrix_mul_elements(maskSample, maskEC);

                                // free memory: EC / ECErr / MaskEC
                                gsl_matrix_free(EC);
                                gsl_matrix_free(ECErr);
                                gsl_matrix_free(maskEC);
                            }

                        gsl_matrix_mul_elements(Sample, maskSample);
                        gsl_matrix_mul_elements(SampleErr, maskSample);

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

                        linesInScriptMerged[iSample][iConf][iPol] =
                            singleDanMultiButton(rowScript, button, label, Sample, SampleErr, maskSample, time);

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
                    if (!singleDanFactorTransThickness(scriptTableManager, rowInScriptDown, absDown, TrDown,
                                                       SigmaTrDown, thicknessDown))
                        continue;

                    //+++ SampleDown:: reading SampleDown, SampleDownErr and maskDown matrixes
                    if (!singleDanSimplified(scriptTableManager, rowInScriptDown, SampleDown, SampleDownErr, maskDown,
                                             TrDown, false))
                        continue;

                    //+++ SampleUp:: reading absUp, thicknessUp, TrUp and SigmaTrUp
                    if (!singleDanFactorTransThickness(scriptTableManager, rowInScriptUp, absUp, TrUp, SigmaTrUp,
                                                       thicknessUp))
                    {
                        gsl_matrix_free(SampleDown);
                        gsl_matrix_free(SampleDownErr);
                        continue;
                    }

                    //+++ SampleUp:: reading  SampleUp, SampleUpErr and maskUp matrixes
                    if (!singleDanSimplified(scriptTableManager, rowInScriptUp, SampleUp, SampleUpErr, maskUp, TrUp,
                                             false))
                    {
                        gsl_matrix_free(SampleDown);
                        gsl_matrix_free(SampleDownErr);
                        continue;
                    }

                    //+++ mask sinchro: maskDown(i,j) = maskDown(i,j) * maskUp(i,j)
                    gsl_matrix_mul_elements(maskDown, maskUp);

                    //+++ current wave length
                    double lambda = scriptTableManager->lambda(rowInScriptDown).toDouble();

                    //+++ polarizer: current polarization and flipper efficiency
                    double P = polarizationSelector->getValue(lambda, scriptTableManager->runSample(rowInScriptDown));
                    double f =
                        polFlipperEfficiencySelector->getValue(lambda, scriptTableManager->runSample(rowInScriptDown));
                    double Pf = 2 * f - 1.0;

                    //+++ Transmission normalization: TrDown -> Tr-; TrUp -> Tr+
                    double TrMinus = TrDown;
                    double TrPlus = TrUp;
                    double TrMinusSigma = SigmaTrDown;
                    double TrPlusSigma = SigmaTrUp;

                    transmissionCorrectionPNx2(TrMinus, TrMinusSigma, TrPlus, TrPlusSigma, P, Pf);

                    //+++ SampleDown -> I-; SampleUp -> I+
                    gsl_matrix *Imin = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_memcpy(Imin, SampleDown);
                    gsl_matrix *IminErr = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_memcpy(IminErr, SampleDownErr);
                    gsl_matrix *Iplus = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_memcpy(Iplus, SampleUp);
                    gsl_matrix *IplusErr = gsl_matrix_calloc(MD, MD);
                    gsl_matrix_memcpy(IplusErr, SampleUpErr);

                    matrixCorrectionPNx2(Imin, IminErr, Iplus, IplusErr, maskDown, TrMinus, TrMinusSigma, TrPlus,
                                         TrPlusSigma, P, Pf, MD);

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
                        if (!singleDanFactorTransThickness(scriptTableManager, rowInScriptBufferUp, absBufferUp,
                                                           TrBufferUp, TrBufferSigmaUp, thicknessBufferUp))
                            rowInScriptBufferUp = -1;

                    //+++ BufferUp:: read Buffer/BufferErr matrixes : Up
                    if (rowInScriptBufferUp >= 0)
                        if (!singleDanSimplified(scriptTableManager, rowInScriptBufferUp, BufferUp, BufferErrUp,
                                                 maskBufferUp, TrBufferUp, false))
                            rowInScriptBufferUp = -1;

                    if (rowInScriptBufferUp >= 0)
                    {
                        //+++ BufferDown:: read absBuffer, thicknessBuffer, TrBuffer, TrBufferSigma : Down
                        if (rowInScriptBufferDown >= 0)
                            if (!singleDanFactorTransThickness(scriptTableManager, rowInScriptBufferDown, absBufferDown,
                                                               TrBufferDown, TrBufferSigmaDown, thicknessBufferDown))
                                rowInScriptBufferDown = -1;

                        //+++ BufferDown:: read Buffer/BufferErr matrixes : Down
                        if (rowInScriptBufferDown >= 0)
                            if (!singleDanSimplified(scriptTableManager, rowInScriptBufferDown, BufferDown,
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
                        std::cout << "bufferVolumeFraction : " << bufferVolumeFraction << "\n";
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

                        //+++ maskSample and maskBuffer sinchronization
                        gsl_matrix_mul_elements(maskDown, maskBufferDown);

                        //+++ Transmission normalization: TrBufferDown -> Tr-; TrBufferUp -> Tr+
                        double TrBufferMinus = TrBufferDown;
                        double TrBufferPlus = TrBufferUp;
                        double TrBufferMinusSigma = TrBufferSigmaDown;
                        double TrBufferPlusSigma = TrBufferSigmaUp;

                        transmissionCorrectionPNx2(TrBufferMinus, TrBufferMinusSigma, TrBufferPlus, TrBufferPlusSigma,
                                                   P, Pf);

                        //+++ BufferDown -> I-; BufferUp -> I+
                        gsl_matrix *IminBuffer = gsl_matrix_calloc(MD, MD);
                        gsl_matrix_memcpy(IminBuffer, BufferDown);
                        gsl_matrix *IminBufferErr = gsl_matrix_calloc(MD, MD);
                        gsl_matrix_memcpy(IminBufferErr, BufferErrDown);
                        gsl_matrix *IplusBuffer = gsl_matrix_calloc(MD, MD);
                        gsl_matrix_memcpy(IplusBuffer, BufferUp);
                        gsl_matrix *IplusBufferErr = gsl_matrix_calloc(MD, MD);
                        gsl_matrix_memcpy(IplusBufferErr, BufferErrUp);

                        matrixCorrectionPNx2(IminBuffer, IminBufferErr, IplusBuffer, IplusBufferErr, maskDown,
                                             TrBufferMinus, TrBufferMinusSigma, TrBufferPlus, TrBufferPlusSigma, P, Pf,
                                             MD);

                        //+++ scale buffer with volume faction
                        gsl_matrix_scale(IminBuffer, bufferVolumeFraction);
                        gsl_matrix_scale(IplusBuffer, bufferVolumeFraction);

                        gsl_matrix_scale(IminBufferErr, bufferVolumeFraction * bufferVolumeFraction);
                        gsl_matrix_scale(IplusBufferErr, bufferVolumeFraction * bufferVolumeFraction);

                        //+++ Sample(i,j) = Sample(i,j) - Buffer(i,j)
                        gsl_matrix_sub(Imin, IminBuffer);
                        gsl_matrix_sub(Iplus, IplusBuffer);

                        //+++ SampleErr(i,j) = SampleErr(i,j) + BufferErr(i,j)
                        gsl_matrix_add(IminErr, IminBufferErr);
                        gsl_matrix_add(IplusErr, IplusBufferErr);

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
                        gsl_matrix_free(IminBufferErr);
                        gsl_matrix_free(IplusBufferErr);
                    }

                    //+++
                    // Empty Cell Actions
                    //+++

                    if (rowInScriptBufferUp < 0 || (bufferVolumeFraction < 1 && bufferVolumeFraction > 0))
                    {
                        gsl_matrix *ECUp, *ECErrUp, *maskECUp;
                        gsl_matrix *ECDown, *ECErrDown, *maskECDown;

                        //+++ read EC/ECErr matrixes : Up
                        if (singleDanSimplified(scriptTableManager, rowInScriptUp, ECUp, ECErrUp, maskECUp, 1.0, true))
                        {
                            if (!singleDanSimplified(scriptTableManager, rowInScriptDown, ECDown, ECErrDown, maskECDown,
                                                     1.0, true))
                            {
                                // free memory: EC / ECErr /MaskEC: Up
                                gsl_matrix_free(ECUp);
                                gsl_matrix_free(ECErrUp);
                                gsl_matrix_free(maskECUp);
                            }
                            else
                            {
                                double bufferScale = (rowInScriptBufferUp < 0) ? 1.0 : 1.0 - bufferVolumeFraction;

                                //+++ECDown -> I-; ECUp -> I+
                                gsl_matrix *IminEC = gsl_matrix_calloc(MD, MD);
                                gsl_matrix_memcpy(IminEC, ECDown);
                                gsl_matrix *IminECErr = gsl_matrix_calloc(MD, MD);
                                gsl_matrix_memcpy(IminECErr, ECErrDown);
                                gsl_matrix *IplusEC = gsl_matrix_calloc(MD, MD);
                                gsl_matrix_memcpy(IplusEC, ECUp);
                                gsl_matrix *IplusECErr = gsl_matrix_calloc(MD, MD);
                                gsl_matrix_memcpy(IplusECErr, ECErrUp);

                                gsl_matrix_mul_elements(maskDown, maskECDown);

                                matrixCorrectionPNx2(IminEC, IminECErr, IplusEC, IplusECErr, maskDown, 1.0, 0.0, 1.0,
                                                     0.0, P, Pf, MD);

                                gsl_matrix_scale(IminEC, bufferScale);
                                gsl_matrix_scale(IplusEC, bufferScale);

                                gsl_matrix_scale(IminECErr, bufferScale * bufferScale);
                                gsl_matrix_scale(IplusECErr, bufferScale * bufferScale);

                                //+++ Sample(i,j) = Sample(i,j) - EC(i,j)
                                gsl_matrix_sub(Imin, IminEC);
                                gsl_matrix_sub(Iplus, IplusEC);

                                //+++ SampleErr(i,j) = SampleErr(i,j) + ECErr(i,j)
                                gsl_matrix_add(IminErr, IminECErr);
                                gsl_matrix_add(IplusErr, IplusECErr);

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
                                gsl_matrix_free(IminECErr);
                                gsl_matrix_free(IplusECErr);
                            }
                        }
                    }

                    gsl_matrix_div_elements_with_mask(IminErr, Imin, maskDown, MD);
                    gsl_matrix_div_elements_with_mask(IminErr, Imin, maskDown, MD);

                    gsl_matrix_div_elements_with_mask(IplusErr, Iplus, maskDown, MD);
                    gsl_matrix_div_elements_with_mask(IplusErr, Iplus, maskDown, MD);

                    gsl_matrix_scale(Imin, absDown / thicknessDown);
                    gsl_matrix_scale(Iplus, absUp / thicknessUp);

                    double time = static_cast<double>(dt.elapsed() - pre_dt) / 1000.0;
                    pre_dt = dt.elapsed();

                    linesInScriptMerged[iSample][iConf][1] = singleDanMultiButton(
                        rowInScriptUp, button, dataSuffix + "-Up", Iplus, IplusErr, maskDown, -1.0);

                    linesInScriptMerged[iSample][iConf][2] = singleDanMultiButton(
                        rowInScriptDown, button, dataSuffix + "-Down", Imin, IminErr, maskDown, time);

                    gsl_matrix_free(maskUp);
                    gsl_matrix_free(maskDown);

                    gsl_matrix_free(Imin);
                    gsl_matrix_free(Iplus);
                    gsl_matrix_free(IminErr);
                    gsl_matrix_free(IplusErr);
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
                    if (!singleDanFactorTransThickness(scriptTableManager, rowInScriptUpUp, absUpUp, TrUpUp,
                                                       SigmaTrUpUp, thicknessUpUp))
                        continue;
                    //+++ SampleUpUp:: read analyzerTrUpUp, analyzerEffUpUp
                    if (!singleDanAnalyzerStatus(scriptTableManager, rowInScriptUpUp, analyzerTrUpUp, analyzerEffUpUp))
                        continue;
                    //+++ SampleUpUp:: read SampleUpUp, SampleUpUpErr, maskUpUp
                    if (!singleDanSimplified(scriptTableManager, rowInScriptUpUp, SampleUpUp, SampleUpUpErr, maskUpUp,
                                             TrUpUp, false))
                        continue;

                    //+++ SampleUpDown:: read absUpDown, thicknessUpDown, TrUpDown, SigmaTrUpDown
                    status &= singleDanFactorTransThickness(scriptTableManager, rowInScriptUpDown, absUpDown, TrUpDown,
                                                            SigmaTrUpDown, thicknessUpDown);
                    //+++ SampleUpDown:: read analyzerTrUpDown, analyzerEffUpDown
                    status &= singleDanAnalyzerStatus(scriptTableManager, rowInScriptUpDown, analyzerTrUpDown,
                                                      analyzerEffUpDown);
                    //+++ SampleUpDown:: read SampleUpDown, SampleUpDownErr, maskUpDown
                    if (!status || !singleDanSimplified(scriptTableManager, rowInScriptUpDown, SampleUpDown,
                                                        SampleUpDownErr, maskUpDown, TrUpDown, false))
                    {
                        gsl_matrix_free(SampleUpUp);
                        gsl_matrix_free(SampleUpUpErr);
                        gsl_matrix_free(maskUpUp);
                        continue;
                    }

                    //+++ SampleDownDown:: read absDownDown, thicknessDownDown, TrDownDown, SigmaTrDownDown
                    status &= singleDanFactorTransThickness(scriptTableManager, rowInScriptDownDown, absDownDown,
                                                            TrDownDown, SigmaTrDownDown, thicknessDownDown);
                    //+++ SampleDownDown:: read analyzerTrDownDown, analyzerEffDownDown
                    status &= singleDanAnalyzerStatus(scriptTableManager, rowInScriptDownDown, analyzerTrDownDown,
                                                      analyzerEffDownDown);
                    //+++ SampleDownDown:: read SampleDownDown, SampleDownDownErr, maskDownDown
                    if (!status || !singleDanSimplified(scriptTableManager, rowInScriptDownDown, SampleDownDown,
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
                    status &= singleDanFactorTransThickness(scriptTableManager, rowInScriptDownUp, absDownUp, TrDownUp,
                                                            SigmaTrDownUp, thicknessDownUp);
                    //+++ SampleDownUp:: read analyzerTrDownUp, analyzerEffDownUp
                    status &= singleDanAnalyzerStatus(scriptTableManager, rowInScriptDownUp, analyzerTrDownUp,
                                                      analyzerEffDownUp);
                    //+++ SampleDownUp:: read SampleDownUp, SampleDownUpErr, maskDownUp
                    if (!status || !singleDanSimplified(scriptTableManager, rowInScriptDownUp, SampleDownUp,
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
                    double lambda = scriptTableManager->lambda(rowInScriptDownDown).toDouble();

                    //+++ polarizer: current polarization and flipper efficiency
                    double P =
                        polarizationSelector->getValue(lambda, scriptTableManager->runSample(rowInScriptDownDown));
                    double f = polFlipperEfficiencySelector->getValue(
                        lambda, scriptTableManager->runSample(rowInScriptDownDown));
                    double Pf = 2 * f - 1.0;
                    double P0 = (1 + P) / 2;
                    double fxP = f * P;

                    // TrDown -> Tr-; TrUp -> Tr+
                    double TrMinus = TrDownDown;
                    double TrPlus = TrUpUp;
                    double TrMinusSigma = SigmaTrDownDown;
                    double TrPlusSigma = SigmaTrUpUp;

                    transmissionCorrectionPNx2(TrMinus, TrMinusSigma, TrPlus, TrPlusSigma, P, Pf);

                    double A1 = analyzerEffDownDown;
                    double A2 = analyzerEffUpDown;
                    double A3 = analyzerEffDownUp;
                    double A4 = analyzerEffUpUp;

                    double thickness = (thicknessDownDown + thicknessDownUp + thicknessUpUp + thicknessUpDown) / 4.0;
                    gsl_matrix_mul_elements(maskDownDown, maskDownUp);
                    gsl_matrix_mul_elements(maskDownDown, maskUpDown);
                    gsl_matrix_mul_elements(maskDownDown, maskUpUp);

                    gsl_matrix_scale(SampleDownDown, absDownDown / thickness / analyzerTrDownDown);
                    gsl_matrix_scale(SampleDownUp, absDownUp / thickness / analyzerTrDownUp);
                    gsl_matrix_scale(SampleUpUp, absUpUp / thickness / analyzerTrUpUp);
                    gsl_matrix_scale(SampleUpDown, absUpDown / thickness / analyzerTrUpDown);

                    matrixCorrectionPNx4(SampleDownDown, SampleDownDownErr, SampleDownUp, SampleDownUpErr, SampleUpUp,
                                         SampleUpUpErr, SampleUpDown, SampleUpDownErr, maskDownDown, TrMinus,
                                         TrMinusSigma, TrPlus, TrPlusSigma, A1, A2, A3, A4, P, f, MD);

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
                        if (!singleDanFactorTransThickness(scriptTableManager, rowInScriptBufferUpUp, absBufferUpUp,
                                                           TrBufferUpUp, TrBufferSigmaUpUp, thicknessBufferUpUp))
                            rowInScriptBufferUpUp = -1;

                        //+++ BufferUpUp:: read analyzerTrBufferUpUp, analyzerEffBufferUpUp
                        if (rowInScriptBufferUpUp >= 0)
                            if (!singleDanAnalyzerStatus(scriptTableManager, rowInScriptBufferUpUp,
                                                         analyzerTrBufferUpUp, analyzerEffBufferUpUp))
                                rowInScriptBufferUpUp = -1;

                        //+++ BufferUpUp:: read Buffer/BufferErr matrixes : UpUp
                        if (rowInScriptBufferUpUp >= 0)
                            if (!singleDanSimplified(scriptTableManager, rowInScriptBufferUpUp, BufferUpUp,
                                                     BufferErrUpUp, maskBufferUpUp, TrBufferUpUp, false))
                                rowInScriptBufferUpUp = -1;

                        if (rowInScriptBufferUpUp >= 0)
                        {
                            //+++ BufferUpDown:: read absBuffer, thicknessBuffer, TrBuffer, TrBufferSigma : UpDown
                            if (!singleDanFactorTransThickness(scriptTableManager, rowInScriptBufferUpDown,
                                                               absBufferUpDown, TrBufferUpDown, TrBufferSigmaUpDown,
                                                               thicknessBufferUpDown))
                                rowInScriptBufferUpUp = -1;

                            //+++ BufferUpDown:: read analyzerTrBufferUpDown, analyzerEffBufferUpDown
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanAnalyzerStatus(scriptTableManager, rowInScriptBufferUpDown,
                                                             analyzerTrBufferUpDown, analyzerEffBufferUpDown))
                                    rowInScriptBufferUpUp = -1;

                            //+++ read Buffer/BufferErr matrixes : UpDown
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanSimplified(scriptTableManager, rowInScriptBufferUpDown, BufferUpDown,
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
                            if (!singleDanFactorTransThickness(scriptTableManager, rowInScriptBufferDownDown,
                                                               absBufferDownDown, TrBufferDownDown,
                                                               TrBufferSigmaDownDown, thicknessBufferDownDown))
                                rowInScriptBufferUpUp = -1;

                            //+++ BufferDownDown:: read analyzerTrBufferDownDown, analyzerEffBufferDownDown
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanAnalyzerStatus(scriptTableManager, rowInScriptBufferDownDown,
                                                             analyzerTrBufferDownDown, analyzerEffBufferDownDown))
                                    rowInScriptBufferUpUp = -1;

                            //+++ BufferDownDown:: read Buffer/BufferErr matrixes : DownDown
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanSimplified(scriptTableManager, rowInScriptBufferDownDown, BufferDownDown,
                                                         BufferErrDownDown, maskBufferDownDown, TrBufferDownDown,
                                                         false))
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
                            if (!singleDanFactorTransThickness(scriptTableManager, rowInScriptBufferDownUp,
                                                               absBufferDownUp, TrBufferDownUp, TrBufferSigmaDownUp,
                                                               thicknessBufferDownUp))
                                rowInScriptBufferUpUp = -1;

                            //+++ BufferDownUp:: read analyzerTrBufferDownUp, analyzerEffBufferDownUp
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanAnalyzerStatus(scriptTableManager, rowInScriptBufferDownUp,
                                                             analyzerTrBufferDownUp, analyzerEffBufferDownUp))
                                    rowInScriptBufferUpUp = -1;

                            //+++ BufferDownUp:: read Buffer/BufferErr matrixes : DownUp
                            if (rowInScriptBufferUpUp >= 0)
                                if (!singleDanSimplified(scriptTableManager, rowInScriptBufferDownUp, BufferDownUp,
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
                            gsl_matrix_mul_elements(maskDownDown, maskBufferDownDown);
                            gsl_matrix_mul_elements(maskDownDown, maskBufferDownUp);
                            gsl_matrix_mul_elements(maskDownDown, maskBufferUpUp);
                            gsl_matrix_mul_elements(maskDownDown, maskBufferUpDown);

                            // TrDown -> Tr-; TrUp -> Tr+
                            double TrMinusBuffer = TrBufferDownDown;
                            double TrPlusBuffer = TrBufferUpUp;
                            double TrMinusSigmaBuffer = TrBufferSigmaDownDown;
                            double TrPlusSigmaBuffer = TrBufferSigmaUpUp;

                            transmissionCorrectionPNx2(TrMinusBuffer, TrMinusSigmaBuffer, TrPlusBuffer,
                                                       TrPlusSigmaBuffer, P, Pf);

                            double A1B = analyzerEffBufferDownDown;
                            double A2B = analyzerEffBufferUpDown;
                            double A3B = analyzerEffBufferDownUp;
                            double A4B = analyzerEffBufferUpUp;

                            gsl_matrix_scale(BufferDownDown,
                                             bufferVolumeFraction * absDownDown / thickness / analyzerTrBufferDownDown);
                            gsl_matrix_scale(BufferDownUp,
                                             bufferVolumeFraction * absDownUp / thickness / analyzerTrBufferDownUp);
                            gsl_matrix_scale(BufferUpUp,
                                             bufferVolumeFraction * absUpUp / thickness / analyzerTrBufferUpUp);
                            gsl_matrix_scale(BufferUpDown,
                                             bufferVolumeFraction * absUpDown / thickness / analyzerTrBufferUpDown);

                            matrixCorrectionPNx4(BufferDownDown, BufferErrDownDown, BufferDownUp, BufferErrDownUp,
                                                 BufferUpUp, BufferErrUpUp, BufferUpDown, BufferErrUpDown, maskDownDown,
                                                 TrMinusBuffer, TrMinusSigmaBuffer, TrPlusBuffer, TrPlusSigmaBuffer,
                                                 A1B, A2B, A3B, A4B, P, f, MD);

                            //+++ Sample(i,j) = Sample(i,j) - Buffer(i,j)
                            gsl_matrix_sub(SampleDownDown, BufferDownDown);
                            gsl_matrix_sub(SampleDownUp, BufferDownUp);
                            gsl_matrix_sub(SampleUpUp, BufferUpUp);
                            gsl_matrix_sub(SampleUpDown, BufferUpDown);

                            gsl_matrix_addscaled(SampleDownDownErr, BufferErrDownDown, SQUARE(bufferVolumeFraction),
                                                 MD);
                            gsl_matrix_addscaled(SampleDownUpErr, BufferErrDownUp, SQUARE(bufferVolumeFraction), MD);
                            gsl_matrix_addscaled(SampleUpUpErr, BufferErrUpUp, SQUARE(bufferVolumeFraction), MD);
                            gsl_matrix_addscaled(SampleUpDownErr, BufferErrUpDown, SQUARE(bufferVolumeFraction), MD);

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

                    if (rowInScriptBufferUpUp < 0 || (bufferVolumeFraction < 1 && bufferVolumeFraction > 0))
                    {
                        status = true;

                        // Empty Cell: from OUT configuration
                        double absEC, TrEC, SigmaTrEC, thicknessEC;

                        if (rowInScriptOut >= 0 && singleDanFactorTransThickness(scriptTableManager, rowInScriptOut,
                                                                                 absEC, TrEC, SigmaTrEC, thicknessEC))
                            status &=
                                singleDanSimplified(scriptTableManager, rowInScriptOut, EC, ECErr, maskEC, 1.0, true);
                        else
                            status = false;

                        if (status)
                            std::cout << "Empty Cell: from OUT configuration\n";

                        // Empty Cell: from DOWN configuration
                        if (!status)
                        {
                            status = true;

                            if (rowInScriptDown >= 0 &&
                                singleDanFactorTransThickness(scriptTableManager, rowInScriptDown, absEC, TrEC,
                                                              SigmaTrEC, thicknessEC))
                                status &= singleDanSimplified(scriptTableManager, rowInScriptDown, EC, ECErr, maskEC,
                                                              1.0, true);
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
                                singleDanFactorTransThickness(scriptTableManager, rowInScriptUp, absEC, TrEC, SigmaTrEC,
                                                              thicknessEC))
                                status &= singleDanSimplified(scriptTableManager, rowInScriptUp, EC, ECErr, maskEC, 1.0,
                                                              true);
                            else
                                status = false;

                            if (status)
                                std::cout << "Empty Cell: from UP configuration\n";
                        }

                        if (status)
                        {
                            gsl_matrix_mul_elements(maskDownDown, maskEC);
                            gsl_matrix_mul_elements(EC, maskDownDown);
                            gsl_matrix_mul_elements(ECErr, maskDownDown);

                            double bufferScale = (rowInScriptBufferUp < 0) ? 1.0 : 1.0 - bufferVolumeFraction;

                            gsl_matrix_scale(EC, absEC / thickness);

                            gsl_matrix_addscaled(SampleDownDown, EC, -1.0 * bufferScale * (P * A1 + (1 - P) * (1 - A1)),
                                                 MD);
                            gsl_matrix_addscaled(SampleDownUp, EC,
                                                 -1.0 * bufferScale * ((1 - fxP) * A2 - fxP * (1 - A2)), MD);
                            gsl_matrix_addscaled(SampleUpDown, EC, -1.0 * bufferScale * ((1 - P) * A3 - P * (1 - A3)),
                                                 MD);
                            gsl_matrix_addscaled(SampleUpUp, EC, -1.0 * bufferScale * (fxP * A4 + (1 - fxP) * (1 - A4)),
                                                 MD);

                            gsl_matrix_mul_elements(ECErr, ECErr);

                            gsl_matrix_addscaled(SampleDownDownErr, ECErr,
                                                 SQUARE(bufferScale * (P * A1 + (1 - P) * (1 - A1))), MD);
                            gsl_matrix_addscaled(SampleDownUpErr, ECErr,
                                                 SQUARE(bufferScale * ((1 - fxP) * A2 - fxP * (1 - A2))), MD);
                            gsl_matrix_addscaled(SampleUpUpErr, ECErr,
                                                 SQUARE(bufferScale * ((1 - P) * A3 - P * (1 - A3))), MD);
                            gsl_matrix_addscaled(SampleUpDownErr, ECErr,
                                                 SQUARE(bufferScale * (fxP * A4 + (1 - fxP) * (1 - A4))), MD);

                            gsl_matrix_free(EC);
                            gsl_matrix_free(ECErr);
                            gsl_matrix_free(maskEC);
                        }
                    }

                    gsl_matrix_div_elements_with_mask(SampleDownDownErr, SampleDownDown, maskDownDown, MD);
                    gsl_matrix_div_elements_with_mask(SampleDownDownErr, SampleDownDown, maskDownDown, MD);

                    gsl_matrix_div_elements_with_mask(SampleDownUpErr, SampleDownUp, maskDownDown, MD);
                    gsl_matrix_div_elements_with_mask(SampleDownUpErr, SampleDownUp, maskDownDown, MD);

                    gsl_matrix_div_elements_with_mask(SampleUpUpErr, SampleUpUp, maskDownDown, MD);
                    gsl_matrix_div_elements_with_mask(SampleUpUpErr, SampleUpUp, maskDownDown, MD);

                    gsl_matrix_div_elements_with_mask(SampleUpDownErr, SampleUpDown, maskDownDown, MD);
                    gsl_matrix_div_elements_with_mask(SampleUpDownErr, SampleUpDown, maskDownDown, MD);

                    double time = static_cast<double>(dt.elapsed() - pre_dt) / 1000.0;
                    pre_dt = dt.elapsed();

                    linesInScriptMerged[iSample][iConf][3] = singleDanMultiButton(
                        rowInScriptUpUp, button, dataSuffix + "-UpUp", SampleUpUp, SampleUpUpErr, maskDownDown, -1.0);
                    linesInScriptMerged[iSample][iConf][4] =
                        singleDanMultiButton(rowInScriptUpDown, button, dataSuffix + "-UpDown", SampleUpDown,
                                             SampleUpDownErr, maskDownDown, -1.0);
                    linesInScriptMerged[iSample][iConf][5] =
                        singleDanMultiButton(rowInScriptDownDown, button, dataSuffix + "-DownDown", SampleDownDown,
                                             SampleDownDownErr, maskDownDown, -1.0);
                    linesInScriptMerged[iSample][iConf][6] =
                        singleDanMultiButton(rowInScriptDownUp, button, dataSuffix + "-DownUp", SampleDownUp,
                                             SampleDownUpErr, maskDownDown, time);

                    gsl_matrix_free(SampleUpUp);
                    gsl_matrix_free(SampleUpUpErr);
                    gsl_matrix_free(maskUpUp);
                    gsl_matrix_free(SampleUpDown);
                    gsl_matrix_free(SampleUpDownErr);
                    gsl_matrix_free(maskUpDown);
                    gsl_matrix_free(SampleDownDown);
                    gsl_matrix_free(SampleDownDownErr);
                    gsl_matrix_free(maskDownDown);
                    gsl_matrix_free(SampleDownUp);
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

    //+++ merging table
    if (button == "I-Q" && checkBoxMergingTable->isChecked())
    {
        generateMergingTablePN(allUniqueSamples, linesInScriptMerged);

        if (checkBoxAutoMerging->isChecked())
        {
            readMergeInfo(true);
            mergeMethod();
        }
    }
    //+++ back to init. folder
    if (radioButtonOpenInProject->isChecked() && checkBoxSortOutputToFoldersisChecked)
    {
        app()->folders->setCurrentItem(cf->folderListItem());
        app()->changeFolder(cf, true);
        if (activeWindow && maximizedExists)
        {
            activeWindow->hide();
            activeWindow->showMaximized();
        }
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
            tableName = "scriptPN";
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

    app()->maximizeWindow(tableName);
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

    if (!scriptTableManager->update(wScript))
    {
        QMessageBox::critical(nullptr, "qtiSAS",
                              "Table ~" + tableName + " has wrong format. <br> Check table or  generate new one.<h4>");
        return;
    }
    int configuratinsNumber = tableEC->columnCount();

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
        QString currentSample = scriptTableManager->info(row);
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
            QString currentSample = scriptTableManager->info(row);
            if (currentSample != allUniqueSamples[iSample])
                continue;

            int currentSampleConfigurationNumber = scriptTableManager->condition(row).toInt();
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
            if (allUniqueSamples[iSample] == scriptTableManager->info(row))
                if (existingPolConfigurations.contains(scriptTableManager->condition(row).toInt()))
                    for (int cp = 0; cp < correspondingPolConfigurations.count(); cp++)
                        if (correspondingPolConfigurations[cp].contains(scriptTableManager->condition(row).toInt()))
                        {
                            wPol->setText(iSample * existingNonPolConfigurations.count() + cp, 0,
                                          allUniqueSamples[iSample]);
                            wPol->setText(iSample * existingNonPolConfigurations.count() + cp, 1,
                                          QString::number(existingNonPolConfigurations[cp]));
                            QString polValue = collimation->readPolarization(scriptTableManager->runSample(row));
                            wPol->setText(iSample * existingNonPolConfigurations.count() + cp,
                                          2 + polarizedAlias(polValue), scriptTableManager->runSample(row));
                            wPol->setText(iSample * existingNonPolConfigurations.count() + cp, 9, "0.0");
                            wPol->setText(iSample * existingNonPolConfigurations.count() + cp, 10, "0");
                            break;
                        }

    // adjust columns
    wPol->adjustColumnsWidth(false);
}

bool dan18::generateMergingTablePN(QStringList allUniqueSamples, QList<QList<QStringList>> linesInScriptMerged)
{
    int numberSamples = allUniqueSamples.count();
    if (numberSamples == 0 || numberSamples != linesInScriptMerged.count())
        return false;
    int numberConfigurations = linesInScriptMerged[0].count();
    if (numberConfigurations == 0)
        return false;
    if (linesInScriptMerged[0][0].count() != 7)
        return false;

    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    QString name = comboBoxMakeScriptTable->currentText();
    name += "-mergingTemplate";

    removeWindows(name);

    //+++
    // mode of the PN: PN / PNx2 / PNx4
    //+++
    int numberCommonFiles = 7;
    if (comboBoxPolReductionMode->currentIndex() == 1)
        numberCommonFiles = 2;
    else if (comboBoxPolReductionMode->currentIndex() == 2)
        numberCommonFiles = 4;


    Table *t = app()->newHiddenTable(name, "DAN::Merging::Template", 0, numberConfigurations + 1);

    // Col-Types: Text
    for (int tt = 0; tt < t->numCols(); tt++)
        t->setColumnType(tt, Table::Text);

    int numRows = 0;
    QString sampleName;
    //+++ Final data reduction steps
    for (int iSample = 0; iSample < numberSamples; iSample++)
        for (int iPol = 0; iPol < 7; iPol++)
        {
            if (numberCommonFiles == 2 && !(iPol == 1 || iPol == 2))
                continue;
            if (numberCommonFiles == 4 && !(iPol == 3 || iPol == 4 || iPol == 5 || iPol == 6))
                continue;

            sampleName = allUniqueSamples[iSample];
            switch (iPol)
            {
            case 0:
                sampleName += "-Out";
                break;
            case 1:
                sampleName += "-Up";
                break;
            case 2:
                sampleName += "-Down";
                break;
            case 3:
                sampleName += "-UpUp";
                break;
            case 4:
                sampleName += "-UpDown";
                break;
            case 5:
                sampleName += "-DownDown";
                break;
            case 6:
                sampleName += "-DownUp";
                break;
            default:
                sampleName += "-Out";
                break;
            }

            numRows++;
            t->setNumRows(numRows);
            t->setText(numRows - 1, 0, sampleName);

            bool existingConfiguration = false;

            for (int iConf = 0; iConf < numberConfigurations; iConf++)
                if (linesInScriptMerged[iSample][iConf][iPol] != "")
                {
                    existingConfiguration = true;
                    t->setText(numRows - 1, iConf + 1, linesInScriptMerged[iSample][iConf][iPol]);
                }
            if (!existingConfiguration)
            {
                numRows--;
                t->setNumRows(numRows);
            }

        }

    for (int i = 0; i < t->numRows(); i++)
    {
        sampleName = t->text(i, 0);
        sampleName = sampleName.replace("]", " ");
        sampleName = sampleName.replace("[", "s");
        sampleName = sampleName.simplified();
        sampleName = sampleName.replace(" ", "-");
        t->setText(i, 0, sampleName);
    }

    //+++ adjust cols
    t->adjustColumnsWidth(false);

    app()->setListViewLabel(t->name(), "DAN::Merging::Template");
    app()->updateWindowLists(t);
    app()->hideWindow(t);

    return true;
}