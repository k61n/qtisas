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

#include <QDialog>

class QPushButton;
class QComboBox;
class QCheckBox;
class QSpinBox;
class Graph;
class Table;
class DoubleSpinBox;

//! Integration options dialog
class IntegrationDialog : public QDialog
{
    Q_OBJECT

public:
	IntegrationDialog(Graph *g, QWidget* parent = 0, Qt::WindowFlags fl = 0 );
	IntegrationDialog(Table *t, QWidget* parent = 0, Qt::WindowFlags fl = 0 );

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
