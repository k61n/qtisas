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
#include <qwt/qwt_transform.h>
#include <qwt/qwt_interval.h>

class ReciprocalScaleTransformation : public QwtTransform
{
public:
    double transform(double value) const override;
    double invTransform(double value) const override;
    QwtTransform *copy() const override;
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

    virtual QwtTransform *transformation() const;

protected:
    static QwtInterval reciprocal(const QwtInterval &);

private:
    static QwtInterval align(const QwtInterval &, double stepSize);

    void buildTicks(const QwtInterval &, double stepSize, int maxMinSteps,
                    QList<double> ticks[QwtScaleDiv::NTickTypes]) const;

    static QList<double> buildMinorTicks(const QList<double> &majorTicks, int maxMinSteps, double stepSize);

    static QList<double> buildMajorTicks(const QwtInterval &interval, double stepSize);
};

#endif
