/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plot associations dialog
 ******************************************************************************/

#ifndef ASSOCIATIONSDIALOG_H
#define ASSOCIATIONSDIALOG_H

#include <QDialog>

class QLabel;
class QListWidget;
class QPushButton;
class QTableWidget;
class QStringList;
class Table;
class Graph;
class MdiSubWindow;

//! Plot associations dialog
class AssociationsDialog : public QDialog
{
    Q_OBJECT

public:
    AssociationsDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );

    void initTablesList(QList<MdiSubWindow *> lst, int curve);
	void setGraph(Graph *g);

private slots:
    void updateTable(int index);
    void updateCurves();
    void accept();

private:
	void changePlotAssociation(int curve, const QStringList& lst);
	void updateColumnTypes();
	void uncheckCol(int col);
	void updatePlotAssociation(int row, int col);
	QStringList plotAssociation(const QString& text);
    Table *findTable(int index);
    bool eventFilter(QObject *object, QEvent *e);

	QList <MdiSubWindow*> tables;
	Table *active_table;
	Graph *graph;
	QStringList plotAssociationsList;

    QLabel* tableCaptionLabel;
	QTableWidget *table;
	QPushButton *btnOK, *btnCancel, *btnApply;
    QListWidget* associations;
};

#endif // ASSOCIATIONSDIALOG_H
