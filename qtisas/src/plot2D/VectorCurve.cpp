/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Vector curve class
 ******************************************************************************/

#include <QPainter>

#include <qwt/qwt_painter.h>
#include <qwt/qwt_scale_map.h>

#include "ErrorBarsCurve.h"
#include "Graph.h"
#include "MultiLayer.h"
#include "VectorCurve.h"

VectorCurve::VectorCurve(VectorStyle style, Table *t, const QString& xColName, const char *name,
				const QString& endCol1, const QString& endCol2, int startRow, int endRow):
    DataCurve(t, xColName, name, startRow, endRow),
	d_pen(QPen(Qt::black, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin)),
	filledArrow (true),
	d_style (style),
	d_headLength (4),
	d_headAngle (45),
	d_position (Tail),
	d_end_x_a (endCol1),
	d_end_y_m (endCol2),
	vectorEnd(0)
{
	d_pen.setCosmetic(true);
	if (style == XYXY){
		setType(Graph::VectXYXY);
		setPlotStyle(Graph::VectXYXY);
	} else if (style == XYAM) {
		setType(Graph::VectXYAM);
		setPlotStyle(Graph::VectXYAM);
	}
}

void VectorCurve::copy(const VectorCurve *vc)
{
	d_style = vc->d_style;
	filledArrow = vc->filledArrow;
	d_headLength = vc->d_headLength;
	d_headAngle = vc->d_headAngle;
	d_position = vc->d_position;
	d_pen = vc->d_pen;
    vectorEnd = new QwtPointArrayData(vc->vectorEnd->xData(), vc->vectorEnd->yData());
}

void VectorCurve::draw(QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap, int from, int to) const
{
    if ( !painter || dataSize() <= 0 )
        return;

    if (to < 0)
        to = dataSize() - 1;

    QwtPlotCurve::drawSeries(painter, xMap, yMap, plot()->canvas()->contentsRect(), from, to);

    painter->save();
    painter->setPen(d_pen);
    drawVector(painter, xMap, yMap, from, to);
    painter->restore();
}

void VectorCurve::drawVector(QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap, int from, int to) const
{
	if (d_style == XYAM){
		for (int i = from; i <= to; i++){
            const double x0 = sample(i).x();
            const double y0 = sample(i).y();
            const double angle = vectorEnd->sample(i).x();
            const double mag = vectorEnd->sample(i).y();

            double xs = 0, ys = 0, xe = 0, ye = 0;
			switch(d_position){
				case Tail:
					xs = xMap.transform(x0);
					ys = yMap.transform(y0);
					xe = xMap.transform(x0 + mag*cos(angle));
					ye = yMap.transform(y0 + mag*sin(angle));
				break;

				case Middle:
					{
					double dxh = 0.5*mag*cos(angle);
					double dyh = 0.5*mag*sin(angle);
					xs = xMap.transform(x0 - dxh);
					ys = yMap.transform(y0 - dyh);
					xe = xMap.transform(x0 + dxh);
					ye = yMap.transform(y0 + dyh);
					}
				break;

				case Head:
					xs = xMap.transform(x0 - mag*cos(angle));
					ys = yMap.transform(y0 - mag*sin(angle));
					xe = xMap.transform(x0);
					ye = yMap.transform(y0);
				break;
			}
			QwtPainter::drawLine(painter, xs, ys, xe, ye);
			drawArrowHead(painter, xs, ys, xe, ye);
		}
	} else {
		for (int i = from; i <= to; i++){
            const double xs = xMap.transform(sample(i).x());
            const double ys = yMap.transform(sample(i).y());
            const double xe = xMap.transform(vectorEnd->sample(i).x());
            const double ye = yMap.transform(vectorEnd->sample(i).y());
			QwtPainter::drawLine(painter, xs, ys, xe, ye);
			drawArrowHead(painter, xs, ys, xe, ye);
		}
	}
}

