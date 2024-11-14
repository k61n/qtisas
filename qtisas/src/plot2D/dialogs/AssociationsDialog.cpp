/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plot associations dialog
 ******************************************************************************/

#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>

#include "AssociationsDialog.h"
#include "BoxCurve.h"
#include "ErrorBarsCurve.h"
#include "PieCurve.h"
#include "PlotCurve.h"
#include "QwtHistogram.h"
#include "Table.h"
#include "VectorCurve.h"

AssociationsDialog::AssociationsDialog(QWidget *parent, Qt::WindowFlags fl) : QDialog(parent, fl), graph(nullptr)
{
    setObjectName("AssociationsDialog");
    setWindowTitle( tr( "QtiSAS - Plot Associations" ) );
	setSizeGripEnabled(true);
	setFocus();

    auto vl = new QVBoxLayout();
    auto hbox1 = new QHBoxLayout();
    hbox1->addWidget(new QLabel(tr( "Spreadsheet: " )));

	tableCaptionLabel = new QLabel();
    hbox1->addWidget(tableCaptionLabel);
    vl->addLayout(hbox1);

	table = new QTableWidget(3, 5);
	table->horizontalHeader()->setSectionsClickable( false );
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->verticalHeader()->hide();
	table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	table->setMaximumHeight(8*table->rowHeight(0));
	table->setHorizontalHeaderLabels(QStringList() << tr("Column") << tr("X") << tr("Y") << tr("xErr") << tr("yErr"));
    vl->addWidget(table);

	associations = new QListWidget();
	associations->setSelectionMode ( QListWidget::SingleSelection );
    vl->addWidget(associations);

	btnApply = new QPushButton(tr( "&Update curves" ));
    btnOK = new QPushButton( tr( "&OK" ) );
	btnOK->setDefault( true );
    btnCancel = new QPushButton( tr( "&Cancel" ) );

    auto hbox2 = new QHBoxLayout();
	hbox2->addStretch();
    hbox2->addWidget(btnApply);
    hbox2->addWidget(btnOK);
    hbox2->addWidget(btnCancel);
    vl->addStretch();
    vl->addLayout(hbox2);
	setLayout(vl);

    active_table = nullptr;

	connect(associations, SIGNAL(currentRowChanged(int)), this, SLOT(updateTable(int)));
	connect(btnOK, SIGNAL(clicked()),this, SLOT(accept()));
	connect(btnCancel, SIGNAL(clicked()),this, SLOT(close()));
	connect(btnApply, SIGNAL(clicked()),this, SLOT(updateCurves()));
}

void AssociationsDialog::accept()
{
	updateCurves();
	close();
}

void AssociationsDialog::updateCurves()
{
	if (!graph)
		return;

	QApplication::setOverrideCursor(Qt::WaitCursor);

	for (int i = 0; i < associations->count(); i++)
		changePlotAssociation(i, plotAssociation(associations->item(i)->text()));
	graph->updatePlot();

	QApplication::restoreOverrideCursor();
}

