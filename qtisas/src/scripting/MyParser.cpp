/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2009 masantos <masantos@5a6a7de5-fb12-0410-b871-c33778c25c60>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Parser class based on muParser
 ******************************************************************************/

#include <QApplication>
#include <QLocale>
#include <QMessageBox>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_const_num.h>

#include "muParserScripting.h"
#include "MyParser.h"
#include "NonLinearFit.h"

MyParser::MyParser()
:Parser()
{
	DefineConst("pi", M_PI);
	DefineConst("Pi", M_PI);
	DefineConst("PI", M_PI);

	for (const muParserScripting::mathFunction *i=muParserScripting::math_functions; i->name; i++){
		if (i->numargs == 1 && i->fun1 != nullptr)
			DefineFun(i->name, i->fun1);
		else if (i->numargs == 2 && i->fun2 != nullptr)
			DefineFun(i->name, i->fun2);
		else if (i->numargs == 3 && i->fun3 != nullptr)
			DefineFun(i->name, i->fun3);
	}
	gsl_set_error_handler_off();

	setLocale(getLocale());
}

QLocale MyParser::getLocale()
{
	bool cLocale = true;
	foreach (QWidget *w, QApplication::allWidgets()){
		ApplicationWindow *app = qobject_cast<ApplicationWindow *>(w);
		if (app){
			cLocale = app->d_muparser_c_locale;
			break;
		}
	}

	QLocale locale = QLocale::c();
	if (!cLocale)
		locale = QLocale();

	return locale;
}

void MyParser::setLocale(const QLocale& locale)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const char decPoint = locale.decimalPoint().toLatin1();
#else
    const char decPoint = locale.decimalPoint()[0].toLatin1();
#endif
	if (decPoint != '.'){
		SetDecSep(decPoint);
		SetArgSep(';');
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        SetThousandsSep(locale.groupSeparator().toLatin1());
#else
        SetThousandsSep(locale.groupSeparator()[0].toLatin1());
#endif
	} else
		ResetLocale();// reset C locale
}

void MyParser::addGSLConstants()
{
	DefineConst("e", M_E);
	DefineConst("E", M_E);

	//Fundamental constants provided by GSL
	DefineConst("c", GSL_CONST_MKSA_SPEED_OF_LIGHT);//The speed of light in vacuum
	DefineConst("eV", GSL_CONST_MKSA_ELECTRON_VOLT);//The energy of 1 electron volt
	DefineConst("g", GSL_CONST_MKSA_GRAV_ACCEL);//The standard gravitational acceleration on Earth
	DefineConst("G", GSL_CONST_MKSA_GRAVITATIONAL_CONSTANT);//The gravitational constant
	DefineConst("h", GSL_CONST_MKSA_PLANCKS_CONSTANT_H);//Planck's constant
	DefineConst("hbar", GSL_CONST_MKSA_PLANCKS_CONSTANT_HBAR);//Planck's constant divided by 2 pi
	DefineConst("k", GSL_CONST_MKSA_PLANCKS_CONSTANT_H);//The Boltzmann constant
	DefineConst("Na", GSL_CONST_NUM_AVOGADRO);//Avogadro's number
	DefineConst("R0", GSL_CONST_MKSA_MOLAR_GAS);//The molar gas constant
	DefineConst("V0", GSL_CONST_MKSA_STANDARD_GAS_VOLUME);//The standard gas volume
	DefineConst("Ry", GSL_CONST_MKSA_RYDBERG);//The Rydberg constant, Ry, in units of energy
}

const QStringList MyParser::functionsList()
{
  QStringList l;

  QString argSeparator = ",";
  if (QString(getLocale().decimalPoint()) == argSeparator)
	argSeparator = ";";

  for (const muParserScripting::mathFunction *i = muParserScripting::math_functions; i->name; i++){
    if (i->numargs == 2)
      l << QString(i->name) + "(" + argSeparator + ")";
	else
      l << QString(i->name) + "()";
  }
  return l;
}

const QStringList MyParser::functionNamesList()
{
	QStringList l;
	for (const muParserScripting::mathFunction *i = muParserScripting::math_functions; i->name; i++)
		l << QString(i->name);

	return l;
}

QString MyParser::explainFunction(int index)
{
	const muParserScripting::mathFunction i = muParserScripting::math_functions[index];
	QString s = i.description;
	if (getLocale().decimalPoint() == ',')
		s.replace(",", ";");

	return s;
}

double MyParser::EvalRemoveSingularity(double *xvar, bool noisy) const
{
	try {
		double result = Eval();
		if ( gsl_isinf(result) || gsl_isnan(result))
			throw Singularity();
		return result;
	} catch (Singularity) {
	    try {
			if (isinf(Eval()))
				throw Pole();
			double backup = *xvar;
			int n;
			frexp (*xvar, &n);
			double xp = *xvar + ldexp(DBL_EPSILON, n);
			double xm = *xvar - ldexp(DBL_EPSILON, n);
			*xvar = xp;
			double yp = Eval();
			if (gsl_isinf(yp) || gsl_isnan(yp))
				throw Pole();
			*xvar = xm;
			double ym = Eval();
			if (gsl_isinf(ym) || gsl_isnan(ym))
				throw Pole();
			*xvar = backup;
			return 0.5*(yp + ym);
	    } catch (Pole) {
	        if (noisy){
	        	QApplication::restoreOverrideCursor();
				//QMessageBox::critical(0, QObject::tr("QtiSAS - Math Error"),
				//QObject::tr("Found non-removable singularity at x = %1.").arg(*xvar));
				throw Pole();
			}
			return GSL_NAN;
	    }
	}
}

//almost verbatim copy from Parser::Diff, adapted to use EvalRemoveSingularity()

double MyParser::DiffRemoveSingularity(double *xvar, double *a_Var, double a_fPos) const
{
    double fRes(0),
		   fBuf(*a_Var),
           f[4] = {0,0,0,0},
	       a_fEpsilon( (a_fPos == 0) ? (double)1e-10 : 1e-7 * a_fPos );

    *a_Var = a_fPos+2 * a_fEpsilon;  f[0] = EvalRemoveSingularity(xvar);
    *a_Var = a_fPos+1 * a_fEpsilon;  f[1] = EvalRemoveSingularity(xvar);
    *a_Var = a_fPos-1 * a_fEpsilon;  f[2] = EvalRemoveSingularity(xvar);
    *a_Var = a_fPos-2 * a_fEpsilon;  f[3] = EvalRemoveSingularity(xvar);
    *a_Var = fBuf; // restore variable

    fRes = (-f[0] + 8*f[1] - 8*f[2] + f[3]) / (12*a_fEpsilon);
    return fRes;
}
