/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS data analysis interface
 ******************************************************************************/

#include "dan18.h"
#include "parser-ascii.h"

//+++ connect
void dan18::sensConnectSlot()
{
    connect(toolButtonCreateSensAs, &QToolButton::clicked, this, [this](bool) { saveSensAs(); });
    connect(toolButtonSaveSens, &QToolButton::clicked, this, &dan18::saveSensFull);
    connect(toolButtonOpenSens, &QToolButton::clicked, this, [this](bool) { loadSensFull(); });
    connect(toolButtonCreateSens, &QToolButton::clicked, this, &dan18::createSens);
    connect(toolButtonAnyPlexi, &QToolButton::clicked, this, &dan18::selectAnyPlexy);
    connect(toolButtonAnyEB, &QToolButton::clicked, this, &dan18::selectAnyEB);
    connect(toolButtonAnyBC, &QToolButton::clicked, this, &dan18::selectAnyBC);
    connect(toolButtonAnyTr, &QToolButton::clicked, this, &dan18::calculateAnyTr);
    connect(pushButtonTrHiden, &QPushButton::clicked, this, &dan18::calculateTrHidden);
    connect(lineEditTransAnyD, &QLineEdit::editingFinished, this, &dan18::SensitivityLineEditCheck);
    connect(lineEditPlexiAnyD, &QLineEdit::editingFinished, this, &dan18::SensitivityLineEditCheck);
    connect(lineEditEBAnyD, &QLineEdit::editingFinished, this, &dan18::SensitivityLineEditCheck);
    connect(lineEditBcAnyD, &QLineEdit::editingFinished, this, &dan18::SensitivityLineEditCheck);
    connect(spinBoxErrLeftLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &dan18::checkErrorLimits);
    connect(spinBoxErrRightLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &dan18::checkErrorLimits);
    connect(comboBoxSensFor, QOverload<int>::of(&QComboBox::activated), this,
            [this](int) { readFileNumbersFromSensitivityMatrix(comboBoxSensFor->currentText()); });
}
//+++ create standart senssitivity
void dan18::createSens()
{
    if (!filesManager->checkFileNumber(lineEditPlexiAnyD->text()) && buttonGroupSensanyD->isChecked())
    {
        danMessage("<b>check sensitivity fields!</b>", true);
        return;
    }

    QString maskName = comboBoxMaskFor->currentText();
    if (!neededMaskExists(lineEditMD->text().toInt(), maskName, true))
        return;

    updateSensList();

    QString sensmatrixName = comboBoxSensFor->currentText();
    bool exist = app()->existWindow(sensmatrixName);

    createSensFull(sensmatrixName);
    if (spinBoxErrLeftLimit->value() > 0 || spinBoxErrRightLimit->value() < 100)
        createSensFull(sensmatrixName);
    
    if (!exist)
    {
        app()->maximizeWindow(sensmatrixName);
        if (checkBoxSortOutputToFolders->isChecked())
            app()->changeFolder("DAN :: mask, sens");
    }
}
//+++ Save mask as
void dan18::saveSensAs(QString sensName)
{
    if (sensName.isEmpty() && m_scriptRun)
    {
        danMessage("no sensitivity name given", true);
        return;
    }
    if (sensName.isEmpty())
    {
        bool ok;
        sensName = comboBoxSensFor->currentText();
        sensName = QInputDialog::getText(this, "QtiSAS", "Input matrix name for sensitivity:", QLineEdit::Normal,
                                         sensName, &ok);
        if (!ok || sensName.isEmpty())
            return;
    }

    int MD = lineEditMD->text().toInt();

    QString maskName = comboBoxMaskFor->currentText();

    if (!neededMaskExists(MD, maskName, true))
        return;

    bool exist = app()->existWindow(sensName);
    // see saveMaskAs(): a sensitivity of another detector size is made anew
    if (exist && !app()->findMatrixListByLabel("DAN::Sensitivity::").contains(sensName))
    {
        danMessage("Selected matrix is NOT CORRECT SENSITIVITY (size, label...)", true);
        return;
    }

    createSensFull(sensName);

    if (spinBoxErrLeftLimit->value() > 0 || spinBoxErrRightLimit->value() < 100)
        createSensFull(sensName);

    if (!exist)
    {
        app()->maximizeWindow(sensName);
        if (checkBoxSortOutputToFolders->isChecked())
            app()->changeFolder("DAN :: mask, sens");
    }

    updateSensList();
    comboBoxSensFor->setCurrentIndex(comboBoxSensFor->findText(sensName));
}
//+++ save Sens
void dan18::saveSensFull()
{
    QString sensName = comboBoxSensFor->currentText();
    saveSensToFile(sensName, filesManager->pathInString() + sensName + ".sens");
}
bool dan18::saveSensToFile(const QString &sensName, const QString &file)
{
    int MD = lineEditMD->text().toInt();

    gsl_matrix *sens = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(sens);

    bool ok = make_GSL_Matrix_Symmetric(sensName, sens, MD);
    if (ok)
        saveMatrixToFile(file, sens, MD);

    gsl_matrix_free(sens);
    return ok;
}
//+++ load Sens
void dan18::loadSensFull()
{
    QString Dir = filesManager->pathInString();
    QString sensName = comboBoxSensFor->currentText();
    if (m_scriptRun)
    {
        danMessage("no sensitivity file given", true);
        return;
    }
    QString sensFileName =
        QFileDialog::getOpenFileName(this, "open file dialog - Choose a Sensitivity file", Dir, "*.sens");

    if (!sensFileName.isEmpty())
        loadSensFull(sensName, sensFileName);
}

