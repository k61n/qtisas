/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Compile interface
 ******************************************************************************/

#include "compile18.h"
#include <QDockWidget>
#include <QUrl>
#include "Folder.h"

compile18::compile18(QWidget *parent)
: QWidget(parent)
{
    setupUi(this);
    initCompile();
    connectSlot();    
}

compile18::~compile18()
{
}
//*******************************************
//+++ setPathExtern
//*******************************************
void compile18::setPathExtern(QString path)
{
    pathFIF=path;
    fitPath->setText(path);
}
//*******************************************
//+++ app
//*******************************************
ApplicationWindow* compile18::app()
{
    QDockWidget *docWin=(QDockWidget*)this->parentWidget();
    return (ApplicationWindow *)docWin->parent();
}

//*******************************************
//*** Log-output
//*******************************************
void compile18::toResLog(QString text)
{
    if (app()->dockWidgetArea(app()->logWindow) == Qt::RightDockWidgetArea && !text.contains("<< compile status >>  ERROR:") )
    {
        app()->current_folder->appendLogInfo(text);
        app()->results->setText(app()->current_folder->logInfo());
    }
    else
    {
        app()->showResults(text, true);
        
        app()->showResults(false);
        app()->showResults(true);
    }
}
