/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: DAN-SANS Python interface: one object per tab (Dan.Options, Dan.Rawdata, Dan.Mask, Dan.Sensitivity,
             Dan.Processing, Dan.Merge)
 ******************************************************************************/

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>
#include <vector>

#include <QCheckBox>
#include <QComboBox>
#include <QDirIterator>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QLineEdit>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSet>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextStream>

#include "Table.h"
#include "dan-python.h"
#include "dan-sans-instruments.h"
#include "dan18.h"
#include "naturalsort.h"

// "~/..." as the user's home folder (as everywhere in QtiSAS's Python API)
static QString expandHome(const QString &path)
{
    if (path == "~" || path.startsWith("~/"))
        return QDir::homePath() + path.mid(1);
    return path;
}

// an empty list of runs: an error, saying the list was empty (a filter that matched none: nothing to do)
static const QString noRuns = "no runs: the list is empty, nothing to do (did a filter match none?)";

// a file as given ("~/..." expanded), or a relative name in DAN's input folder
static QString resolveFile(const QString &file, const QString &inputFolder)
{
    QString path = expandHome(file);
    if (QFileInfo(path).isRelative() && !QFileInfo::exists(path) && !inputFolder.isEmpty())
        return QDir(inputFolder).filePath(path);
    return path;
}

//+++ DanPart
void DanPart::begin()
{
    m_lastError.clear();
    m_lastMessages.clear();
    d->beginScriptRun();
    // Confirmed: the tab open in the GUI changed the result — on the Rawdata tab radial averaging names its tables
    // "raw-..." (dan18-process-dandan.cpp); and the tab's own list refresh ran only on a click
    // Leaving the Mask step (all its masks made) activates the full mask "mask", as after the Mask tab in the GUI: the
    // sensitivity is calculated with it, and the mask parameters shown are its own again
    if (d->sansTab->currentIndex() == 2 && tab() > 2 && isMask("mask"))
    {
        d->comboBoxMaskFor->setCurrentIndex(d->comboBoxMaskFor->findText("mask"));
        d->readMaskParaFromMaskMatrix("mask");
    }
    if (d->sansTab->currentIndex() != tab())
        d->sansTab->setCurrentIndex(tab());
}

bool DanPart::end(bool ok, const QString &reason)
{
    m_lastMessages = d->endScriptRun();
    if (!ok)
        m_lastError = !reason.isEmpty() ? reason : m_lastMessages.join("; ");
    if (!ok && m_lastError.isEmpty())
        m_lastError = "nothing was made";
    return ok;
}

QString DanPart::fail(const QString &reason)
{
    m_lastError = reason;
    return {};
}

bool DanPart::isMask(const QString &name) const
{
    return d->app()->existWindow(name) &&
           d->app()->findMatrixListByLabel("DAN::Mask::" + d->lineEditMD->text()).contains(name);
}

bool DanPart::isSens(const QString &name) const
{
    return d->app()->existWindow(name) &&
           d->app()->findMatrixListByLabel("DAN::Sensitivity::" + d->lineEditMD->text()).contains(name);
}

// "no file ..." with what the name is instead: a run (a mask or sensitivity is made from it, not loaded)
QString DanPart::noFile(const QString &file, const QString &given, bool mask) const
{
    QString reason = "no file " + file;
    // "00000001.sens": the run with an extension guessed
    QString run = QFileInfo(given).completeBaseName();
    if (d->filesManager->checkFileNumber(given))
        run = given;
    else if (run.isEmpty() || run == given || !d->filesManager->checkFileNumber(run))
        run.clear();
    if (!run.isEmpty())
        reason += QString(": \"%1\" is a run; %2")
                      .arg(run, mask ? "a mask is made by Dan.Mask.newMask()"
                                     : "Dan.Sensitivity.newSensitivity(run) makes it");
    return reason;
}

// to the terminal QtiSAS runs in (as qtikws's DAN-BlackBox printed its steps), not into QtiSAS's windows
void DanPart::report(const QString &line)
{
    std::cout << "DAN auto: " << line.toStdString() << std::endl;
}

//+++ DanOptions
void DanOptions::newSession()
{
    begin();
    d->newSession();
    end(true);
}

bool DanOptions::openSession(const QString &scriptTable)
{
    begin();
    d->updateScriptTables();
    if (d->comboBoxMakeScriptTable->findText(scriptTable) < 0)
        return end(false, "no saved session (script table) " + scriptTable);
    d->openSession(scriptTable);
    return end(d->comboBoxMakeScriptTable->currentText() == scriptTable);
}

QString DanOptions::state() const
{
    const QString mode = d->lineEditMD->text();
    const QStringList masks = d->app()->findMatrixListByLabel("DAN::Mask::" + mode);
    const QStringList sens = d->app()->findMatrixListByLabel("DAN::Sensitivity::" + mode);
    const QStringList scripts = d->app()->findTableListByLabel("DAN::Script::Table");
    if (masks.isEmpty() && sens.isEmpty() && scripts.isEmpty())
        return {};
    QStringList configured; // configurations with an empty beam set
    for (int col = 0; col < d->tableEC->columnCount(); col++)
        if (d->tableEC->item(dan18::dptEC, col) && !d->tableEC->item(dan18::dptEC, col)->text().trimmed().isEmpty())
            configured << QString::number(col + 1);
    auto list = [](const QStringList &names) { return names.isEmpty() ? QString("none") : names.join(", "); };
    return QString("instrument %1, input folder %2, output folder %3, %4 configuration(s) (empty beam set in: %5), "
                   "masks: %6, sensitivities: %7, script tables: %8")
        .arg(instrument(), inputPath(), outputPath().isEmpty() ? QString("none") : outputPath())
        .arg(d->tableEC->columnCount())
        .arg(list(configured), list(masks), list(sens), list(scripts));
}

QStringList DanOptions::instruments() const
{
    QStringList names;
    for (int i = 0; i < d->comboBoxInstrument->count(); i++)
        names << d->comboBoxInstrument->itemText(i);
    return names;
}

QString DanOptions::instrument() const
{
    return d->comboBoxInstrument->currentText();
}

bool DanOptions::selectInstrument(const QString &name)
{
    if (d->comboBoxInstrument->findText(name) < 0)
    {
        m_lastError = "no instrument " + name + "; instruments(): " + instruments().join(", ");
        return false;
    }
    begin();
    d->instrumentSelected(name);
    return end(d->comboBoxInstrument->currentText() == name);
}

QStringList DanOptions::modes() const
{
    QStringList names;
    for (int i = 0; i < d->comboBoxMode->count(); i++)
        names << d->comboBoxMode->itemText(i);
    return names;
}

bool DanOptions::selectMode(const QString &mode)
{
    if (d->comboBoxMode->findText(mode) < 0)
    {
        m_lastError = "no mode " + mode + "; modes(): " + modes().join(", ");
        return false;
    }
    begin();
    d->experimentalModeSelected(mode);
    return end(true);
}

QStringList DanOptions::polarizationReductionModes()
{
    return {"none", "x2", "x4", "x421", "x422", "x423", "x424"};
}

QString DanOptions::polarizationReductionMode() const
{
    return polarizationReductionModes().value(d->comboBoxPolReductionMode->currentIndex());
}

bool DanOptions::selectPolarizationReductionMode(const QString &mode)
{
    if (!d->comboBoxMode->currentText().contains("(PN)"))
    {
        m_lastError = "select polarized-neutron mode first with selectMode()";
        return false;
    }
    int index = static_cast<int>(polarizationReductionModes().indexOf(mode.trimmed().toLower()));
    if (index < 0)
        index = d->comboBoxPolReductionMode->findText(mode);
    if (index < 0)
    {
        m_lastError = "no polarization reduction mode " + mode +
                      "; polarizationReductionModes(): " + polarizationReductionModes().join(", ");
        return false;
    }
    begin();
    d->comboBoxPolReductionMode->setCurrentIndex(index);
    return end(d->comboBoxPolReductionMode->currentIndex() == index);
}

bool DanOptions::setInputPath(const QString &pathGiven)
{
    const QString path = expandHome(pathGiven);
    begin();
    return end(d->setPathIn(path), "no folder " + path);
}

QString DanOptions::inputPath() const
{
    return d->filesManager->pathInString();
}

bool DanOptions::setOutputPath(const QString &pathGiven)
{
    const QString path = expandHome(pathGiven);
    if (!path.isEmpty())
        QDir().mkpath(path); // asking for files in a new folder is normal
    begin();
    return end(d->setPathOut(path), "no folder " + path);
}

QString DanOptions::outputPath() const
{
    return d->filesManager->pathOutString();
}

QStringList DanOptionsConfig::centerMethods()
{
    return {"hf", "symmetry", "header"};
}

bool DanOptionsConfig::setCenterMethod(const QString &method)
{
    QRadioButton *buttons[] = {d->radioButtonCenterHF, d->radioButtonRadStdSymm, d->radioButtonCenterReadFromHeader};
    const auto index = centerMethods().indexOf(method);
    if (index < 0)
    {
        m_lastError = "no center method " + method + "; centerMethods(): " + centerMethods().join(", ");
        return false;
    }
    begin();
    buttons[index]->setChecked(true);
    return end(true);
}

QStringList DanOptionsConfig::transmissionMethods() const
{
    QStringList names;
    for (int i = 0; i < d->comboBoxTransmMethod->count(); i++)
        names << d->comboBoxTransmMethod->itemText(i);
    return names;
}

QString DanOptionsConfig::centerMethod() const
{
    if (d->radioButtonCenterHF->isChecked())
        return "hf";
    return d->radioButtonRadStdSymm->isChecked() ? "symmetry" : "header";
}

QString DanOptionsConfig::transmissionMethod() const
{
    return d->comboBoxTransmMethod->currentText();
}

bool DanOptionsConfig::setTransmissionMethod(const QString &method)
{
    const int index = d->comboBoxTransmMethod->findText(method);
    if (index < 0)
    {
        m_lastError =
            "no transmission method " + method + "; transmissionMethods(): " + transmissionMethods().join(" | ");
        return false;
    }
    begin();
    d->comboBoxTransmMethod->setCurrentIndex(index);
    return end(true);
}

//+++ DanRawdata
QStringList DanRawdata::runs() const
{
    // the runs auto mode and info() see: only those DAN opens
    QStringList numbers;
    for (const DanAuto::Run &r : DanAuto(d).runs(false))
        numbers << r.run;
    return numbers;
}

QStringList DanRawdata::headerItems() const
{
    QStringList names;
    for (int i = 0; i < d->comboBoxInfoExtractorCol->count(); i++)
        names << d->comboBoxInfoExtractorCol->itemText(i);
    return names;
}

QStringList DanRawdata::headerKeys() const
{
    QStringList keys;
    for (QString key : d->parserHeader->listOfHeaders)
        keys << key.remove('[').remove(']');
    return keys;
}

// a key as headerKeys() gives it, with or without [], or one of its alternatives ("Detector-X"); case and spaces
// ignored
static QString headerName(const QStringList &listOfHeaders, const QString &key)
{
    auto plain = [](QString s) { return s.remove('[').remove(']').remove(' ').toLower(); };
    const QString wanted = plain(key);
    for (const QString &name : listOfHeaders)
        if (plain(name) == wanted || plain(name).split('|', Qt::SkipEmptyParts).contains(wanted))
            return name;
    return {};
}

// " (did you mean <name>?)" for the nearest of the names (case, spaces and [] ignored), a few letters apart at most;
// empty without one: a misspelled key named with its correction, never taken for it
static QString didYouMean(const QStringList &names, const QString &wanted)
{
    auto plain = [](QString s) { return s.remove('[').remove(']').remove(' ').toLower(); };
    auto distance = [](const QString &a, const QString &b) {
        std::vector<int> row(static_cast<size_t>(b.size()) + 1); // QList has no size constructor in Qt5
        for (int j = 0; j <= b.size(); j++)
            row[j] = j;
        for (int i = 1; i <= a.size(); i++)
        {
            int diagonal = row[0];
            row[0] = i;
            for (int j = 1; j <= b.size(); j++)
            {
                const int above = row[j];
                row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + (a[i - 1] == b[j - 1] ? 0 : 1)});
                diagonal = above;
            }
        }
        return row[b.size()];
    };
    const QString key = plain(wanted);
    QString best;
    int bestDistance = std::max(2, static_cast<int>(key.size()) / 4) + 1;
    for (const QString &name : names)
        if (const int d = distance(key, plain(name)); d < bestDistance)
        {
            bestDistance = d;
            best = name;
        }
    return best.isEmpty() ? QString() : " (did you mean " + best + "?)";
}

QString DanRawdata::headerValue(const QString &run, const QString &key)
{
    m_lastError.clear();
    const QString name = headerName(d->parserHeader->listOfHeaders, key);
    if (name.isEmpty())
        return fail("no header key " + key + didYouMean(headerKeys(), key) +
                    "; headerKeys(): " + headerKeys().join(", "));
    if (!d->filesManager->checkFileNumber(run))
        return fail("no run " + run + " in " + d->filesManager->pathInString());
    return d->parserHeader->readNumberString(run, name);
}

QStringList DanRawdata::setHeaderValue(const QStringList &runs, const QString &key, const QString &value, bool backup)
{
    m_lastError.clear();
    const QString name = headerName(d->parserHeader->listOfHeaders, key);
    if (name.isEmpty())
    {
        fail("no header key " + key + didYouMean(headerKeys(), key) + "; headerKeys(): " + headerKeys().join(", "));
        return {};
    }
    if (runs.isEmpty())
    {
        fail(noRuns);
        return {};
    }
    for (const QString &run : runs)
        if (!d->filesManager->checkFileNumber(run))
        {
            fail("no run " + run + " in " + d->filesManager->pathInString());
            return {};
        }
    QStringList changed;
    for (const QString &run : runs)
    {
        const QString file = d->filesManager->fileNameFull(run, d->filesManager->wildCardHeader());
        if (backup && !QFile::exists(file + ".bak") && !QFile::copy(file, file + ".bak"))
        {
            fail("cannot write the backup " + file + ".bak");
            break;
        }
        QString error;
        if (!d->parserHeader->setNumberString(run, name, value, error))
        {
            fail(changed.isEmpty() ? error : error + "; written before: " + changed.join(", "));
            break;
        }
        changed << file;
    }
    DanAuto::forgetRuns();
    return m_lastError.isEmpty() ? changed : QStringList();
}

QString DanRawdata::newHeaderTable(const QString &name, const QStringList &items, const QString &runs)
{
    const QStringList all = headerItems();
    QStringList indices;
    for (const QString &item : items)
    {
        const auto index = all.indexOf(item);
        if (index < 0)
            return fail("no header item " + item + didYouMean(all, item) + "; headerItems(): " + all.join(", "));
        indices << QString::number(index + 1);
    }
    QString tableName = name;
    tableName.remove(QRegularExpression("[^A-Za-z0-9_-]"));
    if (tableName.isEmpty())
        return fail("no table name");
    begin();
    const bool ok = d->infoExtractorScript(tableName, indices.join(","), runs.isEmpty() ? "*" : runs);
    if (!end(ok && d->app()->checkTableExistence(tableName),
             "no runs " + runs + " in " + d->filesManager->pathInString()))
        return {};
    return tableName;
}

// the files of the input and output folders with their time and size: what an action wrote is what changed
static QMap<QString, QString> folderState(const QStringList &folders)
{
    QMap<QString, QString> state;
    for (const QString &folder : folders)
        for (const QFileInfo &f : QDir(folder).entryInfoList(QDir::Files))
            state[f.absoluteFilePath()] =
                QString::number(f.lastModified().toMSecsSinceEpoch()) + "/" + QString::number(f.size());
    return state;
}

static QStringList written(const QMap<QString, QString> &before, const QMap<QString, QString> &after)
{
    QStringList files;
    for (auto it = after.constBegin(); it != after.constEnd(); ++it)
        if (before.value(it.key()) != it.value())
            files << it.key();
    return files;
}

QStringList DanRawdata::filesOf(const QString &run) const
{
    QStringList files = {d->filesManager->fileNameFull(run, d->filesManager->wildCardDetector())};
    const QString header = d->filesManager->fileNameFull(run, d->filesManager->wildCardHeader());
    if (!header.isEmpty() && !files.contains(header))
        files << header;
    return files;
}

QStringList DanRawdata::files(const QStringList &runs)
{
    m_lastError.clear();
    if (runs.isEmpty())
    {
        fail(noRuns);
        return {};
    }
    QStringList list;
    for (const QString &run : runs)
    {
        if (!d->filesManager->checkFileNumber(run))
        {
            fail("no run " + run + " in " + d->filesManager->pathInString());
            return {};
        }
        list << filesOf(run);
    }
    return list;
}

QStringList DanRawdata::runFiles(const QStringList &runs)
{
    QStringList files;
    for (const QString &run : runs)
    {
        if (!d->filesManager->checkFileNumber(run))
        {
            fail("no run " + run + " in " + d->filesManager->pathInString());
            return {};
        }
        files << d->filesManager->fileNameFull(run, d->filesManager->wildCardDetector());
    }
    return files;
}

QStringList DanRawdata::addRuns(const QStringList &runs, const QString &runGiven)
{
    if (runs.count() < 2)
    {
        fail("at least two runs to add");
        return {};
    }
    if (runFiles(runs).isEmpty())
        return {};
    // the name the interface proposes: the first run's number + the count, 5 digits
    QString run = runGiven;
    if (run.isEmpty())
        run = runs.first() + QString("%1").arg(runs.count(), 5, 10, QChar('0'));
    if (run.contains(QRegularExpression(R"([/\\\s])")))
    {
        fail("run: a name without spaces and slashes");
        return {};
    }
    const QStringList folders = {d->filesManager->pathInString(), d->lineEditPathRAD->text()};
    begin();
    const auto before = folderState(folders);
    d->addSeveralFilesUniSingleFrame(runs, run);
    QStringList files = written(before, folderState(folders));
    if (!end(!files.isEmpty(), "nothing was written (the instrument's file format?)"))
        return {};
    return files;
}

