/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2010 claverie <claverie@5a6a7de5-fb12-0410-b871-c33778c25c60>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Column options dialog
 ******************************************************************************/

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDate>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>

#include "Table.h"
#include "TableDialog.h"

TableDialog::TableDialog(Table *t, QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl),
    d_table(t)
{
    setObjectName( "TableDialog" );
    setWindowTitle( tr( "QtiSAS - Column options" ) );
    setSizeGripEnabled(true);

	QHBoxLayout *hboxa = new QHBoxLayout();
	hboxa->addWidget(new QLabel(tr( "Column Name:" )));
    colName = new QLineEdit();
    hboxa->addWidget(colName);

	enumerateAllBox = new QCheckBox(tr("Enumerate all to the right" ));

	buttonPrev = new QPushButton("&<<");
	buttonPrev->setAutoDefault(false);

	buttonNext = new QPushButton("&>>");
	buttonNext->setAutoDefault(false);

#ifndef Q_OS_MAC
	buttonPrev->setMaximumWidth(40);
	buttonNext->setMaximumWidth(40);
#endif

	QHBoxLayout *hboxb = new QHBoxLayout();
    hboxb->addWidget(buttonPrev);
    hboxb->addWidget(buttonNext);
    hboxb->addStretch();

    QVBoxLayout *vbox1 = new QVBoxLayout();
    vbox1->addLayout(hboxa);
    vbox1->addWidget(enumerateAllBox);
    vbox1->addLayout(hboxb);

	buttonOk = new QPushButton(tr( "&OK" ));
    buttonOk->setDefault(true);

	buttonApply = new QPushButton(tr("&Apply"));
	buttonApply->setAutoDefault(false);

	buttonCancel = new QPushButton(tr( "&Cancel" ));
	buttonCancel->setAutoDefault(false);

	QVBoxLayout  *vbox2 = new QVBoxLayout();
	vbox2->setSpacing(5);
    vbox2->setContentsMargins(5, 5, 5, 5);
	vbox2->addWidget(buttonOk);
	vbox2->addWidget(buttonApply);
	vbox2->addWidget(buttonCancel);

    QHBoxLayout  *hbox1 = new QHBoxLayout();
	hbox1->setSpacing(5);
	hbox1->addLayout(vbox1);
	hbox1->addLayout(vbox2);

	QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel( tr("Plot Designation:")), 0, 0);

   	columnsBox = new QComboBox();
	columnsBox->addItem(tr("None"));
	columnsBox->addItem(tr("X (abscissae)"));
	columnsBox->addItem(tr("Y (ordinates)"));
	columnsBox->addItem(tr("Z (height)"));
	columnsBox->addItem(tr("X Error"));
	columnsBox->addItem(tr("Y Error"));
	columnsBox->addItem(tr("Label"));
    gl1->addWidget(columnsBox, 0, 1);

    gl1->addWidget(new QLabel(tr("Display")), 1, 0);

   	displayBox = new QComboBox();
	displayBox->addItem(tr("Numeric"));
	displayBox->addItem(tr("Text"));
	displayBox->addItem(tr("Date"));
	displayBox->addItem(tr("Time"));
	displayBox->addItem(tr("Month"));
	displayBox->addItem(tr("Day of Week"));
    gl1->addWidget(displayBox, 1, 1);

    labelFormat = new QLabel(tr( "Format:" ));
 	gl1->addWidget(labelFormat, 2, 0);

    formatBox = new QComboBox();
    gl1->addWidget(formatBox, 2, 1);

	labelNumeric = new QLabel(tr( "Precision:" ));
	gl1->addWidget(labelNumeric, 3, 0);

    precisionBox = new QSpinBox();
    precisionBox->setRange(0, 14);
    gl1->addWidget(precisionBox, 3, 1);

    boxReadOnly = new QCheckBox(tr("&Read-only" ));
    gl1->addWidget(boxReadOnly, 4, 0);

	boxHideColumn = new QCheckBox(tr("&Hidden" ));
    gl1->addWidget(boxHideColumn, 4, 1);

	applyToRightCols = new QCheckBox(tr( "Apply to all columns to the right" ));

    QVBoxLayout *vbox3 = new QVBoxLayout();
    vbox3->addLayout(gl1);
    vbox3->addWidget(applyToRightCols);

    QGroupBox *gb = new QGroupBox(tr("Options"));
    gb->setLayout(vbox3);

    QHBoxLayout  *hbox2 = new QHBoxLayout();
    hbox2->addWidget(new QLabel(tr( "Column Width:" )));

	colWidth = new QSpinBox();
    colWidth->setRange(0, 1000);
	colWidth->setSingleStep(10);

	hbox2->addWidget(colWidth);

    applyToAllBox = new QCheckBox(tr( "Apply to all" ));
	hbox2->addWidget(applyToAllBox);

	comments = new QTextEdit();
	boxShowTableComments = new QCheckBox(tr("&Display Comments in Header"));
	boxShowTableComments->setChecked(d_table->commentsEnabled());

	QVBoxLayout* vbox4 = new QVBoxLayout();
    vbox4->addLayout(hbox1);
	vbox4->addWidget(gb);
	vbox4->addLayout(hbox2);
	vbox4->addWidget(new QLabel(tr( "Comment:" )));
	vbox4->addWidget(comments);
	vbox4->addWidget(boxShowTableComments);

    setLayout(vbox4);
    setFocusProxy (colName);

    updateColumn(d_table->selectedColumn());

   // signals and slots connections
	connect(colWidth, SIGNAL(valueChanged(int)), this, SLOT(setColumnWidth(int)));
	connect(buttonApply, SIGNAL(clicked()), this, SLOT(apply()));
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonCancel, SIGNAL( clicked() ), this, SLOT( close() ) );
	connect(columnsBox, SIGNAL(activated(int)), this, SLOT(setPlotDesignation(int)) );
	connect(displayBox, SIGNAL(activated(int)), this, SLOT(updateDisplay(int)));
	connect(buttonPrev, SIGNAL(clicked()), this, SLOT(prevColumn()));
	connect(buttonNext, SIGNAL(clicked()), this, SLOT(nextColumn()));
	connect(precisionBox, SIGNAL(valueChanged(int)), this, SLOT(updatePrecision(int)));
	connect(boxShowTableComments, SIGNAL(toggled(bool)), d_table, SLOT(showComments(bool)));
}

