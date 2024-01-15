/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2009 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: A wrapper around QtColorPicker from QtSolutions
 ******************************************************************************/

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <qtcolorpicker.h>

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
	ColorButton(QWidget *parent = 0);
	//! Set the current color to be displayed
	void setColor(const QColor& c){setCurrentColor (c);};
	//! Get the current color
	QColor color(){return currentColor();};

signals:
    void colorChanged();
};

#endif
