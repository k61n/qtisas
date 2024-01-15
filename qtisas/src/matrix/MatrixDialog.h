/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Matrix properties dialog
 ******************************************************************************/

#ifndef MATRIXDIALOG_H
#define MATRIXDIALOG_H

#include <QDialog>

class Matrix;
class QPushButton;
class QSpinBox;
class QComboBox;

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
    MatrixDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );
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

#endif // MATRIXDIALOG_H
