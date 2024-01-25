/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Function curve class
 ******************************************************************************/

#ifndef FUNCTIONCURVE_H
#define FUNCTIONCURVE_H

#include "PlotCurve.h"

class Graph;

// Function curve class
class FunctionCurve: public PlotCurve
{
public:
	enum FunctionType{Normal = 0, Parametric = 1, Polar = 2};

	FunctionCurve(const FunctionType& t, const QString& name = QString());
	FunctionCurve(const QString& name = QString());

	double startRange(){return d_from;};
	double endRange(){return d_to;};
	void setRange(double from, double to);

	QStringList formulas(){return d_formulas;};
	void setFormulas(const QStringList& lst){d_formulas = lst;};

	//! Provided for convenience when dealing with normal functions
	void setFormula(const QString& s){d_formulas = QStringList() << s;};

	QString variable(){return d_variable;};
	void setVariable(const QString& s){d_variable = s;};

	FunctionType functionType(){return d_function_type;};
	void setFunctionType(const FunctionType& t){d_function_type = t;};

	void copy(FunctionCurve *f);

	//! Returns a string used when saving to a project file
	QString saveToString();
	//! Used when reading from a project file
	static void restore(Graph *g, const QStringList& lst);

	//! Returns a string that can be displayed in a plot legend
	QString legend();

	bool loadData(int points = 0, bool xLog10Scale = false);

	//! Returns the number of parameters in your function formula
	int parametersCount(){return d_constants.count();};
	//! Returns the name of the parameter of rang index
	QString parameterName(int index);
	//! Returns the value of the parameter of rang index
	double parameterValue(int index);
	//! Returns the value of the name parameter
	double parameterValue(const QString& name);

	QMap<QString, double> constants(){return d_constants;};
	void setConstants(const QMap<QString, double>& map){d_constants = map;};
	void setConstant(const QString& parName, double val){d_constants.insert(parName, val);};
	void removeConstant(const QString& parName){d_constants.remove(parName);};
	void removeConstants(){d_constants.clear();};

private:
	FunctionType d_function_type;
	QString d_variable;
	QStringList d_formulas;
	double d_from, d_to;
	QMap<QString, double> d_constants;
};

#endif
