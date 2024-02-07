/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
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
    explicit IntDialog(QWidget *parent = nullptr, Graph *g = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

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
