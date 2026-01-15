/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s)'s fitting buttons functions
 ******************************************************************************/

#include "fittable18.h"

//+++ saveUndo
void fittable18::saveUndo()
{
    int M = spinBoxNumberCurvesToFit->value();
    int p = spinBoxPara->value();

    QString undo, undoErrorbars, undoFittable, undoShareble, undoRange;

    for (int mm = 0; mm < M; mm++)
        for (int pp = 0; pp < p; pp++)
        {
            auto *itS = (QTableWidgetItem *)tablePara->item(pp, 3 * mm + 1);
            undoFittable += QString::number(Qt::CheckState(itS->checkState())) + " ";
            undoRange += itS->text().remove(" ") + " ";
            undo += tablePara->item(pp, 3 * mm + 2)->text() + " ";
            undoErrorbars += tablePara->item(pp, 3 * mm + 3)->text() + " ";
        }

    if (M > 1)
        for (int pp = 0; pp < p; pp++)
        {
            auto *itA = (QTableWidgetItem *)tablePara->item(pp, 0);
            undoShareble += QString::number(Qt::CheckState(itA->checkState())) + " ";
        }

    undoRedo << undo;
    undoRedoErrorbars << undoErrorbars;
    undoRedoFittable << undoFittable;
    undoRedoRange << undoRange;
    undoRedoShareble << undoShareble;

    pushButtonRedo->setEnabled(false);
    pushButtonUndo->setEnabled(true);

    undoRedoActive = undoRedo.count();
}

//+++ undo
void fittable18::undo()
{
    if (undoRedo.count() == 0 || undoRedoErrorbars.count() == 0 || undoRedoFittable.count() == 0 ||
        undoRedoRange.count() == 0 || undoRedoActive < 1)
    {
        undoRedoActive = 0;
        pushButtonUndo->setEnabled(false);
        return;
    }

    int M = spinBoxNumberCurvesToFit->value();
    int p = spinBoxPara->value();

    QStringList lstPara = undoRedo[undoRedoActive - 1].simplified().split(" ");
    QStringList lstParaErrorbars = undoRedoErrorbars[undoRedoActive - 1].simplified().split(" ");
    QStringList lstParaFittable = undoRedoFittable[undoRedoActive - 1].simplified().split(" ");
    QStringList lstParaRange = undoRedoRange[undoRedoActive - 1].simplified().split(" ");
    QStringList lstParaShareble = undoRedoShareble[undoRedoActive - 1].simplified().split(" ");

    if (lstPara.count() != p * M || lstParaErrorbars.count() != p * M || lstParaFittable.count() != p * M ||
        lstParaRange.count() != p * M)
        return;

    undoRedoActive--;
    pushButtonRedo->setEnabled(true);

    tablePara->blockSignals(true);

    for (int mm = 0; mm < M; mm++)
        for (int pp = 0; pp < p; pp++)
        {
            auto itS = (QTableWidgetItem *)tablePara->item(pp, 3 * mm + 1);
            itS->setCheckState(Qt::CheckState(lstParaFittable[mm * p + pp].toInt()));
            itS->setText(lstParaRange[mm * p + pp]);
            tablePara->item(pp, 3 * mm + 2)->setText(lstPara[mm * p + pp]);
            tablePara->item(pp, 3 * mm + 3)->setText(lstParaErrorbars[mm * p + pp]);
        }

    if (M > 1 && lstParaShareble.count() == p)
        for (int pp = 0; pp < p; pp++)
        {
            auto *itA = (QTableWidgetItem *)tablePara->item(pp, 0);
            itA->setCheckState(Qt::CheckState(lstParaShareble[pp].toInt()));
        }

    if (undoRedoActive < 1)
        pushButtonUndo->setEnabled(false);

    pushButtonUndo->setFocus();
    if (checkBoxAutoRecalculate->isChecked())
        plotSwitcher();

    tablePara->blockSignals(false);
}

