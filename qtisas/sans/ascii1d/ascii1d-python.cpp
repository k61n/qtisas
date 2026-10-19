/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Python interface for ascii1d18 (ASCII.SANS.1D) — the options of the
             interface as setters, and the four directions of "Process Data"
             (file -> table, file -> file, table -> file, table -> table)
 ******************************************************************************/

#include <cmath>

#include <QMessageBox>
#include <QRegularExpression>

#include "ascii1d18.h"
#include "globals.h"

namespace
{
QString expandHome(const QString &path)
{
    return path == "~" || path.startsWith("~/") ? QDir::homePath() + path.mid(1) : path;
}
} // namespace

void ascii1d18::warn(const QString &text)
{
    if (m_scripted)
        m_messages << text;
    else
        QMessageBox::warning(this, tr("QtiSAS::ASCII.SANS.1D"), text);
}

// "Modify [data]": on as soon as one of its options is
void ascii1d18::updateActions()
{
    checkBoxActions->setChecked(checkBoxRemoveRange->isChecked() || checkBoxRemoveFirst->isChecked() ||
                                checkBoxRemoveLast->isChecked() || checkBoxNoNegative->isChecked() ||
                                checkBoxConvert->isChecked() || checkBoxMath->isChecked() ||
                                checkBoxMath2->isChecked() || checkBox1DCalculator->isChecked() ||
                                checkBoxBinningLinear->isChecked() || checkBoxBinningProgressive->isChecked());
}

void ascii1d18::reset()
{
    defaultASCII1D();
    asciiFormatSelected();
    m_lastError.clear();
    m_messages.clear();
}

bool ascii1d18::setFormat(const QString &format)
{
    const QString f = format.trimmed().toLower();
    int index = -1;
    if (f == "ascii")
        index = 0;
    else if (f == "ascii+header" || f == "header")
        index = 1;
    else if (f == "cansas" || f == "cansas1d" || f == "cansas1d-xml" || f == "xml")
        index = 2;
    else if (f == "nist")
        index = 3;
    if (index < 0)
    {
        m_lastError = QString("setFormat: '%1' is not a format — use \"ascii\", \"ascii+header\", \"cansas\" or "
                              "\"nist\"")
                          .arg(format);
        return false;
    }
    // as when chosen: the header field of that format — only for another format, so that setHeader() followed by
    // setFormat() of the same format keeps the header (it was reset to 0)
    if (index != comboBoxFormat->currentIndex())
    {
        comboBoxFormat->setCurrentIndex(index);
        asciiFormatSelected();
    }
    return true;
}

QString ascii1d18::format() const
{
    static const QStringList names = {"ascii", "ascii+header", "cansas", "nist"};
    return names.value(comboBoxFormat->currentIndex());
}

// a number of lines, or the text of the last header line ("***** Data *****"): reading starts after it
bool ascii1d18::setHeader(const QString &linesOrMarker)
{
    const QString text = linesOrMarker.trimmed();
    bool isNumber = false;
    const int lines = text.toInt(&isNumber);
    if (text.isEmpty() || (isNumber && lines < 0))
    {
        m_lastError = "setHeader: give a number of header lines (0 or more) or the text of the header's last line";
        return false;
    }
    if (comboBoxFormat->currentIndex() == 0 || comboBoxFormat->currentIndex() == 2)
        setFormat("ascii+header"); // a plain ASCII or XML file has no header setting
    if (isNumber)
        lineEditSkipHeader->setText(QString::number(lines));
    else
        lineEditSkipHeader->setText(text.startsWith("{") && text.endsWith("}") ? text : "{" + text + "}");
    return true;
}

QString ascii1d18::header() const
{
    QString text = lineEditSkipHeader->text().trimmed();
    if (text.startsWith("{") && text.endsWith("}"))
        text = text.mid(1, text.length() - 2);
    return text;
}

bool ascii1d18::setColumns(int n)
{
    if (n < spinBoxForceNumCols->minimum() || n > spinBoxForceNumCols->maximum())
    {
        m_lastError = QString("setColumns: %1 columns — the module writes %2 to %3 (Q, I, dI, sigma)")
                          .arg(n)
                          .arg(spinBoxForceNumCols->minimum())
                          .arg(spinBoxForceNumCols->maximum());
        return false;
    }
    spinBoxForceNumCols->setValue(n);
    return true;
}

