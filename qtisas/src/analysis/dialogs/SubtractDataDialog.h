/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Subtract reference data dialog
 ******************************************************************************/

#ifndef SUBTRACTDATADIALOG_H
#define SUBTRACTDATADIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QPushButton>
#include <QRadioButton>

#include "DoubleSpinBox.h"
#include "Graph.h"

//! Subtract reference data dialog
class SubtractDataDialog : public QDialog
{
	Q_OBJECT

public:
    explicit SubtractDataDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
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
