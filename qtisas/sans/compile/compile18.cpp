/***************************************************************************
 File                   : compile18.cpp
 Project                : QtiSAS
 --------------------------------------------------------------------
 Copyright              : (C) 2017-2021 by Vitaliy Pipich
 Email (use @ for *)    : v.pipich*gmail.com
 Description            : compile interface: cpp file
 
 ***************************************************************************/

/***************************************************************************
 * *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or  *
 *  (at your option) any later version.*
 * *
 *  This program is distributed in the hope that it will be useful,*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  *
 *  GNU General Public License for more details.   *
 * *
 *   You should have received a copy of the GNU General Public License *
 *   along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,*
 *   Boston, MA  02110-1301  USA   *
 * *
 ***************************************************************************/

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