int ascii1d18::columns() const
{
    return spinBoxForceNumCols->value();
}

void ascii1d18::setInputFolder(const QString &folder)
{
    lineEditPath->setText(expandHome(folder.trimmed()));
}

QString ascii1d18::inputFolder() const
{
    return lineEditPath->text();
}

void ascii1d18::setOutputFolder(const QString &folder)
{
    lineEditPathOut->setText(expandHome(folder.trimmed()));
}

QString ascii1d18::outputFolder() const
{
    return lineEditPathOut->text();
}

void ascii1d18::setExtension(const QString &extension)
{
    QString ext = extension.trimmed();
    if (ext.startsWith("."))
        ext = ext.mid(1);
    lineEditExt->setText(ext);
}

QString ascii1d18::extension() const
{
    return lineEditExt->text();
}

void ascii1d18::setIndexing(bool on)
{
    checkBoxIndexing->setChecked(on);
}

void ascii1d18::setAutoPrefix(bool on)
{
    comboBoxPrefixASCII1D->setCurrentIndex(on ? 0 : 1);
}

// 0: the name comes from the file name; k = 1..9: from the k-th number in it
bool ascii1d18::setNameNumber(int index)
{
    if (index < 0 || index >= spinBoxFindIndex->count())
    {
        m_lastError = QString("setNameNumber: %1 — use 0 (the file name) or 1 to %2 (the n-th number in it)")
                          .arg(index)
                          .arg(spinBoxFindIndex->count() - 1);
        return false;
    }
    spinBoxFindIndex->setCurrentIndex(index);
    return true;
}

// first/last: points removed at each end; from..to: a range of points (1-based, 0 = no range); negative: I <= 0
bool ascii1d18::setRemovePoints(int first, int last, int from, int to, bool negative)
{
    if (first < 0 || last < 0 || from < 0 || to < 0)
    {
        m_lastError = "setRemovePoints: numbers of points must be 0 or more";
        return false;
    }
    checkBoxRemoveFirst->setChecked(first > 0);
    lineEditRemoveFirst->setValue(first);
    checkBoxRemoveLast->setChecked(last > 0);
    lineEditRemoveLast->setValue(last);
    checkBoxRemoveRange->setChecked(from > 0 && to > 0);
    lineEditRemoveFrom->setValue(from);
    lineEditRemoveTo->setValue(to);
    checkBoxNoNegative->setChecked(negative);
    removeFirstControl();
    removeLastControl();
    removeRangeControl();
    updateActions();
    return true;
}

QStringList ascii1d18::presentations() const
{
    QStringList names;
    for (int i = 0; i < comboBoxSelectPresentationTo->count(); i++)
        names << comboBoxSelectPresentationTo->itemText(i);
    return names;
}

// to: the presentation written ("Kratky", "Guinier", ...); from: the one read ("QI" for I vs Q); to == from = off
bool ascii1d18::setPresentation(const QString &to, const QString &from)
{
    const QStringList names = presentations();
    const auto find = [&names](const QString &name) {
        for (int i = 0; i < names.size(); i++)
            if (names.at(i).compare(name.trimmed(), Qt::CaseInsensitive) == 0)
                return i;
        return -1;
    };
    const int toIndex = find(to.isEmpty() ? QString("QI") : to);
    const int fromIndex = find(from.isEmpty() ? QString("QI") : from);
    if (toIndex < 0 || fromIndex < 0)
    {
        m_lastError = QString("setPresentation: '%1' is not a SAS presentation — one of %2")
                          .arg(toIndex < 0 ? to : from, names.join(", "));
        return false;
    }
    comboBoxSelectPresentationFrom->setCurrentIndex(fromIndex);
    comboBoxSelectPresentationTo->setCurrentIndex(toIndex);
    checkBoxConvert->setChecked(toIndex != fromIndex);
    presentationFromChanged();
    presentationToChanged();
    updateActions();
    return true;
}

