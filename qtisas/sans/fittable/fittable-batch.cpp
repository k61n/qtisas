/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Batch fitting interface functions
 ******************************************************************************/

#include "fittable18.h"
#include "globals.h"

const int minColumnsBatchFitTable = 7;
const int minRowsBatchFitTable = 20;

void fittable18::connectSlotBatch()
{
    connect(pushButtonBatchToGlobal, &QPushButton::clicked, this, &fittable18::batchToGlobal);
    connect(pushButtonPattern, &QPushButton::clicked, this, &fittable18::selectPattern);
    connect(pushButtonSetBySetFit, &QPushButton::clicked, this, [this]() { setBySetFitOrSim(true); });
    connect(pushButtonSimulateMulti, &QPushButton::clicked, this, [this]() { setBySetFitOrSim(false); });
    connect(tabWidgetGenResults, &QTabWidget::currentChanged, this, [this]() { updateBatchFitTableList(); });
    connect(comboBoxSetBySetTable, &QComboBox::textActivated, this, &fittable18::readBatchTable);
    connect(pushButtonNewBatch, &QPushButton::clicked, this, [this]() {
        newBatchTable();
        updateBatchFitTableList();
    });
    connect(pushButtonDeleteCurrentBatchScript, &QToolButton::clicked, this, [this]() {
        removeTables(comboBoxSetBySetTable->currentText());
        updateBatchFitTableList();
    });
    connect(pushButtonCurrentScriptShow, &QPushButton::clicked, this, [this]() {
        QString tableName = comboBoxSetBySetTable->currentText();
        if (tableName.isEmpty())
            return;
        Table *w;
        if (app()->checkTableExistence(tableName, w))
        {
            app()->activateWindow(w);
            w->showMaximized();
            w->goToColumn(w->numCols() - 1);
        }
    });

}

//+++  Multi Table:: select Pattern
void fittable18::selectPattern()
{
    bool addAllColsSep = checkBoxBatchFitAllYCols->isChecked();

    tableMultiFit->setRowCount(1);

    QStringList tables, tablesAll, tablesCols;

    tablesAll = app()->tableNames();

    QRegularExpression rx = REGEXPS::wildcardToRegex(lineEditPattern->text());

    for (int i = 0; i < tablesAll.count(); i++)
        if (rx.match(tablesAll[i]).hasMatch())
            tables << tablesAll[i];

    tables.prepend("All");
    tableMultiFit->setRowCount(static_cast<int>(tables.count()));

    tablesCols.prepend("All");
    int currentRaw = 1;

    for (int ii = 1; ii < tables.count(); ii++)
    {
        tablesCols << tables[ii];

        auto *yn = new QTableWidgetItem();
        yn->setCheckState(Qt::Unchecked);
        tableMultiFit->setItem(currentRaw, 0, yn);

        auto *yCol = new QComboBox();
        tableMultiFit->setCellWidget(currentRaw, 1, yCol);

        auto *dYcol = new QComboBox();
        tableMultiFit->setCellWidget(currentRaw, 2, dYcol);

        QRegularExpression rxCol("^" + QRegularExpression::escape(tables[ii]) + "_.*");

        QStringList colsY, colsYerr, colsXerr, colTemp;

        colTemp = app()->columnsList(Table::Y);
        for (int j = 0; j < colTemp.count(); j++)
            if (rxCol.match(colTemp[j]).hasMatch())
                colsY << colTemp[j].remove(tables[ii] + "_");
        yCol->addItems(colsY);

        colTemp = app()->columnsList(Table::yErr);
        for (int j = 0; j < colTemp.count(); j++)
            if (rxCol.match(colTemp[j]).hasMatch())
                colsYerr << colTemp[j].remove(tables[ii] + "_");
        dYcol->addItems(colsYerr);

        int start = 3;
        if (checkBoxSANSsupport->isChecked())
        {
            start++;

            auto *xCol = new QComboBox();
            tableMultiFit->setCellWidget(currentRaw, 3, xCol);

            QString currentInstrument = comboBoxInstrument->currentText();
            if (currentInstrument.contains("Back"))
                colsXerr << "from_SPHERES";
            else
            {
                colsXerr << "calculated_in_\"ASCII.1D.SANS\"";
                colsXerr << "\"01%\":_sigma(Q)=0.01*Q";
                colsXerr << "\"02%\":_sigma(Q)=0.02*Q";
                colsXerr << "\"05%\":_sigma(Q)=0.05*Q";
                colsXerr << "\"10%\":_sigma(Q)=0.10*Q";
                colsXerr << "\"20%\":_sigma(Q)=0.20*Q";
            }

            colTemp = app()->columnsList(Table::xErr);
            for (int j = 0; j < colTemp.count(); j++)
                if (rxCol.match(colTemp[j]).hasMatch())
                    colsXerr << colTemp[j].remove(tables[ii] + "_");
            xCol->addItems(colsXerr);
        }

        // +++Start values & adjustibility trasfer
        for (int j = start; j < tableMultiFit->columnCount(); j++)
        {
            tableMultiFit->setItem(currentRaw, j, new QTableWidgetItem(tablePara->item(j - start, 2)->text()));
            auto *fitYN = (QTableWidgetItem *)tableMultiFit->item(0, j);
            auto *fitYN0 = (QTableWidgetItem *)tablePara->item(j - start, 1);
            fitYN->setCheckState(fitYN0->checkState());
        }
        currentRaw++;

        if (addAllColsSep && colsY.count() > 1)
        {
            tableMultiFit->setRowCount(tableMultiFit->rowCount() + static_cast<int>(colsY.count()) - 1);

            for (int cols = 1; cols < colsY.count(); cols++)
            {
                tablesCols << tables[ii];

                auto *yn = new QTableWidgetItem();
                yn->setCheckState(Qt::Unchecked);
                tableMultiFit->setItem(currentRaw, 0, yn);

                auto *yCol = new QComboBox();
                tableMultiFit->setCellWidget(currentRaw, 1, yCol);

                auto *dYcol = new QComboBox();
                tableMultiFit->setCellWidget(currentRaw, 2, dYcol);

                yCol->addItems(colsY);
                yCol->setCurrentIndex(cols);

                dYcol->addItems(colsYerr);
                int start = 3;
                if (checkBoxSANSsupport->isChecked())
                {
                    start++;

                    auto *xCol = new QComboBox();
                    tableMultiFit->setCellWidget(currentRaw, 3, xCol);

                    xCol->addItems(colsXerr);
                }

                //+++ Start values & adjustibility trasfer
                for (int j = start; j < tableMultiFit->columnCount(); j++)
                {
                    tableMultiFit->setItem(currentRaw, j, new QTableWidgetItem(tablePara->item(j - start, 2)->text()));
                    auto *fitYN = (QTableWidgetItem *)tableMultiFit->item(0, j);
                    auto *fitYN0 = (QTableWidgetItem *)tablePara->item(j - start, 1);
                    fitYN->setCheckState(fitYN0->checkState());
                }
                currentRaw++;
            }
        }
        
    }

    tableMultiFit->setVerticalHeaderLabels(tablesCols);

    //+++ weighting use
    auto *wYN = (QTableWidgetItem *)tableMultiFit->item(0, 2);
    auto *wYN0 = (QTableWidgetItem *)tableCurves->item(4, 0);
    wYN->setCheckState(wYN0->checkState());

    // +++ reso
    if (checkBoxSANSsupport->isChecked())
    {
        auto *rYN = (QTableWidgetItem *)tableMultiFit->item(0, 3);
        auto *rYN0 = (QTableWidgetItem *)tableCurves->item(5, 0);
        rYN->setCheckState(rYN0->checkState());
    }

    if (tableMultiFit->columnCount() > 10)
        return; // no Stretch

    int startCol = 3;
    if (checkBoxSANSsupport->isChecked())
        startCol++;

    for (int tt = startCol; tt < tableMultiFit->columnCount(); tt++)
        tableMultiFit->resizeColumnToContents(tt);
}

