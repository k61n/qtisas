/***************************************************************************
	File                 : main.cpp
	Project              : QtiSAS
--------------------------------------------------------------------

	Copyright /QtiPlot/ :   (C) 2011 - 2012 by Stephan Zevenhuizen,
                            (C) 2004 - 2011 by Ion Vasilief,
                            (C) 2006 - June 2007 Tilman Hoener zu Siederdissen, Knut Franke
 
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
	Description          : QtiSAS main function

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

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
