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

#include <QApplication>
#include "ApplicationWindow.h"
#include <QtiPlotApplication.h>
#include <QtPlugin>

int main( int argc, char ** argv )
{
	QtiPlotApplication app( argc, argv );
	app.connect( &app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()) );
    
	return app.exec();
}
