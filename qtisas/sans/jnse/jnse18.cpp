/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: JNSE data reading interface
 ******************************************************************************/

#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>

#include "Folder.h"
#include "fittable18.h"
#include "globals.h"
#include "jnse18.h"
#include "settings.h"

jnse18::jnse18(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
    readSettings();
    connectSlot();
}
//+++ connectSlot()
void jnse18::connectSlot()
{
    connect(pushButtonDATpath, &QPushButton::clicked, this, &jnse18::buttomRADpath);
    connect(pushButtonReadNseData, &QPushButton::clicked, this, &jnse18::readNseData);
    connect(comboBoxHeaderTables, qOverload<int>(&QComboBox::activated), this, &jnse18::headerTableSeleted);
    connect(pushButtonStartNSEfit, &QPushButton::clicked, this, &jnse18::startNseFit);
    connect(jnseTab, &QTabWidget::currentChanged, this, &jnse18::findHeaderTables);
    // Multi-Fitting opened with no functions listed yet: apply the filter
    connect(jnseTab, &QTabWidget::currentChanged, this, [this](int index) {
        if (jnseTab->widget(index) == TabPageFitting && comboBoxFitFunctions->count() == 0)
            applyFunctionsFilter();
    });
    connect(lineEditFunctionsFilter, &QLineEdit::returnPressed, this, &jnse18::applyFunctionsFilter);
}
//+++ applyFunctionsFilter: the functions of FitTable's selected group that match the filter
void jnse18::applyFunctionsFilter()
{
    // No group selected in FitTable yet: select ALL there, which lists its functions here too
    if (app()->fittableWidget->currentFolder().isEmpty())
        app()->fittableWidget->selectFolder("ALL");
    QString currentFolder = app()->fittableWidget->currentFolder();
    if (!currentFolder.isEmpty())
    {
        QStringList functions = app()->fittableWidget->groupFunctions(currentFolder, false);
        filterFitFunctions(functions, false);
    }
}
///+++ destructor
jnse18::~jnse18()
{
    writeSettings();
}
//+++ app
ApplicationWindow *jnse18::app()
{
    if (auto dock = qobject_cast<QDockWidget *>(parentWidget()))
        if (auto mainWin = qobject_cast<ApplicationWindow *>(dock->parentWidget()))
            return mainWin;

    return qobject_cast<ApplicationWindow *>(parentWidget());
}
//+++ readSettings
void jnse18::readSettings()
{
    auto *settings = Settings::DefaultSettings();

    settings->beginGroup("/JNSE");

    bool ok = settings->contains("/lineEditPathDAT");
    QString ss = settings->value("/lineEditPathDAT", 0).toString();
    if (ok && ss != "home")
        lineEditPathDAT->setText(ss);

    ok = settings->contains("/lineEditFileName");
    ss = settings->value("/lineEditFileName", 0).toString();
    if (ok && ss != "info")
        lineEditFileName->setText(ss);

    settings->endGroup();

    delete settings;
}
//+++ readSettings
void jnse18::writeSettings()
{
    QSettings *settings = Settings::DefaultSettings();

    settings->beginGroup("/JNSE");

    if (lineEditPathDAT->text() != "home")
        settings->setValue("/lineEditPathDAT", lineEditPathDAT->text());

    if (lineEditFileName->text() != "info")
        settings->setValue("/lineEditFileName", lineEditFileName->text());

    settings->endGroup();

    delete settings;
}
//+++ buttomRADpath
void jnse18::buttomRADpath()
{
    QString path = lineEditPathDAT->text();
    if (path == "home")
        path = QDir::home().path();

    QString s = QFileDialog::getExistingDirectory(this, "get data directory - Choose a directory", path);
    if (!s.isEmpty())
        lineEditPathDAT->setText(s);
}
namespace
{
// The "key value" lines of a .dtr dataset header, in file order, and the header-table column of each value
// (columns 4..18: after sample-code, sample-info, mode and run-number)
struct DtrHeaderKey
{
    const char *key;
    const char *column;
};
const DtrHeaderKey dtrHeaderKeys[] = {{"q", "q"},
                                      {"q_var", "dq"},
                                      {"q_unit_SI", "q-unit"},
                                      {"t_unit_SI", "t-unit"},
                                      {"temp", "temp"},
                                      {"numor1", "numor"},
                                      {"ref1", "ref"},
                                      {"bgr1", "bgr"},
                                      {"withbgr", "with-bgr"},
                                      {"tfac_bgr", "tfac-bgr"},
                                      {"volfrac", "volfrac"},
                                      {"qbins", "qbins"},
                                      {"tbins", "tbins"},
                                      {"oor_cnt", "oor-cnt"},
                                      {"upd_cnt", "upd-cnt"}};
const int dtrHeaderCount = sizeof(dtrHeaderKeys) / sizeof(dtrHeaderKeys[0]);
const int dtrFirstHeaderColumn = 4;
const int dtrDataColumns = 7; // tau, S(q,t)/S(q), its error and variance, var(tau), nupdates, Qeff

// One dataset of a .dtr file (numbers are C)
struct DtrDataset
{
    QString sample, info, mode, run;
    QStringList header;        // values of dtrHeaderKeys
    QList<QList<double>> rows; // dtrDataColumns values per row
};

bool isDtrMarker(const QString &line)
{
    return line.startsWith("#nxt") || line.startsWith("#eod");
}

// The next line that is neither empty nor a "#nxt"/"#eod" marker; empty at the end of the file
QString nextDtrLine(const QStringList &data, int &lineNum)
{
    while (lineNum < data.count())
    {
        QString s = data[lineNum++].simplified();
        if (!s.isEmpty() && !isDtrMarker(s))
            return s;
    }
    return {};
}

// The 7 numbers of a data line, or false for any other line (column titles, a cut line)
bool dtrDataRow(const QString &line, QList<double> &row)
{
    const QStringList values = line.split(' ', Qt::SkipEmptyParts);
    if (values.count() < dtrDataColumns)
        return false;
    row.clear();
    for (int i = 0; i < dtrDataColumns; i++)
    {
        bool ok = false;
        row << QLocale::c().toDouble(values[i], &ok);
        if (!ok)
            return false;
    }
    return true;
}

enum class DtrRead
{
    Dataset,
    End,
    Error
};

// Reads the dataset that starts at lineNum; on Error, lineNum is the line that does not fit the format
DtrRead readDtrDataset(const QStringList &data, int &lineNum, DtrDataset &d)
{
    // sample name and info
    const QString first = nextDtrLine(data, lineNum);
    if (first.isEmpty())
        return DtrRead::End;
    const int space = static_cast<int>(first.indexOf(' '));
    d.sample = first.left(space);
    d.info = space < 0 ? QString() : first.mid(space + 1).remove('"').trimmed();

    // "<name> <mode> vs t/ns <run>"
    const QString second = nextDtrLine(data, lineNum);
    if (!second.contains("vs t/ns"))
        return DtrRead::Error;
    const QStringList words = second.split(' ', Qt::SkipEmptyParts);
    d.mode = words.value(1);
    d.run = words.last();

    if (!nextDtrLine(data, lineNum).contains("parameter"))
        return DtrRead::Error;

    d.header.clear();
    for (const DtrHeaderKey &key : dtrHeaderKeys)
    {
        const QString line = nextDtrLine(data, lineNum);
        if (line.section(' ', 0, 0) != key.key)
            return DtrRead::Error;
        d.header << line.section(' ', 1).trimmed();
    }

    if (!nextDtrLine(data, lineNum).contains("values"))
        return DtrRead::Error;

    // data lines up to an empty line, a marker or the end of the file; other lines (column titles) are skipped
    d.rows.clear();
    QList<double> row;
    while (lineNum < data.count())
    {
        const QString line = data[lineNum].simplified();
        if (line.isEmpty() || isDtrMarker(line))
            break;
        lineNum++;
        if (dtrDataRow(line, row))
            d.rows << row;
    }
    return DtrRead::Dataset;
}
} // namespace

