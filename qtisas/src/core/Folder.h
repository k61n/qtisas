/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Folder for the project explorer
 ******************************************************************************/

#ifndef FOLDER_H
#define FOLDER_H
 
#include <QObject>
#include <QEvent>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>

#include "MdiSubWindow.h"


class FolderListItem;
class Table;
class Matrix;
class MultiLayer;
class Note;
class Graph3D;


//! Folder for the project explorer
class Folder : public QObject
{
    Q_OBJECT

public:
    Folder( Folder *parent, const QString &name );

	QList<MdiSubWindow *> windowsList(){return lstWindows;};

    void addWindow( MdiSubWindow *w );
	void removeWindow( MdiSubWindow *w );

	//! The list of subfolder names, including first generation children only
	QStringList subfolders();

	//! The list of subfolders
	QList<Folder*> folders();

	//! Pointer to the subfolder called s
	Folder* findSubfolder(const QString& s, bool caseSensitive = true, bool partialMatch = false);

	//! Pointer to the first window matching the search criteria
	MdiSubWindow* findWindow(const QString& s, bool windowNames, bool labels,
							 bool caseSensitive, bool partialMatch);

	//! get a window by name
	  /**
	   * Returns the first window with given name that inherits class cls;
	   * nullptr on failure. If recursive is true, do a depth-first recursive
	   * search.
	   */
	MdiSubWindow *window(const QString &name, const char *cls="MdiSubWindow", bool recursive=false);
	//! Return table named name or nullptr
	Table *table(const QString &name, bool recursive=false) { return (Table*) window(name, "Table", recursive); }
	//! Return matrix named name or nullptr
	Matrix *matrix(const QString &name, bool recursive=false) { return (Matrix*) window(name, "Matrix", recursive); }
	//! Return graph named name or nullptr
	MultiLayer *graph(const QString &name, bool recursive=false) { return (MultiLayer*) window(name, "MultiLayer", recursive); }
	//! Return note named name or nullptr
	Note *note(const QString &name, bool recursive=false) { return (Note*) window(name, "Note", recursive); }
	//! Return plot 3D named name or nullptr
	Graph3D *plot3D(const QString &name, bool recursive=false) { return (Graph3D*) window(name, "Graph3D", recursive); }

	//! The complete path of the folder in the project tree
	QString path();

	//! The depth of the folder in the project tree
	int depth();

	Folder *folderBelow();

	//! The root of the hierarchy this folder belongs to.
	Folder* rootFolder();

	//! Size of the folder as a string
	QString sizeToString();

	QString birthDate(){return birthdate;};
	void setBirthDate(const QString& s){birthdate = s;};

	QString modificationDate(){return modifDate;};
	void setModificationDate(const QString& s){modifDate = s;};

	//! Pointer to the corresponding QListViewItem in the main application
	FolderListItem * folderListItem(){return myFolderListItem;};
	void setFolderListItem(FolderListItem *it){myFolderListItem = it;};

    MdiSubWindow *activeWindow(){return d_active_window;};
    void setActiveWindow(MdiSubWindow *w){d_active_window = w;};

	QString logInfo(){return d_log_info;};
	void appendLogInfo(const QString& text){d_log_info += text;};
	void clearLogInfo(){d_log_info = QString();};
    QString name() { return objectName(); }

protected:
    QString birthdate, modifDate;
	QString d_log_info;
    QList<MdiSubWindow *> lstWindows;
	FolderListItem *myFolderListItem;

	//! Pointer to the active window in the folder
	MdiSubWindow *d_active_window;
};

/*****************************************************************************
 *
 * Class WindowListItem
 *
 *****************************************************************************/
//! Windows list item class
class WindowListItem : public QTreeWidgetItem
{
public:
    WindowListItem(QTreeWidget *parent, MdiSubWindow *w);

    MdiSubWindow *window() { return myWindow; };

    static const int itemType = QTreeWidgetItem::UserType + 2;

protected:
    MdiSubWindow *myWindow;
};

/*****************************************************************************
 *
 * Class FolderListItem
 *
 *****************************************************************************/
//! Folders list item class
class FolderListItem : public QTreeWidgetItem
{
public:
    FolderListItem( QTreeWidget *parent, Folder *f );
    FolderListItem( FolderListItem *parent, Folder *f );

	void setActive( bool o );

    static const int itemType = QTreeWidgetItem::UserType + 1;

    Folder *folder() { return myFolder; };

	//! Checks weather the folder item is a grandchild of the source folder
	/**
	 * \param src source folder item
	 */
	bool isChildOf(FolderListItem *src);

protected:
    Folder *myFolder;
};

/*****************************************************************************
 *
 * Class FolderListView
 *
 *****************************************************************************/
//! Folder list view class
class FolderListView : public QTreeWidget
{
    Q_OBJECT

public:
    FolderListView(QWidget *parent = nullptr);

public slots:
	void adjustColumns();
    bool isRenaming() { return state() == QAbstractItemView::EditingState; };

protected slots:
	void expandedItem(QTreeWidgetItem *item);

protected:
	void startDrag(Qt::DropActions supportedActions = Qt::MoveAction);

    void dropEvent(QDropEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
	void mouseDoubleClickEvent(QMouseEvent *e);
	void keyPressEvent(QKeyEvent *e);
    void mouseReleaseEvent(QMouseEvent *) { mousePressed = false; };
	void enterEvent(QEvent *) { mousePressed = false; };

signals:
	void dragItems(QList<QTreeWidgetItem *> items);
	void dropItems(QTreeWidgetItem *dest);
	void renameItem(QTreeWidgetItem *item);
    void itemRenamed(QTreeWidgetItem *item, int, const QString &);
	void addFolderItem();
	void deleteSelection();

private:
	bool mousePressed;
	QPoint presspos;
};

#endif
