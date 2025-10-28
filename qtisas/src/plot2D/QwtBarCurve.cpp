/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Bar curve
 ******************************************************************************/

#include <QPainter>

#include <qwt/qwt_painter.h>

#include "Graph.h"
#include "QwtBarCurve.h"

QwtBarCurve::QwtBarCurve(BarStyle style, Table *t, const QString& xColName, const QString& name, int startRow, int endRow):
    DataCurve(t, xColName, name, startRow, endRow),
    bar_offset(0),
    bar_gap(20),
	d_is_stacked(false)
{
	bar_style = style;

	QPen pen = QPen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	pen.setCosmetic(true);
	setPen(pen);
	setBrush(QBrush(Qt::red));
	setStyle(QwtPlotCurve::UserCurve);

	if (bar_style == Vertical){
		setType(Graph::VerticalBars);
		setPlotStyle(Graph::VerticalBars);
	} else {
		setType(Graph::HorizontalBars);
		setPlotStyle(Graph::HorizontalBars);
	}
}

void QwtBarCurve::copy(QwtBarCurve *b)
{
	bar_gap = b->bar_gap;
	bar_offset = b->bar_offset;
	bar_style = b->bar_style;
	d_is_stacked = b->isStacked();
}

void QwtBarCurve::draw(QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap, int from, int to) const
{
	if ( !painter || dataSize() <= 0 )
		return;

	if (to < 0)
		to = dataSize() - 1;

	painter->save();
	painter->setPen(QwtPainter::scaledPen(pen()));
	painter->setBrush(QwtPlotCurve::brush());
    qreal halfWidth = QwtPainter::scaledPen(pen()).widthF() / 4.0;

	double dx, dy, ref;
	double bar_width = 0;

	if (bar_style == Vertical)
		ref = yMap.transform(1e-100); //smalest positive value for log scales
	else
		ref = xMap.transform(1e-100);
    
    bar_width = 1 - bar_gap * 0.01;

    if (to - from > 0)
    {
        if (bar_style == Vertical)
        {
            dx = xMap.xTransform(x(from + 1)) - xMap.xTransform(x(from));
            for (int i = from + 2; i < to; i++)
            {
                double min = xMap.xTransform(x(i + 1)) - xMap.xTransform(x(i));
                if (min <= dx)
                    dx = min;
            }
            bar_width *= dx;
        }
        else
        {
            dy = abs(yMap.xTransform(y(from + 1)) - yMap.xTransform(y(from)));
            for (int i = from + 2; i < to; i++)
            {
                double min = yMap.xTransform(y(i + 1)) - yMap.xTransform(y(i));
                if (min <= dy)
                    dy = min;
            }
            bar_width *= dy;
        }
    }
    else
    {
        bar_width *= 0.5;
        if (bar_style == Vertical)
            bar_width *= xMap.xTransform(xMap.s2()) - xMap.xTransform(xMap.s1());
        else
            bar_width *= yMap.xTransform(yMap.s2()) - yMap.xTransform(yMap.s1());
    }

	QList <QwtBarCurve *> stack = stackedCurvesList();

	const double half_width = (0.5 - bar_offset*0.01)*bar_width;
	double bw1 = bar_width;

    QRectF canvas;
    canvas.setCoords(xMap.xTransform(xMap.s1()), yMap.xTransform(yMap.s1()), xMap.xTransform(xMap.s2()),
                     yMap.xTransform(yMap.s2()));
     
	for (int i = from; i <= to; i++){
		const double px = xMap.xTransform(x(i));
		const double py = yMap.xTransform(y(i));

		QRectF rect = QRectF();
		if (bar_style == Vertical){
			if (y(i) < 0)
				rect = QRectF(px - half_width, ref, bw1, (py - ref));
			else {
				if (stack.isEmpty())
					rect = QRectF(px - half_width, py, bw1, (ref - py));
				else {
					double sOffset = stackOffset(i, stack);
					rect = QRectF(px - half_width, yMap.xTransform(y(i) + sOffset), bw1, 1);
					rect.setBottom(yMap.xTransform(sOffset));
				}
			}
		} else {
			if (x(i) < 0)
				rect = QRectF(px, py - half_width, (ref - px), bw1);
			else {
				if (stack.isEmpty())
					rect = QRectF(ref, py - half_width, (px - ref), bw1);
				else {
					double sOffset = stackOffset(i, stack);
					rect = QRectF(xMap.xTransform(sOffset), py - half_width, 1, bw1);
					rect.setRight(xMap.xTransform(x(i) + sOffset));
				}
			}
		}

        if (canvas.intersected(rect).isEmpty())
            rect.adjust(-halfWidth, -halfWidth, halfWidth, halfWidth);

        rect = canvas.intersected(rect);

		if (d_is_stacked)
			painter->fillRect(rect, Qt::white);
		painter->drawRect(rect);
	}
	painter->restore();
}

