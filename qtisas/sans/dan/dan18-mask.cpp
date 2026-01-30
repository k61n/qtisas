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

//+++ connect
void dan18::maskConnectSlot()
{
    connect(toolButtonCreateMask, &QToolButton::clicked, this, [this](bool) { createMask(false); });
    connect(lineEditDeadRows, &QLineEdit::returnPressed, this, [this]() { createMask(false); });
    connect(lineEditDeadCols, &QLineEdit::returnPressed, this, [this]() { createMask(false); });
    connect(lineEditMaskPolygons, &QLineEdit::returnPressed, this, [this]() { createMask(false); });
    connect(toolButtonSaveMaskTr, &QToolButton::clicked, this, [this](bool) { createMask(true); });
    connect(toolButtonCreateMaskAs, &QToolButton::clicked, this, [this](bool) { saveMaskAs(); });
    connect(toolButtonSaveMask, &QToolButton::clicked, this, &dan18::saveMaskFull);
    connect(toolButtonOpenMask, &QToolButton::clicked, this, [this](bool) { loadMaskFull(); });
    connect(pushButtonGetCoord1, &QPushButton::clicked, this, &dan18::readCoord1);
    connect(pushButtonGetCoord2, &QPushButton::clicked, this, &dan18::readCoord2);
    connect(pushButtonGetCoord3, &QPushButton::clicked, this, &dan18::readCoord3);
    connect(pushButtonGetCoord4, &QPushButton::clicked, this, &dan18::readCoord4);
    connect(toolButtonPlusMaskBS, &QToolButton::clicked, this, &dan18::maskPlusMaskBS);
    connect(pushButtonGetCoordDRows, &QPushButton::clicked, this, &dan18::readCoordDRows);
    connect(pushButtonGetCoordDCols, &QPushButton::clicked, this, &dan18::readCoordDCols);
    connect(pushButtonGetCoordTrian, &QPushButton::clicked, this, &dan18::readCoordTriangle);
    connect(comboBoxMaskFor, &QComboBox::textActivated, this, &dan18::readMaskParaFromMaskMatrix);
    connect(pushButtonReadDisplay, &QPushButton::clicked, this, &dan18::readCoorMaskTools);
    connect(toolBoxMask, &QTabWidget::currentChanged, this, [this](int) { matrixList(); });
    connect(comboBoxMlistMask, &QComboBox::textActivated, this, &dan18::matrixList);
    connect(radioButtonToolActiveMask, &QRadioButton::clicked, this, [this](bool) { matrixList(); });
    connect(radioButtonToolGraph, &QRadioButton::clicked, this, [this](bool) { matrixList(); });
    connect(radioButtonSelectMatrix, &QRadioButton::clicked, this, [this](bool) { matrixList(); });
    connect(toolButtonUpdateMatrixActive, &QToolButton::clicked, this, &dan18::updateMatixActive);
    connect(toolButtonSetValue, &QPushButton::clicked, this, &dan18::maskSetValue);
    connect(toolButtonSquared, &QToolButton::clicked, this, &dan18::maskRectangular);
    connect(toolButtonMaskSector, &QToolButton::clicked, this, &dan18::maskSector);
    connect(toolButtonEllipse, &QToolButton::clicked, this, &dan18::maskEllipse);
    connect(toolButtonEllShell, &QToolButton::clicked, this, &dan18::maskEllipseShell);
}
//+++ Save mask as
void dan18::saveMaskAs(QString maskName)
{
    updateMaskList();

    bool ok;

    if (maskName.isEmpty())
    {
        maskName = comboBoxMaskFor->currentText();
        maskName =
            QInputDialog::getText(this, "QtiSAS", "Input matrix name for mask:", QLineEdit::Normal, maskName, &ok);
        if (!ok || maskName.isEmpty())
            return;
    }

    int MD = lineEditMD->text().toInt();

    bool exist = app()->existWindow(maskName);

    if (exist && !neededMaskExists(MD, maskName))
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Selected matrix is NOT CORRECT MASK (size, label...)");
        return;
    }

    createMaskFull(maskName);

    if (!exist && checkBoxSortOutputToFolders->isChecked())
        app()->changeFolder("DAN :: mask, sens");

    updateMaskList();

    comboBoxMaskFor->setCurrentIndex(comboBoxMaskFor->findText(maskName));
}
//+++ create standart mask
void dan18::createMask(bool Tr)
{
    updateMaskList();
    QString maskName = comboBoxMaskFor->currentText();

    (Tr) ? createMaskFullTr(maskName) : createMaskFull(maskName);

    if (!app()->existWindow(maskName) && checkBoxSortOutputToFolders->isChecked())
        app()->changeFolder("DAN :: mask, sens");
}
//+++ save MASK to file
void dan18::saveMaskFull()
{
    QString Dir = filesManager->pathInString();
    int MD = lineEditMD->text().toInt();
    QString maskName = comboBoxMaskFor->currentText();

    gsl_matrix *mask = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(mask);

    if (make_GSL_Matrix_Symmetric(maskName, mask, MD))
        saveMatrixToFile(Dir + maskName + ".mask", mask, MD);

    gsl_matrix_free(mask);
}
//+++ load MASK
void dan18::loadMaskFull()
{
    QString Dir = filesManager->pathInString();
    QString maskName = comboBoxMaskFor->currentText();
    QString maskFileName = QFileDialog::getOpenFileName(this, "open file dialog - Choose a Mask file", Dir, "*.mask");

    if (!maskFileName.isEmpty())
        loadMaskFull(maskName, maskFileName);
}
void dan18::readCoord1()
{
    double x,y;
    if (!readDisplay(x, y))
        return;

    spinBoxLTx->setValue(Qwt3D::round(x));
    spinBoxLTy->setValue(Qwt3D::round(y));
}
void dan18::readCoord2()
{
    double x,y;
    readDisplay(x, y);
    if (!readDisplay(x, y))
        return;

    spinBoxRBx->setValue(Qwt3D::round(x));
    spinBoxRBy->setValue(Qwt3D::round(y));
}
void dan18::readCoord3()
{
    double x,y;
    readDisplay(x, y);
    if (!readDisplay(x, y))
        return;

    spinBoxLTxBS->setValue(Qwt3D::round(x));
    spinBoxLTyBS->setValue(Qwt3D::round(y));
}
void dan18::readCoord4()
{
    double x,y;
    readDisplay(x, y);
    if (!readDisplay(x, y))
        return;

    spinBoxRBxBS->setValue(Qwt3D::round(x));
    spinBoxRByBS->setValue(Qwt3D::round(y));
}
//+++
void dan18::maskPlusMaskBS()
{
    updateMaskList();

    QString maskName = comboBoxMaskFor->currentText();
    if (!app()->existWindow(maskName))
        return;

    addBS2CurrentMask(maskName);
}
//+++ Mask to table
void dan18::createMaskFull(const QString &maskName)
{
    int MD = lineEditMD->text().toInt();
    
    if (checkBoxSortOutputToFolders->isChecked() && !neededMaskExists(MD, maskName))
        app()->changeFolder("DAN :: mask, sens");
    
    QString maskRange;
    maskRange = "|" + QString::number(spinBoxLTx->value());
    maskRange += "|" + QString::number(spinBoxRBx->value());
    maskRange += "|" + QString::number(spinBoxLTy->value());
    maskRange += "|" + QString::number(spinBoxRBy->value());
    maskRange += "|" + QString::number(spinBoxLTxBS->value());
    maskRange += "|" + QString::number(spinBoxRBxBS->value());
    maskRange += "|" + QString::number(spinBoxLTyBS->value());
    maskRange += "|" + QString::number(spinBoxRByBS->value());
    maskRange += "| " + lineEditDeadRows->text();
    maskRange += "| " + lineEditDeadCols->text();
    maskRange += "| " + lineEditMaskPolygons->text();
    maskRange += "|";

    Matrix *m;
    bool matrixExists = false;

    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == maskName)
        {
            m = (Matrix *)w;
            matrixExists = true;
            m->setDimensions(MD, MD);
            break;
        }

    if (!matrixExists)
        m = app()->newMatrix(maskName, MD, MD);

    m->setNumericFormat('f', 0);
    m->setCoordinates(1, MD, 1, MD);

    QString label = "DAN::Mask::" + QString::number(MD) + maskRange;
    m->setWindowLabel(label);
    app()->setListViewLabel(maskName, label);
    
    for (int i = 0; i < MD; i++)
        for (int j = 0; j < MD; j++)
            m->setText(i, j, "1");

    double r;

    int LTy = spinBoxLTy->value() - 1;
    int RBy = spinBoxRBy->value() - 1;
    int LTx = spinBoxLTx->value() - 1;
    int RBx = spinBoxRBx->value() - 1;

    int LTyBS = spinBoxLTyBS->value() - 1;
    int RByBS = spinBoxRByBS->value() - 1;
    int LTxBS = spinBoxLTxBS->value() - 1;
    int RBxBS = spinBoxRBxBS->value() - 1;

    if (groupBoxMask->isChecked())
    {
        if (comboBoxMaskEdgeShape->currentIndex() == 0)
        {
            for (int i = 0; i < MD; i++)
                for (int j = 0; j < MD; j++)
                    if (i < LTy || i > RBy || j < LTx || j > RBx)
                        m->setText(i, j, "0");
        }
        else
        {
            double xCenter = (LTx + RBx) / 2.0;
            double yCenter = (LTy + RBy) / 2.0;
            double xSize = (RBx - LTx) + 2.0;
            double ySize = (RBy - LTy) + 2.0;

            for (int xx = 0; xx < MD; xx++)
                for (int yy = 0; yy < MD; yy++)
                {
                    r = (xCenter - xx) * (xCenter - xx) * 4.0 / double(xSize * xSize);
                    r += (yCenter - yy) * (yCenter - yy) * 4.0 / double(ySize * ySize);
                    if (r > 1.0)
                        m->setText(yy, xx, "0");
                }
        }
    }
    if (groupBoxMaskBS->isChecked())
    {
        if (comboBoxMaskBeamstopShape->currentIndex() == 0)
        {
            for (int i = 0; i < MD; i++)
                for (int j = 0; j < MD; j++)
                    if (i >= LTyBS && i <= RByBS && j >= LTxBS && j <= RBxBS)
                        m->setText(i, j, "0");
        }
        else
        {
            double xCenter = (LTxBS + RBxBS) / 2.0;
            double yCenter = (LTyBS + RByBS) / 2.0;
            double xSize = (RBxBS - LTxBS) + 0.5;
            double ySize = (RByBS - LTyBS) + 0.5;

            for (int xx = 0; xx < MD; xx++)
                for (int yy = 0; yy < MD; yy++)
                {
                    r = (xCenter - xx) * (xCenter - xx) * 4.0 / double(xSize * xSize);
                    r += (yCenter - yy) * (yCenter - yy) * 4.0 / double(ySize * ySize);
                    if (r <= 1)
                        m->setText(yy, xx, "0");
                }
        }
    }

    QString sDeadRows = lineEditDeadRows->text();

    if (!sDeadRows.isEmpty())
    {
        QStringList lst = sDeadRows.split(";", Qt::SkipEmptyParts);

        for (int li = 0; li < lst.count(); li++)
        {
            QStringList lstMin = lst[li].split("-", Qt::SkipEmptyParts);

            if (lstMin.count() == 2)
            {
                if (lstMin[0].toInt() > 0 && lstMin[1].toInt() > 0 && lstMin[0].toInt() <= MD &&
                    lstMin[1].toInt() <= MD && lstMin[0].toInt() <= lstMin[1].toInt())
                    for (int raw = lstMin[0].toInt() - 1; raw <= lstMin[1].toInt() - 1; raw++)
                        for (int yy = 0; yy < MD; yy++)
                            m->setText(raw, yy, "0");
            }
            else if (lstMin.count() == 4 && lstMin[0].toInt() > 0 && lstMin[1].toInt() > 0 && lstMin[0].toInt() <= MD &&
                     lstMin[1].toInt() <= MD && lstMin[0].toInt() <= lstMin[1].toInt())
            {
                int start = 0;
                int finish = MD - 1;

                if (lstMin[2].toInt() <= MD && lstMin[2].toInt() > 0)
                    start = lstMin[2].toInt() - 1;
                if (lstMin[3].toInt() <= MD && lstMin[3].toInt() > 0)
                    finish = lstMin[3].toInt() - 1;
                if (start > finish)
                    std::swap(start, finish);
                
                for (int raw = lstMin[0].toInt() - 1; raw <= lstMin[1].toInt() - 1; raw++)
                    for (int yy = start; yy <= finish; yy++)
                        m->setText(raw, yy, "0");
            }
        }
    }

    QString sDeadCols = lineEditDeadCols->text();
    
    if (!sDeadCols.isEmpty())
    {
        QStringList lst = sDeadCols.split(";", Qt::SkipEmptyParts);
        for (int li = 0; li < lst.count(); li++)
        {
            QStringList lstMin = lst[li].split("-", Qt::SkipEmptyParts);

            if (lstMin.count() == 2)
            {
                if (lstMin[0].toInt() > 0 && lstMin[1].toInt() > 0 && lstMin[0].toInt() <= MD &&
                    lstMin[1].toInt() <= MD && lstMin[0].toInt() <= lstMin[1].toInt())
                    for (int yy = 0; yy < MD; yy++)
                        for (int col = lstMin[0].toInt() - 1; col <= lstMin[1].toInt() - 1; col++)
                            m->setText(yy, col, "0");
            }
            else if (lstMin.count() == 4)
            {
                if (lstMin[0].toInt() > 0 && lstMin[1].toInt() > 0 && lstMin[0].toInt() <= MD &&
                    lstMin[1].toInt() <= MD && lstMin[0].toInt() <= lstMin[1].toInt())
                {
                    int start = 0;
                    int finish = MD - 1;

                    if (lstMin[2].toInt() <= MD && lstMin[2].toInt() > 0)
                        start = lstMin[2].toInt() - 1;
                    if (lstMin[3].toInt() <= MD && lstMin[3].toInt() > 0)
                        finish = lstMin[3].toInt() - 1;
                    if (start > finish)
                        std::swap(start, finish);

                    for (int yy = start; yy <= finish; yy++)
                        for (int col = lstMin[0].toInt() - 1; col <= lstMin[1].toInt() - 1; col++)
                            m->setText(yy, col, "0");
                }
            }
        }
    }

    maskTriangle(m, MD);

    m->setColumnsWidth(25);
    m->resetView();

    toResLog("DAN :: \nMask template is created: \"" + maskName + "\". \nEdge: " + QString::number(++LTx) + " | " +
             QString::number(++RBx) + " | " + QString::number(++LTy) + " | " + QString::number(++RBy) +
             " and Beam-Stop: " + QString::number(++LTxBS) + " | " + QString::number(++RBxBS) + " | " +
             QString::number(++LTyBS) + " | " + QString::number(++RByBS) + ".\n");
    

    m->notifyModifiedData();
    if (!matrixExists)
    {
        m->hide();
        m->showMaximized();
    }

    app()->updateWindowLists(m);
}
//+++ Mask to table Tr
void dan18::createMaskFullTr(const QString &maskName)
{
    int MD = lineEditMD->text().toInt();

    bool cfbool = checkBoxSortOutputToFolders->isChecked() && !neededMaskExists(MD, maskName);

    if (cfbool)
        app()->changeFolder("DAN :: mask, sens");

    QString maskRange;
    maskRange = "|" + QString::number(spinBoxLTx->value());
    maskRange += "|" + QString::number(spinBoxRBx->value());
    maskRange += "|" + QString::number(spinBoxLTy->value());
    maskRange += "|" + QString::number(spinBoxRBy->value());
    maskRange += "|" + QString::number(spinBoxLTxBS->value());
    maskRange += "|" + QString::number(spinBoxRBxBS->value());
    maskRange += "|" + QString::number(spinBoxLTyBS->value());
    maskRange += "|" + QString::number(spinBoxRByBS->value());
    maskRange += "| " + lineEditDeadRows->text();
    maskRange += "| " + lineEditDeadCols->text();
    maskRange += "| " + lineEditMaskPolygons->text();
    maskRange += "|";

    Matrix *m;
    bool matrixExists = false;

    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == maskName)
        {
            m = (Matrix *)w;
            matrixExists = true;
            m->setDimensions(MD, MD);
            break;
        }

    if (!matrixExists)
        m = app()->newMatrix(maskName, MD, MD);

    m->setNumericFormat('f', 0);
    m->setCoordinates(1, MD, 1, MD);

    QString label = "DAN::Mask::" + QString::number(MD) + maskRange;
    m->setWindowLabel(label);
    app()->setListViewLabel(maskName, label);

    for (int i = 0; i < MD; i++)
        for (int j = 0; j < MD; j++)
            m->setText(i, j, "0");

    double r;

    int LTy = spinBoxLTy->value() - 1;
    int RBy = spinBoxRBy->value() - 1;
    int LTx = spinBoxLTx->value() - 1;
    int RBx = spinBoxRBx->value() - 1;

    int LTyBS = spinBoxLTyBS->value() - 1;
    int RByBS = spinBoxRByBS->value() - 1;
    int LTxBS = spinBoxLTxBS->value() - 1;
    int RBxBS = spinBoxRBxBS->value() - 1;

    if (groupBoxMaskBS->isChecked())
    {
        if (comboBoxMaskBeamstopShape->currentIndex() == 0)
        {
            for (int i = 0; i < MD; i++)
                for (int j = 0; j < MD; j++)
                    if (i >= LTyBS && i <= RByBS && j >= LTxBS && j <= RBxBS)
                        m->setText(i, j, "1");
        }
        else
        {
            double xCenter = (LTxBS + RBxBS) / 2.0;
            double yCenter = (LTyBS + RByBS) / 2.0;
            double xSize = (RBxBS - LTxBS) + 0.5;
            double ySize = (RByBS - LTyBS) + 0.5;

            for (int xx = 0; xx < MD; xx++)
                for (int yy = 0; yy < MD; yy++)
                {
                    r = (xCenter - xx) * (xCenter - xx) * 4.0 / double(xSize * xSize);
                    r += (yCenter - yy) * (yCenter - yy) * 4.0 / double(ySize * ySize);
                    if (r <= 1.0)
                        m->setText(yy, xx, "1");
                }
        }
    }

    m->setColumnsWidth(25);
    m->resetView();

    toResLog("DAN :: \nMask template is created: \"" + maskName + "\". \nEdge: " + QString::number(++LTx) + " | " +
             QString::number(++RBx) + " | " + QString::number(++LTy) + " | " + QString::number(++RBy) +
             " and Beam-Stop: " + QString::number(++LTxBS) + " | " + QString::number(++RBxBS) + " | " +
             QString::number(++LTyBS) + " | " + QString::number(++RByBS) + ".\n");

    if (cfbool)
        app()->changeFolder("DAN :: mask, sens");

    m->notifyModifiedData();
}
//+++ load MASK
void dan18::loadMaskFull(const QString &maskName, const QString &maskFileName)
{
    int MD = lineEditMD->text().toInt();

    if (checkBoxSortOutputToFolders->isChecked() && !neededMaskExists(MD, maskName))
        app()->changeFolder("DAN :: mask, sens");
    
    gsl_matrix *mask = gsl_matrix_alloc(MD, MD);
    gsl_matrix_set_zero(mask);

    ParserASCII::readMatrixByNameGSL(maskFileName, mask);

    for (int i = 0; i < MD; i++)
        for (int j = 0; j < MD; j++)
            if (gsl_matrix_get(mask, i, j) > 0.0)
                gsl_matrix_set(mask, i, j, 1.0);
            else
                gsl_matrix_set(mask, i, j, 0.0);

    QString label = "DAN::Mask::" + QString::number(MD) + " Mask file: " + maskFileName;
    makeMatrixSymmetric(mask, maskName, label, MD);
    
    toResLog("DAN :: \nMask template is created: \"" + maskName + "\"." + " Mask file: " + maskFileName + ".\n");

    gsl_matrix_free(mask);
}

