/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2009 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: 2D binning matrix dialog
 ******************************************************************************/

#ifndef BinMatrixDialog_H
#define BinMatrixDialog_H

#include <QDialog>

class QPushButton;
class QSpinBox;
class DoubleSpinBox;
class Table;

//! 2D Binning matrix dialog
class CreateBinMatrixDialog : public QDialog
{
    Q_OBJECT

public:
    CreateBinMatrixDialog(Table *t, int startRow, int endRow, QWidget* parent = 0, Qt::WindowFlags fl = 0);

public slots:
	void accept();

signals:
	void options(const QString&);

private:
    QPushButton* buttonOk;
	QPushButton* buttonCancel;
	DoubleSpinBox *xMinBox, *xMaxBox, *yMinBox, *yMaxBox;
	QSpinBox *rowsBox, *colsBox;
	Table *d_table;
	int d_start_row, d_end_row, d_x_col, d_y_col;
};

#endif
