/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
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
