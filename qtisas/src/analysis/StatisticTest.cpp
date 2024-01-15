/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Base class for statistic tests
 ******************************************************************************/

#include "StatisticTest.h"
#include <Table.h>

StatisticTest::StatisticTest(ApplicationWindow *parent, double testVal, double level, const QString& sample)
: Statistics(parent, sample),
d_test_val(testVal),
d_significance_level(level),
d_tail(Both),
d_descriptive_statistics(true)
{
}

void StatisticTest::outputResultsTo(Table *t)
{
	if (!t)
		return;

	int rows = t->numRows();
	t->setNumRows(rows + 1);

	if (t->numCols() < 9)
		t->setNumCols(9);

	t->setText(rows, 0, sampleName());
	t->setColumnType(0, Table::Text);
	t->setCell(rows, 1, d_n);
	t->setCell(rows, 2, dof());
	t->setCell(rows, 3, d_mean);
	t->setCell(rows, 4, d_sd);
	t->setCell(rows, 5, d_variance);
	t->setCell(rows, 6, d_se);
	t->setCell(rows, 7, statistic());
	t->setCell(rows, 8, pValue());

	for (int i = 0; i < t->numCols(); i++)
		t->table()->resizeColumnToContents(i);
}

Table * StatisticTest::resultTable(const QString& name)
{
	ApplicationWindow *app = (ApplicationWindow *)parent();
	Table *t = app->newTable(1, 9, name, objectName() + " " + QObject::tr("Result Table"));

	QStringList header = QStringList() << QObject::tr("Sample") << QObject::tr("N") << QObject::tr("DoF") << QObject::tr("Mean")
						 << QObject::tr("Standard Deviation") << QObject::tr("Variance") << QObject::tr("Standard Error")
						 << QObject::tr("Statistic") << QObject::tr("P Value");

	t->setHeader(header);
	t->setText(0, 0, sampleName());
	t->setColumnType(0, Table::Text);
	t->setCell(0, 1, d_n);
	t->setCell(0, 2, dof());
	t->setCell(0, 3, d_mean);
	t->setCell(0, 4, d_sd);
	t->setCell(0, 5, d_variance);
	t->setCell(0, 6, d_se);
	t->setCell(0, 7, statistic());
	t->setCell(0, 8, pValue());

	for (int i = 0; i < t->numCols(); i++)
		t->table()->resizeColumnToContents(i);

	t->show();
	return t;
}
