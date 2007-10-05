/***************************************************************************
    File                 : Legend.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Legend marker (extension to QwtPlotMarker)

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "Legend.h"
#include "QwtPieCurve.h"
#include "VectorCurve.h"

#include <QPainter>
#include <QPolygon>
#include <QMessageBox>

#include <qwt_plot.h>
#include <qwt_scale_widget.h>
#include <qwt_painter.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_canvas.h>
#include <qwt_layout_metrics.h>
#include <qwt_symbol.h>

#include <iostream>

Legend::Legend(Plot *plot):
	d_plot(plot),
	d_frame (0),
	d_angle(0)
{
	d_text = new QwtText(QString::null, QwtText::RichText);
	d_text->setFont(QFont("Arial", 12, QFont::Normal, false));
	d_text->setRenderFlags(Qt::AlignTop|Qt::AlignLeft);
	d_text->setBackgroundBrush(QBrush(Qt::NoBrush));
	d_text->setColor(Qt::black);
	d_text->setBackgroundPen (QPen(Qt::NoPen));
	d_text->setPaintAttribute(QwtText::PaintBackground);

	h_space = 5;
	left_margin = 10;
	top_margin = 5;
	line_length = 20;
}

void Legend::draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &) const
{
	const int x = xMap.transform(xValue());
	const int y = yMap.transform(yValue());

	const int symbolLineLength = line_length + symbolsMaxWidth();

	int width, height;
	QwtArray<long> heights = itemsHeight(y, symbolLineLength, width, height);

	QRect rs = QRect(QPoint(x, y), QSize(width, height));
	drawFrame(p, d_frame, rs);
	drawText(p, rs, heights, symbolLineLength);
}

void Legend::setText(const QString& s)
{
	d_text->setText(s);
}

void Legend::setFrameStyle(int style)
{
	if (d_frame == style)
		return;

	d_frame = style;
}

void Legend::setBackgroundColor(const QColor& c)
{
	if (d_text->backgroundBrush().color() == c)
		return;

	d_text->setBackgroundBrush(QBrush(c));
}

QRect Legend::rect() const
{
	const QwtScaleMap &xMap = d_plot->canvasMap(xAxis());
	const QwtScaleMap &yMap = d_plot->canvasMap(yAxis());

	const int x = xMap.transform(xValue());
	const int y = yMap.transform(yValue());

	int width, height;
	itemsHeight(y, line_length + symbolsMaxWidth(), width, height);

	return QRect(QPoint(x, y), QSize(width - 1, height - 1));
}

QwtDoubleRect Legend::boundingRect() const
{
	QRect bounding_rect = rect();
	const QwtScaleMap &x_map = d_plot->canvasMap(xAxis());
	const QwtScaleMap &y_map = d_plot->canvasMap(yAxis());

	double left = x_map.invTransform(bounding_rect.left());
	double right = x_map.invTransform(bounding_rect.right());
	double top = y_map.invTransform(bounding_rect.top());
	double bottom = y_map.invTransform(bounding_rect.bottom());

	return QwtDoubleRect(left, top, qAbs(right-left), qAbs(bottom-top));
}

void Legend::setTextColor(const QColor& c)
{
	if ( c == d_text->color() )
		return;

	d_text->setColor(c);
}

void Legend::setOrigin( const QPoint & p )
{
	d_pos = p;

	const QwtScaleMap &xMap = d_plot->canvasMap(xAxis());
	const QwtScaleMap &yMap = d_plot->canvasMap(yAxis());

	setXValue (xMap.invTransform(p.x()));
	setYValue (yMap.invTransform(p.y()));
}

void Legend::updateOrigin()
{
	if (!d_plot)
		return;

	const QwtScaleMap &xMap = d_plot->canvasMap(xAxis());
	const QwtScaleMap &yMap = d_plot->canvasMap(yAxis());

	const QwtScaleDiv *xScDiv = d_plot->axisScaleDiv (xAxis());
	double xVal = xMap.invTransform(d_pos.x());
  	if (!xScDiv->contains(xVal))
        return;

  	const QwtScaleDiv *yScDiv = d_plot->axisScaleDiv (yAxis());
  	double yVal = yMap.invTransform(d_pos.y());
  	if (!yScDiv->contains(yVal))
        return;

  	setXValue (xVal);
  	setYValue (yVal);
}

void Legend::setOriginCoord(double x, double y)
{
	if (xValue() == x && yValue() == y)
		return;

	setXValue(x);
	setYValue(y);

	const QwtScaleMap &xMap = d_plot->canvasMap(xAxis());
	const QwtScaleMap &yMap = d_plot->canvasMap(yAxis());

	d_pos = QPoint(xMap.transform(x), yMap.transform(y));
}

void Legend::setFont(const QFont& font)
{
	if ( font == d_text->font() )
		return;

	d_text->setFont(font);
}

void Legend::drawFrame(QPainter *p, int type, const QRect& rect) const
{
	p->save();
	p->setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
	if (type == None)
		p->fillRect (rect, d_text->backgroundBrush());

	if (type == Line){
		p->setBrush(d_text->backgroundBrush());
		QwtPainter::drawRect(p, rect);
	} else if (type == Shadow) {
		QRect shadow_right = QRect(rect.right(), rect.y() + 5, 5, rect.height()-1);
		QRect shadow_bottom = QRect(rect.x() + 5, rect.bottom(), rect.width()-1, 5);
		p->setBrush(QBrush(Qt::black));
		p->drawRect(shadow_right);
		p->drawRect(shadow_bottom);

		p->setBrush(d_text->backgroundBrush());
		QwtPainter::drawRect(p,rect);
	}
	p->restore();
}

void Legend::drawVector(QPainter *p, int x, int y, int l, int curveIndex) const
{
	Graph *g = (Graph *)d_plot->parent();
	if (!g)
		return;

	VectorCurve *v = (VectorCurve*)g->curve(curveIndex);
	if (!v)
		return;

	p->save();

	QPen pen(v->color(), v->width(), Qt::SolidLine);
	p->setPen(pen);
	QwtPainter::drawLine(p, x, y, x + l, y);

	p->translate(x+l, y);

	double pi=4*atan(-1.0);
	int headLength = v->headLength();
	int d=qRound(headLength*tan(pi*(double)v->headAngle()/180.0));

	QPolygon endArray(3);
	endArray[0] = QPoint(0, 0);
	endArray[1] = QPoint(-headLength, d);
	endArray[2] = QPoint(-headLength, -d);

	if (v->filledArrowHead())
		p->setBrush(QBrush(pen.color(), Qt::SolidPattern));

	QwtPainter::drawPolygon(p,endArray);
	p->restore();
}

void Legend::drawText(QPainter *p, const QRect& rect,
		QwtArray<long> height, int symbolLineLength) const
{
	Graph *g = (Graph *) d_plot->parent();
	int l = symbolLineLength;
	QString text = d_text->text();
	QStringList titles = text.split("\n", QString::KeepEmptyParts);

	for (int i=0; i<(int)titles.count(); i++){
        int w = rect.x() + left_margin;
		QString s = titles[i];
		while (s.contains("\\l(")){
			int pos = s.indexOf("\\l(", 0);			
			QwtText aux(parse(s.left(pos)));
			aux.setFont(d_text->font());
			aux.setColor(d_text->color());
			
			QSize size = aux.textSize();
			QRect tr = QRect(QPoint(w, height[i] - size.height()/2), size);
			aux.draw(p, tr);
			w += size.width();
			
		    int pos1 = s.indexOf("(", pos);
            int pos2 = s.indexOf(")", pos1);
			int cv = s.mid(pos1+1, pos2-pos1-1).toInt() - 1;
			if (cv < 0 || cv >= g->curves()){
				s = s.right(s.length() - pos2 - 1);
				continue;
			}

			if (g->curveType(cv) == Graph::VectXYXY || g->curveType(cv) == Graph::VectXYAM)
				drawVector(p, w, height[i], l, cv);
			else {
				const QwtPlotCurve *curve = g->curve(cv);
				if (curve && curve->rtti() != QwtPlotItem::Rtti_PlotSpectrogram) {
					QwtSymbol symb = curve->symbol();
					const QBrush br = curve->brush();
					QPen pen = curve->pen();
					p->save();
					if (curve->style()!=0){
						p->setPen (pen);
						if (br.style() != Qt::NoBrush || g->curveType(cv) == Graph::Box) {
							QRect lr = QRect(w, height[i]-4, l, 10);
							p->setBrush(br);
							QwtPainter::drawRect(p, lr);
						} else
							QwtPainter::drawLine(p, w, height[i], w+l, height[i]);
					}
					int symb_size = symb.size().width();
  	                if (symb_size > 15)
  	                	symb_size = 15;
  	                else if (symb_size < 3)
  	                	symb_size = 3;
  	                symb.setSize(symb_size);
					symb.draw(p, w+l/2, height[i]);
					p->restore();
				}
			}
        w += l + h_space;
		s = s.right(s.length() - pos2 - 1);
		}

		if (!s.isEmpty()){
			w += h_space;
			QwtText aux(parse(s));
			aux.setFont(d_text->font());
			aux.setColor(d_text->color());
			QSize size = aux.textSize();
			QRect tr = QRect(QPoint(w, height[i] - size.height()/2), size);
			aux.draw(p, tr);
		}
		
        /*if (titles[i].contains("\\p{")) {
			int pos=titles[i].indexOf("{",0);
			int pos2=titles[i].indexOf("}",pos);
			QString aux=titles[i].mid(pos+1,pos2-pos-1);

			int id=aux.toInt();

			Graph* g=(Graph*)d_plot->parent();
			if (g->isPiePlot()){
				QwtPieCurve *curve = (QwtPieCurve *)d_plot->curve(1);
				if (curve){
					const QBrush br = QBrush(curve->color(id-1), curve->pattern());
					QPen pen = curve->pen();

					p->save();
					p->setPen (QPen(pen.color(), 1, Qt::SolidLine));
					QRect lr=QRect(w,height[i]-4,l,10);
					p->setBrush(br);
					QwtPainter::drawRect(p, lr);
					p->restore();
				}
			}
		}*/
	}
}