QString DanRawdata::newAddingTemplate(const QStringList &runs, int minimum, int consecutive)
{
    // the "Generate Merge Table" layout, grouped as sampleGroups() (DAN's configuration rule, no table of
    // configurations needed), or as neighbours() with "Only consecutive runs"; the button shows single runs too
    // (minimum 1), Python only what can be added (2)
    if (consecutive >= 0)
        d->checkBoxAddConsecutive->setChecked(consecutive == 1);
    const QVariantList groups =
        d->checkBoxAddConsecutive->isChecked() ? neighbours(0, runs, minimum) : sampleGroups(runs, minimum);
    if (!m_lastError.isEmpty())
        return {};
    if (groups.isEmpty())
        return fail(minimum > 1 ? "no sample measured twice in one configuration"
                                : "no runs (runs made by adding are left out)");
    int rows = 0;
    for (const QVariant &g : groups)
        rows = std::max(rows, static_cast<int>(g.toMap()["runs"].toStringList().count()) + 1);
    begin();
    QString name = d->app()->generateUniqueName("adding-template-");
    Table *t = d->app()->newTable(name, rows, static_cast<int>(groups.count()));
    t->setWindowLabel("DAN::Adding::Template");
    d->app()->setListViewLabel(t->name(), "DAN::Adding::Template");
    d->app()->updateWindowLists(t);
    for (int col = 0; col < groups.count(); col++)
    {
        const QVariantMap g = groups[col].toMap();
        const QStringList group = g["runs"].toStringList();
        QString sample = g["sample"].toString().simplified();
        sample.replace(" ", "-").replace("/", "-").replace("_", "-").replace(",", "-").replace(".", "-");
        sample.remove("%").remove("(").remove(")");
        // unique: "S1_H2O" and "S1-H2O" of one configuration are two samples, one column name
        QString colName = "C" + QString::number(g["condition"].toInt()) + "-" + sample;
        for (int k = 2; t->colNames().contains(colName); k++)
            colName = "C" + QString::number(g["condition"].toInt()) + "-" + sample + "-" + QString::number(k);
        t->setColName(col, colName);
        t->setColPlotDesignation(col, Table::None);
        t->setColumnType(col, Table::Text);
        // first row: the new run, as the button names it (<first run> + the count in 5 digits)
        t->setText(0, col, group.first() + QString("%1").arg(group.count(), 5, 10, QChar('0')));
        for (int row = 0; row < group.count(); row++)
            t->setText(row + 1, col, group[row]);
    }
    t->adjustColumnsWidth(false);
    end(true);
    return name;
}

QStringList DanRawdata::addRunsFromTable(const QString &table, const QString &by, int skipSingle)
{
    if (by != "columns" && by != "rows")
    {
        fail(R"(by: "columns" or "rows")");
        return {};
    }
    Table *t = d->app()->table(table);
    if (!t || t->name() != table)
    {
        fail("no table " + table);
        return {};
    }
    const QStringList folders = {d->filesManager->pathInString(), d->lineEditPathRAD->text()};
    begin();
    d->toolBoxAdv->setCurrentIndex(0); // "Conventional": the columns' Add of the RT and TOF pages adds otherwise
    if (skipSingle >= 0)
        d->checkBoxAddSkipSingle->setChecked(skipSingle == 1);
    const auto before = folderState(folders);
    if (by == "columns")
        d->addFilesInTableCols(t);
    else
        d->addFilesInTableRows(t);
    QStringList files = written(before, folderState(folders));
    if (!end(!files.isEmpty(), "nothing was written: no run of " + table + " found" +
                                   (d->checkBoxAddSkipSingle->isChecked() ? ", or single runs only (skipSingle)" : "")))
        return {};
    return files;
}

QString DanRawdata::newInfoTable(const QString &name, const QStringList &runs, int compact, int matrices,
                                 int detectorSum)
{
    if (name.isEmpty())
        return fail("no table name");
    const bool exists = d->app()->checkTableExistence(name);
    if (exists && !d->app()->findTableListByLabel("Info::Table").contains(name))
        return fail(name + " is no info table");
    const QStringList files = runFiles(runs);
    if (files.isEmpty())
        return runs.isEmpty() ? fail(noRuns) : QString();
    begin();
    if (compact >= 0)
        d->checkBoxShortList->setChecked(compact == 1);
    if (matrices >= 0)
        d->checkBoxDatYesNo->setChecked(matrices == 1);
    if (detectorSum >= 0)
        d->checkBoxSumVsMask->setChecked(detectorSum == 1);
    d->addToInfoTable(name, files);
    // the list of info tables, this one active (as after the interface's dialog)
    QStringList tables = d->app()->findTableListByLabel("Info::Table");
    tables.sort();
    d->comboBoxInfoTable->clear();
    d->comboBoxInfoTable->addItems(tables);
    d->comboBoxInfoTable->setCurrentIndex(static_cast<int>(tables.indexOf(name)));
    if (!end(d->app()->checkTableExistence(name)))
        return {};
    return name;
}

QString DanRawdata::newInfoMatrix(const QString &nameGiven, const QStringList &runs, int columns, int mask, int sens,
                                  int norm, int roi)
{
    if (nameGiven.isEmpty())
        return fail("no matrix name");
    QString name = nameGiven;
    name.replace("_", "-").replace("/", "-").replace("\\", "-").replace("%", "-").replace(",", "-");
    Matrix *existing = d->app()->matrix(name);
    if (existing && !existing->windowLabel().startsWith("[1,1]"))
        return fail(name + " is no info matrix");
    if (mask == 1 && !isMask(d->comboBoxMaskFor->currentText()))
        return fail("mask: no active mask");
    if (sens == 1 && !isSens(d->comboBoxSensFor->currentText()))
        return fail("sens: no active sensitivity");
    const QStringList files = runFiles(runs);
    if (files.isEmpty())
        return runs.isEmpty() ? fail(noRuns) : QString();

    begin();
    d->checkBoxBigMatrixASCII->setChecked(false); // the runs' raw data, not the ASCII I(Q) files
    if (columns >= 1)
        d->spinBoxBigMatrixCols->setValue(columns);
    if (mask >= 0)
        d->checkBoxBigMatrixMask->setChecked(mask == 1);
    if (sens >= 0)
        d->checkBoxBigMatrixSens->setChecked(sens == 1);
    if (norm >= 0)
        d->checkBoxBigMatrixNorm->setChecked(norm == 1);
    if (roi >= 0)
        d->checkBoxBigMatrixROI->setChecked(roi == 1);
    if (existing)
    {
        // added to it: the same columns and image width (else DAN asks for a new matrix)
        const int md = d->lineEditMD->text().toInt();
        int width = md;
        if (d->checkBoxBigMatrixROI->isChecked() && d->groupBoxMask->isChecked())
            width = std::min(d->spinBoxRBx->value(), md) - std::max(d->spinBoxLTx->value(), 1) + 1;
        if (d->spinBoxBigMatrixCols->value() * width != existing->numCols())
        {
            end(false,
                QString("%1: %2 columns of images %3 pixels wide; add with the same columns and roi")
                    .arg(name, QString::number(existing->numCols() / std::max(width, 1)), QString::number(width)));
            return {};
        }
        if (d->comboBoxInfoMatrix->findText(name) < 0)
            d->comboBoxInfoMatrix->addItem(name);
        d->comboBoxInfoMatrix->setCurrentIndex(d->comboBoxInfoMatrix->findText(name));
    }
    else
        name = d->newInfoMatrix(name);
    d->slotMakeBigMatrix(files);
    if (!end(d->app()->matrix(name) != nullptr))
        return {};
    return name;
}

QStringList DanRawdata::extractMatrices(const QStringList &runs, int asDouble)
{
    if (runs.isEmpty())
    {
        fail(noRuns);
        return {};
    }
    if (runFiles(runs).isEmpty())
        return {};
    begin();
    if (asDouble >= 0)
        d->checkBoxSaveMatrixDouble->setChecked(asDouble == 1);
    QStringList files = d->extractRawData(runs);
    for (const QString &f : files)
        if (!QFileInfo(f).isFile())
        {
            end(false, "not written: " + f);
            return {};
        }
    end(true);
    return files;
}

//+++ DanMask
// the dead rows (or columns) as createMaskFull() reads them: "a-b" (rows a..b) or "a-b-c-d" (rows a..b, columns
// c..d; for columns the other way round), pixels 1..md, a <= b; empty: none
static QString deadLinesError(const QString &lines, const QString &what, int md)
{
    for (const QString &entry : lines.split(";", Qt::SkipEmptyParts))
    {
        const QStringList numbers = entry.split("-", Qt::SkipEmptyParts);
        bool ok = numbers.count() == 2 || numbers.count() == 4;
        QList<int> values;
        for (const QString &number : numbers)
        {
            bool okNumber = false;
            values << number.trimmed().toInt(&okNumber);
            ok = ok && okNumber && values.last() >= 1 && values.last() <= md;
        }
        if (!ok || values[0] > values[1])
            return QString(R"(%1 "%2": a-b or a-b-c-d, pixels 1..%3, a <= b, e.g. "1-3;250-256")")
                .arg(what, entry.trimmed(), QString::number(md));
    }
    return {};
}

QString DanMask::newMask(const QString &name, const QList<int> &edge, const QList<int> &beamstop, const QString &shape,
                         const QString &triangles, bool transmission, const QString &deadRows, const QString &deadCols,
                         const QString &edgeShape)
{
    if (name.isEmpty())
        return fail("no mask name");
    QSpinBox *edgeBoxes[] = {d->spinBoxLTx, d->spinBoxRBx, d->spinBoxLTy, d->spinBoxRBy};
    QSpinBox *bsBoxes[] = {d->spinBoxLTxBS, d->spinBoxRBxBS, d->spinBoxLTyBS, d->spinBoxRByBS};
    for (const QList<int> *box : {&edge, &beamstop})
        if (!box->isEmpty() && !(box->count() == 1 && box->first() == 0) && box->count() != 4)
            return fail("a mask box is x1, x2, y1, y2 (pixels), or 0 for none");
    if (!shape.isEmpty() && shape != "rectangle" && shape != "ellipse")
        return fail("beam-stop shape: rectangle or ellipse");
    if (!edgeShape.isEmpty() && edgeShape != "rectangle" && edgeShape != "ellipse")
        return fail("edge shape: rectangle or ellipse");
    // as maskTriangles() reads them: "x1-y1,x2-y2,x3-y3;..." in pixels 1..MD, three different points each
    const int md = d->lineEditMD->text().toInt();
    for (const QString &triangle : triangles.split(";", Qt::SkipEmptyParts))
    {
        const QStringList points = triangle.split(",", Qt::SkipEmptyParts);
        QSet<QString> seen;
        bool ok = points.count() == 3;
        for (const QString &point : points)
        {
            const QStringList xy = point.split("-", Qt::SkipEmptyParts);
            bool okX = false, okY = false;
            const int x = xy.count() == 2 ? xy[0].trimmed().toInt(&okX) : 0;
            const int y = xy.count() == 2 ? xy[1].trimmed().toInt(&okY) : 0;
            ok = ok && okX && okY && x >= 1 && y >= 1 && x <= md && y <= md;
            seen.insert(QString::number(x) + "-" + QString::number(y));
        }
        if (!ok || seen.count() != 3)
            return fail("triangle \"" + triangle.trimmed() + "\": three different points x-y, pixels 1.." +
                        QString::number(md) + ", e.g. \"10-10,40-10,60-40\"");
    }
    const QString rowsError = deadLinesError(deadRows, "dead rows", md);
    const QString colsError = deadLinesError(deadCols, "dead columns", md);
    for (const QString &error : {rowsError, colsError})
        if (!error.isEmpty())
            return fail(error);

    if (edge.count() == 4)
    {
        d->groupBoxMask->setChecked(true);
        for (int i = 0; i < 4; i++)
            edgeBoxes[i]->setValue(edge[i]);
    }
    else if (edge.count() == 1)
        d->groupBoxMask->setChecked(false);
    if (beamstop.count() == 4)
    {
        d->groupBoxMaskBS->setChecked(true);
        for (int i = 0; i < 4; i++)
            bsBoxes[i]->setValue(beamstop[i]);
    }
    else if (beamstop.count() == 1)
        d->groupBoxMaskBS->setChecked(false);
    if (!shape.isEmpty())
        d->comboBoxMaskBeamstopShape->setCurrentIndex(shape == "ellipse" ? 1 : 0);
    if (!edgeShape.isEmpty())
        d->comboBoxMaskEdgeShape->setCurrentIndex(edgeShape == "ellipse" ? 1 : 0);
    if (!triangles.isNull())
        d->lineEditMaskTriangles->setText(triangles);
    if (!deadRows.isNull())
        d->lineEditDeadRows->setText(deadRows);
    if (!deadCols.isNull())
        d->lineEditDeadCols->setText(deadCols);

    begin();
    d->saveMaskAs(name);
    if (transmission && d->app()->existWindow(name))
        d->createMaskFullTr(name);
    d->updateMaskList();
    if (!end(isMask(name)))
        return {};
    return name;
}

QString DanMask::loadMask(const QString &name, const QString &fileGiven)
{
    const QString file = resolveFile(fileGiven, d->filesManager->pathInString());
    if (!QFileInfo(file).isFile())
        return fail(noFile(file, fileGiven, true));
    begin();
    d->loadMaskFull(name, file);
    d->updateMaskList();
    if (!end(isMask(name)))
        return {};
    return name;
}

QStringList DanMask::masks() const
{
    return d->app()->findMatrixListByLabel("DAN::Mask::" + d->lineEditMD->text());
}

QString DanMask::tool(const QString &matrix, const QList<QPair<QDoubleSpinBox *, double>> &fields, bool inside,
                      double value, void (dan18::*press)())
{
    if (!d->app()->matrixNames().contains(matrix))
        return fail("no matrix " + matrix);
    // the Python names of the fields, for the message
    static const QMap<QString, QString> names = {
        {"doubleSpinBoxXcenter", "center x"},    {"doubleSpinBoxYcenter", "center y"},
        {"doubleSpinBoxRectXsize", "size x"},    {"doubleSpinBoxRectYsize", "size y"},
        {"doubleSpinBoxEllipseXsize", "size x"}, {"doubleSpinBoxEllipseYsize", "size y"},
        {"doubleSpinBoxAngleFrom", "phi from"},  {"doubleSpinBoxAngleTo", "phi to"},
        {"doubleSpinBoxEllShell", "shell"}};
    for (const auto &field : fields)
        if (field.second < field.first->minimum() || field.second > field.first->maximum())
            return fail(QString("%1: %2..%3")
                            .arg(names.value(field.first->objectName()), QString::number(field.first->minimum()),
                                 QString::number(field.first->maximum())));
    begin();
    // the tools' page, its matrix list showing the matrix: as selecting it there
    d->toolBoxMask->setCurrentIndex(1);
    d->matrixList(matrix);
    if (d->comboBoxMlistMask->currentText() != matrix)
    {
        end(false, "no matrix " + matrix + " in the tools' list");
        return {};
    }
    for (const auto &field : fields)
        field.first->setValue(field.second);
    d->comboBoxMaskInside->setCurrentIndex(inside ? 0 : 1);
    d->comboBoxValue->setCurrentText(QString::number(value));
    (d->*press)();
    end(true);
    return matrix;
}

QString DanMask::setValue(const QString &matrix, double value)
{
    return tool(matrix, {}, true, value, &dan18::maskSetValue);
}

QString DanMask::rectangle(const QString &matrix, double x, double y, double sizeX, double sizeY, bool inside,
                           double value)
{
    return tool(matrix,
                {{d->doubleSpinBoxXcenter, x},
                 {d->doubleSpinBoxYcenter, y},
                 {d->doubleSpinBoxRectXsize, sizeX},
                 {d->doubleSpinBoxRectYsize, sizeY}},
                inside, value, &dan18::maskRectangular);
}

QString DanMask::sector(const QString &matrix, double x, double y, double phiFrom, double phiTo, bool inside,
                        double value)
{
    if (phiTo < phiFrom)
        return fail("sector: phi from <= phi to (-30, 30 goes through 0)");
    return tool(matrix,
                {{d->doubleSpinBoxXcenter, x},
                 {d->doubleSpinBoxYcenter, y},
                 {d->doubleSpinBoxAngleFrom, phiFrom},
                 {d->doubleSpinBoxAngleTo, phiTo}},
                inside, value, &dan18::maskSector);
}

QString DanMask::ellipse(const QString &matrix, double x, double y, double sizeX, double sizeY, bool inside,
                         double value)
{
    if (sizeX <= 0 || sizeY <= 0)
        return fail("ellipse: sizes > 0");
    return tool(matrix,
                {{d->doubleSpinBoxXcenter, x},
                 {d->doubleSpinBoxYcenter, y},
                 {d->doubleSpinBoxEllipseXsize, sizeX},
                 {d->doubleSpinBoxEllipseYsize, sizeY}},
                inside, value, &dan18::maskEllipse);
}

QString DanMask::ellipseShell(const QString &matrix, double x, double y, double sizeX, double sizeY, double shell,
                              bool inside, double value)
{
    if (sizeX <= 0 || sizeY <= 0 || shell <= 0)
        return fail("ellipse shell: sizes and shell > 0");
    return tool(matrix,
                {{d->doubleSpinBoxXcenter, x},
                 {d->doubleSpinBoxYcenter, y},
                 {d->doubleSpinBoxEllipseXsize, sizeX},
                 {d->doubleSpinBoxEllipseYsize, sizeY},
                 {d->doubleSpinBoxEllShell, shell}},
                inside, value, &dan18::maskEllipseShell);
}

QString DanMask::addBeamstop(const QString &mask, const QList<int> &beamstop, const QString &shape)
{
    if (!isMask(mask))
        return fail("no mask " + mask);
    if (!beamstop.isEmpty() && beamstop.count() != 4)
        return fail("beamstop: x1, x2, y1, y2 (pixels)");
    if (!shape.isEmpty() && shape != "rectangle" && shape != "ellipse")
        return fail("beam-stop shape: rectangle or ellipse");
    const int md = d->lineEditMD->text().toInt();
    for (int v : beamstop)
        if (v < 1 || v > md)
            return fail("beamstop: pixels 1.." + QString::number(md));
    begin();
    d->updateMaskList();
    d->comboBoxMaskFor->setCurrentIndex(d->comboBoxMaskFor->findText(mask));
    QSpinBox *boxes[] = {d->spinBoxLTxBS, d->spinBoxRBxBS, d->spinBoxLTyBS, d->spinBoxRByBS};
    if (beamstop.count() == 4)
        for (int i = 0; i < 4; i++)
            boxes[i]->setValue(beamstop[i]);
    if (!shape.isEmpty())
        d->comboBoxMaskBeamstopShape->setCurrentIndex(shape == "ellipse" ? 1 : 0);
    d->maskPlusMaskBS();
    end(true);
    return mask;
}

