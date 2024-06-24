/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Histogram class
 ******************************************************************************/

#ifndef QWTHISTOGRAM_H
#define QWTHISTOGRAM_H

#include "QwtBarCurve.h"
#include "Matrix.h"

//! Histogram class
class QwtHistogram: public QwtBarCurve
{
public:
	QwtHistogram(Table *t, const QString& name, int startRow = 0, int endRow = -1);
	QwtHistogram(Matrix *m);

	void copy(QwtHistogram *h);

	QwtDoubleRect boundingRect() const;

	void setBinning(bool autoBin, double size, double begin, double end);
	//! Convenience function. It disables autobinning
	void setBinning(double binSize, double begin, double end);

	bool autoBinning(){return d_autoBin;};
	//! Convenience function to be used in scripts
	void setAutoBinning(bool autoBin = true);

	double begin(){return d_begin;};
	double end(){return d_end;};
	double binSize(){return d_bin_size;};

	void loadData();

	double mean(){return d_mean;};
	double standardDeviation(){return d_standard_deviation;};
	double minimum(){return d_min;};
	double maximum(){return d_max;};

	Matrix* matrix(){return d_matrix;};

private:
	void init();

	void loadDataFromMatrix();
	virtual void loadLabels();

	Matrix *d_matrix;

	bool d_autoBin;
	double d_bin_size, d_begin, d_end;

	//! Variables storing statistical information
	double d_mean, d_standard_deviation, d_min, d_max;
};

#endif