void TableDialog::accept()
{
	apply();
	close();
}

void TableDialog::prevColumn()
{
	int sc = d_table->selectedColumn();
	apply();
	updateColumn(--sc);
}

void TableDialog::nextColumn()
{
	int sc = d_table->selectedColumn();
	apply();
	updateColumn(++sc);
}

void TableDialog::updateColumn(int sc)
{
    int colType = d_table->columnType(sc);
    if (!sc)
        buttonPrev->setEnabled(false);
    else
        buttonPrev->setEnabled(true);

    if (sc >= d_table->numCols() - 1)
        buttonNext->setEnabled(false);
    else
        buttonNext->setEnabled(true);

    d_table->setSelectedCol(sc);
    d_table->table()->clearSelection();
    d_table->table()->selectColumn(sc);
    int pd = d_table->colPlotDesignation(sc);
    columnsBox->setCurrentIndex(pd);
    displayBox->setEnabled(pd != Table::Label);

    QString colLabel = d_table->colLabel(sc);
    colName->setText(colLabel);
    colName->setFocus();
    colName->selectAll();

    comments->setText(d_table->comment(sc));
    displayBox->setCurrentIndex(colType);
    updateDisplay(colType);

    boxReadOnly->setChecked(d_table->isReadOnlyColumn(sc));
	bool hidden = d_table->isColumnHidden(sc);
	boxHideColumn->setChecked(hidden);
	if (hidden)
		colWidth->setValue(100);
	else
		colWidth->setValue(d_table->columnWidth(sc));

    d_table->saveToMemory();

    if (colType == Table::Numeric){
        int f, prec;
        d_table->columnNumericFormat(sc, &f, &prec);

        formatBox->setCurrentIndex(f);
        precisionBox->setValue(prec);
	} else if (colType == Table::Time || colType == Table::Date){
        QString format = d_table->columnFormat(sc);
        if (formatBox->findText(format) < 0)
            formatBox->insertItem(0, format);

        formatBox->setItemText(formatBox->currentIndex(), format);
    } else if (colType == Table::Day){
        QString format = d_table->columnFormat(sc);
        if (format == "ddd")
            formatBox->setCurrentIndex(0);
        else if (format == "dddd")
            formatBox->setCurrentIndex(1);
        else if (format == "d")
            formatBox->setCurrentIndex(2);
    } else if (colType == Table::Month){
        QString format = d_table->columnFormat(sc);
        if (format == "MMM")
            formatBox->setCurrentIndex(0);
        else if (format == "MMMM")
            formatBox->setCurrentIndex(1);
        else if (format == "M")
            formatBox->setCurrentIndex(2);
    }
}

