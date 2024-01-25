/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Normality test
 ******************************************************************************/

#ifndef NORMTEST_H
#define NORMTEST_H

#include <StatisticTest.h>

//! Normality test
class ShapiroWilkTest : public StatisticTest
{
	Q_OBJECT

	public:
		ShapiroWilkTest(ApplicationWindow *parent, const QString& sample = QString());

		double w(){return d_w;};
		double statistic(){return d_w;};
		double pValue(){return d_pValue;};
		virtual QString logInfo();
		QString shortLogInfo();

	protected:
		QString infoString(bool header = true);
		void swilk(int *init, double *x, int *n, int *n1, int *n2, double *a,  double *w, double *pw, int *ifault);
		static double poly(const double *cc, int nord, double x);

		double d_w;
		double d_pValue;
};

#endif