void AssociationsDialog::changePlotAssociation(int curve, const QStringList& ass)
{
    auto c = (DataCurve *)graph->curve(curve);
	if (!c)
        return;

	if (c->plotAssociation() == ass)
		return;

	QStringList lst = ass;
	if (lst.count() == 1){
		c->setTitle(lst[0]);
		if (c->type() == Graph::Box)
			((BoxCurve*)c)->loadData();
		else if (c->type() == Graph::Pie)
			((PieCurve*)c)->loadData();
		else if (c->type() == Graph::Histogram)
			((QwtHistogram*)c)->loadData();
	} else if (lst.count() == 2){
		c->setXColumnName(lst[0].remove("(X)"));
		c->setTitle(lst[1].remove("(Y)"));
		c->loadData();
	} else if (lst.count() == 3){//curve with error bars
        auto er = (ErrorBarsCurve *)c;
		QString xColName = lst[0].remove("(X)");
		QString yColName = lst[1].remove("(Y)");
		QString erColName = lst[2].remove("(xErr)").remove("(yErr)");
		DataCurve *master_curve = graph->masterCurve(xColName, yColName);
		if (!master_curve)
			return;

		int type = ErrorBarsCurve::Vertical;
		if (ass.join(",").contains("(xErr)"))
			type = ErrorBarsCurve::Horizontal;
		er->setDirection(type);
		er->setTitle(erColName);
		if (master_curve != er->masterCurve())
			er->setMasterCurve(master_curve);
		else
			er->loadData();
	} else if (lst.count() == 4) {
        auto v = (VectorCurve *)c;
		v->setXColumnName(lst[0].remove("(X)"));
		v->setTitle(lst[1].remove("(Y)"));

		QString xEndCol = lst[2].remove("(X)").remove("(A)");
		QString yEndCol = lst[3].remove("(Y)").remove("(M)");
		if (v->vectorEndXAColName() != xEndCol || v->vectorEndYMColName() != yEndCol)
			v->setVectorEnd(xEndCol, yEndCol);
		else
			v->loadData();
	}
	graph->notifyChanges();
}

QStringList AssociationsDialog::plotAssociation(const QString& text)
{
    QStringList lst = text.split(": ", Qt::SkipEmptyParts);
    QStringList cols = lst[1].split(",", Qt::SkipEmptyParts);

    QStringList ass = QStringList() << lst[0] + "_" + cols[0].replace(".", ",");
	for (int i = 1; i < (int)cols.count(); i++ )
        ass << lst[0] + "_" + cols[i].replace(".", ",");

	return ass;
}

void AssociationsDialog::initTablesList(QList<MdiSubWindow *> lst, int curve)
{
    tables = std::move(lst);
    active_table = nullptr;

	if (curve < 0 || curve >= (int)associations->count())
		curve = 0;

	associations->setCurrentRow (curve);
}

Table * AssociationsDialog::findTable(int index)
{
	QString text = associations->item(index)->text();
    QStringList lst = text.split(":", Qt::SkipEmptyParts);
	foreach(MdiSubWindow *w, tables){
		if (w->objectName() == lst[0])
			return (Table *)w;
	}
    return nullptr;
}

void AssociationsDialog::updateTable(int index)
{
	Table *t = findTable(index);
	if (!t)
		return;

	if (active_table != t){
		active_table = t;
		tableCaptionLabel->setText(t->objectName());
		table->clearContents();
		table->setRowCount(t->numCols());

		QStringList colNames = t->colNames();
		for (int i=0; i<table->rowCount(); i++ ){
            auto cell = new QTableWidgetItem(colNames[i].replace(",", "."));
			cell->setBackground (QBrush(Qt::lightGray));
			cell->setFlags (Qt::ItemIsEnabled);
			table->setItem(i, 0, cell);
			}

		for (int j=1; j < table->columnCount(); j++){
			for (int i=0; i < table->rowCount(); i++ )
				{
                auto cell = new QTableWidgetItem();
				cell->setBackground (QBrush(Qt::lightGray));
				table->setItem(i, j, cell);

                auto cb = new QCheckBox(table);
				cb->installEventFilter(this);
				table->setCellWidget(i, j, cb);
				}
			}
		}
	updateColumnTypes();
}