void TableDialog::setColumnWidth(int width)
{
	d_table->setColumnWidth(width, applyToAllBox->isChecked());
	d_table->setHeaderColType();
}

void TableDialog::apply()
{
	if (colName->text().contains("_")){
		QMessageBox::warning(this, tr("QtiSAS - Warning"),
		tr("For internal consistency reasons the underscore character is replaced with a minus sign."));
	}

    QString name = colName->text().replace("-", "_").remove("\n");

	int sc = d_table->selectedColumn();
	d_table->setColumnWidth(colWidth->value(), applyToAllBox->isChecked());
	d_table->setColComment(sc, comments->toPlainText().replace("\n", " ").replace("\t", " "));
	d_table->setColName(sc, name.replace("_", "-"), enumerateAllBox->isChecked());

	bool rightColumns = applyToRightCols->isChecked();
	int format = formatBox->currentIndex();
	int colType = displayBox->currentIndex();
	switch(colType){
		case 0:
			setNumericFormat(formatBox->currentIndex(), precisionBox->value(), rightColumns);
		break;

		case 1:
			setTextFormat(rightColumns);
		break;

		case 2:
			 setDateTimeFormat(colType, formatBox->currentText(), rightColumns);
		break;

		case 3:
			setDateTimeFormat(colType, formatBox->currentText(), rightColumns);
		break;

		case 4:
			if(format == 0)
				setMonthFormat("MMM", rightColumns);
			else if(format == 1)
				setMonthFormat("MMMM", rightColumns);
			else if(format == 2)
				setMonthFormat("M", rightColumns);
		break;

		case 5:
			if(format == 0)
				setDayFormat("ddd", rightColumns);
			else if(format == 1)
				setDayFormat("dddd", rightColumns);
			else if(format == 2)
				setDayFormat("d", rightColumns);
		break;
	}

	if (rightColumns){
		bool readOnly = boxReadOnly->isChecked();
		for (int i = sc; i<d_table->numCols(); i++){
			d_table->setReadOnlyColumn(i, readOnly);
			d_table->hideColumn(i, boxHideColumn->isChecked());
		}
	} else {
		d_table->setReadOnlyColumn(sc, boxReadOnly->isChecked());
		d_table->hideColumn(sc, boxHideColumn->isChecked());
	}
}

void TableDialog::closeEvent( QCloseEvent* ce )
{
	d_table->freeMemory();
	ce->accept();
}

void TableDialog::setPlotDesignation(int i)
{
    d_table->setPlotDesignation((Table::PlotDesignation)i, applyToRightCols->isChecked());
    if (i == Table::Label){
        displayBox->setCurrentIndex(1);
		updateDisplay(1);
		displayBox->setEnabled(false);
    } else
        displayBox->setEnabled(true);
}

void TableDialog::showPrecisionBox(int item)
{
    switch(item)
    {
        case 0:
        {
            precisionBox->hide();
            break;
        }
        case 1:
        {
            precisionBox->show();
            break;
        }
        case 2:
        {
            precisionBox->show();
            break;
        }
    }
}

void TableDialog::updatePrecision(int prec)
{
    setNumericFormat(formatBox->currentIndex(), prec, applyToRightCols->isChecked());
}

