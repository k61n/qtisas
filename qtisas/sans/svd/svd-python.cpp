/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Python interface for svd (contrast-variation SVD): the solvent, the components and the contrasts set by
             calls; the decomposition, match points, the rank of a series and the errors a plan would give
 ******************************************************************************/

#include <cmath>
#include <limits>

#include <QRegularExpression>

#include "svd.h"

void svd::reset()
{
    m_sldH2O = m_sldD2O = qQNaN();
    m_componentNames.clear();
    m_componentH.clear();
    m_componentD.clear();
    m_contrastTables.clear();
    m_fractions.clear();
    m_solventSld.clear();
    m_lastError.clear();
    m_messages.clear();
}

// an SLD in cm^-2: a value of the size of 1e-6 A^-2 (or of its number 6.335) is refused, not taken 1e16 times too small
bool svd::checkSld(double sld, const QString &what)
{
    if (std::isfinite(sld) && (sld == 0 || (fabs(sld) >= 1e5 && fabs(sld) < 1e13)))
        return true;
    // a negative exponent for a positive one (-5.61e-9 for -5.61e9): the value it was meant to be
    const double exponent = sld != 0 && std::isfinite(sld) ? std::floor(std::log10(fabs(sld))) : 0;
    const double flipped = exponent < 0 ? sld * std::pow(10.0, -2 * exponent) : qQNaN();
    const QString meant = std::isfinite(sld) && fabs(flipped) >= 1e5 && fabs(flipped) < 1e13
                              ? tr(" — %1 has a negative exponent: in cm^-2 it is %2").arg(sld).arg(flipped, 0, 'g', 6)
                              : QString();
    m_lastError = tr("%1: %2 is not an SLD in cm^-2; SLDs are in cm^-2 (e.g. D2O ~6.4e10), 1e-6 A^-2 = 1e10 cm^-2%3")
                      .arg(what)
                      .arg(sld)
                      .arg(meant);
    return false;
}

bool svd::setSolvent(double sldH2Ocm2, double sldD2Ocm2)
{
    m_lastError.clear();
    if (!checkSld(sldH2Ocm2, "setSolvent: sldH2Ocm2") || !checkSld(sldD2Ocm2, "setSolvent: sldD2Ocm2"))
        return false;
    if (sldH2Ocm2 == sldD2Ocm2)
    {
        m_lastError = tr("setSolvent: the SLDs of H2O and D2O are equal: no contrast variation");
        return false;
    }
    m_sldH2O = sldH2Ocm2;
    m_sldD2O = sldD2Ocm2;
    return true;
}

bool svd::setComponents(const QStringList &names, const QVector<double> &sldH2Ocm2, const QVector<double> &sldD2Ocm2)
{
    m_lastError.clear();
    if (names.isEmpty() || names.size() != sldH2Ocm2.size() || names.size() != sldD2Ocm2.size())
    {
        m_lastError = tr("setComponents: give one (name, SLD in H2O, SLD in D2O) per component, in cm^-2");
        return false;
    }
    for (int nn = 0; nn < names.size(); nn++)
        if (!checkSld(sldH2Ocm2[nn], "setComponents: " + names[nn] + " in H2O") ||
            !checkSld(sldD2Ocm2[nn], "setComponents: " + names[nn] + " in D2O"))
            return false;
    m_componentNames = names;
    m_componentH = sldH2Ocm2;
    m_componentD = sldD2Ocm2;
    return true;
}

