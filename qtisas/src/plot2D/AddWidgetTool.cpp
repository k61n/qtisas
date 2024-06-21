/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Alex Kargovsky <kargovsky@yumr.phys.msu.su>
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Tool for adding enrichments to a plot
 ******************************************************************************/

#include "AddWidgetTool.h"
#include "Graph.h"
#include "RectangleWidget.h"
#include "LegendWidget.h"
#include "TexWidget.h"
#include "EllipseWidget.h"
#include <ApplicationWindow.h>

#include <QAction>
#include <QCursor>

#include <qwt_plot_canvas.h>
#include <qwt_scale_widget.h>
#include <qwt_text_label.h>

AddWidgetTool::AddWidgetTool(WidgetType type, Graph *graph, QAction *action, const QObject *status_target, const char *status_slot)
	: QObject(graph),
	PlotToolInterface(graph),
	d_action(action),
	d_widget_type(type),
	d_fw(nullptr)
{
	graph->disableTools();
	graph->multiLayer()->applicationWindow()->pickPointerCursor();

    QwtPlotCanvas *canvas = graph->canvas();
	canvas->installEventFilter(this);

	if (type == Rectangle || type == Ellipse){
        graph->setCursor(QCursor(Qt::CrossCursor));
        canvas->setCursor(QCursor(Qt::CrossCursor));
	} else {
        graph->setCursor(QCursor(Qt::IBeamCursor));
        canvas->setCursor(QCursor(Qt::IBeamCursor));
	}

	QwtTextLabel *title = graph->titleLabel();
	if (title)
		title->installEventFilter(this);

	for ( uint i = 0; i < QwtPlot::axisCnt; i++ ){
        QwtScaleWidget *scale = graph->axisWidget(i);
        if (scale)
            scale->installEventFilter(this);
    }

	if (status_target)
		connect(this, SIGNAL(statusText(const QString&)), status_target, status_slot);
	emit statusText(tr("Click on plot to choose the position of the new object!"));
}

AddWidgetTool::~AddWidgetTool()
{
	d_graph->unsetCursor();

	QwtPlotCanvas *canvas = d_graph->canvas();
	canvas->unsetCursor();
	canvas->removeEventFilter(this);

	QwtTextLabel *title = d_graph->titleLabel();
	if (title)
		title->removeEventFilter(this);

	for ( uint i = 0; i < QwtPlot::axisCnt; i++ ){
        QwtScaleWidget *scale = d_graph->axisWidget(i);
        if (scale)
            scale->removeEventFilter(this);
    }

	ApplicationWindow *app = d_graph->multiLayer()->applicationWindow();
	if (app)
		app->pickPointerCursor();

	d_action->setChecked(false);
    emit statusText("");
}

void AddWidgetTool::addEquation(const QPoint& point)
{
	TexWidget *t = new TexWidget(d_graph);
	if (!t)
		return;

	t->setFormula(tr("enter your text here"));
	t->move(point);
	t->setFrameStyle(1);
	ApplicationWindow *app = d_graph->multiLayer()->applicationWindow();
	if (app)
		t->setFramePen(app->d_frame_widget_pen);

    d_graph->add(t, false);
    t->showPropertiesDialog();
    d_graph->notifyChanges();
    d_graph->setActiveTool(nullptr);
}

void AddWidgetTool::addText(const QPoint& point)
{
	LegendWidget *l = new LegendWidget(d_graph);
	if (!l)
		return;

	l->move(point);

	d_graph->add(l, false);
	l->setText(tr("enter your text here"));
	ApplicationWindow *app = d_graph->multiLayer()->applicationWindow();
	if (app){
		l->setFrameStyle(app->legendFrameStyle);
		l->setFont(app->plotLegendFont);
		l->setTextColor(app->legendTextColor);
		l->setBackgroundColor(app->legendBackground);
		l->setFramePen(app->d_frame_widget_pen);
		l->setAngle(app->d_legend_default_angle);
	}
    l->showPropertiesDialog();
    d_graph->notifyChanges();
    d_graph->setActiveTool(nullptr);
}

void AddWidgetTool::addRectangle(const QPoint& point)
{
    if (!d_fw)
        d_fw = new RectangleWidget(d_graph);

	if (!d_fw)
		return;

	d_fw->setSize(0, 0);
	d_fw->move(point);
	d_fw->setFrameColor(Qt::blue);
	d_graph->add(d_fw, false);
	emit statusText(tr("Move cursor in order to resize the new rectangle!"));
	d_graph->notifyChanges();
}

void AddWidgetTool::addEllipse(const QPoint& point)
{
    if (!d_fw)
        d_fw = new EllipseWidget(d_graph);

	if (!d_fw)
		return;

	d_fw->setSize(0, 0);
	d_fw->move(point);
	d_fw->setFrameColor(Qt::blue);
	d_graph->add(d_fw, false);
	emit statusText(tr("Move cursor in order to resize the new ellipse!"));
	d_graph->notifyChanges();
}

void AddWidgetTool::addWidget(const QPoint& point)
{
	switch (d_widget_type){
		case Text:
			addText(point);
		break;
		case TexEquation:
			addEquation(point);
		break;
		case Rectangle:
			addRectangle(point);
		break;
		case Ellipse:
			addEllipse(point);
		break;
		default:
			break;
	}
}

bool AddWidgetTool::eventFilter(QObject *obj, QEvent *event)
{
	switch(event->type()) {
		case QEvent::MouseButtonPress:
			addWidget(d_graph->multiLayer()->canvas()->mapFromGlobal(QCursor::pos()));
			return true;
        break;

        case QEvent::MouseMove:
            if (d_fw){
                QRect r = d_fw->geometry();
                r.setBottomRight(d_graph->multiLayer()->canvas()->mapFromGlobal(QCursor::pos()));
                d_fw->setGeometry(r.normalized());
            }
        break;

        case QEvent::MouseButtonRelease:
            if (d_fw){
				ApplicationWindow *app = d_graph->multiLayer()->applicationWindow();
				if (app){
					d_fw->setFrameStyle(app->legendFrameStyle);
					if(d_widget_type == Ellipse)
						d_fw->setFramePen(app->d_frame_widget_pen);
					else {
						QPen pen = app->d_frame_widget_pen;
						pen.setWidthF(ceil((double)pen.width()));
						d_fw->setFramePen(pen);
					}

					d_fw->setBackgroundColor(app->d_rect_default_background);
					d_fw->setBrush(app->d_rect_default_brush);
				}

				d_fw->updateCoordinates();
                d_fw->repaint();
                d_fw = nullptr;
                emit statusText("");
                d_graph->setActiveTool(nullptr);
            }
        break;

		default:
			break;
	}
	return QObject::eventFilter(obj, event);
}