//+++ redo
void fittable18::redo()
{
    if (undoRedo.count() == 0 || undoRedoErrorbars.count() == 0 || undoRedoFittable.count() == 0 ||
        undoRedoRange.count() == 0 || undoRedoActive >= undoRedo.count() - 1)
    {
        pushButtonRedo->setEnabled(false);
        return;
    }

    int M = spinBoxNumberCurvesToFit->value();
    int p = spinBoxPara->value();

    QStringList lstPara = undoRedo[undoRedoActive + 1].simplified().split(" ");
    QStringList lstParaErrorbars = undoRedoErrorbars[undoRedoActive + 1].simplified().split(" ");
    QStringList lstParaFittable = undoRedoFittable[undoRedoActive + 1].simplified().split(" ");
    QStringList lstParaRange = undoRedoRange[undoRedoActive + 1].simplified().split(" ");
    QStringList lstParaShareble = undoRedoShareble[undoRedoActive + 1].simplified().split(" ");

    if (lstPara.count() != p * M || lstParaErrorbars.count() != p * M || lstParaFittable.count() != p * M ||
        lstParaRange.count() != p * M)
        return;

    undoRedoActive++;
    pushButtonUndo->setEnabled(true);

    tablePara->blockSignals(true);

    for (int mm = 0; mm < M; mm++)
        for (int pp = 0; pp < p; pp++)
        {
            auto itS = (QTableWidgetItem *)tablePara->item(pp, 3 * mm + 1);
            itS->setCheckState(Qt::CheckState(lstParaFittable[mm * p + pp].toInt()));
            itS->setText(lstParaRange[mm * p + pp]);
            tablePara->item(pp, 3 * mm + 2)->setText(lstPara[mm * p + pp]);
            tablePara->item(pp, 3 * mm + 3)->setText(lstParaErrorbars[mm * p + pp]);
        }

    if (M > 1 && lstParaShareble.count() == p)
        for (int pp = 0; pp < p; pp++)
        {
            auto itA = (QTableWidgetItem *)tablePara->item(pp, 0);
            itA->setCheckState(Qt::CheckState(lstParaShareble[pp].toInt()));
        }

    if (undoRedoActive >= undoRedo.count() - 1)
        pushButtonRedo->setEnabled(false);

    pushButtonRedo->setFocus();
    if (checkBoxAutoRecalculate->isChecked())
        plotSwitcher();
    tablePara->blockSignals(false);
}

//+++ changeFunctionLocal
void fittable18::changeFunctionLocal(const QString &newFunction)
{
    QString oldFunction=textLabelFfunc->text();
    if (oldFunction == newFunction)
        return;

    openSharedLibrary(newFunction);

    initLimits();

    initFitPage();

    if (widgetStackFit->currentIndex() == 2)
        for (int i = 0; i < spinBoxPara->value(); i++)
            tableParaSimulate->setItem(i, 0, new QTableWidgetItem(tablePara->item(i, 2)->text()));

    radioButtonDataShowAll->setChecked(true);
    radioButtonParaShowAll->setChecked(true);
}

//+++ plotSwitcher:: calculate button was pressed
void fittable18::plotSwitcher()
{
    fitOrCalculate(true);
}

//+++ fitSwitcher
void fittable18::fitSwitcher()
{
    fitOrCalculate(false);
}

//+++ fitOrCalculate
void fittable18::fitOrCalculate(bool calculateYN)
{
    fitOrCalculate(calculateYN, -1);
}