// I op factor, then I op2 factor2; an empty op switches that step off
bool ascii1d18::setMath(const QString &op, double factor, const QString &op2, double factor2)
{
    static const QStringList ops = {"+", "-", "*", "/"};
    for (const QString &o : {op, op2})
        if (!o.isEmpty() && !ops.contains(o.trimmed()))
        {
            m_lastError = QString("setMath: '%1' is not an operation — use \"+\", \"-\", \"*\" or \"/\"").arg(o);
            return false;
        }
    checkBoxMath->setChecked(!op.isEmpty());
    if (!op.isEmpty())
    {
        comboBoxMath->setCurrentIndex(comboBoxMath->findText(op.trimmed()));
        lineEditMath->setText(cToUi(QString::number(factor, 'g', 15)));
    }
    checkBoxMath2->setChecked(!op2.isEmpty());
    if (!op2.isEmpty())
    {
        comboBoxMath2->setCurrentIndex(comboBoxMath2->findText(op2.trimmed()));
        lineEditMath2->setText(cToUi(QString::number(factor2, 'g', 15)));
    }
    mathControl();
    mathControl2();
    updateActions();
    return true;
}

// I op (a column of another table, point by point; same number of points); column "Table_col" or a table name
bool ascii1d18::setCalculator(const QString &op, const QString &column)
{
    if (op.isEmpty())
    {
        checkBox1DCalculator->setChecked(false);
        slot1DcalcYN();
        updateActions();
        return true;
    }
    static const QStringList ops = {"+", "-", "*", "/"};
    if (!ops.contains(op.trimmed()))
    {
        m_lastError = QString("setCalculator: '%1' is not an operation — use \"+\", \"-\", \"*\" or \"/\"").arg(op);
        return false;
    }
    checkBox1DCalculator->setChecked(true);
    slot1DcalcYN(); // fills the list of Y columns
    QString name = column.trimmed();
    if (comboBox1DclacTables->findText(name) < 0)
    {
        Table *t = nullptr;
        if (app()->checkTableExistence(name, t) && t->numCols() > 1)
            name = t->colName(1); // a table: its I column
    }
    const int index = comboBox1DclacTables->findText(name);
    if (index < 0)
    {
        checkBox1DCalculator->setChecked(false);
        slot1DcalcYN();
        m_lastError = QString("setCalculator: '%1' is neither a Y column (\"Table_col\") nor a table with a Y column")
                          .arg(column);
        return false;
    }
    comboBox1DclacTables->setCurrentIndex(index);
    comboBoxMath1D->setCurrentIndex(comboBoxMath1D->findText(op.trimmed()));
    updateActions();
    return true;
}

// every: average every n points (1 = off); ratio: progressive binning with this common ratio (1 = off)
bool ascii1d18::setBinning(int every, double ratio)
{
    if (every < 1 || every > spinBoxBinningLinear->maximum())
    {
        m_lastError =
            QString("setBinning: every %1 points — use 1 (off) to %2").arg(every).arg(spinBoxBinningLinear->maximum());
        return false;
    }
    if (ratio < 1.0 || ratio > lineEditBinningProgressive->maximum())
    {
        m_lastError =
            QString("setBinning: ratio %1 — use 1 (off) to %2").arg(ratio).arg(lineEditBinningProgressive->maximum());
        return false;
    }
    spinBoxBinningLinear->setValue(every);
    checkBoxBinningLinear->setChecked(every > 1);
    lineEditBinningProgressive->setValue(ratio);
    checkBoxBinningProgressive->setChecked(ratio > 1.0);
    updateActions();
    return true;
}

void ascii1d18::setResolution(bool on)
{
    checkBoxReso->setChecked(on);
    checkBoxResoSlot();
}

