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

ImageSymbol::ImageSymbol(const QString &fileName)
    : QwtSymbol(QwtSymbol::Pixmap, QBrush(), QPen(Qt::NoPen), QSize()), d_image_path(fileName)
{
	d_pixmap.load(fileName);
    setPixmap(d_pixmap);
	setSize(d_pixmap.size());
}

ImageSymbol::ImageSymbol(const QPixmap &pixmap, QString fileName)
    : QwtSymbol(QwtSymbol::Pixmap, QBrush(), QPen(Qt::NoPen), QSize()), d_image_path(std::move(fileName))
{
	d_pixmap = QPixmap(pixmap);
    setPixmap(d_pixmap);
	setSize(d_pixmap.size());
}

