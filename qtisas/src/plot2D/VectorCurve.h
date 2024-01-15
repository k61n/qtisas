/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Vector curve class
 ******************************************************************************/

#ifndef VECTORCURVE_H
#define VECTORCURVE_H

#include "PlotCurve.h"
#include <qwt_plot.h>

class QwtPlot;

//! Vector curve class
class VectorCurve: public DataCurve
{
public:
	enum VectorStyle{XYXY, XYAM};

	VectorCurve(VectorStyle style, Table *t, const QString& xColName, const char *name,
				const QString& endCol1, const QString& endCol2, int startRow, int endRow);
	~VectorCurve();

	enum Position{Tail, Middle, Head};

	void copy(const VectorCurve *vc);

	QwtDoubleRect boundingRect() const;

	QString vectorEndXAColName(){return d_end_x_a;};
	QString vectorEndYMColName(){return d_end_y_m;};
	void setVectorEnd(const QString& xColName, const QString& yColName);

	double width();
	void setWidth(double w);

	QColor color();
	void setColor(const QColor& c);

	int headLength(){return d_headLength;};
	void setHeadLength(int l);

	int headAngle(){return d_headAngle;};
	void setHeadAngle(int a);

	bool filledArrowHead(){return filledArrow;};
	void fillArrowHead(bool fill);

	int position(){return d_position;};
	void setPosition(int pos){d_position = pos;};

	int vectorStyle(){return d_style;};
	void setVectorStyle(int style){d_style = style;};

	bool updateData(Table *t, const QString& colName);
	void loadData();

	QStringList plotAssociation();
	void updateColumnNames(const QString& oldName, const QString& newName, bool updateTableName);

	QPen vectorPen(){return d_pen;};
	void setVectorPen(const QPen& pen){d_pen = pen;};

private:
	void setVectorEnd(const QwtArray<double>&x, const QwtArray<double>&y);

	void draw(QPainter *painter,const QwtScaleMap &xMap,
		const QwtScaleMap &yMap, int from, int to) const;

	void drawVector(QPainter *painter, const QwtScaleMap &xMap,
		const QwtScaleMap &yMap, int from, int to) const;

	void drawArrowHead(QPainter *p, int xs, int ys, int xe, int ye) const;
		double theta(int x0, int y0, int x1, int y1) const;

protected:
	QPen d_pen;
	bool filledArrow;
	int d_style, d_headLength, d_headAngle, d_position;
	QString d_end_x_a;
	QString d_end_y_m;
	QwtArrayData *vectorEnd;
};

#endif
