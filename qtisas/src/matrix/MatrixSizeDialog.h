/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Matrix dimensions dialog
 ******************************************************************************/

#ifndef MATRIXSIZEDIALOG_H
#define MATRIXSIZEDIALOG_H

#include <QDialog>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>

#include "DoubleSpinBox.h"
#include "Matrix.h"

//! Matrix dimensions dialog
class MatrixSizeDialog : public QDialog
{
    Q_OBJECT

public:
	//! Constructor
	/**
	 * \param parent parent widget
	 * \param fl window flags
	 */
    explicit MatrixSizeDialog(Matrix *m, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

private slots:
	//! Accept changes and quit
	void accept();
	//! Apply changes
	void apply();

private:
    QPushButton* buttonOk, *buttonApply;
	QPushButton* buttonCancel;
    QGroupBox* groupBox1, *groupBox2;
	QSpinBox *boxCols, *boxRows;
	DoubleSpinBox *boxXStart, *boxYStart, *boxXEnd, *boxYEnd;
	Matrix *d_matrix;
};

#endif
