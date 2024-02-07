/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Interpolation options dialog
 ******************************************************************************/

#ifndef INTERPOLATIONDIALOG_H
#define INTERPOLATIONDIALOG_H

#include <QDialog>

class QPushButton;
class QComboBox;
class QSpinBox;
class Graph;
class ColorButton;
class DoubleSpinBox;

//! Interpolation options dialog
class InterpolationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InterpolationDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

public slots:
	void activateCurve(const QString&);
	void setGraph(Graph *g);
	void interpolate();
	void changeDataRange();

private:
	Graph *graph;
	QPushButton* buttonFit;
	QPushButton* buttonCancel;
	QComboBox* boxName;
	QComboBox* boxMethod;
	QSpinBox* boxPoints;
	DoubleSpinBox* boxStart;
	DoubleSpinBox* boxEnd;
	ColorButton* boxColor;
};

#endif
