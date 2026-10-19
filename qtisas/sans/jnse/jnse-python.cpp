/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Python interface for jnse18 (J-NSE): .dtr import, the header table, and the multi-fit of its
             datasets in FitTable
 ******************************************************************************/

#include <cmath>

#include <QDir>
#include <QRegularExpression>

#include "fittable18.h"
#include "jnse18.h"

namespace
{
QString expandHome(const QString &path)
{
    return path == "~" || path.startsWith("~/") ? QDir::homePath() + path.mid(1) : path;
}

// files: absolute, "~/...", or relative to the input folder; wildcards in the file part
QStringList expandFiles(const QStringList &files, const QString &inputFolder, QStringList &missing)
{
    QStringList out;
    for (const QString &entry : files)
    {
        QString path = expandHome(entry.trimmed());
        if (QFileInfo(path).isRelative())
            path = QDir(inputFolder).filePath(path);
        const QFileInfo info(path);
        if (info.fileName().contains('*') || info.fileName().contains('?'))
        {
            const QStringList found = QDir(info.path()).entryList({info.fileName()}, QDir::Files, QDir::Name);
            if (found.isEmpty())
                missing << entry + " (looked in " + QDir(info.path()).absolutePath() + ")";
            for (const QString &f : found)
                out << QDir(info.path()).filePath(f);
        }
        else if (info.isDir())
            missing << entry + " (a folder — give its files, e.g. \"" + entry + "/*.dtr\")";
        else if (info.exists())
            out << info.absoluteFilePath();
        else
            missing << entry + " (looked for " + info.absoluteFilePath() + ")";
    }
    return out;
}
} // namespace

void jnse18::reset()
{
    m_inputFolder = QDir::homePath();
    m_lastError.clear();
    m_messages.clear();
}

bool jnse18::setInputFolder(const QString &folder)
{
    m_lastError.clear();
    const QString path = expandHome(folder);
    if (!QDir(path).exists())
    {
        m_lastError = tr("setInputFolder: no folder %1").arg(QDir(path).absolutePath());
        return false;
    }
    m_inputFolder = QDir(path).absolutePath();
    return true;
}

QString jnse18::inputFolder() const
{
    return m_inputFolder.isEmpty() ? QDir::homePath() : m_inputFolder;
}

// the .dtr files into a header table (default: "<first file read>-info"; an existing header table is appended to):
// its name
QString jnse18::importFiles(const QStringList &files, const QString &headerTableName)
{
    m_lastError.clear();
    m_messages.clear();
    QStringList missing;
    const QStringList paths = expandFiles(files, inputFolder(), missing);
    for (const QString &m : missing)
        m_messages << tr("no such file: %1").arg(m);
    if (paths.isEmpty())
    {
        m_lastError = tr("importFiles: no file found: %1").arg(missing.join("; "));
        // the look-alikes: a header table of the project named as a file (it is read with header(), datasets(),
        // fit() — no import), and a name not in the folder (its .dtr files listed)
        for (const QString &entry : files)
        {
            QString base = QFileInfo(entry).fileName();
            base.remove(QRegularExpression("\\.dtr$", QRegularExpression::CaseInsensitiveOption));
            if (headerTable(base) || headerTable(base + "-info"))
            {
                m_lastError += tr(". %1 is a JNSE header table of the project, not a file: use it directly with "
                                  "header(), datasets() or fit() — no import needed")
                                   .arg(headerTable(base) ? base : base + "-info");
                return {};
            }
        }
        const QStringList dtr = QDir(inputFolder()).entryList({"*.dtr", "*.DTR"}, QDir::Files, QDir::Name);
        if (!dtr.isEmpty())
            m_lastError += tr(". The .dtr files in %1 are: %2").arg(inputFolder(), dtr.join(", "));
        return {};
    }
    m_scripted = true;
    const QString made = importDtr(paths, headerTableName);
    m_scripted = false;
    if (made.isEmpty())
        m_lastError = "importFiles: " + m_lastError;
    return made;
}

// the dataset tables of a header table, row by row (its "table-name" column)
QStringList jnse18::datasets(const QString &headerTableName)
{
    bool numeric = false;
    return headerColumn(headerTableName, "table-name", &numeric);
}

// a column of a header table, row by row: texts, numbers as their text (numeric: a number column)
QStringList jnse18::headerColumn(const QString &headerTableName, const QString &column, bool *numeric)
{
    m_lastError.clear();
    m_messages.clear();
    Table *t = headerTable(headerTableName);
    if (!t)
    {
        m_lastError = tr("%1 is not a JNSE header table (one made by importFiles())").arg(headerTableName);
        return {};
    }
    const int col = int(t->colNames().indexOf(column));
    if (col < 0)
    {
        m_lastError =
            tr("%1 has no column %2; its columns are %3").arg(headerTableName, column, t->colNames().join(" "));
        return {};
    }
    *numeric = t->columnType(col) == Table::Numeric;
    QStringList values;
    for (int row = 0; row < t->numRows(); row++)
        values << (*numeric ? QString::number(t->cell(row, col), 'g', 15) : t->text(row, col));
    return values;
}

