/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Subtract baseline dialog
 ******************************************************************************/

#ifndef BASELINE_DIALOG_H
#define BASELINE_DIALOG_H

#include <QDialog>

class QPushButton;
class QRadioButton;
class QComboBox;
class QLineEdit;
class QSpinBox;
class Graph;
class Table;
class PlotCurve;
class BaselineTool;
class QwtPlotCurve;

//! Subtract baseline dialog
class BaselineDialog : public QDialog
{
	Q_OBJECT

public:
	BaselineDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );
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
