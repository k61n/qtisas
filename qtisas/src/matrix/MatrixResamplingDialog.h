/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Matrix resampling dialog
 ******************************************************************************/

#ifndef MATRIXRESAMPLINGDIALOG_H
#define MATRIXRESAMPLINGDIALOG_H

#include <QDialog>

class Matrix;
class QAbstractButton;
class QDialogButtonBox;
class QSpinBox;
class QComboBox;
class QLineEdit;

//! Matrix properties dialog
class MatrixResamplingDialog : public QDialog
{
    Q_OBJECT

public:
	//! Constructor
	/**
	 * \param parent parent widget
	 * \param fl window flags
	 */
    explicit MatrixResamplingDialog(Matrix *m, bool shrink = false, QWidget *parent = nullptr,
                                    Qt::WindowFlags fl = Qt::WindowFlags());

private slots:
	//! Apply changes
	void apply();
	void buttonClicked(QAbstractButton *);
	void updateDimensionsInfo();

private:
	void calculateDimensions(int *newRows, int *newColumns);
	Matrix *d_matrix;

	QDialogButtonBox *buttonBox;
	QSpinBox* boxColumns, *boxRows;
	QComboBox *boxMethod, *boxAction;
	QLineEdit *dimensionsLabel;
};

#endif // MATRIXRESAMPLINGDIALOG_H
