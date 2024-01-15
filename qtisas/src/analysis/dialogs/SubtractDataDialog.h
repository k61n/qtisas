/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Subtract reference data dialog
 ******************************************************************************/

#ifndef SUBTRACT_DATA_DIALOG_H
#define SUBTRACT_DATA_DIALOG_H

#include <QDialog>

class QPushButton;
class QRadioButton;
class QComboBox;
class QCheckBox;
class Graph;
class DoubleSpinBox;

//! Subtract reference data dialog
class SubtractDataDialog : public QDialog
{
	Q_OBJECT

public:
	SubtractDataDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );
	void setGraph(Graph *g);

private slots:
	void interpolate();
	void updateTableColumns(int tabnr);
	void setCurrentFolder(bool);

private:
	double combineValues(double v1, double v2);

	Graph *graph;
	QPushButton* buttonApply;
	QPushButton* buttonCancel;
	QComboBox* boxInputName;
	QComboBox* boxReferenceName;
	QComboBox* boxColumnName;
	QComboBox* boxOperation;
	QRadioButton *btnDataset;
	QRadioButton *btnValue;
	QCheckBox *boxCurrentFolder;
	DoubleSpinBox* boxOffset;
};

#endif
