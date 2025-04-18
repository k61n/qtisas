/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Settings functions of compile interface
 ******************************************************************************/

#include "compile18.h"
#include "settings.h"

//*******************************************
//+++  Read settings
//*******************************************
void compile18::readSettings()
{
    QSettings *settings = Settings::DefaultSettings();

    settings->beginGroup("/Compile");

    QString pathMinGWsettings = settings->value("/Compile_mingwPath").toString();

    if (QFileInfo(pathMinGWsettings + "/bin/g++.exe").exists())
        pathMinGW = pathMinGWsettings;

    mingwPathline->setText(pathMinGW);
    settings->endGroup();

    delete settings;
}
//*******************************************
//+++  Write settings
//*******************************************
void compile18::saveSettings()
{
    QSettings *settings = Settings::DefaultSettings();

    settings->beginGroup("/Compile");
    settings->setValue("/Compile_mingwPath", mingwPathline->text());
    settings->endGroup();

    delete settings;
}