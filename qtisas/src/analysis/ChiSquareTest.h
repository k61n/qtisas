/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Chi square test for variance
 ******************************************************************************/

#ifndef CHISQUARETEST_H
#define CHISQUARETEST_H

#include <StatisticTest.h>

//! Chi-Square test for variance
class ChiSquareTest : public StatisticTest
{
	Q_OBJECT

	public:
		ChiSquareTest(ApplicationWindow *parent, double testValue, double level, const QString& sample = QString());

		virtual QString logInfo();
		double chiSquare();
		double pValue();
		double statistic(){return chiSquare();};

		//! Lower Confidence Limit
		double lcl(double confidenceLevel);
		//! Upper Confidence Limit
		double ucl(double confidenceLevel);
};

#endif
