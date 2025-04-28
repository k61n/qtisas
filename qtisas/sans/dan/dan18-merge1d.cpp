/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS ascii merging tools
 ******************************************************************************/

#include "dan18.h"
#include "Folder.h"

void dan18::mergeSlots()
{
    connect(pushButtonMerge, SIGNAL(clicked()), this, SLOT(mergeProject()));
    connect(pushButtonMergeAscii, SIGNAL(clicked()), this, SLOT(mergeAscii()));

    connect(toolBoxMerge, SIGNAL(currentChanged(int)), this, SLOT(mergeJoinVsRebin(int)));

    connect(radioButtonQrangeManual, SIGNAL(toggled(bool)), spinBoxRebinQmin, SLOT(setVisible(bool)));
    connect(radioButtonQrangeManual, SIGNAL(toggled(bool)), spinBoxRebinQmax, SLOT(setVisible(bool)));

    connect(spinBoxMmergingCond, SIGNAL(valueChanged(int)), this, SLOT(mergingTableChange()));
    connect(spinBoxNtoMerge, SIGNAL(valueChanged(int)), this, SLOT(mergingTableChange()));
    connect(lineEditFilter, SIGNAL(returnPressed()), this, SLOT(updateListOfTables()));

    connect(pushButtonDefineMergeFromTable, SIGNAL(clicked()), this, SLOT(readMergeInfo()));
    connect(pushButtonSaveMergeDataToTable, SIGNAL(clicked()), this, SLOT(saveMergeInfo()));

    connect(checkBoxMergeFromTable, SIGNAL(toggled(bool)), this, SLOT(mergeFromIntegratedTable(bool)));
    connect(checkBoxMergeFromProjectTable, SIGNAL(toggled(bool)), this, SLOT(mergeFromProjectTable(bool)));

    connect(pushButtonSelectActiveTable, SIGNAL(clicked()), this, SLOT(selectActiveTable()));
}

//++++ mergeFromIntegratedTable config
void dan18::mergeFromIntegratedTable(bool YN, bool viaInterface)
{
    if (YN)
    {
        checkBoxMergeFromTable->setMinimumHeight(35);
        checkBoxMergeFromTable->setMaximumHeight(16777215);
    }
    else
    {
        checkBoxMergeFromTable->setMinimumHeight(25);
        checkBoxMergeFromTable->setMaximumHeight(25);
    }
    if (viaInterface)
        checkBoxMergeFromProjectTable->setChecked(!YN);

    layout()->invalidate();
}