void AssociationsDialog::updateColumnTypes()
{
	QString text = associations->currentItem()->text();
    QStringList lst = text.split(": ", Qt::SkipEmptyParts);
    QStringList cols = lst[1].split(",", Qt::SkipEmptyParts);

	QString xColName, yColName;

	int n = (int)cols.count();
	if (n >= 2){
		xColName = cols[0].remove("(X)");
		yColName = cols[1].remove("(Y)");

		table->showColumn(1);
		table->hideColumn(3);
		table->hideColumn(4);
		}
	else if (n == 1){//box plots
		yColName = cols[0];
		table->hideColumn(1);
		table->hideColumn(3);
		table->hideColumn(4);
		}

    QCheckBox *it = nullptr;
	for (int i=0; i < table->rowCount(); i++ ){
		it = (QCheckBox *)table->cellWidget(i, 1);
		if (table->item(i, 0)->text() == xColName)
			it->setChecked(true);
		else
			it->setChecked(false);

		it = (QCheckBox *)table->cellWidget(i, 2);
		if (table->item(i,0)->text() == yColName)
			it->setChecked(true);
		else
			it->setChecked(false);
		}

	bool xerr = false, yerr = false, vectors = false;
	QString errColName, xEndColName, yEndColName;
	if (n > 2){
		table->showColumn(3);
		table->showColumn(4);

		if (cols[2].contains("(xErr)") || cols[2].contains("(yErr)")){//if error bars
			table->horizontalHeaderItem(3)->setText(tr("xErr"));
			table->horizontalHeaderItem(4)->setText(tr("yErr"));
		}

		if (cols[2].contains("(xErr)")){
			xerr = true;
			errColName = cols[2].remove("(xErr)");
		} else if (cols[2].contains("(yErr)")){
			yerr = true;
			errColName = cols[2].remove("(yErr)");
		} else if (cols.count() > 3 && cols[2].contains("(X)") && cols[3].contains("(Y)")){
			vectors = true;
			xEndColName = cols[2].remove("(X)");
			yEndColName = cols[3].remove("(Y)");
			table->horizontalHeaderItem(3)->setText(tr("xEnd"));
			table->horizontalHeaderItem(4)->setText(tr("yEnd"));
		} else if (cols.count() > 3 && cols[2].contains("(A)") && cols[3].contains("(M)")){
			vectors = true;
			xEndColName = cols[2].remove("(A)");
			yEndColName = cols[3].remove("(M)");
			table->horizontalHeaderItem(3)->setText(tr("Angle"));
			table->horizontalHeaderItem(4)->setText(tr("Magn.","Magnitude, vector length"));
		}
	}

	for (int i=0; i < table->rowCount(); i++){
		it = (QCheckBox *)table->cellWidget(i, 3);
		if (xerr || vectors){
			if (table->item(i,0)->text() == errColName || table->item(i,0)->text() == xEndColName)
				it->setChecked(true);
			else
				it->setChecked(false);
		} else
			it->setChecked(false);

		it = (QCheckBox *)table->cellWidget(i, 4);
		if (yerr || vectors){
			if (table->item(i,0)->text() == errColName || table->item(i,0)->text() == yEndColName)
				it->setChecked(true);
			else
				it->setChecked(false);
		} else
			it->setChecked(false);

		it = (QCheckBox *)table->cellWidget(i, 1);
		if (xerr || yerr){
			it->setEnabled(false);
			table->cellWidget(i, 2)->setEnabled(false);
		} else {
			it->setEnabled(true);
			table->cellWidget(i, 2)->setEnabled(true);
		}
	}
}

void AssociationsDialog::uncheckCol(int col)
{
	for (int i=0; i < table->rowCount(); i++ ){
        auto it = (QCheckBox *)table->cellWidget(i, col);
		if (it)
			it->setChecked(false);
		}
}

void AssociationsDialog::setGraph(Graph *g)
{
    graph = g;

    for (int i=0; i<graph->curveCount(); i++){
        const QwtPlotItem *it = (QwtPlotItem *)graph->plotItem(i);
        if (!it)
            continue;
        if (it->rtti() != QwtPlotItem::Rtti_PlotCurve)
            continue;

        if (((DataCurve *)it)->type() != Graph::Function){
			QStringList lst = ((DataCurve *)it)->plotAssociation();
            if (((DataCurve *)it)->table()){
				QString tableName = ((DataCurve *)it)->table()->objectName();
				lst.replaceInStrings(tableName + "_", "").replaceInStrings(",", ".");
				plotAssociationsList << tableName + ": " + lst.join(",");
            }
        }
	}
    associations->addItems(plotAssociationsList);
    int64_t count = plotAssociationsList.count() + 1;
    int height = (count > std::numeric_limits<int>::max()) ? std::numeric_limits<int>::max() : static_cast<int>(count);
    associations->setMaximumHeight(height * associations->visualItemRect(associations->item(0)).height());
}

