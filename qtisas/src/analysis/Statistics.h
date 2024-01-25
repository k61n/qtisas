/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Abstract base class for statistics data analysis
 ******************************************************************************/

#ifndef STATISTICS_H
#define STATISTICS_H

#include <QObject>
#include <ApplicationWindow.h>

class Table;

//! Abstract base class for data analysis operations
class Statistics : public QObject
{
	Q_OBJECT

	public:
		Statistics(ApplicationWindow *parent, const QString& name = QString());
		~Statistics();

		//! Actually does the job. Should be reimplemented in derived classes.
		virtual bool run();
		bool setData(const QString&);
		void showResultsLog(bool show = true){d_result_log = show;};

		QString sampleName(){return d_col_name;};

		//! Returns the size of the input data set
		unsigned int dataSize(){return d_n;};
		//! Returns the degrees of freedom
		virtual int dof(){return d_n - 1;};

		//! Returns the y values of the input data set
		double* data(){return d_data;};

		double mean(){return d_mean;};
		double variance(){return d_variance;};
		double standardDeviation(){return d_sd;};
		double standardError(){return d_se;};

		QString logInfo(bool header = true);

	protected:
		void memoryErrorMessage();
		//! Frees the memory allocated for the X and Y data sets
		virtual void freeMemory();

		//! The name of the source data set
		QString d_col_name;
		//! Flag specifying if the results should be displayed in the results log
		bool d_result_log;
		//! The size of the data set to be analyzed
		unsigned int d_n;
		//! y data set to be analysed
		double *d_data;
		//! A table source of data
		Table *d_table;

		double d_mean;
		double d_sd;
		double d_variance;
		double d_se;
};

#endif