QwtArray<long> Legend::itemsHeight(int y, int symbolLineLength, int &width, int &height) const
{
	Graph *g = (Graph *) d_plot->parent();
	QString text = d_text->text();
	QStringList titles = text.split("\n", QString::KeepEmptyParts);
	int n = (int)titles.count();
	QwtArray<long> heights(n);

	width = 0;
	height = 0;
	int maxL = 0;
	int h = top_margin;
	for (int i=0; i<n; i++){
		QString s = titles[i];		
		int textL = 0;
		while (s.contains("\\l(")){
			int pos = s.indexOf("\\l(", 0);			
			QwtText aux(parse(s.left(pos)));
			aux.setFont(d_text->font());			
			QSize size = aux.textSize();
			textL += size.width();			
			
			int pos1 = s.indexOf("(", pos);
            int pos2 = s.indexOf(")", pos1);
			int cv = s.mid(pos1+1, pos2-pos1-1).toInt() - 1;
			if (cv < 0 || cv >= g->curves()){
				s = s.right(s.length() - pos2 - 1);
				continue;
			}
			textL += symbolLineLength + 2*h_space;
			s = s.right(s.length() - s.indexOf(")", pos) - 1);
		}
		
		QwtText aux(parse(s));
		aux.setFont(d_text->font());
		textL += aux.textSize().width();
		
		if (textL > maxL)
			maxL = textL;

		int textH = aux.textSize().height();
		height += textH;

		heights[i] = y + h + textH/2;
		h += textH;
	}

	height += 2*top_margin;
	width = 2*left_margin + maxL;

	return heights;
}