// SANS geometry, in the units of the Resolution tab: collimation, detector distance and pixel size in cm, apertures
// (width x height) in mm, the wavelength in Å, deltaLambda as Δλ/λ
bool ascii1d18::setResolutionGeometry(double collimation, double detector, double pixel, double sampleApertureW,
                                      double sampleApertureH, double collimationApertureW, double collimationApertureH,
                                      double lambda, double deltaLambda)
{
    for (double v : {collimation, detector, pixel, sampleApertureW, sampleApertureH, collimationApertureW,
                     collimationApertureH, lambda})
        if (!(v > 0.0))
        {
            m_lastError = "setResolutionGeometry: every length and the wavelength must be > 0 (cm, mm, Å)";
            return false;
        }
    if (deltaLambda < 0.0)
    {
        m_lastError = "setResolutionGeometry: deltaLambda (Δλ/λ) must be 0 or more";
        return false;
    }
    radioButtonSANS->setChecked(true);
    const auto number = [this](double v) { return cToUi(QString::number(v, 'g', 15)); };
    lineEditResoCol->setText(number(collimation));
    lineEditResoDet->setText(number(detector));
    lineEditResoPixelSize->setText(number(pixel));
    lineEditResoSamAper->setText(number(sampleApertureW));
    lineEditResoSamAper2->setText(number(sampleApertureH));
    lineEditResoColAper->setText(number(collimationApertureW));
    lineEditResoColAper2->setText(number(collimationApertureH));
    lineEditResoLambda->setText(number(lambda));
    lineEditResoDeltaLambda->setText(number(deltaLambda));
    return true;
}

void ascii1d18::setResolutionLenses(bool on, double focus)
{
    checkBoxLenses->setChecked(on);
    if (focus > 0.0)
        lineEditResoFocus->setText(cToUi(QString::number(focus, 'g', 15)));
    checkBoxLensesChanged();
}

double ascii1d18::resolutionSigma(double q)
{
    return sigma(fabs(q));
}

void ascii1d18::setPlot(bool on, bool xError, bool yError)
{
    checkBoxPlotInActive->setChecked(on);
    checkBoxXerror->setChecked(xError);
    checkBoxYerror->setChecked(yError);
}

// the processing of "Process Data" for given sources: what it made, with its warnings collected
QStringList ascii1d18::runScripted(int source, int destination, const QStringList &sources, const QStringList &names)
{
    m_lastError.clear();
    m_messages.clear();
    m_madeNames.clear();
    if (sources.isEmpty())
    {
        m_lastError = source == 0 ? "no file matches the given names or patterns"
                                  : "no table matches the given names or patterns";
        return {};
    }
    if (names.size() > sources.size())
    {
        m_lastError =
            QString("%1 names for %2 sources — give at most one name per source").arg(names.size()).arg(sources.size());
        return {};
    }
    const int sourceIndex = comboBoxSource->currentIndex();
    const int destinationIndex = comboBoxDestination->currentIndex();
    comboBoxSource->setCurrentIndex(source);
    comboBoxDestination->setCurrentIndex(destination);
    m_scripted = true;
    m_outputNames = names;
    if (source == 0)
        loadASCIIfromFiles(sources, false);
    else
        loadASCIIfromTables(sources, false);
    m_scripted = false;
    m_outputNames.clear();
    m_currentSource = -1;
    comboBoxSource->setCurrentIndex(sourceIndex);
    comboBoxDestination->setCurrentIndex(destinationIndex);
    if (m_madeNames.isEmpty())
        m_lastError = m_messages.isEmpty() ? QString("nothing was made") : m_messages.join("; ");
    return m_madeNames;
}

// file names, absolute or in the input folder, with ~ and wildcards ("*.dat", "~/data/run_*.dat")
static QStringList expandFiles(const QStringList &files, const QString &inputFolder, QStringList &missing)
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
        {
            missing << entry + " (a folder — give its files, e.g. \"" + entry + "/*.dat\")";
        }
        else if (info.exists())
            out << info.absoluteFilePath();
        else
            missing << entry + " (looked for " + info.absoluteFilePath() + ")";
    }
    return out;
}

static QStringList expandTables(ApplicationWindow *app, const QStringList &tables, QStringList &missing)
{
    QStringList out;
    for (const QString &entry : tables)
    {
        const QRegularExpression rx = REGEXPS::wildcardToRegex(entry.trimmed());
        bool any = false;
        for (MdiSubWindow *t : app->tableList())
            if (rx.match(t->name()).hasMatch() && !out.contains(t->name()))
            {
                out << t->name();
                any = true;
            }
        if (!any)
            missing << entry;
    }
    return out;
}

