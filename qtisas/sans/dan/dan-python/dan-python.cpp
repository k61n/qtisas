/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: DAN-SANS Python interface: one object per tab (Dan.Options, Dan.Rawdata, Dan.Mask, Dan.Sensitivity,
             Dan.Processing, Dan.Merge)
 ******************************************************************************/

#include <cmath>

#include <QDirIterator>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QSet>
#include <QSignalBlocker>
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
    QStringList files;
    QDirIterator it(d->filesManager->pathInString(), QStringList() << d->filesManager->wildCardInString(), QDir::Files,
                    d->filesManager->subFoldersYN() ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
    while (it.hasNext())
        files << it.next();
    QStringList numbers;
    QSet<QString> seen;
    for (const QString &file : files)
    {
        const QString number =
            FilesManager::findFileNumberInFileName(d->filesManager->wildCardHeader(), QFileInfo(file).fileName());
        if (!number.isEmpty() && !seen.contains(number))
        {
            seen.insert(number);
            numbers << number;
        }
    }
    NaturalSort::sort(numbers);
    return numbers;
}

QStringList DanRawdata::headerItems() const
{
    QStringList names;
    for (int i = 0; i < d->comboBoxInfoExtractorCol->count(); i++)
        names << d->comboBoxInfoExtractorCol->itemText(i);
    return names;
}

QString DanRawdata::newHeaderTable(const QString &name, const QStringList &items, const QString &runs)
{
    const QStringList all = headerItems();
    QStringList indices;
    for (const QString &item : items)
    {
        const auto index = all.indexOf(item);
        if (index < 0)
            return fail("no header item " + item + "; headerItems(): " + all.join(", "));
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

QStringList DanRawdata::addRunsFromTable(const QString &table, const QString &by)
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
    const auto before = folderState(folders);
    if (by == "columns")
        d->addFilesInTableCols(t);
    else
        d->addFilesInTableRows(t);
    QStringList files = written(before, folderState(folders));
    if (!end(!files.isEmpty(), "nothing was written: no run of " + table + " found"))
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
        return runs.isEmpty() ? fail("no runs") : QString();
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
        return runs.isEmpty() ? fail("no runs") : QString();

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
        fail("no runs");
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
                                       int errorMatrix, double maskedValue)
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
    // A configuration calibrated with the direct beam (no calibrant: KWS-3) gets its absolute factor and its center
    // through the full mask "mask": its beam-stop mask hides the beam both are calculated from. Its own mask is set
    // back for I[Q] and the transmissions.
    QStringList ownMasks;
    const bool fullMask = isMask("mask");
    for (int col = 0; col < configurations(); col++)
    {
        auto *box = qobject_cast<QComboBox *>(d->tableEC->cellWidget(dan18::dptMASK, col));
        ownMasks << (box ? box->currentText() : QString());
        const bool directBeam =
            !d->tableEC->item(dan18::dptACFS, col) || d->tableEC->item(dan18::dptACFS, col)->text().trimmed().isEmpty();
        if (box && fullMask && directBeam)
            box->setCurrentIndex(box->findText("mask", Qt::MatchExactly));
    }
    for (int row : {dan18::dptC, dan18::dptD, dan18::dptWL, dan18::dptBSIZE, dan18::dptDAC, dan18::dptACMU,
                    dan18::dptACTR, dan18::dptACFAC, dan18::dptCENTERX, dan18::dptECTR})
        d->vertHeaderTableECPressed(row, true);
    for (int col = 0; col < configurations() && col < ownMasks.count(); col++)
        if (auto *box = qobject_cast<QComboBox *>(d->tableEC->cellWidget(dan18::dptMASK, col)))
            box->setCurrentIndex(box->findText(ownMasks[col], Qt::MatchExactly));
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
        return fail("no runs");
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