QString DanMask::saveMask(const QString &mask, const QString &fileGiven)
{
    if (!isMask(mask))
        return fail("no mask " + mask);
    const QString folder = d->filesManager->pathInString();
    QString file = fileGiven.isEmpty() ? folder + mask + ".mask" : expandHome(fileGiven);
    if (QFileInfo(file).isRelative() && !folder.isEmpty())
        file = QDir(folder).filePath(file);
    if (!QFileInfo(QFileInfo(file).absolutePath()).isDir())
        return fail("no folder " + QFileInfo(file).absolutePath());
    begin();
    const bool ok = d->saveMaskToFile(mask, file) && QFileInfo(file).isFile();
    if (!end(ok, "the mask was not saved to " + file))
        return {};
    return file;
}

//+++ DanSensitivity
int DanSensitivity::maskedPixels(const QString &mask) const
{
    const int md = d->lineEditMD->text().toInt();
    gsl_matrix *m = gsl_matrix_alloc(md, md);
    gsl_matrix_set_zero(m);
    int n = 0;
    if (d->make_GSL_Matrix_Symmetric(mask, m, md))
        for (int i = 0; i < md; i++)
            for (int j = 0; j < md; j++)
                n += gsl_matrix_get(m, i, j) == 0.0 ? 1 : 0;
    gsl_matrix_free(m);
    return n;
}

QString DanSensitivity::newSensitivity(const QString &name, const QString &run, const QString &emptyBeam,
                                       const QString &darkCurrent, double transmission, const QString &mask,
                                       const QString &transmissionMode, const QList<double> &errorRange,
                                       int errorMatrix, double maskedValue, const QList<double> &center)
{
    if (name.isEmpty())
        return fail("no sensitivity name");
    if (!d->filesManager->checkFileNumber(run))
        return fail("no run " + run + " in " + d->filesManager->pathInString());
    for (const QString &other : {emptyBeam, darkCurrent})
        if (!other.isEmpty() && !d->filesManager->checkFileNumber(other))
            return fail("no run " + other + " in " + d->filesManager->pathInString());
    if (!isMask(mask))
        return fail("no mask " + mask);
    if (!transmissionMode.isEmpty() && transmissionMode != "measured" && transmissionMode != "formula")
        return fail(R"(transmission: a number, "measured" or "formula")");
    if (transmissionMode.isEmpty() && (transmission <= 0 || transmission > 1))
        return fail("transmission: 0 < Tr <= 1");
    if (transmissionMode == "measured" && emptyBeam.isEmpty())
        return fail(R"(transmission "measured": the empty beam is needed)");
    if (!errorRange.isEmpty() &&
        (errorRange.count() != 2 || errorRange[0] < 0 || errorRange[1] > 100 || errorRange[0] >= errorRange[1]))
        return fail("errorRange: (min, max) in %, 0 <= min < max <= 100");
    if (!std::isnan(maskedValue) && maskedValue < 0)
        return fail("maskedValue: >= 0");
    if (!center.isEmpty() && center.count() != 2)
        return fail("center: (x, y)");

    // begin() first: leaving the Mask tab activates "mask", the mask asked for is selected after it
    begin();
    d->lineEditPlexiAnyD->setText(run);
    d->lineEditEBAnyD->setText(emptyBeam);
    d->lineEditBcAnyD->setText(darkCurrent);
    d->lineEditTransAnyD->setText(QString::number(transmissionMode.isEmpty() ? transmission : 1.0, 'f', 4));
    d->SensitivityLineEditCheck();
    d->buttonGroupSensanyD->setChecked(true);
    d->comboBoxMaskFor->setCurrentIndex(d->comboBoxMaskFor->findText(mask));
    if (errorRange.count() == 2)
    {
        // both at once: checkErrorLimits() resets the pair to 0..100 whenever min >= max in between
        const QSignalBlocker left(d->spinBoxErrLeftLimit), right(d->spinBoxErrRightLimit);
        d->spinBoxErrLeftLimit->setValue(errorRange[0]);
        d->spinBoxErrRightLimit->setValue(errorRange[1]);
    }
    if (errorMatrix >= 0)
        d->checkBoxSensError->setChecked(errorMatrix == 1);
    if (!std::isnan(maskedValue))
        d->lineEditSensMaskedPixels->setText(QString::number(maskedValue));
    d->buttonGroupSensCenter->setChecked(center.count() == 2);
    if (center.count() == 2)
    {
        d->doubleSpinBoxXcenterSens->setValue(center[0]);
        d->doubleSpinBoxYcenterSens->setValue(center[1]);
    }
    if (!transmissionMode.isEmpty())
    {
        // the Transmission button: the formula T(lambda) when its option is checked, else measured
        d->checkBoxSensTr->setChecked(transmissionMode == "formula");
        d->calculateAnyTr();
        const double tr = d->lineEditTransAnyD->text().toDouble();
        if (tr <= 0 || tr > 1)
        {
            end(false, "transmission " + transmissionMode + ": " + d->lineEditTransAnyD->text() + " (0 < Tr <= 1)");
            return {};
        }
    }

    const int maskedBefore = maskedPixels(mask);
    d->saveSensAs(name);
    d->updateSensList();
    if (!end(isSens(name)))
        return {};
    const int added = maskedPixels(mask) - maskedBefore;
    if (added > 0)
        m_lastMessages << QString("mask %1: %2 pixels with errors outside %3..%4 % masked")
                              .arg(mask, QString::number(added), QString::number(d->spinBoxErrLeftLimit->value()),
                                   QString::number(d->spinBoxErrRightLimit->value()));
    return name;
}

bool DanSensitivity::center(const QString &name, double &x, double &y, QString &source)
{
    m_lastError.clear();
    if (!isSens(name))
    {
        m_lastError = "no sensitivity " + name;
        return false;
    }
    return d->sensitivityCenter(name, x, y, source);
}

QString DanSensitivity::saveSensitivity(const QString &name, const QString &fileGiven)
{
    if (!isSens(name))
        return fail("no sensitivity " + name);
    const QString folder = d->filesManager->pathInString();
    QString file = fileGiven.isEmpty() ? folder + name + ".sens" : expandHome(fileGiven);
    if (QFileInfo(file).isRelative() && !folder.isEmpty())
        file = QDir(folder).filePath(file);
    if (!QFileInfo(QFileInfo(file).absolutePath()).isDir())
        return fail("no folder " + QFileInfo(file).absolutePath());
    begin();
    const bool ok = d->saveSensToFile(name, file) && QFileInfo(file).isFile();
    if (!end(ok, "the sensitivity was not saved to " + file))
        return {};
    return file;
}

QString DanSensitivity::loadSensitivity(const QString &name, const QString &fileGiven)
{
    const QString file = resolveFile(fileGiven, d->filesManager->pathInString());
    if (!QFileInfo(file).isFile())
        return fail(noFile(file, fileGiven, false));
    begin();
    d->loadSensFull(name, file);
    d->updateSensList();
    if (!end(isSens(name)))
        return {};
    return name;
}

QStringList DanSensitivity::sensitivities() const
{
    return d->app()->findMatrixListByLabel("DAN::Sensitivity::" + d->lineEditMD->text());
}

bool DanSensitivity::transmission(const QString &run, const QString &emptyBeam, const QString &mask, double &tr,
                                  double &dTr)
{
    for (const QString &number : {run, emptyBeam})
        if (!d->filesManager->checkFileNumber(number))
        {
            m_lastError = "no run " + number + " in " + d->filesManager->pathInString();
            return false;
        }
    if (!isMask(mask))
    {
        m_lastError = "no mask " + mask;
        return false;
    }
    begin();
    double sigma = 0.0;
    tr = d->readTransmission(run, emptyBeam, mask, 0, 0, sigma);
    dTr = tr * sigma;
    return end(tr > 0.0 && tr <= 2.0, "no transmission of " + run + " against " + emptyBeam);
}

//+++ DanProcessing
bool DanProcessing::setConfigurations(int number)
{
    if (number < 1 || number > d->sliderConfigurations->maximum())
    {
        m_lastError = QString("1 to %1 configurations").arg(d->sliderConfigurations->maximum());
        return false;
    }
    begin();
    d->sliderConfigurations->setValue(number);
    return end(d->tableEC->columnCount() == number);
}

int DanProcessing::configurations() const
{
    return d->tableEC->columnCount();
}

bool DanProcessing::setConfiguration(int index, const QString &emptyBeam, const QString &darkCurrent,
                                     const QString &calibrant, const QString &calibrantEmptyBeam,
                                     const QString &calibrantDarkCurrent, const QString &center, const QString &mask,
                                     const QString &sens, const QString &transmissionMask, const QString &cellEmptyBeam,
                                     int transmission)
{
    if (index < 1 || index > configurations())
    {
        m_lastError = QString("configuration %1 of 1..%2").arg(index).arg(configurations());
        return false;
    }
    const int col = index - 1;
    const struct
    {
        int row;
        const QString &run;
    } runs[] = {{dan18::dptEC, emptyBeam},
                {dan18::dptBC, darkCurrent},
                {dan18::dptACFS, calibrant},
                {dan18::dptACEB, calibrantEmptyBeam},
                {dan18::dptACBC, calibrantDarkCurrent},
                {dan18::dptCENTER, center},
                {dan18::dptEB, cellEmptyBeam}};
    for (const auto &r : runs)
        if (!r.run.isEmpty() && !d->filesManager->checkFileNumber(r.run))
        {
            m_lastError = "no run " + r.run + " in " + d->filesManager->pathInString();
            return false;
        }
    const struct
    {
        int row;
        const QString &name;
    } matrices[] = {{dan18::dptMASK, mask}, {dan18::dptSENS, sens}, {dan18::dptMASKTR, transmissionMask}};
    d->updateMaskList();
    d->updateSensList();
    for (const auto &m : matrices)
        if (!m.name.isEmpty() &&
            ((QComboBox *)d->tableEC->cellWidget(m.row, col))->findText(m.name, Qt::MatchExactly) < 0)
        {
            m_lastError = "no " + QString(m.row == dan18::dptSENS ? "sensitivity " : "mask ") + m.name;
            return false;
        }

    begin();
    for (const auto &r : runs)
        if (!r.run.isEmpty())
        {
            if (!d->tableEC->item(r.row, col))
                d->tableEC->setItem(r.row, col, new QTableWidgetItem);
            d->tableEC->item(r.row, col)->setText(r.run);
        }
    for (const auto &m : matrices)
        if (!m.name.isEmpty())
        {
            auto *box = (QComboBox *)d->tableEC->cellWidget(m.row, col);
            box->setCurrentIndex(box->findText(m.name, Qt::MatchExactly));
        }
    // the calibration runs' dark current is the configuration's unless given: one B4C run serves both (without it
    // none was subtracted from the calibrant and the factor came out slightly off)
    auto cellText = [this, col](int row) {
        return d->tableEC->item(row, col) ? d->tableEC->item(row, col)->text().trimmed() : QString();
    };
    if (cellText(dan18::dptACBC).isEmpty() && !cellText(dan18::dptBC).isEmpty() &&
        (!cellText(dan18::dptACFS).isEmpty() || !cellText(dan18::dptACEB).isEmpty()))
    {
        if (!d->tableEC->item(dan18::dptACBC, col))
            d->tableEC->setItem(dan18::dptACBC, col, new QTableWidgetItem);
        d->tableEC->item(dan18::dptACBC, col)->setText(cellText(dan18::dptBC));
    }
    if (transmission >= 0 && d->tableEC->item(dan18::dptECTR, col))
        d->tableEC->item(dan18::dptECTR, col)->setCheckState(transmission ? Qt::Checked : Qt::Unchecked);
    return end(true);
}

QVariantMap DanProcessing::configuration(int index)
{
    QVariantMap values;
    if (index < 1 || index > configurations())
    {
        m_lastError = QString("configuration %1 of 1..%2").arg(index).arg(configurations());
        return values;
    }
    const int col = index - 1;
    auto text = [this, col](int row) {
        if (auto *box = qobject_cast<QComboBox *>(d->tableEC->cellWidget(row, col)))
            return box->currentText();
        return d->tableEC->item(row, col) ? d->tableEC->item(row, col)->text() : QString();
    };
    auto number = [&text](int row) { return text(row).split(QChar(177)).first().trimmed().toDouble(); };
    auto error = [&text](int row) {
        const QStringList parts = text(row).split(QChar(177));
        return parts.count() > 1 ? parts[1].trimmed().toDouble() : 0.0;
    };
    values["emptyBeam"] = text(dan18::dptEC);
    values["darkCurrent"] = text(dan18::dptBC);
    values["C"] = number(dan18::dptC);
    values["D"] = number(dan18::dptD);
    values["lambda"] = number(dan18::dptWL);
    values["beamSize"] = text(dan18::dptBSIZE);
    values["calibrant"] = text(dan18::dptACFS);
    values["calibrantEmptyBeam"] = text(dan18::dptACEB);
    values["calibrantDarkCurrent"] = text(dan18::dptACBC);
    values["calibrantD"] = number(dan18::dptDAC);
    values["mu"] = number(dan18::dptACMU);
    values["calibrantTransmission"] = number(dan18::dptACTR);
    values["factor"] = number(dan18::dptACFAC);
    values["factorError"] = error(dan18::dptACFAC); // the calibrant's counting statistics (0: none)
    values["center"] = text(dan18::dptCENTER);
    values["xCenter"] = number(dan18::dptCENTERX);
    values["xCenterError"] = error(dan18::dptCENTERX);
    values["yCenter"] = number(dan18::dptCENTERY);
    values["yCenterError"] = error(dan18::dptCENTERY);
    values["mask"] = text(dan18::dptMASK);
    values["sens"] = text(dan18::dptSENS);
    values["cellEmptyBeam"] = text(dan18::dptEB);
    values["cellTransmission"] = number(dan18::dptECTR);
    values["transmission"] =
        d->tableEC->item(dan18::dptECTR, col) && d->tableEC->item(dan18::dptECTR, col)->checkState() == Qt::Checked;
    values["transmissionMask"] = text(dan18::dptMASKTR);
    return values;
}

bool DanProcessing::calculate()
{
    begin();
    // the headers pressed as in the interface; a direct-beam calibration through the active mask and sensitivity
    // ("mask", "sens" after the Mask step) when the instrument says so ("Direct Beam:: Use Current Mask and
    // Sensitivity Matrices", KWS-3)
    // dptPOL too, as the interface's corner button: in the PN mode the configurations' polarization states (before,
    // they stayed "out" from Python)
    for (int row : {dan18::dptC, dan18::dptD, dan18::dptWL, dan18::dptBSIZE, dan18::dptPOL, dan18::dptDAC,
                    dan18::dptACMU, dan18::dptACTR, dan18::dptACFAC, dan18::dptCENTERX, dan18::dptECTR})
        d->vertHeaderTableECPressed(row, true);
    return end(true); // a configuration without calibrant or center run is not an error: lastMessages() says so
}

bool DanProcessing::selectScriptTable(const QString &name)
{
    d->updateScriptTables();
    const int index = d->comboBoxMakeScriptTable->findText(name);
    if (index < 0)
    {
        m_lastError = "no script table " + name;
        return false;
    }
    d->comboBoxMakeScriptTable->setCurrentIndex(index);
    return true;
}

QString DanProcessing::newScriptTable(const QString &name, const QStringList &runs)
{
    if (name.isEmpty())
        return fail("no script table name");
    if (d->app()->checkTableExistence(name))
        return fail("a table " + name + " exists already");
    QStringList files;
    for (QString run : runs)
    {
        if (const QString path = resolveFile(run, d->filesManager->pathInString()); QFileInfo(path).isFile())
        {
            files << QFileInfo(path).absoluteFilePath();
            continue;
        }
        const QString file = d->filesManager->fileNameFullHeader(run);
        if (file.isEmpty() || !QFileInfo(file).isFile())
            return fail("no run " + run + " in " + d->filesManager->pathInString());
        files << file;
    }
    if (files.isEmpty())
        return fail(noRuns);
    begin();
    d->newScriptTable(name);
    if (d->comboBoxMakeScriptTable->currentText() == name)
        d->makeScriptTable(files);
    Table *t = nullptr;
    if (!end(d->app()->checkTableExistence(name, t) && t->numRows() > 0))
        return {};
    return name;
}

bool DanProcessing::calculateTransmissions(const QString &scriptTable)
{
    if (!selectScriptTable(scriptTable))
        return false;
    begin();
    d->calculateTransmissionAll();
    // nothing calculated (every row "check transmission" or "Not active configuration"): say why, not later
    Table *t = nullptr;
    int calculated = 0;
    if (d->app()->checkTableExistence(scriptTable, t))
    {
        const int col = static_cast<int>(t->colNames().indexOf("Transmission-Sample"));
        double tr = 0.0;
        bool ok = false;
        for (int row = 0; col >= 0 && row < t->numRows(); row++)
            if (tr = t->text(row, col).section('[', 0, 0).trimmed().toDouble(&ok); ok && tr > 0.0)
                calculated++;
    }
    bool measured = false;
    for (int i = 0; i < d->tableEC->columnCount(); i++)
        measured = measured || (d->tableEC->item(dan18::dptECTR, i) &&
                                d->tableEC->item(dan18::dptECTR, i)->checkState() == Qt::Checked);
    return end(calculated > 0, measured ? "no transmission could be calculated in " + scriptTable
                                        : "no configuration measures transmissions: setConfiguration(i, "
                                          "transmission=True) for the configurations they are measured in");
}

bool DanProcessingOptions::setRemovePoints(int first, int last)
{
    if (first < 0 || last < 0)
    {
        m_lastError = "the points to remove are 0 or more";
        return false;
    }
    begin();
    d->spinBoxRemoveFirst->setValue(first);
    d->spinBoxRemoveLast->setValue(last);
    return end(true);
}

