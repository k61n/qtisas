/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: MDI sub window
 ******************************************************************************/

#ifndef MDISUBWINDOW_H
#define MDISUBWINDOW_H

#include <QCloseEvent>
#include <QEvent>
#include <QMdiSubWindow>
#include <QPrinter>
#include <QString>

class ApplicationWindow;
class Folder;

/**
 * \brief Base class of all MDI client windows.
 *
 * These are the main objects of every Qtiplot project.
 * All content (apart from the directory structure) is managed by subclasses of MdiSubWindow.
 *
 * \section future Future Plans
 * Rename to Aspect.
 *
 * \sa Folder, ApplicationWindow
 */
class MdiSubWindow: public QMdiSubWindow
{
	Q_OBJECT

public:

	//! Constructor
	/**
	 * \param label window label
	 * \param parent parent widget
	 * \param name window name
	 * \param f window flags
	 * \sa setCaptionPolicy(), captionPolicy()
	 */
    explicit MdiSubWindow(QString label = QString(), ApplicationWindow *app = nullptr, const QString &name = QString(),
                          Qt::WindowFlags f = Qt::WindowFlags());

	//! Possible window captions.
	enum CaptionPolicy{
		Name = 0, //!< caption determined by the window name
		Label = 1, //!< caption detemined by the window label
		Both = 2 //!< caption = "name - label"
	};
	enum Status{Hidden = -1, Normal = 0, Minimized = 1, Maximized = 2};

    //! Returns a pointer to the parent application
    ApplicationWindow *applicationWindow(){return d_app;};

	//! Return the window label
	QString windowLabel(){return QString(d_label);};
	//! Set the window label
	void setWindowLabel(const QString& s) { d_label = s; updateCaption();};

	//! Return the window name
	QString name(){return objectName();};
	//! Set the window name
	void setName(const QString& s){setObjectName(s); updateCaption();};

	//! Return the caption policy
	CaptionPolicy captionPolicy(){return d_caption_policy;};
	//! Set the caption policy
	void setCaptionPolicy(CaptionPolicy policy) { d_caption_policy = policy; updateCaption(); }

	//! Return the creation date
	QString birthDate(){return d_birthdate;};
	//! Set the creation date
	void setBirthDate(const QString& s){d_birthdate = s;};

	//! Return the window status as a string
	QString aspect();
	//! Return the window status flag (hidden, normal, minimized or maximized)
	Status status(){return d_status;};
	//! Set the window status flag (hidden, normal, minimized or maximized)
	void setStatus(Status s);

	// TODO:
	//! Not implemented yet
	virtual void restore(const QStringList& ){};
	virtual void save(const QString&, const QString &, bool = false){};
	virtual void exportPDF(const QString&){};

	// TODO: make this return something useful
	//! Size of the widget as a string
	virtual QString sizeToString();

	//!Notifies that a window was hidden by a direct user action
	virtual void setHidden();

	//event handlers
	//! Close event handler
	/**
	 * Ask the user "delete, hide, or cancel?" if the
	 * "ask on close" flag is set.
	 */
	void closeEvent( QCloseEvent *);
	void resizeEvent( QResizeEvent* );

	//! Toggle the "ask on close" flag
	void askOnCloseEvent(bool ask){d_confirm_close = ask;};
	//! Filters other object's events (customizes title bar's context menu)
	bool eventFilter(QObject *object, QEvent *e);
	//! Returns the pointer to the parent folder of the window
	Folder* folder(){return d_folder;};

	//! Initializes the pointer to the parent folder of the window
	void setFolder(Folder* f){d_folder = f;};

	void setNormal();
	void setMinimized();
    void setMaximized(MdiSubWindow *initWidget = nullptr);

	//! Show window making sure that layers in MultiLayer windows are not resized
	void restoreWindow();

    //! Returns the size the window had before a change state event to minimized.
    QSize minRestoreSize(){return d_min_restore_size;};
//+++ 2020
    void setMinRestoreSize(QSize size) {d_min_restore_size=size;};
//---
	//! Static function used as a workaround for ASCII files having end line char != '\n'.
	/*
	 * It counts the number of valid rows to be imported and the number of first lines to be ignored.
	 * It creates a temporary file with '\n' terminated lines which can be correctly read by QTextStream
	 * and returnes a path to this file.
	 */
	static QString parseAsciiFile(const QString& fname, const QString &commentString, int endLine,
                                  int ignoreFirstLines, int maxRows, int& rows);

public slots:
	virtual void print(){};
	virtual void print(QPrinter *){};
	//! Notifies the main application that the window has been modified
	void notifyChanges(){emit modifiedWindow(this);};

signals:
	//! Emitted when the window was closed
	void closedWindow(MdiSubWindow *);
	//! Emitted when the window was hidden
	void hiddenWindow(MdiSubWindow *);
	void modifiedWindow(MdiSubWindow *);
	void resizedWindow(MdiSubWindow *);
	//! Emitted when the window status changed
	void statusChanged(MdiSubWindow *);
	//! Show the context menu
	void showContextMenu();

protected:
	//! Catches status changes
	virtual void changeEvent(QEvent *event);

private:
	//! Used to parse ASCII files with carriage return ('\r') endline.
	static QString parseMacAsciiFile(const QString& fname, const QString &commentString,
                        	 int ignoreFirstLines, int maxRows, int& rows);
    //! Set caption according to current CaptionPolicy, name and label
	void updateCaption();
	//!Pointer to the application window
    ApplicationWindow *d_app;
	//!Pointer to the parent folder of the window
	Folder *d_folder;
	//! The window label
	/**
	 * \sa setWindowLabel(), windowLabel(), setCaptionPolicy()
	 */
	QString d_label;
	//! The window status
	Status d_status;
	//! The caption policy
	/**
	 * \sa setCaptionPolicy(), captionPolicy()
	 */
	CaptionPolicy d_caption_policy;
	//! Toggle on/off: Ask the user "delete, hide, or cancel?" on a close event
	bool d_confirm_close;
	//! The creation date
	QString d_birthdate;
    //! Stores the size the window had before a change state event to minimized.
	QSize d_min_restore_size;
};

#endif
