/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2009 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: 2D binning matrix dialog
 ******************************************************************************/

#include <CreateBinMatrixDialog.h>
#include <ApplicationWindow.h>
#include <DoubleSpinBox.h>
#include <Table.h>
#include <Matrix.h>

#include <QPushButton>
#include <QSpinBox>
#include <QLayout>
#include <QFormLayout>

#include <gsl/gsl_histogram2d.h>

CreateBinMatrixDialog::CreateBinMatrixDialog(Table *t, int startRow, int endRow, QWidget* parent,  Qt::WindowFlags fl )
: QDialog( parent, fl ), d_table(t),
d_start_row(startRow),
d_end_row(endRow)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(tr("QtiSAS") + " - " + tr("Bin Matrix Dialog"));

	QVBoxLayout * mainLayout = new QVBoxLayout( this );

	QHBoxLayout * bottomLayout = new QHBoxLayout();
	bottomLayout->addStretch();

	buttonOk = new QPushButton(tr( "&OK" ));
	buttonOk->setAutoDefault( true );
	buttonOk->setDefault( true );
	bottomLayout->addWidget( buttonOk );

	buttonCancel = new QPushButton(tr( "&Cancel" ));
	buttonCancel->setAutoDefault( true );
	bottomLayout->addWidget( buttonCancel );

	d_y_col = d_table->colIndex(d_table->selectedYColumns()[0]);
	d_x_col = d_table->colX(d_y_col);

	double xmin, xmax, ymin, ymax;
	d_table->columnRange(d_x_col, &xmin, &xmax);
	d_table->columnRange(d_y_col, &ymin, &ymax);

	QLocale locale = d_table->locale();

	xMinBox = new DoubleSpinBox();
	xMinBox->setLocale(locale);
	xMinBox->setValue(floor(xmin));

	xMaxBox = new DoubleSpinBox();
	xMaxBox->setLocale(locale);
	xMaxBox->setValue(ceil(xmax));

	colsBox = new QSpinBox();
	colsBox->setValue(10);
	colsBox->setMinimum(1);
	colsBox->setMaximum(INT_MAX);

	yMinBox = new DoubleSpinBox();
	yMinBox->setLocale(locale);
	yMinBox->setValue(floor(ymin));

	yMaxBox = new DoubleSpinBox();
	yMaxBox->setLocale(locale);
	yMaxBox->setValue(ceil(ymax));

	rowsBox = new QSpinBox();
	rowsBox->setValue(10);
	rowsBox->setMinimum(1);
	rowsBox->setMaximum(INT_MAX);

	QFormLayout *formLayout = new QFormLayout;
	formLayout->addRow(tr("X-min"), xMinBox);
	formLayout->addRow(tr("X-max"), xMaxBox);
	formLayout->addRow(tr("Columns"), colsBox);
	formLayout->addRow(tr("Y-min"), yMinBox);
	formLayout->addRow(tr("Y-max"), yMaxBox);
	formLayout->addRow(tr("Rows"), rowsBox);

	mainLayout->addLayout( formLayout );
	mainLayout->addStretch();
	mainLayout->addLayout( bottomLayout );

	connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

void CreateBinMatrixDialog::accept()
{
	double xmin = xMinBox->value();
	double xmax = xMaxBox->value();
	double ymin = yMinBox->value();
	double ymax = yMaxBox->value();

	size_t nx = (size_t)colsBox->value();
	size_t ny = (size_t)rowsBox->value();

	if (INT_MAX/ny < nx){ //avoid integer overflow
    	QMessageBox::critical((ApplicationWindow *)parent(), tr("QtiSAS") + " - " + tr("Input Size Error"),
    	tr("The dimensions you have specified are not acceptable!") + "\n" +
		tr("Please enter positive values for which the product rows*columns does not exceed the maximum integer value available on your system!"));
		return;
	}

	gsl_histogram2d *h = gsl_histogram2d_alloc(nx, ny);
	if (!h){
		ApplicationWindow::memoryAllocationError();
		return;
	}

	Matrix* m = ((ApplicationWindow *)parent())->newMatrix((int)ny, (int)nx);
	if (m->numRows() != (int)ny || m->numCols() != (int)nx){
		//There was not enough memory to create the Matrix window with specified dimensions
		m->hide();
		m->askOnCloseEvent(false);
		((ApplicationWindow *)parent())->closeWindow(m);
		return;
	}

	gsl_histogram2d_set_ranges_uniform(h, xmin, xmax, ymin, ymax);

	QLocale l = ((ApplicationWindow *)parent())->locale();
	for (int i = d_start_row; i <= d_end_row; i++){
		QString xs = d_table->text(i, d_x_col);
		QString ys = d_table->text(i, d_y_col);
		if (!xs.isEmpty() && !ys.isEmpty())
			gsl_histogram2d_increment(h, l.toDouble(xs), l.toDouble(ys));
	}

	double dx = 0.5*fabs(xmax - xmin)/(double)nx;
	double dy = 0.5*fabs(ymax - ymin)/(double)ny;
	m->setCoordinates(xmin + dx, xmax - dx, ymin + dy, ymax - dy);

	for (size_t i = 0; i < nx; i++)
		for (size_t j = 0; j < ny; j++)
			m->setCell(j, i, gsl_histogram2d_get (h, i, j));

	gsl_histogram2d_free(h);

	m->showNormal();
	close();
}
