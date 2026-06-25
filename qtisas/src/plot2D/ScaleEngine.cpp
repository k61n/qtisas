/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Extensions to QwtScaleEngine and QwtScaleTransformation
 ******************************************************************************/

#include <cmath>

#include <QtGlobal>

#include "LnScaleEngine.h"
#include "Log2ScaleEngine.h"
#include "LogitScaleEngine.h"
#include "ProbabilityScaleEngine.h"
#include "ReciprocalScaleEngine.h"
#include "ScaleEngine.h"

/*****************************************************************************
 *
 * Class ScaleEngineTransformation
 *
 *****************************************************************************/

ScaleEngineTransformation::ScaleEngineTransformation(double left_break, double right_break, double s1, double s2,
                                                     double breakPosFrac, double halfGapFrac,
                                                     ScaleTransformation::Type type, bool log10AfterBreak)
    : d_break_left(left_break), d_break_right(right_break), d_s1(s1), d_s2(s2), d_break_pos(breakPosFrac),
      d_half_gap(halfGapFrac), d_type(type), d_log10_after(log10AfterBreak)
{
}

bool ScaleEngineTransformation::logBeforeBreak() const
{
    return d_type == ScaleTransformation::Log10 || d_type == ScaleTransformation::Ln ||
           d_type == ScaleTransformation::Log2;
}

double ScaleEngineTransformation::transform(double s) const
{
    // Fractional position of each break edge along the axis. transform(s1) == 0,
    // transform(s2) == 1, so QwtScaleMap maps these straight onto the canvas.
    const double leftFrac = d_break_pos - d_half_gap;
    const double rightFrac = d_break_pos + d_half_gap;

    if (s <= d_break_left)
    {
        if (logBeforeBreak() && s > 0.0 && d_break_left > 0.0 && d_s1 > 0.0)
            return std::log(s / d_s1) / std::log(d_break_left / d_s1) * leftFrac;
        return (s - d_s1) / (d_break_left - d_s1) * leftFrac;
    }

    if (s >= d_break_right)
    {
        if (d_log10_after && s > 0.0 && d_break_right > 0.0 && d_s2 > 0.0)
            return rightFrac + std::log(s / d_break_right) / std::log(d_s2 / d_break_right) * (1.0 - rightFrac);
        return rightFrac + (s - d_break_right) / (d_s2 - d_break_right) * (1.0 - rightFrac);
    }

    return d_break_pos; // collapse the broken-out region onto the break centre
}

double ScaleEngineTransformation::invTransform(double p) const
{
    const double leftFrac = d_break_pos - d_half_gap;
    const double rightFrac = d_break_pos + d_half_gap;

    if (p <= leftFrac)
    {
        const double t = leftFrac > 0.0 ? p / leftFrac : 0.0;
        if (logBeforeBreak() && d_break_left > 0.0 && d_s1 > 0.0)
            return d_s1 * std::exp(t * std::log(d_break_left / d_s1));
        return d_s1 + t * (d_break_left - d_s1);
    }

    if (p >= rightFrac)
    {
        const double t = rightFrac < 1.0 ? (p - rightFrac) / (1.0 - rightFrac) : 0.0;
        if (d_log10_after && d_break_right > 0.0 && d_s2 > 0.0)
            return d_break_right * std::exp(t * std::log(d_s2 / d_break_right));
        return d_break_right + t * (d_s2 - d_break_right);
    }

    return d_break_left; // anywhere inside the gap maps back to the break
}

QwtTransform *ScaleEngineTransformation::copy() const
{
    return new ScaleEngineTransformation(d_break_left, d_break_right, d_s1, d_s2, d_break_pos, d_half_gap, d_type,
                                         d_log10_after);
}

void ScaleEngine::updateTransformation()
{
    QwtTransform *t = nullptr;
    switch (d_type)
    {
    case ScaleTransformation::Log10:
    case ScaleTransformation::Ln:
    case ScaleTransformation::Log2:
        t = new QwtLogTransform();
        break;

		case ScaleTransformation::Reciprocal:
        t = new ReciprocalScaleTransformation();
        break;

		case ScaleTransformation::Probability:
        t = new ProbabilityScaleTransformation();
        break;

		case ScaleTransformation::Logit:
        t = new LogitScaleTransformation();
        break;

		case ScaleTransformation::Linear:
		default:
        t = new QwtNullTransform();
        break;
	}
    setTransformation(t);
}