//+++ updateBatchFitTableList
void fittable18::updateBatchFitTableList(const QString &activatedTableName)
{
    if (tabWidgetGenResults->currentIndex() != 2)
        return;

    QStringList tablesAllFiltered;

    const auto tables = app()->tableList();
    foreach (MdiSubWindow *w, tables)
        if (w->windowLabel().contains("Fitting Results:: Set-By-Set"))
        {
            auto t = (Table *)w;
            if (t->numRows() < minRowsBatchFitTable && t->numCols() < minColumnsBatchFitTable)
                continue;

            if (t->text(0, 1).remove("-> ") != comboBoxFunction->currentText())
                continue;

            if (t->text(4, 1).contains("No") && checkBoxSANSsupport->isChecked())
                continue;

            if (!t->text(4, 1).contains("No") && !checkBoxSANSsupport->isChecked())
                continue;

            tablesAllFiltered << w->name();
        }

    comboBoxSetBySetTable->blockSignals(true);

    QString oldTable = comboBoxSetBySetTable->currentText();
    if (!activatedTableName.isEmpty())
        oldTable = activatedTableName;

    comboBoxSetBySetTable->clear();
    comboBoxSetBySetTable->addItems(tablesAllFiltered);

    const int oldIndex = comboBoxSetBySetTable->findText(oldTable);
    if (oldIndex >= 0)
        comboBoxSetBySetTable->setCurrentIndex(oldIndex);
    else if (tablesAllFiltered.count() > 0 && activatedTableName.isEmpty())
        readBatchTable(comboBoxSetBySetTable->currentText());

    comboBoxSetBySetTable->blockSignals(false);
}

