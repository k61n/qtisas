/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Numerical correlation of data sets
 ******************************************************************************/

#ifndef CORRELATION_H
#define CORRELATION_H

#include "Filter.h"

class Correlation : public Filter
{
Q_OBJECT

public:
    Correlation(ApplicationWindow *parent, Table *t, const QString& colName1, const QString& colName2, int startRow = 1, int endRow = -1);

	bool setDataFromTable(Table *t, const QString& colName1, const QString& colName2, int startRow = 1, int endRow = -1);

protected:
	//! Handles the graphical output
	void addResultCurve();

private:
    virtual void output();
};

#endif

