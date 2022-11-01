/***************************************************************************
 File : dan18-settings.cpp
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


//*******************************************
//+++  Read settings
//*******************************************
void dan18::readSettings()
{
#ifdef Q_OS_MAC // Mac
    QSettings settings(QSettings::IniFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#endif

    //+++
    settings.setPath("JCNS", "qtiSAS", QSettings::User);
    //+++
    bool ok;
    QString ss;
    //+++
    settings.beginGroup("/DAN");
    //+++
    ss=settings.readEntry("/lineEditPathDAT",0,&ok);
    if (ok && ss.left(4)!="home") lineEditPathDAT->setText(ss);
    //+++
    ss=settings.readEntry("/lineEditPathRAD",0,&ok); 
    if (ok && ss.left(4)!="home") lineEditPathRAD->setText(ss);
    //+++
    ss=settings.readEntry("/instrument",0,&ok);
    //+++
    if (ok)
    {
        QString currentInstrument=ss;
        //+++
        for (int i =0; i<comboBoxSel->count(); i++ )	if (comboBoxSel->text(i)==currentInstrument)
        {
            comboBoxSel->setCurrentItem(i);
            instrumentSelected();
            break;
        }
    }
    //+++
    settings.endGroup();
}

//*******************************************
//+++  Write settings
//*******************************************
void dan18::writeSettings()
{
#ifdef Q_OS_MAC // Mac
    QSettings settings(QSettings::IniFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#endif
    
    settings.setPath ( "JCNS", "qtiSAS", QSettings::User );
    //+++
    settings.beginGroup("/DAN");
    //+++
    if (lineEditPathDAT->text()!="home") settings.writeEntry("/lineEditPathDAT",  lineEditPathDAT->text());
    if (lineEditPathRAD->text()!="home")settings.writeEntry("/lineEditPathRAD",  lineEditPathRAD->text());
    //+++
    settings.writeEntry("/instrument",  comboBoxSel->currentText());
    //+++
    settings.endGroup();
}
