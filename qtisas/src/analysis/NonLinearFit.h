/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: NonLinearFit class
 ******************************************************************************/

#ifndef NONLINEARFIT_H
#define NONLINEARFIT_H

#include "Fit.h"

using namespace std;

class NonLinearFit : public Fit
{
	Q_OBJECT

	public:
		NonLinearFit(ApplicationWindow *parent, Graph *g);
		NonLinearFit(ApplicationWindow *parent, QwtPlotCurve *c);
		NonLinearFit(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end);
		NonLinearFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		NonLinearFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		NonLinearFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

        double eval(double *par, double x);

		virtual bool setParametersList(const QStringList& lst);
		virtual bool setFormula(const QString& s, bool = true);

		QMap<QString, double> constants(){return d_constants;};
		void setConstant(const QString& parName, double val);
		void removeConstant(const QString& parName){d_constants.remove(parName);};
		void removeConstants(){d_constants.clear();};
		int constantsCount(){return d_constants.size();};
		QList<QString> constantsList(){return d_constants.keys();};
		double constValue(const QString& name){return d_constants.value(name);};

		virtual QString legendInfo();
		static QStringList guessParameters(const QString& s, bool *error = 0, string *errMsg = 0, const QString& var = "x");

	protected:
		QString logFitInfo(int iterations, int status);
		FunctionCurve* insertFitFunctionCurve(const QString& name, int penWidth = 1, bool updateData = true);

	private:
		void calculateFitCurveData(double *X, double *Y);
		void init();
		virtual bool removeDataSingularities();
		void removePole(int index);

		QMap<QString, double> d_constants;
};
#endif