// FitTable set up for a multi-fit of the rows fromRow..toRow (1-based, toRow 0: the last) of a header table: one
// dataset per row (its S(q,t)/S(q)), the function's q fixed at the row's q; the datasets, or empty with lastError()
QStringList jnse18::initFit(const QString &headerTableName, const QString &function, int fromRow, int toRow)
{
    m_lastError.clear();
    m_messages.clear();
    Table *t = headerTable(headerTableName);
    if (!t)
    {
        m_lastError = tr("initFit: %1 is not a JNSE header table (one made by importFiles())").arg(headerTableName);
        return {};
    }
    const int rows = t->numRows();
    if (toRow == 0)
        toRow = rows;
    if (fromRow < 1 || toRow > rows || fromRow > toRow)
    {
        m_lastError = tr("initFit: rows %1..%2 are not rows of %3 (1..%4)")
                          .arg(fromRow)
                          .arg(toRow)
                          .arg(headerTableName)
                          .arg(rows);
        return {};
    }
    fittable18 *f = app()->fittableWidget;
    if (!f->fitFunctions().contains(function))
    {
        QStringList similar = f->fitFunctions("*" + QString(function).section('/', -1) + "*");
        m_lastError = tr("initFit: no fit function %1 in the library%2; compile it first (Compiler)")
                          .arg(function, similar.isEmpty() ? QString() : "; similar: " + similar.join(", "));
        return {};
    }
    QStringList datasets;
    for (int row = fromRow - 1; row < toRow; row++)
        datasets << t->text(row, 19) + "_Sqt-To-Sq";
    if (!f->configure(function, datasets))
    {
        m_lastError =
            tr("initFit: FitTable could not set up %1 with the datasets %2").arg(function, datasets.join(", "));
        return {};
    }
    const QStringList names = f->parameterNames();
    const QString q = names.contains("q") ? "q" : (names.contains("Q") ? "Q" : QString());
    if (q.isEmpty())
    {
        m_lastError = tr("initFit: %1 has no parameter q (its parameters are %2); an NSE function is a function of t "
                         "with the parameter q, e.g. simpleDiffusion")
                          .arg(function, names.join(", "));
        return {};
    }
    for (int k = 0; k < datasets.size(); k++)
    {
        f->setParameterValue(q, t->cell(fromRow - 1 + k, 4), k + 1);
        f->setParameterVaries(q, false, k + 1);
    }
    return datasets;
}

// initFit(), the parameters in shared fitted in common, start values, fixed parameters, the fit, and its results in
// the results log: chi2/dof
double jnse18::fit(const QString &headerTableName, const QString &function, int fromRow, int toRow,
                   const QStringList &shared, const QMap<QString, double> &start, const QMap<QString, double> &fixed)
{
    const QStringList sets = initFit(headerTableName, function, fromRow, toRow);
    if (sets.isEmpty())
    {
        m_lastError.replace("initFit:", "fit:");
        return qQNaN();
    }
    fittable18 *f = app()->fittableWidget;
    const QStringList names = f->parameterNames();
    for (const QString &name : shared)
    {
        if (!names.contains(name))
        {
            m_lastError = tr("fit: %1 has no parameter %2 to share (its parameters are %3)")
                              .arg(function, name, names.join(", "));
            return qQNaN();
        }
        f->setParameterShared(name, true);
    }
    // start values (all datasets), and parameters fixed at a value
    for (const QMap<QString, double> &values : {start, fixed})
        for (auto it = values.begin(); it != values.end(); ++it)
            if (!names.contains(it.key()))
            {
                m_lastError =
                    tr("fit: %1 has no parameter %2 (its parameters are %3)").arg(function, it.key(), names.join(", "));
                return qQNaN();
            }
    for (auto it = start.begin(); it != start.end(); ++it)
        f->setParameterValue(it.key(), it.value());
    for (auto it = fixed.begin(); it != fixed.end(); ++it)
    {
        f->setParameterValue(it.key(), it.value());
        f->setParameterVaries(it.key(), false);
    }
    const double chi2 = f->fit();
    if (std::isnan(chi2))
    {
        m_lastError = tr("fit: the fit of %1 did not run (see the messages above)").arg(function);
        return qQNaN();
    }

    // the results log: one line per parameter, shared ones once
    if (toRow == 0)
        toRow = fromRow - 1 + int(sets.size());
    QString log = tr("JNSE fit: %1 of %2, rows %3..%4 (%5 datasets): chi^2/dof = %6\n")
                      .arg(function, headerTableName)
                      .arg(fromRow)
                      .arg(toRow)
                      .arg(sets.size())
                      .arg(chi2, 0, 'g', 6);
    for (const QString &name : names)
    {
        if (f->parameterShared(name))
        {
            log += QString("   %1 = %2 ± %3 (shared)\n")
                       .arg(name)
                       .arg(f->parameterValue(name), 0, 'g', 6)
                       .arg(f->parameterError(name), 0, 'g', 3);
            continue;
        }
        for (int k = 1; k <= sets.size(); k++)
            log +=
                f->parameterVaries(name, k)
                    ? QString("   %1[%2] = %3 ± %4\n")
                          .arg(name)
                          .arg(k)
                          .arg(f->parameterValue(name, k), 0, 'g', 6)
                          .arg(f->parameterError(name, k), 0, 'g', 3)
                    : QString("   %1[%2] = %3 (fixed)\n").arg(name).arg(k).arg(f->parameterValue(name, k), 0, 'g', 6);
    }
    f->toResLog(log);
    return chi2;
}

QString jnse18::lastError() const
{
    return m_lastError;
}

QStringList jnse18::lastMessages() const
{
    return m_messages;
}
