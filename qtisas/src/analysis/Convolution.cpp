/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
    2007 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Numerical convolution/deconvolution of data sets
 ******************************************************************************/

#include <QLocale>
#include <QMessageBox>

#include <gsl/gsl_fft_halfcomplex.h>

#include "Convolution.h"
#include "PlotCurve.h"

Convolution::Convolution(ApplicationWindow *parent, Table *t, const QString& signalColName, const QString& responseColName)
: Filter(parent, t)
{
	setObjectName(tr("Convolution"));
    setDataFromTable(t, signalColName, responseColName);
}

void Convolution::setDataFromTable(Table *t, const QString& signalColName, const QString& responseColName)
{
    if (t && d_table != t)
        d_table = t;

    int signal_col = d_table->colIndex(signalColName);
	int response_col = d_table->colIndex(responseColName);

	if (signal_col < 0)
	{
		QMessageBox::warning((ApplicationWindow *)parent(), tr("QtiSAS") + " - " + tr("Error"),
		tr("The signal data set %1 does not exist!").arg(signalColName));
		d_init_err = true;
		return;
	}
	else if (response_col < 0)
	{
		QMessageBox::warning((ApplicationWindow *)parent(), tr("QtiSAS") + " - " + tr("Error"),
		tr("The response data set %1 does not exist!").arg(responseColName));
		d_init_err = true;
		return;
	}

    if (d_n > 0)//delete previously allocated memory
		freeMemory();

	d_n_response = 0;
	int rows = d_table->numRows();
	for (int i=0; i<rows; i++)
	{
		if (!d_table->text(i, response_col).isEmpty())
			d_n_response++;
	}
	if (d_n_response >= rows/2)
	{
		QMessageBox::warning((ApplicationWindow *)parent(), tr("QtiSAS") + " - " + tr("Error"),
		tr("The response dataset '%1' must be less then half the size of the signal dataset '%2'!").arg(responseColName).arg(signalColName));
		d_init_err = true;
		return;
	}
	else if (d_n_response%2 == 0)
	{
		QMessageBox::warning((ApplicationWindow *)parent(), tr("QtiSAS") + " - " + tr("Error"),
		tr("The response dataset '%1' must contain an odd number of points!").arg(responseColName));
		d_init_err = true;
		return;
	}

	d_n = rows;

	d_n_signal = 16;// tmp number of points
	while (d_n_signal < d_n + d_n_response/2)
		d_n_signal *= 2;

    d_x = new double[d_n_signal]; //signal
	d_y = new double[d_n_response]; //response

    if(d_y && d_x)
	{
		memset( d_x, 0, d_n_signal * sizeof( double ) );// zero-pad signal data array
		for(int i=0; i<d_n; i++)
			d_x[i] = d_table->cell(i, signal_col);
		for(int i=0; i<d_n_response; i++)
			d_y[i] = d_table->cell(i, response_col);
	}
	else
	{
		QMessageBox::critical((ApplicationWindow *)parent(), tr("QtiSAS") + " - " + tr("Error"),
                        tr("Could not allocate memory, operation aborted!"));
        d_init_err = true;
		d_n = 0;
	}
}

void Convolution::output()
{
	convlv(d_x, d_n_signal, d_y, d_n_response, 1);
	addResultCurve();
    d_result_table = d_table;
}

void Convolution::addResultCurve()
{
    ApplicationWindow *app = (ApplicationWindow *)parent();
    if (!app)
        return;

	int cols = d_table->numCols();
	int cols2 = cols+1;

	d_table->addCol();
	d_table->addCol();

    const auto x_temp = new double[d_n];
	QLocale locale = app->locale();
	for (int i = 0; i<d_n; i++){
		double x = i+1;
		x_temp[i] = x;

		d_table->setText(i, cols, QString::number(x));
		d_table->setText(i, cols2, locale.toString(d_x[i], 'g', app->d_decimal_digits));
	}

	QStringList l = d_table->colNames().filter(tr("Index"));
	QString id = QString::number((int)l.size()+1);
	QString label = objectName() + id;

	d_table->setColName(cols, tr("Index") + id);
	d_table->setColName(cols2, label);
	d_table->setColPlotDesignation(cols, Table::X);
	d_table->setHeaderColType();

	if (d_graphics_display){
		if (!d_output_graph)
			createOutputGraph();

    	DataCurve *c = new DataCurve(d_table, d_table->colName(cols), d_table->colName(cols2));
		c->setData(x_temp, d_x, d_n);
		c->setPen(QPen(d_curveColor, 1));
		d_output_graph->insertPlotItem(c, Graph::Line);
		d_output_graph->updatePlot();
	}
    delete[] x_temp;
}

void Convolution::convlv(double *sig, int n, double *dres, int m, int sign)
{
	double *res = new double[n];
	memset(res,0,n*sizeof(double));
	int i, m2 = m/2;
	for (i=0;i<m2;i++)
	{//store the response in wrap around order, see Numerical Recipes doc
		res[i] = dres[m2+i];
		res[n-m2+i] = dres[i];
	}

	if(m2%2==1)
		res[m2]=dres[m-1];

	// calculate ffts
	gsl_fft_real_radix2_transform(res,1,n);
	gsl_fft_real_radix2_transform(sig,1,n);

	double re, im, size;
	for (i=0;i<n/2;i++)
	{// multiply/divide both ffts
		if(i==0 || i==n/2-1)
		{
			if(sign == 1)
				sig[i] = res[i]*sig[i];
			else
				sig[i] = sig[i]/res[i];
		}
		else
		{
			int ni = n-i;
			if(sign == 1)
			{
				re = res[i]*sig[i]-res[ni]*sig[ni];
				im = res[i]*sig[ni]+res[ni]*sig[i];
			}
			else
			{
				size = res[i]*res[i]+res[ni]*res[ni];
				re = res[i]*sig[i]+res[ni]*sig[ni];
				im = res[i]*sig[ni]-res[ni]*sig[i];
				re /= size;
				im /= size;
			}

			sig[i] = re;
			sig[ni] = im;
		}
	}
	delete[] res;
	gsl_fft_halfcomplex_radix2_inverse(sig,1,n);// inverse fft
}
 /**************************************************************************
 *             Class Deconvolution                                         *
 ***************************************************************************/

Deconvolution::Deconvolution(ApplicationWindow *parent, Table *t, const QString& signalColName, const QString& responseColName)
: Convolution(parent, t, signalColName, responseColName)
{
	setObjectName(tr("Deconvolution"));
    setDataFromTable(t, signalColName, responseColName);
}

void Deconvolution::output()
{
	convlv(d_x, signalDataSize(), d_y, responseDataSize(), -1);
	addResultCurve();
    d_result_table = d_table;
}
