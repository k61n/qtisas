/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Singular value decomposition interface (SANS)
 ******************************************************************************/


#include <QDockWidget>
#include <QSettings>
#include <QString>

#include "numericLocale.h"
#include "settings.h"
#include "svd.h"


svd::svd(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    // read settings
    readSettings();

    // connections
    connect(spinBoxMsvd, SIGNAL(valueChanged(int)), this, SLOT(on_spinBoxMsvd_valueChanged(int)));
    connect(spinBoxNsvd, SIGNAL(valueChanged(int)), this, SLOT(on_spinBoxNsvd_valueChanged(int)));
    connect(tableNsvd, SIGNAL(cellChanged(int, int)), this, SLOT(slotTableNsvdChanged(int, int)));
    connect(tableMsvd, SIGNAL(cellChanged(int, int)), this, SLOT(slotTableMsvdChanged(int, int)));
    connect(pushButtonSVD1dataTransfer, SIGNAL(clicked()), this, SLOT(slotSVD1dataTransfer()));
    connect(lineEditRhoD, SIGNAL(editingFinished()), this, SLOT(slotCheckRhoHD()));
    connect(lineEditRhoH, SIGNAL(editingFinished()), this, SLOT(slotCheckRhoHD()));
    connect(comboBoxSVDtables, &QComboBox::textActivated, this, &svd::slotReadSvdData);
    connect(pushButtonUpdate, SIGNAL(clicked()), this, SLOT(findSvdTables()));

    // tables tuning
    tableMsvd->setColumnWidth(0, 125);
    tableMsvd->setColumnWidth(1, 125);
    tableNsvd->setColumnWidth(0, 150);
    tableNsvd->setColumnWidth(1, 125);

    d_widgetLocale = locale();
}

svd::~svd()
{
    writeSettings();
}

void svd::writeSettings()
{
    QSettings *settings = Settings::DefaultSettings();

    settings->beginGroup("/SVD");
    settings->setValue("/RhoH", uiToC(lineEditRhoH->text()));
    settings->setValue("/RhoD", uiToC(lineEditRhoD->text()));
    settings->setValue("/Msvd", spinBoxMsvd->value());
    settings->setValue("/Nsvd", spinBoxNsvd->value());

    QStringList TabList;
    QString ss;

    for (int ii = 0; ii < spinBoxMsvd->value(); ii++)
    {
        ss = " ";
        if (tableMsvd->item(ii, 0))
            ss = uiToC(tableMsvd->item(ii, 0)->text());
        TabList << ss;
    }

    if (TabList.count() == spinBoxMsvd->value())
    {
        settings->setValue("/svdSolventComp", TabList);
        TabList.clear();
    }
    for (int ii = 0; ii < spinBoxMsvd->value(); ii++)
    {
        ss = " ";
        if (tableMsvd->item(ii, 1))
            ss = uiToC(tableMsvd->item(ii, 1)->text());
        TabList << ss;
    }
    if (TabList.count() > 0)
    {
        settings->setValue("/svdSolvSLD", TabList);
        TabList.clear();
    }
    for (int ii = 0; ii < spinBoxMsvd->value(); ii++)
    {
        if (tableMsvd->item(ii, 2))
            ss = tableMsvd->item(ii, 2)->text();
        else
            ss = " ";
        TabList << ss;
    }
    if (TabList.count() == spinBoxMsvd->value())
    {
        settings->setValue("/svdTabNames", TabList);
        TabList.clear();
    }

    QStringList svdComments, svdSLDinH, svdSLDinD;
    for (int ii = 0; ii < spinBoxNsvd->value(); ii++)
    {
        ss = " ";
        if (tableNsvd->item(ii, 0))
            ss = tableNsvd->item(ii, 0)->text();
        svdComments << ss;

        ss = " ";
        if (tableNsvd->item(ii, 1))
            ss = uiToC(tableNsvd->item(ii, 1)->text());

        svdSLDinH << ss;

        ss = " ";
        if (tableNsvd->item(ii, 2))
            ss = uiToC(tableNsvd->item(ii, 2)->text());

        svdSLDinD << ss;
    }
    if (svdComments.count() > 0)
    {
        settings->setValue("/svdComments", svdComments);
        svdComments.clear();
    }
    if (svdSLDinH.count() > 0)
    {
        settings->setValue("/svdSLDinH", svdSLDinH);
        svdSLDinH.clear();
    }
    if (svdSLDinD.count() > 0)
    {
        settings->setValue("/svdSLDinD", svdSLDinD);
        svdSLDinD.clear();
    }

    settings->endGroup();

    delete settings;
}