QStringList ascii1d18::importFiles(const QStringList &files, const QStringList &names)
{
    QStringList missing;
    const QStringList sources = expandFiles(files, lineEditPath->text(), missing);
    if (!missing.isEmpty())
    {
        m_lastError = "no such file: " + missing.join(", ");
        return {};
    }
    return runScripted(0, 0, sources, names);
}

QStringList ascii1d18::convertFiles(const QStringList &files, const QStringList &names)
{
    QStringList missing;
    const QStringList sources = expandFiles(files, lineEditPath->text(), missing);
    if (!missing.isEmpty())
    {
        m_lastError = "no such file: " + missing.join(", ");
        return {};
    }
    QDir().mkpath(lineEditPathOut->text());
    return runScripted(0, 1, sources, names);
}

QStringList ascii1d18::exportTables(const QStringList &tables, const QStringList &names)
{
    QStringList missing;
    const QStringList sources = expandTables(app(), tables, missing);
    if (!missing.isEmpty())
    {
        m_lastError = "no table matches " + missing.join(", ");
        return {};
    }
    QDir().mkpath(lineEditPathOut->text());
    return runScripted(1, 1, sources, names);
}

QStringList ascii1d18::convertTables(const QStringList &tables, const QStringList &names)
{
    QStringList missing;
    const QStringList sources = expandTables(app(), tables, missing);
    if (!missing.isEmpty())
    {
        m_lastError = "no table matches " + missing.join(", ");
        return {};
    }
    // a default name equal to the source would refill the source with its own processed data
    comboBoxDestination->setCurrentIndex(0);
    static const QRegularExpression re("[^a-zA-Z\\d\\s]");
    QStringList replaced;
    for (int i = 0; i < sources.size(); i++)
        if (names.value(i).trimmed().isEmpty() && !checkBoxIndexing->isChecked() &&
            generateTableName(QFileInfo(sources.at(i)).baseName().replace(re, "-").remove(" ")) == sources.at(i))
            replaced << sources.at(i);
    if (!replaced.isEmpty())
    {
        m_lastError = QString("the default name of the result is its source (%1): give names, setAutoPrefix(True) "
                              "with a presentation, or setIndexing(True)")
                          .arg(replaced.join(", "));
        return {};
    }
    return runScripted(1, 0, sources, names);
}

QStringList ascii1d18::formats() const
{
    QStringList names;
    for (int i = 1; i < comboBoxStructureASCII1D->count(); i++) // 0 is "default"
        names << comboBoxStructureASCII1D->itemText(i);
    return names;
}

bool ascii1d18::loadFormat(const QString &name)
{
    findASCII1DFormats();
    const int index = comboBoxStructureASCII1D->findText(name);
    if (index < 0)
    {
        m_lastError = QString("loadFormat: no format '%1' — saved formats: %2")
                          .arg(name, formats().isEmpty() ? QString("none") : formats().join(", "));
        return false;
    }
    comboBoxStructureASCII1D->setCurrentIndex(index);
    m_scripted = true;
    m_messages.clear();
    readCurrentASCII1D();
    m_scripted = false;
    if (!m_messages.isEmpty())
    {
        m_lastError = m_messages.join("; ");
        return false;
    }
    return true;
}

bool ascii1d18::saveFormat(const QString &name)
{
    if (name.trimmed().isEmpty() || name.trimmed() == "default")
    {
        m_lastError = "saveFormat: give a name other than \"default\"";
        return false;
    }
    m_scripted = true;
    m_messages.clear();
    saveCurrentASCII1D(name.trimmed());
    m_scripted = false;
    findASCII1DFormats();
    if (!m_messages.isEmpty())
    {
        m_lastError = m_messages.join("; ");
        return false;
    }
    comboBoxStructureASCII1D->setCurrentIndex(comboBoxStructureASCII1D->findText(name.trimmed()));
    return true;
}

QString ascii1d18::lastError() const
{
    return m_lastError;
}

QStringList ascii1d18::lastMessages() const
{
    return m_messages;
}
