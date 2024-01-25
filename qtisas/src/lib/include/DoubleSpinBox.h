/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A double spin box
 ******************************************************************************/

#ifndef DoubleSpinBox_H
#define DoubleSpinBox_H

#include <QAbstractSpinBox>
#include <QCheckBox>

//! A QDoubleSpinBox allowing to customize numbers display with respect to locale settings. 
/**
 * It allows the user to specify a custom display format.
 */
class DoubleSpinBox : public QAbstractSpinBox
{
	Q_OBJECT

public:
	//! Constructor.
	/**
	* \param format format used to display numbers: has the same meaning as in QLocale::toString ( double i, char f = 'g', int prec = 6 )
	* \param parent parent widget (only affects placement of the dialog)
	*/
    DoubleSpinBox(const char format = 'g', QWidget * parent = 0);

	void setSingleStep(double val);
	void setMaximum(double max);
	void setMinimum(double min);
	void setRange(double min, double max);

	int decimals(){return d_prec;};
	void setDecimals(int prec){if (prec >= 0) d_prec = prec;};

	double value(){interpretText(); return d_value;};
	bool setValue(double val);
//+++ 2020-05
    double minimum(){return d_min_val;};
    double maximum(){return d_max_val;};
//---
	QString prefix() const;
	void setPrefix(const QString & prefix){d_prefix = prefix;}
	
	void setFormat(const char format, int prec = 1){d_format = format; setDecimals(prec);};

	QString textFromValue ( double value ) const;
	virtual QValidator::State validate ( QString & input, int & pos ) const;

signals:
	void valueChanged ( double d );
	//! Signal emitted when the spin box gains focus
	void activated(DoubleSpinBox *);

private slots:
	void interpretText();

protected:
	void stepBy ( int steps );
	StepEnabled stepEnabled () const;
	void focusInEvent(QFocusEvent *);

private:
    char d_format;
	double d_min_val;
	double d_max_val;
	double d_value;
	double d_step;
	int d_prec;
	QString d_prefix;
};

//! A checkable DoubleSpinBox that can be used to select the limits of a double interval. 
class RangeLimitBox : public QWidget
{
public:
	enum LimitType{LeftLimit, RightLimit};

    RangeLimitBox(LimitType type, QWidget * parent = 0);	
	void setDecimals(int prec){d_spin_box->setDecimals(prec);};
	double value();
	bool isChecked(){return d_checkbox->isChecked();};

private:
    DoubleSpinBox *d_spin_box;
    QCheckBox *d_checkbox;
	LimitType d_type;
};
#endif // FITDIALOG_H