void AssociationsDialog::updatePlotAssociation(int row, int col)
{
	int index = associations->currentRow();
	QString text = associations->currentItem()->text();
    QStringList lst = text.split(": ", Qt::SkipEmptyParts);
    QStringList cols = lst[1].split(",", Qt::SkipEmptyParts);

	if (col == 1){
		cols[0] = table->item(row, 0)->text() + "(X)";
		text = lst[0] + ": " + cols.join(",");
		}
	else if (col == 2){
		if (cols.count() >= 2){
			cols[1] = table->item(row, 0)->text() + "(Y)";
			text = lst[0] + ": " + cols.join(",");
		} else // box or pie plots
			text = lst[0] + ": " + table->item(row, 0)->text();
		}
	else if (col == 3){
		if (text.contains("(A)")){//vect XYAM curve
			cols[2] = table->item(row, 0)->text() + "(A)";
			text = lst[0] + ": " + cols.join(",");
			}
		else if (!text.contains("(A)") && text.count("(X)") == 1){
			cols[2] = table->item(row, 0)->text() + "(xErr)";
			text = lst[0] + ": " + cols.join(",");
			uncheckCol(4);
			}
		else if (text.count("(X)") == 2){//vect XYXY curve
			cols[2] = table->item(row, 0)->text() + "(X)";
			text = lst[0] + ": " + cols.join(",");
			}
		}
	else if (col == 4){
		if (text.contains("(M)")){//vect XYAM curve
			cols[3] = table->item(row, 0)->text() + "(M)";
			text = lst[0] + ": " + cols.join(",");
			}
		else if (!text.contains("(M)") && text.count("(X)") == 1){
			cols[2] = table->item(row, 0)->text() + "(yErr)";
			text = lst[0] + ": " + cols.join(",");
			uncheckCol(3);
			}
		else if (text.count("(Y)") == 2){//vect XYXY curve
			cols[3] = table->item(row, 0)->text() + "(Y)";
			text = lst[0] + ": " + cols.join(",");
			}
		}

	//change associations for error bars depending on the curve "index"
	QString old_as = plotAssociationsList[index];
	for (int i = 0; i<(int)plotAssociationsList.count(); i++){
		QString as = plotAssociationsList[i];
		if (as.contains(old_as) && (as.contains("(xErr)") || as.contains("(yErr)"))){
            QStringList ls = as.split(",", Qt::SkipEmptyParts);
			as = text + "," + ls[2];
			plotAssociationsList[i] = as;
			associations->item(i)->setText(as);
			}
		}

	plotAssociationsList [index] = text;
	associations->item(index)->setText(text);
}

bool AssociationsDialog::eventFilter(QObject *object, QEvent *e)
{
    auto it = (QTableWidgetItem *)object;
	if (!it)
		return false;

	if (e->type() == QEvent::MouseButtonPress){
		if (((QCheckBox*)it)->isChecked() || !((QCheckBox*)it)->isEnabled())
			return true;

		int col = 0, row = 0;
		for (int j=1; j<table->columnCount(); j++){
			for (int i=0; i < table->rowCount(); i++ ){
                auto cb = (QCheckBox *)table->cellWidget(i, j);
				if ( cb == (QCheckBox *)object){
					row = i;
					col = j;
					break;
					}
				}
			}

		uncheckCol(col);
		((QCheckBox*)it)->setChecked(true);

		updatePlotAssociation(row, col);
		return true;
		}
	else if (e->type() == QEvent::MouseButtonDblClick)
		return true;
	else
		return false;
}
