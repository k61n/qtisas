/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A QwtSymbol displaying custom images
 ******************************************************************************/

#include "ImageSymbol.h"
#include <QPainter>
#include <qwt_painter.h>

ImageSymbol::ImageSymbol(const QString& fileName):
		QwtSymbol(QwtSymbol::Image, QBrush(), QPen(Qt::NoPen), QSize()),
	d_image_path(fileName)
{
	d_pixmap.load(fileName);
	setSize(d_pixmap.size());
}

ImageSymbol::ImageSymbol(const QPixmap& pixmap, const QString& fileName):
		QwtSymbol(QwtSymbol::Image, QBrush(), QPen(Qt::NoPen), QSize()),
	d_image_path(fileName)
{
	d_pixmap = QPixmap(pixmap);
	setSize(d_pixmap.size());
}

ImageSymbol *ImageSymbol::clone() const
{
	ImageSymbol *other = new ImageSymbol(d_image_path);
	*other = *this;

	return other;
}

/*!
  \brief Draw the symbol into a bounding rectangle.
  \param painter Painter
  \param r Bounding rectangle
*/
void ImageSymbol::draw(QPainter *p, const QRect& r) const
{
	p->drawPixmap(r, d_pixmap);
}
