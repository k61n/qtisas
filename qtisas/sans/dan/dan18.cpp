/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS data analysis interface
 ******************************************************************************/

#include <QDockWidget>
#include <QUrl>

#include "dan18.h"

dan18::dan18(QWidget *parent)
: QWidget(parent)
{
    setupUi(this);

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
