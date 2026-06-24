/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Histogram class
 ******************************************************************************/

#include <cmath>

#include <QPainter>

#include <gsl/gsl_histogram.h>
#include <gsl/gsl_vector.h>

#include "Graph.h"
#include "Matrix.h"
#include "QwtHistogram.h"

QwtHistogram::QwtHistogram(Table *t, const QString& name, int startRow, int endRow):
	QwtBarCurve(QwtBarCurve::Vertical, t, "", name, startRow, endRow)
{
	init();
}

QwtHistogram::QwtHistogram(Matrix *m):
    QwtBarCurve(QwtBarCurve::Vertical, nullptr, "matrix", m->objectName(), 0, 0)
{
	if (m){
		init();
		d_matrix = m;
	}
}

void QwtHistogram::init()
{
    d_autoBin = true;
    d_skip_empty_bins = false;
    d_matrix = nullptr;
    d_ratio = 1.0;
    bar_gap = 0;
    setType(Graph::Histogram);
    setPlotStyle(Graph::Histogram);
    setStyle(QwtPlotCurve::UserCurve);
}

void QwtHistogram::copy(QwtHistogram *h)
{
    QwtBarCurve::copy((QwtBarCurve *)h);

    d_autoBin = h->d_autoBin;
    d_skip_empty_bins = h->d_skip_empty_bins;
    d_bin_size = h->d_bin_size;
    d_begin = h->d_begin;
    d_end = h->d_end;
    d_ratio = h->d_ratio;
}

QRectF QwtHistogram::boundingRect() const
{
    QRectF rect = QwtBarCurve::boundingRect();
    rect.setLeft(rect.left() - sample(1).x());
    rect.setRight(rect.right() + sample(static_cast<int>(dataSize()) - 1).x());
	rect.setTop(0);
	rect.setBottom(1.2*rect.bottom());
	return rect;
}

void QwtHistogram::setBinning(bool autoBin, double size, double begin, double end)
{
	d_autoBin = autoBin;
	d_bin_size = size;
	d_begin = begin;
	d_end = end;
}

void QwtHistogram::setBinning(double binSize, double begin, double end)
{
	d_autoBin = false;
	d_bin_size = binSize;
	d_begin = begin;
	d_end = end;
}

void QwtHistogram::setAutoBinning(bool autoBin)
{
	if (d_autoBin == autoBin)
		return;

	d_autoBin = autoBin;
}

void QwtHistogram::loadData()
{
    if (d_matrix){
        loadDataFromMatrix();
        return;
    }

    int r = abs(d_end_row - d_start_row) + 1;
	QVarLengthArray<double> Y(r);

    int ycol = d_table->colIndex(title().text());
	int size = 0;
	for (int i = 0; i<r; i++ ){
		QString yval = d_table->text(i, ycol);
		if (!yval.isEmpty()){
		    bool valid_data = true;
            Y[size] = ((Graph *)plot())->locale().toDouble(yval, &valid_data);
            if (valid_data)
                size++;
		}
	}
	if(size < 2 || (size==2 && Y[0] == Y[1])){//non valid histogram
		double X[2];
		Y.resize(2);
		for (int i = 0; i<2; i++ ){
			Y[i] = 0;
			X[i] = 0;
		}
        setSamples(X, Y.data(), 2);
		return;
	}

	int n;
	gsl_histogram *h;
	if (d_autoBin){
		n = 10;
		h = gsl_histogram_alloc (n);
		if (!h)
			return;

		gsl_vector *v = gsl_vector_alloc (size);
		for (int i = 0; i<size; i++ )
			gsl_vector_set (v, i, Y[i]);

		double min, max;
		gsl_vector_minmax (v, &min, &max);
		gsl_vector_free (v);

		d_begin = floor(min);
		d_end = ceil(max);
		if (d_end == max)
			d_end += 1.0;

		d_bin_size = (d_end - d_begin)/(double)n;

		gsl_histogram_set_ranges_uniform (h, d_begin, d_end);
	} else {
		n = int((d_end - d_begin)/d_bin_size + 1);
		h = gsl_histogram_alloc (n);
		if (!h)
			return;

		double *range = new double[n+2];
		for (int i = 0; i<= n+1; i++ )
			range[i] = d_begin + i*d_bin_size;

		gsl_histogram_set_ranges (h, range, n+1);
		delete[] range;
	}

	for (int i = 0; i<size; i++ )
		gsl_histogram_increment (h, Y[i]);

    const auto X = new double[n]; // stores ranges (x) and bins (y)
	Y.resize(n);
	for (int i = 0; i<n; i++ ){
		Y[i] = gsl_histogram_get (h, i);
		double lower, upper;
		gsl_histogram_get_range (h, i, &lower, &upper);
		X[i] = lower;
	}
    setSamples(X, Y.data(), n);
    delete[] X;

	d_mean = gsl_histogram_mean(h);
	d_standard_deviation = gsl_histogram_sigma(h);
	d_min = gsl_histogram_min_val(h);
	d_max = gsl_histogram_max_val(h);

	gsl_histogram_free (h);
	if (d_show_labels)
		loadLabels();
    if (plot())
        plot()->replot();
}