bool svd::setContrasts(const QStringList &tables, const QVector<double> &fractions,
                       const QVector<double> &solventSldCm2)
{
    m_lastError.clear();
    if (tables.isEmpty() || tables.size() != fractions.size() || tables.size() != solventSldCm2.size())
    {
        m_lastError = tr("setContrasts: give one (table, D2O fraction) or (table, D2O fraction, solvent SLD in "
                         "cm^-2) per measured solvent");
        return false;
    }
    for (int mm = 0; mm < tables.size(); mm++)
    {
        if (!(fractions[mm] >= 0 && fractions[mm] <= 1))
        {
            m_lastError = tr("setContrasts: the D2O fraction of %1 is %2; it is a fraction from 0 to 1 (34% is 0.34)")
                              .arg(tables[mm])
                              .arg(fractions[mm]);
            return false;
        }
        if (!std::isnan(solventSldCm2[mm]) && !checkSld(solventSldCm2[mm], "setContrasts: solvent of " + tables[mm]))
            return false;
        if (!appWindow()->checkTableExistence(tables[mm]))
        {
            m_lastError = tr("setContrasts: there is no table %1").arg(tables[mm]);
            return false;
        }
    }
    m_contrastTables = tables;
    m_fractions = fractions;
    m_solventSld = solventSldCm2;
    return true;
}

// what is not set yet, each with its call ("" when all is set); reset() clears all
QString svd::missingInput(bool contrasts) const
{
    QStringList missing;
    if (std::isnan(m_sldH2O))
        missing << "the solvent: setSolvent(sldH2Ocm2, sldD2Ocm2)";
    if (m_componentNames.isEmpty())
        missing << "the components: setComponents([(name, sldH2Ocm2, sldD2Ocm2), ...])";
    if (contrasts && m_contrastTables.isEmpty())
        missing << "the contrasts: setContrasts([(table, d2oFraction), ...])";
    if (missing.isEmpty())
        return {};
    return tr("not set: %1 (reset() clears them; readTable(svdTable) sets all three from an SVD table)")
        .arg(missing.join("; "));
}

// the input of an SVD table made before (by decompose() or the interface): solvent, components and contrasts
bool svd::readTable(const QString &name)
{
    m_lastError.clear();
    m_messages.clear();
    Table *t = nullptr;
    if (!appWindow()->checkTableExistence(name, t) || t->windowLabel() != "SVD Table")
    {
        m_lastError = tr("readTable: %1 is not an SVD table (one made by decompose())").arg(name);
        return false;
    }
    QStringList names, tables;
    QVector<double> h, d, fractions, solvent;
    for (int row = 1; !t->text(row, 1).isEmpty(); row++)
    {
        names << t->text(row, 0);
        h << t->cell(row, 1);
        d << t->cell(row, 2);
    }
    for (int row = 0; !t->text(row, 4).isEmpty(); row++)
    {
        tables << t->text(row, 3);
        fractions << t->cell(row, 4);
        solvent << t->cell(row, 5);
    }
    m_sldH2O = t->cell(0, 1);
    m_sldD2O = t->cell(0, 2);
    m_componentNames = names;
    m_componentH = h;
    m_componentD = d;
    m_contrastTables = tables;
    m_fractions = fractions;
    m_solventSld = solvent;
    return true;
}

// the Q points of an SVD table (its Q1 column): len() is the number of Q points
QVector<double> svd::qValues(const QString &svdTable)
{
    m_lastError.clear();
    m_messages.clear();
    Table *t = nullptr;
    const int q1 = appWindow()->checkTableExistence(svdTable, t) ? int(t->colNames().indexOf("Q1")) : -1;
    if (q1 < 0 || t->windowLabel() != "SVD Table")
    {
        m_lastError = tr("qValues: %1 is not an SVD table (one made by decompose())").arg(svdTable);
        return {};
    }
    QVector<double> q;
    for (int row = 0; row < t->numRows() && !t->text(row, q1).isEmpty(); row++)
        q << t->cell(row, q1);
    return q;
}

// the solvent SLD of each contrast: the measured one given, else H2O + phi (D2O - H2O)
QVector<double> svd::contrastSolventSlds() const
{
    QVector<double> sld;
    for (int mm = 0; mm < m_contrastTables.size(); mm++)
        sld << (std::isnan(m_solventSld[mm]) ? m_sldH2O + m_fractions[mm] * (m_sldD2O - m_sldH2O) : m_solventSld[mm]);
    return sld;
}

QStringList svd::componentNames() const
{
    return m_componentNames;
}

QStringList svd::contrastTables() const
{
    return m_contrastTables;
}

QVector<double> svd::contrastFractions() const
{
    return m_fractions;
}

