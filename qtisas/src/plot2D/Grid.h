/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: 2D Grid class
 ******************************************************************************/

#ifndef GRID_H
#define GRID_H

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_marker.h>

class Grid : public QwtPlotGrid
{
public:
    Grid();

    bool xZeroLineEnabled(){return (mrkX != nullptr)?true:false;};
    void enableZeroLineX(bool enable = true);
    bool yZeroLineEnabled(){return (mrkY != nullptr)?true:false;};
    void enableZeroLineY(bool enable = true);

	void setMajPenX(const QPen &p){	setMajPen(p);};
	const QPen& majPenX() const {return majPen();};

	void setMinPenX(const QPen &p){	setMinPen(p);};
	const QPen& minPenX() const {return minPen();};

	void setMajPenY(const QPen &p){	if (d_maj_pen_y != p) d_maj_pen_y = p;};
	const QPen& majPenY() const {return d_maj_pen_y;};

	void setMinPenY(const QPen &p){	if (d_min_pen_y != p) d_min_pen_y = p;};
	const QPen& minPenY() const {return d_min_pen_y;};

	void load(const QStringList& );
	void copy(Grid *);
	QString saveToString();

	QwtPlotMarker *xZeroLineMarker(){return mrkX;};
	QwtPlotMarker *yZeroLineMarker(){return mrkY;};

    QPen xZeroLinePen();
	void setXZeroLinePen(const QPen &p);
    QPen yZeroLinePen();
	void setYZeroLinePen(const QPen &p);

private:
	void draw (QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &rect) const;
	void drawLines(QPainter *painter, const QRect &rect, Qt::Orientation orientation, const QwtScaleMap &map,
    	const QwtValueList &values) const;

	QPen d_maj_pen_y;
	QPen d_min_pen_y;

	QwtPlotMarker *mrkX, *mrkY;//x=0 et y=0 line markers
};

#endif
