/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: JNSE data reading interface
 ******************************************************************************/

#include <QDockWidget>

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

//+++ read of NSE(*.dtr) files and make table(s)
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
    const QStringList selectedDat = fd.selectedFiles();

    lineEditFileName->setText(TableName);

    // the tables go into this folder, created only once files are chosen
    app()->changeFolder("JNSE :: *.dtr files");
    app()->folders->setFocus();

    Table *tableDat = nullptr;
    foreach (MdiSubWindow *w, app()->tableList())
        if (w->name() == TableName)
        {
            tableDat = (Table *)w;
            if (tableDat->numCols() < 20)
            {
                QMessageBox::critical(nullptr, "QtiSAS", "Create new table (# cols)");
                return;
            }
            break;
        };

    if (tableDat == nullptr)
        tableDat = app()->newTable(TableName, 0, 20);

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

    for (const QString &file : selectedDat)
    {
        QFile f(file);
        if (!f.open(QIODevice::ReadOnly))
            continue;

        QString content = QString::fromUtf8(f.readAll());
        f.close();
        content.replace("\r\n", "\n").replace("\r", "\n");
        const QStringList data = content.split("\n", Qt::KeepEmptyParts);

        QString fileCode = QFileInfo(file).fileName();
        fileCode.remove(".dtr").remove(".DTR");

        int lineNum = 0;
        DtrDataset d;
        DtrRead read;
        while ((read = readDtrDataset(data, lineNum, d)) == DtrRead::Dataset)
        {
            const int row = tableDat->numRows();
            tableDat->setNumRows(row + 1);

            tableDat->setText(row, 0, d.sample + "  " + fileCode);
            tableDat->setText(row, 1, d.info);
            tableDat->setText(row, 2, d.mode);
            tableDat->setText(row, 3, d.run);
            for (int k = 0; k < dtrHeaderCount; k++) // .dtr numbers are C
                tableDat->setCellOrText(row, dtrFirstHeaderColumn + k, d.header[k], QLocale::c());

            QString matrixName = fileCode + "-" + d.run + "-v-";
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

        // the rest of a file that does not fit the format is not read: its datasets would be misaligned
        if (read == DtrRead::Error)
            QMessageBox::critical(nullptr, "QtiSAS", tr("Check Format of File: %1; line # %2").arg(file).arg(lineNum));
    }
    tableDat->adjustColumnsWidth(false);

    tableDat->setWindowLabel("NSE Headers");
    app()->setListViewLabel(tableDat->name(), "NSE Headers");
    // maximized as the folder's active window, so that the next new window takes over the maximized state
    tableDat->setNormal(); // also when appending to an already maximized table
    app()->maximizeWindow(tableDat);

    findHeaderTables();
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
//+++ startNseFit
void jnse18::startNseFit()
{
    if (comboBoxHeaderTables->count() > 0)
        emit signalJnseFit(comboBoxHeaderTables->currentText(), comboBoxFitFunctions->currentText(),
                           spinBoxFrom->value(), spinBoxTo->value());
    else
        QMessageBox::warning(this, "QtiSAS", "Select Header's Table first!");
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