//+++ read Batch Table
void fittable18::readBatchTable(const QString &tableName)
{
    Table *batchTable = nullptr;
    if (!app()->checkTableExistence(tableName, batchTable))
    {
        QMessageBox::warning(this, "QtiSAS", "There is no table:: " + tableName);
        return;
    }

    ((QTableWidgetItem *)tableMultiFit->item(0, 1))->setCheckState(Qt::Unchecked);

    int p = spinBoxPara->value();

    //+++ FIRST LINE IS UNTOUCHED
    tableMultiFit->setRowCount(1);

    //+++ COUNTER: NUMBER OF EXISTING TABLES IN SKRIPT TABLE
    int activeDatasets = 0;

    //+++ LIST OF ALL Y-COLUMNS
    const QStringList colTemp = app()->columnsList(Table::Y);
    const QStringList colTempErr = app()->columnsList(Table::yErr);
    const QStringList colTempReso = app()->columnsList(Table::xErr);

    //+++ LIST OF EXISTING COLUMNS IN SKRIPT TABLE
    QStringList tables;

    //+++ FIRST STEP OF DATA TRANSFET ANF MULTITABLE ARRANGMENT | USED FOR LEFT HEADER
    for (int i = 0; i < batchTable->numRows(); i++)
    {
        QString info = batchTable->text(i, 3);
        QString currentTable = info.left(info.indexOf("|t|")).trimmed();
        QString currentY = info.mid(info.indexOf("|t|") + 3, info.indexOf("|y|") - info.indexOf("|t|") - 3).trimmed();
        QString currentWeight =
            info.mid(info.indexOf("|y|") + 3, info.indexOf("|w|") - info.indexOf("|y|") - 3).trimmed();
        QString currentReso =
            info.mid(info.indexOf("|w|") + 3, info.indexOf("|r|") - info.indexOf("|w|") - 3).trimmed();

        if (!colTemp.contains(currentTable + "_" + currentY))
            continue;

        tableMultiFit->setRowCount(activeDatasets + 2);
        tables << currentTable;

        QStringList cols;

        //+++ WILD PATTERN OF Y-COLUMNS OF CURRENT DATASET
        QRegularExpression rxCol("^" + QRegularExpression::escape(currentTable) + "_.*");

        auto *yn = new QTableWidgetItem();
        yn->setCheckState(Qt::Unchecked);
        tableMultiFit->setItem(activeDatasets + 1, 0, yn);

        auto *yCol = new QComboBox();
        tableMultiFit->setCellWidget(activeDatasets + 1, 1, yCol);

        for (int j = 0; j < colTemp.count(); j++)
            if (rxCol.match(colTemp[j]).hasMatch())
                cols << QString(colTemp[j]).remove(currentTable + "_");

        yCol->addItems(cols);
        yCol->setItemText(yCol->currentIndex(), currentY);

        auto *dYcol = new QComboBox();
        tableMultiFit->setCellWidget(activeDatasets + 1, 2, dYcol);

        cols.clear();
        for (int j = 0; j < colTempErr.count(); j++)
            if (rxCol.match(colTempErr[j]).hasMatch())
                cols << QString(colTempErr[j]).remove(currentTable + "_");

        dYcol->addItems(cols);
        dYcol->setItemText(dYcol->currentIndex(), currentWeight);

        int resoShift = 0;
        if (checkBoxSANSsupport->isChecked())
        {

            auto *resoCol = new QComboBox();
            tableMultiFit->setCellWidget(activeDatasets + 1, 3, resoCol);

            cols.clear();
            for (int j = 0; j < colTempReso.count(); j++)
                if (rxCol.match(colTempReso[j]).hasMatch())
                    cols << QString(colTempReso[j]).remove(currentTable + "_");

            resoCol->addItems(cols);
            resoCol->setItemText(resoCol->currentIndex(), currentReso);
            resoShift++;
        }

        for (int j = 0; j < p; j++)
            tableMultiFit->setItem(activeDatasets + 1, 3 + j + resoShift,
                                   new QTableWidgetItem(batchTable->text(i, 7 + 2 * j)));

        activeDatasets++;
    }

    tables.prepend("All");
    tableMultiFit->setVerticalHeaderLabels(tables);

    if (tableMultiFit->columnCount() > 10)
        return;

    int startCol = 3;
    if (checkBoxSANSsupport->isChecked())
        startCol++;

    for (int tt = startCol; tt < tableMultiFit->columnCount(); tt++)
        tableMultiFit->resizeColumnToContents(tt);
}

//+++ create New Batch Table
bool fittable18::newBatchTable(QString tableName)
{
    if (tableName.isEmpty())
    {
        tableName = comboBoxSetBySetTable->currentText();
        if (tableName.isEmpty())
            tableName = "Set-By-Set-Fit";

        bool ok;

        do
        {
            tableName = QInputDialog::getText(this, "Creation of Batch-Table",
                                              "Enter name of a new Batch-Table:", QLineEdit::Normal, tableName, &ok);
        } while (comboBoxSetBySetTable->findText(tableName) != -1 || !ok);

        if (!ok)
            return false;
    }

    if (tableName.isEmpty())
        return false;

    if (comboBoxSetBySetTable->findText(tableName) >= 0 || app()->checkTableExistence(tableName))
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Table " + tableName + " is already exists");
        return false;
    }

    Table *t = nullptr;
    saveBatchTable(t, tableName);
    updateBatchFitTableList(tableName);
    return true;
}

