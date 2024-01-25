/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A QwtSymbol displaying custom image
 ******************************************************************************/

#include <qwt_symbol.h>

//! A QwtSymbol displaying a custom image
class ImageSymbol: public QwtSymbol
{
public:
	ImageSymbol(const QString& fileName);
	ImageSymbol(const QPixmap& pixmap, const QString& fileName = QString());

	virtual ImageSymbol *clone() const;
	virtual void draw(QPainter *p, const QRect &r) const;

	QPixmap pixmap(){return d_pixmap;};
	QString imagePath(){return d_image_path;};

private:
	QString d_image_path;
	QPixmap d_pixmap;
};