//++++ mergeFromProjectTable
void dan18::mergeFromProjectTable(bool YN, bool viaInterface)
{
    if (YN)
    {
        checkBoxMergeFromProjectTable->setMinimumHeight(35);
        checkBoxMergeFromProjectTable->setMaximumHeight(70);
        verticalSpacerMerge->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
    else
    {
        checkBoxMergeFromProjectTable->setMinimumHeight(25);
        checkBoxMergeFromProjectTable->setMaximumHeight(25);
        verticalSpacerMerge->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    if (viaInterface)
        checkBoxMergeFromTable->setChecked(!YN);

    layout()->invalidate();
}

//+++ mergingTableChange()
void dan18::mergingTableChange()
{
    //+++ old numbers
    int oldN = tableMerge->columnCount() - 1;
    int oldM = tableMerge->rowCount();

    //+++ new numbers
    int N = spinBoxNtoMerge->value();
    int M = spinBoxMmergingCond->value();

    spinBoxRefForMerging->setMaximum(N);
    spinBoxRefForMerging->setMinimum(1);

    if (N == oldN && M == oldM)
        return;

    tableMerge->setColumnCount(1 + N);
    tableMerge->setRowCount(M);

    // +++ update col names
    QStringList colsNames;
    int colWidth = tableMerge->width() / 4;
    for (int cc = 0; cc < N; cc++)
    {
        colsNames << "Q-Range-" + QString::number(cc + 1);
        tableMerge->setColumnWidth(cc + 1, colWidth);
    }
    tableMerge->setColumnWidth(0, colWidth);

    colsNames.prepend("New Table Name");

    tableMerge->setHorizontalHeaderLabels(colsNames);

    if (N > 3)
        tableMerge->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    else
        tableMerge->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    //+++ create new-table-name fields
    for (int mm = oldM; mm < M; mm++)
    {
        tableMerge->setItem(mm, 0, new QTableWidgetItem);
        tableMerge->item(mm, 0)->setText("");
    }

    //+++ create table lists comboboxes
    for (int mm = 0; mm < M; mm++)
        for (int nn = 0; nn < N; nn++)
        {
            if (mm < oldM && nn < oldN)
                continue;
            auto *iTable = new QComboBoxInTable(mm, nn + 1, tableMerge);
            tableMerge->setCellWidget(mm, nn + 1, iTable);
        }
    updateListOfTables();
}

//+++ checkTableRange
bool dan18::checkTableRange(const QString &tName, int &Rows, int &Cols, double &Qmin, double &Qmax)
{
    Table *t;
    if (!app()->checkTableExistence(tName, t))
        return false;

    Cols = t->numCols();
    Rows = t->numRows();
    Qmin = t->text(0, 0).toDouble();
    Qmax = Qmin;

    for (int j = 1; j < Rows; j++)
    {
        if (t->text(j, 0).toDouble() < Qmin)
            Qmin = t->text(j, 0).toDouble();
        if (t->text(j, 0).toDouble() > Qmax)
            Qmax = t->text(j, 0).toDouble();
    }

    if (Cols < 2 || Rows == 0 || Qmin == Qmax)
        return false;

    return true;
}

//+++ addTable
bool dan18::addTable(const QString &table, gsl_matrix *&data, int &N, int Rows, int overlap, int &firstPosition,
                     bool &dIExist, bool &dQExist, bool &sigmaExist, bool &anisotropyExist)
{
    Table *t;
    if (!app()->checkTableExistence(table, t))
        return false;

    if (t->numCols() < 2 || t->numRows() == 0)
        return false;

    int coldI = t->indexOf("dI");
    int colSigma = t->indexOf("Sigma");
    int coldQ = t->indexOf("dQ");
    int colAnisotropy = t->indexOf("Anisotropy");

    double Q, I, dI, sigma, dQ, anisotropy;
    
    //+++ number filter
    static const QRegularExpression rx(R"(((\-|\+)?(\d*)?(\.|\,)?\d*((e|E)(\-|\+)\d*)?))");
    
    int NN = N - 1;
    bool first = true;
    if (NN > 0)
        first = false;

    double Qmax, Qmin, middle, diff;
    
    if (!first && overlap < 100)
    {
        Qmax = gsl_matrix_get(data, N - 1, 0);
        Qmin = t->text(0, 0).toDouble();
        middle = 0.5 * (Qmax + Qmin);
        diff = fabs(Qmax - Qmin);

        while (NN > 0 && gsl_matrix_get(data, NN, 0) > middle + diff / 2 * overlap / 100)
        {
            gsl_matrix_set(data, NN, 0, -99.99);
            NN--;
        }
    }

    firstPosition = N;
    
    for (int i = 0; i < Rows; i++)
    {
        if (rx.match(t->text(i, 0)).hasMatch() &&
            (first || overlap == 100 || t->text(i, 0).toDouble() > (middle - diff / 2 * overlap / 100)))
            Q = rx.match(t->text(i, 0)).captured(1).toDouble();
        else
            Q = -99.99;
        if (rx.match(t->text(i, 1)).hasMatch())
            I = rx.match(t->text(i, 1)).captured(1).toDouble();
        else
            I = -99.99;
        if (coldI > 1 && rx.match(t->text(i, coldI)).hasMatch())
            dI = rx.match(t->text(i, coldI)).captured(1).toDouble();
        else
            dI = -99.99;
        if (colSigma > 1 && rx.match(t->text(i, colSigma)).hasMatch())
            sigma = rx.match(t->text(i, colSigma)).captured(1).toDouble();
        else
            sigma = -99.99;
        if (coldQ > 1 && rx.match(t->text(i, coldQ)).hasMatch())
            dQ = rx.match(t->text(i, coldQ)).captured(1).toDouble();
        else
            dQ = -99.99;
        if (colAnisotropy > 1 && rx.match(t->text(i, colAnisotropy)).hasMatch())
            anisotropy = rx.match(t->text(i, colAnisotropy)).captured(1).toDouble();
        else
            anisotropy = -99.99;

        if (Q != -99.99 && I != -99.99)
        {
            gsl_matrix_set(data, N, 0, Q);
            gsl_matrix_set(data, N, 1, I);
            gsl_matrix_set(data, N, 2, dI);
            gsl_matrix_set(data, N, 3, sigma);
            gsl_matrix_set(data, N, 4, dQ);
            gsl_matrix_set(data, N, 5, anisotropy);
            N++;
        }
    }

    if (coldI > 1)
        dIExist = true;
    else
        dIExist = false;
    if (coldQ > 1)
        dQExist = true;
    else
        dQExist = false;
    if (colSigma > 1)
        sigmaExist = true;
    else
        sigmaExist = false;
    if (colAnisotropy > 1)
        anisotropyExist = true;
    else
        anisotropyExist = false;

    return true;
}

//+++ saveMergeInfo
void dan18::saveMergeInfo()
{
    bool ok;
    QString tableName = QInputDialog::getText(this, "Save information from merging interface as a new Table ",
                                              "Enter name of a new Table", QLineEdit::Normal, QString(), &ok);

    if (!ok || tableName.isEmpty())
        return;

    int N = spinBoxNtoMerge->value();
    int M = spinBoxMmergingCond->value();

    tableName = app()->generateUniqueName(tableName);

    Table *t = app()->newTable(tableName, M, N + 1);

    //+++ Cols Names
    t->setColName(0, "NewName");
    t->setColPlotDesignation(0, Table::None);
    t->setColumnType(0, Table::Text);

    for (int nn = 0; nn < N; nn++)
    {
        t->setColName(nn + 1, "Q-Range-" + QString::number(nn + 1));
        t->setColPlotDesignation(nn + 1, Table::None);
        t->setColumnType(nn + 1, Table::Text);
    }

    for (int mm = 0; mm < M; mm++)
    {
        for (int nn = 1; nn < N + 1; nn++)
        {
            auto *iTable = (QComboBoxInTable *)tableMerge->cellWidget(mm, nn);
            if (iTable->currentIndex() > 0)
                t->setText(mm, nn, iTable->currentText());
        }
        t->setText(mm, 0, tableMerge->item(mm, 0)->text());
    }

    //+++
    t->setWindowLabel("DAN::Merging::Template");
    app()->setListViewLabel(t->name(), "DAN::Merging::Template");
    app()->updateWindowLists(t);

    t->adjustColumnsWidth(false);
}

//+++ readMergeInfo
bool dan18::readMergeInfo(bool readFromActiveScript)
{
    Table *t;

    if (readFromActiveScript)
    {
        QString scriptTableName = comboBoxMakeScriptTable->currentText() + "-mergingTemplate";
        if (!app()->checkTableExistence(scriptTableName, t))
            return false;
    }
    else
    {
        if (!app()->activeWindow() || QString(app()->activeWindow()->metaObject()->className()) != "Table")
            return false;
        t = (Table *)app()->activeWindow();
    }

    int N = t->numCols() - 1;
    spinBoxNtoMerge->setValue(0);
    spinBoxNtoMerge->setValue(N);
    int M = t->numRows();
    spinBoxMmergingCond->setValue(0);
    spinBoxMmergingCond->setValue(M);

    //+++ Set Data-Sets List +++
    for (int nn = 0; nn < N; nn++)
        for (int mm = 0; mm < M; mm++)
        {
            auto *iTable = (QComboBoxInTable *)tableMerge->cellWidget(mm, nn + 1);
            if (iTable->findText(t->text(mm, nn + 1), Qt::MatchExactly) > 0)
                iTable->setCurrentIndex(iTable->findText(t->text(mm, nn + 1), Qt::MatchExactly));
        }

    QString nameQI;
    for (int mm = 0; mm < M; mm++)
    {
        nameQI = t->text(mm, 0);
        nameQI = nameQI.simplified();
        nameQI =
            nameQI.replace(" ", "-").replace("/", "-").replace("_", "-").replace(",", "-").replace(".", "-").remove(
                "%");
        tableMerge->item(mm, 0)->setText(nameQI);
    }

    return true;
}

//+++ mergeJoinVsRebin
void dan18::mergeJoinVsRebin(int tab)
{
    if (tab == 0)
    {
        pushButtonMerge->setText("  Merge|Join [project]");
        pushButtonMergeAscii->setText("  Merge|Join [ascii]");
        return;
    }
    pushButtonMerge->setText("  Merge|Rebin [project]");
    pushButtonMergeAscii->setText("  Merge|Rebin [ascii]");
}

//+++ selectActiveTable
void dan18::selectActiveTable()
{
    if (app()->activeWindow() && QString(app()->activeWindow()->metaObject()->className()) == "Table")
        lineEditCheckMergeActiveTable->setText(((Table *)app()->activeWindow())->name());
    else
        lineEditCheckMergeActiveTable->setText("");
}

//+++ updateListOfTables
void dan18::updateListOfTables()
{
    int N = spinBoxNtoMerge->value();
    int M = spinBoxMmergingCond->value();

    //+++ list of filtered tables
    QStringList tableNames;
    QList<MdiSubWindow *> windows = app()->windowsList();
    QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(lineEditFilter->text()));
    foreach (MdiSubWindow *w, windows)
        if (QString(w->metaObject()->className()) == "Table" && rx.match(w->name()).hasMatch())
            tableNames << w->name();
    tableNames.sort();
    tableNames.prepend("select");

    //+++ Set Data-Sets List
    for (int mm = 0; mm < M; mm++)
        for (int nn = 0; nn < N; nn++)
        {
            auto *iTable = (QComboBoxInTable *)tableMerge->cellWidget(mm, nn + 1);
            QString currentTable = iTable->currentText();
            iTable->clear();
            iTable->addItems(tableNames);
            if (tableNames.indexOf(currentTable) > 0)
                iTable->setCurrentIndex(tableNames.indexOf(currentTable));
        }
}

//+++ collectTableNamesSingleSample: Join or Rebin
bool dan18::collectTableNamesSingleSample(QString &nameMerged, QStringList &tablesToMerge, int M, int nn,
                                          Table *activeTable)
{
    tablesToMerge.clear();

    if (checkBoxMergeFromTable->isChecked())
    {
        if (tableMerge->item(nn, 0)->text() == "")
            nameMerged = "merged-" + QString::number(nn + 1);
        else
            nameMerged = tableMerge->item(nn, 0)->text();

        for (int mm = 0; mm < M; mm++)
            if (((QComboBoxInTable *)tableMerge->cellWidget(nn, mm + 1))->currentIndex() > 0)
                tablesToMerge << ((QComboBoxInTable *)tableMerge->cellWidget(nn, mm + 1))->currentText();
    }
    else
    {
        if (!checkBoxMergeVertical->isChecked())
        {
            if (activeTable->text(nn, 0) == "")
                nameMerged = "merged-" + QString::number(nn + 1);
            else
                nameMerged = activeTable->text(nn, 0);

            for (int mm = 0; mm < M; mm++)
                tablesToMerge << activeTable->text(nn, mm + 1);
        }
        else
        {
            if (activeTable->text(0, nn) == "")
                nameMerged = "merged-" + QString::number(nn + 1);
            else
                nameMerged = activeTable->text(0, nn);

            for (int mm = 0; mm < M; mm++)
                tablesToMerge << activeTable->text(mm + 1, nn);
        }
    }
    return true;
}

//+++ singleSampleMergeJoin
bool dan18::singleSampleMergeJoin(const QString &nameMerged, QStringList tablesToMerge, QString &labelList,
                                  gsl_matrix *&data, int &Nall, bool &dIExist, bool &dQExist, bool &sigmaExist,
                                  bool &anisotropyExist)
{
    if (nameMerged == "")
        return false;

    QList<double> QminList;
    QList<double> QmaxList;
    QList<int> rowsList;
    QList<int> colsList;
    QList<int> positions;
    QList<int> order;

    Nall = 0;
    int usedTablesNumber = 0;

    dIExist = true;
    dQExist = true;
    sigmaExist = true;
    anisotropyExist = true;

    //+++
    for (int mm = 0; mm < tablesToMerge.count(); mm++)
    {
        //+++
        int Rows = 0;
        int Cols = 0;
        double Qmin = 0;
        double Qmax = 0;
        //+++
        if (!checkTableRange(tablesToMerge[mm], Rows, Cols, Qmin, Qmax))
            continue;

        Nall += Rows;

        int pos = usedTablesNumber;

        for (int k = 0; k < usedTablesNumber; k++)
            if (Qmin < QminList[k])
            {
                pos--;
                order[k] = order[k] + 1;
            }

        //+++
        order << pos;
        positions << mm;
        QminList << Qmin;
        QmaxList << Qmax;
        rowsList << Rows;
        colsList << Cols;

        usedTablesNumber++;
    }

    if (Nall < 1)
        return false;

    //+++
    data = gsl_matrix_alloc(Nall, 6);

    //+++
    int NN = 0;
    int overlap = spinBoxOverlap->value();
    int firstPointFromList = 0;
    int firstPoint = 1;
    bool dIExistLocal, dQExistLocal, sigmaExistLocal, anisotropyExistLocal;
    //+++
    QList<int> firstPointS;
    //+++
    for (int k = 0; k < usedTablesNumber; k++)
    {
        int currentPos = order.indexOf(k);

        addTable(tablesToMerge[positions[currentPos]], data, NN, rowsList[currentPos], overlap, firstPoint,
                 dIExistLocal, dQExistLocal, sigmaExistLocal, anisotropyExistLocal);
        //+++ existing cols
        if (!dIExistLocal)
            dIExist = false;
        if (!dQExistLocal)
            dQExist = false;
        if (!sigmaExistLocal)
            sigmaExist = false;
        if (!anisotropyExistLocal)
            anisotropyExist = false;
        //+++
        firstPointS << firstPoint;
        //+++ Reference Set Check
        if (positions[currentPos] + 1 == spinBoxRefForMerging->value())
            firstPointFromList = k;
        //+++ label
        labelList += tablesToMerge[positions[currentPos]] + ", ";
    }

    //+++ Smart merging
    if (checkBoxSmart->isChecked())
    {
        double qExponent = comboBoxSmartSelect->currentIndex();
        bool scaleErrors = checkBoxScaleErrors->isChecked();
        int plusLeft = spinBoxSmartMergePlusLeft->value();
        int plusRight = spinBoxSmartMergePlusRight->value();

        //+++ left sets from reference adjasting...
        for (int k = firstPointFromList - 1; k >= 0; k--)
        {
            //--- finding of minimum of dataset [k+1] "reference"
            int QminInt = firstPointS[k + 1];
            double QminRef = gsl_matrix_get(data, QminInt, 0);

            //--- finding of maximum of dataset [k] left from reference
            int QmaxInt = firstPointS[k + 1] - 1;
            double QmaxRef = gsl_matrix_get(data, QmaxInt, 0);
            while (QmaxRef == -99.99)
            {
                QmaxInt--;
                QmaxRef = gsl_matrix_get(data, QmaxInt, 0);
            }

            //--- overlap range
            double currentOverlap = QmaxRef - QminRef;

            if (currentOverlap > 0 || (plusLeft > 0 && plusRight > 0))
            {
                //--- mean value of left dataset in overlap range
                double leftLevel = 0;
                int numLeftLevel = 0;
                double Q;
                while (gsl_matrix_get(data, QmaxInt, 0) >= QminRef || gsl_matrix_get(data, QmaxInt, 0) == -99.99)
                {
                    if (gsl_matrix_get(data, QmaxInt, 0) != -99.99 && gsl_matrix_get(data, QmaxInt, 1) != -99.99)
                    {
                        Q = gsl_matrix_get(data, QmaxInt, 0);
                        leftLevel += gsl_matrix_get(data, QmaxInt, 1) * pow(Q, qExponent);
                        numLeftLevel++;
                    }
                    QmaxInt--;
                }

                for (int lp = 0; lp < plusLeft; lp++)
                {
                    if (gsl_matrix_get(data, QmaxInt, 0) != -99.99 && gsl_matrix_get(data, QmaxInt, 1) != -99.99)
                    {
                        Q = gsl_matrix_get(data, QmaxInt, 0);
                        leftLevel += gsl_matrix_get(data, QmaxInt, 1) * pow(Q, qExponent);
                        numLeftLevel++;
                    }
                    QmaxInt--;
                }

                double rightLevel = 0;
                int numRightLevel = 0;
                while (gsl_matrix_get(data, QminInt, 0) <= QmaxRef || gsl_matrix_get(data, QminInt, 0) == -99.99)
                {
                    if (gsl_matrix_get(data, QminInt, 0) != -99.99 && gsl_matrix_get(data, QminInt, 1) != -99.99)
                    {
                        Q = gsl_matrix_get(data, QminInt, 0);
                        rightLevel += gsl_matrix_get(data, QminInt, 1) * pow(Q, qExponent);
                        numRightLevel++;
                    }
                    QminInt++;
                }

                for (int rp = 0; rp < plusRight; rp++)
                {
                    if (gsl_matrix_get(data, QminInt, 0) != -99.99 && gsl_matrix_get(data, QminInt, 1) != -99.99)
                    {
                        Q = gsl_matrix_get(data, QminInt, 0);
                        rightLevel += gsl_matrix_get(data, QminInt, 1) * pow(Q, qExponent);
                        numRightLevel++;
                    }
                    QminInt++;
                }

                if (numRightLevel > 0 && rightLevel > 0 && numLeftLevel > 0 && leftLevel > 0)
                {
                    double faktor = rightLevel / numRightLevel / leftLevel * numLeftLevel;
                    std::cout << QString::number(faktor, 'E', 5).toLocal8Bit().constData() << "\t";
                    for (int iii = firstPointS[k]; iii < firstPointS[k + 1]; iii++)
                    {
                        if (gsl_matrix_get(data, iii, 0) != -99.99 && gsl_matrix_get(data, iii, 1) != -99.99)
                        {
                            gsl_matrix_set(data, iii, 1, gsl_matrix_get(data, iii, 1) * faktor);
                            if (dIExist && scaleErrors)
                                gsl_matrix_set(data, iii, 2, gsl_matrix_get(data, iii, 2) * faktor);
                        }
                    }
                }
                else
                    break;
            }
            else
                break;
        }

        std::cout << QString::number(1.0, 'E', 5).toLocal8Bit().constData() << "\t";

        //+++ right sets from reference adjasting...
        for (int k = firstPointFromList + 1; k < usedTablesNumber; k++)
        {
            //+++ finding of minimum of dataset [k] right from reference
            int QminInt = firstPointS[k];
            double QminRef = gsl_matrix_get(data, QminInt, 0);
    
            //+++ finding of maximum of dataset [k-1] of "reference"
            int QmaxInt = firstPointS[k] - 1;
            double QmaxRef = gsl_matrix_get(data, QmaxInt, 0);
            while (QmaxRef == -99.99)
            {
                QmaxInt--;
                QmaxRef = gsl_matrix_get(data, QmaxInt, 0);
            }

            //+++ overlap range
            double currentOverlap = QmaxRef - QminRef;

            if (currentOverlap > 0 || (plusLeft > 0 && plusRight > 0))
            {
                //+++ mean value of left dataset in overlap range
                double leftLevel = 0;
                int numLeftLevel = 0;
                double Q;
                while (gsl_matrix_get(data, QmaxInt, 0) >= QminRef || gsl_matrix_get(data, QmaxInt, 0) == -99.99)
                {
                    if (gsl_matrix_get(data, QmaxInt, 0) != -99.99 && gsl_matrix_get(data, QmaxInt, 1) != -99.99)
                    {
                        Q = gsl_matrix_get(data, QmaxInt, 0);
                        leftLevel += gsl_matrix_get(data, QmaxInt, 1) * pow(Q, qExponent);
                        numLeftLevel++;
                    }
                    QmaxInt--;
                }

                for (int lp = 0; lp < plusLeft; lp++)
                {
                    if (gsl_matrix_get(data, QmaxInt, 0) != -99.99 && gsl_matrix_get(data, QmaxInt, 1) != -99.99)
                    {
                        Q = gsl_matrix_get(data, QmaxInt, 0);
                        leftLevel += gsl_matrix_get(data, QmaxInt, 1) * pow(Q, qExponent);
                        numLeftLevel++;
                    }
                    QmaxInt--;
                }

                double rightLevel = 0;
                int numRightLevel = 0;
                while (gsl_matrix_get(data, QminInt, 0) <= QmaxRef || gsl_matrix_get(data, QminInt, 0) == -99.99)
                {
                    if (gsl_matrix_get(data, QminInt, 0) != -99.99 && gsl_matrix_get(data, QminInt, 1) != -99.99)
                    {
                        Q = gsl_matrix_get(data, QmaxInt, 0);
                        rightLevel += gsl_matrix_get(data, QminInt, 1) * pow(Q, qExponent);
                        numRightLevel++;
                    }
                    QminInt++;
                }

                for (int rp = 0; rp < plusRight; rp++)
                {
                    if (gsl_matrix_get(data, QminInt, 0) != -99.99 && gsl_matrix_get(data, QminInt, 1) != -99.99)
                    {
                        Q = gsl_matrix_get(data, QmaxInt, 0);
                        rightLevel += gsl_matrix_get(data, QminInt, 1) * pow(Q, qExponent);
                        numRightLevel++;
                    }
                    QminInt++;
                }

                if (numRightLevel > 0 && rightLevel > 0 && numLeftLevel > 0 && leftLevel > 0)
                {
                    double faktor = leftLevel / numLeftLevel / rightLevel * numRightLevel;
                    std::cout << QString::number(faktor, 'E', 5).toLocal8Bit().constData() << "\t";
                    int lastPoint;
                    if (k + 1 == usedTablesNumber)
                        lastPoint = NN;
                    else
                        lastPoint = firstPointS[k + 1];
                    for (int iii = firstPointS[k]; iii < lastPoint; iii++)
                    {
                        if (gsl_matrix_get(data, iii, 0) != -99.99 && gsl_matrix_get(data, iii, 1) != -99.99)
                        {
                            gsl_matrix_set(data, iii, 1, gsl_matrix_get(data, iii, 1) * faktor);
                            if (scaleErrors)
                                gsl_matrix_set(data, iii, 2, gsl_matrix_get(data, iii, 2) * faktor);
                        }
                    }
                }
                else
                    break;
            }
            else
                break;
        }
        std::cout << nameMerged.toLatin1().constData() << "\n" << std::flush;
    }
    Nall = NN;
    return true;
}

//+++ singleSampleMergeRebin
bool dan18::singleSampleMergeRebin(const QString &nameMerged, QStringList tablesToMerge, QString &labelList,
                                   gsl_matrix *&data, int &Nall, bool &dIExist, bool &dQExist, bool &sigmaExist,
                                   bool &anisotropyExist)
{
    Nall = spinBoxRebinPoints->value();
    bool logScale = checkBoxLogStepRebin->isChecked();

    dIExist = true;
    dQExist = true;
    sigmaExist = true;
    anisotropyExist = true;

    Table *t;
    QStringList listCleanned;
    int nTableCleanned = 0;

    for (int i = 0; i < tablesToMerge.count(); i++)
    {
        if (!app()->checkTableExistence(tablesToMerge[i], t))
            continue;
        if (t->numCols() < 2)
            continue;
        if (t->indexOf("dI") < 1)
            dIExist = false;
        if (t->indexOf("Sigma") < 1)
            sigmaExist = false;
        if (t->indexOf("dQ") < 1)
            dQExist = false;
        if (t->indexOf("Anisotropy") < 1)
            anisotropyExist = false;

        nTableCleanned++;
        listCleanned << tablesToMerge[i];
    }

    double qMin = 1000, qMax = -1000;
    double qCurrent;

    if (radioButton->isChecked())
    {
        //+++ Q-range from data
        for (int i = 0; i < nTableCleanned; i++)
        {
            app()->checkTableExistence(listCleanned[i], t);

            for (int rr = 0; rr < t->numRows(); rr++)
            {
                qCurrent = t->text(rr, 0).toDouble();
                if (!logScale && qCurrent < qMin)
                    qMin = qCurrent;
                if (logScale && qCurrent > 0 && qCurrent < qMin)
                    qMin = qCurrent;
                if (qCurrent > qMax)
                    qMax = qCurrent;
            }
        }
    }
    else
    {
        qMin = spinBoxRebinQmin->value();
        qMax = spinBoxRebinQmax->value();
        if (qMax < qMin)
        {
            double tmp = qMin;
            qMin = qMax;
            qMax = tmp;
        }
    }

    if (qMin == qMax)
        return false;

    if (logScale && qMin <= 0 && qMax <= 0)
        logScale = false;
    if (logScale && qMin <= 0 && qMax > 0)
        qMin = 0.00001;

    // double qStep=fabs(qMax-qMin)/(numberPoints-1);

    auto *n = new int[Nall];
    auto *Q = new double[Nall];
    auto *Qmin = new double[Nall];
    auto *Qmax = new double[Nall];
    auto *I = new double[Nall];
    auto *dI = new double[Nall];
    auto *Sigma = new double[Nall];
    auto *dQ = new double[Nall];
    auto *anisotropy = new double[Nall];

    for (int i = 0; i < Nall; i++)
    {
        Qmin[i] = qMax;
        Qmax[i] = qMin;
        n[i] = 0;
        Q[i] = 0;
        I[i] = 0;
        dI[i] = 0;
        dQ[i] = 0;
        Sigma[i] = 0;
        anisotropy[i] = 0;
    }

    int currentPosition;
    double iCurrent, diCurrent, sigmaCurrent;

    double errLowLimit = spinBoxErrLeftLimitTof->value() / 100.00;
    double errUpperLimit = spinBoxErrRightLimitTof->value() / 100.00;

    for (int i = 0; i < nTableCleanned; i++)
        if (app()->checkTableExistence(listCleanned[i], t))
        {
            int dICol = t->indexOf("dI");
            int sigmaCol = t->indexOf("Sigma");
            int dQCol = t->indexOf("dQ");
            int anisotropyCol = t->indexOf("Anisotropy");

            for (int rr = 0; rr < t->numRows(); rr++)
            {
                qCurrent = t->text(rr, 0).toDouble();
                if (qCurrent < qMin || qCurrent > qMax)
                    continue;

                iCurrent = t->text(rr, 1).toDouble();

                if (dIExist)
                {
                    diCurrent = t->text(rr, dICol).toDouble();
                    if (diCurrent / iCurrent < errLowLimit || diCurrent / iCurrent > errUpperLimit)
                        continue;
                }

                if (logScale)
                    currentPosition = static_cast<int>(
                        std::round((log10(qCurrent) - log10(qMin)) / (log10(qMax) - log10(qMin)) * (Nall - 1)));
                else
                    currentPosition = static_cast<int>(std::round((qCurrent - qMin) / (qMax - qMin) * (Nall - 1)));

                n[currentPosition]++;

                if (qCurrent < Qmin[currentPosition])
                    Qmin[currentPosition] = qCurrent;

                if (qCurrent > Qmax[currentPosition])
                    Qmax[currentPosition] = qCurrent;

                Q[currentPosition] += qCurrent;
                I[currentPosition] += iCurrent;

                if (dIExist)
                    dI[currentPosition] += diCurrent * diCurrent;

                if (sigmaExist)
                    Sigma[currentPosition] += t->text(rr, sigmaCol).toDouble() * t->text(rr, sigmaCol).toDouble();

                if (dQExist)
                    dQ[currentPosition] += t->text(rr, dQCol).toDouble() * t->text(rr, dQCol).toDouble();

                if (anisotropyExist)
                    anisotropy[currentPosition] += t->text(rr, anisotropyCol).toDouble();
            }
        }

    for (int i = 0; i < Nall; i++)
    {
        if (n[i] == 0)
        {
            Qmin[i] = qMin;
            Qmax[i] = qMax;
            n[i] = 0;
            Q[i] = -99.99;
            I[i] = 0;
            dI[i] = 0;
            dQ[i] = 0;
            Sigma[i] = 0;
            anisotropy[i] = 0;

            continue;
        }

        Q[i] = Q[i] / n[i];
        I[i] = I[i] / n[i];
        dI[i] = sqrt(dI[i]) / n[i];
        Sigma[i] = sqrt(Sigma[i]) / n[i] + (Qmax[i] - Qmin[i]) / 2;
        dQ[i] = sqrt(dQ[i]) / n[i];
        anisotropy[i] = anisotropy[i] / n[i];
    }

    data = gsl_matrix_alloc(Nall, 6);

    for (int ii = 0; ii < Nall; ii++)
    {
        gsl_matrix_set(data, ii, 0, Q[ii]);
        gsl_matrix_set(data, ii, 1, I[ii]);
        gsl_matrix_set(data, ii, 2, dI[ii]);
        gsl_matrix_set(data, ii, 3, Sigma[ii]);
        gsl_matrix_set(data, ii, 4, dQ[ii]);
        gsl_matrix_set(data, ii, 5, anisotropy[ii]);
    }

    labelList = listCleanned[0];
    if (nTableCleanned > 1)
        labelList += "," + listCleanned[1];
    if (nTableCleanned == 3 || nTableCleanned == 4)
        labelList += "," + listCleanned[2];
    if (nTableCleanned == 4)
        labelList += "," + listCleanned[3];
    if (nTableCleanned > 4)
        labelList += ", ... , " + listCleanned[nTableCleanned - 2] + "," + listCleanned[nTableCleanned - 1];

    return true;
}

//+++ mergeMethod
void dan18::mergeMethod(bool asciiYN)
{
    //+++ current folder
    auto *cf = (Folder *)app()->current_folder;

    if (asciiYN)
    {
        QString path = lineEditPathRAD->text();
        QDir dd(path);
        if (!dd.cd("./ASCII-QI-MERGED/"))
            dd.mkdir("./ASCII-QI-MERGED/");
    }
    else
        app()->changeFolder("DAN :: Merge.1D");

    gsl_set_error_handler_off();


    int M; // # merging samples
    int N; // # merging conditionns
    QString activeTableName = lineEditCheckMergeActiveTable->text();
    Table *activeTable;
    bool velticalYN = checkBoxMergeVertical->isChecked();

    bool checkSelection = false;
    QItemSelectionModel *select;
    const QModelIndex mIndex;

    if (checkBoxMergeFromTable->isChecked())
    {
        velticalYN = false;
        M = spinBoxNtoMerge->value();
        N = spinBoxMmergingCond->value();
        select = tableMerge->selectionModel();
    }
    else
    {
        if (activeTableName == "")
        {
            QMessageBox::warning(this, tr("QtiSAS"), tr("<h4>Select active merging table...</h4>"));
            return;
        }
        if (!app()->checkTableExistence(activeTableName, activeTable))
        {
            QMessageBox::warning(this, tr("QtiSAS"), tr("<h4>Select active merging table...</h4>"));
            return;
        }
        if (velticalYN)
        {
            M = activeTable->numRows();
            N = activeTable->numCols() - 1;
        }
        else
        {
            M = activeTable->numCols();
            N = activeTable->numRows() - 1;
        }
        select = activeTable->table()->selectionModel();
    }

    if (M < 2 || N < 1)
    {
        QMessageBox::warning(this, tr("QtiSAS"), tr("<h4>There are nothing to do...</h4>"));
        return;
    }

    //+++
    if (select->hasSelection())
        checkSelection = true;

    //+++ Progress Dialog +++
    QProgressDialog progress;
    progress.setMinimumDuration(10);
    progress.setWindowModality(Qt::WindowModal);
    progress.setRange(0, N);
    progress.setCancelButtonText("Abort Merging Processing");
    progress.setMinimumWidth(400);
    progress.setMaximumWidth(4000);
    progress.setLabelText("Data Merging Processing Started...");
    progress.show();

    QString textProgress;
    //+++ time steps
    QTime dt = QTime::currentTime();

    if (checkBoxSmart->isChecked() && toolBoxMerge->currentIndex() == 0)
    {
        std::cout << "\n\nMegring Scaling Factors :: Smart Mode \n";
        for (int mm = 0; mm < M; mm++)
            std::cout << "Q-Range-" << mm + 1 << "\t";
        std::cout << "\n";
    }

    QString nameMerged;
    QStringList tablesToMerge;

    for (int nn = 0; nn < N; nn++)
    {
        if (checkSelection && ((!velticalYN && !select->isRowSelected(nn, mIndex)) ||
                               (velticalYN && !select->isColumnSelected(nn, mIndex))))
            continue;

        collectTableNamesSingleSample(nameMerged, tablesToMerge, M, nn, activeTable);

        gsl_matrix *data;

        int Nall;
        int Ncols;
        bool dIExist, sigmaExist, dQExist, anisotropyExist;
        QString labelList;

        if (toolBoxMerge->currentIndex() == 0)
        {
            if (!singleSampleMergeJoin(nameMerged, tablesToMerge, labelList, data, Nall, dIExist, dQExist, sigmaExist,
                                       anisotropyExist))
                continue;
        }
        else
        {
            if (!singleSampleMergeRebin(nameMerged, tablesToMerge, labelList, data, Nall, dIExist, dQExist, sigmaExist,
                                        anisotropyExist))
                continue;
        }

        saveMergedMatrix(nameMerged, labelList, data, Nall, dIExist, dQExist, sigmaExist, anisotropyExist, asciiYN);

        gsl_matrix_free(data);

        textProgress = "Merged: " + QString::number(nn + 1) + " / " + QString::number(N) + "\n";
        textProgress += "Time left: " +
                        QString::number(int(dt.msecsTo(QTime::currentTime()) / 1000.0 * N / (nn + 1) -
                                            dt.msecsTo(QTime::currentTime()) / 1000.0)) +
                        " / " + QString::number(int(dt.msecsTo(QTime::currentTime()) / 1000.0 * N / (nn + 1))) + "\n";
        if (asciiYN)
            textProgress += "\nASCII file id saved in the folder: \n" + lineEditPathRAD->text() + "ASCII-QI-MERGED/";

        progress.setLabelText(textProgress);
        progress.setValue(nn + 1);
        if (progress.wasCanceled())
            break;
    }

    if (!asciiYN)
        app()->changeFolder(cf, true);
}

//+++ saveMergedMatrix
void dan18::saveMergedMatrix(QString name, const QString &labelList, gsl_matrix *data, int N, bool dIExist,
                             bool dQExist, bool sigmaExist, bool anisotropyExist, bool asciiYN)
{
    int numberCols = 2 + int(dIExist) + int(dQExist) + int(sigmaExist) + int(anisotropyExist);

    int dICol = 1 + int(dIExist);
    int dQCol = dICol + int(dQExist);
    int sigmaCol = dICol + int(sigmaExist);
    if (dQExist && sigmaExist)
        sigmaCol++;
    int anisotropyCol = numberCols - 1;

    if (asciiYN)
        return saveMergedMatrixAscii(name, data, N, dIExist, dQExist, sigmaExist, anisotropyExist);
    
    int removeFirst = spinBoxRemoveFirstMerging->value();
    int removeLast = spinBoxRemoveLastMerging->value();
    if (removeFirst + removeLast >= N)
    {
        removeFirst = 0;
        removeLast = 0;
    }

    QString colLabel = name;
    
    name = name.remove("=").replace(".", "_").remove(" ");
    static const QRegularExpression re(QString::fromUtf8("[`~!@#$%^&*()_â€”+=|:;<>Â«Â»,.?/{}\'\"\\\\[\\]]"));
    name = name.replace(re, "-");

    if (checkBoxMergeIndexing->isChecked())
    {
        name += "-v-";
        name = app()->generateUniqueName(name);
    }

    QString label = "Merged Tables >> " + labelList;
    
    bool newTableOutput = false;

    //+++ create table
    Table* t;

    bool tableExist = app()->checkTableExistence(name, t);

    if (checkBoxMergeIndexing->isChecked() || !tableExist)
    {
        t = app()->newHiddenTable(name, label, N - removeFirst - removeLast, numberCols);
        app()->hideWindow(t);
    }
    else
    {
        t->blockSignals(true);

        if (t->numRows() != N - removeFirst - removeLast)
            t->setNumRows(N - removeFirst - removeLast);

        if (t->numCols() < numberCols)
            t->setNumCols(numberCols);
    }

    if (dIExist)
    {
        t->setColName(dICol, "dI");
        t->setColPlotDesignation(dICol, Table::yErr);
        t->setColNumericFormat(2, 8, dICol, true);
    }
    if (dQExist)
    {
        t->setColName(dQCol, "dQ");
        t->setColPlotDesignation(dQCol, Table::xErr);
        t->setColNumericFormat(2, 8, dQCol, true);
    }
    if (sigmaExist)
    {
        t->setColName(sigmaCol, "Sigma");
        t->setColPlotDesignation(sigmaCol, Table::xErr);
        t->setColNumericFormat(2, 8, sigmaCol, true);
    }
    if (anisotropyExist)
    {
        t->setColName(anisotropyCol, "Anisotropy");
        t->setColPlotDesignation(anisotropyCol, Table::Y);
        t->setColNumericFormat(2, 8, anisotropyCol, true);
    }

    t->setColName(0, "Q");
    t->setColName(1, "I");
    t->setColComment(1, colLabel);
    
    t->setColNumericFormat(2, 8, 0, true);
    t->setColNumericFormat(2, 8, 1, true);
    
    t->setColPlotDesignation(0,Table::X);
    t->setColPlotDesignation(1,Table::Y);
    
    t->setHeaderColType();
    
    app()->updateWindowLists(t);
    
    int currentRow = 0;
    
    for (int i = removeFirst; i < N - removeLast; i++)
    {
        if (gsl_matrix_get(data, i, 0) != -99.99)
        {
            t->setText(currentRow, 0, QString::number(gsl_matrix_get(data, i, 0), 'E'));
            t->setText(currentRow, 1, QString::number(gsl_matrix_get(data, i, 1), 'E'));

            if (dIExist)
            {
                if (gsl_matrix_get(data, i, 2) == -99.99)
                    t->clearCell(currentRow, dICol);
                else
                    t->setText(currentRow, dICol, QString::number(gsl_matrix_get(data, i, 2), 'E'));
            }

            if (sigmaExist)
            {
                if (gsl_matrix_get(data, i, 3) == -99.99)
                    t->clearCell(currentRow, sigmaCol);
                else
                    t->setText(currentRow, sigmaCol, QString::number(gsl_matrix_get(data, i, 3), 'E'));
            }
            
            if (dQExist)
            {
                if (gsl_matrix_get(data, i, 4) == -99.99)
                    t->clearCell(currentRow, dQCol);
                else
                    t->setText(currentRow, dQCol, QString::number(gsl_matrix_get(data, i, 4), 'E'));
            }

            if (anisotropyExist)
            {
                if (gsl_matrix_get(data, i, 5) == -99.99)
                    t->clearCell(currentRow, anisotropyCol);
                else
                    t->setText(currentRow, anisotropyCol, QString::number(gsl_matrix_get(data, i, 5), 'E'));
            }
            currentRow++;
        }
    }

    t->setNumRows(currentRow);

    t->blockSignals(false);

    //+++ adjust cols
    t->adjustColumnsWidth(false);
    
    t->notifyChanges();
    app()->modifiedProject(t);
    app()->showFullRangeAllPlots(name);
}

//+++ saveMergedMatrixAscii
void dan18::saveMergedMatrixAscii(QString name, gsl_matrix *data, int N, bool dIExist, bool dQExist, bool sigmaExist,
                                  bool anisotropyExist)
{
    int removeFirst = spinBoxRemoveFirstMerging->value();
    int removeLast = spinBoxRemoveLastMerging->value();
    if (removeFirst + removeLast >= N)
    {
        removeFirst = 0;
        removeLast = 0;
    }
    name = "QI-MERGED-" + name;

    QString path = lineEditPathRAD->text() + "/ASCII-QI-MERGED/";

    if (checkBoxMergeIndexing->isChecked())
    {
        name += "-v-";

        int fff = 1;
        bool fileExist = true;

        while (fileExist)
            if (QFile::exists(path + name + QString::number(fff) + ".DAT"))
                fff++;
            else
                fileExist = false;

        name += QString::number(fff);
    }

    name = name + ".DAT";
    
    QFile f(path + name);

    if (!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this,tr("QtiSAS"),
                             tr("<h4>... could not write file ...</h4>"));
        return;
    }

    QTextStream stream(&f);

    if (checkBoxASCIIheader->isChecked())
    {
        stream << "###   Q[1/A]        I[1/cm]       ";
        if (dIExist)
            stream << "dI[1/cm]     ";
        if (dQExist)
            stream << "   dQ[1/A]     ";
        if (sigmaExist)
            stream << "Sigma[1/A]     ";
        if (anisotropyExist)
            stream << " Anisotropy     ";

        stream << "\n";
    }

    for (int i = removeFirst; i < N - removeLast; i++)
        if (gsl_matrix_get(data, i, 0) != -99.99)
        {
            stream << QString::number(gsl_matrix_get(data, i, 0), 'E');
            stream << "   " + QString::number(gsl_matrix_get(data, i, 1), 'E');

            if (dIExist)
            {
                if (gsl_matrix_get(data, i, 2) == -99.99)
                    stream << "   ---";
                else
                    stream << "   " + QString::number(gsl_matrix_get(data, i, 2), 'E');
            }
            if (dQExist)
            {
                if (gsl_matrix_get(data, i, 4) == -99.99)
                    stream << "   ---";
                else
                    stream << "   " + QString::number(gsl_matrix_get(data, i, 4), 'E');
            }

            if (sigmaExist)
            {
                if (gsl_matrix_get(data, i, 3) == -99.99)
                    stream << "   ---";
                else
                    stream << "   " + QString::number(gsl_matrix_get(data, i, 3), 'E');
            }
            
            if (anisotropyExist)
            {
                if (gsl_matrix_get(data, i, 5) == -99.99)
                    stream << "   ---";
                else
                    stream << "   " + QString::number(gsl_matrix_get(data, i, 5), 'E');
            }

            stream << "\n";
        }
    f.close();
}