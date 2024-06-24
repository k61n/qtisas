/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Return a transformation for reciprocal (1/t) scales
 ******************************************************************************/

#ifndef RECIPROCALSCALEENGINE_H
#define RECIPROCALSCALEENGINE_H

#include <qwt/qwt_scale_engine.h>
#include <qwt/qwt_scale_map.h>

#include "ScaleEngine.h"

class ReciprocalScaleTransformation: public ScaleTransformation
{
public:
	ReciprocalScaleTransformation(const ScaleEngine *engine):ScaleTransformation(engine){};
	virtual double xForm(double x, double, double, double p1, double p2) const;
	virtual double invXForm(double x, double s1, double s2, double p1, double p2) const;
	QwtScaleTransformation* copy() const;
};

/*!
  \brief A scale engine for reciprocal (1/t) scales
*/

class ReciprocalScaleEngine: public QwtScaleEngine
{
public:
    virtual void autoScale(int maxSteps,
        double &x1, double &x2, double &stepSize) const;

    virtual QwtScaleDiv divideScale(double x1, double x2,
        int numMajorSteps, int numMinorSteps,
        double stepSize = 0.0) const;

    virtual QwtScaleTransformation *transformation() const;

protected:
    QwtDoubleInterval align(const QwtDoubleInterval&,
        double stepSize) const;

private:
    void buildTicks(
        const QwtDoubleInterval &, double stepSize, int maxMinSteps,
        QwtValueList ticks[QwtScaleDiv::NTickTypes]) const;

    void buildMinorTicks(
        const QwtValueList& majorTicks,
        int maxMinMark, double step,
        QwtValueList &, QwtValueList &) const;

    QwtValueList buildMajorTicks(
        const QwtDoubleInterval &interval, double stepSize) const;
};

#endif