void dan18::selectAnyPlexy()
{
    QString fileNumber = "";
    if (filesManager->selectFile(fileNumber))
    {
        lineEditPlexiAnyD->setText(fileNumber);
        lineEditPlexiAnyD->setStyleSheet("background-color: rgb(128, 255, 7);");;
    }
}

void dan18::selectAnyEB()
{
    QString fileNumber = "";
    if (filesManager->selectFile(fileNumber))
    {
        lineEditEBAnyD->setText(fileNumber);
        lineEditEBAnyD->setStyleSheet("background-color: rgb(128, 255, 7);");;
    }
}

void dan18::selectAnyBC()
{
    QString fileNumber = "";
    if (filesManager->selectFile(fileNumber))
    {
        lineEditBcAnyD->setText(fileNumber);
        lineEditBcAnyD->setStyleSheet("background-color: rgb(128, 255, 7);");;
    }
}

void dan18::calculateTrHidden()
{
    if (!filesManager->checkFileNumber(lineEditPlexiAnyD->text()) ||
        !filesManager->checkFileNumber(lineEditEBAnyD->text()))
    {
        lineEditTransAnyD->setText("0.0000");
        return;
    }
    double sigmaTr = 0;
    double Tr = readTransmission(lineEditPlexiAnyD->text(), lineEditEBAnyD->text(), comboBoxMaskFor->currentText(), 0,
                                 0, sigmaTr);

    lineEditTransAnyD->setText(QString::number(Tr, 'f', 4));
}

