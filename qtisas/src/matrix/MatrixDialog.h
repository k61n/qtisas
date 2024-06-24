/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Matrix properties dialog
 ******************************************************************************/

#ifndef MATRIXDIALOG_H
#define MATRIXDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QPushButton>
#include <QSpinBox>

#include "Matrix.h"

//! Matrix properties dialog
class MatrixDialog : public QDialog
{
    Q_OBJECT

public:
	//! Constructor
	/**
	 * \param parent parent widget
	 * \param fl window flags
	 */
    explicit MatrixDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    void setMatrix(Matrix *m);

private slots:
	//! Accept changes and quit
	void accept();
	//! Apply changes
	void apply();
	//! Activate the numeric precision choice box
	void showPrecisionBox(int item);

private:
    Matrix *d_matrix;

    QPushButton* buttonOk;
	QPushButton* buttonCancel, *buttonApply;
	QSpinBox* boxColWidth, *boxPrecision;
	QComboBox *boxFormat, *boxNumericDisplay;
};

#endif
