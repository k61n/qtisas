/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: JNSE data reading interface
 ******************************************************************************/

#ifndef JNSE18_H
#define JNSE18_H

#include "ApplicationWindow.h"
#include "ui_jnse.h"

class jnse18 : public QWidget, private Ui::jnse
{
    Q_OBJECT

  public:
    explicit jnse18(QWidget *parent = nullptr);
    ~jnse18() override;

    ApplicationWindow *app();

    void connectSlot();
    void readSettings();
    void writeSettings();

  public slots:
    void findHeaderTables();
    void buttomRADpath();
    void readNseData();
    void startNseFit();
    void headerTableSeleted();
    void filterFitFunctions(const QStringList &lst = {}, bool local = true);
    void applyFunctionsFilter();

  public:
    // Python interface (jnse-python.cpp)
    void reset();
    bool setInputFolder(const QString &folder);
    QString inputFolder() const;
    QString importFiles(const QStringList &files, const QString &headerTable);
    QStringList datasets(const QString &headerTable);
    QStringList headerColumn(const QString &headerTable, const QString &column, bool *numeric);
    QStringList initFit(const QString &headerTable, const QString &function, int fromRow, int toRow);
    double fit(const QString &headerTable, const QString &function, int fromRow, int toRow, const QStringList &shared,
               const QMap<QString, double> &start, const QMap<QString, double> &fixed);
    QString lastError() const;
    QStringList lastMessages() const;

  private:
    QString importDtr(const QStringList &files, const QString &headerName);
    Table *headerTable(const QString &name);
    void warn(const QString &text);
    bool m_scripted = false; // a Python API run: warnings are collected, not shown
    QStringList m_messages;  // the warnings of the last call
    QString m_lastError;     // why the last call made nothing
    QString m_inputFolder;   // relative file names of importFiles() start here
};

#endif
