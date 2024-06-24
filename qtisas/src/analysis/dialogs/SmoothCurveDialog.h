/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2009 Jonas Bähr <jonas@fs.ei.tum.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Smoothing options dialog
 ******************************************************************************/

#ifndef SMOOTHDIALOG_H
#define SMOOTHDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QPushButton>
#include <QSpinBox>

#include "ColorButton.h"
#include "Graph.h"

//! Smoothing options dialog
class SmoothCurveDialog : public QDialog
{
	Q_OBJECT

public:
    explicit SmoothCurveDialog(int method, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

public slots:
	void setGraph(Graph *g);
	void smooth();
	void activateCurve(const QString&);

private:
	Graph *graph;
	int smooth_method;
    QPushButton* btnSmooth;
	QPushButton* buttonCancel;
	QComboBox* boxName;
	QSpinBox *boxPointsLeft, *boxPointsRight, *boxOrder;
	QDoubleSpinBox *boxF;
	ColorButton* boxColor;
};

#endif



