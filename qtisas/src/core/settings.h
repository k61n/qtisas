/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: settins: default configuration
 ******************************************************************************/

#ifndef DEFAULTSETTINGS_H
#define DEFAULTSETTINGS_H

#include <QSettings>
#include <QStandardPaths>

class Settings
{
  public:
    static QSettings *DefaultSettings()
    {
#ifdef Q_OS_MACOS
        return new QSettings(QSettings::IniFormat, QSettings::UserScope, "qtisas", "QtiSAS");
#elif defined(Q_OS_WIN)
        QString iniPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/QtiSAS.ini";
        return new QSettings(iniPath, QSettings::IniFormat);
#else
        return new QSettings(QSettings::NativeFormat, QSettings::UserScope, "qtisas", "QtiSAS");
#endif
    }
};

#endif // DEFAULTSETTINGS_H

