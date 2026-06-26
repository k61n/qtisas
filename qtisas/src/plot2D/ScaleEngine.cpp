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
                                                     ScaleTransformation::Type beforeBreak,
                                                     ScaleTransformation::Type afterBreak)
    : d_break_left(left_break), d_break_right(right_break), d_s1(s1), d_s2(s2), d_break_pos(breakPosFrac),
      d_half_gap(halfGapFrac), d_before_type(beforeBreak), d_after_type(afterBreak),
      d_before(ScaleEngine::newTransform(beforeBreak)), d_after(ScaleEngine::newTransform(afterBreak))
{
}

ScaleEngineTransformation::~ScaleEngineTransformation()
{
    delete d_before;
    delete d_after;
}

double ScaleEngineTransformation::frac(const QwtTransform *t, double s, double a, double b)
{
    // Map s within [a, b] to [0, 1] in transformed space, using each half's own
    // scale transform (linear, log, reciprocal, ...). bounded() keeps values in
    // the transform's valid domain (e.g. positive for log).
    const double ta = t->transform(t->bounded(a));
    const double tb = t->transform(t->bounded(b));
    if (tb == ta)
        return 0.0;
    return (t->transform(t->bounded(s)) - ta) / (tb - ta);
}

double ScaleEngineTransformation::transform(double s) const
{
    // Fractional position of each break edge along the axis. transform(s1) == 0,
    // transform(s2) == 1, so QwtScaleMap maps these straight onto the canvas.
    const double leftFrac = d_break_pos - d_half_gap;
    const double rightFrac = d_break_pos + d_half_gap;

    if (s <= d_break_left)
        return frac(d_before, s, d_s1, d_break_left) * leftFrac;

    if (s >= d_break_right)
        return rightFrac + frac(d_after, s, d_break_right, d_s2) * (1.0 - rightFrac);

    return d_break_pos; // collapse the broken-out region onto the break centre
}

double ScaleEngineTransformation::invTransform(double p) const
{
    const double leftFrac = d_break_pos - d_half_gap;
    const double rightFrac = d_break_pos + d_half_gap;

    if (p <= leftFrac)
    {
        const double t = leftFrac > 0.0 ? p / leftFrac : 0.0;
        const double ta = d_before->transform(d_before->bounded(d_s1));
        const double tb = d_before->transform(d_before->bounded(d_break_left));
        return d_before->invTransform(ta + t * (tb - ta));
    }

    if (p >= rightFrac)
    {
        const double t = rightFrac < 1.0 ? (p - rightFrac) / (1.0 - rightFrac) : 0.0;
        const double ta = d_after->transform(d_after->bounded(d_break_right));
        const double tb = d_after->transform(d_after->bounded(d_s2));
        return d_after->invTransform(ta + t * (tb - ta));
    }

    return d_break_left; // anywhere inside the gap maps back to the break
}

QwtTransform *ScaleEngineTransformation::copy() const
{
    return new ScaleEngineTransformation(d_break_left, d_break_right, d_s1, d_s2, d_break_pos, d_half_gap,
                                         d_before_type, d_after_type);
}

void ScaleEngine::updateTransformation()
{
    setTransformation(newTransform(d_type));
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

    setTransformation(new ScaleEngineTransformation(lb, rb, d_x1, d_x2, breakPos, halfGap, d_type, d_type_after));
}

/*****************************************************************************
 *
 * Class ScaleEngine
 *
 *****************************************************************************/

ScaleEngine::ScaleEngine(ScaleTransformation::Type type, double left_break, double right_break)
    : QwtScaleEngine(), d_type(type), d_break_left(left_break), d_break_right(right_break), d_break_pos(50),
      d_step_before(0.0), d_step_after(0.0), d_minor_ticks_before(1), d_minor_ticks_after(1), d_type_after(type),
      d_break_width(4), d_break_decoration(true)
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

ScaleTransformation::Type ScaleEngine::typeAfterBreak() const
{
    return d_type_after;
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
    d_type_after = engine->typeAfterBreak();
	d_break_width = engine->breakWidth();
	d_break_decoration = engine->hasBreakDecoration();
	setAttributes(engine->attributes());
	setMargins(engine->lowerMargin(), engine->upperMargin());
}

int ScaleEngine::minorStepsForType(int minorTicks, ScaleTransformation::Type type)
{
    if (minorTicks <= 0)
        return 0;

    // The engines subdivide each major interval into maxMinSteps parts and draw
    // the interior boundaries, i.e. (maxMinSteps - 1) minor ticks. Asking for one
    // more subdivision than the wanted tick count makes the displayed number match
    // what the user typed. Linear/Log10 go through Qwt's nice-number rounding, so a
    // lone minor tick needs a little extra headroom to survive it.
    if (type == ScaleTransformation::Linear || type == ScaleTransformation::Log10)
        return minorTicks == 1 ? 3 : minorTicks + 1;
    return minorTicks + 1;
}

double ScaleEngine::logBaseForType(ScaleTransformation::Type type)
{
    switch (type)
    {
    case ScaleTransformation::Log10:
        return 10.0;
    case ScaleTransformation::Ln:
        return std::exp(1.0);
    case ScaleTransformation::Log2:
        return 2.0;
    default:
        return 0.0;
    }
}

