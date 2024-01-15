/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Filter options dialog
 ******************************************************************************/

#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include <QDialog>

class QPushButton;
class QComboBox;
class QCheckBox;
class Graph;
class ColorButton;
class DoubleSpinBox;

//! Filter options dialog
class FilterDialog : public QDialog
{
    Q_OBJECT

public:
    FilterDialog(int type, QWidget* parent = 0, Qt::WindowFlags fl = 0 );

public slots:
	void setGraph(Graph *g);
	void filter();

private:
	Graph *graph;
	int filter_type;

    QPushButton* buttonFilter;
	QPushButton* buttonCancel;
	QComboBox* boxName;
	QCheckBox* boxOffset;
	DoubleSpinBox* boxStart;
	DoubleSpinBox* boxEnd;
	ColorButton* boxColor;
};

#endif
