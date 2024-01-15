/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2009 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: A wrapper around QtColorPicker from QtSolutions
 ******************************************************************************/

#include "ColorButton.h"
#include "ColorBox.h"
#include <QColorDialog>

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