void ScaleEngine::buildTransformation()
{
    if (!hasBreak() || d_x1 >= d_x2)
    {
        updateTransformation();
        return;
    }

    double lb = qBound(d_x1, d_break_left, d_x2);
    double rb = qBound(d_x1, d_break_right, d_x2);
    if (lb > rb)
        qSwap(lb, rb);

    // A break that sits outside the range, or swallows it, has nothing to rescale.
    if (lb <= d_x1 || rb >= d_x2 || lb >= rb)
    {
        updateTransformation();
        return;
    }

    double breakPos = qBound(0.0, d_break_pos / 100.0, 1.0);
    double halfGap = d_axis_length > 0 ? double(d_break_width) / double(d_axis_length) : double(d_break_width) / 400.0;
    halfGap = qBound(0.0, halfGap, 0.45);
    // Keep both halves of the axis visible around the gap.
    breakPos = qBound(halfGap + 0.01, breakPos, 1.0 - halfGap - 0.01);

    setTransformation(
        new ScaleEngineTransformation(lb, rb, d_x1, d_x2, breakPos, halfGap, d_type, d_log10_scale_after));
}

/*****************************************************************************
 *
 * Class ScaleEngine
 *
 *****************************************************************************/

ScaleEngine::ScaleEngine(ScaleTransformation::Type type,double left_break, double right_break): QwtScaleEngine(),
d_type(type),
d_break_left(left_break),
d_break_right(right_break),
d_break_pos(50),
d_step_before(0.0),
d_step_after(0.0),
d_minor_ticks_before(1),
d_minor_ticks_after(1),
d_log10_scale_after(false),
d_break_width(4),
d_break_decoration(true)
{
    updateTransformation();
}

bool ScaleEngine::hasBreak() const
{
return (d_break_left == d_break_right || (d_break_left == -DBL_MAX && d_break_right == DBL_MAX))?false:true;
}

double ScaleEngine::axisBreakLeft() const
{
    return d_break_left;
}

double ScaleEngine::axisBreakRight() const
{
    return d_break_right;
}

int ScaleEngine::breakWidth() const
{
    return d_break_width;
}

int ScaleEngine::breakPosition() const
{
    return d_break_pos;
}

double ScaleEngine::stepBeforeBreak() const
{
    return d_step_before;
}

double ScaleEngine::stepAfterBreak() const
{
    return d_step_after;
}

ScaleTransformation::Type ScaleEngine::type() const
{
    return d_type;
}

int ScaleEngine::minTicksBeforeBreak() const
{
    return d_minor_ticks_before;
}

int ScaleEngine::minTicksAfterBreak() const
{
    return d_minor_ticks_after;
}

bool ScaleEngine::log10ScaleAfterBreak() const
{
    return d_log10_scale_after;
}

bool ScaleEngine::hasBreakDecoration() const
{
	return d_break_decoration;
}

void ScaleEngine::clone(const ScaleEngine *engine)
{
	d_type = engine->type();
	d_break_left = engine->axisBreakLeft();
	d_break_right = engine->axisBreakRight();
	d_break_pos = engine->breakPosition();
	d_step_before = engine->stepBeforeBreak();
	d_step_after = engine->stepAfterBreak();
	d_minor_ticks_before = engine->minTicksBeforeBreak();
	d_minor_ticks_after = engine->minTicksAfterBreak();
	d_log10_scale_after = engine->log10ScaleAfterBreak();
	d_break_width = engine->breakWidth();
	d_break_decoration = engine->hasBreakDecoration();
	setAttributes(engine->attributes());
	setMargins(engine->lowerMargin(), engine->upperMargin());
}