void dan18::calculateAnyTr()
{
    if (filesManager->checkFileNumber(lineEditPlexiAnyD->text()) && checkBoxSensTr->isChecked())
    {
        QString Number = lineEditPlexiAnyD->text();
        double lambda = selector->readLambda(Number, monitors->readDuration(Number));
        lineEditTransAnyD->setText(QString::number(tCalc(lambda), 'f', 4));
        return;
    }

    if (!filesManager->checkFileNumber(lineEditPlexiAnyD->text()) ||
        !filesManager->checkFileNumber(lineEditEBAnyD->text()))
    {
        lineEditTransAnyD->setText("0.0000");
        return;
    }

    double sigmaTr = 0;
    double Tr = readTransmission(lineEditPlexiAnyD->text(), lineEditEBAnyD->text(), comboBoxMaskFor->currentText(), 0,
                                 0, sigmaTr);
    
    lineEditTransAnyD->setText(QString::number(Tr, 'f', 4));
}
//+++ Sensitivities limits
void dan18::checkErrorLimits()
{
    if (spinBoxErrLeftLimit->value() >= spinBoxErrRightLimit->value())
    {
        spinBoxErrLeftLimit->setValue(0.0);
        spinBoxErrRightLimit->setValue(100.0);
    }
}
//+++ Mask to table
void dan18::createSensFull(const QString &sensName)
{
    int MD = lineEditMD->text().toInt();
    
    if (checkBoxSortOutputToFolders->isChecked() && !neededSensExists(MD, sensName))
        app()->changeFolder("DAN :: mask, sens");

    gsl_matrix *mask = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(mask);

    gsl_matrix *sens = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(sens);

    gsl_matrix *sensErr = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(sensErr);

    QString maskName = comboBoxMaskFor->currentText();

    make_GSL_Matrix_Symmetric(maskName, mask, MD);
    gsl_matrix_memcpy(sens, mask);

    if (!buttonGroupSensanyD->isChecked() || !filesManager->checkFileNumber(lineEditPlexiAnyD->text()))
    {
        
        QString label = "DAN::Sensitivity::" + QString::number(MD) + "::Plexi::No";
        label += "::EB::No";
        label += "::BC::No";
        label += "::Tr::No";
        label += "::Center::No";

        double maskedSensValue = lineEditSensMaskedPixels->text().toDouble();
        if (maskedSensValue > 0.0)
        {
            gsl_matrix_add_constant(mask, -1.0);
            gsl_matrix_scale(mask, 0.0 - maskedSensValue);
            gsl_matrix_add(sens, mask);
        }

        makeMatrixSymmetric(sens, sensName, label, MD);

        gsl_matrix_free(mask);
        gsl_matrix_free(sens);
        gsl_matrix_free(sensErr);

        return;
    }

    if (filesManager->checkFileNumber(lineEditPlexiAnyD->text()))
    {
        double Nmask = 0;
        double sum = 0.0;
        double sum2 = 0.0;

        QString info = "\n\nSensitivity matrix: \"" + sensName + "\".\n";
        info += "Mask matrix: \"" + maskName + "\".\n\n";

        gsl_matrix *PLEXI = gsl_matrix_alloc(MD, MD);
        gsl_matrix_set_zero(PLEXI);

        gsl_matrix *EB = gsl_matrix_alloc(MD, MD);
        gsl_matrix_set_zero(EB);

        gsl_matrix *BC = gsl_matrix_alloc(MD, MD);
        gsl_matrix_set_zero(BC);

        double transmision = 0.0;

        QString PlexiFileNumber = lineEditPlexiAnyD->text();

        double D = detector->readDinM(PlexiFileNumber);

        readErrorMatrixRel(PlexiFileNumber, sensErr);
        gsl_matrix_mul_elements(sensErr, mask);

        double counts;

        if (spinBoxErrLeftLimit->value() > 0 || spinBoxErrRightLimit->value() < 100.0)
            for (int ii = 0; ii < MD; ii++)
                for (int jj = 0; jj < MD; jj++)
                    if (gsl_matrix_get(mask, ii, jj) > 0)
                    {
                        counts = gsl_matrix_get(sensErr, ii, jj) * 100;
                        if (counts < spinBoxErrLeftLimit->value() || counts > spinBoxErrRightLimit->value())
                        {
                            gsl_matrix_set(mask, ii, jj, 0);
                            gsl_matrix_set(sensErr, ii, jj, 0);
                        }
                    }

        if (filesManager->checkFileNumber(PlexiFileNumber))
        {
            readMatrixCor(PlexiFileNumber, PLEXI);
            info += "| Plexi-run-#:: " + PlexiFileNumber;
        }

        QString EBfileNumber = lineEditEBAnyD->text();
        bool EBexistYN = false;
        if (filesManager->checkFileNumber(EBfileNumber))
        {
            readMatrixCor(EBfileNumber, EB);
            info += " | EB-run-#:: " + EBfileNumber;
            EBexistYN = true;
        }
        else
            info += " | EB-run:: ...not used...";

        QString BCfileNumber = lineEditBcAnyD->text();
        if (filesManager->checkFileNumber(BCfileNumber))
        {
            if (checkBoxBCTimeNormalization->isChecked())
            {
                readMatrixCorTimeNormalizationOnly(BCfileNumber, BC);

                double TimeSample = spinBoxNorm->value();
                double ttime = monitors->readDuration(PlexiFileNumber);
                TimeSample = (ttime > 0.0) ? TimeSample / ttime : 0.0;

                double NormSample = monitors->normalizationFactor(PlexiFileNumber);
                NormSample = (TimeSample > 0) ? NormSample / TimeSample : 0;
                
                gsl_matrix_scale(BC, NormSample);
            }
            else
                readMatrixCor(BCfileNumber, BC);

            info += " | BC-run-#:: " + BCfileNumber;
        }
        else
            info += " | BC-run:: ...not used...";

        gsl_matrix_sub(PLEXI, BC);

        if (filesManager->checkFileNumber(BCfileNumber) && checkBoxBCTimeNormalization->isChecked() && EBexistYN)
        {
            readMatrixCorTimeNormalizationOnly(BCfileNumber, BC);

            double TimeSample = spinBoxNorm->value();
            double ttime = monitors->readDuration(EBfileNumber);
            TimeSample = (ttime > 0.0) ? TimeSample / ttime : 0.0;

            double NormSample = monitors->normalizationFactor(EBfileNumber);
            NormSample = (TimeSample > 0) ? NormSample / TimeSample : 0;

            gsl_matrix_scale(BC, NormSample);
        }

        gsl_matrix_sub(EB, BC);

        //+++ Transmision selection
        transmision = lineEditTransAnyD->text().toDouble();
        info += " | Tr : " + QString::number(transmision, 'f', 4);

        // the center of the wide-angle corrections (0-based here): the mask's beam stop, shown in the center fields;
        // or the fields' values when their group is checked. The reduction's corrections use the row mask's beam stop:
        // the same center cancels them for the same run (water as sample stays flat)
        double Xc, Yc;
        QString centerSource = "mask";
        if (buttonGroupSensCenter->isChecked())
        {
            Xc = doubleSpinBoxXcenterSens->value() - 1.0;
            Yc = doubleSpinBoxYcenterSens->value() - 1.0;
            centerSource = "manual";
        }
        else
        {
            readCenterfromMaskName(maskName, Xc, Yc, MD);
            doubleSpinBoxXcenterSens->setValue(Xc + 1.0);
            doubleSpinBoxYcenterSens->setValue(Yc + 1.0);
        }

        if (transmision < 1.0 && transmision > 0.0 && checkBoxParallaxTr->isChecked())
            transmissionThetaDependenceTrEC(EB, Xc, Yc, D * 100, transmision);

        gsl_matrix_scale(EB, transmision);
        if (EBexistYN)
            gsl_matrix_sub(PLEXI, EB);

        gsl_matrix_mul_elements(PLEXI, mask);

        if (checkBoxParallax->isChecked() || checkBoxParallaxTr->isChecked())
            parallaxCorrection(PLEXI, Xc, Yc, D * 100, transmision);

        double sum0 = 0;
        bool maskPixel = false;

        for (int ii = 0; ii < MD; ii++)
            for (int jj = 0; jj < MD; jj++)
            {
                counts = gsl_matrix_get(PLEXI, ii, jj);
                if (gsl_matrix_get(mask, ii, jj) > 0 && counts <= 0 && checkBoxMaskNegative->isChecked())
                {
                    gsl_matrix_set(PLEXI, ii, jj, 0.0);
                    gsl_matrix_set(mask, ii, jj, 0.0);
                    gsl_matrix_set(sens, ii, jj, 0.0);
                    counts = 0;
                    maskPixel = true;
                }
                else
                    maskPixel = false;

                if (!maskPixel)
                {
                    sum0 += counts;
                    sum2 += counts * counts;
                    sum += 1.0 / counts;

                    Nmask += gsl_matrix_get(mask, ii, jj);
                }
            }

        for (int ii = 0; ii < MD; ii++)
            for (int jj = 0; jj < MD; jj++)
                if (gsl_matrix_get(mask, ii, jj) > 0)
                {
                    counts = gsl_matrix_get(PLEXI, ii, jj);

                    if (Nmask != 0 && counts > 0)
                        gsl_matrix_set(sens, ii, jj, sum0 / Nmask / counts);
                    else
                    {
                        gsl_matrix_set(mask, ii, jj, 0.0);
                        gsl_matrix_set(sens, ii, jj, 0);
                    }
                }
        
        info += " | Number of used pixels : N : " + QString::number(Nmask) +
                "  |\n| Normalized Mean Intensity : " + QString::number(sum0 / Nmask) + " " + QChar(177) + " " +
                QString::number(sqrt(fabs(sum2 - sum0 * sum0) / Nmask) / Nmask);
        
        info += " | Normalization : " + comboBoxNorm->currentText() + " : " +
                QString::number(1.0 / monitors->normalizationFactor(lineEditPlexiAnyD->text())) + " |\n";

        info +=
            "| Dead-time correction : " + QString::number(monitors->deadTimeFactorDetector(lineEditPlexiAnyD->text())) +
            " | ";

        info += "High Q Corrections : ";
        if (checkBoxParallax->isChecked())
            info += "Yes : Center : " + QString::number(Xc + 1, 'f', 2) + "x" + QString::number(Yc + 1, 'f', 2) + " (" +
                    centerSource + ") |\n";
        else
            info += "No |\n";

        toResLog("DAN :: " + info + "\n");
        if (checkBoxSensError->isChecked())
            makeMatrixSymmetric(sensErr, sensName + "Error", "DAN::Sensitivity::Error::" + QString::number(MD), MD);
        
        
        // the pixels outside the error range masked in the mask too; its label (the parameters it was made with) kept,
        // not the mask fields' values, which may be another mask's
        QString maskLabel;
        if (auto *w = app()->window(maskName))
            maskLabel = w->windowLabel();
        if (!maskLabel.startsWith("DAN::Mask::"))
            maskLabel = this->maskLabel(false);

        makeMatrixSymmetric(mask, maskName, maskLabel, MD);
        
        QString label = "DAN::Sensitivity::" + QString::number(MD) + "::Plexi::" + lineEditPlexiAnyD->text();
        label += "::EB::" + lineEditEBAnyD->text();
        label += "::BC::" + lineEditBcAnyD->text();
        label += "::Tr::" + lineEditTransAnyD->text();
        label += "::Center::" + QString::number(Xc + 1.0, 'f', 3) + "|" + QString::number(Yc + 1.0, 'f', 3) + "|" +
                 centerSource;

        double maskedSensValue = lineEditSensMaskedPixels->text().toDouble();
        if (maskedSensValue > 0.0)
        {
            gsl_matrix_add_constant(mask, -1.0);
            gsl_matrix_scale(mask, 0.0 - maskedSensValue);
            gsl_matrix_add(sens, mask);
        }

        makeMatrixSymmetric(sens, sensName, label, MD);

        gsl_matrix_free(PLEXI);
        gsl_matrix_free(EB);
        gsl_matrix_free(BC);	
    }   
    
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    gsl_matrix_free(sensErr);
}
//+++ load Sens
void dan18::loadSensFull(const QString &sensName, const QString &sensFileName)
{
    int MD = lineEditMD->text().toInt();

    if (checkBoxSortOutputToFolders->isChecked() && !neededSensExists(MD, sensName))
        app()->changeFolder("DAN :: mask, sens");

    gsl_matrix *sens = gsl_matrix_alloc(MD, MD);

    ParserASCII::readMatrixByNameGSL(sensFileName, sens);

    for (int i = 0; i < MD; i++)
        for (int j = 0; j < MD; j++)
            if (gsl_matrix_get(sens, i, j) < 0.0)
                gsl_matrix_set(sens, i, j, 0.0);

    QString label = "DAN::Sensitivity::" + QString::number(MD) + " Sens file: " + sensFileName;

    makeMatrixSymmetric(sens, sensName, label, MD);

    gsl_matrix_free(sens);
    
    lineEditPlexiAnyD->setText(" Sens file: " + sensFileName);
    lineEditPlexiAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
    lineEditEBAnyD->setText("");
    lineEditEBAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
    lineEditBcAnyD->setText("");
    lineEditBcAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
    lineEditTransAnyD->setText(QString::number(1.0, 'f', 4));
}
//+++
void dan18::readFileNumbersFromSensitivityMatrix(const QString &name)
{
    int MD = lineEditMD->text().toInt();

    if (!app()->existWindow(name) || !neededSensExists(MD, name))
        return;

    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == name)
        {
            if (w->windowLabel().contains(" Sens file:"))
            {
                QString s = w->windowLabel();
                lineEditPlexiAnyD->setText(s.right(s.length()-s.indexOf("Sens file:")));
                lineEditPlexiAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
                lineEditEBAnyD->setText("");
                lineEditEBAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
                lineEditBcAnyD->setText("");
                lineEditBcAnyD->setStyleSheet("background-color: rgb(253, 102, 102);");
                lineEditTransAnyD->setText(QString::number(1.0, 'f', 4));
                break;
            }
            
            QString s = w->windowLabel().remove("DAN::Sensitivity::" + QString::number(MD) + "::Plexi::");
            if (s.contains("::EB::"))
            {
                lineEditPlexiAnyD->setText(s.left(s.indexOf("::EB::")));
                lineEditEBAnyD->setText(s.mid(s.indexOf("::EB::") + 6, s.indexOf("::BC::") - s.indexOf("::EB::") - 6));
                lineEditBcAnyD->setText(s.mid(s.indexOf("::BC::") + 6, s.indexOf("::Tr::") - s.indexOf("::BC::") - 6));
                lineEditTransAnyD->setText(sensitivityLabelField(name, "Tr"));
                SensitivityLineEditCheck();
                double Xc, Yc;
                QString source;
                if (sensitivityCenter(name, Xc, Yc, source))
                {
                    buttonGroupSensCenter->setChecked(source == "manual");
                    doubleSpinBoxXcenterSens->setValue(Xc);
                    doubleSpinBoxYcenterSens->setValue(Yc);
                }
            }
            else
                lineEditPlexiAnyD->setText(s);

            break;
        }
}
//+++ SENSITIVITY-enable-check
void dan18::SensitivityLineEditCheck()
{
    double change = lineEditTransAnyD->text().toDouble();

    lineEditTransAnyD->setText(QString::number((change < 0.0 || change > 1.0) ? 0.0 : change, 'f', 4));

    //Plexi
    lineEditPlexiAnyD->setStyleSheet(filesManager->checkFileNumber(lineEditPlexiAnyD->text())
                                         ? "background-color: rgb(128, 255, 7);"
                                         : "background-color: rgb(253, 102, 102);");
    //EB
    lineEditEBAnyD->setStyleSheet(filesManager->checkFileNumber(lineEditEBAnyD->text())
                                      ? "background-color: rgb(128, 255, 7);"
                                      : "background-color: rgb(253, 102, 102);");
    //BC
    lineEditBcAnyD->setStyleSheet(filesManager->checkFileNumber(lineEditBcAnyD->text())
                                      ? "background-color: rgb(128, 255, 7);"
                                      : "background-color: rgb(253, 102, 102);");
}

