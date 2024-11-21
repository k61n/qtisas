/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Chi square test for variance
 ******************************************************************************/

#include <QLocale>

#include <gsl/gsl_cdf.h>

#include "ChiSquareTest.h"
#include "StatisticTest.h"

ChiSquareTest::ChiSquareTest(ApplicationWindow *parent, double testValue, double level, const QString& sample)
: StatisticTest(parent, testValue, level, sample)
{
	setObjectName(QObject::tr("Chi-square Test"));
}

double ChiSquareTest::chiSquare()
{
	if (!d_n)
		return 0.0;

	return (d_n - 1)*d_variance/d_test_val;
}

double ChiSquareTest::pValue()
{
	if (!d_n)
		return 0.0;

	double p = gsl_cdf_chisq_P(chiSquare(), dof());
	switch(d_tail){
		case Left:
		break;
		case Right:
			p = 1 - p;
		break;
    default:
			if (p < 0.5)
				p = 2*p;
			else
				p = 2*(1 - p);
		break;
	}
	return p;
}

//! Lower Confidence Limit
double ChiSquareTest::lcl(double confidenceLevel)
{
	if (!d_n || confidenceLevel < 0 || confidenceLevel > 100)
		return 0.0;

	double alpha = 1 - confidenceLevel/100.0;
	int df = dof();
	return df*d_variance/gsl_cdf_chisq_Pinv(1 - 0.5*alpha, df);
}

//! Upper Confidence Limit
double ChiSquareTest::ucl(double confidenceLevel)
{
	if (!d_n || confidenceLevel < 0 || confidenceLevel > 100)
		return 0.0;

	double alpha = 1 - confidenceLevel/100.0;
	int df = dof();
	return df*d_variance/gsl_cdf_chisq_Pinv(0.5*alpha, df);
}

QString ChiSquareTest::logInfo()
{
    auto app = (ApplicationWindow *)parent();
	QLocale l = app->locale();
	int p = app->d_decimal_digits;
	QString sep1 = "-----------------------------------------------------------------------------------------------------------------------------\n";

    QString s =
        "[" + l.toString(QDateTime::currentDateTime(), QLocale::ShortFormat) + " \"" + d_table->objectName() + "\"]\t";
	s += QObject::tr("Chi-square Test for Variance") + "\n";
	s += "\n";

	if (d_descriptive_statistics)
		s += Statistics::logInfo();

	QString h0, ha, compare;
	switch((int)d_tail){
		case Left:
			h0 = " >= ";
			ha = " < ";
			compare = QObject::tr("less");
		break;
    case Right:
        h0 = " <= ";
        ha = " > ";
        compare = QObject::tr("greater");
        break;
    default:
        h0 = " = ";
        ha = " <> ";
        compare = QObject::tr("different");
        break;
	}

	QString mText = QObject::tr("Variance");
	s += QObject::tr("Null Hypothesis") + ":\t\t\t" + mText + h0 + l.toString(d_test_val, 'g', p) + "\n";
	s += QObject::tr("Alternative Hypothesis") + ":\t\t" + mText + ha + l.toString(d_test_val, 'g', p) + "\n\n";

	double pval = pValue();
	QString sep = "\t\t";
	s += QObject::tr("Chi-Square") + sep + QObject::tr("DoF") + sep + QObject::tr("P Value") + "\n";
	s += sep1;
	s += l.toString(chiSquare(), 'g', p) + sep + QString::number(dof()) + sep + l.toString(pval, 'g', p) +  + "\n";
	s += sep1;
	s += "\n";

	s += QObject::tr("At the %1 level, the population variance").arg(l.toString(d_significance_level, 'g', 6)) + " ";
	if (pval < d_significance_level)
		s += QObject::tr("is significantly");
	else
		s += QObject::tr("is not significantly");
	s += " " + compare + " ";

	s += QObject::tr("than the test variance");
	s += " (" + l.toString(d_test_val, 'g', p) + ").\n";
	return s;
}
