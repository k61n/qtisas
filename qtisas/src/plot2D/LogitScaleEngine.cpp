/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2009 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Engine for logit=ln(Y/(100-Y)) scales
 ******************************************************************************/

#include "LogitScaleEngine.h"

/*!
  Return a dummy transformation
*/
QwtScaleTransformation *LogitScaleEngine::transformation() const
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
void LogitScaleEngine::autoScale(int,
    double &x1, double &x2, double &stepSize) const
{
    if ( x1 > x2 )
        qSwap(x1, x2);

    QwtDoubleInterval interval(0.1, 99.99);

    if (interval.width() == 0.0)
        interval = buildInterval(interval.minValue());

    stepSize = 1.0;

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
QwtScaleDiv LogitScaleEngine::divideScale(double x1, double x2,
    int, int, double stepSize) const
{
    QwtDoubleInterval interval = QwtDoubleInterval(x1, x2).normalized();
    interval = interval.limited(0.1, 99.99);

    if (interval.width() <= 0 )
        return QwtScaleDiv();

    stepSize = fabs(qRound(stepSize));
    if ( stepSize == 0.0 )
        stepSize = 1.0;

    QwtScaleDiv scaleDiv;
    if ( stepSize != 0.0 ){
        QwtValueList ticks[QwtScaleDiv::NTickTypes];
		buildTicks(interval, (int)stepSize, ticks);
        scaleDiv = QwtScaleDiv(interval, ticks);
    }

    if ( x1 > x2 )
        scaleDiv.invert();

    return scaleDiv;
}

void LogitScaleEngine::buildTicks(
    const QwtDoubleInterval& interval, int stepSize,
    QwtValueList ticks[QwtScaleDiv::NTickTypes]) const
{
    ticks[QwtScaleDiv::MajorTick] = buildMajorTicks(interval, stepSize);
    ticks[QwtScaleDiv::MinorTick] = QwtValueList();

    for ( int i = 0; i < QwtScaleDiv::NTickTypes; i++ )
        ticks[i] = strip(ticks[i], interval);
}

QwtValueList LogitScaleEngine::buildMajorTicks(
	const QwtDoubleInterval &, int stepSize) const
{
	QwtValueList baseTicks;

	baseTicks += 0.01;
	baseTicks += 0.1;
	baseTicks += 1;
	baseTicks += 5;
	baseTicks += 10;
	baseTicks += 25;
	baseTicks += 50;

	QwtValueList ticks;

	int size = baseTicks.size();
	for (int i = 0; i < size; i += stepSize)
		ticks += baseTicks[i];

	size = ticks.size();
	for (int i = 1; i < size; i++)
		ticks += 100 - ticks[size - i];

    return ticks;
}

//! Create a clone of the transformation
QwtScaleTransformation *LogitScaleTransformation::copy() const
{
	return new LogitScaleTransformation(d_engine);
}

double LogitScaleTransformation::xForm(
    double s, double s1, double s2, double p1, double p2) const
{
	return p1 + (p2 - p1) * (func(s) - func(s1))/(func(s2) - func(s1));
}

double LogitScaleTransformation::invXForm(double p, double p1, double p2,
    double s1, double s2) const
{
	return invFunc(func(s1) + (p - p1)/(p2 - p1)*(func(s2) - func(s1)));
}

double LogitScaleTransformation::func(double x) const
{
	return log(x/(100-x));
}

double LogitScaleTransformation::invFunc(double x) const
{
	return 100*exp(x)/(1 + exp(x));
}
