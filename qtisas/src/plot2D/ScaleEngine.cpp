/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Extensions to QwtScaleEngine and QwtScaleTransformation
 ******************************************************************************/

#include "LnScaleEngine.h"
#include "Log2ScaleEngine.h"
#include "LogitScaleEngine.h"
#include "ProbabilityScaleEngine.h"
#include "ReciprocalScaleEngine.h"
#include "ScaleEngine.h"

QwtScaleTransformation* ScaleEngine::transformation() const
{
	return new ScaleTransformation(this);
}

double ScaleTransformation::invXForm(double p, double p1, double p2, double s1, double s2) const
{
	if (!d_engine->hasBreak()){
		QwtScaleTransformation *tr = newScaleTransformation();
		double res = tr->invXForm(p, p1, p2, s1, s2);
		delete tr;
		return res;
	}

    const int d_break_space = d_engine->breakWidth();
	const double lb = d_engine->axisBreakLeft();
    const double rb = d_engine->axisBreakRight();
	const double pm = p1 + (p2 - p1)*(double)d_engine->breakPosition()/100.0;
	double pml, pmr;
	if (p2 > p1){
		pml = pm - d_break_space;
		pmr = pm + d_break_space;
	} else {
		pml = pm + d_break_space;
		pmr = pm - d_break_space;
	}

	if (p > pml && p < pmr)
		return pm;

	bool invertedScale = d_engine->testAttribute(QwtScaleEngine::Inverted);
	ScaleTransformation::Type d_type = d_engine->type();

	if (invertedScale){
		if ((p2 > p1 && p <= pml) || (p2 < p1 && p >= pml)){
			if (d_engine->log10ScaleAfterBreak())
				return s1*exp((p - p1)/(pml - p1)*log(rb/s1));
            else
				return s1 + (rb - s1)/(pml - p1)*(p - p1);
		}

		if ((p2 > p1 && p >= pmr) || (p2 < p1 && p <= pmr)){
			if (d_type == ScaleTransformation::Log10 || d_type == ScaleTransformation::Ln)
				return lb * exp((p - pmr)/(p2 - pmr)*log(s2/lb));
			else if (d_type == ScaleTransformation::Linear)
				return lb + (p - pmr)/(p2 - pmr)*(s2 - lb);
		}

	}

    if ((p2 > p1 && p <= pml) || (p2 < p1 && p >= pml)){
        if (d_type == ScaleTransformation::Linear)
            return s1 + (lb - s1)*(p - p1)/(pml - p1);
        else if (d_type == ScaleTransformation::Log10 || d_type == ScaleTransformation::Ln)
            return s1 * exp((p - p1)/(pml - p1)*log(lb/s1));
    }

	if ((p2 > p1 && p >= pmr) || (p2 < p1 && p <= pmr)){
	    if (d_engine->log10ScaleAfterBreak())
            return rb * exp((p - pmr)/(p2 - pmr)*log(s2/rb));
	    else
            return rb + (p - pmr)*(s2 - rb)/(p2 - pmr);
	}

	return DBL_MAX; // something invalid
}

