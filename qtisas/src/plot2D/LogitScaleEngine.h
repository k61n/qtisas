/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Engine for logit scales
 ******************************************************************************/

#ifndef LOGITSCALEENGINE_H
#define LOGITSCALEENGINE_H

#include <qwt/qwt_scale_engine.h>
#include <qwt/qwt_scale_map.h>

#include "ScaleEngine.h"

class LogitScaleTransformation: public ScaleTransformation
{
public:
	LogitScaleTransformation(const ScaleEngine *engine):ScaleTransformation(engine){};
	virtual double xForm(double x, double, double, double p1, double p2) const;
	virtual double invXForm(double x, double s1, double s2, double p1, double p2) const;
	QwtScaleTransformation* copy() const;

private:
	double func(double x) const;
	double invFunc(double x) const;
};

class LogitScaleEngine: public QwtScaleEngine
{
public:
    virtual void autoScale(int,
        double &x1, double &x2, double &stepSize) const;

    virtual QwtScaleDiv divideScale(double x1, double x2,
        int numMajorSteps, int numMinorSteps,
        double stepSize = 0.0) const;

    virtual QwtScaleTransformation *transformation() const;

private:
    void buildTicks(const QwtDoubleInterval &, int stepSize,
        QwtValueList ticks[QwtScaleDiv::NTickTypes]) const;

    QwtValueList buildMajorTicks(
        const QwtDoubleInterval &interval, int stepSize) const;
};

#endif
