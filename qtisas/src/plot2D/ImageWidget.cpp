/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A widget displaying images in 2D plots
 ******************************************************************************/

#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>
#include <QPainter>

#include "ApplicationWindow.h"
#include "Graph.h"
#include "Graph3D.h"
#include "ImageWidget.h"
#include "MultiLayer.h"
#include "PenStyleBox.h"
#include "Table.h"

ImageWidget::ImageWidget(Graph *plot, const QString &fn)
    : FrameWidget(plot), d_save_xpm(false), d_window_name(QString())
{
	if (load(fn, false)){
		QSize picSize = d_pix.size();
		int w = plot->canvas()->width();
		if (picSize.width() > w)
			picSize.setWidth(w);

		int h = plot->canvas()->height();
		if (picSize.height() > h)
			picSize.setHeight(h);

		setSize(picSize);
	}
	move(plot->mapToParent(plot->canvas()->pos()));
	setObjectName(tr("Image"));
}

ImageWidget::ImageWidget(Graph *plot, const QImage &image)
    : FrameWidget(plot), d_save_xpm(true), d_window_name(QString())
{
	d_pix = QPixmap::fromImage(image);

	QSize picSize = image.size();
	int w = plot->canvas()->width();
	if (picSize.width() > w)
		picSize.setWidth(w);

	int h = plot->canvas()->height();
	if (picSize.height() > h)
		picSize.setHeight(h);

	setSize(picSize);
	move(plot->mapToParent(plot->canvas()->pos()));
	setObjectName(tr("Image"));
}

bool ImageWidget::load(const QString& fn, bool update)
{
    if (fn.isEmpty())
        return false;

    QFileInfo fi(fn);
    if (!fi.exists ()){
        QMessageBox::critical((QWidget *)plot()->multiLayer()->applicationWindow(),
        tr("QtiSAS - File openning error"),
        tr("The file: <b>%1</b> doesn't exist!").arg(fn));
        return false;
    }
    if (!fi.isReadable()){
        QMessageBox::critical((QWidget *)plot()->multiLayer()->applicationWindow(),
        tr("QtiSAS - File openning error"),
        tr("You don't have the permission to open this file: <b>%1</b>").arg(fn));
        return false;
    }

	QList<QByteArray> lst = QImageReader::supportedImageFormats() << "JPG";
	for (int i=0; i<(int)lst.count(); i++){
		if (fn.contains("." + lst[i])){
            d_pix.load(fn, lst[i], Qt::AutoColor);
			d_file_name = fn;
			if (update)
				repaint();
			return true;
		}
	}
	return false;
}

QPixmap ImageWidget::windowPixmap(ApplicationWindow *mw, const QString& name, const QSize& size)
{
	if (!mw || name.isEmpty())
		return QPixmap();

	MdiSubWindow *w = mw->window(name);
	MultiLayer *ml = qobject_cast<MultiLayer *> (w);
	if (ml)
		return ml->canvasPixmap(size);

	Graph3D *g = qobject_cast<Graph3D *> (w);
	if (g)
        return QPixmap::fromImage(g->surface()->grabFramebuffer());

	Table *t = qobject_cast<Table *> (w);
	if (t)
        return t->table()->grab();

	return QPixmap();
}

void ImageWidget::paintEvent(QPaintEvent *e)
{
	if (d_pix.isNull()){
		if (!d_window_name.isEmpty()){
			ApplicationWindow *mw = plot()->multiLayer()->applicationWindow();
			if (!mw)
				return;

			d_pix = windowPixmap(mw, d_window_name, size());
		} else
			return;
	}

	QPainter p(this);
	draw(&p, rect());
	e->accept();
}

void ImageWidget::print(QPainter *painter, const QwtScaleMap map[QwtPlot::axisCnt])
{
	int x = map[QwtPlot::xBottom].transform(calculateXValue());
	int y = map[QwtPlot::yLeft].transform(calculateYValue());

	int right = map[QwtPlot::xBottom].transform(calculateRightValue());
	int bottom = map[QwtPlot::yLeft].transform(calculateBottomValue());

	draw(painter, QRect(x, y, abs(right - x), abs(bottom - y)));
}

