/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Extensions to QwtScaleEngine and QwtScaleTransformation
 ******************************************************************************/

#ifndef SCALEENGINE_H
#define SCALEENGINE_H

#include <qwt/qwt_scale_engine.h>
#include <qwt/qwt_scale_map.h>
#include <cfloat>

class ScaleEngine;

class ScaleTransformation: public QwtScaleTransformation
{
public:
	enum Type{Linear, Log10, Ln, Log2, Reciprocal, Probability, Logit};

	ScaleTransformation(const ScaleEngine *engine):QwtScaleTransformation(Other), d_engine(engine){};
	virtual double xForm(double x, double, double, double p1, double p2) const;
	virtual double invXForm(double x, double s1, double s2, double p1, double p2) const;
	QwtScaleTransformation* copy() const;

protected:
	QwtScaleTransformation* newScaleTransformation() const;
    //! The scale engine that generates the transformation
	const ScaleEngine* d_engine;
};

class ScaleEngine: public QwtScaleEngine
{
public:
	ScaleEngine(ScaleTransformation::Type type = ScaleTransformation::Linear,
				double left_break = -DBL_MAX, double right_break = DBL_MAX);
	QwtScaleTransformation* transformation() const;
	virtual QwtScaleDiv divideScale(double x1, double x2, int maxMajSteps,
		int maxMinSteps, double stepSize = 0.0) const;
	virtual void autoScale (int maxNumSteps, double &x1, double &x2, double &stepSize) const;

    double axisBreakLeft() const;
    double axisBreakRight() const;
	void setBreakRegion(double from, double to){d_break_left = from; d_break_right = to;};

	int breakWidth() const;
	void setBreakWidth(int width){d_break_width = width;};

	int breakPosition() const;
	void setBreakPosition(int pos){d_break_pos = pos;};

    double stepBeforeBreak() const;
    void setStepBeforeBreak(double step){d_step_before = step;};

    double stepAfterBreak() const;
    void setStepAfterBreak(double step){d_step_after = step;};

    int minTicksBeforeBreak() const;
    void setMinTicksBeforeBreak(int ticks){d_minor_ticks_before = ticks;};

    int minTicksAfterBreak() const;
    void setMinTicksAfterBreak(int ticks){d_minor_ticks_after = ticks;};

    bool log10ScaleAfterBreak() const;
    void setLog10ScaleAfterBreak(bool on){d_log10_scale_after = on;};

	ScaleTransformation::Type type() const;
	void setType(ScaleTransformation::Type type){d_type = type;};

	bool hasBreak() const;
	void clone(const ScaleEngine *engine);

	bool hasBreakDecoration() const;
	void drawBreakDecoration(bool draw){d_break_decoration = draw;};

private:

	QwtScaleEngine *newScaleEngine() const;

	ScaleTransformation::Type d_type;
	double d_break_left, d_break_right;
	//! Position of axis break (% of axis length)
	int d_break_pos;
	//! Scale increment before and after break
	double d_step_before, d_step_after;
	//! Minor ticks before and after break
	int d_minor_ticks_before, d_minor_ticks_after;
	//! Log10 scale after break
	bool d_log10_scale_after;
	//! Width of the axis break in pixels
	int d_break_width;
	//! If true draw the break decoration
	bool d_break_decoration;
};

#endif