void TableDialog::updateDisplay(int item)
{
    labelFormat->show();
    formatBox->show();
    formatBox->clear();
    formatBox->setEditable(false);
    labelNumeric->hide();
    precisionBox->hide();

    if (item == 0){
        formatBox->addItem( tr( "Default" ) );
        formatBox->addItem( tr( "Decimal: 1000" ) );
        formatBox->addItem( tr( "Scientific: 1E3" ) );

        labelNumeric->show();
        precisionBox->show();
	} else {
        switch (item)
		{
		case 1:
			labelFormat->hide();
			formatBox->hide();
		break;

		case 2:
            formatBox->setEditable(true);
			formatBox->addItem("dd/MM/yyyy");
			formatBox->addItem("dd/MM/yyyy hh:mm");
			formatBox->addItem("dd/MM/yyyy hh:mm:ss");

			formatBox->addItem("dd.MM.yyyy");
			formatBox->addItem("dd.MM.yyyy hh:mm");
			formatBox->addItem("dd.MM.yyyy hh:mm:ss");

			formatBox->addItem("dd MM yyyy");
			formatBox->addItem("dd MM yyyy hh:mm");
			formatBox->addItem("dd MM yyyy hh:mm:ss");

			formatBox->addItem("yyyy-MM-dd");
			formatBox->addItem("yyyy-MM-dd hh:mm");
			formatBox->addItem("yyyy-MM-dd hh:mm:ss");

			formatBox->addItem("yyyyMMdd");
			formatBox->addItem("yyyyMMdd hh:mm");
			formatBox->addItem("yyyyMMdd hh:mm:ss");
		break;

		case 3:
			{
			formatBox->setEditable ( true );

			formatBox->addItem("h");
			formatBox->addItem("h ap");
			formatBox->addItem("h AP");
			formatBox->addItem("h:mm");
			formatBox->addItem("h:mm ap");
			formatBox->addItem("hh:mm");
			formatBox->addItem("h:mm:ss");
			formatBox->addItem("h:mm:ss.zzz");
			formatBox->addItem("mm:ss");
			formatBox->addItem("mm:ss.zzz");
			formatBox->addItem("hmm");
			formatBox->addItem("hmmss");
			formatBox->addItem("hhmmss");
			}
		break;

		case 4:
		{
			QDate date=QDate::currentDate();
            formatBox->addItem(QLocale().monthName(date.month(), QLocale::ShortFormat));
            formatBox->addItem(QLocale().monthName(date.month()));
            formatBox->addItem(QLocale().monthName(date.month(), QLocale::ShortFormat).left(1));
		}
		break;

		case 5:
		{
			QDate date=QDate::currentDate();
            formatBox->addItem(QLocale().dayName(date.dayOfWeek(), QLocale::ShortFormat));
            formatBox->addItem(QLocale().dayName(date.dayOfWeek()));
            formatBox->addItem(QLocale().dayName(date.dayOfWeek(), QLocale::ShortFormat).left(1));
		}
		break;
		}
	}
}

void TableDialog::setDateTimeFormat(int type, const QString& format, bool allRightColumns)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    bool ok = false;
	int sc = d_table->selectedColumn();
	if (allRightColumns){
		for (int i = sc; i<d_table->numCols(); i++){
		    if (type == Table::Date)
                ok = d_table->setDateFormat(format, i);
            else if (type == Table::Time)
                ok = d_table->setTimeFormat(format, i);
			if (!ok)
                break;
		}
	}
	else if (type == Table::Date)
		ok = d_table->setDateFormat(format, sc);
    else if (type == Table::Time)
		ok = d_table->setTimeFormat(format, sc);

	QApplication::restoreOverrideCursor();

    if (!ok){
        QMessageBox::critical(this, tr("QtiSAS - Error"), tr("Couldn't guess the source data format, please specify it using the 'Format' box!")+"\n\n"+
                             tr("For more information about the supported date/time formats please read the Qt documentation for the QDateTime class!"));
        return;
    }

    if (formatBox->findText(format) < 0){
        formatBox->insertItem(0, format);
        formatBox->setItemText(formatBox->currentIndex(), format);
    }
}

void TableDialog::setNumericFormat(int type, int prec, bool allRightColumns)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	int sc = d_table->selectedColumn();
	if (allRightColumns){
		for (int i = sc; i<d_table->numCols(); i++)
			d_table->setColNumericFormat(type, prec, i);
	} else
		d_table->setColNumericFormat(type, prec, sc);
	QApplication::restoreOverrideCursor();
}

void TableDialog::setTextFormat(bool allRightColumns)
{
	int sc = d_table->selectedColumn();
	if (allRightColumns){
		for (int i = sc; i<d_table->numCols(); i++)
			d_table->setTextFormat(i);
	} else
		d_table->setTextFormat(sc);
}

void TableDialog::setDayFormat(const QString& format, bool allRightColumns)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	int sc = d_table->selectedColumn();
	if (allRightColumns){
		for (int i = sc; i<d_table->numCols(); i++)
            d_table->setDayFormat(format, i);
	} else
        d_table->setDayFormat(format, sc);

	QApplication::restoreOverrideCursor();
}

void TableDialog::setMonthFormat(const QString& format, bool allRightColumns)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	int sc = d_table->selectedColumn();
	if (allRightColumns){
		for (int i = sc; i<d_table->numCols(); i++)
            d_table->setMonthFormat(format, i);
	} else
        d_table->setMonthFormat(format, sc);

	QApplication::restoreOverrideCursor();
}
