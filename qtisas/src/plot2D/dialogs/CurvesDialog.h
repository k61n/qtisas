/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Add/remove curves dialog
 ******************************************************************************/

#ifndef CURVESDIALOG_H
#define CURVESDIALOG_H

#include <QDialog>
#include <QTreeWidget>

class QComboBox;
class QListWidget;
class QPushButton;
class QCheckBox;
class QTreeWidget;
class TreeWidgetFolderItem;
class Graph;
class Folder;
class Matrix;
class Table;
class ApplicationWindow;
class QLineEdit;
//! Add/remove curves dialog
class CurvesDialog : public QDialog
{
    Q_OBJECT

public:
	enum TreeItemType{FolderItem, ColumnItem, TableItem, MatrixItem};
    explicit CurvesDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

	void setGraph(Graph *graph);

private slots:
	void addCurves();
	void removeCurves();
	int curveStyle();
	void showCurveRangeDialog();
	void showPlotAssociations();
	void showFunctionDialog();
	void showCurveBtn(int);
    void enableAddBtn();
	void enableContentsBtns();
	void showCurveRange(bool);
	void updateCurveRange();
	void showCurrentFolder(bool);
	void raiseCurve();
	void shiftCurveBy(int offset = 1);

    //+++
    void filterAvailable();
    //---
private:
	void closeEvent(QCloseEvent*);
	bool addFolderItems(Folder *f, QTreeWidgetItem* parent = 0);

    void init();

	bool addCurveFromMatrix(Matrix *m);
	bool addCurveFromTable(ApplicationWindow *app, Table *t, const QString& name);

    QSize sizeHint() const;
    void contextMenuEvent(QContextMenuEvent *);

	Graph *d_graph;

    QPushButton* btnAdd;
    //+++ 2020-06
    QCheckBox* plusErr;
    QPushButton* log;
    QPushButton* lin;
    //---
    QPushButton* btnRemove;
    QPushButton* btnOK;
    QPushButton* btnCancel;
	QPushButton* btnAssociations;
	QPushButton* btnEditFunction;
	QPushButton* btnRange;
    QTreeWidget* available;
    QListWidget* contents;
	QComboBox* boxStyle;
	QComboBox* boxMatrixStyle;
	QCheckBox* boxShowRange;
	QCheckBox* boxShowCurrentFolder;
    //+++
    QLineEdit* dataFilter; //+++
    //---
	QPushButton* btnUp;
	QPushButton* btnDown;
	QComboBox* boxXAxis;
	QComboBox* boxYAxis;
};

/*****************************************************************************
 *
 * Class TreeWidgetFolderItem
 *
 *****************************************************************************/
//! Folders list item class
class TreeWidgetFolderItem : public QTreeWidgetItem
{
public:
	TreeWidgetFolderItem( QTreeWidget *parent, Folder *f );
	TreeWidgetFolderItem( QTreeWidgetItem *parent, Folder *f );

	Folder *folder() { return myFolder; };

protected:
	Folder *myFolder;
};

#endif // CurvesDialog_H
