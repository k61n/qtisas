/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Fit exponential decay dialog
 ******************************************************************************/

#ifndef EXPDECAYDIALOG_H
#define EXPDECAYDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>

#include "ColorButton.h"
#include "DoubleSpinBox.h"
#include "Fit.h"
#include "Graph.h"

//! Fit exponential decay dialog
class ExpDecayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExpDecayDialog(int type, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

public slots:
	void fit();
	void setGraph(Graph *g);

private slots:
	void activateCurve(const QString&);
	void changeDataRange();

private:
    void closeEvent (QCloseEvent *);

    Fit *fitter;
	Graph *graph;
	int slopes;

	QPushButton* buttonFit;
	QPushButton* buttonCancel;
	QComboBox* boxName;
	DoubleSpinBox* boxAmplitude;
	DoubleSpinBox* boxFirst;
	DoubleSpinBox* boxSecond;
	DoubleSpinBox* boxThird;
	DoubleSpinBox* boxStart;
	DoubleSpinBox* boxYOffset;
	QLabel* thirdLabel, *dampingLabel;
	ColorButton* boxColor;
};

#endif
