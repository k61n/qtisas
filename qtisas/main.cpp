/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: QtiSAS main function
 ******************************************************************************/

#include "QtiPlotApplication.h"

int main(int argc, char **argv)
{
#ifdef Q_OS_WIN
    // Attach to parent console if run from a terminal
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        std::ios::sync_with_stdio();
    }
#endif
    QtiPlotApplication app(argc, argv);
#ifdef Q_OS_MACOS
    QtiPlotApplication::setAttribute(Qt::AA_DontShowIconsInMenus, false);
#endif
    QtiPlotApplication::connect(&app, &QtiPlotApplication::lastWindowClosed, &app, &QtiPlotApplication::quit);
    return QtiPlotApplication::exec();
}
