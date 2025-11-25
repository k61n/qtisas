/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: MDI sub window
 ******************************************************************************/

#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QEvent>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QTemporaryFile>
#include <QTextStream>

#include "ApplicationWindow.h"
#include "globals.h"
#include "MdiSubWindow.h"


using std::ifstream;
using std::string;

MdiSubWindow::MdiSubWindow(QString label, ApplicationWindow *app, const QString &name, Qt::WindowFlags f)
    : QMdiSubWindow(app, f), d_app(app), d_folder(app->currentFolder()), d_label(std::move(label)), d_status(Normal),
      d_caption_policy(Both), d_confirm_close(true),
      d_birthdate(app->locale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)),
      d_min_restore_size(QSize())
{
	setObjectName(name);
	setAttribute(Qt::WA_DeleteOnClose);
	setLocale(app->locale());
	if (d_folder)
		d_folder->addWindow(this);
}

void MdiSubWindow::updateCaption()
{
switch (d_caption_policy)
	{
	case Name:
        setWindowTitle(objectName());
	break;

	case Label:
		if (!d_label.isEmpty())
            setWindowTitle(d_label);
		else
            setWindowTitle(objectName());
	break;

	case Both:
		if (!d_label.isEmpty())
            setWindowTitle(objectName() + " - " + d_label);
		else
            setWindowTitle(objectName());
	break;
	}

	d_app->setListViewLabel(objectName(), d_label);
};

void MdiSubWindow::resizeEvent( QResizeEvent* e )
{
	emit resizedWindow(this);
	QMdiSubWindow::resizeEvent( e );
}

void MdiSubWindow::closeEvent(QCloseEvent *e)
{
    if (d_confirm_close)
    {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle(tr("QtiSAS"));
        msgBox.setText(tr("Do you want to hide or delete <p><b>%1</b>?").arg(objectName()));

        QPushButton *deleteButton = msgBox.addButton(tr("Delete"), QMessageBox::AcceptRole);
        QPushButton *hideButton = msgBox.addButton(tr("Hide"), QMessageBox::RejectRole);
        msgBox.addButton(QMessageBox::Cancel);

        msgBox.exec();

        QAbstractButton *clicked = msgBox.clickedButton();

        if (clicked == deleteButton)
        {
            emit closedWindow(this);
            e->accept();
        }
        else if (clicked == hideButton)
        {
            e->ignore();
            emit hiddenWindow(this);
        }
        else
            e->ignore();
    }
    else
    {
        emit closedWindow(this);
        e->accept();
    }
}

QString MdiSubWindow::aspect()
{
QString s = tr("Normal");
switch (d_status)
	{
	case Normal:
	break;

	case Minimized:
		return tr("Minimized");
	break;

	case Maximized:
		return tr("Maximized");
	break;

	case Hidden:
		return tr("Hidden");
	break;
	}
return s;
}

QString MdiSubWindow::sizeToString()
{
return QString::number(sizeof(MdiSubWindow), 'f', 1) + " " + tr("B");
}

void MdiSubWindow::changeEvent(QEvent *event)
{

	if (!isHidden() && event->type() == QEvent::WindowStateChange)
    {
		Status oldStatus = d_status;
		Status newStatus = Normal;
		if( windowState() & Qt::WindowMinimized )
        {
		    if (oldStatus != Minimized)
                d_min_restore_size = frameSize();
	    	newStatus = Minimized;
		}
        else if ( windowState() & Qt::WindowMaximized )
	     	newStatus = Maximized;

		if (newStatus != oldStatus){
			d_status = newStatus;
    		emit statusChanged (this);
		}
	}
	QMdiSubWindow::changeEvent(event);
}

bool MdiSubWindow::eventFilter(QObject *object, QEvent *e)
{
	if (e->type() == QEvent::ContextMenu && object == widget()){
        emit showContextMenu();
        return true;
	}

	if (e->type() == QEvent::Move && object == widget()){
		QObjectList lst = children();
		foreach(QObject *o, lst){
			if (QString(o->metaObject()->className()) == "QMenu" && d_app){
			    d_app->customWindowTitleBarMenu(this, (QMenu *)o);
				break;
			}
		}
	}

	if (e->type() == QEvent::WindowActivate && object == widget() && !parent()){
		d_app->setActiveWindow(this);
		if (d_folder)
			d_folder->setActiveWindow(this);
	}
	return QMdiSubWindow::eventFilter(object, e);
}

void MdiSubWindow::setStatus(Status s)
{
	if (d_status == s) return;

	d_status = s;
	emit statusChanged (this);
}

