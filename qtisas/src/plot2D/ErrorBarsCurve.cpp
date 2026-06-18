/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Error bars curve
 ******************************************************************************/

#include <iostream>

#include <QPainter>

#include <qwt/qwt_painter.h>
#include <qwt/qwt_scale_map.h>
#include <qwt/qwt_symbol.h>

#include "ErrorBarsCurve.h"
#include "Graph.h"
#include "QwtBarCurve.h"
#include "ScaleEngine.h"

ErrorBarsCurve::ErrorBarsCurve(int orientation, Table *t, const QString& name):
	DataCurve(t, QString(), name),
	d_master_curve(nullptr)
{
	type = orientation;
	init();
}

ErrorBarsCurve::ErrorBarsCurve(Table *t, const QString& name):
	DataCurve(t, QString(), name),
	d_master_curve(nullptr)
{
	type = Vertical;
	init();
}

void ErrorBarsCurve::init()
{
	plus = true;
	minus = true;
	through = false;
	setType(Graph::ErrorBars);
	setPlotStyle(Graph::ErrorBars);
    setStyle(QwtPlotCurve::UserCurve);
}

void ErrorBarsCurve::copy(const ErrorBarsCurve *e)
{
	d_cap_length = e->d_cap_length;
	type = e->type;
	plus = e->plus;
	minus = e->minus;
	through = e->through;
	setPen(e->pen());
	err = e->err;
}

void ErrorBarsCurve::drawSeries(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &,
                                int from, int to) const
{
	if ( !painter || dataSize() <= 0 )
		return;

	if (to < 0)
		to = dataSize() - 1;

	painter->save();
	QPen p = pen();
    p.setCapStyle(Qt::SquareCap);
	p.setJoinStyle(Qt::MiterJoin);
	painter->setPen(p);
	drawErrorBars(painter, xMap, yMap, from, to);
	painter->restore();
}

void ErrorBarsCurve::drawErrorBars(QPainter *painter,
		const QwtScaleMap &xMap, const QwtScaleMap &yMap, int from, int to) const
{
	if (!d_master_curve)
		return;

    const double x_factor = (double)painter->device()->logicalDpiX() / (double)plot()->logicalDpiX();
    const double y_factor = (double)painter->device()->logicalDpiY() / (double)plot()->logicalDpiY();

	double d_xOffset = 0.0;
	double d_yOffset = 0.0;

	QList <QwtBarCurve *> stack;
	if (d_master_curve->type() == Graph::VerticalBars){
		d_xOffset = ((QwtBarCurve *)d_master_curve)->dataOffset();
		stack = ((QwtBarCurve *)d_master_curve)->stackedCurvesList();
	} else if (d_master_curve->type() == Graph::HorizontalBars){
		d_yOffset = ((QwtBarCurve *)d_master_curve)->dataOffset();
		stack = ((QwtBarCurve *)d_master_curve)->stackedCurvesList();
    }

	bool addStackOffset = !stack.isEmpty();

	int skipPoints = d_master_curve->skipSymbolsCount() + d_skip_symbols;
	if (d_skip_symbols > 0)
		skipPoints--;
	if (skipPoints == 0)
		skipPoints = 1;

    // Treat both orientations uniformly: "along" is the axis carrying the error,
    // "cross" is perpendicular to it. point() rebuilds a device coordinate from
    // an (along, cross) pixel pair, swapping axes for horizontal bars.
    const bool vertical = (type == Vertical);
    const QwtScaleMap &alongMap = vertical ? yMap : xMap;
    const QwtScaleMap &crossMap = vertical ? xMap : yMap;
    const int cap2 = (plus || minus) ? qRound(d_cap_length * 0.5 * (vertical ? x_factor : y_factor)) : 0;
    auto point = [vertical](double along, double cross) {
        return vertical ? QPointF(cross, along) : QPointF(along, cross);
    };

    for (int i = from; i <= to; i += skipPoints)
    {
        double xStackOffset = 0.0;
        double yStackOffset = 0.0;

        if (addStackOffset)
        {
            if (vertical)
                yStackOffset = ((QwtBarCurve *)d_master_curve)->stackOffset(i, stack);
            else
                xStackOffset = ((QwtBarCurve *)d_master_curve)->stackOffset(i, stack);
        }

        const double xval = sample(i).x() + xStackOffset;
        const double yval = sample(i).y() + yStackOffset;

        const double error = err[i];
        if (error == 0.0)
            continue;

        const double alongVal = (vertical ? yval : xval) + (vertical ? d_yOffset : d_xOffset);
        const double crossVal = (vertical ? xval : yval) + (vertical ? d_xOffset : d_yOffset);

        // Skip points completely outside the visible area
        if (alongVal + error <= alongMap.s1() || alongVal - error >= alongMap.s2() || crossVal <= crossMap.s1() ||
            crossVal >= crossMap.s2())
            continue;

        // Round to integer pixels so the bar line and its caps stay aligned
        double center = qRound(alongMap.transform(alongVal));
        double high = qRound(alongMap.transform(alongVal + error));
        double low = qRound(alongMap.transform(alongVal - error));
        const double cross = qRound(crossMap.transform(crossVal));

        // Clamp to the visible "along" range and flag clamped ends
        bool highAtEdge = true;
        bool lowAtEdge = true;
        if (alongVal + error >= alongMap.s2())
        {
            highAtEdge = false;
            high = alongMap.p2();
            if (alongVal >= alongMap.s2())
                center = alongMap.p2();
        }
        if (alongVal - error <= alongMap.s1())
        {
            lowAtEdge = false;
            low = alongMap.p1();
            if (alongVal <= alongMap.s1())
                center = alongMap.p1();
        }

        // Draw error bars and caps
        if (plus && highAtEdge)
            painter->drawLine(point(high, cross - cap2), point(high, cross + cap2));
        if (plus && (through || !minus))
            painter->drawLine(point(center, cross), point(high, cross));

        if (minus && lowAtEdge)
            painter->drawLine(point(low, cross - cap2), point(low, cross + cap2));
        if (minus && (through || !plus))
            painter->drawLine(point(center, cross), point(low, cross));

        if (through && !plus && !minus)
            painter->drawLine(point(low, cross), point(high, cross));
    }
}

