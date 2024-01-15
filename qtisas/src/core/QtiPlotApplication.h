/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: QtiSAS application
 ******************************************************************************/

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
#ifdef Q_OS_MACOS
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
#ifdef Q_OS_MACOS
	void newWindow();
	void activateWindow(QAction *);
#endif
};

#endif // QTIPLOTAPPLICATION_H
