/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Alex Kargovsky <kargovsky@yumr.phys.msu.su>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Folder for the project explorer
 ******************************************************************************/

#include <QApplication>
#include <QDateTime>
#include <QDrag>
#include <QDropEvent>
#include <QMimeData>

#include "ApplicationWindow.h"
#include "Folder.h"

Folder::Folder( Folder *parent, const QString &name )
    : QObject(parent), d_log_info(QString()), d_active_window(0)
{
    QLocale l = QLocale::system();
    birthdate = l.toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
	setObjectName(name);
}

QList<Folder*> Folder::folders()
{
	QList<Folder*> lst;
	foreach(QObject *f, children())
		lst.append((Folder*) f);
	return lst;
}

QStringList Folder::subfolders()
{
	QStringList list = QStringList();
	QObjectList folderList = children();
	if (!folderList.isEmpty()){
		QObject * f;
		foreach(f,folderList)
			list << static_cast<Folder *>(f)->objectName();
	}
	return list;
}

QString Folder::path()
{
    QString s = "/" + QString(objectName()) + "/";
    Folder *parentFolder = (Folder *)parent();
    while (parentFolder){
        s.prepend("/" + QString(parentFolder->objectName()));
        parentFolder = (Folder *)parentFolder->parent();
	}
    return s;
}

int Folder::depth()
{
	int d = 0;
    Folder *parentFolder = (Folder *)parent();
    while (parentFolder){
        ++d;
        parentFolder = (Folder *)parentFolder->parent();
	}
    return d;
}

Folder* Folder::folderBelow()
{
	QList<Folder*> lst = folders();
	if (!lst.isEmpty())
		return lst.first();

	Folder *parentFolder = (Folder *)parent();
	Folder *childFolder = this;
	while (parentFolder && childFolder){
		lst = parentFolder->folders();
		int pos = lst.indexOf(childFolder) + 1;
		if (pos < lst.size())
			return lst.at(pos);

		childFolder = parentFolder;
		parentFolder = (Folder *)parentFolder->parent();
	}
	return nullptr;
}

Folder* Folder::findSubfolder(const QString& s, bool caseSensitive, bool partialMatch)
{
	QObjectList folderList = children();
	if (!folderList.isEmpty()){
		QObject * f;
		foreach(f,folderList){
			QString name = static_cast<Folder *>(f)->objectName();
			if (partialMatch){
				if (caseSensitive && name.startsWith(s,Qt::CaseSensitive))
					return static_cast<Folder *>(f);
				else if (!caseSensitive && name.startsWith(s,Qt::CaseInsensitive))
					return static_cast<Folder *>(f);
			} else {// partialMatch == false
				if (caseSensitive && name == s)
					return static_cast<Folder *>(f);
				else if ( !caseSensitive && (name.toLower() == s.toLower()) )
					return static_cast<Folder *>(f);
			}

			Folder* folder = ((Folder*)f)->findSubfolder(s, caseSensitive, partialMatch);
            if(folder)
                return folder;
		}
	}
	return 0;
}

MdiSubWindow* Folder::findWindow(const QString& s, bool windowNames, bool labels,
							 bool caseSensitive, bool partialMatch)
{
	MdiSubWindow* w;
	foreach(w,lstWindows){
		if (windowNames){
			QString name = w->objectName();
			if (partialMatch && name.contains(s, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive))
				return w;
			else if (caseSensitive && name == s)
				return w;
			else {
				QString text = s;
				if (name == text.toLower())
					return w;
			}
		}

		if (labels){
			QString label = w->windowLabel();
			if (partialMatch && label.contains(s, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive))
				return w;
			else if (caseSensitive && label == s)
				return w;
			else {
				QString text = s;
				if (label == text.toLower())
					return w;
			}
		}
	}
	return 0;
}

MdiSubWindow *Folder::window(const QString &name, const char *cls, bool recursive)
{
	foreach (MdiSubWindow *w, lstWindows){
		if (w->inherits(cls) && name == w->objectName())
			return w;
	}

	if (!recursive) return nullptr;
	foreach (QObject *f, children()){
		MdiSubWindow *w = ((Folder*)f)->window(name, cls, true);
		if (w) return w;
	}
	return nullptr;
}

void Folder::addWindow( MdiSubWindow *w )
{
	if (w){
		lstWindows.append( w );
		w->setFolder(this);
		d_active_window = w;
	}
}

void Folder::removeWindow( MdiSubWindow *w )
{
	if (!w)
		return;

	int index = lstWindows.indexOf(w);
	if (index >= 0 && index < lstWindows.size())
		lstWindows.removeAt(index);

	if (d_active_window && d_active_window == w){
		if (!lstWindows.isEmpty())
			d_active_window = lstWindows.first();
		else
			d_active_window = nullptr;
	}
}

QString Folder::sizeToString()
{
	int size = 0;

	QObjectList folderList = children();
	if (!folderList.isEmpty()){
		QObject *f;
		foreach(f,folderList)
			size +=  sizeof(static_cast<Folder *>(f)); // FIXME: Doesn't this function add the size of pointers together? For what?
	}

	MdiSubWindow * w;
	foreach(w, lstWindows)
		size += sizeof(w);

	return QString::number(8*size/1024.0,'f',1)+" "+tr("kB")+" ("+QString::number(8*size)+" "+tr("bytes")+")";
}

Folder* Folder::rootFolder()
{
	Folder *i = this;
	while(i->parent())
		i = (Folder*)i->parent();
	return i;
}

/*****************************************************************************
 *
 * Class FolderListItem
 *
 *****************************************************************************/

