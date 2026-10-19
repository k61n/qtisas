/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Singular value decomposition interface (SANS)
 ******************************************************************************/

#include <algorithm>
#include <memory>

#include <QDockWidget>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QShowEvent>
#include <QString>

#include "number-validator.h"
#include "numericLocale.h"
#include "settings.h"
#include "svd.h"


svd::svd(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    // the solvent SLDs: numbers only, in the numerical format (empty: the defaults of readSettings())
    new NumberValidator(lineEditRhoH, -0.56e10);
    new NumberValidator(lineEditRhoD, 6.34e10);

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
    slotCheckRhoHD(); // the solvent SLDs, converted by their NumberValidator, back to 'e' with 5 digits
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
static ApplicationWindow *app(QWidget *widget)
{
    ApplicationWindow *appM;

    auto *docWin = (QDockWidget *)widget->parent();

    if (docWin->isFloating())
        appM = (ApplicationWindow *)docWin->parent()->parent();
    else
        appM = (ApplicationWindow *)docWin->parent();

    return appM;
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

    if (!app(this)->checkTableExistence(std::move(tableName), t))
        return;

    // the components: the rows of hSLDcomponent after the solvent's (their names may be empty)
    int nn = 0;
    while (!t->text(nn + 1, 1).isEmpty())
        nn++;

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

    // the Q points: the rows of Q1, after the SLD, a, u and V columns and S
    const int q1 = 6 + nn + 3 * (nn * (nn + 1) / 2) + 1;
    int kk = 0;
    while (!t->text(kk, q1).isEmpty())
        kk++;
    if (kk > 0)
        spinBoxKsvd->setValue(kk);

    // the contrasts: the rows of HDcomposition
    int mm = 0;
    while (!t->text(mm, 4).isEmpty())
        mm++;

    spinBoxMsvd->setValue(mm);
    for (int ii = 0; ii < mm; ii++)
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
//+++ SVD of a contrast series: the SVD table, its name returned. Nothing is created when the input cannot be
//+++ decomposed: then the name is empty and m_lastError tells why. Warnings of a made table are in m_messages.
QString svd::decompose(const QString &Name, int Nsvd, const QStringList &TexT, const gsl_matrix *SLDcomponent, int Msvd,
                       const QStringList &Dcomposition, const QStringList &FileNames, const gsl_vector *SLDsolvent,
                       int Ksvd)
{
    m_lastError.clear();
    m_messages.clear();

    // definition of variables...
    const int M = Msvd;                  // Number of H/D Concentrations
    const int N = Nsvd * (Nsvd + 1) / 2; // Number of independent Partial structure factors
    const int K = Ksvd;                  // Number of Q-points

    if (Nsvd < 1 || K < 1 || TexT.size() < Nsvd || Dcomposition.size() < M || FileNames.size() < M)
    {
        m_lastError = tr("SVD: %1 components, %2 contrasts and %3 Q points need a name per component and an H/D "
                         "fraction and a data table per contrast")
                          .arg(Nsvd)
                          .arg(M)
                          .arg(K);
        return {};
    }
    // gsl_linalg_SV_decomp needs at least as many rows (contrasts) as unknowns (partial structure factors)
    if (M < N)
    {
        m_lastError = tr("SVD: %1 components have %2 partial structure factors, so at least %2 H/D contrasts are "
                         "needed (%3 given); add contrasts or reduce the number of components")
                          .arg(Nsvd)
                          .arg(N)
                          .arg(M);
        return {};
    }

    // the data tables: all exist, with K points each
    QList<Table *> tables;
    for (int mm = 0; mm < M; mm++)
    {
        Table *t = nullptr;
        if (!app(this)->checkTableExistence(FileNames[mm], t))
        {
            m_lastError = tr("SVD: the data table \"%1\" of contrast %2 does not exist").arg(FileNames[mm]).arg(mm + 1);
            return {};
        }
        int rows = 0;
        while (rows < K && !t->text(rows, 0).isEmpty())
            rows++;
        if (rows < K)
        {
            m_lastError = tr("SVD: the data table \"%1\" has %2 Q points, fewer than the %3 used; set the number of "
                             "Q points to at most %2")
                              .arg(t->name())
                              .arg(rows)
                              .arg(K);
            return {};
        }
        tables << t;
    }

    // the same Q points in all tables: the SVD solves one Q point of all contrasts at once
    for (int mm = 1; mm < M; mm++)
        for (int kk = 0; kk < K; kk++)
        {
            const double q0 = tables[0]->cell(kk, 0);
            const double q = tables[mm]->cell(kk, 0);
            if (fabs(q - q0) > 1e-3 * fabs(q0))
            {
                m_messages << tr("SVD: the Q points of \"%1\" differ from \"%2\" (row %3: %4 vs %5); the SVD needs the "
                                 "same Q points in all data tables")
                                  .arg(tables[mm]->name(), tables[0]->name())
                                  .arg(kk + 1)
                                  .arg(q)
                                  .arg(q0);
                break;
            }
        }

    // SLD of the components vs. H/D composition
    QVector<double> fraction(M);
    for (int mm = 0; mm < M; mm++)
        fraction[mm] = Dcomposition[mm].toDouble(); // C
    auto componentSLD = [&](int nn, int mm) {
        const double h = gsl_matrix_get(SLDcomponent, nn, 0);
        return h + fraction[mm] * (gsl_matrix_get(SLDcomponent, nn, 1) - h);
    };

    // the gsl objects are freed on every return
    auto freeMatrix = [](gsl_matrix *m) { gsl_matrix_free(m); };
    auto freeVector = [](gsl_vector *v) { gsl_vector_free(v); };
    std::unique_ptr<gsl_matrix, decltype(freeMatrix)> A(gsl_matrix_calloc(M, N), freeMatrix); // Contrast-matrix A
    std::unique_ptr<gsl_matrix, decltype(freeMatrix)> U(gsl_matrix_calloc(M, N), freeMatrix); // A = USV ^ T
    std::unique_ptr<gsl_matrix, decltype(freeMatrix)> V(gsl_matrix_calloc(N, N), freeMatrix);
    std::unique_ptr<gsl_vector, decltype(freeVector)> S(gsl_vector_calloc(N), freeVector); // Diagonal of S-matrix
    std::unique_ptr<gsl_vector, decltype(freeVector)> work(gsl_vector_calloc(N), freeVector);
    std::unique_ptr<gsl_vector, decltype(freeVector)> b(gsl_vector_calloc(M), freeVector); // intensities of a Q
    std::unique_ptr<gsl_vector, decltype(freeVector)> x(gsl_vector_calloc(N), freeVector);
    std::unique_ptr<gsl_matrix, decltype(freeMatrix)> X(gsl_matrix_calloc(K, N), freeMatrix); // S_ij vs Q

    // Making Rho-matrix: a11, a12, ..., a22, ... (twice the cross terms)
    for (int mm = 0; mm < M; mm++)
    {
        const double rhoHD = gsl_vector_get(SLDsolvent, mm);
        int col = 0;
        for (int nn = 0; nn < Nsvd; nn++)
            for (int ll = nn; ll < Nsvd; ll++)
            {
                double a = (componentSLD(nn, mm) - rhoHD) * (componentSLD(ll, mm) - rhoHD);
                if (nn != ll)
                    a *= 2;
                gsl_matrix_set(A.get(), mm, col++, a);
            }
    }

    // GSL- error handler is of !!!!
    gsl_set_error_handler_off();

    // SVD of A to U V and S
    gsl_matrix_memcpy(U.get(), A.get());
    int status = gsl_linalg_SV_decomp(U.get(), V.get(), S.get(), work.get());
    if (status != 0)
    {
        m_lastError = tr("SVD: GSL error %1 in the decomposition of the contrast matrix").arg(status);
        return {};
    }

    // singular values at the rounding level of the largest are zero: their combinations of the S_ij are not
    // measured (e.g. for 3 or more components of one H/D series: the contrasts are quadratic in the D fraction,
    // so the contrast matrix has rank 3 at most). The solution is then the minimum-norm one, without the noise
    // a division by such a value would add. The table shows the values as decomposed.
    QVector<double> singular(N);
    int rank = 0;
    for (int ll = 0; ll < N; ll++)
    {
        singular[ll] = gsl_vector_get(S.get(), ll);
        if (singular[ll] > 1e-12 * singular[0])
            rank++;
        else
            gsl_vector_set(S.get(), ll, 0.0);
    }
    if (rank < N)
        m_messages << tr("SVD: the measured contrasts determine %1 of the %2 combinations of the S_ij (the V columns "
                         "of the nonzero singular values S); the S_ij shown are the minimum-norm solution")
                          .arg(rank)
                          .arg(N);

    // the pseudo-inverse A+ = V S^-1 U^T (S = A+ I), without the zero singular values as gsl_linalg_SV_solve
    std::unique_ptr<gsl_matrix, decltype(freeMatrix)> P(gsl_matrix_calloc(N, M), freeMatrix);
    for (int nn = 0; nn < N; nn++)
        for (int mm = 0; mm < M; mm++)
        {
            double p = 0;
            for (int ll = 0; ll < N; ll++)
                if (gsl_vector_get(S.get(), ll) != 0)
                    p +=
                        gsl_matrix_get(V.get(), nn, ll) / gsl_vector_get(S.get(), ll) * gsl_matrix_get(U.get(), mm, ll);
            gsl_matrix_set(P.get(), nn, mm, p);
        }

    // the output terms: the partial structure factors S11, S12, ..., S22, ... of the components and, with the
    // solvent as component N+1 (Babinet), S_i(N+1) = -sum_j S_ij and S_(N+1)(N+1) = sum_ij S_ij: each a linear
    // combination g of the solved S_ij
    QVector<QVector<int>> pair(Nsvd, QVector<int>(Nsvd)); // the solved S_ij of components i, j (from 0)
    QStringList names;
    for (int nn = 0, index = 0; nn < Nsvd; nn++)
        for (int ll = nn; ll < Nsvd; ll++, index++)
        {
            pair[nn][ll] = pair[ll][nn] = index;
            names << QString::number(nn + 1) + QString::number(ll + 1);
        }
    QVector<QVector<double>> terms;
    for (int nn = 0; nn < N; nn++)
    {
        terms << QVector<double>(N, 0.0);
        terms.last()[nn] = 1;
    }
    const QString solvent = QString::number(Nsvd + 1);
    for (int nn = 0; nn < Nsvd; nn++)
    {
        terms << QVector<double>(N, 0.0);
        for (int ll = 0; ll < Nsvd; ll++)
            terms.last()[pair[nn][ll]] = -1;
        names << QString::number(nn + 1) + solvent;
    }
    terms << QVector<double>(N, 0.0);
    for (int nn = 0; nn < Nsvd; nn++)
        for (int ll = 0; ll < Nsvd; ll++)
            terms.last()[pair[nn][ll]] += 1;
    names << solvent + solvent;

    // the terms of every Q point and their errors: Cov = A+ diag(dI^2) A+^T (the intensities of the contrasts
    // independent), var(g S) = g Cov g
    QVector<QVector<double>> values(K), errors(K);
    QVector<double> cov(N * N);
    for (int kk = 0; kk < K; kk++)
    {
        QVector<double> dI2(M);
        for (int mm = 0; mm < M; mm++)
        {
            gsl_vector_set(b.get(), mm, tables[mm]->cell(kk, 1));
            const double dI = tables[mm]->cell(kk, 2);
            dI2[mm] = dI * dI;
        }

        status = gsl_linalg_SV_solve(U.get(), V.get(), S.get(), b.get(), x.get());
        if (status != 0)
        {
            m_lastError = tr("SVD: GSL error %1 in the solution at Q point %2").arg(status).arg(kk + 1);
            return {};
        }
        gsl_matrix_set_row(X.get(), kk, x.get());

        for (int nn = 0; nn < N; nn++)
            for (int ll = 0; ll < N; ll++)
            {
                double c = 0;
                for (int mm = 0; mm < M; mm++)
                    c += gsl_matrix_get(P.get(), nn, mm) * gsl_matrix_get(P.get(), ll, mm) * dI2[mm];
                cov[nn * N + ll] = c;
            }
        for (const QVector<double> &g : terms)
        {
            double value = 0, variance = 0;
            for (int nn = 0; nn < N; nn++)
            {
                value += g[nn] * gsl_vector_get(x.get(), nn);
                for (int ll = 0; ll < N; ll++)
                    variance += g[nn] * g[ll] * cov[nn * N + ll];
            }
            values[kk] << value;
            errors[kk] << sqrt(fabs(variance));
        }
    }

    // Make Result Table: rows for the solvent and components, the contrasts, V and S, and the Q points
    const int NM = std::max({Nsvd + 1, M, N, K});
    Table *tableRho = app(this)->newTable(Name, NM, 6 + Nsvd + 3 * N + 1 + 3 * M + 2 * int(terms.size()) + M);
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

    // the components: names and SLD in H and D
    for (int nn = 0; nn < Nsvd; nn++)
    {
        tableRho->setText(nn + 1, 0, TexT[nn]);
        tableRho->setCell(nn + 1, 1, gsl_matrix_get(SLDcomponent, nn, 0));
        tableRho->setCell(nn + 1, 2, gsl_matrix_get(SLDcomponent, nn, 1));
    }

    // the contrasts: data table, H/D composition and solvent SLD
    for (int mm = 0; mm < M; mm++)
    {
        tableRho->setText(mm, 3, FileNames[mm]);
        tableRho->setCell(mm, 4, fraction[mm]);
        tableRho->setCell(mm, 5, gsl_vector_get(SLDsolvent, mm));
    }

    // Number of active column
    int coln = 6;

    // Real SLD vs. DHcomposition: SLD<name>, or SLD<number> for an empty or repeated name (a column name is unique)
    QStringList labels;
    for (int nn = 0; nn < Nsvd; nn++)
    {
        const QString label = TexT[nn].trimmed();
        labels << (label.isEmpty() || labels.contains(label) ? QString::number(nn + 1) : label);
        tableRho->setColName(coln, "SLD" + labels.last());
        tableRho->setColNumericFormat(2, 12, coln, true);
        for (int mm = 0; mm < M; mm++)
            tableRho->setCell(mm, coln, componentSLD(nn, mm));
        coln++;
    }

    // a matrix A (rows: contrasts), U (rows: contrasts), V (rows: partial structure factors)
    const QList<QPair<QString, const gsl_matrix *>> matrices = {{"a", A.get()}, {"u", U.get()}, {"V", V.get()}};
    for (const auto &matrix : matrices)
        for (int nn = 0; nn < N; nn++)
        {
            tableRho->setColName(coln, matrix.first + names[nn]);
            tableRho->setColNumericFormat(2, 12, coln, true);
            for (int ll = 0; ll < int(matrix.second->size1); ll++)
                tableRho->setCell(ll, coln, gsl_matrix_get(matrix.second, ll, nn));
            coln++;
        }

    // S-vector (s1,s2...sN)
    tableRho->setColName(coln, "S");
    tableRho->setColNumericFormat(2, 12, coln, true);
    for (int ll = 0; ll < N; ll++)
        tableRho->setCell(ll, coln, singular[ll]);
    coln++;

    // the data tables (Q, I, dI)
    for (int mm = 0; mm < M; mm++)
    {
        const QString index = QString::number(mm + 1);
        tableRho->setColName(coln, "Q" + index);
        tableRho->setColName(coln + 1, "I" + index);
        tableRho->setColName(coln + 2, "dI" + index);
        tableRho->setColPlotDesignation(coln, Table::X);
        tableRho->setColPlotDesignation(coln + 2, Table::yErr);
        for (int col = 0; col < 3; col++)
        {
            tableRho->setColNumericFormat(2, 12, coln + col, true);
            for (int kk = 0; kk < K; kk++)
                tableRho->setCellOrText(kk, coln + col, tables[mm]->text(kk, col), tables[mm]->cellLocale(col));
        }
        coln += 3;
    }

    // the terms S11, dS11, S12, dS12, ..., with the solvent's
    for (int nn = 0; nn < names.size(); nn++)
    {
        tableRho->setColName(coln + 2 * nn, "S" + names[nn]);
        tableRho->setColNumericFormat(2, 12, coln + 2 * nn, true);
        tableRho->setColName(coln + 2 * nn + 1, "dS" + names[nn]);
        tableRho->setColPlotDesignation(coln + 2 * nn + 1, Table::yErr);
        tableRho->setColNumericFormat(2, 12, coln + 2 * nn + 1, true);
        for (int kk = 0; kk < K; kk++)
        {
            tableRho->setCell(kk, coln + 2 * nn, values[kk][nn]);
            tableRho->setCell(kk, coln + 2 * nn + 1, errors[kk][nn]);
        }
    }
    coln += 2 * static_cast<int>(names.size());

    // the intensities of the contrasts reconstructed from the partial structure factors: Icon = A S
    for (int mm = 0; mm < M; mm++)
    {
        tableRho->setColName(coln, "Icon" + QString::number(mm + 1));
        tableRho->setColNumericFormat(2, 12, coln, true);
        for (int kk = 0; kk < K; kk++)
        {
            double Bk = 0;
            for (int nn = 0; nn < N; nn++)
                Bk += gsl_matrix_get(A.get(), mm, nn) * gsl_matrix_get(X.get(), kk, nn);
            tableRho->setCell(kk, coln, Bk);
        }
        coln++;
    }

    tableRho->adjustColumnsWidth(false);
    tableRho->notifyChanges();
    app(this)->modifiedProject(tableRho);

    findSvdTables();
    comboBoxSVDtables->setCurrentIndex(comboBoxSVDtables->findText(tableRho->name()));
    tableRho->setMaximized();
    return tableRho->name();
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
        updateTableNameColors();
}
//+++ the data table names of tableMsvd: green when the table exists, red when not
void svd::updateTableNameColors()
{
    const bool blocked = tableMsvd->blockSignals(true); // a new background is a change of the item
    for (int row = 0; row < tableMsvd->rowCount(); row++)
        if (QTableWidgetItem *item = tableMsvd->item(row, 2))
            item->setBackground(QBrush(app(this)->checkTableExistence(item->text()) ? Qt::green : Qt::red));
    tableMsvd->blockSignals(blocked);
}

// the colours follow the project: tables may have been created or removed while the interface was hidden
void svd::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updateTableNameColors();
}
//+++ SVD of the interface's input, to a table named in a dialog
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

    const QString made =
        decompose(tableName, Nsvd, TexT, SLDcomponent, Msvd, Dcomposition, FileNames, SLDsolvent, Ksvd);
    gsl_vector_free(SLDsolvent);
    gsl_matrix_free(SLDcomponent);

    if (made.isEmpty())
        QMessageBox::critical(this, tr("QtiSAS - Error"), m_lastError);
    else if (!m_messages.isEmpty())
        QMessageBox::warning(this, tr("QtiSAS - Warning"), m_messages.join("\n"));
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

    updateTableNameColors();
}