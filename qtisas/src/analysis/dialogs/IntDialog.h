/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2004 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Integration options dialog
 ******************************************************************************/

#ifndef INTDIALOG_H
#define INTDIALOG_H

#include <QDialog>

class QComboBox;
class QPushButton;
class QCheckBox;
class QLineEdit;
class ScriptEdit;
class QSpinBox;
class DoubleSpinBox;
class Graph;

//! Integration options dialog
class IntDialog : public QDialog
{
    Q_OBJECT

public:
    IntDialog(QWidget* parent = 0, Graph *g = 0, Qt::WindowFlags fl = 0 );

private slots:
	void accept();
	void showFunctionLog();
	void insertFunction();
	void clearFunction();

private:
	bool validInput(const QString& function);

	Graph *d_graph;

    QPushButton* buttonOk;
	QPushButton* buttonCancel;
    QCheckBox* boxPlot;
	ScriptEdit* boxFunction;
	QLineEdit* boxVariable;
	QSpinBox* boxSteps;
	DoubleSpinBox* boxStart;
	DoubleSpinBox* boxEnd;
	DoubleSpinBox* boxTol;

	QPushButton *buttonFunctionLog, *addFunctionBtn, *buttonClear;
	QComboBox* boxMathFunctions;
};

#endif
