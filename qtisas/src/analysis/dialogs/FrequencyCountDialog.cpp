/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Frequency count options dialog
 ******************************************************************************/

#include <QDateTime>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>

#include <gsl/gsl_histogram.h>
#include <gsl/gsl_sort_vector.h>
#include <gsl/gsl_statistics.h>

#include "ApplicationWindow.h"
#include "DoubleSpinBox.h"
#include "FrequencyCountDialog.h"
#include "Table.h"

FrequencyCountDialog::FrequencyCountDialog(Table *t, QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl ),
    d_source_table(t),
    d_result_table(nullptr),
    d_col_name(""),
    d_col_values(nullptr),
	d_bins(10)
{
	setObjectName( "FrequencyCountDialog" );
	setWindowTitle(tr("QtiSAS - Frequency count"));
	setSizeGripEnabled( true );
	setAttribute(Qt::WA_DeleteOnClose);

    QGroupBox *gb1 = new QGroupBox();
    QGridLayout *gl1 = new QGridLayout(gb1);

	ApplicationWindow *app = (ApplicationWindow *)parent;
	double min = 0.0, max = 0.0, step = 0.0;
	if (t){
        int col = -1;
        int sr = 0;
        int er = t->numRows();
        MySelection sel = t->table()->currentSelection();
        if (!sel.isEmpty()) {
            sr = sel.topRow();
            er = sel.bottomRow() + 1;
            col = sel.leftColumn();
            d_col_name = t->colName(col);
        }
        int size = 0;
        for (int i = sr; i < er; i++){
            if (!t->text(i, col).isEmpty())
                size++;
        }

        if (size > 1)
            d_col_values = gsl_vector_alloc(size);

        if (d_col_values){
            int aux = 0;
            for (int i = sr; i < er; i++){
                if (!t->text(i, col).isEmpty()){
                    gsl_vector_set(d_col_values, aux, t->cell(i, col));
                    aux++;
                }
            }

            gsl_sort_vector(d_col_values);

            min = floor(gsl_vector_get(d_col_values, 0));
            max = ceil(gsl_vector_get(d_col_values, size - 1));
            step = (max - min)/(double)d_bins;

            int p = app->d_decimal_digits;
            double *data = d_col_values->data;
            QLocale l = app->locale();
            QString s = "[" + l.toString(QDateTime::currentDateTime(), QLocale::ShortFormat) + " \"" + t->objectName() +
                        "\"]\n";
            s += tr("Statistics on %1").arg(d_col_name) + ":\n";
            s += tr("Mean") + " = " + l.toString(gsl_stats_mean (data, 1, size), 'f', p) + "\n";
            s += tr("Standard Deviation") + " = " + l.toString(gsl_stats_sd(data, 1, size), 'f', p) + "\n";
            s += tr("Median") + " = " + l.toString(gsl_stats_median_from_sorted_data(data, 1, size), 'f', p) + "\n";
            s += tr("Size") + " = " + QString::number(size) + "\n";
            s += "--------------------------------------------------------------------------------------\n";
            app->updateLog(s);
        }
	}

    gl1->addWidget(new QLabel(tr("From Minimum")), 0, 0);

	boxStart = new DoubleSpinBox();
	boxStart->setLocale(app->locale());
	boxStart->setValue(min);
	boxStart->setDecimals(app->d_decimal_digits);
	gl1->addWidget(boxStart, 0, 1);

    gl1->addWidget(new QLabel(tr("To Maximum")), 1, 0);

    boxEnd = new DoubleSpinBox();
	boxEnd->setLocale(app->locale());
    boxEnd->setValue(max);
    boxEnd->setDecimals(app->d_decimal_digits);
    gl1->addWidget(boxEnd, 1, 1);

    gl1->addWidget(new QLabel(tr("Step Size")), 2, 0);

    boxStep = new DoubleSpinBox();
	boxStep->setLocale(app->locale());
    boxStep->setValue(step);
    boxStep->setDecimals(app->d_decimal_digits);
    gl1->addWidget(boxStep, 2, 1);

    gl1->setRowStretch(3, 1);
	gl1->setColumnStretch(1, 1);

	buttonApply = new QPushButton(tr( "&Apply" ));
    buttonApply->setDefault( true );
    buttonCancel = new QPushButton(tr( "&Cancel" ));
    buttonOk = new QPushButton(tr( "&Ok" ));

    QVBoxLayout *vl = new QVBoxLayout();
 	vl->addWidget(buttonApply);
 	vl->addWidget(buttonOk);
	vl->addWidget(buttonCancel);
    vl->addStretch();

    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->addWidget(gb1, 1);
    hb->addLayout(vl);

	connect( buttonApply, SIGNAL( clicked() ), this, SLOT( apply() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( close() ) );
    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
}

bool FrequencyCountDialog::apply()
{
    if (!d_col_values){
        QMessageBox::critical(this, tr("QtiSAS - Error"),
            tr("Not enough data points, operation aborted!"));
        return false;
	}

	double from = boxStart->value();
	double to = boxEnd->value();
    if (from >= to){
        QMessageBox::critical(this, tr("QtiSAS - Frequency input error"),
            tr("Please enter frequency limits that satisfy: From < To !"));
        boxEnd->setFocus();
        return false;
	}

	int old_bins = d_bins;
	double bin_size = boxStep->value();
	d_bins = int((to - from)/bin_size + 1);
	if (!d_bins)
		return false;

	ApplicationWindow *app = (ApplicationWindow *)parent();
	if (!app)
        return false;

    if (!d_result_table){
        d_result_table = app->newTable(30, 4, app->generateUniqueName(tr("Count"), true),
                                    tr("Frequency count of %1").arg(d_col_name));
        d_result_table->setColName(0, tr("BinCtr"));
        d_result_table->setColName(1, tr("Count"));
        d_result_table->setColName(2, tr("BinEnd"));
        d_result_table->setColName(3, tr("Sum"));
        d_result_table->showMaximized();
    }

	gsl_histogram *h = gsl_histogram_alloc(d_bins);
	if (!h)
		return false;

	double *range = (double *) malloc((d_bins + 2)*sizeof(double));
	if (!range)
		return false;
	for (int i = 0; i <= d_bins + 1; i++)
		range[i] = from + i*bin_size;
	gsl_histogram_set_ranges (h, range, d_bins + 1);
	free(range);

	int dataSize = d_col_values->size;
	for (int i = 0; i < dataSize; i++ )
		gsl_histogram_increment (h, gsl_vector_get(d_col_values, i));

	if (d_bins > d_result_table->numRows())
		d_result_table->setNumRows(d_bins);

	for(int i = d_bins; i < old_bins; i++){
		d_result_table->setText(i, 0, "");
		d_result_table->setText(i, 1, "");
		d_result_table->setText(i, 2, "");
		d_result_table->setText(i, 3, "");
	}

	double sum = 0.0;
	for (int i = 0; i<d_bins; i++ ){
		double aux = gsl_histogram_get (h, i);
		sum += aux;
		double lower, upper;
		gsl_histogram_get_range (h, i, &lower, &upper);
		d_result_table->setCell(i, 0, 0.5*(lower + upper));
		d_result_table->setCell(i, 1, aux);
		d_result_table->setCell(i, 2, upper);
		d_result_table->setCell(i, 3, sum);
	}
	return true;
}

void FrequencyCountDialog::accept()
{
	if (apply())
        close();
}

FrequencyCountDialog::~FrequencyCountDialog()
{
    if (d_col_values)
        gsl_vector_free(d_col_values);
}
