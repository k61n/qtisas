/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plot symbol combo box
 ******************************************************************************/

#ifndef SYMBOLBOX_H
#define SYMBOLBOX_H

#include <QComboBox>
#include <qwt/qwt_symbol.h>

//! A modified QComboBox allowing to choose a QwtSmbol style.
/**
 * This is a simple hack on top of the QComboBox class.
 \image html images/symbol_box.png
 */
class SymbolBox : public QComboBox
{
  Q_OBJECT
public:
		//! Constructor.
		/**
		 * \param parent parent widget (only affects placement of the widget)
		 */
		SymbolBox(bool showNoSymbol = true, QWidget *parent = 0);

  		void setStyle(const QwtSymbol::Style& c);
 		QwtSymbol::Style selectedSymbol() const;

  		static QwtSymbol::Style style(int index);
  		static int symbolIndex(const QwtSymbol::Style& style);
		static QList<int> defaultSymbols();

signals:
		//! Signal emitted when the box gains focus
		void activated(SymbolBox *);

protected:
		void init(bool showNoSymbol);
		void focusInEvent(QFocusEvent *);

private:
		static const QwtSymbol::Style symbols[];
};

#endif
