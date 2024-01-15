/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2004 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2011 Stephan Zevenhuizen <>
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
