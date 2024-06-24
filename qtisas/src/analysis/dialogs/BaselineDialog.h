/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Subtract baseline dialog
 ******************************************************************************/

#ifndef BASELINEDIALOG_H
#define BASELINEDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

#include "DataPickerTool.h"
#include "Graph.h"
#include "PlotCurve.h"
#include "Table.h"

//! Subtract baseline dialog
class BaselineDialog : public QDialog
{
	Q_OBJECT

public:
    explicit BaselineDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
	void setGraph(Graph *g);

private slots:
	void updateGraphCurves();
	void updateTableColumns(int tabnr);
	void createBaseline();
	void subtractBaseline(bool add = false);
	void undo();
	void modifyBaseline();
	void enableBaselineOptions();

private:
	void disableBaselineTool();
	double combineValues(double v1, double v2, bool add = false);
	void closeEvent(QCloseEvent* e);

	Graph *graph;
	QPushButton* buttonSubtract;
	QPushButton* buttonUndo;
	QPushButton* buttonModify;
	QPushButton* buttonCreate;
	QPushButton* buttonCancel;

	QComboBox* boxTableName;
	QComboBox* boxColumnName;
	QComboBox* boxInputName;
	QComboBox* boxInterpolationMethod;

	QLineEdit* boxEquation;

	QRadioButton *btnAutomatic;
	QRadioButton *btnEquation;
	QRadioButton *btnDataset;

	QSpinBox* boxPoints;

	PlotCurve *d_baseline;
	Table *d_table;
	BaselineTool *d_picker_tool;
};

#endif