void dan18::addBS2CurrentMask(const QString &maskName)
{
    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == comboBoxMlistMask->currentText())
        {
            auto *m = (Matrix *)w;

            int LTyBS = spinBoxLTyBS->value() - 1;
            int RByBS = spinBoxRByBS->value() - 1;
            int LTxBS = spinBoxLTxBS->value() - 1;
            int RBxBS = spinBoxRBxBS->value() - 1;

            double xCenter = (LTxBS + RBxBS) / 2.0;
            double yCenter = (LTyBS + RByBS) / 2.0;
            double xSize = (RBxBS - LTxBS) + 0.5;
            double ySize = (RByBS - LTyBS) + 0.5;

            double r;

            if (comboBoxMaskBeamstopShape->currentIndex() == 0)
            {
                for (int i = LTyBS; i <= RByBS; i++)
                    for (int j = LTxBS; j <= RBxBS; j++)
                        m->setText(i, j, "0");
            }
            else
            {
                for (int yy = LTyBS; yy <= RByBS; yy++)
                    for (int xx = LTxBS; xx <= RBxBS; xx++)
                    {
                        r = (xCenter - xx) * (xCenter - xx) * 4 / xSize / xSize;
                        r += (yCenter - yy) * (yCenter - yy) * 4 / ySize / ySize;
                        if (r <= 1)
                            m->setText(yy, xx, "0");
                    }
            }

            m->notifyModifiedData();
            return;
        }
}