QList<int> DanProcessingOptions::removePoints() const
{
    return {d->spinBoxRemoveFirst->value(), d->spinBoxRemoveLast->value()};
}

static QStringList tableNames(ApplicationWindow *app)
{
    QStringList names;
    for (MdiSubWindow *w : app->tableList())
        names << w->name();
    return names;
}

static QStringList filesIn(const QString &folder)
{
    QStringList files;
    QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        files << it.next() + "|" + QFileInfo(it.filePath()).lastModified().toString(Qt::ISODateWithMs);
    return files;
}

QStringList DanProcessing::process(const QString &scriptTable, const QString &kind, const QString &output)
{
    static const QStringList kinds = {"I-Q",       "I-x-y", "dI-x-y", "I-Qx",   "I-Qy",
                                      "Sigma-x-y", "Q-x-y", "dQ-x-y", "I-Polar"};
    if (!kinds.contains(kind))
    {
        m_lastError = "no processing " + kind + "; one of " + kinds.join(", ");
        return {};
    }
    if (output != "project" && output != "files")
    {
        m_lastError = "output: project or files";
        return {};
    }
    if (!selectScriptTable(scriptTable))
        return {};
    (output == "project" ? d->radioButtonOpenInProject : d->radioButtonFile)->setChecked(true);

    const QStringList tablesBefore = tableNames(d->app());
    const QString folder = d->filesManager->pathOutString();
    const QStringList filesBefore = output == "files" ? filesIn(folder) : QStringList();
    begin();
    d->danDanMultiButton(kind);
    QStringList made;
    if (output == "project")
    {
        for (const QString &name : tableNames(d->app()))
            if (!tablesBefore.contains(name))
                made << name;
    }
    else
        for (const QString &file : filesIn(folder))
            if (!filesBefore.contains(file))
                made << file.section('|', 0, 0);
    NaturalSort::sort(made);
    if (!end(!made.isEmpty()))
        return {};
    return made;
}

//+++ DanMerge
// the template as the project table to merge from, the page and its options set by the caller; the merge button
QStringList DanMerge::mergeTemplate(const QString &templateTable, int removeFirst, int removeLast,
                                    const QString &output)
{
    Table *t = nullptr;
    if (!d->app()->checkTableExistence(templateTable, t))
        return {fail("no merging template " + templateTable)};
    if (output != "project" && output != "files")
        return {fail("output: project or files")};
    if (removeFirst < 0 || removeLast < 0)
        return {fail("points to remove: 0 or more")};
    d->spinBoxRemoveFirstMerging->setValue(removeFirst);
    d->spinBoxRemoveLastMerging->setValue(removeLast);
    d->checkBoxMergeVertical->setChecked(false);
    d->checkBoxMergeFromProjectTable->setChecked(true);
    d->lineEditCheckMergeActiveTable->setText(templateTable);
    t->table()->clearSelection(); // a selection merges only its rows
    begin();
    d->mergeMethod(output == "files");
    if (!end(!d->m_mergedNames.isEmpty(),
             "nothing to merge in " + templateTable + " (one table per sample: one configuration)"))
        return {};
    return d->m_mergedNames;
}

QStringList DanMerge::join(const QString &templateTable, int overlap, int removeFirst, int removeLast,
                           const QString &output, bool smart)
{
    if (overlap < 0 || overlap > 100)
        return {fail("overlap 0..100 %")};
    d->toolBoxMerge->setCurrentIndex(0);
    d->spinBoxOverlap->setValue(overlap);
    // set by the call, never left from the interface: absolute curves are not scaled unless asked for
    d->checkBoxSmart->setChecked(smart);
    d->checkBoxScaleErrors->setChecked(true); // a scaled curve keeps its relative errors
    return mergeTemplate(templateTable, removeFirst, removeLast, output);
}

QStringList DanMerge::rebin(const QString &templateTable, int points, bool manual, double qMin, double qMax,
                            int logStep, int removeFirst, int removeLast, const QString &output)
{
    d->toolBoxMerge->setCurrentIndex(1);
    if (points > 0)
        d->spinBoxRebinPoints->setValue(points);
    if (logStep >= 0)
        d->checkBoxLogStepRebin->setChecked(logStep > 0);
    (manual ? d->radioButtonQrangeManual : d->radioButton)->setChecked(true);
    if (manual)
    {
        d->spinBoxRebinQmin->setValue(qMin);
        d->spinBoxRebinQmax->setValue(qMax);
    }
    return mergeTemplate(templateTable, removeFirst, removeLast, output);
}

QString DanMerge::joinTables(const QString &name, const QStringList &tables, int overlap)
{
    if (name.isEmpty())
        return fail("no name");
    if (tables.count() < 2)
        return fail("two tables or more");
    for (const QString &table : tables)
        if (!d->app()->checkTableExistence(table))
            return fail("no table " + table);
    if (overlap < 0 || overlap > 100)
        return fail("overlap 0..100 %");
    // as in the interface: the tab's own merging table, one row, a column per table, then the merge button
    d->toolBoxMerge->setCurrentIndex(0);
    d->spinBoxOverlap->setValue(overlap);
    d->checkBoxSmart->setChecked(false); // joined as they are (absolute units)
    d->spinBoxRemoveFirstMerging->setValue(0);
    d->spinBoxRemoveLastMerging->setValue(0);
    d->checkBoxMergeFromTable->setChecked(true);
    d->lineEditFilter->setText("*");
    d->spinBoxMmergingCond->setValue(1);
    d->spinBoxNtoMerge->setValue(int(tables.count()));
    d->updateListOfTables();
    d->tableMerge->clearSelection();
    if (!d->tableMerge->item(0, 0))
        d->tableMerge->setItem(0, 0, new QTableWidgetItem);
    d->tableMerge->item(0, 0)->setText(name);
    for (int i = 0; i < tables.count(); i++)
    {
        auto *box = qobject_cast<QComboBox *>(d->tableMerge->cellWidget(0, i + 1));
        const int index = box ? box->findText(tables[i], Qt::MatchExactly) : -1;
        if (index <= 0)
            return fail("no table " + tables[i] + " in the Merge tab's list");
        box->setCurrentIndex(index);
    }
    begin();
    d->mergeMethod(false);
    if (!end(!d->m_mergedNames.isEmpty(), "nothing joined"))
        return {};
    return d->m_mergedNames.first();
}

// (Q, I) of a table's rows with both numeric and I > 0, sorted by Q
static QList<QPair<double, double>> curve(Table *t)
{
    QList<QPair<double, double>> points;
    double q, i;
    for (int row = 0; row < t->numRows(); row++)
        if (t->tryNumericCell(row, 0, q) && t->tryNumericCell(row, 1, i) && q > 0.0 && i > 0.0)
            points << qMakePair(q, i);
    std::sort(points.begin(), points.end());
    return points;
}

bool DanMerge::overlap(const QString &first, const QString &second, double &qMin, double &qMax, int &points,
                       double &ratio)
{
    Table *a = nullptr;
    Table *b = nullptr;
    if (!d->app()->checkTableExistence(first, a) || !d->app()->checkTableExistence(second, b))
    {
        m_lastError = "no table " + (a ? second : first);
        return false;
    }
    begin();
    end(true);
    const auto ca = curve(a);
    const auto cb = curve(b);
    if (ca.count() < 2 || cb.isEmpty())
    {
        m_lastError = "no (Q, I > 0) points";
        return false;
    }
    qMin = std::max(ca.first().first, cb.first().first);
    qMax = std::min(ca.last().first, cb.last().first);
    double sum = 0.0;
    points = 0;
    for (const auto &p : cb)
    {
        if (p.first < qMin || p.first > qMax)
            continue;
        for (int k = 0; k + 1 < ca.count(); k++)
            if (ca[k].first <= p.first && p.first <= ca[k + 1].first)
            {
                // log-log interpolation of the first curve at the second's Q
                const double x = std::log(p.first / ca[k].first) / std::log(ca[k + 1].first / ca[k].first);
                const double i = ca[k].second * std::pow(ca[k + 1].second / ca[k].second, x);
                sum += p.second / i;
                points++;
                break;
            }
    }
    if (points == 0)
    {
        m_lastError = first + " and " + second + " do not overlap";
        return false;
    }
    ratio = sum / points;
    return true;
}

//+++ DanAuto: auto mode
namespace
{
// one option of the Auto-Mode page: its Python name, the instrument file's key "[AutoMode-<key>]", its widget, the
// value of an instrument without it, and the words a combo box's entries stand for (in their order)
struct AutoOption
{
    const char *name;
    const char *key;
    const char *widget;
    const char *value;
    QStringList choices;
};

const QList<AutoOption> &autoOptionTable()
{
    static const QList<AutoOption> table = {
        {"minDuration", "Min-Duration", "spinBoxAutoMinDuration", "30", {}},
        {"minCounts", "Min-Counts", "spinBoxAutoMinCounts", "100", {}},
        {"files", "Files", "lineEditAutoFilesAccept", "*", {}},
        {"skipFiles", "Skip-Files", "lineEditAutoFilesSkip", "", {}},
        {"samples", "Samples", "lineEditAutoSamplesAccept", "*", {}},
        {"skipSamples", "Skip-Samples", "lineEditAutoSamplesSkip", "", {}},
        {"nameField", "Name-Field", "lineEditAutoNameField", "", {}},
        {"emptyBeam", "EB", "lineEditAutoEB", "*eb*||*ec*||*empty*cell*||*empty*beam*", {}},
        {"darkCurrent", "BC", "lineEditAutoBC", "*bc*||*b4c*", {}},
        {"singleDarkCurrent", "BC-Single", "checkBoxAutoBCSingle", "No", {}},
        {"calibrant", "Calibrant", "lineEditAutoCalibrant", "*plex[i,y]*", {}},
        {"flexibleDistance", "Calibrant-FlexD", "checkBoxAutoCalibrantFlexD", "Yes", {}},
        {"flexibleFrom", "Calibrant-FlexD-From", "doubleSpinBoxAutoFlexDFrom", "3.5", {}},
        {"flexibleMin", "Calibrant-FlexD-Min", "doubleSpinBoxAutoFlexDMin", "1.5", {}},
        {"calibrantEmptyBeam", "Calibrant-EB", "lineEditAutoCalibrantEB", "*eb*||*empty*beam*", {}},
        {"calibrantDarkCurrent", "Calibrant-BC", "lineEditAutoCalibrantBC", "*bc*||*b4c*", {}},
        {"singleCalibrantDarkCurrent", "Calibrant-BC-Single", "checkBoxAutoCalibrantBCSingle", "No", {}},
        {"center", "Center-Mode", "comboBoxAutoCenter", "maxflux", {"maxflux", "emptybeam", "wildcard", "beamstop"}},
        {"centerRuns", "Center", "lineEditAutoCenter", "", {}},
        {"cellEmptyBeam", "Cell-EB", "lineEditAutoCellEB", "*eb*||*empty*beam*", {}},
        {"transmission", "Transmission", "lineEditAutoTransmission", "*", {}},
        {"skip", "Skip", "lineEditAutoSkip", "", {}},
        {"masks", "Masks", "comboBoxAutoMasks", "none", {"none", "configuration", "wavelength", "files"}},
        {"transmissionMargin", "TR-Margin", "spinBoxAutoTrMargin", "-2", {}},
        {"findBeamstop", "Find-BS", "checkBoxAutoFindBS", "Yes", {}},
        {"openMask", "Open-Mask", "comboBoxAutoOpenMask", "auto", {"auto", "yes", "no"}},
        {"beamSize", "Beam-Size", "spinBoxAutoBeamSize", "0", {}},
        {"detectorX", "BS-X", "spinBoxAutoBSX", "0", {}},
        {"detectorY", "BS-Y", "spinBoxAutoBSY", "0", {}},
        {"sensitivity", "Sens-Mode", "comboBoxAutoSens", "flat", {"run", "files", "flat"}},
        {"sensitivityRuns", "Sens", "lineEditAutoSens", "*sens*", {}},
        {"scale", "Scale", "checkBoxAutoScale", "No", {}},
        {"centerOffsets", "Center-Offsets", "checkBoxAutoCenterOffsets", "No", {}},
        {"infoTable", "Info-Table", "checkBoxAutoInfoTable", "No", {}},
        {"autoAdd", "Add", "checkBoxAutoAdd", "No", {}},
        {"processIQ", "Process-IQ", "checkBoxAutoProcessIQ", "No", {}},
        {"merge", "Merge", "checkBoxAutoMerge", "No", {}}};
    return table;
}

// a widget's value as the instrument file writes it ("Yes"/"No", the choice's word, the number or the text)
QString autoText(QWidget *w, const AutoOption &o)
{
    if (auto *line = qobject_cast<QLineEdit *>(w))
        return line->text();
    if (auto *spin = qobject_cast<QSpinBox *>(w))
        return QString::number(spin->value());
    if (auto *spin = qobject_cast<QDoubleSpinBox *>(w))
        return QString::number(spin->value());
    if (auto *check = qobject_cast<QCheckBox *>(w))
        return check->isChecked() ? "Yes" : "No";
    if (auto *combo = qobject_cast<QComboBox *>(w))
        return o.choices.value(combo->currentIndex());
    return {};
}

// sets a widget from such a text; false when it is not a value of it
bool setAutoText(QWidget *w, const AutoOption &o, const QString &text)
{
    if (auto *line = qobject_cast<QLineEdit *>(w))
        line->setText(text.trimmed());
    else if (auto *spin = qobject_cast<QSpinBox *>(w))
    {
        bool ok = false;
        const int value = text.trimmed().toInt(&ok);
        if (!ok)
            return false;
        spin->setValue(value);
    }
    else if (auto *spin = qobject_cast<QDoubleSpinBox *>(w))
    {
        bool ok = false;
        const double value = text.trimmed().toDouble(&ok);
        if (!ok)
            return false;
        spin->setValue(value);
    }
    else if (auto *check = qobject_cast<QCheckBox *>(w))
        check->setChecked(text.contains("Yes", Qt::CaseInsensitive) || text.trimmed() == "1" ||
                          text.trimmed().compare("true", Qt::CaseInsensitive) == 0);
    else if (auto *combo = qobject_cast<QComboBox *>(w))
    {
        const auto index = o.choices.indexOf(text.trimmed().toLower());
        if (index < 0)
            return false;
        combo->setCurrentIndex(static_cast<int>(index));
    }
    return true;
}

// the word of a combo box's current entry ("maxflux", "configuration", ...)
QString autoChoice(const QComboBox *box, const QString &name)
{
    for (const AutoOption &o : autoOptionTable())
        if (name == o.name)
            return o.choices.value(box->currentIndex());
    return {};
}

QString optionText(const QVariant &value)
{
    if (value.userType() == QMetaType::Bool)
        return value.toBool() ? "Yes" : "No";
    return value.toString();
}

QVariant optionValue(QWidget *w, const AutoOption &o)
{
    if (qobject_cast<QSpinBox *>(w))
        return autoText(w, o).toInt();
    if (qobject_cast<QDoubleSpinBox *>(w))
        return autoText(w, o).toDouble();
    if (qobject_cast<QCheckBox *>(w))
        return autoText(w, o) == "Yes";
    return autoText(w, o);
}
} // namespace

QVariantMap DanAuto::options() const
{
    QVariantMap values;
    for (const AutoOption &o : autoOptionTable())
        if (auto *w = d->findChild<QWidget *>(o.widget))
            values[o.name] = optionValue(w, o);
    return values;
}

QString DanAuto::setOptions(const QVariantMap &values)
{
    QStringList names;
    for (const AutoOption &o : autoOptionTable())
        names << o.name;
    for (auto it = values.begin(); it != values.end(); ++it)
    {
        const auto index = names.indexOf(it.key());
        if (index < 0)
            return "no auto option " + it.key() + "; autoOptions(): " + names.join(", ");
        const AutoOption &o = autoOptionTable()[index];
        if (!o.choices.isEmpty() && !o.choices.contains(it.value().toString().toLower()))
            return it.key() + ": one of " + o.choices.join(", ");
        auto *w = d->findChild<QWidget *>(o.widget);
        if (!setAutoText(w, o, optionText(it.value())))
            return it.key() + (qobject_cast<QDoubleSpinBox *>(w) ? ": a number" : ": a whole number");
    }
    return {};
}

void DanAuto::resetOptions()
{
    for (const AutoOption &o : autoOptionTable())
        if (auto *w = d->findChild<QWidget *>(o.widget))
            setAutoText(w, o, o.value);
}

QStringList DanAuto::optionLines() const
{
    QStringList lines;
    for (const AutoOption &o : autoOptionTable())
        if (auto *w = d->findChild<QWidget *>(o.widget))
            lines << QString("[AutoMode-%1] %2").arg(o.key, autoText(w, o));
    return lines;
}

bool DanAuto::readOptionLine(const QString &key, const QString &value)
{
    const QList<AutoOption> &table = autoOptionTable();
    const auto it = std::find_if(table.begin(), table.end(), [&key](const AutoOption &o) { return key == o.key; });
    if (it == table.end())
        return false;
    if (auto *w = d->findChild<QWidget *>(it->widget))
        setAutoText(w, *it, value);
    return true;
}

// "s<separator><n>": the n-th part of the name split at the separator (BlackBox's "Samplename-InsidePosition")
QString DanAuto::sampleName(const QString &name) const
{
    const QString field = d->lineEditAutoNameField->text().trimmed();
    if (field.length() < 3 || !field.startsWith('s'))
        return name;
    const int n = field.mid(2).toInt();
    const QStringList parts = name.split(field.at(1));
    return n >= 1 && n <= parts.count() ? parts[n - 1] : name;
}

