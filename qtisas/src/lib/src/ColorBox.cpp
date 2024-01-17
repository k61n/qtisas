/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Alex Kargovsky <kargovsky@yumr.phys.msu.su>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A combo box to select a standard color
 ******************************************************************************/

#include "ColorBox.h"

#include <QSettings>
#include <QPixmap>
#include <QPainter>
#include <algorithm>

const QColor ColorBox::colors[] = {
  QColor(Qt::black),
  QColor(Qt::red),
  QColor(Qt::green),
  QColor(Qt::blue),
  QColor(Qt::cyan),
  QColor(Qt::magenta),
  QColor(Qt::yellow),
  QColor(Qt::darkYellow),
  QColor(Qt::darkBlue),
  QColor(Qt::darkMagenta),
  QColor(Qt::darkRed),
  QColor(Qt::darkGreen),
  QColor(Qt::darkCyan),
  QColor("#0000A0"),
  QColor("#FF8000"),
  QColor("#8000FF"),
  QColor("#FF0080"),
  QColor(Qt::white),
  QColor(Qt::lightGray),
  QColor(Qt::gray),
  QColor("#FFFF80"),
  QColor("#80FFFF"),
  QColor("#FF80FF"),
  QColor(Qt::darkGray),
};

ColorBox::ColorBox(QWidget *parent) : QComboBox(parent)
{
    setEditable(false);
	init();
}

void ColorBox::init()
{
	QList<QColor> indexedColors = colorList();
	QStringList color_names = colorNames();

	QPixmap icon = QPixmap(28, 16);
	QRect r = QRect(0, 0, 27, 15);

	QPainter p;
	p.begin(&icon);

	for (int i = 0; i < indexedColors.size(); i++){
		p.setBrush(QBrush(indexedColors[i]));
		p.drawRect(r);
		this->addItem(icon, color_names[i]);
	}
	p.end();
}

void ColorBox::setColor(const QColor& c)
{
	setCurrentIndex(colorIndex(c));
}

QColor ColorBox::color() const
{
	return color(this->currentIndex());
}

int ColorBox::colorIndex(const QColor& c)
{
	if (!isValidColor(c))
		return 0;

	return colorList().indexOf(c);
}

QColor ColorBox::color(int colorIndex)
{
	QList<QColor> colorsList = colorList();
	if (colorIndex >= 0 && colorIndex < colorsList.size())
		return colorsList[colorIndex];

	return Qt::black; // default color is black.
}

QList<QColor> ColorBox::colorList()
{
#ifdef Q_OS_MACOS
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#endif
	settings.beginGroup("/General");

	QList<QColor> indexedColors;
	QStringList lst = settings.value("/IndexedColors").toStringList();
	if (!lst.isEmpty()){
		for (int i = 0; i < lst.size(); i++)
			indexedColors << QColor(lst[i]);
	} else {
		for (int i = 0; i < colors_count; i++)
			indexedColors << colors[i];
	}
	settings.endGroup();

	return indexedColors;
}

QStringList ColorBox::colorNames()
{
#ifdef Q_OS_MACOS
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#endif

	settings.beginGroup("/General");
	QStringList color_names = settings.value("/IndexedColorNames", defaultColorNames()).toStringList();
	settings.endGroup();
	return color_names;
}

QColor ColorBox::defaultColor(int colorIndex)
{
	if (colorIndex >= 0 && colorIndex < (int)sizeof(colors))
		return colors[colorIndex];

	return Qt::black; // default color is black.
}

bool ColorBox::isValidColor(const QColor& color)
{
	return colorList().contains(color);
}

int ColorBox::numPredefinedColors()
{
	return colors_count;
}

QStringList ColorBox::defaultColorNames()
{
	QStringList color_names = QStringList() << tr( "black" );
	color_names << tr( "red" );
	color_names << tr( "green" );
	color_names << tr( "blue" );
	color_names << tr( "cyan" );
	color_names << tr( "magenta" );
	color_names << tr( "yellow" );
	color_names << tr( "dark yellow" );
	color_names << tr( "navy" );
	color_names << tr( "purple" );
	color_names << tr( "wine" );
	color_names << tr( "olive" );
	color_names << tr( "dark cyan" );
	color_names << tr( "royal" );
	color_names << tr( "orange" );
	color_names << tr( "violet" );
	color_names << tr( "pink" );
	color_names << tr( "white" );
	color_names << tr( "light gray" );
	color_names << tr( "gray" );
	color_names << tr( "light yellow" );
	color_names << tr( "light cyan" );
	color_names << tr( "light magenta" );
	color_names << tr( "dark gray" );
	return color_names;
}

QList<QColor> ColorBox::defaultColors()
{
	QList<QColor> lst;
	for (int i = 0; i < colors_count; i++)
		lst << colors[i];

	return lst;
}
