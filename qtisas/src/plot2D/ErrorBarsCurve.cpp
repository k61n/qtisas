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
	} else {
		const QwtSymbol symbol = d_master_curve->symbol();
		if (symbol.style() != QwtSymbol::NoSymbol){
			sh2 = int(0.5*y_factor*symbol.size().height());
			sw2 = int(0.5*x_factor*symbol.size().width());
		}
	}
	bool addStackOffset = !stack.isEmpty();

	ScaleEngine *yScaleEngine = (ScaleEngine *)plot()->axisScaleEngine(yAxis());
	//bool logYScale = (yScaleEngine->type() == ScaleTransformation::Log10) ? true : false;
    bool logYScale = (yScaleEngine->type() > 0) ? true : false;
    ScaleEngine *xScaleEngine = (ScaleEngine *)plot()->axisScaleEngine(xAxis());
    //bool logXScale = (xScaleEngine->type() == ScaleTransformation::Log10) ? true : false;
    bool logXScale = (xScaleEngine->type() >0) ? true : false;
    
	int skipPoints = d_master_curve->skipSymbolsCount() + d_skip_symbols;
	if (d_skip_symbols > 0)
		skipPoints--;
	if (skipPoints == 0)
		skipPoints = 1;

	for (int i = from; i <= to; i += skipPoints){
		double xStackOffset = 0.0;
		double yStackOffset = 0.0;
		if (addStackOffset){
			if (d_master_curve->type() == Graph::VerticalBars)
				yStackOffset = ((QwtBarCurve *)d_master_curve)->stackOffset(i, stack);
			else if (d_master_curve->type() == Graph::HorizontalBars)
				xStackOffset = ((QwtBarCurve *)d_master_curve)->stackOffset(i, stack);
		}

		const double xval = x(i) + xStackOffset;
		const double yval = y(i) + yStackOffset;

		const double xi = xMap.xTransform(xval + d_xOffset);
		const double yi = yMap.xTransform(yval + d_yOffset);

		double error = err[i];
		if (error == 0.0)
			continue;

		if (type == Vertical){
            
            if (logYScale && yval<0) continue;//+++2020.04
            if (logXScale && xval<0) continue;//+++2020.04
            
            if (d_master_curve->type() != Graph::VerticalBars && yval < 0) error *= -1.0; //+++ inversion of errors

            
            const double yh = yMap.xTransform(yval + error);
            
            double deltaM=yval-err[i];//+++2020.04
            if (logYScale && deltaM<=0) deltaM=1e-10*yval;//+++2020.04
            else deltaM=yval - error;
            const double yl = yMap.xTransform(deltaM);//+++2020.04
            //const double yl = yMap.xTransform(yval - error);//+++2020.04
            
            const double yhl = yi - sh2;
			const double ylh = yi + sh2;
			const int cap2 = qRound(d_cap_length*0.5*x_factor);

			if (plus){
				painter->drawLine(QLineF(xi, yhl, xi, yh));
				painter->drawLine(QLineF(xi - cap2, yh, xi + cap2, yh));
			}

			if (minus)
            {
				painter->drawLine(QLineF(xi, ylh, xi, yl));
               if (!logYScale || (logYScale && yval-err[i] > 0)) painter->drawLine(QLineF(xi - cap2, yl, xi + cap2, yl));
			}

			if (through && (plus || minus))
				painter->drawLine(QLineF(xi, yhl, xi, ylh));
 
		} else if (type == Horizontal){
            
            if (logYScale && yval<0) continue;//+++2020.04
            if (logXScale && xval<0) continue;//+++2020.04
            
			if (d_master_curve->type() != Graph::HorizontalBars && xval < 0)
				error *= -1.0;

			const double xp = xMap.xTransform(xval + error);
            double deltaM=xval-err[i];//+++2020.04
            if (logXScale && deltaM<=0) deltaM=1e-10*xval;//+++2020.04
            else deltaM=xval - error;
            //const double xm = xMap.xTransform(xval - error);//+++2020.04
            const double xm = xMap.xTransform(deltaM);//+++2020.04
            
			const double xpm = xi + sw2;
			const double xmp = xi - sw2;
			const int cap2 = qRound(d_cap_length*0.5*y_factor);

			if (plus){
				painter->drawLine(QLineF(xp, yi, xpm, yi));
				painter->drawLine(QLineF(xp, yi - cap2, xp, yi + cap2));
			}
			if (minus){
				painter->drawLine(QLineF(xm, yi, xmp, yi));
				if (!logXScale || (logXScale && xval-err[i] > 0)) painter->drawLine(QLineF(xm, yi - cap2, xm, yi + cap2));
			}
			if (through && (plus || minus))
				painter->drawLine(QLineF(xmp, yi, xpm, yi));
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

void ErrorBarsCurve::setWidth(double w)
{
	QPen p = pen();
 	p.setWidthF(w);
  	setPen(p);
}

void ErrorBarsCurve::setColor(const QColor& c)
{
	QPen p = pen();
  	p.setColor(c);
  	setPen(p);
}

QwtDoubleRect ErrorBarsCurve::boundingRect() const
{
	QwtDoubleRect rect = QwtPlotCurve::boundingRect();

	int size = dataSize();

	QwtArray <double> X(size), Y(size), min(size), max(size);
	for (int i=0; i<size; i++){
		X[i]=x(i);
		Y[i]=y(i);
		if (type == Vertical){
			min[i] = y(i) - err[i];
			max[i] = y(i) + err[i];
		} else {
			min[i] = x(i) - err[i];
			max[i] = x(i) + err[i];
		}
	}

	QwtArrayData *erMin, *erMax;
	if (type == Vertical) {
		erMin=new QwtArrayData(X, min);
		erMax=new QwtArrayData(X, max);
	} else {
		erMin=new QwtArrayData(min, Y);
		erMax=new QwtArrayData(max, Y);
	}

	QwtDoubleRect minrect = erMin->boundingRect();
	QwtDoubleRect maxrect = erMax->boundingRect();

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
	setAxis(c->xAxis(), c->yAxis());
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
	QVector<double> X(r), Y(r), err(r);
    int data_size = 0;
    QLocale locale = d_table->locale();
	for (int i = d_start_row; i <= d_end_row; i++){
		QString xval = mt->text(i, xcol);
		QString yval = mt->text(i, ycol);
		QString errval = d_table->text(i, errcol);
		if (!xval.isEmpty() && !yval.isEmpty()){
			X[data_size] = d_master_curve->x(data_size);
			Y[data_size] = d_master_curve->y(data_size);

			if (!errval.isEmpty())
				err[data_size] = locale.toDouble(errval);
			else
				err[data_size] = 0.0;

			data_size++;
		}
	}

	if (!data_size)
		remove();

    X.resize(data_size);
	Y.resize(data_size);
	err.resize(data_size);

	setData(X.data(), Y.data(), data_size);
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
