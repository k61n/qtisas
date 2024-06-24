/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Integration options dialog
 ******************************************************************************/

#ifndef INTEGRATIONDIALOG_H
#define INTEGRATIONDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QPushButton>
#include <QSpinBox>

#include "DoubleSpinBox.h"
#include "Graph.h"
#include "Table.h"

//! Integration options dialog
class IntegrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IntegrationDialog(Graph *g, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    explicit IntegrationDialog(Table *t, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

public slots:
	void activateCurve(const QString&);
	void integrate();
	void changeDataRange();

private:
	void closeEvent (QCloseEvent * e );

	void setGraph(Graph *g);
	void setTable(Table *t);
	void integrateCurve();
	void integrateTable();

	Graph *d_graph;
	Table *d_table;
	QPushButton* buttonFit;
	QPushButton* buttonCancel;
	QComboBox* boxName;
	QCheckBox *boxSortData;
	QCheckBox *boxShowPlot;
	QCheckBox *boxShowTable;
	QSpinBox *boxStartRow;
	QSpinBox *boxEndRow;
	DoubleSpinBox* boxStart;
	DoubleSpinBox* boxEnd;
};

#endif
