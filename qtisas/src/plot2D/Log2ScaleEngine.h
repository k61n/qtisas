/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Engine for log2 scales
 ******************************************************************************/

#ifndef LOG2SCALEENGINE_H
#define LOG2SCALEENGINE_H

#include <qwt/qwt_scale_engine.h>
#include <qwt/qwt_interval.h>

class Log2ScaleEngine: public QwtScaleEngine
{
public:
    virtual void autoScale(int maxSteps,
        double &x1, double &x2, double &stepSize) const;

    virtual QwtScaleDiv divideScale(double x1, double x2,
        int numMajorSteps, int numMinorSteps,
        double stepSize = 0.0) const;

    virtual QwtTransform *transformation() const;

protected:
    static QwtInterval log2(const QwtInterval &);

private:
    static QwtInterval align(const QwtInterval &, double stepSize);

    void buildTicks(const QwtInterval &, double stepSize, int maxMinSteps,
                    QList<double> ticks[QwtScaleDiv::NTickTypes]) const;

    static QList<double> buildMinorTicks(const QList<double> &majorTicks, int maxMinMark, double step);

    static QList<double> buildMajorTicks(const QwtInterval &interval, double stepSize);
};

#endif