void svd::readSettings()
{
    QSettings *settings = Settings::DefaultSettings();

    settings->beginGroup("/SVD");
    QString ss = settings->value("/RhoH", "-0.56e10").toString();
    if (ss != "")
        lineEditRhoH->setText(cToUi(ss));
    ss = settings->value("/RhoD", "+6.34e10").toString();
    if (ss != "")
        lineEditRhoD->setText(cToUi(ss));

    int nn = settings->value("/Nsvd", 2).toInt();
    spinBoxNsvd->setValue(nn);
    int mm = settings->value("/Msvd", 2).toInt();
    spinBoxMsvd->setValue(mm);

    on_spinBoxMsvd_valueChanged(mm);
    on_spinBoxNsvd_valueChanged(nn);

    QStringList svdTabNames = settings->value("/svdTabNames").toStringList();
    QStringList svdSolventComp = settings->value("/svdSolventComp").toStringList();
    QStringList svdSolvSLD = settings->value("/svdSolvSLD").toStringList();

    for (int ii = 0; ii < mm; ii++)
    {
        if (ii < svdTabNames.count())
        {
            ss = svdTabNames[ii];
            auto *_item1 = new QTableWidgetItem();
            _item1->setText(ss);
            tableMsvd->setItem(ii, 2, _item1);
        }
        if (ii < svdSolventComp.count())
        {
            ss = svdSolventComp[ii];
            auto *_item2 = new QTableWidgetItem();
            _item2->setText(cToUi(ss));
            tableMsvd->setItem(ii, 0, _item2);
        }
        if (ii < svdSolvSLD.count())
        {
            ss = svdSolvSLD[ii];
            auto *_item3 = new QTableWidgetItem();
            _item3->setText(cToUi(ss));
            tableMsvd->setItem(ii, 1, _item3);
        }
    }

    QStringList svdComments = settings->value("/svdComments").toStringList();
    QStringList svdSLDinH = settings->value("/svdSLDinH").toStringList();
    QStringList svdSLDinD = settings->value("/svdSLDinD").toStringList();

    for (int ii = 0; ii < nn; ii++)
    {
        if (ii < svdComments.count())
        {
            ss = svdComments[ii];
            auto *_item1 = new QTableWidgetItem();
            _item1->setText(ss);
            tableNsvd->setItem(ii, 0, _item1);
        }
        if (ii < svdSLDinH.count())
        {
            ss = svdSLDinH[ii];
            auto *_item2 = new QTableWidgetItem();
            _item2->setText(cToUi(ss));
            tableNsvd->setItem(ii, 1, _item2);
        }
        if (ii < svdSLDinD.count())
        {
            ss = svdSLDinD[ii];
            auto *_item3 = new QTableWidgetItem();
            _item3->setText(cToUi(ss));
            tableNsvd->setItem(ii, 2, _item3);
        }
    }
    settings->endGroup();
    delete settings;
}
//+++ numbers in the SVD interface
double svd::uiToDouble(const QString &text) const
{
    double value = 0.0;
    NumericLocale::toDouble(text, locale(), value);
    return value;
}

QString svd::uiNumber(double value) const
{
    return locale().toString(value, 'e', 5);
}

QString svd::uiToC(const QString &text) const
{
    return NumericLocale::convert(text, locale(), QLocale::c(), 'e', 5);
}

QString svd::cToUi(const QString &text) const
{
    return NumericLocale::convert(text, QLocale::c(), locale(), 'e', 5);
}

QString svd::tableToUi(Table *t, int row, int col) const
{
    return NumericLocale::convert(t->text(row, col), t->cellLocale(col), locale(), 'e', 5);
}

