/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Pen style combobox
 ******************************************************************************/

#ifndef PENSTYLEBOX_H
#define PENSTYLEBOX_H

#include <QComboBox>

//! A modified QComboBox allowing to choose a Qt::PenStyle.
/**
 * This is a simple hack on top of the QComboBox class.
 */
class PenStyleBox : public QComboBox
{
  Q_OBJECT
	
public:
		//! Constructor.
		/**
		 * \param parent parent widget (only affects placement of the widget)
		 */
		PenStyleBox(QWidget *parent = 0);
  		void setStyle(const Qt::PenStyle& style);
  		Qt::PenStyle style() const;

  		static int styleIndex(const Qt::PenStyle& style);
  		static Qt::PenStyle penStyle(int index);

private:
  		static const Qt::PenStyle patterns[];
};

#endif
