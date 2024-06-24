/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Function dialog
 ******************************************************************************/

#ifndef FUNCTIONDIALOG_H
#define FUNCTIONDIALOG_H

#include <QAbstractButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QWidget>

#include "ApplicationWindow.h"
#include "DoubleSpinBox.h"
#include "Graph.h"
#include "ScriptEdit.h"

//! Function dialog
class FunctionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FunctionDialog(ApplicationWindow *parent, bool standAlone = true, Qt::WindowFlags fl = Qt::WindowFlags());

    void setCurveToModify(Graph *g, int curve);
    void setCurveToModify(FunctionCurve *c);
	void setGraph(Graph *g){graph = g;};

public slots:
	bool apply();

private slots:
	void raiseWidget(int index);
	void insertFunction();
	void updateFunctionExplain(int);
	void accept();
	bool acceptFunction();
	bool acceptParametric();
	bool acceptPolar();
	void showFunctionLog();
	void showXParLog();
	void showYParLog();
	void showPolarRadiusLog();
	void showPolarThetaLog();
	void setActiveEditor(ScriptEdit *edit){d_active_editor = edit;};
	void guessConstants();
	void buttonClicked(QAbstractButton *);
	void clearList();

private:
	void setConstants(FunctionCurve *c, const QMap<QString, double>& constants);

	Graph *graph;
	int curveID;

	ScriptEdit* boxXFunction;
    ScriptEdit* boxYFunction;
	ScriptEdit* boxPolarRadius;
	ScriptEdit* boxPolarTheta;
    QComboBox* boxType;
    QLabel* textFunction;
    DoubleSpinBox* boxFrom;
    DoubleSpinBox* boxTo;
	QLineEdit* boxParameter;
	DoubleSpinBox* boxParFrom;
	DoubleSpinBox* boxParTo;
	QLineEdit* boxPolarParameter;
	DoubleSpinBox* boxPolarFrom;
	DoubleSpinBox* boxPolarTo;
    QPushButton* buttonClear;
	QSpinBox* boxPoints;
	QSpinBox* boxParPoints;
	QSpinBox* boxPolarPoints;
	QStackedWidget* optionStack;
	ScriptEdit* boxFunction;
	QWidget* functionPage;
	QWidget* polarPage;
	QWidget* parametricPage;
	QTableWidget *boxConstants;
	QPushButton *addFunctionBtn;
	QComboBox* boxMathFunctions;
	QTextEdit* boxFunctionExplain;
	QPushButton *buttonFunctionLog, *buttonXParLog, *buttonYParLog, *buttonPolarRadiusLog, *buttonPolarRThetaLog;
	QDialogButtonBox *buttonBox;

	ApplicationWindow *d_app;
	ScriptEdit *d_active_editor;
	bool d_stand_alone;
};

#endif
