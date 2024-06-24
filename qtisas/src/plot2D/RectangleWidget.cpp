/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A widget displaying rectangles in 2D plots
 ******************************************************************************/

#include <QPaintEngine>
#include <QPainter>
#include <QPalette>

#include "Graph.h"
#include "MultiLayer.h"
#include "PatternBox.h"
#include "PenStyleBox.h"
#include "RectangleWidget.h"

RectangleWidget::RectangleWidget(Graph *plot):FrameWidget(plot),
d_linked_layer(-1)
{
	setFrameStyle(Line);
	setSize(100, 50);
	setObjectName(tr("Rectangle"));
}

void RectangleWidget::clone(RectangleWidget* r)
{
	d_frame = r->frameStyle();
	setFramePen(r->framePen());
	setBackgroundColor(r->backgroundColor());
	setBrush(r->brush());
	setCoordinates(r->xValue(), r->yValue(), r->right(), r->bottom());
}

QString RectangleWidget::saveToString()
{
	QString s = "<Rectangle>\n";
	s += FrameWidget::saveToString();
	QColor bc = backgroundColor();
	s += "<Background>" + bc.name() + "</Background>\n";
	s += "<Alpha>" + QString::number(bc.alpha()) + "</Alpha>\n";
	s += "<BrushColor>" + d_brush.color().name() + "</BrushColor>\n";
	s += "<BrushStyle>" + QString::number(PatternBox::patternIndex(d_brush.style())) + "</BrushStyle>\n";
    s += "<LinkedLayer>" + QString::number(d_linked_layer) + "</LinkedLayer>\n";
	return s + "</Rectangle>\n";
}

void RectangleWidget::restore(Graph *g, const QStringList& lst)
{
	double x = 0.0, y = 0.0, right = 0.0, bottom = 0.0;
	QStringList::const_iterator line;
	QColor backgroundColor = Qt::white;
	QBrush brush = QBrush();
	RectangleWidget *r = new RectangleWidget(g);
	if (!r)
		return;

	for (line = lst.begin(); line != lst.end(); line++){
		QString s = *line;
		if (s.contains("<Frame>"))
			r->setFrameStyle(s.remove("<Frame>").remove("</Frame>").toInt());
		else if (s.contains("<Color>"))
			r->setFrameColor(QColor(s.remove("<Color>").remove("</Color>")));
		else if (s.contains("<FrameWidth>"))
			r->setFrameWidth(s.remove("<FrameWidth>").remove("</FrameWidth>").toInt());
		else if (s.contains("<LineStyle>"))
			r->setFrameLineStyle(PenStyleBox::penStyle(s.remove("<LineStyle>").remove("</LineStyle>").toInt()));
		else if (s.contains("<x>"))
			x = s.remove("<x>").remove("</x>").toDouble();
		else if (s.contains("<y>"))
			y = s.remove("<y>").remove("</y>").toDouble();
		else if (s.contains("<right>"))
			right = s.remove("<right>").remove("</right>").toDouble();
		else if (s.contains("<bottom>"))
			bottom = s.remove("<bottom>").remove("</bottom>").toDouble();
		else if (s.contains("<attachTo>"))
			r->setAttachPolicy((FrameWidget::AttachPolicy)s.remove("<attachTo>").remove("</attachTo>").toInt());
		else if (s.contains("<onTop>"))
			r->setOnTop(s.remove("<onTop>").remove("</onTop>").toInt());
		else if (s.contains("<visible>"))
			r->setVisible(s.remove("<visible>").remove("</visible>").toInt());
		else if (s.contains("<Background>"))
			backgroundColor = QColor(s.remove("<Background>").remove("</Background>"));
		else if (s.contains("<Alpha>"))
			backgroundColor.setAlpha(s.remove("<Alpha>").remove("</Alpha>").toInt());
		else if (s.contains("<BrushColor>"))
			brush.setColor(QColor(s.remove("<BrushColor>").remove("</BrushColor>")));
		else if (s.contains("<BrushStyle>"))
			brush.setStyle(PatternBox::brushStyle((s.remove("<BrushStyle>").remove("</BrushStyle>")).toInt()));
		else if (s.contains("<LinkedLayer>"))
			r->setLinkedLayer(s.remove("<LinkedLayer>").remove("</LinkedLayer>").toInt());
	}

	r->setBackgroundColor(backgroundColor);
	r->setBrush(brush);
	r->setCoordinates(x, y, right, bottom);
	g->add(r, false);
}

void RectangleWidget::updateCoordinates()
{
    FrameWidget::updateCoordinates();

    if (d_linked_layer >= 0 && !receivers("changedCoordinates"))
        setLinkedLayer(d_linked_layer);

    changedCoordinates(boundingRect());
}

void RectangleWidget::setLinkedLayer(int layerIndex)
{
    d_linked_layer = layerIndex;

    Graph *g = plot()->multiLayer()->layer(layerIndex + 1);
    if (!g)
        return;

    this->disconnect(g, SLOT(setCanvasCoordinates(const QRectF&)));
    connect(this, SIGNAL(changedCoordinates(const QRectF&)),
            g, SLOT(setCanvasCoordinates(const QRectF&)));
    connect(g, SIGNAL(destroyed(QObject *)), this, SLOT(closedLinkedLayer(QObject *)));
}

void RectangleWidget::closedLinkedLayer(QObject *)
{
    d_linked_layer = -1;
}