void ImageWidget::draw(QPainter *painter, const QRect& rect)
{
	QRect r = rect;
	drawFrame(painter, r);

	int lw = d_frame_pen.width();
    switch(d_frame){
		case None:
			break;
		case Line:
			//r.adjust(0, 0, -1, -1);
			if (lw % 2)
				r.adjust(lw - 1, lw - 1, -lw, -lw);
			else
				r.adjust(lw - 1, lw - 1, -lw - 1, -lw - 1);
		break;
		case Shadow:
			//r.adjust(0, 0, -d_shadow_width - 1, -d_shadow_width - 1);
			if (lw == 3)
				r.adjust(lw - 1, lw - 1, -d_shadow_width - lw + 1, -d_shadow_width - lw + 1);
			else
				r.adjust(lw - 1, lw - 1, -d_shadow_width - lw, -d_shadow_width - lw);
		break;
	}

    if (d_frame != None && (painter->device()->devType() == QInternal::Widget ||
        painter->device()->devType() == QInternal::Pixmap))
        r.adjust(1, 1, 0, 0);

	painter->drawPixmap(r, d_pix);
}

void ImageWidget::setPixmap(const QPixmap& pix)
{
	d_pix = pix;
	int width = pix.width();
	int height = pix.height();
	if (d_frame == Shadow){
		width += d_shadow_width;
		height += d_shadow_width;
	}
	resize(QSize(width, height));
	repaint();
}

void ImageWidget::clone(ImageWidget* t)
{
	d_frame = t->frameStyle();
	setFramePen(t->framePen());
	d_file_name = t->fileName();
	d_save_xpm = t->saveInternally();
	setPixmap(t->pixmap());
	setCoordinates(t->xValue(), t->yValue(), t->right(), t->bottom());
}

QString ImageWidget::saveToString()
{
	QString s = "<Image>\n";
	s += FrameWidget::saveToString();
	s += "<path>" + d_file_name + "</path>\n";
	if (d_save_xpm){
		s += "<xpm>\n";
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		d_pix.save(&buffer, "XPM");
		s += QString(bytes);
		s += "</xpm>\n";
	}
	return s + "</Image>\n";
}

void ImageWidget::restore(Graph *g, const QStringList& lst)
{
	int frameStyle = 0;
	QPen pen = QPen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	double x = 0.0, y = 0.0, right = 0.0, bottom = 0.0;
	QStringList::const_iterator line;
	QString fn;
	bool save_xpm = false;
	ImageWidget *i = nullptr;
	AttachPolicy attachTo = FrameWidget::Page;
	bool onTop = true;
	for (line = lst.begin(); line != lst.end(); line++){
        QString s = *line;
        if (s.contains("<Frame>"))
			frameStyle = s.remove("<Frame>").remove("</Frame>").toInt();
		else if (s.contains("<Color>"))
			pen.setColor(QColor(s.remove("<Color>").remove("</Color>")));
		else if (s.contains("<FrameWidth>"))
			pen.setWidth(s.remove("<FrameWidth>").remove("</FrameWidth>").toInt());
		else if (s.contains("<LineStyle>"))
			pen.setStyle(PenStyleBox::penStyle(s.remove("<LineStyle>").remove("</LineStyle>").toInt()));
		else if (s.contains("<x>"))
			x = s.remove("<x>").remove("</x>").toDouble();
		else if (s.contains("<y>"))
			y = s.remove("<y>").remove("</y>").toDouble();
		else if (s.contains("<right>"))
			right = s.remove("<right>").remove("</right>").toDouble();
		else if (s.contains("<bottom>"))
			bottom = s.remove("<bottom>").remove("</bottom>").toDouble();
		else if (s.contains("<attachTo>"))
			attachTo = (FrameWidget::AttachPolicy)s.remove("<attachTo>").remove("</attachTo>").toInt();
		else if (s.contains("<onTop>"))
			onTop = s.remove("<onTop>").remove("</onTop>").toInt();
		else if (s.contains("<path>"))
			i = g->addImage(s.remove("<path>").remove("</path>"));
		else if (s.contains("<xpm>")){
			save_xpm = true;
			if (!i){
				QString xpm;
				while ( s != "</xpm>" ){
					s = *(++line);
					xpm += s + "\n";
				}
				QImage image;
                if (image.loadFromData(xpm.toLocal8Bit()))
					i = g->addImage(image);
			}
		}
	}

	if (i){
		i->setFrameStyle(frameStyle);
		i->setFramePen(pen);
		i->setCoordinates(x, y, right, bottom);
		i->setSaveInternally(save_xpm);
		i->setAttachPolicy(attachTo);
		i->setOnTop(onTop);
	}
}