void svd::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() != QEvent::LocaleChange || locale() == d_widgetLocale)
        return;

    const QLocale from = d_widgetLocale, to = locale();
    const auto convert = [&from, &to](const QString &text) { return NumericLocale::convert(text, from, to, 'e', 5); };
    for (QLineEdit *edit : {lineEditRhoH, lineEditRhoD})
        edit->setText(convert(edit->text()));
    for (QTableWidget *table : {tableMsvd, tableNsvd})
    {
        const bool blocked = table->blockSignals(true);
        const int first = table == tableMsvd ? 0 : 1; // tableMsvd: H/D fraction and SLD; tableNsvd: SLD in H and D
        for (int row = 0; row < table->rowCount(); row++)
            for (int col = first; col < first + 2; col++)
                if (QTableWidgetItem *item = table->item(row, col))
                    item->setText(convert(item->text()));
        table->blockSignals(blocked);
    }
    d_widgetLocale = to;
}
//+++ app(this): connection to QtiPlot
ApplicationWindow *app(QWidget *widget)
{
    ApplicationWindow *appM;

    auto *docWin = (QDockWidget *)widget->parent();

    if (docWin->isFloating())
        appM = (ApplicationWindow *)docWin->parent()->parent();
    else
        appM = (ApplicationWindow *)docWin->parent();

    return appM;
}
//+++ Text-To-Results Log-Window
void toResLogAll(const QString &interfaceName, const QString &text, QWidget *widget)
{
    QDateTime dt = QDateTime::currentDateTime();
    QString info = dt.toString("dd.MM | hh:mm ->>" + interfaceName + ">> ");
    info += text;
    info += "\n";
    app(widget)->results->setText(app(widget)->results->toPlainText() + text);
}

void svd::on_spinBoxMsvd_valueChanged(int value)
{
    tableMsvd->setRowCount(value);
}

