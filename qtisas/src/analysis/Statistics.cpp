/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Abstract base class for statistics data analysis
 ******************************************************************************/

#include "Statistics.h"
#include <Table.h>

#include <QApplication>
#include <QMessageBox>
#include <QLocale>

#include <gsl/gsl_statistics.h>

Statistics::Statistics(ApplicationWindow *parent, const QString& colName)
    : QObject(parent), d_col_name(QString()), d_result_log(true), d_n(0), d_data(nullptr), d_table(nullptr)
{
	setData(colName);
}

bool Statistics::run()
{
	if (d_n < 0){
		QMessageBox::critical((ApplicationWindow *)parent(), tr("QtiSAS") + " - " + tr("Error"),
				tr("You didn't specify a valid data set for this operation!"));
		return false;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	if (d_result_log)
		((ApplicationWindow *)parent())->updateLog(logInfo());
	QApplication::restoreOverrideCursor();
	return true;
}

bool Statistics::setData(const QString& colName)
{
	ApplicationWindow *app = (ApplicationWindow *)parent();
	if (!app)
		return false;

	d_table = app->table(colName);
	if (!d_table)
		return false;

	int col = d_table->colIndex(colName);
	if (col < 0){
		QMessageBox::information((ApplicationWindow *)parent(), QObject::tr("Attention!"),
				tr("There is no sample dataset called %1 in this project.").arg(colName));
		return false;
	}
	d_col_name = colName;

	unsigned int rows = d_table->numRows();
	if (!rows)
		return false;

	if (d_n > 0)//delete previousely allocated memory
		freeMemory();

	for (unsigned int i = 0; i < rows; i++){
		if (!d_table->text(i, col).isEmpty())
			d_n++;
	}
	if (!d_n){
		QMessageBox::information((ApplicationWindow *)parent(), QObject::tr("Attention!"),
		QObject::tr("The sample dataset (%1) must have at least one data point.").arg(colName));
		return false;
	}

	d_data = (double *)malloc(d_n*sizeof(double));
	if (!d_data){
		memoryErrorMessage();
		return false;
	}

	int aux = 0;
	for (unsigned int i = 0; i < rows; i++){
		if (!d_table->text(i, col).isEmpty()){
			d_data[aux] = d_table->cell(i, col);
			aux++;
		}
	}
	d_mean = gsl_stats_mean (d_data, 1, d_n);
	d_variance = gsl_stats_variance(d_data, 1, d_n);
	d_sd = gsl_stats_sd(d_data, 1, d_n);
	d_se = d_sd/sqrt(d_n);

	return true;
}

QString Statistics::logInfo(bool header)
{
	ApplicationWindow *app = (ApplicationWindow *)parent();
	QLocale l = app->locale();
	int p = app->d_decimal_digits;

	QStringList lst;
	lst << QObject::tr("Sample");
	lst << QObject::tr("N");
	lst << QObject::tr("Mean");
	lst << QObject::tr("Standard Deviation");
	lst << QObject::tr("Variance");
	lst << QObject::tr("Standard Error");
	lst << d_col_name;
	lst << QString::number(d_n);
	lst << l.toString(d_mean, 'g', p);
	lst << l.toString(d_sd, 'g', p);
	lst << l.toString(d_variance, 'g', p);
	lst << l.toString(d_se, 'g', p);

    CompatQFontMetrics fm(app->font());
	int width = 0;
	foreach(QString s, lst){
        int aw = fm.horizontalAdvance(s);
		if (aw > width)
			width = aw;
	}
	width += 6;

	QString s;
	QString lineSep;
	for (int i = 0; i < 6; i++){
		QString aux = lst[i];
        int spaces =
            (int)ceil((double)(width - fm.horizontalAdvance(aux)) / (double)fm.horizontalAdvance(QLatin1Char(' '))) + 1;
		s += aux + QString(spaces, QLatin1Char(' '));
		if (i == 5){
            int scores = ceil((double)fm.horizontalAdvance(s) / (double)fm.horizontalAdvance(QLatin1Char('-')));
			lineSep = "\n" + QString(scores, QLatin1Char('-')) + "\n";
			s += lineSep;
		}
	}

	if (!header)
		s = QString();

	for (int i = 6; i < lst.size(); i++){
		QString aux = lst[i];
        int spaces =
            (int)ceil((double)(width - fm.horizontalAdvance(aux)) / (double)fm.horizontalAdvance(QLatin1Char(' '))) + 1;
		s += aux + QString(spaces, QLatin1Char(' '));
	}

	return s + lineSep;
}

void Statistics::memoryErrorMessage()
{
	QApplication::restoreOverrideCursor();

	QMessageBox::critical((ApplicationWindow *)parent(),
	tr("QtiSAS") + " - " + tr("Memory Allocation Error"), tr("Not enough memory, operation aborted!"));
}

void Statistics::freeMemory()
{
	if (d_data && d_n > 0) {
		free(d_data);
		d_data = nullptr;
		d_n = 0;
	}
}

Statistics::~Statistics()
{
	freeMemory();
}
