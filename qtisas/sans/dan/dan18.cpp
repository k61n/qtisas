/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS data analysis interface
 ******************************************************************************/

#include <QDockWidget>
#include <QTextDocument>
#include <QUrl>

#include "dan18.h"

dan18::dan18(QWidget *parent)
: QWidget(parent)
{
    setupUi(this);
    danOptions = new DanOptions(this);
    danRawdata = new DanRawdata(this);
    danMask = new DanMask(this);
    danOptionsConfig = new DanOptionsConfig(this);
    danSensitivity = new DanSensitivity(this);
    danProcessingOptions = new DanProcessingOptions(this);
    danProcessing = new DanProcessing(this);
    danMerge = new DanMerge(this);
    danOptions->config = danOptionsConfig;
    danProcessing->options = danProcessingOptions;
    // DAN's own fields, tables and spin boxes stay C like its instrument and configuration files; only the QtiSAS
    // tables and matrices it writes follow the application's numerical format
    setLocale(QLocale::c());

#ifdef Q_OS_MACOS
    this->setMinimumWidth(600);
#endif
    initDAN();
    
    //+++ connections
    connectSlot();
    optionsConnectSlot();
    rawdataConnectSlot();
    maskConnectSlot();
    sensConnectSlot();
    processdataConnectSlots();
    dandanConnectSlots();
    rtConnectSlots();
    tofConnectSlots();
    addfilesConnectSlots();
    extractorConnectSlots();
    mergeSlots();
    polarizedConnectSlots();
}

dan18::~dan18()
{
    writeSettings();
    delete danOptions;
    delete danRawdata;
    delete danMask;
    delete danOptionsConfig;
    delete danSensitivity;
    delete danProcessingOptions;
    delete danProcessing;
    delete danMerge;
}

// *******************************************
// +++ app
// *******************************************
ApplicationWindow *dan18::app()
{
    if (this->parentWidget()->parent())
        return (ApplicationWindow *)((QDockWidget *)this->parentWidget())->parent();
    return (ApplicationWindow *)this->parent();
}

// *******************************************
// *** Log-output
// *******************************************
void dan18::danMessage(const QString &text, bool critical)
{
    if (!m_scriptRun)
    {
        if (critical)
            QMessageBox::critical(this, "QtiSAS", text);
        else
            QMessageBox::warning(this, "QtiSAS", text);
        return;
    }
    QTextDocument plain;
    plain.setHtml(text);
    const QString line = plain.toPlainText().simplified();
    m_scriptMessages << line;
    toResLog("DAN :: " + line + "\n");
}

void dan18::beginScriptRun()
{
    m_scriptRun = true;
    m_scriptMessages.clear();
}

QStringList dan18::endScriptRun()
{
    m_scriptRun = false;
    return m_scriptMessages;
}

void dan18::toResLog(QString text)
{
    QString info =text;
    if (app()->dockWidgetArea(app()->logWindow) == Qt::RightDockWidgetArea )
    {
        app()->current_folder->appendLogInfo(text);
        app()->results->setText(app()->current_folder->logInfo());
    }
    else app()->showResults(text, true);
}

//+++ progress windows: shown also while a script runs DAN (before: hidden, a long reduction looked frozen)
void dan18::startProgress(QProgressDialog &progress)
{
    if (!app()->isVisible())
    {
        progress.setMinimumDuration(std::numeric_limits<int>::max());
        return;
    }
    // a script's run: not modal, so that nothing else (a click, the script server) runs in between
    if (m_scriptRun)
        progress.setWindowModality(Qt::NonModal);
    progress.setMinimumDuration(0);
    progress.show();
    progress.raise();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
}

void dan18::stepProgress(QProgressDialog &progress, int value)
{
    // a modal one draws itself in setValue(); a non-modal one only when the events are processed
    progress.setValue(value);
    if (progress.isVisible() && progress.windowModality() == Qt::NonModal)
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
}
