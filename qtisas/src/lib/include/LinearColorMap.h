/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A wrapper around QwtLinearColorMap from Qwt
 ******************************************************************************/

#ifndef LINEAR_COLOR_MAP_H
#define LINEAR_COLOR_MAP_H

#include "compat.h"

#include <qwt/qwt_color_map.h>

//! A customized QwtLinearColorMap.
/**
 *
 */
class LinearColorMap : public QwtLinearColorMap
{
public:
	//! Constructor.
	LinearColorMap();
	//! Constructor.
	LinearColorMap(const QColor &from, const QColor &to);
	//! Set the intensity range
	void setIntensityRange(const QwtDoubleInterval& range){d_range = range;}
	//! Set the intensity range
	void setIntensityRange(double vmin, double vmax){d_range = QwtDoubleInterval(vmin, vmax);}
	//! Get the intensity range
	QwtDoubleInterval intensityRange() const;
	//! Get the lower range limit
	double lowerBound(){return d_range.minValue();}
	//! Get the upper range limit
	double upperBound(){return d_range.maxValue();}


	//! Exports the map to a pseudo-XML string
	QString toXmlString();
	//! Used when restoring from project files
	static LinearColorMap fromXmlStringList(const QStringList& lst);

private:
	QwtDoubleInterval d_range;
};

#endif
