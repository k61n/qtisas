/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Column options dialog
 ******************************************************************************/

#ifndef TABLEDIALOG_H
#define TABLEDIALOG_H

#include "Table.h"
#include <QDialog>

class QPushButton;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QLabel;
class QTextEdit;
class QSpinBox;

//! Column options dialog
class TableDialog : public QDialog
{
    Q_OBJECT

public:
    TableDialog(Table *t, QWidget *parent, Qt::WindowFlags fl = Qt::WindowFlags());

private slots:
	void prevColumn();
	void nextColumn();
	void updateColumn(int);
	void setColumnWidth(int width);
	void showPrecisionBox(int item);
	void updatePrecision(int prec);
	void setPlotDesignation(int i);
	void accept();
	void apply();
	void updateDisplay(int item);
	void setNumericFormat(int type, int prec, bool allRightColumns);
	void setDayFormat(const QString& format, bool allRightColumns);
	void setMonthFormat(const QString& format, bool allRightColumns);

signals:
	void nameChanged(const QString&);
	void enumRightCols(bool);
	void changeWidth(const QString&, bool);

private:
    void setDateTimeFormat(int type, const QString& format, bool allRightColumns);
	void setTextFormat(bool allRightColumns);
    void closeEvent( QCloseEvent *);

    Table * d_table;

    QPushButton* buttonOk;
    QPushButton* buttonCancel;
	QPushButton* buttonApply;
	QPushButton* buttonPrev;
	QPushButton* buttonNext;
    QLineEdit* colName;
    QCheckBox* enumerateAllBox;
    QCheckBox* applyToRightCols;
    QCheckBox* applyToAllBox;
    QComboBox* formatBox;
	QComboBox* displayBox;
    QComboBox* columnsBox;
    QSpinBox* colWidth, *precisionBox;
	QLabel *labelNumeric, *labelFormat;
	QTextEdit *comments;
	QCheckBox *boxShowTableComments, *boxReadOnly, *boxHideColumn;
};

#endif