// the SVD table of the contrasts set: its name; qPoints 0 = all Q points the tables share
QString svd::decompose(const QString &name, int qPoints)
{
    m_lastError.clear();
    m_messages.clear();
    if (const QString missing = missingInput(true); !missing.isEmpty())
    {
        m_lastError = "decompose: " + missing;
        return {};
    }
    const int N = int(m_componentNames.size()), M = int(m_contrastTables.size());
    int K = qPoints;
    if (K <= 0)
    {
        K = std::numeric_limits<int>::max();
        for (const QString &tableName : m_contrastTables)
        {
            Table *t = nullptr;
            int rows = 0;
            if (appWindow()->checkTableExistence(tableName, t))
                while (rows < t->numRows() && !t->text(rows, 0).isEmpty())
                    rows++;
            K = std::min(K, rows);
        }
    }

    gsl_matrix *components = gsl_matrix_alloc(N, 2);
    gsl_vector *solvent = gsl_vector_alloc(M);
    for (int nn = 0; nn < N; nn++)
    {
        gsl_matrix_set(components, nn, 0, m_componentH[nn]);
        gsl_matrix_set(components, nn, 1, m_componentD[nn]);
    }
    QStringList fractions;
    for (int mm = 0; mm < M; mm++)
    {
        const double sld = m_sldH2O + m_fractions[mm] * (m_sldD2O - m_sldH2O);
        gsl_vector_set(solvent, mm, std::isnan(m_solventSld[mm]) ? sld : m_solventSld[mm]);
        fractions << QString::number(m_fractions[mm], 'g', 17);
    }
    // a taken name gets a number of its own (svdAB -> svdAB1), not the generic "Table2" of newTable()
    QString tableName = name.isEmpty() ? QString("svd") : name;
    if (appWindow()->checkTableExistence(tableName))
        tableName = appWindow()->generateUniqueName(tableName);
    const QString made = decompose(tableName, N, m_componentNames, components, M, fractions, m_contrastTables, solvent,
                                   K, m_sldH2O, m_sldD2O);
    gsl_matrix_free(components);
    gsl_vector_free(solvent);
    if (made.isEmpty())
        m_lastError = "decompose: " + m_lastError;
    return made;
}

// a value of an SVD table by column label and Q: the row whose Q1 is q (relative 1e-6)
double svd::value(const QString &svdTable, const QString &column, double q)
{
    m_lastError.clear();
    m_messages.clear();
    Table *t = nullptr;
    if (!appWindow()->checkTableExistence(svdTable, t) || t->windowLabel() != "SVD Table")
    {
        m_lastError = tr("value: %1 is not an SVD table (one made by decompose())").arg(svdTable);
        return qQNaN();
    }
    const QStringList labels = t->colNames();
    const int col = int(labels.indexOf(column));
    const int q1 = int(labels.indexOf("Q1"));
    if (col < 0 || q1 < 0)
    {
        m_lastError = tr("value: %1 has no column %2; its columns are %3").arg(svdTable, column, labels.join(" "));
        return qQNaN();
    }
    // only the columns whose rows are Q points: the others' rows are contrasts or combinations, and a value "at Q"
    // would be the one of the contrast in that row
    static const QRegularExpression perQRe(R"(^(?:Q|I|dI|Icon)\d+$|^d?S\d{2,}$)");
    if (!perQRe.match(column).hasMatch())
    {
        m_lastError = tr("value: %1 is not a column of Q points (its rows are contrasts or combinations); read the "
                         "input with readTable() + contrasts() / components(), or by its row")
                          .arg(column);
        return qQNaN();
    }
    int nearest = -1;
    for (int row = 0; row < t->numRows() && !t->text(row, q1).isEmpty(); row++)
    {
        if (nearest < 0 || fabs(t->cell(row, q1) - q) < fabs(t->cell(nearest, q1) - q))
            nearest = row;
    }
    if (nearest < 0 || fabs(t->cell(nearest, q1) - q) > 1e-6 * fabs(q))
    {
        m_lastError = nearest < 0 ? tr("value: %1 has no Q points").arg(svdTable)
                                  : tr("value: %1 has no Q point %2; the nearest is %3 (row %4)")
                                        .arg(svdTable)
                                        .arg(q)
                                        .arg(t->cell(nearest, q1))
                                        .arg(nearest + 1);
        return qQNaN();
    }
    if (t->text(nearest, col).isEmpty())
    {
        m_lastError = tr("value: %1 has no %2 at Q = %3").arg(svdTable, column).arg(q);
        return qQNaN();
    }
    return t->cell(nearest, col);
}

