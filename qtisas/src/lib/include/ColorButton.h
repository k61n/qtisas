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

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include "qtcolorpicker.h"

//! A customized QtColorPicker used for color selection.
/**
 *
 */
class ColorButton : public QtColorPicker
{
	Q_OBJECT

public:
	//! Constructor.
	/**
	* \param parent parent widget (only affects placement of the widget)
	*/
    explicit ColorButton(QWidget *parent = nullptr);
	//! Set the current color to be displayed
	void setColor(const QColor& c){setCurrentColor (c);};
	//! Get the current color
	QColor color(){return currentColor();};

signals:
    void colorChanged();
};

#endif