void VectorCurve::drawArrowHead(QPainter *p, const double xs, const double ys, const double xe, const double ye) const
{
	p->save();
	p->translate(xe, ye);
    const double t = theta(xs, ys, xe, ye);
	p->rotate(-t);

    const double pi = 4 * atan(-1.0);
    const double headLength =
        d_headLength * static_cast<double>(p->device()->logicalDpiX()) / static_cast<double>(plot()->logicalDpiX());
    const double d = headLength * tan(pi * static_cast<double>(d_headAngle) / 180.0);

    QPolygonF endArray(3);
    endArray[0] = QPointF(0, 0);
    endArray[1] = QPointF(-headLength, d);
    endArray[2] = QPointF(-headLength, -d);

	if (filledArrow)
		p->setBrush(QBrush(d_pen.color(), Qt::SolidPattern));

	QwtPainter::drawPolygon(p, endArray);
	p->restore();
}

double VectorCurve::theta(const double x0, const double y0, const double x1, const double y1)
{
	double t = 0.0, pi = 4*atan(-1.0);
	if (x1 == x0){
		if (y0 > y1)
			t = 90;
		else
			t = 270;
	} else {
		t = atan2((y1 - y0)*1.0, (x1 - x0)*1.0)*180/pi;
		if (t < 0)
			t = 360 + t;
	}
	return t;
}

void VectorCurve::setVectorEnd(const QString& xColName, const QString& yColName)
{
	if (d_end_x_a == xColName && d_end_y_m == yColName)
		return;

	d_end_x_a = xColName;
	d_end_y_m = yColName;

	loadData();
}

void VectorCurve::setVectorEnd(const QVector<double> &x, const QVector<double> &y)
{
    vectorEnd = new QwtPointArrayData(x, y);
}

void VectorCurve::setVectorEnd(const QVector<QPointF> &p)
{
    QVector<double> x;
    QVector<double> y;
    for (const auto &xy : p)
    {
        x.push_back(xy.x());
        y.push_back(xy.y());
    }
    setVectorEnd(x, y);
}

double VectorCurve::width()
{
	return d_pen.widthF();
}

void VectorCurve::setWidth(double w)
{
	d_pen.setWidthF(w);
}

QColor VectorCurve::color()
{
	return d_pen.color();
}

void VectorCurve::setColor(const QColor& c)
{
	if (d_pen.color() != c)
		d_pen.setColor(c);
}

void VectorCurve::setHeadLength(int l)
{
	if (d_headLength != l)
		d_headLength = l;
}

void VectorCurve::setHeadAngle(int a)
{
	if (d_headAngle != a)
		d_headAngle = a;
}

void VectorCurve::fillArrowHead(bool fill)
{
	if (filledArrow != fill)
		filledArrow = fill;
}

QRectF VectorCurve::boundingRect() const
{
    QRectF rect = PlotCurve::boundingRect();
	if (!vectorEnd)
		return rect;

    QRectF vrect = vectorEnd->boundingRect();

	if (d_style == XYXY){
		rect.setTop(qMin((double)rect.top(), (double)vrect.top()));
		rect.setBottom(qMax((double)rect.bottom(), (double)vrect.bottom()));
		rect.setLeft(qMin((double)rect.left(), (double)vrect.left()));
		rect.setRight(qMax((double)rect.right(), (double)vrect.right()));
	} else {
        const double angle = vectorEnd->sample(0).x();
        double mag = vectorEnd->sample(0).y();
		switch(d_position)
			{
			case Tail:
				rect.setTop(qMin((double)rect.top(), (double)(rect.top()+mag*sin(angle))));
				rect.setBottom(qMax((double)rect.bottom(), (double)(rect.bottom()+mag*sin(angle))));
				rect.setLeft(qMin((double)rect.left(), (double)(rect.left()+mag*cos(angle))));
				rect.setRight(qMax((double)rect.right(), (double)(rect.right()+mag*cos(angle))));
			break;

			case Middle:
				{
				mag *= 0.5;
				rect.setTop(qMin((double)rect.top(), (double)(rect.top() - fabs(mag*sin(angle)))));
				rect.setBottom(qMax((double)rect.bottom(), (double)(rect.bottom() + fabs(mag*sin(angle)))));
				rect.setLeft(qMin((double)rect.left(), (double)(rect.left() - fabs(mag*cos(angle)))));
				rect.setRight(qMax((double)rect.right(), (double)(rect.right() + fabs(mag*cos(angle)))));
				}
			break;

			case Head:
				rect.setTop(qMin((double)rect.top(), (double)(rect.top() - mag*sin(angle))));
				rect.setBottom(qMax((double)rect.bottom(), (double)(rect.bottom() - mag*sin(angle))));
				rect.setLeft(qMin((double)rect.left(), (double)(rect.left() - mag*cos(angle))));
				rect.setRight(qMax((double)rect.right(), (double)(rect.right() - mag*cos(angle))));
			break;
			}
		}
	return rect;
}

