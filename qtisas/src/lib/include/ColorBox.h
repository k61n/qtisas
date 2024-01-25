/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A combo box to select a standard color
 ******************************************************************************/

#ifndef COLORBOX_H
#define COLORBOX_H

#include <QComboBox>

//! A modified QComboBox used for color selection.
/**
 * It contains a list of 24 predefined colors.
 */
class ColorBox : public QComboBox
{
	Q_OBJECT

public:
	//! Constructor
	/**
	 * \param parent parent widget
	 */
	ColorBox(QWidget *parent = 0);
	//! Set the current color
	void setColor(const QColor& c);
	//! Return the current color
	QColor color() const;
	//! Return the list of colors
	static QList<QColor> colorList();
	//! Returns the color names
	static QStringList colorNames();
	//! Return the index for a given color
	static int colorIndex(const QColor& c);
	//! Return the color at index 'colorIndex'
	static QColor color(int colorIndex);
	//! Return the default color at index 'colorIndex'
	static QColor defaultColor(int colorIndex);
    //! Returns true if the color is included in the color box, otherwise returns false.
    static bool isValidColor(const QColor& color);
	//! Returns the number of predefined colors
    static int numPredefinedColors();
    //! Returns the color name for the predefined colors
	static QStringList defaultColorNames();
	//! Returns the list of predefined colors
	static QList<QColor> defaultColors();

protected:
	//! Internal initialization function
	void init();
	//! The number of predefined colors
	static const int colors_count = 24;
	//! Array containing the 24 predefined colors
	static const QColor colors[];
};

#endif