DanAuto::Run DanAuto::readRun(const QString &run, const QString &file) const
{
    Run r;
    r.run = run;
    r.file = file;
    r.name = d->sample->readName(run).simplified();
    r.duration = d->monitors->readDuration(run);
    r.counts = d->monitors->readSum(run);
    r.C = d->collimation->readC(run) / 100.0; // [m] (DAN reads cm)
    r.D = d->detector->readD(run) / 100.0;
    r.lambda = d->selector->readLambda(run, r.duration);
    r.CA = d->collimation->readCA(run);
    r.SA = d->collimation->readSA(run);
    r.polarization = d->collimation->readPolarization(run);
    r.polarizationAliased = r.polarization;
    if (d->comboBoxMode->currentText().contains("(PN)"))
        d->polarizedAlias(r.polarizationAliased);
    r.attenuator = d->collimation->readAttenuator(run);
    r.detectorX = d->detector->readDetectorX(run);
    r.detectorY = d->detector->readDetectorY(run);
    r.rotationX = d->detector->readDetRotationX(run);
    r.rotationY = d->detector->readDetRotationY(run);
    r.position = d->sample->readPositionNumber(run).toInt();
    // "added files: 53217, 53218" (addHeadersAscii / addNheadersYaml write it into Comment1)
    static const QRegularExpression addedFiles("added files:\\s*([^;]+)");
    const QRegularExpressionMatch m = addedFiles.match(d->sample->readComment1(run));
    if (m.hasMatch())
        for (const QString &one : m.captured(1).split(",", Qt::SkipEmptyParts))
            r.added << one.trimmed();
    return r;
}

static QHash<QString, QPair<QString, DanAuto::Run>> &runCache()
{
    static QHash<QString, QPair<QString, DanAuto::Run>> cache;
    return cache;
}

const QString DanAuto::addedSources = "added-sources";

void DanAuto::forgetRuns()
{
    runCache().clear();
}

