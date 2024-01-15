/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tomomasa Ohkubo <>
Description: Pattern combobox
 ******************************************************************************/

#ifndef PATTERNBOX_H
#define PATTERNBOX_H

#include <QComboBox>

//! A modified QComboBox allowing to choose a Qt::BrushStyle style.
/**
 * This is a simple hack on top of the QComboBox class.
 */
class PatternBox : public QComboBox
{
  Q_OBJECT
	
public:
		//! Constructor.
		/**
		 * \param parent parent widget (only affects placement of the widget)
		 */
		PatternBox(QWidget *parent = 0);
		void setPattern(const Qt::BrushStyle& c);
		Qt::BrushStyle getSelectedPattern() const;

		static int patternIndex(const Qt::BrushStyle& style);
		static Qt::BrushStyle brushStyle(int index);

protected:
		void init();

private:
		static const Qt::BrushStyle patterns[];
};

#endif
