/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: single persistent QSettings instance
 ******************************************************************************/

#ifndef DEFAULTSETTINGS_H
#define DEFAULTSETTINGS_H

#include <QDir>
#include <QSettings>

class Settings
{
  public:
    static QSettings *DefaultSettings()
    {
#ifdef Q_OS_WIN
        QString dir = QDir::homePath() + "/AppData/Local/qtisas/qtisas.ini";
#else
        QString dir = QDir::homePath() + "/.config/qtisas/qtisas.ini";
#endif
        return new QSettings(dir, QSettings::IniFormat);
    }
};

#endif // DEFAULTSETTINGS_H