//+++ save Batch Table
bool fittable18::saveBatchTable(Table *&t, QString tableName)
{
    if (tableName.isEmpty())
        tableName = comboBoxSetBySetTable->currentText();

    if (tableName.isEmpty())
        return false;

    int p = spinBoxPara->value();

    // +++ check #1
    int Ntot = tableMultiFit->rowCount() - 1; // number of Availeble Datasets in table
    

    // +++ weight
    bool weight = (QTableWidgetItem *)tableMultiFit->item(0, 2)->checkState();

    // +++ reso & poly
    QString SANSsupport = "No";
    QString polyUse = "No";
    bool reso = false;
    bool poly = false;
    int start = 3;

    if (checkBoxSANSsupport->isChecked())
    {
        reso = (QTableWidgetItem *)tableMultiFit->item(0, 3)->checkState();
        poly = (QTableWidgetItem *)tableCurves->item(6, 0)->checkState();
        if (poly)
            polyUse = "Yes";
        start++;
        SANSsupport = "Yes";
    }

    int prec = spinBoxSignDigits->value();

    // +++  create Table
    bool tableNew = false;

    if (!app()->checkTableExistence(tableName, t))
    {
        t = app()->newTable(tableName, GSL_MAX(Ntot, 20), 2 + 1 + 1 + 3 + 2 * p);
        t->setWindowLabel("Fitting Results:: Set-By-Set");
        app()->setListViewLabel(t->name(), "Fitting Results:: Set-By-Set");
        app()->updateWindowLists(t);
        tableNew = true;
    }
    else
    {
        t->setNumRows(Ntot);
        t->setNumCols(2 + 1 + 1 + 3 + 2 * p);
    }

    t->blockSignals(true);

    t->setColName(0, "Parameter");
    t->setColPlotDesignation(0, Table::None);
    t->setColumnType(0, Table::Text);

    t->setColName(1, "Value");
    t->setColPlotDesignation(1, Table::None);
    t->setColumnType(1, Table::Text);

    t->setColName(2, "X");
    t->setColPlotDesignation(2, Table::X);

    t->setColName(3, "Dataset");
    t->setColumnType(3, Table::Text);

    t->setColName(4, "Chi2");

    t->setColName(5, "R2");

    t->setColName(6, "Fit-Time");
    t->setColumnType(6, Table::Text);

    QString s = "-> ";
    for (int i = 0; i < p; i++)
    {
        s += (i < p - 1) ? F_paraList[i] + " , " : F_paraList[i];

        t->setColName(7 + 2 * i, F_paraList[i]);
        t->setColNumericFormat(2, prec + 1, 7 + 2 * i, true);

        t->setColName(7 + 2 * i + 1, "d" + F_paraList[i]);
        t->setColPlotDesignation(7 + 2 * i + 1, Table::yErr);
        t->setColNumericFormat(2, prec + 2, 7 + 2 * i + 1, true);
        t->setColumnType(7 + 2 * i + 1, Table::Text);
    }

    // Fit Conrtrol
    int currentChar = 0;

    // +++  Fitting Function
    t->setText(currentChar, 0, "Fitting Function");
    t->setText(currentChar, 1, "-> " + textLabelFfunc->text());
    currentChar++;
    // +++  Number of Parameters
    t->setText(currentChar, 0, "Number of Parameters");
    t->setText(currentChar, 1, "-> " + QString::number(p));
    currentChar++;
    // +++  Parameters
    t->setText(currentChar, 0, "Parameters");
    t->setText(currentChar, 1, s);
    currentChar++;
    //+++ Multi Mode
    t->setText(currentChar, 0, "Number of Datasets");
    t->setText(currentChar, 1, "-> 1");
    currentChar++;
    //+++ SANS Mode
    t->setText(currentChar, 0, "SANS Mode");
    t->setText(currentChar, 1, "-> " + SANSsupport);
    currentChar++;
    //+++ Resolusion On
    SANSsupport = (reso && checkBoxSANSsupport->isChecked()) ? "Yes" : "No";
    t->setText(currentChar, 0, "Resolution On");
    t->setText(currentChar, 1, "-> " + SANSsupport);
    currentChar++;
    //+++ Weight On
    SANSsupport = weight ? "Yes" : "No";
    t->setText(currentChar, 0, "Weight On");
    t->setText(currentChar, 1, "-> " + SANSsupport);
    currentChar++;
    //+++ Polydispersity On
    t->setText(currentChar, 0, "Polydispersity On");
    t->setText(currentChar, 1, "-> " + polyUse);
    currentChar++;
    //+++ Polydispersity Parameter
    t->setText(currentChar, 0, "Polydisperse Parameter");
    auto *pp = qobject_cast<QComboBoxInTable *>(tableCurves->cellWidget(6, 1));
    t->setText(currentChar, 1, checkBoxSANSsupport->isChecked() && pp ? "-> " + pp->currentText() : "-> No");
    currentChar++;
    //+++ Fitting Range: From x[min]
    t->setText(currentChar, 0, "Fitting Range: From x[min]");
    t->setText(currentChar, 1, "-> " + lineEditFromQ->text());
    currentChar++;
    //+++ Fitting Range: To x[max]
    t->setText(currentChar, 0, "Fitting Range: To x[max]");
    t->setText(currentChar, 1, "-> " + lineEditToQ->text());
    currentChar++;
    //+++ Simulation Range: x-Range Source
    t->setText(currentChar, 0, "Simulation Range: x-Range Source");
    if (radioButtonSameQrange->isChecked())
        t->setText(currentChar, 1, "-> Same Q as Fitting Data");
    else
        t->setText(currentChar, 1, "-> Uniform Q");
    currentChar++;
    //+++ Simulation Range: x-min
    t->setText(currentChar, 0, "Simulation Range: x-min");
    t->setText(currentChar, 1, "-> " + lineEditFromQsim->text());
    currentChar++;
    //+++ Simulation Range: x-max
    t->setText(currentChar, 0, "Simulation Range: x-max");
    t->setText(currentChar, 1, "-> " + lineEditToQsim->text());
    currentChar++;
    //+++ Simulation Range: Number Points
    t->setText(currentChar, 0, "Simulation Range: Number Points");
    t->setText(currentChar, 1, "-> " + lineEditNumPointsSim->text());
    currentChar++;
    //+++ Simulation Range: Logarithmic Step
    t->setText(currentChar, 0, "Simulation Range: Logarithmic Step");
    if (checkBoxLogStep->isChecked())
        t->setText(currentChar, 1, "-> Yes");
    else
        t->setText(currentChar, 1, "-> No");
    currentChar++;
    //+++ Simulation Range: Logarithmic Step
    t->setText(currentChar, 0, "Simulation Range: y-min");
    if (checkBoxLogStep->isChecked())
        t->setText(currentChar, 1, "-> " + lineEditImin->text());
    else
        t->setText(currentChar, 1, "-> 0");
    currentChar++;
    //+++ Fit-Control
    QString line;
    line = QString::number(comboBoxFitMethod->currentIndex()) + " , ";
    line += spinBoxMaxIter->text() + " , ";
    line += lineEditTolerance->text() + " , ";
    line += QString::number(comboBoxColor->currentIndex()) + " , ";
    line += spinBoxSignDigits->text() + " , ";
    line += QString::number(comboBoxWeightingMethod->currentIndex()) + " , ";
    line += checkBoxCovar->isChecked() ? "1 , " : "0 , ";
    t->setText(currentChar, 0, "Fit-Control");
    t->setText(currentChar, 1, "-> " + line);
    currentChar++;
    //+++ Resolution Integral
    t->setText(currentChar, 0, "Resolution Integral");
    line = "-> " + lineEditAbsErr->text();
    line += " , " + lineEditRelErr->text();
    line += " , " + spinBoxIntWorkspase->text();
    line += " , " + spinBoxIntLimits->text();
    line += " , " + comboBoxResoFunction->currentText();
    t->setText(currentChar, 1, line);
    currentChar++;
    //+++ Polydispersity Integral
    t->setText(currentChar, 0, "Polydispersity Integral");
    line = "-> " + lineEditAbsErrPoly->text();
    line += " , " + lineEditRelErrPoly->text();
    line += " , " + spinBoxIntWorkspasePoly->text();
    line += " , " + spinBoxIntLimitsPoly->text();
    line += " , " + comboBoxPolyFunction->currentText();
    t->setText(currentChar, 1, line);
    currentChar++;
    // +++ Q/N
    auto *NQ = (QComboBoxInTable *)tableCurves->cellWidget(1, 0);
    if (NQ)
        NQ->setCurrentIndex(1);
    // +++ From
    auto *fromCheckItem = (QTableWidgetItem *)tableCurves->item(2, 0);
    if (fromCheckItem)
        fromCheckItem->setCheckState(Qt::Checked);
    tableCurves->item(2, 1)->setText(lineEditFromQ->text());
    // +++ To
    auto *toCheckItem = (QTableWidgetItem *)tableCurves->item(3, 0);
    if (toCheckItem)
        toCheckItem->setCheckState(Qt::Checked);
    tableCurves->item(3, 1)->setText(lineEditToQ->text());
    // +++ DataSet
    auto *dataSetItem = (QComboBoxInTable *)tableCurves->cellWidget(0, 1);
    // +++ weight check & Col
    auto *WrealYN = (QTableWidgetItem *)tableCurves->item(4, 0);
    auto *weightColItem = (QComboBoxInTable *)tableCurves->cellWidget(4, 1);
    // +++ reso check & Col
    QTableWidgetItem *RrealYN;
    QComboBoxInTable *resoColItem;
    if (checkBoxSANSsupport->isChecked())
    {
        RrealYN = (QTableWidgetItem *)tableCurves->item(5, 0);
        resoColItem = (QComboBoxInTable *)tableCurves->cellWidget(5, 1);
    }

    for (int i = 0; i < Ntot; i++)
    {
        t->setText(i, 2, QString::number(i + 1));

        s = tableMultiFit->verticalHeaderItem(i + 1)->text() + " |t| ";
        s += ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 1))->currentText();
        s += " |y| ";

        if (weight)
            s += ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 2))->currentText();
        s += " |w| ";

        if (reso)
            s += ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 3))->currentText();
        s += " |r| ";
        t->setText(i, 3, s);
        t->setText(i, 4, "");
        t->setText(i, 5, "");
        t->setText(i, 6, "");

        for (int j = 0; j < p; j++)
        {
            t->setText(i, 7 + 2 * j, tableMultiFit->item(i + 1, j + start)->text());
            t->setText(i, 7 + 2 * j + 1, "");
        }
    }

    t->adjustColumnsWidth(false);
    t->blockSignals(false);
    if (tableNew)
        t->setMaximized();

    return true;
}

