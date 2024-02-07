/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Set matrix values dialog
 ******************************************************************************/

#ifndef MVALUESDIALOG_H
#define MVALUESDIALOG_H

#include <QDialog>
#include <ScriptingEnv.h>
#include <Script.h>
#include <ScriptEdit.h>
#include "Matrix.h"

#ifdef SCRIPTING_PYTHON
class QCheckBox;
#endif
class QComboBox;
class QTextEdit;
class QCompleter;
class QSpinBox;
class QPushButton;
class ScriptEdit;
class Matrix;

//! Set matrix values dialog
class MatrixValuesDialog : public QDialog, public scripted
{
    Q_OBJECT

public:
    explicit MatrixValuesDialog(ScriptingEnv *env, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
	void setMatrix(Matrix *m);
    void setCompleter(QCompleter *);

private slots:
	bool apply();
	void addCell();
	void insertFunction();
	void insertExplain(int index);
#ifdef SCRIPTING_PYTHON
	void updateFunctionsList(bool);
#endif

private:
	Matrix *matrix;

	QSize sizeHint() const ;
	void customEvent( QEvent *e);
	void closeEvent(QCloseEvent*);

	ScriptEdit* commands;
    QComboBox* functions;
    QPushButton* btnAddFunction;
	QPushButton* btnAddCell;
    QPushButton* btnCancel;
    QTextEdit* explain;
	QSpinBox *startRow, *endRow, *startCol, *endCol;
	QPushButton *btnApply;
#ifdef SCRIPTING_PYTHON
	QCheckBox *boxMuParser;
#endif
};

#endif //
