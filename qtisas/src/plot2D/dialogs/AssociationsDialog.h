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
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>

#include "compat.h"
#include "Graph.h"
#include "MdiSubWindow.h"
#include "Table.h"

//! Plot associations dialog
class AssociationsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AssociationsDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    void initTablesList(QList<MdiSubWindow *> lst, int curve);
	void setGraph(Graph *g);

private slots:
    void updateTable(int index);
    void updateCurves();
    void accept() override;

private:
	void changePlotAssociation(int curve, const QStringList& lst);
	void updateColumnTypes();
	void uncheckCol(int col);
	void updatePlotAssociation(int row, int col);
    static QStringList plotAssociation(const QString &text);
    Table *findTable(int index);
    bool eventFilter(QObject *object, QEvent *e) override;

	QList <MdiSubWindow*> tables;
	Table *active_table;
	Graph *graph;
	QStringList plotAssociationsList;

    QLabel* tableCaptionLabel;
	QTableWidget *table;
	QPushButton *btnOK, *btnCancel, *btnApply;
    QListWidget* associations;
};

#endif
