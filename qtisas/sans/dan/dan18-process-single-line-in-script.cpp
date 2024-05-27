/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Vitaliy Pipich <v.pipich@gmail.com>
Description: Data reduction functions defined by line number in a script
 ******************************************************************************/
#include "dan18.h"


//+++ singleDanMultiButton
QString dan18::singleDanMultiButton(const QString &button, const QString &dataSuffix, Table *wScript, int iRow,
                                    gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask, double time)
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

    //+++ Script table Indexing
    QStringList scriptColList = wScript->colNames();

    //+++ Run-info
    int indexInfo = scriptColList.indexOf("Run-info");
    //+++  #-Condition : condition of the Sample
    int indexCond = scriptColList.indexOf("#-Condition");
    if (indexCond < 0)
    {
        std::cout << "#-Condition column is missing ...\n";
        return "";
    }
    //+++ #-Run : Sample Run Number
    int indexSample = scriptColList.indexOf("#-Run");
    if (indexSample < 0)
    {
        std::cout << "#-Run column is missing ...\n";
        return "";
    }
    QString Nsample = wScript->text(iRow, indexSample);
    if (Nsample == "" || !filesManager->checkFileNumber(Nsample))
    {
        std::cout << "Sample Run number does not exists ...\n";
        return "";
    }
    //+++ D: sample-to-detector distance
    int indexD = scriptColList.indexOf("D");
    if (indexD < 0)
    {
        std::cout << "D-column does not exists ...\n";
        return "";
    }
    //+++ C
    int indexC = scriptColList.indexOf("C");
    if (indexC < 0)
    {
        std::cout << "C-column does not exists ...\n";
        // return false;
    }
    double C = 100.0 * wScript->text(iRow, indexC).toDouble();
    //+++ Lambda
    int indexLam = scriptColList.indexOf("Lambda");
    if (indexLam < 0)
    {
        std::cout << "Lambda-column does not exists ...\n";
        return "";
    }
    //+++ Delta-Lambda
    double deltaLambda = selector->readDeltaLambda(Nsample);
    //+++ Beam Size
    int indexCA = scriptColList.indexOf("Beam Size");
    if (indexCA < 0)
    {
        std::cout << "Beam Size column does not exists ...\n";
        // return false;
    }
    //+++  X-center
    int indexXC = scriptColList.indexOf("X-center");
    if (indexXC < 0)
    {
        std::cout << "X-center column does not exists ...\n";
        return "";
    }
    //+++  Y-center
    int indexYC = scriptColList.indexOf("Y-center");
    if (indexYC < 0)
    {
        std::cout << "Y-center column does not exists ...\n";
        return "";
    }
    //+++ optional parameter if column "Suffix" exist
    int indexSuffix = scriptColList.indexOf("Suffix");

    int condition = wScript->text(iRow, indexCond).toInt();
    if (condition <= 0)
    {
        std::cout << "Condition is negative ...\n";
        return "";
    }
    double detdist = 100.0 * wScript->text(iRow, indexD).toDouble();
    if (detdist <= 0.0)
    {
        std::cout << "D <= 0 \n";
        return "";
    }
    double lambda = wScript->text(iRow, indexLam).toDouble();
    if (lambda <= 0)
    {
        std::cout << "Lambda <= 0 \n";
        return "";
    }
    double Xcenter = wScript->text(iRow, indexXC).toDouble();
    Xcenter -= 1.0;
    double Ycenter = wScript->text(iRow, indexYC).toDouble();
    Ycenter -= 1.0;

    double r2 = collimation->readR2(Nsample);
    double r1 = collimation->readR1(Nsample);

    double detRotationX = detector->readDetRotationX(Nsample);
    double detRotationY = detector->readDetRotationY(Nsample);

    //+++
    // NamesQI/Label generation
    //+++

    //+++ GENERAL suffix: from Process Interface
    QString currentExt = lineEditFileExt->text().remove(" ");
    if (currentExt != "")
        currentExt += "-";

    //+++ INDIVIDUAL suffix: from the script table
    QString Suffix = "";
    if (indexSuffix > 0)
        Suffix = wScript->text(iRow, indexSuffix).remove(" ");

    //+++ INDEPENDENT part of the widget/file name
    QString nameQI = wScript->text(iRow, indexSample).simplified();
    if (lineEditWildCard->text().contains("#"))
        nameQI = nameQI + "-" + wScript->text(iRow, indexInfo).simplified();

    if (checkBoxNameAsTableNameisChecked)
    {
        nameQI = wScript->text(iRow, indexInfo);
        if (lineEditWildCard->text().contains("#"))
            nameQI = nameQI + "-" + Nsample;
        else
            nameQI += "-c" + wScript->text(iRow, indexCond).split(":")[0];
    }
    nameQI = nameQI.simplified();
    nameQI =
        nameQI.replace(" ", "-").replace("/", "-").replace("_", "-").replace(",", "-").replace(".", "-").remove("%");
    nameQI = currentExt + dataSuffix + "-" + nameQI;

    //+++ LABEL of a widget
    QString label = wScript->text(iRow, indexInfo);
    if (Suffix != "")
        label = Suffix + "-" + label;

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

            nameQI = pathPart + nameQI + "-" + comboBoxSel->currentText() + ".DAT";
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

            nameQI = pathPart + nameQI + "-" + comboBoxSel->currentText() + ".DAT";
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
                                 angleMS);
        }
        else
        {
            double angleAnisotropy = double(spinBoxAnisotropyOffset->value()) / 180.0 * M_PI;
            radUni(MD, Sample, SampleErr, mask, Xcenter, Ycenter, nameQI, C, lambda, deltaLambda, detdist,
                   pixel * binning, r1, r2, label, selector->readRotations(Nsample, monitors->readDuration(Nsample)),
                   pixelAsymetry, detRotationX, detRotationY, angleAnisotropy);
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
                        pixel * binning * pixelAsymetry, r1, r2, label);
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
                      pixel * binning * pixelAsymetry, r1, r2, label);
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
bool dan18::singleDan(Table *wScript, int iRow, gsl_matrix *&Sample, gsl_matrix *&SampleErr, gsl_matrix *&mask)
{
    if (!wScript)
        return false;

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

    QString comboBoxModecurrentText = comboBoxMode->currentText();
    //+++ Subtract Bufffer
    bool subtractBuffer = false;
    if (comboBoxModecurrentText.contains("(BS)"))
        subtractBuffer = true;
    //+++ Subtract Bufffer && Correct to own Sensitivity
    bool bufferAsSens = false;
    if (comboBoxModecurrentText.contains("(BS-SENS)"))
        bufferAsSens = true;

    //+++ Script table Indexing
    QStringList scriptColList = wScript->colNames();

    //+++ Check of script-table-structure
    //+++ Run-info

    int indexInfo = scriptColList.indexOf("Run-info");
    //+++  #-Condition : condition of the Sample
    int indexCond = scriptColList.indexOf("#-Condition");
    if (indexCond < 0)
    {
        std::cout << "#-Condition column is missing ...\n";
        return false;
    }
    int condition = wScript->text(iRow, indexCond).toInt();
    if (condition <= 0)
    {
        std::cout << "Condition is negative ...\n";
        return false;
    }
    //+++ #-Run : Sample Run Number
    int indexSample = scriptColList.indexOf("#-Run");
    if (indexSample < 0)
    {
        std::cout << "#-Run column is missing ...\n";
        return false;
    }
    QString Nsample = wScript->text(iRow, indexSample);
    if (Nsample == "" || !filesManager->checkFileNumber(Nsample))
    {
        std::cout << "Sample Run number does not exists ...\n";
        return false;
    }
    //+++  #-EC [EB]
    int indexEC = scriptColList.indexOf("#-EC [EB]");
    if (indexEC < 0)
        std::cout << "#-EC [EB] column is missing ...\n";
    QString NEC = "";
    if (indexEC > 0)
    {
        NEC = wScript->text(iRow, indexEC);
        if (NEC != "" && !filesManager->checkFileNumber(NEC))
        {
            std::cout << "EC[EB] IS not subtructed ...\n";
            NEC = "";
        }
    }
    //+++  #-BC
    int indexBC = scriptColList.indexOf("#-BC");
    if (indexBC < 0)
        std::cout << "#-BC column is missing ...\n";
    QString NBC = "";
    if (indexBC > 0)
    {
        NBC = wScript->text(iRow, indexBC);
        if (NBC != "" && !filesManager->checkFileNumber(NBC))
        {
            std::cout << "BC IS not subtructed ...\n";
            NBC = "";
        }
    }
    //+++ C
    int indexC = scriptColList.indexOf("C");
    if (indexC < 0)
    {
        std::cout << "C-column does not exists ...\n";
        // return false;
    }
    double C = 100.0 * wScript->text(iRow, indexC).toDouble();
    //+++ D: sample-to-detector distance
    int indexD = scriptColList.indexOf("D");
    if (indexD < 0)
    {
        std::cout << "D-column does not exists ...\n";
        return false;
    }
    double detdist = 100.0 * wScript->text(iRow, indexD).toDouble();
    if (detdist <= 0.0)
    {
        std::cout << "D <= 0 \n";
        return false;
    }
    //+++ Lambda
    int indexLam = scriptColList.indexOf("Lambda");
    if (indexLam < 0)
    {
        std::cout << "Lambda-column does not exists ...\n";
        return false;
    }
    double lambda = wScript->text(iRow, indexLam).toDouble();
    if (lambda <= 0)
    {
        std::cout << "Lambda <= 0 \n";
        return false;
    }
    double deltaLambda = selector->readDeltaLambda(Nsample);
    //+++ Beam Size
    int indexCA = scriptColList.indexOf("Beam Size");
    if (indexCA < 0)
    {
        std::cout << "Beam Size column does not exists ...\n";
        // return false;
    }
    //+++ Thickness
    int indexThickness = scriptColList.indexOf("Thickness");
    if (indexThickness < 0)
    {
        std::cout << "Thickness column does not exists ...\n";
        return false;
    }
    double thickness = wScript->text(iRow, indexThickness).toDouble();
    if (thickness <= 0.0 || thickness > 100.0)
    {
        std::cout << "Line # " << iRow + 1 << ": check Thickness!\n";
        return false;
    }
    //+++ Transmission-Sample
    double trans = 1.0;
    double sigmaTrans = 0.0;
    int indexTr = scriptColList.indexOf("Transmission-Sample");
    QStringList lst;
    if (indexTr < 0)
    {
        std::cout << "Transmission-Sample column does not exists ...\n";
        return false;
    }
    else
    {
        lst.clear();
        lst = wScript->text(iRow, indexTr)
                  .remove(" ")
                  .remove(QChar(177))
                  .remove("\t")
                  .remove("]")
                  .split("[", Qt::SkipEmptyParts);
        if (lst.count() <= 0)
        {
            wScript->setText(iRow, indexTr, "Check!!!");
            std::cout << "Line # " << iRow + 1 << ": check transmission!\n";
            return false;
        }
        trans = lst[0].toDouble();
        if (trans <= 0.0 || trans > 2.0)
        {
            wScript->setText(iRow, indexTr, "Check!!!");
            std::cout << "Line # " << iRow + 1 << ": check transmission!\n";
            return false;
        }

        if (lst.count() > 1)
            sigmaTrans = lst[1].toDouble();
        if (trans != 0)
            sigmaTrans = fabs(sigmaTrans / trans);
    }
    //+++  Factor
    int indexFactor = scriptColList.indexOf("Factor");
    if (indexFactor < 0)
    {
        std::cout << "Factor column does not exists ...\n";
        return false;
    }
    double abs0 = wScript->text(iRow, indexFactor).toDouble();
    if (abs0 <= 0.0)
    {
        wScript->setText(iRow, indexFactor, "check!!!");
        std::cout << "Line # " << iRow + 1 << ": check Abs.Factor!\n";
        return false;
    }
    //+++  X-center
    int indexXC = scriptColList.indexOf("X-center");
    if (indexXC < 0)
    {
        std::cout << "X-center column does not exists ...\n";
        return false;
    }
    double Xcenter = wScript->text(iRow, indexXC).toDouble();
    Xcenter -= 1.0;
    //+++  Y-center
    int indexYC = scriptColList.indexOf("Y-center");
    if (indexYC < 0)
    {
        std::cout << "Y-center column does not exists ...\n";
        return false;
    }
    double Ycenter = wScript->text(iRow, indexYC).toDouble();
    Ycenter -= 1.0;
    //+++ Mask
    int indexMask = scriptColList.indexOf("Mask");
    if (indexMask < 0)
    {
        std::cout << "Mask column does not exists ...\n";
        // return false;
    }
    //+++ Sens
    int indexSens = scriptColList.indexOf("Sens");
    if (indexSens < 0)
    {
        std::cout << "Sens column does not exists ...\n";
        // return false;
    }
    //+++  Status
    int indexStatus = scriptColList.indexOf("Status");
    if (indexStatus < 0)
    {
        std::cout << "Status column does not exists ...\n";
        // return false;
    }
    //+++ #-Buffer
    int indexBuffer = scriptColList.indexOf("#-Buffer");
    QString Nbuffer = "";
    if (subtractBuffer)
    {
        if (indexBuffer < 0)
        {
            std::cout << "#-Buffer column is missing ...\n";
            return false;
        }
        Nbuffer = wScript->text(iRow, indexBuffer);
        if (!filesManager->checkFileNumber(Nbuffer))
        {
            std::cout << "Buffer file does not exist ...\n";
            return false;
        }
    }
    //+++ Transmission-Buffer
    double transBuffer = 1.0;
    double sigmaTransBuffer = 0.0;
    int indexTrBuffer = scriptColList.indexOf("Transmission-Buffer");
    if (indexTrBuffer < 0)
    {
        std::cout << "Transmission-Buffer column is missing ...\n";
        if (subtractBuffer)
            return false;
    }
    else
    {
        if (subtractBuffer)
        {
            lst.clear();
            lst = wScript->text(iRow, indexTrBuffer)
                      .remove(" ")
                      .remove(QChar(177))
                      .remove("\t")
                      .remove("]")
                      .split("[", Qt::SkipEmptyParts);

            transBuffer = lst[0].toDouble();
            if (transBuffer <= 0.0 || transBuffer > 2.0)
            {
                wScript->setText(iRow, indexTrBuffer, "Check!!!");
                std::cout << "Line # " << iRow + 1 << ": check buffer transmission!\n";
                return false;
            }

            if (lst.count() > 1.0)
                sigmaTransBuffer = lst[1].toDouble();

            if (transBuffer != 0.0)
                sigmaTransBuffer = fabs(sigmaTransBuffer / transBuffer);
        }
    }
    //+++ Buffer-Fraction
    double fractionBuffer = 0.0;
    int indexBufferFraction = scriptColList.indexOf("Buffer-Fraction");
    if (indexBufferFraction < 0)
    {
        std::cout << "Buffer-Fraction column is missing ...\n";
        if (subtractBuffer)
            return false;
    }
    else
    {
        if (subtractBuffer)
        {
            fractionBuffer = wScript->text(iRow, indexBufferFraction).toDouble();
            if (fractionBuffer < 0.0)
            {
                std::cout << "DAN :: Fraction of Buffer is negative :: " << iRow + 1 << "\n";
                fractionBuffer = 0.0;
            }
        }
    }
    //+++ Use sensitivity Local
    int indexUseSensBufferLocal = scriptColList.indexOf("Use-Buffer-as-Sensitivity");

    //+++ optional parameter if column "Scale" exist
    double scale = 1.0;
    int indexScale = scriptColList.indexOf("Scale");
    if (indexScale > 0)
    {
        scale = wScript->text(iRow, indexScale).toDouble();
        if (scale <= 0.0)
            scale = 1.0;
    }
    //+++ optional parameter if column "Background" exist
    double BackgroundConst = 0.0;
    int indexBackgroundConst = scriptColList.indexOf("Background");
    if (indexBackgroundConst > 0)
        BackgroundConst = wScript->text(iRow, indexBackgroundConst).toDouble();
    //+++ optional parameter if column "VShift" exist
    double VShift = 0.0;
    int indexVShift = scriptColList.indexOf("VShift");
    if (indexVShift > 0)
        VShift = wScript->text(iRow, indexVShift).toDouble();
    //+++ optional parameter if column "HShift" exist
    double HShift = 0.0;
    int indexHShift = scriptColList.indexOf("HShift");
    if (indexHShift > 0)
        HShift = wScript->text(iRow, indexHShift).toDouble();
    //+++ optional parameter if column "Suffix" exist
    QString Suffix = "";
    int indexSuffix = scriptColList.indexOf("Suffix");
    if (indexSuffix > 0)
        Suffix = wScript->text(iRow, indexSuffix).remove(" ");
    //+++ optional parameter if column "TrDet" exist
    double TrDet = 1.0;
    bool checkBoxWaTrDetChecked = checkBoxWaTrDet->isChecked();
    int indexTrDet = scriptColList.indexOf("TrDet");
    if (indexTrDet < 0)
        checkBoxWaTrDetChecked = false;
    else
    {
        TrDet = wScript->text(iRow, indexTrDet).toDouble();
        if (TrDet > 1.0)
            TrDet = 1.0;
        else if (TrDet < 0)
            TrDet = 0.0;
    }

    // Absolute Calibration factors
    abs0 *= scale;
    if (thickness != 0.0)
        abs0 /= thickness;

    double abs = abs0;
    if (trans != 0.0)
        abs /= trans;
    double absBuffer = abs0;
    if (transBuffer != 0.0)
        absBuffer /= transBuffer;

    Sample = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(Sample);
    readMatrixCor(Nsample, Sample);
    gslMatrixShift(Sample, MD, HShift, VShift);

    SampleErr = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(SampleErr);
    readErrorMatrix(Nsample, SampleErr); // (dN/N)^2
    gslMatrixShift(SampleErr, MD, HShift, VShift);

    gsl_matrix *EC = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(EC);
    gsl_matrix *ECErr = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(ECErr); // (dN/N)^2
    if (NEC != "")
    {
        readMatrixCor(NEC, EC);
        readErrorMatrix(NEC, ECErr);
    }

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
    QString maskName = wScript->text(iRow, indexMask);
    if (!make_GSL_Matrix_Symmetric(maskName, mask, MD))
    {
        std::cout << "mask matrix: " << maskName.toLatin1().constData() << " does not exist, trying to use: ";
        maskName = comboBoxMaskFor->currentText();
        std::cout << maskName.toLatin1().constData() << " instead\n";
        if (!make_GSL_Matrix_Symmetric(maskName, mask, MD))
        {
            std::cout << "mask matrix: " << maskName.toLatin1().constData()
                      << " does not exist also, mask[i,j] = 1 now\n ";
            gsl_matrix_set_all(mask, 1.0);
        }
    }

    //+++ sens gsl matrix
    gsl_matrix *sens = gsl_matrix_alloc(MD, MD);
    QString sensName = wScript->text(iRow, indexSens);
    if (!make_GSL_Matrix_Symmetric(sensName, sens, MD))
    {
        std::cout << "sensitivity matrix: " << sensName.toLatin1().constData() << " does not exist, trying to use: ";
        sensName = comboBoxSensFor->currentText();
        std::cout << sensName.toLatin1().constData() << " instead\n";
        if (!make_GSL_Matrix_Symmetric(sensName, sens, MD))
        {
            std::cout << "sensitivity matrix: " << sensName.toLatin1().constData()
                      << " does not exist also, sensitivity[i,j] = 1 now\n ";
            gsl_matrix_set_all(sens, 1.0);
            sensName = "";
        }
    }
    //+++ sens & mask sinchronization
    if (!(subtractBuffer && bufferAsSens &&
          (indexUseSensBufferLocal < 0 || wScript->text(iRow, indexUseSensBufferLocal) == "yes")))
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
    //+++
    gsl_matrix *Buffer, *BufferErr;
    if (subtractBuffer)
    {
        Buffer = gsl_matrix_calloc(MD, MD);
        BufferErr = gsl_matrix_calloc(MD, MD);
        readMatrixCor(Nbuffer, Buffer);
        readErrorMatrix(Nbuffer, BufferErr);
    }

    double err2;
    double Isample, Ibc, Iec, ItransSample;

    double Ibuffer = 0.0;
    double IecBuffer = 0.0;
    double IbcBuffer = 0.0;
    double ItransBuffer = 0.0;
    double scaleBufferData = 0.0;

    if (subtractBuffer)
        scaleBufferData = fractionBuffer * absBuffer / abs;

    for (int iii = 0; iii < MD; iii++)
        for (int jjj = 0; jjj < MD; jjj++)
        {
            //+++
            Isample = gsl_matrix_get(Sample, iii, jjj);
            err2 = Isample;
            Isample *= Isample;
            Isample *= gsl_matrix_get(SampleErr, iii, jjj);
            //+++
            Iec = gsl_matrix_get(EC, iii, jjj);
            err2 -= (trans * Iec);
            Iec *= Iec;
            Iec *= trans * trans;
            Iec *= gsl_matrix_get(ECErr, iii, jjj);
            //+++
            Ibc = gsl_matrix_get(BC, iii, jjj);
            err2 -= (1.0 - trans) * Ibc;
            Ibc *= Ibc;
            Ibc *= (1.0 - trans) * (1.0 - trans);
            Ibc *= gsl_matrix_get(BCErr, iii, jjj);

            //+++  Error of transmission
            ItransSample = gsl_matrix_get(Sample, iii, jjj) - gsl_matrix_get(BC, iii, jjj);
            ItransSample *= ItransSample;
            ItransSample *= sigmaTrans * sigmaTrans;

            if (subtractBuffer)
            {
                //+++
                Ibuffer = scaleBufferData * gsl_matrix_get(Buffer, iii, jjj);
                err2 -= Ibuffer;
                Ibuffer *= Ibuffer;
                Ibuffer *= gsl_matrix_get(BufferErr, iii, jjj);
                //+++
                IecBuffer = transBuffer * scaleBufferData * gsl_matrix_get(EC, iii, jjj);
                err2 += IecBuffer;
                IecBuffer *= IecBuffer;
                IecBuffer *= gsl_matrix_get(ECErr, iii, jjj);
                //+++
                IbcBuffer = scaleBufferData * (1.0 - transBuffer) * gsl_matrix_get(BC, iii, jjj);
                err2 += IbcBuffer;
                IbcBuffer *= IbcBuffer;
                IbcBuffer *= gsl_matrix_get(BCErr, iii, jjj);
                //+++ Error of transmission
                ItransBuffer = scaleBufferData * (gsl_matrix_get(Buffer, iii, jjj) - gsl_matrix_get(BC, iii, jjj));
                ItransBuffer *= ItransBuffer;
                ItransBuffer = ItransBuffer * sigmaTransBuffer * sigmaTransBuffer;
            }

            if (err2 != 0.0)
                err2 = 1.0 / err2;
            else
                err2 = 0.0;

            err2 *= err2;

            err2 *= (Isample + Iec + Ibc + ItransSample + Ibuffer + IecBuffer + IbcBuffer + ItransBuffer);

            err2 += gsl_matrix_get(sensErr, iii, jjj);

            gsl_matrix_set(SampleErr, iii, jjj, err2); // sigma^2
        }

    gsl_matrix_mul_elements(SampleErr, mask);

    if (checkBoxWaTrDetChecked)
        gsl_matrix_mul_elements(Sample, sensTrDet);

    //+++ Matrix-to-Matrix actions +++

    gsl_matrix_sub(Sample, BC); // Sample = Sample - BC

    //+++ BC: scale to EC
    if (NBC != "" && checkBoxBCTimeNormalizationisChecked && NEC != "")
    {
        readMatrixCorTimeNormalizationOnly(NBC, BC);
        // Normalization constant
        double TimeSample = spinBoxNorm->value();
        double ttime = monitors->readDuration(NEC);
        if (ttime > 0.0)
            TimeSample /= ttime;
        else
            TimeSample = 0.0;

        double NormSample = monitors->normalizationFactor(NEC);

        if (TimeSample > 0)
            NormSample /= TimeSample;
        else
            NormSample = 0.0;

        gsl_matrix_scale(BC, NormSample);
    }

    if (checkBoxWaTrDetChecked)
        gsl_matrix_mul_elements(EC, sensTrDet);
    gsl_matrix_sub(EC, BC); // EC = EC - BC

    if (subtractBuffer)
    {
        if (NBC != "" && checkBoxBCTimeNormalizationisChecked && NEC != "")
        {
            readMatrixCorTimeNormalizationOnly(NBC, BC);
            // Normalization constant
            double TimeSample = spinBoxNorm->value();
            double ttime = monitors->readDuration(Nbuffer);
            if (ttime > 0.0)
                TimeSample /= ttime;
            else
                TimeSample = 0.0;
            double NormSample = monitors->normalizationFactor(Nbuffer);
            if (TimeSample > 0.0)
                NormSample /= TimeSample;
            else
                NormSample = 0.0;

            gsl_matrix_scale(BC, NormSample);
        }
        if (checkBoxWaTrDetChecked)
            gsl_matrix_mul_elements(Buffer, sensTrDet);
        gsl_matrix_sub(Buffer, BC); // Buffer = Buffer - BC
    }

    double Xc, Yc;
    readCenterfromMaskName(maskName, Xc, Yc, MD);

    if (trans < 1.0 && trans > 0.0 && checkBoxParallaxTrisChecked)
    {
        double Detector = detector->readD(wScript->text(iRow, indexSample)); // [cm]
        transmissionThetaDependenceTrEC(EC, Xc, Yc, Detector, trans);
    }

    gsl_matrix_scale(EC, trans); // EC = T * EC
    if (NEC != "")
        gsl_matrix_sub(Sample, EC); // Sample = Sample  - EC

    if (subtractBuffer)
    {
        gsl_matrix_scale(EC, transBuffer / trans); // EC = Tbuffer * EC
        if (NEC != "")
            gsl_matrix_sub(Buffer, EC); // Buffer = Buffer  - EC
    }

    gsl_matrix_mul_elements(Sample, mask);

    if (subtractBuffer)
        gsl_matrix_mul_elements(Buffer, mask);

    //+++ Paralax Correction
    if (checkBoxParallaxisChecked || checkBoxParallaxTrisChecked)
    {
        double Detector = detector->readD(wScript->text(iRow, indexSample)); // [cm]
        parallaxCorrection(Sample, Xc, Yc, Detector, trans);
        if (subtractBuffer)
        {
            double Detector = detector->readD(wScript->text(iRow, indexBuffer)); // [cm]
            parallaxCorrection(Buffer, Xc, Yc, Detector, transBuffer);
        }
    }
    if (subtractBuffer)
    {
        gsl_matrix_scale(Buffer, fractionBuffer * absBuffer / abs);
        gsl_matrix_sub(Sample, Buffer);
    }

    if (comboBoxACmethod->currentIndex() == 3)
    {
        double normalization = monitors->normalizationFactor(wScript->text(iRow, indexSample));
        if (normalization > 0)
            gsl_matrix_scale(Sample, 1 / normalization);
    }
    else
        gsl_matrix_scale(Sample, abs);

    //+++ Sensitivity correction
    if (!(subtractBuffer && bufferAsSens &&
          (indexUseSensBufferLocal < 0 || wScript->text(iRow, indexUseSensBufferLocal) == "yes")))
        gsl_matrix_mul_elements(Sample, sens);
    else
    {
        double Isample, Ibuffer;
        int numberPixels = 0;
        double summBufferSens = 0;

        for (int iii = 0; iii < MD; iii++)
            for (int jjj = 0; jjj < MD; jjj++)
                if (gsl_matrix_get(mask, iii, jjj) > 0)
                {
                    numberPixels++;
                    Isample = gsl_matrix_get(Sample, iii, jjj);
                    Ibuffer = gsl_matrix_get(Buffer, iii, jjj);
                    if (Ibuffer > 0)
                        Isample /= Ibuffer;
                    summBufferSens += Ibuffer * gsl_matrix_get(sens, iii, jjj);
                    gsl_matrix_set(Sample, iii, jjj, Isample);
                }
        gsl_matrix_scale(Sample, summBufferSens / numberPixels);
    }

    //+++ subtract const bgd
    if (indexBackgroundConst > 0)
        for (int iii = 0; iii < MD; iii++)
            for (int jjj = 0; jjj < MD; jjj++)
                if (gsl_matrix_get(mask, iii, jjj) > 0)
                    gsl_matrix_set(Sample, iii, jjj, gsl_matrix_get(Sample, iii, jjj) - BackgroundConst);

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

    gsl_matrix_free(EC);
    gsl_matrix_free(ECErr);
    gsl_matrix_free(BC);
    gsl_matrix_free(BCErr);
    gsl_matrix_free(sens);
    gsl_matrix_free(sensErr);

    if (checkBoxWaTrDetChecked)
        gsl_matrix_free(sensTrDet);

    if (subtractBuffer)
    {
        gsl_matrix_free(Buffer);
        gsl_matrix_free(BufferErr);
    }

    scriptColList.clear();
    //--- Clean Memory ---

    return true;
}