/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2009 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Engine for log2 scales
 ******************************************************************************/

#ifndef LOG2_SCALE_ENGINE_H
#define LOG2_SCALE_ENGINE_H

#include <qwt_scale_engine.h>
#include <qwt_scale_map.h>

class Log2ScaleEngine: public QwtScaleEngine
{
public:
    virtual void autoScale(int maxSteps,
        double &x1, double &x2, double &stepSize) const;

    virtual QwtScaleDiv divideScale(double x1, double x2,
        int numMajorSteps, int numMinorSteps,
        double stepSize = 0.0) const;

    virtual QwtScaleTransformation *transformation() const;

protected:
    QwtDoubleInterval log2(const QwtDoubleInterval&) const;

private:
    QwtDoubleInterval align(const QwtDoubleInterval&,
        double stepSize) const;

    void buildTicks(
        const QwtDoubleInterval &, double stepSize, int maxMinSteps,
        QwtValueList ticks[QwtScaleDiv::NTickTypes]) const;

    QwtValueList buildMinorTicks(const QwtValueList& majorTicks,
        int maxMinMark, double step) const;

    QwtValueList buildMajorTicks(
        const QwtDoubleInterval &interval, double stepSize) const;
};

#endif