FolderListItem::FolderListItem(QTreeWidget *parent, Folder *f)
    : QTreeWidgetItem(parent, FolderListItem::itemType)
{
    myFolder = f;

    setText(0, f->objectName());
	setExpanded(true);
	setActive(true);
	parent->setDragDropMode(QAbstractItemView::DragDrop);
    parent->setDropIndicatorShown(false);
}

FolderListItem::FolderListItem(FolderListItem *parent, Folder *f)
    : QTreeWidgetItem(parent, FolderListItem::itemType)
{
    myFolder = f;

    setText(0, f->objectName());
    setExpanded(true);
	setActive(true);
}

void FolderListItem::setActive( bool o )
{
	if (o)
		setIcon(0, QIcon(":/folder_open.png"));
	else
        setIcon(0, QIcon(":/folder_closed.png"));

	setSelected(o);
}

bool FolderListItem::isChildOf(FolderListItem *src)
{
	FolderListItem *parent = (FolderListItem *)this->parent();
	while (parent){
		if (parent == src)
			return true;

		parent = (FolderListItem *)parent->parent();
	}
	return false;
}

/*****************************************************************************
 *
 * Class FolderListView
 *
 *****************************************************************************/

FolderListView::FolderListView(QWidget *parent)
    : QTreeWidget(parent), mousePressed(false)
{
    setAcceptDrops( true );
    viewport()->setAcceptDrops( true );

	if (parent){
		connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem *)), (ApplicationWindow *)parent, SLOT(modifiedProject()));
		connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), (ApplicationWindow *)parent, SLOT(modifiedProject()));
		connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(expandedItem(QTreeWidgetItem *)));
	}
}

void FolderListView::expandedItem(QTreeWidgetItem *item)
{
    QTreeWidgetItem *next = itemBelow(item);
	if (next)
        setCurrentItem(next, QItemSelectionModel::Deselect);
}

void FolderListView::startDrag(Qt::DropActions supportedActions)
{
    QTreeWidgetItem *item = currentItem();
	if (!item)
		return;

	if (item == topLevelItem(0) && item->treeWidget()->rootIsDecorated())
		return;//it's the project folder so we don't want the user to move it

    QTreeWidgetItem *last = topLevelItem(topLevelItemCount() - 1);
    QRect rect = visualItemRect(last);
    this->scrollContentsBy(0, rect.bottom() - viewport()->height());

	QIcon icon;
	if (item->type() == FolderListItem::itemType)
		icon = QIcon(":/folder_closed.png");
	else
		icon = item->icon(0);

	QDrag *drag = new QDrag(viewport());
    QPixmap pix = icon.pixmap(64);
	drag->setPixmap(pix);
    drag->setHotSpot(QPoint(pix.width() / 2, pix.height() / 2));

	QList<QTreeWidgetItem *> lst;
	for (item = topLevelItem(0); item; item = itemBelow(item)){
		if (item->isSelected())
			lst.append(item);
	}

	emit dragItems(lst);
    drag->setMimeData(mimeData(lst));
    drag->exec(supportedActions);
}

void FolderListView::dropEvent(QDropEvent *e)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTreeWidgetItem *dest = itemAt(e->pos());
#else
    QTreeWidgetItem *dest = itemAt(e->position().toPoint());
#endif
	if (dest && dest->type() == FolderListItem::itemType) {
		emit dropItems(dest);
		e->accept();
	} else
		e->ignore();
}

void FolderListView::keyPressEvent(QKeyEvent *e)
{
	if (isRenaming()) {
		e->ignore();
		return;
	}

    QTreeWidgetItem *item = currentItem();
	if (!item) {
        QTreeWidget::keyPressEvent(e);
        return;
	}

	if (item->type() == FolderListItem::itemType && (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)) {
		emit itemDoubleClicked(item, 0);
		e->accept();
	} else if (e->key() == Qt::Key_F2) {
		if (item)
			emit renameItem(item);
		e->accept();
	} else if(e->key() == Qt::Key_A && e->modifiers() == Qt::ControlModifier){
		selectAll();
		e->accept();
	} else if(e->key() == Qt::Key_F7) {
		emit addFolderItem();
		e->accept();
	} else if(e->key() == Qt::Key_F8){
		emit deleteSelection();
		e->accept();
	} else
		QTreeWidget::keyPressEvent(e);
}

void FolderListView::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (isRenaming()) {
		e->ignore();
		return;
    }
	QTreeWidget::mouseDoubleClickEvent(e);
}

void FolderListView::mousePressEvent(QMouseEvent *e)
{
	QTreeWidget::mousePressEvent(e);
	QTreeWidgetItem *i = itemAt(e->pos());
	if (i)
        if (e->button() == Qt::LeftButton) {
            presspos = e->pos();
            mousePressed = true;
        }
}

void FolderListView::mouseMoveEvent(QMouseEvent *e)
{
    if (mousePressed && (presspos - e->pos()).manhattanLength() > QApplication::startDragDistance()) {
        mousePressed = false;
        QTreeWidgetItem *item = itemAt(presspos);
        if (item)
            startDrag();
    }
}

void FolderListView::adjustColumns()
{
    for (int i = 0; i < columnCount(); i++)
        resizeColumnToContents(i);
}

/*****************************************************************************
 *
 * Class WindowListItem
 *
 *****************************************************************************/

WindowListItem::WindowListItem(QTreeWidget *parent, MdiSubWindow *w)
    : QTreeWidgetItem(parent, WindowListItem::itemType)
{
    myWindow = w;
    setFlags(flags() | Qt::ItemIsDragEnabled);
}
