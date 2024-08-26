/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2024 Vitaliy Pipich <v.pipich@gmail.com>
Description: Data reduction functions defined by line number in a script
 ******************************************************************************/

#include "dan18.h"

//+++ button controlled output afzer data reduction
QString dan18::singleDanMultiButton(int iRow, const QString &button, const QString &dataSuffix, gsl_matrix *Sample,
                                    gsl_matrix *SampleErr, gsl_matrix *mask, double time)
{
    int MD = lineEditMD->text().toInt();

    double pixel = lineEditResoPixelSize->text().toDouble();
    double binning = comboBoxBinning->currentText().toDouble();
    double pixelAsymetry = lineEditAsymetry->text().toDouble();
    if (pixelAsymetry <= 0)
        pixelAsymetry = 1.0;

    bool radioButtonOpenInProjectisChecked = radioButtonOpenInProject->isChecked();
    bool checkBoxSortOutputToFoldersisChecked = checkBoxSortOutputToFolders->isChecked();
    bool checkBoxNameAsTableNameisChecked = checkBoxNameAsTableName->isChecked();
    bool checkBoxRewriteOutputisChecked = checkBoxRewriteOutput->isChecked();
    bool radioButtonXYdimQisChecked = radioButtonXYdimQ->isChecked();

    QString Nsample = scriptTableManager->runSample(iRow);
    if (Nsample == "" || !filesManager->checkFileNumber(Nsample))
    {
        std::cout << "Sample Run number does not exists ...\n";
        return "";
    }
    //+++ D
    double detdist = 100.0 * scriptTableManager->distance(iRow).toDouble();
    if (detdist <= 0.0)
    {
        std::cout << "D <= 0 \n";
        return "";
    }
    //+++ C
    double C = 100.0 * scriptTableManager->collimation(iRow).toDouble();
    //+++ Lambda
    double lambda = scriptTableManager->lambda(iRow).toDouble();
    if (lambda <= 0)
    {
        std::cout << "Lambda <= 0 \n";
        return "";
    }
    //+++ Delta-Lambda
    double deltaLambda = selector->readDeltaLambda(Nsample);
    //+++ optional parameter if column "Suffix" exist
    QString Suffix = scriptTableManager->suffix(iRow).remove(" ");
    //+++ condition
    int condition = scriptTableManager->condition(iRow).toInt();
    if (condition <= 0)
    {
        std::cout << "Condition is negative ...\n";
        return "";
    }

    //+++ Xcenter
    double Xcenter = scriptTableManager->centerX(iRow).toDouble();
    Xcenter -= 1.0;
    //+++ Ycenter
    double Ycenter = scriptTableManager->centerY(iRow).toDouble();
    Ycenter -= 1.0;
    //+++
    double r2 = collimation->readR2(Nsample);
    double r1 = collimation->readR1(Nsample);
    //+++
    double detRotationX = detector->readDetRotationX(Nsample);
    double detRotationY = detector->readDetRotationY(Nsample);

    //+++
    // NamesQI/Label generation
    //+++

    //+++ GENERAL suffix: from Process Interface
    QString currentExt = lineEditFileExt->text().remove(" ");
    if (currentExt != "")
        currentExt += "-";

    //+++ INDEPENDENT part of the widget/file name
    QString nameQI = Nsample.simplified();
    if (lineEditWildCard->text().contains("#"))
        nameQI = nameQI + "-" + scriptTableManager->info(iRow).simplified();

    if (checkBoxNameAsTableNameisChecked)
    {
        nameQI = scriptTableManager->info(iRow).simplified();
        if (lineEditWildCard->text().contains("#"))
            nameQI = nameQI + "-" + Nsample;
        else
            nameQI += "-c" + scriptTableManager->condition(iRow);
    }
    nameQI = nameQI.simplified();
    nameQI =
        nameQI.replace(" ", "-").replace("/", "-").replace("_", "-").replace(",", "-").replace(".", "-").remove("%");
    nameQI = currentExt + dataSuffix + "-" + nameQI;

    //+++ LABEL of a widget
    QString label = scriptTableManager->info(iRow).simplified();
    if (Suffix != "")
        label += " " + Suffix;
    QStringList dataSuffixList = dataSuffix.split("-");
    if (dataSuffixList.count() > 0 && dataSuffixList[0] != "")
        label += " [mode: " + dataSuffixList[0] + "]";
    if (dataSuffixList.count() > 1 && dataSuffixList[1] != "")
        label += " [polarization: " + dataSuffixList[1] + "]";


    int skipFirst = spinBoxRemoveFirst->value();
    int skipLast = spinBoxRemoveLast->value();

    int tmpInt = scriptTableManager->removeFirst(iRow).toInt();
    if (tmpInt > 0)
        skipFirst = tmpInt;

    tmpInt = scriptTableManager->removeLast(iRow).toInt();
    if (tmpInt > skipFirst)
        skipLast = tmpInt;

    //+++
    // "button"-driven output
    //+++

    if (button == "I-x-y") //+++   Reduced Matrix
    {
        nameQI = "I-" + nameQI;

        if (radioButtonOpenInProjectisChecked)
        {
            if (!checkBoxRewriteOutputisChecked)
                nameQI = app()->generateUniqueName(nameQI + "-v-");

            if (!radioButtonXYdimQisChecked || detdist == 0)
                makeMatrixSymmetric(Sample, nameQI, label, MD, true);
            else
            {

                double xs = 4.0 * M_PI / lambda * sin(0.5 * atan((1 - (Xcenter + 1)) * pixel * binning / detdist));
                double xe = 4.0 * M_PI / lambda * sin(0.5 * atan((MD - (Xcenter + 1)) * pixel * binning / detdist));
                double ys = 4.0 * M_PI / lambda *
                            sin(0.5 * atan((1 - (Ycenter + 1)) * pixel * binning * pixelAsymetry / detdist));
                double ye = 4.0 * M_PI / lambda *
                            sin(0.5 * atan((MD - (Ycenter + 1)) * pixelAsymetry * pixel * binning / detdist));

                makeMatrixSymmetric(Sample, nameQI, label, MD, xs, xe, ys, ye, true);
            }
        }
        else
        {
            //+++ Sigma -> dI
            double errTmp, samTmp;
            for (int iii = 0; iii < MD; iii++)
                for (int jjj = 0; jjj < MD; jjj++)
                {
                    errTmp = sqrt(fabs(gsl_matrix_get(SampleErr, iii, jjj)));
                    samTmp = gsl_matrix_get(Sample, iii, jjj);
                    gsl_matrix_set(SampleErr, iii, jjj, errTmp * fabs(samTmp));
                }

            QString pathPart = lineEditPathRAD->text();
            if (checkBoxSortOutputToFoldersisChecked)
                pathPart += "/ASCII-I/";
            else
                pathPart += "/";

            nameQI = pathPart + nameQI + "-" + comboBoxInstrument->currentText() + ".DAT";
            nameQI = nameQI.replace("//", "/");

            QDir dd;
            if (pathPart.contains("/ASCII-I") && !dd.cd(pathPart))
                dd.mkdir(pathPart);

            if (comboBoxIxyFormat->currentText().contains("Matrix"))
                saveMatrixToFile(nameQI, Sample, MD);
            else
            {
                gsl_matrix *sigmaMa = gsl_matrix_alloc(MD, MD);
                sigmaGslMatrix(sigmaMa, MD, mask, Xcenter, Ycenter, lambda, deltaLambda, C, detdist, pixel * binning,
                               r1, r2);
                saveMatrixAsTableToFile(nameQI, Sample, SampleErr, sigmaMa, mask, MD, Xcenter, Ycenter, lambda,
                                        deltaLambda, detdist, pixel * binning, pixel * binning * pixelAsymetry);
                gsl_matrix_free(sigmaMa);
            }
        }
    }
    else if (button == "dI-x-y") //+++ Error Matrix dI-x-y
    {
        //+++ Sigma -> dI
        double errTmp, samTmp;
        for (int iii = 0; iii < MD; iii++)
            for (int jjj = 0; jjj < MD; jjj++)
            {
                errTmp = sqrt(fabs(gsl_matrix_get(SampleErr, iii, jjj)));
                samTmp = gsl_matrix_get(Sample, iii, jjj);
                gsl_matrix_set(SampleErr, iii, jjj, errTmp * fabs(samTmp));
            }

        nameQI = "dI-" + nameQI;

        if (radioButtonOpenInProjectisChecked)
        {
            if (!checkBoxRewriteOutputisChecked)
                nameQI = app()->generateUniqueName(nameQI + "-v-");

            makeMatrixSymmetric(SampleErr, nameQI, label, MD, true);
        }
        else
        {
            QString pathPart = lineEditPathRAD->text() + "/";
            if (checkBoxSortOutputToFoldersisChecked)
            {
                pathPart += "ASCII-dI/";
                QDir().mkdir(pathPart);
            }

            nameQI = pathPart + nameQI + "-" + comboBoxInstrument->currentText() + ".DAT";
            nameQI = nameQI.replace("//", "/");

            saveMatrixToFile(nameQI, SampleErr, MD);
        }
    }
    else if (button == "I-Q") //+++ Standart radial averiging
    {
        if (comboBoxMode->currentText().contains("(MS)"))
        {
            double angleMS = double(spinBoxMCshiftAngle->value()) / 180.0 * M_PI;
            radUniStandartMSmode(MD, Sample, SampleErr, mask, Xcenter, Ycenter, nameQI, C, lambda, deltaLambda, detdist,
                                 pixel * binning, r1, r2, label,
                                 selector->readRotations(Nsample, monitors->readDuration(Nsample)), pixelAsymetry,
                                 angleMS, skipFirst, skipLast);
        }
        else
        {
            double angleAnisotropy = double(spinBoxAnisotropyOffset->value()) / 180.0 * M_PI;
            radUni(MD, Sample, SampleErr, mask, Xcenter, Ycenter, nameQI, C, lambda, deltaLambda, detdist,
                   pixel * binning, r1, r2, label, selector->readRotations(Nsample, monitors->readDuration(Nsample)),
                   pixelAsymetry, detRotationX, detRotationY, angleAnisotropy, skipFirst, skipLast);
        }
    }
    else if (button == "I-Qx") //+++ Hosizontal Slice
    {
        bool slicesBS = checkBoxSlicesBS->isChecked();
        int from = spinBoxFrom->value();
        if (slicesBS && spinBoxLTyBS->value() > from)
            from = spinBoxLTyBS->value();
        int to = spinBoxTo->value();
        if (slicesBS && spinBoxRByBS->value() < to)
            to = spinBoxRByBS->value();
        if (from <= to && from > 0 && to <= MD)
        {
            for (int xxx = 0; xxx < MD; xxx++)
                for (int yyy = 0; yyy < (from - 1); yyy++)
                    gsl_matrix_set(mask, yyy, xxx, 0.0);
            for (int xxx = 0; xxx < MD; xxx++)
                for (int yyy = to; yyy < MD; yyy++)
                    gsl_matrix_set(mask, yyy, xxx, 0.0);
        }

        horizontalSlice(MD, Sample, SampleErr, mask, Xcenter, Ycenter, nameQI, C, lambda, deltaLambda, detdist,
                        pixel * binning * pixelAsymetry, r1, r2, label, skipFirst, skipLast);
    }
    else if (button == "I-Qy") //+++ Vertical Slice
    {
        bool slicesBS = checkBoxSlicesBS->isChecked();
        int from = spinBoxFrom->value();
        if (slicesBS && spinBoxLTxBS->value() > from)
            from = spinBoxLTxBS->value();
        int to = spinBoxTo->value();
        if (slicesBS && spinBoxRBxBS->value() < to)
            to = spinBoxRBxBS->value();
        if (from <= to && from > 0 && to <= MD)
        {
            for (int yyy = 0; yyy < MD; yyy++)
                for (int xxx = 0; xxx < (from - 1); xxx++)
                    gsl_matrix_set(mask, yyy, xxx, 0.0);
            for (int yyy = 0; yyy < MD; yyy++)
                for (int xxx = to; xxx < MD; xxx++)
                    gsl_matrix_set(mask, yyy, xxx, 0.0);
        }

        verticalSlice(MD, Sample, SampleErr, mask, Xcenter, Ycenter, nameQI, C, lambda, deltaLambda, detdist,
                      pixel * binning * pixelAsymetry, r1, r2, label, skipFirst, skipLast);
    }
    else if (button == "I-Polar") //+++ Polarv Coordinates
        radUniPolar(MD, Sample, mask, Xcenter, Ycenter, nameQI, lambda, detdist, pixel * binning, pixelAsymetry);
    else if (button == "Sigma-x-y") //+++ SIGMA [x,y]
        sigmaMatrix(MD, mask, Xcenter, Ycenter, nameQI, lambda, deltaLambda, C, detdist, pixel * binning, r1, r2);
    else if (button == "Q-x-y") //+++ Q [x,y]
        MatrixQ(MD, mask, Xcenter, Ycenter, nameQI, lambda, detdist, pixel * binning, pixelAsymetry, detRotationX,
                detRotationY);
    else if (button == "dQ-x-y") //+++ dQ [x,y]
        dQmatrix(MD, mask, Xcenter, Ycenter, nameQI, lambda, detdist, pixel * binning, pixelAsymetry);

    if (time >= 0)
        printf("DAN|Result: [%6.5lgsec]\t%s\n", time, nameQI.toLocal8Bit().constData());
    else
        printf("DAN|Result: \t\t%s\n", nameQI.toLocal8Bit().constData());

    return nameQI;
}
//+++ singledan
bool dan18::singleDanSimplified(ScriptTableManager *scriptTableManager, int iRow, gsl_matrix *&Sample,
                                gsl_matrix *&SampleErr, gsl_matrix *&mask, double trans, bool ecInSampleRow)
{
    bool checkBoxMaskNegativeisChecked = checkBoxMaskNegative->isChecked();
    bool checkBoxParallaxTrisChecked = checkBoxParallaxTr->isChecked();
    bool checkBoxParallaxisChecked = checkBoxParallax->isChecked();
    bool checkBoxBCTimeNormalizationisChecked = checkBoxBCTimeNormalization->isChecked();

    double pixel = lineEditResoPixelSize->text().toDouble();
    double binning = comboBoxBinning->currentText().toDouble();
    double pixelAsymetry = lineEditAsymetry->text().toDouble();
    if (pixelAsymetry <= 0)
        pixelAsymetry = 1.0;

    int MD = lineEditMD->text().toInt();

    //+++ #-Run : Sample Run Number
    QString Nsample = scriptTableManager->runSample(iRow);
    if (ecInSampleRow)
        Nsample = scriptTableManager->runEC(iRow);
    if (Nsample == "" || !filesManager->checkFileNumber(Nsample))
    {
        std::cout << "Sample Run number does not exists ...\n";
        return false;
    }
    //+++  #-BC
    QString NBC = scriptTableManager->runBC(iRow);
    if (NBC != "" && !filesManager->checkFileNumber(NBC))
    {
        std::cout << "BC IS not subtructed ...\n";
        NBC = "";
    }
    //+++ C
    double C = 100.0 * scriptTableManager->collimation(iRow).toDouble();
    //+++ D
    double detdist = 100.0 * scriptTableManager->distance(iRow).toDouble();
    if (detdist <= 0.0)
    {
        std::cout << "D <= 0 \n";
        return false;
    }
    //+++ Lambda
    double lambda = scriptTableManager->lambda(iRow).toDouble();
    if (lambda <= 0)
    {
        std::cout << "Lambda <= 0 \n";
        return false;
    }
    double deltaLambda = selector->readDeltaLambda(Nsample);
    //+++  X-center
    double Xcenter = scriptTableManager->centerX(iRow).toDouble();
    Xcenter -= 1.0;
    //+++  Y-center
    double Ycenter = scriptTableManager->centerY(iRow).toDouble();
    Ycenter -= 1.0;
    //+++ optional parameter if column "VShift" exist
    double VShift = 0.0;
    //+++ optional parameter if column "HShift" exist
    double HShift = 0.0;
    if (!ecInSampleRow)
    {
        VShift = scriptTableManager->VSchift(iRow).toDouble();
        HShift = scriptTableManager->HSchift(iRow).toDouble();
    }

    //+++ optional parameter if column "TrDet" exist
    double TrDet = 1.0;
    bool checkBoxWaTrDetChecked = checkBoxWaTrDet->isChecked();
    if (checkBoxWaTrDetChecked)
    {
        TrDet = scriptTableManager->transmissionDetector(iRow).toDouble();
        if (TrDet > 1.0)
            TrDet = 1.0;
        else if (TrDet < 0)
            TrDet = 0.0;
    }

    Sample = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(Sample);
    readMatrixCor(Nsample, Sample);
    gslMatrixShift(Sample, MD, HShift, VShift);

    SampleErr = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(SampleErr);
    readErrorMatrix(Nsample, SampleErr); // (dN/N)^2
    gslMatrixShift(SampleErr, MD, HShift, VShift);

    gsl_matrix *BC = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(BC);
    gsl_matrix *BCErr = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(BCErr);

    if (NBC != "")
    {
        if (checkBoxBCTimeNormalizationisChecked)
        {
            readMatrixCorTimeNormalizationOnly(NBC, BC);

            // Normalization constant
            double TimeSample = spinBoxNorm->value();
            double ttime = monitors->readDuration(Nsample);
            if (ttime > 0.0)
                TimeSample /= ttime;
            else
                TimeSample = 0.0;

            double NormSample = monitors->normalizationFactor(Nsample);
            if (TimeSample > 0)
                NormSample /= TimeSample;
            else
                NormSample = 0;

            gsl_matrix_scale(BC, NormSample); // EB = T * EB
        }
        else
            readMatrixCor(NBC, BC);

        readErrorMatrix(NBC, BCErr);
    }

    //+++ mask gsl matrix
    mask = gsl_matrix_alloc(MD, MD);
    QString maskName = scriptTableManager->mask(iRow);
    if (!make_GSL_Matrix_Symmetric(maskName, mask, MD))
    {
        std::cout << "mask matrix: " << maskName.toLocal8Bit().constData() << " does not exist, trying to use: ";
        maskName = comboBoxMaskFor->currentText();
        std::cout << maskName.toLocal8Bit().constData() << " instead\n";
        if (!make_GSL_Matrix_Symmetric(maskName, mask, MD))
        {
            std::cout << "mask matrix: " << maskName.toLocal8Bit().constData()
                      << " does not exist also, mask[i,j] = 1 now\n ";
            gsl_matrix_set_all(mask, 1.0);
        }
    }

    //+++ sens gsl matrix
    gsl_matrix *sens = gsl_matrix_alloc(MD, MD);
    QString sensName = scriptTableManager->sens(iRow);
    if (!make_GSL_Matrix_Symmetric(sensName, sens, MD))
    {
        std::cout << "sensitivity matrix: " << sensName.toLocal8Bit().constData() << " does not exist, trying to use: ";
        sensName = comboBoxSensFor->currentText();
        std::cout << sensName.toLocal8Bit().constData() << " instead\n";
        if (!make_GSL_Matrix_Symmetric(sensName, sens, MD))
        {
            std::cout << "sensitivity matrix: " << sensName.toLocal8Bit().constData()
                      << " does not exist also, sensitivity[i,j] = 1 now\n ";
            gsl_matrix_set_all(sens, 1.0);
            sensName = "";
        }
    }
    //+++ sens & mask sinchronization
    sensAndMaskSynchro(mask, sens, MD);

    //+++ Sensetivty Error Matrix
    gsl_matrix *sensErr = gsl_matrix_calloc(MD, MD);
    QString sensFile = getSensitivityNumber(sensName);
    if (sensFile != "" && filesManager->checkFileNumber(sensFile))
        readErrorMatrix(sensFile, sensErr);

    gsl_matrix *sensTrDet;
    if (checkBoxWaTrDetChecked)
    {
        sensTrDet = gsl_matrix_alloc(MD, MD);
        gslMatrixWaTrDet(MD, TrDet, sensTrDet, Xcenter, Ycenter, detdist, pixel * binning, pixelAsymetry);
    }

    double err2;
    double Isample, Ibc, Idiff;

    for (int iii = 0; iii < MD; iii++)
        for (int jjj = 0; jjj < MD; jjj++)
        {
            //+++
            Isample = gsl_matrix_get(Sample, iii, jjj);
            //+++
            Ibc = gsl_matrix_get(BC, iii, jjj);
            //+++
            Idiff = Isample - Ibc;
            Idiff *= Idiff;
            //+++ sample statistics error
            Isample *= Isample;
            Isample *= gsl_matrix_get(SampleErr, iii, jjj);
            //+++ B4C statistics error
            Ibc *= Ibc;
            Ibc *= gsl_matrix_get(BCErr, iii, jjj);
            //+++
            err2 = Isample + Ibc;
            if (Idiff != 0.0)
                err2 /= Idiff;
            else
                err2 = 0.0;

            gsl_matrix_set(SampleErr, iii, jjj, err2); // sigma^2
        }

    gsl_matrix_add(SampleErr, sensErr); // sensitivity error

    gsl_matrix_mul_elements(SampleErr, mask);

    if (checkBoxWaTrDetChecked)
        gsl_matrix_mul_elements(Sample, sensTrDet);

    //+++ Matrix-to-Matrix actions +++
    gsl_matrix_sub(Sample, BC); // Sample = Sample - BC

    double Xc, Yc;
    readCenterfromMaskName(maskName, Xc, Yc, MD);

    gsl_matrix_mul_elements(Sample, mask);

    //+++ Paralax Correction
    if (checkBoxParallaxisChecked || checkBoxParallaxTrisChecked)
    {
        double Detector = detector->readD(Nsample); // [cm]
        parallaxCorrection(Sample, Xc, Yc, Detector, trans);
    }

    if (comboBoxACmethod->currentIndex() == 3)
    {
        double normalization = monitors->normalizationFactor(Nsample);
        if (normalization > 0)
            gsl_matrix_scale(Sample, 1 / normalization);
    }

    //+++ Sensitivity correction
    gsl_matrix_mul_elements(Sample, sens);

    //+++ Masking of  Negative Points
    if (checkBoxMaskNegativeisChecked)
        for (int iii = 0; iii < MD; iii++)
            for (int jjj = 0; jjj < MD; jjj++)
                if (gsl_matrix_get(Sample, iii, jjj) <= 0)
                {
                    gsl_matrix_set(Sample, iii, jjj, 0.0);
                    gsl_matrix_set(mask, iii, jjj, 0.0);
                    gsl_matrix_set(sens, iii, jjj, 0.0);
                }

    //+++ Matrix convolusion
    matrixConvolusion(Sample, mask, MD);

    //+++ Clean Memory +++
    gsl_matrix_free(BC);
    gsl_matrix_free(BCErr);
    gsl_matrix_free(sens);
    gsl_matrix_free(sensErr);

    if (checkBoxWaTrDetChecked)
        gsl_matrix_free(sensTrDet);
    //--- Clean Memory ---

    return true;
}
//+++ read absfactor, transmission, and thickness from the active script table
bool dan18::singleDanFactorTransThickness(ScriptTableManager *scriptTableManager, int iRow, double &abs, double &trans,
                                          double &sigmaTrans, double &thickness)
{

    //+++  Factor
    abs = scriptTableManager->absoluteFactor(iRow).toDouble();
    if (abs <= 0.0)
    {
        scriptTableManager->absoluteFactorWrite(iRow, "check!!!");
        std::cout << "Line # " << iRow + 1 << ": check Abs.Factor!\n";
        return false;
    }

    //+++ optional parameter if column "Scale" exist
    double scale = 1.0;
    if (scriptTableManager->scale(iRow) != "")
    {
        scale = scriptTableManager->scale(iRow).toDouble();
        if (scale <= 0.0)
            scale = 1.0;
    }

    // Absolute Calibration factors
    abs *= scale;

    //+++ Thickness
    thickness = scriptTableManager->thickness(iRow).toDouble();
    if (thickness <= 0.0 || thickness > 100.0)
    {
        std::cout << "Line # " << iRow + 1 << ": check Thickness!\n";
        return false;
    }

    //+++ Transmission-Sample
    trans = 1.0;
    sigmaTrans = 0.0;

    QStringList lst;
    lst = scriptTableManager->transmission(iRow)
              .remove(" ")
              .remove(QChar(177))
              .remove("\t")
              .remove("]")
              .split("[", Qt::SkipEmptyParts);
    if (lst.count() <= 0)
    {
        scriptTableManager->transmissionWrite(iRow, "Check!!!");
        std::cout << "Line # " << iRow + 1 << ": check transmission!\n";
        return false;
    }
    trans = lst[0].toDouble();
    if (trans <= 0.0 || trans > 2.0)
    {
        scriptTableManager->transmissionWrite(iRow, "Check!!!");
        std::cout << "Line # " << iRow + 1 << ": check transmission!\n";
        return false;
    }
    if (lst.count() > 1)
        sigmaTrans = lst[1].toDouble();
    if (trans != 0)
        sigmaTrans = fabs(sigmaTrans / trans);

    return true;
}
//+++ read analyzer transmission and efficiency
bool dan18::singleDanAnalyzerStatus(ScriptTableManager *scriptTableManager, int iRow, double &analyzerTransmission,
                                    double &analyzerEfficiency)
{
    analyzerTransmission = scriptTableManager->analyzerTransmission(iRow).toDouble();
    if (analyzerTransmission <= 0.0 || analyzerTransmission > 1)
    {
        scriptTableManager->analyzerTransmissionWrite(iRow, "Check!!!");
        std::cout << "Line # " << iRow + 1 << ": check Analyzer Transmission!\n";
        return false;
    }

    //+++  Analyzer Efficiency
    analyzerEfficiency = scriptTableManager->analyzerEfficiency(iRow).toDouble();
    if (analyzerEfficiency <= 0.0 || analyzerEfficiency > 1)
    {
        scriptTableManager->analyzerEfficiencyWrite(iRow, "check!!!");
        std::cout << "Line # " << iRow + 1 << ": check Analyzer Efficiency!\n";
        return false;
    }
    return true;
}