QList <QwtBarCurve *> QwtBarCurve::stackedCurvesList() const
{
	QList <QwtBarCurve *> stack;
	Graph *g = (Graph *)plot();
	if (!g)
		return stack;

	for (int i = 0; i < g->curveCount(); i++){
		DataCurve *c = g->dataCurve(i);
		if (!c || c == this)
			continue;

		if (c->type() != Graph::VerticalBars && c->type() != Graph::HorizontalBars)
			continue;

		QwtBarCurve *bc = (QwtBarCurve *)c;
		if (bc->isStacked() && bc->orientation() == bar_style &&
			g->curveIndex((QwtPlotItem *)bc) < g->curveIndex((QwtPlotItem *)this))
			stack << bc;
	}
	return stack;
}

double QwtBarCurve::stackOffset(int i, QList <QwtBarCurve *> stack) const
{
	double n = (double)dataSize();
	if (i < 0 || i >= n)
		return 0.0;

	double stackOffset = 0.0;
	if (bar_style == Vertical){
		foreach(QwtBarCurve *bc, stack)
			stackOffset += bc->y(i);
	} else {
		foreach(QwtBarCurve *bc, stack)
			stackOffset += bc->x(i);
	}
	return stackOffset;
}

QwtDoubleRect QwtBarCurve::boundingRect() const
{
	QwtDoubleRect rect = QwtPlotCurve::boundingRect();

	double n = (double)dataSize();

	if (bar_style == Vertical){
		double dx = (rect.right() - rect.left())/n;
		rect.setLeft(rect.left() - dx);
		rect.setRight(rect.right() + dx);
	} else {
		double dy = (rect.bottom() - rect.top())/n;
		rect.setTop(rect.top() - dy);
		rect.setBottom(rect.bottom() + dy);
	}

	if (!d_is_stacked)
		return rect;

	QList <QwtBarCurve *> stack = stackedCurvesList();
	if (!stack.isEmpty()){
		double maxStackOffset = 0.0;
		for (int i = 0; i < n; i++){
			const double soffset = stackOffset(i, stack);
			if (soffset > maxStackOffset)
				maxStackOffset = soffset;
		}
		if (bar_style == Vertical)
			rect.setBottom(rect.bottom() + maxStackOffset);
		else
			rect.setRight(rect.right() + maxStackOffset);
	}
	return rect;
}

void QwtBarCurve::setGap (int gap)
{
	if (bar_gap == gap)
		return;

	bar_gap =gap;
}

void QwtBarCurve::setOffset(int offset)
{
	if (bar_offset == offset)
		return;

	bar_offset = offset;
}

double QwtBarCurve::dataOffset()
{
	if (bar_style == Vertical){
		const QwtScaleMap &xMap = plot()->canvasMap(xAxis());
		double dx = xMap.xTransform(x(1)) - xMap.xTransform(x(0));
		if (plot()->isVisible()){
			for (int i = 2; i < dataSize(); i++){
				double min = xMap.xTransform(x(i)) - xMap.xTransform(x(i - 1));
				if (min <= dx)
					 dx = min;
			}
			double bar_width = dx*(1 - bar_gap*0.01);
			double x1 = xMap.xTransform(minXValue()) + bar_offset*0.01*bar_width;
			return xMap.invTransform(x1) - minXValue();
		} else
			return 0.5*bar_offset*0.01*dx*(1 - bar_gap*0.01);
	} else {
		const QwtScaleMap &yMap = plot()->canvasMap(yAxis());
		double dy = yMap.xTransform(y(1)) - yMap.xTransform(y(0));
		if (plot()->isVisible()){
			for (int i = 2; i < dataSize(); i++){
				double min = yMap.xTransform(y(i)) - yMap.xTransform(y(i - 1));
				if (min <= dy)
					dy = min;
			}
			double bar_width = dy*(1 - bar_gap*0.01);
			double y1 = yMap.xTransform(minYValue()) + bar_offset*0.01*bar_width;
			return yMap.invTransform(y1) - minYValue();
		} else
			return 0.5*bar_offset*0.01*dy*(1 - bar_gap*0.01);
	}
	return 0;
}

QString QwtBarCurve::saveToString()
{
	QString s = DataCurve::saveToString();
	if (d_is_stacked)
		s += "<StackWhiteOut>1</StackWhiteOut>\n";
	return s;
}
