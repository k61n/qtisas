/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Canvas picker
 ******************************************************************************/

#include <QDrag>
#include <QMimeData>
#include <QVector>

#include <qwt/qwt_plot_canvas.h>
#include <qwt/qwt_text_label.h>

#include "ApplicationWindow.h"
#include "ArrowMarker.h"
#include "CanvasPicker.h"
#include "PlotCurve.h"
#include "Spectrogram.h"

CanvasPicker::CanvasPicker(Graph *graph):
	QObject(graph)
{
	pointSelected = false;
    d_editing_marker = nullptr;

	QwtPlotCanvas *canvas = graph->canvas();
	canvas->installEventFilter(this);
}

bool CanvasPicker::eventFilter(QObject *object, QEvent *e)
{
	if (object != (QObject *)plot()->canvas())
		return false;

	Graph *g = plot();
	QList<ArrowMarker *> lines = g->arrowsList();
	switch(e->type()){
		case QEvent::MouseButtonPress:
			{
        auto me = (const QMouseEvent *)e;

				if (!(me->modifiers() & Qt::ShiftModifier))
					g->deselect();
	
				g->activateGraph();
	
				if (!g->zoomOn() && selectMarker(me)){
					if (me->button() == Qt::RightButton)
						emit showMarkerPopupMenu();
					return true;
				}

				int dist, point;
				if (g->closestCurve(me->pos().x(), me->pos().y(), dist, point))
					return true;

				if (me->button() == Qt::LeftButton && (g->drawLineActive())){
					startLinePoint = me->pos();
					return true;
				}

				if (!g->zoomOn()){
					QList<FrameWidget *> eLst = g->increasingAreaEnrichmentsList();
					foreach(FrameWidget *fw, eLst){
						QPoint p = plot()->canvas()->mapTo(plot()->multiLayer()->canvas(), me->pos());
						if (fw->frameGeometry().contains(p)){
							fw->mousePressEvent((QMouseEvent *)e);
							if (me->button() == Qt::RightButton)
								emit showMarkerPopupMenu();

							return true;
						}
					}
				}

				if (me->button() == Qt::LeftButton && !g->zoomOn() &&
					!g->hasPanningMagnifierEnabled() && !g->activeTool() && !g->selectedCurveLabels()){
					g->selectCanvas();

                    auto drag = new QDrag(plot());
                    auto mimeData = new QMimeData;
					QPoint p = plot()->canvas()->mapToParent(me->pos());
					mimeData->setText(QString::number(abs(plot()->x() - p.x())) + ";" +
									QString::number(abs(plot()->y() - p.y())));
					drag->setMimeData(mimeData);
					drag->setPixmap(plot()->multiLayer()->windowIcon().pixmap(16));
					drag->exec();
					return true;
				}

				return !(me->modifiers() & Qt::ShiftModifier);
			}
			break;

		case QEvent::MouseButtonDblClick:
			{
				if (d_editing_marker) {
					return d_editing_marker->eventFilter(g->canvas(), e);
				} else if (g->selectedArrow()) {
					if (lines.contains(g->selectedArrow())){
						emit viewLineDialog();
						return true;
					}
				} else if (g->isPiePlot()){
                	emit showPlotDialog(0);
                    return true;
				} else {
            auto me = (const QMouseEvent *)e;
                    int dist, point;
                    QwtPlotItem *c = g->closestCurve(me->pos().x(), me->pos().y(), dist, point);
                    if (c && dist < 10)
                        emit showPlotDialog(g->curveIndex(c));
                    else
                        emit showPlotDialog(-1);
					return true;
				}
			}
			break;

		case QEvent::MouseMove:
			{
        auto me = (const QMouseEvent *)e;
				if (me->buttons() != Qt::LeftButton)
  	            	return true;

				QPoint pos = me->pos();

				QwtPlotItem *c = g->selectedCurveLabels();
				if (c){
					if (c->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
						((Spectrogram *)c)->moveLabel(pos);
					else
						((DataCurve *)c)->moveLabels(pos);
					return true;
				}

				if (plot()->drawLineActive()) {
					drawLineMarker(pos, g->drawArrow());
					return true;
				}

				return false;
			}
			break;

		case QEvent::MouseButtonRelease:
			{
        auto me = (const QMouseEvent *)e;
				if (g->drawLineActive()) {
					ApplicationWindow *app = g->multiLayer()->applicationWindow();
					if (!app)
						return true;

					ArrowMarker mrk;
					mrk.attach(g);
					mrk.setAttachPolicy((ArrowMarker::AttachPolicy)app->d_graph_attach_policy);
					mrk.setStartPoint(startLinePoint);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            mrk.setEndPoint(me->pos());
#else
            mrk.setEndPoint(me->position().toPoint());
#endif
					mrk.setColor(app->defaultArrowColor);
					mrk.setWidth(app->defaultArrowLineWidth);
					mrk.setStyle(app->defaultArrowLineStyle);
					mrk.setHeadLength(app->defaultArrowHeadLength);
					mrk.setHeadAngle(app->defaultArrowHeadAngle);
					mrk.fillArrowHead(app->defaultArrowHeadFill);
					mrk.drawEndArrow(g->drawArrow());
					mrk.drawStartArrow(false);

					g->addArrow(&mrk);
					g->drawLine(false);
					mrk.detach();
					g->replot();

					return true;
				}
				return false;
			}
			break;

		case QEvent::KeyPress:
			{
				int key = ((const QKeyEvent *)e)->key();

				ArrowMarker *selectedMarker = g->selectedArrow();
				if (lines.contains(selectedMarker) &&
					(key == Qt::Key_Enter || key == Qt::Key_Return)){
					emit viewLineDialog();
					return true;
				}
			}
			break;

		default:
			break;
	}
	return QObject::eventFilter(object, e);
}

void CanvasPicker::disableEditing()
{
	if (d_editing_marker) {
		d_editing_marker->setEditable(false);
		d_editing_marker = nullptr;
	}
}

void CanvasPicker::drawLineMarker(const QPoint& point, bool endArrow)
{
	Graph *g = plot();
	g->canvas()->repaint();
	ArrowMarker mrk;
	mrk.attach(g);

	int clw = g->canvas()->lineWidth();
	mrk.setStartPoint(QPoint(startLinePoint.x() + clw, startLinePoint.y() + clw));
	mrk.setEndPoint(QPoint(point.x() + clw,point.y() + clw));
	mrk.setWidth(1);
	mrk.setStyle(Qt::SolidLine);
	mrk.drawEndArrow(endArrow);
	mrk.drawStartArrow(false);

	if (plot()->drawLineActive())
		mrk.setColor(Qt::black);
	else
		mrk.setColor(Qt::red);

	g->replot();
	mrk.detach();
}

bool CanvasPicker::selectMarker(const QMouseEvent *e)
{
	Graph *g = plot();
	const QPoint point = e->pos();
	QList<ArrowMarker *> lines = g->arrowsList();
	foreach(ArrowMarker *i, lines){
		double dist = i->dist(point.x(), point.y());
		if (dist <= i->arrowWidth()){
			disableEditing();
			if (e->modifiers() & Qt::ShiftModifier) {
				plot()->setSelectedArrow(i, true);
				return true;
			} else if (e->button() == Qt::RightButton) {
				i->setEditable(false);
				g->setSelectedArrow(i, true);
				return true;
			}
			g->deselectMarker();
			i->setEditable(true);
			g->setSelectedArrow(i, false);
			d_editing_marker = i;
			return true;
		}
	}
	return false;
}
