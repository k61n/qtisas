/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A widget displaying ellipses/circles in 2D plots
 ******************************************************************************/

#include <QPainter>
#include <QPaintEngine>
#include <QPainterPath>
#include <QPalette>

#include <qwt_painter.h>

#include "EllipseWidget.h"
#include "Graph.h"
#include "PatternBox.h"
#include "PenStyleBox.h"

EllipseWidget::EllipseWidget(Graph *plot):FrameWidget(plot)
{
	setFrameStyle(Line);
	setObjectName(tr("Ellipse"));
}

void EllipseWidget::clone(EllipseWidget* r)
{
	d_frame = r->frameStyle();
	setFramePen(r->framePen());
	setBackgroundColor(r->backgroundColor());
	setBrush(r->brush());
	setCoordinates(r->xValue(), r->yValue(), r->right(), r->bottom());
}

QString EllipseWidget::saveToString()
{
	QString s = "<Ellipse>\n";
	s += FrameWidget::saveToString();
	QColor bc = backgroundColor();
	s += "<Background>" + bc.name() + "</Background>\n";
	s += "<Alpha>" + QString::number(bc.alpha()) + "</Alpha>\n";
	s += "<BrushColor>" + d_brush.color().name() + "</BrushColor>\n";
	s += "<BrushStyle>" + QString::number(PatternBox::patternIndex(d_brush.style())) + "</BrushStyle>\n";
	return s + "</Ellipse>\n";
}

void EllipseWidget::restore(Graph *g, const QStringList& lst)
{
	double x = 0.0, y = 0.0, right = 0.0, bottom = 0.0;
	QStringList::const_iterator line;
	QColor backgroundColor = Qt::white;
	QBrush brush = QBrush();
	EllipseWidget *r = new EllipseWidget(g);
	if (!r)
		return;

	for (line = lst.begin(); line != lst.end(); line++){
		QString s = *line;
		if (s.contains("<Frame>"))
			r->setFrameStyle(s.remove("<Frame>").remove("</Frame>").toInt());
		else if (s.contains("<Color>"))
			r->setFrameColor(QColor(s.remove("<Color>").remove("</Color>")));
		else if (s.contains("<FrameWidth>"))
			r->setFrameWidth(s.remove("<FrameWidth>").remove("</FrameWidth>").toDouble());
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
	}

	r->setBackgroundColor(backgroundColor);
	r->setBrush(brush);
	r->setCoordinates(x, y, right, bottom);
	g->add(r, false);
}

void EllipseWidget::drawFrame(QPainter *p, const QRect& rect)
{
	p->save();
	if (d_plot->antialiasing())
		p->setRenderHints(QPainter::Antialiasing);

	QPainterPath ellipse;
	if (d_frame == Line){
		QPen pen = QwtPainter::scaledPen(d_frame_pen);
		p->setPen(pen);

		int lw = pen.width()/2;
		QRect r = rect.adjusted(lw + 1, lw + 1, -lw - 1, -lw - 1);

		ellipse.addEllipse(r);
		p->fillPath(ellipse, palette().color(QPalette::Window));
		if (d_brush.style() != Qt::NoBrush)
			p->setBrush(d_brush);

        p->drawEllipse(r);
	} else {
		ellipse.addEllipse(rect);
		p->fillPath(ellipse, palette().color(QPalette::Window));
		if (d_brush.style() != Qt::NoBrush)
			p->fillPath(ellipse, d_brush);
	}

	p->restore();
}
