/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Bar curve
 ******************************************************************************/

#ifndef BARCURVE_H
#define BARCURVE_H

#include <qwt/qwt_plot.h>

#include "PlotCurve.h"

//! Bar curve
class QwtBarCurve: public DataCurve
{
public:
	enum BarStyle{Vertical = 0, Horizontal = 1};
	QwtBarCurve(BarStyle style, Table *t, const QString& xColName, const QString& name, int startRow, int endRow);

	void copy(QwtBarCurve *b);

	virtual QwtDoubleRect boundingRect() const;

	BarStyle orientation(){return bar_style;};

	void setGap (int gap);
	int gap() const {return bar_gap;};

	void setOffset(int offset);
	int offset() const {return bar_offset;};

	double dataOffset();
    double dataGap();

	bool isStacked(){return d_is_stacked;};
	void setStacked(bool on = true){d_is_stacked = on;};

	virtual QString saveToString();

	QList <QwtBarCurve *> stackedCurvesList() const;
	double stackOffset(int i, QList <QwtBarCurve *> stack) const;

private:
	virtual void draw(QPainter *painter,const QwtScaleMap &xMap,
		const QwtScaleMap &yMap, int from, int to) const;

protected:
	int bar_offset;
	int bar_gap;
	bool d_is_stacked;
	BarStyle bar_style;
};

#endif