void QwtHistogram::loadDataFromMatrix()
{
	if (!d_matrix)
		return;

	int size = d_matrix->numRows()*d_matrix->numCols();
	const double *data = d_matrix->matrixModel()->dataVector();

	int n;
	gsl_histogram *h;
    if (d_autoBin)
    {
        double minVal = data[0], maxVal = data[0];
        for (int i = 1; i < size; i++)
        {
            if (data[i] < minVal)
                minVal = data[i];
            if (data[i] > maxVal)
                maxVal = data[i];
        }

        n = qMax(2, qRound(sqrt((double)size)));
        double span = maxVal - minVal;
        double step0 = (span > 0.0) ? span / n : 1.0;

        auto *range = new double[n + 1];
        if (step0 <= 1.0)
        {
            for (int i = 0; i <= n; i++)
                range[i] = minVal + i * step0;
            d_ratio = 1.0;
        }
        else
        {
            // geometric progression: first step = 1, solve (r^n-1)/(r-1) = span
            double lo = 1.0 + 1e-12, hi = 2.0;
            while (hi < 1e100 && (pow(hi, n) - 1.0) / (hi - 1.0) < span)
                hi *= 2.0;
            double r = lo;
            for (int iter = 0; iter < 100; iter++)
            {
                r = 0.5 * (lo + hi);
                if ((pow(r, n) - 1.0) / (r - 1.0) < span)
                {
                    lo = r;
                }
                else
                {
                    hi = r;
                }
            }
            range[0] = minVal;
            for (int i = 0; i < n; i++)
                range[i + 1] = range[i] + pow(r, i);
            d_ratio = r;
        }
        range[n] = maxVal + fabs(maxVal) * 1e-6 + 1e-300;

        d_begin = range[0];
        d_end = range[n];
        d_bin_size = range[1] - range[0];

        h = gsl_histogram_alloc(n);
        if (!h)
        {
            delete[] range;
            return;
        }
        gsl_histogram_set_ranges(h, range, n + 1);
        delete[] range;

        for (int i = 0; i < size; i++)
            gsl_histogram_increment(h, data[i]);
    }
    else
    {
        double span = d_end - d_begin;
        if (d_ratio > 1.0 && d_bin_size > 0.0 && span > 0.0)
        {
            n = qMax(1, qRound(log(1.0 + span * (d_ratio - 1.0) / d_bin_size) / log(d_ratio)));
        }
        else
        {
            n = qMax(1, qRound(span / d_bin_size));
        }

        h = gsl_histogram_alloc(n);
        if (!h)
            return;

        auto *range = new double[n + 1];
        if (d_ratio > 1.0 && d_bin_size > 0.0)
        {
            range[0] = d_begin;
            for (int i = 0; i < n; i++)
                range[i + 1] = range[i] + d_bin_size * pow(d_ratio, i);
        }
        else
        {
            for (int i = 0; i <= n; i++)
                range[i] = d_begin + i * d_bin_size;
        }
        range[n] = d_end;

        gsl_histogram_set_ranges(h, range, n + 1);
        delete[] range;

        for (int i = 0; i < size; i++)
            gsl_histogram_increment(h, data[i]);
    }

    const auto X = new double[n];
    const auto Y = new double[n];
    for (int i = 0; i < n; i++)
    {
        double lower, upper;
        gsl_histogram_get_range(h, i, &lower, &upper);
        X[i] = lower;
        Y[i] = gsl_histogram_get(h, i);
    }
    if (!d_autoBin && d_skip_empty_bins)
    {
        int nz = 0;
        for (int i = 0; i < n; i++)
            if (Y[i] > 0.0)
                nz++;
        const auto Xf = new double[nz];
        const auto Yf = new double[nz];
        int j = 0;
        for (int i = 0; i < n; i++)
        {
            if (Y[i] > 0.0)
            {
                Xf[j] = X[i];
                Yf[j] = Y[i];
                j++;
            }
        }
        setSamples(Xf, Yf, nz);
        delete[] Xf;
        delete[] Yf;
    }
    else
    {
        setSamples(X, Y, n);
    }
    delete[] X;
    delete[] Y;

	d_mean = gsl_histogram_mean(h);
	d_standard_deviation = gsl_histogram_sigma(h);
	d_min = gsl_histogram_min_val(h);
	d_max = gsl_histogram_max_val(h);

	gsl_histogram_free (h);
	if (d_show_labels)
		loadLabels();
    if (plot())
        plot()->replot();
}


void QwtHistogram::loadLabels()
{
	clearLabels();

	int size = this->dataSize();
	if (!size)
		return;

	QwtPlot *d_plot = plot();
	if (!d_plot)
		return;

	int index = 0;
	for (int i = 0; i < size; i++){
		PlotMarker *m = new PlotMarker(index, d_labels_angle);

        QwtText t(QString::number(sample(i).y()));
		t.setColor(d_labels_color);
		t.setFont(d_labels_font);
		if (d_white_out_labels)
			t.setBackgroundBrush(QBrush(Qt::white));
		else
			t.setBackgroundBrush(QBrush(Qt::transparent));
		m->setLabel(t);

		int x_axis = xAxis();
		int y_axis = yAxis();
        m->setAxes(x_axis, y_axis);

        QSizeF size(t.textSize());
        const double dx = d_labels_x_offset * 0.01 * size.height();
        const double dy = -(d_labels_y_offset * 0.01 + 0.5) * size.height();
        double x2 = d_plot->transform(x_axis, sample(index).x()) + dx;
        double y2 = d_plot->transform(y_axis, sample(index).y()) + dy;
		switch(d_labels_align){
			case Qt::AlignLeft:
			break;
			case Qt::AlignHCenter:
				x2 -= size.width()/2;
			break;
			case Qt::AlignRight:
				x2 -= size.width();
			break;
		}
		m->setXValue(d_plot->invTransform(x_axis, x2));
		m->setYValue(d_plot->invTransform(y_axis, y2));
		m->attach(d_plot);
		d_labels_list << m;
		index++;
	}

	d_show_labels = true;
}