bool dan18::neededSensExists(int MD, const QString &sensName, bool showErrorMessage)
{
    QStringList lst = app()->findMatrixListByLabel("DAN::Sensitivity::" + QString::number(MD));
    if (!lst.contains(sensName))
    {
        if (showErrorMessage)
            danMessage("<b>" + sensName + "</b> does not exist!", true);
        return false;
    }
    return true;
}

void dan18::updateSensList()
{
    int MD = lineEditMD->text().toInt();

    QStringList lst = app()->findMatrixListByLabel("DAN::Sensitivity::" + QString::number(MD));
    if (!lst.contains("sens"))
        lst.prepend("sens");

    QString currentSens = comboBoxSensFor->currentText();
    comboBoxSensFor->clear();
    comboBoxSensFor->addItems(lst);
    if (lst.contains(currentSens))
        comboBoxSensFor->setCurrentIndex(lst.indexOf(currentSens));

    updateMatrixListInConfigurations(dptSENS, lst); // see updateMaskList()
}
//+++  get-Sensitivity-Number
QString dan18::getSensitivityNumber(const QString &sensName)
{
    int MD = lineEditMD->text().toInt();

    if (!app()->existWindow(sensName) || !neededSensExists(MD, sensName))
        return "";

    QString res;
    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == sensName)
        {
            if (w->windowLabel().contains("Sens file:"))
                return "";

            QString s = w->windowLabel().remove("DAN::Sensitivity::" + QString::number(MD) + "::Plexi::");
            if (s.contains("::EB::"))
                s = s.left(s.indexOf("::EB::"));
            return s;
        }

    return "";
}
//+++ a field of the sensitivity's label
QString dan18::sensitivityLabelField(const QString &sensName, const QString &key)
{
    auto *w = app()->window(sensName);
    if (!w)
        return {};
    const QString label = w->windowLabel();
    const QString tag = "::" + key + "::";
    const int start = static_cast<int>(label.indexOf(tag));
    if (!label.startsWith("DAN::Sensitivity::") || start < 0)
        return {};
    const int from = start + static_cast<int>(tag.length());
    const int end = static_cast<int>(label.indexOf("::", from));
    return (end < 0 ? label.mid(from) : label.mid(from, end - from)).trimmed();
}
//+++ the center the sensitivity was made with
bool dan18::sensitivityCenter(const QString &sensName, double &Xc, double &Yc, QString &source)
{
    const QStringList lst = sensitivityLabelField(sensName, "Center").split("|");
    if (lst.count() < 2)
        return false;
    bool okX, okY;
    Xc = lst[0].toDouble(&okX);
    Yc = lst[1].toDouble(&okY);
    source = lst.count() > 2 ? lst[2] : "mask";
    return okX && okY;
}
//+++ a row against its sensitivity
void dan18::checkRowAgainstSensitivity(int iRow, const QString &sensName, const QString &run, double trans, double Xc,
                                       double Yc)
{
    const QString line = "Line # " + QString::number(iRow + 1) + ": sensitivity \"" + sensName + "\" ";

    // the sensitivity's own run with another transmission: its EB subtraction differs, I(Q) is not flat
    bool ok;
    const double trSens = sensitivityLabelField(sensName, "Tr").toDouble(&ok);
    if (ok && trSens > 0 && run == getSensitivityNumber(sensName) && std::fabs(trSens - trans) > 0.5e-4)
        danWarning(line + "is made of the same run " + run + " with Tr = " + QString::number(trSens, 'f', 4) +
                   ", the row has T = " + QString::number(trans, 'f', 4));

    // wide-angle corrections with another center than the sensitivity's
    double XcSens, YcSens;
    QString source;
    if ((checkBoxParallax->isChecked() || checkBoxParallaxTr->isChecked()) &&
        sensitivityCenter(sensName, XcSens, YcSens, source) &&
        std::hypot(XcSens - (Xc + 1.0), YcSens - (Yc + 1.0)) > 0.5)
        danWarning(line + "is made with the center " + QString::number(XcSens, 'f', 2) + "|" +
                   QString::number(YcSens, 'f', 2) + " (" + source + "), the row's wide-angle corrections use " +
                   QString::number(Xc + 1.0, 'f', 2) + "|" + QString::number(Yc + 1.0, 'f', 2) +
                   " (its mask's beam stop)");
}
