/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Curve range dialog
 ******************************************************************************/

#ifndef CURVERANGEDIALOG_H
#define CURVERANGEDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

#include "Graph.h"
#include "PlotCurve.h"

//! Curve range dialog
class CurveRangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CurveRangeDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

public slots:
	void setCurveToModify(Graph *g, int curve);
	void setCurvesToModify(Graph *g, const QList<int>& indexes);
	void accept();

private:
	QList<DataCurve *> d_curves;
	Graph *d_graph;

    QPushButton* buttonOK;
	QPushButton* buttonCancel;
	QLabel* boxName;
	QSpinBox* boxStart;
	QSpinBox* boxEnd;
	QCheckBox* boxApplyToAll;
};

#endif