void MdiSubWindow::setHidden()
{
    d_status = Hidden;
    emit statusChanged (this);
    hide();
}

void MdiSubWindow::restoreWindow()
{
	MultiLayer *ml = qobject_cast<MultiLayer *>(this);
	bool resizeLayers = false;
	if (ml){
		resizeLayers = ml->scaleLayersOnResize();
		ml->setScaleLayersOnResize(false);
	}

	switch (d_status){
		case MdiSubWindow::Hidden:
		case MdiSubWindow::Normal:
			showNormal();
			break;

		case MdiSubWindow::Minimized:
			showMinimized();
			break;

		case MdiSubWindow::Maximized:
			showMaximized();
			break;
	}

	if (ml)
		ml->setScaleLayersOnResize(resizeLayers);
}

void MdiSubWindow::setNormal()
{
    //app->activeWindow()->status()
	showNormal();
	d_status = Normal;
	emit statusChanged (this);
}

void MdiSubWindow::setMinimized()
{
	d_status = Minimized;
	emit statusChanged (this);
	showMinimized();
}

void MdiSubWindow::setMaximized(MdiSubWindow *initWidget)
{
    if (initWidget)
    {
        if (initWidget->status() != MdiSubWindow::Maximized)
            return showNormal();
        initWidget->showNormal();
    }

    showMaximized();
    d_status = Maximized;

    if (d_folder)
        d_folder->setActiveWindow(this);

    emit statusChanged(this);
}

QString MdiSubWindow::parseAsciiFile(const QString& fname, const QString &commentString,
                        int endLine, int ignoreFirstLines, int maxRows, int& rows)
{
	if (endLine == ApplicationWindow::CR)
		return parseMacAsciiFile(fname, commentString, ignoreFirstLines, maxRows, rows);

	//QTextStream replaces '\r\n' with '\n', therefore we don't need a special treatement in this case!

	QFile f(fname);
 	if(!f.open(QIODevice::ReadOnly))
        return {};

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	QTextStream t(&f);

	QTemporaryFile tempFile;
	tempFile.open();
	QTextStream temp(&tempFile);

	for (int i = 0; i < ignoreFirstLines; i++)//skip first 'ignoreFirstLines' lines
		t.readLine();

	bool validCommentString = !commentString.isEmpty();
    QRegularExpression rx = REGEXPS::wildcardToRegex(commentString);
    rx.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
	rows = 0;
	if (maxRows <= 0){//read all valid lines
		while(!t.atEnd()){//count the number of valid rows
			QString s = t.readLine();
			if (validCommentString && s.contains(rx))
				continue;

			rows++;
			temp << s + "\n";
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
	} else {//we write only 'maxRows' valid rows to the temp file
		while(!t.atEnd() && rows < maxRows){
			QString s = t.readLine();
			if (validCommentString && s.contains(rx))
				continue;

			rows++;
			temp << s + "\n";
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
	}
	f.close();

	tempFile.setAutoRemove(false);
	QString path = tempFile.fileName();
	tempFile.close();

	QApplication::restoreOverrideCursor();
	return path;
}

QString MdiSubWindow::parseMacAsciiFile(const QString& fname, const QString &commentString,
                        				int ignoreFirstLines, int maxRows, int& rows)
{
	ifstream f;
    f.open(fname.toLocal8Bit());
 	if(!f)
        return {};

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QTemporaryFile tempFile;
	tempFile.open();
	QTextStream temp(&tempFile);

	for (int i = 0; i < ignoreFirstLines; i++){//skip first 'ignoreFirstLines' lines
		string s;
		getline(f, s, '\r');
	}

	bool validCommentString = !commentString.isEmpty();
    string comment = commentString.toLocal8Bit().constData();
	rows = 0;
	if (maxRows <= 0){//read all valid lines
		while(f.good() && !f.eof()){//count the number of valid rows
			string s;
			getline(f, s, '\r');
			if (validCommentString && s.find(comment) != string::npos)
				continue;

			rows++;
			temp << QString(s.c_str()) + "\n";
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
	} else {//we write only 'maxRows' valid rows to the temp file
		while(f.good() && !f.eof() && rows < maxRows){
			string s;
			getline(f, s, '\r');
			if (validCommentString && s.find(comment) != string::npos)
				continue;

			rows++;
			temp << QString(s.c_str()) + "\n";
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
	}
	f.close();

	tempFile.setAutoRemove(false);
	QString path = tempFile.fileName();
	tempFile.close();

	QApplication::restoreOverrideCursor();
	return path;
}
