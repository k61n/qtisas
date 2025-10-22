/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Error bars curve
 ******************************************************************************/

#ifndef ERRORBARS_H
#define ERRORBARS_H

#include <qwt/qwt_plot.h>

#include "PlotCurve.h"

//! Error bars curve
class ErrorBarsCurve: public DataCurve
{
public:
	enum Orientation{Horizontal = 0, Vertical = 1};

	ErrorBarsCurve(int orientation, Table *t, const QString& name);
	ErrorBarsCurve(Table *t, const QString& name);

	void copy(const ErrorBarsCurve *e);

	QwtDoubleRect boundingRect() const;

	double errorValue(int i);
	QwtArray<double> errors(){return err;};
	void setErrors(const QwtArray<double>&data){err=data;};

	int capLength(){return d_cap_length;};
	void setCapLength(int t){d_cap_length = t;};

	double width(){return pen().widthF();};
    void setWidth(double w, bool cosmetic = true);

	QColor color(){return pen().color();};
	void setColor(const QColor& c);

	int direction(){return type;};
	void setDirection(int o){type = o;};

	bool xErrors();
	void setXErrors(bool yes);

	bool throughSymbol(){return through;};
	void drawThroughSymbol(bool yes){through=yes;};

	bool plusSide(){return plus;};
	void drawPlusSide(bool yes){plus=yes;};

	bool minusSide(){return minus;};
	void drawMinusSide(bool yes){minus=yes;};

	//! Returns the master curve to which this error bars curve is attached.
	DataCurve* masterCurve(){return d_master_curve;};
	void setMasterCurve(DataCurve *c);

	//! Causes the master curve to delete this curve from its managed error bars list.
	void detachFromMasterCurve(){d_master_curve->removeErrorBars(this);};

	QStringList plotAssociation();

	bool updateData(Table *t, const QString& colName);
	void loadData();

private:
	virtual void draw(QPainter *painter,const QwtScaleMap &xMap,
		const QwtScaleMap &yMap, int from, int to) const;

	void drawErrorBars(QPainter *painter, const QwtScaleMap &xMap,
		const QwtScaleMap &yMap, int from, int to) const;

	void init();

    //! Stores the error bar values
    QwtArray<double> err;

	//! Orientation of the bars: Horizontal or Vertical
	int type;

	//! Length of the bar cap decoration
	int d_cap_length;

	bool plus, minus, through;

	//! Reference to the master curve to which this error bars curve is attached.
	DataCurve *d_master_curve;
};

#endif