QList<DanAuto::Run> DanAuto::runs(bool filtered) const
{
    QMap<QString, QString> files; // run -> its first file (the path)
    QDirIterator it(d->filesManager->pathInString(), QStringList() << d->filesManager->wildCardInString(), QDir::Files,
                    d->filesManager->subFoldersYN() ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
    while (it.hasNext())
    {
        const QString path = it.next();
        QString number =
            FilesManager::findFileNumberInFileName(d->filesManager->wildCardHeader(), QFileInfo(path).fileName());
        if (number.isEmpty())
            continue;
        // a file of a sub-folder is the run "<sub-folder>/<number>", as DAN names it; the runs autoAdd() added moved
        // to "added-sources" are no runs any more
        const QString sub = QDir(d->filesManager->pathInString()).relativeFilePath(QFileInfo(path).path());
        if (sub == addedSources || sub.startsWith(addedSources + "/"))
            continue;
        if (sub != ".")
            number = sub + "/" + number;
        // only what DAN opens as that run: the folder pattern is wider than the run pattern (KWSX-init "*_0_#.edf"
        // listed db_..._0_00007_0.edf of a sub-folder as the run 00007, which no call then found)
        // the file DAN reads for it (..._0_00041.edf, not ..._0_00041_2D_sum.edf, which the number pattern also
        // matches): its time and size tell when the run is read again
        if (!files.contains(number) && d->filesManager->checkFileNumber(number))
            files[number] = d->filesManager->fileNameFull(number, d->filesManager->wildCardDetector());
    }
    QStringList numbers = files.keys();
    NaturalSort::sort(numbers);
    // the headers read once: a folder of 500 runs took minutes when every auto call read them again. A run is read
    // again when its file changed or another instrument (another header map) reads it
    auto &cache = runCache();
    // ... and the header map: its format and its cells (a path or a post-action edited reads every run again)
    QString instrument = d->comboBoxInstrument->currentText() + "|" + d->comboBoxMode->currentText() + "|" +
                         QString::number(d->comboBoxHeaderFormat->currentIndex()) + "|";
    for (int row = 0; row < d->tableHeaderPosNew->rowCount(); row++)
        for (int col = 0; col < 2; col++)
            if (const QTableWidgetItem *item = d->tableHeaderPosNew->item(row, col))
                instrument += item->text() + "|";
    QList<Run> all;
    for (const QString &number : numbers)
    {
        const QFileInfo info(files[number]);
        const QString stamp = info.lastModified().toString(Qt::ISODateWithMs) + "|" + QString::number(info.size());
        auto found = cache.constFind(instrument + info.absoluteFilePath());
        Run r;
        if (found != cache.constEnd() && found->first == stamp)
            r = found->second;
        else
        {
            r = readRun(number, info.fileName());
            cache.insert(instrument + info.absoluteFilePath(), qMakePair(stamp, r));
        }
        r.sample = sampleName(r.name);
        all << r;
    }
    // an added run of an instrument whose header has no comment (KWSX-init's EDF): DAN names it <first run> + the
    // count in 5 digits ("00023" + 3 runs: "0002300003"); it is one when those runs' durations add up to its own
    QHash<QString, int> index;
    for (int i = 0; i < all.count(); i++)
        index[all[i].run] = i;
    for (Run &r : all)
    {
        if (!r.added.isEmpty() || r.run.length() <= 5)
            continue;
        const int first = index.value(r.run.left(r.run.length() - 5), -1);
        bool ok = false;
        const int count = r.run.right(5).toInt(&ok);
        if (!ok || first < 0 || count < 2 || first + count > all.count())
            continue;
        double duration = 0;
        QStringList parts;
        for (int k = first; k < first + count; k++)
        {
            duration += all[k].duration;
            parts << all[k].run;
        }
        if (std::abs(duration - r.duration) <= 1e-6 * std::max(1.0, r.duration))
            r.added = parts;
    }
    if (!filtered)
        return all;
    return filter(all, d->spinBoxAutoMinDuration->value(), d->spinBoxAutoMinCounts->value(),
                  d->lineEditAutoFilesAccept->text(), d->lineEditAutoFilesSkip->text(),
                  d->lineEditAutoSamplesAccept->text(), d->lineEditAutoSamplesSkip->text());
}

QList<DanAuto::Run> DanAuto::filter(const QList<Run> &all, int minDuration, int minCounts, const QString &files,
                                    const QString &skipFiles, const QString &samples, const QString &skipSamples)
{
    QList<Run> kept;
    for (const Run &r : all)
        // minCounts 0: no count filter (a header map without Sum reads 0 counts for every run)
        if (r.duration > minDuration && (minCounts <= 0 || r.counts > minCounts) &&
            (files.trimmed().isEmpty() || matches(r.file, files)) &&
            (skipFiles.trimmed().isEmpty() || !matches(r.file, skipFiles)) &&
            (samples.trimmed().isEmpty() || matches(r.sample, samples)) &&
            (skipSamples.trimmed().isEmpty() || !matches(r.sample, skipSamples)))
            kept << r;
    return kept;
}

// wildcards "||"-separated, case and spaces ignored, the whole name: "*ec*" anywhere, "EB" exactly that name,
// "*plex[i,y]*" a character class ("[0-9]" a range, "[!...]" the negation);
// "*" also matches "/" (a header value is no path)
bool DanAuto::matches(const QString &name, const QString &wildcards)
{
    QString text = name;
    text.remove(' ');
    for (QString w : wildcards.split("||"))
    {
        w.remove(' ');
        if (w.isEmpty())
            continue;
        QString pattern = QRegularExpression::escape(w);
        pattern.replace("\\*", ".*");
        pattern.replace("\\?", ".");
        pattern.replace("\\[\\!", "[^");
        pattern.replace("\\[", "[");
        pattern.replace("\\]", "]");
        pattern.replace("\\-", "-"); // ranges "[0-9]"; outside a class "-" is literal anyway
        const QRegularExpression re(QRegularExpression::anchoredPattern(pattern),
                                    QRegularExpression::CaseInsensitiveOption);
        if (re.isValid() && re.match(text).hasMatch())
            return true;
    }
    return false;
}

QString DanAuto::label(const Run &r) const
{
    QString text = "C" + QString::number(qRound(r.C)) + "D" + QString::number(r.D, 'f', 2) + "L" +
                   QString::number(r.lambda, 'f', 1) + "BS" + r.CA + "|" + r.SA;
    if (d->checkBoxPolarizationAsPara->isChecked())
        text += "P" + r.polarizationAliased;
    if (d->checkBoxAttenuatorAsPara->isChecked())
        text += "A" + r.attenuator;
    return text;
}

// the script table's rule (dan18::compareConfigurations); without D for a calibrant at another distance
// dan18::compareConfigurations() (the script table's rule) on the headers read once; ignoreD: at any distance
bool DanAuto::sameConfiguration(const Run &a, const Run &b, bool ignoreD) const
{
    const double range = 0.05;
    auto close = [](double x, double y, double r) { return fabs(x - y) <= r * fabs(x + y) / 2.0; };
    if (!close(a.lambda, b.lambda, range) || (!ignoreD && !close(a.D, b.D, range)) || !close(a.C, b.C, range) ||
        a.CA != b.CA || a.SA != b.SA)
        return false;
    if (d->checkBoxBeamcenterAsPara->isChecked() &&
        (!close(a.detectorX, b.detectorX, range / 5.0) || !close(a.detectorY, b.detectorY, range / 5.0)))
        return false;
    if (d->checkBoxRecalculateUseNumber->isChecked() && a.position != b.position)
        return false;
    if (d->checkBoxPolarizationAsPara->isChecked() && a.polarizationAliased != b.polarizationAliased)
        return false;
    if (d->checkBoxAttenuatorAsPara->isChecked() && a.attenuator != b.attenuator)
        return false;
    if (d->checkBoxDetRotAsPara->isChecked() &&
        (!close(a.rotationX, b.rotationX, range / 5.0) || !close(a.rotationY, b.rotationY, range / 5.0)))
        return false;
    return true;
}

QList<QList<DanAuto::Run>> DanAuto::conditions(const QList<Run> &runs) const
{
    QList<QList<Run>> groups;
    for (const Run &r : runs)
    {
        bool placed = false;
        for (QList<Run> &g : groups)
            if (sameConfiguration(g.first(), r))
            {
                g << r;
                placed = true;
                break;
            }
        if (!placed)
            groups << QList<Run>{r};
    }
    return groups;
}

namespace
{
QList<DanAuto::Run> named(const QList<DanAuto::Run> &runs, const QString &wildcards)
{
    QList<DanAuto::Run> found;
    if (!wildcards.trimmed().isEmpty())
        for (const DanAuto::Run &r : runs)
            if (DanAuto::matches(r.sample, wildcards))
                found << r;
    return found;
}

// the longest run (or the one with the most counts); first of equals
const DanAuto::Run *best(const QList<const DanAuto::Run *> &runs, bool counts)
{
    const DanAuto::Run *found = nullptr;
    for (const DanAuto::Run *r : runs)
        if (!found || (counts ? r->counts > found->counts : r->duration > found->duration))
            found = r;
    return found;
}

bool contains(const QList<DanAuto::Run> &runs, const QString &run)
{
    return std::ranges::any_of(runs, [&run](const DanAuto::Run &r) { return r.run == run; });
}
} // namespace

QList<DanAuto::Run> DanAuto::standardRuns(const QList<Run> &runs) const
{
    QList<Run> standard;
    for (const QString &wildcards :
         {d->lineEditAutoEB->text(), d->lineEditAutoBC->text(), d->lineEditAutoCalibrant->text(),
          d->lineEditAutoCalibrantEB->text(), d->lineEditAutoCalibrantBC->text()})
        for (const Run &r : named(runs, wildcards))
            if (!contains(standard, r.run))
                standard << r;
    return standard;
}

QList<DanAuto::Run> DanAuto::sampleRuns(const QList<Run> &runs) const
{
    QList<Run> standard = standardRuns(runs);
    QStringList more = {d->lineEditAutoCellEB->text()};
    if (d->lineEditAutoSens->text().trimmed() != "*")
        more << d->lineEditAutoSens->text();
    for (const QString &wildcards : more)
        standard << named(runs, wildcards);
    QList<Run> samples;
    for (const Run &r : runs)
        if (!contains(standard, r.run))
            samples << r;
    return samples;
}

QList<DanAuto::Choice> DanAuto::configurations(const QList<Run> &runs, QStringList &dropped) const
{
    const QList<Run> emptyBeams = named(runs, d->lineEditAutoEB->text());
    const QList<Run> darkCurrents = named(runs, d->lineEditAutoBC->text());
    const QList<Run> calibrants = named(runs, d->lineEditAutoCalibrant->text());
    const QList<Run> calibrantEBs = named(runs, d->lineEditAutoCalibrantEB->text());
    const QList<Run> calibrantBCs = named(runs, d->lineEditAutoCalibrantBC->text());
    const QList<Run> cellEBs = named(runs, d->lineEditAutoCellEB->text());
    const QList<Run> standard = standardRuns(runs);
    const QString centerMode = autoChoice(d->comboBoxAutoCenter, "center");

    // the runs of a list in the configuration of a run (ignoreD: at any distance)
    auto in = [this](const QList<Run> &list, const Run &of, bool ignoreD = false) {
        QList<const Run *> found;
        for (const Run &r : list)
            if (sameConfiguration(of, r, ignoreD))
                found << &r;
        return found;
    };
    auto all = [](const QList<Run> &list) {
        QList<const Run *> found;
        for (const Run &r : list)
            found << &r;
        return found;
    };
    auto run = [](const Run *r) { return r ? r->run : QString(); };

    // the empty beam of each condition, the skipped ones left out, and the configurations measuring the transmissions
    QList<Run> chosen;
    for (const QList<Run> &condition : conditions(emptyBeams))
    {
        const Run &eb = *best(all(condition), false);
        if (matches(label(eb), d->lineEditAutoSkip->text()))
            dropped << label(eb) + ": skipped (auto option skip)";
        else
            chosen << eb;
    }
    const QSet<QString> transmissions = transmissionRuns(chosen);

    QList<Choice> choices;
    for (const Run &eb : chosen)
    {
        Choice c;
        c.emptyBeam = eb;
        const QString name = label(eb);
        c.darkCurrent =
            run(best(d->checkBoxAutoBCSingle->isChecked() ? all(darkCurrents) : in(darkCurrents, eb), false));
        // the calibrant: most counts in the configuration; "other D" (from flexibleFrom on, 3.5 m): the nearest
        // distance above flexibleMin (1.5 m), most counts among equals; no calibrant wildcard: the direct beam
        const Run *calibrant = nullptr;
        if (!calibrants.isEmpty())
        {
            calibrant = best(in(calibrants, eb), true);
            const double from = d->doubleSpinBoxAutoFlexDFrom->value();
            const double minimum = d->doubleSpinBoxAutoFlexDMin->value();
            if (d->checkBoxAutoCalibrantFlexD->isChecked() && eb.D >= from)
            {
                const Run *nearest = nullptr;
                for (const Run *r : in(calibrants, eb, true))
                    if (r->D > minimum &&
                        (!nearest || fabs(r->D - eb.D) < fabs(nearest->D - eb.D) ||
                         (fabs(r->D - eb.D) == fabs(nearest->D - eb.D) && r->counts > nearest->counts)))
                        nearest = r;
                if (nearest)
                    calibrant = nearest;
            }
        }
        c.calibrant = run(calibrant);
        const Run &calibration = calibrant ? *calibrant : eb;
        c.calibrantEmptyBeam = run(best(in(calibrantEBs, calibration), false));
        c.calibrantDarkCurrent = run(best(
            d->checkBoxAutoCalibrantBCSingle->isChecked() ? all(calibrantBCs) : in(calibrantBCs, calibration), false));
        c.transmission = transmissions.contains(eb.run);
        if (centerMode == "emptybeam")
            c.center = eb.run;
        else
        {
            QList<Run> candidates;
            for (const Run &r : centerMode == "wildcard" ? named(runs, d->lineEditAutoCenter->text()) : runs)
                if (!contains(standard, r.run))
                    candidates << r;
            c.center = run(best(in(candidates, eb), true));
            // beamstop: one without a direct beam is kept, decided below
            if (c.center.isEmpty() && (centerMode == "maxflux" || (centerMode == "beamstop" && c.transmission)))
            {
                dropped << name + ": no run for the center (only standard runs)";
                continue;
            }
        }
        c.cellEmptyBeam = run(best(in(cellEBs, eb), false));
        choices << c;
    }
    // beamstop: max flux where the direct beam is measured, else no center run (the center of the beam-stop mask):
    // with the beam behind the beam stop in all runs a sample's center can be far off
    if (centerMode == "beamstop")
    {
        QList<Choice> kept;
        for (Choice &c : choices)
            if (!directBeam(c, choices))
                c.center.clear();
            else if (c.center.isEmpty())
                dropped << label(c.emptyBeam) + ": no run for the center (only standard runs)";
        for (const Choice &c : choices)
            if (!c.center.isEmpty() || !directBeam(c, choices))
                kept << c;
        return kept;
    }
    return choices;
}

QString DanAuto::maskName(const Run &emptyBeam, bool transmission, const QList<Run> &emptyBeams)
{
    const QString distance = QString::number(emptyBeam.D, 'g', 4);
    QString name = QString(transmission ? "mask-tr-" : "mask-bs-") + distance + "m";
    for (const Run &other : emptyBeams)
        if (other.run != emptyBeam.run && QString::number(other.D, 'g', 4) == distance)
            return name + "-" + QString::number(emptyBeam.lambda, 'g', 3) + "A";
    return name;
}

QSet<QString> DanAuto::transmissionRuns(const QList<Run> &emptyBeams) const
{
    QList<Run> matching;
    for (const Run &eb : emptyBeams)
        if (matches(label(eb), d->lineEditAutoTransmission->text()))
            matching << eb;
    // separate transmission configurations (Skip Transmission Configurations) where the transmissions are measured
    // with the direct beam (Transmission Method "Direct Beam", or the optional one at 9.5 A and more; the others read
    // them from the header in every configuration): of those with the same beam-stop mask (another attenuator or
    // beam-stop position) the one whose empty beam is the direct beam, most of its counts inside the beam stop's box
    const bool skip = d->checkBoxSkiptransmisionConfigurations->isChecked();
    auto separate = [this, skip](const Run &eb) { return skip && directBeamTransmission(eb.lambda); };
    const QList<int> box = {d->spinBoxLTxBS->value(), d->spinBoxRBxBS->value(), d->spinBoxLTyBS->value(),
                            d->spinBoxRByBS->value()};
    QSet<QString> found;
    QHash<QString, const Run *> direct;
    QHash<QString, double> share;
    for (const Run &eb : matching)
    {
        if (!separate(eb))
        {
            found << eb.run;
            continue;
        }
        const QString mask = maskName(eb, false, emptyBeams);
        share[eb.run] = directBeamShare(eb.run, box);
        if (!direct.contains(mask) || share[eb.run] > share[direct[mask]->run])
            direct[mask] = &eb;
    }
    for (const Run *eb : direct)
        found << eb->run;
    return found;
}

bool DanAuto::openMask() const
{
    const QString open = autoChoice(d->comboBoxAutoOpenMask, "openMask");
    if (open != "auto")
        return open == "yes";
    return autoChoice(d->comboBoxAutoMasks, "masks") == "configuration" &&
           autoChoice(d->comboBoxAutoCenter, "center") == "emptybeam" &&
           autoChoice(d->comboBoxAutoSens, "sensitivity") == "flat";
}

bool DanAuto::directBeamTransmission(double lambda) const
{
    const int method = d->comboBoxTransmMethod->currentIndex();
    return method == 1 || (method == 4 && lambda >= 9.5);
}

double DanAuto::directBeamShare(const QString &run, const QList<int> &box) const
{
    const int md = d->lineEditMD->text().toInt();
    const int halfX = std::abs(box[1] - box[0]) / 2;
    const int halfY = std::abs(box[3] - box[2]) / 2;
    gsl_matrix *data = gsl_matrix_calloc(md, md);
    d->readMatrixCor(run, data);
    size_t row = 0;
    size_t col = 0;
    gsl_matrix_max_index(data, &row, &col);
    double total = 0.0;
    double inside = 0.0;
    for (int i = 0; i < md; i++)
        for (int j = 0; j < md; j++)
        {
            const double v = gsl_matrix_get(data, i, j);
            total += v;
            if (std::abs(i - int(row)) <= halfY && std::abs(j - int(col)) <= halfX)
                inside += v;
        }
    gsl_matrix_free(data);
    return total > 0 ? inside / total : 0.0;
}

bool DanAuto::beamVisible(const QString &run, const QList<int> &box) const
{
    if (directBeamShare(run, box) > 0.5)
        return true;
    // a sharp peak: the brightest pixel 20 times the mean 4..6 pixels around it (a beam stop's shadow with the most
    // scattering at its edge: 1.2..11, the beam beside or through it: 26 and more)
    const int md = d->lineEditMD->text().toInt();
    gsl_matrix *data = gsl_matrix_calloc(md, md);
    d->readMatrixCor(run, data);
    size_t row = 0;
    size_t col = 0;
    gsl_matrix_max_index(data, &row, &col);
    const double peak = gsl_matrix_get(data, row, col);
    double ring = 0.0;
    int n = 0;
    for (int i = std::max(0, int(row) - 6); i <= std::min(md - 1, int(row) + 6); i++)
        for (int j = std::max(0, int(col) - 6); j <= std::min(md - 1, int(col) + 6); j++)
            if (std::max(std::abs(i - int(row)), std::abs(j - int(col))) >= 4)
            {
                ring += gsl_matrix_get(data, i, j);
                n++;
            }
    gsl_matrix_free(data);
    return peak > 0 && n > 0 && peak > 20.0 * ring / n;
}

const DanAuto::Run *DanAuto::directBeam(const Choice &c, const QList<Choice> &choices)
{
    if (c.transmission)
        return &c.emptyBeam;
    QList<Run> emptyBeams;
    for (const Choice &other : choices)
        emptyBeams << other.emptyBeam;
    const QString mask = maskName(c.emptyBeam, false, emptyBeams);
    for (const Choice &other : choices)
        if (other.transmission && maskName(other.emptyBeam, false, emptyBeams) == mask)
            return &other.emptyBeam;
    return nullptr;
}

QVariantMap DanOptionsConfig::autoOptions() const
{
    return DanAuto(d).options();
}

bool DanOptionsConfig::setAutoOptions(const QVariantMap &values)
{
    const QString error = DanAuto(d).setOptions(values);
    if (!error.isEmpty())
    {
        fail(error);
        return false;
    }
    begin();
    return end(true);
}

namespace
{
QVariantMap runInfo(const DanAuto::Run &r)
{
    return {{"run", r.run},
            {"sample", r.sample},
            {"duration", r.duration},
            {"counts", r.counts},
            {"C", r.C},
            {"D", r.D},
            {"lambda", r.lambda},
            {"beamSize", r.CA + "|" + r.SA},
            {"polarization", r.polarization},
            {"attenuator", r.attenuator},
            {"detectorX", r.detectorX},
            {"detectorY", r.detectorY},
            {"added", r.added}};
}

// the runs given (all of the folder when none), in the folder's order
QList<DanAuto::Run> selected(const DanAuto &a, const QStringList &runs, bool filtered)
{
    QList<DanAuto::Run> all = runs.isEmpty() ? a.runs(filtered) : a.runs(false);
    if (runs.isEmpty())
        return all;
    QList<DanAuto::Run> found;
    for (const DanAuto::Run &r : all)
        if (runs.contains(r.run))
            found << r;
    return found;
}

QStringList numbers(const QList<DanAuto::Run> &runs)
{
    QStringList list;
    for (const DanAuto::Run &r : runs)
        list << r.run;
    return list;
}
} // namespace

QStringList DanRawdata::runs(int minDuration, int minCounts, const QString &files, const QString &skipFiles,
                             const QString &samples, const QString &skipSamples)
{
    const DanAuto a(d);
    return numbers(DanAuto::filter(a.runs(false), minDuration, minCounts, files, skipFiles, samples, skipSamples));
}

QVariantList DanRawdata::info(const QStringList &runs)
{
    m_lastError.clear();
    const QList<DanAuto::Run> found = selected(DanAuto(d), runs, false);
    // a run asked for and not found is an error (it was left out without a word)
    const QStringList known = numbers(found);
    QStringList missing;
    for (const QString &run : runs)
        if (!known.contains(run) && !missing.contains(run))
            missing << run;
    if (!missing.isEmpty())
    {
        fail("no run " + missing.join(", ") + " in " + d->filesManager->pathInString());
        return {};
    }
    QVariantList list;
    for (const DanAuto::Run &r : found)
    {
        QVariantMap one = runInfo(r);
        one["files"] = filesOf(r.run);
        list << one;
    }
    return list;
}

QVariantList DanRawdata::conditions(const QStringList &runs)
{
    const DanAuto a(d);
    QVariantList list;
    for (const QList<DanAuto::Run> &g : a.conditions(selected(a, runs, true)))
    {
        const DanAuto::Run &r = g.first();
        list << QVariantMap{{"label", a.label(r)},           {"C", r.C},          {"D", r.D}, {"lambda", r.lambda},
                            {"beamSize", r.CA + "|" + r.SA}, {"runs", numbers(g)}};
    }
    return list;
}

QVariantList DanRawdata::neighbours(int n, const QStringList &runs)
{
    return neighbours(n, runs, 2);
}

QVariantList DanRawdata::neighbours(int n, const QStringList &runs, int minimum)
{
    m_lastError.clear();
    // n 0: every whole series (2 runs or more, as many as were measured); n >= 2: the series cut into groups of n
    if (n == 1 || n < 0)
    {
        fail("n: None (the whole series) or 2 or more");
        return {};
    }
    const DanAuto a(d);
    // next to each other in the FOLDER: all its runs, those not asked for part a sequence
    const QList<DanAuto::Run> found = a.runs(false);
    const QStringList known = numbers(found);
    for (const QString &run : runs)
        if (!known.contains(run))
        {
            fail("no run " + run + " in " + d->filesManager->pathInString());
            return {};
        }
    // the folder's order, without the runs made by adding (an added run between two would part them)
    QList<DanAuto::Run> measured;
    for (const DanAuto::Run &r : found)
        if (r.added.isEmpty())
            measured << r;
    // DAN's configuration of each run
    QHash<QString, int> condition;
    const QList<QList<DanAuto::Run>> groups = a.conditions(measured);
    for (int i = 0; i < groups.count(); i++)
        for (const DanAuto::Run &r : groups[i])
            condition[r.run] = i;
    QVariantList list;
    QList<DanAuto::Run> sequence;
    auto close = [&]() {
        // a whole series needs minimum runs (the merge-table button shows single runs too: 1)
        const int size = n == 0 ? (sequence.count() >= minimum ? static_cast<int>(sequence.count()) : 0) : n;
        for (int start = 0; size > 0 && start + size <= sequence.count(); start += size)
        {
            QStringList group;
            for (int k = start; k < start + size; k++)
                group << sequence[k].run;
            list << QVariantMap{{"label", a.label(sequence[start])},
                                {"condition", condition.value(sequence[start].run) + 1},
                                {"sample", sequence[start].sample},
                                {"runs", group}};
        }
        sequence.clear();
    };
    for (const DanAuto::Run &r : measured)
    {
        if (!runs.isEmpty() && !runs.contains(r.run))
        {
            close();
            continue;
        }
        if (!sequence.isEmpty() &&
            (r.sample != sequence.last().sample || condition.value(r.run) != condition.value(sequence.last().run)))
            close();
        sequence << r;
    }
    close();
    return list;
}

QVariantList DanRawdata::sampleGroups(const QStringList &runs)
{
    return sampleGroups(runs, 2);
}

QVariantList DanRawdata::sampleGroups(const QStringList &runs, int minimum)
{
    m_lastError.clear();
    const DanAuto a(d);
    const QList<DanAuto::Run> found = a.runs(false);
    const QStringList known = numbers(found);
    for (const QString &run : runs)
        if (!known.contains(run))
        {
            fail("no run " + run + " in " + d->filesManager->pathInString());
            return {};
        }
    QList<DanAuto::Run> measured;
    for (const DanAuto::Run &r : found)
        if (r.added.isEmpty() && (runs.isEmpty() || runs.contains(r.run)))
            measured << r;
    // DAN's configurations (numbered as conditions() lists them), then the sample in each, in the folder's order
    const QList<QList<DanAuto::Run>> groups = a.conditions(measured);
    QVariantList list;
    for (int i = 0; i < groups.count(); i++)
    {
        QStringList samples;
        QHash<QString, QStringList> bySample;
        for (const DanAuto::Run &r : groups[i])
        {
            if (!bySample.contains(r.sample))
                samples << r.sample;
            bySample[r.sample] << r.run;
        }
        for (const QString &sample : samples)
            if (bySample[sample].count() >= minimum)
                list << QVariantMap{{"label", a.label(groups[i].first())},
                                    {"condition", i + 1},
                                    {"sample", sample},
                                    {"runs", bySample[sample]}};
    }
    return list;
}

QStringList DanRawdata::sampleRuns(const QStringList &runs)
{
    const DanAuto a(d);
    return numbers(a.sampleRuns(selected(a, runs, true)));
}

namespace
{
// the matrices of the input folder's files matching a pattern: "mask"/"sens" the one file or <name>.mask/.sens,
// the others by their base names
QStringList folderFiles(const QString &folder, const QString &pattern)
{
    QStringList files = QDir(folder).entryList(QStringList() << pattern, QDir::Files, QDir::Name);
    return files;
}

QString matrixName(const QStringList &files, const QString &file, const QString &standard)
{
    QString base = QFileInfo(file).completeBaseName();
    const bool standardFile = files.contains(standard + "." + QFileInfo(file).suffix(), Qt::CaseInsensitive);
    if (files.count() == 1 || base.compare(standard, Qt::CaseInsensitive) == 0 ||
        (!standardFile && file == files.first()))
        return standard;
    return base;
}
} // namespace

QStringList DanMask::loadMasks(const QString &pattern)
{
    const QString folder = d->filesManager->pathInString();
    const QStringList files = folderFiles(folder, pattern);
    if (files.isEmpty())
        return {fail("no " + pattern + " file in " + folder)};
    QStringList made;
    for (const QString &file : files)
    {
        const QString name = loadMask(matrixName(files, file, "mask"), QDir(folder).filePath(file));
        if (name.isEmpty())
            return {};
        made << name;
    }
    return made;
}

QList<int> DanMask::findBeamstop(const QString &run, const QList<int> &box)
{
    const int md = d->lineEditMD->text().toInt();
    const int w = std::abs(box[1] - box[0]) + 1;
    const int h = std::abs(box[3] - box[2]) + 1;
    const int ring = 3;
    if (md <= 0 || w + 2 * ring > md || h + 2 * ring > md)
        return box;
    gsl_matrix *data = gsl_matrix_calloc(md, md);
    gsl_matrix *mask = gsl_matrix_alloc(md, md);
    gsl_matrix_set_all(mask, 1.0);
    d->readMatrixCor(run, data);
    d->make_GSL_Matrix_Symmetric("mask", mask, md);
    // sums of the counts and of the pixels used (the edges and dead pixels of "mask" left out, its beam stop not)
    // from the corner to each pixel: any box's sums in four lookups
    std::vector<double> counts((md + 1) * (md + 1), 0.0);
    std::vector<double> pixels((md + 1) * (md + 1), 0.0);
    auto at = [md](int y, int x) { return y * (md + 1) + x; };
    for (int y = 0; y < md; y++)
        for (int x = 0; x < md; x++)
        {
            const bool used = gsl_matrix_get(mask, y, x) > 0 ||
                              (x + 1 >= std::min(box[0], box[1]) && x + 1 <= std::max(box[0], box[1]) &&
                               y + 1 >= std::min(box[2], box[3]) && y + 1 <= std::max(box[2], box[3]));
            const double v = used ? gsl_matrix_get(data, y, x) : 0.0;
            counts[at(y + 1, x + 1)] = v + counts[at(y, x + 1)] + counts[at(y + 1, x)] - counts[at(y, x)];
            pixels[at(y + 1, x + 1)] =
                (used ? 1.0 : 0.0) + pixels[at(y, x + 1)] + pixels[at(y + 1, x)] - pixels[at(y, x)];
        }
    gsl_matrix_free(data);
    gsl_matrix_free(mask);
    auto sum = [&at](const std::vector<double> &s, int y1, int x1, int y2, int x2) {
        return s[at(y2, x2)] - s[at(y1, x2)] - s[at(y2, x1)] + s[at(y1, x1)];
    };
    // the box whose surrounding ring (3 pixels) is the brightest against its inside, of any size from half to 1.5
    // times the instrument's: a box larger than the shadow is placed off its center
    double best = 0.0;
    double xc = 0.0;
    double yc = 0.0;
    for (int bh = std::max(3, h / 2); bh <= 3 * h / 2 && bh + 2 * ring <= md; bh++)
        for (int bw = std::max(3, w / 2); bw <= 3 * w / 2 && bw + 2 * ring <= md; bw++)
            for (int y = 0; y + bh <= md; y++)
                for (int x = 0; x + bw <= md; x++)
                {
                    const double inPixels = sum(pixels, y, x, y + bh, x + bw);
                    if (inPixels < 0.8 * bw * bh)
                        continue;
                    const int y1 = std::max(0, y - ring);
                    const int x1 = std::max(0, x - ring);
                    const int y2 = std::min(md, y + bh + ring);
                    const int x2 = std::min(md, x + bw + ring);
                    const double ringPixels = sum(pixels, y1, x1, y2, x2) - inPixels;
                    if (ringPixels < 10)
                        continue;
                    const double inCounts = sum(counts, y, x, y + bh, x + bw);
                    const double score = (sum(counts, y1, x1, y2, x2) - inCounts) / ringPixels - inCounts / inPixels;
                    if (score > best)
                    {
                        best = score;
                        xc = x + 1 + (bw - 1) / 2.0;
                        yc = y + 1 + (bh - 1) / 2.0;
                    }
                }
    // the instrument's box size around the shadow's center
    QList<int> found = box;
    if (best > 0)
        found = {int(std::lround(xc - (w - 1) / 2.0)), int(std::lround(xc - (w - 1) / 2.0)) + w - 1,
                 int(std::lround(yc - (h - 1) / 2.0)), int(std::lround(yc - (h - 1) / 2.0)) + h - 1};
    return found;
}

QStringList DanMask::autoMasks()
{
    begin();
    const DanAuto a(d);
    const QString mode = autoChoice(d->comboBoxAutoMasks, "masks");
    QStringList made;
    if (mode == "files")
    {
        made = loadMasks("*.mask");
        if (!made.contains("mask") && !newMask("mask", {}, {}, QString(), QString(), false).isEmpty())
            made.prepend("mask");
        report("masks: " + made.join(", ") + " (the *.mask files of the input folder)");
        end(!made.isEmpty());
        return made;
    }
    // centers from the empty beams, a beam-stop mask per configuration and a flat sensitivity: "mask" without the
    // beam stop. The flat "sens" is made from it (= "mask"), the direct beam's center and the absolute factor
    // (DirectBeam calibrant, "Use Current Mask and Sensitivity") then count the beam itself; the samples get the
    // configurations' beam-stop masks
    const bool beamOpen = a.openMask();
    // the instrument's box and beam stop again for the masks made later, however this call ends; the edge-only mask
    // of the direct beams' centers removed
    const QList<int> box = {d->spinBoxLTxBS->value(), d->spinBoxRBxBS->value(), d->spinBoxLTyBS->value(),
                            d->spinBoxRByBS->value()};
    const bool beamStopChecked = d->groupBoxMaskBS->isChecked();
    const QString edgeOnly = "mask-auto-edge";
    const auto restore = qScopeGuard([this, &box, beamStopChecked, &edgeOnly] {
        QSpinBox *spins[] = {d->spinBoxLTxBS, d->spinBoxRBxBS, d->spinBoxLTyBS, d->spinBoxRByBS};
        for (int i = 0; i < 4; i++)
            spins[i]->setValue(box[i]);
        d->groupBoxMaskBS->setChecked(beamStopChecked);
        if (MdiSubWindow *w = d->app()->window(edgeOnly))
        {
            w->askOnCloseEvent(false);
            d->app()->closeWindow(w);
            d->updateMaskList();
        }
    });
    if (newMask("mask", {}, beamOpen ? QList<int>{0} : QList<int>{}, QString(), QString(), false).isEmpty())
    {
        end(false);
        return {};
    }
    made << "mask";
    const int margin = d->spinBoxAutoTrMargin->value();
    if (mode == "none")
    {
        // transmissions with the direct beam in a configuration: "mask" hides it (SANS-1), a transmission mask of the
        // instrument's box
        QStringList dropped;
        const QList<DanAuto::Choice> choices = a.configurations(a.runs(true), dropped);
        const bool trMask = std::ranges::any_of(
            choices, [&a](const DanAuto::Choice &c) { return a.directBeamTransmission(c.emptyBeam.lambda); });
        const QList<int> tr = {box[0] - margin, box[1] + margin, box[2] - margin, box[3] + margin};
        if (trMask && newMask("mask-tr", {}, tr, QString(), QString(), true).isEmpty())
        {
            end(false);
            return {};
        }
        if (trMask)
            made << "mask-tr";
        report(trMask ? QString("masks: mask (the instrument's), mask-tr [x %1..%2, y %3..%4] (transmissions with the "
                                "direct beam)")
                            .arg(tr[0])
                            .arg(tr[1])
                            .arg(tr[2])
                            .arg(tr[3])
                      : QString("masks: mask (the instrument's)"));
        end(true);
        return made;
    }
    // the beam-stop masks need the center of each empty beam, calculated through "mask" and "sens" (made anew: one
    // of an earlier folder or session is not this one's)
    if (d->danSensitivity->autoSensitivity().isEmpty())
    {
        end(false, "no sensitivity for the centers of the masks: " + d->danSensitivity->lastError());
        return {};
    }
    // the direct beams' centers through a mask without the beam stop: through "mask" the beam under the instrument's
    // box was hidden and the center drifted to the scattering around it (PSI 8 m: 4 pixels)
    if (!beamOpen && newMask(edgeOnly, {}, {0}, QString(), QString(), false).isEmpty())
    {
        end(false);
        return {};
    }
    const QString centerMask = beamOpen ? QString("mask") : edgeOnly;
    const double dX = fabs(double(box[1] - box[0]));
    const double dY = fabs(double(box[3] - box[2]));
    // a box of w x h pixels around a center, rounded (truncated, the boxes moved up to half a pixel)
    auto around = [](double xc, double yc, int w, int h) {
        const int x1 = int(std::lround(xc - (w - 1) / 2.0));
        const int y1 = int(std::lround(yc - (h - 1) / 2.0));
        return QList<int>{x1, x1 + w - 1, y1, y1 + h - 1};
    };
    const QList<DanAuto::Run> runs = a.runs(true);
    QStringList dropped;
    const QList<DanAuto::Choice> choices = a.configurations(runs, dropped);
    QList<DanAuto::Run> emptyBeams;
    for (const DanAuto::Choice &c : choices)
        emptyBeams << c.emptyBeam;
    QStringList boxes;
    // the center of a direct beam (1-based pixels), from its brightest pixel: from the instrument's beam-stop center
    // the center was found far off when the beam stop is elsewhere (a detector moved sideways)
    const int md = d->lineEditMD->text().toInt();
    auto center = [this, md, &centerMask](const DanAuto::Run &eb, double &xc, double &yc) {
        gsl_matrix *data = gsl_matrix_calloc(md, md);
        d->readMatrixCor(eb.run, data);
        size_t row = 0;
        size_t col = 0;
        gsl_matrix_max_index(data, &row, &col);
        gsl_matrix_free(data);
        // calcCenterNew() starts from and returns 1-based pixels (as runCenter() calls it): the center was one pixel
        // off, the start 0-based and 1 added to the result
        xc = double(col) + 1.0;
        yc = double(row) + 1.0;
        double xcErr = 0.0;
        double ycErr = 0.0;
        d->calcCenterNew(eb.run, -1, xc, yc, xcErr, ycErr, centerMask, "sens");
    };
    // one beam-stop mask per name (configurations of another attenuator or beam-stop position share it), centered on
    // the direct beam, else (masks "configuration") on the beam stop's shadow in the empty beam; a transmission mask
    // only where the direct beam is measured and the transmissions are calculated with it
    // the configuration making a shared mask: one with the beam stop in (not measuring the transmissions), whose
    // detector position moves the box (wavelength; one with the beam stop out moved it off the detector)
    QList<const DanAuto::Choice *> makers;
    for (const DanAuto::Choice &c : choices)
    {
        const QString name = DanAuto::maskName(c.emptyBeam, false, emptyBeams);
        auto same = [&name, &emptyBeams](const DanAuto::Choice *m) {
            return DanAuto::maskName(m->emptyBeam, false, emptyBeams) == name;
        };
        const auto it = std::find_if(makers.begin(), makers.end(), same);
        if (it == makers.end())
            makers << &c;
        else if ((*it)->transmission && !c.transmission)
            *it = &c;
    }
    for (const DanAuto::Choice *maker : makers)
    {
        const DanAuto::Choice &c = *maker;
        const DanAuto::Run &eb = c.emptyBeam;
        const DanAuto::Run *direct = DanAuto::directBeam(c, choices);
        // a transmission mask where the transmissions are measured with the direct beam, not read from the header
        // (Transmission Method; KWS1-2020's optional one below 9.5 A)
        const bool trWindow = direct && a.directBeamTransmission(eb.lambda);
        QList<int> bs;
        QList<int> tr;
        // per configuration: the instrument's box size scaled by the beam size (CA) / the reference beam size
        double scale = 1.0;
        const double reference = d->spinBoxAutoBeamSize->value();
        const QStringList ca = eb.CA.split('x');
        if (reference > 0 && ca.count() == 2)
        {
            scale = (ca[0].toDouble() + ca[1].toDouble()) / 2.0 / reference;
            if (scale < 0.2 || scale > 5.0)
                scale = 1.0;
        }
        const int w = int(std::lround(dX * scale)) + 1;
        const int h = int(std::lround(dY * scale)) + 1;
        // no direct beam measured: around the beam stop's shadow in the empty beam (searched with the box of that
        // size); around the instrument's box's center when the beam is seen in it (beside or through the beam stop:
        // KWS1's leaks moved both the shadow and the beam's center 3-4 pixels)
        if (mode == "configuration" && !direct)
        {
            const QList<int> scaled = around((box[0] + box[1]) / 2.0, (box[2] + box[3]) / 2.0, w, h);
            bs = d->checkBoxAutoFindBS->isChecked() && !a.beamVisible(eb.run, box) ? findBeamstop(eb.run, scaled)
                                                                                   : scaled;
        }
        else if (mode == "configuration")
        {
            // around the direct beam's center
            double xc = 0.0;
            double yc = 0.0;
            center(*direct, xc, yc);
            bs = around(xc, yc, w, h);
            tr = bs;
        }
        else
        {
            // the instrument's box moved with the detector from its reference position (0: not moved)
            const double binning = d->comboBoxBinning->currentText().toDouble();
            const double pixelX = binning * d->lineEditResoPixelSize->text().toDouble();
            const double pixelY = pixelX * d->lineEditAsymetry->text().toDouble();
            const int x0 = d->spinBoxAutoBSX->value();
            const int y0 = d->spinBoxAutoBSY->value();
            const int dx = x0 != 0 && pixelX > 0 ? int(std::lround((eb.detectorX - x0) / pixelX / 10.0)) : 0;
            const int dy = y0 != 0 && pixelY > 0 ? int(std::lround((eb.detectorY - y0) / pixelY / 10.0)) : 0;
            bs = {box[0] + dx, box[1] + dx, box[2] + dy, box[3] + dy};
            // the transmission window around the direct beam's center
            if (trWindow)
            {
                double xc = 0.0;
                double yc = 0.0;
                center(*direct, xc, yc);
                tr = around(xc, yc, int(dX) + 1, int(dY) + 1);
            }
        }
        if (trWindow)
            tr = {tr[0] - margin, tr[1] + margin, tr[2] - margin, tr[3] + margin};
        for (const bool transmission : {false, true})
        {
            if (transmission && !trWindow)
                continue;
            const QString name = DanAuto::maskName(eb, transmission, emptyBeams);
            if (newMask(name, {}, transmission ? tr : bs, QString(), QString(), transmission).isEmpty())
            {
                end(false);
                return {};
            }
            made << name;
            const QList<int> &b = transmission ? tr : bs;
            boxes << QString("%1 [x %2..%3, y %4..%5]").arg(name).arg(b[0]).arg(b[1]).arg(b[2]).arg(b[3]);
        }
    }
    report(QString(beamOpen ? "masks: mask (no beam stop: flat sens, direct beam), " : "masks: mask, ") +
           boxes.join(", "));
    end(true);
    return made;
}

QStringList DanSensitivity::loadSensitivities(const QString &pattern)
{
    const QString folder = d->filesManager->pathInString();
    const QStringList files = folderFiles(folder, pattern);
    if (files.isEmpty())
        return {fail("no " + pattern + " file in " + folder)};
    QStringList made;
    for (const QString &file : files)
    {
        const QString name = loadSensitivity(matrixName(files, file, "sens"), QDir(folder).filePath(file));
        if (name.isEmpty())
            return {};
        made << name;
    }
    return made;
}

QString DanSensitivity::autoSensitivity()
{
    begin();
    if (!isMask("mask") && d->danMask->newMask("mask", {}, {}, QString(), QString(), false).isEmpty())
    {
        end(false, R"(no mask "mask": )" + d->danMask->lastError());
        return {};
    }
    const QString mode = autoChoice(d->comboBoxAutoSens, "sensitivity");
    if (mode == "files")
    {
        const QStringList made = loadSensitivities("*.sens");
        if (!end(made.contains("sens"), lastError()))
            return {};
        report("sensitivity: " + made.join(", ") + " (the *.sens files of the input folder)");
        return "sens";
    }
    if (mode == "run")
    {
        const DanAuto a(d);
        const DanAuto::Run *found = nullptr;
        const QList<DanAuto::Run> runs = a.runs(true);
        for (const DanAuto::Run &r : runs)
            if (DanAuto::matches(r.sample, d->lineEditAutoSens->text()) && (!found || r.counts > found->counts))
                found = &r;
        if (found)
        {
            QString name = newSensitivity("sens", found->run, QString(), QString(), 1.0, "mask");
            if (!end(!name.isEmpty(), lastError()))
                return {};
            report("sensitivity: sens from run " + found->run + " (" + found->sample + ")");
            return name;
        }
        d->danMessage("no sensitivity run (auto option sensitivityRuns: " + d->lineEditAutoSens->text() +
                      "): a flat sensitivity");
        report("sensitivity: no run named " + d->lineEditAutoSens->text() + " among the runs");
    }
    d->buttonGroupSensanyD->setChecked(false);
    d->comboBoxMaskFor->setCurrentIndex(d->comboBoxMaskFor->findText("mask"));
    d->saveSensAs("sens");
    d->updateSensList();
    if (!end(isSens("sens")))
        return {};
    report("sensitivity: sens flat");
    return "sens";
}

QVariantList DanProcessing::autoConfigurations()
{
    begin();
    const DanAuto a(d);
    const QList<DanAuto::Run> runs = a.runs(true);
    QStringList dropped;
    const QList<DanAuto::Choice> choices = a.configurations(runs, dropped);
    report(QString("%1 runs, %2 configuration(s)").arg(runs.count()).arg(choices.count()));
    for (const QString &reason : dropped)
    {
        d->danMessage(reason);
        report("  left out: " + reason);
    }
    // no run left by the filters: say which one (a header map without Sum gave every run 0 counts, and the error
    // named the empty beam)
    if (runs.isEmpty())
    {
        const QList<DanAuto::Run> all = a.runs(false);
        const int minDuration = d->spinBoxAutoMinDuration->value();
        const int minCounts = d->spinBoxAutoMinCounts->value();
        int longer = 0, counted = 0, withCounts = 0;
        for (const DanAuto::Run &r : all)
        {
            longer += r.duration > minDuration ? 1 : 0;
            counted += minCounts <= 0 || r.counts > minCounts ? 1 : 0;
            withCounts += r.counts > 0 ? 1 : 0;
        }
        QString text = QString("no runs pass the auto filters: %1 runs, %2 with duration > %3 s (auto option "
                               "minDuration), %4 with counts > %5 (minCounts)")
                           .arg(all.count())
                           .arg(longer)
                           .arg(minDuration)
                           .arg(counted)
                           .arg(minCounts);
        if (!all.isEmpty() && withCounts == 0)
            text += "; no run has counts: is the header's Sum read for this instrument? (or minCounts 0: no count "
                    "filter)";
        else if (longer > 0 && counted > 0 && longer + counted > all.count())
            text += "; the files / samples wildcards (files, skipFiles, samples, skipSamples) leave none";
        end(false, text);
        return {};
    }
    if (choices.isEmpty())
    {
        end(false, "no configuration: no empty beam among the runs (auto option emptyBeam: " +
                       d->lineEditAutoEB->text() + ")" + (dropped.isEmpty() ? QString() : "; " + dropped.join("; ")));
        return {};
    }
    QList<DanAuto::Run> emptyBeams;
    for (const DanAuto::Choice &c : choices)
        emptyBeams << c.emptyBeam;
    if (!setConfigurations(int(choices.count())))
    {
        end(false, lastError());
        return {};
    }
    const QString masks = autoChoice(d->comboBoxAutoMasks, "masks");
    const QString folder = d->filesManager->pathInString();
    QVariantList list;
    for (int i = 0; i < choices.count(); i++)
    {
        const DanAuto::Choice &c = choices[i];
        // masks "none": "mask" (a beam-stop mask of an earlier folder or session is not this one's); "files": the mask
        // file named as auto mode names the configuration's mask ("mask-bs-9.2m.mask"), told when there is none
        QString mask = DanAuto::maskName(c.emptyBeam, false, emptyBeams);
        const bool noFile = masks == "files" && !QFileInfo::exists(QDir(folder).filePath(mask + ".mask"));
        if (noFile)
            d->danMessage(QString("configuration %1 (%2): no mask file %3.mask, \"mask\" used")
                              .arg(i + 1)
                              .arg(a.label(c.emptyBeam), mask));
        mask = masks != "none" && !noFile && isMask(mask) ? mask : "mask";
        // no transmission mask where the direct beam is not measured (one of an earlier folder is not this one's): the
        // beam-stop mask
        QString trMask = DanAuto::maskName(c.emptyBeam, true, emptyBeams);
        trMask = masks != "none" && DanAuto::directBeam(c, choices) && a.directBeamTransmission(c.emptyBeam.lambda) &&
                         isMask(trMask)
                     ? trMask
                     : mask;
        // masks "none", transmissions with the direct beam: autoMasks()' "mask-tr"
        if (masks == "none" && a.directBeamTransmission(c.emptyBeam.lambda) && isMask("mask-tr"))
            trMask = "mask-tr";
        // the run cells emptied first: setConfiguration() leaves a cell whose run is not given, and a run chosen
        // before (another option, folder) stayed where this choice has none
        for (const int row : {dan18::dptEC, dan18::dptBC, dan18::dptACFS, dan18::dptACEB, dan18::dptACBC,
                              dan18::dptCENTER, dan18::dptEB})
            if (QTableWidgetItem *item = d->tableEC->item(row, i))
                item->setText(QString());
        if (!setConfiguration(i + 1, c.emptyBeam.run, c.darkCurrent, c.calibrant, c.calibrantEmptyBeam,
                              c.calibrantDarkCurrent, c.center, mask, isSens("sens") ? "sens" : QString(), trMask,
                              c.cellEmptyBeam, c.transmission ? 1 : 0))
        {
            end(false, lastError());
            return {};
        }
        // no center run: the center of the mask's beam stop (calculate() keeps it)
        if (c.center.isEmpty())
        {
            double xc = 0.0;
            double yc = 0.0;
            d->readCenterfromMaskName(mask, xc, yc, d->lineEditMD->text().toInt());
            auto cell = [this, i](int row, const QString &text) {
                if (!d->tableEC->item(row, i))
                    d->tableEC->setItem(row, i, new QTableWidgetItem);
                d->tableEC->item(row, i)->setText(text);
            };
            cell(dan18::dptCENTER, QString());
            cell(dan18::dptCENTERX, QString::number(xc + 1.0, 'f', 3) + QChar(177) + "0.000");
            cell(dan18::dptCENTERY, QString::number(yc + 1.0, 'f', 3) + QChar(177) + "0.000");
        }
        list << configuration(i + 1);
        report(QString("  %1 %2: EB %3, B4C %4, %5, center %6, cell EB %7%8")
                   .arg(i + 1)
                   .arg(a.label(c.emptyBeam), c.emptyBeam.run, c.darkCurrent.isEmpty() ? "-" : c.darkCurrent,
                        c.calibrant.isEmpty() ? "direct beam " + c.calibrantEmptyBeam
                                              : "calibrant " + c.calibrant + " (EB " + c.calibrantEmptyBeam + ")",
                        c.center.isEmpty() ? "of " + mask : c.center, c.cellEmptyBeam.isEmpty() ? "-" : c.cellEmptyBeam,
                        c.transmission ? ", transmissions measured" : ""));
    }
    end(true);
    return list;
}

bool DanProcessing::runCenter(const QString &run, const QString &mask, const QString &sens, double &x, double &y)
{
    if (!d->filesManager->checkFileNumber(run))
    {
        fail("no run " + run + " in " + d->filesManager->pathInString());
        return false;
    }
    if (!isMask(mask))
    {
        fail("no mask " + mask);
        return false;
    }
    if (!isSens(sens))
    {
        fail("no sensitivity " + sens);
        return false;
    }
    if (d->radioButtonCenterReadFromHeader->isChecked())
    {
        x = d->detector->readDetectorX(run);
        y = d->detector->readDetectorY(run);
        return true;
    }
    // as the configurations' centers: H.F. starts from the mask's beam-stop center (1-based)
    const int md = d->lineEditMD->text().toInt();
    d->readCenterfromMaskName(mask, x, y, md);
    x += 1.0;
    y += 1.0;
    double xError = 0.0;
    double yError = 0.0;
    d->calcCenterNew(run, -1, x, y, xError, yError, mask, sens);
    return true;
}

bool DanProcessing::centerOffset(const QString &run, double xCenter, double yCenter, const QString &mask,
                                 const QString &sens, double &hShift, double &vShift)
{
    double x = 0.0;
    double y = 0.0;
    if (!runCenter(run, mask, sens, x, y))
        return false;
    hShift = xCenter - x;
    vShift = yCenter - y;
    return true;
}

int DanProcessing::addCenterOffsets(const QString &scriptTable, const QString &mask, const QString &sens)
{
    Table *t = nullptr;
    if (!d->app()->checkTableExistence(scriptTable, t))
    {
        fail("no script table " + scriptTable);
        return -1;
    }
    QStringList cn = t->colNames();
    const int runCol = static_cast<int>(cn.indexOf("#-Run"));
    const int xCol = static_cast<int>(cn.indexOf("X-center"));
    const int yCol = static_cast<int>(cn.indexOf("Y-center"));
    const int maskCol = static_cast<int>(cn.indexOf("Mask"));
    const int sensCol = static_cast<int>(cn.indexOf("Sens"));
    if (runCol < 0 || xCol < 0 || yCol < 0 || maskCol < 0 || sensCol < 0)
    {
        fail("script table " + scriptTable + " without #-Run, X-center, Y-center, Mask or Sens");
        return -1;
    }
    for (const QString &m : {mask, sens})
        if (!m.isEmpty() && !(m == mask ? isMask(m) : isSens(m)))
        {
            fail(QString(m == mask ? "no mask " : "no sensitivity ") + m);
            return -1;
        }
    // the shifts first: a row without them fails the call before a column is made
    auto number = [t](int row, int col) { return t->text(row, col).split(QChar(177)).first().trimmed().toDouble(); };
    QList<QPair<double, double>> shifts;
    for (int row = 0; row < t->numRows(); row++)
    {
        double h = 0.0;
        double v = 0.0;
        const QString run = t->text(row, runCol).trimmed();
        // not given: as the configuration's center was found (calcCenterNew(col)): the row's mask and sensitivity,
        // or the active ones with a direct-beam calibration through "Use Current Mask and Sensitivity"
        const bool active = d->checkBoxACDBuseActive->isChecked() &&
                            (d->comboBoxACmethod->currentIndex() == 1 || d->comboBoxACmethod->currentIndex() == 4);
        const QString rowMask = !mask.isEmpty() ? mask
                                : active        ? d->comboBoxMaskFor->currentText()
                                                : t->text(row, maskCol).trimmed();
        const QString rowSens = !sens.isEmpty() ? sens
                                : active        ? d->comboBoxSensFor->currentText()
                                                : t->text(row, sensCol).trimmed();
        if (!centerOffset(run, number(row, xCol), number(row, yCol), rowMask, rowSens, h, v))
        {
            m_lastError = "row " + QString::number(row + 1) + " (" + run + "): " + m_lastError;
            return -1;
        }
        shifts << qMakePair(h, v);
    }
    begin();
    for (const QString &name : {QString("VShift"), QString("HShift")})
        if (!cn.contains(name))
        {
            t->addColumns(1);
            t->setColName(t->numCols() - 1, name, false, false);
            t->setColumnType(t->numCols() - 1, Table::Text);
            cn = t->colNames();
        }
    const int vCol = static_cast<int>(cn.indexOf("VShift"));
    const int hCol = static_cast<int>(cn.indexOf("HShift"));
    QStringList lines;
    for (int row = 0; row < shifts.count(); row++)
    {
        t->setText(row, hCol, QString::number(shifts[row].first, 'f', 3));
        t->setText(row, vCol, QString::number(shifts[row].second, 'f', 3));
        lines << QString("%1 %2, %3")
                     .arg(t->text(row, runCol).trimmed(), QString::number(shifts[row].first, 'f', 2),
                          QString::number(shifts[row].second, 'f', 2));
    }
    report("center offsets (HShift, VShift) to the empty beams' centers: " + lines.join("; "));
    end(true);
    return static_cast<int>(shifts.count());
}

int DanProcessing::autoScale(const QString &scriptTable)
{
    Table *t = nullptr;
    if (!d->app()->checkTableExistence(scriptTable, t))
    {
        fail("no script table " + scriptTable);
        return -1;
    }
    // the configurations by wavelength; a wavelength counts when every one of its configurations measures its own
    // transmissions; its reference: the longest distance
    auto number = [this](int row, int col) {
        return d->tableEC->item(row, col) ? d->tableEC->item(row, col)->text().split(QChar(177)).first().toDouble()
                                          : 0.0;
    };
    const int n = configurations();
    QMap<int, int> reference; // configuration (1..n) -> the configuration it is scaled to
    for (int i = 0; i < n; i++)
    {
        QList<int> same;
        for (int j = 0; j < n; j++)
            if (fabs(number(dan18::dptWL, i) - number(dan18::dptWL, j)) <=
                0.05 * fabs(number(dan18::dptWL, i) + number(dan18::dptWL, j)) / 2.0)
                same << j;
        bool own = same.count() > 1;
        int longest = i;
        for (int j : same)
        {
            own = own && d->tableEC->item(dan18::dptECTR, j) &&
                  d->tableEC->item(dan18::dptECTR, j)->checkState() == Qt::Checked;
            if (number(dan18::dptD, j) > number(dan18::dptD, longest))
                longest = j;
        }
        if (own)
            reference[i + 1] = longest + 1;
    }
    if (reference.isEmpty())
        return 0;
    const QStringList cn = t->colNames();
    const int sampleCol = static_cast<int>(cn.indexOf("Run-info"));
    const int conditionCol = static_cast<int>(cn.indexOf("#-Condition"));
    const int trCol = static_cast<int>(cn.indexOf("Transmission-Sample"));
    if (sampleCol < 0 || conditionCol < 0 || trCol < 0)
    {
        fail("script table " + scriptTable + " without Run-info, #-Condition or Transmission-Sample");
        return -1;
    }
    auto tr = [t, trCol](int row) { return t->text(row, trCol).split("[").first().trimmed().toDouble(); };
    int scaleCol = static_cast<int>(cn.indexOf("Scale"));
    if (scaleCol < 0)
    {
        t->addColumns(1);
        scaleCol = t->numCols() - 1;
        t->setColName(scaleCol, "Scale", false, false);
        t->setColumnType(scaleCol, Table::Text);
    }
    begin();
    int scaled = 0;
    QStringList scales;
    for (int row = 0; row < t->numRows(); row++)
    {
        const int condition = t->text(row, conditionCol).toInt();
        double scale = 1.0;
        if (reference.contains(condition) && reference[condition] != condition)
        {
            int match = -1;
            for (int other = 0; other < t->numRows(); other++)
                if (t->text(other, conditionCol).toInt() == reference[condition] &&
                    t->text(other, sampleCol).trimmed() == t->text(row, sampleCol).trimmed())
                    match = other;
            if (match >= 0 && tr(match) > 0.0 && tr(row) > 0.0)
            {
                scale = tr(row) / tr(match);
                scaled++;
            }
            else
                d->danMessage("row " + QString::number(row + 1) + " (" + t->text(row, sampleCol).trimmed() +
                              "): no transmission of the same sample in configuration " +
                              QString::number(reference[condition]) + ": Scale 1");
        }
        t->setText(row, scaleCol, QString::number(scale, 'g', 13));
        if (scale != 1.0)
            scales << QString("%1 %2").arg(t->text(row, sampleCol).trimmed(), QString::number(scale, 'f', 4));
    }
    report("Scale (to the longest distance of the wavelength): " + (scales.isEmpty() ? "none" : scales.join(", ")));
    end(true);
    return scaled;
}

QStringList DanProcessing::autoAdd()
{
    begin();
    const QString folder = d->filesManager->pathInString();
    // the runs the run filters select, of the input folder itself (a run "<sub-folder>/<number>" would be added under
    // a name with "/"); the groups as the adding template makes them: all runs of a sample + configuration, or the
    // neighbours with "Only consecutive runs"; a single run is no group
    QStringList runs;
    QStringList inSubFolders;
    for (const DanAuto::Run &r : DanAuto(d).runs(true))
        (r.run.contains('/') ? inSubFolders : runs) << r.run;
    if (!inSubFolders.isEmpty())
        report(QString("adding: %1 runs of sub-folders left out (%2, ...)")
                   .arg(inSubFolders.count())
                   .arg(inSubFolders.first()));
    const bool consecutive = d->checkBoxAddConsecutive->isChecked();
    const QVariantList groups = runs.isEmpty() ? QVariantList()
                                : consecutive  ? d->danRawdata->neighbours(0, runs)
                                               : d->danRawdata->sampleGroups(runs);
    if (groups.isEmpty())
    {
        report("adding: no sample measured twice in one configuration" +
               QString(consecutive ? " (consecutive runs)" : ""));
        end(true);
        return {};
    }
    // the added runs written into the input folder (the output folder for the time of the call), their single runs
    // moved to "added-sources" there: the folder then holds what a reduction reads
    const QString output = d->lineEditPathRAD->text();
    const auto restore = qScopeGuard([this, &output] { d->lineEditPathRAD->setText(output); });
    d->lineEditPathRAD->setText(folder);
    const QDir sources(QDir(folder).filePath(DanAuto::addedSources));
    if (!sources.exists() && !QDir(folder).mkdir(DanAuto::addedSources))
    {
        end(false, "no folder " + sources.path());
        return {};
    }
    QStringList made;
    for (const QVariant &g : groups)
    {
        const QStringList group = g.toMap()["runs"].toStringList();
        const QStringList files = d->danRawdata->files(group);
        if (files.isEmpty() || d->danRawdata->addRuns(group, QString()).isEmpty())
        {
            end(false, "adding " + group.join(", ") + ": " + d->danRawdata->lastError());
            return made;
        }
        const QString run = group.first() + QString("%1").arg(group.count(), 5, 10, QChar('0'));
        for (const QString &file : files)
            if (QFileInfo::exists(file) && !QFile::rename(file, sources.filePath(QFileInfo(file).fileName())))
            {
                end(false, "adding: " + file + " not moved to " + sources.path());
                return made;
            }
        made << run;
        report(QString("added %1 = %2 (%3, %4); its runs moved to %5/")
                   .arg(run, group.join(" + "), g.toMap()["label"].toString(), g.toMap()["sample"].toString(),
                        DanAuto::addedSources));
    }
    DanAuto::forgetRuns();
    int moved = 0;
    for (const QVariant &g : groups)
        moved += static_cast<int>(g.toMap()["runs"].toStringList().count());
    report(QString("adding: %1 run(s) added, %2 single runs moved to %3/")
               .arg(made.count())
               .arg(moved)
               .arg(DanAuto::addedSources));
    end(true);
    return made;
}

QString DanProcessing::autoInfoTable(const QString &name)
{
    if (name.isEmpty())
        return fail("no table name");
    if (d->app()->checkTableExistence(name))
        return fail("a table " + name + " exists already");
    begin();
    QStringList runs;
    for (const DanAuto::Run &r : DanAuto(d).runs(true))
        runs << r.run;
    if (runs.isEmpty())
    {
        end(false, "no runs pass the auto filters (minDuration, minCounts, files, skipFiles, samples, skipSamples)");
        return {};
    }
    if (d->danRawdata->newInfoTable(name, runs, -1, -1, -1).isEmpty())
    {
        end(false, d->danRawdata->lastError());
        return {};
    }
    report(QString("info table %1: %2 runs").arg(name).arg(runs.count()));
    end(true);
    return name;
}

QString DanProcessing::autoConfig(const QString &scriptTable)
{
    if (d->app()->checkTableExistence(scriptTable))
        return fail("a table " + scriptTable + " exists already");
    begin();
    auto stop = [this](const QString &reason) {
        end(false, reason);
        return QString();
    };
    // the runs of a sample measured several times added first, when the option asks for it: the steps below see the
    // added runs, their single runs moved away
    if (d->checkBoxAutoAdd->isChecked())
    {
        autoAdd();
        if (!lastError().isEmpty())
            return stop("adding: " + lastError());
    }
    if (d->danMask->autoMasks().isEmpty())
        return stop("masks: " + d->danMask->lastError());
    // "sens" anew by the option (a "sens" of an earlier folder or session is not this one's); the beam-stop masks
    // made it already, for their centers
    const QString masks = autoChoice(d->comboBoxAutoMasks, "masks");
    if ((masks == "none" || masks == "files") && d->danSensitivity->autoSensitivity().isEmpty())
        return stop("sensitivity: " + d->danSensitivity->lastError());
    // "mask" without the beam stop (auto option openMask): a direct-beam calibration takes the center and the factor
    // through it, "Use Current Mask and Sensitivity" checked as one would (unchecked, the configuration's beam-stop
    // mask hid the beam: KWSX-init-auto)
    const int calibrant = d->comboBoxACmethod->currentIndex();
    if (DanAuto(d).openMask() && (calibrant == 1 || calibrant == 4) && !d->checkBoxACDBuseActive->isChecked())
    {
        d->checkBoxACDBuseActive->setChecked(true);
        report(R"(direct beam: "Use Current Mask and Sensitivity" checked (mask and sens without beam stop))");
    }
    if (autoConfigurations().isEmpty())
        return stop(lastError());
    if (!calculate())
        return stop(lastError());
    for (int i = 1; i <= configurations(); i++)
    {
        const QVariantMap c = configuration(i);
        report(QString("  %1: factor %2, center %3, %4")
                   .arg(i)
                   .arg(c["factor"].toDouble(), 0, 'g', 6)
                   .arg(c["xCenter"].toDouble(), 0, 'f', 2)
                   .arg(c["yCenter"].toDouble(), 0, 'f', 2));
    }
    const QStringList samples = d->danRawdata->sampleRuns({});
    if (samples.isEmpty())
        return stop("no sample runs (every run is a standard run by the auto options)");
    if (newScriptTable(scriptTable, samples).isEmpty())
        return stop(lastError());
    report(QString("script table %1: %2 sample runs").arg(scriptTable).arg(samples.count()));
    if (!calculateTransmissions(scriptTable))
        return stop(lastError());
    // the samples' centers onto their empty beams' (HShift, VShift), through "mask" and "sens", when the option
    // asks for it
    if (d->checkBoxAutoCenterOffsets->isChecked() && addCenterOffsets(scriptTable, "mask", "sens") < 0)
        return stop(lastError());
    // an info table of all runs the run filters select ("<script table>-info"), when the option asks for it
    if (d->checkBoxAutoInfoTable->isChecked())
    {
        QString info = scriptTable + "-info";
        if (d->app()->checkTableExistence(info))
            info = d->app()->generateUniqueName(info + "-");
        if (autoInfoTable(info).isEmpty())
            return stop("info table: " + lastError());
    }
    // the "Scale" column (normalized to the transmission at the longest distance) only when the option asks for it
    if (d->checkBoxAutoScale->isChecked() && autoScale(scriptTable) < 0)
        return stop(lastError());
    if (!d->checkBoxAutoScale->isChecked())
        report("no Scale column (auto option scale off)");
    // radial averaging (I[Q] into the project) and merging, when the options ask for it
    if (d->checkBoxAutoProcessIQ->isChecked())
    {
        const QStringList curves = process(scriptTable, "I-Q", "project");
        if (curves.isEmpty())
            return stop("I[Q]: " + lastError());
        report(QString("I[Q]: %1 tables").arg(curves.count()));
        if (d->checkBoxAutoMerge->isChecked())
        {
            const QStringList merged = d->danMerge->join(scriptTable + "-mergingTemplate", 30, 0, 0, "project", false);
            if (merged.isEmpty())
                return stop("merging: " + d->danMerge->lastError());
            report(QString("merged: %1").arg(merged.join(", ")));
        }
        report(QString("ready: %1, I[Q] in the project").arg(scriptTable));
    }
    else
        report(QString(R"(ready: %1 (processing: process("%1"); merging: Dan.Merge.join("%1-mergingTemplate")))")
                   .arg(scriptTable));
    end(true);
    return scriptTable;
}

//+++ the instrument's configuration (Dan.Options.Config.parameters())
namespace
{
// "[Key] value" lines -> key -> value (a key written twice: the last)
QVariantMap instrumentMap(const QString &text)
{
    QVariantMap values;
    for (const QString &line : text.split('\n'))
    {
        if (!line.startsWith('['))
            continue;
        const auto close = line.indexOf("]");
        if (close < 0)
            continue;
        values[line.mid(1, close - 1)] = line.mid(close + 1).trimmed();
    }
    return values;
}

QString parameterText(const QVariant &value)
{
    if (value.userType() == QMetaType::Bool)
        return value.toBool() ? "Yes" : "No";
    return value.toString();
}
} // namespace

QVariantMap DanOptionsConfig::parameters()
{
    return instrumentMap(d->instrumentText(d->comboBoxInstrument->currentText()));
}

bool DanOptionsConfig::setParameters(const QVariantMap &values)
{
    const QString name = d->comboBoxInstrument->currentText();
    QStringList lines = d->instrumentText(name).split('\n');
    const QVariantMap now = instrumentMap(lines.join('\n'));
    for (auto it = values.begin(); it != values.end(); ++it)
        if (!now.contains(it.key()))
        {
            fail("no parameter " + it.key() + " (parameters() has the names)");
            return false;
        }
    for (QString &line : lines)
        for (auto it = values.begin(); it != values.end(); ++it)
            if (line.startsWith("[" + it.key() + "]"))
                line = "[" + it.key() + "] " + parameterText(it.value());
    begin();
    d->applyInstrumentLines(lines);
    const QVariantMap after = parameters();
    QStringList refused;
    for (auto it = values.begin(); it != values.end(); ++it)
        if (after.value(it.key()).toString().simplified() != parameterText(it.value()).simplified())
            refused << it.key() + " (now " + after.value(it.key()).toString() + ")";
    return end(refused.isEmpty(), "not taken: " + refused.join(", "));
}

bool DanOptions::saveInstrument(const QString &name)
{
    if (name.trimmed().isEmpty() || name.contains('/'))
    {
        m_lastError = "no instrument name";
        return false;
    }
    if (InstrumentSANS::defaultInstruments.contains(name))
    {
        m_lastError = name + " is a built-in instrument: save it under a name of its own";
        return false;
    }
    const QString folder = QDir(d->app()->sasPath).filePath("SANSinstruments");
    QDir().mkpath(folder);
    begin();
    d->saveInstrumentAsCpp(folder, name);
    QFile f(QDir(folder).filePath(name + ".SANS"));
    if (!f.open(QIODevice::WriteOnly))
        return end(false, "could not write " + f.fileName());
    QTextStream(&f) << d->instrumentText(name);
    f.close();
    d->findSANSinstruments();
    if (d->comboBoxInstrument->findText(name) >= 0)
        d->comboBoxInstrument->setCurrentIndex(d->comboBoxInstrument->findText(name));
    d->instrumentSelected();
    return end(d->comboBoxInstrument->currentText() == name, "the instrument " + name + " was not found again");
}
