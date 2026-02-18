/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2009 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Return a transformation for reciprocal (1/t) scales
 ******************************************************************************/

#include <cmath>

#include "ReciprocalScaleEngine.h"

QwtTransform *ReciprocalScaleEngine::transformation() const
{
    return new ReciprocalScaleTransformation();
}

void ReciprocalScaleEngine::autoScale(int maxNumSteps,
    double &x1, double &x2, double &stepSize) const
{
    if (x1 > x2)
        qSwap(x1, x2);

    if (x1 <= 0.0)
        x1 = 1e-4;
    if (x2 <= 0.0)
        x2 = 1e-3;

    QwtInterval interval(x1 / exp(lowerMargin()), x2 * exp(upperMargin()));

    double ref = 1.0;
    if (reference() > 0.0)
        ref = reference();

    if (testAttribute(QwtScaleEngine::Symmetric))
    {
        const double delta = qMax(interval.maxValue() / ref, ref / interval.minValue());
        interval.setInterval(ref / delta, ref * delta);
    }

    if (testAttribute(QwtScaleEngine::IncludeReference))
        interval = interval.extend(ref);

    if (interval.width() == 0.0)
        interval = buildInterval(interval.minValue());

    stepSize = divideInterval(reciprocal(interval).width(), qMax(maxNumSteps, 1));

    if (!testAttribute(QwtScaleEngine::Floating))
        interval = align(interval, stepSize);

    x1 = interval.minValue();
    x2 = interval.maxValue();

    if (testAttribute(QwtScaleEngine::Inverted))
    {
        qSwap(x1, x2);
        stepSize = -stepSize;
    }
}

QwtScaleDiv ReciprocalScaleEngine::divideScale(double x1, double x2, int maxMajSteps, int maxMinSteps,
                                               double stepSize) const
{
    QwtInterval interval = QwtInterval(x1, x2).normalized();

    if (interval.minValue() <= 0.0)
        interval.setMinValue(1e-100);

    if (interval.width() <= 0)
        return QwtScaleDiv();

    if (interval.maxValue() / interval.minValue() < 2)
    {
        QwtLinearScaleEngine linearScaler;
        linearScaler.setAttributes(attributes());
        linearScaler.setReference(reference());
        linearScaler.setMargins(lowerMargin(), upperMargin());

        return linearScaler.divideScale(x1, x2, maxMajSteps, maxMinSteps, stepSize);
    }

    stepSize = qAbs(stepSize);
    if (stepSize == 0.0)
    {
        if (maxMajSteps < 1)
            maxMajSteps = 1;
        stepSize = divideInterval(reciprocal(interval).width(), maxMajSteps);
    }

    QwtScaleDiv scaleDiv;
    if (stepSize != 0.0)
    {
        QList<double> ticks[QwtScaleDiv::NTickTypes];
        buildTicks(interval, stepSize, maxMinSteps, ticks);
        scaleDiv = QwtScaleDiv(interval, ticks);
    }

    if (x1 > x2)
        scaleDiv.invert();

    return scaleDiv;
}

void ReciprocalScaleEngine::buildTicks(const QwtInterval &interval, double stepSize, int maxMinSteps,
                                       QList<double> ticks[QwtScaleDiv::NTickTypes]) const
{
    const QwtInterval boundingInterval = align(interval, stepSize);

    ticks[QwtScaleDiv::MajorTick] = buildMajorTicks(boundingInterval, stepSize);
    if (maxMinSteps > 0)
        ticks[QwtScaleDiv::MinorTick] = buildMinorTicks(ticks[QwtScaleDiv::MajorTick], maxMinSteps, stepSize);

    for (int i = 0; i < QwtScaleDiv::NTickTypes; i++)
        ticks[i] = strip(ticks[i], interval);
}

QList<double> ReciprocalScaleEngine::buildMajorTicks(const QwtInterval &interval, double stepSize)
{
    double width = reciprocal(interval).width();

    int numTicks = qRound(width / stepSize) + 1;
    if (numTicks > 10000)
        numTicks = 10000;

    // Work in reciprocal space: [1/maxValue, 1/minValue]
    const double rmin = 1.0 / interval.maxValue();
    const double rmax = 1.0 / interval.minValue();
    const double rstep = (rmax - rmin) / double(numTicks - 1);

    QList<double> ticks;

    ticks += interval.minValue();

    // Iterate from rmax downward so data-space values increase
    for (int i = 1; i < numTicks; i++)
        ticks += 1.0 / (rmax - double(i) * rstep);

    ticks += interval.maxValue();

    return ticks;
}

QList<double> ReciprocalScaleEngine::buildMinorTicks(const QList<double> &majorTicks, int maxMinSteps, double)
{
    if (maxMinSteps < 1)
        return {};

    int majTicks = (int)majorTicks.count();
    if (majTicks > 1)
    {
        QList<double> minorTicks;
        for (int i = 0; i < majTicks - 1; i++)
        {
            // Space minor ticks evenly in reciprocal space
            const double r1 = 1.0 / majorTicks[i];
            const double r2 = 1.0 / majorTicks[i + 1];
            const double dr = (r1 - r2) / double(maxMinSteps);
            for (int j = 1; j < maxMinSteps; j++)
                minorTicks += 1.0 / (r1 - j * dr);
        }
        return minorTicks;
    }
    return {};
}

QwtInterval ReciprocalScaleEngine::align(const QwtInterval &interval, double stepSize)
{
    const QwtInterval intv = reciprocal(interval);

    const double x1 = QwtScaleArithmetic::floorEps(intv.minValue(), stepSize);
    const double x2 = QwtScaleArithmetic::ceilEps(intv.maxValue(), stepSize);

    // Transform back; 1/x reverses order so swap
    return {1.0 / x2, 1.0 / x1};
}

/*!
  Return the interval [1/maxValue, 1/minValue] (normalized in reciprocal space)
*/
QwtInterval ReciprocalScaleEngine::reciprocal(const QwtInterval &interval)
{
    return {1.0 / interval.maxValue(), 1.0 / interval.minValue()};
}

//! Create a clone of the transformation
QwtTransform *ReciprocalScaleTransformation::copy() const
{
    return new ReciprocalScaleTransformation();
}

double ReciprocalScaleTransformation::transform(double s) const
{
    if (s == 0.0)
        return 0.0;
    return 1.0 / s;
}

double ReciprocalScaleTransformation::invTransform(double s) const
{
    if (s == 0.0)
        return 0.0;
    return 1.0 / s;
}
