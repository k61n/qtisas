/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: JNSE data reading interface
 ******************************************************************************/

#include <QDebug>
#include <QDockWidget>
#include <QUrl>

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
    connect(lineEditFunctionsFilter, &QLineEdit::returnPressed, this, [this]() {
        QString currentFolder = app()->fittableWidget->currentFolder();
        if (!currentFolder.isEmpty())
        {
            QStringList functions = app()->fittableWidget->groupFunctions(currentFolder, false);
            filterFitFunctions(functions, false);
        }
    });
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
QString nextNonEmptyLine(const QStringList &data, int &lineNum)
{
    while (lineNum < data.count())
    {
        QString s = data[lineNum++].trimmed().simplified();
        if (!s.isEmpty())
            return s;
    }
    return {};
}
//+++ read of NSE(*.dtr) files and make table(s)
void jnse18::readNseData()
{
    app()->changeFolder("JNSE :: *.dtr files");
    app()->folders->setFocus();

    bool ok;
    QString TableName = QInputDialog::getText(this, "Table's Generation: all header info",
                                              "Enter name of Table:", QLineEdit::Normal, lineEditFileName->text(), &ok);
    if (!ok || TableName.isEmpty())
        return;
    
    QString DatDir = lineEditPathDAT->text();
    auto *fd = new QFileDialog(this, "NSE - File Import", DatDir, "NSE - dtr files (*.dtr)");
    fd->setFileMode(QFileDialog::ExistingFiles );
    fd->setWindowTitle(tr("NSE - File Import"));
    fd->setNameFilters({QStringLiteral("NSE - dtr files (*.dtr)"), QStringLiteral("All Files (*)")});
    if (fd->exec() == QDialog::Rejected)
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Select first!");
        return;
    }
    QStringList selectedDat = fd->selectedFiles();
    int filesNumber = static_cast<int>(selectedDat.count());

    lineEditFileName->setText(TableName);

    Table *tableDat = nullptr;
    int startRaw = 0;

    foreach (MdiSubWindow *w, app()->tableList())
        if (w->name() == TableName)
        {
            tableDat = (Table *)w;
            if (tableDat->numCols() < 20)
            {
                QMessageBox::critical(nullptr, "QtiSAS", "Create new table (# cols)");
                return;
            }
            startRaw = tableDat->numRows();
            break;
        };

    if (tableDat == nullptr)
        tableDat = app()->newTable(TableName, 0, 20);

    QStringList typeCols;
    
    //+++ Cols Names
    tableDat->setColName(0, "sample-code");
    tableDat->setColPlotDesignation(0, Table::None);
    typeCols << "1";
    tableDat->setColName(1, "sample-info");
    tableDat->setColPlotDesignation(1, Table::None);
    typeCols << "1";
    tableDat->setColName(2, "mode");
    tableDat->setColPlotDesignation(2, Table::None);
    typeCols << "1";
    tableDat->setColName(3, "run-number");
    tableDat->setColPlotDesignation(3, Table::None);
    typeCols << "1";
    tableDat->setColName(4, "q");
    tableDat->setColPlotDesignation(4, Table::X);
    typeCols << "0";
    tableDat->setColName(5, "dq");
    tableDat->setColPlotDesignation(5, Table::xErr);
    typeCols << "0";
    tableDat->setColName(6, "q-unit");
    tableDat->setColPlotDesignation(6, Table::None);
    typeCols << "0";
    tableDat->setColName(7, "t-unit");
    tableDat->setColPlotDesignation(7, Table::None);
    typeCols << "0";
    tableDat->setColName(8, "temp");
    tableDat->setColPlotDesignation(8, Table::None);
    typeCols << "0";
    tableDat->setColName(9, "numor");
    tableDat->setColPlotDesignation(9, Table::None);
    typeCols << "0";
    tableDat->setColName(10, "ref");
    tableDat->setColPlotDesignation(10, Table::None);
    typeCols << "0";
    tableDat->setColName(11, "bgr");
    tableDat->setColPlotDesignation(11, Table::None);
    typeCols << "0";
    tableDat->setColName(12, "with-bgr");
    tableDat->setColPlotDesignation(12, Table::None);
    typeCols << "0";
    tableDat->setColName(13, "tfac-bgr");
    tableDat->setColPlotDesignation(13, Table::None);
    typeCols << "0";
    tableDat->setColName(14, "volfrac");
    tableDat->setColPlotDesignation(14, Table::None);
    typeCols << "0";
    tableDat->setColName(15, "qbins");
    tableDat->setColPlotDesignation(15, Table::None);
    typeCols << "0";
    tableDat->setColName(16, "tbins");
    tableDat->setColPlotDesignation(16, Table::None);
    typeCols << "0";
    tableDat->setColName(17, "oor-cnt");
    tableDat->setColPlotDesignation(17, Table::None);
    typeCols << "0";
    tableDat->setColName(18, "upd-cnt");
    tableDat->setColPlotDesignation(18, Table::None);
    typeCols << "0";
    tableDat->setColName(19, "table-name");
    tableDat->setColPlotDesignation(19, Table::None);
    typeCols << "1";

    tableDat->setColumnTypes(typeCols);
    tableDat->show();

    QString fnameOnly, nameMatrix, currentQ;
    QString s, ss;

    for (int iter = 0; iter < filesNumber; iter++)
    {
        QFile f(selectedDat[iter]);
        if (!f.open(QIODevice::ReadOnly))
            continue;

        QByteArray raw = f.readAll();

        QString content = QString::fromUtf8(raw);
        content = content.replace("\r\n", "\n").replace("\r", "\n");

        QStringList data = content.split("\n", Qt::KeepEmptyParts);

        QStringList lst = selectedDat[iter].split("/", Qt::SkipEmptyParts);
        QString fileCode = lst[lst.count() - 1];

        fileCode = fileCode.remove(".dtr").remove(".DTR");
        int lineNum = 0;

        do
        {
            tableDat->setNumRows(startRaw + 1);

            //+++ First line
            s = data[lineNum++].trimmed();
            lst = s.split(" ", Qt::SkipEmptyParts);
            //+++ First line: Table Name
            tableDat->setText(startRaw, 0, lst[0] + "  " + fileCode);
            //+++  First line: Date
            tableDat->setText(startRaw, 1, s.right(s.length() - s.indexOf(" ", 0)).trimmed().remove("\""));

            //+++ Second Line
            s = nextNonEmptyLine(data, lineNum);
            lst = s.split(" ", Qt::SkipEmptyParts);
            if (!s.contains("vs t/ns"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            //+++ Second Line: mode
            if (lst.count() > 0)
                tableDat->setText(startRaw, 2, lst[1]);
            //+++ Second Line: Run Number
            QString matrixName = lst[lst.count() - 1];
            tableDat->setText(startRaw, 3, matrixName);
            matrixName.prepend(fileCode + "-");
            matrixName.append("-v-");
            matrixName.replace(QRegularExpression("[.,;:]"), "-");
            matrixName = app()->generateUniqueName(matrixName);

            //+++ parameter
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("parameter"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            //+++ q
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("q"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            currentQ = s.remove("q").trimmed();
            tableDat->setText(startRaw, 4, currentQ);
            //+++  q_var dq
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("q_var"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 5, s.remove("q_var").trimmed());
            //+++  q_unit_SI
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("q_unit_SI"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 6, s.remove("q_unit_SI").trimmed());
            //+++  t_unit_SI
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("t_unit_SI"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 7, s.remove("t_unit_SI").trimmed());
            //+++  temp
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("temp"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 8, s.remove("temp").trimmed());
            //+++  numor1
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("numor1"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 9, s.remove("numor1").trimmed());
            //+++  ref
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("ref1"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 10, s.remove("ref1").trimmed());
            //+++  bgr1
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("bgr1"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 11, s.remove("bgr1").trimmed());
            //+++  withbgr
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("withbgr"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 12, s.remove("withbgr").trimmed());
            //+++  tfac_bgr
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("tfac_bgr"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 13, s.remove("tfac_bgr").trimmed());
            //+++  volfrac
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("volfrac"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 14, s.remove("volfrac").trimmed());
            //+++  qbins
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("qbins"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 15, s.remove("qbins").trimmed());
            //+++  tbins
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("tbins"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 16, s.remove("tbins").trimmed());
            //+++  oor_cnt
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("oor_cnt"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
            }
            tableDat->setText(startRaw, 17, s.remove("oor_cnt").trimmed());
            //+++  upd_cnt
            s = nextNonEmptyLine(data, lineNum);
            if (!s.contains("upd_cnt"))
            {
                QMessageBox::critical(nullptr, "QtiSAS",
                                      tr("Check Format of File: %1; line # %2")
                                          .arg(selectedDat[iter - startRaw])
                                          .arg(QString::number(lineNum)));
                break;
            }
            tableDat->setText(startRaw, 18, s.remove("upd_cnt").trimmed());
            //+++  table_name
            tableDat->setText(startRaw,19,matrixName);

            //+++  Read Table +++
            s = nextNonEmptyLine(data, lineNum);
            if (s.contains("values"))
                s = data[lineNum++].trimmed();

            Table *tableResults = app()->newTable(matrixName, 0, 7);

            //+++ Cols Names
            tableResults->setColName(0, "tau-ns");
            tableResults->setColPlotDesignation(0, Table::X);
            tableResults->setColName(1, "Sqt-To-Sq");
            tableResults->setColPlotDesignation(1, Table::Y);
            tableResults->setColComment(1, "Q=" + currentQ + "/A");
            tableResults->setColName(2, "d-Sqt-To-Sq");
            tableResults->setColPlotDesignation(2, Table::yErr);
            tableResults->setColName(3, "var-Sqt-To-Sq");
            tableResults->setColPlotDesignation(3, Table::yErr);
            tableResults->setColName(4, "var-tau-ns");
            tableResults->setColPlotDesignation(4, Table::xErr);
            tableResults->setColName(5, "nupdates");
            tableResults->setColPlotDesignation(5, Table::Y);
            tableResults->setColName(6, "Qeff");
            tableResults->setColPlotDesignation(6, Table::Y);

            //+++
            int Ncur = 0;
            s = data[lineNum++].trimmed();
            s.replace(',', '.');

            while (!s.isEmpty() && !s.contains("#eod") && !s.contains("#nxt") && lineNum < data.count())
            {
                s = s.trimmed().simplified();
                lst = s.split(" ", Qt::SkipEmptyParts);
                
                if (lst.count() < 7)
                    continue;

                tableResults->setNumRows(Ncur + 1);
                for (int i = 0; i < 7; i++)
                    tableResults->setText(Ncur, i, QString::number(lst[i].toDouble(), 'E'));
                Ncur++;

                if (lineNum >= data.count())
                    break;

                s = data[lineNum++].trimmed();
                s.replace(',', '.');
            }
            tableResults->adjustColumnsWidth(false);

            // +++ Hide all files
            tableResults->setWindowLabel("NSE Dataset");
            app()->setListViewLabel(tableResults->name(), "NSE Dataset");
            tableResults->setHidden();

            s = nextNonEmptyLine(data, lineNum);

            startRaw++;
        } while (lineNum + 3 < data.count());

        f.close();
    }
    tableDat->adjustColumnsWidth(false);

    tableDat->setWindowLabel("NSE Headers");
    app()->setListViewLabel(tableDat->name(), "NSE Headers");
    app()->updateWindowLists(tableDat);

    tableDat->showMaximized();

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

    const QStringList filtered = lst.filter(rx);

    comboBoxFitFunctions->clear();
    comboBoxFitFunctions->addItems(filtered);
}
