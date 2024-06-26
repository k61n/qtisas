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

#include <algorithm>

#include <QPainter>
#include <QPixmap>

#include "SymbolBox.h"

const QwtSymbol::Style SymbolBox::symbols[] = {
  QwtSymbol::NoSymbol,
  QwtSymbol::Ellipse,
  QwtSymbol::Rect,
  QwtSymbol::Diamond,
  QwtSymbol::Triangle,
  QwtSymbol::DTriangle,
  QwtSymbol::UTriangle,
  QwtSymbol::LTriangle,
  QwtSymbol::RTriangle,
  QwtSymbol::Cross,
  QwtSymbol::XCross,
  QwtSymbol::HLine,
  QwtSymbol::VLine,
  QwtSymbol::Star1,
  QwtSymbol::Star2,
  QwtSymbol::Hexagon
};

SymbolBox::SymbolBox(bool showNoSymbol, QWidget *parent) : QComboBox(parent)
{
	init(showNoSymbol);
}

void SymbolBox::init(bool showNoSymbol)
{
	QPixmap icon = QPixmap(15, 15);
	QColor c = QColor (Qt::gray);
	icon.fill(c);
	const QRect r = QRect(1, 1, 14, 14);
	QPainter p(&icon);
	p.setRenderHint(QPainter::Antialiasing);
	QwtSymbol symb;
	p.setBrush(QBrush(QColor(Qt::white)));

	if (showNoSymbol)
		this->addItem(tr("No Symbol" ));

	symb.setStyle (QwtSymbol::Ellipse);
	symb.draw(&p, r);
	this->addItem(icon, tr("Ellipse" ));

	symb.setStyle (QwtSymbol::Rect);
	icon.fill(c);
	symb.draw(&p, r.adjusted(0, 0, -1, -1));
	this->addItem(icon,tr("Rectangle"));

	symb.setStyle (QwtSymbol::Diamond);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Diamond"));

	symb.setStyle (QwtSymbol::Triangle);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Triangle"));

	symb.setStyle (QwtSymbol::DTriangle);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Down Triangle"));

	symb.setStyle (QwtSymbol::UTriangle);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Up Triangle"));

	symb.setStyle (QwtSymbol::LTriangle);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Left Triangle"));

	symb.setStyle (QwtSymbol::RTriangle);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Right Triangle"));

	symb.setStyle (QwtSymbol::Cross);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Cross"));

	symb.setStyle (QwtSymbol::XCross);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Diagonal Cross"));

	symb.setStyle (QwtSymbol::HLine);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Horizontal Line"));

	symb.setStyle (QwtSymbol::VLine);
	p.eraseRect ( r );
	symb.draw(&p, r);
	this->addItem(icon,tr("Vertical Line"));

	symb.setStyle (QwtSymbol::Star1);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Star 1"));

	symb.setStyle (QwtSymbol::Star2);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Star 2"));

	symb.setStyle (QwtSymbol::Hexagon);
	icon.fill(c);
	symb.draw(&p, r);
	this->addItem(icon,tr("Hexagon"));

	p.end();
}

void SymbolBox::setStyle(const QwtSymbol::Style& style)
{
	const QwtSymbol::Style*ite = std::find(symbols, symbols + sizeof(symbols), style);
	if (ite == symbols + sizeof(symbols))
		this->setCurrentIndex(0);
	else
		this->setCurrentIndex(ite - symbols);
}

QwtSymbol::Style SymbolBox::selectedSymbol() const
{
	size_t i = this->currentIndex();
	if (i < sizeof(symbols))
		return symbols[this->currentIndex()];

	return QwtSymbol::NoSymbol;
}

int SymbolBox::symbolIndex(const QwtSymbol::Style& style)
{
	const QwtSymbol::Style*ite = std::find(symbols, symbols + sizeof(symbols), style);
	if (ite == symbols + sizeof(symbols))
		return 0;

	return (ite - symbols);
}

QwtSymbol::Style SymbolBox::style(int index)
{
	if (index >= 0 && index < (int)sizeof(symbols))
		return symbols[index];

	return QwtSymbol::NoSymbol;
}

QList<int> SymbolBox::defaultSymbols()
{
	QList<int> lst;
    lst<<0<<1<<2<<4<<5<<6<<7;
    lst<<8<<9<<10<<11<<12<<13<<14;
    /*
	for (int i = 0; i < QwtSymbol::StyleCnt; i++)
		lst << i;
     */
	return lst;
}

void SymbolBox::focusInEvent(QFocusEvent * e)
{
	emit activated(this);
	return QComboBox::focusInEvent(e);
}
