/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
    2007 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plot tool for translating curves
 ******************************************************************************/

#include <QLocale>
#include <QMessageBox>

#include <qwt/qwt_plot_curve.h>

#include "ApplicationWindow.h"
#include "DataPickerTool.h"
#include "FunctionCurve.h"
#include "Graph.h"
#include "PlotCurve.h"
#include "ScreenPickerTool.h"
#include "TranslateCurveTool.h"

TranslateCurveTool::TranslateCurveTool(Graph *graph, ApplicationWindow *app, Direction dir, const QObject *status_target, const char *status_slot)
	: PlotToolInterface(graph, status_target, status_slot),
	d_dir(dir),
	d_app(app)
{
	if (status_target)
		connect(this, SIGNAL(statusText(const QString&)), status_target, status_slot);

	// Phase 1: select curve point
	d_sub_tool = new DataPickerTool(d_graph, app, DataPickerTool::Display, this, SIGNAL(statusText(const QString&)));
	connect((DataPickerTool*)d_sub_tool, SIGNAL(selected(QwtPlotCurve*, int)),
			this, SLOT(selectCurvePoint(QwtPlotCurve*,int)));

	emit statusText(tr("Double-click on plot to select a data point!"));
}

void TranslateCurveTool::selectCurvePoint(QwtPlotCurve *curve, int point_index)
{
	if (!curve)
		return;

	if(((PlotCurve *)curve)->type() != Graph::Function){
		DataCurve *c = (DataCurve *)curve;
		Table *t = c->table();
		if (!t)
			return;

	    if (d_dir == Horizontal && t->isReadOnlyColumn(t->colIndex(c->xColumnName()))){
            QMessageBox::warning(d_app, tr("QtiSAS - Warning"),
            tr("The column '%1' is read-only! Operation aborted!").arg(c->xColumnName()));
			delete d_sub_tool;
			d_graph->setActiveTool(nullptr);
			return;
        } else if (d_dir == Vertical && t->isReadOnlyColumn(t->colIndex(c->title().text()))){
            QMessageBox::warning(d_app, tr("QtiSAS - Warning"),
            tr("The column '%1' is read-only! Operation aborted!").arg(c->title().text()));
			delete d_sub_tool;
			d_graph->setActiveTool(nullptr);
			return;
        }
	}

	d_selected_curve = curve;
	d_curve_point = QwtDoublePoint(curve->x(point_index), curve->y(point_index));
	delete d_sub_tool;

	// Phase 2: select destination
	d_sub_tool = new ScreenPickerTool(d_graph, this, SIGNAL(statusText(const QString&)));
	((ScreenPickerTool*)d_sub_tool)->append(d_curve_point);
	ScreenPickerTool::MoveRestriction moveRestriction = ScreenPickerTool::Vertical;
	if (d_dir == Horizontal)
		moveRestriction = ScreenPickerTool::Horizontal;
	((ScreenPickerTool*)d_sub_tool)->setMoveRestriction(moveRestriction);
	connect((ScreenPickerTool*)d_sub_tool, SIGNAL(selected(const QwtDoublePoint&)), this, SLOT(selectDestination(const QwtDoublePoint&)));
	emit statusText(tr("Curve selected! Move cursor and click to choose a point and double-click/press 'Enter' to finish!"));
}

void TranslateCurveTool::selectDestination(const QwtDoublePoint &point)
{
	delete d_sub_tool;
	if (!d_selected_curve)
		return;

	// Phase 3: execute the translation

	if(((PlotCurve *)d_selected_curve)->type() == Graph::Function){
	    if (d_dir == Horizontal){
            QMessageBox::warning(d_app, tr("QtiSAS - Warning"),
            tr("This operation cannot be performed on function curves."));
        } else {
            FunctionCurve *func = (FunctionCurve *)d_selected_curve;
            if (func->functionType() == FunctionCurve::Normal){
                QString formula = func->formulas().first();
                double d = point.y() - d_curve_point.y();
                if (d > 0)
                    func->setFormula(formula + "+" + QString::number(d, 'g', 15));
                else
                    func->setFormula(formula + QString::number(d, 'g', 15));
                func->loadData();
            }
        }
	    d_graph->setActiveTool(nullptr);
	    return;
    } else {
    	DataCurve *c = (DataCurve *)d_selected_curve;
		double d = 0.0;
		QString col_name;
		switch(d_dir) {
			case Vertical:
			{
				col_name = c->title().text();
				d = point.y() - d_curve_point.y();
				break;
			}
			case Horizontal:
			{
				col_name = c->xColumnName();
				d = point.x() - d_curve_point.x();
				break;
			}
		}
		Table *tab = d_app->table(col_name);
		if (!tab) return;
		int col = tab->colIndex(col_name);
		if (tab->columnType(col) != Table::Numeric) {
			QMessageBox::warning(d_app, tr("QtiSAS - Warning"),
				tr("This operation cannot be performed on curves plotted from columns having a non-numerical format."));
			return;
		}

		int prec; char f;
		tab->columnNumericFormat(col, &f, &prec);
		int row_start = c->tableRow(0);
    	int row_end = row_start + c->dataSize();

    	QLocale locale = d_app->locale();
		int j = 0;//point index
		for (int i = row_start; i<row_end; i++){
			if (!tab->text(i, col).isEmpty()){
				tab->setText(i, col, locale.toString(
					(d_dir == Horizontal ? d_selected_curve->x(j) : d_selected_curve->y(j)) + d, f, prec));
			j++;
			}
		}
		d_app->updateCurves(tab, col_name);
		d_app->modifiedProject();
		d_graph->setActiveTool(nullptr);// attention: I'm now deleted. Maybe there is a cleaner solution...*/
    }
}