double ErrorBarsCurve::errorValue(int i)
{
	if (i >= 0 && i < dataSize())
		return err[i];
	else
		return 0.0;
}

bool ErrorBarsCurve::xErrors()
{
	bool x = false;
	if (type == Horizontal)
		x = true;

	return x;
}

void ErrorBarsCurve::setXErrors(bool yes)
{
	if (yes)
		type = Horizontal;
	else
		type = Vertical;
}

void ErrorBarsCurve::setWidth(double w, bool cosmetic)
{
	QPen p = pen();
 	p.setWidthF(w);
    p.setCosmetic(cosmetic);
  	setPen(p);
}

void ErrorBarsCurve::setColor(const QColor& c)
{
	QPen p = pen();
  	p.setColor(c);
  	setPen(p);
}

QRectF ErrorBarsCurve::boundingRect() const
{
    QRectF rect = PlotCurve::boundingRect();

	int size = dataSize();

    QVector<double> X(size), Y(size), min(size), max(size);
	for (int i=0; i<size; i++){
        X[i] = sample(i).x();
        Y[i] = sample(i).y();
		if (type == Vertical){
            min[i] = sample(i).y() - err[i];
            max[i] = sample(i).y() + err[i];
		} else {
            min[i] = sample(i).x() - err[i];
            max[i] = sample(i).x() + err[i];
		}
	}

    QwtPointArrayData<double> *erMin, *erMax;
	if (type == Vertical) {
        erMin = new QwtPointArrayData(X, min);
        erMax = new QwtPointArrayData(X, max);
	} else {
        erMin = new QwtPointArrayData(min, Y);
        erMax = new QwtPointArrayData(max, Y);
	}

    QRectF minrect = erMin->boundingRect();
    QRectF maxrect = erMax->boundingRect();

	rect.setTop(qMin(minrect.top(), maxrect.top()));
	rect.setBottom(qMax(minrect.bottom(), maxrect.bottom()));
	rect.setLeft(qMin(minrect.left(), maxrect.left()));
	rect.setRight(qMax(minrect.right(), maxrect.right()));

	delete erMin;
	delete erMax;

	return rect;
}

void ErrorBarsCurve::setMasterCurve(DataCurve *c)
{
	if (!c || d_master_curve == c)
		return;

	d_master_curve = c;
    setAxes(c->xAxis(), c->yAxis());
	d_start_row = c->startRow();
	d_end_row = c->endRow();
	c->addErrorBars(this);

	loadData();
}

void ErrorBarsCurve::loadData()
{
	if (!d_master_curve)
		return;

    if (!plot())
        return;

	Table *mt = d_master_curve->table();
	if (!mt)
		return;

	int xcol = mt->colIndex(d_master_curve->xColumnName());
	int ycol = mt->colIndex(d_master_curve->title().text());
	int errcol = d_table->colIndex(title().text());
	if (xcol<0 || ycol<0 || errcol<0)
		return;

	d_start_row = d_master_curve->startRow();
	d_end_row = d_master_curve->endRow();
    int r = abs(d_end_row - d_start_row) + 1;
    QVector<QPointF> P(r);
    QVector<double> err(r);
    int data_size = 0;
    QLocale locale = d_table->locale();
	for (int i = d_start_row; i <= d_end_row; i++){
		QString xval = mt->text(i, xcol);
		QString yval = mt->text(i, ycol);
		QString errval = d_table->text(i, errcol);
		if (!xval.isEmpty() && !yval.isEmpty()){
            P[data_size] = d_master_curve->sample(data_size);

			if (!errval.isEmpty())
				err[data_size] = locale.toDouble(errval);
			else
				err[data_size] = 0.0;

			data_size++;
		}
	}

	if (!data_size)
		remove();

    P.resize(data_size);
	err.resize(data_size);

    setSamples(P);
	setErrors(err);
}

QStringList ErrorBarsCurve::plotAssociation()
{
	if (!d_master_curve)
		return QStringList();

	QStringList lst = QStringList() << d_master_curve->xColumnName() + "(X)" << d_master_curve->title().text() + "(Y)";
	if (type == Horizontal)
		lst << title().text() + "(xErr)";
	else
		lst << title().text() + "(yErr)";

	return lst;
}

bool ErrorBarsCurve::updateData(Table *t, const QString& colName)
{
	if (d_table != t || colName != title().text())
		return false;

	loadData();
	return true;
}