void svd::on_spinBoxNsvd_valueChanged(int value)
{
    tableNsvd->setRowCount(value);
}
//+++ mREAD-SVD-DATA-FROM-TABLE
void svd::slotReadSvdData(QString tableName)
{
    Table *t;
    QString ss;

    if (!app(this)->checkTableExistence(std::move(tableName), t))
        return;

    ss = t->name();
    int nn = 0;
    while (t->text(nn, 0) != "")
        nn++;

    nn--;
    lineEditRhoH->setText(tableToUi(t, 0, 1));
    lineEditRhoD->setText(tableToUi(t, 0, 2));

    spinBoxNsvd->setValue(nn);
    for (int ii = 0; ii < nn; ii++)
    {
        auto *_item1 = new QTableWidgetItem();
        _item1->setText(t->text(ii + 1, 0));
        tableNsvd->setItem(ii, 0, _item1);

        auto *_item2 = new QTableWidgetItem();
        _item2->setText(tableToUi(t, ii + 1, 1));
        tableNsvd->setItem(ii, 1, _item2);

        auto *_item3 = new QTableWidgetItem();
        _item3->setText(tableToUi(t, ii + 1, 2));
        tableNsvd->setItem(ii, 2, _item3);
    }

    nn = 0;
    while (t->text(nn, 3) != "")
        nn++;

    spinBoxMsvd->setValue(nn);
    for (int ii = 0; ii < nn; ii++)
    {
        auto *_item1 = new QTableWidgetItem();
        _item1->setText(tableToUi(t, ii, 4));
        tableMsvd->setItem(ii, 0, _item1);

        auto *_item2 = new QTableWidgetItem();
        _item2->setText(tableToUi(t, ii, 5));
        tableMsvd->setItem(ii, 1, _item2);

        auto *_item3 = new QTableWidgetItem();
        _item3->setText(t->text(ii, 3));
        tableMsvd->setItem(ii, 2, _item3);
    }
}
//+++ Data Transfer
void svd::slotSVD1dataTransfer(const QString &Name, int Nsvd, QStringList TexT, gsl_matrix *SLDcomponent, int Msvd,
                               QStringList Dcomposition, QStringList FileNames, gsl_vector *SLDsolvent, int Ksvd)
{

    // Temp variables
    int KN, coln;
    double gslTemp, rhoHD, rho1, rho2, Bk, error;
    QString ss;

    // definition of variables...
    int M = Msvd;                         // Number of H/D Concentrations
    int N = Nsvd * (Nsvd - 1) / 2 + Nsvd; // Number of independent Partial structure factors
    int K = Ksvd;                         // Number of Q-points

    int NM;
    if (Nsvd > Msvd)
        NM = Nsvd;
    else
        NM = Msvd;
    if (K > NM)
        NM = K; // Table Dimension

    Table *t; // Table-Pointer to Transfer Data

    // A-matrix SVD-on
    gsl_matrix *A = gsl_matrix_calloc(M, N); // Contrast-matrix A
    gsl_matrix *U = gsl_matrix_calloc(M, N); // A = USV ^ T
    gsl_matrix *V = gsl_matrix_calloc(N, N); //
    gsl_vector *S = gsl_vector_calloc(N);    // Diagonal of S-matrix
    gsl_vector *work = gsl_vector_calloc(N); // temp ...

    gsl_matrix *Idata = gsl_matrix_calloc(K, M);  // Matrix of Scattering Intensity
    gsl_matrix *dIdata = gsl_matrix_calloc(K, M); // Matrix of errorbars of Scattering Intensity
    gsl_vector *b = gsl_vector_calloc(M);         // b = Idata(0..K, M)
    gsl_vector *x = gsl_vector_calloc(N);         //

    // Make Result Table
    Table *tableRho = app(this)->newTable(Name, NM, 6 + Nsvd + N + N + N + N + 1 + M + M + N + M + M);
    tableRho->setWindowLabel("SVD Table");

    // Col-Names
    tableRho->setColName(0, "Comment");
    tableRho->setColPlotDesignation(0, Table::None);
    tableRho->setTextFormat(0);
    tableRho->setColName(1, "hSLDcomponent");
    tableRho->setColPlotDesignation(1, Table::None);
    tableRho->setColNumericFormat(2, 5, 1, true);
    tableRho->setColName(2, "dSLDcomponent");
    tableRho->setColPlotDesignation(2, Table::None);
    tableRho->setColNumericFormat(2, 5, 2, true);
    tableRho->setColName(3, "TableName");
    tableRho->setColPlotDesignation(3, Table::None);
    tableRho->setTextFormat(3);
    tableRho->setColName(4, "HDcomposition");
    tableRho->setColPlotDesignation(4, Table::X);
    tableRho->setColNumericFormat(2, 5, 4, true);
    tableRho->setColName(5, "SLDsolvent");
    tableRho->setColPlotDesignation(5, Table::Y);
    tableRho->setColNumericFormat(2, 5, 5, true);

    // Solvent Parameters
    tableRho->setText(0, 0, "Solvent SLD");
    tableRho->setCell(0, 1, uiToDouble(lineEditRhoH->text()));
    tableRho->setCell(0, 2, uiToDouble(lineEditRhoD->text()));

    // Transfer of Data: from SLDcomponent-Matrix and TexT-QStringList to tableRho
    for (int nn = 0; nn < Nsvd; nn++)
    {
        tableRho->setText(nn + 1, 0, TexT[nn]);
        tableRho->setCell(nn + 1, 1, gsl_matrix_get(SLDcomponent, nn, 0));
        tableRho->setCell(nn + 1, 2, gsl_matrix_get(SLDcomponent, nn, 1));
    }

    // Transfer of Data: from Dcomposition-QStringList, SLDsolvent-Vector and FileNames-QStringList to tableRho
    for (int mm = 0; mm < Msvd; mm++)
    {
        tableRho->setText(mm, 3, FileNames[mm]);
        tableRho->setCell(mm, 4, Dcomposition[mm].toDouble()); // C
        tableRho->setCell(mm, 5, gsl_vector_get(SLDsolvent, mm));
    }

    // Number of active column
    coln = 6;

    // Calculation of Real SLD vs. DHcomposition
    for (int nn = 0; nn < Nsvd; nn++)
    {
        ss = "SLD";
        ss += TexT[nn];
        tableRho->setColName(coln, ss);
        tableRho->setColNumericFormat(2, 12, coln, true);
        for (int mm = 0; mm < Msvd; mm++)
        {
            ss = Dcomposition[mm];
            gslTemp = ss.toDouble();
            rhoHD = gsl_matrix_get(SLDcomponent, nn, 0) +
                    gslTemp * (gsl_matrix_get(SLDcomponent, nn, 1) - gsl_matrix_get(SLDcomponent, nn, 0));
            tableRho->setCell(mm, coln, rhoHD);
        }
        coln++;
    }
    KN = coln;

    // Making Rho-matrix
    for (int nn = 1; nn < Nsvd + 1; nn++)
        for (int mm = nn; mm < Nsvd + 1; mm++)
        {
            ss = ss.setNum(10 * nn + mm);                     // Indexing. For-example 11,12,13,22,23,33.
            tableRho->setColName(coln, ss.prepend('a'));      // Set col-names like a11,a12,a13...
            tableRho->setColNumericFormat(2, 12, coln, true); // Format
            for (int ll = 0; ll < Msvd; ll++)
            {
                rho1 = tableRho->cell(ll, 6 + nn - 1);
                rho2 = tableRho->cell(ll, 6 + mm - 1);
                rhoHD = tableRho->cell(ll, 5);

                gslTemp = (rho1 - rhoHD) * (rho2 - rhoHD);
                if (nn != mm)
                    gslTemp *= 2;
                tableRho->setCell(ll, coln, gslTemp);
                gsl_matrix_set(A, ll, coln - KN, gslTemp);
            }
            coln++;
        }

    // A initial matrix and U result of SVD
    gsl_matrix_memcpy(U, A);

    // GSL- error handler is of !!!!
    gsl_set_error_handler_off();

    // SVD of A to U V and S
    int status = gsl_linalg_SV_decomp(U, V, S, work);

    // Handling of errors in GSL-functions by hand :)
    if (status)
        if (status != 0)
        {
            ss = ss.setNum(status);
            QMessageBox::critical(this, tr("QtiSAS - Error"), tr("<b> %1 </b>: GSL error - SVD").arg(ss));
            goto NoTableLabel;
        }

    // current column number
    KN = coln;

    // Print of U-matrix to tableRho (u11,u12....)
    for (int nn = 1; nn < Nsvd + 1; nn++)
        for (int mm = nn; mm < Nsvd + 1; mm++)
        {
            ss = ss.setNum(10 * nn + mm);
            tableRho->setColName(coln, ss.prepend('u'));
            tableRho->setColNumericFormat(2, 12, coln, true);
            for (int ll = 0; ll < Msvd; ll++)
            {
                tableRho->setCell(ll, coln, gsl_matrix_get(U, ll, coln - KN));
            }
            coln++;
        }

    // current column number
    KN = coln;

    // Print of V-matrix to tableRho (v11,v12....)
    for (int nn = 1; nn < Nsvd + 1; nn++)
        for (int mm = nn; mm < Nsvd + 1; mm++)
        {
            ss = ss.setNum(10 * nn + mm);
            tableRho->setColName(coln, ss.prepend('V'));
            tableRho->setColNumericFormat(2, 12, coln, true);
            for (int ll = 0; ll < N; ll++)
            {
                tableRho->setCell(ll, coln, gsl_matrix_get(V, ll, coln - KN));
            }
            coln++;
        }

    //
    tableRho->setColName(coln, "S");
    tableRho->setColNumericFormat(2, 12, coln, true);

    // Print of S-vector to tableRho (s1,s2...sN)
    for (int ll = 0; ll < N; ll++)
    {
        tableRho->setCell(ll, coln, gsl_vector_get(S, ll));
    }
    coln++;

    // Current column number
    KN = coln;

    // Transfet data from individual tables to tableRho (I and Q); making MxN Idata matrix
    for (int mm = 0; mm < M; mm++)
    {
        ss = ss.setNum(mm + 1);
        tableRho->setColName(coln + mm * 3, 'Q' + ss);
        tableRho->setColName(coln + mm * 3 + 1, 'I' + ss);
        tableRho->setColName(coln + mm * 3 + 2, "dI" + ss);
        tableRho->setColPlotDesignation(coln + mm * 3, Table::X);        // Set first Q-column as X
        tableRho->setColPlotDesignation(coln + mm * 3 + 2, Table::yErr); // Set first Q-column as dY
        tableRho->setColNumericFormat(2, 12, coln + mm * 3, true);
        tableRho->setColNumericFormat(2, 12, coln + mm * 3 + 1, true);
        tableRho->setColNumericFormat(2, 12, coln + mm * 3 + 2, true);
        ss = tableRho->text(mm, 3);

        // Checking of existance of individual tables
        if (!app(this)->checkTableExistence(ss, t))
        {
            QMessageBox::critical(this, tr("QtiSAS - Error"), tr("<b> %1 </b>: Table Does Not Exist").arg(ss));
            goto NoTableLabel;
        }

        //
        for (int kk = 0; kk < K; kk++)
        {
            for (int col = 0; col < 3; col++)
                tableRho->setCellOrText(kk, coln + mm * 3 + col, t->text(kk, col), t->cellLocale(col));

            gsl_matrix_set(Idata, kk, mm, t->cell(kk, 1));
            gsl_matrix_set(dIdata, kk, mm, t->cell(kk, 2));
        }
    }

    coln += 3 * M;

    //
    for (int nn = 1; nn < Nsvd + 1; nn++)
        for (int mm = nn; mm < Nsvd + 1; mm++)
        {
            ss = ss.setNum(10 * nn + mm);
            tableRho->setColName(coln, ss.prepend('S'));
            tableRho->setColNumericFormat(2, 12, coln, true);
            coln++;
            tableRho->setColName(coln, ss.prepend('d'));
            tableRho->setColPlotDesignation(coln, Table::yErr);
            tableRho->setColNumericFormat(2, 12, coln, true);
            coln++;
        }

    //
    if (Nsvd == 2)
    {
        tableRho->addColumns(6);
        tableRho->setColName(coln, "S13");
        tableRho->setColNumericFormat(2, 12, coln, true);
        tableRho->setColName(coln + 1, "dS13");
        tableRho->setColPlotDesignation(coln + 1, Table::yErr);
        tableRho->setColNumericFormat(2, 12, coln + 1, true);

        tableRho->setColName(coln + 2, "S23");
        tableRho->setColNumericFormat(2, 12, coln + 2, true);
        tableRho->setColName(coln + 3, "dS23");
        tableRho->setColPlotDesignation(coln + 3, Table::yErr);
        tableRho->setColNumericFormat(2, 12, coln + 3, true);

        tableRho->setColName(coln + 4, "S33");
        tableRho->setColNumericFormat(2, 12, coln + 4, true);
        tableRho->setColName(coln + 5, "dS33");
        tableRho->setColPlotDesignation(coln + 5, Table::yErr);
        tableRho->setColNumericFormat(2, 12, coln + 5, true);
    }

    //+++
    coln = coln - 2 * N;

    //+++
    KN = coln;

    //+++
    double S11, S12, S22, S13, S23, S33;

    //+++
    for (int kk = 0; kk < K; kk++)
    {
        error = 0;
        for (int mm = 0; mm < M; mm++)
        {
            gsl_vector_set(b, mm, gsl_matrix_get(Idata, kk, mm));
            error += gsl_matrix_get(dIdata, kk, mm) / gsl_matrix_get(Idata, kk, mm) * gsl_matrix_get(dIdata, kk, mm) /
                     gsl_matrix_get(Idata, kk, mm);
        }
        error = sqrt(error);
        //+++
        status = gsl_linalg_SV_solve(U, V, S, b, x);

        //+++
        if (status)
            if (status != 0)
            {
                ss = ss.setNum(status);
                QMessageBox::critical(this, tr("QtiSAS - Error"), tr("<b> %1 </b>: GSL error - Solve").arg(ss));
                goto NoTableLabel;
            }

        //+++
        for (int nn = 0; nn < N; nn++)
        {
            gslTemp = gsl_vector_get(x, nn);

            tableRho->setCell(kk, coln + 2 * nn, gslTemp);
            tableRho->setCell(kk, coln + 2 * nn + 1, fabs(gslTemp * error));
            if (Nsvd == 2)
            {
                if (nn == 0)
                    S11 = gslTemp;
                if (nn == 1)
                    S12 = gslTemp;
                if (nn == 2)
                    S22 = gslTemp;
            }
        }
        if (Nsvd == 2)
        {
            S33 = 2 * S12 + S11 + S22;
            S23 = 0.5 * (S11 - S22 - S33);
            S13 = 0.5 * (-S11 + S22 - S33);
            tableRho->setCell(kk, coln + 6, S13);
            tableRho->setCell(kk, coln + 7, fabs(S13 * error));
            tableRho->setCell(kk, coln + 8, S23);
            tableRho->setCell(kk, coln + 9, fabs(S23 * error));
            tableRho->setCell(kk, coln + 10, S33);
            tableRho->setCell(kk, coln + 11, fabs(S33 * error));
        }
    }

    coln += 2 * N;
    if (Nsvd == 2)
        coln += 6;

    //
    for (int mm = 0; mm < M; mm++)
    {
        ss = ss.setNum(mm + 1);
        tableRho->setColName(coln + mm, ss.prepend("Icon"));
        tableRho->setColNumericFormat(2, 12, coln + mm, true);
    }

    KN = coln;

    for (int mm = 0; mm < Msvd; mm++)
    {
        for (int kk = 0; kk < Ksvd; kk++)
        {
            Bk = 0;
            for (int nn = 0; nn < N; nn++)
            {
                gslTemp = tableRho->cell(kk, KN - N + nn);
                Bk += gsl_matrix_get(A, mm, nn) * gslTemp;
                gslTemp = gsl_matrix_get(A, mm, nn);
            }
            tableRho->setCell(kk, coln, Bk);
        }
        coln++;
    }

    tableRho->adjustColumnsWidth(false);
    tableRho->notifyChanges();
    app(this)->modifiedProject(tableRho);

NoTableLabel:;

    findSvdTables();
    comboBoxSVDtables->setCurrentIndex(comboBoxSVDtables->findText(Name));
    tableRho->setMaximized();
}
//+++ Chech rhoHD
void svd::slotCheckRhoHD()
{
    lineEditRhoH->setText(uiNumber(uiToDouble(lineEditRhoH->text())));
    lineEditRhoD->setText(uiNumber(uiToDouble(lineEditRhoD->text())));
}
//+++ SVD
void svd::slotTableMsvdChanged(int raw, int col)
{
    slotCheckRhoHD();

    QString text = "";
    if (tableMsvd->item(raw, 0))
        text = tableMsvd->item(raw, 0)->text();
    double textD = uiToDouble(text);
    if ((textD < 0 || textD > 1))
        textD = 0.0;

    // no fraction yet when the table name is typed first
    if (tableMsvd->item(raw, 0))
        tableMsvd->item(raw, 0)->setText(uiNumber(textD));

    if (col == 0)
    {
        double RhoH = uiToDouble(lineEditRhoH->text());
        double RhoD = uiToDouble(lineEditRhoD->text());

        if (!tableMsvd->item(raw, 1))
            tableMsvd->setItem(raw, 1, new QTableWidgetItem());
        tableMsvd->item(raw, 1)->setText(uiNumber(RhoH + textD * (RhoD - RhoH)));
    }
    else if (col == 1)
    {
        auto *_item1 = tableMsvd->item(raw, 1);
        text = "";
        if (_item1)
            text = _item1->text();
        textD = uiToDouble(text);
        _item1->setText(uiNumber(textD));
    }
    else if (col == 2)
    {
        auto *_item1 = tableMsvd->item(raw, 2);
        text = "";
        if (_item1)
            text = _item1->text();
        if (!app(this)->checkTableExistence(text))
            _item1->setBackground(QBrush(Qt::red));
        else
            _item1->setBackground(QBrush(Qt::green));
    }
}
//+++ Check rhoHD
void svd::slotSVD1dataTransfer()
{
    QString tableName;
    bool ok;
    do
    {
        tableName = QInputDialog::getText(this, "Creation of SVD-Table",
                                          "Enter name of a new SVD-Table:", QLineEdit::Normal, "SVD", &ok);
        if (!ok)
            return;
    } while (tableName == "" || app(this)->checkTableExistence(tableName));

    QString text;
    QStringList Dcomposition, FileNames, TexT;

    int Nsvd = spinBoxNsvd->value();
    int Msvd = spinBoxMsvd->value();
    int Ksvd = spinBoxKsvd->value();
    gsl_vector *SLDsolvent = gsl_vector_calloc(Msvd);
    gsl_matrix *SLDcomponent = gsl_matrix_calloc(Nsvd, 2);

    for (int mm = 0; mm < Msvd; mm++)
    {
        text = "";
        if (tableMsvd->item(mm, 1))
            text = tableMsvd->item(mm, 1)->text();
        gsl_vector_set(SLDsolvent, mm, uiToDouble(text));
        text = "";
        if (tableMsvd->item(mm, 0))
            text = tableMsvd->item(mm, 0)->text();
        Dcomposition.append(uiToC(text)); // C: read with toDouble() by the calculation
        text = "";
        if (tableMsvd->item(mm, 2))
            text = tableMsvd->item(mm, 2)->text();
        FileNames.append(text);
    }
    for (int nn = 0; nn < Nsvd; nn++)
    {
        text = "";
        if (tableNsvd->item(nn, 1))
            text = tableNsvd->item(nn, 1)->text();
        gsl_matrix_set(SLDcomponent, nn, 0, uiToDouble(text));
        text = "";
        if (tableNsvd->item(nn, 2))
            text = tableNsvd->item(nn, 2)->text();
        gsl_matrix_set(SLDcomponent, nn, 1, uiToDouble(text));
        text = "";
        if (tableNsvd->item(nn, 0))
            text = tableNsvd->item(nn, 0)->text();
        TexT.append(text);
    }

    slotSVD1dataTransfer(tableName, Nsvd, TexT, SLDcomponent, Msvd, Dcomposition, FileNames, SLDsolvent, Ksvd);
}
//+++ Chech Nsvd
void svd::slotTableNsvdChanged(int row, int col)
{
    slotCheckRhoHD();

    if (col == 1 || col == 2)
    {
        QString text = "";

        auto *_item = tableNsvd->item(row, col);

        if (_item)
            text = _item->text();
        double textD = uiToDouble(text);
        _item->setText(uiNumber(textD));
    }
}
//+++ Read data from a table to the interface
void svd::slotReadDataFromTable()
{
    QString tableName;
    bool ok;
    do
    {
        tableName = QInputDialog::getText(this, "Creation of SVD-Table",
                                          "Enter name of a new SVD-Table:", QLineEdit::Normal, "SVD", &ok);
        if (!ok)
            return;
    } while (tableName != "" && !app(this)->checkTableExistence(tableName));

    slotReadSvdData(tableName);
}
void svd::setFont(QFont fonts)
{
    // toolBox20->setFont(fonts);
    textLabelInfo->setFont(fonts);
    textLabelInfoSAS->setFont(fonts);
    textLabelInfoAuthor->setFont(fonts);
    textLabelVersion->setFont(fonts);

    /*
    toolBoxSVD->setFont(fonts);
    textLabelInfo->setFont(fonts);
    textLabelInfoSAS->setFont(fonts);
    textLabelInfoVersion->setFont(fonts);
    textLabelInfoAuthor->setFont(fonts);
    tableMsvd->setFont(fonts);
    textBrowserHelp->setFont(fonts);
     */
}

//+++ Search of  SVD tables
void svd::findSvdTables()
{
    QString activeSVDTable = comboBoxSVDtables->currentText();
    QStringList list;
    QStringList listOpen;

    //+++ Find table
    QList<MdiSubWindow *> tableList = app(this)->tableList();
    foreach (MdiSubWindow *t, tableList)
    {
        auto *tt = (Table *)t;
        if (tt->windowLabel() == "SVD Table")
            list << tt->name();
    }

    //
    comboBoxSVDtables->clear();
    comboBoxSVDtables->addItems(list);
    if (list.contains(activeSVDTable))
        comboBoxSVDtables->setCurrentIndex(comboBoxSVDtables->findText(activeSVDTable));
}