//+++ Plot Fitting Curves via Simulate Interface
void fittable18::fitOrCalculate(bool calculateYN, int mmm)
{
    bool toPlot = false;
    bool toPlotResidulas = false;
    bool toPlotResidulasRight = false;
    bool setToSetFit = (mmm == -2);

    int plotDirector = comboBoxPlotActions->currentIndex();

    if (plotDirector < 3)
        toPlot = true;

    MultiLayer *plot = nullptr;
    Graph *g = nullptr;
    Graph *gR = nullptr;

    if (toPlot)
        toPlot = app()->findActivePlot(plot);

    if (toPlot && plot->numLayers() < 1)
        toPlot = false;

    if (toPlot)
    {
        g = plot->layer(1);

        if (plotDirector == 1 || plotDirector == 2)
            toPlotResidulas = true;

        if (plotDirector == 2)
            toPlotResidulasRight = true;

        if (!g)
        {
            toPlot = false;
            toPlotResidulasRight = false;
            toPlotResidulas = false;
        }
        else if (toPlotResidulasRight)
            gR = g;
        else if (toPlotResidulas && plot->numLayers() >= 2)
            gR = plot->layer(2);
        else
        {
            toPlotResidulas = false;
        }
    }

    bool maximaizedYN = (toPlot && plot->status() == MdiSubWindow::Maximized);

    chekLimits();
    chekLimitsAndFittedParameters();

    bool fitOK = true;

    int M = spinBoxNumberCurvesToFit->value();
    int p = spinBoxPara->value();

    if (!calculateYN)
    {
        //+++ Results to Interface
        for (int pp = 0; pp < p; pp++)
            for (int mm = 0; mm < M; mm++)
                tablePara->item(pp, 3 * mm + 3)->setText("---");

        fitOK = checkBoxSANSsupport->isChecked() ? sansFit() : simplyFit();
    }

    // mmm = -1 all sets
    if (!fitOK)
        return;

    //+++ curves to generate/plot:
    //+++ mmm==-1  all [1..M]  (fitting interface)
    int mmStart, mmFinish;

    if (mmm > -1)
    {
        mmStart = mmm;
        mmFinish = mmm + 1;
    }
    else
    {
        mmStart = 0;
        mmFinish = spinBoxNumberCurvesToFit->value();
    }

    //+++  currrent table name
    QString tableName;

    //np,chi2,TSS
    int npAll = 0;
    double chi2All = 0;
    double TSSAll = 0;

    int tableType = (widgetStackFit->currentIndex() == 1) ? 1 : 2;

    //+++ finally actions
    for (int mm = mmStart; mm < mmFinish; mm++)
    {
        Table *ttt = nullptr;
        int np = 0;
        double chi2 = 0;
        double TSS = 0;

        if (!generateSimulatedTable(true, tableType, mm, false, tableName, ttt, np, chi2, TSS))
            return;

        npAll += np;
        chi2All += chi2;
        TSSAll += TSS;

        checkConstrains(mm);
        if (M > 1 && mmm == -1 && mm == 0)
            for (int pp = 0; pp < p; pp++)
                checkGlobalParameters(pp, -2);
        
        if (toPlot)
        {
            int colorIndex = (checkBoxColorIndexing->isChecked() || spinBoxCurrentFunction->value() > 0) ? mm : 0;
            colorIndex += comboBoxColor->currentIndex();

            auto *cell = (QComboBoxInTable *)tableCurves->cellWidget(0, 2 * mm + 1);

            if (cell && !cell->currentText().isEmpty() && g->insertCurveScatter(cell->currentText(), false))
                addGeneralCurve(g, tableName, colorIndex, ttt);

            if (toPlotResidulas && radioButtonSameQrange->isChecked())
                addGeneralCurve(gR, tableName + "_residues", colorIndex, ttt, toPlotResidulasRight);
        }
    }

    if (checkBoxSaveSessionFit->isChecked())
        saveFittingSession("fitCurve-" + textLabelFfunc->text() + "-session");

    chi2();

    if (!pushButtonUndo->hasFocus() && !pushButtonRedo->hasFocus())
        saveUndo();

    if (maximaizedYN)
        plot->setMaximized();

    if (toPlot && !setToSetFit)
    {
        g->setAutoScale(true);
        g->replot();
        g->notifyChanges();

        if (toPlotResidulas && radioButtonSameQrange->isChecked())
        {
            gR->setAutoScale(true);
            gR->replot();
            gR->notifyChanges();
        }
    }
}