QwtScaleDiv ScaleEngine::divideScale(double x1, double x2, int maxMajSteps,
		int maxMinSteps, double stepSize) const
{
    // Capture the live scale interval and (re)build the break-aware transform so
    // the value->position mapping tracks zoom/pan/autoscale, not just setScale().
    d_x1 = qMin(x1, x2);
    d_x2 = qMax(x1, x2);
    const_cast<ScaleEngine *>(this)->buildTransformation();

	QwtScaleEngine *engine;
	if (!hasBreak()){
        engine = newScaleEngine();
		QwtScaleDiv div = engine->divideScale(x1, x2, maxMajSteps, maxMinSteps, stepSize);
		delete engine;
		return div;
	}

    double lb = d_break_left;
    double rb = d_break_right;
	double step1 = d_step_before;
	double step2 = d_step_after;
    if (x1 > x2){
        lb = d_break_right;
        rb = d_break_left;
		step1 = d_step_after;
		step2 = d_step_before;
        if (d_log10_scale_after)
            engine = new QwtLogScaleEngine();
        else
            engine = new QwtLinearScaleEngine();
    } else
		engine = newScaleEngine();

    int max_min_intervals = d_minor_ticks_before;
    if (max_min_intervals)
        max_min_intervals = d_minor_ticks_before == 1 ? 3 : d_minor_ticks_before + 1;
	QwtScaleDiv div1 = engine->divideScale(x1, lb, maxMajSteps/2, max_min_intervals, step1);

    delete engine;
    if (testAttribute(QwtScaleEngine::Inverted))
		engine = newScaleEngine();
    else if (d_log10_scale_after)
        engine = new QwtLogScaleEngine();
	else
		engine = new QwtLinearScaleEngine();

    max_min_intervals = d_minor_ticks_after;
    if (max_min_intervals)
        max_min_intervals = d_minor_ticks_after == 1 ? 3 : d_minor_ticks_after + 1;
    QwtScaleDiv div2 = engine->divideScale(rb, x2, maxMajSteps/2, max_min_intervals, step2);

    QList<double> ticks[QwtScaleDiv::NTickTypes];
    ticks[QwtScaleDiv::MinorTick] = div1.ticks(QwtScaleDiv::MinorTick) + div2.ticks(QwtScaleDiv::MinorTick);
    ticks[QwtScaleDiv::MediumTick] = div1.ticks(QwtScaleDiv::MediumTick) + div2.ticks(QwtScaleDiv::MediumTick);
    ticks[QwtScaleDiv::MajorTick] = div1.ticks(QwtScaleDiv::MajorTick) + div2.ticks(QwtScaleDiv::MajorTick);

	delete engine;
	return QwtScaleDiv(x1, x2, ticks);
}

void ScaleEngine::autoScale (int maxNumSteps, double &x1, double &x2, double &stepSize) const
{
	if (!hasBreak() || testAttribute(QwtScaleEngine::Inverted)){
		QwtScaleEngine *engine = newScaleEngine();
		engine->setAttributes(attributes());
		engine->setReference(reference());
		engine->setMargins(lowerMargin(), upperMargin());

		if (type() == ScaleTransformation::Log10 || type() == ScaleTransformation::Ln || type() == ScaleTransformation::Log2){
			if (x1 <= 0.0)
				x1 = 1e-4;
			if (x2 <= 0.0)
				x2 = 1e-3;
		}

		engine->autoScale(maxNumSteps, x1, x2, stepSize);
		delete engine;
	} else {
		QwtScaleEngine *engine = newScaleEngine();
		engine->setAttributes(attributes());
		double breakLeft = d_break_left;
		engine->autoScale(maxNumSteps, x1, breakLeft, stepSize);
		delete engine;

		engine = new QwtLinearScaleEngine();
		engine->setAttributes(attributes());
		double breakRight = d_break_right;
		engine->autoScale(maxNumSteps, breakRight, x2, stepSize);
		delete engine;
	}
}

QwtScaleEngine *ScaleEngine::newScaleEngine() const
{
	QwtScaleEngine *engine = nullptr;
	switch (d_type){
		case ScaleTransformation::Log10:
        engine = new QwtLogScaleEngine();
		break;

		case ScaleTransformation::Ln:
			engine = new LnScaleEngine();
		break;

		case ScaleTransformation::Log2:
			engine = new Log2ScaleEngine();
		break;

		case ScaleTransformation::Reciprocal:
			engine = new ReciprocalScaleEngine();
		break;

		case ScaleTransformation::Probability:
			engine = new ProbabilityScaleEngine();
		break;

		case ScaleTransformation::Logit:
			engine = new LogitScaleEngine();
		break;

		case ScaleTransformation::Linear:
		default:
			engine = new QwtLinearScaleEngine();
	}
	return engine;
}