void dan18::readCoordDRows()
{
    double x, y;
    if (!readDisplay(x, y))
        return;
    
    QString currentStr = lineEditDeadRows->text().remove(" ");
    currentStr +=
        (currentStr.right(1) == "-" ? QString::number(Qwt3D::round(y)) + ";" : QString::number(Qwt3D::round(y)) + "-");

    lineEditDeadRows->setText(currentStr);
}

void dan18::readCoordDCols()
{
    double x, y;
    if (!readDisplay(x, y))
        return;

    QString currentStr = lineEditDeadRows->text().remove(" ");

    currentStr +=
        (currentStr.right(1) == "-" ? QString::number(Qwt3D::round(x)) + ";" : QString::number(Qwt3D::round(x)) + "-");

    lineEditDeadCols->setText(currentStr);
}

void dan18::readCoordTriangle()
{
    double x, y;
    if (!readDisplay(x, y))
        return;

    QString currentStr = lineEditDeadRows->text().remove(" ");

    if (int(currentStr.count(",") / 2) * 2 != currentStr.count(",") || currentStr.count(",") == 0 ||
        currentStr.right(1) == ";")
        currentStr += QString::number(Qwt3D::round(x)) + "-" + QString::number(Qwt3D::round(y)) + ",";
    else
        currentStr += QString::number(Qwt3D::round(x)) + "-" + QString::number(Qwt3D::round(y)) + ";";

    lineEditMaskPolygons->setText(currentStr);
}

