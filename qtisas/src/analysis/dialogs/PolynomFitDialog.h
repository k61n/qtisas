/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Fit polynomial dialog
 ******************************************************************************/

#ifndef POLINOMFITDIALOG_H
#define POLINOMFITDIALOG_H

#include <QDialog>

class QCheckBox;
class QSpinBox;
class QPushButton;
class QComboBox;
class Graph;
class ColorButton;
class DoubleSpinBox;

//! Fit polynomial dialog
class PolynomFitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PolynomFitDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

public slots:
	void fit();
	void setGraph(Graph *g);
	void activateCurve(const QString&);
	void changeDataRange();

private:
	Graph *graph;

    QPushButton* buttonFit;
	QPushButton* buttonCancel;
    QCheckBox* boxShowFormula;
	QComboBox* boxName;
	QSpinBox* boxOrder;
	QSpinBox* boxPoints;
	DoubleSpinBox* boxStart;
	DoubleSpinBox* boxEnd;
	ColorButton* boxColor;
};

#endif
