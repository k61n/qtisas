/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A wrapper around QwtLinearColorMap from Qwt
 ******************************************************************************/

#ifndef LINEARCOLORMAP_H
#define LINEARCOLORMAP_H

#include <qwt/qwt_color_map.h>
#include <qwt/qwt_interval.h>

//! A customized QwtLinearColorMap.
/**
 *
 */
class LinearColorMap : public QwtLinearColorMap
{
public:
	//! Constructor.
	LinearColorMap();
    explicit LinearColorMap(LinearColorMap *colorMap);
	LinearColorMap(const QColor &from, const QColor &to);
	//! Set the intensity range
    void setIntensityRange(const QwtInterval &range)
    {
        d_range = range;
    }
	//! Set the intensity range
    void setIntensityRange(double vmin, double vmax)
    {
        d_range = QwtInterval(vmin, vmax);
    }
	//! Get the intensity range
    QwtInterval intensityRange() const;
	//! Get the lower range limit
	double lowerBound(){return d_range.minValue();}
	//! Get the upper range limit
	double upperBound(){return d_range.maxValue();}

    QColor color(int index) const;

	//! Exports the map to a pseudo-XML string
	QString toXmlString();
	//! Used when restoring from project files
    static LinearColorMap *fromXmlStringList(const QStringList &lst);

private:
    QwtInterval d_range;
};

#endif
