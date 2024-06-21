/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2010 roemmic <roemmic@5a6a7de5-fb12-0410-b871-c33778c25c60>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Extension to QwtScaleDraw
 ******************************************************************************/

#ifndef SCALES_H
#define SCALES_H

#include <QDateTime>
#include <QStringList>
#include <QLocale>

#include <qwt/qwt_scale_draw.h>

class Graph;
	
//! Extension to QwtScaleDraw
class ScaleDraw: public QwtScaleDraw
{
public:
	enum TicksStyle{None = 0, Out = 1, Both = 2, In = 3};
	enum ScaleType{Numeric = 0, Text = 1, Day = 2, Month = 3, Time = 4, Date = 5, ColHeader = 6};
	enum NumericFormat{Automatic, Decimal, Scientific, Superscripts, Engineering, SuperscriptsGER};
	enum NameFormat{ShortName, LongName, Initial};
	enum ShowTicksPolicy{ShowAll = 0, HideBegin = 1, HideEnd = 2, HideBeginEnd = 3};

    //! Constructs a new scale draw which is a clone of sd.
    ScaleDraw(Graph *plot, ScaleDraw *sd);
    explicit ScaleDraw(Graph *plot, const QString &formula = QString());
	ScaleDraw(Graph *plot, const QStringList& labels, const QString& format, ScaleType type = Text);

	Graph *plot(){return d_plot;};

	QString formatString(){return d_format_info;}

	QString formula() {return d_formula;};
	void setFormula(const QString& formula) {d_formula = formula;};

	double transformValue(double value) const;
	virtual QwtText label(double value) const;
	QString labelString(double value) const;

	int labelNumericPrecision()const {return d_prec;};
	void setNumericPrecision(int prec){d_prec = prec;};

	int majorTicksStyle()const {return d_majTicks;};
	void setMajorTicksStyle(TicksStyle type){d_majTicks = type;};

	int minorTicksStyle()const {return d_minTicks;};
	void setMinorTicksStyle(TicksStyle type){d_minTicks = type;};

	ShowTicksPolicy showTicksPolicy(){return d_show_ticks_policy;};
	void setShowTicksPolicy(const ShowTicksPolicy& policy){d_show_ticks_policy = policy;};

	void setSelected(bool select = true){d_selected = select;};

	int axis() const;

	ScaleType scaleType(){return d_type;};
	void setScaleType(ScaleType type){d_type = type;};

	void labelFormat(char &f, int &prec) const;

	NumericFormat labelNumericFormat(){return d_numeric_format;};
	void setNumericFormat(NumericFormat format);

	void setDayFormat(NameFormat format);
	void setMonthFormat(NameFormat format);
	void setTimeFormat(const QString& format);
	void setDateFormat(const QString& format);

	QStringList labelsList(){return d_text_labels;};
	void setLabelsList(const QStringList& list){d_text_labels = list;};

	NameFormat nameFormat(){return d_name_format;};

	QString prefix(){return d_prefix;};
	void setPrefix(const QString& s){d_prefix = s;};
	QString suffix(){return d_suffix;};
	void setSuffix(const QString& s){d_suffix = s;};

protected:
	virtual void draw (QPainter *, const QPalette &) const;
    virtual void drawLabel(QPainter *painter, double value) const;
	virtual void drawTick(QPainter *p, double value, int len) const;
	void drawInwardTick(QPainter *painter, double value, int len) const;
	virtual void drawBackbone(QPainter *painter) const;
	void drawBreak(QPainter *) const;

private:
	//! Pointer to the parent plot
	Graph *d_plot;
	//! Stores the scale type (numeric, text, etc...). See: enum NumericFormat
	ScaleType d_type;
	//! Stores the scale numeric format: Automatic, Decimal, Scientific, Superscripts
	NumericFormat d_numeric_format;
	//! Numerical format specification
	char d_fmt;
	//! Numerical precision
    int d_prec;
	//! A mathematical expression used to calculate the label values
	QString d_formula;
    //! Max ticks style
	int d_majTicks;
    //! Min ticks style
	int d_minTicks;
	//! Flag telling if the scale labels are selected (a blue frame is drawn around each labels if true)
	bool d_selected;
	//! Stores the scale name format for Day and Month scales
	NameFormat d_name_format;
	//! Stores extra format information:
	/* - the date/time format used to calculate labels for Date/Time scales
	 * - the Column/Table name in the case of Text/ColHeader scales
	 */
	QString d_format_info;
	//! Stores the labels for Txt scales
	QStringList d_text_labels;

	ShowTicksPolicy d_show_ticks_policy;
	QString d_prefix, d_suffix;
};
#endif
