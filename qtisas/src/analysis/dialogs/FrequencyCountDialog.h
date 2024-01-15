/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Frequency count options dialog
 ******************************************************************************/

#ifndef FREQUENCYCOUNTDIALOG_H
#define FREQUENCYCOUNTDIALOG_H

#include <QDialog>
#include <gsl/gsl_vector.h>

class QPushButton;
class DoubleSpinBox;
class Table;

//! Filter options dialog
class FrequencyCountDialog : public QDialog
{
    Q_OBJECT

public:
    FrequencyCountDialog(Table *t, QWidget* parent = 0, Qt::WindowFlags fl = 0 );
    ~FrequencyCountDialog();

private slots:
    bool apply();
    void accept();

private:
    Table *d_source_table;
    Table *d_result_table;
    QString d_col_name;
    gsl_vector *d_col_values;
	int d_bins;

    QPushButton* buttonApply;
	QPushButton* buttonCancel;
	QPushButton* buttonOk;
	DoubleSpinBox* boxStart;
	DoubleSpinBox* boxEnd;
	DoubleSpinBox* boxStep;
};

#endif
