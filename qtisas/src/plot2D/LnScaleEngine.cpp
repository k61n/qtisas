/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2009 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Engine for ln scales
 ******************************************************************************/

#include "LnScaleEngine.h"

/*!
    Return a dummy transformation
*/
QwtScaleTransformation *LnScaleEngine::transformation() const
{
    return new QwtScaleTransformation(QwtScaleTransformation::Other);
}

/*!
    Align and divide an interval

   \param maxNumSteps Max. number of steps
   \param x1 First limit of the interval (In/Out)
   \param x2 Second limit of the interval (In/Out)
   \param stepSize Step size (Out)
*/
void LnScaleEngine::autoScale(int maxNumSteps,
    double &x1, double &x2, double &stepSize) const
{
    if ( x1 > x2 )
        qSwap(x1, x2);

    QwtDoubleInterval interval(x1 / exp(lowerMargin()),
        x2 * exp(upperMargin()) );

    double logRef = 1.0;
    if (reference() > LOG_MIN / 2)
        logRef = qwtMin(reference(), LOG_MAX / 2);

    if (testAttribute(QwtScaleEngine::Symmetric)){
        const double delta = qwtMax(interval.maxValue() / logRef,
            logRef / interval.minValue());
        interval.setInterval(logRef / delta, logRef * delta);
    }

    if (testAttribute(QwtScaleEngine::IncludeReference))
        interval = interval.extend(logRef);

    interval = interval.limited(LOG_MIN, LOG_MAX);

    if (interval.width() == 0.0)
        interval = buildInterval(interval.minValue());

    stepSize = divideInterval(ln(interval).width(), qwtMax(maxNumSteps, 1));

    if (!testAttribute(QwtScaleEngine::Floating))
        interval = align(interval, stepSize);

    x1 = interval.minValue();
    x2 = interval.maxValue();

    if (testAttribute(QwtScaleEngine::Inverted)){
        qSwap(x1, x2);
        stepSize = -stepSize;
    }
}

/*!
   \brief Calculate a scale division

   \param x1 First interval limit
   \param x2 Second interval limit
   \param maxMajSteps Maximum for the number of major steps
   \param maxMinSteps Maximum number of minor steps
   \param stepSize Step size. If stepSize == 0, the scaleEngine
                   calculates one.
*/
QwtScaleDiv LnScaleEngine::divideScale(double x1, double x2,
    int maxMajSteps, int maxMinSteps, double stepSize) const
{
    QwtDoubleInterval interval = QwtDoubleInterval(x1, x2).normalized();
    interval = interval.limited(LOG_MIN, LOG_MAX);

    if (interval.width() <= 0 )
        return QwtScaleDiv();

    if (interval.maxValue() / interval.minValue() < exp(1)){
        // scale width is less than one exp -> build linear scale
        QwtLinearScaleEngine linearScaler;
        linearScaler.setAttributes(attributes());
        linearScaler.setReference(reference());
        linearScaler.setMargins(lowerMargin(), upperMargin());

        return linearScaler.divideScale(x1, x2,
            maxMajSteps, maxMinSteps, stepSize);
    }

    stepSize = qwtAbs(stepSize);
    if ( stepSize == 0.0 ){
        if ( maxMajSteps < 1 )
            maxMajSteps = 1;

		stepSize = ceil(ln(interval).width()/double(maxMajSteps));
    }

    QwtScaleDiv scaleDiv;
    if ( stepSize != 0.0 ){
        QwtValueList ticks[QwtScaleDiv::NTickTypes];
		buildTicks(interval, stepSize, maxMinSteps, ticks);
        scaleDiv = QwtScaleDiv(interval, ticks);
    }

    if ( x1 > x2 )
        scaleDiv.invert();

    return scaleDiv;
}

void LnScaleEngine::buildTicks(
    const QwtDoubleInterval& interval, double stepSize, int maxMinSteps,
    QwtValueList ticks[QwtScaleDiv::NTickTypes]) const
{
    const QwtDoubleInterval boundingInterval = align(interval, stepSize);

    ticks[QwtScaleDiv::MajorTick] = buildMajorTicks(boundingInterval, stepSize);
    if ( maxMinSteps > 0 )
        ticks[QwtScaleDiv::MinorTick] = buildMinorTicks(ticks[QwtScaleDiv::MajorTick], maxMinSteps, stepSize);

    for ( int i = 0; i < QwtScaleDiv::NTickTypes; i++ )
        ticks[i] = strip(ticks[i], interval);
}

QwtValueList LnScaleEngine::buildMajorTicks(
    const QwtDoubleInterval &interval, double stepSize) const
{
    double width = ln(interval).width();

    int numTicks = qRound(width / stepSize) + 1;
    if ( numTicks > 10000 )
        numTicks = 10000;

    const double lxmin = log(interval.minValue());
    const double lxmax = log(interval.maxValue());
    const double lstep = (lxmax - lxmin) / double(numTicks - 1);

    QwtValueList ticks;

    ticks += interval.minValue();

    for (int i = 1; i < numTicks; i++)
       ticks += exp(lxmin + double(i) * lstep);

    ticks += interval.maxValue();

    return ticks;
}

QwtValueList LnScaleEngine::buildMinorTicks(const QwtValueList &majorTicks,
    int maxMinSteps, double) const
{
	if ( maxMinSteps < 1 )
		return QwtValueList();

	int majTicks = (int)majorTicks.count();
    if (majTicks > 1){
        QwtValueList minorTicks;
		for (int i = 0; i < majTicks - 1; i++){
			const double v = majorTicks[i];
            const double dv = fabs(majorTicks[i + 1] - majorTicks[i])/double(maxMinSteps - 1);
            for (int j = 0; j < maxMinSteps; j++)
                minorTicks += v + j*dv;
        }
        return minorTicks;
    }
    return QwtValueList();
}

/*!
  \brief Align an interval to a step size

  The limits of an interval are aligned that both are integer
  multiples of the step size.

  \param interval Interval
  \param stepSize Step size

  \return Aligned interval
*/
QwtDoubleInterval LnScaleEngine::align(
    const QwtDoubleInterval &interval, double stepSize) const
{
    const QwtDoubleInterval intv = ln(interval);

    const double x1 = QwtScaleArithmetic::floorEps(intv.minValue(), stepSize);
    const double x2 = QwtScaleArithmetic::ceilEps(intv.maxValue(), stepSize);

    return QwtDoubleInterval(exp(x1), exp(x2));
}

/*!
  Return the interval [log(interval.minValue(), log(interval.maxValue]
*/

QwtDoubleInterval LnScaleEngine::ln(
    const QwtDoubleInterval &interval) const
{
    return QwtDoubleInterval(::log(interval.minValue()),
            ::log(interval.maxValue()));
}
