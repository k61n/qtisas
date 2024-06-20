/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Random XYZ gridding options dialog
 ******************************************************************************/

#ifndef GRIDDINGDIALOG_H
#define GRIDDINGDIALOG_H

#include <qwt3d_curve.h>
//#include <qwt3d_surfaceplot.h>

#include <QDialog>
#include <alglib/ap.h>

class QGroupBox;
class QLabel;
class QPushButton;
class QComboBox;
class QCheckBox;
class QSpinBox;
class Table;
class DoubleSpinBox;

using namespace Qwt3D;

//! Random XYZ gridding options dialog
class GriddingDialog : public QDialog
{
    Q_OBJECT

public:
    GriddingDialog(Table *t, const QString &colName, int nodes, QWidget *parent = nullptr,
                   Qt::WindowFlags fl = Qt::WindowFlags());
	~GriddingDialog();

private slots:
	void accept();
	void preview();
	void setPlotStyle(int);
	void showMethodParameters(int);

private:
	void loadDataFromTable();
	void resetAxesLabels();
	void findBestLayout();

	Table *d_table;
	Qwt3D::Plot3D* sp;

	QString d_col_name;
	int d_nodes;
	ap::real_2d_array xy;

	QPushButton* buttonFit;
	QPushButton* buttonCancel;
	QLabel* boxName;
	QComboBox* boxMethod;
	QSpinBox* boxCols, *boxRows;
	DoubleSpinBox* boxXStart;
	DoubleSpinBox* boxXEnd;
	DoubleSpinBox* boxYStart;
	DoubleSpinBox* boxYEnd;
	DoubleSpinBox* boxRadius;
	QComboBox* boxPlotStyle;
	QCheckBox *showPlotBox;
	QGroupBox *previewBox;
	QGroupBox *gbRadius;
	QGroupBox *gbModel;
	QComboBox* boxModel;
	QSpinBox* boxNQ;
	QSpinBox* boxNW;
};

#endif
