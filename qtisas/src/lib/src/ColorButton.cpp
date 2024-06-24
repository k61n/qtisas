/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A wrapper around QtColorPicker from QtSolutions
 ******************************************************************************/

#include <QColorDialog>

#include "ColorBox.h"
#include "ColorButton.h"

ColorButton::ColorButton(QWidget *parent) : QtColorPicker(parent)
{
	QStringList color_names = ColorBox::defaultColorNames();
	QList<QColor> defaultColors = ColorBox::defaultColors();
	for (int i = 0; i < ColorBox::numPredefinedColors(); i++)
		insertColor(defaultColors[i], color_names[i]);

	QList<QColor> colors = ColorBox::colorList();
	color_names = ColorBox::colorNames();
	for (int i = 0; i < colors.count(); i++){
		QColor c = colors[i];
		if (!defaultColors.contains(c))
			insertColor(c, color_names[i]);
	}

	connect(this, SIGNAL(colorChanged(const QColor & )), this, SIGNAL(colorChanged()));
}
