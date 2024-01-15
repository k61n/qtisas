/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Student's t-Test
 ******************************************************************************/

#ifndef TTEST_H
#define TTEST_H

#include <StatisticTest.h>

//! Student's t-Test
class tTest : public StatisticTest
{
	Q_OBJECT

	public:
		tTest(ApplicationWindow *parent, double testValue, double level,
			const QString& sample1 = QString(), const QString& sample2 = QString(), bool paired = false);

		bool setSample1(const QString& colName){return setData(colName);};
		bool setSample2(const QString& colName, bool paired = false);

		virtual QString logInfo();

		double t();
		double statistic(){return t();};
		double pValue();
		int dof();

		double power(double alpha, int size = 0);
		//! Lower Confidence Limit
		double lcl(double confidenceLevel);
		//! Upper Confidence Limit
		double ucl(double confidenceLevel);

	protected:
		double t(int size);
		void freeMemory();

		Statistics *d_sample2;
		bool d_independent_test;
		double d_s12, d_diff;
};

#endif