int Legend::symbolsMaxWidth() const
{
	QList<int> cvs = d_plot->curveKeys();
	int curves = cvs.count();
	if (!curves)
		return 0;
	
	int maxL = 0;
	QString text = d_text->text();
	QStringList titles = text.split("\n", QString::KeepEmptyParts);
	for (int i=0; i<(int)titles.count(); i++){
		QString s = titles[i];
		while (s.contains("\\l(")){
			int pos = s.indexOf("\\l(", 0);
		    int pos1 = s.indexOf("(", pos);
            int pos2 = s.indexOf(")", pos1);
			int cv = s.mid(pos1+1, pos2-pos1-1).toInt()-1;
			if (cv < 0 || cv >= curves){
				s = s.right(s.length() - pos2 - 1);
				continue;
			}

			const QwtPlotCurve *c = (QwtPlotCurve *)d_plot->curve(cvs[cv]);
			if (c && c->rtti() != QwtPlotItem::Rtti_PlotSpectrogram) {
				int l = c->symbol().size().width();
				if (l < 3)
  	            	l = 3;
  	            else if (l > 15)
  	            	l = 15;
  	            if (l>maxL && c->symbol().style() != QwtSymbol::NoSymbol)
					maxL = l;
			}
			s = s.right(s.length() - pos2 - 1);
		}

		if (titles[i].contains("\\p{"))
			maxL = 10;
	}
	return maxL;
}

QString Legend::parse(const QString& str) const
{
    QString s = str;
    s.remove(QRegExp("\\l(*)", Qt::CaseSensitive, QRegExp::Wildcard));
    s.remove(QRegExp("\\p{*}", Qt::CaseSensitive, QRegExp::Wildcard));
	
	QString aux = str;
    while (aux.contains(QRegExp("%(*)", Qt::CaseInsensitive, QRegExp::Wildcard))){//curve name specification
        int pos = str.indexOf("%(", 0, Qt::CaseInsensitive);
        int pos2 = str.indexOf(")", pos, Qt::CaseInsensitive);
        int cv = str.mid(pos+2, pos2-pos-2).toInt() - 1;
		Graph *g = (Graph *)d_plot->parent();
        if (g && cv >= 0 && cv < g->curves()){
			const QwtPlotCurve *c = (QwtPlotCurve *)g->curve(cv);
            if (c)
                s = s.replace(pos, pos2-pos+1, c->title().text());
        }
		aux = aux.right(aux.length() - pos2 - 1);
    }
    return s;
}

Legend::~Legend()
{
	delete d_text;
}
