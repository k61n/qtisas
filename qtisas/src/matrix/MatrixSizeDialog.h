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

#include "Matrix.h"
#include <QDialog>

class QGroupBox;
class QPushButton;
class QSpinBox;
class DoubleSpinBox;

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
    MatrixSizeDialog(Matrix *m, QWidget* parent = 0, Qt::WindowFlags fl = 0);

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

#endif // MATRIXSIZEDIALOG_H
