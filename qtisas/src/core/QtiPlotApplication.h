/***************************************************************************
	File                 : QtiPlotApplication.h
	Project              : QtiSAS
--------------------------------------------------------------------
    Copyright  /QtiPlot/        : (C) 2010-2011 by Ion Vasilief     (ion_vasilief*yahoo.fr)
 
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

#ifndef QTIPLOTAPPLICATION_H
#define QTIPLOTAPPLICATION_H

#include <QApplication>
#include <QDebug>
#include <QAction>

class ApplicationWindow;

class QtiPlotApplication : public QApplication
{
	Q_OBJECT
public:
	QtiPlotApplication(int & argc, char ** argv);
	void remove(ApplicationWindow *w);
	void append(ApplicationWindow *w){if (w) d_windows.append(w);};
	void activateWindow(ApplicationWindow *w);
#ifdef Q_WS_MAC
	void updateDockMenu();
#endif

  // reimplemented from QApplication so we can throw exceptions in slots
  virtual bool notify(QObject * receiver, QEvent * event) {
    try {
      return QApplication::notify(receiver, event);
    } catch(std::exception& e) {
      qCritical() << "Exception thrown:" << e.what();
      Qt::CursorShape shape = Qt::ArrowCursor;
      QApplication::setOverrideCursor(QCursor(shape)); 
    }
    return false;
  }

	
	
protected:
	bool event(QEvent *);

private:
	QList<ApplicationWindow *> d_windows;

private slots:
	void close();
#ifdef Q_WS_MAC
	void newWindow();
	void activateWindow(QAction *);
#endif
};

#endif // QTIPLOTAPPLICATION_H
