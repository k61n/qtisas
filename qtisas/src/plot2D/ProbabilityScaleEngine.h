/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Engine for normal probability scales
 ******************************************************************************/

#ifndef PROBABILITYSCALEENGINE_H
#define PROBABILITYSCALEENGINE_H

#include <qwt/qwt_scale_engine.h>
#include <qwt/qwt_transform.h>

class ProbabilityScaleTransformation : public QwtTransform
{
public:
    double transform(double value) const override;
    double invTransform(double value) const override;
    QwtTransform *copy() const override;
};

class ProbabilityScaleEngine: public QwtScaleEngine
{
public:
    virtual void autoScale(int,
        double &x1, double &x2, double &stepSize) const;

    virtual QwtScaleDiv divideScale(double x1, double x2,
        int numMajorSteps, int numMinorSteps,
        double stepSize = 0.0) const;

    virtual QwtTransform *transformation() const;

private:
    void buildTicks(const QwtInterval &, int stepSize, QList<double> ticks[QwtScaleDiv::NTickTypes]) const;

    static QList<double> buildMajorTicks(const QwtInterval &interval, int stepSize);
};

#endif
