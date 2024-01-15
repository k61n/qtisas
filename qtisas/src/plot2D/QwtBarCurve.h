/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Bar curve
 ******************************************************************************/

#ifndef BARCURVE_H
#define BARCURVE_H

#include "PlotCurve.h"
#include <qwt_plot.h>

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