bool dan18::maskTriangle(Matrix *m, int md)
{
    QString sMask = lineEditMaskPolygons->text();

    if (!sMask.isEmpty())
    {
        QStringList lst = sMask.split(";", Qt::SkipEmptyParts);

        auto *pointListX = new int[3];
        auto *pointListY = new int[3];
        
        for (int li = 0; li < lst.count(); li++)
        {
            QStringList lstPoint = lst[li].split(",", Qt::SkipEmptyParts);
            if (lstPoint.count() != 3)
                continue;
            
            for (int pi = 0; pi < lstPoint.count(); pi++)
            {
                QStringList lstXY = lstPoint[pi].split("-", Qt::SkipEmptyParts);
                pointListX[pi] = lstXY[0].toInt();
                pointListY[pi] = lstXY[1].toInt();
            }

            maskTriangle(m, md, pointListX[0], pointListY[0], pointListX[1], pointListY[1], pointListX[2], pointListY[2]);
            
        }
        delete[] pointListX;
        delete[] pointListY;
    }
    return true;
}
//+++ mask triangle
bool dan18::maskTriangle(Matrix *m, int md, int x1, int y1, int x2, int y2, int x3, int y3)
{
    if (md <= 0)
        return false;
    
    if (x1 > md || x2 > md || x3 > md || y1 > md || y2 > md || y3 > md)
        return false;

    if (x1 < 1 || x2 < 1 || x3 < 1 || y1 < 1 || y2 < 1 || y3 < 1)
        return false;

    if ((x1 == x2 && y1 == y2) || (x1 == x3 && y1 == y3) || (x3 == x2 && y3 == y2))
        return false;

    x1--; y1--;
    x2--; y2--;
    x3--; y3--;
    
    for (int ix = 0; ix < md; ix++)
        for (int iy = 0; iy < md; iy++)
        {
            double a = (x1 - ix) * (y2 - y1) - (x2 - x1) * (y1 - iy);
            double b = (x2 - ix) * (y3 - y2) - (x3 - x2) * (y2 - iy);
            double c = (x3 - ix) * (y1 - y3) - (x1 - x3) * (y3 - iy);

            if ((a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0))
                m->setText(iy, ix, "0");
        }
    return true;    
}
//+++
void dan18::readMaskParaFromMaskMatrix(const QString &name)
{
    int MD = lineEditMD->text().toInt();
    QString label = "DAN::Mask::" + QString::number(MD);

    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == name && w->windowLabel().contains(label))
        {
            QString s = w->windowLabel().remove(label);
            if ( s.contains("|") )
            {
                s = s.simplified();
                QStringList lst = s.split("|", Qt::SkipEmptyParts);
                if (lst.count() < 8)
                    return;

                spinBoxLTx->setValue(lst[0].toInt());
                spinBoxRBx->setValue(lst[1].toInt());
                spinBoxLTy->setValue(lst[2].toInt());
                spinBoxRBy->setValue(lst[3].toInt());
                spinBoxLTxBS->setValue(lst[4].toInt());
                spinBoxRBxBS->setValue(lst[5].toInt());
                spinBoxLTyBS->setValue(lst[6].toInt());
                spinBoxRByBS->setValue(lst[7].toInt());

                if (lst.count() >= 11)
                {
                    lineEditDeadRows->setText(lst[8]);
                    lineEditDeadCols->setText(lst[9]);
                    lineEditMaskPolygons->setText(lst[10]);
                }
            }
            return;
        }
}