double ScaleTransformation::xForm(double s, double s1, double s2, double p1, double p2) const
{
	if ((d_engine->type() == ScaleTransformation::Log10 || d_engine->type() == ScaleTransformation::Ln ||
		 d_engine->type() == ScaleTransformation::Log2) && s < 0.0){
		if (p1 < p2){
			if (d_engine->testAttribute(QwtScaleEngine::Inverted))
				return DBL_MAX;
			return -DBL_MAX;
		}

		if (d_engine->testAttribute(QwtScaleEngine::Inverted))
			return -DBL_MAX;

		return DBL_MAX;
	}

	if (!d_engine->hasBreak()){
		QwtScaleTransformation *tr = newScaleTransformation();
		double res = tr->xForm(s, s1, s2, p1, p2);
		delete tr;
		return res;
	}

    const int d_break_space = d_engine->breakWidth();
	const double lb = d_engine->axisBreakLeft();
    const double rb = d_engine->axisBreakRight();
	const double pm = p1 + (p2 - p1)*(double)d_engine->breakPosition()/100.0;
	double pml, pmr;
	if (p2 > p1){
		pml = pm - d_break_space;
		pmr = pm + d_break_space;
	} else {
		pml = pm + d_break_space;
		pmr = pm - d_break_space;
	}

	if (s > lb && s < rb)
		return pm;

	bool invertedScale = d_engine->testAttribute(QwtScaleEngine::Inverted);
	ScaleTransformation::Type d_type = d_engine->type();

	if (invertedScale){
		if (s <= lb){
			if (d_type == ScaleTransformation::Linear)
				return pmr + (lb - s)/(lb - s2)*(p2 - pmr);
			else if (d_type == ScaleTransformation::Log10 ||
					 d_type == ScaleTransformation::Ln ||
					 d_type == ScaleTransformation::Log2){
				return pmr + log(lb/s)/log(lb/s2)*(p2 - pmr);
			}
		}

		if (s >= rb){
			if (d_engine->log10ScaleAfterBreak())
				return p1 + log(s1/s)/log(s1/rb)*(pml - p1);
			else
				return p1 + (s1 - s)/(s1 - rb)*(pml - p1);
		}

	}

    if (s <= lb){
        if (d_type == ScaleTransformation::Linear)
            return p1 + (s - s1)/(lb - s1)*(pml - p1);
        else if (d_type == ScaleTransformation::Log10 ||
				 d_type == ScaleTransformation::Ln ||
				 d_type == ScaleTransformation::Log2)
            return p1 + log(s/s1)/log(lb/s1)*(pml - p1);
    }

	if (s >= rb){
	    if (d_engine->log10ScaleAfterBreak())
            return pmr + log(s/rb)/log(s2/rb)*(p2 - pmr);
	    else
            return pmr + (s - rb)/(s2 - rb)*(p2 - pmr);
	}

	return DBL_MAX; // something invalid
}

QwtScaleTransformation *ScaleTransformation::copy() const
{
    return new ScaleTransformation(d_engine);
}

QwtScaleTransformation* ScaleTransformation::newScaleTransformation() const
{
	QwtScaleTransformation *transform = nullptr;
	switch (d_engine->type()){
		case ScaleTransformation::Log2:
		case ScaleTransformation::Ln:
		case ScaleTransformation::Log10:
			transform = new QwtScaleTransformation(QwtScaleTransformation::Log10);
		break;

		case ScaleTransformation::Reciprocal:
			transform = new ReciprocalScaleTransformation(d_engine);
		break;

		case ScaleTransformation::Probability:
			transform = new ProbabilityScaleTransformation(d_engine);
		break;

		case ScaleTransformation::Logit:
			transform = new LogitScaleTransformation(d_engine);
		break;

		case ScaleTransformation::Linear:
		default:
			transform = new QwtScaleTransformation (QwtScaleTransformation::Linear);
	}
	return transform;
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
{}

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
            engine = new QwtLog10ScaleEngine();
        else
            engine = new QwtLinearScaleEngine();
    } else
		engine = newScaleEngine();

    int max_min_intervals = d_minor_ticks_before;
	if (d_minor_ticks_before == 1)
		max_min_intervals = 3;
	if (d_minor_ticks_before > 1)
		max_min_intervals = d_minor_ticks_before + 1;

	QwtScaleDiv div1 = engine->divideScale(x1, lb, maxMajSteps/2, max_min_intervals, step1);

    max_min_intervals = d_minor_ticks_after;
	if (d_minor_ticks_after == 1)
		max_min_intervals = 3;
	if (d_minor_ticks_after > 1)
		max_min_intervals = d_minor_ticks_after + 1;

    delete engine;
    if (testAttribute(QwtScaleEngine::Inverted))
		engine = newScaleEngine();
    else if (d_log10_scale_after)
		engine = new QwtLog10ScaleEngine();
	else
		engine = new QwtLinearScaleEngine();

    QwtScaleDiv div2 = engine->divideScale(rb, x2, maxMajSteps/2, max_min_intervals, step2);

    QwtValueList ticks[QwtScaleDiv::NTickTypes];
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
			engine = new QwtLog10ScaleEngine();
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
