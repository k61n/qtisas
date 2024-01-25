/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Define surface plot dialog
 ******************************************************************************/

#ifndef SURFACEDIALOG_H
#define SURFACEDIALOG_H

#include <QDialog>

class QPushButton;
class DoubleSpinBox;
class QCheckBox;
class QComboBox;
class QStackedWidget;
class QSpinBox;
class Graph3D;
class QLineEdit;
class ScriptEdit;

//! Define surface plot dialog
class SurfaceDialog : public QDialog
{
    Q_OBJECT

public:
    SurfaceDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );

public slots:
	void setFunction(Graph3D *);
	void setParametricSurface(Graph3D *);
	void setGraph(Graph3D *g){d_graph = g;};

private slots:
	void clearFunction();
	void accept();
	void showFunctionLog();
	void showXLog();
	void showYLog();
	void showZLog();

private:
	Graph3D *d_graph;

	void initFunctionPage();
	void initParametricSurfacePage();
	void acceptParametricSurface();
	void acceptFunction();

	QWidget* functionPage;
	QWidget* parametricPage;
	QStackedWidget* optionStack;
    QPushButton* buttonOk;
	QPushButton* buttonCancel;
	QPushButton* buttonClear;
	QComboBox* boxType;
	ScriptEdit* boxFunction;
	DoubleSpinBox* boxXFrom;
	DoubleSpinBox* boxXTo;
	DoubleSpinBox* boxYFrom;
	DoubleSpinBox* boxYTo;
	DoubleSpinBox* boxZFrom;
	DoubleSpinBox* boxZTo;

	ScriptEdit* boxX;
	ScriptEdit* boxY;
	ScriptEdit* boxZ;

	DoubleSpinBox* boxUFrom;
	DoubleSpinBox* boxUTo;
	DoubleSpinBox* boxVFrom;
	DoubleSpinBox* boxVTo;

	QCheckBox *boxUPeriodic, *boxVPeriodic;
	QSpinBox *boxColumns, *boxRows, *boxFuncColumns, *boxFuncRows;
	QPushButton *buttonRecentFunc, *buttonXLog, *buttonYLog, *buttonZLog;
};

#endif