// the D2O fraction where a component's SLD (exchanging linearly from H2O to D2O) equals the solvent's
double svd::matchPoint(double sldH2Ocm2, double sldD2Ocm2)
{
    m_lastError.clear();
    m_messages.clear();
    if (std::isnan(m_sldH2O))
    {
        m_lastError = tr("matchPoint: set setSolvent(sldH2Ocm2, sldD2Ocm2) first");
        return qQNaN();
    }
    if (!checkSld(sldH2Ocm2, "matchPoint: sldH2Ocm2") || !checkSld(sldD2Ocm2, "matchPoint: sldD2Ocm2"))
        return qQNaN();
    const double slope = (m_sldD2O - m_sldH2O) - (sldD2Ocm2 - sldH2Ocm2);
    if (slope == 0)
    {
        m_lastError = tr("matchPoint: the component's SLD follows the solvent's at every D2O fraction: no match point");
        return qQNaN();
    }
    const double fraction = (sldH2Ocm2 - m_sldH2O) / slope;
    if (fraction < 0 || fraction > 1)
        m_messages
            << tr("matchPoint: %1 is outside 0..1: the component is matched by no H2O/D2O mixture").arg(fraction);
    return fraction;
}

// the contrast matrix of the components set for D2O fractions: rows of (rho_i - rho_s)(rho_j - rho_s), x2 for i != j
bool svd::contrastMatrix(const QVector<double> &fractions, QVector<QVector<double>> &rows)
{
    if (const QString missing = missingInput(false); !missing.isEmpty())
    {
        m_lastError = missing;
        return false;
    }
    rows.clear();
    for (double x : fractions)
    {
        if (!(x >= 0 && x <= 1))
        {
            m_lastError = tr("the D2O fraction %1 is not a fraction from 0 to 1 (34% is 0.34)").arg(x);
            return false;
        }
        const double solvent = m_sldH2O + x * (m_sldD2O - m_sldH2O);
        QVector<double> rho, row;
        for (int nn = 0; nn < m_componentNames.size(); nn++)
            rho << m_componentH[nn] + x * (m_componentD[nn] - m_componentH[nn]) - solvent;
        for (int nn = 0; nn < rho.size(); nn++)
            for (int ll = nn; ll < rho.size(); ll++)
                row << (nn == ll ? 1 : 2) * rho[nn] * rho[ll];
        rows << row;
    }
    return true;
}

namespace
{
// the SVD of rows (M x N, M >= N padded with zero rows): U, singular values s, V
struct Decomposition
{
    QVector<QVector<double>> U, V;
    QVector<double> s;
};

Decomposition decomposeRows(const QVector<QVector<double>> &rows, int N)
{
    const int M = std::max(int(rows.size()), N);
    gsl_matrix *U = gsl_matrix_calloc(M, N);
    gsl_matrix *V = gsl_matrix_alloc(N, N);
    gsl_vector *S = gsl_vector_alloc(N);
    gsl_vector *work = gsl_vector_alloc(N);
    for (int mm = 0; mm < rows.size(); mm++)
        for (int nn = 0; nn < N; nn++)
            gsl_matrix_set(U, mm, nn, rows[mm][nn]);
    gsl_set_error_handler_off();
    gsl_linalg_SV_decomp(U, V, S, work);
    Decomposition d;
    d.U.resize(M);
    d.V.resize(N);
    for (int mm = 0; mm < M; mm++)
        for (int nn = 0; nn < N; nn++)
            d.U[mm] << gsl_matrix_get(U, mm, nn);
    for (int nn = 0; nn < N; nn++)
    {
        d.s << gsl_vector_get(S, nn);
        for (int ll = 0; ll < N; ll++)
            d.V[nn] << gsl_matrix_get(V, nn, ll);
    }
    gsl_matrix_free(U);
    gsl_matrix_free(V);
    gsl_vector_free(S);
    gsl_vector_free(work);
    return d;
}

int determined(const Decomposition &d)
{
    int rank = 0;
    for (double s : d.s)
        if (s > svd::singularThreshold * d.s[0])
            rank++;
    return rank;
}
} // namespace