QwtScaleDiv ScaleEngine::divideHalf(QwtScaleEngine *engine, ScaleTransformation::Type type, double x1, double x2,
                                    int maxMajSteps, int maxMinSteps, double stepSize)
{
    QwtScaleDiv div = engine->divideScale(x1, x2, maxMajSteps, maxMinSteps, stepSize);

    const double base = logBaseForType(type);
    const double lo = qMin(x1, x2);
    const double hi = qMax(x1, x2);
    if (base <= 1.0 || lo <= 0.0 || hi <= 0.0 || hi / lo >= base)
        return div; // not a log half, or wide enough that Qwt already spaces it logarithmically

    // The range spans less than one base, so QwtLogScaleEngine (and friends) fell
    // back to a linear division: its minor ticks are evenly, i.e. proportionally,
    // spaced. Re-divide over the enclosing base-aligned interval to recover proper
    // logarithmic minor ticks, then strip them back to the real range. The major
    // ticks are kept from the linear fallback so the half still carries labels.
    const double lb = std::log(base);
    const double elo = std::pow(base, std::floor(std::log(lo) / lb));
    const double ehi = std::pow(base, std::ceil(std::log(hi) / lb));
    const QwtScaleDiv ext = engine->divideScale(elo, ehi, maxMajSteps, maxMinSteps, stepSize);

    const QList<double> majors = div.ticks(QwtScaleDiv::MajorTick);
    auto stripped = [&](const QList<double> &src) {
        QList<double> out;
        for (double v : src)
            if (v >= lo && v <= hi && !majors.contains(v))
                out += v;
        return out;
    };

    QList<double> ticks[QwtScaleDiv::NTickTypes];
    ticks[QwtScaleDiv::MajorTick] = majors;
    ticks[QwtScaleDiv::MediumTick] = stripped(ext.ticks(QwtScaleDiv::MediumTick));
    ticks[QwtScaleDiv::MinorTick] = stripped(ext.ticks(QwtScaleDiv::MinorTick));
    return QwtScaleDiv(lo, hi, ticks);
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
        QwtScaleDiv div =
            divideHalf(engine, d_type, x1, x2, maxMajSteps, minorStepsForType(maxMinSteps, d_type), stepSize);
		delete engine;
		return div;
	}

    double lb = d_break_left;
    double rb = d_break_right;
	double step1 = d_step_before;
	double step2 = d_step_after;
    // The half drawn first is the main-type half, unless the axis runs high->low.
    ScaleTransformation::Type type1 = (x1 > x2) ? d_type_after : d_type;
    if (x1 > x2){
        lb = d_break_right;
        rb = d_break_left;
		step1 = d_step_after;
		step2 = d_step_before;
    }
    engine = newScaleEngine(type1);

    QwtScaleDiv div1 =
        divideHalf(engine, type1, x1, lb, maxMajSteps / 2, minorStepsForType(d_minor_ticks_before, type1), step1);

    delete engine;
    // After-break half uses its own scale type; an inverted axis draws the
    // main-type half on this side instead.
    ScaleTransformation::Type type2 = testAttribute(QwtScaleEngine::Inverted) ? d_type : d_type_after;
    engine = newScaleEngine(type2);

    QwtScaleDiv div2 =
        divideHalf(engine, type2, rb, x2, maxMajSteps / 2, minorStepsForType(d_minor_ticks_after, type2), step2);

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

        engine = newScaleEngine(d_type_after);
		engine->setAttributes(attributes());
		double breakRight = d_break_right;
		engine->autoScale(maxNumSteps, breakRight, x2, stepSize);
		delete engine;
	}
}

QwtScaleEngine *ScaleEngine::newScaleEngine() const
{
    return newScaleEngine(d_type);
}

QwtScaleEngine *ScaleEngine::newScaleEngine(ScaleTransformation::Type type)
{
    switch (type)
    {
    case ScaleTransformation::Log10:
        return new QwtLogScaleEngine();
    case ScaleTransformation::Ln:
        return new LnScaleEngine();
    case ScaleTransformation::Log2:
        return new Log2ScaleEngine();
    case ScaleTransformation::Reciprocal:
        return new ReciprocalScaleEngine();
    case ScaleTransformation::Probability:
        return new ProbabilityScaleEngine();
    case ScaleTransformation::Logit:
        return new LogitScaleEngine();
    case ScaleTransformation::Linear:
    default:
        return new QwtLinearScaleEngine();
    }
}

QwtTransform *ScaleEngine::newTransform(ScaleTransformation::Type type)
{
    switch (type)
    {
		case ScaleTransformation::Log10:
		case ScaleTransformation::Ln:
		case ScaleTransformation::Log2:
        // The base only scales the transform by a constant, which cancels in
        // the fractional positioning, so a single log transform serves all.
        return new QwtLogTransform();
		case ScaleTransformation::Reciprocal:
        return new ReciprocalScaleTransformation();
		case ScaleTransformation::Probability:
        return new ProbabilityScaleTransformation();
		case ScaleTransformation::Logit:
        return new LogitScaleTransformation();
		case ScaleTransformation::Linear:
		default:
        return new QwtNullTransform();
	}
}