void VectorCurve::updateColumnNames(const QString& oldName, const QString& newName, bool updateTableName)
{
    if (updateTableName){
        QString s = title().text();
        QStringList lst = s.split("_", Qt::SkipEmptyParts);
        if (lst[0] == oldName)
            setTitle(newName + "_" + lst[1]);

        lst = d_x_column.split("_", Qt::SkipEmptyParts);
        if (lst[0] == oldName)
            d_x_column = newName + "_" + lst[1];

        lst = d_end_x_a.split("_", Qt::SkipEmptyParts);
        if (lst[0] == oldName)
            d_end_x_a = newName + "_" + lst[1];

        lst = d_end_y_m.split("_", Qt::SkipEmptyParts);
        if (lst[0] == oldName)
            d_end_y_m = newName + "_" + lst[1];
    } else {
        if (title().text() == oldName)
            setTitle(newName);
        if (d_x_column == oldName)
            d_x_column = newName;
		if (d_end_x_a == oldName)
            d_end_x_a = newName;
		if (d_end_y_m == oldName)
            d_end_y_m = newName;
    }
}

QStringList VectorCurve::plotAssociation()
{
	QStringList lst = QStringList() << d_x_column + "(X)" << title().text() + "(Y)";
	if (d_style == XYAM)
		lst << d_end_x_a + "(A)" << d_end_y_m + "(M)";
	else
		lst << d_end_x_a + "(X)" << d_end_y_m + "(Y)";

	if (!d_labels_column.isEmpty())
		lst << d_labels_column + "(L)";

	return lst;
}

bool VectorCurve::updateData(Table *t, const QString& colName)
{
	if (d_table != t ||
	   (colName != title().text() && d_x_column != colName && d_end_x_a != colName && d_end_y_m != colName))
		return false;

	loadData();
	return true;
}

void VectorCurve::loadData()
{
	if (!plot())
		return;

	int xcol = d_table->colIndex(d_x_column);
	int ycol = d_table->colIndex(title().text());
	int endXCol = d_table->colIndex(d_end_x_a.replace("_ ", "_"));
	int endYCol = d_table->colIndex(d_end_y_m.replace("_ ", "_"));

	int rows = abs(d_end_row - d_start_row) + 1;

    QVector<QPointF> P1, P2;
    P1.reserve(rows);
    P2.reserve(rows);

    QLocale locale = ((Graph *)plot())->multiLayer()->locale();

    auto parseDouble = [&locale](const QString &s, double &out) -> bool {
        if (s.isEmpty())
            return false;
        bool ok;
        out = locale.toDouble(s, &ok);
        return ok;
    };

    for (int i = d_start_row; i <= d_end_row; i++)
    {
        double xval, yval, xend, yend;
        if (parseDouble(d_table->text(i, xcol), xval) && parseDouble(d_table->text(i, ycol), yval) &&
            parseDouble(d_table->text(i, endXCol), xend) && parseDouble(d_table->text(i, endYCol), yend))
        {
            P1.append(QPointF(xval, yval));
            P2.append(QPointF(xend, yend));
        }
    }

    if (P1.isEmpty())
		return;
    setSamples(P1);
    for (const auto c : d_error_bars)
        c->setSamples(P1);
    setVectorEnd(P2);
}

VectorCurve::~VectorCurve()
{
	if (vectorEnd)
		delete vectorEnd;
}
