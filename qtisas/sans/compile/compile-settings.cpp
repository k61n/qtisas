/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Settings functions of compile interface
 ******************************************************************************/

#include "compile18.h"

//*******************************************
//+++  Read settings
//*******************************************
void compile18::readSettings()
{
#ifdef Q_OS_MACOS
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "qtisas", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "qtisas", "QtiSAS");
#endif
    
    /* ---------------- group Compile --------------- */
    settings.beginGroup("/Compile");

    pathMinGW = settings.value("/Compile_mingwPath").toString();
    mingwPathline->setText(pathMinGW);
    settings.endGroup();
}

//*******************************************
//+++  Write settings
//*******************************************
void compile18::saveSettings()
{
#ifdef Q_OS_MACOS
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "qtisas", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "qtisas", "QtiSAS");
#endif
    
    /* ---------------- group Compile --------------- */
    settings.beginGroup("/Compile");
    settings.setValue("/Compile_mingwPath", mingwPathline->text());
    settings.endGroup();
}