//+++ setBySetFitOrSim
void fittable18::setBySetFitOrSim(bool fitYN)
{
    if (fitYN && comboBoxSetBySetTable->count() == 0 || comboBoxSetBySetTable->currentText().isEmpty())
    {
        QMessageBox::warning(this, "QtiSAS", "There is no Batch-Fitting Table | Create it first! ");
        return;
    }

    Table *t;
    if (!saveBatchTable(t, ""))
    {
        QMessageBox::warning(this, "QtiSAS", "Cannot save/create Batch-Fitting Table! ");
        return;
    }

    Graph *g;
    bool currentGraphAutoscaled = false;
    if (!checkBoxPlotBatch->isChecked() && app()->findActiveGraph(g) && g->isAutoscalingEnabled())
    {
        currentGraphAutoscaled = true;
        g->enableAutoscaling(false);
    }

    // timer
    QElapsedTimer time;
    time.start();
    qint64 pre_t = 0;

    int Ntot = tableMultiFit->rowCount() - 1;
    if (Ntot == 0)
    {
        QMessageBox::warning(this, "QtiSAS", "There is no table | Select datasets! | Use [Select] button ");
        return;
    }

    int Nselected = 0;
    int p = spinBoxPara->value();
    setToSetProgressControl = true;

    // +++ weight
    bool weight = (QTableWidgetItem *)tableMultiFit->item(0, 2)->checkState();

    // +++ reso & poly
    bool reso = false;
    bool poly = false;
    int start = 3;
    if (checkBoxSANSsupport->isChecked())
    {
        reso = (QTableWidgetItem *)tableMultiFit->item(0, 3)->checkState();
        poly = (QTableWidgetItem *)tableCurves->item(6, 0)->checkState();
        start++;
    }

    QStringList tables, colList, weightColList, resoColList;

    for (int i = 0; i < Ntot; i++)
    {
        if (!(QTableWidgetItem *)tableMultiFit->item(i + 1, 0)->checkState())
            continue;

        tables << tableMultiFit->verticalHeaderItem(i + 1)->text();

        colList << ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 1))->currentText();

        if (weight)
            weightColList << ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 2))->currentText();

        if (reso)
            resoColList << ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 3))->currentText();

        Nselected++;
    }

    if (Nselected == 0)
    {
        QMessageBox::warning(this, "QtiSAS", "There are no SELECTED tables | Select datasets! ");
        return;
    }

    int plotOldStatus = comboBoxPlotActions->currentIndex();
    if (checkBoxPlotBatch->isChecked())
        comboBoxPlotActions->setCurrentIndex(3);

    bool oldColorIndexing = checkBoxColorIndexing->isChecked();
    checkBoxColorIndexing->setChecked(checkBoxColorIndexingBatch->isChecked());

    int prec = spinBoxSignDigits->value();

    auto *NQ = (QComboBoxInTable *)tableCurves->cellWidget(1, 0); //+++ Q/N
    if (NQ)
        NQ->setCurrentIndex(1);

    auto *fromCheckItem = (QTableWidgetItem *)tableCurves->item(2, 0); //+++ From
    if (fromCheckItem)
        fromCheckItem->setCheckState(Qt::Checked);
    tableCurves->item(2, 1)->setText(lineEditFromQ->text());

    auto *toCheckItem = (QTableWidgetItem *)tableCurves->item(3, 0); //+++ To
    if (toCheckItem)
        toCheckItem->setCheckState(Qt::Checked);
    tableCurves->item(3, 1)->setText(lineEditToQ->text());

    auto *dataSetItem = (QComboBoxInTable *)tableCurves->cellWidget(0, 1); //+++ DataSet

    auto *WrealYN = (QTableWidgetItem *)tableCurves->item(4, 0); //+++ weight check & Col
    auto *weightColItem = (QComboBoxInTable *)tableCurves->cellWidget(4, 1);

    QTableWidgetItem *RrealYN; //+++ reso check & Col
    QComboBoxInTable *resoColItem;
    if (checkBoxSANSsupport->isChecked())
    {
        RrealYN = (QTableWidgetItem *)tableCurves->item(5, 0);
        resoColItem = (QComboBoxInTable *)tableCurves->cellWidget(5, 1);
    }

    tablePara->blockSignals(true);

    for (int j = start; j < tableMultiFit->columnCount(); j++)
    {
        //+++ Start values & adjustibility trasfer
        auto *fitYN = (QTableWidgetItem *)tableMultiFit->item(0, j);
        auto *fitYN0 = (QTableWidgetItem *)tablePara->item(j - start, 1);

        fitYN0->setCheckState(fitYN->checkState());
    }

    tablePara->blockSignals(false);

    int firstColor = comboBoxColor->currentIndex();
    int indexingColor = 0;
    int NselTot = Nselected;

    //+++ Progress dialog
    int progressIter = 0;
    auto *progress = new QProgressDialog("Set-to-Set Fit", "Abort Set-To-Set FIT", 0, NselTot);
    progress->setWindowModality(Qt::ApplicationModal);
    progress->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    progress->setMinimumDuration(0);

    QString s;
    Nselected = 0;

    t->blockSignals(true);
    for (int i = 0; i < Ntot; i++)
    {
        if (!(QTableWidgetItem *)tableMultiFit->item(i + 1, 0)->checkState())
            continue;

        QString infoStr =
            "Current data-set: # " + QString::number(progressIter + 1) + " of " + QString::number(NselTot);
        progress->setValue(progressIter);
        progress->setLabelText(infoStr);
        progressIter++;

        // +++ TRANSFER INFO ABOUT FITTED DATASET
        s = tables[Nselected] + "_" + colList[Nselected];
        dataSetItem->setItemText(dataSetItem->currentIndex(), s);
        tableCurvechanged(0, 1);

        //+++ TRANSFER OF WEIGHT INFO
        std::cout << "Set-to-Set Fit" << "\n";
        std::cout << infoStr.toLocal8Bit().constData() << "\n";
        std::cout << "Dataset:" << s.toLocal8Bit().constData() << "\n";

        if (weight)
        {
            if (weightColList[Nselected].isEmpty() &&
                (comboBoxWeightingMethod->currentIndex() == 0 || comboBoxWeightingMethod->currentIndex() == 2))
                WrealYN->setCheckState(Qt::Unchecked);
            else
            {
                WrealYN->setCheckState(Qt::Checked);
                s = tables[Nselected] + "_" + weightColList[Nselected];
                weightColItem->setItemText(weightColItem->currentIndex(), s);
                tableCurvechanged(4, 1);
            }
        }
        else
            WrealYN->setCheckState(Qt::Unchecked);

        //+++ TRANSFER OF RESOLUTION INFO
        if (reso && !resoColList[Nselected].isEmpty())
        {
            RrealYN->setCheckState(Qt::Checked);

            if (resoColList[Nselected].contains("from DANP") || resoColList[Nselected].contains("ASCII.1D.SANS"))
                s = "calculated_in_\"ASCII.1D.SANS\"";
            else if (resoColList[Nselected].contains("20%"))
                s = "\"20%\":_sigma(Q)=0.20*Q";
            else if (resoColList[Nselected].contains("10%"))
                s = "\"10%\":_sigma(Q)=0.10*Q";
            else if (resoColList[Nselected].contains("05%"))
                s = "\"05%\":_sigma(Q)=0.05*Q";
            else if (resoColList[Nselected].contains("02%"))
                s = "\"02%\":_sigma(Q)=0.02*Q";
            else if (resoColList[Nselected].contains("01%"))
                s = "\"01%\":_sigma(Q)=0.01*Q";
            else if (resoColList[Nselected] == "from_SPHERES")
                s = "from_SPHERES";
            else
                s = tables[Nselected] + "_" + resoColList[Nselected];

            resoColItem->setItemText(resoColItem->currentIndex(), s);
            tableCurvechanged(5, 1);
        }

        //+++ MOVING OF PARAMETERS TO FITTING INTERFACE
        if (tableMultiFit->item(0, 0)->text() != "c" || progressIter == 1)
            for (int j = start; j < tableMultiFit->columnCount(); j++)
                tablePara->item(j - start, 2)->setText(tableMultiFit->item(i + 1, j)->text());

        //+++ FITTING OF CURRENT DATASET
        setToSetNumber = i + 1;

        fitOrCalculate(!fitYN, -2); // -2 means Set-to-Set Fit

        // +++ COLOR CONTROL
        if (checkBoxColorIndexing->isChecked())
        {
            const qsizetype count = app()->indexedColors().count();
            indexingColor = static_cast<int>((firstColor + setToSetNumber) % count);
            comboBoxColor->setCurrentIndex(indexingColor);
        }

        //+++ TRANSFER OF OBTEINED PARAMETERS TO SET-BY-SET TABLE AND TO RESULT TABLE
        for (int j = 0; j < p; j++)
        {
            t->setText(Nselected, 2 * j + 7,
                       QString::number(tablePara->item(j, 2)->text().toDouble(), 'G', spinBoxSignDigits->value()));
            t->setText(Nselected, 2 * j + 8,
                       QString::number(tablePara->item(j, 3)->text().remove(QString(QChar(0x00B1))).toDouble(), 'G',
                                       spinBoxSignDigits->value()));

            tableMultiFit->item(i + 1, j + start)->setText(tablePara->item(j, 2)->text());
        }
        t->setText(Nselected, 4, textLabelChi->text());
        t->setText(Nselected, 5, textLabelR2->text());
        t->setText(Nselected, 6, textLabelTime->text());
        Nselected++;

        if (progress->wasCanceled())
            break;
    }
    progress->setValue(NselTot);
    progress->close();

    t->adjustColumnsWidth(false);
    t->blockSignals(false);

    setToSetProgressControl = false;
    if (checkBoxColorIndexing->isChecked())
        comboBoxColor->setCurrentIndex(firstColor);

    if (currentGraphAutoscaled)
    {
        g->enableAutoscaling(true);
        g->setAutoScale();
    }

    if (app()->findActiveGraph(g))
    {
        g->replot();
        g->notifyChanges();
        app()->updateWindowLists();
    }
    app()->showFullRangeAllPlots(t->name());

    comboBoxPlotActions->setCurrentIndex(plotOldStatus);
    checkBoxColorIndexing->setChecked(oldColorIndexing);
}

