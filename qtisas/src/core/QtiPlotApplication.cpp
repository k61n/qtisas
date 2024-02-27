/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: QtiSAS application
 ******************************************************************************/

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
    dockMenu->setAsDockMenu();
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
