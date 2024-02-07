/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Fast Fourier transform options dialog
 ******************************************************************************/

#ifndef FFTDIALOG_H
#define FFTDIALOG_H

#include <QDialog>

class QPushButton;
class QRadioButton;
class DoubleSpinBox;
class QComboBox;
class QCheckBox;
class Graph;
class Table;
class Matrix;

//! Fast Fourier transform options dialog
class FFTDialog : public QDialog
{
    Q_OBJECT

public:
	enum DataType{onGraph = 0, onTable = 1, onMatrix = 2};

    explicit FFTDialog(int type, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

public slots:
	void setGraph(Graph *g);
	void setTable(Table *t);
	void setMatrix(Matrix *m);

private slots:
	void activateDataSet(const QString& curveName);
	void accept();
	void updateShiftLabel();

private:
    void fftMatrix();
	void closeEvent (QCloseEvent *);

	Graph *graph;
	Table *d_table;
	int d_type;

	QPushButton* buttonOK;
	QPushButton* buttonCancel;
	QRadioButton *forwardBtn, *backwardBtn;
	QComboBox* boxName, *boxReal, *boxImaginary;
	DoubleSpinBox* boxSampling;
	QCheckBox* boxNormalize, *boxOrder, *boxPower2;
};

#endif
