/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Filter options dialog
 ******************************************************************************/

#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QPushButton>

#include "ColorButton.h"
#include "DoubleSpinBox.h"
#include "Graph.h"

//! Filter options dialog
class FilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDialog(int type, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

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
