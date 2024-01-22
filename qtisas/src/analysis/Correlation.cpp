/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Numerical correlation of data sets
 ******************************************************************************/

#include "Correlation.h"
#include <MultiLayer.h>
#include <PlotCurve.h>
#include <ColorBox.h>

#include <QMessageBox>
#include <QLocale>

#include <gsl/gsl_fft_halfcomplex.h>

Correlation::Correlation(ApplicationWindow *parent, Table *t, const QString& colName1, const QString& colName2, int startRow, int endRow)
: Filter(parent, t)
{
	setObjectName(tr("Correlation"));
    setDataFromTable(t, colName1, colName2, startRow, endRow);
}

bool Correlation::setDataFromTable(Table *t, const QString& colName1, const QString& colName2, int startRow, int endRow)
{
    if (!t)
        return false;

    d_table = t;

    int col1 = d_table->colIndex(colName1);
	int col2 = d_table->colIndex(colName2);

	if (col1 < 0){
		QMessageBox::warning((ApplicationWindow *)parent(), tr("QtiSAS") + " - " + tr("Error"),
		tr("The data set %1 does not exist!").arg(colName1));
		d_init_err = true;
		return false;
	} else if (col2 < 0){
		QMessageBox::warning((ApplicationWindow *)parent(), tr("QtiSAS") + " - " + tr("Error"),
		tr("The data set %1 does not exist!").arg(colName2));
		d_init_err = true;
		return false;
	}

    if (d_n > 0)//delete previousely allocated memory
		freeMemory();

    startRow--; endRow--;
	if (startRow < 0 || startRow >= t->numRows())
		startRow = 0;
	if (endRow < 0 || endRow >= t->numRows())
		endRow = t->numRows() - 1;

    int from = qMin(startRow, endRow);
    int to = qMax(startRow, endRow);

	int rows = abs(to - from) + 1;
	d_n = 16; // tmp number of points
	while (d_n < rows)
		d_n *= 2;

    d_x = (double *)malloc(d_n*sizeof(double));
	if (!d_x){
		memoryErrorMessage();
		return false;
	};

	d_y = (double *)malloc(d_n*sizeof(double));
	if (!d_y){
		memoryErrorMessage();
		free(d_x);
		return false;
	};

    if(d_y && d_x){
		memset( d_x, 0, d_n * sizeof( double ) ); // zero-pad the two arrays...
		memset( d_y, 0, d_n * sizeof( double ) );
		for(int i = 0; i< d_n; i++){
		    int j = i + from;
			d_x[i] = d_table->cell(j, col1);
			d_y[i] = d_table->cell(j, col2);
		}
	} else {
		memoryErrorMessage();
		d_n = 0;
		return false;
	}
	return true;
}

void Correlation::output()
{
    // calculate the FFTs of the two functions
	if( gsl_fft_real_radix2_transform( d_x, 1, d_n ) == 0 &&
        gsl_fft_real_radix2_transform( d_y, 1, d_n ) == 0)
	{
		for(int i=0; i<d_n/2; i++ ){// multiply the FFT by its complex conjugate
			if( i==0 || i==(d_n/2)-1 )
				d_x[i] *= d_x[i];
			else{
				int ni = d_n-i;
				double dReal = d_x[i] * d_y[i] + d_x[ni] * d_y[ni];
				double dImag = d_x[i] * d_y[ni] - d_x[ni] * d_y[i];
				d_x[i] = dReal;
				d_x[ni] = dImag;
			}
		}
	} else {
		QMessageBox::warning((ApplicationWindow *)parent(), tr("QtiSAS") + " - " + tr("Error"),
                             tr("Error in GSL forward FFT operation!"));
		return;
	}

	gsl_fft_halfcomplex_radix2_inverse(d_x, 1, d_n );	//inverse FFT

	addResultCurve();
    d_result_table = d_table;
}

void Correlation::addResultCurve()
{
    ApplicationWindow *app = (ApplicationWindow *)parent();
    if (!app)
        return;

    QLocale locale = app->locale();

    if (d_n > d_table->numRows())
        d_table->setNumRows(d_n);

	int cols = d_table->numCols();
	int cols2 = cols+1;
	d_table->addCol();
	d_table->addCol();
	int n = d_n/2;

    double x_temp[d_n], y_temp[d_n];
	for (int i = 0; i<d_n; i++){
	    double x = i - n;
        x_temp[i] = x;

        double y;
        if(i < n)
			y = d_x[n + i];
		else
			y = d_x[i - n];
        y_temp[i] = y;

		d_table->setText(i, cols, QString::number(x));
		d_table->setText(i, cols2, locale.toString(y, 'g', app->d_decimal_digits));
	}

	QStringList l = d_table->colNames().filter(tr("Lag"));
	QString id = QString::number((int)l.size()+1);
	QString label = objectName() + id;

	d_table->setColName(cols, tr("Lag") + id);
	d_table->setColName(cols2, label);
	d_table->setColPlotDesignation(cols, Table::X);
	d_table->setHeaderColType();

	if (d_graphics_display){
		if (!d_output_graph)
			createOutputGraph();

    	DataCurve *c = new DataCurve(d_table, d_table->colName(cols), d_table->colName(cols2));
		c->setData(x_temp, y_temp, d_n);
		c->setPen(QPen(d_curveColor, 1));
		d_output_graph->insertPlotItem(c, Graph::Line);
		d_output_graph->updatePlot();
	}
}