bool dan18::readDisplay(double &x, double &y)
{
    QString info = app()->info->text();

    QStringList lst = info.split(";", Qt::SkipEmptyParts);

    if (lst.count() < 2)
        return false;

    QString xs = lst[0].remove("x=").remove(" ");
    xs = QString::number(xs.toDouble(), 'f', 3);
    x = xs.toDouble();

    QString ys = lst[1].remove("y=").remove(" ");
    ys = QString::number(ys.toDouble(), 'f', 3);
    y = ys.toDouble();

    return true;
}

bool dan18::neededMaskExists(int MD, const QString &maskName, bool showErrorMessage)
{
    QStringList lst = app()->findMatrixListByLabel("DAN::Mask::" + QString::number(MD));
    if (!lst.contains(maskName))
    {
        if (showErrorMessage)
            QMessageBox::critical(nullptr, "QtiSAS", "<b>" + maskName + "</b> does not exist!");
        return false;
    }
    return true;
}

void dan18::updateMaskList()
{
    int MD = lineEditMD->text().toInt();

    QStringList lst = app()->findMatrixListByLabel("DAN::Mask::" + QString::number(MD));
    if (!lst.contains("mask"))
        lst.prepend("mask");

    QString currentMask = comboBoxMaskFor->currentText();

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

void dan18::matrixList(const QString &selectedName)
{
    if (toolBoxMask->currentIndex() == 0)
        return;
 
    if (!selectedName.isEmpty())
        toolButtonUpdateMatrixActive->setVisible(selectedName == "Matrix-Active");

    QStringList lst;
    foreach (MdiSubWindow *w, app()->matrixList())
        lst << w->name();

    QString currentMask;
    currentMask = comboBoxMlistMask->currentText();

    if (!selectedName.isEmpty() && lst.contains(selectedName))
        currentMask = selectedName;

    if (selectedName.isEmpty() && radioButtonToolActiveMask->isChecked() &&
        lst.contains(comboBoxMaskFor->currentText()))
        currentMask = comboBoxMaskFor->currentText();

    if (selectedName.isEmpty() && radioButtonToolGraph->isChecked())
    {
        auto *plot = (MultiLayer *)app()->activeWindow();
        if (plot)
        {
            auto *g = (Graph *)plot->activeLayer();
            if (g && g->curvesList().count() > 0 && g->curvesList()[0]->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
            {
                auto *sp = (Spectrogram *)g->curvesList()[0];
                QString sTmp = sp->matrix()->name();
                if (lst.contains(sTmp))
                    currentMask = sTmp;
            }
        }
    }

    comboBoxMlistMask->clear();
    comboBoxMlistMask->addItems(lst);
    if (lst.contains(currentMask))
        comboBoxMlistMask->setItemText(comboBoxMlistMask->currentIndex(), currentMask);

    toolButtonUpdateMatrixActive->setVisible(comboBoxMlistMask->currentText() == "Matrix-Active");
}
//+++ fill matrix
void dan18::maskSetValue()
{
    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == comboBoxMlistMask->currentText())
        {
            auto *m = (Matrix *)w;
            double newValue = comboBoxValue->currentText().toDouble();

            for (int xx = 0; xx < m->numCols(); xx++)
                for (int yy = 0; yy < m->numRows(); yy++)
                    m->setText(yy, xx, QString::number(newValue));

            m->notifyModifiedData();
            return;
        }
}

void dan18::updateMatixActive()
{
    QString Number = lineEditCheck->text().simplified();

    if(Number.contains(";"))
    {
        QStringList lstNumberIn = Number.split(";", Qt::SkipEmptyParts);
        Number = lstNumberIn[0];
    }

    check(Number, false, "View Matrix [Matrix-Active]");
}
//+++ mask rectangular
void dan18::maskRectangular()
{
    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == comboBoxMlistMask->currentText())
        {
            auto *m = (Matrix *)w;
            
            double xCenter = doubleSpinBoxXcenter->value() - 1.0;
            double yCenter = doubleSpinBoxYcenter->value() - 1.0;
            double xSize = doubleSpinBoxRectXsize->value();
            double ySize = doubleSpinBoxRectYsize->value();

            bool maskInside = (comboBoxMaskInside->currentIndex() != 1);
            double newValue = comboBoxValue->currentText().toDouble();

            for (int xx = 0; xx < m->numCols(); xx++)
                for (int yy = 0; yy < m->numRows(); yy++)
                    if ((xx >= xCenter - xSize / 2.0 && xx <= xCenter + xSize / 2.0 && yy >= yCenter - ySize / 2.0 &&
                         yy <= yCenter + ySize / 2.0) ^
                        !maskInside)
                        m->setText(yy, xx, QString::number(newValue));

            m->notifyModifiedData();
            return;
        }
}
//+++ mask sector
void dan18::maskSector()
{
    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == comboBoxMlistMask->currentText())
        {
            auto *m = (Matrix *)w;
            
            double xCenter = doubleSpinBoxXcenter->value() - 1.0;
            double yCenter = doubleSpinBoxYcenter->value() - 1.0;
            double phiMin = doubleSpinBoxAngleFrom->value();
            double phiMax = doubleSpinBoxAngleTo->value();
            double currentPhi, r;

            bool maskInside = (comboBoxMaskInside->currentIndex() != 1);
            double newValue = comboBoxValue->currentText().toDouble();

            for (int xx = 0; xx < m->numCols(); xx++)
                for (int yy = 0; yy < m->numRows(); yy++)
                {
                    r = sqrt((xx - xCenter) * (xx - xCenter) + (yy - yCenter) * (yy - yCenter));
                    if (xx >= xCenter && yy >= yCenter && r > 0)
                        currentPhi = asin((xx - xCenter) / r) / M_PI * 180;
                    else if (xx >= xCenter && yy < yCenter && r > 0)
                        currentPhi = 90 + asin((yCenter - yy) / r) / M_PI * 180;
                    else if (xx < xCenter && yy <= yCenter && r > 0)
                        currentPhi = 180 + asin((xCenter - xx) / r) / M_PI * 180;
                    else if (xx < xCenter && yy > yCenter && r > 0)
                        currentPhi = 270 + asin((yy - yCenter) / r) / M_PI * 180;
                    else
                        currentPhi = phiMin;

                    auto inRange = [&](double phi) {
                        return (phiMin < 0 ? phi >= 360 + phiMin : phi >= phiMin && phi <= phiMax);
                    };

                    if (maskInside ? inRange(currentPhi) : !inRange(currentPhi))
                        m->setText(yy, xx, QString::number(newValue));
            }
            m->notifyModifiedData();
            return;
        }
}
//+++ ellipse mask
void dan18::maskEllipse()
{
    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == comboBoxMlistMask->currentText())
        {
            auto *m = (Matrix *)w;

            double xCenter = doubleSpinBoxXcenter->value() - 1.0;
            double yCenter = doubleSpinBoxYcenter->value() - 1.0;
            double xSize = doubleSpinBoxEllipseXsize->value();
            double ySize = doubleSpinBoxEllipseYsize->value();

            bool maskInside = (comboBoxMaskInside->currentIndex() != 1);
            double newValue = comboBoxValue->currentText().toDouble();
            double r;

            if (xSize == 0 || ySize == 0)
                return;
        
            for (int xx = 0; xx < m->numCols(); xx++)
                for (int yy = 0; yy < m->numRows(); yy++)
                {
                    r = (xCenter - xx) * (xCenter - xx) * 4 / xSize / xSize;
                    r += (yCenter - yy) * (yCenter - yy) * 4 / ySize / ySize;
                    if ((maskInside && r <= 1) || (!maskInside && r > 1))
                        m->setText(yy, xx, QString::number(newValue));
                }

            m->notifyModifiedData();
            return;
        }
}
//+++ ellipse shell mask
void dan18::maskEllipseShell()
{
    foreach (MdiSubWindow *w, app()->matrixList())
        if (w->name() == comboBoxMlistMask->currentText())
        {
            auto *m = (Matrix *)w;

            double xCenter = doubleSpinBoxXcenter->value() - 1.0;
            double yCenter = doubleSpinBoxYcenter->value() - 1.0;
            double xSize = doubleSpinBoxEllipseXsize->value();
            double ySize = doubleSpinBoxEllipseYsize->value();
            double shell = doubleSpinBoxEllShell->value();

            bool maskInside = (comboBoxMaskInside->currentIndex() != 1);

            double newValue = comboBoxValue->currentText().toDouble();
            if (xSize == 0 || ySize == 0 || shell == 0)
                return;

            double rPlus, rMinus;
            for (int xx = 0; xx < m->numCols(); xx++)
                for (int yy = 0; yy < m->numRows(); yy++)
                {
                    rPlus = (xCenter - xx) * (xCenter - xx) * 4 / (xSize + shell) / (xSize + shell);
                    rPlus += (yCenter - yy) * (yCenter - yy) * 4 / (ySize + shell) / (ySize + shell);

                    rMinus = (xCenter - xx) * (xCenter - xx) * 4 / (xSize - shell) / (xSize - shell);
                    rMinus += (yCenter - yy) * (yCenter - yy) * 4 / (ySize - shell) / (ySize - shell);

                    if ((maskInside && rPlus <= 1 && rMinus >= 1) || (!maskInside && (rPlus > 1 || rMinus < 1)))
                        m->setText(yy, xx, QString::number(newValue));
                }
            m->notifyModifiedData();
            return;
        }
}