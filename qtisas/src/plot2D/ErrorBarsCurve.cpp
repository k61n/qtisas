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

void ErrorBarsCurve::draw(QPainter *painter,
		const QwtScaleMap &xMap, const QwtScaleMap &yMap, int from, int to) const
{
	if ( !painter || dataSize() <= 0 )
		return;

	if (to < 0)
		to = dataSize() - 1;

	painter->save();
	QPen p = pen();
	p.setCapStyle(Qt::FlatCap);
	p.setJoinStyle(Qt::MiterJoin);
	if ((double)painter->device()->logicalDpiX()/(double)plot()->logicalDpiX() != 1.0)
		p = QwtPainter::scaledPen(p);
	painter->setPen(p);
	drawErrorBars(painter, xMap, yMap, from, to);
	painter->restore();
}


QLineF horizontalLine(double xStart, double xEnd, double y, double xMin, double xMax)
{
    // Ensure proper ordering
    if (xStart > xEnd)
        std::swap(xStart, xEnd);
    if (xMin > xMax)
        std::swap(xMin, xMax);

    // Clamp to visible X range
    xStart = std::clamp(xStart, xMin, xMax);
    xEnd = std::clamp(xEnd, xMin, xMax);

    return {xStart, y, xEnd, y};
}

QLineF verticalLine(double yStart, double yEnd, double x, double yMin, double yMax)
{
    // Ensure correct ordering
    if (yStart > yEnd)
        std::swap(yStart, yEnd);
    if (yMin > yMax)
        std::swap(yMin, yMax);

    // Clamp to visible Y range
    yStart = std::clamp(yStart, yMin, yMax);
    yEnd = std::clamp(yEnd, yMin, yMax);

    return {x, yStart, x, yEnd};
}

void ErrorBarsCurve::drawErrorBars(QPainter *painter,
		const QwtScaleMap &xMap, const QwtScaleMap &yMap, int from, int to) const
{
	if (!d_master_curve)
		return;

	int sh2 = 0, sw2 = 0;
	double x_factor = (double)painter->device()->logicalDpiX()/(double)plot()->logicalDpiX();
	double y_factor = (double)painter->device()->logicalDpiY()/(double)plot()->logicalDpiY();
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

    auto *yScaleEngine = (ScaleEngine *)plot()->axisScaleEngine(yAxis());
    auto *xScaleEngine = (ScaleEngine *)plot()->axisScaleEngine(xAxis());
    
	int skipPoints = d_master_curve->skipSymbolsCount() + d_skip_symbols;
	if (d_skip_symbols > 0)
		skipPoints--;
	if (skipPoints == 0)
		skipPoints = 1;

    for (int i = from; i <= to; i += skipPoints)
    {
        double xStackOffset = 0.0;
        double yStackOffset = 0.0;

        if (addStackOffset)
        {
            if (d_master_curve->type() == Graph::VerticalBars)
                yStackOffset = ((QwtBarCurve *)d_master_curve)->stackOffset(i, stack);
            else if (d_master_curve->type() == Graph::HorizontalBars)
                xStackOffset = ((QwtBarCurve *)d_master_curve)->stackOffset(i, stack);
        }

        double xval = sample(i).x() + xStackOffset;
        double yval = sample(i).y() + yStackOffset;

        double xi = xMap.transform(xval + d_xOffset);
        double yi = yMap.transform(yval + d_yOffset);

        double error = err[i];
        if (error == 0.0)
            continue;

        bool plusAtEdge = true;
        bool minusAtEdge = true;

        if (type == Vertical)
        {
            // Skip points completely outside Y or X ranges
            if (yval + error <= yMap.s1() || yval - error >= yMap.s2() || xval <= xMap.s1() || xval >= xMap.s2())
                continue;

            double yh = yMap.transform(yval + error);
            double yl = yMap.transform(yval - error);

            int cap2 = (plus || minus) ? qRound(d_cap_length * 0.5 * x_factor) : 0;

            // Clamp to visible Y range and handle edge flags
            if (yval + error >= yMap.s2())
            {
                plusAtEdge = false;
                yh = yMap.p2();
                if (yval >= yMap.s2())
                    yi = yMap.p2();
            }

            if (yval - error <= yMap.s1())
            {
                minusAtEdge = false;
                yl = yMap.p1();
                if (yval <= yMap.s1())
                    yi = yMap.p1() + 2;
            }

            const double yhl = yi - sh2;
            const double ylh = yi + sh2;

            // Draw error bars and caps
            if (plus && plusAtEdge)
                painter->drawLine(horizontalLine(xi - cap2, xi + cap2, yh, xMap.p1(), xMap.p2()));
            if (plus && (through || !minus))
                painter->drawLine(verticalLine(yhl, yh, xi, yMap.p2(), yMap.p1()));

            if (minus && minusAtEdge)
                painter->drawLine(horizontalLine(xi - cap2, xi + cap2, yl, xMap.p1(), xMap.p2()));
            if (minus && (through || !plus))
                painter->drawLine(verticalLine(ylh, yl, xi, yMap.p2(), yMap.p1()));

            if (through && !plus && !minus)
                painter->drawLine(verticalLine(yl, yh, xi, yMap.p2(), yMap.p1()));
        }
        else if (type == Horizontal)
        {
            // Skip points completely outside X or Y ranges
            if (xval + error <= xMap.s1() || xval - error >= xMap.s2() || yval <= yMap.s1() || yval >= yMap.s2())
                continue;

            double xh = xMap.transform(xval + error);
            double xl = xMap.transform(xval - error);

            int cap2 = (plus || minus) ? qRound(d_cap_length * 0.5 * y_factor) : 0;

            // Clamp to visible X range and handle edge flags
            if (xval + error >= xMap.s2())
            {
                plusAtEdge = false;
                xh = xMap.p2();
                if (xval >= xMap.s2())
                    xi = xMap.p2();
            }

            if (xval - error <= xMap.s1())
            {
                minusAtEdge = false;
                xl = xMap.p1();
                if (xval <= xMap.s1())
                    xi = xMap.p1() + 2;
            }

            const double xhl = xi - sh2;
            const double xlh = xi + sh2;

            // Draw error bars and caps
            if (plus && plusAtEdge)
                painter->drawLine(verticalLine(yi - cap2, yi + cap2, xh, yMap.p2(), yMap.p1()));
            if (plus && (through || !minus))
                painter->drawLine(horizontalLine(xhl, xh, yi, xMap.p1(), xMap.p2()));

            if (minus && minusAtEdge)
                painter->drawLine(verticalLine(yi - cap2, yi + cap2, xl, yMap.p2(), yMap.p1()));
            if (minus && (through || !plus))
                painter->drawLine(horizontalLine(xlh, xl, yi, xMap.p1(), xMap.p2()));

            if (through && !plus && !minus)
                painter->drawLine(horizontalLine(xl, xh, yi, xMap.p1(), xMap.p2()));
        }
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
    QRectF rect = QwtPlotCurve::boundingRect();

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
