/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Curve range dialog
 ******************************************************************************/

#ifndef CURVERANGEDIALOG_H
#define CURVERANGEDIALOG_H

#include <QDialog>

class QPushButton;
class QLabel;
class QCheckBox;
class QSpinBox;
class Graph;
class DataCurve;

//! Curve range dialog
class CurveRangeDialog : public QDialog
{
    Q_OBJECT

public:
    CurveRangeDialog(QWidget* parent = 0, Qt::WindowFlags fl = 0 );

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
