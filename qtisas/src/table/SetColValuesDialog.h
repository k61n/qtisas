/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
Description: Set column values dialog
 ******************************************************************************/

#ifndef VALUESDIALOG_H
#define VALUESDIALOG_H

#include <ScriptingEnv.h>
#include <Script.h>
#include <QDialog>

class QComboBox;
class QTextEdit;
class QSpinBox;
class QPushButton;
class QLabel;
class QCompleter;
#ifdef SCRIPTING_PYTHON
class QCheckBox;
#endif
class Table;
class ScriptingEnv;
class ScriptEdit;

//! Set column values dialog
class SetColValuesDialog : public QDialog, public scripted
{
    Q_OBJECT

public:
    SetColValuesDialog( ScriptingEnv *env, QWidget* parent = 0, Qt::WindowFlags fl = 0 );
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

#endif //