//+++ convert current barch script to global fit configuration
void fittable18::batchToGlobal()
{
    QString tableName = comboBoxSetBySetTable->currentText();
    Table *batchTable = nullptr;
    if (tableName.isEmpty() || !app()->checkTableExistence(tableName, batchTable))
    {
        QMessageBox::warning(this, "QtiSAS", "There is no table:: " + tableName);
        return;
    }

    int Ntot = tableMultiFit->rowCount() - 1;
    if (Ntot == 0)
    {
        QMessageBox::warning(this, "QtiSAS", "There is no table | Select datasets! | Use [Select] button ");
        return;
    }

    int Nselected = 0;
    int p = spinBoxPara->value();

    // +++ weight
    bool weight = (QTableWidgetItem *)tableMultiFit->item(0, 2)->checkState();

    // +++ reso & poly
    bool reso = false;
    bool poly = false;
    int start = 3;
    if (checkBoxSANSsupport->isChecked())
    {
        reso = (QTableWidgetItem *)tableMultiFit->item(0, 3)->checkState();
        poly = (QTableWidgetItem *)tableCurves->item(6, 0)->checkState();
        start++;
    }

    QStringList tables, colList, weightColList, resoColList, values;

    for (int i = 0; i < Ntot; i++)
    {
        if (!(QTableWidgetItem *)tableMultiFit->item(i + 1, 0)->checkState())
            continue;

        tables << tableMultiFit->verticalHeaderItem(i + 1)->text();

        colList << ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 1))->currentText();

        if (weight)
            weightColList << ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 2))->currentText();

        if (reso)
            resoColList << ((QComboBoxInTable *)tableMultiFit->cellWidget(i + 1, 3))->currentText();

        for (int j = start; j < tableMultiFit->columnCount(); j++)
            values << tableMultiFit->item(i + 1, j)->text();

        Nselected++;
    }

    QList<Qt::CheckState> fitStatus;
    for (int j = 0; j < p; j++)
        fitStatus << ((QTableWidgetItem *)tableMultiFit->item(0, start + j))->checkState();

    if (Nselected == 0)
    {
        QMessageBox::warning(this, "QtiSAS", "There are no SELECTED tables | Select datasets! ");
        return;
    }

    slotStackFitPrev();
    slotStackFitPrev();

    checkBoxMultiData->setChecked(true);
    spinBoxNumberCurvesToFit->setValue(Nselected);

    slotStackFitNext();

    QString s;
    for (int i = 0; i < Nselected; i++)
    {
        auto *NQ = (QComboBoxInTable *)tableCurves->cellWidget(1, 2 * i); //+++ Q/N
        if (NQ)
            NQ->setCurrentIndex(1);

        auto *fromCheckItem = (QTableWidgetItem *)tableCurves->item(2, 2 * i); //+++ From
        if (fromCheckItem)
            fromCheckItem->setCheckState(Qt::Checked);
        tableCurves->item(2, 2 * i + 1)->setText(lineEditFromQ->text());

        auto *toCheckItem = (QTableWidgetItem *)tableCurves->item(3, 2 * i); //+++ To
        if (toCheckItem)
            toCheckItem->setCheckState(Qt::Checked);
        tableCurves->item(3, 2 * i + 1)->setText(lineEditToQ->text());

        // +++ TRANSFER INFO ABOUT FITTED DATASET
        s = tables[i] + "_" + colList[i];
        auto *dataSetItem = (QComboBoxInTable *)tableCurves->cellWidget(0, 2 * i + 1); //+++ DataSet
        dataSetItem->setItemText(dataSetItem->currentIndex(), s);
        tableCurvechanged(0, 2 * i + 1);

        auto *WrealYN = (QTableWidgetItem *)tableCurves->item(4, 2 * i); //+++ weight check & Col
        if (weight)
        {
            if (weightColList[i].isEmpty() &&
                (comboBoxWeightingMethod->currentIndex() == 0 || comboBoxWeightingMethod->currentIndex() == 2))
                WrealYN->setCheckState(Qt::Unchecked);
            else
            {
                auto *weightColItem = (QComboBoxInTable *)tableCurves->cellWidget(4, 2 * i + 1);
                WrealYN->setCheckState(Qt::Checked);
                s = tables[i] + "_" + weightColList[i];
                weightColItem->setItemText(weightColItem->currentIndex(), s);
                tableCurvechanged(4, 2 * i + 1);
            }
        }
        else
            WrealYN->setCheckState(Qt::Unchecked);

        //+++ TRANSFER OF RESOLUTION INFO
        if (reso && !resoColList[Nselected].isEmpty())
        {
            auto *RrealYN = (QTableWidgetItem *)tableCurves->item(5, 2 * i); //+++ reso check & Col
            auto *resoColItem = (QComboBoxInTable *)tableCurves->cellWidget(5, 2 * i + 1);

            RrealYN->setCheckState(Qt::Checked);

            if (resoColList[i].contains("from DANP") || resoColList[i].contains("ASCII.1D.SANS"))
                s = "calculated_in_\"ASCII.1D.SANS\"";
            else if (resoColList[Nselected].contains("20%"))
                s = "\"20%\":_sigma(Q)=0.20*Q";
            else if (resoColList[Nselected].contains("10%"))
                s = "\"10%\":_sigma(Q)=0.10*Q";
            else if (resoColList[Nselected].contains("05%"))
                s = "\"05%\":_sigma(Q)=0.05*Q";
            else if (resoColList[Nselected].contains("02%"))
                s = "\"02%\":_sigma(Q)=0.02*Q";
            else if (resoColList[Nselected].contains("01%"))
                s = "\"01%\":_sigma(Q)=0.01*Q";
            else if (resoColList[Nselected] == "from_SPHERES")
                s = "from_SPHERES";
            else
                s = tables[i] + "_" + resoColList[i];

            resoColItem->setItemText(resoColItem->currentIndex(), s);
            tableCurvechanged(5, 2 * i + 1);
        }

        //+++ MOVING OF PARAMETERS TO FITTING INTERFACE
        for (int j = 0; j < p; j++)
        {
            tablePara->item(j, 3 * i + 2)->setText(values[i * p + j]);
            ((QTableWidgetItem *)tablePara->item(j, 3 * i + 1))->setCheckState(fitStatus[j]);
        }
    }
}