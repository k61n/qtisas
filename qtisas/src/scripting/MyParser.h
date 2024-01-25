/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
    2009 masantos <masantos@5a6a7de5-fb12-0410-b871-c33778c25c60>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Parser class based on muParser
 ******************************************************************************/

#ifndef MYPARSER_H
#define MYPARSER_H

#include <muParser.h>
#include <qstringlist.h>

using namespace mu;

class QLocale;

/*!\brief Mathematical parser class based on muParser.
 *
 * \section future Future Plans
 * Eliminate in favour of Script/ScriptingEnv.
 * This will allow you to use e.g. Python's global variables and functions everywhere.
 * Before this happens, a cleaner and more generic solution for accessing the current ScriptingEnv
 * should be implemented (maybe by making it a property of Project; see ApplicationWindow).
 */
class MyParser : public Parser
{
public:
	MyParser();
	void addGSLConstants();
	void setLocale(const QLocale& locale);
	static QLocale getLocale();

	const static QStringList functionsList();
	const static QStringList functionNamesList();
	static QString explainFunction(int index);

	double EvalRemoveSingularity(double *xvar, bool noisy = true) const;
	double DiffRemoveSingularity(double *xvar, double *a_Var,double a_fPos) const;
	static void SingularityErrorMessage(double xvar);

	class Singularity {};
	class Pole {};
};

#endif
