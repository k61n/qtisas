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
#include <qwt/qwt_transform.h>
#include <cfloat>

class ScaleEngine;

class ScaleTransformation
{
public:
	enum Type{Linear, Log10, Ln, Log2, Reciprocal, Probability, Logit};
};

//! Transformation that rescales each half of a broken axis.
/*!
  Qwt 6 splits the old QwtScaleTransformation into a value->value QwtTransform
  plus the linear paint mapping done by QwtScaleMap:

      p = p1 + (T(s) - T(s1)) / (T(s2) - T(s1)) * (p2 - p1)

  To place the break at \a breakPosition (% of the axis) and let each half fill
  its share of the canvas, we make T(s) return the wanted fractional position
  directly: T(s1) == 0, T(s2) == 1, the break centred at breakPosition with a
  small gap on either side. The data before/after the break is stretched to fill
  [0, breakPos-gap] and [breakPos+gap, 1] respectively.
*/
class ScaleEngineTransformation : public QwtTransform
{
  public:
    ScaleEngineTransformation(double left_break, double right_break, double s1, double s2, double breakPosFrac,
                              double halfGapFrac, ScaleTransformation::Type type, bool log10AfterBreak);

    double transform(double s) const override;
    double invTransform(double p) const override;
    QwtTransform *copy() const override;

  private:
    bool logBeforeBreak() const;

    double d_break_left, d_break_right;
    double d_s1, d_s2;
    double d_break_pos; //!< center of the break, fraction of the axis [0, 1]
    double d_half_gap;  //!< half width of the break gap, fraction of the axis
    ScaleTransformation::Type d_type;
    bool d_log10_after;
};

class ScaleEngine: public QwtScaleEngine
{
public:
	ScaleEngine(ScaleTransformation::Type type = ScaleTransformation::Linear,
				double left_break = -DBL_MAX, double right_break = DBL_MAX);
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
    void setType(ScaleTransformation::Type type)
    {
        d_type = type;
        updateTransformation();
    }

	bool hasBreak() const;
	void clone(const ScaleEngine *engine);

	bool hasBreakDecoration() const;
	void drawBreakDecoration(bool draw){d_break_decoration = draw;};

    //! Length in pixels of the axis the engine drives; used to size the break gap.
    void setAxisLength(int length)
    {
        d_axis_length = length;
    }

private:

    void updateTransformation();
    //! (Re)build the value->position transform, honoring the axis break if any.
    void buildTransformation();
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
    //! Last known scale interval, captured in divideScale() to size the break.
    mutable double d_x1 = 0.0, d_x2 = 1.0;
    //! Axis length in pixels, used to convert the pixel break width to a fraction.
    int d_axis_length = 0;
};

#endif
