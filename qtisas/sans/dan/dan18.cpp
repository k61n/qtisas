/***************************************************************************
 File : dan18.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : SANS Data Analisys Interface
 
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

#include "dan18.h"
#include <QDockWidget>
#include <QUrl>
#include "Folder.h"

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

    //+++ Load list Of Calibrators
    findCalibrators();
    
    //+++ Load List Of Instruments
    findSANSinstruments();
    
    readSettings();
}

dan18::~dan18()
{
    writeSettings();
}

// *******************************************
// +++ app
// *******************************************
ApplicationWindow* dan18::app()
{
    if (this->parentWidget()->parent())
    {
    QDockWidget *docWin=(QDockWidget*)this->parentWidget();
    return (ApplicationWindow *)docWin->parent();
    }
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
