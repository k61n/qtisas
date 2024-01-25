/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Extract data values dialog
 ******************************************************************************/

#ifndef EXTRACTDATADIALOG_H
#define EXTRACTDATADIALOG_H

#include <ScriptingEnv.h>
#include <Script.h>
#include <QDialog>

class QLineEdit;
class QComboBox;
class QTextEdit;
class QSpinBox;
class QPushButton;
class QLabel;
class QCompleter;
class Table;
class ScriptingEnv;
class ScriptEdit;


//! Set column values dialog
class ExtractDataDialog : public QDialog, public scripted
{
    Q_OBJECT

public:
	ExtractDataDialog( ScriptingEnv *env, QWidget* parent = 0, Qt::WindowFlags fl = 0 );
	void setTable(Table* w);
	void setCompleter(QCompleter *);

private slots:
	void apply();
	void insertFunction();
	void insertCol();
	void insertOp();
	void insertExplain(int index);
	void clearFormulas();

private:
	Table* table;

	QSize sizeHint() const ;

    QComboBox* functions;
    QComboBox* boxColumn;
	QComboBox *boxOperators;

    QPushButton* btnAddFunction;
    QPushButton* btnAddCol;
    QPushButton* btnCancel;
    QPushButton *btnApply;
	QPushButton* buttonClearFormulas;
	QPushButton* btnAddOp;

    ScriptEdit* commands;
    QTextEdit* explain;
	QSpinBox* start, *end;
	QLineEdit *destNameBox;
};

#endif //
