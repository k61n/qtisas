/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Set column values dialog
 ******************************************************************************/

#ifndef SETCOLVALUESDIALOG_H
#define SETCOLVALUESDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>

#include "Script.h"
#include "ScriptEdit.h"
#include "ScriptingEnv.h"
#include "ScriptingEnv.h"
#include "Table.h"

//! Set column values dialog
class SetColValuesDialog : public QDialog, public scripted
{
    Q_OBJECT

public:
    explicit SetColValuesDialog(ScriptingEnv *env, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
	void setTable(Table* w);
	void setCompleter(QCompleter *);

private slots:
	bool apply();
	void prevColumn();
	void nextColumn();
	void insertFunction();
	void insertCol();
	void insertCell();
	void insertExplain(int index);
	void updateColumn(int sc);
	void clearFormulas();
#ifdef SCRIPTING_PYTHON
	void updateFunctionsList(bool);
#endif

private:
	Table* table;

	QSize sizeHint() const ;
	void customEvent( QEvent *e );
	void closeEvent(QCloseEvent*);

    QComboBox* functions;
    QComboBox* boxColumn;
    QPushButton* btnAddFunction;
    QPushButton* btnAddCol;
    QPushButton* btnCancel;
    QPushButton *buttonPrev;
    QPushButton *buttonNext;
    QPushButton *addCellButton;
    QPushButton *btnApply;
	QPushButton* buttonClearFormulas;
    ScriptEdit* commands;
    QTextEdit* explain;
	QSpinBox* start, *end;
	QLabel *colNameLabel;

#ifdef SCRIPTING_PYTHON
	QCheckBox *boxMuParser;
#endif
};

#endif
