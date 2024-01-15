/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
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
    InterpolationDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );

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
