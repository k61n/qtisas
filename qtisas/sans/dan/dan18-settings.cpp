/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS data analysis interface
 ******************************************************************************/

#include "dan18.h"


//*******************************************
//+++  Read settings
//*******************************************
void dan18::readSettings()
{
#ifdef Q_OS_MACOS
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#endif

    //+++
    bool ok;
    QString ss;
    //+++
    settings.beginGroup("/DAN");
    //+++
    ok = settings.contains("/lineEditPathDAT");
    ss = settings.value("/lineEditPathDAT",0).toString();
    if (ok && ss.left(4)!="home") lineEditPathDAT->setText(ss);
    //+++
    ok = settings.contains("/lineEditPathRAD");
    ss = settings.value("/lineEditPathRAD",0).toString();
    if (ok && ss.left(4)!="home") lineEditPathRAD->setText(ss);
    //+++
    ok = settings.contains("/instrument");
    ss = settings.value("/instrument",0).toString();
    //+++
    if (ok)
    {
        if (comboBoxSel->findText(ss) >= 0)
        {
            comboBoxSel->setCurrentIndex(comboBoxSel->findText(ss));
            instrumentSelected();
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
#ifdef Q_OS_MACOS
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#endif

    //+++
    settings.beginGroup("/DAN");
    //+++
    if (lineEditPathDAT->text()!="home")
        settings.setValue("/lineEditPathDAT",  lineEditPathDAT->text());
    if (lineEditPathRAD->text()!="home")
        settings.setValue("/lineEditPathRAD",  lineEditPathRAD->text());
    //+++
    settings.setValue("/instrument",  comboBoxSel->currentText());
    //+++
    settings.endGroup();
}
