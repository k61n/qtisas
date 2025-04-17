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
#ifdef Q_OS_WIN
    QSettings *settings = Settings::DefaultSettings();
    settings->beginGroup("/Compile");

    QString batFileMSVCsettings = settings->value("/batFile_MSVC").toString();
    if (QFileInfo(batFileMSVCsettings).exists())
        batFileMSVC = batFileMSVCsettings;
    lineEditBatFileMSVC->setText(batFileMSVC);

    settings->endGroup();
    delete settings;
#endif
}
//*******************************************
//+++  Write settings
//*******************************************
void compile18::saveSettings()
{
#ifdef Q_OS_WIN
    QSettings *settings = Settings::DefaultSettings();

    settings->beginGroup("/Compile");
    settings->setValue("/batFile_MSVC", lineEditBatFileMSVC->text());
    settings->endGroup();

    delete settings;
#endif
}