// how many combinations of the N(N+1)/2 partial structure factors the D2O fractions determine
int svd::rank(const QVector<double> &fractions)
{
    m_lastError.clear();
    m_messages.clear();
    QVector<QVector<double>> rows;
    if (!contrastMatrix(fractions, rows))
    {
        m_lastError = "rank: " + m_lastError;
        return -1;
    }
    const int N = int(m_componentNames.size() * (m_componentNames.size() + 1) / 2);
    return rows.isEmpty() ? 0 : determined(decomposeRows(rows, N));
}

// the relative errors dS/S of S11, S12, ..., expected for the D2O fractions, the partial structure factors at a Q
// and intensity errors dI = relError I + background I(D2O); inf for an S_ij the fractions do not determine
QVector<double> svd::plan(const QVector<double> &fractions, const QVector<double> &partials, double relError,
                          double background)
{
    m_lastError.clear();
    m_messages.clear();
    QVector<QVector<double>> rows, d2o;
    if (!contrastMatrix(fractions, rows) || !contrastMatrix({1.0}, d2o))
    {
        m_lastError = "plan: " + m_lastError;
        return {};
    }
    const int N = int(m_componentNames.size() * (m_componentNames.size() + 1) / 2);
    if (partials.size() != N || rows.isEmpty() || relError < 0 || background < 0)
    {
        m_lastError = tr("plan: give the D2O fractions, the %1 partial structure factors S11, S12, ... at one Q (in "
                         "the order of the SVD table) and relError, background >= 0")
                          .arg(N);
        return {};
    }
    auto intensity = [&](const QVector<double> &row) {
        double I = 0;
        for (int nn = 0; nn < N; nn++)
            I += row[nn] * partials[nn];
        return I;
    };
    const double floor = background * fabs(intensity(d2o[0]));
    QVector<double> dI2;
    for (const QVector<double> &row : rows)
    {
        const double dI = relError * fabs(intensity(row)) + floor;
        dI2 << dI * dI;
    }

    const Decomposition d = decomposeRows(rows, N);
    const int rank = determined(d);
    if (rank < N)
        m_messages << tr("plan: the fractions determine %1 of the %2 combinations of the S_ij").arg(rank).arg(N);
    QVector<double> result;
    for (int nn = 0; nn < N; nn++)
    {
        // S_nn is determined when its unit vector lies in the span of the determined V columns
        double inSpan = 0;
        for (int kk = 0; kk < rank; kk++)
            inSpan += d.V[nn][kk] * d.V[nn][kk];
        if (inSpan < 1 - 1e-9)
        {
            result << std::numeric_limits<double>::infinity();
            continue;
        }
        // row nn of the pseudo-inverse: P[nn][mm] = sum_k V[nn][k] U[mm][k] / s_k
        double variance = 0;
        for (int mm = 0; mm < rows.size(); mm++)
        {
            double p = 0;
            for (int kk = 0; kk < rank; kk++)
                p += d.V[nn][kk] * d.U[mm][kk] / d.s[kk];
            variance += p * p * dI2[mm];
        }
        result << (partials[nn] != 0 ? sqrt(variance) / fabs(partials[nn]) : std::numeric_limits<double>::infinity());
    }
    return result;
}

// why the last decompose() made no table (empty when it made one)
QString svd::lastError() const
{
    return m_lastError;
}

// the warnings of the last decompose(), e.g. Q points that differ between the data tables
QStringList svd::lastMessages() const
{
    return m_messages;
}
