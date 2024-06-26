/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Add error bars dialog
 ******************************************************************************/

#include <QButtonGroup>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QRadioButton>
#include <QWidget>

#include <gsl/gsl_statistics.h>

#include "ApplicationWindow.h"
#include "DoubleSpinBox.h"
#include "ErrDialog.h"
#include "ErrorBarsCurve.h"
#include "MultiLayer.h"
#include "Table.h"

ErrDialog::ErrDialog( QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
	setAttribute(Qt::WA_DeleteOnClose);
	setFocusPolicy( Qt::StrongFocus );
	setSizeGripEnabled( true );

	QVBoxLayout *vbox1 = new QVBoxLayout();
	vbox1->setSpacing (5);

	QHBoxLayout *hbox1 = new QHBoxLayout();
	vbox1->addLayout(hbox1);

	textLabel1 = new QLabel();
	hbox1->addWidget(textLabel1);

	nameLabel = new QComboBox();
	hbox1->addWidget(nameLabel, 1);

	groupBox1 = new QGroupBox(tr("Source of errors"));
	QGridLayout * gridLayout = new QGridLayout(groupBox1);
	vbox1->addWidget(groupBox1);

	buttonGroup1 = new QButtonGroup();
	buttonGroup1->setExclusive( true );

	columnBox = new QRadioButton();
	columnBox->setChecked( true );
	buttonGroup1->addButton(columnBox);
	gridLayout->addWidget(columnBox, 0, 0 );

	colNamesBox = new QComboBox();
	tableNamesBox = new QComboBox();

	QHBoxLayout * comboBoxes = new QHBoxLayout();
	comboBoxes->addWidget(tableNamesBox);
	comboBoxes->addWidget(colNamesBox);

	gridLayout->addLayout(comboBoxes, 0, 1);

	percentBox = new QRadioButton();
	buttonGroup1->addButton(percentBox);
	gridLayout->addWidget(percentBox, 1, 0 );

	valueBox = new DoubleSpinBox();
	valueBox->setMinimum(0.0);
	valueBox->setLocale(QLocale());
	valueBox->setValue(5);
	valueBox->setAlignment( Qt::AlignHCenter );
	valueBox->setEnabled(false);
	gridLayout->addWidget(valueBox, 1, 1);

	standardBox = new QRadioButton();
	buttonGroup1->addButton(standardBox);
	gridLayout->addWidget(standardBox, 2, 0 );
	gridLayout->setColumnStretch(1, 1);

	groupBox3 = new QGroupBox();
	vbox1->addWidget(groupBox3);
	vbox1->addStretch();
	QHBoxLayout * hbox2 = new QHBoxLayout(groupBox3);

	buttonGroup2 = new QButtonGroup();
	buttonGroup2->setExclusive(true);

	xErrBox = new QRadioButton();
	buttonGroup2->addButton(xErrBox);
	hbox2->addWidget(xErrBox);

	yErrBox = new QRadioButton();
	buttonGroup2->addButton(yErrBox);
	hbox2->addWidget(yErrBox);
	yErrBox->setChecked(true);
	hbox2->addStretch();

	QVBoxLayout * vbox2 = new QVBoxLayout();
	buttonAdd = new QPushButton();
	buttonAdd->setDefault( true );
	vbox2->addWidget(buttonAdd);

	buttonCancel = new QPushButton();
	vbox2->addWidget(buttonCancel);

	vbox2->addStretch();

	QHBoxLayout * hlayout1 = new QHBoxLayout(this);
	hlayout1->addLayout(vbox1);
	hlayout1->addLayout(vbox2);

	languageChange();

	// signals and slots connections
	connect( buttonAdd, SIGNAL( clicked() ), this, SLOT( add() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( percentBox, SIGNAL( toggled(bool) ), valueBox, SLOT( setEnabled(bool) ) );
	connect( columnBox, SIGNAL( toggled(bool) ), tableNamesBox, SLOT( setEnabled(bool) ) );
	connect( columnBox, SIGNAL( toggled(bool) ), colNamesBox, SLOT( setEnabled(bool) ) );
	connect( tableNamesBox, SIGNAL( activated(int) ), this, SLOT( selectSrcTable(int) ));
    connect( nameLabel, SIGNAL( activated(int) ), this, SLOT( selectedDataset() ));
}
//+++
void ErrDialog::selectedDataset()
{
    tableNamesBox->setItemText(tableNamesBox->currentIndex(),
                               nameLabel->currentText().split("_", Qt::SkipEmptyParts)[0]);
    selectSrcTable(tableNamesBox->currentIndex());
}

void ErrDialog::setCurveNames(const QStringList& names)
{
	nameLabel->addItems(names);
}

void ErrDialog::setSrcTables(QList<MdiSubWindow *> tables)
{
	if (tables.isEmpty())
		return;

	srcTables = tables;
	tableNamesBox->clear();
    

    foreach(MdiSubWindow *w, tables)
		tableNamesBox->addItem(w->objectName());

    tableNamesBox->setItemText(tableNamesBox->currentIndex(),
                               nameLabel->currentText().split("_", Qt::SkipEmptyParts)[0]);
    
	selectSrcTable(tableNamesBox->currentIndex());
}

void ErrDialog::selectSrcTable(int tabnr)
{
	colNamesBox->clear();
	colNamesBox->addItems(((Table*)srcTables.at(tabnr))->colNames());
    if (yErrBox->isChecked()) colNamesBox->setItemText(colNamesBox->currentIndex(), "dI");
    if (xErrBox->isChecked()) colNamesBox->setItemText(colNamesBox->currentIndex(), "Sigma");
}

void ErrDialog::add()
{
	ApplicationWindow *app = qobject_cast<ApplicationWindow *>(parent());
	if (!app)
		return;
	MultiLayer *plot = (MultiLayer *)app->activeWindow(ApplicationWindow::MultiLayerWindow);
	if (!plot)
		return;
	Graph* g = plot->activeLayer();
	if (!g)
		return;

	QString name = nameLabel->currentText();
	name = name.left(name.indexOf(" ["));
	DataCurve *curve = g->dataCurve(g->curveIndex(name));
	if (!curve){
		QMessageBox::critical(app, tr("QtiSAS - Error"),
		tr("This feature is not available for user defined function curves!"));
		return;
	}

	if (curve->xColumnName().isEmpty())
		return;

	int direction = xErrBox->isChecked() ? 0 : 1;

	ErrorBarsCurve *er = nullptr;
	if (columnBox->isChecked()){
		QString errColumnName = tableNamesBox->currentText() + "_" + colNamesBox->currentText();
		Table *errTable = app->table(errColumnName);
		if (!errTable)
			return;
		/*if (w->numRows() != errTable->numRows()){
			QMessageBox::critical(app, tr("QtiSAS - Error"), tr("The selected columns have different numbers of rows!"));
			return;
		}*/
		if (errTable->isEmptyColumn(errTable->colIndex(errColumnName))){
			QMessageBox::critical(app, tr("QtiSAS - Error"), tr("The selected error column is empty!"));
			return;
		}
        er = g->addErrorBars(curve, errTable, errColumnName, direction, app->defaultCurveLineWidth, 0, QColor(Qt::black),true,true, true);
        
	} else {
		Table *t = app->table(name);
		if (!t)
			return;
		if (direction == ErrorBarsCurve::Horizontal)
			t->addCol(Table::xErr);
		else
			t->addCol(Table::yErr);

		int r = curve->dataSize();
		int rows = t->numRows();
		int col = t->numCols() - 1;
		int ycol = t->colIndex(name);
		if (!direction)
			ycol = t->colIndex(curve->xColumnName());

		QVarLengthArray<double> Y(r);
		if (direction == ErrorBarsCurve::Horizontal){
			for (int i = 0; i < r; i++)
				Y[i] = curve->x(i);
		} else {
			for (int i = 0; i < r; i++)
				Y[i] = curve->y(i);
		}

		if (percentBox->isChecked()){
			double prc = 0.01*valueBox->value();
			int aux = 0;
			for (int i = curve->startRow(); i <= curve->endRow(); i++){
				if (!t->text(i, ycol).isEmpty() && aux < r){
					t->setCell(i, col, Y[aux]*prc);
					aux++;
				}
			}
		} else {
			double sd = gsl_stats_sd(Y.data(), 1, r);
			for (int i = 0; i < rows; i++){
				if (!t->text(i, ycol).isEmpty())
					t->setCell(i, col, sd);
			}
		}
		er = g->addErrorBars(curve, t, t->colName(col), direction);
	}

	if (er){
		er->setColor(curve->pen().color());
		g->replot();
		plot->notifyChanges();
	}
}

void ErrDialog::languageChange()
{
    setWindowTitle( tr( "QtiSAS - Error Bars" ) );
    xErrBox->setText( tr( "&X Error Bars" ) );
	buttonAdd->setText( tr( "&Add" ) );
    textLabel1->setText( tr( "Add Error Bars to" ) );
    groupBox1->setTitle( tr( "Source of errors" ) );
    percentBox->setText( tr( "Percent of data (%)" ) );
    standardBox->setText( tr( "Standard Deviation of Data" ) );
    yErrBox->setText( tr( "&Y Error Bars" ) );
    buttonCancel->setText( tr( "&Close" ) );
	columnBox->setText(tr("Existing column"));
}