//+++ a warning: a message box in the GUI, collected for the caller in a scripted run (no modal dialog)
void jnse18::warn(const QString &text)
{
    if (m_scripted)
        m_messages << text;
    else
        QMessageBox::warning(this, "QtiSAS - JNSE", text);
}

//+++ a JNSE header table: the table made by importDtr() (its 4th column "run-number", 20 columns)
Table *jnse18::headerTable(const QString &name)
{
    Table *t = nullptr;
    if (!app()->checkTableExistence(name, t) || t->numCols() < 20 || t->colLabel(3) != "run-number")
        return nullptr;
    return t;
}

//+++ read of NSE(*.dtr) files and make table(s): the header table's name, or empty with m_lastError when nothing was
//+++ read. A header table of that name is appended to (no name: "<first file read>-info"); problems of single files
//+++ are warnings (warn()).
QString jnse18::importDtr(const QStringList &files, const QString &headerName)
{
    m_lastError.clear();

    Table *tableDat = nullptr;
    if (!headerName.isEmpty() && app()->checkTableExistence(headerName))
    {
        tableDat = headerTable(headerName);
        if (!tableDat)
        {
            m_lastError =
                tr("%1 is a table but not a JNSE header table; give another name for the header table").arg(headerName);
            return {};
        }
    }

    // every file read first: the tables are made only for datasets that were read
    struct FileDatasets
    {
        QString code;
        QList<DtrDataset> datasets;
    };
    QList<FileDatasets> read;
    for (const QString &file : files)
    {
        QFile f(file);
        if (!f.open(QIODevice::ReadOnly))
        {
            warn(tr("%1 cannot be opened, and is skipped").arg(file));
            continue;
        }
        QString content = QString::fromUtf8(f.readAll());
        f.close();
        content.replace("\r\n", "\n").replace("\r", "\n");
        const QStringList data = content.split("\n", Qt::KeepEmptyParts);

        FileDatasets fd;
        fd.code = QFileInfo(file).fileName();
        fd.code.remove(".dtr").remove(".DTR");

        int lineNum = 0;
        DtrDataset d;
        DtrRead result;
        while ((result = readDtrDataset(data, lineNum, d)) == DtrRead::Dataset)
            fd.datasets << d;
        // the rest of a file that does not fit the format is not read: its datasets would be misaligned
        if (result == DtrRead::Error)
            warn(
                tr("Check Format of File: %1; line # %2 does not fit the .dtr format (the datasets before it are read)")
                    .arg(file)
                    .arg(lineNum));
        if (fd.datasets.isEmpty())
            warn(tr("%1 holds no .dtr dataset, and is skipped").arg(file));
        else
            read << fd;
    }
    if (read.isEmpty())
    {
        m_lastError = tr("no .dtr dataset read from %1").arg(files.join(", "));
        return {};
    }
    // no name given: the first file that was read names the header table ("report19971_1-2-info")
    QString name = headerName;
    if (name.isEmpty())
    {
        name = read.first().code + "-info";
        name.replace(QRegularExpression("[.,;:]"), "-");
        tableDat = headerTable(name);
        if (!tableDat && app()->checkTableExistence(name))
        {
            m_lastError = tr("%1 is a table but not a JNSE header table; give a name for the header table").arg(name);
            return {};
        }
    }

    // the tables go into this folder
    Folder *before = app()->currentFolder();
    app()->changeFolder("JNSE :: *.dtr files");
    if (!m_scripted)
        app()->folders->setFocus();

    if (tableDat == nullptr)
        tableDat = app()->newTable(name, 0, 20);

    //+++ Cols Names
    const QStringList textColumns = {"sample-code", "sample-info", "mode", "run-number"};
    for (int col = 0; col < textColumns.count(); col++)
        tableDat->setColName(col, textColumns[col]);
    for (int k = 0; k < dtrHeaderCount; k++)
        tableDat->setColName(dtrFirstHeaderColumn + k, dtrHeaderKeys[k].column);
    tableDat->setColName(19, "table-name");

    QStringList typeCols;
    for (int col = 0; col < 20; col++)
    {
        tableDat->setColPlotDesignation(col, col == 4 ? Table::X : (col == 5 ? Table::xErr : Table::None));
        typeCols << (col < 4 || col == 19 ? "1" : "0");
    }
    tableDat->setColumnTypes(typeCols);
    tableDat->show();

    const QStringList datasetColumns = {"tau-ns",     "Sqt-To-Sq", "d-Sqt-To-Sq", "var-Sqt-To-Sq",
                                        "var-tau-ns", "nupdates",  "Qeff"};
    const Table::PlotDesignation datasetDesignations[] = {Table::X,    Table::Y, Table::yErr, Table::yErr,
                                                          Table::xErr, Table::Y, Table::Y};

    for (const FileDatasets &fd : read)
        for (const DtrDataset &d : fd.datasets)
        {
            const int row = tableDat->numRows();
            tableDat->setNumRows(row + 1);

            tableDat->setText(row, 0, d.sample + "  " + fd.code);
            tableDat->setText(row, 1, d.info);
            tableDat->setText(row, 2, d.mode);
            tableDat->setText(row, 3, d.run);
            for (int k = 0; k < dtrHeaderCount; k++) // .dtr numbers are C
                tableDat->setCellOrText(row, dtrFirstHeaderColumn + k, d.header[k], QLocale::c());

            QString matrixName = fd.code + "-" + d.run + "-v-";
            matrixName.replace(QRegularExpression("[.,;:]"), "-");
            matrixName = app()->generateUniqueName(matrixName);
            tableDat->setText(row, 19, matrixName);

            // the dataset table, created hidden
            Table *tableResults =
                app()->newHiddenTable(matrixName, "NSE Dataset", static_cast<int>(d.rows.count()), dtrDataColumns);
            for (int col = 0; col < dtrDataColumns; col++)
            {
                tableResults->setColName(col, datasetColumns[col]);
                tableResults->setColPlotDesignation(col, datasetDesignations[col]);
            }
            tableResults->setColComment(1, "Q=" + d.header[0] + "/A");
            for (int r = 0; r < d.rows.count(); r++)
                for (int col = 0; col < dtrDataColumns; col++)
                    tableResults->setCell(r, col, d.rows[r][col]);
            tableResults->adjustColumnsWidth(false);
            app()->setListViewLabel(tableResults->name(), "NSE Dataset");
        }
    tableDat->adjustColumnsWidth(false);

    tableDat->setWindowLabel("NSE Headers");
    app()->setListViewLabel(tableDat->name(), "NSE Headers");
    // maximized as the folder's active window, so that the next new window takes over the maximized state
    tableDat->setNormal(); // also when appending to an already maximized table
    app()->maximizeWindow(tableDat);

    findHeaderTables();
    // a script's current folder stays the one it was in
    if (m_scripted && before)
        app()->changeFolder(before, true);
    return tableDat->name();
}

