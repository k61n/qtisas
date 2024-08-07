/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A double spin box
 ******************************************************************************/

#include <cfloat>
#include <cmath>

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLineEdit>

#include "DoubleSpinBox.h"
#include "MyParser.h"

DoubleSpinBox::DoubleSpinBox(const char format, QWidget * parent)
    : QAbstractSpinBox(parent), d_format(format), d_min_val(-DBL_MAX), d_max_val(DBL_MAX), d_value(0.0), d_step(0.1),
      d_prec(14), d_prefix(QString())
{
	if (format == 'f')
		d_prec = 1;

	setFocusPolicy(Qt::StrongFocus);
	lineEdit()->setText(locale().toString(d_value, d_format, d_prec));
	setWrapping(false);
	connect(this, SIGNAL(editingFinished()), this, SLOT(interpretText()));
}

void DoubleSpinBox::setSingleStep(double val)
{
    if (d_step != val && val < d_max_val)
        d_step = val;
}

void DoubleSpinBox::setMaximum(double max)
{
	if (max == d_max_val || max > DBL_MAX)
		return;

	d_max_val = max;
}

void DoubleSpinBox::setMinimum(double min)
{
	if (min == d_min_val || min < -DBL_MAX)
		return;

	d_min_val = min;
}

void DoubleSpinBox::setRange(double min, double max)
{
	setMinimum(min);
	setMaximum(max);
}

void DoubleSpinBox::interpretText()
{
	bool ok = false;
	QString s = text();
	if (!d_prefix.isEmpty())
		s = s.mid(d_prefix.length());

	double value = locale().toDouble(s, &ok);
	if (ok && value == d_value)
		return;

	if (!ok){
		MyParser parser;
		parser.setLocale(QLocale());
		parser.addGSLConstants();
		try {
            parser.SetExpr(s.toLocal8Bit().constData());
			value = parser.Eval();
		} catch (mu::ParserError &e){
			lineEdit()->setText(textFromValue(d_value));
			return;
		}
	}

	if (setValue(value))
        emit valueChanged(d_value);
    else
        lineEdit()->setText(textFromValue(d_value));
}

void DoubleSpinBox::stepBy ( int steps )
{
	double val = d_value + steps*d_step;
	if (fabs(fabs(d_value) - d_step) < 1e-14 && d_value * steps < 0)//possible zero
		val = 0.0;

	if (setValue(val))
    	emit valueChanged(d_value);
}

QAbstractSpinBox::StepEnabled DoubleSpinBox::stepEnabled () const
{
	QAbstractSpinBox::StepEnabled stepDown = QAbstractSpinBox::StepNone;
	if (d_value > d_min_val)
		stepDown = StepDownEnabled;

	QAbstractSpinBox::StepEnabled stepUp = QAbstractSpinBox::StepNone;
	if (d_value < d_max_val)
		stepUp = StepUpEnabled;

	return stepDown | stepUp;
}

QString DoubleSpinBox::prefix() const
{
	return d_prefix;
}

bool DoubleSpinBox::setValue(double val)
{
	if (val >= d_min_val && val <= d_max_val){
		d_value = val;
		lineEdit()->setText(textFromValue(d_value));
		return true;
	}

	lineEdit()->setText(textFromValue(d_value));
	return false;
}

QString DoubleSpinBox::textFromValue (double value) const
{
	if (d_format == 'g' && fabs(value) < 1e-15){
		if (!specialValueText().isEmpty())
			return specialValueText();
		return d_prefix + "0";
	}

    QString s = QString();
	if (!specialValueText().isEmpty() && value == d_min_val)
		s = specialValueText();

	if (d_prec <= 14)
		s = locale().toString(value, d_format, d_prec);
	else
		s = locale().toString(value, d_format, 6);

	return d_prefix + s;
}

QValidator::State DoubleSpinBox::validate(QString & , int & ) const
{
	return QValidator::Acceptable;
}

void DoubleSpinBox::focusInEvent(QFocusEvent * e)
{
	emit activated(this);
	return QAbstractSpinBox::focusInEvent(e);
}

/*****************************************************************************
 *
 * Class RangeLimitBox
 *
 *****************************************************************************/

RangeLimitBox::RangeLimitBox(LimitType type, QWidget * parent)
:QWidget(parent),
d_type(type)
{
    d_checkbox = new QCheckBox();
	d_spin_box = new DoubleSpinBox();
	d_spin_box->setSpecialValueText(" ");
	d_spin_box->setValue(-DBL_MAX);
	d_spin_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d_spin_box->setEnabled(false);

	QHBoxLayout *l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	l->addWidget(d_checkbox);
	l->addWidget(d_spin_box);

	setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(d_spin_box);
	connect(d_checkbox, SIGNAL(toggled(bool)), d_spin_box, SLOT(setEnabled(bool)));
}

double RangeLimitBox::value()
{
	if (d_checkbox->isChecked())
		return d_spin_box->value();

	double val = -DBL_MAX;
	if (d_type == RightLimit)
		return DBL_MAX;
	return val;
}
