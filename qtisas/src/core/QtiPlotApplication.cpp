/***************************************************************************
	File                 : QtiPlotApplication.cpp
	Project              : QtiSAS
--------------------------------------------------------------------
    Copyright /QtiSAS/   : (C) 2012-2021  by Vitaliy Pipich
    Copyright /QtiPlot/  : (C) 2004-2011  by Ion Vasilief
 
    Email (use @ for *)  : v.pipich*gmail.com, ion_vasilief*yahoo.fr
	Description          : QtiSAS application
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

#include <QtiPlotApplication.h>
#include <ApplicationWindow.h>
#include <QFileOpenEvent>
#include <QTimer>
#include <QMenu>


QtiPlotApplication::QtiPlotApplication( int & argc, char ** argv) : QApplication( argc, argv)
{
	QStringList args = arguments();
	args.removeFirst(); // remove application name

	if ( (args.count() == 1) && (args[0] == "-a" || args[0] == "--about") ) {
	#ifdef Q_OS_WIN
		QMessageBox *msg = ApplicationWindow::about();
		connect(msg, SIGNAL(destroyed()), this, SLOT(quit()));
	#else
		ApplicationWindow::about(false);
	#endif
	} else {
		bool factorySettings = false;
		if (args.contains("-d") || args.contains("--default-settings"))
			factorySettings = true;

		ApplicationWindow *mw = new ApplicationWindow(factorySettings);
		mw->restoreApplicationGeometry();

		mw->parseCommandLineArguments(args);
	}

#ifdef Q_OS_MACOS
		updateDockMenu();
#endif
    

}

void QtiPlotApplication::close()
{
	ApplicationWindow *mw = d_windows.last();

	quit();
}

bool QtiPlotApplication::event(QEvent *event)
{
	switch (event->type()) {
	case QEvent::FileOpen:
	{
		QString file = static_cast<QFileOpenEvent *>(event)->file();
		if (!d_windows.isEmpty()){
			foreach(ApplicationWindow *w, d_windows){
				if (w->projectname == file){
					if (!w->isActiveWindow() && d_windows.count() > 1)
						activateWindow(w);
					return true;
				}
			}

			ApplicationWindow *mw = d_windows.last();
			if (mw){
				mw->open(file, false, true);
				if (mw->projectname == "untitled"){
					mw->close();
					d_windows.removeAll(mw);
				}
			}
		}

#ifdef Q_OS_MACOS
		updateDockMenu();
#endif
		return true;
	}

	default:
		return QApplication::event(event);
	}
}

void QtiPlotApplication::remove(ApplicationWindow *w)
{
	d_windows.removeAll(w);

#ifdef Q_OS_MACOS
	updateDockMenu();
#endif
}

void QtiPlotApplication::activateWindow(ApplicationWindow *w)
{
	if (!w)
		return;

	((QWidget *)w)->activateWindow();
	w->raise();
	setActiveWindow(w);

#ifdef Q_OS_MACOS
	updateDockMenu();
#endif
}

#ifdef Q_OS_MACOS
void QtiPlotApplication::updateDockMenu()
{
	QMenu *dockMenu = new QMenu();

#ifdef QT_MAC_USE_COCOA
	foreach(ApplicationWindow *w, d_windows){
		QAction *a = dockMenu->addAction(w->windowTitle());
		a->setIconVisibleInMenu(true);
		a->setCheckable(true);
		a->setChecked(w == activeWindow() && !w->isMinimized());
		a->setData(QVariant(d_windows.indexOf(w)));
	}
	dockMenu->addSeparator();
	connect(dockMenu, SIGNAL(triggered(QAction *)), this, SLOT(activateWindow(QAction *)));
#endif

	dockMenu->addAction(QObject::tr("New Window"), this, SLOT(newWindow()));

	qt_mac_set_dock_menu(dockMenu);
    
}

void QtiPlotApplication::activateWindow(QAction *a)
{
	if (!a)
		return;

	QVariant data = a->data();
	if (!data.isValid())
		return;
	activateWindow(d_windows.at(data.toInt()));
}

void QtiPlotApplication::newWindow()
{
	if (d_windows.isEmpty())
		return;

	ApplicationWindow *mw = new ApplicationWindow();
	if (!mw)
		return;

	mw->restoreApplicationGeometry();

	mw->initWindow();

	updateDockMenu();
}
#endif