//+++ read of NSE(*.dtr) files chosen in dialogs
void jnse18::readNseData()
{
    bool ok;
    QString TableName = QInputDialog::getText(this, "Table's Generation: all header info",
                                              "Enter name of Table:", QLineEdit::Normal, lineEditFileName->text(), &ok);
    if (!ok || TableName.isEmpty())
        return;

    QFileDialog fd(this, "NSE - File Import", lineEditPathDAT->text(), "NSE - dtr files (*.dtr)");
    fd.setFileMode(QFileDialog::ExistingFiles);
    fd.setNameFilters({QStringLiteral("NSE - dtr files (*.dtr)"), QStringLiteral("All Files (*)")});
    if (fd.exec() == QDialog::Rejected)
        return;

    lineEditFileName->setText(TableName);
    m_messages.clear();
    if (importDtr(fd.selectedFiles(), TableName).isEmpty())
        QMessageBox::critical(this, "QtiSAS - JNSE", m_lastError);
}
//+++ Search of  Header tables
void jnse18::findHeaderTables()
{
    QString activeTable = comboBoxHeaderTables->currentText();

    QStringList list;

    foreach (MdiSubWindow *w, app()->tableList())
    {
        auto *t = (Table *)w;
        if (t->colLabel(3) == "run-number")
            list << t->name();
    }

    comboBoxHeaderTables->clear();

    if (list.count() == 0)
        return;

    comboBoxHeaderTables->addItems(list);

    if (!activeTable.isEmpty() && list.contains(activeTable))
        if (int idx = comboBoxHeaderTables->findText(activeTable); idx >= 0)
            comboBoxHeaderTables->setCurrentIndex(idx);

    headerTableSeleted();
}
//+++ headerTableSeleted
void jnse18::headerTableSeleted()
{
    if (comboBoxHeaderTables->count() > 0)
    {
        foreach (MdiSubWindow *w, app()->tableList())
            if (w->name() == comboBoxHeaderTables->currentText())
            {
                auto *t = (Table *)w;
                spinBoxFrom->setMaximum(t->numRows());
                spinBoxTo->setValue(1);
                spinBoxTo->setMaximum(t->numRows());
                spinBoxTo->setValue(t->numRows());
                break;
            }
    }
    else
    {
        spinBoxFrom->setMaximum(1);
        spinBoxTo->setMaximum(1);
        findHeaderTables();
    }
    
}
//+++ startNseFit: the multi-fit of the selected rows, set up in FitTable
void jnse18::startNseFit()
{
    if (comboBoxHeaderTables->count() == 0)
    {
        QMessageBox::warning(this, "QtiSAS", "Select Header's Table first!");
        return;
    }
    m_messages.clear();
    if (initFit(comboBoxHeaderTables->currentText(), comboBoxFitFunctions->currentText(), spinBoxFrom->value(),
                spinBoxTo->value())
            .isEmpty())
        QMessageBox::critical(this, "QtiSAS - JNSE", m_lastError);
}
//+++ filterFitFunctions
void jnse18::filterFitFunctions(const QStringList &inputList, bool local)
{
    QStringList lst;

    if (local)
    {
        const int count = comboBoxFitFunctions->count();
        if (count == 0)
            return;

        lst.reserve(count);
        for (int i = 0; i < count; ++i)
            lst << comboBoxFitFunctions->itemText(i);
    }
    else
    {
        if (inputList.isEmpty())
            return;

        lst = inputList;
    }

    const QString pattern = lineEditFunctionsFilter->text();
    const QRegularExpression rx = REGEXPS::wildcardToRegex(pattern);

    if (!rx.isValid())
        return;

    // Sorted as in FitTable: functions of the library's top level first, then those in subfolders
    QStringList root, subfolders;
    for (const QString &function : lst.filter(rx))
        (function.contains('/') ? subfolders : root) << function;
    root.sort();
    subfolders.sort();

    comboBoxFitFunctions->clear();
    comboBoxFitFunctions->addItems(root + subfolders);
}
